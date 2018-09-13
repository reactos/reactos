/*
 * misc - misc stuff
 */

#include "tweakui.h"

#pragma BEGIN_CONST_DATA

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  Misc_NextPx
 *
 *  Allocate another slot in the growable X array, returning its index.
 *  The slot is not eaten, however.
 *
 *  The array may move as a result of this call; the caller promises
 *  not to use the resulting pointer after the next alloc call.
 *
 *****************************************************************************/

PV PASCAL
Misc_AllocPx(PGXA pgxa)
{
    if (pgxa->cx >= pgxa->cxAlloc) {
	PV pv = lReAlloc(pgxa->pv, pgxa->cbx * (pgxa->cxAlloc + 5));
	if (pv) {
	    pgxa->pv = pv;
	    pgxa->cxAlloc += 5;
	} else {
	    return 0;
	}
    }
    return (PBYTE)pgxa->pv + pgxa->cbx * pgxa->cx;
}

/*****************************************************************************
 *
 *  Misc_InitPgxa
 *
 *****************************************************************************/

BOOL PASCAL
Misc_InitPgxa(PGXA pgxa, int cbx)
{
    pgxa->cbx = cbx;
    pgxa->cxAlloc = 5;
    pgxa->cx = 0;
    pgxa->pv = lAlloc(pgxa->cbx * pgxa->cxAlloc);
    return (BOOL)pgxa->pv;
}

/*****************************************************************************
 *
 *  Misc_FreePgxa
 *
 *****************************************************************************/

void PASCAL
Misc_FreePgxa(PGXA pgxa)
{
    if (pgxa->pv) {
	lFree(pgxa->pv);
    }
}

/*****************************************************************************
 *
 *  Misc_EnableMenuFromHdlgId
 *
 *	Enable or disable a menu item based on a dialog item.
 *
 *****************************************************************************/

void PASCAL
Misc_EnableMenuFromHdlgId(HMENU hmenu, HWND hdlg, UINT idc)
{
    EnableMenuItem(hmenu, idc, MF_BYCOMMAND | (
		IsWindowEnabled(GetDlgItem(hdlg, idc)) ? 0 : MFS_GRAYED));
}

/*****************************************************************************
 *
 *  Misc_LV_GetCurSel
 *
 *	Return the selection index.
 *
 *****************************************************************************/


int PASCAL
Misc_LV_GetCurSel(HWND hwnd)
{
    return ListView_GetNextItem(hwnd, -1, LVNI_FOCUSED);
}

/*****************************************************************************
 *
 *  Misc_LV_SetCurSel
 *
 *	Set the selection index.
 *
 *****************************************************************************/

void PASCAL
Misc_LV_SetCurSel(HWND hwnd, int iIndex)
{
    ListView_SetItemState(hwnd, iIndex, LVIS_FOCUSED | LVIS_SELECTED,
					LVIS_FOCUSED | LVIS_SELECTED);
}

/*****************************************************************************
 *
 *  Misc_LV_EnsureSel
 *
 *  Make sure *something* is selected.  Try to make it iItem.
 *
 *****************************************************************************/

void PASCAL
Misc_LV_EnsureSel(HWND hwnd, int iItem)
{
    int iItemMax = ListView_GetItemCount(hwnd) - 1;
    if (iItem > iItemMax) {
	iItem = iItemMax;
    }
    Misc_LV_SetCurSel(hwnd, iItem);
}

/*****************************************************************************
 *
 *  Misc_LV_GetItemInfo
 *
 *  Grab some info out of an item.
 *
 *  The incoming LV_ITEM should have any fields initialized which are
 *  required by the mask.  (E.g., the state mask.)
 *
 *****************************************************************************/

void PASCAL
Misc_LV_GetItemInfo(HWND hwnd, LV_ITEM FAR *plvi, int iItem, UINT mask)
{
    plvi->iItem = iItem;
    plvi->iSubItem = 0;
    plvi->mask = mask;
    ListView_GetItem(hwnd, plvi);
}

/*****************************************************************************
 *
 *  Misc_LV_GetParam
 *
 *	Convert an iItem into the associated parameter.
 *
 *****************************************************************************/

LPARAM PASCAL
Misc_LV_GetParam(HWND hwnd, int iItem)
{
    LV_ITEM lvi;
    Misc_LV_GetItemInfo(hwnd, &lvi, iItem, LVIF_PARAM);
    return lvi.lParam;
}

/*****************************************************************************
 *
 *  Misc_LV_HitTest
 *
 *	Perform a hit-test on the listview.  The LPARAM parameter is
 *	the screen coordinates of the message.
 *
 *****************************************************************************/

