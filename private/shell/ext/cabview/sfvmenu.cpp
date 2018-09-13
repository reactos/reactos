//*******************************************************************************************
//
// Filename : SFVMenu.cpp
//	
//				Implementation file for CSFView menu related methods
//
// Copyright (c) 1994 - 1996 Microsoft Corporation. All rights reserved
//
//*******************************************************************************************

#include "Pch.H"

#include "SFView.H"
#include "SFVWnd.H"

#include "Resource.H"

enum
{
	SUBMENU_EDIT,
	SUBMENU_VIEW,
} ;

enum
{
	SUBMENUNF_EDIT,
	SUBMENUNF_VIEW,
} ;

static HMENU _GetMenuFromID(HMENU hmMain, UINT uID)
{
	MENUITEMINFO miiSubMenu;

	miiSubMenu.cbSize = sizeof(MENUITEMINFO);
	miiSubMenu.fMask  = MIIM_SUBMENU;
	miiSubMenu.cch = 0;     // just in case

	if (!GetMenuItemInfo(hmMain, uID, FALSE, &miiSubMenu))
	{
		return NULL;
	}

	return(miiSubMenu.hSubMenu);
}


static int _GetOffsetFromID(HMENU hmMain, UINT uID)
{
	int index;

	for (index = GetMenuItemCount(hmMain)-1; index>=0; index--)
	{
		MENUITEMINFO mii;
		mii.cbSize = sizeof(MENUITEMINFO);
		mii.fMask = MIIM_ID;
		mii.cch = 0;	// just in case

		if (GetMenuItemInfo(hmMain, (UINT)index, TRUE, &mii)
			&& (mii.wID == FCIDM_MENU_VIEW_SEP_OPTIONS))
		{
			// merge it right above the separator.		
			break;
		}
	}

	return(index);
}


int CSFView::GetMenuIDFromViewMode()
{
	switch (m_fs.ViewMode)
	{
	case FVM_SMALLICON:
		return(IDC_VIEW_SMALLICON);
	case FVM_LIST:
		return(IDC_VIEW_LIST);
	case FVM_DETAILS:
		return(IDC_VIEW_DETAILS);
	default:
		return(IDC_VIEW_ICON);
	}
}


BOOL CSFView::GetArrangeText(int iCol, UINT idFmt, LPSTR pszText, UINT cText)
{
	char szFormat[200];
	LoadString(g_ThisDll.GetInstance(), idFmt, szFormat, sizeof(szFormat));

	LV_COLUMN col;
	char szText[80];

	col.mask = LVCF_TEXT;
	col.pszText = szText;
	col.cchTextMax = sizeof(szText);

	if (!m_cView.GetColumn(iCol, &col))
	{
		return(FALSE);
	}

	char szCommand[sizeof(szFormat)+sizeof(szText)];
	wsprintf(szCommand, szFormat, szText);

	lstrcpyn(pszText, szCommand, cText);

	return(TRUE);
}


void CSFView::MergeArrangeMenu(HMENU hmView)
{
	// Now add the sorting menu items
	for (int i=0; i<MAX_COL; ++i)
	{
		MENUITEMINFO mii;
		mii.cbSize = sizeof(mii);

		char szCommand[100];
		if (!GetArrangeText(i, IDS_BYCOL_FMT, szCommand, sizeof(szCommand)))
		{
			mii.fMask = MIIM_TYPE;
			mii.fType = MFT_SEPARATOR;
			InsertMenuItem(hmView, IDC_ARRANGE_AUTO, FALSE, &mii);

			break;
		}

		mii.fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID;
		mii.fType = MFT_STRING;
		mii.fState = MF_ENABLED;
		mii.wID = IDC_ARRANGE_BY + i;
		mii.dwTypeData = szCommand;

		InsertMenuItem(hmView, IDC_ARRANGE_AUTO, FALSE, &mii);
	}
}


