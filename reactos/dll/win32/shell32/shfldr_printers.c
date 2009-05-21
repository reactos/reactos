/*
 *    Virtual Printers Folder
 *
 *    Copyright 1997                Marcus Meissner
 *    Copyright 1998, 1999, 2002    Juergen Schmied
 *    Copyright 2005                Huw Davies
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

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL (shell);

/***********************************************************************
 *   Printers_IExtractIconW implementation
 */
typedef struct
{
    IExtractIconWVtbl *lpVtbl;
    IExtractIconAVtbl *lpvtblExtractIconA;
    volatile LONG              ref;
    LPITEMIDLIST       pidl;
} IExtractIconWImpl;

#define _IExtractIconA_Offset ((int)(&(((IExtractIconWImpl*)0)->lpvtblExtractIconA)))
#define _ICOM_THIS_From_IExtractIconA(class, name) class* This = (class*)(((char*)name)-_IExtractIconA_Offset);

static shvheader PrinterSFHeader[] = {
    {IDS_SHV_COLUMN8, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15},
    {IDS_SHV_COLUMN_DOCUMENTS , SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15},
    {IDS_SHV_COLUMN_STATUS, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15},
    {IDS_SHV_COLUMN_COMMENTS, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15},
    {IDS_SHV_COLUMN_LOCATION, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15},
    {IDS_SHV_COLUMN_MODEL, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15}
};

#define COLUMN_NAME          0
#define COLUMN_DOCUMENTS     1
#define COLUMN_STATUS        2
#define COLUMN_COMMENTS      3
#define COLUMN_LOCATION      4
#define COLUMN_MODEL         5


#define PrinterSHELLVIEWCOLUMNS (6)


/**************************************************************************
 *  IExtractIconW_QueryInterface
 */
