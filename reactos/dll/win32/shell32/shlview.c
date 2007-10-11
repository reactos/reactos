/*
 *	ShellView
 *
 *	Copyright 1998,1999	<juergen.schmied@debitel.net>
 *
 * This is the view visualizing the data provied by the shellfolder.
 * No direct access to data from pidls should be done from here.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * FIXME: The order by part of the background context menu should be
 * buily according to the columns shown.
 *
 * FIXME: Load/Save the view state from/into the stream provied by
 * the ShellBrowser
 *
 * FIXME: CheckToolbar: handle the "new folder" and "folder up" button
 *
 * FIXME: ShellView_FillList: consider sort orders
 *
 * FIXME: implement the drag and drop in the old (msg-based) way
 *
 * FIXME: when the ShellView_WndProc gets a WM_NCDESTROY should we do a
 * Release() ???
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winerror.h"
#include "winbase.h"
#include "winnls.h"
#include "objbase.h"
#include "servprov.h"
#include "shlguid.h"
#include "wingdi.h"
#include "winuser.h"
#include "shlobj.h"
#include "undocshell.h"
#include "shresdef.h"
#include "wine/debug.h"

#include "docobj.h"
#include "pidl.h"
#include "shell32_main.h"
#include "shellfolder.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#undef SV_CLASS_NAME

static const WCHAR SV_CLASS_NAME[] = {'S','H','E','L','L','D','L','L','_','D','e','f','V','i','e','w',0};

typedef struct
{   BOOL    bIsAscending;
    INT     nHeaderID;
    INT     nLastHeaderID;
}LISTVIEW_SORT_INFO, *LPLISTVIEW_SORT_INFO;

typedef struct
{
	const IShellViewVtbl*	lpVtbl;
	LONG			ref;
	const IOleCommandTargetVtbl* lpvtblOleCommandTarget;
	const IDropTargetVtbl*	lpvtblDropTarget;
	const IDropSourceVtbl*	lpvtblDropSource;
	const IViewObjectVtbl*	lpvtblViewObject;
	IShellFolder*	pSFParent;
	IShellFolder2*	pSF2Parent;
	IShellBrowser*	pShellBrowser;
	ICommDlgBrowser*	pCommDlgBrowser;
	HWND		hWnd;		/* SHELLDLL_DefView */
	HWND		hWndList;	/* ListView control */
	HWND		hWndParent;
	FOLDERSETTINGS	FolderSettings;
	HMENU		hMenu;
	UINT		uState;
	UINT		cidl;
	LPITEMIDLIST	*apidl;
        LISTVIEW_SORT_INFO ListViewSortInfo;
	ULONG			hNotify;	/* change notification handle */
	HANDLE		hAccel;
	DWORD		dwAspects;
	DWORD		dwAdvf;
	IAdviseSink    *pAdvSink;
        IDropTarget*    pCurDropTarget; /* The sub-item, which is currently dragged over */
        IDataObject*    pCurDataObject; /* The dragged data-object */
        LONG            iDragOverItem;  /* Dragged over item's index, iff pCurDropTarget != NULL */
        UINT            cScrollDelay;   /* Send a WM_*SCROLL msg every 250 ms during drag-scroll */
        POINT           ptLastMousePos; /* Mouse position at last DragOver call */
} IShellViewImpl;

static const IShellViewVtbl svvt;
static const IOleCommandTargetVtbl ctvt;
static const IDropTargetVtbl dtvt;
static const IDropSourceVtbl dsvt;
static const IViewObjectVtbl vovt;


static inline IShellViewImpl *impl_from_IOleCommandTarget( IOleCommandTarget *iface )
{
    return (IShellViewImpl *)((char*)iface - FIELD_OFFSET(IShellViewImpl, lpvtblOleCommandTarget));
}

static inline IShellViewImpl *impl_from_IDropTarget( IDropTarget *iface )
{
    return (IShellViewImpl *)((char*)iface - FIELD_OFFSET(IShellViewImpl, lpvtblDropTarget));
}

static inline IShellViewImpl *impl_from_IDropSource( IDropSource *iface )
{
    return (IShellViewImpl *)((char*)iface - FIELD_OFFSET(IShellViewImpl, lpvtblDropSource));
}

static inline IShellViewImpl *impl_from_IViewObject( IViewObject *iface )
{
    return (IShellViewImpl *)((char*)iface - FIELD_OFFSET(IShellViewImpl, lpvtblViewObject));
}

/* ListView Header ID's */
#define LISTVIEW_COLUMN_NAME 0
#define LISTVIEW_COLUMN_SIZE 1
#define LISTVIEW_COLUMN_TYPE 2
#define LISTVIEW_COLUMN_TIME 3
#define LISTVIEW_COLUMN_ATTRIB 4

/*menu items */
#define IDM_VIEW_FILES  (FCIDM_SHVIEWFIRST + 0x500)
#define IDM_VIEW_IDW    (FCIDM_SHVIEWFIRST + 0x501)
#define IDM_MYFILEITEM  (FCIDM_SHVIEWFIRST + 0x502)

#define ID_LISTVIEW     1

#define SHV_CHANGE_NOTIFY WM_USER + 0x1111

/*windowsx.h */
#define GET_WM_COMMAND_ID(wp, lp)               LOWORD(wp)
#define GET_WM_COMMAND_HWND(wp, lp)             (HWND)(lp)
#define GET_WM_COMMAND_CMD(wp, lp)              HIWORD(wp)

/*
  Items merged into the toolbar and the filemenu
*/
typedef struct
{  int   idCommand;
   int   iImage;
   int   idButtonString;
   int   idMenuString;
   BYTE  bState;
   BYTE  bStyle;
} MYTOOLINFO, *LPMYTOOLINFO;

static const MYTOOLINFO Tools[] =
{
{ FCIDM_SHVIEW_BIGICON,    0, 0, IDS_VIEW_LARGE,   TBSTATE_ENABLED, BTNS_BUTTON },
{ FCIDM_SHVIEW_SMALLICON,  0, 0, IDS_VIEW_SMALL,   TBSTATE_ENABLED, BTNS_BUTTON },
{ FCIDM_SHVIEW_LISTVIEW,   0, 0, IDS_VIEW_LIST,    TBSTATE_ENABLED, BTNS_BUTTON },
{ FCIDM_SHVIEW_REPORTVIEW, 0, 0, IDS_VIEW_DETAILS, TBSTATE_ENABLED, BTNS_BUTTON },
{ -1, 0, 0, 0, 0, 0}
};

typedef void (CALLBACK *PFNSHGETSETTINGSPROC)(LPSHELLFLAGSTATE lpsfs, DWORD dwMask);

/**********************************************************
 *	IShellView_Constructor
 */
IShellView * IShellView_Constructor( IShellFolder * pFolder)
{	IShellViewImpl * sv;
	sv=HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IShellViewImpl));
	sv->ref=1;
	sv->lpVtbl=&svvt;
	sv->lpvtblOleCommandTarget=&ctvt;
	sv->lpvtblDropTarget=&dtvt;
	sv->lpvtblDropSource=&dsvt;
	sv->lpvtblViewObject=&vovt;

	sv->pSFParent = pFolder;
	if(pFolder) IShellFolder_AddRef(pFolder);
	IShellFolder_QueryInterface(sv->pSFParent, &IID_IShellFolder2, (LPVOID*)&sv->pSF2Parent);

        sv->pCurDropTarget = NULL;
        sv->pCurDataObject = NULL;
        sv->iDragOverItem = 0;
        sv->cScrollDelay = 0;
        sv->ptLastMousePos.x = 0;
        sv->ptLastMousePos.y = 0;

	TRACE("(%p)->(%p)\n",sv, pFolder);
	return (IShellView *) sv;
}

/**********************************************************
 *
 * ##### helperfunctions for communication with ICommDlgBrowser #####
 */
static BOOL IsInCommDlg(IShellViewImpl * This)
{	return(This->pCommDlgBrowser != NULL);
}

static HRESULT IncludeObject(IShellViewImpl * This, LPCITEMIDLIST pidl)
{
	HRESULT ret = S_OK;

	if ( IsInCommDlg(This) )
	{
	  TRACE("ICommDlgBrowser::IncludeObject pidl=%p\n", pidl);
	  ret = ICommDlgBrowser_IncludeObject(This->pCommDlgBrowser, (IShellView*)This, pidl);
	  TRACE("--0x%08x\n", ret);
	}
	return ret;
}

static HRESULT OnDefaultCommand(IShellViewImpl * This)
{
	HRESULT ret = S_FALSE;

	if (IsInCommDlg(This))
	{
	  TRACE("ICommDlgBrowser::OnDefaultCommand\n");
	  ret = ICommDlgBrowser_OnDefaultCommand(This->pCommDlgBrowser, (IShellView*)This);
	  TRACE("-- returns %08x\n", ret);
	}
	return ret;
}

static HRESULT OnStateChange(IShellViewImpl * This, UINT uFlags)
{
	HRESULT ret = S_FALSE;

	if (IsInCommDlg(This))
	{
	  TRACE("ICommDlgBrowser::OnStateChange flags=%x\n", uFlags);
	  ret = ICommDlgBrowser_OnStateChange(This->pCommDlgBrowser, (IShellView*)This, uFlags);
	  TRACE("--\n");
	}
	return ret;
}
/**********************************************************
 *	set the toolbar of the filedialog buttons
 *
 * - activates the buttons from the shellbrowser according to
 *   the view state
 */
static void CheckToolbar(IShellViewImpl * This)
{
	LRESULT result;

	TRACE("\n");

	if (IsInCommDlg(This))
	{
	  IShellBrowser_SendControlMsg(This->pShellBrowser, FCW_TOOLBAR, TB_CHECKBUTTON,
		FCIDM_TB_SMALLICON, (This->FolderSettings.ViewMode==FVM_LIST)? TRUE : FALSE, &result);
	  IShellBrowser_SendControlMsg(This->pShellBrowser, FCW_TOOLBAR, TB_CHECKBUTTON,
		FCIDM_TB_REPORTVIEW, (This->FolderSettings.ViewMode==FVM_DETAILS)? TRUE : FALSE, &result);
	  IShellBrowser_SendControlMsg(This->pShellBrowser, FCW_TOOLBAR, TB_ENABLEBUTTON,
		FCIDM_TB_SMALLICON, TRUE, &result);
	  IShellBrowser_SendControlMsg(This->pShellBrowser, FCW_TOOLBAR, TB_ENABLEBUTTON,
		FCIDM_TB_REPORTVIEW, TRUE, &result);
	}
}

/**********************************************************
 *
 * ##### helperfunctions for initializing the view #####
 */
/**********************************************************
 *	change the style of the listview control
 */
static void SetStyle(IShellViewImpl * This, DWORD dwAdd, DWORD dwRemove)
{
	DWORD tmpstyle;

	TRACE("(%p)\n", This);

	tmpstyle = GetWindowLongW(This->hWndList, GWL_STYLE);
	SetWindowLongW(This->hWndList, GWL_STYLE, dwAdd | (tmpstyle & ~dwRemove));
}

