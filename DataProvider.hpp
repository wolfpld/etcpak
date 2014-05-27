#ifndef __DATAPROVIDER_HPP__
#define __DATAPROVIDER_HPP__

#include <memory>
#include <vector>

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

    bool Alpha() const { return m_bmp[0]->Alpha(); }
    const v2i& Size() const { return m_bmp[0]->Size(); }
    const Bitmap& ImageData() const { return *m_bmp[0]; }

private:
    std::vector<std::unique_ptr<Bitmap>> m_bmp;
    Bitmap* m_current;
    int m_offset;
    bool m_mipmap;
    bool m_done;
};

#endif
