/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/include/engobj.h
 * PURPOSE:         Eng* Structures
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

#ifndef _WIN32K_ENGOBJ_H
#define _WIN32K_ENGOBJ_H

//#define PAL_INDEXED          0x000001
//#define PAL_BITFIELDS        0x000002
//#define PAL_RGB              0x000004
//#define PAL_BGR              0x000008
//#define PAL_CMYK             0x000010
#define PAL_DC               0x000100
#define PAL_FIXED            0x000200
#define PAL_FREE             0x000400
#define PAL_MANAGED          0x000800
#define PAL_NOSTATIC         0x001000
#define PAL_MONOCHROME       0x002000
#define PAL_BRUSHHACK        0x004000
#define PAL_DIBSECTION       0x008000
#define PAL_HT               0x100000
#define PAL_RGB16_555        0x200000
#define PAL_RGB16_565        0x400000
#define PAL_GAMMACORRECTION  0x800000

typedef unsigned long HDEVPPAL;

typedef struct _EPALOBJ
{
    BASEOBJECT BaseObject;
    FLONG flPal;
    ULONG cEntries;
    ULONG ulUnique;
    HDC hdcHead;
    HDEVPPAL hSelected;
    ULONG cRefhpal;
    ULONG cRefRegular;
    PVOID ptransFore; // PTRANSLATE
    PVOID ptransCurrent; // PTRANSLATE
    PVOID ptransOld; // PTRANSLATE
    ULONG unk_038;
    PFN pfnGetNearest;
    PFN pfnGetMatch;
    ULONG ulRGBTime;
    struct _PRGB555XL *pRGBXlate;
    PALETTEENTRY *pFirstColor;  // 0x4c, this->apalColors, attention!! Yuan is wrong here
    struct _EPALOBJ *pPalette;     // 0x50, this, attention!! Yuan is wrong here
    PALETTEENTRY apalColors[1];
} EPALOBJ, *PEPALOBJ;

#endif // _WIN32K_ENGOBJ_H
