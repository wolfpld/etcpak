#include "DataProvider.hpp"

enum { Lines = 32 };

DataProvider::DataProvider( const char* fn, bool mipmap )
    : m_bmp( new Bitmap( fn, Lines ) )
    , m_offset( 0 )
    , m_mipmap( mipmap )
{
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
        m_bmp->NextBlock( lines, done ),
        m_bmp->Size().x,
        lines,
        m_offset
    };

    m_offset += m_bmp->Size().x / 4 * lines;

    return ret;
}