static HRESULT WINAPI IEI_Printers_fnQueryInterface(IExtractIconW *iface, REFIID riid, LPVOID *ppvObj)
{
    IExtractIconWImpl *This = (IExtractIconWImpl *)iface;

    TRACE("(%p)->(\n\tIID:\t%s,%p)\n", This, debugstr_guid(riid), ppvObj);

    *ppvObj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = This;
    }
    else if (IsEqualIID(riid, &IID_IExtractIconW))
    {
        *ppvObj = (IExtractIconW*)This;
    }
    else if (IsEqualIID(riid, &IID_IExtractIconA))
    {
        *ppvObj = (IExtractIconA*)&(This->lpvtblExtractIconA);
    }

    if(*ppvObj)
    {
        ((IExtractIconW*)*ppvObj)->lpVtbl->AddRef(*ppvObj);
        TRACE("-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
        return S_OK;
    }
    TRACE("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

/**************************************************************************
 *  IExtractIconW_AddRef
 */
static ULONG WINAPI IEI_Printers_fnAddRef(IExtractIconW * iface)
{
    IExtractIconWImpl *This = (IExtractIconWImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(count=%lu)\n", This, refCount - 1);

    return refCount;
}

/**************************************************************************
 *  IExtractIconW_Release
 */
static ULONG WINAPI IEI_Printers_fnRelease(IExtractIconW * iface)
{
    IExtractIconWImpl *This = (IExtractIconWImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(count=%lu)\n", This, refCount + 1);

    if (!refCount)
    {
        TRACE(" destroying IExtractIcon(%p)\n",This);
        SHFree(This->pidl);
        HeapFree(GetProcessHeap(),0,This);
        return 0;
    }
    return refCount;
}

/**************************************************************************
 *  IExtractIconW_GetIconLocation
 *
 * mapping filetype to icon
 */
static HRESULT WINAPI IEI_Printers_fnGetIconLocation(
        IExtractIconW * iface,
        UINT uFlags,		/* GIL_ flags */
        LPWSTR szIconFile,
        UINT cchMax,
        int *piIndex,
        UINT *pwFlags)		/* returned GIL_ flags */
{
    IExtractIconWImpl *This = (IExtractIconWImpl *)iface;

    TRACE("(%p) (flags=%u %p %u %p %p)\n", This, uFlags, szIconFile, cchMax, piIndex, pwFlags);

    if (pwFlags)
        *pwFlags = 0;

    lstrcpynW(szIconFile, swShell32Name, cchMax);
    *piIndex = -IDI_SHELL_PRINTERS_FOLDER; /* FIXME: other icons for default, network, print to file */

    TRACE("-- %s %x\n", debugstr_w(szIconFile), *piIndex);
    return NOERROR;
}

/**************************************************************************
 *  IExtractIconW_Extract
 */
static HRESULT WINAPI IEI_Printers_fnExtract(IExtractIconW *iface, LPCWSTR pszFile,
                                             UINT nIconIndex, HICON *phiconLarge,
                                             HICON *phiconSmall, UINT nIconSize)
{
    IExtractIconWImpl *This = (IExtractIconWImpl *)iface;
    int index;

    FIXME("(%p) (file=%p index=%d %p %p size=%x) semi-stub\n", This, debugstr_w(pszFile),
          (signed)nIconIndex, phiconLarge, phiconSmall, nIconSize);

    index = SIC_GetIconIndex(pszFile, nIconIndex, 0);

    if (phiconLarge)
        *phiconLarge = ImageList_GetIcon(ShellBigIconList, index, ILD_TRANSPARENT);

    if (phiconSmall)
        *phiconSmall = ImageList_GetIcon(ShellSmallIconList, index, ILD_TRANSPARENT);

    return S_OK;
}

static struct IExtractIconWVtbl eivt =
{
    IEI_Printers_fnQueryInterface,
    IEI_Printers_fnAddRef,
    IEI_Printers_fnRelease,
    IEI_Printers_fnGetIconLocation,
    IEI_Printers_fnExtract
};

/**************************************************************************
 *  IExtractIconA_QueryInterface
 */
static HRESULT WINAPI IEIA_Printers_fnQueryInterface(IExtractIconA * iface, REFIID riid, LPVOID *ppvObj)
{
    _ICOM_THIS_From_IExtractIconA(IExtractIconW, iface);

    return This->lpVtbl->QueryInterface(This, riid, ppvObj);
}

/**************************************************************************
 *  IExtractIconA_AddRef
 */
static ULONG WINAPI IEIA_Printers_fnAddRef(IExtractIconA * iface)
{
    _ICOM_THIS_From_IExtractIconA(IExtractIconW, iface);

    return This->lpVtbl->AddRef(This);
}
/**************************************************************************
 *  IExtractIconA_Release
 */
static ULONG WINAPI IEIA_Printers_fnRelease(IExtractIconA * iface)
{
    _ICOM_THIS_From_IExtractIconA(IExtractIconW, iface);

    return This->lpVtbl->AddRef(This);
}

/**************************************************************************
 *  IExtractIconA_GetIconLocation
 */
static HRESULT WINAPI IEIA_Printers_fnGetIconLocation(
	IExtractIconA * iface,
	UINT uFlags,
	LPSTR szIconFile,
	UINT cchMax,
	int * piIndex,
	UINT * pwFlags)
{
    HRESULT ret;
    LPWSTR lpwstrFile = HeapAlloc(GetProcessHeap(), 0, cchMax * sizeof(WCHAR));
    _ICOM_THIS_From_IExtractIconA(IExtractIconW, iface);

    TRACE("(%p) (flags=%u %p %u %p %p)\n", This, uFlags, szIconFile, cchMax, piIndex, pwFlags);

    ret = This->lpVtbl->GetIconLocation(This, uFlags, lpwstrFile, cchMax, piIndex, pwFlags);
    WideCharToMultiByte(CP_ACP, 0, lpwstrFile, -1, szIconFile, cchMax, NULL, NULL);
    HeapFree(GetProcessHeap(), 0, lpwstrFile);

    TRACE("-- %s %x\n", szIconFile, *piIndex);
    return ret;
}
/**************************************************************************
 *  IExtractIconA_Extract
 */
static HRESULT WINAPI IEIA_Printers_fnExtract(IExtractIconA *iface, LPCSTR pszFile,
                                     UINT nIconIndex, HICON *phiconLarge,
                                     HICON *phiconSmall, UINT nIconSize)
{
    HRESULT ret;
    INT len = MultiByteToWideChar(CP_ACP, 0, pszFile, -1, NULL, 0);
    LPWSTR lpwstrFile = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    _ICOM_THIS_From_IExtractIconA(IExtractIconW, iface);

    TRACE("(%p) (file=%p index=%u %p %p size=%u)\n", This, pszFile, nIconIndex, phiconLarge, phiconSmall, nIconSize);

    MultiByteToWideChar(CP_ACP, 0, pszFile, -1, lpwstrFile, len);
    ret = This->lpVtbl->Extract(This, lpwstrFile, nIconIndex, phiconLarge, phiconSmall, nIconSize);
    HeapFree(GetProcessHeap(), 0, lpwstrFile);
    return ret;
}

static struct IExtractIconAVtbl eiavt =
{
    IEIA_Printers_fnQueryInterface,
    IEIA_Printers_fnAddRef,
    IEIA_Printers_fnRelease,
    IEIA_Printers_fnGetIconLocation,
    IEIA_Printers_fnExtract
};

/**************************************************************************
 *  IExtractIcon_Constructor
 */
static IUnknown *IEI_Printers_Constructor(LPCITEMIDLIST pidl)
{
    IExtractIconWImpl* ei;

    TRACE("%p\n", pidl);

    ei = HeapAlloc(GetProcessHeap(),0,sizeof(IExtractIconWImpl));
    ei->ref = 1;
    ei->lpVtbl = &eivt;
    ei->lpvtblExtractIconA = &eiavt;
    ei->pidl=ILClone(pidl);

    pdump(pidl);

    TRACE("(%p)\n", ei);
    return (IUnknown *)ei;
}

/***********************************************************************
 *     Printers folder implementation
 */

typedef struct {
    IShellFolder2Vtbl *lpVtbl;
    IPersistFolder2Vtbl *lpVtblPersistFolder2;

    LONG ref;

    CLSID *pclsid;

    LPITEMIDLIST pidlRoot;  /* absolute pidl */

    int dwAttributes;        /* attributes returned by GetAttributesOf FIXME: use it */
} IGenericSFImpl;

#define _IPersistFolder2_Offset ((int)(&(((IGenericSFImpl*)0)->lpVtblPersistFolder2)))
#define _ICOM_THIS_From_IPersistFolder2(class, name) class* This = (class*)(((char*)name)-_IPersistFolder2_Offset);

#define _IUnknown_(This)    (IShellFolder*)&(This->lpVtbl)
#define _IShellFolder_(This)    (IShellFolder*)&(This->lpVtbl)

#define _IPersist_(This)    (IPersist*)&(This->lpVtblPersistFolder2)
#define _IPersistFolder_(This)    (IPersistFolder*)&(This->lpVtblPersistFolder2)
#define _IPersistFolder2_(This)    (IPersistFolder2*)&(This->lpVtblPersistFolder2)

/**************************************************************************
 *    ISF_Printers_fnQueryInterface
 *
 * NOTE does not support IPersist/IPersistFolder
 */
static HRESULT WINAPI ISF_Printers_fnQueryInterface(
                IShellFolder2 * iface, REFIID riid, LPVOID * ppvObj)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    TRACE ("(%p)->(%s,%p)\n", This, shdebugstr_guid (riid), ppvObj);

    *ppvObj = NULL;

    if (IsEqualIID (riid, &IID_IUnknown) ||
        IsEqualIID (riid, &IID_IShellFolder) ||
        IsEqualIID (riid, &IID_IShellFolder2))
    {
        *ppvObj = This;
    }

    else if (IsEqualIID (riid, &IID_IPersist) ||
             IsEqualIID (riid, &IID_IPersistFolder) ||
             IsEqualIID (riid, &IID_IPersistFolder2))
    {
        *ppvObj = _IPersistFolder2_ (This);
    }

    if (*ppvObj)
    {
        IUnknown_AddRef ((IUnknown *) (*ppvObj));
        TRACE ("-- Interface: (%p)->(%p)\n", ppvObj, *ppvObj);
        return S_OK;
    }
    TRACE ("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ISF_Printers_fnAddRef (IShellFolder2 * iface)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE ("(%p)->(count=%lu)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI ISF_Printers_fnRelease (IShellFolder2 * iface)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE ("(%p)->(count=%lu)\n", This, refCount + 1);

    if (!refCount)
    {
        TRACE ("-- destroying IShellFolder(%p)\n", This);
        if (This->pidlRoot)
            SHFree (This->pidlRoot);
        LocalFree ((HLOCAL) This);
        return 0;
    }
    return refCount;
}

/**************************************************************************
 *    ISF_Printers_fnParseDisplayName
 *
 * This is E_NOTIMPL in Windows too.
 */
static HRESULT WINAPI ISF_Printers_fnParseDisplayName (IShellFolder2 * iface,
                HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName,
                DWORD * pchEaten, LPITEMIDLIST * ppidl, DWORD * pdwAttributes)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    TRACE("(%p)->(HWND=%p,%p,%p=%s,%p,pidl=%p,%p)\n",
           This, hwndOwner, pbc, lpszDisplayName, debugstr_w(lpszDisplayName),
           pchEaten, ppidl, pdwAttributes);

    *ppidl = 0;
    if (pchEaten)
        *pchEaten = 0;

    return E_NOTIMPL;
}

static LPITEMIDLIST _ILCreatePrinterItem(PRINTER_INFO_4W *pi)
{
    PIDLDATA tmp;
    LPITEMIDLIST pidl;
    PIDLPrinterStruct * p;
    int size0 = (char*)&tmp.u.cprinter.szName-(char*)&tmp.u.cprinter;
    int size = size0;

    tmp.type = 0x00;
    tmp.u.cprinter.dummy = 0xFF;
    if (pi->pPrinterName)
        tmp.u.cprinter.offsServer = wcslen(pi->pPrinterName) + 1;
    else
        tmp.u.cprinter.offsServer = 1;

    size += tmp.u.cprinter.offsServer * sizeof(WCHAR);
    if (pi->pServerName)
        size += ( + wcslen(pi->pServerName) + 1) * sizeof(WCHAR);
    else
        size += sizeof(WCHAR);

    pidl = (LPITEMIDLIST)SHAlloc(size + 4);
    if (!pidl)
        return pidl;

    pidl->mkid.cb = size+2;
    memcpy(pidl->mkid.abID, &tmp, 2+size0);

    p = &((PIDLDATA*)pidl->mkid.abID)->u.cprinter;

    p->Attributes = pi->Attributes;
    if (pi->pPrinterName)
        wcscpy(p->szName, pi->pPrinterName);
    else
        p->szName[0] = L'\0';

    if (pi->pServerName)
        wcscpy(p->szName + p->offsServer, pi->pServerName);
    else
        p->szName[p->offsServer] = L'\0';

    *(WORD*)((char*)pidl+(size+2)) = 0;
    return pidl;
}

static PIDLPrinterStruct * _ILGetPrinterStruct(LPCITEMIDLIST pidl)
{
    LPPIDLDATA pdata = _ILGetDataPointer(pidl);

    if (pdata && pdata->type==0x00)
        return (PIDLPrinterStruct*)&(pdata->u.cfont);

    return NULL;
}

/**************************************************************************
 *  CreatePrintersEnumList()
 */
static BOOL CreatePrintersEnumList(IEnumIDList *list, DWORD dwFlags)
{
    BOOL ret = TRUE;

    TRACE("(%p)->(flags=0x%08lx) \n",list,dwFlags);

    /* enumerate the folders */
    if (dwFlags & SHCONTF_NONFOLDERS)
    {
        DWORD needed = 0, num = 0, i;
        PRINTER_INFO_4W *pi;

        EnumPrintersW(PRINTER_ENUM_LOCAL, NULL, 4, NULL, 0, &needed, &num);
        if (!needed)
            return ret;

        pi = HeapAlloc(GetProcessHeap(), 0, needed);
        if(!EnumPrintersW(PRINTER_ENUM_LOCAL, NULL, 4, (LPBYTE)pi, needed, &needed, &num)) {
            HeapFree(GetProcessHeap(), 0, pi);
            return FALSE;
        }

        for(i = 0; i < num; i++) {
            LPITEMIDLIST pidl = _ILCreatePrinterItem(&pi[i]);
            if (pidl)
            {
                if (!AddToEnumList(list, pidl))
                    SHFree(pidl);
            }
        }
        HeapFree(GetProcessHeap(), 0, pi);
    }
    return ret;
}

/**************************************************************************
 *        ISF_Printers_fnEnumObjects
 */
static HRESULT WINAPI ISF_Printers_fnEnumObjects (IShellFolder2 * iface,
                HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST * ppEnumIDList)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    TRACE ("(%p)->(HWND=%p flags=0x%08lx pplist=%p)\n",
           This, hwndOwner, dwFlags, ppEnumIDList);

    if(!ppEnumIDList) return E_OUTOFMEMORY;
    *ppEnumIDList = IEnumIDList_Constructor();
    if (*ppEnumIDList)
        CreatePrintersEnumList(*ppEnumIDList, dwFlags);

    TRACE ("-- (%p)->(new ID List: %p)\n", This, *ppEnumIDList);

    return (*ppEnumIDList) ? S_OK : E_OUTOFMEMORY;
}

/**************************************************************************
 *        ISF_Printers_fnBindToObject
 */
static HRESULT WINAPI ISF_Printers_fnBindToObject (IShellFolder2 * iface,
                LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    TRACE ("(%p)->(pidl=%p,%p,%s,%p)\n", This,
            pidl, pbcReserved, shdebugstr_guid (riid), ppvOut);

    return SHELL32_BindToChild (This->pidlRoot, NULL, pidl, riid, ppvOut);
}

/**************************************************************************
 *    ISF_Printers_fnBindToStorage
 */
static HRESULT WINAPI ISF_Printers_fnBindToStorage (IShellFolder2 * iface,
                LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    FIXME ("(%p)->(pidl=%p,%p,%s,%p) stub\n",
           This, pidl, pbcReserved, shdebugstr_guid (riid), ppvOut);

    *ppvOut = NULL;
    return E_NOTIMPL;
}

/**************************************************************************
 *     ISF_Printers_fnCompareIDs
 */
static HRESULT WINAPI ISF_Printers_fnCompareIDs (IShellFolder2 * iface,
                        LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    int nReturn;

    TRACE ("(%p)->(0x%08lx,pidl1=%p,pidl2=%p)\n", This, lParam, pidl1, pidl2);
    nReturn = SHELL32_CompareIDs (_IShellFolder_ (This), lParam, pidl1, pidl2);
    TRACE ("-- %i\n", nReturn);
    return nReturn;
}

/**************************************************************************
 *    ISF_Printers_fnCreateViewObject
 */
static HRESULT WINAPI ISF_Printers_fnCreateViewObject (IShellFolder2 * iface,
                              HWND hwndOwner, REFIID riid, LPVOID * ppvOut)
{

    LPSHELLVIEW pShellView;
    HRESULT hr = E_INVALIDARG;
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    TRACE ("(%p)->(hwnd=%p,%s,%p)\n", This,
            hwndOwner, shdebugstr_guid (riid), ppvOut);

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if (IsEqualIID (riid, &IID_IDropTarget))
    {
        WARN ("IDropTarget not implemented\n");
        hr = E_NOTIMPL;
    }
    else if (IsEqualIID (riid, &IID_IContextMenu))
    {
        WARN ("IContextMenu not implemented\n");
        hr = E_NOTIMPL;
    }
    else if (IsEqualIID (riid, &IID_IShellView))
    {
        pShellView = IShellView_Constructor ((IShellFolder *) iface);
        if (pShellView)
        {
            hr = IShellView_QueryInterface (pShellView, riid, ppvOut);
            IShellView_Release (pShellView);
        }
    }
    TRACE ("-- (%p)->(interface=%p)\n", This, ppvOut);
    return hr;
}

/**************************************************************************
 *  ISF_Printers_fnGetAttributesOf
 */
static HRESULT WINAPI ISF_Printers_fnGetAttributesOf (IShellFolder2 * iface,
                UINT cidl, LPCITEMIDLIST * apidl, DWORD * rgfInOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    FIXME ("(%p)->(cidl=%d apidl=%p mask=0x%08lx): stub\n",
           This, cidl, apidl, *rgfInOut);

    return E_NOTIMPL;
}

/**************************************************************************
 *    ISF_Printers_fnGetUIObjectOf
 *
 * PARAMETERS
 *  HWND           hwndOwner, //[in ] Parent window for any output
 *  UINT           cidl,      //[in ] array size
 *  LPCITEMIDLIST* apidl,     //[in ] simple pidl array
 *  REFIID         riid,      //[in ] Requested Interface
 *  UINT*          prgfInOut, //[   ] reserved
 *  LPVOID*        ppvObject) //[out] Resulting Interface
 *
 */
static HRESULT WINAPI ISF_Printers_fnGetUIObjectOf (IShellFolder2 * iface,
                HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl,
                REFIID riid, UINT * prgfInOut, LPVOID * ppvOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    IUnknown *pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(%p,%u,apidl=%p,%s,%p,%p)\n",
       This, hwndOwner, cidl, apidl, shdebugstr_guid (riid), prgfInOut, ppvOut);

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if ((IsEqualIID (riid, &IID_IExtractIconA) || IsEqualIID(riid, &IID_IExtractIconW)) && (cidl == 1))
    {
        IUnknown *pUnk = IEI_Printers_Constructor(apidl[0]);
        hr = IUnknown_QueryInterface(pUnk, riid, (void**)&pObj);
        IUnknown_Release(pUnk);
    }
    else
        hr = E_NOINTERFACE;

    if (SUCCEEDED(hr) && !pObj)
        hr = E_OUTOFMEMORY;

    *ppvOut = pObj;
    TRACE ("(%p)->hr=0x%08lx\n", This, hr);
    return hr;
}

/**************************************************************************
 *    ISF_Printers_fnGetDisplayNameOf
 *
 */
static HRESULT WINAPI ISF_Printers_fnGetDisplayNameOf (IShellFolder2 * iface,
                LPCITEMIDLIST pidl, DWORD dwFlags, LPSTRRET strRet)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    PIDLPrinterStruct * p;

    TRACE ("(%p)->(pidl=%p,0x%08lx,%p)\n", This, pidl, dwFlags, strRet);
    pdump (pidl);

    if (!strRet)
        return E_INVALIDARG;

    p = _ILGetPrinterStruct(pidl);
    if (!p)
        return E_INVALIDARG;

    strRet->u.pOleStr = SHAlloc(p->offsServer * sizeof(WCHAR));
    if (!strRet->u.pOleStr)
        return E_OUTOFMEMORY;

    memcpy((LPVOID)strRet->u.pOleStr, (LPVOID)p->szName, p->offsServer * sizeof(WCHAR));
    TRACE("ret %s\n", debugstr_w(strRet->u.pOleStr));

    return S_OK;
}

