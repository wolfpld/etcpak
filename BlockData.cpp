#include <assert.h>
#include <string.h>

#include "BlockData.hpp"
#include "ColorSpace.hpp"
#include "Debug.hpp"

static const int32 table[][4] = {
    {  2,  8,   -2,   -8 },
    {  5, 17,   -5,  -17 },
    {  9, 29,   -9,  -29 },
    { 13, 42,  -13,  -42 },
    { 18, 60,  -18,  -60 },
    { 24, 80,  -24,  -80 },
    { 33, 106, -33, -106 },
    { 47, 183, -47, -183 } };

static const int32 table256[][4] = {
    {  2*256,  8*256,   -2*256,   -8*256 },
    {  5*256, 17*256,   -5*256,  -17*256 },
    {  9*256, 29*256,   -9*256,  -29*256 },
    { 13*256, 42*256,  -13*256,  -42*256 },
    { 18*256, 60*256,  -18*256,  -60*256 },
    { 24*256, 80*256,  -24*256,  -80*256 },
    { 33*256, 106*256, -33*256, -106*256 },
    { 47*256, 183*256, -47*256, -183*256 } };

static v3i Average( const uint8* data )
{
    uint32 r = 0, g = 0, b = 0;
    for( int i=0; i<8; i++ )
    {
        b += *data++;
        g += *data++;
        r += *data++;
    }
    return v3i( r / 8, g / 8, b / 8 );
}

static uint Average1( const uint8* data )
{
    uint32 a = 0;
    for( int i=0; i<8; i++ )
    {
        a += *data++;
    }
    return a / 8;
}

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

