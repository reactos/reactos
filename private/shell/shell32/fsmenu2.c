//--------------------------------------------------------------------------
// 
//---------------------------------------------------------------------------
#include "shellprv.h"
#pragma  hdrstop

//----------------------------------------------------------------------------
__inline static BOOL LAlloc(UINT cb, PVOID *ppv)
{
    *ppv = (PVOID*)LocalAlloc(LPTR, cb);
    return *ppv ? TRUE : FALSE;
}

//----------------------------------------------------------------------------
__inline static BOOL LFree(PVOID pv)
{
    return LocalFree(pv) ? FALSE : TRUE;
}

//---------------------------------------------------------------------------
typedef BOOL (*PFN_FM_ENUM_CALLBACK)(HMENU hmenu, int i, PVOID pv);

//----------------------------------------------------------------------------
typedef enum
{
    FMI_NONE                = 0x0000,
    FMI_LARGE_ICONS         = 0x0001,
    FMI_EXPAND              = 0x0002,
    FMI_IMAGES_ARE_INVALID  = 0x0004,
    FMI_ITEMS_ARE_INVALID   = 0x0008,
    FMI_CHILD               = 0x0010,
} FMI_FLAGS;

//----------------------------------------------------------------------------
typedef LPITEMIDLIST PIDL;
typedef struct
{
    DWORD dwMagic;
    FMI_FLAGS flags;
    union
    {
        UINT idCurrent;
        PUINT pidCurrent;
    } id;
    HIMAGELIST himlLarge;
    HIMAGELIST himlSmall;
    PIDL pidlFolder;
    HDPA hdpaFree;
    UINT cyExtra;
    UINT nPos;
} FM_INFO;
typedef FM_INFO *PFM_INFO;

//---------------------------------------------------------------------------
typedef struct
{
    PIDL pidl;
    UINT iImage;
    UINT iImageSel;
    HMENU hmenu;
} FM_ITEM_DATA;
typedef FM_ITEM_DATA *PFM_ITEM_DATA;

//---------------------------------------------------------------------------
BOOL FM_EnumMenu(HMENU hmenu, PFN_FM_ENUM_CALLBACK pfn, PVOID pv);
// BOOL DestroyEverythingCallback(HMENU hmenu, int i, PVOID pv);
BOOL FM_GetItemData(HMENU hmenu, UINT iItem, PFM_ITEM_DATA *ppfmid);

//---------------------------------------------------------------------------
void Menu_SetData(HMENU hmenu, LPVOID pv)
{
    MENUINFO mi;

    mi.cbSize = SIZEOF(mi);
    mi.fMask = MIM_MENUDATA|MIM_STYLE;
    mi.dwStyle = MNS_NOCHECK;
    mi.dwMenuData = (DWORD)pv;
#ifdef LIMIT_HEIGHT    
    mi.fMask |= MIM_MAXHEIGHT;
    mi.cyMax = (GetSystemMetrics(SM_CYSCREEN)*2)/3;
#endif    
    SetMenuInfo(hmenu, &mi);
}

//---------------------------------------------------------------------------
HBRUSH Menu_GetBrush(HMENU hmenu)
{
    MENUINFO mi;

    mi.cbSize = SIZEOF(mi);
    mi.fMask = MIM_BACKGROUND;
    GetMenuInfo(hmenu, &mi);
    return mi.hbrBack;
}

//---------------------------------------------------------------------------
#define FM_MAGIC mmioFOURCC('D','N','R','C')

//---------------------------------------------------------------------------
BOOL FM_GetInfo(HMENU hmenu, PFM_INFO *ppfmi)
{
    BOOL fRet = FALSE;
    MENUINFO mi;
    ASSERT(hmenu && ppfmi);
        
    mi.cbSize = SIZEOF(mi);
    mi.fMask = MIM_MENUDATA;
    if (GetMenuInfo(hmenu, &mi))
    {
        if (mi.dwMenuData && !IsBadReadPtr((PVOID)mi.dwMenuData, SIZEOF(FM_INFO)))
        {
            *ppfmi = (PVOID)mi.dwMenuData;
            if ((*ppfmi)->dwMagic == FM_MAGIC)
                fRet = TRUE;
        }
    }
    
    return fRet;;
}

//---------------------------------------------------------------------------
#define ID_MAX  0xffff

//---------------------------------------------------------------------------
BOOL FM_CreateFromMenu(HMENU hmenu, HIMAGELIST himlLarge, HIMAGELIST himlSmall, UINT cyExtra)
{
    BOOL fRet = FALSE;
    PFM_INFO pfmi;

    if (!FM_GetInfo(hmenu, &pfmi))
    {
        if (LAlloc(SIZEOF(*pfmi), &pfmi))
        {
            pfmi->hdpaFree = DPA_Create(0);
            if (pfmi->hdpaFree)
            {
                pfmi->dwMagic = FM_MAGIC;
                pfmi->himlLarge = himlLarge;
                pfmi->himlSmall = himlSmall;
                pfmi->cyExtra = cyExtra;
                Menu_SetData(hmenu, pfmi);
                fRet = TRUE;
            }
        }
    }
    
    return fRet;
}

//---------------------------------------------------------------------------
BOOL FM_Create(HIMAGELIST himlLarge, HIMAGELIST himlSmall, HMENU *phmenu, UINT cyExtra)
{
    BOOL fRet = FALSE;

    *phmenu = CreatePopupMenu();
    if (*phmenu)
        fRet = FM_CreateFromMenu(*phmenu, himlLarge, himlSmall, cyExtra);
    
    return fRet;
}

//---------------------------------------------------------------------------
BOOL FMI_Create(HMENU hmenu, PIDL pidlItem, UINT iImage, PFM_ITEM_DATA *ppfmid)
{
    BOOL fRet = FALSE;
    ASSERT(ppfmid);
    
    if (LAlloc(SIZEOF(FM_ITEM_DATA), ppfmid))
    {
        // NB We're not cloning this so the caller better do it. Typically we're getting
        // these from and enum so that's fine.
        (*ppfmid)->pidl = pidlItem; 
        (*ppfmid)->iImage = iImage;
        (*ppfmid)->hmenu = hmenu;
        fRet = TRUE;
    }
    
    return fRet;
}

