#include <assert.h>
#include <string.h>

#include "BlockData.hpp"
#include "ColorSpace.hpp"
#include "CpuArch.hpp"
#include "Debug.hpp"
#include "MipMap.hpp"
#include "mmap.hpp"
#include "ProcessAlpha.hpp"
#include "ProcessRGB.hpp"
#include "ProcessRGB_AVX2.hpp"
#include "Tables.hpp"
#include "TaskDispatch.hpp"

BlockData::BlockData( const char* fn )
    : m_done( true )
    , m_file( fopen( fn, "rb" ) )
{
    assert( m_file );
    fseek( m_file, 0, SEEK_END );
    m_maplen = ftell( m_file );
    fseek( m_file, 0, SEEK_SET );
    m_data = (uint8*)mmap( nullptr, m_maplen, PROT_READ, MAP_SHARED, fileno( m_file ), 0 );

    auto data32 = (uint32*)m_data;
    if( *data32 == 0x03525650 )
    {
        m_size.y = *(data32+6);
        m_size.x = *(data32+7);
        m_dataOffset = 52 + *(data32+12);
    }
    else if( *data32 == 0x58544BAB )
    {
        m_size.x = *(data32+9);
        m_size.y = *(data32+10);
        m_dataOffset = 17 + *(data32+15);
    }
    else
    {
        assert( false );
    }
}

static uint8* OpenForWriting( const char* fn, size_t len, const v2i& size, FILE** f, int levels )
{
    *f = fopen( fn, "wb+" );
    assert( *f );
    fseek( *f, len - 1, SEEK_SET );
    const char zero = 0;
    fwrite( &zero, 1, 1, *f );
    fseek( *f, 0, SEEK_SET );

    auto ret = (uint8*)mmap( nullptr, len, PROT_WRITE, MAP_SHARED, fileno( *f ), 0 );
    auto dst = (uint32*)ret;

    *dst++ = 0x03525650;  // version
    *dst++ = 0;           // flags
    *dst++ = 6;           // pixelformat[0]
    *dst++ = 0;           // pixelformat[1]
    *dst++ = 0;           // colourspace
    *dst++ = 0;           // channel type
    *dst++ = size.y;      // height
    *dst++ = size.x;      // width
    *dst++ = 1;           // depth
    *dst++ = 1;           // num surfs
    *dst++ = 1;           // num faces
    *dst++ = levels;      // mipmap count
    *dst++ = 0;           // metadata size

    return ret;
}

static int AdjustSizeForMipmaps( const v2i& size, int levels )
{
    int len = 0;
    v2i current = size;
    for( int i=1; i<levels; i++ )
    {
        assert( current.x != 1 || current.y != 1 );
        current.x = std::max( 1, current.x / 2 );
        current.y = std::max( 1, current.y / 2 );
        len += std::max( 4, current.x ) * std::max( 4, current.y ) / 2;
    }
    assert( current.x == 1 && current.y == 1 );
    return len;
}

BlockData::BlockData( const char* fn, const v2i& size, bool mipmap )
    : m_size( size )
    , m_done( false )
    , m_dataOffset( 52 )
    , m_maplen( 52 + m_size.x*m_size.y/2 )
{
    assert( m_size.x%4 == 0 && m_size.y%4 == 0 );

    uint32 cnt = m_size.x * m_size.y / 16;
    DBGPRINT( cnt << " blocks" );

    int levels = 1;

    if( mipmap )
    {
        levels = NumberOfMipLevels( size );
        DBGPRINT( "Number of mipmaps: " << levels );
        m_maplen += AdjustSizeForMipmaps( size, levels );
    }

    m_data = OpenForWriting( fn, m_maplen, m_size, &m_file, levels );
}

BlockData::BlockData( const v2i& size, bool mipmap )
    : m_size( size )
    , m_done( false )
    , m_dataOffset( 52 )
    , m_file( nullptr )
    , m_maplen( 52 + m_size.x*m_size.y/2 )
{
    assert( m_size.x%4 == 0 && m_size.y%4 == 0 );
    if( mipmap )
    {
        const int levels = NumberOfMipLevels( size );
        m_maplen += AdjustSizeForMipmaps( size, levels );
    }
    m_data = new uint8[m_maplen];
}

