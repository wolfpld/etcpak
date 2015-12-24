#include <array>
#include <string.h>

#include "Math.hpp"
#include "ProcessCommon.hpp"
#include "ProcessRGB.hpp"
#include "Tables.hpp"
#include "Types.hpp"
#include "Vector.hpp"
#ifdef _MSC_VER
#  include <intrin.h>
#  include <Windows.h>
#else
#  include <x86intrin.h>
#endif

typedef std::array<uint16, 4> v4i;

static void Average( const uint8* data, v4i* a )
{
#ifdef __SSE4_1__
    __m128i d0 = _mm_loadu_si128(((__m128i*)data) + 0);
    __m128i d1 = _mm_loadu_si128(((__m128i*)data) + 1);
    __m128i d2 = _mm_loadu_si128(((__m128i*)data) + 2);
    __m128i d3 = _mm_loadu_si128(((__m128i*)data) + 3);

    __m128i d0l = _mm_unpacklo_epi8(d0, _mm_setzero_si128());
    __m128i d0h = _mm_unpackhi_epi8(d0, _mm_setzero_si128());
    __m128i d1l = _mm_unpacklo_epi8(d1, _mm_setzero_si128());
    __m128i d1h = _mm_unpackhi_epi8(d1, _mm_setzero_si128());
    __m128i d2l = _mm_unpacklo_epi8(d2, _mm_setzero_si128());
    __m128i d2h = _mm_unpackhi_epi8(d2, _mm_setzero_si128());
    __m128i d3l = _mm_unpacklo_epi8(d3, _mm_setzero_si128());
    __m128i d3h = _mm_unpackhi_epi8(d3, _mm_setzero_si128());

    __m128i sum0 = _mm_add_epi16(d0l, d1l);
    __m128i sum1 = _mm_add_epi16(d0h, d1h);
    __m128i sum2 = _mm_add_epi16(d2l, d3l);
    __m128i sum3 = _mm_add_epi16(d2h, d3h);

    __m128i sum0l = _mm_unpacklo_epi16(sum0, _mm_setzero_si128());
    __m128i sum0h = _mm_unpackhi_epi16(sum0, _mm_setzero_si128());
    __m128i sum1l = _mm_unpacklo_epi16(sum1, _mm_setzero_si128());
    __m128i sum1h = _mm_unpackhi_epi16(sum1, _mm_setzero_si128());
    __m128i sum2l = _mm_unpacklo_epi16(sum2, _mm_setzero_si128());
    __m128i sum2h = _mm_unpackhi_epi16(sum2, _mm_setzero_si128());
    __m128i sum3l = _mm_unpacklo_epi16(sum3, _mm_setzero_si128());
    __m128i sum3h = _mm_unpackhi_epi16(sum3, _mm_setzero_si128());

    __m128i b0 = _mm_add_epi32(sum0l, sum0h);
    __m128i b1 = _mm_add_epi32(sum1l, sum1h);
    __m128i b2 = _mm_add_epi32(sum2l, sum2h);
    __m128i b3 = _mm_add_epi32(sum3l, sum3h);

    __m128i a0 = _mm_srli_epi32(_mm_add_epi32(_mm_add_epi32(b2, b3), _mm_set1_epi32(4)), 3);
    __m128i a1 = _mm_srli_epi32(_mm_add_epi32(_mm_add_epi32(b0, b1), _mm_set1_epi32(4)), 3);
    __m128i a2 = _mm_srli_epi32(_mm_add_epi32(_mm_add_epi32(b1, b3), _mm_set1_epi32(4)), 3);
    __m128i a3 = _mm_srli_epi32(_mm_add_epi32(_mm_add_epi32(b0, b2), _mm_set1_epi32(4)), 3);

    _mm_storeu_si128((__m128i*)&a[0], _mm_packus_epi32(_mm_shuffle_epi32(a0, _MM_SHUFFLE(3, 0, 1, 2)), _mm_shuffle_epi32(a1, _MM_SHUFFLE(3, 0, 1, 2))));
    _mm_storeu_si128((__m128i*)&a[2], _mm_packus_epi32(_mm_shuffle_epi32(a2, _MM_SHUFFLE(3, 0, 1, 2)), _mm_shuffle_epi32(a3, _MM_SHUFFLE(3, 0, 1, 2))));
#else
    uint32 r[4];
    uint32 g[4];
    uint32 b[4];

    memset(r, 0, sizeof(r));
    memset(g, 0, sizeof(g));
    memset(b, 0, sizeof(b));

    for( int j=0; j<4; j++ )
    {
        for( int i=0; i<4; i++ )
        {
            int index = (j & 2) + (i >> 1);
            b[index] += *data++;
            g[index] += *data++;
            r[index] += *data++;
            data++;
        }
    }

    a[0] = v4i{ (r[2] + r[3] + 4) / 8, (g[2] + g[3] + 4) / 8, (b[2] + b[3] + 4) / 8, 0};
    a[1] = v4i{ (r[0] + r[1] + 4) / 8, (g[0] + g[1] + 4) / 8, (b[0] + b[1] + 4) / 8, 0};
    a[2] = v4i{ (r[1] + r[3] + 4) / 8, (g[1] + g[3] + 4) / 8, (b[1] + b[3] + 4) / 8, 0};
    a[3] = v4i{ (r[0] + r[2] + 4) / 8, (g[0] + g[2] + 4) / 8, (b[0] + b[2] + 4) / 8, 0};
#endif
}

