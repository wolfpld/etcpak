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
    BlockData( const BlockBitmapPtr& bitmap, uint quality );
    ~BlockData();

    BitmapPtr Decode();

private:
    void ProcessBlocksRGB( const uint8* src, uint64* dst, uint num );
    void ProcessBlocksLab( const uint8* src, uint64* dst, uint num );

    uint64* m_data;
    v2i m_size;
};

typedef std::shared_ptr<BlockData> BlockDataPtr;

#endif
