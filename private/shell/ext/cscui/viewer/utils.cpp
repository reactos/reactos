//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       utils.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "utils.h"
#include "strret.h"
#include "resource.h"

void 
CAutoWaitCursor::Reset(
    void
    )
{ 
    ShowCursor(FALSE); 
    if (NULL != m_hCursor) 
        SetCursor(m_hCursor); 
    m_hCursor = NULL;
}


bool
DblNulTermListIter::Next(
    LPCTSTR *ppszItem
    ) throw()
{
    if (m_pszCurrent && *m_pszCurrent)
    {
        *ppszItem = m_pszCurrent;
        m_pszCurrent += lstrlen(m_pszCurrent) + 1;
        return true;
    }
    return false;
}


//
// Center a popup window in it's parent.
// If hwndParent is NULL, the window's parent is used.
// If hwndParent is not NULL, hwnd is centered in it.
// If hwndParent is NULL and hwnd doesn't have a parent, it is centered
// on the desktop.
//
void
CenterPopupWindow(
    HWND hwnd, 
    HWND hwndParent
    )
{
    RECT rcScreen;

    if (NULL != hwnd)
    {
        rcScreen.left   = rcScreen.top = 0;
        rcScreen.right  = GetSystemMetrics(SM_CXSCREEN);
        rcScreen.bottom = GetSystemMetrics(SM_CYSCREEN);

        if (NULL == hwndParent)
        {
            hwndParent = GetParent(hwnd);
            if (NULL == hwndParent)
                hwndParent = GetDesktopWindow();
        }

        RECT rcWnd;
        RECT rcParent;

        GetWindowRect(hwnd, &rcWnd);
        GetWindowRect(hwndParent, &rcParent);

        INT cxWnd    = rcWnd.right  - rcWnd.left;
        INT cyWnd    = rcWnd.bottom - rcWnd.top;
        INT cxParent = rcParent.right  - rcParent.left;
        INT cyParent = rcParent.bottom - rcParent.top;
        POINT ptParentCtr;

        ptParentCtr.x = rcParent.left + (cxParent / 2);
        ptParentCtr.y = rcParent.top  + (cyParent / 2);

        if ((ptParentCtr.x + (cxWnd / 2)) > rcScreen.right)
        {
            //
            // Window would run off the right edge of the screen.
            //
            rcWnd.left = rcScreen.right - cxWnd;
        }
        else if ((ptParentCtr.x - (cxWnd / 2)) < rcScreen.left)
        {
            //
            // Window would run off the left edge of the screen.
            //
            rcWnd.left = rcScreen.left;
        }
        else
        {
            rcWnd.left = ptParentCtr.x - (cxWnd / 2);
        }

        if ((ptParentCtr.y + (cyWnd / 2)) > rcScreen.bottom)
        {
            //
            // Window would run off the bottom edge of the screen.
            //
            rcWnd.top = rcScreen.bottom - cyWnd;
        }
        else if ((ptParentCtr.y - (cyWnd / 2)) < rcScreen.top)
        {
            //
            // Window would run off the top edge of the screen.
            //
            rcWnd.top = rcScreen.top;
        }
        else
        {
            rcWnd.top = ptParentCtr.y - (cyWnd / 2);
        }

        MoveWindow(hwnd, rcWnd.left, rcWnd.top, cxWnd, cyWnd, TRUE);
    }
}

//
// This is for assisting the context help functions.
// Determine if the control has it's help text in windows.hlp or
// in our cscui.hlp.
//
bool UseWindowsHelp(int idCtl)
{
    bool bUseWindowsHelp = false;    
    switch(idCtl)
    {
        case IDOK:
        case IDCANCEL:
        case IDC_STATIC:
            bUseWindowsHelp = true;
            break;

        default:
            break;
    }
    return bUseWindowsHelp;
}




bool
GetShellItemDisplayName(
    LPITEMIDLIST pidl,
    CString *pstrName
    )
{
    DBGASSERT((NULL != pidl));
    DBGASSERT((NULL != pstrName));

    bool bResult = false;
    com_autoptr<IShellFolder> ptrsf;
    HRESULT hr = SHGetDesktopFolder(ptrsf.getaddr());
    if (SUCCEEDED(hr))
    {
        StrRet s(pidl); // cleans up on dtor.
        hr = ptrsf->GetDisplayNameOf(pidl, SHGDN_NORMAL, &s);
        if (SUCCEEDED(hr))
        {   
            s.GetString(pstrName->GetBuffer(MAX_PATH), MAX_PATH);
            bResult = true;
        }
        else
        {
            DBGERROR((TEXT("GetDisplayNameOf failed with error 0x%08X"), hr));
        }
    }
    return bResult;
}


//
// BUGBUG:  This is temporary.  Remove before flight.
//
void 
NotYetImplemented(
    HWND hwndParent
    )
{
    MessageBox(hwndParent, TEXT("Feature not yet implemented."), 
                           TEXT("Offline Folders"),
                           MB_ICONINFORMATION);
}



