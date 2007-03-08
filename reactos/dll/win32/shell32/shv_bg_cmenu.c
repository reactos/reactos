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

#define COBJMACROS
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

extern BOOL fileMoving;

/**************************************************************************
*  IContextMenu Implementation
*/
typedef struct
{
	const IContextMenu2Vtbl *lpVtbl;
	IShellFolder*	pSFParent;
	LONG		ref;
	BOOL		bDesktop;
} BgCmImpl;


static const IContextMenu2Vtbl cmvt;

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

	TRACE("(%p)->(count=%lu)\n", This, refCount - 1);

	return refCount;
}

/**************************************************************************
*  ISVBgCm_fnRelease
*/
static ULONG WINAPI ISVBgCm_fnRelease(IContextMenu2 *iface)
{
	BgCmImpl *This = (BgCmImpl *)iface;
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(count=%li)\n", This, refCount + 1);

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
    HRESULT hr;

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
	BgCmImpl *This = (BgCmImpl *)iface;
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

/***************************************************************************/
static BOOL DoLink(LPCSTR pSrcFile, LPCSTR pDstFile)
{
    IShellLinkA *psl = NULL;
    IPersistFile *pPf = NULL;
	HRESULT hres;
	WCHAR widelink[MAX_PATH];
    BOOL ret = FALSE;

	CoInitialize(0);

	hres = CoCreateInstance( &CLSID_ShellLink,
				 NULL,
				 CLSCTX_INPROC_SERVER,
				 &IID_IShellLinkA,
			 (LPVOID )&psl);

    if(SUCCEEDED(hres))
    {
	    hres = IShellLinkA_QueryInterface(psl, &IID_IPersistFile, (LPVOID *)&pPf);
	    if(FAILED(hres))
	    {
    		ERR("failed QueryInterface for IPersistFile %08lx\n", hres);
    		goto fail;
	    }

        DPRINT1("shortcut point to %s\n", pSrcFile);

		hres = IShellLinkA_SetPath(psl, pSrcFile);

	    if(FAILED(hres))
	    {
    		ERR("failed Set{IDList|Path} %08lx\n", hres);
    		goto fail;
        }

	    MultiByteToWideChar(CP_ACP, 0, pDstFile, -1, widelink, MAX_PATH);
	    
	    /* create the short cut */
	    hres = IPersistFile_Save(pPf, widelink, TRUE);
	    
	    if(FAILED(hres))
	    {
    		ERR("failed IPersistFile::Save %08lx\n", hres);
    		IPersistFile_Release(pPf);
    		IShellLinkA_Release(psl);
    		goto fail;
	    }

	    hres = IPersistFile_SaveCompleted(pPf, widelink);
	    IPersistFile_Release(pPf);
	    IShellLinkA_Release(psl);
	    DPRINT1("shortcut %s has been created, result=%08lx\n", pDstFile, hres);
		ret = TRUE;
	}
	else
	{
	    DPRINT1("CoCreateInstance failed, hres=%08lx\n", hres);
	}

 fail:
    CoUninitialize();
    return ret;
}

static BOOL MakeLink(IContextMenu2 *iface)
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
	  if(SUCCEEDED(IDataObject_GetData(pda, &formatetc, &medium)))
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
	      IPersistFolder2 *ppfdst = NULL;
	      IPersistFolder2 *ppfsrc = NULL;
	      IShellFolder_QueryInterface(This->pSFParent, &IID_IPersistFolder2, (LPVOID*)&ppfdst);
	      IShellFolder_QueryInterface(psfFrom, &IID_IPersistFolder2, (LPVOID*)&ppfsrc);

	      DPRINT1("[%p,%p]\n",ppfdst,ppfsrc);

	      /* do the link/s */
	      /* hack to get desktop path */
	      if ( (ppfdst && ppfsrc) || (This->bDesktop && ppfsrc) )
	      {
            int i;
            char szSrcPath[MAX_PATH];
            char szDstPath[MAX_PATH];
            BOOL ret = FALSE;
            LPITEMIDLIST pidl2;
            char filename[MAX_PATH];
            char linkFilename[MAX_PATH];
            char srcFilename[MAX_PATH];

            IPersistFolder2_GetCurFolder(ppfsrc, &pidl2);
            SHGetPathFromIDListA (pidl2, szSrcPath);

            if (This->bDesktop)
            {
                SHGetSpecialFolderLocation(0, CSIDL_DESKTOPDIRECTORY, &pidl2);
                SHGetPathFromIDListA (pidl2, szDstPath);
            }
            else
            {
                IPersistFolder2_GetCurFolder(ppfdst, &pidl2);
                SHGetPathFromIDListA (pidl2, szDstPath);
            }

	        for (i = 0; i < lpcida->cidl; i++)
	        {
                _ILSimpleGetText (apidl[i], filename, MAX_PATH);
                
                DPRINT1("filename %s\n", filename);

	            lstrcpyA(linkFilename, szDstPath);
	            PathAddBackslashA(linkFilename);
	            lstrcatA(linkFilename, "Shortcut to ");
                lstrcatA(linkFilename, filename);
                lstrcatA(linkFilename, ".lnk");
                
                DPRINT1("linkFilename %s\n", linkFilename);

                lstrcpyA(srcFilename, szSrcPath);
                PathAddBackslashA(srcFilename);
	            lstrcatA(srcFilename, filename);

	            DPRINT1("srcFilename %s\n", srcFilename);

	            ret = DoLink(srcFilename, linkFilename);

    	        if (ret)
    	        {
                    SHChangeNotify(SHCNE_CREATE, SHCNF_PATHA, linkFilename, NULL);
    	        }
	        }
	      }
	      if(ppfdst) IPersistFolder2_Release(ppfdst);
	      if(ppfsrc) IPersistFolder2_Release(ppfsrc);
	      IShellFolder_Release(psfFrom);
	    }

	    _ILFreeaPidl(apidl, lpcida->cidl);
	    SHFree(pidl);

	    /* release the medium*/
	    ReleaseStgMedium(&medium);
	  }
	  IDataObject_Release(pda);
	}
	return bSuccess;
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

	      if (This->bDesktop)
	      {
	        /* unimplemented
	        SHGetDesktopFolder(&psfDesktop);
	        IFSFolder_Constructor(psfDesktop, &IID_ISFHelper, (LPVOID*)&psfhlpdst);
	        IShellFolder_QueryInterface(psfhlpdst, &IID_ISFHelper, (LPVOID*)&psfhlpdst);
	        */
	        IShellFolder_QueryInterface(This->pSFParent, &IID_ISFHelper, (LPVOID*)&psfhlpdst);
	      }
	      else
	      {
	          IShellFolder_QueryInterface(This->pSFParent, &IID_ISFHelper, (LPVOID*)&psfhlpdst);
	      }
	      
	      IShellFolder_QueryInterface(psfFrom, &IID_ISFHelper, (LPVOID*)&psfhlpsrc);

	      /* do the copy/move */
	      if (psfhlpdst && psfhlpsrc)
	      {
	        ISFHelper_CopyItems(psfhlpdst, psfFrom, lpcida->cidl, (LPCITEMIDLIST*)apidl);
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
	      case FCIDM_SHVIEW_REFRESH:
	        if (lpSV) IShellView_Refresh(lpSV);
	        break;

	      case FCIDM_SHVIEW_NEWFOLDER:
	        DoNewFolder(iface, lpSV);
		break;

	      case FCIDM_SHVIEW_INSERT:
	        DoPaste(iface);
	        break;

	      case FCIDM_SHVIEW_INSERTLINK:
	        MakeLink(iface);
	        break;

	      case FCIDM_SHVIEW_PROPERTIES:
		if (This->bDesktop) {
		    ShellExecuteA(lpcmi->hwnd, "open", "rundll32.exe shell32.dll,Control_RunDLL desk.cpl", NULL, NULL, SW_SHOWNORMAL);
		} else {
		    FIXME("launch item properties dialog\n");
		}
		break;

	      default:
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
	BgCmImpl *This = (BgCmImpl *)iface;

	FIXME("(%p)->(msg=%x wp=%x lp=%lx)\n",This, uMsg, wParam, lParam);

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
