#include <string.h>
#include <utility>

#include "BitmapDownsampled.hpp"
#include "Debug.hpp"

#if defined __SSE4_1__ || defined __AVX2__ || defined _MSC_VER
#  ifdef _MSC_VER
#    include <intrin.h>
#  else
#    include <x86intrin.h>
#  endif
#endif


BitmapDownsampled::BitmapDownsampled( const Bitmap& bmp, unsigned int lines )
    : Bitmap( bmp, lines )
{
    m_size.x = std::max( 1, bmp.Size().x / 2 );
    m_size.y = std::max( 1, bmp.Size().y / 2 );

    int w = std::max( m_size.x, 4 );
    int h = std::max( m_size.y, 4 );

    DBGPRINT( "Subbitmap " << m_size.x << "x" << m_size.y );

    m_block = m_data = new uint32_t[w*h];

    if( m_size.x < w || m_size.y < h )
    {
        memset( m_data, 0, w*h*sizeof( uint32_t ) );
        m_linesLeft = h / 4;
        unsigned int lines = 0;
        for( int i=0; i<h/4; i++ )
        {
            for( int j=0; j<4; j++ )
            {
                lines++;
                if( lines > m_lines )
                {
                    lines = 0;
                    m_sema.unlock();
                }
            }
        }
        if( lines != 0 )
        {
            m_sema.unlock();
        }
    }
    else
    {
        m_linesLeft = h / 4;
        m_load = std::async( std::launch::async, [this, &bmp, w, h]() mutable
        {
            auto ptr = m_data;
            auto src1 = bmp.Data();
            auto src2 = src1 + bmp.Size().x;
            unsigned int lines = 0;
            for( int i=0; i<h/4; i++ )
            {
                for( int j=0; j<4; j++ )
                {
                    int k = m_size.x;
                    while( k-- )
                    {
#ifdef __SSE4_1__
                        uint64_t p0, p1;
                        memcpy( &p0, src1, 8 );
                        memcpy( &p1, src2, 8 );
                        src1 += 2;
                        src2 += 2;

                        __m128i px = _mm_set_epi64x( p0, p1 );
                        __m128i px0 = _mm_unpacklo_epi8( px, _mm_setzero_si128() );
                        __m128i px1 = _mm_unpackhi_epi8( px, _mm_setzero_si128() );

                        __m128i s0 = _mm_add_epi16( px0, px1 );
                        __m128i s1 = _mm_shuffle_epi32( s0, _MM_SHUFFLE( 1, 0, 3, 2 ) );
                        __m128i s2 = _mm_add_epi16( s0, s1 );

                        __m128i r0 = _mm_srli_epi16( s2, 2 );
                        __m128i r1 = _mm_packus_epi16( r0, r0 );

                        *ptr++ = _mm_cvtsi128_si32( r1 );
#else
                        int r = ( ( *src1 & 0x000000FF ) + ( *(src1+1) & 0x000000FF ) + ( *src2 & 0x000000FF ) + ( *(src2+1) & 0x000000FF ) ) / 4;
                        int g = ( ( ( *src1 & 0x0000FF00 ) + ( *(src1+1) & 0x0000FF00 ) + ( *src2 & 0x0000FF00 ) + ( *(src2+1) & 0x0000FF00 ) ) / 4 ) & 0x0000FF00;
                        int b = ( ( ( *src1 & 0x00FF0000 ) + ( *(src1+1) & 0x00FF0000 ) + ( *src2 & 0x00FF0000 ) + ( *(src2+1) & 0x00FF0000 ) ) / 4 ) & 0x00FF0000;
                        int a = ( ( ( ( ( *src1 & 0xFF000000 ) >> 8 ) + ( ( *(src1+1) & 0xFF000000 ) >> 8 ) + ( ( *src2 & 0xFF000000 ) >> 8 ) + ( ( *(src2+1) & 0xFF000000 ) >> 8 ) ) / 4 ) & 0x00FF0000 ) << 8;
                        *ptr++ = r | g | b | a;
                        src1 += 2;
                        src2 += 2;
#endif
                    }
                    src1 += m_size.x * 2;
                    src2 += m_size.x * 2;
                }
                lines++;
                if( lines >= m_lines )
                {
                    lines = 0;
                    m_sema.unlock();
                }
            }

            if( lines != 0 )
            {
                m_sema.unlock();
            }
        } );
    }
}

BitmapDownsampled::~BitmapDownsampled()
{
}
