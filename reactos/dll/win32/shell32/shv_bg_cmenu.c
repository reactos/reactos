/*
 *	IContextMenu
 *	ShellView Background Context Menu (shv_bg_cm)
 *
 *	Copyright 1999	Juergen Schmied <juergen.schmied@metronet.de>
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
 */
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT
//#define YDEBUG
#include "wine/debug.h"

#include "windef.h"
#include "wingdi.h"
#include "pidl.h"
#include "shlobj.h"
#include "shtypes.h"
#include "shell32_main.h"
#include "shellfolder.h"
#include "undocshell.h"
#include "shlwapi.h"
#include "stdio.h"
#include "winuser.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/**************************************************************************
*  IContextMenu Implementation
*/
typedef struct
{
	const IContextMenu2Vtbl *lpVtbl;
	IShellFolder*	pSFParent;
	LONG		ref;
	BOOL		bDesktop;
    IContextMenu2 * icm_new;
} BgCmImpl;

static const IContextMenu2Vtbl cmvt;

BOOL
HasClipboardData()
{
    BOOL ret = FALSE;
    IDataObject * pda;

    if(SUCCEEDED(OleGetClipboard(&pda)))
    {
      STGMEDIUM medium;
      FORMATETC formatetc;

      TRACE("pda=%p\n", pda);

      /* Set the FORMATETC structure*/
      InitFormatEtc(formatetc, RegisterClipboardFormatA(CFSTR_SHELLIDLIST), TYMED_HGLOBAL);
      if(SUCCEEDED(IDataObject_GetData(pda,&formatetc,&medium)))
      {
          ret = TRUE;
      }

      IDataObject_Release(pda);
      ReleaseStgMedium(&medium);
    }

    return ret;
}


/**************************************************************************
*   ISVBgCm_Constructor()
*/
IContextMenu2 *ISvBgCm_Constructor(IShellFolder* pSFParent, BOOL bDesktop)
{
	BgCmImpl* cm;

	cm = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(BgCmImpl));
	cm->lpVtbl = &cmvt;
	cm->ref = 1;
	cm->pSFParent = pSFParent;
	cm->bDesktop = bDesktop;
	if(pSFParent) IShellFolder_AddRef(pSFParent);

	TRACE("(%p)->()\n",cm);
	return (IContextMenu2*)cm;
}

