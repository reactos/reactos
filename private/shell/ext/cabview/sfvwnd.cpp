//*******************************************************************************************
//
// Filename : SFVWnd.cpp
//	
//				Implementation file for CSFVDropSource and CSFViewDlg
//
// Copyright (c) 1994 - 1996 Microsoft Corporation. All rights reserved
//
//*******************************************************************************************

#include "Pch.H"

#include "SFVWnd.H"

#include "ThisDll.h"

#include "Resource.H"

class CSFVDropSource : public CUnknown, public IDropSource
{
public:
	CSFVDropSource() : m_grfInitialKeyState(0) {}
	virtual ~CSFVDropSource();

	STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	STDMETHODIMP QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState);
	STDMETHODIMP GiveFeedback(DWORD dwEffect);

private:
	DWORD m_grfInitialKeyState;
} ;


CSFVDropSource::~CSFVDropSource()
{
}

STDMETHODIMP CSFVDropSource::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
	static const IID *apiid[] = { &IID_IDropSource, NULL };
	LPUNKNOWN aobj[] = { (IDropSource *)this };

	return(QIHelper(riid, ppvObj, apiid, aobj));
}


STDMETHODIMP_(ULONG) CSFVDropSource::AddRef()
{
	return(AddRefHelper());
}


STDMETHODIMP_(ULONG) CSFVDropSource::Release()
{
	return(ReleaseHelper());
}


STDMETHODIMP CSFVDropSource::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
	if (fEscapePressed)
	{
		return(DRAGDROP_S_CANCEL);
	}

	// initialize ourself with the drag begin button
	if (m_grfInitialKeyState == 0)
	{
		m_grfInitialKeyState = (grfKeyState & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON));
	}

	if (!(grfKeyState & m_grfInitialKeyState))
	{
		return(DRAGDROP_S_DROP);
	}

	return(S_OK);
}


STDMETHODIMP CSFVDropSource::GiveFeedback(DWORD dwEffect)
{
	return(DRAGDROP_S_USEDEFAULTCURSORS);
}


// Note that the OLESTR gets freed, so don't try to use it later
BOOL StrRetToStr(LPSTR szOut, UINT uszOut, LPSTRRET pStrRet, LPCITEMIDLIST pidl)
{
    switch (pStrRet->uType)
    {
    case STRRET_OLESTR:
    {
        CSafeMalloc sm;

    	OleStrToStrN(szOut, uszOut, pStrRet->pOleStr, -1);
		sm.Free(pStrRet->pOleStr);

    	break;
    }

    case STRRET_CSTR:
    	lstrcpyn(szOut, pStrRet->cStr, uszOut);
    	break;

    case STRRET_OFFSET:
    	if (pidl)
    	{
    	    lstrcpyn(szOut, ((LPCSTR)&pidl->mkid)+pStrRet->uOffset, uszOut);
    	    break;
    	}

    	// Fall through
    default:
    	if (uszOut)
    	{
    	    *szOut = '\0';
    	}
    	return(FALSE);
    }

    return(TRUE);
}


static void _PrettyMenu(HMENU hm)
{
	BOOL bSeparated = TRUE;
	int i;

	for (i=GetMenuItemCount(hm)-1; i>0; --i)
	{
		if (CSFViewDlg::IsMenuSeparator(hm, i))
		{
			if (bSeparated)
			{
				DeleteMenu(hm, i, MF_BYPOSITION);
			}

			bSeparated = TRUE;
		}
		else
		{
			bSeparated = FALSE;
		}
	}

	// The above loop does not handle the case of many separators at
	// the beginning of the menu
	while (CSFViewDlg::IsMenuSeparator(hm, 0))
	{
		DeleteMenu(hm, 0, MF_BYPOSITION);
	}
}


static HMENU _LoadPopupMenu(UINT id, UINT uSubMenu)
{
    HMENU hmParent = LoadMenu(g_ThisDll.GetInstance(), MAKEINTRESOURCE(id));
    if (!hmParent)
    {
		return(NULL);
	}

    HMENU hmPopup = GetSubMenu(hmParent, 0);
    RemoveMenu(hmParent, uSubMenu, MF_BYPOSITION);
    DestroyMenu(hmParent);

    return(hmPopup);
}


