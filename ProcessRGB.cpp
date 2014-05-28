#include <string.h>

#include "ProcessCommon.hpp"
#include "ProcessRGB.hpp"
#include "Tables.hpp"
#include "Types.hpp"
#include "Vector.hpp"

static v3i Average( const uint8* data )
{
    uint32 r = 0, g = 0, b = 0;
    for( int i=0; i<8; i++ )
    {
        b += *data++;
        g += *data++;
        r += *data++;
        data++;
    }
    return v3i( r / 8, g / 8, b / 8 );
}

static void CalcErrorBlock( const uint8* data, uint err[4] )
{
    for( int i=0; i<8; i++ )
    {
        uint d = *data++;
        err[0] += d;
        err[3] += d*d;
        d = *data++;
        err[1] += d;
        err[3] += d*d;
        d = *data++;
        err[2] += d;
        err[3] += d*d;
        data++;
    }
}

static uint CalcError( const uint block[4], const v3i& average )
{
    uint err = block[3];
    err -= block[0] * 2 * average.z;
    err -= block[1] * 2 * average.y;
    err -= block[2] * 2 * average.x;
    err += 8 * ( sq( average.x ) + sq( average.y ) + sq( average.z ) );
    return err;
}

static void ProcessAverages( v3i* a )
{
    for( int i=0; i<2; i++ )
    {
        for( int j=0; j<3; j++ )
        {
            int32 c1 = a[i*2+1][j] >> 3;
            int32 c2 = a[i*2][j] >> 3;

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
        a[i].x = g_avg2[a[i].x >> 4];
        a[i].y = g_avg2[a[i].y >> 4];
        a[i].z = g_avg2[a[i].z >> 4];
    }
}

static void EncodeAverages( uint64& _d, const v3i* a, size_t idx )
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
    const uint8* ptr = src + 4;
    for( int i=1; i<16; i++ )
    {
        if( memcmp( src, ptr, 4 ) != 0 )
        {
            return 0;
        }
        ptr += 4;
    }
    return 0x02000000 |
        ( uint( src[0] & 0xF8 ) << 16 ) |
        ( uint( src[1] & 0xF8 ) << 8 ) |
        ( uint( src[2] & 0xF8 ) );
}

uint64 ProcessRGB( const uint8* src )
{
    uint64 d = CheckSolid( src );
    if( d != 0 ) return d;

    uint8 b23[2][32];
    const uint8* b[4] = { src+32, src, b23[0], b23[1] };

    for( int i=0; i<4; i++ )
    {
        memcpy( b23[1]+i*8, src+i*16, 8 );
        memcpy( b23[0]+i*8, src+i*16+8, 8 );
    }

    v3i a[8];
    for( int i=0; i<4; i++ )
    {
        a[i] = Average( b[i] );
    }
    ProcessAverages( a );

    uint err[4] = {};
    for( int i=0; i<4; i++ )
    {
        uint errblock[4] = {};
        CalcErrorBlock( b[i], errblock );
        err[i/2] += CalcError( errblock, a[i] );
        err[2+i/2] += CalcError( errblock, a[i+4] );
    }
    size_t idx = GetLeastError( err, 4 );

    EncodeAverages( d, a, idx );

    uint64 terr[2][8] = {};
    uint tsel[16][8];
    auto id = g_id[idx];
    const uint8* data = src;
    for( size_t i=0; i<16; i++ )
    {
        uint* sel = tsel[i];
        uint bid = id[i];
        uint64* ter = terr[bid%2];

        uint8 b = *data++;
        uint8 g = *data++;
        uint8 r = *data++;
        data++;

        int dr = a[bid].x - r;
        int dg = a[bid].y - g;
        int db = a[bid].z - b;

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
    }
    size_t tidx[2];
    tidx[0] = GetLeastError( terr[0], 8 );
    tidx[1] = GetLeastError( terr[1], 8 );

    d |= tidx[0] << 26;
    d |= tidx[1] << 29;
    for( int i=0; i<16; i++ )
    {
        uint64 t = tsel[i][tidx[id[i]%2]];
        d |= ( t & 0x1 ) << ( i + 32 );
        d |= ( t & 0x2 ) << ( i + 47 );
    }

    return FixByteOrder( d );
}
