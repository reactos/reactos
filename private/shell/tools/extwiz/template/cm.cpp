// $$ClassType$$CM.cpp : Implementation of C$$ClassType$$CM
#include "stdafx.h"
#include "$$root$$.h"
#include "$$ClassType$$CM.h"

#define ResultFromShort(i)      MAKE_HRESULT(SEVERITY_SUCCESS, 0, (USHORT)(i))



#define IDI_EXECUTEITEM     0

/////////////////////////////////////////////////////////////////////////////
// CContents

HRESULT C$$ClassType$$CM::Initialize ( LPCITEMIDLIST pidlFolder,
                     LPDATAOBJECT lpdobj, 
                     HKEY hkeyProgID)
{
    // TODO: Extract items from the dataobject and Folder.

    return NOERROR;
}

HRESULT C$$ClassType$$CM::QueryContextMenu(HMENU hmenu,
                            UINT indexMenu,
                            UINT idCmdFirst,
                            UINT idCmdLast,
                            UINT uFlags)
{
    MENUITEMINFO mfi;
    UINT idCmd = idCmdFirst;

    if (idCmdFirst + IDI_EXECUTEITEM < idCmdLast)
    {
        // TODO: Chance this to match your item
        mfi.cbSize = sizeof(MENUITEMINFO);
        mfi.fMask = MIIM_ID|MIIM_TYPE;
        mfi.wID = idCmdFirst + IDI_EXECUTEITEM;
        mfi.fType = MFT_STRING;
        mfi.dwTypeData = (LPTSTR)TEXT("Example Item");

        idCmd++;
    }

    // TODO: Add more items here

    if (!InsertMenuItem(hmenu, indexMenu, TRUE, &mfi))
        idCmd--; // We weren't able to insert an item

    return ResultFromShort(idCmd - idCmdFirst);
}

HRESULT C$$ClassType$$CM::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    if (LOWORD(lpici->lpVerb) == IDI_EXECUTEITEM)
    {
        MessageBox(lpici->hwnd, "You Selected the example item", "C$$ClassType$$CM", MB_OK);
    }

    return NOERROR;
}

HRESULT C$$ClassType$$CM::GetCommandString(UINT  idCmd,
                            UINT        uType,
                            UINT      * pwReserved,
                            LPSTR       pszName,
                            UINT        cchMax)
{
    return E_NOTIMPL;
}

HRESULT C$$ClassType$$CM::HandleMenuMsg2(UINT uMsg,
                             WPARAM wParam,
                             LPARAM lParam,
                             LRESULT* plResult)
{
    LRESULT lres = 0;

    switch (uMsg)
    {
    case WM_INITMENUPOPUP:
        break;
    case WM_MEASUREITEM:
        break;
    case WM_DRAWITEM:
        break;
    case WM_MENUCHAR:
        lres = 0;   // Ignore the character, and issue a short beep.
        break;
    }

    if (plResult)
        *plResult = lres;

    return NOERROR;
}