void CSFViewDlg::InitDialog()
{
	m_cList.Init(GetDlgItem(m_hDlg, IDC_LISTVIEW), GetDlgItem(m_hDlg, IDC_LISTBOX),
		IDI_GENERIC);

	SetWindowLong(m_cList, GWL_EXSTYLE,
		GetWindowLong(m_cList, GWL_EXSTYLE) | WS_EX_CLIENTEDGE);

	m_hrOLE = OleInitialize(NULL);
}


LRESULT CSFViewDlg::BeginDrag()
{
	if (!OleInited())
	{
		return(0);
	}

	// Get the dwEffect from the selection.
	ULONG dwEffect = DROPEFFECT_LINK | DROPEFFECT_MOVE | DROPEFFECT_COPY;
	GetAttributesFromItem(&dwEffect, SVGIO_SELECTION);

	// Just in case
	dwEffect &= DROPEFFECT_LINK | DROPEFFECT_MOVE | DROPEFFECT_COPY;
	if (!dwEffect)
	{
		return(0);
	}

	LPDATAOBJECT pdtobj;
	if (FAILED(GetUIObjectFromItem(IID_IDataObject, (LPVOID*)&pdtobj, SVGIO_SELECTION)))
	{
		return(0);
	}
	CEnsureRelease erData(pdtobj);

	CSFVDropSource *pcsrc = new CSFVDropSource;
	if (!pcsrc)
	{
		return(0);
	}
	pcsrc->AddRef();
	CEnsureRelease erCDSrc((IDropSource*)pcsrc);

	IDropSource *pdsrc;
	HRESULT hres = pcsrc->QueryInterface(IID_IDropSource, (LPVOID*)&pdsrc);
	if (FAILED(hres))
	{
		return(0);
	}
	CEnsureRelease erPDSrc(pdsrc);

	DoDragDrop(pdtobj, pdsrc, dwEffect, &dwEffect);

	return(0);
}


BOOL CSFViewDlg::Notify(LPNMHDR pNotify)
{
	LPNM_LISTVIEW pNotifyLV = (LPNM_LISTVIEW)pNotify;
	LV_DISPINFO *pNotifyDI = (LV_DISPINFO *)pNotify;
	LPTBNOTIFY pNotifyTB = (LPTBNOTIFY)pNotify;
	LPTOOLTIPTEXT pNotifyTT = (LPTOOLTIPTEXT)pNotify;

	switch(pNotify->code)
	{
	case TTN_NEEDTEXT:
		m_psfv->GetCommandHelpText(pNotifyTT->hdr.idFrom, pNotifyTT->szText,
			sizeof(pNotifyTT->szText), TRUE);
		break;

	case TBN_BEGINDRAG:
		m_psfv->OnMenuSelect(pNotifyTB->iItem, 0, 0);
		break;

	case LVN_DELETEITEM:
		m_psfv->m_cMalloc.Free((LPITEMIDLIST)pNotifyLV->lParam);
		break;

	case LVN_GETDISPINFO:
		if (pNotifyDI->item.iSubItem == 0)
		{
			LPCITEMIDLIST pidl = (LPCITEMIDLIST)pNotifyDI->item.lParam;
			if (pNotifyDI->item.mask & LVIF_TEXT)
			{
				STRRET strret;

				if (FAILED(m_psfv->m_psf->GetDisplayNameOf(pidl, 0, &strret)))
				{
					lstrcpyn(pNotifyDI->item.pszText, "", pNotifyDI->item.cchTextMax);
				}
				else
				{
					StrRetToStr(pNotifyDI->item.pszText, pNotifyDI->item.cchTextMax,
						&strret, pidl);
				}
			}

			if (pNotifyDI->item.mask & LVIF_IMAGE)
			{
				// Get the image
				pNotifyDI->item.iImage = m_cList.GetIcon(m_psfv->m_psf, pidl);
			}

			pNotifyDI->item.mask |= LVIF_DI_SETITEM;
		}
		else if (pNotifyDI->item.mask & LVIF_TEXT)
		{
			SFVCB_GETDETAILSOF_DATA gdo;
			gdo.pidl = (LPCITEMIDLIST)pNotifyDI->item.lParam;

			if (m_psfv->CallCB(SFVCB_GETDETAILSOF, pNotifyDI->item.iSubItem, (LPARAM)&gdo)
				== S_OK)
			{
				StrRetToStr(pNotifyDI->item.pszText, pNotifyDI->item.cchTextMax,
					&gdo.str, gdo.pidl);
			}
		}
		break;

	case LVN_COLUMNCLICK:
		m_psfv->ColumnClick(pNotifyLV->iSubItem);
		break;

	case LVN_ITEMCHANGED:
		// We only care about STATECHANGE messages
		if (!(pNotifyLV->uChanged & LVIF_STATE))
		{
			// If the text is changed, we need to flush the cached
			// context menu.
			if (pNotifyLV->uChanged & LVIF_TEXT)
			{
				m_psfv->ReleaseSelContextMenu();
			}
			break;
		}

		// tell commdlg that selection may have changed
		m_psfv->OnStateChange(CDBOSC_SELCHANGE);

		// The rest only cares about SELCHANGE messages
		if ((pNotifyLV->uNewState^pNotifyLV->uOldState) & LVIS_SELECTED)
		{
			m_psfv->ReleaseSelContextMenu();
		}

		break;

	case LVN_BEGINDRAG:
	case LVN_BEGINRDRAG:
		return(BeginDrag());

    case NM_RETURN:
    case NM_DBLCLK:
        ContextMenu((DWORD)-1, TRUE);
        break;

	default:
		return(FALSE);
	}

	return(TRUE);
}