BlockData::~BlockData()
{
    Finish();
    if( m_file )
    {
        munmap( m_data, m_maplen );
        fclose( m_file );
    }
    else
    {
        delete[] m_data;
    }
}

void BlockData::Process( const uint8* src, uint32 blocks, size_t offset, uint quality, Channels type )
{
    auto dst = ((uint64*)( m_data + m_dataOffset )) + offset;

    if( type == Channels::Alpha )
    {
        TaskDispatch::Queue( [src, dst, blocks, this]() mutable
        {
            do { *dst++ = ProcessAlpha( src ); src += 4*4; } while( --blocks );
            std::lock_guard<std::mutex> lock( m_lock );
            m_done = true;
            m_cv.notify_all();
        } );
    }
    else
    {
        switch( quality )
        {
        case 0:
#ifdef __SSE4_1__
            if( can_use_intel_core_4th_gen_features() )
            {
                TaskDispatch::Queue( [src, dst, blocks, this]() mutable
                {
                    do { *dst++ = ProcessRGB_AVX2( src ); src += 4*4*4; } while( --blocks );
                    std::lock_guard<std::mutex> lock( m_lock );
                    m_done = true;
                    m_cv.notify_all();
                } );
            }
            else
#endif
            {
                TaskDispatch::Queue( [src, dst, blocks, this]() mutable
                {
                    do { *dst++ = ProcessRGB( src ); src += 4*4*4; } while( --blocks );
                    std::lock_guard<std::mutex> lock( m_lock );
                    m_done = true;
                    m_cv.notify_all();
                } );
            }
            break;
        case 1:
            //m_work.push_back( std::async( [src, dst, blocks, this]{ ProcessBlocksLab( src, dst, blocks ); } ) );
            break;
        default:
            assert( false );
            break;
        }
    }
}

void BlockData::Finish()
{
    std::unique_lock<std::mutex> lock( m_lock );
    m_cv.wait( lock, [this]{ return m_done; } );
    m_bmp.reset();
}

struct BlockColor
{
    uint32 r1, g1, b1;
    uint32 r2, g2, b2;
};

static void DecodeBlockColor( uint64 d, BlockColor& c )
{
    if( d & 0x2 )
    {
        int32 dr, dg, db;

        c.r1 = ( d & 0xF8000000 ) >> 27;
        c.g1 = ( d & 0x00F80000 ) >> 19;
        c.b1 = ( d & 0x0000F800 ) >> 11;

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

        c.r2 = c.r1 + dr;
        c.g2 = c.g1 + dg;
        c.b2 = c.b1 + db;

        c.r1 = ( c.r1 << 3 ) | ( c.r1 >> 2 );
        c.g1 = ( c.g1 << 3 ) | ( c.g1 >> 2 );
        c.b1 = ( c.b1 << 3 ) | ( c.b1 >> 2 );
        c.r2 = ( c.r2 << 3 ) | ( c.r2 >> 2 );
        c.g2 = ( c.g2 << 3 ) | ( c.g2 >> 2 );
        c.b2 = ( c.b2 << 3 ) | ( c.b2 >> 2 );
    }
    else
    {
        c.r1 = ( ( d & 0xF0000000 ) >> 24 ) | ( ( d & 0xF0000000 ) >> 28 );
        c.r2 = ( ( d & 0x0F000000 ) >> 20 ) | ( ( d & 0x0F000000 ) >> 24 );
        c.g1 = ( ( d & 0x00F00000 ) >> 16 ) | ( ( d & 0x00F00000 ) >> 20 );
        c.g2 = ( ( d & 0x000F0000 ) >> 12 ) | ( ( d & 0x000F0000 ) >> 16 );
        c.b1 = ( ( d & 0x0000F000 ) >> 8  ) | ( ( d & 0x0000F000 ) >> 12 );
        c.b2 = ( ( d & 0x00000F00 ) >> 4  ) | ( ( d & 0x00000F00 ) >> 8  );
    }
}

