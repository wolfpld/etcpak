#include <stdio.h>
#include <math.h>
#include <memory>
#include <string.h>

#include "Bitmap.hpp"
#include "BlockBitmap.hpp"
#include "BlockData.hpp"
#include "Debug.hpp"
#include "Error.hpp"

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
    fprintf( stderr, "  -a          disable alpha channel processing\n" );
    fprintf( stderr, "  -s          display image quality measurements\n" );
}

int main( int argc, char** argv )
{
    DebugLog::AddCallback( &DebugCallback );

    int quality = 0;
    bool viewMode = false;
    int save = 1;
    bool alpha = true;
    bool stats = false;

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
        else
        {
            Usage();
            return 1;
        }
    }
#undef CSTR

    if( viewMode )
    {
        auto bd = std::make_shared<BlockData>( argv[1] );
        auto out = bd->Decode();
        out->Write( "out.png" );
    }
    else
    {
        auto bmp = std::make_shared<Bitmap>( argv[1] );
        auto bb = std::make_shared<BlockBitmap>( bmp, Channels::RGB );
        BlockBitmapPtr bba;
        if( bmp->Alpha() && alpha )
        {
            bba = std::make_shared<BlockBitmap>( bmp, Channels::Alpha );
        }
        if( !stats )
        {
            bmp.reset();
        }

        auto bd = std::make_shared<BlockData>( bb, quality );
        BlockDataPtr bda;
        if( bba )
        {
            bda = std::make_shared<BlockData>( bba, quality );
        }
        bb.reset();

        if( stats )
        {
            auto out = bd->Decode();
            float mse = CalcMSE3( bmp, out );
            printf( "RGB data\n" );
            printf( "  RMSE: %f\n", sqrt( mse ) );
            printf( "  PSNR: %f\n", 20 * log10( 255 ) - 10 * log10( mse ) );

            if( bda )
            {
                auto out = bda->Decode();
                float mse = CalcMSE1( bmp, out );
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
        if( save & 0x1 )
        {
            bd->WritePVR( "out.pvr" );
            if( bda )
            {
                bda->WritePVR( "outa.pvr" );
            }
        }
    }

    return 0;
}