#ifdef __SSE4_1__
static void Average_AVX2( const uint8* data, v4i* a )
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
#endif

static void CalcErrorBlock( const uint8* data, uint err[4][4] )
{
#ifdef __SSE4_1__
    __m128i d0 = _mm_loadu_si128(((__m128i*)data) + 0);
    __m128i d1 = _mm_loadu_si128(((__m128i*)data) + 1);
    __m128i d2 = _mm_loadu_si128(((__m128i*)data) + 2);
    __m128i d3 = _mm_loadu_si128(((__m128i*)data) + 3);

    __m128i dm0 = _mm_and_si128(d0, _mm_setr_epi8(-1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0));
    __m128i dm1 = _mm_and_si128(d1, _mm_setr_epi8(-1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0));
    __m128i dm2 = _mm_and_si128(d2, _mm_setr_epi8(-1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0));
    __m128i dm3 = _mm_and_si128(d3, _mm_setr_epi8(-1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0));

    __m128i d0l = _mm_unpacklo_epi8(dm0, _mm_setzero_si128());
    __m128i d0h = _mm_unpackhi_epi8(dm0, _mm_setzero_si128());
    __m128i d1l = _mm_unpacklo_epi8(dm1, _mm_setzero_si128());
    __m128i d1h = _mm_unpackhi_epi8(dm1, _mm_setzero_si128());
    __m128i d2l = _mm_unpacklo_epi8(dm2, _mm_setzero_si128());
    __m128i d2h = _mm_unpackhi_epi8(dm2, _mm_setzero_si128());
    __m128i d3l = _mm_unpacklo_epi8(dm3, _mm_setzero_si128());
    __m128i d3h = _mm_unpackhi_epi8(dm3, _mm_setzero_si128());

    __m128i sqrSum0, sqrSum1, sqrSum2, sqrSum3;

    {
        __m128i sum0l = _mm_madd_epi16(d0l, d0l);
        __m128i sum0h = _mm_madd_epi16(d0h, d0h);
        __m128i sum1l = _mm_madd_epi16(d1l, d1l);
        __m128i sum1h = _mm_madd_epi16(d1h, d1h);
        __m128i sum2l = _mm_madd_epi16(d2l, d2l);
        __m128i sum2h = _mm_madd_epi16(d2h, d2h);
        __m128i sum3l = _mm_madd_epi16(d3l, d3l);
        __m128i sum3h = _mm_madd_epi16(d3h, d3h);

        __m128i sum0 = _mm_add_epi32(sum0l, sum1l);
        __m128i sum1 = _mm_add_epi32(sum0h, sum1h);
        __m128i sum2 = _mm_add_epi32(sum2l, sum3l);
        __m128i sum3 = _mm_add_epi32(sum2h, sum3h);

        __m128i sum4 = _mm_hadd_epi32(sum0, sum1);
        __m128i sum5 = _mm_hadd_epi32(sum2, sum3);

        __m128i sum6 = _mm_hadd_epi32(sum4, sum5);

        __m128i t = _mm_shuffle_epi32(sum6, _MM_SHUFFLE(2, 0, 3, 1));

        __m128i sum = _mm_add_epi32(t, sum6);

        sqrSum0 = _mm_shuffle_epi32(sum, _MM_SHUFFLE(3, 3, 3, 3));
        sqrSum1 = _mm_shuffle_epi32(sum, _MM_SHUFFLE(0, 0, 0, 0));
        sqrSum2 = _mm_shuffle_epi32(sum, _MM_SHUFFLE(1, 1, 1, 1));
        sqrSum3 = _mm_shuffle_epi32(sum, _MM_SHUFFLE(2, 2, 2, 2));

        sqrSum0 = _mm_and_si128(sqrSum0, _mm_setr_epi32(0, 0, 0, -1));
        sqrSum1 = _mm_and_si128(sqrSum1, _mm_setr_epi32(0, 0, 0, -1));
        sqrSum2 = _mm_and_si128(sqrSum2, _mm_setr_epi32(0, 0, 0, -1));
        sqrSum3 = _mm_and_si128(sqrSum3, _mm_setr_epi32(0, 0, 0, -1));
    }

    __m128i sum0 = _mm_add_epi16(d0l, d1l);
    __m128i sum1 = _mm_add_epi16(d0h, d1h);
    __m128i sum2 = _mm_add_epi16(d2l, d3l);
    __m128i sum3 = _mm_add_epi16(d2h, d3h);

    __m128i sum0l = _mm_unpacklo_epi16(sum0, _mm_setzero_si128());
    __m128i sum0h = _mm_unpackhi_epi16(sum0, _mm_setzero_si128());
    __m128i sum1l = _mm_unpacklo_epi16(sum1, _mm_setzero_si128());
    __m128i sum1h = _mm_unpackhi_epi16(sum1, _mm_setzero_si128());
    __m128i sum2l = _mm_unpacklo_epi16(sum2, _mm_setzero_si128());
    __m128i sum2h = _mm_unpackhi_epi16(sum2, _mm_setzero_si128());
    __m128i sum3l = _mm_unpacklo_epi16(sum3, _mm_setzero_si128());
    __m128i sum3h = _mm_unpackhi_epi16(sum3, _mm_setzero_si128());

    __m128i b0 = _mm_add_epi32(sum0l, sum0h);
    __m128i b1 = _mm_add_epi32(sum1l, sum1h);
    __m128i b2 = _mm_add_epi32(sum2l, sum2h);
    __m128i b3 = _mm_add_epi32(sum3l, sum3h);

    __m128i a0 = _mm_or_si128(_mm_add_epi32(b2, b3), sqrSum0);
    __m128i a1 = _mm_or_si128(_mm_add_epi32(b0, b1), sqrSum1);
    __m128i a2 = _mm_or_si128(_mm_add_epi32(b1, b3), sqrSum2);
    __m128i a3 = _mm_or_si128(_mm_add_epi32(b0, b2), sqrSum3);

    _mm_storeu_si128((__m128i*)&err[0], a0);
    _mm_storeu_si128((__m128i*)&err[1], a1);
    _mm_storeu_si128((__m128i*)&err[2], a2);
    _mm_storeu_si128((__m128i*)&err[3], a3);
#else
    uint terr[4][4];

    memset(terr, 0, 16 * sizeof(uint));

    for( int j=0; j<4; j++ )
    {
        for( int i=0; i<4; i++ )
        {
            int index = (j & 2) + (i >> 1);
            uint d = *data++;
            terr[index][0] += d;
            terr[index][3] += d*d;
            d = *data++;
            terr[index][1] += d;
            terr[index][3] += d*d;
            d = *data++;
            terr[index][2] += d;
            terr[index][3] += d*d;
            data++;
        }
    }

    for( int i=0; i<4; i++ )
    {
        err[0][i] = terr[2][i] + terr[3][i];
        err[1][i] = terr[0][i] + terr[1][i];
        err[2][i] = terr[1][i] + terr[3][i];
        err[3][i] = terr[0][i] + terr[2][i];
    }
#endif
}

