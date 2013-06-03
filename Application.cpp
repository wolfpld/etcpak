#include <stdio.h>
#include <memory>
#include <string.h>

#include "Bitmap.hpp"
#include "BlockBitmap.hpp"
#include "BlockData.hpp"
#include "Debug.hpp"

struct DebugCallback_t : public DebugLog::Callback
{
    void OnDebugMessage( const char* msg ) override
    {
        fprintf( stderr, "%s\n", msg );
    }
} DebugCallback;

void Usage()
{
    fprintf( stderr, "Usage: etcpak input.png [-q quality]\n" );
}

int main( int argc, char** argv )
{
    DebugLog::AddCallback( &DebugCallback );

    int quality = 0;
    bool viewMode = false;
    int save = 1;

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
        else if( CSTR( "-s" ) )
        {
            i++;
            save = atoi( argv[i] );
            assert( ( save & 0x3 ) != 0 );
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
        if( bmp->Alpha() )
        {
            bba = std::make_shared<BlockBitmap>( bmp, Channels::Alpha );
        }
        bmp.reset();

        auto bd = std::make_shared<BlockData>( bb, quality );
        BlockDataPtr bda;
        if( bba )
        {
            bda = std::make_shared<BlockData>( bba, quality );
        }
        bb.reset();

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
