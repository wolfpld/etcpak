#include <assert.h>
#include <string.h>

#include "BlockData.hpp"
#include "ColorSpace.hpp"
#include "Debug.hpp"
#include "mmap.hpp"
#include "ProcessAlpha.hpp"
#include "ProcessRGB.hpp"
#include "Tables.hpp"

static Color::Lab Average( const Color::Lab* data )
{
    Color::Lab ret;
    for( int i=0; i<8; i++ )
    {
        ret.L += data->L;
        ret.a += data->a;
        ret.b += data->b;
        data++;
    }
    ret.L /= 8;
    ret.a /= 8;
    ret.b /= 8;
    return ret;
}

static float CalcError( Color::Lab* c, const Color::Lab& avg )
{
    float err = 0;
    for( int i=0; i<8; i++ )
    {
        err += sq( c[i].L - avg.L ) + sq( c[i].a - avg.a ) + sq( c[i].b - avg.b );
    }
    return err;
}

static float CalcError( Color::Lab* c, const v3i& average )
{
    return CalcError( c, Color::Lab( average ) );
}

static inline Color::Lab ToLab( const uint8* data )
{
    uint32 b = *data++;
    uint32 g = *data++;
    uint32 r = *data;
    return Color::Lab( v3b( r, g, b ) );
}

BlockData::BlockData( const char* fn )
    : m_data( nullptr )
    , m_done( true )
{
    FILE* f = fopen( fn, "rb" );

    uint32 data;
    fread( &data, 1, 4, f );
    if( data == 0x03525650 )
    {
        fseek( f, 20, SEEK_CUR );
        fread( &data, 1, 4, f );
        m_size.y = data;
        fread( &data, 1, 4, f );
        m_size.x = data;
        fseek( f, 20, SEEK_CUR );
    }
    else if( data == 0x58544BAB )
    {
        fseek( f, 32, SEEK_CUR );
        fread( &data, 1, 4, f );
        m_size.x = data;
        fread( &data, 1, 4, f );
        m_size.y = data;
        fseek( f, 16, SEEK_CUR );
        fread( &data, 1, 4, f );
        fseek( f, data + 4, SEEK_CUR );
    }
    else
    {
        assert( false );
    }

    uint32 cnt = m_size.x * m_size.y / 16;
    m_data = new uint64[cnt];
    fread( m_data, 1, cnt * 8, f );

    cnt *= 2;
    uint32* ptr = (uint32*)m_data;
    for( uint j=0; j<cnt; j++ )
    {
        uint32 v = *ptr;
        *ptr++ =
            ( v >> 24 ) |
            ( ( v & 0x00FF0000 ) >> 8 ) |
            ( ( v & 0x0000FF00 ) << 8 ) |
            ( v << 24 );
    }

    fclose( f );
}

BlockData::BlockData( const BlockBitmapPtr& bitmap, uint quality )
    : m_size( bitmap->Size() )
    , m_bmp( bitmap )
    , m_done( false )
{
    assert( m_size.x%4 == 0 && m_size.y%4 == 0 );

    uint32 cnt = m_size.x * m_size.y / 16;
    DBGPRINT( cnt << " blocks" );
    m_data = new uint64[cnt];

    Process( bitmap->Data(), cnt, 0, quality, bitmap->Type() );
}

BlockData::BlockData( const v2i& size )
    : m_size( size )
    , m_done( false )
{
    assert( m_size.x%4 == 0 && m_size.y%4 == 0 );

    uint32 cnt = m_size.x * m_size.y / 16;
    DBGPRINT( cnt << " blocks" );
    m_data = new uint64[cnt];
}

BlockData::~BlockData()
{
    if( !m_done ) Finish();
    delete[] m_data;
}

void BlockData::Process( const uint8* src, uint32 blocks, size_t offset, uint quality, Channels type )
{
    uint64* dst = m_data + offset;

    std::lock_guard<std::mutex> lock( m_lock );

    if( type == Channels::Alpha )
    {
        m_work.push_back( std::async( [src, dst, blocks, this]{ ProcessBlocksAlpha( src, dst, blocks ); } ) );
    }
    else
    {
        switch( quality )
        {
        case 0:
            m_work.push_back( std::async( [src, dst, blocks, this]{ ProcessBlocksRGB( src, dst, blocks ); } ) );
            break;
        case 1:
            m_work.push_back( std::async( [src, dst, blocks, this]{ ProcessBlocksLab( src, dst, blocks ); } ) );
            break;
        default:
            assert( false );
            break;
        }
    }
}