/**************************************************************************
 *  ISF_Printers_fnSetNameOf
 *  Changes the name of a file object or subfolder, possibly changing its item
 *  identifier in the process.
 *
 * PARAMETERS
 *  HWND          hwndOwner,  //[in ] Owner window for output
 *  LPCITEMIDLIST pidl,       //[in ] simple pidl of item to change
 *  LPCOLESTR     lpszName,   //[in ] the items new display name
 *  DWORD         dwFlags,    //[in ] SHGNO formatting flags
 *  LPITEMIDLIST* ppidlOut)   //[out] simple pidl returned
 */
static HRESULT WINAPI ISF_Printers_fnSetNameOf (IShellFolder2 * iface,
                HWND hwndOwner, LPCITEMIDLIST pidl,    /* simple pidl */
                LPCOLESTR lpName, DWORD dwFlags, LPITEMIDLIST * pPidlOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    FIXME ("(%p)->(%p,pidl=%p,%s,%lu,%p)\n", This, hwndOwner, pidl,
           debugstr_w (lpName), dwFlags, pPidlOut);

    return E_FAIL;
}

static HRESULT WINAPI ISF_Printers_fnGetDefaultSearchGUID(IShellFolder2 *iface,
                GUID * pguid)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_Printers_fnEnumSearches (IShellFolder2 *iface,
                IEnumExtraSearch ** ppenum)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_Printers_fnGetDefaultColumn (IShellFolder2 * iface,
                DWORD dwRes, ULONG * pSort, ULONG * pDisplay)
{
    if (pSort)
        *pSort = 0;
    if (pDisplay)
        *pDisplay = 0;

    return S_OK;
}
static HRESULT WINAPI ISF_Printers_fnGetDefaultColumnState (
                IShellFolder2 * iface, UINT iColumn, DWORD * pcsFlags)
{
    if (!pcsFlags || iColumn >= PrinterSHELLVIEWCOLUMNS)
        return E_INVALIDARG;
    *pcsFlags = PrinterSFHeader[iColumn].pcsFlags;
    return S_OK;

}

