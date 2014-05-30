#include <algorithm>
#include <string.h>

#include "Dither.hpp"
#include "Math.hpp"

static uint8 e5[32];
static uint8 e6[64];
static uint8 qrb[256+16];
static uint8 qg[256+16];

void InitDither()
{
    for( int i=0; i<32; i++ )
    {
        e5[i] = (i<<3) | (i>>2);
    }
    for( int i=0; i<64; i++ )
    {
        e6[i] = (i<<2) | (i>>4);
    }
    for( int i=0; i<256+16; i++ )
    {
        int v = std::min( std::max( 0, i-8 ), 255 );
        qrb[i] = e5[mul8bit( v, 31 )];
        qg[i] = e6[mul8bit( v, 63 )];
    }
}

void Dither( uint8* data )
{
    int err[8];
    int* ep1 = err;
    int* ep2 = err+4;

    for( int ch=0; ch<3; ch++ )
    {
        uint8* ptr = data + ch;
        uint8* quant = (ch == 1) ? qg + 8 : qrb + 8;
        memset( err, 0, sizeof( err ) );

        for( int y=0; y<4; y++ )
        {
            uint8 tmp;
            tmp = quant[ptr[0] + ( ( 3 * ep2[1] + 5 * ep2[0] ) >> 4 )];
            ep1[0] = ptr[0] - tmp;
            ptr[0] = tmp;
            tmp = quant[ptr[4] + ( ( 7 * ep1[0] + 3 * ep2[2] + 5 * ep2[1] + ep2[0] ) >> 4 )];
            ep1[1] = ptr[4] - tmp;
            ptr[4] = tmp;
            tmp = quant[ptr[8] + ( ( 7 * ep1[1] + 3 * ep2[3] + 5 * ep2[2] + ep2[1] ) >> 4 )];
            ep1[2] = ptr[8] - tmp;
            ptr[8] = tmp;
            tmp = quant[ptr[12] + ( ( 7 * ep1[2] + 5 * ep2[3] + ep2[2] ) >> 4 )];
            ep1[3] = ptr[12] - tmp;
            ptr[12] = tmp;
            ptr += 16;
            std::swap( ep1, ep2 );
        }
    }
}