/**************************************************************************
*  ISVBgCm_fnQueryInterface
*/
static HRESULT WINAPI ISVBgCm_fnQueryInterface(IContextMenu2 *iface, REFIID riid, LPVOID *ppvObj)
{
	BgCmImpl *This = (BgCmImpl *)iface;

	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,debugstr_guid(riid),ppvObj);

	*ppvObj = NULL;

        if(IsEqualIID(riid, &IID_IUnknown) ||
           IsEqualIID(riid, &IID_IContextMenu) ||
           IsEqualIID(riid, &IID_IContextMenu2))
	{
	  *ppvObj = This;
	}
	else if(IsEqualIID(riid, &IID_IShellExtInit))  /*IShellExtInit*/
	{
	  FIXME("-- LPSHELLEXTINIT pointer requested\n");
	}

	if(*ppvObj)
	{
	  IUnknown_AddRef((IUnknown*)*ppvObj);
	  TRACE("-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE("-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}

/**************************************************************************
*  ISVBgCm_fnAddRef
*/
static ULONG WINAPI ISVBgCm_fnAddRef(IContextMenu2 *iface)
{
	BgCmImpl *This = (BgCmImpl *)iface;
	ULONG refCount = InterlockedIncrement(&This->ref);

	TRACE("(%p)->(count=%u)\n", This, refCount - 1);

	return refCount;
}

/**************************************************************************
*  ISVBgCm_fnRelease
*/
static ULONG WINAPI ISVBgCm_fnRelease(IContextMenu2 *iface)
{
	BgCmImpl *This = (BgCmImpl *)iface;
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(count=%i)\n", This, refCount + 1);

	if (!refCount)
	{
	  TRACE(" destroying IContextMenu(%p)\n",This);

	  if(This->pSFParent)
	    IShellFolder_Release(This->pSFParent);

	  HeapFree(GetProcessHeap(),0,This);
	}
	return refCount;
}

/**************************************************************************
* ISVBgCm_fnQueryContextMenu()
*/

static HRESULT WINAPI ISVBgCm_fnQueryContextMenu(
	IContextMenu2 *iface,
	HMENU hMenu,
	UINT indexMenu,
	UINT idCmdFirst,
	UINT idCmdLast,
	UINT uFlags)
{
    HMENU	hMyMenu;
    UINT	idMax;
    MENUITEMINFOW mii;
    HRESULT hr;

    IContextMenu2 * icm;
    BgCmImpl *This = (BgCmImpl *)iface;

    TRACE("(%p)->(hmenu=%p indexmenu=%x cmdfirst=%x cmdlast=%x flags=%x )\n",
          This, hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);


    hMyMenu = LoadMenuA(shell32_hInstance, "MENU_002");
    if (uFlags & CMF_DEFAULTONLY)
    {
        HMENU ourMenu = GetSubMenu(hMyMenu,0);
        UINT oldDef = GetMenuDefaultItem(hMenu,TRUE,GMDI_USEDISABLED);
        UINT newDef = GetMenuDefaultItem(ourMenu,TRUE,GMDI_USEDISABLED);
        if (newDef != oldDef)
            SetMenuDefaultItem(hMenu,newDef,TRUE);
        if (newDef!=0xFFFFFFFF)
            hr =  MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, newDef+1);
        else
            hr =  MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);
    }
    else
    {
        idMax = Shell_MergeMenus (hMenu, GetSubMenu(hMyMenu,0), indexMenu,
                                  idCmdFirst, idCmdLast, MM_SUBMENUSHAVEIDS);
        hr =  MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, idMax-idCmdFirst+1);
    }
    DestroyMenu(hMyMenu);

    if (!HasClipboardData())
    {
      mii.cbSize = sizeof(mii);
      mii.fMask = MIIM_STATE;
      mii.fState = MFS_DISABLED;
      mii.fType = 0;
      SetMenuItemInfoW(hMenu, FCIDM_SHVIEW_INSERT, FALSE, &mii);
      SetMenuItemInfoW(hMenu, FCIDM_SHVIEW_INSERTLINK, FALSE, &mii);
    }

    /*
     * FIXME
     * load other shell extensions
     */
    
    if (SUCCEEDED(INewItem_Constructor(This->pSFParent, &IID_IContextMenu2, (LPVOID*)&icm)))
    {
        if (SUCCEEDED(IContextMenu_QueryContextMenu(icm, hMenu, 10, idCmdFirst, idCmdLast, uFlags)))
        {
            This->icm_new = icm;
            _InsertMenuItem(hMenu, 11, TRUE, -1, MFT_SEPARATOR, NULL, MFS_ENABLED);
        }
        else
        {
            This->icm_new = NULL;
        }
    }

    if (This->bDesktop)
    {
      /* desktop menu has no view option */
      DeleteMenu(hMenu, 0, MF_BYPOSITION);
      DeleteMenu(hMenu, 0, MF_BYPOSITION);
    }
    TRACE("(%p)->returning 0x%x\n",This,hr);
    return hr;
}


