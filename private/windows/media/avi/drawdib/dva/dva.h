/****************************************************************************

 DVA.H

 Copyright (c) 1993-1994 Microsoft Corporation

 DVA 1.0 Interface Definitions

 ***************************************************************************/

#ifndef _INC_DVA
#define _INC_DVA
#ifdef WIN32
#define DVABeginAccess(hdva, x, y, dx, dy) 0
#define DVAEndAccess(hdva) 0
#define DVAGetSurfaceFmt(hdva) 0
#define DVAGetSurfacePtr(hdva) 0
#else

#ifdef __cplusplus
    #define __inline inline
    extern "C" {
#endif

/****************************************************************************
 ***************************************************************************/

#include "dvaddi.h"     // interface to the display driver

/****************************************************************************
 ***************************************************************************/

typedef DVASURFACEINFO FAR *PDVA;
typedef PDVA HDVA;

/****************************************************************************
 ***************************************************************************/

//
// this code in biCompression means the frame buffer must be accesed via
// 48 bit pointers! using *ONLY* the given selector
//
// BI_1632 has bitmasks (just like BI_BITFIELDS) for biBitCount == 16,24,32
//
#ifndef BI_1632
#define BI_1632  0x32333631     // '1632'
#endif

#ifndef BI_BITFIELDS
#define BI_BITFIELDS 3
#endif

/****************************************************************************
 ***************************************************************************/

#if defined(_INC_VFW) || defined(_INC_DRAWDIB)
//
//  this API is in MSVIDEO.DLL
//
BOOL VFWAPI DVAGetSurface(HDC hdc, int nSurface, DVASURFACEINFO FAR *lpSurfaceInfo);

#else

//
//  this API uses the Escape to the display driver only
//
__inline BOOL DVAGetSurface(HDC hdc, int nSurface, DVASURFACEINFO FAR *pdva)
{
    int i;

    i = Escape(hdc, DVAGETSURFACE,sizeof(int),(LPCSTR)&nSurface,(LPVOID)pdva);

    return i > 0;
}

#endif

/****************************************************************************
 ***************************************************************************/

__inline PDVA DVAOpenSurface(HDC hdc, int nSurface)
{
    PDVA pdva;

    pdva = (PDVA)GlobalLock(GlobalAlloc(GHND|GMEM_SHARE, sizeof(DVASURFACEINFO)));

    if (pdva == NULL)
        return NULL;

    if (!DVAGetSurface(hdc, nSurface, pdva) ||
        !pdva->OpenSurface(pdva->lpSurface))
    {
        GlobalFree((HGLOBAL)SELECTOROF(pdva));
        return NULL;
    }

    return pdva;
}

/****************************************************************************
 ***************************************************************************/

__inline void DVACloseSurface(PDVA pdva)
{
    if (pdva == NULL)
        return;

    pdva->CloseSurface(pdva->lpSurface);

    GlobalFree((HGLOBAL)SELECTOROF(pdva));
}

/****************************************************************************
 ***************************************************************************/

__inline BOOL DVABeginAccess(PDVA pdva, int x, int y, int dx, int dy)
{
    return pdva->BeginAccess(pdva->lpSurface, x, y, dx, dy);
}

/****************************************************************************
 ***************************************************************************/

__inline void DVAEndAccess(PDVA pdva)
{
    pdva->EndAccess(pdva->lpSurface);
}

/****************************************************************************
 ***************************************************************************/

__inline LPBITMAPINFOHEADER DVAGetSurfaceFmt(PDVA pdva)
{
    if (pdva == NULL)
        return NULL;

    return &pdva->BitmapInfo;
}

/****************************************************************************
 ***************************************************************************/

__inline LPVOID DVAGetSurfacePtr(PDVA pdva)
{
    if (pdva == NULL)
        return NULL;

    return (LPVOID)MAKELONG(pdva->offSurface, pdva->selSurface);
}

#ifdef __cplusplus
    }
#endif

#endif // else WIN32
#endif // _INC_DVA
