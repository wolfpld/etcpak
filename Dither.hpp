#ifndef __DITHER_HPP__
#define __DITHER_HPP__

#include "Types.hpp"

void InitDither();
void Dither( uint8* data );

void Swizzle(const uint8* data, const ptrdiff_t pitch, uint8* output);

#ifdef __SSE4_1__
void Dither_SSE41(const uint8* data0, const uint8* data1, uint8* output0, uint8* output1);
void Swizzle_SSE41(const uint8* data, const ptrdiff_t pitch, uint8* output0, uint8* output1);
void Dither_Swizzle_SSE41(const uint8* data, const ptrdiff_t pitch, uint8* output0, uint8* output1);
#endif

#endif
