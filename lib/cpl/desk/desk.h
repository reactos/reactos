#ifndef __CPL_DESK_H__
#define __CPL_DESK_H__

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <cpl.h>
#include <tchar.h>
#include <setupapi.h>
#include <stdio.h>

#include "resource.h"

typedef struct
{
    int idIcon;
    int idName;
    int idDescription;
    
    APPLET_PROC AppletProc;
    
} APPLET, *PAPPLET;

extern HINSTANCE hApplet;

typedef struct
{
    BITMAPFILEHEADER *header;
    BITMAPINFO       *info;
    BYTE             *bits;
    
    int               width;
    int               height;

} DIBitmap;

extern DIBitmap *DibLoadImage(TCHAR *filename);
extern void DibFreeImage(DIBitmap *bitmap);

DWORD DbgPrint(PCH Format,...);

#endif /* __CPL_DESK_H__ */

