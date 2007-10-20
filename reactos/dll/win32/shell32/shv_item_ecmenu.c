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
#include "debughlp.h"
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
    LPWSTR szCommand;
    LPWSTR szVerb;
    INT iCommand;

} StaticItemCmImpl;

static const IContextMenu2Vtbl cmvt;

/**************************************************************************
*   ISvItemCm_Constructor()
*/
IContextMenu2 *ISvStaticItemCm_Constructor(LPSHELLFOLDER pSFParent, LPCITEMIDLIST pidl, LPCITEMIDLIST *apidl, UINT cidl, HKEY hKey)
{	
    StaticItemCmImpl* cm;
	UINT  u;

	cm = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(StaticItemCmImpl));
	cm->lpVtbl = &cmvt;
	cm->ref = 1;
    cm->iCommand = -1;
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
static HRESULT WINAPI ISvStaticItemCm_fnQueryInterface(IContextMenu2 *iface, REFIID riid, LPVOID *ppvObj)
{
	StaticItemCmImpl *This = (StaticItemCmImpl *)iface;

	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,debugstr_guid(riid),ppvObj);

	*ppvObj = NULL;

        if(IsEqualIID(riid, &IID_IUnknown) ||
           IsEqualIID(riid, &IID_IContextMenu) ||
           IsEqualIID(riid, &IID_IContextMenu2))
	{
	  *ppvObj = This;
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
static ULONG WINAPI ISvStaticItemCm_fnAddRef(IContextMenu2 *iface)
{
	StaticItemCmImpl *This = (StaticItemCmImpl *)iface;
	ULONG refCount = InterlockedIncrement(&This->ref);

	TRACE("(%p)->(count=%u)\n", This, refCount - 1);

	return refCount;
}

/**************************************************************************
*  ISvItemCm_fnRelease
*/
static ULONG WINAPI ISvStaticItemCm_fnRelease(IContextMenu2 *iface)
{
	StaticItemCmImpl *This = (StaticItemCmImpl *)iface;
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


}

/**************************************************************************
* ISvItemCm_fnQueryContextMenu()
* FIXME: load menu MENU_SHV_FILE out of resources instead if creating
*		 each menu item by calling _InsertMenuItem()
*/
static HRESULT WINAPI ISvStaticItemCm_fnQueryContextMenu(
	IContextMenu2 *iface,
	HMENU hmenu,
	UINT indexMenu,
	UINT idCmdFirst,
	UINT idCmdLast,
	UINT uFlags)
{
    IDataObject * pDataObj;
    MENUITEMINFOW	mii;

	StaticItemCmImpl *This = (StaticItemCmImpl *)iface;
    
    if (!This->szVerb || This->szCommand)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);


    ZeroMemory(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
    mii.dwTypeData = (LPWSTR) This->szVerb;
    mii.cch = strlenW( mii.dwTypeData );
    mii.fState = fState;
	mii.wID = idCmdFirst + 1;
	mii.fType = fType;
	InsertMenuItemW( hmenu, indexMenu, fByPosition, &mii);


	return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 1);
}

/**************************************************************************
* ISvItemCm_fnInvokeCommand()
*/
static HRESULT WINAPI ISvStaticItemCm_fnInvokeCommand(
	IContextMenu2 *iface,
	LPCMINVOKECOMMANDINFO lpcmi)
{
    StaticItemCmImpl *This = (StaticItemCmImpl *)iface;

    if (lpcmi->cbSize != sizeof(CMINVOKECOMMANDINFO))
        FIXME("Is an EX structure\n");

    TRACE("(%p)->(invcom=%p verb=%p wnd=%p)\n",This,lpcmi,lpcmi->lpVerb, lpcmi->hwnd);

    if( HIWORD(lpcmi->lpVerb)==0 && LOWORD(lpcmi->lpVerb) > FCIDM_SHVIEWLAST)
    {
        TRACE("Invalid Verb %x\n",LOWORD(lpcmi->lpVerb));
        return E_INVALIDARG;
    }




    return NOERROR;
}

/**************************************************************************
*  ISvItemCm_fnGetCommandString()
*/
static HRESULT WINAPI ISvStaticItemCm_fnGetCommandString(
	IContextMenu2 *iface,
	UINT_PTR idCommand,
	UINT uFlags,
	UINT* lpReserved,
	LPSTR lpszName,
	UINT uMaxNameLen)
{
	StaticItemCmImpl *This = (StaticItemCmImpl *)iface;

	HRESULT  hr = E_INVALIDARG;

	TRACE("(%p)->(idcom=%lx flags=%x %p name=%p len=%x)\n",This, idCommand, uFlags, lpReserved, lpszName, uMaxNameLen);

	TRACE("-- (%p)->(name=%s)\n",This, lpszName);
	return hr;
}

/**************************************************************************
* ISvItemCm_fnHandleMenuMsg()
* NOTES
*  should be only in IContextMenu2 and IContextMenu3
*  is nevertheless called from word95
*/
static HRESULT WINAPI ISvStaticItemCm_fnHandleMenuMsg(
	IContextMenu2 *iface,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	StaticItemCmImpl *This = (StaticItemCmImpl *)iface;

	TRACE("(%p)->(msg=%x wp=%lx lp=%lx)\n",This, uMsg, wParam, lParam);

	return E_NOTIMPL;
}

static const IContextMenu2Vtbl cmvt =
{
	ISvStaticItemCm_fnQueryInterface,
	ISvStaticItemCm_fnAddRef,
	ISvStaticItemCm_fnRelease,
	ISvStaticItemCm_fnQueryContextMenu,
	ISvStaticItemCm_fnInvokeCommand,
	ISvStaticItemCm_fnGetCommandString,
	ISvStaticItemCm_fnHandleMenuMsg
};

