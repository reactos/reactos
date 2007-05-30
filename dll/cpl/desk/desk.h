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

typedef struct _APPLET
{
    int idIcon;
    int idName;
    int idDescription;
    APPLET_PROC AppletProc;
} APPLET, *PAPPLET;

typedef struct _DIBITMAP
{
    BITMAPFILEHEADER *header;
    BITMAPINFO       *info;
    BYTE             *bits;
    int               width;
    int               height;
} DIBITMAP, *PDIBITMAP;

extern HINSTANCE hApplet;

PDIBITMAP DibLoadImage(LPTSTR lpFilename);
VOID DibFreeImage(PDIBITMAP lpBitmap);

INT AllocAndLoadString(LPTSTR *lpTarget,
                       HINSTANCE hInst,
                       UINT uID);

DWORD DbgPrint(PCH Format,...);

#endif /* __CPL_DESK_H__ */