void CSFView::MergeViewMenu(HMENU hmenu, HMENU hmMerge)
{
	HMENU hmView = _GetMenuFromID(hmenu, FCIDM_MENU_VIEW);
	if (!hmView)
	{
		return;
	}

	int iOptions = _GetOffsetFromID(hmView, FCIDM_MENU_VIEW_SEP_OPTIONS);

	Cab_MergeMenus(hmView, hmMerge, (UINT)iOptions, FCIDM_SHVIEWFIRST, FCIDM_SHVIEWLAST,
		MM_SUBMENUSHAVEIDS | MM_ADDSEPARATOR);

	MergeArrangeMenu(hmView);
}

//*****************************************************************************
//
// CSFView::OnActivate
//
// Purpose:
//
//        handles the UIActivate call by ShellBrowser
//
// Parameters:
//
//    UINT  uState    -    UIActivate flag
//
// Comments:
//
//*****************************************************************************


BOOL CSFView::OnActivate(UINT uState)
{
	if (m_uState == uState)
	{
		return(TRUE);
	}

	OnDeactivate();

	{
		// Scope cMenu
		CMenuTemp cMenu(CreateMenu());

		if (!((HMENU)cMenu))
		{
			return(TRUE);
		}

		OLEMENUGROUPWIDTHS mwidth = { { 0, 0, 0, 0, 0, 0 } };
        
		// insert menus into the shell browser
		// Explorer returns the wrong value so don't check
		m_psb->InsertMenusSB(cMenu, &mwidth);
        
		// store this current menu 
		m_cmCur.Attach(cMenu.Detach());
	}

	// Get the edit menu in the browser
	HMENU hmEdit = _GetMenuFromID(m_cmCur, FCIDM_MENU_EDIT);

	if (uState == SVUIA_ACTIVATE_FOCUS)
	{
		// load the menu resource
		CMenuTemp cmMerge(LoadMenu(g_ThisDll.GetInstance(),
			MAKEINTRESOURCE(MENU_SFV_MAINMERGE)));
		if ((HMENU)cmMerge)
		{
			// merge it with Edit submenu
			Cab_MergeMenus(hmEdit, GetSubMenu(cmMerge, SUBMENU_EDIT), (UINT)-1,
				FCIDM_SHVIEWFIRST, FCIDM_SHVIEWLAST, MM_ADDSEPARATOR | MM_SUBMENUSHAVEIDS);

			// add the view menu
			MergeViewMenu(m_cmCur, GetSubMenu(cmMerge, SUBMENU_VIEW));
		}
	}
	else
	{
		CMenuTemp cmMerge(LoadMenu(g_ThisDll.GetInstance(),
			MAKEINTRESOURCE(MENU_SFV_MAINMERGENF)));
		if ((HMENU)cmMerge)
		{
			Cab_MergeMenus(hmEdit, GetSubMenu(cmMerge, SUBMENUNF_EDIT), (UINT)-1,
				FCIDM_SHVIEWFIRST, FCIDM_SHVIEWLAST, MM_ADDSEPARATOR | MM_SUBMENUSHAVEIDS);

			// view menu
			MergeViewMenu(m_cmCur, GetSubMenu(cmMerge, SUBMENUNF_VIEW));
		}
	}

	// install the composite menu into the shell browser
	m_psb->SetMenuSB(m_cmCur, NULL, m_cView);
	m_uState = uState;

	return TRUE;
}


BOOL CSFView::OnDeactivate()
{
	if (m_uState != SVUIA_DEACTIVATE)
	{
		m_psb->SetMenuSB(NULL, NULL, NULL);
		m_psb->RemoveMenusSB(m_cmCur);
		DestroyMenu(m_cmCur.Detach());
		m_uState = SVUIA_DEACTIVATE;
	}

	return TRUE;
}

//*****************************************************************************
//
// CSFView::OnCommand
//
// Purpose:
//
//       handle the WM_COMMAND sent by the explorer to the view
//
// Comments:
//
//*****************************************************************************

