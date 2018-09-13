#include "folder.h"
#include "item.h"
#include "utils.h"

#include <mluisupp.h>

///////////////////////////////////////////////////////////////////////////////
// View functions

const struct {
    short int iCol;
    short int ids;
    short int cchCol;
    short int iFmt;
} s_ControlFolder_cols[] = {
    {SI_CONTROL, IDS_COL_CONTROL, 20, LVCFMT_LEFT},
    {SI_STATUS, IDS_COL_STATUS, 20, LVCFMT_LEFT},
    {SI_TOTALSIZE, IDS_COL_TOTALSIZE, 18, LVCFMT_LEFT},
    {SI_CREATION, IDS_COL_CREATION, 18, LVCFMT_LEFT},
    {SI_LASTACCESS, IDS_COL_LASTACCESS, 18, LVCFMT_LEFT},
    {SI_VERSION, IDS_COL_VERSION, 18, LVCFMT_LEFT}
};

HRESULT ControlFolderView_Command(HWND hwnd, UINT uID)
{
    switch (uID)
    {
    case IDM_SORTBYNAME:
    case IDM_SORTBYSTATUS:
    case IDM_SORTBYTOTALSIZE:
    case IDM_SORTBYCREATION:
    case IDM_SORTBYLASTACCESS:
    case IDM_SORTBYVERSION:
        ShellFolderView_ReArrange(hwnd, uID - IDM_SORTBYNAME);
        break;

    default:
        return E_FAIL;
    }

    return NOERROR;
}

HMENU GetMenuFromID(HMENU hmenu, UINT idm)
{
    MENUITEMINFO mii = {sizeof(mii), MIIM_SUBMENU, 0, 0, 0, NULL, NULL, NULL, 0, NULL, 0};
    GetMenuItemInfo(hmenu, idm, FALSE, &mii);
    return mii.hSubMenu;
}

UINT MergeMenuHierarchy(
                    HMENU hmenuDst, 
                    HMENU hmenuSrc, 
                    UINT idcMin, 
                    UINT idcMax)
{
    UINT idcMaxUsed = idcMin;
    int imi = GetMenuItemCount(hmenuSrc);

    while (--imi >= 0) 
    {
        MENUITEMINFO mii = {sizeof(mii), MIIM_ID | MIIM_SUBMENU, 0, 0, 0, NULL, NULL, NULL, 0, NULL, 0};

        if (GetMenuItemInfo(hmenuSrc, imi, TRUE, &mii)) 
        {
            UINT idcT = Shell_MergeMenus(
                                  GetMenuFromID(hmenuDst, mii.wID),
                                  mii.hSubMenu, 
                                  0, idcMin, idcMax, 
                                  MM_ADDSEPARATOR|MM_SUBMENUSHAVEIDS);
            idcMaxUsed = max(idcMaxUsed, idcT);
        }
    }
    return idcMaxUsed;
}

HRESULT ControlFolderView_MergeMenu(LPQCMINFO pqcm)
{
    HMENU hmenu = LoadMenu(MLGetHinst(), MAKEINTRESOURCE(IDR_CONTROLFOLDER));
    Assert(hmenu != NULL);
    if (hmenu)
    {
        MENUITEMINFO mii;
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_ID;
        mii.wID = SFVIDM_MENU_ARRANGE;
        SetMenuItemInfo(hmenu, 0, TRUE, &mii);
        MergeMenuHierarchy(pqcm->hmenu, hmenu, pqcm->idCmdFirst, pqcm->idCmdLast);
        DestroyMenu(hmenu);
    }
    return NOERROR;
}

HRESULT ControlFolderView_InitMenuPopup(
                                   HWND hwnd, 
                                   UINT idCmdFirst, 
                                   int nIndex, 
                                   HMENU hmenu)
{
    return NOERROR;
}

