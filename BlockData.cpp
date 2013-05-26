#include "BlockData.hpp"

BlockData::BlockData( const BlockBitmapPtr& bitmap )
    : m_data( new uint8[bitmap->Size().x * bitmap->Size().y / 2] )
    , m_size( bitmap->Size() )
{
}

BlockData::~BlockData()
{
    delete[] m_data;
}

BitmapPtr BlockData::Decode()
{
    auto ret = std::make_shared<Bitmap>( m_size );
    return ret;
}
