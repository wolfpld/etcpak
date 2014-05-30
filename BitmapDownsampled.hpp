#ifndef __DARKRL__BITMAPDOWNSAMPLED_HPP__
#define __DARKRL__BITMAPDOWNSAMPLED_HPP__

#include "Bitmap.hpp"
#include "Types.hpp"

class BitmapDownsampled : public Bitmap
{
public:
    BitmapDownsampled( const Bitmap& bmp, uint lines );
    ~BitmapDownsampled();
};

#endif
