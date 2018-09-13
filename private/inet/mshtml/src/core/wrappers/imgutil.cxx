//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       imgutil.cxx
//
//  Contents:   Dynamic wrappers for plugin image decoder support DLL.
//
//----------------------------------------------------------------------------

#include "precomp.hxx"

#ifndef X_OCMM_H_
#define X_OCMM_H_
#include "ocmm.h"
#endif

#ifndef X_CDERR_H_
#define X_CDERR_H_
#include <cderr.h>
#endif

#ifdef UNIX
#ifndef X_DDRAW_H_
#define X_DDRAW_H_
#include <ddraw.h>
#endif
#endif

DYNLIB g_dynlibIMGUTIL = { NULL, NULL, "imgutil.dll" };

void APIENTRY
InitImgUtil(void)
{
    static DYNPROC s_dynprocInitImgUtil =
            { NULL, &g_dynlibIMGUTIL, "InitImgUtil" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocInitImgUtil);
    if (hr)
        return ;

    (*(void (APIENTRY *)(void))s_dynprocInitImgUtil.pfn)
            ();
}

STDAPI DecodeImage( IStream* pStream, IMapMIMEToCLSID* pMap, 
   IUnknown* pEventSink )
{
    static DYNPROC s_dynprocDecodeImage =
            { NULL, &g_dynlibIMGUTIL, "DecodeImage" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocDecodeImage);
    if (hr)
        return NULL;

    return(*(HRESULT (APIENTRY *)(IStream*, IMapMIMEToCLSID*, IUnknown*))s_dynprocDecodeImage.pfn)
            (pStream, pMap, pEventSink);
}

STDAPI CreateMIMEMap( IMapMIMEToCLSID** ppMap )
{
    static DYNPROC s_dynprocCreateMIMEMap =
            { NULL, &g_dynlibIMGUTIL, "CreateMIMEMap" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocCreateMIMEMap);
    if (hr)
        return NULL;

    return(*(HRESULT (APIENTRY *)(IMapMIMEToCLSID**))s_dynprocCreateMIMEMap.pfn)
            (ppMap);
}

STDAPI GetMaxMIMEIDBytes( ULONG* pnMaxBytes )
{
    static DYNPROC s_dynprocGetMaxMIMEIDBytes =
            { NULL, &g_dynlibIMGUTIL, "GetMaxMIMEIDBytes" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocGetMaxMIMEIDBytes);
    if (hr)
        return NULL;

    return(*(HRESULT (APIENTRY *)(ULONG*))s_dynprocGetMaxMIMEIDBytes.pfn)
            (pnMaxBytes);
}

STDAPI IdentifyMIMEType( const BYTE* pbBytes, ULONG nBytes, 
   UINT* pnFormat )
{
    static DYNPROC s_dynprocIdentifyMIMEType =
            { NULL, &g_dynlibIMGUTIL, "IdentifyMIMEType" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocIdentifyMIMEType);
    if (hr)
        return NULL;

    return(*(HRESULT (APIENTRY *)(const BYTE*, ULONG, UINT*))s_dynprocIdentifyMIMEType.pfn)
            (pbBytes, nBytes, pnFormat);
}

STDAPI SniffStream( IStream* pInStream, UINT* pnFormat, 
   IStream** ppOutStream )
{
    static DYNPROC s_dynprocSniffStream =
            { NULL, &g_dynlibIMGUTIL, "SniffStream" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocSniffStream);
    if (hr)
        return NULL;

    return(*(HRESULT (APIENTRY *)(IStream*, UINT*, IStream**))s_dynprocSniffStream.pfn)
            (pInStream, pnFormat, ppOutStream);
}

STDAPI ComputeInvCMAP(const RGBQUAD *pRGBColors, ULONG nColors, BYTE *pInvTable, ULONG cbTable)
{
    static DYNPROC s_dynprocComputeInvCMAP =
            { NULL, &g_dynlibIMGUTIL, "ComputeInvCMAP" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocComputeInvCMAP);
    if (hr)
        return NULL;

    return(*(HRESULT (APIENTRY *)(const RGBQUAD *, ULONG, BYTE *, ULONG))s_dynprocComputeInvCMAP.pfn)
            (pRGBColors, nColors, pInvTable, cbTable);
}

STDAPI CreateDDrawSurfaceOnDIB(HBITMAP hbmDib, IDirectDrawSurface **ppSurface)
{
    static DYNPROC s_dynprocCreateDDrawSurfaceOnDIB =
            { NULL, &g_dynlibIMGUTIL, "CreateDDrawSurfaceOnDIB" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocCreateDDrawSurfaceOnDIB);
    if (hr)
        return NULL;

    return(*(HRESULT (APIENTRY *)(HBITMAP, IDirectDrawSurface **))s_dynprocCreateDDrawSurfaceOnDIB.pfn)
            (hbmDib, ppSurface);
}

#if 0

// KENSY: This functionality is folded into MSHTML so that IMGUTIL doesn't
//        have to be loaded

STDAPI DitherTo8( BYTE * pDestBits, LONG nDestPitch, 
                   BYTE * pSrcBits, LONG nSrcPitch, REFGUID bfidSrc, 
                   RGBQUAD * prgbDestColors, RGBQUAD * prgbSrcColors,
                   BYTE * pbDestInvMap,
                   LONG x, LONG y, LONG cx, LONG cy,
                   LONG lDestTrans, LONG lSrcTrans)
{
    static DYNPROC s_dynprocDitherTo8 =
            { NULL, &g_dynlibIMGUTIL, "DitherTo8" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocDitherTo8);
    if (hr)
        return NULL;

    return(*(HRESULT (APIENTRY *)(BYTE *, LONG, BYTE *, LONG, REFGUID, RGBQUAD *, RGBQUAD *, BYTE *, LONG, LONG, LONG, LONG, LONG, LONG))s_dynprocDitherTo8.pfn)
        (pDestBits, nDestPitch, pSrcBits, nSrcPitch, bfidSrc, prgbDestColors, prgbSrcColors, pbDestInvMap, x, y, cx, cy, lDestTrans, lSrcTrans);
}

#endif
