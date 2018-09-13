/******************************Module*Header*******************************\
* Module Name: ssutil.cxx
*
* Screen-saver utility functions
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <commdlg.h>
#include <scrnsave.h>
#include <GL\gl.h>
#include <memory.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <time.h>
#include "tk.h"
#include "ssintrnl.hxx"

#include "ssutil.hxx"

static OSVERSIONINFO gosvi = {0};
static BOOL gbGLv1_1 = FALSE;  // GL version 1.1 boolean

/******************************Public*Routine******************************\
* SSU_ChoosePixelFormat
*
* Local implementation of ChoosePixelFormat
*
* Choose pixel format based on flags.
* This allows us a little a more control than just calling ChoosePixelFormat
\**************************************************************************/

static int
SSU_ChoosePixelFormat( HDC hdc, int flags )
{
    int MaxPFDs;
    int iBest = 0;
    PIXELFORMATDESCRIPTOR pfd;

    // Always choose native pixel depth
    int cColorBits =
                GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);

    BOOL bDoubleBuf = flags & SS_DOUBLEBUF_BIT;

    int cDepthBits = 0;
    if( SS_HAS_DEPTH16(flags) )
        cDepthBits = 16;
    else if( SS_HAS_DEPTH32(flags) )
        cDepthBits = 32;

    int i = 1;
    do
    {
        MaxPFDs = DescribePixelFormat(hdc, i, sizeof(pfd), &pfd);
        if ( MaxPFDs <= 0 )
            return 0;

        if( ! (pfd.dwFlags & PFD_SUPPORT_OPENGL) )
            continue;

        if( flags & SS_BITMAP_BIT ) {
            // need bitmap pixel format
            if( ! (pfd.dwFlags & PFD_DRAW_TO_BITMAP) )
                continue;
        } else {
            // need window pixel format
            if( ! (pfd.dwFlags & PFD_DRAW_TO_WINDOW) )
                continue;
            // a window can be double buffered...
            if( ( bDoubleBuf && !(pfd.dwFlags & PFD_DOUBLEBUFFER) ) ||
                ( !bDoubleBuf && (pfd.dwFlags & PFD_DOUBLEBUFFER) ) )
                continue;
        }

        if ( pfd.iPixelType != PFD_TYPE_RGBA )
            continue;
        if( pfd.cColorBits != cColorBits )
            continue;

        if( flags & SS_ALPHA_BIT ) {
            // We want alpha bit planes
            if( pfd.cAlphaBits == 0 )
                continue;
        } else {
            // We don't want alpha
            if( pfd.cAlphaBits )
                continue;
        }

        if( (flags & SS_GENERIC_UNACCELERATED_BIT) &&
            ((pfd.dwFlags & (PFD_GENERIC_FORMAT|PFD_GENERIC_ACCELERATED))
		    != PFD_GENERIC_FORMAT) )
            continue;

        if( (flags & SS_NO_SYSTEM_PALETTE_BIT) &&
            (pfd.dwFlags & PFD_NEED_SYSTEM_PALETTE) )
            continue;

        if( cDepthBits ) {
            if( pfd.cDepthBits < cDepthBits )
                continue;
        } else {
            // No depth buffer required, but use it if nothing better
            if( pfd.cDepthBits ) {
                if( pfd.dwFlags & PFD_GENERIC_ACCELERATED )
                    // Accelerated pixel format - we may as well use this, even
                    // though we don't need depth.  Otherwise if we keep going
                    // to find a better match, we run the risk of overstepping
                    // all the accelerated formats and picking a slower format.
                    return i;
                iBest = i;
                continue;
            }
        }

        // We have found something useful
        return i;

    } while (++i <= MaxPFDs);

    if( iBest )
        // not an exact match, but good enough
        return iBest;

    // If we reach here, we have failed to find a suitable pixel format.
    // See if the system can find us one.

    memset( &pfd, 0, sizeof( PIXELFORMATDESCRIPTOR ) );
    pfd.nSize = sizeof( PIXELFORMATDESCRIPTOR );
    pfd.cColorBits = (BYTE)cColorBits;
    pfd.cDepthBits = (BYTE)cDepthBits;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.dwFlags = PFD_SUPPORT_OPENGL;
    if( bDoubleBuf )
        pfd.dwFlags |= PFD_DOUBLEBUFFER;
    if( flags & SS_BITMAP_BIT )
        pfd.dwFlags |= PFD_DRAW_TO_BITMAP;
    else
        pfd.dwFlags |= PFD_DRAW_TO_WINDOW;

    if( (flags & SS_GENERIC_UNACCELERATED_BIT) ||
        (flags & SS_NO_SYSTEM_PALETTE_BIT) )
        // If either of these flags are set, we should be safe specifying a
        // 'slow' pixel format that supports bitmap drawing
        //mf: DRAW_TO_WINDOW seems to override this...
        pfd.dwFlags |= PFD_DRAW_TO_BITMAP;

    SS_WARNING( "SSU_ChoosePixelFormat failed, calling ChoosePIxelFormat\n" );

    return ChoosePixelFormat( hdc, &pfd );
}