void CSFView::OnCommand(IContextMenu *pcm, WPARAM wParam, LPARAM lParam)
{
	if (!pcm)
	{
		pcm = m_pcmSel;
	}

	if (pcm)
	{
		pcm->AddRef();
	}
	CEnsureRelease erContext(pcm);

	int idCmd = GET_WM_COMMAND_ID(wParam, lParam);
	DWORD dwStyle;

	switch (idCmd)
	{

	    // Set the FOLDERSETTINGS for this view

	case IDC_VIEW_ICON + FCIDM_SHVIEWFIRST:
		dwStyle = LVS_ICON;
		m_fs.ViewMode = FVM_ICON;
		goto SetStyle;

	case IDC_VIEW_SMALLICON + FCIDM_SHVIEWFIRST:
		dwStyle = LVS_SMALLICON;
		m_fs.ViewMode = FVM_SMALLICON;
		goto SetStyle;

	case IDC_VIEW_LIST + FCIDM_SHVIEWFIRST:
		dwStyle = LVS_LIST;
		m_fs.ViewMode = FVM_LIST;
		goto SetStyle;

	case IDC_VIEW_DETAILS + FCIDM_SHVIEWFIRST:
		dwStyle = LVS_REPORT;
		m_fs.ViewMode = FVM_DETAILS;
		goto SetStyle;

		// set the style of the Listview accordingly
SetStyle:
		m_cView.SetStyle(dwStyle, LVS_TYPEMASK);
		CheckToolbar();
		break;

		// handle the Copy operation
	case IDC_EDIT_COPY + FCIDM_SHVIEWFIRST:
	{
		LPDATAOBJECT pdtobj;
		if (!m_cView.OleInited()
			|| FAILED(m_cView.GetUIObjectFromItem(IID_IDataObject, (LPVOID*)&pdtobj,
			SVGIO_SELECTION)))
		{
			MessageBeep(0);
			break;
		}
		CEnsureRelease erData(pdtobj);

		OleSetClipboard(pdtobj);
		break;
	}


	    // handle selection of items
	case IDC_EDIT_SELALL + FCIDM_SHVIEWFIRST:
		SetFocus(m_cView);
		m_cView.SelAll();
		break;

	case IDC_EDIT_INVSEL + FCIDM_SHVIEWFIRST:
		SetFocus(m_cView);
		m_cView.InvSel();
		break;

	default:
		if (idCmd>=IDC_ARRANGE_BY && idCmd<IDC_ARRANGE_BY+MAX_COL)
		{
			ColumnClick(idCmd - IDC_ARRANGE_BY);
		}
		else if (pcm && idCmd>=SFV_CONTEXT_FIRST && idCmd<=SFV_CONTEXT_LAST)
		{

			// invoke the context menu
			CMINVOKECOMMANDINFO ici;

			ici.cbSize = sizeof(ici);
			ici.fMask = 0;
			ici.hwnd = m_hwndMain;
			ici.lpVerb = MAKEINTRESOURCE(idCmd - SFV_CONTEXT_FIRST);
			ici.lpParameters = NULL;
			ici.lpDirectory = NULL;
			ici.nShow = SW_SHOWNORMAL;

			pcm->InvokeCommand(&ici);
		}
		break;
	}
}


IContextMenu * CSFView::GetSelContextMenu()
{
	if (!m_pcmSel)
	{
		if (FAILED(m_cView.GetUIObjectFromItem(IID_IContextMenu, (LPVOID *)&m_pcmSel,
			SVGIO_SELECTION)))
		{
			m_pcmSel = NULL;
			return(m_pcmSel);
		}
	}

	m_pcmSel->AddRef();
	return(m_pcmSel);
}


void CSFView::ReleaseSelContextMenu()
{
	if (m_pcmSel)
	{
		m_pcmSel->Release();
		m_pcmSel = NULL;
	}
}


void CSFView::InitFileMenu(HMENU hmInit)
{
	//
	// Don't touch the file menu unless we have the focus.
	//
	if (m_uState != SVUIA_ACTIVATE_FOCUS)
	{
		return;
	}

	BOOL bDeleteItems = FALSE;
	int i;

	// Remove all the menu items we've added.
	for (i = GetMenuItemCount(hmInit) - 1; i >= 0; --i)
	{
		if (!bDeleteItems)
		{
			MENUITEMINFO mii;
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_ID;
			mii.cch = 0;     // just in case

			if (GetMenuItemInfo(hmInit, i, TRUE, &mii))
			{
				if (mii.wID>=SFV_CONTEXT_FIRST && mii.wID<=SFV_CONTEXT_LAST)
				{
					bDeleteItems = TRUE;
				}
			}
		}

		if (bDeleteItems)
		{
			DeleteMenu(hmInit, i, MF_BYPOSITION);
		}
	}

	// Let the object add the separator.
	if (CSFViewDlg::IsMenuSeparator(hmInit, 0))
	{
		DeleteMenu(hmInit, 0, MF_BYPOSITION);
	}

	//
	// Now add item specific commands to the menu
	// This is done by seeing if we already have a context menu
	// object for our selection.  If not we generate it now.
	//
	IContextMenu *pcmSel = GetSelContextMenu();
	if (pcmSel)
	{
		pcmSel->QueryContextMenu(hmInit, 0, SFV_CONTEXT_FIRST,
			SFV_CONTEXT_LAST, CMF_DVFILE);
		pcmSel->Release();
	}

	// Note that the SelContextMenu stays around until the selection changes or we
	// close the window, but it doesn't really matter that much
}


