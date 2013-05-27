#ifndef __BLOCKDATA_HPP__
#define __BLOCKDATA_HPP__

#include <memory>

#include "BlockBitmap.hpp"
#include "Bitmap.hpp"
#include "Types.hpp"
#include "Vector.hpp"

class BlockData
{
public:
    BlockData( const BlockBitmapPtr& bitmap, bool perc );
    ~BlockData();

    BitmapPtr Decode();

private:
    void ProcessBlocks( const uint8* src, uint64* dst, uint num );

    uint64* m_data;
    v2i m_size;
    bool m_perc;
};

typedef std::shared_ptr<BlockData> BlockDataPtr;

#endif