static HRESULT WINAPI ISF_Printers_fnGetDetailsEx (IShellFolder2 * iface,
                LPCITEMIDLIST pidl, const SHCOLUMNID * pscid, VARIANT * pv)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    FIXME ("(%p): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_Printers_fnGetDetailsOf (IShellFolder2 * iface,
                LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS * psd)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    WCHAR buffer[MAX_PATH] = {0};
    HRESULT hr = E_FAIL;

    TRACE("(%p)->(%p %i %p): stub\n", This, pidl, iColumn, psd);

    if (iColumn >= PrinterSHELLVIEWCOLUMNS)
        return E_FAIL;

    psd->fmt = PrinterSFHeader[iColumn].fmt;
    psd->cxChar = PrinterSFHeader[iColumn].cxChar;
    if (pidl == NULL)
    {
        psd->str.uType = STRRET_WSTR;
        if (LoadStringW(shell32_hInstance, PrinterSFHeader[iColumn].colnameid, buffer, MAX_PATH))
            hr = SHStrDupW(buffer, &psd->str.u.pOleStr);

        return hr;
    }

    if (iColumn == COLUMN_NAME)
    {
        psd->str.uType = STRRET_WSTR;
        return IShellFolder2_GetDisplayNameOf(iface, pidl, SHGDN_NORMAL, &psd->str);
    }

    psd->str.uType = STRRET_CSTR;
    psd->str.u.cStr[0] = '\0';

    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_Printers_fnMapColumnToSCID (
                IShellFolder2 * iface, UINT column, SHCOLUMNID * pscid)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    FIXME ("(%p): stub\n", This);
    return E_NOTIMPL;
}