void BlockData::Finish()
{
    assert( !m_done );
    assert( !m_work.empty() );
    for( auto& f : m_work )
    {
        f.wait();
    }
    m_done = true;
    m_work.clear();
    m_bmp.reset();
}

BitmapPtr BlockData::Decode()
{
    if( !m_done ) Finish();

    auto ret = std::make_shared<Bitmap>( m_size );

    uint32* l[4];
    l[0] = ret->Data();
    l[1] = l[0] + m_size.x;
    l[2] = l[1] + m_size.x;
    l[3] = l[2] + m_size.x;

    const uint64* src = (const uint64*)m_data;

    for( int y=0; y<m_size.y/4; y++ )
    {
        for( int x=0; x<m_size.x/4; x++ )
        {
            uint64 d = *src++;

            uint32 r1, g1, b1;
            uint32 r2, g2, b2;

            if( d & 0x2 )
            {
                int32 dr, dg, db;

                r1 = ( d & 0xF8000000 ) >> 27;
                g1 = ( d & 0x00F80000 ) >> 19;
                b1 = ( d & 0x0000F800 ) >> 11;

                dr = ( d & 0x07000000 ) >> 24;
                dg = ( d & 0x00070000 ) >> 16;
                db = ( d & 0x00000700 ) >> 8;

                if( dr & 0x4 )
                {
                    dr |= 0xFFFFFFF8;
                }
                if( dg & 0x4 )
                {
                    dg |= 0xFFFFFFF8;
                }
                if( db & 0x4 )
                {
                    db |= 0xFFFFFFF8;
                }

                r2 = r1 + dr;
                g2 = g1 + dg;
                b2 = b1 + db;

                r1 = ( r1 << 3 ) | ( r1 >> 2 );
                g1 = ( g1 << 3 ) | ( g1 >> 2 );
                b1 = ( b1 << 3 ) | ( b1 >> 2 );
                r2 = ( r2 << 3 ) | ( r2 >> 2 );
                g2 = ( g2 << 3 ) | ( g2 >> 2 );
                b2 = ( b2 << 3 ) | ( b2 >> 2 );
            }
            else
            {
                r1 = ( ( d & 0xF0000000 ) >> 24 ) | ( ( d & 0xF0000000 ) >> 28 );
                r2 = ( ( d & 0x0F000000 ) >> 20 ) | ( ( d & 0x0F000000 ) >> 24 );
                g1 = ( ( d & 0x00F00000 ) >> 16 ) | ( ( d & 0x00F00000 ) >> 20 );
                g2 = ( ( d & 0x000F0000 ) >> 12 ) | ( ( d & 0x000F0000 ) >> 16 );
                b1 = ( ( d & 0x0000F000 ) >> 8  ) | ( ( d & 0x0000F000 ) >> 12 );
                b2 = ( ( d & 0x00000F00 ) >> 4  ) | ( ( d & 0x00000F00 ) >> 8  );
            }

            uint tcw[2];
            tcw[0] = ( d & 0xE0 ) >> 5;
            tcw[1] = ( d & 0x1C ) >> 2;

            uint ra, ga, ba;
            uint rb, gb, bb;
            uint rc, gc, bc;
            uint rd, gd, bd;

            if( d & 0x1 )
            {
                int o = 0;
                for( int i=0; i<4; i++ )
                {
                    ra = clampu8( r1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 32 ) ) ) >> ( o + 32 ) ) | ( ( d & ( 1ll << ( o + 48 ) ) ) >> ( o + 47 ) ) ] );
                    ga = clampu8( g1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 32 ) ) ) >> ( o + 32 ) ) | ( ( d & ( 1ll << ( o + 48 ) ) ) >> ( o + 47 ) ) ] );
                    ba = clampu8( b1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 32 ) ) ) >> ( o + 32 ) ) | ( ( d & ( 1ll << ( o + 48 ) ) ) >> ( o + 47 ) ) ] );

                    rb = clampu8( r1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 33 ) ) ) >> ( o + 33 ) ) | ( ( d & ( 1ll << ( o + 49 ) ) ) >> ( o + 48 ) ) ] );
                    gb = clampu8( g1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 33 ) ) ) >> ( o + 33 ) ) | ( ( d & ( 1ll << ( o + 49 ) ) ) >> ( o + 48 ) ) ] );
                    bb = clampu8( b1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 33 ) ) ) >> ( o + 33 ) ) | ( ( d & ( 1ll << ( o + 49 ) ) ) >> ( o + 48 ) ) ] );

                    rc = clampu8( r2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 34 ) ) ) >> ( o + 34 ) ) | ( ( d & ( 1ll << ( o + 50 ) ) ) >> ( o + 49 ) ) ] );
                    gc = clampu8( g2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 34 ) ) ) >> ( o + 34 ) ) | ( ( d & ( 1ll << ( o + 50 ) ) ) >> ( o + 49 ) ) ] );
                    bc = clampu8( b2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 34 ) ) ) >> ( o + 34 ) ) | ( ( d & ( 1ll << ( o + 50 ) ) ) >> ( o + 49 ) ) ] );

                    rd = clampu8( r2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 35 ) ) ) >> ( o + 35 ) ) | ( ( d & ( 1ll << ( o + 51 ) ) ) >> ( o + 50 ) ) ] );
                    gd = clampu8( g2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 35 ) ) ) >> ( o + 35 ) ) | ( ( d & ( 1ll << ( o + 51 ) ) ) >> ( o + 50 ) ) ] );
                    bd = clampu8( b2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 35 ) ) ) >> ( o + 35 ) ) | ( ( d & ( 1ll << ( o + 51 ) ) ) >> ( o + 50 ) ) ] );

                    *l[0]++ = ra | ( ga << 8 ) | ( ba << 16 ) | 0xFF000000;
                    *l[1]++ = rb | ( gb << 8 ) | ( bb << 16 ) | 0xFF000000;
                    *l[2]++ = rc | ( gc << 8 ) | ( bc << 16 ) | 0xFF000000;
                    *l[3]++ = rd | ( gd << 8 ) | ( bd << 16 ) | 0xFF000000;

                    o += 4;
                }
            }
            else
            {
                int o = 0;
                for( int i=0; i<2; i++ )
                {
                    ra = clampu8( r1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 32 ) ) ) >> ( o + 32 ) ) | ( ( d & ( 1ll << ( o + 48 ) ) ) >> ( o + 47 ) ) ] );
                    ga = clampu8( g1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 32 ) ) ) >> ( o + 32 ) ) | ( ( d & ( 1ll << ( o + 48 ) ) ) >> ( o + 47 ) ) ] );
                    ba = clampu8( b1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 32 ) ) ) >> ( o + 32 ) ) | ( ( d & ( 1ll << ( o + 48 ) ) ) >> ( o + 47 ) ) ] );

                    rb = clampu8( r1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 33 ) ) ) >> ( o + 33 ) ) | ( ( d & ( 1ll << ( o + 49 ) ) ) >> ( o + 48 ) ) ] );
                    gb = clampu8( g1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 33 ) ) ) >> ( o + 33 ) ) | ( ( d & ( 1ll << ( o + 49 ) ) ) >> ( o + 48 ) ) ] );
                    bb = clampu8( b1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 33 ) ) ) >> ( o + 33 ) ) | ( ( d & ( 1ll << ( o + 49 ) ) ) >> ( o + 48 ) ) ] );

                    rc = clampu8( r1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 34 ) ) ) >> ( o + 34 ) ) | ( ( d & ( 1ll << ( o + 50 ) ) ) >> ( o + 49 ) ) ] );
                    gc = clampu8( g1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 34 ) ) ) >> ( o + 34 ) ) | ( ( d & ( 1ll << ( o + 50 ) ) ) >> ( o + 49 ) ) ] );
                    bc = clampu8( b1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 34 ) ) ) >> ( o + 34 ) ) | ( ( d & ( 1ll << ( o + 50 ) ) ) >> ( o + 49 ) ) ] );

                    rd = clampu8( r1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 35 ) ) ) >> ( o + 35 ) ) | ( ( d & ( 1ll << ( o + 51 ) ) ) >> ( o + 50 ) ) ] );
                    gd = clampu8( g1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 35 ) ) ) >> ( o + 35 ) ) | ( ( d & ( 1ll << ( o + 51 ) ) ) >> ( o + 50 ) ) ] );
                    bd = clampu8( b1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 35 ) ) ) >> ( o + 35 ) ) | ( ( d & ( 1ll << ( o + 51 ) ) ) >> ( o + 50 ) ) ] );

                    *l[0]++ = ra | ( ga << 8 ) | ( ba << 16 ) | 0xFF000000;
                    *l[1]++ = rb | ( gb << 8 ) | ( bb << 16 ) | 0xFF000000;
                    *l[2]++ = rc | ( gc << 8 ) | ( bc << 16 ) | 0xFF000000;
                    *l[3]++ = rd | ( gd << 8 ) | ( bd << 16 ) | 0xFF000000;

                    o += 4;
                }
                for( int i=0; i<2; i++ )
                {
                    ra = clampu8( r2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 32 ) ) ) >> ( o + 32 ) ) | ( ( d & ( 1ll << ( o + 48 ) ) ) >> ( o + 47 ) ) ] );
                    ga = clampu8( g2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 32 ) ) ) >> ( o + 32 ) ) | ( ( d & ( 1ll << ( o + 48 ) ) ) >> ( o + 47 ) ) ] );
                    ba = clampu8( b2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 32 ) ) ) >> ( o + 32 ) ) | ( ( d & ( 1ll << ( o + 48 ) ) ) >> ( o + 47 ) ) ] );

                    rb = clampu8( r2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 33 ) ) ) >> ( o + 33 ) ) | ( ( d & ( 1ll << ( o + 49 ) ) ) >> ( o + 48 ) ) ] );
                    gb = clampu8( g2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 33 ) ) ) >> ( o + 33 ) ) | ( ( d & ( 1ll << ( o + 49 ) ) ) >> ( o + 48 ) ) ] );
                    bb = clampu8( b2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 33 ) ) ) >> ( o + 33 ) ) | ( ( d & ( 1ll << ( o + 49 ) ) ) >> ( o + 48 ) ) ] );

                    rc = clampu8( r2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 34 ) ) ) >> ( o + 34 ) ) | ( ( d & ( 1ll << ( o + 50 ) ) ) >> ( o + 49 ) ) ] );
                    gc = clampu8( g2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 34 ) ) ) >> ( o + 34 ) ) | ( ( d & ( 1ll << ( o + 50 ) ) ) >> ( o + 49 ) ) ] );
                    bc = clampu8( b2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 34 ) ) ) >> ( o + 34 ) ) | ( ( d & ( 1ll << ( o + 50 ) ) ) >> ( o + 49 ) ) ] );

                    rd = clampu8( r2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 35 ) ) ) >> ( o + 35 ) ) | ( ( d & ( 1ll << ( o + 51 ) ) ) >> ( o + 50 ) ) ] );
                    gd = clampu8( g2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 35 ) ) ) >> ( o + 35 ) ) | ( ( d & ( 1ll << ( o + 51 ) ) ) >> ( o + 50 ) ) ] );
                    bd = clampu8( b2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 35 ) ) ) >> ( o + 35 ) ) | ( ( d & ( 1ll << ( o + 51 ) ) ) >> ( o + 50 ) ) ] );

                    *l[0]++ = ra | ( ga << 8 ) | ( ba << 16 ) | 0xFF000000;
                    *l[1]++ = rb | ( gb << 8 ) | ( bb << 16 ) | 0xFF000000;
                    *l[2]++ = rc | ( gc << 8 ) | ( bc << 16 ) | 0xFF000000;
                    *l[3]++ = rd | ( gd << 8 ) | ( bd << 16 ) | 0xFF000000;

                    o += 4;
                }
            }
        }

        l[0] += m_size.x * 3;
        l[1] += m_size.x * 3;
        l[2] += m_size.x * 3;
        l[3] += m_size.x * 3;
    }

    return ret;
}

