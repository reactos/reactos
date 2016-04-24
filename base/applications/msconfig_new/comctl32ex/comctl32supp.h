/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig_new/comctl32ex/comctl32supp.h
 * PURPOSE:     Common Controls helper functions.
 * COPYRIGHT:   Copyright 2011-2012 Hermes BELUSCA - MAITO <hermes.belusca@sfr.fr>
 */

#ifndef __COMCTL32SUPP_H__
#define __COMCTL32SUPP_H__

#include <windowsx.h>
/*
#define GET_X_LPARAM(lp)    ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)    ((int)(short)HIWORD(lp))
*/

#define Button_IsEnabled(hwndCtl) IsWindowEnabled((hwndCtl))

#define UM_CHECKSTATECHANGE (WM_USER + 100)

#if 0
// this typedef, present in newer include files,
// supports the building tokenmon on older systems
typedef struct _DLLVERSIONINFO
{
    DWORD cbSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformID;
} DLLVERSIONINFO, *PDLLVERSIONINFO;
// Version information function
typedef HRESULT (WINAPI* DLLGETVERSIONPROC)(PDLLVERSIONINFO);
#endif

HRESULT GetComCtl32Version(OUT PDWORD pdwMajor, OUT PDWORD pdwMinor, OUT PDWORD pdwBuild);

#endif // __COMCTL32SUPP_H__