/**************************************************************************
* DoPaste
*/
static BOOL DoPaste(
	IContextMenu2 *iface)
{
	BgCmImpl *This = (BgCmImpl *)iface;
	BOOL bSuccess = FALSE;
	IDataObject * pda;

	TRACE("\n");

	if(SUCCEEDED(OleGetClipboard(&pda)))
	{
	  STGMEDIUM medium;
	  FORMATETC formatetc;

	  TRACE("pda=%p\n", pda);

	  /* Set the FORMATETC structure*/
	  InitFormatEtc(formatetc, RegisterClipboardFormatA(CFSTR_SHELLIDLIST), TYMED_HGLOBAL);

	  /* Get the pidls from IDataObject */
	  if(SUCCEEDED(IDataObject_GetData(pda,&formatetc,&medium)))
          {
	    LPITEMIDLIST * apidl;
	    LPITEMIDLIST pidl;
	    IShellFolder *psfFrom = NULL, *psfDesktop;

	    LPIDA lpcida = GlobalLock(medium.u.hGlobal);
	    TRACE("cida=%p\n", lpcida);

	    apidl = _ILCopyCidaToaPidl(&pidl, lpcida);

	    /* bind to the source shellfolder */
	    SHGetDesktopFolder(&psfDesktop);
	    if(psfDesktop)
	    {
	      IShellFolder_BindToObject(psfDesktop, pidl, NULL, &IID_IShellFolder, (LPVOID*)&psfFrom);
	      IShellFolder_Release(psfDesktop);
	    }

	    if (psfFrom)
	    {
	      /* get source and destination shellfolder */
	      ISFHelper *psfhlpdst, *psfhlpsrc;
	      IShellFolder_QueryInterface(This->pSFParent, &IID_ISFHelper, (LPVOID*)&psfhlpdst);
	      IShellFolder_QueryInterface(psfFrom, &IID_ISFHelper, (LPVOID*)&psfhlpsrc);

	      /* do the copy/move */
	      if (psfhlpdst && psfhlpsrc)
	      {
	        ISFHelper_CopyItems(psfhlpdst, psfFrom, lpcida->cidl, (LPCITEMIDLIST*)apidl);
		/* FIXME handle move
		ISFHelper_DeleteItems(psfhlpsrc, lpcida->cidl, apidl);
		*/
	      }
	      if(psfhlpdst) ISFHelper_Release(psfhlpdst);
	      if(psfhlpsrc) ISFHelper_Release(psfhlpsrc);
	      IShellFolder_Release(psfFrom);
	    }

	    _ILFreeaPidl(apidl, lpcida->cidl);
	    SHFree(pidl);

	    /* release the medium*/
	    ReleaseStgMedium(&medium);
	  }
	  IDataObject_Release(pda);
	}
#if 0
	HGLOBAL  hMem;

	OpenClipboard(NULL);
	hMem = GetClipboardData(CF_HDROP);

	if(hMem)
	{
	  char * pDropFiles = (char *)GlobalLock(hMem);
	  if(pDropFiles)
	  {
	    int len, offset = sizeof(DROPFILESTRUCT);

	    while( pDropFiles[offset] != 0)
	    {
	      len = strlen(pDropFiles + offset);
	      TRACE("%s\n", pDropFiles + offset);
	      offset += len+1;
	    }
	  }
	  GlobalUnlock(hMem);
	}
	CloseClipboard();
#endif
	return bSuccess;
}

