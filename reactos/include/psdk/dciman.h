/*
 * DCI driver interface
 *
 * Copyright (C) 2005 Francois Gouget
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef _INC_DCIMAN
#define _INC_DCIMAN

#ifdef __cplusplus
    #define __inline inline
    extern "C" {
#endif

#include "dciddi.h"

DECLARE_HANDLE(HWINWATCH);

#define WINWATCHNOTIFY_START            0
#define WINWATCHNOTIFY_STOP             1
#define WINWATCHNOTIFY_DESTROY          2
#define WINWATCHNOTIFY_CHANGING         3
#define WINWATCHNOTIFY_CHANGED          4


extern HWINWATCH WINAPI WinWatchOpen(HWND hwnd);
extern void WINAPI WinWatchClose(HWINWATCH hWW);

extern UINT WINAPI WinWatchGetClipList(HWINWATCH hWW,
                                       LPRECT prc,
                                       UINT size,
                                       LPRGNDATA prd);

extern BOOL WINAPI WinWatchDidStatusChange(HWINWATCH hWW);

typedef void (CALLBACK *WINWATCHNOTIFYPROC)(HWINWATCH hww,
                                            HWND hwnd,
                                            DWORD code,
                                            LPARAM lParam);

extern BOOL WINAPI WinWatchNotify(HWINWATCH hWW,
                                  WINWATCHNOTIFYPROC NotifyCallback,
                                  LPARAM NotifyParam );

extern HDC WINAPI DCIOpenProvider(void);
extern void WINAPI DCICloseProvider(HDC hdc);

extern int WINAPI DCICreatePrimary(HDC hdc, 
                                   LPDCISURFACEINFO *lplpSurface);

extern int WINAPI DCICreateOverlay(HDC hdc,
                                   LPVOID lpOffscreenSurf,
                                   LPDCIOVERLAY *lplpSurface);

extern int WINAPI DCICreateOffscreen(HDC hdc,
                                     DWORD dwCompression,
                                     DWORD dwRedMask,
                                     DWORD dwGreenMask,
                                     DWORD dwBlueMask,
                                     DWORD dwWidth,
                                     DWORD dwHeight,
                                     DWORD dwDCICaps,
                                     DWORD dwBitCount,
                                     LPDCIOFFSCREEN *lplpSurface);

extern int WINAPI DCIEnum(HDC hdc,
                          LPRECT lprDst,
                          LPRECT lprSrc,
                          LPVOID lpFnCallback,
                          LPVOID lpContext);

extern DCIRVAL WINAPI DCISetSrcDestClip(LPDCIOFFSCREEN pdci,
                                        LPRECT srcrc,
                                        LPRECT destrc,
                                        LPRGNDATA prd);

extern DWORD WINAPI GetDCRegionData(HDC hdc, DWORD size, LPRGNDATA prd);
extern DWORD WINAPI GetWindowRegionData(HWND hwnd, DWORD size, LPRGNDATA prd);

#ifdef WIN32

    extern DCIRVAL WINAPI DCIBeginAccess(LPDCISURFACEINFO pdci, int x, int y, int dx, int dy);
    extern void WINAPI DCIEndAccess(LPDCISURFACEINFO pdci);

    extern DCIRVAL WINAPI DCIDraw(LPDCIOFFSCREEN pdci);
    extern DCIRVAL WINAPI DCISetClipList(LPDCIOFFSCREEN pdci, LPRGNDATA prd);
    extern DCIRVAL WINAPI DCISetDestination(LPDCIOFFSCREEN pdci, LPRECT dst, LPRECT src);
    extern void WINAPI DCIDestroy(LPDCISURFACEINFO pdci);
#else
    extern int WINAPI DCISendCommand(HDC hdc, VOID  *pcmd, int nSize, VOID  ** lplpOut);

    __inline DCIRVAL DCISetDestination(LPDCIOFFSCREEN pdci, LPRECT dst, LPRECT src)
    {
        DCIRVAL retValue = DCI_OK;

        if( pdci->SetDestination != NULL )
        {
            retValue = pdci->SetDestination(pdci, dst, src);
        }

        return retValue;
    }

    __inline DCIRVAL DCIDraw(LPDCIOFFSCREEN pdci)
    {
        DCIRVAL retValue = DCI_OK;
        if( pdci->Draw != NULL )
        {
            retValue = pdci->Draw(pdci);
        }
        return retValue;
    }

    __inline DCIRVAL DCIBeginAccess(LPDCISURFACEINFO pdci, int x, int y, int dx, int dy)
    {
        RECT rc;
        DCIRVAL retValue = DCI_OK;
        if( pdci->BeginAccess != NULL )
        {
            rc.top=y;
            rc.left=x;
            rc.right = rc.left+dx;
            rc.bottom = rc.top+dy;
            retValue = pdci->BeginAccess(pdci, &rc);
        }
        return retValue;
    }

    __inline DCIRVAL DCISetClipList(LPDCIOFFSCREEN pdci, LPRGNDATA prd)
    {
        DCIRVAL retValue = DCI_OK;
        if( pdci->SetClipList != NULL )
        {
            retValue = pdci->SetClipList(pdci, prd);
        }
        return retValue;
    }

    __inline void DCIDestroy(LPDCISURFACEINFO pdci)
    {
        if( pdci->DestroySurface != NULL )
        {
            pdci->DestroySurface(pdci);
        }
    }

    __inline void DCIEndAccess(LPDCISURFACEINFO pdci)
    {
        if( pdci->EndAccess != NULL )
        {
            pdci->EndAccess(pdci);
        }
    }
#endif

#ifdef __cplusplus
    }
#endif

#endif // _INC_DCIMAN
