/****************************************************************************
 *
 *   cappal.h
 *
 *   Microsoft Video for Windows Sample Capture Class
 *
 *   Copyright (c) 1992-1994 Microsoft Corporation.  All Rights Reserved.
 *
 *    You have a royalty-free right to use, modify, reproduce and
 *    distribute the Sample Files (and/or any modified version) in
 *    any way you find useful, provided that you agree that
 *    Microsoft has no warranty obligations or liability for any
 *    Sample Application Files which are modified.
 *
 ***************************************************************************/

// Structure used when capturing a palette
typedef struct tagCAPPAL {
    WORD                wNumFrames;
    WORD                wNumColors;
    LPBYTE              lpBits;
    LPBYTE              lp16to8;
    VIDEOHDR            vHdr;
    BITMAPINFOHEADER    bi16;
    LPHISTOGRAM         lpHistogram;
    LPBITMAPINFO        lpbiSave;
} CAPPAL, FAR * LPCAPPAL;

BOOL PalInit (LPCAPSTREAM lpcs);
void PalFini (LPCAPSTREAM lpcs);
void PalDeleteCurrentPalette (LPCAPSTREAM lpcs);
BOOL PalGetPaletteFromDriver (LPCAPSTREAM lpcs);
DWORD PalSendPaletteToDriver (LPCAPSTREAM lpcs, HPALETTE hpal, LPBYTE lpXlateTable);
HPALETTE CopyPalette (HPALETTE hpal);
DWORD CapturePaletteInit (LPCAPSTREAM lpcs, LPCAPPAL lpcp);
DWORD CapturePaletteFini (LPCAPSTREAM lpcs, LPCAPPAL lpcp);
DWORD CapturePaletteFrames (LPCAPSTREAM lpcs, LPCAPPAL lpCapPal, int nCount);
BOOL CapturePaletteAuto (LPCAPSTREAM lpcs, int nCount, int nColors);
BOOL CapturePaletteManual (LPCAPSTREAM lpcs, BOOL fGrab, int nColors);
void CapturePaletteDialog (LPCAPSTREAM lpcs);
LONG FAR PASCAL EXPORT cappalDlgProc(HWND hwnd, unsigned msg, UINT wParam, LONG lParam);

