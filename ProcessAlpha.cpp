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
#if defined __SSE4_1__
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
    __m128i smin3 = _mm_alignr_epi8( min2, min2, 2 );
    __m128i max3 = _mm_max_epu8( max2, smax3 );
    __m128i min3 = _mm_min_epu8( min2, smin3 );
    __m128i smax4 = _mm_alignr_epi8( max3, max3, 1 );
    __m128i smin4 = _mm_alignr_epi8( min3, min3, 1 );
    __m128i max = _mm_max_epu8( max3, smax4 );
    __m128i min = _mm_min_epu8( min3, smin4 );

    // src range, mid
    __m128i srcRange = _mm_sub_epi8( max, min );
    __m128i srcRangeHalf1 = _mm_srli_epi16( srcRange, 1 );
    __m128i srcRangeHalf2 = _mm_and_si128( srcRangeHalf1, _mm_set1_epi8( 0x7F ) );
    __m128i srcMid = _mm_add_epi8( min, srcRangeHalf2 );
    __m128i srcMid16 = _mm_unpacklo_epi8( srcMid, _mm_setzero_si128() );

    // multiplier
    __m128i srcRange16 = _mm_unpacklo_epi8( srcRange, _mm_setzero_si128() );
    __m128i mulA1 = _mm_mulhi_epi16( srcRange16, g_alphaRange_SIMD[0] );
    __m128i mulB1 = _mm_mulhi_epi16( srcRange16, g_alphaRange_SIMD[1] );
    __m128i mul[2] = { _mm_add_epi16( mulA1, _mm_set1_epi16( 1 ) ), _mm_add_epi16( mulB1, _mm_set1_epi16( 1 ) ) };

    // wide multiplier
    __m128i rangeMul[16];
    __m128i rmtmp = _mm_shufflelo_epi16( mul[0], _MM_SHUFFLE( 0, 0, 0, 0 ) );
    rangeMul[0] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 0, 0, 0, 0 ) );
    rmtmp = _mm_shufflelo_epi16( mul[0], _MM_SHUFFLE( 1, 1, 1, 1 ) );
    rangeMul[1] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 0, 0, 0, 0 ) );
    rmtmp = _mm_shufflelo_epi16( mul[0], _MM_SHUFFLE( 2, 2, 2, 2 ) );
    rangeMul[2] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 0, 0, 0, 0 ) );
    rmtmp = _mm_shufflelo_epi16( mul[0], _MM_SHUFFLE( 3, 3, 3, 3 ) );
    rangeMul[3] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 0, 0, 0, 0 ) );
    rmtmp = _mm_shufflehi_epi16( mul[0], _MM_SHUFFLE( 0, 0, 0, 0 ) );
    rangeMul[4] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 2, 2, 2, 2 ) );
    rmtmp = _mm_shufflehi_epi16( mul[0], _MM_SHUFFLE( 1, 1, 1, 1 ) );
    rangeMul[5] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 2, 2, 2, 2 ) );
    rmtmp = _mm_shufflehi_epi16( mul[0], _MM_SHUFFLE( 2, 2, 2, 2 ) );
    rangeMul[6] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 2, 2, 2, 2 ) );
    rmtmp = _mm_shufflehi_epi16( mul[0], _MM_SHUFFLE( 3, 3, 3, 3 ) );
    rangeMul[7] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 2, 2, 2, 2 ) );
    rmtmp = _mm_shufflelo_epi16( mul[1], _MM_SHUFFLE( 0, 0, 0, 0 ) );
    rangeMul[8] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 0, 0, 0, 0 ) );
    rmtmp = _mm_shufflelo_epi16( mul[1], _MM_SHUFFLE( 1, 1, 1, 1 ) );
    rangeMul[9] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 0, 0, 0, 0 ) );
    rmtmp = _mm_shufflelo_epi16( mul[1], _MM_SHUFFLE( 2, 2, 2, 2 ) );
    rangeMul[10] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 0, 0, 0, 0 ) );
    rmtmp = _mm_shufflelo_epi16( mul[1], _MM_SHUFFLE( 3, 3, 3, 3 ) );
    rangeMul[11] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 0, 0, 0, 0 ) );
    rmtmp = _mm_shufflehi_epi16( mul[1], _MM_SHUFFLE( 0, 0, 0, 0 ) );
    rangeMul[12] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 2, 2, 2, 2 ) );
    rmtmp = _mm_shufflehi_epi16( mul[1], _MM_SHUFFLE( 1, 1, 1, 1 ) );
    rangeMul[13] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 2, 2, 2, 2 ) );
    rmtmp = _mm_shufflehi_epi16( mul[1], _MM_SHUFFLE( 2, 2, 2, 2 ) );
    rangeMul[14] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 2, 2, 2, 2 ) );
    rmtmp = _mm_shufflehi_epi16( mul[1], _MM_SHUFFLE( 3, 3, 3, 3 ) );
    rangeMul[15] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 2, 2, 2, 2 ) );

    // wide source
    __m128i s16_1 = _mm_shuffle_epi32( s, _MM_SHUFFLE( 3, 2, 3, 2 ) );
    __m128i s16[2] = { _mm_unpacklo_epi8( s, _mm_setzero_si128() ), _mm_unpacklo_epi8( s16_1, _mm_setzero_si128() ) };

    __m128i sr[16];
    rmtmp = _mm_shufflelo_epi16( s16[0], _MM_SHUFFLE( 0, 0, 0, 0 ) );
    sr[0] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 0, 0, 0, 0 ) );
    rmtmp = _mm_shufflelo_epi16( s16[0], _MM_SHUFFLE( 1, 1, 1, 1 ) );
    sr[1] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 0, 0, 0, 0 ) );
    rmtmp = _mm_shufflelo_epi16( s16[0], _MM_SHUFFLE( 2, 2, 2, 2 ) );
    sr[2] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 0, 0, 0, 0 ) );
    rmtmp = _mm_shufflelo_epi16( s16[0], _MM_SHUFFLE( 3, 3, 3, 3 ) );
    sr[3] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 0, 0, 0, 0 ) );
    rmtmp = _mm_shufflehi_epi16( s16[0], _MM_SHUFFLE( 0, 0, 0, 0 ) );
    sr[4] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 2, 2, 2, 2 ) );
    rmtmp = _mm_shufflehi_epi16( s16[0], _MM_SHUFFLE( 1, 1, 1, 1 ) );
    sr[5] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 2, 2, 2, 2 ) );
    rmtmp = _mm_shufflehi_epi16( s16[0], _MM_SHUFFLE( 2, 2, 2, 2 ) );
    sr[6] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 2, 2, 2, 2 ) );
    rmtmp = _mm_shufflehi_epi16( s16[0], _MM_SHUFFLE( 3, 3, 3, 3 ) );
    sr[7] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 2, 2, 2, 2 ) );
    rmtmp = _mm_shufflelo_epi16( s16[1], _MM_SHUFFLE( 0, 0, 0, 0 ) );
    sr[8] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 0, 0, 0, 0 ) );
    rmtmp = _mm_shufflelo_epi16( s16[1], _MM_SHUFFLE( 1, 1, 1, 1 ) );
    sr[9] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 0, 0, 0, 0 ) );
    rmtmp = _mm_shufflelo_epi16( s16[1], _MM_SHUFFLE( 2, 2, 2, 2 ) );
    sr[10] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 0, 0, 0, 0 ) );
    rmtmp = _mm_shufflelo_epi16( s16[1], _MM_SHUFFLE( 3, 3, 3, 3 ) );
    sr[11] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 0, 0, 0, 0 ) );
    rmtmp = _mm_shufflehi_epi16( s16[1], _MM_SHUFFLE( 0, 0, 0, 0 ) );
    sr[12] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 2, 2, 2, 2 ) );
    rmtmp = _mm_shufflehi_epi16( s16[1], _MM_SHUFFLE( 1, 1, 1, 1 ) );
    sr[13] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 2, 2, 2, 2 ) );
    rmtmp = _mm_shufflehi_epi16( s16[1], _MM_SHUFFLE( 2, 2, 2, 2 ) );
    sr[14] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 2, 2, 2, 2 ) );
    rmtmp = _mm_shufflehi_epi16( s16[1], _MM_SHUFFLE( 3, 3, 3, 3 ) );
    sr[15] = _mm_shuffle_epi32( rmtmp, _MM_SHUFFLE( 2, 2, 2, 2 ) );

    // find indices
    int buf[16][16];
    int err = std::numeric_limits<int>::max();
    int sel;
    for( int r=0; r<16; r++ )
    {
        __m128i modVal = _mm_mullo_epi16( rangeMul[r], g_alpha_SIMD[r] );
        __m128i recVal1 = _mm_add_epi16( srcMid16, modVal );
        __m128i recVal = _mm_packus_epi16( recVal1, recVal1 );
        __m128i recVal16 = _mm_unpacklo_epi8( recVal, _mm_setzero_si128() );

        int rangeErr = 0;
        for( int i=0; i<16; i++ )
        {
            __m128i err1 = _mm_sub_epi16( sr[i], recVal16 );
            __m128i err = _mm_abs_epi16( err1 );
            __m128i minerr = _mm_minpos_epu16( err );
            uint32_t tmp;
            _mm_storeu_si32( &tmp, minerr );
            buf[r][i] = tmp >> 16;
            rangeErr += tmp & 0xFFFF;
        }

        if( rangeErr < err )
        {
            err = rangeErr;
            sel = r;
            if( err == 0 ) break;
        }
    }

    uint16_t sm, selmul;
    _mm_storeu_si16( &sm, srcMid16 );
    _mm_storeu_si16( &selmul, rangeMul[sel] );

    uint64_t d = ( uint64_t( sm ) << 56 ) |
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
