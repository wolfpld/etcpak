#include <assert.h>

#include "BlockBitmap.hpp"

BlockBitmap::BlockBitmap( const uint32* data, const v2i& size, Channels type )
    : m_data( new uint8[size.x * size.y * ( type == Channels::RGB ? 4 : 1 )] )
    , m_size( size )
    , m_type( type )
{
    assert( m_size.x % 4 == 0 && m_size.y % 4 == 0 );

    Process( data );
}

BlockBitmap::BlockBitmap( const BitmapPtr& bmp, Channels type )
    : m_data( new uint8[bmp->Size().x * bmp->Size().y * ( type == Channels::RGB ? 4 : 1 )] )
    , m_size( bmp->Size() )
    , m_type( type )
{
    Process( bmp->Data() );
}

void BlockBitmap::Process( const uint32* src )
{
    uint8* dst = m_data;

    assert( m_size.x % 4 == 0 && m_size.y % 4 == 0 );

    if( m_type == Channels::RGB )
    {
        for( int by=0; by<m_size.y/4; by++ )
        {
            for( int bx=0; bx<m_size.x/4; bx++ )
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
        for( int by=0; by<m_size.y/4; by++ )
        {
            for( int bx=0; bx<m_size.x/4; bx++ )
            {
                for( int x=0; x<4; x++ )
                {
                    for( int y=0; y<4; y++ )
                    {
                        *dst++ = 255 - ( *src >> 24 );
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
