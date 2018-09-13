// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  CURSOR.CPP
//
//  This file has the implementations of the cursor system objects.
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "cursor.h"

long        fCursorDataInit = FALSE;
HCURSOR     rghcurSystem[CCURSOR_SYSTEM] =
{
    (HCURSOR)IDC_ARROW,
    (HCURSOR)IDC_IBEAM,
    (HCURSOR)IDC_WAIT,
    (HCURSOR)IDC_CROSS,
    (HCURSOR)IDC_UPARROW,
    (HCURSOR)IDC_SIZENWSE,
    (HCURSOR)IDC_SIZENESW,
    (HCURSOR)IDC_SIZEWE,
    (HCURSOR)IDC_SIZENS,
    (HCURSOR)IDC_SIZEALL,
    (HCURSOR)IDC_NO,
    (HCURSOR)IDC_APPSTARTING,
    (HCURSOR)IDC_HELP,
    (HCURSOR)MAKEINTRESOURCE(32631)
};


// --------------------------------------------------------------------------
//
//  MapCursorIndex()
//
// --------------------------------------------------------------------------
long MapCursorIndex(HCURSOR hcur)
{
    long iIndex;
    int iCur;

    iIndex = CURSOR_SYSTEM_UNKNOWN;

    for (iCur = 0; iCur < CCURSOR_SYSTEM; iCur++)
    {
        if (rghcurSystem[iCur] == hcur)
        {
            iIndex = iCur+1;
            break;
        }
    }

    return(iIndex);
}



// --------------------------------------------------------------------------
//
//  CreateCursorObject()
//
// --------------------------------------------------------------------------
HRESULT CreateCursorObject(HWND hwnd, long idObject, REFIID riid, void** ppvCursor)
{
    UNUSED(hwnd);
    UNUSED(idObject);

    if (! InterlockedExchange(&fCursorDataInit, TRUE))
    {
        int icur;

        for (icur = 0; icur < CCURSOR_SYSTEM; icur++)
        {
            rghcurSystem[icur] = LoadCursor(NULL, (LPTSTR)rghcurSystem[icur]);
        }
    }

    return(CreateCursorThing(riid, ppvCursor));
}



// --------------------------------------------------------------------------
//
//  CCursor::Clone()
//
// --------------------------------------------------------------------------
STDMETHODIMP CCursor::Clone(IEnumVARIANT** ppenum)
{
    return(CreateCursorThing(IID_IEnumVARIANT, (void**)ppenum));
}



// --------------------------------------------------------------------------
//
//  CreateCursorThing()
//
// --------------------------------------------------------------------------
HRESULT CreateCursorThing(REFIID riid, void** ppvCursor)
{
    CCursor * pcursor;
    HRESULT hr;

    InitPv(ppvCursor);

    pcursor = new CCursor();
    if (!pcursor)
        return(E_OUTOFMEMORY);

    hr = pcursor->QueryInterface(riid, ppvCursor);
    if (!SUCCEEDED(hr))
        delete pcursor;

    return(hr);
}

                                                                      

// --------------------------------------------------------------------------
//
//  CCursor::get_accName()
//
// --------------------------------------------------------------------------
STDMETHODIMP CCursor::get_accName(VARIANT varChild, BSTR* pszName)
{
    CURSORINFO   ci;

    InitPv(pszName);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    MyGetCursorInfo(&ci);

    return(HrCreateString(STR_CURSORNAMEFIRST+MapCursorIndex(ci.hCursor),
        pszName));
}



// --------------------------------------------------------------------------
//
//  CCursor::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CCursor::get_accRole(VARIANT varChild, VARIANT* pvarRole)
{
    InitPvar(pvarRole);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarRole->vt = VT_I4;
    pvarRole->lVal = ROLE_SYSTEM_CURSOR;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CCursor::get_accState()
//
// --------------------------------------------------------------------------
STDMETHODIMP CCursor::get_accState(VARIANT varChild, VARIANT * pvarState)
{
    CURSORINFO  ci;

    InitPvar(pvarState);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarState->vt = VT_I4;
    pvarState->lVal = 0;

    MyGetCursorInfo(&ci);

    if (!(ci.flags & CURSOR_SHOWING))
        pvarState->lVal |= STATE_SYSTEM_INVISIBLE;

    pvarState->lVal |= STATE_SYSTEM_FLOATING;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CCursor::accLocation()
//
// --------------------------------------------------------------------------
STDMETHODIMP CCursor::accLocation(long* pxLeft, long* pyTop, long* pcxWidth,
    long* pcyHeight, VARIANT varChild)
{
    CURSORINFO  ci;

    InitAccLocation(pxLeft, pyTop, pcxWidth, pcyHeight);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    MyGetCursorInfo(&ci);

    *pxLeft = ci.ptScreenPos.x;
    *pyTop = ci.ptScreenPos.y;
    *pcxWidth = 1;
    *pcyHeight = 1;

    return(S_OK);
}


// --------------------------------------------------------------------------
//
//  CCursor::accHitTest()
//
// --------------------------------------------------------------------------
STDMETHODIMP CCursor::accHitTest(long xLeft, long yTop, VARIANT * pvarChild)
{
    CURSORINFO  ci;

    InitPvar(pvarChild);

    MyGetCursorInfo(&ci);

    if ((xLeft == ci.ptScreenPos.x) && (yTop == ci.ptScreenPos.y))
    {
        pvarChild->vt = VT_I4;
        pvarChild->lVal = 0;
        return(S_OK);
    }

    return(S_FALSE);
}