static uint CalcError( const uint block[4], const v4i& average )
{
    uint err = block[3];
    err -= block[0] * 2 * average[2];
    err -= block[1] * 2 * average[1];
    err -= block[2] * 2 * average[0];
    err += 8 * ( sq( average[0] ) + sq( average[1] ) + sq( average[2] ) );
    return err;
}

static void ProcessAverages( v4i* a )
{
#ifdef __SSE4_1__
    for( int i=0; i<2; i++ )
    {
        __m128i d = _mm_loadu_si128((__m128i*)a[i*2].data());

        __m128i t = _mm_add_epi16(_mm_mullo_epi16(d, _mm_set1_epi16(31)), _mm_set1_epi16(128));

        __m128i c = _mm_srli_epi16(_mm_add_epi16(t, _mm_srli_epi16(t, 8)), 8);

        __m128i c1 = _mm_shuffle_epi32(c, _MM_SHUFFLE(3, 2, 3, 2));
        __m128i diff = _mm_sub_epi16(c, c1);
        diff = _mm_max_epi16(diff, _mm_set1_epi16(-4));
        diff = _mm_min_epi16(diff, _mm_set1_epi16(3));

        __m128i co = _mm_add_epi16(c1, diff);

        c = _mm_blend_epi16(co, c, 0xF0);

        __m128i a0 = _mm_or_si128(_mm_slli_epi16(c, 3), _mm_srli_epi16(c, 2));

        _mm_storeu_si128((__m128i*)a[4+i*2].data(), a0);
    }

    for( int i=0; i<2; i++ )
    {
        __m128i d = _mm_loadu_si128((__m128i*)a[i*2].data());

        __m128i t0 = _mm_add_epi16(_mm_mullo_epi16(d, _mm_set1_epi16(15)), _mm_set1_epi16(128));
        __m128i t1 = _mm_srli_epi16(_mm_add_epi16(t0, _mm_srli_epi16(t0, 8)), 8);

        __m128i t2 = _mm_or_si128(t1, _mm_slli_epi16(t1, 4));

        _mm_storeu_si128((__m128i*)a[i*2].data(), t2);
    }
#else
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
        a[i][0] = g_avg2[mul8bit( a[i][0], 15 )];
        a[i][1] = g_avg2[mul8bit( a[i][1], 15 )];
        a[i][2] = g_avg2[mul8bit( a[i][2], 15 )];
    }
