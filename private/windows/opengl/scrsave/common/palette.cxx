/******************************Module*Header*******************************\
* Module Name: palette.cxx
*
* Palette processing functions
*
* Adapted from tk.c
*
* Copyright (c) 1996 Microsoft Corporation
*
\**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <GL/gl.h>
#include "ssintrnl.hxx"
#include "palette.hxx"

#if(WINVER < 0x0400)
// Ordinarily not defined for versions before 4.00.
#define COLOR_3DDKSHADOW        21
#define COLOR_3DLIGHT           22
#define COLOR_INFOTEXT          23
#define COLOR_INFOBK            24
#endif

#define TKASSERT(x) SS_ASSERT( x, "palette processing failure\n" )

/******************************************************************************/

// Fixed palette support.

#define BLACK   PALETTERGB(0,0,0)
#define WHITE   PALETTERGB(255,255,255)
#define MAX_STATIC_COLORS   (COLOR_INFOBK - COLOR_SCROLLBAR + 1)
static int gNumStaticColors = MAX_STATIC_COLORS;

// TRUE if static system color settings have been replaced with B&W settings.

// TRUE if original static colors saved
static BOOL tkStaticColorsSaved = FALSE;

// saved system static colors (initialize with default colors)
static COLORREF gacrSave[MAX_STATIC_COLORS];

// new B&W system static colors
static COLORREF gacrBlackAndWhite[] = {
    WHITE,  // COLOR_SCROLLBAR
    BLACK,  // COLOR_BACKGROUND
    BLACK,  // COLOR_ACTIVECAPTION
    WHITE,  // COLOR_INACTIVECAPTION
    WHITE,  // COLOR_MENU
    WHITE,  // COLOR_WINDOW
    BLACK,  // COLOR_WINDOWFRAME
    BLACK,  // COLOR_MENUTEXT
    BLACK,  // COLOR_WINDOWTEXT
    WHITE,  // COLOR_CAPTIONTEXT
    WHITE,  // COLOR_ACTIVEBORDER
    WHITE,  // COLOR_INACTIVEBORDER
    WHITE,  // COLOR_APPWORKSPACE
    BLACK,  // COLOR_HIGHLIGHT
    WHITE,  // COLOR_HIGHLIGHTTEXT
    WHITE,  // COLOR_BTNFACE
    BLACK,  // COLOR_BTNSHADOW
    BLACK,  // COLOR_GRAYTEXT
    BLACK,  // COLOR_BTNTEXT
    BLACK,  // COLOR_INACTIVECAPTIONTEXT
    BLACK,  // COLOR_BTNHIGHLIGHT
    BLACK,  // COLOR_3DDKSHADOW
    WHITE,  // COLOR_3DLIGHT
    BLACK,  // COLOR_INFOTEXT
    WHITE   // COLOR_INFOBK
    };
static INT gaiStaticIndex[] = {
    COLOR_SCROLLBAR          ,
    COLOR_BACKGROUND         ,
    COLOR_ACTIVECAPTION      ,
    COLOR_INACTIVECAPTION    ,
    COLOR_MENU               ,
    COLOR_WINDOW             ,
    COLOR_WINDOWFRAME        ,
    COLOR_MENUTEXT           ,
    COLOR_WINDOWTEXT         ,
    COLOR_CAPTIONTEXT        ,
    COLOR_ACTIVEBORDER       ,
    COLOR_INACTIVEBORDER     ,
    COLOR_APPWORKSPACE       ,
    COLOR_HIGHLIGHT          ,
    COLOR_HIGHLIGHTTEXT      ,
    COLOR_BTNFACE            ,
    COLOR_BTNSHADOW          ,
    COLOR_GRAYTEXT           ,
    COLOR_BTNTEXT            ,
    COLOR_INACTIVECAPTIONTEXT,
    COLOR_BTNHIGHLIGHT       ,
    COLOR_3DDKSHADOW         ,
    COLOR_3DLIGHT            ,
    COLOR_INFOTEXT           ,
    COLOR_INFOBK
    };