void CSFViewDlg::ContextMenu(DWORD dwPos, BOOL bDoDefault)
{
	int idDefault = -1;
	int nInsert;
	UINT fFlags = 0;
	POINT pt;

	// Find the selected item
	int iItem = ListView_GetNextItem(m_cList, -1, LVNI_SELECTED);

	if (dwPos == (DWORD) -1)
	{
		if (iItem != -1)
		{
			RECT rc;
			int iItemFocus = ListView_GetNextItem(m_cList, -1, LVNI_FOCUSED|LVNI_SELECTED);
			if (iItemFocus == -1)
			{
				iItemFocus = iItem;
			}

			// Note that ListView_GetItemRect returns client coordinates
			ListView_GetItemRect(m_cList, iItemFocus, &rc, LVIR_ICON);
			pt.x = (rc.left+rc.right)/2;
			pt.y = (rc.top+rc.bottom)/2;
		}
		else
		{
			pt.x = pt.y = 0;
		}

		MapWindowPoints(m_cList, HWND_DESKTOP, &pt, 1);
	}
	else
	{
		pt.x = LOWORD(dwPos);
		pt.y = HIWORD(dwPos);
	}

	CMenuTemp cmContext(CreatePopupMenu());
	if (!(HMENU)cmContext)
	{
		// There should be an error message here
		return;
	}

	LPCONTEXTMENU pcm = NULL;

	if (iItem == -1)
	{
		// No selected item; use the background context menu
		nInsert = -1;

		CMenuTemp cmMerge(_LoadPopupMenu(MENU_SFV, 0));
		if (!(HMENU)cmMerge)
		{
			//  There should be an error message here
			return;
		}

		Cab_MergeMenus(cmContext, cmMerge, 0, FCIDM_SHVIEWFIRST, FCIDM_SHVIEWLAST, 0);
		m_psfv->MergeArrangeMenu(cmContext);
		m_psfv->InitViewMenu(cmContext);

		if (FAILED(m_psfv->m_psf->CreateViewObject(m_psfv->m_hwndMain, IID_IContextMenu,
			(LPVOID *)&pcm)))
		{
			pcm = NULL;
		}
	}
	else
	{
		nInsert = 0;

		pcm = m_psfv->GetSelContextMenu();
	}
	CEnsureRelease erPCM(pcm);

	if (pcm)
	{
		if (m_psfv->m_psb)
		{
			// Determine whether we are in Explorer mode
			HWND hwnd = NULL;
			m_psfv->m_psb->GetControlWindow(FCW_TREE, &hwnd);
			if (hwnd)
			{
				fFlags |= CMF_EXPLORE;
			}
		}

		pcm->QueryContextMenu(cmContext, nInsert,
			SFV_CONTEXT_FIRST, SFV_CONTEXT_LAST, fFlags);

		// If this is the common dialog browser, we need to make the
		// default command "Select" so that double-clicking (which is
		// open in common dialog) makes sense.
		if (m_psfv->IsInCommDlg() && iItem!=-1)
		{
			// make sure this is an item
			CMenuTemp cmSelect(_LoadPopupMenu(MENU_SFV, 1));

			Cab_MergeMenus(cmContext, cmSelect, 0, 0, (UINT)-1, MM_ADDSEPARATOR);

			SetMenuDefaultItem(cmContext, 0, MF_BYPOSITION);
		}

		idDefault = GetMenuDefaultItem(cmContext, MF_BYCOMMAND, 0);
	}

	_PrettyMenu(cmContext);

    int idCmd;
    if (bDoDefault)
    {
        if (idDefault < 0)
        {
            return;
        }

        idCmd = idDefault;
    }
    else
    {
	    idCmd = TrackPopupMenu(cmContext, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
		    pt.x, pt.y, 0, m_psfv->m_cView, NULL);
    }

	if ((idCmd==idDefault) && m_psfv->OnDefaultCommand()==S_OK)
	{
		// commdlg browser ate the default command
	}
	else if (idCmd == 0)
	{
		// No item selected
	}
	else
	{
		m_psfv->OnCommand(pcm, GET_WM_COMMAND_MPS(idCmd, 0, 0));
	}
}