//---------------------------------------------------------------------------
BOOL FMI_Destroy(PFM_ITEM_DATA pfmid)
{
    if (pfmid->pidl)
        ILFree(pfmid->pidl);
    LFree(pfmid);
    return TRUE;
}

//---------------------------------------------------------------------------
BOOL Menu_InsertItem(HMENU hmenu, UINT iPos, UINT id, LPCTSTR pszName, UINT fState, 
    HMENU hmenuSub, DWORD dwItemData)
{
    BOOL fRet = FALSE;
    MENUITEMINFO mii;
    
    mii.cbSize = SIZEOF(mii);
    mii.fMask = MIIM_ID|MIIM_STRING|MIIM_STATE;
    mii.wID = id;
    mii.dwTypeData = (LPSTR)pszName;
    mii.fState = fState;
    if (dwItemData)
    {
        mii.fMask |= MIIM_BITMAP|MIIM_DATA;
        mii.hbmpItem = (HBITMAP)-1; // Callback...
        mii.dwItemData = dwItemData;
    }
    if (hmenuSub)
    {
        mii.fMask |= MIIM_SUBMENU;
        mii.hSubMenu = hmenuSub;
    }
    if (InsertMenuItem(hmenu, iPos, TRUE, &mii))
    {
        fRet = TRUE;
    }
    else
    {
        ASSERT(FALSE);
    }    

    return fRet;
}

//---------------------------------------------------------------------------
BOOL Menu_InsertFMItem(HMENU hmenu, UINT iPos, UINT id, LPCTSTR pszName, PIDL pidl, UINT iImage, UINT fState, HMENU hmenuSub)
{
    BOOL fRet = FALSE;
    PFM_ITEM_DATA pfmid;
    ASSERT((id > 0) && (id < 0xffff));
    
    if (FMI_Create(hmenu, pidl, iImage, &pfmid))
        fRet = Menu_InsertItem(hmenu, iPos, id, pszName, fState, hmenuSub, (DWORD)pfmid);

    return fRet;
}

//---------------------------------------------------------------------------
BOOL FM_GetNextId(HMENU hmenu, PUINT pid)
{
    BOOL fRet = FALSE;
    PFM_INFO pfmi;

    if (FM_GetInfo(hmenu, &pfmi))
    {
        // Check the free list first.
        UINT cItems = DPA_GetPtrCount(pfmi->hdpaFree);
        ASSERT(pfmi->hdpaFree);
        if (cItems)
        {
            *pid = (UINT)DPA_DeletePtr(pfmi->hdpaFree, cItems-1);
            fRet = TRUE;
        }
        else
        {
            // Use a brand new id...
            // Is it a ptr or an id?
            if (pfmi->flags & FMI_CHILD)
            {
                // It's a ptr.
                ASSERT(pfmi->id.idCurrent > ID_MAX)
                *pid = (*pfmi->id.pidCurrent)++;
            }
            else
            {
                // It's an id.
                ASSERT(pfmi->id.idCurrent <= ID_MAX)
                *pid = pfmi->id.idCurrent++;
            }
            fRet = TRUE;
        }
    }
    
    ASSERT((0 < *pid) && (*pid < 0xffff));

    return fRet;
}

//---------------------------------------------------------------------------
BOOL FM_FreeId(HMENU hmenu, UINT id)
{
    BOOL fRet = FALSE;
    PFM_INFO pfmi;

    ASSERT((0 < id) && (id < 0xffff));
    if (FM_GetInfo(hmenu, &pfmi))
    {
        // Stick the id on the free list.
        ASSERT(pfmi->hdpaFree);
        DPA_AppendPtr(pfmi->hdpaFree, (PVOID)id);
        fRet = TRUE;
    }

    return fRet;
}

//---------------------------------------------------------------------------
// The real ILIsEqual faults if either pidl is null.
BOOL _ILIsEqual(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    BOOL fRet = FALSE;
    
    if (!pidl1 && !pidl2)
        fRet = TRUE;
    else if (pidl1 && pidl2)
        fRet = ILIsEqual(pidl1, pidl2);

    return fRet;
}


//---------------------------------------------------------------------------
BOOL IsProgramsPidl(PIDL pidl)
{
    BOOL fRet = FALSE;
    PIDL pidlPrograms = SHCloneSpecialIDList(NULL, CSIDL_PROGRAMS, TRUE);
    ASSERT(pidl);
    if (_ILIsEqual(pidl, pidlPrograms))
    {
        fRet = TRUE;
    }

    return fRet;
}

//---------------------------------------------------------------------------
BOOL FM_CreateFromExisting(HMENU hmenuOld, PIDL pidlNew, FMI_FLAGS flagsNew, HMENU *phmenuNew)
{
    BOOL fRet = FALSE;
    PFM_INFO pfmiNew;

    *phmenuNew = CreatePopupMenu();
    if (*phmenuNew)
    {
        if (LAlloc(SIZEOF(*pfmiNew), &pfmiNew))
        {
            PFM_INFO pfmiOld;
            if (FM_GetInfo(hmenuOld, &pfmiOld))
            {
                // Copy the basics.
                *pfmiNew = *pfmiOld;
                // New stuff.
                pfmiNew->pidlFolder = pidlNew;
                pfmiNew->flags = flagsNew;
                pfmiNew->nPos = 0;
                // Special case the id stuff...
                // Only the top-level menu keeps the idCurrent. All the submenu's
                // keep a pointer to that one so we can keep all the id's in
                // sync easily.
                if (!(pfmiOld->flags & FMI_CHILD) && (pfmiNew->flags & FMI_CHILD))
                {
                    ASSERT(pfmiOld->id.idCurrent <= ID_MAX);
                    pfmiNew->id.pidCurrent = &pfmiOld->id.idCurrent;
                }
                Menu_SetData(*phmenuNew, pfmiNew);
                fRet = TRUE;
            }
        }
    }
    
    return fRet;
}