#define RESTORE_FROM_REGISTRY   1
#if RESTORE_FROM_REGISTRY
// Registry names for the system colors.
static CHAR *gaszSysClrNames[] = {
    "Scrollbar",      // COLOR_SCROLLBAR              0
    "Background",     // COLOR_BACKGROUND             1   (also COLOR_DESKTOP)
    "ActiveTitle",    // COLOR_ACTIVECAPTION          2
    "InactiveTitle",  // COLOR_INACTIVECAPTION        3
    "Menu",           // COLOR_MENU                   4
    "Window",         // COLOR_WINDOW                 5
    "WindowFrame",    // COLOR_WINDOWFRAME            6
    "MenuText",       // COLOR_MENUTEXT               7
    "WindowText",     // COLOR_WINDOWTEXT             8
    "TitleText",      // COLOR_CAPTIONTEXT            9
    "ActiveBorder",   // COLOR_ACTIVEBORDER          10
    "InactiveBorder", // COLOR_INACTIVEBORDER        11
    "AppWorkspace",   // COLOR_APPWORKSPACE          12
    "Hilight",        // COLOR_HIGHLIGHT             13
    "HilightText",    // COLOR_HIGHLIGHTTEXT         14
    "ButtonFace",     // COLOR_BTNFACE               15   (also COLOR_3DFACE)
    "ButtonShadow",   // COLOR_BTNSHADOW             16   (also COLOR_3DSHADOW)
    "GrayText",       // COLOR_GRAYTEXT              17
    "ButtonText",     // COLOR_BTNTEXT               18
    "InactiveTitleText", // COLOR_INACTIVECAPTIONTEXT   19
    "ButtonHilight",  // COLOR_BTNHIGHLIGHT          20   (also COLOR_3DHILIGHT)
    "ButtonDkShadow", // COLOR_3DDKSHADOW            21
    "ButtonLight",    // COLOR_3DLIGHT               22
    "InfoText",       // COLOR_INFOTEXT              23
    "InfoWindow"      // COLOR_INFOBK                24
};

static BOOL GetRegistrySysColors(COLORREF *, int);
#endif

unsigned char ss_ComponentFromIndex(int i, int nbits, int shift );
static int ss_PixelFormatDescriptorFromDc( HDC hdc, PIXELFORMATDESCRIPTOR *Pfd );

/******************************************************************************/

