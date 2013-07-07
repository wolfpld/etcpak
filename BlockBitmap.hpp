#ifndef __BLOCKBITMAP_HPP__
#define __BLOCKBITMAP_HPP__

#include <memory>

#include "Bitmap.hpp"
#include "Types.hpp"
#include "Vector.hpp"

enum class Channels
{
    RGB,
    Alpha
};

class BlockBitmap
{
public:
    BlockBitmap( const uint32* data, const v2i& size, Channels type );
    BlockBitmap( const BitmapPtr& bmp, Channels type );
    ~BlockBitmap();

    const uint8* Data() const { return m_data; }
    const v2i& Size() const { return m_size; }
    const Channels Type() const { return m_type; }

private:
    void Process( const uint32* src );

    uint8* m_data;
    v2i m_size;
    Channels m_type;
};

typedef std::shared_ptr<BlockBitmap> BlockBitmapPtr;

#endif