//---------------------------------------------------------------------------
BOOL FileList_AddItemToMenu(HMENU hmenu, PIDL pidlFolder, PFILELIST_ITEM pfli, int iPos, FMIUP_FLAGS flags)
{
    BOOL fRet = FALSE;
                
    if ((pfli->dwAttribs & SFGAO_FOLDER) && !(flags & FMIUP_NO_SUBFOLDERS))
    {
        PIDL pidlChild = ILCombine(pidlFolder, pfli->pidl);
        if (pidlChild)
        {
            if (!(flags & FMIUP_NO_PROGRAMS_SUBFOLDER) || !IsProgramsPidl(pidlChild))
            {
                HMENU hmenuNew;
                if (FM_CreateFromExisting(hmenu, pidlChild, FMI_EXPAND|FMI_CHILD, &hmenuNew))
                {
                    UINT id;
                    if (FM_GetNextId(hmenu, &id))
                    {
                        Menu_InsertFMItem(hmenu, iPos, id, pfli->pszName, pfli->pidl, (UINT)-1, MFS_ENABLED, hmenuNew);
                        fRet = TRUE;
                    }
                }
            }
        }
    }
    else
    {
        UINT id;
        if (FM_GetNextId(hmenu, &id))
        {
            Menu_InsertFMItem(hmenu, iPos, id, pfli->pszName, pfli->pidl, (UINT)-1, MFS_ENABLED, NULL);
            fRet = TRUE;
        }
    }

    return fRet;
}

//---------------------------------------------------------------------------
BOOL InsertEmptyItem(HMENU hmenu)
{
    TCHAR szName[MAX_PATH];
    LoadString(HINST_THISDLL, IDS_NONE, szName, ARRAYSIZE(szName));
    return Menu_InsertItem(hmenu, 0, (UINT)-1, szName, MFS_DISABLED, NULL, 0);
}

//---------------------------------------------------------------------------
BOOL FileList_AddToMenu(PIDL pidlFolder, HMENU hmenu, HDPA hdpaFLI, FMIUP_FLAGS flags, UINT nPos, PUINT pcItems)
{
    BOOL fRet = FALSE;
    int cItems = DPA_GetPtrCount(hdpaFLI);
    int i;

    *pcItems = 0;
    if (cItems)
    {
        for (i = 0; i < cItems; i++)
        {
            PFILELIST_ITEM pfli = DPA_GetPtr(hdpaFLI, i);
            ASSERT(pfli);
            if (FileList_AddItemToMenu(hmenu, pidlFolder, pfli, i+nPos, flags))
                (*pcItems)++;
        }
        fRet = TRUE;
    }
    else if (!(flags & FMIUP_NO_EMPTY_ITEM) && !nPos)
    {
        if (InsertEmptyItem(hmenu))
        {
            (*pcItems)++;
            fRet = TRUE;
        }
    }

    return fRet;
}

//----------------------------------------------------------------------------
void MangleAmpersands(LPTSTR pszSrc)
{
    TCHAR szDst[MAX_PATH];
    LPTSTR pszDst = szDst;
    LPTSTR pszSrcOrig = pszSrc;
    ASSERT(pszSrc)

    *pszDst++ = TEXT('&');
    while (*pszSrc)
    {
        if (*pszSrc == TEXT('&'))
            pszSrc++;
        else
            *pszDst++ = *pszSrc++;
    }
    *pszDst = TEXT('\0');
    lstrcpy(pszSrcOrig, szDst);
}

//---------------------------------------------------------------------------
void FileList_MangleAmpersands(HDPA hdpaFLI)
{
    int cItems = DPA_GetPtrCount(hdpaFLI);
    int i;

    for (i = 0; i < cItems; i++)
    {
        TCHAR szName[MAX_PATH];
        PFILELIST_ITEM pfli = DPA_GetPtr(hdpaFLI, i);
        LPTSTR pszName;
        lstrcpy(szName, pfli->pszName);
        MangleAmpersands(szName);
        if (Sz_AllocCopy(szName, &pszName))
        {
            LFree(pfli->pszName);
            pfli->pszName = pszName;
        }        
    }
}

//---------------------------------------------------------------------------
BOOL InsertUsingPidl(HMENU hmenu, PIDL pidlFolder, FMIUP_FLAGS flags, UINT nPos, PUINT pcItems)
{
    BOOL fRet = TRUE;
    HDPA hdpa;
    
    if (FileList_Create(pidlFolder, &hdpa, NULL))
    {
        FileList_MangleAmpersands(hdpa);
        if (FileList_Sort(hdpa))
            fRet = FileList_AddToMenu(pidlFolder, hmenu, hdpa, flags, nPos, pcItems);
        FileList_Destroy(hdpa);
    }

    return fRet;
}

//---------------------------------------------------------------------------
BOOL InsertAtPosUsingPidl(HMENU hmenu, PIDL pidlFolder, UINT idFirst, FMIUP_FLAGS flags, UINT nPos, PUINT pcItems)
{
    BOOL fRet = FALSE;
    PFM_INFO pfmi;

    // Menu must already be a proper FileMenu...
    if (FM_GetInfo(hmenu, &pfmi))
    {
        if (!pfmi->pidlFolder)
        {
            pfmi->pidlFolder = ILClone(pidlFolder);
            pfmi->id.idCurrent = idFirst;
            pfmi->nPos = nPos;
            
            // Do it later?
            if (flags & FMIUP_DELAY)
            {
                pfmi->flags |= FMI_EXPAND;
                fRet = TRUE;
            }
            else
            {
                UINT cItems;
                if (!pcItems)
                    pcItems = &cItems;
                if (InsertUsingPidl(hmenu, pidlFolder, flags, nPos, pcItems))
                    fRet = TRUE;
            }
        }
        else
        {
            ASSERT(0);
        }
    }

    return fRet;
}

//---------------------------------------------------------------------------
BOOL FM_InsertUsingPidl(HMENU hmenu, PIDL pidlFolder, UINT idFirst, FMIUP_FLAGS flags, PUINT pcItems)
{
    return InsertAtPosUsingPidl(hmenu, pidlFolder, idFirst, flags, 0, pcItems);
}

//---------------------------------------------------------------------------
BOOL FM_AppendUsingPidl(HMENU hmenu, PIDL pidlFolder, UINT idFirst, FMIUP_FLAGS flags, PUINT pcItems)
{
    return InsertAtPosUsingPidl(hmenu, pidlFolder, idFirst, flags, GetMenuItemCount(hmenu), pcItems);
}