static uint CalcError( const uint8* data, const v3i& average )
{
    uint err = 0;
    uint sum[3] = {};
    for( int i=0; i<8; i++ )
    {
        uint d = *data++;
        sum[0] += d;
        err += d*d;
        d = *data++;
        sum[1] += d;
        err += d*d;
        d = *data++;
        sum[2] += d;
        err += d*d;
    }
    err -= sum[0] * 2 * average.z;
    err -= sum[1] * 2 * average.y;
    err -= sum[2] * 2 * average.x;
    err += 8 * ( sq( average.x ) + sq( average.y ) + sq( average.z ) );
    return err;
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
        m_work.push_back( std::async( std::launch::async, [src, dst, blocks, this]{ ProcessBlocksAlpha( src, dst, blocks ); } ) );
    }
    else
    {
        switch( quality )
        {
        case 0:
            m_work.push_back( std::async( std::launch::async, [src, dst, blocks, this]{ ProcessBlocksRGB( src, dst, blocks ); } ) );
            break;
        case 1:
            m_work.push_back( std::async( std::launch::async, [src, dst, blocks, this]{ ProcessBlocksLab( src, dst, blocks ); } ) );
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
                    ra = clampu8( r1 + table[tcw[0]][ ( ( d & ( 1ll << ( o + 32 ) ) ) >> ( o + 32 ) ) | ( ( d & ( 1ll << ( o + 48 ) ) ) >> ( o + 47 ) ) ] );
                    ga = clampu8( g1 + table[tcw[0]][ ( ( d & ( 1ll << ( o + 32 ) ) ) >> ( o + 32 ) ) | ( ( d & ( 1ll << ( o + 48 ) ) ) >> ( o + 47 ) ) ] );
                    ba = clampu8( b1 + table[tcw[0]][ ( ( d & ( 1ll << ( o + 32 ) ) ) >> ( o + 32 ) ) | ( ( d & ( 1ll << ( o + 48 ) ) ) >> ( o + 47 ) ) ] );

                    rb = clampu8( r1 + table[tcw[0]][ ( ( d & ( 1ll << ( o + 33 ) ) ) >> ( o + 33 ) ) | ( ( d & ( 1ll << ( o + 49 ) ) ) >> ( o + 48 ) ) ] );
                    gb = clampu8( g1 + table[tcw[0]][ ( ( d & ( 1ll << ( o + 33 ) ) ) >> ( o + 33 ) ) | ( ( d & ( 1ll << ( o + 49 ) ) ) >> ( o + 48 ) ) ] );
                    bb = clampu8( b1 + table[tcw[0]][ ( ( d & ( 1ll << ( o + 33 ) ) ) >> ( o + 33 ) ) | ( ( d & ( 1ll << ( o + 49 ) ) ) >> ( o + 48 ) ) ] );

                    rc = clampu8( r2 + table[tcw[1]][ ( ( d & ( 1ll << ( o + 34 ) ) ) >> ( o + 34 ) ) | ( ( d & ( 1ll << ( o + 50 ) ) ) >> ( o + 49 ) ) ] );
                    gc = clampu8( g2 + table[tcw[1]][ ( ( d & ( 1ll << ( o + 34 ) ) ) >> ( o + 34 ) ) | ( ( d & ( 1ll << ( o + 50 ) ) ) >> ( o + 49 ) ) ] );
                    bc = clampu8( b2 + table[tcw[1]][ ( ( d & ( 1ll << ( o + 34 ) ) ) >> ( o + 34 ) ) | ( ( d & ( 1ll << ( o + 50 ) ) ) >> ( o + 49 ) ) ] );

                    rd = clampu8( r2 + table[tcw[1]][ ( ( d & ( 1ll << ( o + 35 ) ) ) >> ( o + 35 ) ) | ( ( d & ( 1ll << ( o + 51 ) ) ) >> ( o + 50 ) ) ] );
                    gd = clampu8( g2 + table[tcw[1]][ ( ( d & ( 1ll << ( o + 35 ) ) ) >> ( o + 35 ) ) | ( ( d & ( 1ll << ( o + 51 ) ) ) >> ( o + 50 ) ) ] );
                    bd = clampu8( b2 + table[tcw[1]][ ( ( d & ( 1ll << ( o + 35 ) ) ) >> ( o + 35 ) ) | ( ( d & ( 1ll << ( o + 51 ) ) ) >> ( o + 50 ) ) ] );

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
                    ra = clampu8( r1 + table[tcw[0]][ ( ( d & ( 1ll << ( o + 32 ) ) ) >> ( o + 32 ) ) | ( ( d & ( 1ll << ( o + 48 ) ) ) >> ( o + 47 ) ) ] );
                    ga = clampu8( g1 + table[tcw[0]][ ( ( d & ( 1ll << ( o + 32 ) ) ) >> ( o + 32 ) ) | ( ( d & ( 1ll << ( o + 48 ) ) ) >> ( o + 47 ) ) ] );
                    ba = clampu8( b1 + table[tcw[0]][ ( ( d & ( 1ll << ( o + 32 ) ) ) >> ( o + 32 ) ) | ( ( d & ( 1ll << ( o + 48 ) ) ) >> ( o + 47 ) ) ] );

                    rb = clampu8( r1 + table[tcw[0]][ ( ( d & ( 1ll << ( o + 33 ) ) ) >> ( o + 33 ) ) | ( ( d & ( 1ll << ( o + 49 ) ) ) >> ( o + 48 ) ) ] );
                    gb = clampu8( g1 + table[tcw[0]][ ( ( d & ( 1ll << ( o + 33 ) ) ) >> ( o + 33 ) ) | ( ( d & ( 1ll << ( o + 49 ) ) ) >> ( o + 48 ) ) ] );
                    bb = clampu8( b1 + table[tcw[0]][ ( ( d & ( 1ll << ( o + 33 ) ) ) >> ( o + 33 ) ) | ( ( d & ( 1ll << ( o + 49 ) ) ) >> ( o + 48 ) ) ] );

                    rc = clampu8( r1 + table[tcw[0]][ ( ( d & ( 1ll << ( o + 34 ) ) ) >> ( o + 34 ) ) | ( ( d & ( 1ll << ( o + 50 ) ) ) >> ( o + 49 ) ) ] );
                    gc = clampu8( g1 + table[tcw[0]][ ( ( d & ( 1ll << ( o + 34 ) ) ) >> ( o + 34 ) ) | ( ( d & ( 1ll << ( o + 50 ) ) ) >> ( o + 49 ) ) ] );
                    bc = clampu8( b1 + table[tcw[0]][ ( ( d & ( 1ll << ( o + 34 ) ) ) >> ( o + 34 ) ) | ( ( d & ( 1ll << ( o + 50 ) ) ) >> ( o + 49 ) ) ] );

                    rd = clampu8( r1 + table[tcw[0]][ ( ( d & ( 1ll << ( o + 35 ) ) ) >> ( o + 35 ) ) | ( ( d & ( 1ll << ( o + 51 ) ) ) >> ( o + 50 ) ) ] );
                    gd = clampu8( g1 + table[tcw[0]][ ( ( d & ( 1ll << ( o + 35 ) ) ) >> ( o + 35 ) ) | ( ( d & ( 1ll << ( o + 51 ) ) ) >> ( o + 50 ) ) ] );
                    bd = clampu8( b1 + table[tcw[0]][ ( ( d & ( 1ll << ( o + 35 ) ) ) >> ( o + 35 ) ) | ( ( d & ( 1ll << ( o + 51 ) ) ) >> ( o + 50 ) ) ] );

                    *l[0]++ = ra | ( ga << 8 ) | ( ba << 16 ) | 0xFF000000;
                    *l[1]++ = rb | ( gb << 8 ) | ( bb << 16 ) | 0xFF000000;
                    *l[2]++ = rc | ( gc << 8 ) | ( bc << 16 ) | 0xFF000000;
                    *l[3]++ = rd | ( gd << 8 ) | ( bd << 16 ) | 0xFF000000;

                    o += 4;
                }
                for( int i=0; i<2; i++ )
                {
                    ra = clampu8( r2 + table[tcw[1]][ ( ( d & ( 1ll << ( o + 32 ) ) ) >> ( o + 32 ) ) | ( ( d & ( 1ll << ( o + 48 ) ) ) >> ( o + 47 ) ) ] );
                    ga = clampu8( g2 + table[tcw[1]][ ( ( d & ( 1ll << ( o + 32 ) ) ) >> ( o + 32 ) ) | ( ( d & ( 1ll << ( o + 48 ) ) ) >> ( o + 47 ) ) ] );
                    ba = clampu8( b2 + table[tcw[1]][ ( ( d & ( 1ll << ( o + 32 ) ) ) >> ( o + 32 ) ) | ( ( d & ( 1ll << ( o + 48 ) ) ) >> ( o + 47 ) ) ] );

                    rb = clampu8( r2 + table[tcw[1]][ ( ( d & ( 1ll << ( o + 33 ) ) ) >> ( o + 33 ) ) | ( ( d & ( 1ll << ( o + 49 ) ) ) >> ( o + 48 ) ) ] );
                    gb = clampu8( g2 + table[tcw[1]][ ( ( d & ( 1ll << ( o + 33 ) ) ) >> ( o + 33 ) ) | ( ( d & ( 1ll << ( o + 49 ) ) ) >> ( o + 48 ) ) ] );
                    bb = clampu8( b2 + table[tcw[1]][ ( ( d & ( 1ll << ( o + 33 ) ) ) >> ( o + 33 ) ) | ( ( d & ( 1ll << ( o + 49 ) ) ) >> ( o + 48 ) ) ] );

                    rc = clampu8( r2 + table[tcw[1]][ ( ( d & ( 1ll << ( o + 34 ) ) ) >> ( o + 34 ) ) | ( ( d & ( 1ll << ( o + 50 ) ) ) >> ( o + 49 ) ) ] );
                    gc = clampu8( g2 + table[tcw[1]][ ( ( d & ( 1ll << ( o + 34 ) ) ) >> ( o + 34 ) ) | ( ( d & ( 1ll << ( o + 50 ) ) ) >> ( o + 49 ) ) ] );
                    bc = clampu8( b2 + table[tcw[1]][ ( ( d & ( 1ll << ( o + 34 ) ) ) >> ( o + 34 ) ) | ( ( d & ( 1ll << ( o + 50 ) ) ) >> ( o + 49 ) ) ] );

                    rd = clampu8( r2 + table[tcw[1]][ ( ( d & ( 1ll << ( o + 35 ) ) ) >> ( o + 35 ) ) | ( ( d & ( 1ll << ( o + 51 ) ) ) >> ( o + 50 ) ) ] );
                    gd = clampu8( g2 + table[tcw[1]][ ( ( d & ( 1ll << ( o + 35 ) ) ) >> ( o + 35 ) ) | ( ( d & ( 1ll << ( o + 51 ) ) ) >> ( o + 50 ) ) ] );
                    bd = clampu8( b2 + table[tcw[1]][ ( ( d & ( 1ll << ( o + 35 ) ) ) >> ( o + 35 ) ) | ( ( d & ( 1ll << ( o + 51 ) ) ) >> ( o + 50 ) ) ] );

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
    FILE* f = fopen( fn, "wb" );
    assert( f );

    uint32 data;
    data = 0x03525650;  // version
    fwrite( &data, 1, 4, f );
    data = 0;           // flags
    fwrite( &data, 1, 4, f );
    data = 6;           // pixelformat[0]
    fwrite( &data, 1, 4, f );
    data = 0;           // pixelformat[1]
    fwrite( &data, 1, 4, f );
    data = 0;           // colourspace
    fwrite( &data, 1, 4, f );
    data = 0;           // channel type
    fwrite( &data, 1, 4, f );
    data = m_size.y;    // height
    fwrite( &data, 1, 4, f );
    data = m_size.x;    // width
    fwrite( &data, 1, 4, f );
    data = 1;           // depth
    fwrite( &data, 1, 4, f );
    data = 1;           // num surfs
    fwrite( &data, 1, 4, f );
    data = 1;           // num faces
    fwrite( &data, 1, 4, f );
    data = 1;           // mipmap count
    fwrite( &data, 1, 4, f );
    data = 0;           // metadata size
    fwrite( &data, 1, 4, f );

    if( !m_done ) Finish();

    uint cnt = m_size.x*m_size.y/8;
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
    fwrite( m_data, 1, cnt*4, f );

    fclose( f );
}

template<class T>
static size_t GetLeastError( const T* err, size_t num )
{
    size_t idx = 0;
    for( size_t i=1; i<num; i++ )
    {
        if( err[i] < err[idx] )
        {
            idx = i;
        }
    }
    return idx;
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
            diff = std::min( std::max( diff, -4 ), 3 );

            int32 co = c1 + diff;

            a[5+i*2][j] = ( c1 << 3 ) | ( c1 >> 2 );
            a[4+i*2][j] = ( co << 3 ) | ( co >> 2 );
        }
    }
    for( int i=0; i<4; i++ )
    {
        for( int j=0; j<3; j++ )
        {
            uint32 c = a[i][j];
            a[i][j] = ( c & 0xF0 ) | ( c >> 4 );
        }
    }
}