#if RESTORE_FROM_REGISTRY
/******************************Public*Routine******************************\
* GetRegistrySysColors
*
* Reads the Control Panel's color settings from the registry and stores
* those values in pcr.  If we fail to get any value, then the corresponding
* entry in pcr is not modified.
*
* History:
*  12-Apr-1995 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

static BOOL GetRegistrySysColors(COLORREF *pcr, int nColors)
{
    BOOL bRet = FALSE;
    long lRet;
    HKEY hkSysColors = (HKEY) NULL;
    int i;
    DWORD dwDataType;
    char achColor[64];
    DWORD cjColor;

    TKASSERT(nColors <= gNumStaticColors);

// Open the key for the system color settings.

    lRet = RegOpenKeyExA(HKEY_CURRENT_USER,
                         "Control Panel\\Colors",
                         0,
                         KEY_QUERY_VALUE,
                         &hkSysColors);

    if ( lRet != ERROR_SUCCESS )
    {
        goto GetRegistrySysColors_exit;
    }

// Read each system color value.  The names are stored in the global
// array of char *, gaszSysClrNames.

    for (i = 0; i < nColors; i++)
    {
        cjColor = sizeof(achColor);
        lRet = RegQueryValueExA(hkSysColors,
                                (LPSTR) gaszSysClrNames[i],
                                (LPDWORD) NULL,
                                &dwDataType,
                                (LPBYTE) achColor,
                                &cjColor);

        TKASSERT(lRet != ERROR_MORE_DATA);

        if ( lRet == ERROR_SUCCESS && dwDataType == REG_SZ )
        {
            DWORD r, g, b;

            sscanf(achColor, "%ld %ld %ld", &r, &g, &b);
            pcr[i] = RGB(r, g, b);
        }
    }

    bRet = TRUE;

GetRegistrySysColors_exit:
    if (hkSysColors)
        RegCloseKey(hkSysColors);

    return bRet;
}
#endif

/******************************Public*Routine******************************\
* GrabStaticEntries
*
* Support routine for Realize to manage the static system color
* usage.
*
* This function will save the current static system color usage state.
* It will fail if:
*
*   1.  TK is not in "sys color in use state but system palette is in
*       SYSPAL_NOSTATIC mode.  This means that another app still possesses
*       the static system colors.  If this happens <TBD>
*
* Side effect:
*   If system colors are changed, then WM_SYSCOLORCHANGE message is
*   broadcast to all top level windows.
*
* Returns:
*   TRUE if successful, FALSE otherwise (see above).
*
* History:
*  26-Apr-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL 
SS_PAL::GrabStaticEntries()
{
    int i;
    BOOL bRet = FALSE;

    // Do nothing if sys colors already in use.

    if ( !bSystemColorsInUse )
    {
    // Take possession only if no other app has the static colors.
    // How can we tell?  If the return from SetSystemPaletteUse is
    // SYSPAL_STATIC, then no other app has the statics.  If it is
    // SYSPAL_NOSTATIC, someone else has them and we must fail.
    //
    // SetSystemPaletteUse is properly synchronized internally
    // so that it is atomic.
    //
    // Because we are relying on SetSystemPaletteUse to synchronize TK,
    // it is important to observe the following order for grabbing and
    // releasing:
    //
    //      Grab        call SetSystemPaletteUse and check for SYSPAL_STATIC
    //                  save sys color settings
    //                  set new sys color settings
    //
    //      Release     restore sys color settings
    //                  call SetSystemPaletteUse

//mf: ! potential pitfall here, if a 'bad' app has not released the static
// colors on deactivation.
        if ( SetSystemPaletteUse( hdc, SYSPAL_NOSTATIC ) == SYSPAL_STATIC )
        {
        // Save current sys color settings.

            for (i = COLOR_SCROLLBAR; i <= COLOR_BTNHIGHLIGHT; i++)
                gacrSave[i - COLOR_SCROLLBAR] = GetSysColor(i);

            bSystemColorsInUse = TRUE;

            // Set b&w sys color settings.

            SetSysColors(gNumStaticColors, gaiStaticIndex, gacrBlackAndWhite);

            // Inform all other top-level windows of the system color change.

            PostMessage(HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0);

            bRet = TRUE;
        } else {
            // handle case where can't get sys colors
        }
    }
    else
        bRet = TRUE;

    return bRet;
}

/******************************Public*Routine******************************\
* ReleaseStaticEntries
*
* Support routine for Realize to manage the static system color
* usage.
*
* This function will reset the current static system color usage state.
* It will fail if:
*
*   1.  TK is not in a "sys colors in use" state.  If we are in this case,
*       then the static system colors do not need to be released.
*
* Side effect:
*   If system colors are changed, then WM_SYSCOLORCHANGE message is
*   broadcast to all top level windows.
*
* Returns:
*   TRUE if successful, FALSE otherwise (see above).
*
* History:
*  21-Jul-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL 
SS_PAL::ReleaseStaticEntries()
{
    BOOL bRet = FALSE;

    // Do nothing if sys colors not in use.

    if ( bSystemColorsInUse )
    {
#if RESTORE_FROM_REGISTRY
    // Replace saved system colors with registry values.  We do it now
    // rather than earlier because someone may have changed registry while
    // TK app was running in the foreground (very unlikely, but it could
    // happen).
    //
    // Also, we still try to save current setting in GrabStaticEntries so
    // that if for some reason we fail to grab one or more of the colors
    // from the registry, we can still fall back on what we grabbed via
    // GetSysColors (even though there is a chance its the wrong color).

        GetRegistrySysColors(gacrSave, gNumStaticColors);
#endif

        // Do this now, since SetSysColors() generates WM_SYSCOLORCHANGE,
        // which can cause this routine to be re-entered
        // back to here.
        bSystemColorsInUse = FALSE;

        // Return the system palette to SYSPAL_STATIC.

        SetSystemPaletteUse( hdc, SYSPAL_STATIC );

        // Restore the saved system color settings.

        SetSysColors(gNumStaticColors, gaiStaticIndex, gacrSave);

        // Inform all other top-level windows of the system color change.

        PostMessage(HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0);

        // Reset the "sys colors in use" state and return success.

        bSystemColorsInUse = FALSE;
        bRet = TRUE;
    }

    return bRet;
}

// Gamma correction factor * 10
#define GAMMA_CORRECTION 14

// Maximum color distance with 8-bit components
#define MAX_COL_DIST (3*256*256L)

// Number of static colors
#define STATIC_COLORS 20

// Flags used when matching colors
#define EXACT_MATCH 1
#define COLOR_USED 1

// Conversion tables for n bits to eight bits

#if GAMMA_CORRECTION == 10
// These tables are corrected for a gamma of 1.0
static unsigned char abThreeToEight[8] =
{
    0, 0111 >> 1, 0222 >> 1, 0333 >> 1, 0444 >> 1, 0555 >> 1, 0666 >> 1, 0377
};
static unsigned char abTwoToEight[4] =
{
    0, 0x55, 0xaa, 0xff
};
static unsigned char abOneToEight[2] =
{
    0, 255
};
#else
// These tables are corrected for a gamma of 1.4
static unsigned char abThreeToEight[8] =
{
    0, 63, 104, 139, 171, 200, 229, 255
};
static unsigned char abTwoToEight[4] =
{
    0, 116, 191, 255
};
static unsigned char abOneToEight[2] =
{
    0, 255
};
#endif

// Table which indicates which colors in a 3-3-2 palette should be
// replaced with the system default colors
#if GAMMA_CORRECTION == 10
static int aiDefaultOverride[STATIC_COLORS] =
{
    0, 4, 32, 36, 128, 132, 160, 173, 181, 245,
    247, 164, 156, 7, 56, 63, 192, 199, 248, 255
};
#else
static int aiDefaultOverride[STATIC_COLORS] =
{
    0, 3, 24, 27, 64, 67, 88, 173, 181, 236,
    247, 164, 91, 7, 56, 63, 192, 199, 248, 255
};
#endif

unsigned char
ss_ComponentFromIndex(int i, int nbits, int shift)
{
    unsigned char val;

    TKASSERT(nbits >= 1 && nbits <= 3);
    
    val = i >> shift;
    switch (nbits)
    {
    case 1:
        return abOneToEight[val & 1];

    case 2:
        return abTwoToEight[val & 3];

    case 3:
        return abThreeToEight[val & 7];
    }
    return 0;
}

// System default colors
static PALETTEENTRY apeDefaultPalEntry[STATIC_COLORS] =
{
    { 0,   0,   0,    0 },
    { 0x80,0,   0,    0 },
    { 0,   0x80,0,    0 },
    { 0x80,0x80,0,    0 },
    { 0,   0,   0x80, 0 },
    { 0x80,0,   0x80, 0 },
    { 0,   0x80,0x80, 0 },
    { 0xC0,0xC0,0xC0, 0 },

    { 192, 220, 192,  0 },
    { 166, 202, 240,  0 },
    { 255, 251, 240,  0 },
    { 160, 160, 164,  0 },

    { 0x80,0x80,0x80, 0 },
    { 0xFF,0,   0,    0 },
    { 0,   0xFF,0,    0 },
    { 0xFF,0xFF,0,    0 },
    { 0,   0,   0xFF, 0 },
    { 0xFF,0,   0xFF, 0 },
    { 0,   0xFF,0xFF, 0 },
    { 0xFF,0xFF,0xFF, 0 }
};

/******************************Public*Routine******************************\
*
* UpdateStaticMapping
*
* Computes the best match between the current system static colors
* and a 3-3-2 palette
*
* History:
*  Tue Aug 01 18:18:12 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

static void
UpdateStaticMapping(PALETTEENTRY *pe332Palette)
{
    HPALETTE hpalStock;
    int iStatic, i332;
    int iMinDist, iDist;
    int iDelta;
    int iMinEntry;
    PALETTEENTRY *peStatic, *pe332;

    hpalStock = (HPALETTE) GetStockObject(DEFAULT_PALETTE);

    // The system should always have one of these
    TKASSERT(hpalStock != NULL);
    // Make sure there's the correct number of entries
    TKASSERT(GetPaletteEntries(hpalStock, 0, 0, NULL) == STATIC_COLORS);

    // Get the current static colors
    GetPaletteEntries(hpalStock, 0, STATIC_COLORS, apeDefaultPalEntry);

    // Zero the flags in the static colors because they are used later
    peStatic = apeDefaultPalEntry;
    for (iStatic = 0; iStatic < STATIC_COLORS; iStatic++)
    {
        peStatic->peFlags = 0;
        peStatic++;
    }

    // Zero the flags in the incoming palette because they are used later
    pe332 = pe332Palette;
    for (i332 = 0; i332 < 256; i332++)
    {
        pe332->peFlags = 0;
        pe332++;
    }

    // Try to match each static color exactly
    // This saves time by avoiding the least-squares match for each
    // exact match
    peStatic = apeDefaultPalEntry;
    for (iStatic = 0; iStatic < STATIC_COLORS; iStatic++)
    {
        pe332 = pe332Palette;
        for (i332 = 0; i332 < 256; i332++)
        {
            if (peStatic->peRed == pe332->peRed &&
                peStatic->peGreen == pe332->peGreen &&
                peStatic->peBlue == pe332->peBlue)
            {
                TKASSERT(pe332->peFlags != COLOR_USED);
                
                peStatic->peFlags = EXACT_MATCH;
                pe332->peFlags = COLOR_USED;
                aiDefaultOverride[iStatic] = i332;
                
                break;
            }

            pe332++;
        }

        peStatic++;
    }
    
    // Match each static color as closely as possible to an entry
    // in the 332 palette by minimized the square of the distance
    peStatic = apeDefaultPalEntry;
    for (iStatic = 0; iStatic < STATIC_COLORS; iStatic++)
    {
        // Skip colors already matched exactly
        if (peStatic->peFlags == EXACT_MATCH)
        {
            peStatic++;
            continue;
        }
        
        iMinDist = MAX_COL_DIST+1;
#if DBG
        iMinEntry = -1;
#endif

        pe332 = pe332Palette;
        for (i332 = 0; i332 < 256; i332++)
        {
            // Skip colors already used
            if (pe332->peFlags == COLOR_USED)
            {
                pe332++;
                continue;
            }
            
            // Compute Euclidean distance squared
            iDelta = pe332->peRed-peStatic->peRed;
            iDist = iDelta*iDelta;
            iDelta = pe332->peGreen-peStatic->peGreen;
            iDist += iDelta*iDelta;
            iDelta = pe332->peBlue-peStatic->peBlue;
            iDist += iDelta*iDelta;

            if (iDist < iMinDist)
            {
                iMinDist = iDist;
                iMinEntry = i332;
            }

            pe332++;
        }

        TKASSERT(iMinEntry != -1);

        // Remember the best match
        aiDefaultOverride[iStatic] = iMinEntry;
        pe332Palette[iMinEntry].peFlags = COLOR_USED;
        
        peStatic++;
    }

    // Zero the flags in the static colors because they may have been
    // set.  We want them to be zero so the colors can be remapped
    peStatic = apeDefaultPalEntry;
    for (iStatic = 0; iStatic < STATIC_COLORS; iStatic++)
    {
        peStatic->peFlags = 0;
        peStatic++;
    }

    // Reset the 332 flags because we may have set them
    pe332 = pe332Palette;
    for (i332 = 0; i332 < 256; i332++)
    {
        pe332->peFlags = PC_NOCOLLAPSE;
        pe332++;
    }
}

/******************************Public*Routine******************************\
* FlushPalette
*
* Because of Win 3.1 compatibility, GDI palette mapping always starts
* at zero and stops at the first exact match.  So if there are duplicates,
* the higher colors aren't mapped to--which is often a problem if we
* are trying to make to any of the upper 10 static colors.  To work around
* this, we flush the palette to all black.
*
\**************************************************************************/

