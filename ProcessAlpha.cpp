#include <limits>

#include "Math.hpp"
#include "ProcessAlpha.hpp"
#include "Tables.hpp"

uint64_t ProcessAlpha( const uint8_t* src )
{
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

    int buf[16][16];
    int err = std::numeric_limits<int>::max();
    int sel;
    int selmul;
    for( int r=0; r<16; r++ )
    {
        int mul = ( srcRange / g_alphaRange[r] ) + 1;

        for( int i=0; i<16; i++ )
        {
            buf[r][i] = int( src[i] - min ) / mul + g_alpha[r][3];
        }

        int rangeErr = 0;
        for( int i=0; i<16; i++ )
        {
            int localErr = std::numeric_limits<int>::max();
            for( int j=0; j<8; j++ )
            {
                const auto errProbe = sq( buf[r][i] - g_alpha[r][j] );
                if( errProbe < localErr )
                {
                    localErr = errProbe;
                }
            }
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

    const int base = min - g_alpha[sel][3] * selmul;
    uint64_t d = ( uint64_t( base ) << 56 ) |
                 ( uint64_t( selmul ) << 52 ) |
                 ( uint64_t( sel ) << 48 );

    int offset = 45;
    for( int i=0; i<16; i++ )
    {
        int idx = 0;
        int localErr = std::numeric_limits<int>::max();
        for( int j=0; j<8; j++ )
        {
            const auto errProbe = sq( buf[sel][i] - g_alpha[sel][j] );
            if( errProbe < localErr )
            {
                localErr = errProbe;
                idx = j;
            }
        }

        d |= uint64_t( idx ) << offset;
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
}