BitmapPtr BlockData::Decode()
{
    Finish();

    auto ret = std::make_shared<Bitmap>( m_size );

    uint32* l[4];
    l[0] = ret->Data();
    l[1] = l[0] + m_size.x;
    l[2] = l[1] + m_size.x;
    l[3] = l[2] + m_size.x;

    const uint64* src = (const uint64*)( m_data + m_dataOffset );

    for( int y=0; y<m_size.y/4; y++ )
    {
        for( int x=0; x<m_size.x/4; x++ )
        {
            uint64 d = *src++;

            d = ( ( d & 0xFF000000FF000000 ) >> 24 ) |
                ( ( d & 0x000000FF000000FF ) << 24 ) |
                ( ( d & 0x00FF000000FF0000 ) >> 8 ) |
                ( ( d & 0x0000FF000000FF00 ) << 8 );

            BlockColor c;
            DecodeBlockColor( d, c );

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
                    ra = clampu8( c.r1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 32 ) ) ) >> ( o + 32 ) ) | ( ( d & ( 1ll << ( o + 48 ) ) ) >> ( o + 47 ) ) ] );
                    ga = clampu8( c.g1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 32 ) ) ) >> ( o + 32 ) ) | ( ( d & ( 1ll << ( o + 48 ) ) ) >> ( o + 47 ) ) ] );
                    ba = clampu8( c.b1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 32 ) ) ) >> ( o + 32 ) ) | ( ( d & ( 1ll << ( o + 48 ) ) ) >> ( o + 47 ) ) ] );

                    rb = clampu8( c.r1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 33 ) ) ) >> ( o + 33 ) ) | ( ( d & ( 1ll << ( o + 49 ) ) ) >> ( o + 48 ) ) ] );
                    gb = clampu8( c.g1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 33 ) ) ) >> ( o + 33 ) ) | ( ( d & ( 1ll << ( o + 49 ) ) ) >> ( o + 48 ) ) ] );
                    bb = clampu8( c.b1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 33 ) ) ) >> ( o + 33 ) ) | ( ( d & ( 1ll << ( o + 49 ) ) ) >> ( o + 48 ) ) ] );

                    rc = clampu8( c.r2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 34 ) ) ) >> ( o + 34 ) ) | ( ( d & ( 1ll << ( o + 50 ) ) ) >> ( o + 49 ) ) ] );
                    gc = clampu8( c.g2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 34 ) ) ) >> ( o + 34 ) ) | ( ( d & ( 1ll << ( o + 50 ) ) ) >> ( o + 49 ) ) ] );
                    bc = clampu8( c.b2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 34 ) ) ) >> ( o + 34 ) ) | ( ( d & ( 1ll << ( o + 50 ) ) ) >> ( o + 49 ) ) ] );

                    rd = clampu8( c.r2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 35 ) ) ) >> ( o + 35 ) ) | ( ( d & ( 1ll << ( o + 51 ) ) ) >> ( o + 50 ) ) ] );
                    gd = clampu8( c.g2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 35 ) ) ) >> ( o + 35 ) ) | ( ( d & ( 1ll << ( o + 51 ) ) ) >> ( o + 50 ) ) ] );
                    bd = clampu8( c.b2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 35 ) ) ) >> ( o + 35 ) ) | ( ( d & ( 1ll << ( o + 51 ) ) ) >> ( o + 50 ) ) ] );

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
                    ra = clampu8( c.r1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 32 ) ) ) >> ( o + 32 ) ) | ( ( d & ( 1ll << ( o + 48 ) ) ) >> ( o + 47 ) ) ] );
                    ga = clampu8( c.g1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 32 ) ) ) >> ( o + 32 ) ) | ( ( d & ( 1ll << ( o + 48 ) ) ) >> ( o + 47 ) ) ] );
                    ba = clampu8( c.b1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 32 ) ) ) >> ( o + 32 ) ) | ( ( d & ( 1ll << ( o + 48 ) ) ) >> ( o + 47 ) ) ] );

                    rb = clampu8( c.r1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 33 ) ) ) >> ( o + 33 ) ) | ( ( d & ( 1ll << ( o + 49 ) ) ) >> ( o + 48 ) ) ] );
                    gb = clampu8( c.g1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 33 ) ) ) >> ( o + 33 ) ) | ( ( d & ( 1ll << ( o + 49 ) ) ) >> ( o + 48 ) ) ] );
                    bb = clampu8( c.b1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 33 ) ) ) >> ( o + 33 ) ) | ( ( d & ( 1ll << ( o + 49 ) ) ) >> ( o + 48 ) ) ] );

                    rc = clampu8( c.r1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 34 ) ) ) >> ( o + 34 ) ) | ( ( d & ( 1ll << ( o + 50 ) ) ) >> ( o + 49 ) ) ] );
                    gc = clampu8( c.g1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 34 ) ) ) >> ( o + 34 ) ) | ( ( d & ( 1ll << ( o + 50 ) ) ) >> ( o + 49 ) ) ] );
                    bc = clampu8( c.b1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 34 ) ) ) >> ( o + 34 ) ) | ( ( d & ( 1ll << ( o + 50 ) ) ) >> ( o + 49 ) ) ] );

                    rd = clampu8( c.r1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 35 ) ) ) >> ( o + 35 ) ) | ( ( d & ( 1ll << ( o + 51 ) ) ) >> ( o + 50 ) ) ] );
                    gd = clampu8( c.g1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 35 ) ) ) >> ( o + 35 ) ) | ( ( d & ( 1ll << ( o + 51 ) ) ) >> ( o + 50 ) ) ] );
                    bd = clampu8( c.b1 + g_table[tcw[0]][ ( ( d & ( 1ll << ( o + 35 ) ) ) >> ( o + 35 ) ) | ( ( d & ( 1ll << ( o + 51 ) ) ) >> ( o + 50 ) ) ] );

                    *l[0]++ = ra | ( ga << 8 ) | ( ba << 16 ) | 0xFF000000;
                    *l[1]++ = rb | ( gb << 8 ) | ( bb << 16 ) | 0xFF000000;
                    *l[2]++ = rc | ( gc << 8 ) | ( bc << 16 ) | 0xFF000000;
                    *l[3]++ = rd | ( gd << 8 ) | ( bd << 16 ) | 0xFF000000;

                    o += 4;
                }
                for( int i=0; i<2; i++ )
                {
                    ra = clampu8( c.r2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 32 ) ) ) >> ( o + 32 ) ) | ( ( d & ( 1ll << ( o + 48 ) ) ) >> ( o + 47 ) ) ] );
                    ga = clampu8( c.g2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 32 ) ) ) >> ( o + 32 ) ) | ( ( d & ( 1ll << ( o + 48 ) ) ) >> ( o + 47 ) ) ] );
                    ba = clampu8( c.b2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 32 ) ) ) >> ( o + 32 ) ) | ( ( d & ( 1ll << ( o + 48 ) ) ) >> ( o + 47 ) ) ] );

                    rb = clampu8( c.r2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 33 ) ) ) >> ( o + 33 ) ) | ( ( d & ( 1ll << ( o + 49 ) ) ) >> ( o + 48 ) ) ] );
                    gb = clampu8( c.g2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 33 ) ) ) >> ( o + 33 ) ) | ( ( d & ( 1ll << ( o + 49 ) ) ) >> ( o + 48 ) ) ] );
                    bb = clampu8( c.b2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 33 ) ) ) >> ( o + 33 ) ) | ( ( d & ( 1ll << ( o + 49 ) ) ) >> ( o + 48 ) ) ] );

                    rc = clampu8( c.r2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 34 ) ) ) >> ( o + 34 ) ) | ( ( d & ( 1ll << ( o + 50 ) ) ) >> ( o + 49 ) ) ] );
                    gc = clampu8( c.g2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 34 ) ) ) >> ( o + 34 ) ) | ( ( d & ( 1ll << ( o + 50 ) ) ) >> ( o + 49 ) ) ] );
                    bc = clampu8( c.b2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 34 ) ) ) >> ( o + 34 ) ) | ( ( d & ( 1ll << ( o + 50 ) ) ) >> ( o + 49 ) ) ] );

                    rd = clampu8( c.r2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 35 ) ) ) >> ( o + 35 ) ) | ( ( d & ( 1ll << ( o + 51 ) ) ) >> ( o + 50 ) ) ] );
                    gd = clampu8( c.g2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 35 ) ) ) >> ( o + 35 ) ) | ( ( d & ( 1ll << ( o + 51 ) ) ) >> ( o + 50 ) ) ] );
                    bd = clampu8( c.b2 + g_table[tcw[1]][ ( ( d & ( 1ll << ( o + 35 ) ) ) >> ( o + 35 ) ) | ( ( d & ( 1ll << ( o + 51 ) ) ) >> ( o + 50 ) ) ] );

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