BOOL CSFViewDlg::RealDlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
		InitDialog();
		break;

	case WM_DESTROY:
		m_cList.DeleteAllItems();
		if (OleInited())
		{
			OleUninitialize();
			m_hrOLE = E_UNEXPECTED;
		}
		break;

	case WM_NOTIFY:
		SetWindowLong(m_hDlg, DWL_MSGRESULT, Notify((LPNMHDR)lParam));
		break;

	case WM_INITMENUPOPUP:
        m_psfv->OnInitMenuPopup((HMENU)wParam, LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_COMMAND:
		m_psfv->OnCommand(NULL, wParam, lParam);
		break;

	case WM_CONTEXTMENU:
		ContextMenu(lParam);
		break;

	case WM_SIZE:
		SetWindowPos(m_cList, NULL, 0, 0, LOWORD(lParam), HIWORD(lParam),
			SWP_NOZORDER|SWP_SHOWWINDOW);
		break;

	case WM_MENUSELECT:
	    m_psfv->OnMenuSelect(GET_WM_MENUSELECT_CMD(wParam, lParam),
	    	GET_WM_MENUSELECT_FLAGS(wParam, lParam),
	    	GET_WM_MENUSELECT_HMENU(wParam, lParam));
	    break;

	default:
		return(FALSE);
	}

	return(TRUE);
}


UINT CSFViewDlg::CharWidth()
{
	HDC hdc = GetDC(m_cList);
	SelectFont(hdc, FORWARD_WM_GETFONT(m_cList, SendMessage));

	SIZE siz;
	GetTextExtentPoint(hdc, "0", 1, &siz);
	ReleaseDC(m_cList, hdc);

	return(siz.cx);
}


int CSFViewDlg::AddObject(LPCITEMIDLIST pidl)
{
	LV_ITEM item;

	item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	item.iItem = 0x7fff;     // add at end
	item.iSubItem = 0;

	item.iImage = I_IMAGECALLBACK;
	item.pszText = LPSTR_TEXTCALLBACK;
	item.lParam = (LPARAM)pidl;

	return(m_cList.InsertItem(&item));
}


