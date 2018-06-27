#include "Math.hpp"
#include "ProcessAlpha.hpp"
#include "ProcessCommon.hpp"
#include "Tables.hpp"
#include "Vector.hpp"

static unsigned int Average1( const uint8_t* data )
{
    uint32_t a = 4;
    for( int i=0; i<8; i++ )
    {
        a += *data++;
    }
    return a / 8;
}

static void CalcErrorBlock( const uint8_t* data, unsigned int err[2] )
{
    for( int i=0; i<8; i++ )
    {
        unsigned int v = *data++;
        err[0] += v;
        err[1] += v*v;
    }
}

static unsigned int CalcError( const unsigned int block[2], unsigned int average )
{
    unsigned int err = block[1];
    err -= block[0] * 2 * average;
    err += 8 * sq( average );
    return err;
}

static void ProcessAverages( unsigned int* a )
{
    for( int i=0; i<2; i++ )
    {
        int c1 = mul8bit( a[i*2+1], 31 );
        int c2 = mul8bit( a[i*2], 31 );

        int diff = c2 - c1;
        if( diff > 3 ) diff = 3;
        else if( diff < -4 ) diff = -4;

        int co = c1 + diff;

        a[5+i*2] = ( c1 << 3 ) | ( c1 >> 2 );
        a[4+i*2] = ( co << 3 ) | ( co >> 2 );
    }
    for( int i=0; i<4; i++ )
    {
        a[i] = g_avg2[mul8bit( a[i], 15 )];
    }
}

static void EncodeAverages( uint64_t& _d, const unsigned int* a, size_t idx )
{
    auto d = _d;
    d |= ( idx << 24 );
    size_t base = idx << 1;

    unsigned int v;
    if( ( idx & 0x2 ) == 0 )
    {
        v = ( a[base+0] >> 4 ) | ( a[base+1] & 0xF0 );
    }
    else
    {
        v = a[base+1] & 0xF8;
        int32_t c = ( ( a[base+0] & 0xF8 ) - ( a[base+1] & 0xF8 ) ) >> 3;
        v |= c & ~0xFFFFFFF8;
    }
    d |= v | ( v << 8 ) | ( v << 16 );
    _d = d;
}

uint64_t ProcessAlpha( const uint8_t* src )
{
    uint64_t d = 0;

    {
        bool solid = true;
        const uint8_t* ptr = src + 1;
        for( int i=1; i<16; i++ )
        {
            if( *src != *ptr++ )
            {
                solid = false;
                break;
            }
        }
        if( solid )
        {
            unsigned int c = *src & 0xF8;
            d |= 0x02000000 | ( c << 16 ) | ( c << 8 ) | c;
            return d;
        }
    }

    uint8_t b23[2][8];
    const uint8_t* b[4] = { src+8, src, b23[0], b23[1] };

    for( int i=0; i<4; i++ )
    {
        *(b23[1]+i*2) = *(src+i*4);
        *(b23[0]+i*2) = *(src+i*4+3);
    }

    unsigned int a[8];
    for( int i=0; i<4; i++ )
    {
        a[i] = Average1( b[i] );
    }
    ProcessAverages( a );

    unsigned int err[4] = {};
    for( int i=0; i<4; i++ )
    {
        unsigned int errblock[2] = {};
        CalcErrorBlock( b[i], errblock );
        err[i/2] += CalcError( errblock, a[i] );
        err[2+i/2] += CalcError( errblock, a[i+4] );
    }
    size_t idx = GetLeastError( err, 4 );

    EncodeAverages( d, a, idx );

    unsigned int terr[2][8] = {};
    uint16_t tsel[16][8];
    auto id = g_id[idx];
    const uint8_t* data = src;
    for( size_t i=0; i<16; i++ )
    {
        uint16_t* sel = tsel[i];
        unsigned int bid = id[i];
        unsigned int* ter = terr[bid%2];

        uint8_t c = *data++;
#ifdef __SSE4_1__
        __m128i pix = _mm_set1_epi16(a[bid] - c);
        // Taking the absolute value is way faster. The values are only used to sort, so the result will be the same.
        __m128i error0 = _mm_abs_epi16(_mm_add_epi16(pix, g_table_SIMD[0]));
        __m128i error1 = _mm_abs_epi16(_mm_add_epi16(pix, g_table_SIMD[1]));
        __m128i error2 = _mm_abs_epi16(_mm_sub_epi16(pix, g_table_SIMD[0]));
        __m128i error3 = _mm_abs_epi16(_mm_sub_epi16(pix, g_table_SIMD[1]));

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

        squareErrorLow = _mm_add_epi32(squareErrorLow, _mm_lddqu_si128(((__m128i*)ter) + 0));
        _mm_storeu_si128(((__m128i*)ter) + 0, squareErrorLow);
        squareErrorHigh = _mm_add_epi32(squareErrorHigh, _mm_lddqu_si128(((__m128i*)ter) + 1));
        _mm_storeu_si128(((__m128i*)ter) + 1, squareErrorHigh);

        _mm_storeu_si128((__m128i*)sel, minIndex);
#else
        int32_t pix = a[bid] - c;

        for( int t=0; t<8; t++ )
        {
            const int32_t* tab = g_table[t];
            unsigned int idx = 0;
            unsigned int err = sq( tab[0] + pix );
            for( int j=1; j<4; j++ )
            {
                unsigned int local = sq( tab[j] + pix );
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

    return FixByteOrder( EncodeSelectors( d, terr, tsel, id ) );
}
