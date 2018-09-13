/******************************Module*Header*******************************\
* Module Name: sspipes.cxx
*
* Startup code
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <time.h>
#include <windows.h>

#include "sspipes.h"
#include "dialog.h"
#include "state.h"

#ifdef DO_TIMING
int pipeCount;
#endif

void InitPipes( void *data );

/******************************Public*Routine******************************\
* ss_Init
*
* Initialize - called on first entry into ss.
* Called BEFORE gl is initialized!
* Just do basic stuff here, like set up callbacks, verify dialog stuff, etc.
*
* Fills global SSContext structure with required data, and returns ptr
* to it.
*
\**************************************************************************/

static SSContext ssc;

SSContext *
ss_Init( void )
{
    // validate some initial dialog settings
    getIniSettings();  // also called on dialog init    

    if( ulSurfStyle == SURFSTYLE_TEX ) {
        // Texture verification has to go here, before gl is loaded, in case
        // error msgs are displayed.
        
        for( int i = 0; i < gnTextures; i ++ ) {
            if( !ss_VerifyTextureFile( &gTexFile[i]) ) {
                // user texture is invalid - substitute resource texture ?
                // If gnTextures > nRes
                // get rid of this one - move the others up
                gnTextures--;
                for( int j = i; j < gnTextures; j++ )
                    gTexFile[j] = gTexFile[j+1];
            }
        }
    }

    ss_InitFunc( InitPipes );

    // set configuration info to return
    ssc.bDoubleBuf = FALSE;
    ssc.depthType = SS_DEPTH16;
    ssc.bFloater = FALSE;

    return &ssc;
}

static void
Draw( void *data )
{
    // don't need data here, but I was hoping to be able to use the STATE
    // member functions directly as callbacks
    ((STATE *) data)->Draw(data);
}

static void
Reshape( int width, int height, void *data )
{
    ((STATE *) data)->Reshape( width, height, data );
}

static void
Repaint( LPRECT pRect, void *data )
{
    ((STATE *) data)->Repaint( pRect, data );
}

static void
Finish( void *data )
{
    ((STATE *) data)->Finish( data );
}

/******************************Public*Routine******************************\
* InitPipes
*
* - Called when GL window has been initialized
*
\**************************************************************************/
void 
InitPipes( void *data )
{
    // create world of pipes

    //mf: for now, bFlexMode used to choose between normal/flex

    STATE *pPipeWorld = new STATE( bFlexMode, bMultiPipes );

#if 0
    //mf: compiler doesn't like me using class member functions as callbacks
    ss_UpdateFunc( pState->Draw );
    ss_ReshapeFunc( pState->Reshape );
    ss_FinishFunc( pState->Finish );
#else
    // mf: use wrappers for now
    ss_UpdateFunc( Draw );
    ss_ReshapeFunc( Reshape );
    ss_RepaintFunc( Repaint );
    ss_FinishFunc( Finish );
#endif
    //mf: this should no longer be necessary
    ss_DataPtr( pPipeWorld ); 
}

#ifdef DO_TIMING
void CalcPipeRate( struct _timeb baseTime, int pipeCount ) {
    static struct _timeb thisTime;
    double elapsed, pipeRate;
    char buf[100];

    _ftime( &thisTime );
    elapsed = thisTime.time + thisTime.millitm/1000.0 -
       (baseTime.time + baseTime.millitm/1000.0);


    if( elapsed == 0.0 )
        pipeRate = 0.0;
    else
        pipeRate = pipeCount / elapsed;

    sprintf( buf, "Last frame's pipe rate = %4.1f pps", pipeRate );
#ifdef SS_DEBUG
    SendMessage(ss_GetHWND(), WM_SETTEXT, 0, (LPARAM)buf);
#endif
}

void Timer( int mode )
{
    static struct _timeb baseTime;

    switch( mode ) {
        case TIMER_START:
            pipeCount = 0;
 	        _ftime( &baseTime );
            break;
        case TIMER_STOP:
            CalcPipeRate( baseTime, pipeCount );
            break;
        case TIMER_TIMING:
            break;
        case TIMER_RESET:
        default:
            break;
    }
}
#endif

