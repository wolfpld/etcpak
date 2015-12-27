#ifdef __SSE4_1__

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
#  define _bswap(x) _byteswap_ulong(x)
#else
#  include <x86intrin.h>
#endif

#ifdef _MSC_VER
    static inline unsigned long _bit_scan_forward( unsigned long mask )
    {
        unsigned long ret;
        _BitScanForward( &ret, mask );
        return ret;
    }
#else
#  pragma GCC push_options
#  pragma GCC target ("avx2")
#endif

namespace
{

typedef std::array<uint16, 4> v4i;

__m256i Sum4_AVX2( const uint8* data)
{
    __m128i d0 = _mm_loadu_si128(((__m128i*)data) + 0);
    __m128i d1 = _mm_loadu_si128(((__m128i*)data) + 1);
    __m128i d2 = _mm_loadu_si128(((__m128i*)data) + 2);
    __m128i d3 = _mm_loadu_si128(((__m128i*)data) + 3);

    __m128i dm0 = _mm_and_si128(d0, _mm_set1_epi32(0x00FFFFFF));
    __m128i dm1 = _mm_and_si128(d1, _mm_set1_epi32(0x00FFFFFF));
    __m128i dm2 = _mm_and_si128(d2, _mm_set1_epi32(0x00FFFFFF));
    __m128i dm3 = _mm_and_si128(d3, _mm_set1_epi32(0x00FFFFFF));

    __m256i t0 = _mm256_cvtepu8_epi16(dm0);
    __m256i t1 = _mm256_cvtepu8_epi16(dm1);
    __m256i t2 = _mm256_cvtepu8_epi16(dm2);
    __m256i t3 = _mm256_cvtepu8_epi16(dm3);

    __m256i sum0 = _mm256_add_epi16(t0, t1);
    __m256i sum1 = _mm256_add_epi16(t2, t3);

    __m256i s0 = _mm256_permute2x128_si256(sum0, sum1, (0) | (3 << 4)); // 0, 0, 3, 3
    __m256i s1 = _mm256_permute2x128_si256(sum0, sum1, (1) | (2 << 4)); // 1, 1, 2, 2

    __m256i s2 = _mm256_permute4x64_epi64(s0, _MM_SHUFFLE(1, 3, 0, 2));
    __m256i s3 = _mm256_permute4x64_epi64(s0, _MM_SHUFFLE(0, 2, 1, 3));
    __m256i s4 = _mm256_permute4x64_epi64(s1, _MM_SHUFFLE(3, 1, 0, 2));
    __m256i s5 = _mm256_permute4x64_epi64(s1, _MM_SHUFFLE(2, 0, 1, 3));

    __m256i sum5 = _mm256_add_epi16(s2, s3); //   3,   0,   3,   0
    __m256i sum6 = _mm256_add_epi16(s4, s5); //   2,   1,   1,   2
    return _mm256_add_epi16(sum5, sum6);     // 3+2, 0+1, 3+1, 3+2
}

__m256i Average_AVX2( const __m256i data)
{
    __m256i a = _mm256_add_epi16(data, _mm256_set1_epi16(4));

    return _mm256_srli_epi16(a, 3);
}

__m128i CalcErrorBlock_AVX2( const __m256i data, const v4i* a)
{
    //
    __m256i a0 = _mm256_load_si256((__m256i*)a[0].data());
    __m256i a1 = _mm256_load_si256((__m256i*)a[4].data());
    __m256i a2 = _mm256_slli_epi16(a0, 1);
    __m256i a3 = _mm256_slli_epi16(a1, 1);

    // err = 8 * ( sq( average[0] ) + sq( average[1] ) + sq( average[2] ) );
    __m256i a4 = _mm256_madd_epi16(a0, a0);
    __m256i a5 = _mm256_madd_epi16(a1, a1);

    __m256i a6 = _mm256_hadd_epi32(a4, a5);
    __m256i a7 = _mm256_slli_epi32(a6, 3);

    __m256i a8 = _mm256_add_epi32(a7, _mm256_set1_epi32(0x3FFFFFFF)); // Big value to prevent negative values, but small enough to prevent overflow

    // average is not swapped
    // err -= block[0] * 2 * average[0];
    // err -= block[1] * 2 * average[1];
    // err -= block[2] * 2 * average[2];
    __m256i b0 = _mm256_madd_epi16(a2, data);
    __m256i b1 = _mm256_madd_epi16(a3, data);

    __m256i b2 = _mm256_hadd_epi32(b0, b1);
    __m256i b3 = _mm256_sub_epi32(a8, b2);
    __m256i b4 = _mm256_hadd_epi32(b3, b3);

    __m256i b5 = _mm256_permutevar8x32_epi32(b4, _mm256_set_epi32(0, 0, 0, 0, 5, 1, 4, 0));

    return _mm256_castsi256_si128(b5);
}

void ProcessAverages_AVX2(const __m256i d, v4i* a )
{
    __m256i t = _mm256_add_epi16(_mm256_mullo_epi16(d, _mm256_set1_epi16(31)), _mm256_set1_epi16(128));

    __m256i c = _mm256_srli_epi16(_mm256_add_epi16(t, _mm256_srli_epi16(t, 8)), 8);

    __m256i c1 = _mm256_shuffle_epi32(c, _MM_SHUFFLE(3, 2, 3, 2));
    __m256i diff = _mm256_sub_epi16(c, c1);
    diff = _mm256_max_epi16(diff, _mm256_set1_epi16(-4));
    diff = _mm256_min_epi16(diff, _mm256_set1_epi16(3));

    __m256i co = _mm256_add_epi16(c1, diff);

    c = _mm256_blend_epi16(co, c, 0xF0);

    __m256i a0 = _mm256_or_si256(_mm256_slli_epi16(c, 3), _mm256_srli_epi16(c, 2));

    _mm256_store_si256((__m256i*)a[4].data(), a0);

    __m256i t0 = _mm256_add_epi16(_mm256_mullo_epi16(d, _mm256_set1_epi16(15)), _mm256_set1_epi16(128));
    __m256i t1 = _mm256_srli_epi16(_mm256_add_epi16(t0, _mm256_srli_epi16(t0, 8)), 8);

    __m256i t2 = _mm256_or_si256(t1, _mm256_slli_epi16(t1, 4));

    _mm256_store_si256((__m256i*)a[0].data(), t2);
}

uint64 EncodeAverages_AVX2( const v4i* a, size_t idx )
{
    uint64 d = ( idx << 24 );
    size_t base = idx << 1;

    __m128i a0 = _mm_load_si128((const __m128i*)a[base].data());

    __m128i r0, r1;

    if( ( idx & 0x2 ) == 0 )
    {
        r0 = _mm_srli_epi16(a0, 4);

        __m128i a1 = _mm_unpackhi_epi64(r0, r0);
        r1 = _mm_slli_epi16(a1, 4);
    }
    else
    {
        __m128i a1 = _mm_and_si128(a0, _mm_set1_epi16(0xFFF8));

        r0 = _mm_unpackhi_epi64(a1, a1);
        __m128i a2 = _mm_sub_epi16(a1, r0);
        __m128i a3 = _mm_srai_epi16(a2, 3);
        r1 = _mm_and_si128(a3, _mm_set1_epi16(0x07));
    }

    __m128i r2 = _mm_or_si128(r0, r1);
    // do missing swap for average values
    __m128i r3 = _mm_shufflelo_epi16(r2, _MM_SHUFFLE(3, 0, 1, 2));
    __m128i r4 = _mm_packus_epi16(r3, _mm_setzero_si128());
    d |= _mm_cvtsi128_si32(r4);

    return d;
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

__m128i PrepareAverages_AVX2( v4i a[8], const uint8* src)
{
    __m256i sum4 = Sum4_AVX2( src );

    ProcessAverages_AVX2(Average_AVX2( sum4 ), a );

    return CalcErrorBlock_AVX2( sum4, a);
}

void FindBestFit_AVX2( uint32 terr[2][8], uint32 tsel[8], v4i a[8], const uint32* id, const uint8* data)
{
    __m256i sel = _mm256_setzero_si256();

    for( size_t i=0; i<16; i+=2 )
    {
        uint bid = id[i];
        uint32* ter = terr[bid%2];

        __m128i a0 = _mm_loadl_epi64((const __m128i*)a[bid].data());
        __m128i rgb = _mm_loadl_epi64((const __m128i*)(data + i * 4));

        __m128i rgb16 = _mm_cvtepu8_epi16(rgb);
        __m128i d = _mm_sub_epi16(_mm_broadcastq_epi64(a0), rgb16);

        // The scaling values are divided by two and rounded, to allow the differences to be in the range of signed int16
        // This produces slightly different results, but is significant faster
        __m128i pixel0 = _mm_madd_epi16(d, _mm_set_epi16(0, 38, 76, 14, 0, 38, 76, 14));
        __m128i pixel1 = _mm_packs_epi32(pixel0, pixel0);
        __m128i pixel2 = _mm_hadd_epi16(pixel1, pixel1);

        __m128i pix0 = _mm_broadcastw_epi16(pixel2);
        __m128i pix1 = _mm_broadcastw_epi16(_mm_srli_epi32(pixel2, 16));
        __m256i pixel = _mm256_insertf128_si256(_mm256_castsi128_si256(pix0), pix1, 1);

        __m256i pix = _mm256_abs_epi16(pixel);

        // Taking the absolute value is way faster. The values are only used to sort, so the result will be the same.
        // Since the selector table is symmetrical, we need to calculate the difference only for half of the entries.
        __m256i error0 = _mm256_abs_epi16(_mm256_sub_epi16(pix, _mm256_broadcastsi128_si256(g_table128_SIMD[0])));
        __m256i error1 = _mm256_abs_epi16(_mm256_sub_epi16(pix, _mm256_broadcastsi128_si256(g_table128_SIMD[1])));

        __m256i minIndex0 = _mm256_and_si256(_mm256_cmpgt_epi16(error0, error1), _mm256_set1_epi16(1));
        __m256i minError = _mm256_min_epi16(error0, error1);

        // Exploiting symmetry of the selector table
        __m256i minIndex1 = _mm256_and_si256(_mm256_cmpgt_epi16(pixel, _mm256_setzero_si256()), _mm256_set1_epi16(1));

        // Interleaving values so madd instruction can be used
        __m256i minErrorLo = _mm256_permute4x64_epi64(minError, _MM_SHUFFLE(1, 1, 0, 0));
        __m256i minErrorHi = _mm256_permute4x64_epi64(minError, _MM_SHUFFLE(3, 3, 2, 2));

        __m256i minError2 = _mm256_unpacklo_epi16(minErrorLo, minErrorHi);
        // Squaring the minimum error to produce correct values when adding
        __m256i squareErrorSum = _mm256_madd_epi16(minError2, minError2);

        __m256i squareError = _mm256_add_epi32(squareErrorSum, _mm256_load_si256((__m256i*)ter));

        _mm256_store_si256((__m256i*)ter, squareError);

        // Packing selector bits
        __m256i minIndexLo0 = _mm256_unpacklo_epi16(minIndex0, minIndex1);
        __m256i minIndexHi0 = _mm256_unpackhi_epi16(minIndex0, minIndex1);

        __m256i minIndexLo1 = _mm256_permute2x128_si256(minIndexLo0, minIndexHi0, (0) | (2 << 4));
        __m256i minIndexHi1 = _mm256_permute2x128_si256(minIndexLo0, minIndexHi0, (1) | (3 << 4));

        __m256i minIndexLo2 = _mm256_sll_epi32(minIndexLo1, _mm_set1_epi64x(i));
        __m256i minIndexHi2 = _mm256_sll_epi32(minIndexHi1, _mm_set1_epi64x(i));
        __m256i minIndexHi3 = _mm256_add_epi32(minIndexHi2, minIndexHi2);

        sel = _mm256_or_si256(sel, minIndexLo2);
        sel = _mm256_or_si256(sel, minIndexHi3);
    }

    _mm256_store_si256((__m256i*)tsel, sel);
}

uint64 EncodeSelectors_AVX2( uint64 d, const uint32 terr[2][8], const uint32 tsel[8], const bool rotate)
{
    size_t tidx[2];

    // Get index of minimum error (terr[0] and terr[1])
    __m256i err0 = _mm256_load_si256((const __m256i*)terr[0]);
    __m256i err1 = _mm256_load_si256((const __m256i*)terr[1]);

    __m256i errLo = _mm256_permute2x128_si256(err0, err1, (0) | (2 << 4));
    __m256i errHi = _mm256_permute2x128_si256(err0, err1, (1) | (3 << 4));

    __m256i errMin0 = _mm256_min_epu32(errLo, errHi);

    __m256i errMin1 = _mm256_shuffle_epi32(errMin0, _MM_SHUFFLE(2, 3, 0, 1));
    __m256i errMin2 = _mm256_min_epu32(errMin0, errMin1);

    __m256i errMin3 = _mm256_shuffle_epi32(errMin2, _MM_SHUFFLE(1, 0, 3, 2));
    __m256i errMin4 = _mm256_min_epu32(errMin3, errMin2);

    __m256i errMin5 = _mm256_permute2x128_si256(errMin4, errMin4, (0) | (0 << 4));
    __m256i errMin6 = _mm256_permute2x128_si256(errMin4, errMin4, (1) | (1 << 4));

    __m256i errMask0 = _mm256_cmpeq_epi32(errMin5, err0);
    __m256i errMask1 = _mm256_cmpeq_epi32(errMin6, err1);

    uint32 mask0 = _mm256_movemask_epi8(errMask0);
    uint32 mask1 = _mm256_movemask_epi8(errMask1);

    tidx[0] = _bit_scan_forward(mask0) >> 2;
    tidx[1] = _bit_scan_forward(mask1) >> 2;

    d |= tidx[0] << 26;
    d |= tidx[1] << 29;

    uint t0 = tsel[tidx[0]];
    uint t1 = tsel[tidx[1]];

    if (!rotate)
    {
        t0 &= 0xFF00FF00;
        t1 &= 0x00FF00FF;
    }
    else
    {
        t0 &= 0xCCCCCCCC;
        t1 &= 0x33333333;
    }

    return d | static_cast<uint64>(_bswap(t0 | t1)) << 32;
}

}

uint64 ProcessRGB_AVX2( const uint8* src )
{
    uint64 d = CheckSolid_AVX2( src );
    if( d != 0 ) return d;

    alignas(32) v4i a[8];

    __m128i err0 = PrepareAverages_AVX2( a, src );

    // Get index of minimum error (err0)
    __m128i err1 = _mm_shuffle_epi32(err0, _MM_SHUFFLE(2, 3, 0, 1));
    __m128i errMin0 = _mm_min_epu32(err0, err1);

    __m128i errMin1 = _mm_shuffle_epi32(errMin0, _MM_SHUFFLE(1, 0, 3, 2));
    __m128i errMin2 = _mm_min_epu32(errMin1, errMin0);

    __m128i errMask = _mm_cmpeq_epi32(errMin2, err0);

    uint32 mask = _mm_movemask_epi8(errMask);

    size_t idx = _bit_scan_forward(mask) >> 2;

    d = EncodeAverages_AVX2( a, idx );

    alignas(32) uint32 terr[2][8] = {};
    alignas(32) uint32 tsel[8];
    auto id = g_id[idx];
    FindBestFit_AVX2( terr, tsel, a, id, src );

    return EncodeSelectors_AVX2( d, terr, tsel, (idx % 2) == 1 );
}

#pragma GCC pop_options

#endif
