#ifndef __DITHER_HPP__
#define __DITHER_HPP__

#include "Types.hpp"

void InitDither();
void DitherRGB( uint8* data );
void DitherA( uint8* data );

#endif