/**********************************************************
* ShellView_CreateList()
*
* - creates the list view window
*/
static BOOL ShellView_CreateList (IShellViewImpl * This)
{	DWORD dwStyle, dwExStyle;

	TRACE("%p\n",This);

	dwStyle = WS_TABSTOP | WS_VISIBLE | WS_CHILDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		  LVS_SHAREIMAGELISTS | LVS_EDITLABELS | LVS_ALIGNLEFT | LVS_AUTOARRANGE;
        dwExStyle = WS_EX_CLIENTEDGE;

	switch (This->FolderSettings.ViewMode)
	{
	  case FVM_ICON:	dwStyle |= LVS_ICON;		break;
	  case FVM_DETAILS: 	dwStyle |= LVS_REPORT;		break;
	  case FVM_SMALLICON: 	dwStyle |= LVS_SMALLICON;	break;
	  case FVM_LIST: 	dwStyle |= LVS_LIST;		break;
	  default:		dwStyle |= LVS_LIST;		break;
	}

	if (This->FolderSettings.fFlags & FWF_AUTOARRANGE)	dwStyle |= LVS_AUTOARRANGE;
	if (This->FolderSettings.fFlags & FWF_DESKTOP)
	  This->FolderSettings.fFlags |= FWF_NOCLIENTEDGE | FWF_NOSCROLL;
	if (This->FolderSettings.fFlags & FWF_SINGLESEL)	dwStyle |= LVS_SINGLESEL;
	if (This->FolderSettings.fFlags & FWF_NOCLIENTEDGE)
	  dwExStyle &= ~WS_EX_CLIENTEDGE;

	This->hWndList=CreateWindowExW( dwExStyle,
					WC_LISTVIEWW,
					NULL,
					dwStyle,
					0,0,0,0,
					This->hWnd,
					(HMENU)ID_LISTVIEW,
					shell32_hInstance,
					NULL);

	if(!This->hWndList)
	  return FALSE;

        This->ListViewSortInfo.bIsAscending = TRUE;
        This->ListViewSortInfo.nHeaderID = -1;
        This->ListViewSortInfo.nLastHeaderID = -1;

       if (This->FolderSettings.fFlags & FWF_DESKTOP) {
         /*
          * FIXME: look at the registry value
          * HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced\ListviewShadow
          * and activate drop shadows if necessary
          */
         if (0)
           SendMessageW(This->hWndList, LVM_SETTEXTBKCOLOR, 0, CLR_NONE);
         else
           SendMessageW(This->hWndList, LVM_SETTEXTBKCOLOR, 0, GetSysColor(COLOR_DESKTOP));

         SendMessageW(This->hWndList, LVM_SETTEXTCOLOR, 0, RGB(255,255,255));
       }

        /*  UpdateShellSettings(); */
	return TRUE;
}

/**********************************************************
* ShellView_InitList()
*
* - adds all needed columns to the shellview
*/
static BOOL ShellView_InitList(IShellViewImpl * This)
{
	LVCOLUMNW	lvColumn;
	SHELLDETAILS	sd;
	int	i;
	WCHAR	szTemp[50];

	TRACE("%p\n",This);

	SendMessageW(This->hWndList, LVM_DELETEALLITEMS, 0, 0);

	lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
	lvColumn.pszText = szTemp;

	if (This->pSF2Parent)
	{
	  for (i=0; 1; i++)
	  {
	    if (!SUCCEEDED(IShellFolder2_GetDetailsOf(This->pSF2Parent, NULL, i, &sd)))
	      break;
	    lvColumn.fmt = sd.fmt;
	    lvColumn.cx = sd.cxChar*8; /* chars->pixel */
	    StrRetToStrNW( szTemp, 50, &sd.str, NULL);
	    SendMessageW(This->hWndList, LVM_INSERTCOLUMNW, i, (LPARAM) &lvColumn);
	  }
	}
	else
	{
	  FIXME("no SF2\n");
	}

	SendMessageW(This->hWndList, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)ShellSmallIconList);
	SendMessageW(This->hWndList, LVM_SETIMAGELIST, LVSIL_NORMAL, (LPARAM)ShellBigIconList);

	return TRUE;
}
/**********************************************************
* ShellView_CompareItems()
*
* NOTES
*  internal, CALLBACK for DSA_Sort
*/
static INT CALLBACK ShellView_CompareItems(LPVOID lParam1, LPVOID lParam2, LPARAM lpData)
{
	int ret;
	TRACE("pidl1=%p pidl2=%p lpsf=%p\n", lParam1, lParam2, (LPVOID) lpData);

	if(!lpData) return 0;

	ret =  (SHORT) SCODE_CODE(IShellFolder_CompareIDs((LPSHELLFOLDER)lpData, 0, (LPITEMIDLIST)lParam1, (LPITEMIDLIST)lParam2));
	TRACE("ret=%i\n",ret);
	return ret;
}

/*************************************************************************
 * ShellView_ListViewCompareItems
 *
 * Compare Function for the Listview (FileOpen Dialog)
 *
 * PARAMS
 *     lParam1       [I] the first ItemIdList to compare with
 *     lParam2       [I] the second ItemIdList to compare with
 *     lpData        [I] The column ID for the header Ctrl to process
 *
 * RETURNS
 *     A negative value if the first item should precede the second,
 *     a positive value if the first item should follow the second,
 *     or zero if the two items are equivalent
 *
 * NOTES
 *	FIXME: function does what ShellView_CompareItems is supposed to do.
 *	unify it and figure out how to use the undocumented first parameter
 *	of IShellFolder_CompareIDs to do the job this function does and
 *	move this code to IShellFolder.
 *	make LISTVIEW_SORT_INFO obsolete
 *	the way this function works is only usable if we had only
 *	filesystemfolders  (25/10/99 jsch)
 */
static INT CALLBACK ShellView_ListViewCompareItems(LPVOID lParam1, LPVOID lParam2, LPARAM lpData)
{
    INT nDiff=0;
    FILETIME fd1, fd2;
    char strName1[MAX_PATH], strName2[MAX_PATH];
    BOOL bIsFolder1, bIsFolder2,bIsBothFolder;
    LPITEMIDLIST pItemIdList1 = (LPITEMIDLIST) lParam1;
    LPITEMIDLIST pItemIdList2 = (LPITEMIDLIST) lParam2;
    LISTVIEW_SORT_INFO *pSortInfo = (LPLISTVIEW_SORT_INFO) lpData;


    bIsFolder1 = _ILIsFolder(pItemIdList1);
    bIsFolder2 = _ILIsFolder(pItemIdList2);
    bIsBothFolder = bIsFolder1 && bIsFolder2;

    /* When sorting between a File and a Folder, the Folder gets sorted first */
    if( (bIsFolder1 || bIsFolder2) && !bIsBothFolder)
    {
        nDiff = bIsFolder1 ? -1 : 1;
    }
    else
    {
        /* Sort by Time: Folders or Files can be sorted */

        if(pSortInfo->nHeaderID == LISTVIEW_COLUMN_TIME)
        {
            _ILGetFileDateTime(pItemIdList1, &fd1);
            _ILGetFileDateTime(pItemIdList2, &fd2);
            nDiff = CompareFileTime(&fd2, &fd1);
        }
        /* Sort by Attribute: Folder or Files can be sorted */
        else if(pSortInfo->nHeaderID == LISTVIEW_COLUMN_ATTRIB)
        {
            _ILGetFileAttributes(pItemIdList1, strName1, MAX_PATH);
            _ILGetFileAttributes(pItemIdList2, strName2, MAX_PATH);
            nDiff = lstrcmpiA(strName1, strName2);
        }
        /* Sort by FileName: Folder or Files can be sorted */
        else if(pSortInfo->nHeaderID == LISTVIEW_COLUMN_NAME || bIsBothFolder)
        {
            /* Sort by Text */
            _ILSimpleGetText(pItemIdList1, strName1, MAX_PATH);
            _ILSimpleGetText(pItemIdList2, strName2, MAX_PATH);
            nDiff = lstrcmpiA(strName1, strName2);
        }
        /* Sort by File Size, Only valid for Files */
        else if(pSortInfo->nHeaderID == LISTVIEW_COLUMN_SIZE)
        {
            nDiff = (INT)(_ILGetFileSize(pItemIdList1, NULL, 0) - _ILGetFileSize(pItemIdList2, NULL, 0));
        }
        /* Sort by File Type, Only valid for Files */
        else if(pSortInfo->nHeaderID == LISTVIEW_COLUMN_TYPE)
        {
            /* Sort by Type */
            _ILGetFileType(pItemIdList1, strName1, MAX_PATH);
            _ILGetFileType(pItemIdList2, strName2, MAX_PATH);
            nDiff = lstrcmpiA(strName1, strName2);
        }
    }
    /*  If the Date, FileSize, FileType, Attrib was the same, sort by FileName */

    if(nDiff == 0)
    {
        _ILSimpleGetText(pItemIdList1, strName1, MAX_PATH);
        _ILSimpleGetText(pItemIdList2, strName2, MAX_PATH);
        nDiff = lstrcmpiA(strName1, strName2);
    }

    if(!pSortInfo->bIsAscending)
    {
        nDiff = -nDiff;
    }

    return nDiff;

}

/**********************************************************
*  LV_FindItemByPidl()
*/
static int LV_FindItemByPidl(
	IShellViewImpl * This,
	LPCITEMIDLIST pidl)
{
	LVITEMW lvItem;
	lvItem.iSubItem = 0;
	lvItem.mask = LVIF_PARAM;
	for(lvItem.iItem = 0;
		SendMessageW(This->hWndList, LVM_GETITEMW, 0, (LPARAM) &lvItem);
		lvItem.iItem++)
	{
	  LPITEMIDLIST currentpidl = (LPITEMIDLIST) lvItem.lParam;
	  HRESULT hr = IShellFolder_CompareIDs(This->pSFParent, 0, pidl, currentpidl);
	  if(SUCCEEDED(hr) && !HRESULT_CODE(hr))
	  {
	    return lvItem.iItem;
	  }
  	}
	return -1;
}

/**********************************************************
* LV_AddItem()
*/
static BOOLEAN LV_AddItem(IShellViewImpl * This, LPCITEMIDLIST pidl)
{
	LVITEMW	lvItem;

	TRACE("(%p)(pidl=%p)\n", This, pidl);

	lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;	/*set the mask*/
	lvItem.iItem = ListView_GetItemCount(This->hWndList);	/*add the item to the end of the list*/
	lvItem.iSubItem = 0;
	lvItem.lParam = (LPARAM) ILClone(ILFindLastID(pidl));				/*set the item's data*/
	lvItem.pszText = LPSTR_TEXTCALLBACKW;			/*get text on a callback basis*/
	lvItem.iImage = I_IMAGECALLBACK;			/*get the image on a callback basis*/
	return (-1==ListView_InsertItemW(This->hWndList, &lvItem))? FALSE: TRUE;
}

/**********************************************************
* LV_DeleteItem()
*/
static BOOLEAN LV_DeleteItem(IShellViewImpl * This, LPCITEMIDLIST pidl)
{
	int nIndex;

	TRACE("(%p)(pidl=%p)\n", This, pidl);

	nIndex = LV_FindItemByPidl(This, ILFindLastID(pidl));
	return (-1==ListView_DeleteItem(This->hWndList, nIndex))? FALSE: TRUE;
}

/**********************************************************
* LV_RenameItem()
*/
static BOOLEAN LV_RenameItem(IShellViewImpl * This, LPCITEMIDLIST pidlOld, LPCITEMIDLIST pidlNew )
{
	int nItem;
	LVITEMW lvItem;

	TRACE("(%p)(pidlold=%p pidlnew=%p)\n", This, pidlOld, pidlNew);

	nItem = LV_FindItemByPidl(This, ILFindLastID(pidlOld));
	if ( -1 != nItem )
	{
	  lvItem.mask = LVIF_PARAM;		/* only the pidl */
	  lvItem.iItem = nItem;
	  SendMessageW(This->hWndList, LVM_GETITEMW, 0, (LPARAM) &lvItem);

	  SHFree((LPITEMIDLIST)lvItem.lParam);
	  lvItem.mask = LVIF_PARAM;
	  lvItem.iItem = nItem;
	  lvItem.lParam = (LPARAM) ILClone(ILFindLastID(pidlNew));	/* set the item's data */
	  SendMessageW(This->hWndList, LVM_SETITEMW, 0, (LPARAM) &lvItem);
	  SendMessageW(This->hWndList, LVM_UPDATE, nItem, 0);
	  return TRUE;					/* FIXME: better handling */
	}
	return FALSE;
}
/**********************************************************
* ShellView_FillList()
*
* - gets the objectlist from the shellfolder
* - sorts the list
* - fills the list into the view
*/

