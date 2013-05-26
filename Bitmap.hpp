#ifndef __DARKRL__BITMAP_HPP__
#define __DARKRL__BITMAP_HPP__

#include <memory>

#include "Types.hpp"
#include "Vector.hpp"

class Bitmap
{
public:
    Bitmap( const char* fn );
    ~Bitmap();

    void Write( const char* fn );

    const uint32* Data() const { return m_data; }
    const v2i& Size() const { return m_size; }

private:
    uint32* m_data;
    v2i m_size;
};

typedef std::shared_ptr<Bitmap> BitmapPtr;

#endif
