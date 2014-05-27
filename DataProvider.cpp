#include <assert.h>
#include <utility>

#include "BitmapDownsampled.hpp"
#include "DataProvider.hpp"
#include "MipMap.hpp"

enum { Lines = 32 };

DataProvider::DataProvider( const char* fn, bool mipmap )
    : m_offset( 0 )
    , m_mipmap( mipmap )
    , m_done( false )
{
    m_bmp.emplace_back( new Bitmap( fn, Lines ) );
    m_current = m_bmp[0].get();
}

DataProvider::~DataProvider()
{
}

int DataProvider::NumberOfParts() const
{
    int parts = ( ( m_bmp[0]->Size().y / 4 ) + Lines - 1 ) / Lines;

    if( m_mipmap )
    {
        v2i current = m_bmp[0]->Size();
        int levels = NumberOfMipLevels( current );
        for( int i=1; i<levels; i++ )
        {
            assert( current.x != 1 && current.y != 1 );
            current.x = std::max( 1, current.x / 2 );
            current.y = std::max( 1, current.y / 2 );
            parts += ( ( std::max( 4, current.y ) / 4 ) + Lines - 1 ) / Lines;
        }
        assert( current.x == 1 && current.y == 1 );
    }

    return parts;
}

DataPart DataProvider::NextPart()
{
    assert( !m_done );

    uint lines = Lines;
    bool done;

    DataPart ret = {
        m_current->NextBlock( lines, done ),
        std::max( 4, m_current->Size().x ),
        lines,
        m_offset
    };

    m_offset += m_current->Size().x / 4 * lines;

    if( done )
    {
        if( m_current->Size().x != 1 || m_current->Size().y != 1 )
        {
            m_bmp.emplace_back( new BitmapDownsampled( *m_current, Lines ) );
            m_current = m_bmp[m_bmp.size()-1].get();
        }
        else
        {
            m_done = true;
        }
    }

    return ret;
}