static INT CALLBACK fill_list( LPVOID ptr, LPVOID arg )
{
    LPITEMIDLIST pidl = ptr;
    IShellViewImpl *This = arg;
    /* in a commdlg This works as a filemask*/
    if ( IncludeObject(This, pidl)==S_OK ) LV_AddItem(This, pidl);
    SHFree(pidl);
    return TRUE;
}

static HRESULT ShellView_FillList(IShellViewImpl * This)
{
	LPENUMIDLIST	pEnumIDList;
	LPITEMIDLIST	pidl;
	DWORD		dwFetched;
	HRESULT		hRes;
	HDPA		hdpa;

	TRACE("%p\n",This);

	/* get the itemlist from the shfolder*/
	hRes = IShellFolder_EnumObjects(This->pSFParent,This->hWnd, SHCONTF_NONFOLDERS | SHCONTF_FOLDERS, &pEnumIDList);
	if (hRes != S_OK)
	{
	  if (hRes==S_FALSE)
	    return(NOERROR);
	  return(hRes);
	}

	/* create a pointer array */
	hdpa = DPA_Create(16);
	if (!hdpa)
	{
	  return(E_OUTOFMEMORY);
	}

	/* copy the items into the array*/
	while((S_OK == IEnumIDList_Next(pEnumIDList,1, &pidl, &dwFetched)) && dwFetched)
	{
	  if (DPA_InsertPtr(hdpa, 0x7fff, pidl) == -1)
	  {
	    SHFree(pidl);
	  }
	}

	/* sort the array */
	DPA_Sort(hdpa, ShellView_CompareItems, (LPARAM)This->pSFParent);

	/*turn the listview's redrawing off*/
	SendMessageA(This->hWndList, WM_SETREDRAW, FALSE, 0);

        DPA_DestroyCallback( hdpa, fill_list, This );

	/*turn the listview's redrawing back on and force it to draw*/
	SendMessageA(This->hWndList, WM_SETREDRAW, TRUE, 0);

	IEnumIDList_Release(pEnumIDList); /* destroy the list*/

	return S_OK;
}

/**********************************************************
*  ShellView_OnCreate()
*/
static LRESULT ShellView_OnCreate(IShellViewImpl * This)
{
	IDropTarget* pdt;
	SHChangeNotifyEntry ntreg;
	IPersistFolder2 * ppf2 = NULL;

	TRACE("%p\n",This);

	if(ShellView_CreateList(This))
	{
	  if(ShellView_InitList(This))
	  {
	    ShellView_FillList(This);
	  }
	}

        if (SUCCEEDED(IUnknown_QueryInterface((IUnknown*)&This->lpVtbl, &IID_IDropTarget, (LPVOID*)&pdt)))
	{
	    RegisterDragDrop(This->hWnd, pdt);
	    IDropTarget_Release(pdt);
	}

	/* register for receiving notifications */
	IShellFolder_QueryInterface(This->pSFParent, &IID_IPersistFolder2, (LPVOID*)&ppf2);
	if (ppf2)
	{
	  IPersistFolder2_GetCurFolder(ppf2, (LPITEMIDLIST*)&ntreg.pidl);
	  ntreg.fRecursive = TRUE;
	  This->hNotify = SHChangeNotifyRegister(This->hWnd, SHCNF_IDLIST, SHCNE_ALLEVENTS, SHV_CHANGE_NOTIFY, 1, &ntreg);
	  SHFree((LPITEMIDLIST)ntreg.pidl);
	  IPersistFolder2_Release(ppf2);
	}

	This->hAccel = LoadAcceleratorsA(shell32_hInstance, "shv_accel");

	return S_OK;
}

/**********************************************************
 *	#### Handling of the menus ####
 */

/**********************************************************
* ShellView_BuildFileMenu()
*/
static HMENU ShellView_BuildFileMenu(IShellViewImpl * This)
{	WCHAR	szText[MAX_PATH];
	MENUITEMINFOW	mii;
	int	nTools,i;
	HMENU	hSubMenu;

	TRACE("(%p)\n",This);

	hSubMenu = CreatePopupMenu();
	if(hSubMenu)
	{ /*get the number of items in our global array*/
	  for(nTools = 0; Tools[nTools].idCommand != -1; nTools++){}

	  /*add the menu items*/
	  for(i = 0; i < nTools; i++)
	  {
	    LoadStringW(shell32_hInstance, Tools[i].idMenuString, szText, MAX_PATH);

	    ZeroMemory(&mii, sizeof(mii));
	    mii.cbSize = sizeof(mii);
	    mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;

	    if(BTNS_SEP != Tools[i].bStyle) /* no separator*/
	    {
	      mii.fType = MFT_STRING;
	      mii.fState = MFS_ENABLED;
	      mii.dwTypeData = szText;
	      mii.wID = Tools[i].idCommand;
	    }
	    else
	    {
	      mii.fType = MFT_SEPARATOR;
	    }
	    /* tack This item onto the end of the menu */
	    InsertMenuItemW(hSubMenu, (UINT)-1, TRUE, &mii);
	  }
	}
	TRACE("-- return (menu=%p)\n",hSubMenu);
	return hSubMenu;
}
/**********************************************************
* ShellView_MergeFileMenu()
*/
static void ShellView_MergeFileMenu(IShellViewImpl * This, HMENU hSubMenu)
{	TRACE("(%p)->(submenu=%p) stub\n",This,hSubMenu);

	if(hSubMenu)
	{ /*insert This item at the beginning of the menu */
	  _InsertMenuItem(hSubMenu, 0, TRUE, 0, MFT_SEPARATOR, NULL, MFS_ENABLED);
	  _InsertMenuItem(hSubMenu, 0, TRUE, IDM_MYFILEITEM, MFT_STRING, "dummy45", MFS_ENABLED);

	}
	TRACE("--\n");
}

/**********************************************************
* ShellView_MergeViewMenu()
*/

static void ShellView_MergeViewMenu(IShellViewImpl * This, HMENU hSubMenu)
{
	TRACE("(%p)->(submenu=%p)\n",This,hSubMenu);

	if(hSubMenu)
	{ /*add a separator at the correct position in the menu*/
	  MENUITEMINFOA mii;
	  static char view[] = "View";

	  _InsertMenuItem(hSubMenu, FCIDM_MENU_VIEW_SEP_OPTIONS, FALSE, 0, MFT_SEPARATOR, NULL, MFS_ENABLED);

	  ZeroMemory(&mii, sizeof(mii));
	  mii.cbSize = sizeof(mii);
	  mii.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_DATA;
	  mii.fType = MFT_STRING;
	  mii.dwTypeData = view;
	  mii.hSubMenu = LoadMenuA(shell32_hInstance, "MENU_001");
	  InsertMenuItemA(hSubMenu, FCIDM_MENU_VIEW_SEP_OPTIONS, FALSE, &mii);
	}
}

/**********************************************************
*   ShellView_GetSelections()
*
* - fills the this->apidl list with the selected objects
*
* RETURNS
*  number of selected items
*/
static UINT ShellView_GetSelections(IShellViewImpl * This)
{
	LVITEMW	lvItem;
	UINT	i = 0;

	SHFree(This->apidl);

	This->cidl = ListView_GetSelectedCount(This->hWndList);
	This->apidl = (LPITEMIDLIST*)SHAlloc(This->cidl * sizeof(LPITEMIDLIST));

	TRACE("selected=%i\n", This->cidl);

	if(This->apidl)
	{
	  TRACE("-- Items selected =%u\n", This->cidl);

	  lvItem.mask = LVIF_STATE | LVIF_PARAM;
	  lvItem.stateMask = LVIS_SELECTED;
	  lvItem.iItem = 0;
	  lvItem.iSubItem = 0;

	  while(ListView_GetItemW(This->hWndList, &lvItem) && (i < This->cidl))
	  {
	    if(lvItem.state & LVIS_SELECTED)
	    {
	      This->apidl[i] = (LPITEMIDLIST)lvItem.lParam;
	      i++;
	      TRACE("-- selected Item found\n");
	    }
	    lvItem.iItem++;
	  }
	}
	return This->cidl;

}

/**********************************************************
 *	ShellView_OpenSelectedItems()
 */
static HRESULT ShellView_OpenSelectedItems(IShellViewImpl * This)
{
	static UINT CF_IDLIST = 0;
	HRESULT hr;
	IDataObject* selection;
	FORMATETC fetc;
	STGMEDIUM stgm;
	LPIDA pIDList;
	LPCITEMIDLIST parent_pidl;
	WCHAR parent_path[MAX_PATH];
	LPCWSTR parent_dir = NULL;
	SFGAOF attribs;
	int i;

	if (0 == ShellView_GetSelections(This))
	{
	  return S_OK;
	}
	hr = IShellFolder_GetUIObjectOf(This->pSFParent, This->hWnd, This->cidl,
	                                (LPCITEMIDLIST*)This->apidl, &IID_IDataObject,
	                                0, (LPVOID *)&selection);
	if (FAILED(hr))
	  return hr;

	if (0 == CF_IDLIST)
	{
	  CF_IDLIST = RegisterClipboardFormatA(CFSTR_SHELLIDLIST);
	}
	fetc.cfFormat = CF_IDLIST;
	fetc.ptd = NULL;
	fetc.dwAspect = DVASPECT_CONTENT;
	fetc.lindex = -1;
	fetc.tymed = TYMED_HGLOBAL;

	hr = IDataObject_QueryGetData(selection, &fetc);
	if (FAILED(hr))
	  return hr;

	hr = IDataObject_GetData(selection, &fetc, &stgm);
	if (FAILED(hr))
	  return hr;

	pIDList = GlobalLock(stgm.u.hGlobal);

	parent_pidl = (LPCITEMIDLIST) ((LPBYTE)pIDList+pIDList->aoffset[0]);
	hr = IShellFolder_GetAttributesOf(This->pSFParent, 1, &parent_pidl, &attribs);
	if (SUCCEEDED(hr) && (attribs & SFGAO_FILESYSTEM) &&
	    SHGetPathFromIDListW(parent_pidl, parent_path))
	{
	  parent_dir = parent_path;
	}

	for (i = pIDList->cidl; i > 0; --i)
	{
	  LPCITEMIDLIST pidl;

	  pidl = (LPCITEMIDLIST)((LPBYTE)pIDList+pIDList->aoffset[i]);

	  attribs = SFGAO_FOLDER;
	  hr = IShellFolder_GetAttributesOf(This->pSFParent, 1, &pidl, &attribs);

	  if (SUCCEEDED(hr) && ! (attribs & SFGAO_FOLDER))
	  {
	    SHELLEXECUTEINFOW shexinfo;

	    shexinfo.cbSize = sizeof(SHELLEXECUTEINFOW);
	    shexinfo.fMask = SEE_MASK_INVOKEIDLIST;	/* SEE_MASK_IDLIST is also possible. */
	    shexinfo.hwnd = NULL;
	    shexinfo.lpVerb = NULL;
	    shexinfo.lpFile = NULL;
	    shexinfo.lpParameters = NULL;
	    shexinfo.lpDirectory = parent_dir;
	    shexinfo.nShow = SW_NORMAL;
	    shexinfo.lpIDList = ILCombine(parent_pidl, pidl);

	    ShellExecuteExW(&shexinfo);    /* Discard error/success info */

	    ILFree((LPITEMIDLIST)shexinfo.lpIDList);
	  }
	}

	GlobalUnlock(stgm.u.hGlobal);
	ReleaseStgMedium(&stgm);

	IDataObject_Release(selection);

	return S_OK;
}