void BlockData::WritePVR( const char* fn )
{
    FILE* f = fopen( fn, "wb+" );
    assert( f );
    const size_t len = 52 + m_size.x*m_size.y/2;
    fseek( f, len - 1, SEEK_SET );
    const char zero = 0;
    fwrite( &zero, 1, 1, f );
    fseek( f, 0, SEEK_SET );

    auto map = (uint32*)mmap( nullptr, len, PROT_WRITE, MAP_SHARED, fileno( f ), 0 );
    auto dst = map;

    *dst++ = 0x03525650;  // version
    *dst++ = 0;           // flags
    *dst++ = 6;           // pixelformat[0]
    *dst++ = 0;           // pixelformat[1]
    *dst++ = 0;           // colourspace
    *dst++ = 0;           // channel type
    *dst++ = m_size.y;    // height
    *dst++ = m_size.x;    // width
    *dst++ = 1;           // depth
    *dst++ = 1;           // num surfs
    *dst++ = 1;           // num faces
    *dst++ = 1;           // mipmap count
    *dst++ = 0;           // metadata size

    if( !m_done ) Finish();

    const uint cnt = m_size.x*m_size.y/8;
    const uint32* ptr = (uint32*)m_data;
    for( uint j=0; j<cnt; j++ )
    {
        const uint32 v = *ptr++;
        *dst++ =
            ( v >> 24 ) |
            ( ( v & 0x00FF0000 ) >> 8 ) |
            ( ( v & 0x0000FF00 ) << 8 ) |
            ( v << 24 );
    }

    munmap( map, len );
    fclose( f );
}