#endif
}

#ifdef __SSE4_1__
static void ProcessAverages_AVX2( v4i* a )
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
#endif

static void EncodeAverages( uint64& _d, const v4i* a, size_t idx )
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
#ifdef __SSE4_1__
    __m128i d0 = _mm_loadu_si128(((__m128i*)src) + 0);
    __m128i d1 = _mm_loadu_si128(((__m128i*)src) + 1);
    __m128i d2 = _mm_loadu_si128(((__m128i*)src) + 2);
    __m128i d3 = _mm_loadu_si128(((__m128i*)src) + 3);

    __m128i c = _mm_shuffle_epi32(d0, _MM_SHUFFLE(0, 0, 0, 0));

    __m128i c0 = _mm_cmpeq_epi8(d0, c);
    __m128i c1 = _mm_cmpeq_epi8(d1, c);
    __m128i c2 = _mm_cmpeq_epi8(d2, c);
    __m128i c3 = _mm_cmpeq_epi8(d3, c);

    __m128i m0 = _mm_and_si128(c0, c1);
    __m128i m1 = _mm_and_si128(c2, c3);
    __m128i m = _mm_and_si128(m0, m1);

    if (!_mm_testc_si128(m, _mm_set1_epi32(-1)))
    {
        return 0;
    }
#else
    const uint8* ptr = src + 4;
    for( int i=1; i<16; i++ )
    {
        if( memcmp( src, ptr, 4 ) != 0 )
        {
            return 0;
        }
        ptr += 4;
    }
