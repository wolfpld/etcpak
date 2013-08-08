#include "ProcessAlpha.hpp"
#include "ProcessCommon.hpp"
#include "Tables.hpp"
#include "Types.hpp"
#include "Vector.hpp"

static uint Average1( const uint8* data )
{
    uint32 a = 0;
    for( int i=0; i<8; i++ )
    {
        a += *data++;
    }
    return a / 8;
}

static uint CalcError( const uint8* data, uint average )
{
    uint err = 0;
    uint sum = 0;
    for( int i=0; i<8; i++ )
    {
        uint v = *data++;
        sum += v;
        err += v*v;
    }
    err -= sum * 2 * average;
    err += 8 * sq( average );
    return err;
}

static void ProcessAverages( uint* a )
{
    for( int i=0; i<2; i++ )
    {
        int c1 = a[i*2+1] >> 3;
        int c2 = a[i*2] >> 3;

        int diff = c2 - c1;
        if( diff > 3 ) diff = 3;
        else if( diff < -4 ) diff = -4;

        int co = c1 + diff;

        a[5+i*2] = ( c1 << 3 ) | ( c1 >> 2 );
        a[4+i*2] = ( co << 3 ) | ( co >> 2 );
    }
    for( int i=0; i<4; i++ )
    {
        uint c = a[i];
        a[i] = ( c & 0xF0 ) | ( c >> 4 );
    }
}

static void EncodeAverages( uint64& d, const uint* a, size_t idx )
{
    d |= idx;
    size_t base = idx << 1;

    if( ( idx & 0x2 ) == 0 )
    {
        for( int i=0; i<3; i++ )
        {
            d |= uint64( a[base+0] & 0xF0 ) << ( i*8 + 4 );
            d |= uint64( a[base+1] & 0xF0 ) << ( i*8 + 8 );
        }
    }
    else
    {
        for( int i=0; i<3; i++ )
        {
            d |= uint64( a[base+1] & 0xF8 ) << ( i*8 + 8 );
            int32 c = ( ( a[base+0] & 0xF8 ) - ( a[base+1] & 0xF8 ) ) >> 3;
            c &= ~0xFFFFFFF8;
            d |= ((uint64)c) << ( i*8 + 8 );
        }
    }
}

uint64 ProcessAlpha( const uint8* src )
{
    uint64 d = 0;

    {
        bool solid = true;
        const uint8* ptr = src + 1;
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
            uint c = *src & 0xF8;
            d |= 0x2 | ( c << 24 ) | ( c << 16 ) | ( c << 8 );
            return d;
        }
    }

    uint8 b23[2][8];
    const uint8* b[4] = { src+8, src, b23[0], b23[1] };

    for( int i=0; i<4; i++ )
    {
        *(b23[1]+i*2) = *(src+i*4);
        *(b23[0]+i*2) = *(src+i*4+3);
    }

    uint a[8];
    for( int i=0; i<4; i++ )
    {
        a[i] = Average1( b[i] );
    }
    ProcessAverages( a );

    uint err[4] = {};
    for( int i=0; i<4; i++ )
    {
        err[i/2] += CalcError( b[i], a[i] );
        err[2+i/2] += CalcError( b[i], a[i+4] );
    }
    size_t idx = GetLeastError( err, 4 );

    EncodeAverages( d, a, idx );

    uint terr[2][8] = {};
    uint tsel[16][8];
    uint id[16];
    for( int i=0; i<16; i++ )
    {
        id[i] = (uint)GetBufId( i, idx );
    }
    const uint8* data = src;
    for( size_t i=0; i<16; i++ )
    {
        uint* sel = tsel[i];
        uint bid = id[i];
        uint* ter = terr[bid%2];

        uint8 c = *data++;
        int32 pix = a[bid] - c;

        for( int t=0; t<8; t++ )
        {
            const int32* tab = g_table[t];
            uint idx = 0;
            uint err = sq( tab[0] + pix );
            for( int j=1; j<4; j++ )
            {
                uint local = sq( tab[j] + pix );
                if( local < err )
                {
                    err = local;
                    idx = j;
                }
            }
            *sel++ = idx;
            *ter++ += err;
        }
    }
    size_t tidx[2];
    tidx[0] = GetLeastError( terr[0], 8 );
    tidx[1] = GetLeastError( terr[1], 8 );

    d |= tidx[0] << 2;
    d |= tidx[1] << 5;
    for( int i=0; i<16; i++ )
    {
        uint64 t = tsel[i][tidx[id[i]%2]];
        d |= ( t & 0x1 ) << ( i + 32 );
        d |= ( t & 0x2 ) << ( i + 47 );
    }

    return d;
}
