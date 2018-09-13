#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "pgIconSz.h"

CIconSizePg::CIconSizePg(
	LPPROPSHEETPAGE ppsp
	) : WizardPage(ppsp, IDS_LKPREV_ICONTITLE, IDS_LKPREV_ICONTITLE)
{
	m_dwPageId = IDD_PREV_ICON1;
	ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
	m_himageNormalSmall = 0;
	m_himageNormalLarge = 0;
	m_himageLargeSmall = 0;
	m_himageLargeLarge = 0;
	m_himageExLargeSmall = 0;
	m_himageExLargeLarge = 0;
}


CIconSizePg::~CIconSizePg(
	VOID
	)
{
	if(m_himageNormalSmall)
		ImageList_Destroy(m_himageNormalSmall);
	if(m_himageNormalLarge)
		ImageList_Destroy(m_himageNormalLarge);
	if(m_himageLargeSmall)
		ImageList_Destroy(m_himageLargeSmall);
	if(m_himageLargeLarge)
		ImageList_Destroy(m_himageLargeLarge);
	if(m_himageExLargeSmall)
		ImageList_Destroy(m_himageExLargeSmall);
	if(m_himageExLargeLarge)
		ImageList_Destroy(m_himageExLargeLarge);
}

LRESULT
CIconSizePg::OnInitDialog(
	HWND hwnd,
	WPARAM wParam,
	LPARAM lParam
	)
{

	m_himageNormalSmall = ImageList_LoadBitmap(g_hInstDll, MAKEINTRESOURCE(IDB_ICON_NORMAL_SMALL), 16, 10, RGB(255, 0, 0));
	m_himageNormalLarge = ImageList_LoadBitmap(g_hInstDll, MAKEINTRESOURCE(IDB_ICON_NORMAL_LARGE), 32, 10, RGB(255, 0, 0));
	m_himageLargeSmall = ImageList_LoadBitmap(g_hInstDll, MAKEINTRESOURCE(IDB_ICON_NORMAL_SMALL), 16, 10, RGB(255, 0, 0));
	m_himageLargeLarge = ImageList_LoadBitmap(g_hInstDll, MAKEINTRESOURCE(IDB_ICON_LARGE_LARGE), 48, 10, RGB(255, 0, 0));
	m_himageExLargeSmall = ImageList_LoadBitmap(g_hInstDll, MAKEINTRESOURCE(IDB_ICON_NORMAL_SMALL), 16, 10, RGB(255, 0, 0));
	m_himageExLargeLarge = ImageList_LoadBitmap(g_hInstDll, MAKEINTRESOURCE(IDB_ICON_EXLARGE_LARGE), 64, 10, RGB(255, 0, 0));


	m_hwndList = GetDlgItem(m_hwnd, IDC_LIST1);

	ListView_SetImageList(m_hwndList, m_himageNormalSmall, LVSIL_SMALL);
	ListView_SetImageList(m_hwndList, m_himageNormalLarge, LVSIL_NORMAL);

	Button_SetCheck(GetDlgItem(m_hwnd, IDC_PREVIEW_LARGE), TRUE);
	Button_SetCheck(GetDlgItem(m_hwnd, IDC_ICON_NORMAL), TRUE);

	LV_ITEM lvi;
	memset(&lvi, 0, sizeof(lvi));
	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;


	int rgnStringIds[] = {	IDS_ICONPREV_ITEM1,
							IDS_ICONPREV_ITEM2,
							IDS_ICONPREV_ITEM3,
							IDS_ICONPREV_ITEM4,
							IDS_ICONPREV_ITEM5,
							IDS_ICONPREV_ITEM6,
							};
	
	TCHAR sz[50];
	for(int i=0;i<6;i++)
	{
		LoadString(g_hInstDll, rgnStringIds[i], sz, ARRAYSIZE(sz));
		lvi.iItem = i;
		lvi.pszText = sz;
		lvi.cchTextMax = lstrlen(lvi.pszText);
		lvi.iImage = i;
		lvi.lParam = i;
		ListView_InsertItem(m_hwndList, &lvi);
	}

#if 0 // JMC: Maybe set the background color to the desktop color
	ListView_SetBkColor(m_hwndList, RGB(0, 0, 0));
	ListView_SetTextBkColor(m_hwndList, RGB(0, 0, 0));
	ListView_SetTextColor(m_hwndList, RGB(255, 255, 255));
#endif
	
	UpdateControls();
	return 1;
}