/*
static uint64 ProcessLab( const uint8* src )
{
    uint64 d = 0;

    Color::Lab b[4][8];
    {
        Color::Lab tmp[16];
        for( int i=0; i<16; i++ )
        {
            tmp[i] = ToLab( src + i*3 );
        }
        size_t s = sizeof( Color::Lab );
        memcpy( b[1], tmp, 8*s );
        memcpy( b[0], tmp + 8, 8*s );
        for( int i=0; i<4; i++ )
        {
            memcpy( b[3]+i*2, tmp+i*4, 2*s );
            memcpy( b[2]+i*2, tmp+i*4+2, 2*s );
        }
    }

    Color::Lab la[4];
    for( int i=0; i<4; i++ )
    {
        la[i] = Average( b[i] );
    }

    v3i a[8];
    for( int i=0; i<4; i++ )
    {
        a[i] = Color::XYZ( la[i] ).RGB();
    }
    ProcessAverages( a );

    float err[4] = {};
    for( int i=0; i<4; i++ )
    {
        err[i/2] += CalcError( b[i], a[i] );
        err[2+i/2] += CalcError( b[i], a[i+4] );
    }
    size_t idx = GetLeastError( err, 4 );

    EncodeAverages( d, a, idx );

    float terr[2][8] = {};
    uint8 tsel[8][16];
    uint8 id[16];
    for( int i=0; i<16; i++ )
    {
        id[i] = (uint8)GetBufId( i, idx );
    }
    const uint8* data = src;
    for( size_t i=0; i<16; i++ )
    {
        uint8 b = *data++;
        uint8 g = *data++;
        uint8 r = *data++;

        uint8 bid = id[i];
        const v3b& avg = a[bid];

        Color::Lab lab( v3b( r, g, b ) );

        for( int t=0; t<8; t++ )
        {
            float lerr[4] = { 0 };
            const int32* tab = g_table[t];
            for( int j=0; j<4; j++ )
            {
                v3b crgb;
                for( int k=0; k<3; k++ )
                {
                    crgb[k] = clampu8( avg[k] + tab[j] );
                }
                Color::Lab c( crgb );
                lerr[j] += sq( c.L - lab.L ) + sq( c.a - lab.a ) + sq( c.b - lab.b );
            }
            size_t lidx = GetLeastError( lerr, 4 );
            tsel[t][i] = (uint8)lidx;
            terr[bid%2][t] += lerr[lidx];
        }
    }
    size_t tidx[2];
    tidx[0] = GetLeastError( terr[0], 8 );
    tidx[1] = GetLeastError( terr[1], 8 );

    d |= tidx[0] << 2;
    d |= tidx[1] << 5;
    for( int i=0; i<16; i++ )
    {
        uint64 t = tsel[tidx[id[i]%2]][i];
        d |= ( t & 0x1 ) << ( i + 32 );
        d |= ( t & 0x2 ) << ( i + 47 );
    }

    return d;
}
*/

void BlockData::ProcessBlocksRGB( const uint8* src, uint64* dst, uint num )
{
    do
    {
        *dst++ = ProcessRGB( src );
        src += 4*4*4;
    }
    while( --num );
}

void BlockData::ProcessBlocksLab( const uint8* src, uint64* dst, uint num )
{
    do
    {
//        *dst++ = ProcessLab( src );
        src += 4*4*3;
    }
    while( --num );
}

void BlockData::ProcessBlocksAlpha( const uint8* src, uint64* dst, uint num )
{
    do
    {
        *dst++ = ProcessAlpha( src );
        src += 4*4;
    }
    while( --num );
}