void PASCAL
Misc_LV_HitTest(HWND hwnd, LV_HITTESTINFO FAR *phti, LPARAM lpPos)
{
    phti->pt.x = (signed short)LOWORD(lpPos);
    phti->pt.y = (signed short)HIWORD(lpPos);

    MapWindowPoints(HWND_DESKTOP, hwnd, &phti->pt, 1);
    
    ListView_HitTest(hwnd, phti);
}


/*****************************************************************************
 *
 *  Misc_Trim
 *
 *	Trim leading and trailing spaces from a string.  Returns a pointer
 *	to the first non-space character, and slaps a '\0' on top of the
 *	last trailing space.
 *
 *****************************************************************************/

LPTSTR PASCAL
Misc_Trim(LPTSTR ptsz)
{
    int ctch;

    for ( ; (unsigned)(*ptsz - 1) <= ' '; ptsz++);
    for (ctch = lstrlen(ptsz); ctch && (unsigned)ptsz[ctch-1] <= ' '; ctch--);
    if (ctch) {
	ptsz[ctch] = '\0';
    }
    return ptsz;
}

/*****************************************************************************
 *
 *  Misc_SetDrbSize
 *
 *	Set the size of the buffer in a DRB.
 *
 *****************************************************************************/

typedef struct DRB {			/* dynamically resizable buffer */
    PV pv;				/* Actual buffer */
    UINT cb;				/* Size of buffer in bytes */
} DRB, *PDRB;

void PASCAL
Misc_SetDrbSize(PDRB pdrb, UINT cb)
{
    if (pdrb->pv) {
	LocalFree(pdrb->pv);
	pdrb->pv = 0;
	pdrb->cb = 0;
    }
    if (cb) {
	pdrb->pv = LocalAlloc(LMEM_FIXED, cb);
	if (pdrb->pv) {
	    pdrb->cb = cb;
	}
    }
}

/*****************************************************************************
 *
 *  Misc_EnsureDrb
 *
 *	Make sure that the DRB can handle at least cb bytes.
 *
 *	Round up to 4K boundary for convenience.
 *
 *****************************************************************************/

BOOL PASCAL
Misc_EnsureDrb(PDRB pdrb, UINT cb)
{
    if (cb <= pdrb->cb) {
    } else {
	Misc_SetDrbSize(pdrb, (cb + 1023) & ~1023);
    }
    return (BOOL)pdrb->pv;
}

/*****************************************************************************
 *
 *  Misc_CopyRegWorker
 *
 *  Copy a registry tree from one point to another.
 *
 *  Since this is a recursive routine, don't allocate lots of goo off the
 *  stack.
 *
 *****************************************************************************/

typedef struct RCI {			/* registry copy info */
    DRB drb;				/* Data being copied */
    TCH tszKey[ctchKeyMax];		/* Key name buffer */
} RCI, *PRCI;

BOOL PASCAL
Misc_CopyRegWorker(PRCI prci, HKEY hkSrcRoot, PCTSTR ptszSrc,
			      HKEY hkDstRoot, PCTSTR ptszDst)
{
    HKEY hkSrc, hkDst;
    BOOL fRc;
    if (_RegOpenKey(hkSrcRoot, ptszSrc, &hkSrc) == 0) {
	if (RegCreateKey(hkDstRoot, ptszDst, &hkDst) == 0) {
	    int i;

	    /*
	     *	The first loop copies the values.
	     */
	    for (i = 0; ; i++) {
		DWORD ctch = cA(prci->tszKey), cb = 0, dwType;
		switch (RegEnumValue(hkSrc, i, prci->tszKey, &ctch, 0,
				     &dwType, 0, &cb)) {
		case ERROR_NO_MORE_ITEMS: goto valuesdone;

		/*
		 *  We can get an ERROR_SUCCESS if the value length is 0.
		 */
		case ERROR_SUCCESS:
		case ERROR_MORE_DATA:
		    if (Misc_EnsureDrb(&prci->drb, cb)) {
			DWORD dwType;
			if (RegQueryValueEx(hkSrc, prci->tszKey, 0, &dwType,
				            prci->drb.pv, &cb) == 0 &&
			    RegSetValueEx(hkDst, prci->tszKey, 0, dwType,
					  prci->drb.pv, cb) == 0) {
			} else {
			    fRc = 0; goto stopkey;
			}
		    } else {
			fRc = 0; goto stopkey;
		    }
		    break;

		default:
		    fRc = 0; goto stopkey;
		}
	    }
	    valuesdone:;

	    /*
	     *	The second loop recurses on each subkey.
	     */
	    for (i = 0; ; i++) {
		switch (RegEnumKey(hkSrc, i, prci->tszKey, cA(prci->tszKey))) {
		case ERROR_NO_MORE_ITEMS: goto keysdone;

		/*
		 *  We can get an ERROR_SUCCESS if the subkey length is 0.
		 *  (Thought that might be illegal for other reasons, but
		 *  we'll go along with it anyway.)
		 */
		case ERROR_SUCCESS:
		case ERROR_MORE_DATA:
		    if (Misc_CopyRegWorker(prci, hkSrc, prci->tszKey,
						 hkDst, prci->tszKey)) {
		    } else {
			fRc = 0; goto stopkey;
		    }
		    break;

		default:
		    fRc = 0; goto stopkey;
		}
	    }
	    keysdone:;

	    stopkey:;
	    RegCloseKey(hkDst);
	} else {
	    fRc = 0;
	}
	RegCloseKey(hkSrc);
    } else {
	fRc = 0;
    }
    return fRc;
}

