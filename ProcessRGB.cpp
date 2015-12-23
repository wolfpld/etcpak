#include <string.h>

#include "Math.hpp"
#include "ProcessCommon.hpp"
#include "ProcessRGB.hpp"
#include "Tables.hpp"
#include "Types.hpp"
#include "Vector.hpp"
#ifdef __AVX2__
#ifdef _MSC_VER
#include <intrin.h>
#include <Windows.h>
#else
#include <x86intrin.h>
#endif
#endif

static v3i Average( const uint8* data )
{
    uint32 r = 0, g = 0, b = 0;
    for( int i=0; i<8; i++ )
    {
        b += *data++;
        g += *data++;
        r += *data++;
        data++;
    }
    return v3i( r / 8, g / 8, b / 8 );
}

static void CalcErrorBlock( const uint8* data, uint err[4] )
{
    for( int i=0; i<8; i++ )
    {
        uint d = *data++;
        err[0] += d;
        err[3] += d*d;
        d = *data++;
        err[1] += d;
        err[3] += d*d;
        d = *data++;
        err[2] += d;
        err[3] += d*d;
        data++;
    }
}

static uint CalcError( const uint block[4], const v3i& average )
{
    uint err = block[3];
    err -= block[0] * 2 * average.z;
    err -= block[1] * 2 * average.y;
    err -= block[2] * 2 * average.x;
    err += 8 * ( sq( average.x ) + sq( average.y ) + sq( average.z ) );
    return err;
}

static void ProcessAverages( v3i* a )
{
    for( int i=0; i<2; i++ )
    {
        for( int j=0; j<3; j++ )
        {
            int32 c1 = mul8bit( a[i*2+1][j], 31 );
            int32 c2 = mul8bit( a[i*2][j], 31 );

            int32 diff = c2 - c1;
            if( diff > 3 ) diff = 3;
            else if( diff < -4 ) diff = -4;

            int32 co = c1 + diff;

            a[5+i*2][j] = ( c1 << 3 ) | ( c1 >> 2 );
            a[4+i*2][j] = ( co << 3 ) | ( co >> 2 );
        }
    }
    for( int i=0; i<4; i++ )
    {
        a[i].x = g_avg2[mul8bit( a[i].x, 15 )];
        a[i].y = g_avg2[mul8bit( a[i].y, 15 )];
        a[i].z = g_avg2[mul8bit( a[i].z, 15 )];
    }
}

static void EncodeAverages( uint64& _d, const v3i* a, size_t idx )
{
    auto d = _d;
    d |= ( idx << 24 );
    size_t base = idx << 1;

    if( ( idx & 0x2 ) == 0 )
    {
        for( int i=0; i<3; i++ )
        {
            d |= uint64( a[base+0][i] >> 4 ) << ( i*8 );
            d |= uint64( a[base+1][i] >> 4 ) << ( i*8 + 4 );
        }
    }
    else
    {
        for( int i=0; i<3; i++ )
        {
            d |= uint64( a[base+1][i] & 0xF8 ) << ( i*8 );
            int32 c = ( ( a[base+0][i] & 0xF8 ) - ( a[base+1][i] & 0xF8 ) ) >> 3;
            c &= ~0xFFFFFFF8;
            d |= ((uint64)c) << ( i*8 );
        }
    }
    _d = d;
}

static uint64 CheckSolid( const uint8* src )
{
    const uint8* ptr = src + 4;
    for( int i=1; i<16; i++ )
    {
        if( memcmp( src, ptr, 4 ) != 0 )
        {
            return 0;
        }
        ptr += 4;
    }
    return 0x02000000 |
        ( uint( src[0] & 0xF8 ) << 16 ) |
        ( uint( src[1] & 0xF8 ) << 8 ) |
        ( uint( src[2] & 0xF8 ) );
}

static void PrepareBuffers( uint8 b23[2][32], const uint8* src )
{
    for( int i=0; i<4; i++ )
    {
        memcpy( b23[1]+i*8, src+i*16, 8 );
        memcpy( b23[0]+i*8, src+i*16+8, 8 );
    }
}