void
SS_PAL::Flush()
{
    LOGPALETTE *pPal;
    HPALETTE hpalBlack, hpalOld;
    int i;

    if( nEntries == 256 )
    {
        pPal = (LOGPALETTE *) LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT,
                         sizeof(LOGPALETTE) + nEntries * sizeof(PALETTEENTRY));

        if (pPal)
        {
    	    pPal->palVersion = 0x300;
	        pPal->palNumEntries = (WORD)nEntries;

            // Mark everything PC_NOCOLLAPSE and PC_RESERVED to force every 
            // thing into the palette.  Colors are already black because 
            // we zero initialized during memory allocation.

            for (i = 0; i < nEntries; i++)
            {
                pPal->palPalEntry[i].peFlags = PC_NOCOLLAPSE | PC_RESERVED;
            }

            hpalBlack = CreatePalette(pPal);
            LocalFree(pPal);

            hpalOld = SelectPalette(hdc, hpalBlack, FALSE);
            RealizePalette(hdc);

            SelectPalette(hdc, hpalOld, FALSE);
            DeleteObject(hpalBlack);
        }
    }
}

/******************************Public*Routine******************************\
* Realize
*
* Select the given palette in background or foreground mode (as specified
* by the bForceBackground flag), and realize the palette.
*
* If static system color usage is set, the system colors are replaced.
*
* History:
*  26-Apr-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

//mf: ! this grabbing of colors n stuff should only be done by the top
// level window - ? but what if it's not a GL window ?

long
SS_PAL::Realize( HWND hwndArg, HDC hdcArg, BOOL bForceBackground )
{
    // cache:
    hwnd = hwndArg;
    hdc = hdcArg;

    if( bTakeOver ) {
        // Easy case
        SetSystemPaletteUse(hdc, SYSPAL_NOSTATIC);
        SelectPalette(hdc, hPal, bForceBackground );
        RealizePalette(hdc);
        return 1;
    }

    // Else general purpose 'tk' method

    if( bFlush ) {
        Flush();
        bFlush = FALSE;
    }

    return Realize( bForceBackground );
}

long
SS_PAL::Realize( BOOL bForceBackground )
{
    long Result = -1;
    BOOL bHaveSysPal = TRUE;

    SS_DBGLEVEL2( SS_LEVEL_INFO, "SS_PAL::Realize: %d for %d\n", bForceBackground, hwnd );

    // If static system color usage is set, prepare to take over the
    // system palette.

    if( bUseStatic )
    {
        // If foreground, take over the static colors.  If background, release
        // the static colors.

        if ( !bForceBackground )
        {
            // If GrabStaticEntries succeeds, then it is OK to take over the
            // static colors.  If not <mf:TBD>

            bHaveSysPal = GrabStaticEntries();
        }
        else
        {
            // If we are currently using the system colors (bSystemColorsInUse)
            // and Realize was called with bForceBackground set, we
            // are being deactivated and must release the static system colors.

            ReleaseStaticEntries();
        }

        // Rerealize the palette.
        //
        // If set to TRUE, bForceBackground will force the palette to be 
        // realized as a background palette, regardless of focus.  This 
        // will happen anyway if the TK window does not have the keyboard focus.

        if ( (bForceBackground || bHaveSysPal) &&
             UnrealizeObject( hPal ) &&
             NULL != SelectPalette( hdc, hPal, bForceBackground ) )
        {
            Result = RealizePalette( hdc );
        }
//mf: ?? klugey fix for rude apps
        // If some rude app still has the system colors and we're in the
        // foreground, make the best of it.
        if( !bForceBackground && !bHaveSysPal ) {
            if( UnrealizeObject( hPal ) &&
                NULL != SelectPalette( hdc, hPal, TRUE ) )
            {
                Result = RealizePalette( hdc );
            }
        }
    }
    else
    {
        if ( NULL != SelectPalette( hdc, hPal, FALSE ) )
        {
            Result = RealizePalette( hdc );
        }
    }

    return( Result );
}

/******************************Public*Routine******************************\
* SS_PAL constructor
*
* This creates the palette, but does not select or realize it
*
\**************************************************************************/

