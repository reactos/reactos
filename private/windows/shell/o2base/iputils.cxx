//+---------------------------------------------------------------------
//
//  File:       iputils.hxx
//
//  Contents:   Helper functions for in-place activation
//
//----------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#define MAXLABELLEN 32

//+---------------------------------------------------------------
//
//  Function:   InsertServerMenus
//
//  Synopsis:   Inserts the objects menus into a shared menu after
//              the top-level application has inserted its menus
//
//  Arguments:  [hmenuShared] -- the shared menu to recieve the objects menus
//              [hmenuObject] -- all of the objects menus
//              [lpmgw] -- menu group widths indicating where the menus
//                          should be inserted
//
//  Returns:    Success if the menus were merged successfully
//
//  Notes:      The function does most of the shared menu work
//              by the object between the IOleInPlaceFrame::InsertMenus and
//              IOleInPlaceFrame::SetMenu method calls.
//              c.f. RemoveServerMenus
//
//----------------------------------------------------------------

HRESULT
InsertServerMenus(HMENU hmenuShared,
        HMENU hmenuObject,
        LPOLEMENUGROUPWIDTHS lpmgw)
{
    int i, j;
    HMENU hmenuXfer;
    WCHAR szLabel[MAXLABELLEN];
    UINT iServer = 0;
    UINT iShared = 0;

    // for each of the Edit, Object, and Help menu groups
    for (j = 1; j <= 5; j += 2)
    {
        // advance over container menus
        iShared += (UINT)lpmgw->width[j-1];

        // pull out the popup menus from servers menu
        for (i = 0; i < lpmgw->width[j]; i++)
        {
            GetMenuString(hmenuObject,
                        iServer,
                        szLabel,
                        MAXLABELLEN,
                        MF_BYPOSITION);
            hmenuXfer = GetSubMenu(hmenuObject, iServer++);
            if (!InsertMenu(hmenuShared,
                        iShared++,
                        MF_BYPOSITION|MF_POPUP,
                        (UINT_PTR)hmenuXfer,
                        szLabel))
            {
                return HRESULT_FROM_WIN32(GetLastError());
            }
        }
    }
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Function:   RemoveServerMenus
//
//  Synopsis:   Removes the objects menus from a shared menu
//
//  Arguments:  [hmenuShared] -- the menu contain both the application's
//                              and the object's menus
//              [lpmgw] -- menu group widths indicating which menus should
//                          be removed
//
//  Notes:      c.f. InsertServerMenus
//
//----------------------------------------------------------------

void
RemoveServerMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpmgw)
{
    int i, j;
    UINT iShared = 0;

    // for each of the Edit, Object, and Help menu groups
    for (j = 1; j <= 5; j += 2)
    {
        // advance over container menus
        iShared += (UINT)lpmgw->width[j-1];

        // pull out the popup menus from shared menu
        for (i = 0; i < lpmgw->width[j]; i++)
        {
            RemoveMenu(hmenuShared, iShared, MF_BYPOSITION);
        }
    }
}

