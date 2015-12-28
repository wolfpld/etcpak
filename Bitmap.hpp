#ifndef __DARKRL__BITMAP_HPP__
#define __DARKRL__BITMAP_HPP__

#include <future>
#include <memory>
#include <mutex>

#include "Semaphore.hpp"
#include "Types.hpp"
#include "Vector.hpp"

enum class Channels
{
    RGB,
    Alpha
};

class Bitmap
{
public:
    Bitmap( const char* fn, uint lines );
    Bitmap( const v2i& size );
    virtual ~Bitmap();

    void Write( const char* fn );

    uint32* Data() { if( m_load.valid() ) m_load.wait(); return m_data; }
    const uint32* Data() const { if( m_load.valid() ) m_load.wait(); return m_data; }
    const v2i& Size() const { return m_size; }
    bool Alpha() const { return m_alpha; }

    const uint32* NextBlock( uint& lines, bool& done );

protected:
    Bitmap( const Bitmap& src, uint lines );

    uint32* m_data;
    uint32* m_block;
    uint m_lines;
    uint m_linesLeft;
    v2i m_size;
    bool m_alpha;
    Semaphore m_sema;
    std::mutex m_lock;
    std::future<void> m_load;
};

typedef std::shared_ptr<Bitmap> BitmapPtr;

#endif