//---------------------------------------------------------------------------
BOOL FM_AppendSubMenu(HMENU hmenu, LPCTSTR pszText, UINT id, UINT iImage, UINT fState, HMENU hmenuSub)
{
    return Menu_InsertFMItem(hmenu, (UINT)-1, id, pszText, NULL, iImage, fState, hmenuSub);
}

//---------------------------------------------------------------------------
BOOL FM_AppendItem(HMENU hmenu, LPCSTR pszText, UINT id, UINT fState, UINT iImage)
{
    return Menu_InsertFMItem(hmenu, (UINT)-1, id, pszText, NULL, iImage, fState, NULL);
}

//---------------------------------------------------------------------------
// NB This is only used to supply the image dimensions.
LRESULT FM_OnMeasureItem(LPMEASUREITEMSTRUCT pmi)
{
    BOOL lRet = FALSE;
    PFM_ITEM_DATA pfmid = (PFM_ITEM_DATA)pmi->itemData;
    
    if (!IsBadReadPtr(pfmid, SIZEOF(FM_ITEM_DATA)))
    {
        PFM_INFO pfmi;
        if (FM_GetInfo(pfmid->hmenu, &pfmi))
        {
            if (pfmi->flags & FMI_LARGE_ICONS)
            {
                pmi->itemWidth = g_cxIcon;
                pmi->itemHeight = g_cyIcon;
            }
            else
            {
                pmi->itemWidth = g_cxSmIcon;
                pmi->itemHeight = g_cySmIcon + pfmi->cyExtra;
            }
            lRet = TRUE;
        }
        else
        {
            ASSERT(FALSE);
        }
    }
    else
    {
        ASSERT(FALSE);
    }

    return lRet;    
}

//---------------------------------------------------------------------------
BOOL SetImageIndexFromPidl(PFM_INFO pfmi, PFM_ITEM_DATA pfmid, BOOL fSelImage)
{
    BOOL fRet = FALSE;
    ASSERT(pfmi && pfmid);

    if (pfmi->pidlFolder && pfmid->pidl)
    {    
        IShellFolder *psfDesktop;
        if (SUCCEEDED(SHGetDesktopFolder(&psfDesktop)))
        {
            IShellFolder *psf;

            if (SUCCEEDED(psfDesktop->lpVtbl->BindToObject(psfDesktop, pfmi->pidlFolder, NULL, &IID_IShellFolder, &psf)))
            {
                PUINT piImageSel = NULL;
                if (fSelImage)
                    piImageSel = &pfmid->iImageSel;
                pfmid->iImage = SHMapPIDLToSystemImageListIndex(psf, pfmid->pidl, piImageSel);
                fRet = TRUE;
                psf->lpVtbl->Release(psf);
            }
            psfDesktop->lpVtbl->Release(psfDesktop);
        }
    }
        
    return fRet;
}

//---------------------------------------------------------------------------
BOOL ItemHasSubmenu(HMENU hmenu, UINT idCmd)
{
    BOOL fRet = FALSE;
    MENUITEMINFO mii;
    
    mii.cbSize = SIZEOF(mii);
    mii.fMask = MIIM_SUBMENU;
    if (GetMenuItemInfo(hmenu, idCmd, FALSE, &mii) && mii.hSubMenu)
        fRet = TRUE;

    return fRet;
}

//---------------------------------------------------------------------------
// NB This is only used to draw the image.
LRESULT FM_OnDrawItem(LPDRAWITEMSTRUCT pdi)
{
    BOOL lRet = FALSE;
    
    if ((pdi->itemAction & ODA_SELECT) || (pdi->itemAction & ODA_DRAWENTIRE))
    {
        UINT iImage;
        PFM_ITEM_DATA pfmid = (PFM_ITEM_DATA)pdi->itemData;
        if (!IsBadReadPtr(pfmid, SIZEOF(FM_ITEM_DATA)))
        {
            PFM_INFO pfmi;
            if (FM_GetInfo(pfmid->hmenu, &pfmi))
            {
                HDC hdc = pdi->hDC;
                BOOL fSubMenu = ItemHasSubmenu(pfmid->hmenu, pdi->itemID);
                int x, y, cyIcon;
                HIMAGELIST himl;
                if (pfmi->flags & FMI_LARGE_ICONS)
                {
                    himl = pfmi->himlLarge;
                    cyIcon = g_cyIcon;
                }
                else
                {
                    himl = pfmi->himlSmall;
                    cyIcon = g_cySmIcon;
                }
#if 0               
                if (!(pdi->itemState & ODS_SELECTED))   
                {
                    HBRUSH hbr = (HBRUSH)SendMessage(hwnd, WM_CTLCOLOR, (WPARAM)hdc, (LPARAM)pfmid->hmenu);
                    if (hbr)
                    {
                        HBRUSH hbrOld = SelectObject(hdc, hbr);
                        RECT rc;
                        rc.left = 0; rc.right = 10000;
                        rc.top = pdi->rcItem.top-1;
                        rc.bottom = pdi->rcItem.bottom+1;
                        FillRect(hdc, &rc, hbr);
                        SelectObject(hdc, hbrOld);
                    }
                }
#endif                    
                x = pdi->rcItem.left;
                y = (pdi->rcItem.bottom+pdi->rcItem.top-cyIcon)/2;
                if (pfmid->iImage == (UINT)-1)
                    SetImageIndexFromPidl(pfmi, pfmid, fSubMenu);
                if (pfmid->iImageSel && (pdi->itemState & ODS_SELECTED) && fSubMenu)
                    iImage = pfmid->iImageSel;
                else
                    iImage = pfmid->iImage;
                ImageList_DrawEx(himl, iImage, hdc, x, y, 0, 0, GetBkColor(hdc), CLR_NONE, 
                    (GetBkMode(hdc) == TRANSPARENT) ? ILD_TRANSPARENT : ILD_NORMAL);
                lRet = TRUE;
            }
            else
            {
                ASSERT(FALSE);
            }
        }
        else
        {
            ASSERT(FALSE);
        }
    }

    return lRet;
}