// Block type:
//  red - 2x4, green - 4x2
//  dark - 444, bright - 555 + 333
void BlockData::Dissect()
{
    auto size = m_size / 4;
    const uint64* data = (const uint64*)( m_data + m_dataOffset );

    auto src = data;

    auto bmp = std::make_shared<Bitmap>( size );
    auto dst = bmp->Data();

    auto bmp2 = std::make_shared<Bitmap>( m_size );
    uint32* l[4];
    l[0] = bmp2->Data();
    l[1] = l[0] + m_size.x;
    l[2] = l[1] + m_size.x;
    l[3] = l[2] + m_size.x;

    auto bmp3 = std::make_shared<Bitmap>( size );
    auto dst3 = bmp3->Data();

    for( int y=0; y<size.y; y++ )
    {
        for( int x=0; x<size.x; x++ )
        {
            uint64 d = *src++;

            d = ( ( d & 0xFF000000FF000000 ) >> 24 ) |
                ( ( d & 0x000000FF000000FF ) << 24 ) |
                ( ( d & 0x00FF000000FF0000 ) >> 8 ) |
                ( ( d & 0x0000FF000000FF00 ) << 8 );

            switch( d & 0x3 )
            {
            case 0:
                *dst++ = 0xFF000088;
                break;
            case 1:
                *dst++ = 0xFF008800;
                break;
            case 2:
                *dst++ = 0xFF0000FF;
                break;
            case 3:
                *dst++ = 0xFF00FF00;
                break;
            default:
                assert( false );
                break;
            }

            uint tcw[2];
            tcw[0] = ( d & 0xE0 );
            tcw[1] = ( d & 0x1C ) << 3;

            *dst3++ = 0xFF000000 | ( tcw[0] << 8 ) | ( tcw[1] );

            BlockColor c;
            DecodeBlockColor( d, c );

            if( d & 0x1 )
            {
                for( int i=0; i<4; i++ )
                {
                    *l[0]++ = 0xFF000000 | ( c.b1 << 16 ) | ( c.g1 << 8 ) | c.r1;
                    *l[1]++ = 0xFF000000 | ( c.b1 << 16 ) | ( c.g1 << 8 ) | c.r1;
                    *l[2]++ = 0xFF000000 | ( c.b2 << 16 ) | ( c.g2 << 8 ) | c.r2;
                    *l[3]++ = 0xFF000000 | ( c.b2 << 16 ) | ( c.g2 << 8 ) | c.r2;
                }
            }
            else
            {
                for( int i=0; i<2; i++ )
                {
                    *l[0]++ = 0xFF000000 | ( c.b1 << 16 ) | ( c.g1 << 8 ) | c.r1;
                    *l[1]++ = 0xFF000000 | ( c.b1 << 16 ) | ( c.g1 << 8 ) | c.r1;
                    *l[2]++ = 0xFF000000 | ( c.b1 << 16 ) | ( c.g1 << 8 ) | c.r1;
                    *l[3]++ = 0xFF000000 | ( c.b1 << 16 ) | ( c.g1 << 8 ) | c.r1;
                }
                for( int i=0; i<2; i++ )
                {
                    *l[0]++ = 0xFF000000 | ( c.b2 << 16 ) | ( c.g2 << 8 ) | c.r2;
                    *l[1]++ = 0xFF000000 | ( c.b2 << 16 ) | ( c.g2 << 8 ) | c.r2;
                    *l[2]++ = 0xFF000000 | ( c.b2 << 16 ) | ( c.g2 << 8 ) | c.r2;
                    *l[3]++ = 0xFF000000 | ( c.b2 << 16 ) | ( c.g2 << 8 ) | c.r2;
                }
            }
        }
        l[0] += m_size.x * 3;
        l[1] += m_size.x * 3;
        l[2] += m_size.x * 3;
        l[3] += m_size.x * 3;
    }

    bmp->Write( "out_block_type.png" );
    bmp2->Write( "out_block_color.png" );
    bmp3->Write( "out_block_selectors.png" );
}
