#include <string.h>
#include <utility>

#include "BitmapDownsampled.hpp"
#include "Debug.hpp"

BitmapDownsampled::BitmapDownsampled( const Bitmap& bmp, uint lines )
    : Bitmap( bmp, lines )
{
    m_size.x = std::max( 1, bmp.Size().x / 2 );
    m_size.y = std::max( 1, bmp.Size().y / 2 );

    int w = std::max( m_size.x, 4 );
    int h = std::max( m_size.y, 4 );

    DBGPRINT( "Subbitmap " << m_size.x << "x" << m_size.y );

    m_block = m_data = new uint32[w*h];

    if( m_size.x < w || m_size.y < h )
    {
        memset( m_data, 0, w*h*sizeof( uint32 ) );
    }

    m_linesLeft = h / 4;
    m_load = std::async( [this, &bmp, w, h]() mutable
    {
        auto ptr = m_data;
        uint lines = 0;
        for( int i=0; i<h/4; i++ )
        {
            for( int j=0; j<4; j++ )
            {
            }
            lines++;
            if( lines >= m_lines )
            {
                lines = 0;
                m_sema.unlock();
            }
        }

        if( lines != 0 )
        {
            m_sema.unlock();
        }
    } );
}

BitmapDownsampled::~BitmapDownsampled()
{
}
