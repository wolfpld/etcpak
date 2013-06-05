#ifndef __ERROR_HPP__
#define __ERROR_HPP__

#include "Bitmap.hpp"

float CalcMSE3( const BitmapPtr& bmp, const BitmapPtr& out );
float CalcMSE1( const BitmapPtr& bmp, const BitmapPtr& out );

#endif