/******************************Public*Routine******************************\
* SSU_SetupPixelFormat
*
* Choose pixel format according to supplied flags.  If ppfd is non-NULL,
* call DescribePixelFormat with it.
*
\**************************************************************************/

BOOL
SSU_SetupPixelFormat(HDC hdc, int flags, PIXELFORMATDESCRIPTOR *ppfd )
{
    int pixelFormat;
    int nTryAgain = 4;

    do{
        if( (pixelFormat = SSU_ChoosePixelFormat(hdc, flags)) &&
            SetPixelFormat(hdc, pixelFormat, NULL) ) {
            SS_DBGLEVEL1( SS_LEVEL_INFO,
               "SSU_SetupPixelFormat: Setting pixel format %d\n", pixelFormat );
            if( ppfd )
                DescribePixelFormat(hdc, pixelFormat,
                                sizeof(PIXELFORMATDESCRIPTOR), ppfd);
            return TRUE; // Success
        }
        // Failed to set pixel format.  Try again after waiting a bit (win95
        // bug with full screen dos box)
        Sleep( 1000 ); // Wait a second between attempts
    } while( nTryAgain-- );

    return FALSE;
}

/******************************Public*Routine******************************\
* SSU_bNeedPalette
*
\**************************************************************************/

BOOL
SSU_bNeedPalette( PIXELFORMATDESCRIPTOR *ppfd )
{
    if (ppfd->dwFlags & PFD_NEED_PALETTE)
        return TRUE;
    else
        return FALSE;
}


/******************************Public*Routine******************************\
* SSU_PixelFormatDescriptorFromDc
*
\**************************************************************************/

int
SSU_PixelFormatDescriptorFromDc( HDC hdc, PIXELFORMATDESCRIPTOR *Pfd )
{
    int PfdIndex;

    if ( 0 < (PfdIndex = GetPixelFormat( hdc )) )
    {
        if ( 0 < DescribePixelFormat( hdc, PfdIndex, sizeof(*Pfd), Pfd ) )
        {
            return(PfdIndex);
        }
    }
    return 0;
}

/******************************Public*Routine******************************\
* ss_ChangeDisplaySettings
*
* Try changing display settings.
* If bitDepth is 0, use current bit depth
*
\**************************************************************************/

BOOL
ss_ChangeDisplaySettings( int width, int height, int bitDepth )
{
    int change;
    DEVMODE dm = {0};

	dm.dmSize       = sizeof(dm);
    dm.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT;
	dm.dmPelsWidth  = width;
	dm.dmPelsHeight = height;

    if( bitDepth != 0 ) {
	    dm.dmFields |= DM_BITSPERPEL;
    	dm.dmBitsPerPel = bitDepth;
    }

//    change = ChangeDisplaySettings(&dm, CDS_TEST);
    change = ChangeDisplaySettings(&dm, CDS_FULLSCREEN);

    if( change == DISP_CHANGE_SUCCESSFUL )
        return TRUE;
    else
        return FALSE;
}