void CIconSizePg::UpdateControls()
{
}

// Default Sort Function
int CALLBACK SimpleCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	if(lParam1 > lParam2)
		return 1;
	if(lParam1 < lParam2)
		return -1;
	return 0;
}



LRESULT
CIconSizePg::OnCommand(
	HWND hwnd,
	WPARAM wParam,
	LPARAM lParam
	)
{
	LRESULT lResult = 1;

	WORD wNotifyCode = HIWORD(wParam);
	WORD wCtlID      = LOWORD(wParam);
	HWND hwndCtl     = (HWND)lParam;


	DWORD dwStyle;

	// Fake item that will be added and removed to make sure the scroll bar
	// shows correctly after changing views
	LV_ITEM lviFakeItem;
	memset(&lviFakeItem, 0, sizeof(lviFakeItem));
	lviFakeItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lviFakeItem.iItem = ListView_GetItemCount(m_hwndList);
	lviFakeItem.pszText = __TEXT("");
	lviFakeItem.cchTextMax = lstrlen(lviFakeItem.pszText);
	lviFakeItem.iImage = 0;
	lviFakeItem.lParam = lviFakeItem.iItem;


	switch(wCtlID)
	{
	case IDC_PREVIEW_LARGE:
		// Switch to large view
		dwStyle = GetWindowLong(m_hwndList, GWL_STYLE);
		dwStyle &= ~LVS_SMALLICON;
		dwStyle |= LVS_ICON;
		SetWindowLong(m_hwndList, GWL_STYLE, dwStyle);
		// Add and remove a fake item to make sure the scroll bar is set correctly.
		ListView_InsertItem(m_hwndList, &lviFakeItem);
		ListView_DeleteItem(m_hwndList, lviFakeItem.iItem);

		break;
	case IDC_PREVIEW_SMALL:
		dwStyle = GetWindowLong(m_hwndList, GWL_STYLE);
		dwStyle &= ~LVS_ICON;
		dwStyle |= LVS_SMALLICON;
		SetWindowLong(m_hwndList, GWL_STYLE, dwStyle);
		// Add and remove a fake item to make sure the scroll bar is set correctly.
		ListView_InsertItem(m_hwndList, &lviFakeItem);
		ListView_DeleteItem(m_hwndList, lviFakeItem.iItem);
		break;
	case IDC_ICON_NORMAL:
		ListView_SetImageList(m_hwndList, m_himageNormalSmall, LVSIL_SMALL);
		ListView_SetImageList(m_hwndList, m_himageNormalLarge, LVSIL_NORMAL);
		break;
	case IDC_ICON_LARGE:
		ListView_SetImageList(m_hwndList, m_himageLargeSmall, LVSIL_SMALL);
		ListView_SetImageList(m_hwndList, m_himageLargeLarge, LVSIL_NORMAL);
		break;
	case IDC_ICON_EXLARGE:
		ListView_SetImageList(m_hwndList, m_himageExLargeSmall, LVSIL_SMALL);
		ListView_SetImageList(m_hwndList, m_himageExLargeLarge, LVSIL_NORMAL);
		break;
	default:
		break;
	}

	ListView_SortItems(m_hwndList, SimpleCompareFunc, 0);
	ListView_Update(m_hwndList, 0);

	return lResult;
}

LRESULT
CIconSizePg::OnPSN_WizNext(
						   HWND hwnd,
						   INT idCtl,
						   LPPSHNOTIFY pnmh
						   )
{
	int nIconSize = 32;

	if(Button_GetCheck(GetDlgItem(m_hwnd, IDC_ICON_LARGE)))
		nIconSize = 48;
	else if(Button_GetCheck(GetDlgItem(m_hwnd, IDC_ICON_LARGE)))
		nIconSize = 64;

	g_Options.m_schemePreview.m_nIconSize = nIconSize;
	g_Options.ApplyPreview();

	return WizardPage::OnPSN_WizNext(hwnd, idCtl, pnmh);
}

