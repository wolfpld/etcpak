#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "libpng/png.h"

#include "Bitmap.hpp"
#include "Debug.hpp"

Bitmap::Bitmap( const char* fn )
    : m_alpha( true )
{
    FILE* f = fopen( fn, "rb" );
    assert( f );

    unsigned int sig_read = 0;
    int bit_depth, color_type, interlace_type;

    png_structp png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
    png_infop info_ptr = png_create_info_struct( png_ptr );
    setjmp( png_jmpbuf( png_ptr ) );

    png_init_io( png_ptr, f );
    png_set_sig_bytes( png_ptr, sig_read );

    png_uint_32 w, h;

    png_read_info( png_ptr, info_ptr );
    png_get_IHDR( png_ptr, info_ptr, &w, &h, &bit_depth, &color_type, &interlace_type, NULL, NULL );

    m_size = v2i( w, h );

    png_set_strip_16( png_ptr );
    if( color_type == PNG_COLOR_TYPE_PALETTE )
    {
        png_set_palette_to_rgb( png_ptr );
    }
    else if( color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8 )
    {
        png_set_expand_gray_1_2_4_to_8( png_ptr );
    }
    if( png_get_valid( png_ptr, info_ptr, PNG_INFO_tRNS ) )
    {
        png_set_tRNS_to_alpha( png_ptr );
    }
    if( color_type == PNG_COLOR_TYPE_GRAY_ALPHA )
    {
        png_set_gray_to_rgb(png_ptr);
    }

    switch( color_type )
    {
    case PNG_COLOR_TYPE_PALETTE:
        if( !png_get_valid( png_ptr, info_ptr, PNG_INFO_tRNS ) )
        {
            png_set_filler( png_ptr, 0xff, PNG_FILLER_AFTER );
            m_alpha = false;
        }
        break;
    case PNG_COLOR_TYPE_GRAY_ALPHA:
        png_set_gray_to_rgb( png_ptr );
        break;
    case PNG_COLOR_TYPE_RGB:
        png_set_filler( png_ptr, 0xff, PNG_FILLER_AFTER );
        m_alpha = false;
        break;
    default:
        break;
    }

    DBGPRINT( "Bitmap " << fn << "  " << w << "x" << h );

    m_data = new uint32[w*h];
    uint32* ptr = m_data;
    while( h-- )
    {
        png_read_rows( png_ptr, (png_bytepp)&ptr, NULL, 1 );
        ptr += w;
    }

    png_read_end( png_ptr, info_ptr );

    png_destroy_read_struct( &png_ptr, &info_ptr, NULL );
    fclose( f );
}

Bitmap::Bitmap( const v2i& size )
    : m_data( new uint32[size.x*size.y] )
    , m_size( size )
{
}

Bitmap::~Bitmap()
{
    delete[] m_data;
}

void Bitmap::Write( const char* fn )
{
    FILE* f = fopen( fn, "wb" );
    assert( f );

    png_structp png_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
    png_infop info_ptr = png_create_info_struct( png_ptr );
    setjmp( png_jmpbuf( png_ptr ) );
    png_init_io( png_ptr, f );

    png_set_IHDR( png_ptr, info_ptr, m_size.x, m_size.y, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE );

    png_write_info( png_ptr, info_ptr );

    uint32* ptr = m_data;
    for( int i=0; i<m_size.y; i++ )
    {
        png_write_rows( png_ptr, (png_bytepp)(&ptr), 1 );
        ptr += m_size.x;
    }

    png_write_end( png_ptr, info_ptr );
    png_destroy_write_struct( &png_ptr, &info_ptr );

    fclose( f );
}