static void ProcessAverages( uint* a )
{
    for( int i=0; i<2; i++ )
    {
        int c1 = a[i*2+1] >> 3;
        int c2 = a[i*2] >> 3;

        int diff = c2 - c1;
        diff = std::min( std::max( diff, -4 ), 3 );

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

static void EncodeAverages( uint64& d, const v3i* a, size_t idx )
{
    d |= idx;
    size_t base = idx << 1;

    if( ( idx & 0x2 ) == 0 )
    {
        for( int i=0; i<3; i++ )
        {
            d |= uint64( a[base+0][2-i] & 0xF0 ) << ( i*8 + 4 );
            d |= uint64( a[base+1][2-i] & 0xF0 ) << ( i*8 + 8 );
        }
    }
    else
    {
        for( int i=0; i<3; i++ )
        {
            d |= uint64( a[base+1][2-i] & 0xF8 ) << ( i*8 + 8 );
            int32 c = ( ( a[base+0][2-i] & 0xF8 ) - ( a[base+1][2-i] & 0xF8 ) ) >> 3;
            c &= ~0xFFFFFFF8;
            d |= ((uint64)c) << ( i*8 + 8 );
        }
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

static inline size_t GetBufId( size_t i, size_t base )
{
    assert( i < 16 );
    assert( base < 4 );
    if( base % 2 == 0 )
    {
        return base * 2 + 1 - i / 8;
    }
    else
    {
        return base * 2 + 1 - ( ( i / 2 ) % 2 );
    }
}

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
            const int32* tab = table[t];
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

static uint64 ProcessRGB( const uint8* src )
{
    uint64 d = 0;

    {
        bool solid = true;
        const uint8* ptr = src + 3;
        for( int i=1; i<16; i++ )
        {
            if( src[0] != ptr[0] || src[1] != ptr[1] || src[2] != ptr[2] )
            {
                solid = false;
                break;
            }
            ptr += 3;
        }
        if( solid )
        {
            d |= 0x2 |
                ( uint( src[0] & 0xF8 ) << 8 ) |
                ( uint( src[1] & 0xF8 ) << 16 ) |
                ( uint( src[2] & 0xF8 ) << 24 );

            return d;
        }
    }

    uint8 b23[2][24];
    const uint8* b[4] = { src+24, src, b23[0], b23[1] };

    for( int i=0; i<4; i++ )
    {
        memcpy( b23[1]+i*6, src+i*12, 6 );
        memcpy( b23[0]+i*6, src+i*12+6, 6 );
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
        err[i/2] += CalcError( b[i], a[i] );
        err[2+i/2] += CalcError( b[i], a[i+4] );
    }
    size_t idx = GetLeastError( err, 4 );

    EncodeAverages( d, a, idx );

    uint64 terr[2][8] = {};
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
        uint64* ter = terr[bid%2];

        uint8 b = *data++;
        uint8 g = *data++;
        uint8 r = *data++;

        int dr = a[bid].x - r;
        int dg = a[bid].y - g;
        int db = a[bid].z - b;

        int pix = dr * 77 + dg * 151 + db * 28;

        for( int t=0; t<8; t++ )
        {
            const int32* tab = table256[t];
            uint idx = 0;
            uint64 err = sq( tab[0] + pix );
            for( int j=1; j<4; j++ )
            {
                uint64 local = sq( uint64( tab[j] ) + pix );
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

static uint64 ProcessAlpha( const uint8* src )
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
            const int32* tab = table[t];
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

void BlockData::ProcessBlocksRGB( const uint8* src, uint64* dst, uint num )
{
    do
    {
        *dst++ = ProcessRGB( src );
        src += 4*4*3;
    }
    while( --num );
}

void BlockData::ProcessBlocksLab( const uint8* src, uint64* dst, uint num )
{
    do
    {
        *dst++ = ProcessLab( src );
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
