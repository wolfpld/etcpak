#include <assert.h>
#include <utility>

#include "BlockBitmap.hpp"
#include "Dither.hpp"

BlockBitmap::BlockBitmap( const uint32* data, const v2i& size, Channels type )
    : m_data( new uint8[std::max( 4, size.x ) * std::max( 4, size.y ) * ( type == Channels::RGB ? 4 : 1 )] )
    , m_size( size )
    , m_type( type )
{
    Process( data );
}

BlockBitmap::BlockBitmap( const BitmapPtr& bmp, Channels type )
    : m_data( new uint8[std::max( 4, bmp->Size().x ) * std::max( 4, bmp->Size().y ) * ( type == Channels::RGB ? 4 : 1 )] )
    , m_size( bmp->Size() )
    , m_type( type )
{
    Process( bmp->Data() );
}

void BlockBitmap::Process( const uint32* __restrict src )
{
    uint8* __restrict dst = m_data;

    int w = std::max( 4, m_size.x );
    int h = std::max( 4, m_size.y );

    assert( w % 4 == 0 && h % 4 == 0 );

    if( m_type == Channels::RGB )
    {
        for( int by=0; by<h/4; by++ )
        {
            for( int bx=0; bx<w/4; bx++ )
            {
                for( int x=0; x<4; x++ )
                {
                    for( int y=0; y<4; y++ )
                    {
                        const uint32 c = *src;
                        src += m_size.x;
                        *dst++ = ( c & 0x00FF0000 ) >> 16;
                        *dst++ = ( c & 0x0000FF00 ) >> 8;
                        *dst++ =   c & 0x000000FF;
                        *dst++ = 0;
                    }
                    src -= m_size.x * 4 - 1;
                }
            }
            src += m_size.x * 3;
        }
    }
    else
    {
        for( int by=0; by<h/4; by++ )
        {
            for( int bx=0; bx<w/4; bx++ )
            {
                for( int x=0; x<4; x++ )
                {
                    for( int y=0; y<4; y++ )
                    {
                        *dst++ = *src >> 24;
                        src += m_size.x;
                    }
                    src -= m_size.x * 4 - 1;
                }
            }
            src += m_size.x * 3;
        }
    }
}

BlockBitmap::~BlockBitmap()
{
    delete[] m_data;
}

void BlockBitmap::Dither()
{
    int w = std::max( 4, m_size.x );
    int h = std::max( 4, m_size.y );

    uint8* ptr = m_data;

    if( m_type == Channels::RGB )
    {
        for( int by=0; by<h/4; by++ )
        {
            for( int bx=0; bx<w/4; bx++ )
            {
                DitherRGB( ptr );
                ptr += 64;
            }
        }
    }
    else
    {
        for( int by=0; by<h/4; by++ )
        {
            for( int bx=0; bx<w/4; bx++ )
            {
                DitherA( ptr );
                ptr += 16;
            }
        }
    }
}