static IShellFolder2Vtbl vt_ShellFolder2 =
{
    ISF_Printers_fnQueryInterface,
    ISF_Printers_fnAddRef,
    ISF_Printers_fnRelease,
    ISF_Printers_fnParseDisplayName,
    ISF_Printers_fnEnumObjects,
    ISF_Printers_fnBindToObject,
    ISF_Printers_fnBindToStorage,
    ISF_Printers_fnCompareIDs,
    ISF_Printers_fnCreateViewObject,
    ISF_Printers_fnGetAttributesOf,
    ISF_Printers_fnGetUIObjectOf,
    ISF_Printers_fnGetDisplayNameOf,
    ISF_Printers_fnSetNameOf,
    /* ShellFolder2 */
    ISF_Printers_fnGetDefaultSearchGUID,
    ISF_Printers_fnEnumSearches,
    ISF_Printers_fnGetDefaultColumn,
    ISF_Printers_fnGetDefaultColumnState,
    ISF_Printers_fnGetDetailsEx,
    ISF_Printers_fnGetDetailsOf,
    ISF_Printers_fnMapColumnToSCID
};

/************************************************************************
 *    IPF_Printers_QueryInterface
 */
static HRESULT WINAPI IPF_Printers_QueryInterface (
               IPersistFolder2 * iface, REFIID iid, LPVOID * ppvObj)
{
    _ICOM_THIS_From_IPersistFolder2 (IGenericSFImpl, iface);

    TRACE ("(%p)\n", This);

    return IUnknown_QueryInterface (_IUnknown_ (This), iid, ppvObj);
}

