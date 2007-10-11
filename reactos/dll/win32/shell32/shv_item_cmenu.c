/*
 *	IContextMenu for items in the shellview
 *
 * Copyright 1998, 2000 Juergen Schmied <juergen.schmied@debitel.net>
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

#include "winerror.h"
#include "wine/debug.h"

#include "windef.h"
#include "wingdi.h"
#include "pidl.h"
#include "undocshell.h"
#include "shlobj.h"

#include "shell32_main.h"
#include "shellfolder.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/**************************************************************************
*  IContextMenu Implementation
*/
typedef struct
{	const IContextMenu2Vtbl *lpVtbl;
	LONG		ref;
	IShellFolder*	pSFParent;
	LPITEMIDLIST	pidl;		/* root pidl */
	LPITEMIDLIST	*apidl;		/* array of child pidls */
	UINT		cidl;
	BOOL		bAllValues;
} ItemCmImpl;


static const IContextMenu2Vtbl cmvt;

/**************************************************************************
* ISvItemCm_CanRenameItems()
*/
static BOOL ISvItemCm_CanRenameItems(ItemCmImpl *This)
{	UINT  i;
	DWORD dwAttributes;

	TRACE("(%p)->()\n",This);

	if(This->apidl)
	{
	  for(i = 0; i < This->cidl; i++){}
	  if(i > 1) return FALSE;		/* can't rename more than one item at a time*/
	  dwAttributes = SFGAO_CANRENAME;
	  IShellFolder_GetAttributesOf(This->pSFParent, 1, (LPCITEMIDLIST*)This->apidl, &dwAttributes);
	  return dwAttributes & SFGAO_CANRENAME;
	}
	return FALSE;
}

/**************************************************************************
*   ISvItemCm_Constructor()
*/
IContextMenu2 *ISvItemCm_Constructor(LPSHELLFOLDER pSFParent, LPCITEMIDLIST pidl, LPCITEMIDLIST *apidl, UINT cidl)
{	ItemCmImpl* cm;
	UINT  u;

	cm = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(ItemCmImpl));
	cm->lpVtbl = &cmvt;
	cm->ref = 1;
	cm->pidl = ILClone(pidl);
	cm->pSFParent = pSFParent;

	if(pSFParent) IShellFolder_AddRef(pSFParent);

	cm->apidl = _ILCopyaPidl(apidl, cidl);
	cm->cidl = cidl;

	cm->bAllValues = 1;
	for(u = 0; u < cidl; u++)
	{
	  cm->bAllValues &= (_ILIsValue(apidl[u]) ? 1 : 0);
	}

	TRACE("(%p)->()\n",cm);

	return (IContextMenu2*)cm;
}

