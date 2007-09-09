/*
  PROJECT:    ReactOS
  LICENSE:    GPL v2 or any later version
  FILE:       tools/wrc/wrcrostypes.h
  PURPOSE:    Definitions and macros from Windows headers, which are needed for wrc
  COPYRIGHT:  Copyright 2007 Colin Finck <mail@colinfinck.de>
*/

#ifndef _WRC_ROSTYPES_H
#define _WRC_ROSTYPES_H

#include <typedefs_host.h>
#include <string.h>

// Definitions copied from various <win....h> files
// We only want to include host headers, so we define them manually
#define LOBYTE(w)            ((BYTE)(w))
#define HIBYTE(w)            ((BYTE)(((WORD)(w)>>8)&0xFF))
#define LOWORD(l)            ((WORD)((DWORD_PTR)(l)))
#define HIWORD(l)            ((WORD)(((DWORD_PTR)(l)>>16)&0xFFFF))
#define MAKELANGID(p,s)      ((((WORD)(s))<<10)|(WORD)(p))
#define VS_FFI_SIGNATURE     0xFEEF04BD
#define VS_FFI_STRUCVERSION  0x10000
#define PRIMARYLANGID(l)     ((WORD)(l)&0x3ff)
#define SUBLANGID(l)         ((WORD)(l)>>10)

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

#define LANG_AFRIKAANS       0x36
#define LANG_ALBANIAN        0x1c
#define LANG_ARABIC          0x01
#define LANG_ARMENIAN        0x2b
#define LANG_AZERI           0x2c
#define LANG_BASQUE          0x2d
#define LANG_BELARUSIAN      0x23
#define LANG_BULGARIAN       0x02
#define LANG_CATALAN         0x03
#define LANG_CHINESE         0x04
#define LANG_CROATIAN        0x1a
#define LANG_CZECH           0x05
#define LANG_DANISH          0x06
#define LANG_DIVEHI          0x65
#define LANG_DUTCH           0x13
#define LANG_ENGLISH         0x09
#define LANG_ESTONIAN        0x25
#define LANG_FAEROESE        0x38
#define LANG_FARSI           0x29
#define LANG_FINNISH         0x0b
#define LANG_FRENCH          0x0c
#define LANG_GALICIAN        0x56
#define LANG_GEORGIAN        0x37
#define LANG_GERMAN          0x07
#define LANG_GREEK           0x08
#define LANG_GUJARATI        0x47
#define LANG_HEBREW          0x0d
#define LANG_HINDI           0x39
#define LANG_HUNGARIAN       0x0e
#define LANG_ICELANDIC       0x0f
#define LANG_INDONESIAN      0x21
#define LANG_ITALIAN         0x10
#define LANG_JAPANESE        0x11
#define LANG_KANNADA         0x4b
#define LANG_KAZAK           0x3f
#define LANG_KONKANI         0x57
#define LANG_KOREAN          0x12
#define LANG_KYRGYZ          0x40
#define LANG_LATVIAN         0x26
#define LANG_LITHUANIAN      0x27
#define LANG_MACEDONIAN      0x2f
#define LANG_MALAY           0x3e
#define LANG_MARATHI         0x4e
#define LANG_MONGOLIAN       0x50
#define LANG_NEUTRAL         0x00
#define LANG_NORWEGIAN       0x14
#define LANG_POLISH          0x15
#define LANG_PORTUGUESE      0x16
#define LANG_PUNJABI         0x46
#define LANG_ROMANIAN        0x18
#define LANG_RUSSIAN         0x19
#define LANG_SANSKRIT        0x4f
#define LANG_SERBIAN         0x1a
#define LANG_SLOVAK          0x1b
#define LANG_SLOVENIAN       0x24
#define LANG_SPANISH         0x0a
#define LANG_SWAHILI         0x41
#define LANG_SWEDISH         0x1d
#define LANG_SYRIAC          0x5a
#define LANG_TAMIL           0x49
#define LANG_TATAR           0x44
#define LANG_TELUGU          0x4a
#define LANG_THAI            0x1e
#define LANG_TURKISH         0x1f
#define LANG_UKRAINIAN       0x22
#define LANG_URDU            0x20
#define LANG_UZBEK           0x43
#define LANG_VIETNAMESE      0x2a
#define SUBLANG_AZERI_CYRILLIC   0x02
#define SUBLANG_CHINESE_SINGAPORE   0x04
#define SUBLANG_CHINESE_SIMPLIFIED   0x02
#define SUBLANG_NEUTRAL      0x00
#define SUBLANG_SERBIAN_CYRILLIC   0x03
#define SUBLANG_UZBEK_CYRILLIC   0x02

#include <pshpack2.h>
typedef struct tagBITMAPFILEHEADER {
   WORD   bfType;
   DWORD  bfSize;
   WORD   bfReserved1;
   WORD   bfReserved2;
   DWORD  bfOffBits;
} BITMAPFILEHEADER,*LPBITMAPFILEHEADER,*PBITMAPFILEHEADER;
#include <poppack.h>

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