/**********************************************************
 *	ShellView_DoContextMenu()
 */
static void ShellView_DoContextMenu(IShellViewImpl * This, WORD x, WORD y, BOOL bDefault)
{	UINT	uCommand;
	DWORD	wFlags;
	HMENU	hMenu;
	BOOL	fExplore = FALSE;
	HWND	hwndTree = 0;
	LPCONTEXTMENU	pContextMenu = NULL;
	IContextMenu2 *pCM = NULL;
	CMINVOKECOMMANDINFO	cmi;

	TRACE("(%p)->(0x%08x 0x%08x 0x%08x) stub\n",This, x, y, bDefault);

	/* look, what's selected and create a context menu object of it*/
	if( ShellView_GetSelections(This) )
	{
	  IShellFolder_GetUIObjectOf( This->pSFParent, This->hWndParent, This->cidl, (LPCITEMIDLIST*)This->apidl,
					(REFIID)&IID_IContextMenu, NULL, (LPVOID *)&pContextMenu);

	  if(pContextMenu)
	  {
	    TRACE("-- pContextMenu\n");
	    hMenu = CreatePopupMenu();

	    if( hMenu )
	    {
	      /* See if we are in Explore or Open mode. If the browser's tree is present, we are in Explore mode.*/
	      if(SUCCEEDED(IShellBrowser_GetControlWindow(This->pShellBrowser,FCW_TREE, &hwndTree)) && hwndTree)
	      {
	        TRACE("-- explore mode\n");
	        fExplore = TRUE;
	      }

	      /* build the flags depending on what we can do with the selected item */
	      wFlags = CMF_NORMAL | (This->cidl != 1 ? 0 : CMF_CANRENAME) | (fExplore ? CMF_EXPLORE : 0);

	      /* let the ContextMenu merge its items in */
	      if (SUCCEEDED(IContextMenu_QueryContextMenu( pContextMenu, hMenu, 0, FCIDM_SHVIEWFIRST, FCIDM_SHVIEWLAST, wFlags )))
	      {
	        if (This->FolderSettings.fFlags & FWF_DESKTOP)
		  SetMenuDefaultItem(hMenu, FCIDM_SHVIEW_OPEN, MF_BYCOMMAND);

		if( bDefault )
		{
		  TRACE("-- get menu default command\n");
		  uCommand = GetMenuDefaultItem(hMenu, FALSE, GMDI_GOINTOPOPUPS);
		}
		else
		{
		  TRACE("-- track popup\n");
		  uCommand = TrackPopupMenu( hMenu,TPM_LEFTALIGN | TPM_RETURNCMD,x,y,0,This->hWnd,NULL);
		}

		if(uCommand > 0)
		{
		  TRACE("-- uCommand=%u\n", uCommand);
		  if (uCommand==FCIDM_SHVIEW_OPEN && IsInCommDlg(This))
		  {
		    TRACE("-- dlg: OnDefaultCommand\n");
		    if (OnDefaultCommand(This) != S_OK)
		    {
		      ShellView_OpenSelectedItems(This);
		    }
		  }
		  else
		  {
		    TRACE("-- explore -- invoke command\n");
		    ZeroMemory(&cmi, sizeof(cmi));
		    cmi.cbSize = sizeof(cmi);
		    cmi.hwnd = This->hWndParent; /* this window has to answer CWM_GETISHELLBROWSER */
		    cmi.lpVerb = (LPCSTR)MAKEINTRESOURCEA(uCommand);
		    IContextMenu_InvokeCommand(pContextMenu, &cmi);
		  }
		}
		DestroyMenu(hMenu);
	      }
	    }
	    if (pContextMenu)
	      IContextMenu_Release(pContextMenu);
	  }
	}
	else	/* background context menu */
	{
	  hMenu = CreatePopupMenu();

	  pCM = ISvBgCm_Constructor(This->pSFParent, FALSE);
	  IContextMenu2_QueryContextMenu(pCM, hMenu, 0, FCIDM_SHVIEWFIRST, FCIDM_SHVIEWLAST, 0);

	  uCommand = TrackPopupMenu( hMenu, TPM_LEFTALIGN | TPM_RETURNCMD,x,y,0,This->hWnd,NULL);
	  DestroyMenu(hMenu);

	  TRACE("-- (%p)->(uCommand=0x%08x )\n",This, uCommand);

	  ZeroMemory(&cmi, sizeof(cmi));
	  cmi.cbSize = sizeof(cmi);
	  cmi.lpVerb = (LPCSTR)MAKEINTRESOURCEA(uCommand);
	  cmi.hwnd = This->hWndParent;
	  IContextMenu2_InvokeCommand(pCM, &cmi);

	  IContextMenu2_Release(pCM);
	}
}

/**********************************************************
 *	##### message handling #####
 */

/**********************************************************
*  ShellView_OnSize()
*/
static LRESULT ShellView_OnSize(IShellViewImpl * This, WORD wWidth, WORD wHeight)
{
	TRACE("%p width=%u height=%u\n",This, wWidth,wHeight);

	/*resize the ListView to fit our window*/
	if(This->hWndList)
	{
	  MoveWindow(This->hWndList, 0, 0, wWidth, wHeight, TRUE);
	}

	return S_OK;
}
/**********************************************************
* ShellView_OnDeactivate()
*
* NOTES
*  internal
*/
static void ShellView_OnDeactivate(IShellViewImpl * This)
{
	TRACE("%p\n",This);

	if(This->uState != SVUIA_DEACTIVATE)
	{
	  if(This->hMenu)
	  {
	    IShellBrowser_SetMenuSB(This->pShellBrowser,0, 0, 0);
	    IShellBrowser_RemoveMenusSB(This->pShellBrowser,This->hMenu);
	    DestroyMenu(This->hMenu);
	    This->hMenu = 0;
	  }

	  This->uState = SVUIA_DEACTIVATE;
	}
}

/**********************************************************
* ShellView_OnActivate()
*/
static LRESULT ShellView_OnActivate(IShellViewImpl * This, UINT uState)
{	OLEMENUGROUPWIDTHS   omw = { {0, 0, 0, 0, 0, 0} };
	MENUITEMINFOA         mii;
	CHAR                szText[MAX_PATH];

	TRACE("%p uState=%x\n",This,uState);

	/*don't do anything if the state isn't really changing */
	if(This->uState == uState)
	{
	  return S_OK;
	}

	ShellView_OnDeactivate(This);

	/*only do This if we are active */
	if(uState != SVUIA_DEACTIVATE)
	{
	  /*merge the menus */
	  This->hMenu = CreateMenu();

	  if(This->hMenu)
	  {
	    IShellBrowser_InsertMenusSB(This->pShellBrowser, This->hMenu, &omw);
	    TRACE("-- after fnInsertMenusSB\n");

	    /*build the top level menu get the menu item's text*/
	    strcpy(szText,"dummy 31");

	    ZeroMemory(&mii, sizeof(mii));
	    mii.cbSize = sizeof(mii);
	    mii.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_STATE;
	    mii.fType = MFT_STRING;
	    mii.fState = MFS_ENABLED;
	    mii.dwTypeData = szText;
	    mii.hSubMenu = ShellView_BuildFileMenu(This);

	    /*insert our menu into the menu bar*/
	    if(mii.hSubMenu)
	    {
	      InsertMenuItemA(This->hMenu, FCIDM_MENU_HELP, FALSE, &mii);
	    }

	    /*get the view menu so we can merge with it*/
	    ZeroMemory(&mii, sizeof(mii));
	    mii.cbSize = sizeof(mii);
	    mii.fMask = MIIM_SUBMENU;

	    if(GetMenuItemInfoA(This->hMenu, FCIDM_MENU_VIEW, FALSE, &mii))
	    {
	      ShellView_MergeViewMenu(This, mii.hSubMenu);
	    }

	    /*add the items that should only be added if we have the focus*/
	    if(SVUIA_ACTIVATE_FOCUS == uState)
	    {
	      /*get the file menu so we can merge with it */
	      ZeroMemory(&mii, sizeof(mii));
	      mii.cbSize = sizeof(mii);
	      mii.fMask = MIIM_SUBMENU;

	      if(GetMenuItemInfoA(This->hMenu, FCIDM_MENU_FILE, FALSE, &mii))
	      {
	        ShellView_MergeFileMenu(This, mii.hSubMenu);
	      }
	    }
	    TRACE("-- before fnSetMenuSB\n");
	    IShellBrowser_SetMenuSB(This->pShellBrowser, This->hMenu, 0, This->hWnd);
	  }
	}
	This->uState = uState;
	TRACE("--\n");
	return S_OK;
}

/**********************************************************
*  ShellView_OnSetFocus()
*
*/
static LRESULT ShellView_OnSetFocus(IShellViewImpl * This)
{
	TRACE("%p\n",This);

	/* Tell the browser one of our windows has received the focus. This
	should always be done before merging menus (OnActivate merges the
	menus) if one of our windows has the focus.*/

	IShellBrowser_OnViewWindowActive(This->pShellBrowser,(IShellView*) This);
	ShellView_OnActivate(This, SVUIA_ACTIVATE_FOCUS);

	/* Set the focus to the listview */
	SetFocus(This->hWndList);

	/* Notify the ICommDlgBrowser interface */
	OnStateChange(This,CDBOSC_SETFOCUS);

	return 0;
}

/**********************************************************
* ShellView_OnKillFocus()
*/
static LRESULT ShellView_OnKillFocus(IShellViewImpl * This)
{
	TRACE("(%p) stub\n",This);

	ShellView_OnActivate(This, SVUIA_ACTIVATE_NOFOCUS);
	/* Notify the ICommDlgBrowser */
	OnStateChange(This,CDBOSC_KILLFOCUS);

	return 0;
}

/**********************************************************
* ShellView_OnCommand()
*
* NOTES
*	the CmdID's are the ones from the context menu
*/
static LRESULT ShellView_OnCommand(IShellViewImpl * This,DWORD dwCmdID, DWORD dwCmd, HWND hwndCmd)
{
	TRACE("(%p)->(0x%08x 0x%08x %p) stub\n",This, dwCmdID, dwCmd, hwndCmd);

	switch(dwCmdID)
	{
	  case FCIDM_SHVIEW_SMALLICON:
	    This->FolderSettings.ViewMode = FVM_SMALLICON;
	    SetStyle (This, LVS_SMALLICON, LVS_TYPEMASK);
	    CheckToolbar(This);
	    break;

	  case FCIDM_SHVIEW_BIGICON:
	    This->FolderSettings.ViewMode = FVM_ICON;
	    SetStyle (This, LVS_ICON, LVS_TYPEMASK);
	    CheckToolbar(This);
	    break;

	  case FCIDM_SHVIEW_LISTVIEW:
	    This->FolderSettings.ViewMode = FVM_LIST;
	    SetStyle (This, LVS_LIST, LVS_TYPEMASK);
	    CheckToolbar(This);
	    break;

	  case FCIDM_SHVIEW_REPORTVIEW:
	    This->FolderSettings.ViewMode = FVM_DETAILS;
	    SetStyle (This, LVS_REPORT, LVS_TYPEMASK);
	    CheckToolbar(This);
	    break;

	  /* the menu-ID's for sorting are 0x30... see shrec.rc */
	  case 0x30:
	  case 0x31:
	  case 0x32:
	  case 0x33:
	    This->ListViewSortInfo.nHeaderID = (LPARAM) (dwCmdID - 0x30);
	    This->ListViewSortInfo.bIsAscending = TRUE;
	    This->ListViewSortInfo.nLastHeaderID = This->ListViewSortInfo.nHeaderID;
	    SendMessageA(This->hWndList, LVM_SORTITEMS, (WPARAM) &This->ListViewSortInfo, (LPARAM)ShellView_ListViewCompareItems);
	    break;

	  default:
	    TRACE("-- COMMAND 0x%04x unhandled\n", dwCmdID);
	}
	return 0;
}

