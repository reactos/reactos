/*
 *  Implementation of IShellBrowser for the File Open common dialog
 *
 * Copyright 1999 Francois Boisvert
 * Copyright 1999, 2000 Juergen Schmied
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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"

#define NO_SHLWAPI_STREAM
#include "shlwapi.h"
#include "filedlgbrowser.h"
#include "cdlg.h"
#include "shlguid.h"
#include "servprov.h"
#include "wine/debug.h"
#include "wine/heap.h"
#ifdef __REACTOS__
EXTERN_C HRESULT DoUpdateAutoCompleteWithCWD(const FileOpenDlgInfos *info, LPCITEMIDLIST pidl);
#endif

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);

typedef struct
{

    IShellBrowser IShellBrowser_iface;
    ICommDlgBrowser ICommDlgBrowser_iface;
    IServiceProvider IServiceProvider_iface;
    LONG ref;                                   /* Reference counter */
    HWND hwndOwner;                             /* Owner dialog of the interface */

} IShellBrowserImpl;

static inline IShellBrowserImpl *impl_from_IShellBrowser(IShellBrowser *iface)
{
    return CONTAINING_RECORD(iface, IShellBrowserImpl, IShellBrowser_iface);
}

static inline IShellBrowserImpl *impl_from_ICommDlgBrowser( ICommDlgBrowser *iface )
{
    return CONTAINING_RECORD(iface, IShellBrowserImpl, ICommDlgBrowser_iface);
}

static inline IShellBrowserImpl *impl_from_IServiceProvider( IServiceProvider *iface )
{
    return CONTAINING_RECORD(iface, IShellBrowserImpl, IServiceProvider_iface);
}

/**************************************************************************
*   vtable
*/
static const IShellBrowserVtbl IShellBrowserImpl_Vtbl;
static const ICommDlgBrowserVtbl IShellBrowserImpl_ICommDlgBrowser_Vtbl;
static const IServiceProviderVtbl IShellBrowserImpl_IServiceProvider_Vtbl;

/*
 *   Helper functions
 */

#define add_flag(a) if (flags & a) {strcat(str, #a );strcat(str," ");}
static void COMDLG32_DumpSBSPFlags(UINT uflags)
{
    if (TRACE_ON(commdlg))
    {
	unsigned int   i;
	static const struct {
	    DWORD       mask;
	    const char  *name;
	} flags[] = {
#define FE(x) { x, #x}
            /* SBSP_DEFBROWSER == 0 */
            FE(SBSP_SAMEBROWSER),
            FE(SBSP_NEWBROWSER),

            /* SBSP_DEFMODE == 0 */
            FE(SBSP_OPENMODE),
            FE(SBSP_EXPLOREMODE),
            FE(SBSP_HELPMODE),
            FE(SBSP_NOTRANSFERHIST),

            /* SBSP_ABSOLUTE == 0 */
            FE(SBSP_RELATIVE),
            FE(SBSP_PARENT),
            FE(SBSP_NAVIGATEBACK),
            FE(SBSP_NAVIGATEFORWARD),
            FE(SBSP_ALLOW_AUTONAVIGATE),

            FE(SBSP_NOAUTOSELECT),
            FE(SBSP_WRITENOHISTORY),

            FE(SBSP_REDIRECT),
            FE(SBSP_INITIATEDBYHLINKFRAME),
        };
#undef FE
        TRACE("SBSP Flags: %08x =", uflags);
	for (i = 0; i < ARRAY_SIZE(flags); i++)
	    if (flags[i].mask & uflags)
		TRACE("%s ", flags[i].name);
	TRACE("\n");
    }
}

static void COMDLG32_UpdateCurrentDir(const FileOpenDlgInfos *fodInfos)
{
    LPSHELLFOLDER psfDesktop;
    STRRET strret;
    HRESULT res;

    res = SHGetDesktopFolder(&psfDesktop);
    if (FAILED(res))
        return;
    
    res = IShellFolder_GetDisplayNameOf(psfDesktop, fodInfos->ShellInfos.pidlAbsCurrent,
                                        SHGDN_FORPARSING, &strret);
    if (SUCCEEDED(res)) {
        WCHAR wszCurrentDir[MAX_PATH];
        
        res = StrRetToBufW(&strret, fodInfos->ShellInfos.pidlAbsCurrent, wszCurrentDir, MAX_PATH);
        if (SUCCEEDED(res))
            SetCurrentDirectoryW(wszCurrentDir);
    }
#ifdef __REACTOS__
    DoUpdateAutoCompleteWithCWD(fodInfos, fodInfos->ShellInfos.pidlAbsCurrent);
#endif
    
    IShellFolder_Release(psfDesktop);
}