/************************************************************************
 *    IPF_Printers_AddRef
 */
static ULONG WINAPI IPF_Printers_AddRef (IPersistFolder2 * iface)
{
    _ICOM_THIS_From_IPersistFolder2 (IGenericSFImpl, iface);

    TRACE ("(%p)->(count=%lu)\n", This, This->ref);

    return IUnknown_AddRef (_IUnknown_ (This));
}

/************************************************************************
 *    IPF_Printers_Release
 */
static ULONG WINAPI IPF_Printers_Release (IPersistFolder2 * iface)
{
    _ICOM_THIS_From_IPersistFolder2 (IGenericSFImpl, iface);

    TRACE ("(%p)->(count=%lu)\n", This, This->ref);

    return IUnknown_Release (_IUnknown_ (This));
}

/************************************************************************
 *    IPF_Printers_GetClassID
 */
static HRESULT WINAPI IPF_Printers_GetClassID (
               IPersistFolder2 * iface, CLSID * lpClassId)
{
    _ICOM_THIS_From_IPersistFolder2 (IGenericSFImpl, iface);

    TRACE ("(%p)\n", This);

    *lpClassId = CLSID_Printers;

    return S_OK;
}

/************************************************************************
 *    IPF_Printers_Initialize
 *
 */
static HRESULT WINAPI IPF_Printers_Initialize (
               IPersistFolder2 * iface, LPCITEMIDLIST pidl)
{
    _ICOM_THIS_From_IPersistFolder2 (IGenericSFImpl, iface);
    if (This->pidlRoot)
        SHFree((LPVOID)This->pidlRoot);

    This->pidlRoot = ILClone(pidl);
    return S_OK;
}

