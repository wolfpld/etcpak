#include <future>
#include <stdio.h>
#include <limits>
#include <math.h>
#include <memory>
#include <string.h>

#include "Bitmap.hpp"
#include "BlockBitmap.hpp"
#include "BlockData.hpp"
#include "DataProvider.hpp"
#include "Debug.hpp"
#include "Dither.hpp"
#include "Error.hpp"
#include "Timing.hpp"

struct DebugCallback_t : public DebugLog::Callback
{
    void OnDebugMessage( const char* msg ) override
    {
        fprintf( stderr, "%s\n", msg );
    }
} DebugCallback;

void Usage()
{
    fprintf( stderr, "Usage: etcpak input.png [options]\n" );
    fprintf( stderr, "  Options:\n" );
    //fprintf( stderr, "  -q 0        set quality to given value\n" );
    fprintf( stderr, "  -v          view mode (loads pvr/ktx file, decodes it and saves to png)\n" );
    fprintf( stderr, "  -o 1        output selection (sum of: 1 - save pvr file; 2 - save png file)\n" );
    fprintf( stderr, "                note: pvr files are written regardless of this option\n" );
    fprintf( stderr, "  -a          disable alpha channel processing\n" );
    fprintf( stderr, "  -s          display image quality measurements\n" );
    fprintf( stderr, "  -b          benchmark mode\n" );
    fprintf( stderr, "  -m          generate mipmaps\n" );
    fprintf( stderr, "  -d          enable dithering\n" );
}

int main( int argc, char** argv )
{
    DebugLog::AddCallback( &DebugCallback );

    int quality = 0;
    bool viewMode = false;
    int save = 1;
    bool alpha = true;
    bool stats = false;
    bool benchmark = false;
    bool mipmap = false;
    bool dither = false;

    if( argc < 2 )
    {
        Usage();
        return 1;
    }

#define CSTR(x) strcmp( argv[i], x ) == 0
    for( int i=2; i<argc; i++ )
    {
        if( CSTR( "-q" ) )
        {
            i++;
            quality = atoi( argv[i] );
        }
        else if( CSTR( "-v" ) )
        {
            viewMode = true;
        }
        else if( CSTR( "-o" ) )
        {
            i++;
            save = atoi( argv[i] );
            assert( ( save & 0x3 ) != 0 );
        }
        else if( CSTR( "-a" ) )
        {
            alpha = false;
        }
        else if( CSTR( "-s" ) )
        {
            stats = true;
        }
        else if( CSTR( "-b" ) )
        {
            benchmark = true;
        }
        else if( CSTR( "-m" ) )
        {
            mipmap = true;
        }
        else if( CSTR( "-d" ) )
        {
            dither = true;
        }
        else
        {
            Usage();
            return 1;
        }
    }
#undef CSTR

    if( dither )
    {
        InitDither();
    }

    if( benchmark )
    {
        InitTiming();
        auto start = GetTime();
        auto bmp = std::make_shared<Bitmap>( argv[1], std::numeric_limits<uint>::max() );
        auto data = bmp->Data();
        auto end = GetTime();
        printf( "Image load time: %0.3f ms\n", ( end - start ) / 1000.f );

        start = GetTime();
        enum { NumTasks = 50 };
        auto tasks = new std::future<void>[NumTasks];
        for( int i=0; i<NumTasks; i++ )
        {
            tasks[i] = std::async( [&bmp]()
            {
                auto block = std::make_shared<BlockBitmap>( bmp, Channels::RGB );
                auto bd = std::make_shared<BlockData>( bmp->Size(), false );
                bd->Process( block->Data(), bmp->Size().x * bmp->Size().y / 16, 0, 0, Channels::RGB );
            } );
        }
        for( uint i=0; i<NumTasks; i++ )
        {
            tasks[i].wait();
        }
        delete[] tasks;
        end = GetTime();
        printf( "Mean compression time for %i runs: %0.3f ms\n", NumTasks, ( end - start ) / ( NumTasks * 1000.f ) );
    }
    else if( viewMode )
    {
        auto bd = std::make_shared<BlockData>( argv[1] );
        auto out = bd->Decode();
        out->Write( "out.png" );
    }
    else
    {
        DataProvider dp( argv[1], mipmap );
        auto num = dp.NumberOfParts();

        auto bd = std::make_shared<BlockData>( "out.pvr", dp.Size(), mipmap );
        BlockDataPtr bda;
        if( alpha && dp.Alpha() )
        {
            bda = std::make_shared<BlockData>( "outa.pvr", dp.Size(), mipmap );
        }

        auto tasks = new std::future<void>[num];
        auto blocks = new BlockBitmapPtr[num];
        auto blocksa = new BlockBitmapPtr[num];

        for( int i=0; i<num; i++ )
        {
            auto part = dp.NextPart();

            tasks[i] = std::async( [part, i, &bd, &blocks, &bda, &blocksa, quality, &dither]()
            {
                blocks[i] = std::make_shared<BlockBitmap>( part.src, v2i( part.width, part.lines * 4 ), Channels::RGB );
                if( dither )
                {
                    blocks[i]->Dither();
                }
                bd->Process( blocks[i]->Data(), part.width / 4 * part.lines, part.offset, quality, Channels::RGB );
                if( bda )
                {
                    blocksa[i] = std::make_shared<BlockBitmap>( part.src, v2i( part.width, part.lines * 4 ), Channels::Alpha );
                    bda->Process( blocksa[i]->Data(), part.width / 4 * part.lines, part.offset, quality, Channels::Alpha );
                }
            } );
        }

        for( int i=0; i<num; i++ )
        {
            tasks[i].wait();
        }
        delete[] tasks;

        if( stats )
        {
            auto out = bd->Decode();
            float mse = CalcMSE3( dp.ImageData(), *out );
            printf( "RGB data\n" );
            printf( "  RMSE: %f\n", sqrt( mse ) );
            printf( "  PSNR: %f\n", 20 * log10( 255 ) - 10 * log10( mse ) );
            if( bda )
            {
                auto out = bda->Decode();
                float mse = CalcMSE1( dp.ImageData(), *out );
                printf( "A data\n" );
                printf( "  RMSE: %f\n", sqrt( mse ) );
                printf( "  PSNR: %f\n", 20 * log10( 255 ) - 10 * log10( mse ) );
            }
        }

        if( save & 0x2 )
        {
            auto out = bd->Decode();
            out->Write( "out.png" );
            if( bda )
            {
                auto outa = bda->Decode();
                outa->Write( "outa.png" );
            }
        }

        bd.reset();
        bda.reset();

        delete[] blocks;
        delete[] blocksa;
    }

    return 0;
}