SS_PAL::SS_PAL( HDC hdcArg, PIXELFORMATDESCRIPTOR *ppfd, BOOL bTakeOverPalette )
{
    hwnd = 0;
    hPal = 0;
    bUseStatic = FALSE;
    bSystemColorsInUse = FALSE;
    pfd = *ppfd;  // this is for palette purposes only (other fields may not apply)
    hdc = hdcArg;
    bTakeOver = bTakeOverPalette;  // mf: for now, when this is set, it means
        // the screen saver is running in full screen mode - implying that
        // interaction with other apps not necessary

    if( bTakeOver ) {
//mf: !!! bFlush should be per-window, not per SS_PAL !!
//mf: hmmm, not so sure about that...
        bFlush = FALSE;
        bUseStatic = TRUE;
    } else {
        bFlush = TRUE;
        bUseStatic = ppfd->dwFlags & PFD_NEED_SYSTEM_PALETTE;
    }

    if( bUseStatic )
        // save current static palette usage so we can restore it
        uiOldStaticUse = GetSystemPaletteUse( hdc );

    paletteManageProc = NullPaletteManageProc;

    // Now create the palette and return
    hPal = MakeRGBPalette();
    SS_ASSERT( hPal, "SS_PAL constructor failure\n" );
}

/******************************Public*Routine******************************\
* SS_PAL destructor

\**************************************************************************/