#endif
    return 0x02000000 |
        ( uint( src[0] & 0xF8 ) << 16 ) |
        ( uint( src[1] & 0xF8 ) << 8 ) |
        ( uint( src[2] & 0xF8 ) );
}

#ifdef __SSE4_1__
static uint64 CheckSolid_AVX2( const uint8* src )
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
#endif

static void PrepareAverages( v4i a[8], const uint8* src, uint err[4] )
{
    Average( src, a );
    ProcessAverages( a );

    uint errblock[4][4];
    CalcErrorBlock( src, errblock );

    for( int i=0; i<4; i++ )
    {
        err[i/2] += CalcError( errblock[i], a[i] );
        err[2+i/2] += CalcError( errblock[i], a[i+4] );
    }
}

#ifdef __SSE4_1__
static void PrepareAverages_AVX2( v4i a[8], const uint8* src, uint err[4] )
{
    Average_AVX2( src, a );
    ProcessAverages_AVX2( a );

    uint errblock[4][4];
    CalcErrorBlock( src, errblock );

    for( int i=0; i<4; i++ )
    {
        err[i/2] += CalcError( errblock[i], a[i] );
        err[2+i/2] += CalcError( errblock[i], a[i+4] );
    }
}
#endif

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

        int dr = a[bid][0] - r;
        int dg = a[bid][1] - g;
        int db = a[bid][2] - b;

#ifdef __SSE4_1__
        // Reference implementation

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
// Non-reference implementation, but faster
static void FindBestFit( uint32 terr[2][8], uint16 tsel[16][8], v4i a[8], const uint32* id, const uint8* data )
{
    for( size_t i=0; i<16; i++ )
    {
        uint16* sel = tsel[i];
        uint bid = id[i];
        uint32* ter = terr[bid%2];

        uint8 b = *data++;
        uint8 g = *data++;
        uint8 r = *data++;
        data++;

        int dr = a[bid][0] - r;
        int dg = a[bid][1] - g;
        int db = a[bid][2] - b;

        // The scaling values are divided by two and rounded, to allow the differences to be in the range of signed int16
        // This produces slightly different results, but is significant faster
        __m128i pixel = _mm_set1_epi16(dr * 38 + dg * 76 + db * 14);
        __m128i pix = _mm_abs_epi16(pixel);

        // Taking the absolute value is way faster. The values are only used to sort, so the result will be the same.
        // Since the selector table is symmetrical, we need to calculate the difference only for half of the entries.
        __m128i error0 = _mm_abs_epi16(_mm_sub_epi16(pix, g_table128_SIMD[0]));
        __m128i error1 = _mm_abs_epi16(_mm_sub_epi16(pix, g_table128_SIMD[1]));

        __m128i index = _mm_and_si128(_mm_cmplt_epi16(error1, error0), _mm_set1_epi16(1));
        __m128i minError = _mm_min_epi16(error0, error1);

        // Exploiting symmetry of the selector table
        __m128i minIndex = _mm_or_si128(index, _mm_and_si128(_mm_set1_epi16(2), _mm_cmpgt_epi16(pixel, _mm_setzero_si128())));

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
}

static void FindBestFit_AVX2( uint32 terr[2][8], uint16 tsel[16][8], v4i a[8], const uint32* id, const uint8* data )
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
#endif

uint64 ProcessRGB( const uint8* src )
{
    uint64 d = CheckSolid( src );
    if( d != 0 ) return d;

    v4i a[8];
    uint err[4] = {};
    PrepareAverages( a, src, err );
    size_t idx = GetLeastError( err, 4 );
    EncodeAverages( d, a, idx );

#if defined __SSE4_1__ && !defined REFERENCE_IMPLEMENTATION
    uint32 terr[2][8] = {};
#else
    uint64 terr[2][8] = {};
#endif
    uint16 tsel[16][8];
    auto id = g_id[idx];
    FindBestFit( terr, tsel, a, id, src );

    return FixByteOrder( EncodeSelectors( d, terr, tsel, id ) );
}

#ifdef __SSE4_1__
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
#endif