static void PrepareAverages( v3i a[8], const uint8* b[4], uint err[4] )
{
    for( int i=0; i<4; i++ )
    {
        a[i] = Average( b[i] );
    }
    ProcessAverages( a );

    for( int i=0; i<4; i++ )
    {
        uint errblock[4] = {};
        CalcErrorBlock( b[i], errblock );
        err[i/2] += CalcError( errblock, a[i] );
        err[2+i/2] += CalcError( errblock, a[i+4] );
    }
}

static void FindBestFit( uint64 terr[2][8], uint16 tsel[16][8], v3i a[8], const uint32* id, const uint8* data )
{
    for( size_t i=0; i<16; i++ )
    {
        uint16* sel = tsel[i];
        uint bid = id[i];
        uint64* ter = terr[bid%2];

        uint8 b = *data++;
        uint8 g = *data++;
        uint8 r = *data++;
        data++;

        int dr = a[bid].x - r;
        int dg = a[bid].y - g;
        int db = a[bid].z - b;

#ifdef __SSE4_1__
        __m128i pix = _mm_set1_epi32(dr * 77 + dg * 151 + db * 28);
        // Taking the absolute value is way faster. The values are only used to sort, so the result will be the same.
        __m128i error0 = _mm_abs_epi32(_mm_add_epi32(pix, g_table256_SIMD[0]));
        __m128i error1 = _mm_abs_epi32(_mm_add_epi32(pix, g_table256_SIMD[1]));
        __m128i error2 = _mm_abs_epi32(_mm_sub_epi32(pix, g_table256_SIMD[0]));
        __m128i error3 = _mm_abs_epi32(_mm_sub_epi32(pix, g_table256_SIMD[1]));

        __m128i index0 = _mm_and_si128(_mm_cmplt_epi32(error1, error0), _mm_set1_epi32(1));
        __m128i minError0 = _mm_min_epi32(error0, error1);

        __m128i index1 = _mm_sub_epi32(_mm_set1_epi32(2), _mm_cmplt_epi32(error3, error2));
        __m128i minError1 = _mm_min_epi32(error2, error3);

        __m128i minIndex0 = _mm_blendv_epi8(index0, index1, _mm_cmplt_epi32(minError1, minError0));
        __m128i minError = _mm_min_epi32(minError0, minError1);

        // Squaring the minimum error to produce correct values when adding
        __m128i minErrorLow = _mm_shuffle_epi32(minError, _MM_SHUFFLE(1, 1, 0, 0));
        __m128i squareErrorLow = _mm_mul_epi32(minErrorLow, minErrorLow);
        squareErrorLow = _mm_add_epi64(squareErrorLow, _mm_loadu_si128(((__m128i*)ter) + 0));
        _mm_storeu_si128(((__m128i*)ter) + 0, squareErrorLow);
        __m128i minErrorHigh = _mm_shuffle_epi32(minError, _MM_SHUFFLE(3, 3, 2, 2));
        __m128i squareErrorHigh = _mm_mul_epi32(minErrorHigh, minErrorHigh);
        squareErrorHigh = _mm_add_epi64(squareErrorHigh, _mm_loadu_si128(((__m128i*)ter) + 1));
        _mm_storeu_si128(((__m128i*)ter) + 1, squareErrorHigh);

        // Taking the absolute value is way faster. The values are only used to sort, so the result will be the same.
        error0 = _mm_abs_epi32(_mm_add_epi32(pix, g_table256_SIMD[2]));
        error1 = _mm_abs_epi32(_mm_add_epi32(pix, g_table256_SIMD[3]));
        error2 = _mm_abs_epi32(_mm_sub_epi32(pix, g_table256_SIMD[2]));
        error3 = _mm_abs_epi32(_mm_sub_epi32(pix, g_table256_SIMD[3]));

        index0 = _mm_and_si128(_mm_cmplt_epi32(error1, error0), _mm_set1_epi32(1));
        minError0 = _mm_min_epi32(error0, error1);

        index1 = _mm_sub_epi32(_mm_set1_epi32(2), _mm_cmplt_epi32(error3, error2));
        minError1 = _mm_min_epi32(error2, error3);

        __m128i minIndex1 = _mm_blendv_epi8(index0, index1, _mm_cmplt_epi32(minError1, minError0));
        minError = _mm_min_epi32(minError0, minError1);

        // Squaring the minimum error to produce correct values when adding
        minErrorLow = _mm_shuffle_epi32(minError, _MM_SHUFFLE(1, 1, 0, 0));
        squareErrorLow = _mm_mul_epi32(minErrorLow, minErrorLow);
        squareErrorLow = _mm_add_epi64(squareErrorLow, _mm_loadu_si128(((__m128i*)ter) + 2));
        _mm_storeu_si128(((__m128i*)ter) + 2, squareErrorLow);
        minErrorHigh = _mm_shuffle_epi32(minError, _MM_SHUFFLE(3, 3, 2, 2));
        squareErrorHigh = _mm_mul_epi32(minErrorHigh, minErrorHigh);
        squareErrorHigh = _mm_add_epi64(squareErrorHigh, _mm_loadu_si128(((__m128i*)ter) + 3));
        _mm_storeu_si128(((__m128i*)ter) + 3, squareErrorHigh);
        __m128i minIndex = _mm_packs_epi32(minIndex0, minIndex1);
        _mm_storeu_si128((__m128i*)sel, minIndex);
#else
        int pix = dr * 77 + dg * 151 + db * 28;

        for( int t=0; t<8; t++ )
        {
            const int64* tab = g_table256[t];
            uint idx = 0;
            uint64 err = sq( tab[0] + pix );
            for( int j=1; j<4; j++ )
            {
                uint64 local = sq( tab[j] + pix );
                if( local < err )
                {
                    err = local;
                    idx = j;
                }
            }
            *sel++ = idx;
            *ter++ += err;
        }
#endif
    }
}