LPCITEMIDLIST CSFViewDlg::GetPIDL(int iItem)
{
	LV_ITEM item;

	item.mask = LVIF_PARAM;
	item.iItem = iItem;
	item.iSubItem = 0;
	item.lParam = 0;
	if (iItem != -1)
	{
		ListView_GetItem(m_cList, &item);
	}

	return((LPCITEMIDLIST)item.lParam);
}


UINT CSFViewDlg::GetItemPIDLS(LPCITEMIDLIST apidl[], UINT cItemMax, UINT uItem)
{
	// We should put the focused one at the top of the list.
	int iItem = -1;
	int iItemFocus = -1;
	UINT cItem = 0;
	UINT uType;

	switch (uItem)
	{
	case SVGIO_SELECTION:
		// special case for faster search
		if (!cItemMax)
		{
			return ListView_GetSelectedCount(m_cList);
		}
		iItemFocus = ListView_GetNextItem(m_cList, -1, LVNI_FOCUSED);
		uType = LVNI_SELECTED;
		break;

	case SVGIO_ALLVIEW:
		// special case for faster search
		if (!cItemMax)
		{
			return ListView_GetItemCount(m_cList);
		}
		uType = LVNI_ALL;
		break;

	default:
		return(0);
	}

	while((iItem=ListView_GetNextItem(m_cList, iItem, uType)) != -1)
	{
		if (cItem < cItemMax)
		{
			// Check if the item is the focused one or not.
			if (iItem == iItemFocus)
			{
				// Yes, put it at the top.
				apidl[cItem] = apidl[0];
				apidl[0] = GetPIDL(iItem);
			}
			else
			{
				// No, put it at the end of the list.
				apidl[cItem] = GetPIDL(iItem);
			}
		}

		cItem++;
	}

	return cItem;
}


HRESULT CSFViewDlg::GetItemObjects(LPCITEMIDLIST **ppidl, UINT uItem)
{
	UINT cItems = GetItemPIDLS(NULL, 0, uItem);
	LPCITEMIDLIST * apidl;

	if (ppidl != NULL)
	{
		*ppidl = NULL;

		if (cItems == 0)
		{
			return(ResultFromShort(0));  // nothing allocated...
		}

		apidl = new LPCITEMIDLIST[cItems];
		if (!apidl)
		{
			return(E_OUTOFMEMORY);
		}

		*ppidl = apidl;
		cItems = GetItemPIDLS(apidl, cItems, uItem);
	}

	return(ResultFromShort(cItems));
}


HRESULT CSFViewDlg::GetUIObjectFromItem(REFIID riid, LPVOID * ppv, UINT uItem)
{
	LPCITEMIDLIST * apidl;
	HRESULT hres = GetItemObjects(&apidl, uItem);
	UINT cItems = ShortFromResult(hres);

	if (FAILED(hres))
	{
		return(hres);
	}

	if (!cItems)
	{
		return(E_INVALIDARG);
	}

	hres = m_psfv->m_psf->GetUIObjectOf(m_psfv->m_hwndMain, cItems, apidl, riid, 0, ppv);

	delete apidl;

	return hres;
}


HRESULT CSFViewDlg::GetAttributesFromItem(ULONG *pdwAttr, UINT uItem)
{
	LPCITEMIDLIST * apidl;
	HRESULT hres = GetItemObjects(&apidl, uItem);
	UINT cItems = ShortFromResult(hres);

	if (FAILED(hres))
	{
		return(hres);
	}

	if (!cItems)
	{
		return(E_INVALIDARG);
	}

	hres = m_psfv->m_psf->GetAttributesOf(cItems, apidl, pdwAttr);

	delete apidl;

	return hres;
}


BOOL CSFViewDlg::IsMenuSeparator(HMENU hm, int i)
{
	MENUITEMINFO mii;

	mii.cbSize = sizeof(MENUITEMINFO);
	mii.fMask = MIIM_TYPE;
	mii.cch = 0;    // WARNING: We MUST initialize it to 0!!!
	if (!GetMenuItemInfo(hm, i, TRUE, &mii))
	{
		return(FALSE);
	}

	if (mii.fType & MFT_SEPARATOR)
	{
		return(TRUE);
	}

	return(FALSE);
}
