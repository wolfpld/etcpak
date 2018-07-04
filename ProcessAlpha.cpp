#include <limits>

#include "Math.hpp"
#include "ProcessAlpha.hpp"
#include "Tables.hpp"

#ifdef __SSE4_1__
#  ifdef _MSC_VER
#    include <intrin.h>
#    include <Windows.h>
#    define _bswap(x) _byteswap_ulong(x)
#  else
#    include <x86intrin.h>
#  endif
#else
#  ifndef _MSC_VER
#    include <byteswap.h>
#    define _bswap(x) bswap_32(x)
#  endif
#endif

#ifndef _bswap
#  define _bswap(x) __builtin_bswap32(x)
#endif


uint64_t ProcessAlpha( const uint8_t* src )
{
#if defined __SSE4_1__ && 0
    // Check solid
    __m128i s = _mm_loadu_si128( (__m128i*)src );
    __m128i solidCmp = _mm_set1_epi8( src[0] );
    __m128i cmpRes = _mm_cmpeq_epi8( s, solidCmp );
    if( _mm_testc_si128( cmpRes, _mm_set1_epi32( -1 ) ) )
    {
        return src[0];
    }

    // Calculate min, max
    __m128i s1 = _mm_shuffle_epi32( s, _MM_SHUFFLE( 2, 3, 0, 1 ) );
    __m128i max1 = _mm_max_epu8( s, s1 );
    __m128i min1 = _mm_min_epu8( s, s1 );
    __m128i smax2 = _mm_shuffle_epi32( max1, _MM_SHUFFLE( 0, 0, 2, 2 ) );
    __m128i smin2 = _mm_shuffle_epi32( min1, _MM_SHUFFLE( 0, 0, 2, 2 ) );
    __m128i max2 = _mm_max_epu8( max1, smax2 );
    __m128i min2 = _mm_min_epu8( min1, smin2 );
    __m128i smax3 = _mm_alignr_epi8( max2, max2, 2 );
    __m128i smin3 = _mm_alignr_epi8( max2, max2, 2 );
    __m128i max3 = _mm_max_epu8( max2, smax3 );
    __m128i min3 = _mm_min_epu8( min2, smin3 );
    __m128i smax4 = _mm_alignr_epi8( max3, max3, 1 );
    __m128i smin4 = _mm_alignr_epi8( max3, max3, 1 );
    __m128i max = _mm_max_epu8( max3, smax4 );
    __m128i min = _mm_min_epu8( min3, smin4 );

    // src range, mid
    __m128i srcRange = _mm_sub_epi8( max, min );
    __m128i srcRangeHalf1 = _mm_srli_epi16( srcRange, 1 );
    __m128i srcRangeHalf2 = _mm_and_si128( srcRangeHalf1, _mm_set1_epi8( 0x7F ) );
    __m128i srcMid = _mm_add_epi8( min, srcRangeHalf2 );

    // multiplier
    __m128i srcRange16 = _mm_srai_epi16( srcRange, 8 );
    __m128i mulA1 = _mm_mulhi_epi16( srcRange16, g_alphaRange_SIMD[0] );
    __m128i mulB1 = _mm_mulhi_epi16( srcRange16, g_alphaRange_SIMD[1] );
    __m128i mulA = _mm_add_epi16( mulA1, g_one_SIMD16 );
    __m128i mulB = _mm_add_epi16( mulB1, g_one_SIMD16 );

    return 0;
#else
    {
        bool solid = true;
        const uint8_t* ptr = src + 1;
        const uint8_t ref = *src;
        for( int i=1; i<16; i++ )
        {
            if( ref != *ptr++ )
            {
                solid = false;
                break;
            }
        }
        if( solid )
        {
            return ref;
        }
    }

    uint8_t min = src[0];
    uint8_t max = src[0];
    for( int i=1; i<16; i++ )
    {
        if( min > src[i] ) min = src[i];
        else if( max < src[i] ) max = src[i];
    }
    int srcRange = max - min;
    int srcMid = min + srcRange / 2;

    int buf[16][16];
    int err = std::numeric_limits<int>::max();
    int sel;
    int selmul;
    for( int r=0; r<16; r++ )
    {
        int mul = ( ( srcRange * g_alphaRange[r] ) >> 16 ) + 1;

        int rangeErr = 0;
        for( int i=0; i<16; i++ )
        {
            const auto srcVal = src[i];

            int idx = 0;
            const auto modVal = g_alpha[r][0] * mul;
            const auto recVal = clampu8( srcMid + modVal );
            int localErr = sq( srcVal - recVal );

            if( localErr != 0 )
            {
                for( int j=1; j<8; j++ )
                {
                    const auto modVal = g_alpha[r][j] * mul;
                    const auto recVal = clampu8( srcMid + modVal );
                    const auto errProbe = sq( srcVal - recVal );
                    if( errProbe < localErr )
                    {
                        localErr = errProbe;
                        idx = j;
                    }
                }
            }

            buf[r][i] = idx;
            rangeErr += localErr;
        }

        if( rangeErr < err )
        {
            err = rangeErr;
            sel = r;
            selmul = mul;
            if( err == 0 ) break;
        }
    }

    uint64_t d = ( uint64_t( srcMid ) << 56 ) |
                 ( uint64_t( selmul ) << 52 ) |
                 ( uint64_t( sel ) << 48 );

    int offset = 45;
    auto ptr = buf[sel];
    for( int i=0; i<16; i++ )
    {
        d |= uint64_t( *ptr++ ) << offset;
        offset -= 3;
    }

    d = ( ( d & 0xFF00000000000000 ) >> 56 ) |
        ( ( d & 0x00FF000000000000 ) >> 40 ) |
        ( ( d & 0x0000FF0000000000 ) >> 24 ) |
        ( ( d & 0x000000FF00000000 ) >> 8 ) |
        ( ( d & 0x00000000FF000000 ) << 8 ) |
        ( ( d & 0x0000000000FF0000 ) << 24 ) |
        ( ( d & 0x000000000000FF00 ) << 40 ) |
        ( ( d & 0x00000000000000FF ) << 56 );

    return d;
#endif
}
