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
#define YDEBUG
#include "winerror.h"
#include "wine/debug.h"

#include "windef.h"
#include "wingdi.h"
#include "pidl.h"
#include "undocshell.h"
#include "shlobj.h"
#include "objbase.h"

#include "shlwapi.h"
#include "shell32_main.h"
#include "shellfolder.h"
#include "debughlp.h"
WINE_DEFAULT_DEBUG_CHANNEL(shell);

/* ugly hack for cut&paste files */
BOOL fileMoving = FALSE;

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
    IContextMenu ** ecmenu;
    UINT           esize;
    UINT           ecount;
    UINT           iIdSHEFirst;
    UINT           iIdSHELast;
    SFGAOF         rfg;
} ItemCmImpl;

UINT
SH_EnumerateDynamicContextHandlerForKey(LPWSTR szFileClass, ItemCmImpl *This, IDataObject * pDataObj, LPITEMIDLIST pidlFolder);
WCHAR *build_paths_list(LPCWSTR wszBasePath, int cidl, LPCITEMIDLIST *pidls);

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

	if(pSFParent)
	{
	    HRESULT hr;
	    IShellFolder_AddRef(pSFParent);
	    cm->rfg = SFGAO_BROWSABLE | SFGAO_CANCOPY | SFGAO_CANMOVE | SFGAO_CANDELETE | SFGAO_CANRENAME | SFGAO_HASPROPSHEET;
	    hr = IShellFolder_GetAttributesOf(pSFParent, cidl, apidl, &cm->rfg);
	    if (!SUCCEEDED(hr))
	        cm->rfg = 0; /* No action available */
	}

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

HRESULT
DoCustomItemAction(ItemCmImpl *This, LPARAM lParam, UINT uMsg)
{
   IContextMenu2 * cmenu;
   IContextMenu * menu;
   MEASUREITEMSTRUCT * lpmis = (MEASUREITEMSTRUCT *)lParam;
   DRAWITEMSTRUCT * drawItem = (DRAWITEMSTRUCT *)lParam;
   HRESULT hResult;


   TRACE("DoCustomItemAction entered with uMsg %x lParam %p\n", uMsg, lParam);

   if (uMsg == WM_MEASUREITEM)
   {
       menu = This->ecmenu[lpmis->itemID - This->iIdSHEFirst];
   }
   else if (uMsg == WM_DRAWITEM)
   {
       menu = This->ecmenu[drawItem->itemID - This->iIdSHEFirst];
   }
   else
   {
      ERR("unexpected message\n");
      return E_FAIL;
   }

   if (!menu)
   {
     ERR("item is not valid\n");
     return E_FAIL;
   }
  
   hResult = menu->lpVtbl->QueryInterface(menu, &IID_IContextMenu2, (void**)&cmenu);
   if (hResult != S_OK)
   {
     ERR("failed to get IID_IContextMenu2 interface\n");
     return hResult;
   }

   hResult = cmenu->lpVtbl->HandleMenuMsg(cmenu, uMsg, (WPARAM)0, lParam);
   TRACE("returning hResult %x\n", hResult);
   return hResult;
}

BOOL
SH_EnlargeContextMenuArray(ItemCmImpl *This, UINT newsize)
{
    BOOL ret = FALSE;

    if (This->ecmenu == NULL)
    {
        This->ecmenu = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, sizeof(IContextMenu*) *10);
        if(This->ecmenu)
            ret = TRUE;
        This->ecount = 0;
        This->esize = 10;
    }
    else
    {
        IContextMenu ** newcmenu = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IContextMenu*)*newsize);
        if (newcmenu)
        {
            memcpy(newcmenu, This->ecmenu, This->ecount * sizeof(IContextMenu*));
            HeapFree(GetProcessHeap(), 0, This->ecmenu);
            This->ecmenu = newcmenu;
            ret = TRUE;
        }
    }
    return ret;
}