/**************************************************************************
*  ISvItemCm_fnQueryInterface
*/
static HRESULT WINAPI ISvItemCm_fnQueryInterface(IContextMenu2 *iface, REFIID riid, LPVOID *ppvObj)
{
	ItemCmImpl *This = (ItemCmImpl *)iface;

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
*  ISvItemCm_fnAddRef
*/
static ULONG WINAPI ISvItemCm_fnAddRef(IContextMenu2 *iface)
{
	ItemCmImpl *This = (ItemCmImpl *)iface;
	ULONG refCount = InterlockedIncrement(&This->ref);

	TRACE("(%p)->(count=%u)\n", This, refCount - 1);

	return refCount;
}

/**************************************************************************
*  ISvItemCm_fnRelease
*/
static ULONG WINAPI ISvItemCm_fnRelease(IContextMenu2 *iface)
{
	ItemCmImpl *This = (ItemCmImpl *)iface;
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(count=%i)\n", This, refCount + 1);

	if (!refCount)
	{
	  TRACE(" destroying IContextMenu(%p)\n",This);

	  if(This->pSFParent)
	    IShellFolder_Release(This->pSFParent);

	  SHFree(This->pidl);

	  /*make sure the pidl is freed*/
	  _ILFreeaPidl(This->apidl, This->cidl);

	  HeapFree(GetProcessHeap(),0,This);
	}
	return refCount;
}

/**************************************************************************
*  ICM_InsertItem()
*/
void WINAPI _InsertMenuItem (
	HMENU hmenu,
	UINT indexMenu,
	BOOL fByPosition,
	UINT wID,
	UINT fType,
	LPCSTR dwTypeData,
	UINT fState)
{
	MENUITEMINFOA	mii;

	ZeroMemory(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
	if (fType == MFT_SEPARATOR)
	{
	  mii.fMask = MIIM_ID | MIIM_TYPE;
	}
	else
	{
	  mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
	  mii.dwTypeData = (LPSTR) dwTypeData;
	  mii.fState = fState;
	}
	mii.wID = wID;
	mii.fType = fType;
	InsertMenuItemA( hmenu, indexMenu, fByPosition, &mii);
}

/**************************************************************************
* ISvItemCm_fnQueryContextMenu()
* FIXME: load menu MENU_SHV_FILE out of resources instead if creating
*		 each menu item by calling _InsertMenuItem()
*/
static HRESULT WINAPI ISvItemCm_fnQueryContextMenu(
	IContextMenu2 *iface,
	HMENU hmenu,
	UINT indexMenu,
	UINT idCmdFirst,
	UINT idCmdLast,
	UINT uFlags)
{
	ItemCmImpl *This = (ItemCmImpl *)iface;

	TRACE("(%p)->(hmenu=%p indexmenu=%x cmdfirst=%x cmdlast=%x flags=%x )\n",This, hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

	if (idCmdFirst != 0)
	  FIXME("We should use idCmdFirst=%d and idCmdLast=%d for command ids\n", idCmdFirst, idCmdLast);

	if(!(CMF_DEFAULTONLY & uFlags) && This->cidl>0)
	{
	  if(!(uFlags & CMF_EXPLORE))
	    _InsertMenuItem(hmenu, indexMenu++, TRUE, FCIDM_SHVIEW_OPEN, MFT_STRING, "&Select", MFS_ENABLED);

	  if(This->bAllValues)
	  {
	    _InsertMenuItem(hmenu, indexMenu++, TRUE, FCIDM_SHVIEW_OPEN, MFT_STRING, "&Open", MFS_ENABLED);
	    _InsertMenuItem(hmenu, indexMenu++, TRUE, FCIDM_SHVIEW_EXPLORE, MFT_STRING, "&Explore", MFS_ENABLED);
	  }
	  else
	  {
	    _InsertMenuItem(hmenu, indexMenu++, TRUE, FCIDM_SHVIEW_EXPLORE, MFT_STRING, "&Explore", MFS_ENABLED);
	    _InsertMenuItem(hmenu, indexMenu++, TRUE, FCIDM_SHVIEW_OPEN, MFT_STRING, "&Open", MFS_ENABLED);
	  }

	  SetMenuDefaultItem(hmenu, 0, MF_BYPOSITION);

	  _InsertMenuItem(hmenu, indexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
	  _InsertMenuItem(hmenu, indexMenu++, TRUE, FCIDM_SHVIEW_COPY, MFT_STRING, "&Copy", MFS_ENABLED);
	  _InsertMenuItem(hmenu, indexMenu++, TRUE, FCIDM_SHVIEW_CUT, MFT_STRING, "&Cut", MFS_ENABLED);

	  _InsertMenuItem(hmenu, indexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
	  _InsertMenuItem(hmenu, indexMenu++, TRUE, FCIDM_SHVIEW_DELETE, MFT_STRING, "&Delete", MFS_ENABLED);

	  if(uFlags & CMF_CANRENAME)
	    _InsertMenuItem(hmenu, indexMenu++, TRUE, FCIDM_SHVIEW_RENAME, MFT_STRING, "&Rename", ISvItemCm_CanRenameItems(This) ? MFS_ENABLED : MFS_DISABLED);

	  _InsertMenuItem(hmenu, indexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
	  _InsertMenuItem(hmenu, indexMenu++, TRUE, FCIDM_SHVIEW_PROPERTIES, MFT_STRING, "&Properties", MFS_ENABLED);

	  return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (FCIDM_SHVIEWLAST));
	}
	return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);
}

/**************************************************************************
* DoOpenExplore
*
*  for folders only
*/

static void DoOpenExplore(
	IContextMenu2 *iface,
	HWND hwnd,
	LPCSTR verb)
{
	ItemCmImpl *This = (ItemCmImpl *)iface;

	UINT i, bFolderFound = FALSE;
	LPITEMIDLIST	pidlFQ;
	SHELLEXECUTEINFOA	sei;

	/* Find the first item in the list that is not a value. These commands
	    should never be invoked if there isn't at least one folder item in the list.*/

	for(i = 0; i<This->cidl; i++)
	{
	  if(!_ILIsValue(This->apidl[i]))
	  {
	    bFolderFound = TRUE;
	    break;
	  }
	}

	if (!bFolderFound) return;

	pidlFQ = ILCombine(This->pidl, This->apidl[i]);

	ZeroMemory(&sei, sizeof(sei));
	sei.cbSize = sizeof(sei);
	sei.fMask = SEE_MASK_INVOKEIDLIST | SEE_MASK_CLASSNAME;
	sei.lpIDList = pidlFQ;
	sei.lpClass = "Folder";
	sei.hwnd = hwnd;
	sei.nShow = SW_SHOWNORMAL;
	sei.lpVerb = verb;
	ShellExecuteExA(&sei);
	SHFree(pidlFQ);
}

/**************************************************************************
* DoRename
*/
static void DoRename(
	IContextMenu2 *iface,
	HWND hwnd)
{
	ItemCmImpl *This = (ItemCmImpl *)iface;

	LPSHELLBROWSER	lpSB;
	LPSHELLVIEW	lpSV;

	TRACE("(%p)->(wnd=%p)\n",This, hwnd);

	/* get the active IShellView */
	if ((lpSB = (LPSHELLBROWSER)SendMessageA(hwnd, CWM_GETISHELLBROWSER,0,0)))
	{
	  if(SUCCEEDED(IShellBrowser_QueryActiveShellView(lpSB, &lpSV)))
	  {
	    TRACE("(sv=%p)\n",lpSV);
	    IShellView_SelectItem(lpSV, This->apidl[0],
              SVSI_DESELECTOTHERS|SVSI_EDIT|SVSI_ENSUREVISIBLE|SVSI_FOCUSED|SVSI_SELECT);
	    IShellView_Release(lpSV);
	  }
	}
}

/**************************************************************************
 * DoDelete
 *
 * deletes the currently selected items
 */
static void DoDelete(IContextMenu2 *iface)
{
	ItemCmImpl *This = (ItemCmImpl *)iface;
	ISFHelper * psfhlp;

	IShellFolder_QueryInterface(This->pSFParent, &IID_ISFHelper, (LPVOID*)&psfhlp);
	if (psfhlp)
	{
	  ISFHelper_DeleteItems(psfhlp, This->cidl, (LPCITEMIDLIST *)This->apidl);
	  ISFHelper_Release(psfhlp);
	}
}

/**************************************************************************
 * DoCopyOrCut
 *
 * copies the currently selected items into the clipboard
 */
static BOOL DoCopyOrCut(
	IContextMenu2 *iface,
	HWND hwnd,
	BOOL bCut)
{
	ItemCmImpl *This = (ItemCmImpl *)iface;

	LPSHELLBROWSER	lpSB;
	LPSHELLVIEW	lpSV;
	LPDATAOBJECT    lpDo;

	TRACE("(%p)->(wnd=%p,bCut=0x%08x)\n",This, hwnd, bCut);

	/* get the active IShellView */
	if ((lpSB = (LPSHELLBROWSER)SendMessageA(hwnd, CWM_GETISHELLBROWSER,0,0)))
	{
	  if (SUCCEEDED(IShellBrowser_QueryActiveShellView(lpSB, &lpSV)))
	  {
	    if (SUCCEEDED(IShellView_GetItemObject(lpSV, SVGIO_SELECTION, &IID_IDataObject, (LPVOID*)&lpDo)))
	    {
	      OleSetClipboard(lpDo);
	      IDataObject_Release(lpDo);
	    }
	    IShellView_Release(lpSV);
	  }
	}
	return TRUE;
}
static void DoProperties(
	IContextMenu2 *iface,
	HWND hwnd)
{
	ItemCmImpl *This = (ItemCmImpl *)iface;
	LPITEMIDLIST	pidlFQ = NULL;
	SHELLEXECUTEINFOA	sei;

    if (_ILIsMyComputer(This->apidl[0]))
    {
        ShellExecuteA(hwnd, "open", "rundll32.exe shell32.dll,Control_RunDLL sysdm.cpl", NULL, NULL, SW_SHOWNORMAL);
        return;
    }
    else if (_ILIsDesktop(This->apidl[0]))
    {
        ShellExecuteA(hwnd, "open", "rundll32.exe shell32.dll,Control_RunDLL desk.cpl", NULL, NULL, SW_SHOWNORMAL);
        return;
    }
    else if (_ILIsDrive(This->apidl[0]))
    {
       WCHAR buffer[111];
       ILGetDisplayName(This->apidl[0], buffer);
       SH_ShowDriveProperties(buffer);
       return;
    }
    else
    {
        pidlFQ = ILCombine(This->pidl, This->apidl[0]);
    }

	ZeroMemory(&sei, sizeof(sei));
	sei.cbSize = sizeof(sei);
	sei.fMask = SEE_MASK_INVOKEIDLIST;
	sei.lpIDList = pidlFQ;
	sei.hwnd = hwnd;
	sei.nShow = SW_SHOWNORMAL;
	sei.lpVerb = "properties";

    TRACE("DoProperties before ShellExecuteEx\n");
	ShellExecuteExA(&sei);
    TRACE("DoProperties after ShellExecuteEx\n");

    if (pidlFQ)
    {
	   SHFree(pidlFQ);
    }
}

/**************************************************************************
* ISvItemCm_fnInvokeCommand()
*/
static HRESULT WINAPI ISvItemCm_fnInvokeCommand(
	IContextMenu2 *iface,
	LPCMINVOKECOMMANDINFO lpcmi)
{
    ItemCmImpl *This = (ItemCmImpl *)iface;

    if (lpcmi->cbSize != sizeof(CMINVOKECOMMANDINFO))
        FIXME("Is an EX structure\n");

    TRACE("(%p)->(invcom=%p verb=%p wnd=%p)\n",This,lpcmi,lpcmi->lpVerb, lpcmi->hwnd);

    if( HIWORD(lpcmi->lpVerb)==0 && LOWORD(lpcmi->lpVerb) > FCIDM_SHVIEWLAST)
    {
        TRACE("Invalid Verb %x\n",LOWORD(lpcmi->lpVerb));
        return E_INVALIDARG;
    }

    if (HIWORD(lpcmi->lpVerb) == 0)
    {
        switch(LOWORD(lpcmi->lpVerb))
        {
        case FCIDM_SHVIEW_EXPLORE:
            TRACE("Verb FCIDM_SHVIEW_EXPLORE\n");
            DoOpenExplore(iface, lpcmi->hwnd, "explore");
            break;
        case FCIDM_SHVIEW_OPEN:
            TRACE("Verb FCIDM_SHVIEW_OPEN\n");
            DoOpenExplore(iface, lpcmi->hwnd, "open");
            break;
        case FCIDM_SHVIEW_RENAME:
            TRACE("Verb FCIDM_SHVIEW_RENAME\n");
            DoRename(iface, lpcmi->hwnd);
            break;
        case FCIDM_SHVIEW_DELETE:
            TRACE("Verb FCIDM_SHVIEW_DELETE\n");
            DoDelete(iface);
            break;
        case FCIDM_SHVIEW_COPY:
            TRACE("Verb FCIDM_SHVIEW_COPY\n");
            DoCopyOrCut(iface, lpcmi->hwnd, FALSE);
            break;
        case FCIDM_SHVIEW_CUT:
            TRACE("Verb FCIDM_SHVIEW_CUT\n");
            DoCopyOrCut(iface, lpcmi->hwnd, TRUE);
            break;
        case FCIDM_SHVIEW_PROPERTIES:
            TRACE("Verb FCIDM_SHVIEW_PROPERTIES\n");
            DoProperties(iface, lpcmi->hwnd);
            break;
        default:
            FIXME("Unhandled Verb %xl\n",LOWORD(lpcmi->lpVerb));
        }
    }
    else
    {
        TRACE("Verb is %s\n",debugstr_a(lpcmi->lpVerb));
        if (strcmp(lpcmi->lpVerb,"delete")==0)
            DoDelete(iface);
        else
            FIXME("Unhandled string verb %s\n",debugstr_a(lpcmi->lpVerb));
    }
    return NOERROR;
}

/**************************************************************************
*  ISvItemCm_fnGetCommandString()
*/
static HRESULT WINAPI ISvItemCm_fnGetCommandString(
	IContextMenu2 *iface,
	UINT_PTR idCommand,
	UINT uFlags,
	UINT* lpReserved,
	LPSTR lpszName,
	UINT uMaxNameLen)
{
	ItemCmImpl *This = (ItemCmImpl *)iface;

	HRESULT  hr = E_INVALIDARG;

	TRACE("(%p)->(idcom=%lx flags=%x %p name=%p len=%x)\n",This, idCommand, uFlags, lpReserved, lpszName, uMaxNameLen);

	switch(uFlags)
	{
	  case GCS_HELPTEXTA:
	  case GCS_HELPTEXTW:
	    hr = E_NOTIMPL;
	    break;

	  case GCS_VERBA:
	    switch(idCommand)
	    {
	      case FCIDM_SHVIEW_RENAME:
	        strcpy((LPSTR)lpszName, "rename");
	        hr = NOERROR;
	        break;
	    }
	    break;

	     /* NT 4.0 with IE 3.0x or no IE will always call This with GCS_VERBW. In This
	     case, you need to do the lstrcpyW to the pointer passed.*/
	  case GCS_VERBW:
	    switch(idCommand)
	    { case FCIDM_SHVIEW_RENAME:
                MultiByteToWideChar( CP_ACP, 0, "rename", -1, (LPWSTR)lpszName, uMaxNameLen );
	        hr = NOERROR;
	        break;
	    }
	    break;

	  case GCS_VALIDATEA:
	  case GCS_VALIDATEW:
	    hr = NOERROR;
	    break;
	}
	TRACE("-- (%p)->(name=%s)\n",This, lpszName);
	return hr;
}

/**************************************************************************
* ISvItemCm_fnHandleMenuMsg()
* NOTES
*  should be only in IContextMenu2 and IContextMenu3
*  is nevertheless called from word95
*/
static HRESULT WINAPI ISvItemCm_fnHandleMenuMsg(
	IContextMenu2 *iface,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	ItemCmImpl *This = (ItemCmImpl *)iface;

	TRACE("(%p)->(msg=%x wp=%lx lp=%lx)\n",This, uMsg, wParam, lParam);

	return E_NOTIMPL;
}

static const IContextMenu2Vtbl cmvt =
{
	ISvItemCm_fnQueryInterface,
	ISvItemCm_fnAddRef,
	ISvItemCm_fnRelease,
	ISvItemCm_fnQueryContextMenu,
	ISvItemCm_fnInvokeCommand,
	ISvItemCm_fnGetCommandString,
	ISvItemCm_fnHandleMenuMsg
};