/* copied from shell32 to avoid linking to it */
static BOOL COMDLG32_StrRetToStrNW (LPVOID dest, DWORD len, LPSTRRET src, LPCITEMIDLIST pidl)
{
        TRACE("dest=%p len=0x%x strret=%p pidl=%p\n", dest , len, src, pidl);

	switch (src->uType)
	{
	  case STRRET_WSTR:
            lstrcpynW(dest, src->u.pOleStr, len);
	    CoTaskMemFree(src->u.pOleStr);
	    break;

	  case STRRET_CSTR:
            if (len && !MultiByteToWideChar( CP_ACP, 0, src->u.cStr, -1, dest, len ))
                ((LPWSTR)dest)[len-1] = 0;
	    break;

	  case STRRET_OFFSET:
	    if (pidl)
	    {
                if (len && !MultiByteToWideChar( CP_ACP, 0, ((LPCSTR)&pidl->mkid)+src->u.uOffset,
                                                 -1, dest, len ))
                    ((LPWSTR)dest)[len-1] = 0;
	    }
	    break;

	  default:
	    FIXME("unknown type!\n");
	    if (len)
	    { *(LPWSTR)dest = '\0';
	    }
	    return(FALSE);
	}
        return TRUE;
}

/*
 *	IShellBrowser
 */

/**************************************************************************
*  IShellBrowserImpl_Construct
*/
IShellBrowser * IShellBrowserImpl_Construct(HWND hwndOwner)
{
    FileOpenDlgInfos *fodInfos = get_filedlg_infoptr(hwndOwner);
    IShellBrowserImpl *sb;

    sb = heap_alloc(sizeof(*sb));

    /* Initialisation of the member variables */
    sb->ref=1;
    sb->hwndOwner = hwndOwner;

    /* Initialisation of the vTables */
    sb->IShellBrowser_iface.lpVtbl = &IShellBrowserImpl_Vtbl;
    sb->ICommDlgBrowser_iface.lpVtbl = &IShellBrowserImpl_ICommDlgBrowser_Vtbl;
    sb->IServiceProvider_iface.lpVtbl = &IShellBrowserImpl_IServiceProvider_Vtbl;
    SHGetSpecialFolderLocation(hwndOwner, CSIDL_DESKTOP,
                               &fodInfos->ShellInfos.pidlAbsCurrent);

    TRACE("%p\n", sb);

    return &sb->IShellBrowser_iface;
}