//---------------------------------------------------------------------------
void Menu_SetItemBitmap(HMENU hmenu, UINT iPos, HBITMAP hbmp)
{
    MENUITEMINFO mii;
    mii.cbSize = SIZEOF(mii);
    mii.fMask = MIIM_BITMAP;
    mii.hbmpItem = hbmp;
    SetMenuItemInfo(hmenu, iPos, TRUE, &mii);
}

//---------------------------------------------------------------------------
void InvalidateAllImages(HMENU hmenuSub)
{
    int cItems = GetMenuItemCount(hmenuSub);
    int i;
    for (i=0; i<cItems; i++)
    {
        PFM_ITEM_DATA pfmid;
        if (FM_GetItemData(hmenuSub, i, &pfmid))
        {
            if (pfmid->pidl)
                pfmid->iImage = (UINT)-1;
#ifndef USERS_WIERDNESS_FIXED
            Menu_SetItemBitmap(hmenuSub, i, (HBITMAP)0);
#endif            
            Menu_SetItemBitmap(hmenuSub, i, (HBITMAP)-1);
        }
    }
}

//---------------------------------------------------------------------------
UINT Menu_GetItemId(HMENU hmenu, UINT iPos)
{
    MENUITEMINFO mii;
    mii.cbSize = SIZEOF(mii);
    mii.fMask = MIIM_ID;
    GetMenuItemInfo(hmenu, iPos, TRUE, &mii);
    return mii.wID;
}

//---------------------------------------------------------------------------
typedef struct 
{
    UINT cItems;
    UINT cSkip;
} RAI_ENUM_INFO, *PRAI_ENUM_INFO; 

//---------------------------------------------------------------------------
BOOL RemoveAllItemsCallback(HMENU hmenu, int i, PVOID pv)
{
    PRAI_ENUM_INFO prei = pv;
    PFM_ITEM_DATA pfmid;

    if (FM_GetItemData(hmenu, prei->cSkip, &pfmid))
    {
        HMENU hmenuSub = GetSubMenu(hmenu, prei->cSkip);
        if (pfmid->pidl)
            FM_FreeId(hmenu, Menu_GetItemId(hmenu, prei->cSkip));
        FMI_Destroy(pfmid);
        RemoveMenu(hmenu, prei->cSkip, MF_BYPOSITION);
        if (hmenuSub)
            FM_Destroy(hmenuSub);
    }
    else
    {
        prei->cSkip++;
    }
    
    return TRUE;
}

//---------------------------------------------------------------------------
// Recursively remove all the items from the menu without actually nuking it.
BOOL RemoveAllItems(HMENU hmenu)
{
    BOOL fRet = FALSE;
    RAI_ENUM_INFO rei = {0, 0};
    
    if (FM_EnumMenu(hmenu, RemoveAllItemsCallback, &rei))
    {
        DebugMsg(DM_TRACE, "rai: Removed %d items.", rei.cItems);
        fRet = TRUE;
    }

    return fRet;
}

//---------------------------------------------------------------------------
LRESULT FM_OnInitMenuPopup(HMENU hmenuPopup)
{
    LRESULT lRet = 1;
    PFM_INFO pfmi;
    
    if (FM_GetInfo(hmenuPopup, &pfmi))
    {
        if (pfmi->flags & FMI_EXPAND)
        {
            UINT cItems;
            if (InsertUsingPidl(hmenuPopup, pfmi->pidlFolder, FMIUP_NONE, 0, &cItems))
            {
                pfmi->flags &= ~FMI_EXPAND;
            }
        }
        else if (pfmi->flags & FMI_ITEMS_ARE_INVALID)
        {
            if (RemoveAllItems(hmenuPopup))
            {
                UINT cItems = 0;
                if (InsertUsingPidl(hmenuPopup, pfmi->pidlFolder, FMIUP_NONE, pfmi->nPos, &cItems))
                {
                    pfmi->flags &= ~(FMI_ITEMS_ARE_INVALID|FMI_IMAGES_ARE_INVALID);
                }
            }
        }
        else if (pfmi->flags & FMI_IMAGES_ARE_INVALID)
        {
            InvalidateAllImages(hmenuPopup);
            pfmi->flags &= ~FMI_IMAGES_ARE_INVALID;
        }
        lRet = 0;
    }

    return lRet;
}

//---------------------------------------------------------------------------
BOOL FM_GetPidlFromMenuCommand(HMENU hmenu, UINT idCmd, PIDL *ppidlFolder, PIDL *ppidlItem)
{
    BOOL fRet = FALSE;
    MENUITEMINFO mii;

    ASSERT(ppidlFolder);
    ASSERT(ppidlItem);
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_DATA;
    if (GetMenuItemInfo(hmenu, idCmd, FALSE, &mii))
    {
        PFM_ITEM_DATA pfmid = (PFM_ITEM_DATA)mii.dwItemData;
        PFM_INFO pfmi;
        ASSERT(pfmid);
        if (FM_GetInfo(pfmid->hmenu, &pfmi))
        {
            *ppidlFolder = ILClone(pfmi->pidlFolder);
            if (*ppidlFolder)
            {
                *ppidlItem = ILClone(pfmid->pidl);
                if (*ppidlItem)
                    fRet = TRUE;
                else
                    ILFree(*ppidlFolder);
            }
        }
    }

    return fRet;
}

//---------------------------------------------------------------------------
BOOL InvalidateMenuImagesCallback(HMENU hmenu, int i, PVOID pv)
{
    HMENU hmenuSub = GetSubMenu(hmenu, i);
    if (hmenuSub)
        FM_InvalidateAllImages(hmenuSub, TRUE);

    return TRUE;
}

//---------------------------------------------------------------------------
BOOL FM_EnumMenu(HMENU hmenu, PFN_FM_ENUM_CALLBACK pfn, PVOID pv)
{
    BOOL fRet = FALSE;
    PFM_INFO pfmi;
    
    if (FM_GetInfo(hmenu, &pfmi))
    {
        int cItems = GetMenuItemCount(hmenu);
        int i;
        for (i=0; i<cItems; i++)
        {
            if (!(pfn)(hmenu, i, pv))
                break;
        }
        fRet = TRUE;
    }

    return fRet;
}

