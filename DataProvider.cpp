#include <assert.h>

#include "DataProvider.hpp"
#include "MipMap.hpp"

enum { Lines = 32 };

DataProvider::DataProvider( const char* fn, bool mipmap )
    : m_offset( 0 )
    , m_mipmap( mipmap )
{
    m_bmp.emplace_back( new Bitmap( fn, Lines ) );
    m_current = m_bmp[0].get();
}

DataProvider::~DataProvider()
{
}

int DataProvider::NumberOfParts() const
{
    return ( ( m_bmp->Size().y / 4 ) + Lines - 1 ) / Lines;
}

DataPart DataProvider::NextPart()
{
    uint lines = Lines;
    bool done;

    DataPart ret = {
        m_current->NextBlock( lines, done ),
        m_current->Size().x,
        lines,
        m_offset
    };

    m_offset += m_current->Size().x / 4 * lines;

    return ret;
}
