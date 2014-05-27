#include "Error.hpp"
#include "Math.hpp"

float CalcMSE3( const Bitmap& bmp, const Bitmap& out )
{
    float err = 0;

    const uint32* p1 = bmp.Data();
    const uint32* p2 = out.Data();
    size_t cnt = bmp.Size().x * bmp.Size().y;

    for( size_t i=0; i<cnt; i++ )
    {
        uint32 c1 = *p1++;
        uint32 c2 = *p2++;

        err += sq( ( c1 & 0x000000FF ) - ( c2 & 0x000000FF ) );
        err += sq( ( ( c1 & 0x0000FF00 ) >> 8 ) - ( ( c2 & 0x0000FF00 ) >> 8 ) );
        err += sq( ( ( c1 & 0x00FF0000 ) >> 16 ) - ( ( c2 & 0x00FF0000 ) >> 16 ) );
    }

    err /= cnt * 3;

    return err;
}

float CalcMSE1( const Bitmap& bmp, const Bitmap& out )
{
    float err = 0;

    const uint32* p1 = bmp.Data();
    const uint32* p2 = out.Data();
    size_t cnt = bmp.Size().x * bmp.Size().y;

    for( size_t i=0; i<cnt; i++ )
    {
        uint32 c1 = *p1++;
        uint32 c2 = *p2++;

        err += sq( ( c1 >> 24 ) - ( c2 & 0xFF ) );
    }

    err /= cnt;

    return err;
}