/*****************************************************************************
 *
 *  Misc_CopyReg
 *
 *  Copy a registry tree from one point to another.
 *
 *****************************************************************************/

BOOL PASCAL
Misc_CopyReg(HKEY hkSrcRoot, PCTSTR ptszSrc, HKEY hkDstRoot, PCTSTR ptszDst)
{
    BOOL fRc;
    RCI rci;
    rci.drb.pv = 0;
    rci.drb.cb = 0;

    fRc = Misc_CopyRegWorker(&rci, hkSrcRoot, ptszSrc, hkDstRoot, ptszDst);

    Misc_SetDrbSize(&rci.drb, 0);

    if (fRc) {
    } else {
	/* Clean up partial copy */
	RegDeleteTree(hkDstRoot, ptszDst);
    }

    return fRc;
}

/*****************************************************************************
 *
 *  Misc_RenameReg
 *
 *  Rename a registry key by copying it and deleting the original.
 *
 *****************************************************************************/

BOOL PASCAL
Misc_RenameReg(HKEY hkRoot, PCTSTR ptszKey, PCTSTR ptszSrc, PCTSTR ptszDst)
{
    BOOL fRc;
    HKEY hk;

    if (_RegOpenKey(hkRoot, ptszKey, &hk) == 0) {
        if (Misc_CopyReg(hk, ptszSrc, hk, ptszDst)) {
            RegDeleteTree(hk, ptszSrc);
            fRc = TRUE;
        } else {
            fRc = FALSE;
        }
        RegCloseKey(hk);
    } else {
        fRc = FALSE;
    }

    return fRc;
}

/*****************************************************************************
 *
 *  lstrcatnBs
 *
 *  Like lstrcatn, but with a backslash stuck in between.
 *
 *****************************************************************************/

void PASCAL
lstrcatnBs(PTSTR ptszDst, PCTSTR ptszSrc, int ctch)
{
    int ctchDst = lstrlen(ptszDst);
    ptszDst += ctchDst;
    ctch -= ctchDst;
    if (ctch > 1) {
	ptszDst[0] = TEXT('\\');
	lstrcpyn(ptszDst + 1, ptszSrc, ctch - 1);
    }
}

/*****************************************************************************
 *
 *  Misc_GetShellIconSize
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

KL const c_klShellIconSize = { &c_hkCU, c_tszMetrics, c_tszShellIconSize };

#pragma END_CONST_DATA


UINT PASCAL
Misc_GetShellIconSize(void)
{
    return GetIntPkl(GetSystemMetrics(SM_CXICON), &c_klShellIconSize);
}

/*****************************************************************************
 *
 *  Misc_SetShellIconSize
 *
 *	Dork the shell icon size and rebuild.
 *
 *****************************************************************************/

void PASCAL
Misc_SetShellIconSize(UINT ui)
{
    SetIntPkl(ui, &c_klShellIconSize);
    SendMessage(HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETNONCLIENTMETRICS, 0L);
}

/*****************************************************************************
 *
 *  Misc_RebuildIcoCache
 *
 *	Force the shell to rebuild its icon cache.
 *
 *	Due to the way the shell works, we have to do this by changing
 *	the icon sizes, then changing it back.
 *
 *****************************************************************************/

void PASCAL
Misc_RebuildIcoCache(void)
{
    UINT cxIcon = Misc_GetShellIconSize();
    Misc_SetShellIconSize(cxIcon-1);
    Misc_SetShellIconSize(cxIcon);
    Explorer_HackPtui();
}
