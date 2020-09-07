#ifndef __PROCESSCOMMON_HPP__
#define __PROCESSCOMMON_HPP__

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

template<class T>
static size_t GetLeastError( const T* err, size_t num )
{
    size_t idx = 0;
    for( size_t i=1; i<num; i++ )
    {
        if( err[i] < err[idx] )
        {
            idx = i;
        }
    }
    return idx;
}

static uint64_t FixByteOrder( uint64_t d )
{
    return ( ( d & 0x00000000FFFFFFFF ) ) |
           ( ( d & 0xFF00000000000000 ) >> 24 ) |
           ( ( d & 0x000000FF00000000 ) << 24 ) |
           ( ( d & 0x00FF000000000000 ) >> 8 ) |
           ( ( d & 0x0000FF0000000000 ) << 8 );
}

template<class T, class S>
static uint64_t EncodeSelectors( uint64_t d, const T terr[2][8], const S tsel[16][8], const uint32_t* id )
{
    size_t tidx[2];
    tidx[0] = GetLeastError( terr[0], 8 );
    tidx[1] = GetLeastError( terr[1], 8 );

    d |= tidx[0] << 26;
    d |= tidx[1] << 29;
    for( int i=0; i<16; i++ )
    {
        uint64_t t = tsel[i][tidx[id[i]%2]];
        d |= ( t & 0x1 ) << ( i + 32 );
        d |= ( t & 0x2 ) << ( i + 47 );
    }

    return d;
}


// ETCPACK math macros and definitions
#define CLAMP(ll,x,ul) (((x)<(ll)) ? (ll) : (((x)>(ul)) ? (ul) : (x)))
#define CLAMP_L(ll,x) (((x)<(ll)) ? (ll) : (x))
#define CLAMP_R(x,ul) (((x)>(ul)) ? (ul) : (x))
#define SQUARE(x) ((x)*(x))
#define SHIFT(size,startpos) ((startpos)-(size)+1)
#define MASK(size, startpos) (((2<<(size-1))-1) << SHIFT(size,startpos))
#define PUTBITS( dest, data, size, startpos) dest = ((dest & ~MASK(size, startpos)) | ((data << SHIFT(size, startpos)) & MASK(size,startpos)))
#define GETBITS(source, size, startpos)      (( (source) >> ( (startpos)    -(size)+1) ) & ((1<<(size)) -1))
#define SHIFTHIGH(size, startpos) (((startpos)-32)-(size)+1)
#define MASKHIGH(size, startpos) (((1<<(size))-1) << SHIFTHIGH(size,startpos))
#define PUTBITSHIGH(dest, data, size, startpos) dest = ((dest & ~MASKHIGH(size, startpos)) | ((data << SHIFTHIGH(size, startpos)) & MASKHIGH(size,startpos)))
#define GETBITSHIGH(source, size, startpos)  (( (source) >> (((startpos)-32)-(size)+1) ) & ((1<<(size)) -1))

// reversed RGB order for etcpak
const unsigned int R = 2;
const unsigned int G = 1;
const unsigned int B = 0;

// common T-/H-mode table
static uint8_t tableTH[8] = { 3,6,11,16,23,32,41,64 };  // 3-bit table for the 59-/58-bit T-/H-mode

// three decoding functions come from ETCPACK v2.74 and are slightly changed.
static void decompressColor(uint8_t(colors_RGB444)[2][3], uint8_t(colors)[2][3])
{
    // The color should be retrieved as:
    //
    // c = round(255/(r_bits^2-1))*comp_color
    //
    // This is similar to bit replication
    //
    // Note -- this code only work for bit replication from 4 bits and up --- 3 bits needs
    // two copy operations.
    colors[0][R] = (colors_RGB444[0][R] << 4) | colors_RGB444[0][R];
    colors[0][G] = (colors_RGB444[0][G] << 4) | colors_RGB444[0][G];
    colors[0][B] = (colors_RGB444[0][B] << 4) | colors_RGB444[0][B];
    colors[1][R] = (colors_RGB444[1][R] << 4) | colors_RGB444[1][R];
    colors[1][G] = (colors_RGB444[1][G] << 4) | colors_RGB444[1][G];
    colors[1][B] = (colors_RGB444[1][B] << 4) | colors_RGB444[1][B];
}

// calculates the paint colors from the block colors
// using a distance d and one of the H- or T-patterns.
static void calculatePaintColors59T(uint8_t d, uint8_t(colors)[2][3], uint8_t(possible_colors)[4][3])
{
    //////////////////////////////////////////////
    //
    //		C3      C1		C4----C1---C2
    //		|		|			  |
    //		|		|			  |
    //		|-------|			  |
    //		|		|			  |
    //		|		|			  |
    //		C4      C2			  C3
    //
    //////////////////////////////////////////////

    // C4
    possible_colors[3][R] = CLAMP_L(0, colors[1][R] - tableTH[d]);
    possible_colors[3][G] = CLAMP_L(0, colors[1][G] - tableTH[d]);
    possible_colors[3][B] = CLAMP_L(0, colors[1][B] - tableTH[d]);

    // C3
    possible_colors[0][R] = colors[0][R];
    possible_colors[0][G] = colors[0][G];
    possible_colors[0][B] = colors[0][B];
    // C2
    possible_colors[1][R] = CLAMP_R(colors[1][R] + tableTH[d], 255);
    possible_colors[1][G] = CLAMP_R(colors[1][G] + tableTH[d], 255);
    possible_colors[1][B] = CLAMP_R(colors[1][B] + tableTH[d], 255);
    // C1
    possible_colors[2][R] = colors[1][R];
    possible_colors[2][G] = colors[1][G];
    possible_colors[2][B] = colors[1][B];
}

static void calculatePaintColors58H(uint8_t d, uint8_t(colors)[2][3], uint8_t(possible_colors)[4][3])
{
    possible_colors[3][R] = CLAMP_L(0, colors[1][R] - tableTH[d]);
    possible_colors[3][G] = CLAMP_L(0, colors[1][G] - tableTH[d]);
    possible_colors[3][B] = CLAMP_L(0, colors[1][B] - tableTH[d]);

    // C1
    possible_colors[0][R] = CLAMP_R(colors[0][R] + tableTH[d], 255);
    possible_colors[0][G] = CLAMP_R(colors[0][G] + tableTH[d], 255);
    possible_colors[0][B] = CLAMP_R(colors[0][B] + tableTH[d], 255);
    // C2
    possible_colors[1][R] = CLAMP_L(0, colors[0][R] - tableTH[d]);
    possible_colors[1][G] = CLAMP_L(0, colors[0][G] - tableTH[d]);
    possible_colors[1][B] = CLAMP_L(0, colors[0][B] - tableTH[d]);
    // C3
    possible_colors[2][R] = CLAMP_R(colors[1][R] + tableTH[d], 255);
    possible_colors[2][G] = CLAMP_R(colors[1][G] + tableTH[d], 255);
    possible_colors[2][B] = CLAMP_R(colors[1][B] + tableTH[d], 255);
}

#endif