//---------------------------------------------------------------------------
BOOL FM_SetImageSize(HMENU hmenu, BOOL fLarge)
{
    PFM_INFO pfmi;
    BOOL fRet = FALSE;
    
    if (FM_GetInfo(hmenu, &pfmi))
    {
        if (fLarge)
            pfmi->flags |= FMI_LARGE_ICONS;
        else
            pfmi->flags &= ~FMI_LARGE_ICONS;
        fRet = TRUE;
    }

    return fRet;
}

//---------------------------------------------------------------------------
void FM_InvalidateAllImages(HMENU hmenu, BOOL fRecurse)
{
    PFM_INFO pfmi;
    
    if (FM_GetInfo(hmenu, &pfmi))
    {
        pfmi->flags |= FMI_IMAGES_ARE_INVALID;
        if (fRecurse)
            FM_EnumMenu(hmenu, InvalidateMenuImagesCallback, NULL);
    }
}

//---------------------------------------------------------------------------
BOOL InvalidateItemsCallback(HMENU hmenu, int i, PVOID pv)
{
    HMENU hmenuSub = GetSubMenu(hmenu, i);
    if (hmenuSub)
        FM_InvalidateItems(hmenuSub);

    return TRUE;
}

//---------------------------------------------------------------------------
void FM_InvalidateItems(HMENU hmenu)
{
    PFM_INFO pfmi;
    if (FM_GetInfo(hmenu, &pfmi))
    {
        pfmi->flags |= FMI_ITEMS_ARE_INVALID;
        FM_EnumMenu(hmenu, InvalidateItemsCallback, NULL);
    }
}

//---------------------------------------------------------------------------
void FM_InvalidateItemsByPidl(HMENU hmenu, PIDL pidl)
{
    BOOL fExists;
    HMENU hmenuSub;
    if (FM_GetMenuFromPidl(hmenu, pidl, &hmenuSub, &fExists))
    {
        // Is the item already in the menu?
        if (fExists)
        {
            FM_InvalidateItems(hmenuSub);
        }
    }
}

//---------------------------------------------------------------------------
BOOL FM_EnableItemByCmd(HMENU hmenu, UINT idCmd, BOOL fEnable)
{
    return EnableMenuItem(hmenu, idCmd, MF_BYCOMMAND | fEnable ? MF_ENABLED : MF_DISABLED);
}

//---------------------------------------------------------------------------
BOOL FM_GetItemData(HMENU hmenu, UINT iItem, PFM_ITEM_DATA *ppfmid)
{
    BOOL fRet = FALSE;
    MENUITEMINFO mii;
    ASSERT(hmenu && ppfmid);
        
    mii.cbSize = SIZEOF(mii);
    mii.fMask = MIIM_DATA;
    if (GetMenuItemInfo(hmenu, iItem, TRUE, &mii))
    {
        if (!IsBadReadPtr((PVOID)mii.dwItemData, SIZEOF(FM_ITEM_DATA)))
        {
            *ppfmid = (PVOID)mii.dwItemData;
            fRet = TRUE;
        }
    }
    
    return fRet;;
}

#if 0
//---------------------------------------------------------------------------
BOOL DestroyEverythingCallback(HMENU hmenu, int i, PVOID pv)
{
    PUINT pcItems = pv;
    
    HMENU hmenuSub = GetSubMenu(hmenu, i);
    if (hmenuSub)
    {
        PFM_INFO pfmi;
        if (FM_GetInfo(hmenuSub, &pfmi))
        {
            FM_EnumMenu(hmenuSub, DestroyEverythingCallback, pcItems);
            if (pfmi->pidlFolder)
                ILFree(pfmi->pidlFolder);
        }
    }
    else
    {
        PFM_ITEM_DATA pfmid;
        if (FM_GetItemData(hmenu, i, &pfmid))
            FMI_Destroy(pfmid);
    }    
    
    (*pcItems)++;    
    return TRUE;
}
#endif

//---------------------------------------------------------------------------
BOOL FM_DeleteAllItems(HMENU hmenu)
{
    BOOL fRet = FALSE;
    PFM_INFO pfmi;
    
    if (FM_GetInfo(hmenu, &pfmi))
    {
        // UINT cItems = 0;
        // FM_EnumMenu(hmenu, DestroyEverythingCallback, &cItems);
        // FM_EnumMenu(hmenu, RemoveAllItemsCallback, &cItems);
        // DebugMsg(DM_TRACE, "fm_d: %d items destroyed.", cItems);
        RemoveAllItems(hmenu);
        fRet = TRUE;
    }

    return fRet;
}

//---------------------------------------------------------------------------
// Recursively free all the private data and then nuke the menu.
BOOL FM_Destroy(HMENU hmenu)
{
    BOOL fRet = FALSE;
    PFM_INFO pfmi;
    
    if (FM_GetInfo(hmenu, &pfmi))
    {
        if (FM_DeleteAllItems(hmenu))
        {
            if (pfmi->pidlFolder)
                ILFree(pfmi->pidlFolder);
            // Top level menu thing?
            if (!(pfmi->flags & FMI_CHILD))
            {
                ASSERT(pfmi->id.idCurrent <= ID_MAX);
                DPA_Destroy(pfmi->hdpaFree);
            }
            LFree(pfmi);
            DestroyMenu(hmenu);
            fRet = TRUE;
        }
    }
    
    return fRet;
}

//---------------------------------------------------------------------------
typedef struct
{
    PIDL pidlTarget;
    PIDL pidlChild;
    HMENU hmenuSub;
    BOOL fExists;
} GMFP_ENUM_INFO;
typedef GMFP_ENUM_INFO* PGMFP_ENUM_INFO;