/******************************Public*Routine******************************\
* ss_QueryDisplaySettings
*
* Find out what diplay resolutions are available.
*
\**************************************************************************/

void
ss_QueryDisplaySettings( void )
{
    int i = 0;
    DEVMODE devMode = {0};

    while( EnumDisplaySettings( NULL, i, &devMode ) ) {
        i++;
    }
}

/******************************Public*Routine******************************\
* ss_QueryGLVersion
*
* Find out what GL version is being loaded.  If it's 1.1, set various
* global capabilities.
*
\**************************************************************************/

void
ss_QueryGLVersion( void )
{
    // Get GL version

    if( strstr( (char *) glGetString(GL_VERSION), "1.1") ) {
        gbGLv1_1 = TRUE;
        gbTextureObjects = TRUE;
    } else {
        gbGLv1_1 = FALSE;
        gbTextureObjects = FALSE;
    }
    if( !gbTextureObjects ) {
        SS_DBGINFO( "ss_QueryGLVersion: Texture Objects disabled\n" );
    }
}

/******************************Public*Routine******************************\
* ss_fOnGL11
*
* True if running on OpenGL v.1.1x
*
\**************************************************************************/

BOOL
ss_fOnGL11( void )
{
    return gbGLv1_1;
}

/******************************Public*Routine******************************\
* ss_QueryOSVersion
*
* Query the OS version
*
\**************************************************************************/

void
ss_QueryOSVersion( void )
{
    gosvi.dwOSVersionInfoSize = sizeof(gosvi);
    GetVersionEx(&gosvi);
}

/******************************Public*Routine******************************\
* ss_fOnNT35
*
* True if running on NT version 3.51 or less
*
\**************************************************************************/

BOOL
ss_fOnNT35( void )
{
    static fOnNT35;
    static bInited = FALSE;

    if( !bInited ) {
        if( !gosvi.dwOSVersionInfoSize )
            ss_QueryOSVersion();
        fOnNT35 =
        (
            (gosvi.dwPlatformId == VER_PLATFORM_WIN32_NT) &&
            (gosvi.dwMajorVersion == 3 && gosvi.dwMinorVersion <= 51)
        );
        bInited = TRUE;
    }
    return fOnNT35;
}

/******************************Public*Routine******************************\
* ss_fOnWin95
*
* True if running on Windows 95
*
\**************************************************************************/

BOOL
ss_fOnWin95( void )
{
    static fOnWin95;
    static bInited = FALSE;

    if( !bInited ) {
        if( !gosvi.dwOSVersionInfoSize )
            ss_QueryOSVersion();
        fOnWin95 = ( gosvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS );
        bInited = TRUE;
    }
    return fOnWin95;
}

/******************************Public*Routine******************************\
* ss_fPreviewMode
*
* True if running in Display setting's child preview window
*
\**************************************************************************/

BOOL
ss_fPreviewMode( void )
{
    return gpss->type == SS_TYPE_PREVIEW;
}

/******************************Public*Routine******************************\
* ss_fFullScreenMode
*
* True if running full screen (/s option)
*
\**************************************************************************/

BOOL
ss_fFullScreenMode( void )
{
    return gpss->type == SS_TYPE_FULLSCREEN;
}

BOOL
ss_fConfigMode( void )
{
    return gpss->type == SS_TYPE_CONFIG;
}

BOOL
ss_fWindowMode( void )
{
    return gpss->type == SS_TYPE_NORMAL;
}

/******************************Public*Routine******************************\
* ss_RedrawDesktop
*
* Causes the entire desktop to be redrawn
*
\**************************************************************************/

BOOL
ss_RedrawDesktop( void )
{
    return RedrawWindow( NULL, NULL, NULL, RDW_ERASE | RDW_INVALIDATE |
                            RDW_ERASENOW | RDW_UPDATENOW | RDW_ALLCHILDREN );
}
