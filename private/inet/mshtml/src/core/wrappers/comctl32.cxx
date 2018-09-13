//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       comctl32.cxx
//
//  Contents:   Dynamic wrappers for common control procedures.
//
//----------------------------------------------------------------------------

#include "precomp.hxx"

#undef WINCOMMCTRLAPI
#define WINCOMMCTRLAPI
#include <commctrl.h>

#ifndef X_COMCTRLP_H_
#define X_COMCTRLP_H_
#include "comctrlp.h"
#endif

#ifndef X_CDERR_H_
#define X_CDERR_H_
#include <cderr.h>
#endif

DYNLIB g_dynlibCOMCTL32 = { NULL, NULL, "comctl32.dll" };

void APIENTRY
InitCommonControls(void)
{
    static DYNPROC s_dynprocInitCommonControls =
            { NULL, &g_dynlibCOMCTL32, "InitCommonControls" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocInitCommonControls);
    if (hr)
        return ;

    (*(void (APIENTRY *)(void))s_dynprocInitCommonControls.pfn)
            ();
}


BOOL APIENTRY
InitCommonControlsEx(LPINITCOMMONCONTROLSEX lpICC)
{
    static DYNPROC s_dynprocInitCommonControlsEx =
            { NULL, &g_dynlibCOMCTL32, "InitCommonControlsEx" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocInitCommonControlsEx);
    if (hr)
        return FALSE;

    return(*(BOOL (APIENTRY *)(LPINITCOMMONCONTROLSEX))s_dynprocInitCommonControlsEx.pfn)
            (lpICC);
}


HIMAGELIST APIENTRY
WINAPI ImageList_Create(int cx,int cy,UINT flags,int cInitial,int cGrow)
{
    static DYNPROC s_dynprocImageList_Create =
            { NULL, &g_dynlibCOMCTL32, "ImageList_Create" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocImageList_Create);
    if (hr)
        return NULL;

    return(*(HIMAGELIST (APIENTRY *)(int ,int ,UINT ,int ,int ))s_dynprocImageList_Create.pfn)
            (cx,cy,flags,cInitial, cGrow);
}


int APIENTRY
WINAPI ImageList_AddMasked( HIMAGELIST  himl,   // handle to the image list
                            HBITMAP  hbmImage,  // handle to the bitmap
                            COLORREF  crMask    // color used to generate mask
                            )
{
    static DYNPROC s_dynprocImageList_AddMasked =
            { NULL, &g_dynlibCOMCTL32, "ImageList_AddMasked" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocImageList_AddMasked);
    if (hr)
        return -1;

    return(*(int (APIENTRY *)(HIMAGELIST ,HBITMAP , COLORREF ))s_dynprocImageList_AddMasked.pfn)
            (himl,hbmImage,crMask);
}


HIMAGELIST APIENTRY
WINAPI ImageList_Merge(HIMAGELIST  himl1,
                       int  i1,
                       HIMAGELIST  himl2,
                       int  i2,
                       int  dx,
                       int  dy
                       )
{
    static DYNPROC s_dynprocImageList_Merge =
            { NULL, &g_dynlibCOMCTL32, "ImageList_Merge" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocImageList_Merge);
    if (hr)
        return NULL;

    return(*(HIMAGELIST (APIENTRY *)(HIMAGELIST,int, HIMAGELIST,int,int,int))s_dynprocImageList_Merge.pfn)
            (himl1, i1,himl2, i2, dx, dy);
}


HICON APIENTRY
WINAPI ImageList_GetIcon( HIMAGELIST himl, int  i, UINT  flags )
{
    static DYNPROC s_dynprocImageList_GetIcon =
            { NULL, &g_dynlibCOMCTL32, "ImageList_GetIcon" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocImageList_GetIcon);
    if (hr)
        return NULL;

    return(*(HICON (APIENTRY *)(HIMAGELIST , int, UINT  ))s_dynprocImageList_GetIcon.pfn)
            (himl,  i, flags );
}



BOOL APIENTRY
WINAPI ImageList_Destroy( HIMAGELIST  himl)
{
    static DYNPROC s_dynprocImageList_Destroy =
            { NULL, &g_dynlibCOMCTL32, "ImageList_Destroy" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocImageList_Destroy);
    if (hr)
        return FALSE;

    return(*(BOOL (APIENTRY *)(HIMAGELIST ))s_dynprocImageList_Destroy.pfn)
            (himl);

}

WINCOMMCTRLAPI void APIENTRY
WINAPI DoReaderMode(PREADERMODEINFO prmi)
{
    // use ordinal instead of function name
    //
    static DYNPROC s_dynprocDoReaderMode =
#ifdef UNIX
            { NULL, &g_dynlibCOMCTL32, "DoReaderMode"};
#else
            { NULL, &g_dynlibCOMCTL32, (LPSTR) 383 };
#endif

    HRESULT hr;
    hr = LoadProcedure(&s_dynprocDoReaderMode);
    if (hr)
        return;

    (*(void (APIENTRY *) (PREADERMODEINFO )) s_dynprocDoReaderMode.pfn) (prmi);
}

#ifndef PRODUCT_96

HWND APIENTRY
WINAPI CreateToolbarEx(HWND        hwnd,
                       DWORD       ws,
                       UINT        wID,
                       int         nBitmaps,
                       HINSTANCE   hBMInst,
                       UINT        wBMID,
                       LPCTBBUTTON lpButton,
                       int         iNumButtons,
                       int         dxButton,
                       int         dyButton,
                       int         dxBitmap,
                       int         dyBitmap,
                       UINT        uStructSize)
{
    static DYNPROC s_dynprocCreateToolbarEx =
            { NULL, &g_dynlibCOMCTL32, "CreateToolbarEx" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocCreateToolbarEx);
    if (hr)
        return FALSE;

    return(*(HWND (APIENTRY *)(HWND,DWORD,UINT,int,HINSTANCE,UINT,LPCTBBUTTON,int,int,int,int,int,UINT ))s_dynprocCreateToolbarEx.pfn)
            (hwnd,ws,wID,nBitmaps,hBMInst,wBMID,lpButton,iNumButtons,dxButton,dyButton,dxBitmap,dyBitmap,uStructSize);

}

#endif


#ifdef VSTUDIO7


WINCOMMCTRLAPI
BOOL
WINAPI _TrackMouseEvent(LPTRACKMOUSEEVENT lpTME)
{
    static DYNPROC s_dynprocTrackMouseEvent =
            { NULL, &g_dynlibCOMCTL32, "_TrackMouseEvent" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocTrackMouseEvent);
    if (hr)
        return FALSE;

    return(*(BOOL (APIENTRY *)(LPTRACKMOUSEEVENT))s_dynprocTrackMouseEvent.pfn)
            (lpTME);
}

#endif