void CSFView::InitEditMenu(HMENU hmInit)
{
	ULONG dwAttr = SFGAO_CANCOPY;
	UINT uFlags = (m_cView.OleInited()
		&& SUCCEEDED(m_cView.GetAttributesFromItem(&dwAttr, SVGIO_SELECTION))
		&& (dwAttr & SFGAO_CANCOPY)) ? MF_ENABLED : MF_GRAYED;

	EnableMenuItem(hmInit, IDC_EDIT_COPY + FCIDM_SHVIEWFIRST, uFlags | MF_BYCOMMAND);
}


void CSFView::InitViewMenu(HMENU hmInit)
{
	int iCurViewMenuItem = GetMenuIDFromViewMode() + FCIDM_SHVIEWFIRST;
	UINT uEnable;

	CheckMenuRadioItem(hmInit, IDC_VIEW_ICON, IDC_VIEW_DETAILS,
		iCurViewMenuItem, MF_BYCOMMAND | MF_CHECKED);

	uEnable = (iCurViewMenuItem==IDC_VIEW_LIST+FCIDM_SHVIEWFIRST 
		|| iCurViewMenuItem==IDC_VIEW_DETAILS+FCIDM_SHVIEWFIRST) ?
		(MF_GRAYED | MF_BYCOMMAND)  :  (MF_ENABLED | MF_BYCOMMAND);
	uEnable = MF_GRAYED | MF_BYCOMMAND;

	EnableMenuItem(hmInit, IDC_ARRANGE_GRID + FCIDM_SHVIEWFIRST, uEnable);
	EnableMenuItem(hmInit, IDC_ARRANGE_AUTO + FCIDM_SHVIEWFIRST, uEnable);
	CheckMenuItem(hmInit, IDC_ARRANGE_AUTO + FCIDM_SHVIEWFIRST,
		((uEnable == (MF_ENABLED | MF_BYCOMMAND)) && (m_fs.fFlags & FWF_AUTOARRANGE))
		? MF_CHECKED : MF_UNCHECKED);
}


//*****************************************************************************
//
// CSFView::OnInitMenuPopup
//
// Purpose:
//
//    handle the WM_INITMENUPOPUP message received by CSFViewDlg
//
//
//*****************************************************************************
BOOL CSFView::OnInitMenuPopup(HMENU hmInit, int nIndex, BOOL fSystemMenu)
{
	if (!(HMENU)m_cmCur)
	{
		return(TRUE);
	}

	MENUITEMINFO mii;
	mii.cbSize = sizeof(MENUITEMINFO);
	mii.fMask = MIIM_SUBMENU|MIIM_ID;
	mii.cch = 0;     // just in case

	if (!GetMenuItemInfo(m_cmCur, nIndex, TRUE, &mii) || mii.hSubMenu!=hmInit)
	{
		return(TRUE);
	}

	switch (mii.wID)
	{
	case FCIDM_MENU_FILE:
		InitFileMenu(hmInit);
		break;

	case FCIDM_MENU_EDIT:
		InitEditMenu(hmInit);
		break;

	case FCIDM_MENU_VIEW:
		InitViewMenu(hmInit);
		break;

	default:
		return 1L;
	}

	return 0L;
}