/***************************************************************************
*  IShellBrowserImpl_QueryInterface
*/
static HRESULT WINAPI IShellBrowserImpl_QueryInterface(IShellBrowser *iface, REFIID riid, void **ppvObj)
{
    IShellBrowserImpl *This = impl_from_IShellBrowser(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppvObj);

    *ppvObj = NULL;

    if(IsEqualIID(riid, &IID_IUnknown))
        *ppvObj = &This->IShellBrowser_iface;
    else if(IsEqualIID(riid, &IID_IOleWindow))
        *ppvObj = &This->IShellBrowser_iface;
    else if(IsEqualIID(riid, &IID_IShellBrowser))
        *ppvObj = &This->IShellBrowser_iface;
    else if(IsEqualIID(riid, &IID_ICommDlgBrowser))
        *ppvObj = &This->ICommDlgBrowser_iface;
    else if(IsEqualIID(riid, &IID_IServiceProvider))
        *ppvObj = &This->IServiceProvider_iface;

    if(*ppvObj) {
        IUnknown_AddRef((IUnknown*)*ppvObj);
        return S_OK;
    }

    FIXME("unsupported interface, %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

/**************************************************************************
*  IShellBrowser::AddRef
*/
static ULONG WINAPI IShellBrowserImpl_AddRef(IShellBrowser * iface)
{
    IShellBrowserImpl *This = impl_from_IShellBrowser(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p,%u)\n", This, ref - 1);

    return ref;
}

/**************************************************************************
*  IShellBrowserImpl_Release
*/
static ULONG WINAPI IShellBrowserImpl_Release(IShellBrowser * iface)
{
    IShellBrowserImpl *This = impl_from_IShellBrowser(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p,%u)\n", This, ref + 1);

    if (!ref)
        heap_free(This);

    return ref;
}

/*
 * IOleWindow
 */

/**************************************************************************
*  IShellBrowserImpl_GetWindow  (IOleWindow)
*
*  Inherited from IOleWindow::GetWindow
*
*  See Windows documentation for more details
*
*  Note : We will never be window less in the File Open dialog
*
*/
static HRESULT WINAPI IShellBrowserImpl_GetWindow(IShellBrowser * iface,
                                           HWND * phwnd)
{
    IShellBrowserImpl *This = impl_from_IShellBrowser(iface);

    TRACE("(%p)\n", This);

    if(!This->hwndOwner)
        return E_FAIL;

    *phwnd = This->hwndOwner;

    return (*phwnd) ? S_OK : E_UNEXPECTED;

}

/**************************************************************************
*  IShellBrowserImpl_ContextSensitiveHelp
*/
static HRESULT WINAPI IShellBrowserImpl_ContextSensitiveHelp(IShellBrowser * iface,
                                                      BOOL fEnterMode)
{
    IShellBrowserImpl *This = impl_from_IShellBrowser(iface);

    TRACE("(%p)\n", This);

    /* Feature not implemented */
    return E_NOTIMPL;
}

/*
 * IShellBrowser
 */

/**************************************************************************
*  IShellBrowserImpl_BrowseObject
*
*  See Windows documentation on IShellBrowser::BrowseObject for more details
*
*  This function will override user specified flags and will always
*  use SBSP_DEFBROWSER and SBSP_DEFMODE.
*/
static HRESULT WINAPI IShellBrowserImpl_BrowseObject(IShellBrowser *iface,
                                              LPCITEMIDLIST pidl,
                                              UINT wFlags)
{
    HRESULT hRes;
    IShellFolder *folder;
    IShellView *psvTmp;
    FileOpenDlgInfos *fodInfos;
    LPITEMIDLIST pidlTmp;
    HWND hwndView;
    HWND hDlgWnd;
    BOOL bViewHasFocus;
    RECT rectView;

    IShellBrowserImpl *This = impl_from_IShellBrowser(iface);

    TRACE("(%p)(pidl=%p,flags=0x%08x)\n", This, pidl, wFlags);
    COMDLG32_DumpSBSPFlags(wFlags);

    fodInfos = get_filedlg_infoptr(This->hwndOwner);

    /* Format the pidl according to its parameter's category */
    if(wFlags & SBSP_RELATIVE)
    {

        /* SBSP_RELATIVE  A relative pidl (relative from the current folder) */
        if (FAILED(hRes = IShellFolder_BindToObject(fodInfos->Shell.FOIShellFolder,
             pidl, NULL, &IID_IShellFolder, (void **)&folder)))
        {
            ERR("bind to object failed\n");
	    return hRes;
        }
        /* create an absolute pidl */
        pidlTmp = ILCombine(fodInfos->ShellInfos.pidlAbsCurrent, pidl);
    }
    else if(wFlags & SBSP_PARENT)
    {
        /* Browse the parent folder (ignores the pidl) */
        pidlTmp = GetParentPidl(fodInfos->ShellInfos.pidlAbsCurrent);
        folder = GetShellFolderFromPidl(pidlTmp);
    }
    else /* SBSP_ABSOLUTE is 0x0000 */
    {
        /* An absolute pidl (relative from the desktop) */
        pidlTmp = ILClone(pidl);
        folder = GetShellFolderFromPidl(pidlTmp);
    }

    if (!folder)
    {
        ERR("could not browse to folder\n");
        ILFree(pidlTmp);
        return E_FAIL;
    }

    /* If the pidl to browse to is equal to the actual pidl ...
       do nothing and pretend you did it*/
    if (ILIsEqual(pidlTmp, fodInfos->ShellInfos.pidlAbsCurrent))
    {
        IShellFolder_Release(folder);
        ILFree(pidlTmp);
        TRACE("keep current folder\n");
        return S_OK;
    }

    /* Release the current DataObject */
    if (fodInfos->Shell.FOIDataObject)
    {
      IDataObject_Release(fodInfos->Shell.FOIDataObject);
      fodInfos->Shell.FOIDataObject = NULL;
    }

    /* Create the associated view */
    TRACE("create view object\n");
    if (FAILED(hRes = IShellFolder_CreateViewObject(folder, fodInfos->ShellInfos.hwndOwner,
           &IID_IShellView, (void **)&psvTmp)))
    {
        IShellFolder_Release(folder);
        ILFree(pidlTmp);
        return hRes;
    }

    /* Check if listview has focus */
    bViewHasFocus = IsChild(fodInfos->ShellInfos.hwndView,GetFocus());

    /* Get the foldersettings from the old view */
    if(fodInfos->Shell.FOIShellView)
      IShellView_GetCurrentInfo(fodInfos->Shell.FOIShellView, &fodInfos->ShellInfos.folderSettings);

    /* Release the old fodInfos->Shell.FOIShellView and update its value.
    We have to update this early since ShellView_CreateViewWindow of native
    shell32 calls OnStateChange and needs the correct view here.*/
    if(fodInfos->Shell.FOIShellView)
    {
      IShellView_DestroyViewWindow(fodInfos->Shell.FOIShellView);
      IShellView_Release(fodInfos->Shell.FOIShellView);
    }
    fodInfos->Shell.FOIShellView = psvTmp;

    /* Release old FOIShellFolder and update its value */
    if (fodInfos->Shell.FOIShellFolder)
      IShellFolder_Release(fodInfos->Shell.FOIShellFolder);
    fodInfos->Shell.FOIShellFolder = folder;

    /* Release old pidlAbsCurrent and update its value */
    ILFree(fodInfos->ShellInfos.pidlAbsCurrent);
    fodInfos->ShellInfos.pidlAbsCurrent = pidlTmp;

    COMDLG32_UpdateCurrentDir(fodInfos);

    GetWindowRect(GetDlgItem(This->hwndOwner, IDC_SHELLSTATIC), &rectView);
    MapWindowPoints(0, This->hwndOwner, (LPPOINT)&rectView, 2);

    /* Create the window */
    TRACE("create view window\n");
    if (FAILED(hRes = IShellView_CreateViewWindow(psvTmp, NULL,
            &fodInfos->ShellInfos.folderSettings, fodInfos->Shell.FOIShellBrowser,
            &rectView, &hwndView)))
    {
        WARN("Failed to create view window, hr %#x.\n", hRes);
        return hRes;
    }

    fodInfos->ShellInfos.hwndView = hwndView;

    /* Set view window control id to 5002 */
    SetWindowLongPtrW(hwndView, GWLP_ID, lst2);
    SendMessageW( hwndView, WM_SETFONT, SendMessageW( GetParent(hwndView), WM_GETFONT, 0, 0 ), FALSE );

    /* Select the new folder in the Look In combo box of the Open file dialog */
    FILEDLG95_LOOKIN_SelectItem(fodInfos->DlgInfos.hwndLookInCB,fodInfos->ShellInfos.pidlAbsCurrent);

    /* changes the tab order of the ListView to reflect the window's File Dialog */
    hDlgWnd = GetDlgItem(GetParent(hwndView), IDC_LOOKIN);
    SetWindowPos(hwndView, hDlgWnd, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE);

    /* Since we destroyed the old view if it had focus set focus to the newly created view */
    if (bViewHasFocus)
      SetFocus(fodInfos->ShellInfos.hwndView);

    return hRes;
}

/**************************************************************************
*  IShellBrowserImpl_EnableModelessSB
*/
static HRESULT WINAPI IShellBrowserImpl_EnableModelessSB(IShellBrowser *iface,
                                              BOOL fEnable)

{
    IShellBrowserImpl *This = impl_from_IShellBrowser(iface);

    TRACE("(%p)\n", This);

    /* Feature not implemented */
    return E_NOTIMPL;
}

/**************************************************************************
*  IShellBrowserImpl_GetControlWindow
*/
static HRESULT WINAPI IShellBrowserImpl_GetControlWindow(IShellBrowser *iface,
                                              UINT id,
                                              HWND *lphwnd)

{
    IShellBrowserImpl *This = impl_from_IShellBrowser(iface);

    TRACE("(%p)\n", This);

    /* Feature not implemented */
    return E_NOTIMPL;
}

/**************************************************************************
*  IShellBrowserImpl_GetViewStateStream
*/
static HRESULT WINAPI IShellBrowserImpl_GetViewStateStream(IShellBrowser *iface,
                                                DWORD grfMode,
                                                LPSTREAM *ppStrm)

{
    IShellBrowserImpl *This = impl_from_IShellBrowser(iface);

    FIXME("(%p 0x%08x %p)\n", This, grfMode, ppStrm);

    /* Feature not implemented */
    return E_NOTIMPL;
}

/**************************************************************************
*  IShellBrowserImpl_InsertMenusSB
*/
static HRESULT WINAPI IShellBrowserImpl_InsertMenusSB(IShellBrowser *iface,
                                           HMENU hmenuShared,
                                           LPOLEMENUGROUPWIDTHS lpMenuWidths)

{
    IShellBrowserImpl *This = impl_from_IShellBrowser(iface);

    TRACE("(%p)\n", This);

    /* Feature not implemented */
    return E_NOTIMPL;
}

/**************************************************************************
*  IShellBrowserImpl_OnViewWindowActive
*/
static HRESULT WINAPI IShellBrowserImpl_OnViewWindowActive(IShellBrowser *iface,
                                                IShellView *ppshv)

{
    IShellBrowserImpl *This = impl_from_IShellBrowser(iface);

    TRACE("(%p)\n", This);

    /* Feature not implemented */
    return E_NOTIMPL;
}

/**************************************************************************
*  IShellBrowserImpl_QueryActiveShellView
*/
static HRESULT WINAPI IShellBrowserImpl_QueryActiveShellView(IShellBrowser *iface,
                                                  IShellView **ppshv)

{
    IShellBrowserImpl *This = impl_from_IShellBrowser(iface);

    FileOpenDlgInfos *fodInfos;

    TRACE("(%p)\n", This);

    fodInfos = get_filedlg_infoptr(This->hwndOwner);

    if(!(*ppshv = fodInfos->Shell.FOIShellView))
    {
        return E_FAIL;
    }
    IShellView_AddRef(fodInfos->Shell.FOIShellView);
    return NOERROR;
}

/**************************************************************************
*  IShellBrowserImpl_RemoveMenusSB
*/
static HRESULT WINAPI IShellBrowserImpl_RemoveMenusSB(IShellBrowser *iface,
                                           HMENU hmenuShared)

{
    IShellBrowserImpl *This = impl_from_IShellBrowser(iface);

    TRACE("(%p)\n", This);

    /* Feature not implemented */
    return E_NOTIMPL;
}

/**************************************************************************
*  IShellBrowserImpl_SendControlMsg
*/
static HRESULT WINAPI IShellBrowserImpl_SendControlMsg(IShellBrowser *iface,
                                            UINT id,
                                            UINT uMsg,
                                            WPARAM wParam,
                                            LPARAM lParam,
                                            LRESULT *pret)

{
    IShellBrowserImpl *This = impl_from_IShellBrowser(iface);
    LRESULT lres;

    TRACE("(%p)->(0x%08x 0x%08x 0x%08lx 0x%08lx %p)\n", This, id, uMsg, wParam, lParam, pret);

    switch (id)
    {
      case FCW_TOOLBAR:
        lres = SendDlgItemMessageA( This->hwndOwner, IDC_TOOLBAR, uMsg, wParam, lParam);
	break;
      default:
        FIXME("ctrl id: %x\n", id);
        return E_NOTIMPL;
    }
    if (pret) *pret = lres;
    return S_OK;
}

/**************************************************************************
*  IShellBrowserImpl_SetMenuSB
*/
static HRESULT WINAPI IShellBrowserImpl_SetMenuSB(IShellBrowser *iface,
                                       HMENU hmenuShared,
                                       HOLEMENU holemenuReserved,
                                       HWND hwndActiveObject)

{
    IShellBrowserImpl *This = impl_from_IShellBrowser(iface);

    TRACE("(%p)\n", This);

    /* Feature not implemented */
    return E_NOTIMPL;
}

/**************************************************************************
*  IShellBrowserImpl_SetStatusTextSB
*/
static HRESULT WINAPI IShellBrowserImpl_SetStatusTextSB(IShellBrowser *iface,
                                             LPCOLESTR lpszStatusText)

{
    IShellBrowserImpl *This = impl_from_IShellBrowser(iface);

    TRACE("(%p)\n", This);

    /* Feature not implemented */
    return E_NOTIMPL;
}

/**************************************************************************
*  IShellBrowserImpl_SetToolbarItems
*/
static HRESULT WINAPI IShellBrowserImpl_SetToolbarItems(IShellBrowser *iface,
                                             LPTBBUTTON lpButtons,
                                             UINT nButtons,
                                             UINT uFlags)

{
    IShellBrowserImpl *This = impl_from_IShellBrowser(iface);

    TRACE("(%p)\n", This);

    /* Feature not implemented */
    return E_NOTIMPL;
}

/**************************************************************************
*  IShellBrowserImpl_TranslateAcceleratorSB
*/
static HRESULT WINAPI IShellBrowserImpl_TranslateAcceleratorSB(IShellBrowser *iface,
                                                    LPMSG lpmsg,
                                                    WORD wID)

{
    IShellBrowserImpl *This = impl_from_IShellBrowser(iface);

    TRACE("(%p)\n", This);

    /* Feature not implemented */
    return E_NOTIMPL;
}

static const IShellBrowserVtbl IShellBrowserImpl_Vtbl =
{
        /* IUnknown */
        IShellBrowserImpl_QueryInterface,
        IShellBrowserImpl_AddRef,
        IShellBrowserImpl_Release,
        /* IOleWindow */
        IShellBrowserImpl_GetWindow,
        IShellBrowserImpl_ContextSensitiveHelp,
        /*  IShellBrowser */
        IShellBrowserImpl_InsertMenusSB,
        IShellBrowserImpl_SetMenuSB,
        IShellBrowserImpl_RemoveMenusSB,
        IShellBrowserImpl_SetStatusTextSB,
        IShellBrowserImpl_EnableModelessSB,
        IShellBrowserImpl_TranslateAcceleratorSB,
        IShellBrowserImpl_BrowseObject,
        IShellBrowserImpl_GetViewStateStream,
        IShellBrowserImpl_GetControlWindow,
        IShellBrowserImpl_SendControlMsg,
        IShellBrowserImpl_QueryActiveShellView,
        IShellBrowserImpl_OnViewWindowActive,
        IShellBrowserImpl_SetToolbarItems
};



/*
 * ICommDlgBrowser
 */

/***************************************************************************
*  IShellBrowserImpl_ICommDlgBrowser_QueryInterface
*/
static HRESULT WINAPI IShellBrowserImpl_ICommDlgBrowser_QueryInterface(
	ICommDlgBrowser *iface,
	REFIID riid,
	LPVOID *ppvObj)
{
    IShellBrowserImpl *This = impl_from_ICommDlgBrowser(iface);

    TRACE("(%p)\n", This);

    return IShellBrowserImpl_QueryInterface(&This->IShellBrowser_iface,riid,ppvObj);
}

/**************************************************************************
*  IShellBrowserImpl_ICommDlgBrowser_AddRef
*/
static ULONG WINAPI IShellBrowserImpl_ICommDlgBrowser_AddRef(ICommDlgBrowser * iface)
{
    IShellBrowserImpl *This = impl_from_ICommDlgBrowser(iface);

    TRACE("(%p)\n", This);

    return IShellBrowserImpl_AddRef(&This->IShellBrowser_iface);
}

/**************************************************************************
*  IShellBrowserImpl_ICommDlgBrowser_Release
*/
static ULONG WINAPI IShellBrowserImpl_ICommDlgBrowser_Release(ICommDlgBrowser * iface)
{
    IShellBrowserImpl *This = impl_from_ICommDlgBrowser(iface);

    TRACE("(%p)\n", This);

    return IShellBrowserImpl_Release(&This->IShellBrowser_iface);
}

/**************************************************************************
*  IShellBrowserImpl_ICommDlgBrowser_OnDefaultCommand
*
*   Called when a user double-clicks in the view or presses the ENTER key
*/
static HRESULT WINAPI IShellBrowserImpl_ICommDlgBrowser_OnDefaultCommand(ICommDlgBrowser *iface,
                                                                  IShellView *ppshv)
{
    LPITEMIDLIST pidl;
    FileOpenDlgInfos *fodInfos;

    IShellBrowserImpl *This = impl_from_ICommDlgBrowser(iface);

    TRACE("(%p)\n", This);

    fodInfos = get_filedlg_infoptr(This->hwndOwner);

    /* If the selected object is not a folder, send an IDOK command to parent window */
    if((pidl = GetPidlFromDataObject(fodInfos->Shell.FOIDataObject, 1)))
    {
        HRESULT hRes;

        ULONG  ulAttr = SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_FILESYSANCESTOR;
        IShellFolder_GetAttributesOf(fodInfos->Shell.FOIShellFolder, 1, (LPCITEMIDLIST *)&pidl, &ulAttr);
	if ((ulAttr & (SFGAO_FOLDER | SFGAO_HASSUBFOLDER)) && (ulAttr & SFGAO_FILESYSANCESTOR))
	{
            hRes = IShellBrowser_BrowseObject(&This->IShellBrowser_iface,pidl,SBSP_RELATIVE);
            if(fodInfos->ofnInfos->Flags & OFN_EXPLORER)
                SendCustomDlgNotificationMessage(This->hwndOwner, CDN_FOLDERCHANGE);
	}
        else
	{
          /* Tell the dialog that the user selected a file */
	  PostMessageA(This->hwndOwner, WM_COMMAND, IDOK, 0L);
         hRes = S_OK;
	}

        ILFree(pidl);

        return hRes;
    }

    return E_FAIL;
}

/**************************************************************************
*  IShellBrowserImpl_OnSelChange
*/
static HRESULT IShellBrowserImpl_OnSelChange(IShellBrowserImpl *This, const IShellView *ppshv)
{
    FileOpenDlgInfos *fodInfos;

    fodInfos = get_filedlg_infoptr(This->hwndOwner);
    TRACE("(%p do=%p view=%p)\n", This, fodInfos->Shell.FOIDataObject, fodInfos->Shell.FOIShellView);

    /* release old selections */
    if (fodInfos->Shell.FOIDataObject)
        IDataObject_Release(fodInfos->Shell.FOIDataObject);

    /* get a new DataObject from the ShellView */
    if(FAILED(IShellView_GetItemObject(fodInfos->Shell.FOIShellView, SVGIO_SELECTION,
                                       &IID_IDataObject, (void**)&fodInfos->Shell.FOIDataObject)))
        return E_FAIL;

    FILEDLG95_FILENAME_FillFromSelection(This->hwndOwner);

    if(fodInfos->ofnInfos->Flags & OFN_EXPLORER)
        SendCustomDlgNotificationMessage(This->hwndOwner, CDN_SELCHANGE);
    return S_OK;
}

/**************************************************************************
*  IShellBrowserImpl_ICommDlgBrowser_OnStateChange
*/
static HRESULT WINAPI IShellBrowserImpl_ICommDlgBrowser_OnStateChange(ICommDlgBrowser *iface,
                                                               IShellView *ppshv,
                                                               ULONG uChange)
{

    IShellBrowserImpl *This = impl_from_ICommDlgBrowser(iface);

    TRACE("(%p shv=%p)\n", This, ppshv);

    switch (uChange)
    {
        case CDBOSC_SETFOCUS:
             /* FIXME: Reset the default button.
	        This should be taken care of by defdlg. If control
	        other than button receives focus the default button
	        should be restored. */
             SendMessageA(This->hwndOwner, DM_SETDEFID, IDOK, 0);

            break;
        case CDBOSC_KILLFOCUS:
	    {
                FileOpenDlgInfos *fodInfos = get_filedlg_infoptr(This->hwndOwner);
		if(fodInfos->DlgInfos.dwDlgProp & FODPROP_SAVEDLG)
		{
		    WCHAR szSave[16];
		    LoadStringW(COMDLG32_hInstance, IDS_SAVE_BUTTON, szSave, ARRAY_SIZE(szSave));
		    SetDlgItemTextW(fodInfos->ShellInfos.hwndOwner, IDOK, szSave);
		}
            }
            break;
        case CDBOSC_SELCHANGE:
            return IShellBrowserImpl_OnSelChange(This, ppshv);
        case CDBOSC_RENAME:
	    /* nothing to do */
            break;
    }

    return NOERROR;
}

/*         send_includeitem_notification
 *
 * Sends a CDN_INCLUDEITEM notification for "pidl" to hwndParentDlg
 */
static LRESULT send_includeitem_notification(HWND hwndParentDlg, LPCITEMIDLIST pidl)
{
    LRESULT hook_result = 0;
    FileOpenDlgInfos *fodInfos = get_filedlg_infoptr(hwndParentDlg);

    if(!fodInfos) return 0;

    if(fodInfos->DlgInfos.hwndCustomDlg)
    {
        TRACE("call notify CDN_INCLUDEITEM for pidl=%p\n", pidl);
        if(fodInfos->unicode)
        {
                OFNOTIFYEXW ofnNotify;
                ofnNotify.psf = fodInfos->Shell.FOIShellFolder;
                ofnNotify.pidl = (LPITEMIDLIST)pidl;
                ofnNotify.hdr.hwndFrom = hwndParentDlg;
                ofnNotify.hdr.idFrom = 0;
                ofnNotify.hdr.code = CDN_INCLUDEITEM;
                ofnNotify.lpOFN = fodInfos->ofnInfos;
                hook_result = SendMessageW(fodInfos->DlgInfos.hwndCustomDlg, WM_NOTIFY, 0, (LPARAM)&ofnNotify);
        }
        else
        {
                OFNOTIFYEXA ofnNotify;
                ofnNotify.psf = fodInfos->Shell.FOIShellFolder;
                ofnNotify.pidl = (LPITEMIDLIST)pidl;
                ofnNotify.hdr.hwndFrom = hwndParentDlg;
                ofnNotify.hdr.idFrom = 0;
                ofnNotify.hdr.code = CDN_INCLUDEITEM;
                ofnNotify.lpOFN = (LPOPENFILENAMEA)fodInfos->ofnInfos;
                hook_result = SendMessageA(fodInfos->DlgInfos.hwndCustomDlg, WM_NOTIFY, 0, (LPARAM)&ofnNotify);
        }
    }
    TRACE("Retval: 0x%08lx\n", hook_result);
    return hook_result;
}

/**************************************************************************
*  IShellBrowserImpl_ICommDlgBrowser_IncludeObject
*/
static HRESULT WINAPI IShellBrowserImpl_ICommDlgBrowser_IncludeObject(ICommDlgBrowser *iface,
                                                               IShellView * ppshv,
                                                               LPCITEMIDLIST pidl)
{
    FileOpenDlgInfos *fodInfos;
    ULONG ulAttr;
    STRRET str;
    WCHAR szPathW[MAX_PATH];

    IShellBrowserImpl *This = impl_from_ICommDlgBrowser(iface);

    TRACE("(%p)\n", This);

    fodInfos = get_filedlg_infoptr(This->hwndOwner);

    ulAttr = SFGAO_HIDDEN | SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_FILESYSANCESTOR | SFGAO_LINK;
    IShellFolder_GetAttributesOf(fodInfos->Shell.FOIShellFolder, 1, &pidl, &ulAttr);

    if( (ulAttr & SFGAO_HIDDEN) ||                                      /* hidden */
        !(ulAttr & (SFGAO_FILESYSTEM | SFGAO_FILESYSANCESTOR))) /* special folder */
        return S_FALSE;

    /* always include directories and links */
    if(ulAttr & (SFGAO_FOLDER | SFGAO_LINK))
        return S_OK;

    /* if the application takes care of including the item we are done */
    if(fodInfos->ofnInfos->Flags & OFN_ENABLEINCLUDENOTIFY &&
       send_includeitem_notification(This->hwndOwner, pidl))
        return S_OK;

    /* Check if there is a mask to apply if not */
    if(!fodInfos->ShellInfos.lpstrCurrentFilter || !fodInfos->ShellInfos.lpstrCurrentFilter[0])
        return S_OK;

    if (SUCCEEDED(IShellFolder_GetDisplayNameOf(fodInfos->Shell.FOIShellFolder, pidl, SHGDN_INFOLDER | SHGDN_FORPARSING, &str)))
    {
      if (COMDLG32_StrRetToStrNW(szPathW, MAX_PATH, &str, pidl))
      {
	  if (PathMatchSpecW(szPathW, fodInfos->ShellInfos.lpstrCurrentFilter))
          return S_OK;
      }
    }
    return S_FALSE;

}

static const ICommDlgBrowserVtbl IShellBrowserImpl_ICommDlgBrowser_Vtbl =
{
        /* IUnknown */
        IShellBrowserImpl_ICommDlgBrowser_QueryInterface,
        IShellBrowserImpl_ICommDlgBrowser_AddRef,
        IShellBrowserImpl_ICommDlgBrowser_Release,
        /* ICommDlgBrowser */
        IShellBrowserImpl_ICommDlgBrowser_OnDefaultCommand,
        IShellBrowserImpl_ICommDlgBrowser_OnStateChange,
        IShellBrowserImpl_ICommDlgBrowser_IncludeObject
};




/*
 * IServiceProvider
 */

/***************************************************************************
*  IShellBrowserImpl_IServiceProvider_QueryInterface
*/
static HRESULT WINAPI IShellBrowserImpl_IServiceProvider_QueryInterface(
	IServiceProvider *iface,
	REFIID riid,
	LPVOID *ppvObj)
{
    IShellBrowserImpl *This = impl_from_IServiceProvider(iface);

    FIXME("(%p)\n", This);

    return IShellBrowserImpl_QueryInterface(&This->IShellBrowser_iface,riid,ppvObj);
}

/**************************************************************************
*  IShellBrowserImpl_IServiceProvider_AddRef
*/
static ULONG WINAPI IShellBrowserImpl_IServiceProvider_AddRef(IServiceProvider * iface)
{
    IShellBrowserImpl *This = impl_from_IServiceProvider(iface);

    FIXME("(%p)\n", This);

    return IShellBrowserImpl_AddRef(&This->IShellBrowser_iface);
}

/**************************************************************************
*  IShellBrowserImpl_IServiceProvider_Release
*/
static ULONG WINAPI IShellBrowserImpl_IServiceProvider_Release(IServiceProvider * iface)
{
    IShellBrowserImpl *This = impl_from_IServiceProvider(iface);

    FIXME("(%p)\n", This);

    return IShellBrowserImpl_Release(&This->IShellBrowser_iface);
}

/**************************************************************************
*  IShellBrowserImpl_IServiceProvider_Release
*
* NOTES
*  the w2k shellview asks for (guidService = SID_STopLevelBrowser,
*  riid = IShellBrowser) to call SendControlMsg ().
*
* FIXME
*  this is a hack!
*/

static HRESULT WINAPI IShellBrowserImpl_IServiceProvider_QueryService(
	IServiceProvider * iface,
	REFGUID guidService,
	REFIID riid,
	void** ppv)
{
    IShellBrowserImpl *This = impl_from_IServiceProvider(iface);

    FIXME("(%p)\n\t%s\n\t%s\n", This,debugstr_guid(guidService), debugstr_guid(riid) );

    *ppv = NULL;
    if(guidService && IsEqualIID(guidService, &SID_STopLevelBrowser))
        return IShellBrowserImpl_QueryInterface(&This->IShellBrowser_iface,riid,ppv);

    FIXME("(%p) unknown interface requested\n", This);
    return E_NOINTERFACE;

}

static const IServiceProviderVtbl IShellBrowserImpl_IServiceProvider_Vtbl =
{
        /* IUnknown */
        IShellBrowserImpl_IServiceProvider_QueryInterface,
        IShellBrowserImpl_IServiceProvider_AddRef,
        IShellBrowserImpl_IServiceProvider_Release,
        /* IServiceProvider */
        IShellBrowserImpl_IServiceProvider_QueryService
};
