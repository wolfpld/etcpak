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

static void CalcErrorBlock( const uint8* data, uint err[2] )
{
    for( int i=0; i<8; i++ )
    {
        uint v = *data++;
        err[0] += v;
        err[1] += v*v;
    }
}

static uint CalcError( const uint block[2], uint average )
{
    uint err = block[1];
    err -= block[0] * 2 * average;
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
        a[i] = g_avg2[a[i] >> 4];
    }
}

static void EncodeAverages( uint64& _d, const uint* a, size_t idx )
{
    auto d = _d;
    d |= ( idx << 24 );
    size_t base = idx << 1;

    uint v;
    if( ( idx & 0x2 ) == 0 )
    {
        v = ( a[base+0] >> 4 ) | ( a[base+1] & 0xF0 );
    }
    else
    {
        v = a[base+1] & 0xF8;
        int32 c = ( ( a[base+0] & 0xF8 ) - ( a[base+1] & 0xF8 ) ) >> 3;
        v |= c & ~0xFFFFFFF8;
    }
    d |= v | ( v << 8 ) | ( v << 16 );
    _d = d;
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
            d |= 0x02000000 | ( c << 16 ) | ( c << 8 ) | c;
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
        uint errblock[2] = {};
        CalcErrorBlock( b[i], errblock );
        err[i/2] += CalcError( errblock, a[i] );
        err[2+i/2] += CalcError( errblock, a[i+4] );
    }
    size_t idx = GetLeastError( err, 4 );

    EncodeAverages( d, a, idx );

    uint terr[2][8] = {};
    uint tsel[16][8];
    auto id = g_id[idx];
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

    return FixByteOrder( EncodeSelectors( d, terr, tsel, id ) );
}
