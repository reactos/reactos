//+---------------------------------------------------------------------
//
//  File:       wutils.cxx
//
//  Contents:   Windows helper functions
//
//----------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

//+---------------------------------------------------------------
//
//  Function:   LoadResourceData
//
//  Synopsis:   Loads a resource RCDATA block into a buffer
//
//  Arguments:  [hinst] -- instance of the module with the resource
//              [lpstrId] -- the identifier of the RCDATA resource
//              [lpvBuf] -- the buffer where the resource is to be loaded
//              [cbBuf] -- the number of bytes of resource data to load
//
//  Returns:    lpvBuf if the resource was successfully loaded, NULL otherwise
//
//  Notes:      This function combines Windows' FindResource, LoadResource,
//              LockResource, and a memory copy.
//
//----------------------------------------------------------------

LPVOID
LoadResourceData(HINSTANCE hinst,
        LPCWSTR lpstrId,
        LPVOID lpvBuf,
        int cbBuf)
{
    LPVOID lpvRet = NULL;
    HRSRC hrsrc = FindResource(hinst, lpstrId, RT_RCDATA);
    if (hrsrc != NULL)
    {
        HGLOBAL hgbl = LoadResource(hinst, hrsrc);
        if (hgbl != NULL)
        {
            LPVOID lpvSrc = LockResource(hgbl);
            if (lpvSrc != NULL)
            {
                lpvRet = _fmemcpy(lpvBuf, lpvSrc, cbBuf);
                UnlockResource(hgbl);
            }
            FreeResource(hgbl);
        }
    }
    return lpvRet;
}

//+---------------------------------------------------------------
//
//  Function:   GetChildWindowRect
//
//  Synopsis:   Gets the rectangle of the child window in
//              its parent window coordinates
//
//  Arguments:  [hwndChild] -- the child window
//              [lprect]  -- the rectangle to fill with childs coordinates
//
//  Notes:      This function gets the screen coordinates of the child
//              then maps them into the client coordinates of its parent.
//
//----------------------------------------------------------------

void
GetChildWindowRect(HWND hwndChild, LPRECT lprect)
{
    HWND hwndParent;
    POINT ptUpperLeft;
    POINT ptLowerRight;

    // get the screen coordinates of the child window
    GetWindowRect(hwndChild, lprect);

    // get the parent window of the child
    if ((hwndParent = GetParent(hwndChild)) != NULL)
    {
        // map the screen coordinates to client coordinates
        ptUpperLeft.x   = lprect->left;
        ptUpperLeft.y   = lprect->top;
        ptLowerRight.x  = lprect->right;
        ptLowerRight.y  = lprect->bottom;

        ScreenToClient(hwndParent, &ptUpperLeft);
        ScreenToClient(hwndParent, &ptLowerRight);

        SetRect(lprect,
                ptUpperLeft.x,
                ptUpperLeft.y,
                ptLowerRight.x,
                ptLowerRight.y);
    }
}

//+---------------------------------------------------------------
//
//  Function:   SizeClientRect
//
//  Synopsis:   Resizes the window so its client size is a specified
//              area.  Can also move the window so its client region
//              covers a specified rectangle.
//
//  Arguments:  [hwnd] -- the window to resize/move
//              [rc] -- rectangle indicating size and possibly position of
//                      client area
//              [fMove] -- flag indicating resize-only or move
//
//  Notes:      This function uses SetWindowPos which does not handle
//              accurately the case where the menu wraps to more than one line
//              as a result of the resize.
//
//----------------------------------------------------------------

void
SizeClientRect(HWND hwnd, RECT& rc, BOOL fMove)
{
    AdjustWindowRect(&rc,
            GetWindowLong(hwnd, GWL_STYLE),
            GetMenu(hwnd) != NULL);
    SetWindowPos(hwnd,
            NULL,
            rc.left,
            rc.top,
            rc.right-rc.left,
            rc.bottom-rc.top,
            SWP_NOZORDER|SWP_NOACTIVATE | (fMove ? 0 : SWP_NOMOVE));
}

