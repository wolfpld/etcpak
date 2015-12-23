#ifndef __PROCESSRGB_HPP__
#define __PROCESSRGB_HPP__

#include "Types.hpp"

uint64 ProcessRGB( const uint8* src );

#ifdef __SSE4_1__
uint64 ProcessRGB_AVX2( const uint8* src );
#endif

#endif