SS_PAL::~SS_PAL()
{
    if( bUseStatic )
    {
        if( uiOldStaticUse )
            //mf: ! make sure hdc is valid !!!
            SetSystemPaletteUse(hdc, uiOldStaticUse);

        PostMessage(HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0);
    }
    if( hPal ) {
        SelectPalette( hdc, (HPALETTE) GetStockObject(DEFAULT_PALETTE), TRUE );
        DeleteObject( hPal );
    }
}

/******************************Public*Routine******************************\
* ReCreateRGBPalette
* 
*
\**************************************************************************/

void
SS_PAL::ReCreateRGBPalette()
{
    if( bTakeOver )
        return;

    HPALETTE hPalTmp = hPal;
    hPal = MakeRGBPalette();
    if( hPal ) {
        DeleteObject( hPalTmp );
        bFlush = TRUE;
    }
}

/******************************Public*Routine******************************\
* MakeRGBPalette
*
* Creates an HPALETTE with values required for a logical rgb palette.
* If bUseStatic is TRUE, the static system
* colors will be overridden.  Otherwise, the PALETTEENTRY array will be
* fixed up to contain the default static system colors.
*
\**************************************************************************/

HPALETTE
SS_PAL::MakeRGBPalette()
{
    LOGPALETTE *pPal;
    HPALETTE hpal;
    int count, i;
    PIXELFORMATDESCRIPTOR *ppfd = &pfd;

    count = 1 << ppfd->cColorBits;
    nEntries = count;
    pPal = (PLOGPALETTE)LocalAlloc(LMEM_FIXED, sizeof(LOGPALETTE) +
            count * sizeof(PALETTEENTRY));
    if( !pPal )
        return (HPALETTE) 0;

    pPal->palVersion = 0x300;
    pPal->palNumEntries = (WORD)count;

    PALETTEENTRY *pEntry = pPal->palPalEntry;

    for ( i = 0; i < count ; i++, pEntry++ )
    {
        pEntry->peRed   = ss_ComponentFromIndex(i, ppfd->cRedBits,
                                ppfd->cRedShift);
        pEntry->peGreen = ss_ComponentFromIndex(i, ppfd->cGreenBits,
                                ppfd->cGreenShift);
        pEntry->peBlue  = ss_ComponentFromIndex(i, ppfd->cBlueBits,
                                ppfd->cBlueShift);
        pEntry->peFlags = PC_NOCOLLAPSE;
    }

    if( count == 256 )
    {
    // If app set static system color usage for fixed palette support,
    // setup to take over the static colors.  Otherwise, fixup the
    // static system colors.

        if ( bUseStatic )
        {
        // Black and white already exist as the only remaining static
        // colors.  Let those remap.  All others should be put into
        // the palette (i.e., set PC_NOCOLLAPSE).

            pPal->palPalEntry[0].peFlags = 0;
            pPal->palPalEntry[255].peFlags = 0;
        }
        else
        {
        // The defaultOverride array is computed assuming a 332
        // palette where red has zero shift, etc.

            if ( (3 == ppfd->cRedBits)   && (0 == ppfd->cRedShift)   &&
                 (3 == ppfd->cGreenBits) && (3 == ppfd->cGreenShift) &&
                 (2 == ppfd->cBlueBits)  && (6 == ppfd->cBlueShift) )
            {
                pEntry = pPal->palPalEntry;
                UpdateStaticMapping( pEntry );
                
                for ( i = 0 ; i < STATIC_COLORS ; i++)
                {
                    pEntry[aiDefaultOverride[i]] = apeDefaultPalEntry[i];
                }
            }
        }
    }

    hpal = CreatePalette(pPal);
    LocalFree(pPal);
    return hpal;
}
