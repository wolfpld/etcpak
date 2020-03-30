#include <future>
#include <stdio.h>
#include <limits>
#include <math.h>
#include <memory>
#include <string.h>

#include "getopt/getopt.h"

#include "Bitmap.hpp"
#include "BlockData.hpp"
#include "DataProvider.hpp"
#include "Debug.hpp"
#include "Dither.hpp"
#include "Error.hpp"
#include "System.hpp"
#include "TaskDispatch.hpp"
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
    fprintf( stderr, "Usage: etcpak [options] input.png output.pvr\n" );
    fprintf( stderr, "  Options:\n" );
    fprintf( stderr, "  -v          view mode (loads pvr/ktx file, decodes it and saves to png)\n" );
    fprintf( stderr, "  -o 1        output selection (sum of: 1 - save pvr file; 2 - save png file)\n" );
    fprintf( stderr, "                note: pvr files are written regardless of this option\n" );
    fprintf( stderr, "  -a          disable alpha channel processing\n" );
    fprintf( stderr, "  -s          display image quality measurements\n" );
    fprintf( stderr, "  -b          benchmark mode\n" );
    fprintf( stderr, "  -m          generate mipmaps\n" );
    fprintf( stderr, "  -d          enable dithering\n" );
    fprintf( stderr, "  --debug     dissect ETC texture\n" );
    fprintf( stderr, "  --etc2      enable ETC2 mode\n" );
    fprintf( stderr, "  --rgba      enable ETC2 RGBA mode\n" );
}

int main( int argc, char** argv )
{
    DebugLog::AddCallback( &DebugCallback );

    bool viewMode = false;
    int save = 1;
    bool alpha = true;
    bool stats = false;
    bool benchmark = false;
    bool mipmap = false;
    bool dither = false;
    bool debug = false;
    bool etc2 = false;
    bool rgba = false;
    unsigned int cpus = System::CPUCores();

    if( argc < 3 )
    {
        Usage();
        return 1;
    }

    enum Options
    {
        OptDebug,
        OptEtc2,
        OptRgba
    };

    struct option longopts[] = {
        { "debug", no_argument, nullptr, OptDebug },
        { "etc2", no_argument, nullptr, OptEtc2 },
        { "rgba", no_argument, nullptr, OptRgba },
        {}
    };

    const char* input = nullptr;
    const char* output = nullptr;
    int c;
    while( ( c = getopt_long( argc, argv, "vo:asbmd", longopts, nullptr ) ) != -1 )
    {
        switch( c )
        {
        case 'v':
            viewMode = true;
            break;
        case 'o':
            save = atoi( optarg );
            assert( ( save & 0x3 ) != 0 );
            break;
        case 'a':
            alpha = false;
            break;
        case 's':
            stats = true;
            break;
        case 'b':
            benchmark = true;
            break;
        case 'm':
            mipmap = true;
            break;
        case 'd':
            dither = true;
            break;
        case OptDebug:
            debug = true;
            break;
        case OptEtc2:
            etc2 = true;
            break;
        case OptRgba:
            rgba = true;
            etc2 = true;
            break;
        default:
            break;
        }
    }

    if( argc - optind < 2 )
    {
        Usage();
        return 1;
    }

    input = argv[optind];
    output = argv[optind+1];

    if( dither )
    {
        InitDither();
    }

    TaskDispatch taskDispatch( cpus );

    if( benchmark )
    {
        auto start = GetTime();
        auto bmp = std::make_shared<Bitmap>( argv[1], std::numeric_limits<unsigned int>::max() );
        auto data = bmp->Data();
        auto end = GetTime();
        printf( "Image load time: %0.3f ms\n", ( end - start ) / 1000.f );

        const int NumTasks = cpus * 10;
        start = GetTime();
        for( int i=0; i<NumTasks; i++ )
        {
            TaskDispatch::Queue( [&bmp, &dither, i, etc2, rgba]()
            {
                const BlockData::Type type = rgba ? BlockData::Etc2_RGBA : ( etc2 ? BlockData::Etc2_RGB : BlockData::Etc1 );
                auto bd = std::make_shared<BlockData>( bmp->Size(), false, type );
                if( rgba )
                {
                    bd->ProcessRGBA( bmp->Data(), bmp->Size().x * bmp->Size().y / 16, 0, bmp->Size().x, dither );
                }
                else
                {
                    bd->Process( bmp->Data(), bmp->Size().x * bmp->Size().y / 16, 0, bmp->Size().x, Channels::RGB, dither );
                }
            } );
        }
        TaskDispatch::Sync();
        end = GetTime();
        const auto mct = ( end - start ) / ( NumTasks * 1000.f );
        printf( "Mean compression time for %i runs: %0.3f ms\n", NumTasks, mct );
        printf( "Throughput: %0.3f Mpx/s per core\n", bmp->Size().x * bmp->Size().y / mct / 1000 / cpus );
    }
    else if( viewMode )
    {
        auto bd = std::make_shared<BlockData>( argv[1] );
        auto out = bd->Decode();
        out->Write( "out.png" );
    }
    else if( debug )
    {
        auto bd = std::make_shared<BlockData>( argv[1] );
        bd->Dissect();
    }
    else
    {
        DataProvider dp( argv[1], mipmap );
        auto num = dp.NumberOfParts();

        BlockData::Type type;
        if( etc2 )
        {
            if( rgba && dp.Alpha() )
            {
                type = BlockData::Etc2_RGBA;
            }
            else
            {
                type = BlockData::Etc2_RGB;
            }
        }
        else
        {
            type = BlockData::Etc1;
        }

        auto bd = std::make_shared<BlockData>( "out.pvr", dp.Size(), mipmap, type );
        BlockDataPtr bda;
        if( alpha && dp.Alpha() && !rgba )
        {
            bda = std::make_shared<BlockData>( "outa.pvr", dp.Size(), mipmap, type );
        }

        if( bda )
        {
            for( int i=0; i<num; i++ )
            {
                auto part = dp.NextPart();

                TaskDispatch::Queue( [part, i, &bd, &dither]()
                {
                    bd->Process( part.src, part.width / 4 * part.lines, part.offset, part.width, Channels::RGB, dither );
                } );
                TaskDispatch::Queue( [part, i, &bda]()
                {
                    bda->Process( part.src, part.width / 4 * part.lines, part.offset, part.width, Channels::Alpha, false );
                } );
            }
        }
        else
        {
            for( int i=0; i<num; i++ )
            {
                auto part = dp.NextPart();

                if( type == BlockData::Etc2_RGBA )
                {
                    TaskDispatch::Queue( [part, i, &bd, &dither]()
                    {
                        bd->ProcessRGBA( part.src, part.width / 4 * part.lines, part.offset, part.width, dither );
                    } );
                }
                else
                {
                    TaskDispatch::Queue( [part, i, &bd, &dither]()
                    {
                        bd->Process( part.src, part.width / 4 * part.lines, part.offset, part.width, Channels::RGB, dither );
                    } );
                }
            }
        }

        TaskDispatch::Sync();

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
    }

    return 0;
}
