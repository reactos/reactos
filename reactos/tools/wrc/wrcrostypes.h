/*
  PROJECT:    ReactOS
  LICENSE:    GPL v2 or any later version
  FILE:       tools/wrc/wrcrostypes.h
  PURPOSE:    Definitions and macros from Windows headers, which are needed for wrc
  COPYRIGHT:  Copyright 2007 Colin Finck <mail@colinfinck.de>
*/

#ifndef _WRC_ROSTYPES_H
#define _WRC_ROSTYPES_H

#include <host/typedefs.h>
#include <host/nls.h>
#include <string.h>

// Definitions copied from various <win....h> files
// We only want to include host headers, so we define them manually
#define VS_FFI_SIGNATURE     0xFEEF04BD
#define VS_FFI_STRUCVERSION  0x10000

#define BS_3STATE            5
#define BS_AUTO3STATE        6
#define BS_AUTOCHECKBOX      3
#define BS_AUTORADIOBUTTON   9
#define BS_CHECKBOX          2
#define CBS_DROPDOWN         2
#define CBS_DROPDOWNLIST     3
#define CBS_SIMPLE           1
#define BS_DEFPUSHBUTTON     1
#define BS_GROUPBOX          7
#define BS_PUSHBUTTON        0
#define BS_RADIOBUTTON       4
#define DS_SETFONT           64
#define ES_LEFT              0
#define LBS_NOTIFY           1
#define MF_CHECKED           8
#define MF_GRAYED            1
#define MF_DISABLED          2
#define MF_HELP              0x4000
#define MF_MENUBARBREAK      32
#define MF_MENUBREAK         64
#define MF_POPUP             16
#define MF_END               128
#define SBS_HORZ             0
#define SS_CENTER            1
#define SS_ICON              3
#define SS_LEFT              0
#define SS_RIGHT             2
#define WS_BORDER            0x800000
#define WS_CAPTION           0xc00000
#define WS_CHILD             0x40000000
#define WS_GROUP             0x20000
#define WS_POPUP             0x80000000
#define WS_POPUPWINDOW       0x80880000
#define WS_TABSTOP           0x10000
#define WS_VISIBLE           0x10000000

#include <host/pshpack2.h>
typedef struct tagBITMAPFILEHEADER {
   WORD   bfType;
   DWORD  bfSize;
   WORD   bfReserved1;
   WORD   bfReserved2;
   DWORD  bfOffBits;
} BITMAPFILEHEADER,*LPBITMAPFILEHEADER,*PBITMAPFILEHEADER;
#include <host/poppack.h>

typedef int FXPT2DOT30;
typedef struct tagCIEXYZ {
   FXPT2DOT30 ciexyzX;
   FXPT2DOT30 ciexyzY;
   FXPT2DOT30 ciexyzZ;
} CIEXYZ,*LPCIEXYZ;

typedef struct tagCIEXYZTRIPLE {
   CIEXYZ ciexyzRed;
   CIEXYZ ciexyzGreen;
   CIEXYZ ciexyzBlue;
} CIEXYZTRIPLE,*LPCIEXYZTRIPLE;

typedef struct tagBITMAPINFOHEADER{
   DWORD  biSize;
   LONG   biWidth;
   LONG   biHeight;
   WORD   biPlanes;
   WORD   biBitCount;
   DWORD  biCompression;
   DWORD  biSizeImage;
   LONG   biXPelsPerMeter;
   LONG   biYPelsPerMeter;
   DWORD  biClrUsed;
   DWORD  biClrImportant;
} BITMAPINFOHEADER,*LPBITMAPINFOHEADER,*PBITMAPINFOHEADER;

typedef struct {
   DWORD  bV4Size;
   LONG   bV4Width;
   LONG   bV4Height;
   WORD   bV4Planes;
   WORD   bV4BitCount;
   DWORD  bV4V4Compression;
   DWORD  bV4SizeImage;
   LONG   bV4XPelsPerMeter;
   LONG   bV4YPelsPerMeter;
   DWORD  bV4ClrUsed;
   DWORD  bV4ClrImportant;
   DWORD  bV4RedMask;
   DWORD  bV4GreenMask;
   DWORD  bV4BlueMask;
   DWORD  bV4AlphaMask;
   DWORD  bV4CSType;
   CIEXYZTRIPLE bV4Endpoints;
   DWORD  bV4GammaRed;
   DWORD  bV4GammaGreen;
   DWORD  bV4GammaBlue;
} BITMAPV4HEADER,*LPBITMAPV4HEADER,*PBITMAPV4HEADER;

#endif