/**************************************************************************
* ISVBgCm_fnInvokeCommand()
*/
static HRESULT WINAPI ISVBgCm_fnInvokeCommand(
	IContextMenu2 *iface,
	LPCMINVOKECOMMANDINFO lpcmi)
{
	BgCmImpl *This = (BgCmImpl *)iface;

	LPSHELLBROWSER	lpSB;
	LPSHELLVIEW lpSV = NULL;
	HWND hWndSV = 0;

	TRACE("(%p)->(invcom=%p verb=%p wnd=%p)\n",This,lpcmi,lpcmi->lpVerb, lpcmi->hwnd);

	/* get the active IShellView */
	if((lpSB = (LPSHELLBROWSER)SendMessageA(lpcmi->hwnd, CWM_GETISHELLBROWSER,0,0)))
	{
	  if(SUCCEEDED(IShellBrowser_QueryActiveShellView(lpSB, &lpSV)))
	  {
	    IShellView_GetWindow(lpSV, &hWndSV);
	  }
	}

	  if(HIWORD(lpcmi->lpVerb))
	  {
	    TRACE("%s\n",lpcmi->lpVerb);

	    if (! strcmp(lpcmi->lpVerb,CMDSTR_VIEWLISTA))
	    {
	      if(hWndSV) SendMessageA(hWndSV, WM_COMMAND, MAKEWPARAM(FCIDM_SHVIEW_LISTVIEW,0),0 );
	    }
	    else if (! strcmp(lpcmi->lpVerb,CMDSTR_VIEWDETAILSA))
	    {
	      if(hWndSV) SendMessageA(hWndSV, WM_COMMAND, MAKEWPARAM(FCIDM_SHVIEW_REPORTVIEW,0),0 );
	    }
	    else
	    {
	      FIXME("please report: unknown verb %s\n",lpcmi->lpVerb);
	    }
	  }
	  else
	  {
	    switch(LOWORD(lpcmi->lpVerb))
	    {
	      case FCIDM_SHVIEW_REFRESH:
	        if (lpSV) IShellView_Refresh(lpSV);
	        break;
	      case FCIDM_SHVIEW_INSERT:
	        DoPaste(iface);
	        break;

	      case FCIDM_SHVIEW_PROPERTIES:
		if (This->bDesktop) {
		    ShellExecuteA(lpcmi->hwnd, "open", "rundll32.exe shell32.dll,Control_RunDLL desk.cpl", NULL, NULL, SW_SHOWNORMAL);
		} else {
		    FIXME("launch item properties dialog\n");
		}
		break;

	      default:

              if (This->icm_new && SUCCEEDED(IContextMenu2_InvokeCommand(This->icm_new, lpcmi)))
                  return S_OK;

	        /* if it's an id just pass it to the parent shv */
	        if (hWndSV) SendMessageA(hWndSV, WM_COMMAND, MAKEWPARAM(LOWORD(lpcmi->lpVerb), 0),0 );
		break;
	    }
	  }

        if (lpSV)
	  IShellView_Release(lpSV);	/* QueryActiveShellView does AddRef */

	return NOERROR;
}

/**************************************************************************
 *  ISVBgCm_fnGetCommandString()
 *
 */
static HRESULT WINAPI ISVBgCm_fnGetCommandString(
	IContextMenu2 *iface,
	UINT_PTR idCommand,
	UINT uFlags,
	UINT* lpReserved,
	LPSTR lpszName,
	UINT uMaxNameLen)
{
	BgCmImpl *This = (BgCmImpl *)iface;

	TRACE("(%p)->(idcom=%lx flags=%x %p name=%p len=%x)\n",This, idCommand, uFlags, lpReserved, lpszName, uMaxNameLen);

	/* test the existence of the menu items, the file dialog enables
	   the buttons according to this */
	if (uFlags == GCS_VALIDATEA)
	{
	  if(HIWORD(idCommand))
	  {
	    if (!strcmp((LPSTR)idCommand, CMDSTR_VIEWLISTA) ||
	        !strcmp((LPSTR)idCommand, CMDSTR_VIEWDETAILSA))
	    {
	      return NOERROR;
	    }
	  }
	}

	FIXME("unknown command string\n");
	return E_FAIL;
}

/**************************************************************************
* ISVBgCm_fnHandleMenuMsg()
*/
static HRESULT WINAPI ISVBgCm_fnHandleMenuMsg(
	IContextMenu2 *iface,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
    BgCmImpl *This = (BgCmImpl *)iface;

	TRACE("ISVBgCm_fnHandleMenuMsg (%p)->(msg=%x wp=%lx lp=%lx)\n",This, uMsg, wParam, lParam);

    if (This->icm_new)
    {
        IContextMenu2_HandleMenuMsg(This->icm_new, uMsg, wParam, lParam);
    }

	return E_NOTIMPL;
}

/**************************************************************************
* IContextMenu2 VTable
*
*/
static const IContextMenu2Vtbl cmvt =
{
	ISVBgCm_fnQueryInterface,
	ISVBgCm_fnAddRef,
	ISVBgCm_fnRelease,
	ISVBgCm_fnQueryContextMenu,
	ISVBgCm_fnInvokeCommand,
	ISVBgCm_fnGetCommandString,
	ISVBgCm_fnHandleMenuMsg
};
