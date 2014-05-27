#ifndef __DATAPROVIDER_HPP__
#define __DATAPROVIDER_HPP__

#include <memory>

#include "Bitmap.hpp"
#include "Types.hpp"

struct DataPart
{
    const uint32* src;
    int width;
    int lines;
    int offset;
};

class DataProvider
{
public:
    DataProvider( const char* fn, bool mipmap );
    ~DataProvider();

    int NumberOfParts() const;

    DataPart NextPart();

    bool Alpha() const { return m_bmp->Alpha(); }
    const v2i& Size() const { return m_bmp->Size(); }
    const Bitmap& ImageData() const { return *m_bmp; }

private:
    std::unique_ptr<Bitmap> m_bmp;
    int m_offset;
};

#endif