HRESULT ControlFolderView_OnGetDetailsOf(
                                    HWND hwnd, 
                                    UINT iColumn, 
                                    PDETAILSINFO pdi)
{
    BOOL bResult = TRUE;
    LPCONTROLPIDL pcpidl = (LPCONTROLPIDL)pdi->pidl;

    if (iColumn >= NUM_COLUMNS)
        return E_NOTIMPL;

    pdi->str.uType = STRRET_CSTR;
    pdi->str.cStr[0] = '\0';

    // if NULL, asking for column info
    if (pcpidl == NULL)
    {
        MLLoadString(
              s_ControlFolder_cols[iColumn].ids, 
              pdi->str.cStr, 
              ARRAYSIZE(pdi->str.cStr));

        pdi->fmt = s_ControlFolder_cols[iColumn].iFmt;
        pdi->cxChar = s_ControlFolder_cols[iColumn].cchCol;

        return NOERROR;
    }
           
    switch (iColumn)
    {
    case SI_CONTROL:
        lstrcpy(pdi->str.cStr, GetStringInfo(pcpidl, SI_CONTROL));
        break;

    case SI_VERSION:
        lstrcpy(pdi->str.cStr, GetStringInfo(pcpidl, SI_VERSION));
        break;

    case SI_CREATION:
        lstrcpy(pdi->str.cStr, GetStringInfo(pcpidl, SI_CREATION));
        break;

    case SI_LASTACCESS:
        lstrcpy(pdi->str.cStr, GetStringInfo(pcpidl, SI_LASTACCESS));
        break;

    case SI_TOTALSIZE:
        GetSizeSaved(pcpidl, pdi->str.cStr);
        break;

    case SI_STATUS:
        GetStatus(pcpidl, pdi->str.cStr, sizeof(pdi->str.cStr));
        break;

    default:
        bResult = FALSE;
    }

    return (bResult ? NOERROR : E_FAIL);
}

HRESULT ControlFolderView_OnColumnClick(HWND hwnd, UINT iColumn)
{
    ShellFolderView_ReArrange(hwnd, iColumn);
    return NOERROR;
}

HRESULT ControlFolderView_DidDragDrop(HWND hwnd, IDataObject *pdo, DWORD dwEffect)
{
    HRESULT hr = E_FAIL;

    if (dwEffect & DROPEFFECT_MOVE)
    {
        CControlItem *pCItem;
        if (SUCCEEDED(pdo->QueryInterface(CLSID_ControlFolder, (void **)&pCItem)))
        {
            hr = pCItem->Remove(hwnd);
            pCItem->Release();
        }
    }

    return hr;
}

HRESULT CALLBACK ControlFolderView_ViewCallback(
                                     IShellView *psvOuter,
                                     IShellFolder *psf,
                                     HWND hwnd,
                                     UINT uMsg,
                                     WPARAM wParam,
                                     LPARAM lParam)
{
    HRESULT hres = NOERROR;

    switch (uMsg)
    {
    case DVM_GETHELPTEXT:
        {
            UINT id = LOWORD(wParam);
            UINT cchBuf = HIWORD(wParam);
            LPTSTR pszBuf = (LPTSTR)lParam;
            MLLoadString(id + IDS_HELP_SORTBYNAME ,pszBuf, cchBuf);
        }
        break;

    case DVM_DIDDRAGDROP:
        hres = ControlFolderView_DidDragDrop(
                                        hwnd, 
                                        (IDataObject*)lParam, 
                                        (DWORD)wParam);
        break;

    case DVM_INITMENUPOPUP:
        hres = ControlFolderView_InitMenuPopup(
                                           hwnd, 
                                           LOWORD(wParam), 
                                           HIWORD(wParam), 
                                           (HMENU)lParam);
        break;

    case DVM_INVOKECOMMAND:
        ControlFolderView_Command(hwnd, (UINT)wParam);
        break;

    case DVM_COLUMNCLICK:
        hres = ControlFolderView_OnColumnClick(hwnd, (UINT)wParam);
        break;

    case DVM_GETDETAILSOF:
        hres = ControlFolderView_OnGetDetailsOf(hwnd, (UINT)wParam, (PDETAILSINFO)lParam);
        break;

    case DVM_MERGEMENU:
        hres = ControlFolderView_MergeMenu((LPQCMINFO)lParam);
        break;

    case DVM_DEFVIEWMODE:
        *(FOLDERVIEWMODE *)lParam = FVM_DETAILS;
        break;

    default:
        hres = E_FAIL;
    }

    return hres;
}

HRESULT ControlFolderView_CreateInstance(
                                    CControlFolder *pCFolder, 
                                    LPCITEMIDLIST pidl, 
                                    void **ppvOut)
{
    CSFV csfv;

    csfv.cbSize = sizeof(csfv);
    csfv.pshf = (IShellFolder*)pCFolder;
    csfv.psvOuter = NULL;
    csfv.pidl = pidl;
    csfv.lEvents = SHCNE_DELETE | SHCNE_UPDATEITEM; // SHCNE_DISKEVENTS | SHCNE_ASSOCCHANGED | SHCNE_GLOBALEVENTS;
    csfv.pfnCallback = ControlFolderView_ViewCallback;
    csfv.fvm = (FOLDERVIEWMODE)0;         // Have defview restore the folder view mode

    return SHCreateShellFolderViewEx(&csfv, (IShellView**)ppvOut); // &this->psv);
}