int _FindIt(const UINT *puFirst, UINT uFind, UINT uStep, int cCount)
{
	LPBYTE pbFirst = (LPBYTE)puFirst;

	for (--cCount, pbFirst+=cCount*uStep; cCount>=0; --cCount, pbFirst-=uStep)
	{
		if (*(UINT *)pbFirst == uFind)
		{
			break;
		}
	}

	return(cCount);
}


const struct
{
	UINT idCmd;
	UINT idStr;
	UINT idTT;
} c_idTbl[] =
{
	IDC_VIEW_ICON, IDS_VIEW_ICON, IDS_TT_VIEW_ICON,
	IDC_VIEW_SMALLICON, IDS_VIEW_SMALLICON, IDS_TT_VIEW_SMALLICON,
	IDC_VIEW_LIST, IDS_VIEW_LIST, IDS_TT_VIEW_LIST,
	IDC_VIEW_DETAILS, IDS_VIEW_DETAILS, IDS_TT_VIEW_DETAILS,
	IDC_EDIT_COPY, IDS_EDIT_COPY, IDS_TT_EDIT_COPY,
	IDC_EDIT_SELALL, IDS_EDIT_SELALL, 0,
	IDC_EDIT_INVSEL, IDS_EDIT_INVSEL, 0,
	IDC_ARRANGE_GRID, IDS_ARRANGE_GRID, 0,
	IDC_ARRANGE_AUTO, IDS_ARRANGE_AUTO, 0,
} ;


void CSFView::GetCommandHelpText(UINT idCmd, LPSTR pszText, UINT cchText, BOOL bToolTip)
{
	*pszText = 0;

	if (idCmd>=SFV_CONTEXT_FIRST && idCmd<=SFV_CONTEXT_LAST && m_pcmSel)
	{
		if (bToolTip)
		{
			return;
		}

		m_pcmSel->GetCommandString(idCmd - SFV_CONTEXT_FIRST, GCS_HELPTEXT, NULL,
			pszText, cchText);
	}
	else if (idCmd>=IDC_ARRANGE_BY && idCmd<IDC_ARRANGE_BY+MAX_COL)
	{
		if (bToolTip)
		{
			return;
		}

		GetArrangeText(idCmd-IDC_ARRANGE_BY, IDS_BYCOL_HELP_FMT, pszText, cchText);
	}
	else
	{
		int iid = _FindIt(&c_idTbl[0].idCmd, idCmd-FCIDM_SHVIEWFIRST, sizeof(c_idTbl[0]),
			ARRAYSIZE(c_idTbl));
		if (iid >= 0)
		{
			LoadString(g_ThisDll.GetInstance(),
				bToolTip ? c_idTbl[iid].idTT : c_idTbl[iid].idStr, pszText, cchText);
		}
	}
}


LRESULT CSFView::OnMenuSelect(UINT idCmd, UINT uFlags, HMENU hmenu)
{
	// If we dismissed the menu restore our status bar...
	if (!hmenu && LOWORD(uFlags)==0xffff)
	{
		m_psb->SendControlMsg(FCW_STATUS, SB_SIMPLE, 0, 0L, NULL);
		return 0L;
	}

	if (uFlags & (MF_SYSMENU | MF_SEPARATOR))
	{
		return 0L;
	}

	char szHelpText[80 + 2*MAX_PATH];
	szHelpText[0] = 0;   // in case of failures below

	if (uFlags & MF_POPUP)
	{
		MENUITEMINFO miiSubMenu;

		miiSubMenu.cbSize = sizeof(MENUITEMINFO);
		miiSubMenu.fMask = MIIM_ID;
		miiSubMenu.cch = 0;     // just in case

		if (!GetMenuItemInfo(hmenu, idCmd, TRUE, &miiSubMenu))
		{
			return 0;
		}

		// Change the parameters to simulate a "normal" menu item
		idCmd = miiSubMenu.wID;
		uFlags &= ~MF_POPUP;
	}

	GetCommandHelpText(idCmd, szHelpText, sizeof(szHelpText), FALSE);
	m_psb->SendControlMsg(FCW_STATUS, SB_SETTEXT, SBT_NOBORDERS | 255,
		(LPARAM)szHelpText, NULL);

	m_psb->SendControlMsg(FCW_STATUS, SB_SIMPLE, 1, 0L, NULL);

	return 0;
}