//---------------------------------------------------------------------------
BOOL GetMenuFromPidlCallback(HMENU hmenu, int i, PVOID pv)
{
    PGMFP_ENUM_INFO pei = pv;
    BOOL fRet = TRUE;

    if (pei->pidlChild)
    {
        PFM_ITEM_DATA pfmid;
        if (FM_GetItemData(hmenu, i, &pfmid))
        {
            if (_ILIsEqual(pfmid->pidl, pei->pidlChild))
            {
                pei->fExists = TRUE;
                fRet = FALSE;
            }
        }
    }
    else
    {
        HMENU hmenuSub = GetSubMenu(hmenu, i);
        if (hmenuSub)
        {
            if (FM_GetMenuFromPidl(hmenuSub, pei->pidlTarget, &pei->hmenuSub, &pei->fExists))
            {
                if (pei->hmenuSub)
                    fRet = FALSE;
            }
        }
    }
    
    return fRet;
}

//---------------------------------------------------------------------------
// The real ILIsParent faults if either pidl is null.
BOOL _ILIsParent(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, BOOL fImmediate)
{
    BOOL fRet = FALSE;
    
    if (pidl1 && pidl2)
        fRet = ILIsParent(pidl1, pidl2, fImmediate);

    return fRet;
}

//---------------------------------------------------------------------------
BOOL FM_GetMenuFromPidl(HMENU hmenu, PIDL pidl, HMENU *phmenuSub, BOOL *pfExists)
{
    BOOL fRet = FALSE;
    PFM_INFO pfmi;

    *phmenuSub = NULL;
    *pfExists = FALSE;
    if (FM_GetInfo(hmenu, &pfmi))
    {
        if (_ILIsEqual(pfmi->pidlFolder, pidl))
        {
            *pfExists = TRUE;
            *phmenuSub = hmenu;
            fRet = TRUE;
        }
        else if (_ILIsParent(pfmi->pidlFolder, pidl, FALSE))
        {
            UINT cItems = 0;
            GMFP_ENUM_INFO ei = {pidl, NULL, NULL, FALSE};
            if (_ILIsParent(pfmi->pidlFolder, pidl, TRUE))
            {
                ei.hmenuSub = hmenu;
                ei.pidlChild = ILFindLastID(pidl);
            }
            FM_EnumMenu(hmenu, GetMenuFromPidlCallback, &ei);
            if (ei.hmenuSub)
            {
                *pfExists = ei.fExists;
                *phmenuSub = ei.hmenuSub;
                fRet = TRUE;
            }
        }
    }

    return fRet;
}

//---------------------------------------------------------------------------
typedef struct
{
    LPTSTR psz;
    DWORD dwAttribs;
    PINT piPos;
} GIP_ENUM_INFO;
typedef GIP_ENUM_INFO *PGIP_ENUM_INFO;

//----------------------------------------------------------------------------
void RemoveAmpersands(LPTSTR pszSrc)
{
    TCHAR szDst[MAX_PATH];
    LPTSTR pszDst = szDst;
    LPTSTR pszSrcOrig = pszSrc;
    ASSERT(pszSrc)

    while (*pszSrc)
    {
        if (*pszSrc == TEXT('&'))
            pszSrc++;
        else
            *pszDst++ = *pszSrc++;
    }
    *pszDst = TEXT('\0');
    lstrcpy(pszSrcOrig, szDst);
}

//---------------------------------------------------------------------------
BOOL GetInsertPosCallback(HMENU hmenu, int i, PVOID pv)
{   
    BOOL fCont = TRUE;
    PGIP_ENUM_INFO pei = pv;
    HMENU hmenuSub = GetSubMenu(hmenu, i);
    PFM_ITEM_DATA pfmid;
    
    if (FM_GetItemData(hmenu, i, &pfmid) && pfmid->pidl)
    {
        if ((pei->dwAttribs & SFGAO_FOLDER) && !hmenuSub)
        {
            // Folders come first.
            *(pei->piPos) = i;
            fCont = FALSE;
        }
        else if (((pei->dwAttribs & SFGAO_FOLDER) && hmenuSub) || 
            (!(pei->dwAttribs & SFGAO_FOLDER) && !hmenuSub))
        {
            TCHAR szMenuItem[MAX_PATH];
            GetMenuString(hmenu, i, szMenuItem, ARRAYSIZE(szMenuItem), MF_BYPOSITION);
            RemoveAmpersands(szMenuItem);
            if (lstrcmpi(pei->psz, szMenuItem) < 0)
            {
                *(pei->piPos) = i;
                fCont = FALSE;
            }
        }
    }
    else
    {
        *(pei->piPos) = i;
        fCont = FALSE;
    }    
    
    return fCont;
}

//---------------------------------------------------------------------------
BOOL GetInsertPos(HMENU hmenu, LPTSTR psz, DWORD dwAttribs, PINT piPos)
{
    GIP_ENUM_INFO ei = {psz, dwAttribs, piPos};
    *piPos = 0;
    // NB This assumes pidl items come first and that they are sorted.
    return FM_EnumMenu(hmenu, GetInsertPosCallback, &ei);
}

//---------------------------------------------------------------------------
BOOL DeletePidlItemCallback(HMENU hmenu, int i, PVOID pv)
{   
    BOOL fCont = TRUE;
    PIDL pidlItem = pv;
    PFM_ITEM_DATA pfmid;

    if (FM_GetItemData(hmenu, i, &pfmid))
    {    
        if (_ILIsEqual(pfmid->pidl, pidlItem))
        {
            HMENU hmenuSub = GetSubMenu(hmenu, i);
            if (hmenuSub)
            {
                if (pfmid->pidl)
                    FM_FreeId(hmenu, Menu_GetItemId(hmenu, i));
                FMI_Destroy(pfmid);
                RemoveMenu(hmenu, i, MF_BYPOSITION);                
                FM_Destroy(hmenuSub);
            }
            else
            {
                if (pfmid->pidl)
                    FM_FreeId(hmenu, Menu_GetItemId(hmenu, i));
                FMI_Destroy(pfmid);
                DeleteMenu(hmenu, i, MF_BYPOSITION);                
            }    
        }
    }    
    return fCont;
}

//---------------------------------------------------------------------------
BOOL DeletePidlItem(HMENU hmenuRoot, HMENU hmenu, PIDL pidlItem)
{
    return FM_EnumMenu(hmenu, DeletePidlItemCallback, pidlItem);
}