/**************************************************************************
 *    IPF_Printers_fnGetCurFolder
 */
static HRESULT WINAPI IPF_Printers_GetCurFolder (
               IPersistFolder2 * iface, LPITEMIDLIST * pidl)
{
    _ICOM_THIS_From_IPersistFolder2 (IGenericSFImpl, iface);

    TRACE ("(%p)->(%p)\n", This, pidl);

    *pidl = ILClone (This->pidlRoot);
    return S_OK;
}

static IPersistFolder2Vtbl vt_PersistFolder2 =
{
    IPF_Printers_QueryInterface,
    IPF_Printers_AddRef,
    IPF_Printers_Release,
    IPF_Printers_GetClassID,
    IPF_Printers_Initialize,
    IPF_Printers_GetCurFolder
};

/**************************************************************************
 *    ISF_Printers_Constructor
 */
HRESULT WINAPI ISF_Printers_Constructor (
                IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv)
{
    IGenericSFImpl *sf;
    HRESULT hr;

    TRACE ("unkOut=%p %s\n", pUnkOuter, shdebugstr_guid (riid));

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    sf = HeapAlloc( GetProcessHeap(), 0, sizeof(*sf) );
    if (!sf)
        return E_OUTOFMEMORY;

    sf->ref = 1;
    sf->lpVtbl = &vt_ShellFolder2;
    sf->lpVtblPersistFolder2 = &vt_PersistFolder2;

    sf->pidlRoot = _ILCreatePrinters();    /* my qualified pidl */

    hr = IUnknown_QueryInterface( _IUnknown_(sf), riid, ppv );
    IUnknown_Release( _IUnknown_(sf) );

    TRACE ("--(%p)\n", *ppv);
    return hr;
}
