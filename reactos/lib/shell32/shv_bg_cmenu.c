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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "wine/debug.h"

#include "windef.h"
#include "wingdi.h"
#include "pidl.h"
#include "shlguid.h"
#include "shlobj.h"

#include "shell32_main.h"
#include "shellfolder.h"
#include "undocshell.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/**************************************************************************
*  IContextMenu Implementation
*/
typedef struct
{
	ICOM_VFIELD(IContextMenu2);
	IShellFolder*	pSFParent;
	DWORD		ref;
} BgCmImpl;


static struct ICOM_VTABLE(IContextMenu2) cmvt;

/**************************************************************************
*   ISVBgCm_Constructor()
*/
IContextMenu2 *ISvBgCm_Constructor(IShellFolder*	pSFParent)
{
	BgCmImpl* cm;

	cm = (BgCmImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(BgCmImpl));
	cm->lpVtbl = &cmvt;
	cm->ref = 1;
	cm->pSFParent = pSFParent;
	if(pSFParent) IShellFolder_AddRef(pSFParent);

	TRACE("(%p)->()\n",cm);
	return (IContextMenu2*)cm;
}

/**************************************************************************
*  ISVBgCm_fnQueryInterface
*/
static HRESULT WINAPI ISVBgCm_fnQueryInterface(IContextMenu2 *iface, REFIID riid, LPVOID *ppvObj)
{
	ICOM_THIS(BgCmImpl, iface);

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
	ICOM_THIS(BgCmImpl, iface);

	TRACE("(%p)->(count=%lu)\n",This, This->ref);

	return ++(This->ref);
}

/**************************************************************************
*  ISVBgCm_fnRelease
*/
static ULONG WINAPI ISVBgCm_fnRelease(IContextMenu2 *iface)
{
	ICOM_THIS(BgCmImpl, iface);

	TRACE("(%p)->()\n",This);

	if (!--(This->ref))
	{
	  TRACE(" destroying IContextMenu(%p)\n",This);

	  if(This->pSFParent)
	    IShellFolder_Release(This->pSFParent);

	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}

	return This->ref;
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
    HRESULT hr;

    ICOM_THIS(BgCmImpl, iface);

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

    TRACE("(%p)->returning 0x%lx\n",This,hr);
    return hr;
}

/**************************************************************************
* DoNewFolder
*/
static void DoNewFolder(
	IContextMenu2 *iface,
	IShellView *psv)
{
	ICOM_THIS(BgCmImpl, iface);
	ISFHelper * psfhlp;
	char szName[MAX_PATH];

	IShellFolder_QueryInterface(This->pSFParent, &IID_ISFHelper, (LPVOID*)&psfhlp);
	if (psfhlp)
	{
	  LPITEMIDLIST pidl;
	  ISFHelper_GetUniqueName(psfhlp, szName, MAX_PATH);
	  ISFHelper_AddFolder(psfhlp, 0, szName, &pidl);

	  if(psv)
	  {
	    /* if we are in a shellview do labeledit */
	    IShellView_SelectItem(psv,
                    pidl,(SVSI_DESELECTOTHERS | SVSI_EDIT | SVSI_ENSUREVISIBLE
                    |SVSI_FOCUSED|SVSI_SELECT));
	  }
	  SHFree(pidl);

	  ISFHelper_Release(psfhlp);
	}
}

/**************************************************************************
* DoPaste
*/
static BOOL DoPaste(
	IContextMenu2 *iface)
{
	ICOM_THIS(BgCmImpl, iface);
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
	ICOM_THIS(BgCmImpl, iface);

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

	    if (! strcmp(lpcmi->lpVerb,CMDSTR_NEWFOLDERA))
	    {
                DoNewFolder(iface, lpSV);
	    }
	    else if (! strcmp(lpcmi->lpVerb,CMDSTR_VIEWLISTA))
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
	      case FCIDM_SHVIEW_NEWFOLDER:
	        DoNewFolder(iface, lpSV);
		break;
	      case FCIDM_SHVIEW_INSERT:
	        DoPaste(iface);
	        break;
	      default:
	        /* if it's a id just pass it to the parent shv */
	        SendMessageA(hWndSV, WM_COMMAND, MAKEWPARAM(LOWORD(lpcmi->lpVerb), 0),0 );
		break;
	    }
	  }

        if (lpSV)
	  IShellView_Release(lpSV);	/* QueryActiveShellView does AddRef*/

	return NOERROR;
}

/**************************************************************************
 *  ISVBgCm_fnGetCommandString()
 *
 */
static HRESULT WINAPI ISVBgCm_fnGetCommandString(
	IContextMenu2 *iface,
	UINT idCommand,
	UINT uFlags,
	UINT* lpReserved,
	LPSTR lpszName,
	UINT uMaxNameLen)
{
	ICOM_THIS(BgCmImpl, iface);

	TRACE("(%p)->(idcom=%x flags=%x %p name=%p len=%x)\n",This, idCommand, uFlags, lpReserved, lpszName, uMaxNameLen);

	/* test the existence of the menu items, the file dialog enables
	   the buttons according to this */
	if (uFlags == GCS_VALIDATEA)
	{
	  if(HIWORD(idCommand))
	  {
	    if (!strcmp((LPSTR)idCommand, CMDSTR_VIEWLISTA) ||
	        !strcmp((LPSTR)idCommand, CMDSTR_VIEWDETAILSA) ||
	        !strcmp((LPSTR)idCommand, CMDSTR_NEWFOLDERA))
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
	ICOM_THIS(BgCmImpl, iface);

	FIXME("(%p)->(msg=%x wp=%x lp=%lx)\n",This, uMsg, wParam, lParam);

	return E_NOTIMPL;
}

/**************************************************************************
* IContextMenu2 VTable
*
*/
static struct ICOM_VTABLE(IContextMenu2) cmvt =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	ISVBgCm_fnQueryInterface,
	ISVBgCm_fnAddRef,
	ISVBgCm_fnRelease,
	ISVBgCm_fnQueryContextMenu,
	ISVBgCm_fnInvokeCommand,
	ISVBgCm_fnGetCommandString,
	ISVBgCm_fnHandleMenuMsg
};
