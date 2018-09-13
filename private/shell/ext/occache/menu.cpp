#include "folder.h"
#include "utils.h"

#include <mluisupp.h>

///////////////////////////////////////////////////////////////////////////////
// IContextMenu methods

HRESULT CControlFolder::QueryContextMenu(
                                     HMENU hmenu, 
                                     UINT indexMenu, 
                                     UINT idCmdFirst,
                                     UINT idCmdLast,
                                     UINT uFlags)
{
    USHORT cItems = 0;

    DebugMsg(DM_TRACE, TEXT("cf - cm - QueryContextMenu() called."));
    if (uFlags == CMF_NORMAL)
    {
        HMENU hCtrlMenu = LoadMenu(MLGetHinst(), MAKEINTRESOURCE(IDR_CONTROLFOLDER));
        if (hmenu)
        {
            MENUITEMINFO mii;
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_ID;
            mii.wID = SFVIDM_MENU_ARRANGE;
            SetMenuItemInfo(hCtrlMenu, 0, TRUE, &mii);
            cItems = (USHORT)MergeMenuHierarchy(hmenu, hCtrlMenu, idCmdFirst, idCmdLast);
            DestroyMenu(hCtrlMenu);
        }
    }
    SetMenuDefaultItem(hmenu, indexMenu, MF_BYPOSITION);

    return ResultFromShort(cItems);    // number of menu items    
}

HRESULT CControlFolder::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    // We don't deal with the VERBONLY case
    DebugMsg(DM_TRACE, TEXT("cf - cm - InvokeCommand() called."));
    Assert((DWORD_PTR)(pici->lpVerb) <= 0xFFFF);
    
    int idCmd;
    if ((DWORD_PTR)(pici->lpVerb) > 0xFFFF)
        idCmd = -1;
    else
        idCmd = LOWORD(pici->lpVerb);
    
    return ControlFolderView_Command(pici->hwnd, idCmd);
}

HRESULT CControlFolder::GetCommandString(
                                     UINT_PTR idCmd, 
                                     UINT uFlags, 
                                     UINT *pwReserved,
                                     LPTSTR pszName, 
                                     UINT cchMax)
{
    HRESULT hres = E_FAIL;

    DebugMsg(DM_TRACE, TEXT("cf - cm - GetCommandString() called."));

    if (uFlags == GCS_HELPTEXT)
    {
        MLLoadString((UINT) (idCmd + IDS_HELP_SORTBYNAME), pszName, cchMax);
        hres = S_OK;
    }

    return hres;
}