//---------------------------------------------------------------------------
BOOL InsertPidlItem(HMENU hmenu, PIDL pidlItem, FMIUP_FLAGS flags)
{
    BOOL fRet = FALSE;
    PFM_INFO pfmi;
    
    if (FM_GetInfo(hmenu, &pfmi) && !(pfmi->flags & FMI_EXPAND))
    {
        IShellFolder *psfDesktop;
        if (SUCCEEDED(SHGetDesktopFolder(&psfDesktop)))
        {
            IShellFolder *psf;

            ASSERT(pfmi->pidlFolder && pidlItem);
        
            if (SUCCEEDED(psfDesktop->lpVtbl->BindToObject(psfDesktop, pfmi->pidlFolder, NULL, &IID_IShellFolder, &psf)))
            {
                PFILELIST_ITEM pfli;
                if (FileList_CreateItem(psf, pidlItem, &pfli))
                {
                    int iPos;
                    if (GetInsertPos(hmenu, pfli->pszName, pfli->dwAttribs, &iPos))
                    {
                        MangleAmpersands(pfli->pszName);
                        if (FileList_AddItemToMenu(hmenu, pfmi->pidlFolder, pfli, iPos, flags))
                            fRet = TRUE;
                    }
                    FileList_DestroyItem(pfli);
                }
                psf->lpVtbl->Release(psf);
            }
            psfDesktop->lpVtbl->Release(psfDesktop);
        }
    }

    return fRet;
}

//----------------------------------------------------------------------------
void RemoveEmptyItemIfReq(HMENU hmenu)
{
    PFM_INFO pfmi;

    if (FM_GetInfo(hmenu, &pfmi))
    {
        if ((GetMenuItemCount(hmenu) == 1) && (GetMenuState(hmenu, 0, MF_BYPOSITION) & MF_DISABLED))
        {
            TCHAR szName[MAX_PATH];
            TCHAR szName2[MAX_PATH];
            LoadString(HINST_THISDLL, IDS_NONE, szName, ARRAYSIZE(szName));
            GetMenuString(hmenu, 0, szName2, ARRAYSIZE(szName2), MF_BYPOSITION);
            if (lstrcmpi(szName, szName2) == 0)
            {
                DeleteMenu(hmenu, 0, MF_BYPOSITION);
            }
        }
    }
}

//----------------------------------------------------------------------------
void InsertEmptyItemIfReq(HMENU hmenu)
{
    PFM_INFO pfmi;

    if (FM_GetInfo(hmenu, &pfmi) && !(pfmi->flags & FMI_EXPAND))
    {
        if (GetMenuItemCount(hmenu) == 0)
            InsertEmptyItem(hmenu);
    }
}

//----------------------------------------------------------------------------
BOOL FM_InsertItemByPidl(HMENU hmenu, PIDL pidl, FMIUP_FLAGS flags)
{
    BOOL fRet = FALSE;
    HMENU hmenuSub;
    BOOL fExists;
    
    if (FM_GetMenuFromPidl(hmenu, pidl, &hmenuSub, &fExists))
    {
        // Is the item already in the menu?
        if (!fExists)
        {
            // Nope, add it now.
            PIDL pidlItem = ILClone(ILFindLastID(pidl));
            if (pidlItem)
            {
                RemoveEmptyItemIfReq(hmenuSub);
                if (InsertPidlItem(hmenuSub, pidlItem, flags))
                    fRet = TRUE;
                else
                    InsertEmptyItemIfReq(hmenuSub);
            }
        }
    }

    return fRet;
}

//----------------------------------------------------------------------------
BOOL FM_DeleteItemByPidl(HMENU hmenu, PIDL pidl, FMIUP_FLAGS flags)
{
    BOOL fRet = FALSE;
    HMENU hmenuSub;
    BOOL fExists;
    
    if (FM_GetMenuFromPidl(hmenu, pidl, &hmenuSub, &fExists))
    {
        // Is the item already in the menu?
        if (fExists)
        {
            // Yep, nuke it now.
            PIDL pidlItem = ILFindLastID(pidl);
            DeletePidlItem(hmenu, hmenuSub, pidlItem);
            if (!(flags & FMIUP_NO_EMPTY_ITEM))
                InsertEmptyItemIfReq(hmenuSub);
            fRet = TRUE;
        }
    }

    return TRUE;
}

//---------------------------------------------------------------------------
BOOL InvalidateImageCallback(HMENU hmenu, UINT i, PVOID pv)
{
    UINT iImage = (UINT)pv;
    
    HMENU hmenuSub = GetSubMenu(hmenu, i);
    if (hmenuSub)
    {
        FM_InvalidateImage(hmenuSub, iImage);
    }
    else
    {
        PFM_ITEM_DATA pfmid;
        if (FM_GetItemData(hmenu, i, &pfmid))
        {
            if (pfmid->pidl && (pfmid->iImage == iImage))
                pfmid->iImage = (UINT)-1;
        }
    }    
    return TRUE;
}

//----------------------------------------------------------------------------
BOOL FM_InvalidateImage(HMENU hmenu, UINT iImage)
{
    BOOL fRet = FALSE;
    PFM_INFO pfmi;
    if (FM_GetInfo(hmenu, &pfmi))
    {
        FM_EnumMenu(hmenu, InvalidateImageCallback, (PVOID)iImage);
        fRet = TRUE;
    }

    return fRet;
}

//----------------------------------------------------------------------------
BOOL FM_InvalidateAllImagesByPidl(HMENU hmenu, PIDL pidl)
{
    BOOL fRet = FALSE;
    BOOL fExists;
    HMENU hmenuSub;
    if (FM_GetMenuFromPidl(hmenu, pidl, &hmenuSub, &fExists))
    {
        // Is the item already in the menu?
        if (fExists)
        {
            // Nope, add it now.
            FM_InvalidateAllImages(hmenuSub, FALSE);
            fRet = TRUE;
        }
    }

    return fRet;
}

//----------------------------------------------------------------------------
BOOL FM_ReplaceUsingPidl(HMENU hmenu, PIDL pidlFolder, UINT idFirst, FMIUP_FLAGS flags, PUINT pcItems)
{
    BOOL fRet = FALSE;
    
    if (FM_DeleteAllItems(hmenu))
        fRet = FM_InsertUsingPidl(hmenu, pidlFolder, idFirst, flags, pcItems);

    return fRet;
}