/**********************************************************
* ShellView_OnNotify()
*/

static LRESULT ShellView_OnNotify(IShellViewImpl * This, UINT CtlID, LPNMHDR lpnmh)
{	LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW)lpnmh;
	NMLVDISPINFOW *lpdi = (NMLVDISPINFOW *)lpnmh;
	LPITEMIDLIST pidl;

	TRACE("%p CtlID=%u lpnmh->code=%x\n",This,CtlID,lpnmh->code);

	switch(lpnmh->code)
	{
	  case NM_SETFOCUS:
	    TRACE("-- NM_SETFOCUS %p\n",This);
	    ShellView_OnSetFocus(This);
	    break;

	  case NM_KILLFOCUS:
	    TRACE("-- NM_KILLFOCUS %p\n",This);
	    ShellView_OnDeactivate(This);
	    /* Notify the ICommDlgBrowser interface */
	    OnStateChange(This,CDBOSC_KILLFOCUS);
	    break;

	  case NM_CUSTOMDRAW:
	    TRACE("-- NM_CUSTOMDRAW %p\n",This);
	    return CDRF_DODEFAULT;

	  case NM_RELEASEDCAPTURE:
	    TRACE("-- NM_RELEASEDCAPTURE %p\n",This);
	    break;

	  case NM_CLICK:
	    TRACE("-- NM_CLICK %p\n",This);
	    break;

	  case NM_RCLICK:
	    TRACE("-- NM_RCLICK %p\n",This);
	    break;	    

          case NM_DBLCLK:
            TRACE("-- NM_DBLCLK %p\n",This);
            if (OnDefaultCommand(This) != S_OK) ShellView_OpenSelectedItems(This);
            break;

          case NM_RETURN:
            TRACE("-- NM_DBLCLK %p\n",This);
            if (OnDefaultCommand(This) != S_OK) ShellView_OpenSelectedItems(This);
            break;

	  case HDN_ENDTRACKW:
	    TRACE("-- HDN_ENDTRACKW %p\n",This);
	    /*nColumn1 = ListView_GetColumnWidth(This->hWndList, 0);
	    nColumn2 = ListView_GetColumnWidth(This->hWndList, 1);*/
	    break;

	  case LVN_DELETEITEM:
	    TRACE("-- LVN_DELETEITEM %p\n",This);
	    SHFree((LPITEMIDLIST)lpnmlv->lParam);     /*delete the pidl because we made a copy of it*/
	    break;

	  case LVN_DELETEALLITEMS:
	    TRACE("-- LVN_DELETEALLITEMS %p\n",This);
	    return FALSE;

	  case LVN_INSERTITEM:
	    TRACE("-- LVN_INSERTITEM (STUB)%p\n",This);
	    break;

	  case LVN_ITEMACTIVATE:
	    TRACE("-- LVN_ITEMACTIVATE %p\n",This);
	    OnStateChange(This, CDBOSC_SELCHANGE);  /* the browser will get the IDataObject now */
	    break;

	  case LVN_COLUMNCLICK:
	    This->ListViewSortInfo.nHeaderID = lpnmlv->iSubItem;
	    if(This->ListViewSortInfo.nLastHeaderID == This->ListViewSortInfo.nHeaderID)
	    {
	      This->ListViewSortInfo.bIsAscending = !This->ListViewSortInfo.bIsAscending;
	    }
	    else
	    {
	      This->ListViewSortInfo.bIsAscending = TRUE;
	    }
	    This->ListViewSortInfo.nLastHeaderID = This->ListViewSortInfo.nHeaderID;

	    SendMessageW(lpnmlv->hdr.hwndFrom, LVM_SORTITEMS, (WPARAM) &This->ListViewSortInfo, (LPARAM)ShellView_ListViewCompareItems);
	    break;

	  case LVN_GETDISPINFOA:
          case LVN_GETDISPINFOW:
	    TRACE("-- LVN_GETDISPINFO %p\n",This);
	    pidl = (LPITEMIDLIST)lpdi->item.lParam;

	    if(lpdi->item.mask & LVIF_TEXT)	/* text requested */
	    {
	      if (This->pSF2Parent)
	      {
	        SHELLDETAILS sd;
	        IShellFolder2_GetDetailsOf(This->pSF2Parent, pidl, lpdi->item.iSubItem, &sd);
                if (lpnmh->code == LVN_GETDISPINFOA)
                {
                    /* shouldn't happen */
                    NMLVDISPINFOA *lpdiA = (NMLVDISPINFOA *)lpnmh;
                    StrRetToStrNA( lpdiA->item.pszText, lpdiA->item.cchTextMax, &sd.str, NULL);
                    TRACE("-- text=%s\n",lpdiA->item.pszText);
                }
                else /* LVN_GETDISPINFOW */
                {
                    StrRetToStrNW( lpdi->item.pszText, lpdi->item.cchTextMax, &sd.str, NULL);
                    TRACE("-- text=%s\n",debugstr_w(lpdi->item.pszText));
                }
	      }
	      else
	      {
	        FIXME("no SF2\n");
	      }
	    }
	    if(lpdi->item.mask & LVIF_IMAGE)	/* image requested */
	    {
	      lpdi->item.iImage = SHMapPIDLToSystemImageListIndex(This->pSFParent, pidl, 0);
	    }
	    break;

	  case LVN_ITEMCHANGED:
	    TRACE("-- LVN_ITEMCHANGED %p\n",This);
	    OnStateChange(This, CDBOSC_SELCHANGE);  /* the browser will get the IDataObject now */
	    break;

	  case LVN_BEGINDRAG:
	  case LVN_BEGINRDRAG:
	    TRACE("-- LVN_BEGINDRAG\n");

	    if (ShellView_GetSelections(This))
	    {
	      IDataObject * pda;
	      DWORD dwAttributes = SFGAO_CANLINK;
	      DWORD dwEffect = DROPEFFECT_COPY | DROPEFFECT_MOVE;

	      if (SUCCEEDED(IShellFolder_GetUIObjectOf(This->pSFParent, This->hWnd, This->cidl, (LPCITEMIDLIST*)This->apidl, &IID_IDataObject,0,(LPVOID *)&pda)))
	      {
	          IDropSource * pds = (IDropSource*)&(This->lpvtblDropSource);	/* own DropSource interface */

	  	  if (SUCCEEDED(IShellFolder_GetAttributesOf(This->pSFParent, This->cidl, (LPCITEMIDLIST*)This->apidl, &dwAttributes)))
		  {
		    if (dwAttributes & SFGAO_CANLINK)
		    {
		      dwEffect |= DROPEFFECT_LINK;
		    }
		  }

	          if (pds)
	          {
	            DWORD dwEffect;
		    DoDragDrop(pda, pds, dwEffect, &dwEffect);
		  }
	          IDataObject_Release(pda);
	      }
	    }
	    break;

	  case LVN_BEGINLABELEDITW:
	    {
	      DWORD dwAttr = SFGAO_CANRENAME;
	      pidl = (LPITEMIDLIST)lpdi->item.lParam;

	      TRACE("-- LVN_BEGINLABELEDITA %p\n",This);

	      IShellFolder_GetAttributesOf(This->pSFParent, 1, (LPCITEMIDLIST*)&pidl, &dwAttr);
	      if (SFGAO_CANRENAME & dwAttr)
	      {
	        return FALSE;
	      }
	      return TRUE;
	    }

	  case LVN_ENDLABELEDITW:
	    {
	      TRACE("-- LVN_ENDLABELEDITA %p\n",This);
	      if (lpdi->item.pszText)
	      {
	        HRESULT hr;
		LVITEMW lvItem;

		lvItem.iItem = lpdi->item.iItem;
		lvItem.iSubItem = 0;
		lvItem.mask = LVIF_PARAM;
		SendMessageW(This->hWndList, LVM_GETITEMW, 0, (LPARAM) &lvItem);

		pidl = (LPITEMIDLIST)lpdi->item.lParam;
	        hr = IShellFolder_SetNameOf(This->pSFParent, 0, pidl, lpdi->item.pszText, SHGDN_INFOLDER, &pidl);

		if(SUCCEEDED(hr) && pidl)
		{
	          lvItem.mask = LVIF_PARAM;
		  lvItem.lParam = (LPARAM)pidl;
		  SendMessageW(This->hWndList, LVM_SETITEMW, 0, (LPARAM) &lvItem);
		  return TRUE;
		}
	      }
	      return FALSE;
	    }

	  case LVN_KEYDOWN:
	    {
	    /*  MSG msg;
	      msg.hwnd = This->hWnd;
	      msg.message = WM_KEYDOWN;
	      msg.wParam = plvKeyDown->wVKey;
	      msg.lParam = 0;
	      msg.time = 0;
	      msg.pt = 0;*/

	      LPNMLVKEYDOWN plvKeyDown = (LPNMLVKEYDOWN) lpnmh;

              /* initiate a rename of the selected file or directory */
              if(plvKeyDown->wVKey == VK_F2)
              {
                /* see how many files are selected */
                int i = ListView_GetSelectedCount(This->hWndList);

                /* get selected item */
                if(i == 1)
                {
                  /* get selected item */
                  i = ListView_GetNextItem(This->hWndList, -1,
			LVNI_SELECTED);

                  SendMessageW(This->hWndList, LVM_ENSUREVISIBLE, i, 0);
                  SendMessageW(This->hWndList, LVM_EDITLABELW, i, 0);
                }
              }
#if 0
	      TranslateAccelerator(This->hWnd, This->hAccel, &msg)
#endif
	      else if(plvKeyDown->wVKey == VK_DELETE)
              {
		UINT i;
		int item_index;
		LVITEMA item;
		LPITEMIDLIST* pItems;
		ISFHelper *psfhlp;

		IShellFolder_QueryInterface(This->pSFParent, &IID_ISFHelper,
		 	(LPVOID*)&psfhlp);

		if (psfhlp == NULL)
		  break;

		if(!(i = ListView_GetSelectedCount(This->hWndList)))
		  break;

		/* allocate memory for the pidl array */
		pItems = HeapAlloc(GetProcessHeap(), 0,
			sizeof(LPITEMIDLIST) * i);

		/* retrieve all selected items */
		i = 0;
		item_index = -1;
		while(ListView_GetSelectedCount(This->hWndList) > i)
		{
		  /* get selected item */
		  item_index = ListView_GetNextItem(This->hWndList,
			item_index, LVNI_SELECTED);
		  item.iItem = item_index;
		  item.mask = LVIF_PARAM;
		  SendMessageA(This->hWndList, LVM_GETITEMA, 0, (LPARAM) &item);

		  /* get item pidl */
		  pItems[i] = (LPITEMIDLIST)item.lParam;

		  i++;
		}

		/* perform the item deletion */
		ISFHelper_DeleteItems(psfhlp, i, (LPCITEMIDLIST*)pItems);

		/* free pidl array memory */
		HeapFree(GetProcessHeap(), 0, pItems);
              }

              /* Initiate a refresh */
              else if(plvKeyDown->wVKey == VK_F5)
              {
		IShellView_Refresh((IShellView*)This);
              }

              else
		FIXME("LVN_KEYDOWN key=0x%08x\n",plvKeyDown->wVKey);
	    }
	    break;

	  default:
	    TRACE("-- %p WM_COMMAND %x unhandled\n", This, lpnmh->code);
	    break;
	}
	return 0;
}

/**********************************************************
* ShellView_OnChange()
*/