#ifdef __SSE4_1__
static void FindBestFit( uint32 terr[2][8], uint16 tsel[16][8], v3i a[8], const uint32* id, const uint8* data )
{
#ifdef __AVX2__
    for( size_t i=0; i<16; i+=2 )
    {
        uint16* sel = tsel[i];
        uint bid = id[i];
        uint32* ter = terr[bid%2];

        uint8 b0 = *data++;
        uint8 g0 = *data++;
        uint8 r0 = *data++;
        data++;

        uint8 b1 = *data++;
        uint8 g1 = *data++;
        uint8 r1 = *data++;
        data++;

        int dr0 = a[bid].x - r0;
        int dg0 = a[bid].y - g0;
        int db0 = a[bid].z - b0;

        int dr1 = a[bid].x - r1;
        int dg1 = a[bid].y - g1;
        int db1 = a[bid].z - b1;

        // The scaling values are divided by four and rounded, to allow the differences to be in the range of signed int16
        // This produces slightly different results, but is significant faster
        int pixel0 = dr0 * 19 + dg0 * 38 + db0 * 7;
        int pixel1 = dr1 * 19 + dg1 * 38 + db1 * 7;

        __m256i pix0 = _mm256_set1_epi16(pixel0);
        __m128i pix1 = _mm_set1_epi16(pixel1);
        __m256i pix = _mm256_insertf128_si256(pix0, pix1, 1);

        // Taking the absolute value is way faster. The values are only used to sort, so the result will be the same.
        __m256i error0 = _mm256_abs_epi16(_mm256_add_epi16(pix, _mm256_broadcastsi128_si256(g_table64_SIMD[0])));
        __m256i error1 = _mm256_abs_epi16(_mm256_add_epi16(pix, _mm256_broadcastsi128_si256(g_table64_SIMD[1])));
        __m256i error2 = _mm256_abs_epi16(_mm256_sub_epi16(pix, _mm256_broadcastsi128_si256(g_table64_SIMD[0])));
        __m256i error3 = _mm256_abs_epi16(_mm256_sub_epi16(pix, _mm256_broadcastsi128_si256(g_table64_SIMD[1])));

        __m256i index0 = _mm256_and_si256(_mm256_cmpgt_epi16(error0, error1), _mm256_set1_epi16(1));
        __m256i minError0 = _mm256_min_epi16(error0, error1);

        __m256i index1 = _mm256_sub_epi16(_mm256_set1_epi16(2), _mm256_cmpgt_epi16(error2, error3));
        __m256i minError1 = _mm256_min_epi16(error2, error3);

        __m256i minIndex = _mm256_blendv_epi8(index0, index1, _mm256_cmpgt_epi16(minError0, minError1));
        __m256i minError = _mm256_min_epi16(minError0, minError1);

        // Interleaving values so madd instruction can be used
        __m256i minErrorLo = _mm256_permute4x64_epi64(minError, _MM_SHUFFLE(1, 1, 0, 0));
        __m256i minErrorHi = _mm256_permute4x64_epi64(minError, _MM_SHUFFLE(3, 3, 2, 2));

        __m256i minError2 = _mm256_unpacklo_epi16(minErrorLo, minErrorHi);
        // Squaring the minimum error to produce correct values when adding
        __m256i squareErrorSum = _mm256_madd_epi16(minError2, minError2);

        __m256i squareError = _mm256_add_epi32(squareErrorSum, _mm256_loadu_si256((__m256i*)ter));

        _mm256_storeu_si256((__m256i*)ter, squareError);
        _mm256_storeu_si256((__m256i*)sel, minIndex);
    }
#else
    for( size_t i=0; i<16; i++ )
    {
        uint16* sel = tsel[i];
        uint bid = id[i];
        uint32* ter = terr[bid%2];

        uint8 b = *data++;
        uint8 g = *data++;
        uint8 r = *data++;
        data++;

        int dr = a[bid].x - r;
        int dg = a[bid].y - g;
        int db = a[bid].z - b;

        // The scaling values are divided by four and rounded, to allow the differences to be in the range of signed int16
        // This produces slightly different results, but is significant faster
        int pixel = dr * 19 + dg * 38 + db * 7;

        __m128i pix = _mm_set1_epi16(pixel);

        // Taking the absolute value is way faster. The values are only used to sort, so the result will be the same.
        __m128i error0 = _mm_abs_epi16(_mm_add_epi16(pix, g_table64_SIMD[0]));
        __m128i error1 = _mm_abs_epi16(_mm_add_epi16(pix, g_table64_SIMD[1]));
        __m128i error2 = _mm_abs_epi16(_mm_sub_epi16(pix, g_table64_SIMD[0]));
        __m128i error3 = _mm_abs_epi16(_mm_sub_epi16(pix, g_table64_SIMD[1]));

        __m128i index0 = _mm_and_si128(_mm_cmplt_epi16(error1, error0), _mm_set1_epi16(1));
        __m128i minError0 = _mm_min_epi16(error0, error1);

        __m128i index1 = _mm_sub_epi16(_mm_set1_epi16(2), _mm_cmplt_epi16(error3, error2));
        __m128i minError1 = _mm_min_epi16(error2, error3);

        __m128i minIndex = _mm_blendv_epi8(index0, index1, _mm_cmplt_epi16(minError1, minError0));
        __m128i minError = _mm_min_epi16(minError0, minError1);

        // Squaring the minimum error to produce correct values when adding
        __m128i squareErrorLo = _mm_mullo_epi16(minError, minError);
        __m128i squareErrorHi = _mm_mulhi_epi16(minError, minError);

        __m128i squareErrorLow = _mm_unpacklo_epi16(squareErrorLo, squareErrorHi);
        __m128i squareErrorHigh = _mm_unpackhi_epi16(squareErrorLo, squareErrorHi);

        squareErrorLow = _mm_add_epi32(squareErrorLow, _mm_loadu_si128(((__m128i*)ter) + 0));
        _mm_storeu_si128(((__m128i*)ter) + 0, squareErrorLow);
        squareErrorHigh = _mm_add_epi32(squareErrorHigh, _mm_loadu_si128(((__m128i*)ter) + 1));
        _mm_storeu_si128(((__m128i*)ter) + 1, squareErrorHigh);

        _mm_storeu_si128((__m128i*)sel, minIndex);
    }
#endif
}
#endif

uint64 ProcessRGB( const uint8* src )
{
    uint64 d = CheckSolid( src );
    if( d != 0 ) return d;

    uint8 b23[2][32];
    const uint8* b[4] = { src+32, src, b23[0], b23[1] };
    PrepareBuffers( b23, src );

    v3i a[8];
    uint err[4] = {};
    PrepareAverages( a, b, err );
    size_t idx = GetLeastError( err, 4 );
    EncodeAverages( d, a, idx );

#ifdef __SSE4_1__
    uint32 terr[2][8] = {};
#else
    uint64 terr[2][8] = {};
#endif
    uint16 tsel[16][8];
    auto id = g_id[idx];
    FindBestFit( terr, tsel, a, id, src );

    return FixByteOrder( EncodeSelectors( d, terr, tsel, id ) );
}