VOID
SH_LoadContextMenuHandlers(ItemCmImpl *This, IDataObject * pDataObj, HMENU hMenu, UINT indexMenu )
{
    UINT i;
    WCHAR buffer[111];
    char ebuf[10];
    HRESULT hr;
    IContextMenu * cmenu;
    HRESULT hResult;
    UINT idCmdFirst = 0x5000;
    UINT idCmdLast = 0xFFF0;
    static WCHAR szAny[] = { '*',0};

    SH_EnumerateDynamicContextHandlerForKey(szAny, This, pDataObj, This->pidl);

    for (i = 0; i < This->cidl; i++)
    {
        GUID * guid = _ILGetGUIDPointer(This->apidl[i]);
        if (guid)
        {
            LPOLESTR pwszCLSID;
            static const WCHAR CLSID[] = { 'C','L','S','I','D','\\',0 };
            wcscpy(buffer, CLSID);
            hr = StringFromCLSID(guid, &pwszCLSID);
            if (hr == S_OK)
            {
                memcpy(&buffer[6], pwszCLSID, 38 * sizeof(WCHAR));
                SH_EnumerateDynamicContextHandlerForKey(buffer, This, pDataObj, This->pidl);
            }
        }

        if (_ILGetExtension(This->apidl[i], &ebuf[1], sizeof(ebuf) / sizeof(char)))
        {
            ebuf[0] = L'.';
            buffer[0] = L'\0';
            if (MultiByteToWideChar(CP_ACP, 0, ebuf, -1, buffer, 111))
                SH_EnumerateDynamicContextHandlerForKey(buffer, This, pDataObj, This->pidl);
        }
    }

    TRACE("SH_LoadContextMenuHandlers num extensions %u\n", This->ecount);
    This->iIdSHEFirst = idCmdFirst;
    for (i = 0; i < This->ecount; i++)
    {
       cmenu = This->ecmenu[i];
       hResult = cmenu->lpVtbl->QueryContextMenu(cmenu, hMenu, indexMenu, idCmdFirst, idCmdLast, CMF_NORMAL);
       idCmdFirst += (hResult & 0xFFFF);
    }
    This->iIdSHELast = idCmdFirst;

    TRACE("SH_LoadContextMenuHandlers first %x last %x\n", This->iIdSHEFirst, This->iIdSHELast);
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
    IDataObject * pDataObj;
	ItemCmImpl *This = (ItemCmImpl *)iface;
    USHORT lastindex = 0;


	TRACE("(%p)->(hmenu=%p indexmenu=%x cmdfirst=%x cmdlast=%x flags=%x )\n",This, hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

	if (idCmdFirst != 0)
	  FIXME("We should use idCmdFirst=%d and idCmdLast=%d for command ids\n", idCmdFirst, idCmdLast);

	if(!(CMF_DEFAULTONLY & uFlags) && This->cidl>0)
	{
	  if(!(uFlags & CMF_EXPLORE))
	    _InsertMenuItem(hmenu, indexMenu++, TRUE, FCIDM_SHVIEW_OPEN, MFT_STRING, "&Select", MFS_ENABLED);

      TRACE("rfg %x\n", This->rfg);

	  if (This->rfg & SFGAO_HASSUBFOLDER)
	  {
          /* FIXME 
           * check if folder hasnt already been extended
           * use reduce verb then
           */

          _InsertMenuItem(hmenu, indexMenu++, TRUE, FCIDM_SHVIEW_EXPLORE, MFT_STRING, "&Explore", MFS_ENABLED);
	  }

      _InsertMenuItem(hmenu, indexMenu++, TRUE, FCIDM_SHVIEW_OPEN, MFT_STRING, "&Open", MFS_ENABLED);

	  SetMenuDefaultItem(hmenu, 0, MF_BYPOSITION);

	  if (This->rfg & (SFGAO_CANCOPY | SFGAO_CANMOVE))
	  {
	      _InsertMenuItem(hmenu, indexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
	      if (This->rfg & SFGAO_CANCOPY)
	          _InsertMenuItem(hmenu, indexMenu++, TRUE, FCIDM_SHVIEW_COPY, MFT_STRING, "&Copy", MFS_ENABLED);
	      if (This->rfg & SFGAO_CANMOVE)
	          _InsertMenuItem(hmenu, indexMenu++, TRUE, FCIDM_SHVIEW_CUT, MFT_STRING, "&Cut", MFS_ENABLED);
	  }

	  if (This->rfg & SFGAO_CANDELETE)
	  {
	      _InsertMenuItem(hmenu, indexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
	      _InsertMenuItem(hmenu, indexMenu++, TRUE, FCIDM_SHVIEW_DELETE, MFT_STRING, "&Delete", MFS_ENABLED);
	  }

	  if ((uFlags & CMF_CANRENAME) && (This->rfg & SFGAO_CANRENAME))
	      _InsertMenuItem(hmenu, indexMenu++, TRUE, FCIDM_SHVIEW_RENAME, MFT_STRING, "&Rename", ISvItemCm_CanRenameItems(This) ? MFS_ENABLED : MFS_DISABLED);

	  if (This->rfg & SFGAO_HASPROPSHEET)
	  {
	      _InsertMenuItem(hmenu, indexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
	      _InsertMenuItem(hmenu, indexMenu++, TRUE, FCIDM_SHVIEW_PROPERTIES, MFT_STRING, "&Properties", MFS_ENABLED);
	  }

      lastindex = FCIDM_SHVIEWLAST;
	}
    pDataObj = IDataObject_Constructor(NULL, This->pidl, This->apidl, This->cidl);
    if (pDataObj)
    {
        SH_LoadContextMenuHandlers(This, pDataObj, hmenu, indexMenu);
        IDataObject_Release(pDataObj);
    }


	return MAKE_HRESULT(SEVERITY_SUCCESS, 0, lastindex);
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
static void DoDelete(IContextMenu2 *iface, HWND hwnd)
{
    WCHAR szPath[MAX_PATH];
    WCHAR * szTarget;
    SHFILEOPSTRUCTW op;
    LPSHELLBROWSER	lpSB;
    LPSHELLVIEW	lpSV;
    IPersistFolder2 * psf;
    LPITEMIDLIST pidl;
    STRRET strTemp;
    ItemCmImpl *This = (ItemCmImpl *)iface;

    if (IShellFolder2_QueryInterface(This->pSFParent, &IID_IPersistFolder2, (LPVOID*)&psf) != S_OK)
    {
      ERR("Failed to get interface IID_IPersistFolder2\n");
      return;
    }

    if (IPersistFolder2_GetCurFolder(psf, &pidl) != S_OK)
    {
      ERR("IPersistFolder2_GetCurFolder failed\n");
      IPersistFolder2_Release(psf);
      return;
    }

     if (IShellFolder2_GetDisplayNameOf(This->pSFParent, pidl, SHGDN_FORPARSING, &strTemp) != S_OK)
     {
       ERR("IShellFolder_GetDisplayNameOf failed\n");
       IPersistFolder2_Release(psf);
       return;
     }
     szPath[MAX_PATH-1] = 0;
     StrRetToBufW(&strTemp, pidl, szPath, MAX_PATH);
     PathAddBackslashW(szPath);
     IPersistFolder2_Release(psf);

     szTarget = build_paths_list(szPath, This->cidl, This->apidl);

     if (pidl)
     {
       if (SHGetPathFromIDListW(pidl, szPath))
       {
         ZeroMemory(&op, sizeof(op));
         op.hwnd = GetActiveWindow();
         op.wFunc = FO_DELETE;
         op.pFrom = szTarget;
         op.fFlags = FOF_ALLOWUNDO;
         SHFileOperationW(&op);
       }
       ILFree(pidl);
     }

    if ((lpSB = (LPSHELLBROWSER)SendMessageA(hwnd, CWM_GETISHELLBROWSER,0,0)))
	{
	  if (SUCCEEDED(IShellBrowser_QueryActiveShellView(lpSB, &lpSV)))
	  {
        IShellView_Refresh(lpSV);
      }
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

    /*
     * FIXXME
     *
     * The IShellFolder interface GetUIObject should create the specific item and then query if it has an IContextMenu interface
     * If yes return interface to it.
     */

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
    else if (_ILIsBitBucket(This->apidl[0]))
    {
       ///FIXME
       WCHAR szDrive = 'C';
       SH_ShowRecycleBinProperties(szDrive);
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
HRESULT
DoShellExtensions(ItemCmImpl *This, LPCMINVOKECOMMANDINFO lpcmi)
{
    HRESULT hResult = NOERROR;
    UINT i;

    TRACE("DoShellExtensions %p verb %x count %u\n",This, LOWORD(lpcmi->lpVerb), This->ecount);
    for(i = 0; i < This->ecount; i++)
    {
        IContextMenu * cmenu = This->ecmenu[i];

        hResult = cmenu->lpVtbl->InvokeCommand(cmenu, lpcmi);
        if (SUCCEEDED(hResult))
        {
            break;
        }
    }
    TRACE("DoShellExtensions result %x\n", hResult);
    return hResult;
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
            DoDelete(iface, lpcmi->hwnd);
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
            if (LOWORD(lpcmi->lpVerb) >= This->iIdSHEFirst && LOWORD(lpcmi->lpVerb) <= This->iIdSHELast)
            {
                return DoShellExtensions(This, lpcmi);
            }
            FIXME("Unhandled Verb %xl\n",LOWORD(lpcmi->lpVerb));
        }
    }
    else
    {
        TRACE("Verb is %s\n",debugstr_a(lpcmi->lpVerb));
        if (strcmp(lpcmi->lpVerb,"delete")==0)
            DoDelete(iface, lpcmi->hwnd);
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
    LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT) lParam;
	TRACE("(%p)->(msg=%x wp=%lx lp=%lx)\n",This, uMsg, wParam, lParam);

    switch(uMsg)
    {
       case WM_MEASUREITEM:
       case WM_DRAWITEM:
          if (lpmis->itemID >= This->iIdSHEFirst && lpmis->itemID <= This->iIdSHELast)
             return DoCustomItemAction(This, lParam, uMsg);
          break;

    }
   
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

HRESULT
SH_LoadDynamicContextMenuHandler(HKEY hKey, const CLSID * szClass, IContextMenu** ppv, IDataObject * pDataObj, LPITEMIDLIST pidlFolder)
{
  HRESULT hr;
  IContextMenu * cmobj;
  IShellExtInit *shext;
  
  TRACE("SH_LoadDynamicContextMenuHandler entered with %s\n",wine_dbgstr_guid(szClass));

  hr = SHCoCreateInstance(NULL, szClass, NULL, &IID_IContextMenu, (void**)&cmobj);
  if (hr != S_OK)
  {
      TRACE("SHCoCreateInstance failed %x\n", GetLastError());
      return hr;
  }

  hr = cmobj->lpVtbl->QueryInterface(cmobj, &IID_IShellExtInit, (void**)&shext);
  if (hr != S_OK)
  {
      TRACE("Failed to query for interface IID_IShellExtInit\n");
      cmobj->lpVtbl->Release(cmobj);
      return FALSE;
  }
  hr = shext->lpVtbl->Initialize(shext, NULL, pDataObj, hKey);
  if (hr != S_OK)
  {
      TRACE("Failed to initialize shell extension\n");
      shext->lpVtbl->Release(shext);
      cmobj->lpVtbl->Release(cmobj);
  }
  else
  {
      *ppv = cmobj;
  }

  return hr;
}

UINT
SH_EnumerateDynamicContextHandlerForKey(const LPWSTR szFileClass, ItemCmImpl *This, IDataObject * pDataObj, LPITEMIDLIST pidlFolder)
{
   HKEY hKey;
   WCHAR szKey[MAX_PATH] = {0};
   WCHAR szName[MAX_PATH] = {0};
   DWORD dwIndex, dwName;
   LONG res;
   HRESULT hResult;
   IContextMenu * cmobj;
   UINT index;
   CLSID clsid;
   static const WCHAR szShellEx[] = { '\\','s','h','e','l','l','e','x','\\','C','o','n','t','e','x','t','M','e','n','u','H','a','n','d','l','e','r','s',0 };

   wcscpy(szKey, szFileClass);
   wcscat(szKey, szShellEx);

   TRACE("SH_EnumerateDynamicContextHandlerForKey key %s\n", debugstr_w(szFileClass));

   if (RegOpenKeyExW(HKEY_CLASSES_ROOT, szKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
   {
      TRACE("RegOpenKeyExW failed for key %s\n", debugstr_w(szKey));
      return 0;
   }

   dwIndex = 0;
   index = 0;
   do
   {
      dwName = MAX_PATH;
      res = RegEnumKeyExW(hKey, dwIndex, szName, &dwName, NULL, NULL, NULL, NULL);
      if (res == ERROR_SUCCESS)
      {
         hResult = CLSIDFromString(szName, &clsid);
         if (hResult != NOERROR)
         {
             dwName = MAX_PATH;
             if (RegGetValueW(hKey, szName, NULL, RRF_RT_REG_SZ, NULL, szKey, &dwName) == ERROR_SUCCESS)
             {
                 hResult = CLSIDFromString(szKey, &clsid);
             }
         }
         TRACE("hResult %x szKey %s name %s\n",hResult, debugstr_w(szKey), debugstr_w(szName));
         if (hResult == S_OK)
         {
             hResult = SH_LoadDynamicContextMenuHandler(hKey, &clsid, &cmobj, pDataObj, pidlFolder);
         }
         if (hResult == S_OK)
         {
            if (This->ecount + 1 > This->esize)
            {
                if (!SH_EnlargeContextMenuArray(This, max(This->esize * 2, 10)))
                    break;
            }

            This->ecmenu[This->ecount] = cmobj;
            This->ecount++;
         }
      }
      dwIndex++;
   }while(res == ERROR_SUCCESS);

   RegCloseKey(hKey);
   return index;
}

/*************************************************************************
 * SHCreateDefaultContextMenu			[SHELL32.325] Vista API
 *
 */

HRESULT WINAPI SHCreateDefaultContextMenu(
	const DEFCONTEXTMENU *pdcm,
	REFIID riid,
	void **ppv)
{
   HRESULT hr;
   IContextMenu2 * pcm;

   if (pdcm->cidl > 0)
      pcm = ISvItemCm_Constructor( pdcm->psf, pdcm->pidlFolder, pdcm->apidl, pdcm->cidl );
   else
      pcm = ISvBgCm_Constructor( pdcm->psf, TRUE );

   hr = S_OK;
   *ppv = pcm;

   return hr;
}

/*************************************************************************
 * CDefFolderMenu_Create2			[SHELL32.701]
 *
 */

INT CDefFolderMenu_Create2(
	LPCITEMIDLIST pidlFolder,
	HWND hwnd,
	UINT cidl,
	LPCITEMIDLIST *apidl,
	IShellFolder *psf,
	LPFNDFMCALLBACK lpfn,
	UINT nKeys,
	HKEY *ahkeyClsKeys,
	IContextMenu **ppcm)
{
   DEFCONTEXTMENU pdcm;
   HRESULT hr;

   pdcm.hwnd = hwnd;
   pdcm.pcmcb = NULL; //FIXME
   pdcm.pidlFolder = pidlFolder;
   pdcm.psf = psf;
   pdcm.cidl = cidl;
   pdcm.apidl = apidl;
   pdcm.punkAssociationInfo = NULL;
   pdcm.cKeys = nKeys;
   pdcm.aKeys = ahkeyClsKeys;

   hr = SHCreateDefaultContextMenu(&pdcm, &IID_IContextMenu, (void**)ppcm);
   return hr;
}
