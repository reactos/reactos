/******************************Module*Header*******************************\
* Module Name: palette.hxx
*
* Copyright (c) 1996 Microsoft Corporation
*
\**************************************************************************/

#ifndef __palette_hxx__
#define __palette_hxx__

typedef LONG (*PALETTEMANAGEPROC)(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);


/**************************************************************************\
* SS_PAL(ETTE)
*
\**************************************************************************/

class SS_PAL {
public:
    HPALETTE hPal;
    BOOL     bUseStatic;
    BOOL     bTakeOver; // usually if in full screen mode
//mf: !!! this should be per window !!!
    BOOL     bFlush; // if palette needs to be flushed
    BOOL     bSystemColorsInUse;
    UINT     uiOldStaticUse;  // original static mode at startup
    PIXELFORMATDESCRIPTOR pfd;
    PALETTEMANAGEPROC paletteManageProc;
    
    SS_PAL( HDC hdc, PIXELFORMATDESCRIPTOR *ppfd, BOOL bTakeOverPalette );
    ~SS_PAL();
    long     Realize( HWND hwnd, HDC hdc, BOOL bForceBackground );
    void     ReCreateRGBPalette();
    void     SetDC( HDC hdcArg ) { hdc = hdcArg; }
private:
    HWND     hwnd;  // cached here for convenience sometimes during processing
    HDC      hdc;   //           "
    int      nEntries;  // number of entries in palette
    HPALETTE MakeRGBPalette();
    BOOL     GrabStaticEntries();
    BOOL     ReleaseStaticEntries();
    long     Realize( BOOL bForceBackground );
    void     Flush();
};

#endif // __palette_hxx__
