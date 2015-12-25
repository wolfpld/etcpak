#include <array>
#include <string.h>

#include "Math.hpp"
#include "ProcessCommon.hpp"
#include "ProcessRGB_AVX2.hpp"
#include "Tables.hpp"
#include "Types.hpp"
#include "Vector.hpp"
#ifdef _MSC_VER
#  include <intrin.h>
#  include <Windows.h>
#else
#  include <x86intrin.h>
#endif

#pragma GCC push_options
#pragma GCC target ("avx2")

namespace
{

typedef std::array<uint16, 4> v4i;

void Average_AVX2( const uint8* data, v4i* a )
{
    __m128i d0 = _mm_loadu_si128(((__m128i*)data) + 0);
    __m128i d1 = _mm_loadu_si128(((__m128i*)data) + 1);
    __m128i d2 = _mm_loadu_si128(((__m128i*)data) + 2);
    __m128i d3 = _mm_loadu_si128(((__m128i*)data) + 3);

    __m256i t0 = _mm256_cvtepu8_epi16(d0);
    __m256i t1 = _mm256_cvtepu8_epi16(d1);
    __m256i t2 = _mm256_cvtepu8_epi16(d2);
    __m256i t3 = _mm256_cvtepu8_epi16(d3);

    __m256i sum0 = _mm256_add_epi16(t0, t1);
    __m256i sum1 = _mm256_add_epi16(t2, t3);

    __m256i sum0l = _mm256_unpacklo_epi16(sum0, _mm256_setzero_si256());
    __m256i sum0h = _mm256_unpackhi_epi16(sum0, _mm256_setzero_si256());
    __m256i sum1l = _mm256_unpacklo_epi16(sum1, _mm256_setzero_si256());
    __m256i sum1h = _mm256_unpackhi_epi16(sum1, _mm256_setzero_si256());

    __m256i b0 = _mm256_add_epi32(sum0l, sum0h);
    __m256i b1 = _mm256_add_epi32(sum1l, sum1h);

    __m256i t4 = _mm256_add_epi32(b0, b1);
    __m256i t5 = _mm256_permute2x128_si256(t4, t4, (1) | (0 << 4));
    __m256i t6 = _mm256_permute2x128_si256(b0, b1, (2) | (0 << 4));
    __m256i t7 = _mm256_permute2x128_si256(b0, b1, (3) | (1 << 4));
    __m256i t8 = _mm256_add_epi32(t6, t7);

    __m256i v0 = _mm256_add_epi32(t5, _mm256_set1_epi32(4));
    __m256i v1 = _mm256_add_epi32(t8, _mm256_set1_epi32(4));

    __m256i a0 = _mm256_shuffle_epi32(_mm256_srli_epi32(v0, 3), _MM_SHUFFLE(3, 0, 1, 2));
    __m256i a1 = _mm256_shuffle_epi32(_mm256_srli_epi32(v1, 3), _MM_SHUFFLE(3, 0, 1, 2));

    __m256i a2 = _mm256_packus_epi32(a0, a1);
    __m256i a3 = _mm256_permute4x64_epi64(a2, _MM_SHUFFLE(2, 0, 3, 1));

    _mm256_storeu_si256((__m256i*)&a[0], a3);
}

void CalcErrorBlock_AVX2( const uint8* data, uint err[4][4] )
{
    __m128i d0 = _mm_loadu_si128(((__m128i*)data) + 0);
    __m128i d1 = _mm_loadu_si128(((__m128i*)data) + 1);
    __m128i d2 = _mm_loadu_si128(((__m128i*)data) + 2);
    __m128i d3 = _mm_loadu_si128(((__m128i*)data) + 3);

    __m128i dm0 = _mm_and_si128(d0, _mm_setr_epi8(-1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0));
    __m128i dm1 = _mm_and_si128(d1, _mm_setr_epi8(-1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0));
    __m128i dm2 = _mm_and_si128(d2, _mm_setr_epi8(-1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0));
    __m128i dm3 = _mm_and_si128(d3, _mm_setr_epi8(-1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0));

    __m256i t0 = _mm256_cvtepu8_epi16(dm0);
    __m256i t1 = _mm256_cvtepu8_epi16(dm1);
    __m256i t2 = _mm256_cvtepu8_epi16(dm2);
    __m256i t3 = _mm256_cvtepu8_epi16(dm3);

    __m256i sqrSum0, sqrSum1;

    {
        __m256i sum0 = _mm256_madd_epi16(t0, t0);
        __m256i sum1 = _mm256_madd_epi16(t1, t1);
        __m256i sum2 = _mm256_madd_epi16(t2, t2);
        __m256i sum3 = _mm256_madd_epi16(t3, t3);

        __m256i sum4 = _mm256_add_epi32(sum0, sum1);
        __m256i sum5 = _mm256_add_epi32(sum2, sum3);

        __m256i sum6 = _mm256_hadd_epi32(sum4, sum5);

        __m256i sum7 = _mm256_hadd_epi32(sum6, sum6);

		__m256i sum8 = _mm256_permute4x64_epi64(sum7, _MM_SHUFFLE(3, 1, 2, 0));

        __m256i t = _mm256_shuffle_epi32(sum8, _MM_SHUFFLE(2, 0, 3, 1));

        __m256i sum = _mm256_add_epi32(t, sum8);

		sqrSum0 = _mm256_permutevar8x32_epi32(sum, _mm256_setr_epi32(3, 3, 3, 3, 0, 0, 0, 0));
		sqrSum1 = _mm256_permutevar8x32_epi32(sum, _mm256_setr_epi32(1, 1, 1, 1, 2, 2, 2, 2));

        sqrSum0 = _mm256_and_si256(sqrSum0, _mm256_setr_epi32(0, 0, 0, -1, 0, 0, 0, -1));
        sqrSum1 = _mm256_and_si256(sqrSum1, _mm256_setr_epi32(0, 0, 0, -1, 0, 0, 0, -1));
    }

    __m256i sum0 = _mm256_add_epi16(t0, t1);
    __m256i sum1 = _mm256_add_epi16(t2, t3);

    __m256i sum0l = _mm256_unpacklo_epi16(sum0, _mm256_setzero_si256());
    __m256i sum0h = _mm256_unpackhi_epi16(sum0, _mm256_setzero_si256());
    __m256i sum1l = _mm256_unpacklo_epi16(sum1, _mm256_setzero_si256());
    __m256i sum1h = _mm256_unpackhi_epi16(sum1, _mm256_setzero_si256());

    __m256i b0 = _mm256_add_epi32(sum0l, sum0h);
    __m256i b1 = _mm256_add_epi32(sum1l, sum1h);

    __m256i t4 = _mm256_add_epi32(b0, b1);
    __m256i t5 = _mm256_permute2x128_si256(t4, t4, (1) | (0 << 4));
    __m256i t6 = _mm256_permute2x128_si256(b0, b1, (2) | (0 << 4));
    __m256i t7 = _mm256_permute2x128_si256(b0, b1, (3) | (1 << 4));
    __m256i t8 = _mm256_add_epi32(t6, t7);

    __m256i a0 = _mm256_or_si256(t5, sqrSum0);
    __m256i a1 = _mm256_or_si256(t8, sqrSum1);

    _mm256_storeu_si256((__m256i*)&err[0], a1);
    _mm256_storeu_si256((__m256i*)&err[2], a0);
}

uint CalcError( const uint block[4], const v4i& average )
{
    uint err = block[3];
    err -= block[0] * 2 * average[2];
    err -= block[1] * 2 * average[1];
    err -= block[2] * 2 * average[0];
    err += 8 * ( sq( average[0] ) + sq( average[1] ) + sq( average[2] ) );
    return err;
}

void ProcessAverages_AVX2( v4i* a )
{
    {
        __m256i d = _mm256_loadu_si256((__m256i*)a[0].data());

        __m256i t = _mm256_add_epi16(_mm256_mullo_epi16(d, _mm256_set1_epi16(31)), _mm256_set1_epi16(128));

        __m256i c = _mm256_srli_epi16(_mm256_add_epi16(t, _mm256_srli_epi16(t, 8)), 8);

        __m256i c1 = _mm256_shuffle_epi32(c, _MM_SHUFFLE(3, 2, 3, 2));
        __m256i diff = _mm256_sub_epi16(c, c1);
        diff = _mm256_max_epi16(diff, _mm256_set1_epi16(-4));
        diff = _mm256_min_epi16(diff, _mm256_set1_epi16(3));

        __m256i co = _mm256_add_epi16(c1, diff);

        c = _mm256_blend_epi16(co, c, 0xF0);

        __m256i a0 = _mm256_or_si256(_mm256_slli_epi16(c, 3), _mm256_srli_epi16(c, 2));

        _mm256_storeu_si256((__m256i*)a[4].data(), a0);
    }

    {
        __m256i d = _mm256_loadu_si256((__m256i*)a[0].data());

        __m256i t0 = _mm256_add_epi16(_mm256_mullo_epi16(d, _mm256_set1_epi16(15)), _mm256_set1_epi16(128));
        __m256i t1 = _mm256_srli_epi16(_mm256_add_epi16(t0, _mm256_srli_epi16(t0, 8)), 8);

        __m256i t2 = _mm256_or_si256(t1, _mm256_slli_epi16(t1, 4));

        _mm256_storeu_si256((__m256i*)a[0].data(), t2);
    }
}

void EncodeAverages( uint64& _d, const v4i* a, size_t idx )
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

uint64 CheckSolid_AVX2( const uint8* src )
{
    __m256i d0 = _mm256_loadu_si256(((__m256i*)src) + 0);
    __m256i d1 = _mm256_loadu_si256(((__m256i*)src) + 1);

    __m256i c = _mm256_broadcastd_epi32(_mm256_castsi256_si128(d0));

    __m256i c0 = _mm256_cmpeq_epi8(d0, c);
    __m256i c1 = _mm256_cmpeq_epi8(d1, c);

    __m256i m = _mm256_and_si256(c0, c1);

    if (!_mm256_testc_si256(m, _mm256_set1_epi32(-1)))
    {
        return 0;
    }

    return 0x02000000 |
        ( uint( src[0] & 0xF8 ) << 16 ) |
        ( uint( src[1] & 0xF8 ) << 8 ) |
        ( uint( src[2] & 0xF8 ) );
}

void PrepareAverages_AVX2( v4i a[8], const uint8* src, uint err[4] )
{
    Average_AVX2( src, a );
    ProcessAverages_AVX2( a );

    uint errblock[4][4];
    CalcErrorBlock_AVX2( src, errblock );

    for( int i=0; i<4; i++ )
    {
        err[i/2] += CalcError( errblock[i], a[i] );
        err[2+i/2] += CalcError( errblock[i], a[i+4] );
    }
}

void FindBestFit_AVX2( uint32 terr[2][8], uint16 tsel[16][8], v4i a[8], const uint32* id, const uint8* data )
{
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

        int dr0 = a[bid][0] - r0;
        int dg0 = a[bid][1] - g0;
        int db0 = a[bid][2] - b0;

        int dr1 = a[bid][0] - r1;
        int dg1 = a[bid][1] - g1;
        int db1 = a[bid][2] - b1;

        // The scaling values are divided by two and rounded, to allow the differences to be in the range of signed int16
        // This produces slightly different results, but is significant faster
        int pixel0 = dr0 * 38 + dg0 * 76 + db0 * 14;
        int pixel1 = dr1 * 38 + dg1 * 76 + db1 * 14;

        __m256i pix0 = _mm256_set1_epi16(pixel0);
        __m128i pix1 = _mm_set1_epi16(pixel1);
        __m256i pixel = _mm256_insertf128_si256(pix0, pix1, 1);
        __m256i pix = _mm256_abs_epi16(pixel);

        // Taking the absolute value is way faster. The values are only used to sort, so the result will be the same.
        // Since the selector table is symmetrical, we need to calculate the difference only for half of the entries.
        __m256i error0 = _mm256_abs_epi16(_mm256_sub_epi16(pix, _mm256_broadcastsi128_si256(g_table128_SIMD[0])));
        __m256i error1 = _mm256_abs_epi16(_mm256_sub_epi16(pix, _mm256_broadcastsi128_si256(g_table128_SIMD[1])));

        __m256i index = _mm256_and_si256(_mm256_cmpgt_epi16(error0, error1), _mm256_set1_epi16(1));
        __m256i minError = _mm256_min_epi16(error0, error1);

        // Exploiting symmetry of the selector table
        __m256i minIndex = _mm256_or_si256(index, _mm256_and_si256(_mm256_set1_epi16(2), _mm256_cmpgt_epi16(pixel, _mm256_setzero_si256())));

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
}
}

uint64 ProcessRGB_AVX2( const uint8* src )
{
    uint64 d = CheckSolid_AVX2( src );
    if( d != 0 ) return d;

    v4i a[8];
    uint err[4] = {};
    PrepareAverages_AVX2( a, src, err );
    size_t idx = GetLeastError( err, 4 );
    EncodeAverages( d, a, idx );

    uint32 terr[2][8] = {};
    uint16 tsel[16][8];
    auto id = g_id[idx];
    FindBestFit_AVX2( terr, tsel, a, id, src );

    return FixByteOrder( EncodeSelectors( d, terr, tsel, id ) );
}

#pragma GCC pop_options