static LRESULT ShellView_OnChange(IShellViewImpl * This, LPITEMIDLIST * Pidls, LONG wEventId)
{

	TRACE("(%p)(%p,%p,0x%08x)\n", This, Pidls[0], Pidls[1], wEventId);
	switch(wEventId)
	{
	  case SHCNE_MKDIR:
	  case SHCNE_CREATE:
	    LV_AddItem(This, Pidls[0]);
	    break;
	  case SHCNE_RMDIR:
	  case SHCNE_DELETE:
	    LV_DeleteItem(This, Pidls[0]);
	    break;
	  case SHCNE_RENAMEFOLDER:
	  case SHCNE_RENAMEITEM:
	    LV_RenameItem(This, Pidls[0], Pidls[1]);
	    break;
	  case SHCNE_UPDATEITEM:
	    break;
	}
	return TRUE;
}
/**********************************************************
*  ShellView_WndProc
*/

static LRESULT CALLBACK ShellView_WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	IShellViewImpl * pThis = (IShellViewImpl*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
	LPCREATESTRUCTW lpcs;

	TRACE("(hwnd=%p msg=%x wparm=%lx lparm=%lx)\n",hWnd, uMessage, wParam, lParam);

	switch (uMessage)
	{
	  case WM_NCCREATE:
	    lpcs = (LPCREATESTRUCTW)lParam;
	    pThis = (IShellViewImpl*)(lpcs->lpCreateParams);
	    SetWindowLongPtrW(hWnd, GWLP_USERDATA, (ULONG_PTR)pThis);
	    pThis->hWnd = hWnd;        /*set the window handle*/
	    break;

	  case WM_SIZE:		return ShellView_OnSize(pThis,LOWORD(lParam), HIWORD(lParam));
	  case WM_SETFOCUS:	return ShellView_OnSetFocus(pThis);
	  case WM_KILLFOCUS:	return ShellView_OnKillFocus(pThis);
	  case WM_CREATE:	return ShellView_OnCreate(pThis);
	  case WM_ACTIVATE:	return ShellView_OnActivate(pThis, SVUIA_ACTIVATE_FOCUS);
	  case WM_NOTIFY:	return ShellView_OnNotify(pThis,(UINT)wParam, (LPNMHDR)lParam);
	  case WM_COMMAND:      return ShellView_OnCommand(pThis,
					GET_WM_COMMAND_ID(wParam, lParam),
					GET_WM_COMMAND_CMD(wParam, lParam),
					GET_WM_COMMAND_HWND(wParam, lParam));
	  case SHV_CHANGE_NOTIFY: return ShellView_OnChange(pThis, (LPITEMIDLIST*)wParam, (LONG)lParam);

	  case WM_CONTEXTMENU:  ShellView_DoContextMenu(pThis, LOWORD(lParam), HIWORD(lParam), FALSE);
	                        return 0;

	  case WM_SHOWWINDOW:	UpdateWindow(pThis->hWndList);
				break;

	  case WM_GETDLGCODE:   return SendMessageA(pThis->hWndList,uMessage,0,0);

	  case WM_DESTROY:	
	  			RevokeDragDrop(pThis->hWnd);
				SHChangeNotifyDeregister(pThis->hNotify);
	                        break;

	  case WM_ERASEBKGND:
	    if ((pThis->FolderSettings.fFlags & FWF_DESKTOP) ||
	        (pThis->FolderSettings.fFlags & FWF_TRANSPARENT))
	      return 1;
	    break;
	}

	return DefWindowProcW(hWnd, uMessage, wParam, lParam);
}
/**********************************************************
*
*
*  The INTERFACE of the IShellView object
*
*
**********************************************************
*  IShellView_QueryInterface
*/
static HRESULT WINAPI IShellView_fnQueryInterface(IShellView * iface,REFIID riid, LPVOID *ppvObj)
{
	IShellViewImpl *This = (IShellViewImpl *)iface;

	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,debugstr_guid(riid),ppvObj);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))
	{
	  *ppvObj = This;
	}
	else if(IsEqualIID(riid, &IID_IShellView))
	{
	  *ppvObj = (IShellView*)This;
	}
	else if(IsEqualIID(riid, &IID_IOleCommandTarget))
	{
	  *ppvObj = (IOleCommandTarget*)&(This->lpvtblOleCommandTarget);
	}
	else if(IsEqualIID(riid, &IID_IDropTarget))
	{
	  *ppvObj = (IDropTarget*)&(This->lpvtblDropTarget);
	}
	else if(IsEqualIID(riid, &IID_IDropSource))
	{
	  *ppvObj = (IDropSource*)&(This->lpvtblDropSource);
	}
	else if(IsEqualIID(riid, &IID_IViewObject))
	{
	  *ppvObj = (IViewObject*)&(This->lpvtblViewObject);
	}

	if(*ppvObj)
	{
	  IUnknown_AddRef( (IUnknown*)*ppvObj );
	  TRACE("-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE("-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}

/**********************************************************
*  IShellView_AddRef
*/
static ULONG WINAPI IShellView_fnAddRef(IShellView * iface)
{
	IShellViewImpl *This = (IShellViewImpl *)iface;
	ULONG refCount = InterlockedIncrement(&This->ref);

	TRACE("(%p)->(count=%u)\n", This, refCount - 1);

	return refCount;
}
/**********************************************************
*  IShellView_Release
*/
static ULONG WINAPI IShellView_fnRelease(IShellView * iface)
{
	IShellViewImpl *This = (IShellViewImpl *)iface;
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(count=%i)\n", This, refCount + 1);

	if (!refCount)
	{
	  TRACE(" destroying IShellView(%p)\n",This);

	  DestroyWindow(This->hWndList);

	  if(This->pSFParent)
	    IShellFolder_Release(This->pSFParent);

	  if(This->pSF2Parent)
	    IShellFolder2_Release(This->pSF2Parent);

	  SHFree(This->apidl);

	  if(This->pAdvSink)
	    IAdviseSink_Release(This->pAdvSink);

	  HeapFree(GetProcessHeap(),0,This);
	}
	return refCount;
}

/**********************************************************
*  ShellView_GetWindow
*/
static HRESULT WINAPI IShellView_fnGetWindow(IShellView * iface,HWND * phWnd)
{
	IShellViewImpl *This = (IShellViewImpl *)iface;

	TRACE("(%p)\n",This);

	*phWnd = This->hWnd;

	return S_OK;
}

static HRESULT WINAPI IShellView_fnContextSensitiveHelp(IShellView * iface,BOOL fEnterMode)
{
	IShellViewImpl *This = (IShellViewImpl *)iface;

	FIXME("(%p) stub\n",This);

	return E_NOTIMPL;
}

/**********************************************************
* IShellView_TranslateAccelerator
*
* FIXME:
*  use the accel functions
*/
static HRESULT WINAPI IShellView_fnTranslateAccelerator(IShellView * iface,LPMSG lpmsg)
{
#if 0
	IShellViewImpl *This = (IShellViewImpl *)iface;

	FIXME("(%p)->(%p: hwnd=%x msg=%x lp=%x wp=%x) stub\n",This,lpmsg, lpmsg->hwnd, lpmsg->message, lpmsg->lParam, lpmsg->wParam);
#endif

	if ((lpmsg->message>=WM_KEYFIRST) && (lpmsg->message>=WM_KEYLAST))
	{
	  TRACE("-- key=0x04%lx\n",lpmsg->wParam) ;
	}
	return S_FALSE; /* not handled */
}

static HRESULT WINAPI IShellView_fnEnableModeless(IShellView * iface,BOOL fEnable)
{
	IShellViewImpl *This = (IShellViewImpl *)iface;

	FIXME("(%p) stub\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IShellView_fnUIActivate(IShellView * iface,UINT uState)
{
	IShellViewImpl *This = (IShellViewImpl *)iface;

/*
	CHAR	szName[MAX_PATH];
*/
	LRESULT	lResult;
	int	nPartArray[1] = {-1};

	TRACE("(%p)->(state=%x) stub\n",This, uState);

	/*don't do anything if the state isn't really changing*/
	if(This->uState == uState)
	{
	  return S_OK;
	}

	/*OnActivate handles the menu merging and internal state*/
	ShellView_OnActivate(This, uState);

	/*only do This if we are active*/
	if(uState != SVUIA_DEACTIVATE)
	{

/*
	  GetFolderPath is not a method of IShellFolder
	  IShellFolder_GetFolderPath( This->pSFParent, szName, sizeof(szName) );
*/
	  /* set the number of parts */
	  IShellBrowser_SendControlMsg(This->pShellBrowser, FCW_STATUS, SB_SETPARTS, 1,
	  						(LPARAM)nPartArray, &lResult);

	  /* set the text for the parts */
/*
	  IShellBrowser_SendControlMsg(This->pShellBrowser, FCW_STATUS, SB_SETTEXTA,
							0, (LPARAM)szName, &lResult);
*/
	}

	return S_OK;
}

static HRESULT WINAPI IShellView_fnRefresh(IShellView * iface)
{
	IShellViewImpl *This = (IShellViewImpl *)iface;

	TRACE("(%p)\n",This);

	SendMessageW(This->hWndList, LVM_DELETEALLITEMS, 0, 0);
	ShellView_FillList(This);

	return S_OK;
}

static HRESULT WINAPI IShellView_fnCreateViewWindow(
	IShellView * iface,
	IShellView *lpPrevView,
	LPCFOLDERSETTINGS lpfs,
	IShellBrowser * psb,
	RECT * prcView,
	HWND  *phWnd)
{
	IShellViewImpl *This = (IShellViewImpl *)iface;

	WNDCLASSW wc;
	*phWnd = 0;


	TRACE("(%p)->(shlview=%p set=%p shlbrs=%p rec=%p hwnd=%p) incomplete\n",This, lpPrevView,lpfs, psb, prcView, phWnd);
	TRACE("-- vmode=%x flags=%x left=%i top=%i right=%i bottom=%i\n",lpfs->ViewMode, lpfs->fFlags ,prcView->left,prcView->top, prcView->right, prcView->bottom);

	/*set up the member variables*/
	This->pShellBrowser = psb;
	This->FolderSettings = *lpfs;

	/*get our parent window*/
	IShellBrowser_AddRef(This->pShellBrowser);
	IShellBrowser_GetWindow(This->pShellBrowser, &(This->hWndParent));

	/* try to get the ICommDlgBrowserInterface, adds a reference !!! */
	This->pCommDlgBrowser=NULL;
	if ( SUCCEEDED (IShellBrowser_QueryInterface( This->pShellBrowser,
			(REFIID)&IID_ICommDlgBrowser, (LPVOID*) &This->pCommDlgBrowser)))
	{
	  TRACE("-- CommDlgBrowser\n");
	}

	/*if our window class has not been registered, then do so*/
	if(!GetClassInfoW(shell32_hInstance, SV_CLASS_NAME, &wc))
	{
	  ZeroMemory(&wc, sizeof(wc));
	  wc.style		= CS_HREDRAW | CS_VREDRAW;
	  wc.lpfnWndProc	= ShellView_WndProc;
	  wc.cbClsExtra		= 0;
	  wc.cbWndExtra		= 0;
	  wc.hInstance		= shell32_hInstance;
	  wc.hIcon		= 0;
	  wc.hCursor		= LoadCursorW(0, (LPWSTR)IDC_ARROW);
	  wc.hbrBackground	= (HBRUSH) (COLOR_WINDOW + 1);
	  wc.lpszMenuName	= NULL;
	  wc.lpszClassName	= SV_CLASS_NAME;

	  if(!RegisterClassW(&wc))
	    return E_FAIL;
	}

	*phWnd = CreateWindowExW(0,
				SV_CLASS_NAME,
				NULL,
				WS_CHILD | WS_TABSTOP,
				prcView->left,
				prcView->top,
				prcView->right - prcView->left,
				prcView->bottom - prcView->top,
				This->hWndParent,
				0,
				shell32_hInstance,
				(LPVOID)This);

	CheckToolbar(This);

	if(!*phWnd) return E_FAIL;

	SetWindowPos(*phWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
	UpdateWindow(*phWnd);

	return S_OK;
}

static HRESULT WINAPI IShellView_fnDestroyViewWindow(IShellView * iface)
{
	IShellViewImpl *This = (IShellViewImpl *)iface;

	TRACE("(%p)\n",This);

	/*Make absolutely sure all our UI is cleaned up.*/
	IShellView_UIActivate((IShellView*)This, SVUIA_DEACTIVATE);

	if(This->hMenu)
	{
	  DestroyMenu(This->hMenu);
	}

	DestroyWindow(This->hWnd);
	if(This->pShellBrowser) IShellBrowser_Release(This->pShellBrowser);
	if(This->pCommDlgBrowser) ICommDlgBrowser_Release(This->pCommDlgBrowser);


	return S_OK;
}

static HRESULT WINAPI IShellView_fnGetCurrentInfo(IShellView * iface, LPFOLDERSETTINGS lpfs)
{
	IShellViewImpl *This = (IShellViewImpl *)iface;

	TRACE("(%p)->(%p) vmode=%x flags=%x\n",This, lpfs,
		This->FolderSettings.ViewMode, This->FolderSettings.fFlags);

	if (!lpfs) return E_INVALIDARG;

	*lpfs = This->FolderSettings;
	return NOERROR;
}

static HRESULT WINAPI IShellView_fnAddPropertySheetPages(IShellView * iface, DWORD dwReserved,LPFNADDPROPSHEETPAGE lpfn, LPARAM lparam)
{
	IShellViewImpl *This = (IShellViewImpl *)iface;

	FIXME("(%p) stub\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IShellView_fnSaveViewState(IShellView * iface)
{
	IShellViewImpl *This = (IShellViewImpl *)iface;

	FIXME("(%p) stub\n",This);

	return S_OK;
}

static HRESULT WINAPI IShellView_fnSelectItem(
	IShellView * iface,
	LPCITEMIDLIST pidl,
	UINT uFlags)
{
	IShellViewImpl *This = (IShellViewImpl *)iface;
	int i;

	TRACE("(%p)->(pidl=%p, 0x%08x) stub\n",This, pidl, uFlags);

	i = LV_FindItemByPidl(This, pidl);

	if (i != -1)
	{
	  LVITEMW lvItem;

	  if(uFlags & SVSI_ENSUREVISIBLE)
	    SendMessageW(This->hWndList, LVM_ENSUREVISIBLE, i, 0);

	  lvItem.mask = LVIF_STATE;
	  lvItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
	  lvItem.iItem = 0;
	  lvItem.iSubItem = 0;

          while(SendMessageW(This->hWndList, LVM_GETITEMW, 0, (LPARAM) &lvItem))
	  {
	    if (lvItem.iItem == i)
	    {
	      if (uFlags & SVSI_SELECT)
	        lvItem.state |= LVIS_SELECTED;
	      else
		lvItem.state &= ~LVIS_SELECTED;

	      if(uFlags & SVSI_FOCUSED)
	        lvItem.state &= ~LVIS_FOCUSED;
	    }
	    else
	    {
	      if (uFlags & SVSI_DESELECTOTHERS)
	        lvItem.state &= ~LVIS_SELECTED;
	    }
	    SendMessageW(This->hWndList, LVM_SETITEMW, 0, (LPARAM) &lvItem);
	    lvItem.iItem++;
	  }


	  if(uFlags & SVSI_EDIT)
	    SendMessageW(This->hWndList, LVM_EDITLABELW, i, 0);

	}
	return S_OK;
}

static HRESULT WINAPI IShellView_fnGetItemObject(IShellView * iface, UINT uItem, REFIID riid, LPVOID *ppvOut)
{
	IShellViewImpl *This = (IShellViewImpl *)iface;

	TRACE("(%p)->(uItem=0x%08x,\n\tIID=%s, ppv=%p)\n",This, uItem, debugstr_guid(riid), ppvOut);

	*ppvOut = NULL;

	switch(uItem)
	{
	  case SVGIO_BACKGROUND:
	    *ppvOut = ISvBgCm_Constructor(This->pSFParent, FALSE);
	    break;

	  case SVGIO_SELECTION:
	    ShellView_GetSelections(This);
	    IShellFolder_GetUIObjectOf(This->pSFParent, This->hWnd, This->cidl, (LPCITEMIDLIST*)This->apidl, riid, 0, ppvOut);
	    break;
	}
	TRACE("-- (%p)->(interface=%p)\n",This, *ppvOut);

	if(!*ppvOut) return E_OUTOFMEMORY;

	return S_OK;
}

static const IShellViewVtbl svvt =
{
	IShellView_fnQueryInterface,
	IShellView_fnAddRef,
	IShellView_fnRelease,
	IShellView_fnGetWindow,
	IShellView_fnContextSensitiveHelp,
	IShellView_fnTranslateAccelerator,
	IShellView_fnEnableModeless,
	IShellView_fnUIActivate,
	IShellView_fnRefresh,
	IShellView_fnCreateViewWindow,
	IShellView_fnDestroyViewWindow,
	IShellView_fnGetCurrentInfo,
	IShellView_fnAddPropertySheetPages,
	IShellView_fnSaveViewState,
	IShellView_fnSelectItem,
	IShellView_fnGetItemObject
};


/**********************************************************
 * ISVOleCmdTarget_QueryInterface (IUnknown)
 */
static HRESULT WINAPI ISVOleCmdTarget_QueryInterface(
	IOleCommandTarget *	iface,
	REFIID			iid,
	LPVOID*			ppvObj)
{
	IShellViewImpl *This = impl_from_IOleCommandTarget(iface);

	return IShellFolder_QueryInterface((IShellFolder*)This, iid, ppvObj);
}

/**********************************************************
 * ISVOleCmdTarget_AddRef (IUnknown)
 */
static ULONG WINAPI ISVOleCmdTarget_AddRef(
	IOleCommandTarget *	iface)
{
	IShellViewImpl *This = impl_from_IOleCommandTarget(iface);

	return IShellFolder_AddRef((IShellFolder*)This);
}

/**********************************************************
 * ISVOleCmdTarget_Release (IUnknown)
 */
static ULONG WINAPI ISVOleCmdTarget_Release(
	IOleCommandTarget *	iface)
{
	IShellViewImpl *This = impl_from_IOleCommandTarget(iface);

	return IShellFolder_Release((IShellFolder*)This);
}

/**********************************************************
 * ISVOleCmdTarget_QueryStatus (IOleCommandTarget)
 */
static HRESULT WINAPI ISVOleCmdTarget_QueryStatus(
	IOleCommandTarget *iface,
	const GUID* pguidCmdGroup,
	ULONG cCmds,
	OLECMD * prgCmds,
	OLECMDTEXT* pCmdText)
{
    UINT i;
    IShellViewImpl *This = impl_from_IOleCommandTarget(iface);

    FIXME("(%p)->(%p(%s) 0x%08x %p %p\n",
              This, pguidCmdGroup, debugstr_guid(pguidCmdGroup), cCmds, prgCmds, pCmdText);

    if (!prgCmds)
        return E_POINTER;
    for (i = 0; i < cCmds; i++)
    {
        FIXME("\tprgCmds[%d].cmdID = %d\n", i, prgCmds[i].cmdID);
        prgCmds[i].cmdf = 0;
    }
    return OLECMDERR_E_UNKNOWNGROUP;
}

/**********************************************************
 * ISVOleCmdTarget_Exec (IOleCommandTarget)
 *
 * nCmdID is the OLECMDID_* enumeration
 */
static HRESULT WINAPI ISVOleCmdTarget_Exec(
	IOleCommandTarget *iface,
	const GUID* pguidCmdGroup,
	DWORD nCmdID,
	DWORD nCmdexecopt,
	VARIANT* pvaIn,
	VARIANT* pvaOut)
{
	IShellViewImpl *This = impl_from_IOleCommandTarget(iface);

	FIXME("(%p)->(\n\tTarget GUID:%s Command:0x%08x Opt:0x%08x %p %p)\n",
              This, debugstr_guid(pguidCmdGroup), nCmdID, nCmdexecopt, pvaIn, pvaOut);

	if (IsEqualIID(pguidCmdGroup, &CGID_Explorer) &&
	   (nCmdID == 0x29) &&
	   (nCmdexecopt == 4) && pvaOut)
	   return S_OK;
	if (IsEqualIID(pguidCmdGroup, &CGID_ShellDocView) &&
	   (nCmdID == 9) &&
	   (nCmdexecopt == 0))
	   return 1;

	return OLECMDERR_E_UNKNOWNGROUP;
}

static const IOleCommandTargetVtbl ctvt =
{
	ISVOleCmdTarget_QueryInterface,
	ISVOleCmdTarget_AddRef,
	ISVOleCmdTarget_Release,
	ISVOleCmdTarget_QueryStatus,
	ISVOleCmdTarget_Exec
};

/**********************************************************
 * ISVDropTarget implementation
 */

static HRESULT WINAPI ISVDropTarget_QueryInterface(
	IDropTarget *iface,
	REFIID riid,
	LPVOID *ppvObj)
{
	IShellViewImpl *This = impl_from_IDropTarget(iface);

	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,debugstr_guid(riid),ppvObj);

	return IShellFolder_QueryInterface((IShellFolder*)This, riid, ppvObj);
}

static ULONG WINAPI ISVDropTarget_AddRef( IDropTarget *iface)
{
	IShellViewImpl *This = impl_from_IDropTarget(iface);

	TRACE("(%p)->(count=%u)\n",This,This->ref);

	return IShellFolder_AddRef((IShellFolder*)This);
}

static ULONG WINAPI ISVDropTarget_Release( IDropTarget *iface)
{
	IShellViewImpl *This = impl_from_IDropTarget(iface);

	TRACE("(%p)->(count=%u)\n",This,This->ref);

	return IShellFolder_Release((IShellFolder*)This);
}

/******************************************************************************
 * drag_notify_subitem [Internal]
 *
 * Figure out the shellfolder object, which is currently under the mouse cursor
 * and notify it via the IDropTarget interface.
 */

#define SCROLLAREAWIDTH 20

static HRESULT drag_notify_subitem(IShellViewImpl *This, DWORD grfKeyState, POINTL pt,
    DWORD *pdwEffect)
{
    LVHITTESTINFO htinfo;
    LVITEMW lvItem;
    LONG lResult;
    HRESULT hr;
    RECT clientRect;

    /* Map from global to client coordinates and query the index of the listview-item, which is 
     * currently under the mouse cursor. */
    htinfo.pt.x = pt.x;
    htinfo.pt.y = pt.y;
    htinfo.flags = LVHT_ONITEM;
    ScreenToClient(This->hWndList, &htinfo.pt);
    lResult = SendMessageW(This->hWndList, LVM_HITTEST, 0, (LPARAM)&htinfo);

    /* Send WM_*SCROLL messages every 250 ms during drag-scrolling */
    GetClientRect(This->hWndList, &clientRect);
    if (htinfo.pt.x == This->ptLastMousePos.x && htinfo.pt.y == This->ptLastMousePos.y &&
        (htinfo.pt.x < SCROLLAREAWIDTH || htinfo.pt.x > clientRect.right - SCROLLAREAWIDTH ||
         htinfo.pt.y < SCROLLAREAWIDTH || htinfo.pt.y > clientRect.bottom - SCROLLAREAWIDTH ))
    {
        This->cScrollDelay = (This->cScrollDelay + 1) % 5; /* DragOver is called every 50 ms */
        if (This->cScrollDelay == 0) { /* Mouse did hover another 250 ms over the scroll-area */
            if (htinfo.pt.x < SCROLLAREAWIDTH) 
                SendMessageW(This->hWndList, WM_HSCROLL, SB_LINEUP, 0);
            if (htinfo.pt.x > clientRect.right - SCROLLAREAWIDTH)
                SendMessageW(This->hWndList, WM_HSCROLL, SB_LINEDOWN, 0);
            if (htinfo.pt.y < SCROLLAREAWIDTH)
                SendMessageW(This->hWndList, WM_VSCROLL, SB_LINEUP, 0);
            if (htinfo.pt.y > clientRect.bottom - SCROLLAREAWIDTH)
                SendMessageW(This->hWndList, WM_VSCROLL, SB_LINEDOWN, 0);
        }
    } else {
        This->cScrollDelay = 0; /* Reset, if the cursor is not over the listview's scroll-area */
    }
    This->ptLastMousePos = htinfo.pt;
 
    /* If we are still over the previous sub-item, notify it via DragOver and return. */
    if (This->pCurDropTarget && lResult == This->iDragOverItem)
    return IDropTarget_DragOver(This->pCurDropTarget, grfKeyState, pt, pdwEffect);
  
    /* We've left the previous sub-item, notify it via DragLeave and Release it. */
    if (This->pCurDropTarget) {
        IDropTarget_DragLeave(This->pCurDropTarget);
        IDropTarget_Release(This->pCurDropTarget);
        This->pCurDropTarget = NULL;
    }

    This->iDragOverItem = lResult;
    if (lResult == -1) {
        /* We are not above one of the listview's subitems. Bind to the parent folder's
         * DropTarget interface. */
        hr = IShellFolder_QueryInterface(This->pSFParent, &IID_IDropTarget, 
                                         (LPVOID*)&This->pCurDropTarget);
    } else {
        /* Query the relative PIDL of the shellfolder object represented by the currently
         * dragged over listview-item ... */
        lvItem.mask = LVIF_PARAM;
        lvItem.iItem = lResult;
        lvItem.iSubItem = 0;
        SendMessageW(This->hWndList, LVM_GETITEMW, 0, (LPARAM) &lvItem);

        /* ... and bind pCurDropTarget to the IDropTarget interface of an UIObject of this object */
        hr = IShellFolder_GetUIObjectOf(This->pSFParent, This->hWndList, 1,
            (LPCITEMIDLIST*)&lvItem.lParam, &IID_IDropTarget, NULL, (LPVOID*)&This->pCurDropTarget);
    }

    /* If anything failed, pCurDropTarget should be NULL now, which ought to be a save state. */
    if (FAILED(hr)) 
        return hr;

    /* Notify the item just entered via DragEnter. */
    return IDropTarget_DragEnter(This->pCurDropTarget, This->pCurDataObject, grfKeyState, pt, pdwEffect);
}

static HRESULT WINAPI ISVDropTarget_DragEnter(IDropTarget *iface, IDataObject *pDataObject,
    DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    IShellViewImpl *This = impl_from_IDropTarget(iface);

    /* Get a hold on the data object for later calls to DragEnter on the sub-folders */
    This->pCurDataObject = pDataObject;
    IDataObject_AddRef(pDataObject);

    return drag_notify_subitem(This, grfKeyState, pt, pdwEffect);
}

static HRESULT WINAPI ISVDropTarget_DragOver(IDropTarget *iface, DWORD grfKeyState, POINTL pt,
    DWORD *pdwEffect)
{
    IShellViewImpl *This = impl_from_IDropTarget(iface);
    return drag_notify_subitem(This, grfKeyState, pt, pdwEffect);
}

static HRESULT WINAPI ISVDropTarget_DragLeave(IDropTarget *iface) {
    IShellViewImpl *This = impl_from_IDropTarget(iface);

    IDropTarget_DragLeave(This->pCurDropTarget);

    IDropTarget_Release(This->pCurDropTarget);
    IDataObject_Release(This->pCurDataObject);
    This->pCurDataObject = NULL;
    This->pCurDropTarget = NULL;
    This->iDragOverItem = 0;
     
    return S_OK;
}

static HRESULT WINAPI ISVDropTarget_Drop(IDropTarget *iface, IDataObject* pDataObject, 
    DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    IShellViewImpl *This = impl_from_IDropTarget(iface);

    IDropTarget_Drop(This->pCurDropTarget, pDataObject, grfKeyState, pt, pdwEffect);

    IDropTarget_Release(This->pCurDropTarget);
    IDataObject_Release(This->pCurDataObject);
    This->pCurDataObject = NULL;
    This->pCurDropTarget = NULL;
    This->iDragOverItem = 0;

    return S_OK;
}

static const IDropTargetVtbl dtvt =
{
	ISVDropTarget_QueryInterface,
	ISVDropTarget_AddRef,
	ISVDropTarget_Release,
	ISVDropTarget_DragEnter,
	ISVDropTarget_DragOver,
	ISVDropTarget_DragLeave,
	ISVDropTarget_Drop
};

/**********************************************************
 * ISVDropSource implementation
 */

static HRESULT WINAPI ISVDropSource_QueryInterface(
	IDropSource *iface,
	REFIID riid,
	LPVOID *ppvObj)
{
	IShellViewImpl *This = impl_from_IDropSource(iface);

	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,debugstr_guid(riid),ppvObj);

	return IShellFolder_QueryInterface((IShellFolder*)This, riid, ppvObj);
}

static ULONG WINAPI ISVDropSource_AddRef( IDropSource *iface)
{
	IShellViewImpl *This = impl_from_IDropSource(iface);

	TRACE("(%p)->(count=%u)\n",This,This->ref);

	return IShellFolder_AddRef((IShellFolder*)This);
}

static ULONG WINAPI ISVDropSource_Release( IDropSource *iface)
{
	IShellViewImpl *This = impl_from_IDropSource(iface);

	TRACE("(%p)->(count=%u)\n",This,This->ref);

	return IShellFolder_Release((IShellFolder*)This);
}
static HRESULT WINAPI ISVDropSource_QueryContinueDrag(
	IDropSource *iface,
	BOOL fEscapePressed,
	DWORD grfKeyState)
{
	IShellViewImpl *This = impl_from_IDropSource(iface);
	TRACE("(%p)\n",This);

	if (fEscapePressed)
	  return DRAGDROP_S_CANCEL;
	else if (!(grfKeyState & MK_LBUTTON) && !(grfKeyState & MK_RBUTTON))
	  return DRAGDROP_S_DROP;
	else
	  return NOERROR;
}

static HRESULT WINAPI ISVDropSource_GiveFeedback(
	IDropSource *iface,
	DWORD dwEffect)
{
	IShellViewImpl *This = impl_from_IDropSource(iface);
	TRACE("(%p)\n",This);

	return DRAGDROP_S_USEDEFAULTCURSORS;
}

static const IDropSourceVtbl dsvt =
{
	ISVDropSource_QueryInterface,
	ISVDropSource_AddRef,
	ISVDropSource_Release,
	ISVDropSource_QueryContinueDrag,
	ISVDropSource_GiveFeedback
};
/**********************************************************
 * ISVViewObject implementation
 */

static HRESULT WINAPI ISVViewObject_QueryInterface(
	IViewObject *iface,
	REFIID riid,
	LPVOID *ppvObj)
{
	IShellViewImpl *This = impl_from_IViewObject(iface);

	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,debugstr_guid(riid),ppvObj);

	return IShellFolder_QueryInterface((IShellFolder*)This, riid, ppvObj);
}

static ULONG WINAPI ISVViewObject_AddRef( IViewObject *iface)
{
	IShellViewImpl *This = impl_from_IViewObject(iface);

	TRACE("(%p)->(count=%u)\n",This,This->ref);

	return IShellFolder_AddRef((IShellFolder*)This);
}

static ULONG WINAPI ISVViewObject_Release( IViewObject *iface)
{
	IShellViewImpl *This = impl_from_IViewObject(iface);

	TRACE("(%p)->(count=%u)\n",This,This->ref);

	return IShellFolder_Release((IShellFolder*)This);
}

static HRESULT WINAPI ISVViewObject_Draw(
	IViewObject 	*iface,
	DWORD dwDrawAspect,
	LONG lindex,
	void* pvAspect,
	DVTARGETDEVICE* ptd,
	HDC hdcTargetDev,
	HDC hdcDraw,
	LPCRECTL lprcBounds,
	LPCRECTL lprcWBounds,
	BOOL (CALLBACK *pfnContinue)(ULONG_PTR dwContinue),
	ULONG_PTR dwContinue)
{

	IShellViewImpl *This = impl_from_IViewObject(iface);

	FIXME("Stub: This=%p\n",This);

	return E_NOTIMPL;
}
static HRESULT WINAPI ISVViewObject_GetColorSet(
	IViewObject 	*iface,
	DWORD dwDrawAspect,
	LONG lindex,
	void *pvAspect,
	DVTARGETDEVICE* ptd,
	HDC hicTargetDevice,
	LOGPALETTE** ppColorSet)
{

	IShellViewImpl *This = impl_from_IViewObject(iface);

	FIXME("Stub: This=%p\n",This);

	return E_NOTIMPL;
}
static HRESULT WINAPI ISVViewObject_Freeze(
	IViewObject 	*iface,
	DWORD dwDrawAspect,
	LONG lindex,
	void* pvAspect,
	DWORD* pdwFreeze)
{

	IShellViewImpl *This = impl_from_IViewObject(iface);

	FIXME("Stub: This=%p\n",This);

	return E_NOTIMPL;
}
static HRESULT WINAPI ISVViewObject_Unfreeze(
	IViewObject 	*iface,
	DWORD dwFreeze)
{

	IShellViewImpl *This = impl_from_IViewObject(iface);

	FIXME("Stub: This=%p\n",This);

	return E_NOTIMPL;
}
static HRESULT WINAPI ISVViewObject_SetAdvise(
	IViewObject 	*iface,
	DWORD aspects,
	DWORD advf,
	IAdviseSink* pAdvSink)
{

	IShellViewImpl *This = impl_from_IViewObject(iface);

	FIXME("partial stub: %p %08x %08x %p\n",
              This, aspects, advf, pAdvSink);

	/* FIXME: we set the AdviseSink, but never use it to send any advice */
	This->pAdvSink = pAdvSink;
	This->dwAspects = aspects;
	This->dwAdvf = advf;

	return S_OK;
}

static HRESULT WINAPI ISVViewObject_GetAdvise(
	IViewObject 	*iface,
	DWORD* pAspects,
	DWORD* pAdvf,
	IAdviseSink** ppAdvSink)
{

	IShellViewImpl *This = impl_from_IViewObject(iface);

	TRACE("This=%p pAspects=%p pAdvf=%p ppAdvSink=%p\n",
              This, pAspects, pAdvf, ppAdvSink);

	if( ppAdvSink )
	{
		IAdviseSink_AddRef( This->pAdvSink );
		*ppAdvSink = This->pAdvSink;
	}
	if( pAspects )
		*pAspects = This->dwAspects;
	if( pAdvf )
		*pAdvf = This->dwAdvf;

	return S_OK;
}


static const IViewObjectVtbl vovt =
{
	ISVViewObject_QueryInterface,
	ISVViewObject_AddRef,
	ISVViewObject_Release,
	ISVViewObject_Draw,
	ISVViewObject_GetColorSet,
	ISVViewObject_Freeze,
	ISVViewObject_Unfreeze,
	ISVViewObject_SetAdvise,
	ISVViewObject_GetAdvise
};
