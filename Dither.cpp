#include <algorithm>
#include <string.h>

#include "Dither.hpp"
#include "Math.hpp"
#ifdef __SSE4_1__
#  ifdef _MSC_VER
#    include <intrin.h>
#    include <Windows.h>
#  else
#    include <x86intrin.h>
#  endif
#endif

void Dither( uint8_t* data )
{
    static constexpr int8_t Bayer31[16] = {
        ( 0-8)*2/3, ( 8-8)*2/3, ( 2-8)*2/3, (10-8)*2/3,
        (12-8)*2/3, ( 4-8)*2/3, (14-8)*2/3, ( 6-8)*2/3,
        ( 3-8)*2/3, (11-8)*2/3, ( 1-8)*2/3, ( 9-8)*2/3,
        (15-8)*2/3, ( 7-8)*2/3, (13-8)*2/3, ( 5-8)*2/3
    };
    static constexpr int8_t Bayer63[16] = {
        ( 0-8)*2/6, ( 8-8)*2/6, ( 2-8)*2/6, (10-8)*2/6,
        (12-8)*2/6, ( 4-8)*2/6, (14-8)*2/6, ( 6-8)*2/6,
        ( 3-8)*2/6, (11-8)*2/6, ( 1-8)*2/6, ( 9-8)*2/6,
        (15-8)*2/6, ( 7-8)*2/6, (13-8)*2/6, ( 5-8)*2/6
    };

    for( int i=0; i<16; i++ )
    {
        uint32_t col;
        memcpy( &col, data, 4 );
        uint8_t r = col & 0xFF;
        uint8_t g = ( col >> 8 ) & 0xFF;
        uint8_t b = ( col >> 16 ) & 0xFF;

        r = clampu8( r + Bayer31[i] );
        g = clampu8( g + Bayer63[i] );
        b = clampu8( b + Bayer31[i] );

        col = r | ( g << 8 ) | ( b << 16 );
        memcpy( data, &col, 4 );
        data += 4;
    }
}
