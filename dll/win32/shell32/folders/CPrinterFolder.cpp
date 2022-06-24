/*
 *    Virtual Printers Folder
 *
 *    Copyright 1997                Marcus Meissner
 *    Copyright 1998, 1999, 2002    Juergen Schmied
 *    Copyright 2005                Huw Davies
 *    Copyright 2009                Andrew Hill
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <precomp.h>

#include <winspool.h>

WINE_DEFAULT_DEBUG_CHANNEL (shell);

static shvheader PrinterSFHeader[] = {
    {IDS_SHV_COLUMN_NAME, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 15},
    {IDS_SHV_COLUMN_DOCUMENTS , SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 15},
    {IDS_SHV_COLUMN_STATUS, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 15},
    {IDS_SHV_COLUMN_COMMENTS, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 15},
    {IDS_SHV_COLUMN_LOCATION, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 15},
    {IDS_SHV_COLUMN_MODEL, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 15}
};

#define COLUMN_NAME          0
#define COLUMN_DOCUMENTS     1
#define COLUMN_STATUS        2
#define COLUMN_COMMENTS      3
#define COLUMN_LOCATION      4
#define COLUMN_MODEL         5

#define PrinterSHELLVIEWCOLUMNS (6)

/**************************************************************************
 *  CPrintersExtractIconW_CreateInstane
 *
 *  There is no CPrintersExtractIconW. We just initialize CExtractIcon properly to do our job.
 */
HRESULT WINAPI CPrintersExtractIconW_CreateInstane(LPCITEMIDLIST pidl, REFIID riid, LPVOID *ppv)
{
    CComPtr<IDefaultExtractIconInit> initIcon;
    HRESULT hr = SHCreateDefaultExtractIcon(IID_PPV_ARG(IDefaultExtractIconInit,&initIcon));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    /* FIXME: other icons for default, network, print to file */
    initIcon->SetNormalIcon(swShell32Name, -IDI_SHELL_PRINTER);

    return initIcon->QueryInterface(riid,ppv);
}

/***********************************************************************
 *     Printers folder implementation
 */

class CPrintersEnum: public CEnumIDListBase
{
    public:
        CPrintersEnum();
        ~CPrintersEnum();
        HRESULT WINAPI Initialize(HWND hwndOwner, DWORD dwFlags);
        BOOL CreatePrintersEnumList(DWORD dwFlags);

        BEGIN_COM_MAP(CPrintersEnum)
        COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
        END_COM_MAP()
};

CPrintersEnum::CPrintersEnum()
{
}

CPrintersEnum::~CPrintersEnum()
{
}

HRESULT WINAPI CPrintersEnum::Initialize(HWND hwndOwner, DWORD dwFlags)
{
    if (CreatePrintersEnumList(dwFlags) == FALSE)
        return E_FAIL;
    return S_OK;
}

static LPITEMIDLIST _ILCreatePrinterItem(PRINTER_INFO_4W *pi)
{
    PIDLDATA tmp;
    LPITEMIDLIST pidl;
    PIDLPrinterStruct * p;
    int size0 = (char*)&tmp.u.cprinter.szName - (char*)&tmp.u.cprinter;
    int size = size0;
    SIZE_T cchPrinterName = 0;
    SIZE_T cchServerName = 0;

    if (pi->pPrinterName)
        cchPrinterName = wcslen(pi->pPrinterName);
    if (pi->pServerName)
        cchServerName = wcslen(pi->pServerName);
    if ((cchPrinterName + cchServerName) > (MAXUSHORT - 2))
    {
        return NULL;
    }

    tmp.type = 0x00;
    tmp.u.cprinter.dummy = 0xFF;
    if (pi->pPrinterName)
        tmp.u.cprinter.offsServer = cchPrinterName + 1;
    else
        tmp.u.cprinter.offsServer = 1;

    size += tmp.u.cprinter.offsServer * sizeof(WCHAR);
    if (pi->pServerName)
        size += (cchServerName + 1) * sizeof(WCHAR);
    else
        size += sizeof(WCHAR);

    pidl = (LPITEMIDLIST)SHAlloc(size + 4);
    if (!pidl)
        return pidl;

    pidl->mkid.cb = size + 2;
    memcpy(pidl->mkid.abID, &tmp, 2 + size0);

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

    *(WORD*)((char*)pidl + (size + 2)) = 0;
    return pidl;
}

/**************************************************************************
 *  CPrintersEnum::CreatePrintersEnumList()
 */
BOOL CPrintersEnum::CreatePrintersEnumList(DWORD dwFlags)
{
    BOOL ret = TRUE;

    TRACE("(%p)->(flags=0x%08lx) \n", this, dwFlags);

    /* enumerate the folders */
    if (dwFlags & SHCONTF_NONFOLDERS)
    {
        DWORD needed = 0, num = 0, i;
        PRINTER_INFO_4W *pi;

        EnumPrintersW(PRINTER_ENUM_LOCAL, NULL, 4, NULL, 0, &needed, &num);
        if (!needed)
            return ret;

        pi = (PRINTER_INFO_4W *)HeapAlloc(GetProcessHeap(), 0, needed);
        if(!EnumPrintersW(PRINTER_ENUM_LOCAL, NULL, 4, (LPBYTE)pi, needed, &needed, &num)) {
            HeapFree(GetProcessHeap(), 0, pi);
            return FALSE;
        }

        for(i = 0; i < num; i++) {
            LPITEMIDLIST pidl = _ILCreatePrinterItem(&pi[i]);
            if (pidl)
            {
                if (!AddToEnumList(pidl))
                    SHFree(pidl);
            }
        }
        HeapFree(GetProcessHeap(), 0, pi);
    }
    return ret;
}

CPrinterFolder::CPrinterFolder()
{
    pidlRoot = NULL;
    dwAttributes = 0;
    pclsid = NULL;
}

CPrinterFolder::~CPrinterFolder()
{
    TRACE("-- destroying IShellFolder(%p)\n", this);
    if (pidlRoot)
        SHFree(pidlRoot);
}

/**************************************************************************
 *    CPrinterFolder::ParseDisplayName
 *
 * This is E_NOTIMPL in Windows too.
 */
HRESULT WINAPI CPrinterFolder::ParseDisplayName(HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName,
        DWORD *pchEaten, PIDLIST_RELATIVE *ppidl, DWORD *pdwAttributes)
{
    TRACE("(%p)->(HWND=%p,%p,%p=%s,%p,pidl=%p,%p)\n",
          this, hwndOwner, pbc, lpszDisplayName, debugstr_w(lpszDisplayName),
          pchEaten, ppidl, pdwAttributes);

    *ppidl = 0;
    if (pchEaten)
        *pchEaten = 0;

    return E_NOTIMPL;
}

static PIDLPrinterStruct * _ILGetPrinterStruct(LPCITEMIDLIST pidl)
{
    LPPIDLDATA pdata = _ILGetDataPointer(pidl);

    if (pdata && pdata->type == 0x00)
        return (PIDLPrinterStruct*) & (pdata->u.cfont);

    return NULL;
}

/**************************************************************************
 *        CPrinterFolder::EnumObjects
 */
HRESULT WINAPI CPrinterFolder::EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST * ppEnumIDList)
{
    return ShellObjectCreatorInit<CPrintersEnum>(hwndOwner, dwFlags, IID_PPV_ARG(IEnumIDList, ppEnumIDList));
}

/**************************************************************************
 *        CPrinterFolder::BindToObject
 */
HRESULT WINAPI CPrinterFolder::BindToObject(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    return E_NOTIMPL;
}

/**************************************************************************
 *    ISF_Printers_fnBindToStorage
 */
HRESULT WINAPI CPrinterFolder::BindToStorage(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    FIXME ("(%p)->(pidl=%p,%p,%s,%p) stub\n",
           this, pidl, pbcReserved, shdebugstr_guid (&riid), ppvOut);

    *ppvOut = NULL;
    return E_NOTIMPL;
}

/**************************************************************************
 *     CPrinterFolder::CompareIDs
 */
HRESULT WINAPI CPrinterFolder::CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
{
    return SHELL32_CompareDetails(this, lParam, pidl1, pidl2);
}

/**************************************************************************
 *    CPrinterFolder::CreateViewObject
 */
HRESULT WINAPI CPrinterFolder::CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID * ppvOut)
{
    CComPtr<IShellView> pShellView;
    HRESULT hr = E_INVALIDARG;

    TRACE("(%p)->(hwnd=%p,%s,%p)\n", this,
          hwndOwner, shdebugstr_guid (&riid), ppvOut);

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if (IsEqualIID(riid, IID_IDropTarget))
    {
        WARN("IDropTarget not implemented\n");
        hr = E_NOTIMPL;
    }
    else if(IsEqualIID(riid, IID_IContextMenu))
    {
        WARN("IContextMenu not implemented\n");
        hr = E_NOTIMPL;
    }
    else if(IsEqualIID(riid, IID_IShellView))
    {
        SFV_CREATE sfvparams = {sizeof(SFV_CREATE), this};
        hr = SHCreateShellFolderView(&sfvparams, (IShellView**)ppvOut);
    }
    TRACE ("-- (%p)->(interface=%p)\n", this, ppvOut);
    return hr;
}

/**************************************************************************
 *  CPrinterFolder::GetAttributesOf
 */
HRESULT WINAPI CPrinterFolder::GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD *rgfInOut)
{
    static const DWORD dwPrintersAttributes =
        SFGAO_HASPROPSHEET | SFGAO_STORAGEANCESTOR | SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_CANRENAME | SFGAO_CANDELETE;
    HRESULT hr = S_OK;

    FIXME ("(%p)->(cidl=%d apidl=%p mask=0x%08lx): stub\n",
           this, cidl, apidl, *rgfInOut);

    *rgfInOut &= dwPrintersAttributes;

    *rgfInOut &= ~SFGAO_VALIDATE;

    TRACE ("-- result=0x%08x\n", *rgfInOut);
    return hr;
}

/**************************************************************************
 *    CPrinterFolder::GetUIObjectOf
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
HRESULT WINAPI CPrinterFolder::GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl,
        REFIID riid, UINT * prgfInOut, LPVOID * ppvOut)
{
    LPVOID pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(%p,%u,apidl=%p,%s,%p,%p)\n",
           this, hwndOwner, cidl, apidl, shdebugstr_guid (&riid), prgfInOut, ppvOut);

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if ((IsEqualIID (riid, IID_IExtractIconA) || IsEqualIID(riid, IID_IExtractIconW)) && cidl == 1)
        hr = CPrintersExtractIconW_CreateInstane(apidl[0], riid, &pObj);
    else
        hr = E_NOINTERFACE;

    if (SUCCEEDED(hr) && !pObj)
        hr = E_OUTOFMEMORY;

    *ppvOut = pObj;
    TRACE ("(%p)->hr=0x%08lx\n", this, hr);
    return hr;
}

/**************************************************************************
 *    CPrinterFolder::GetDisplayNameOf
 *
 */
HRESULT WINAPI CPrinterFolder::GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet)
{
    PIDLPrinterStruct * p;

    TRACE ("(%p)->(pidl=%p,0x%08lx,%p)\n", this, pidl, dwFlags, strRet);
    pdump (pidl);

    if (!strRet)
    {
        WARN("no strRet\n");
        return E_INVALIDARG;
    }

    p = _ILGetPrinterStruct(pidl);
    if (!p)
    {
        ERR("no printer struct\n");
        return E_INVALIDARG;
    }

    return SHSetStrRet(strRet, p->szName);
}

/**************************************************************************
 *  CPrinterFolder::SetNameOf
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
HRESULT WINAPI CPrinterFolder::SetNameOf(HWND hwndOwner, PCUITEMID_CHILD pidl,    /* simple pidl */
        LPCOLESTR lpName, DWORD dwFlags, PITEMID_CHILD *pPidlOut)
{
    FIXME("(%p)->(%p,pidl=%p,%s,%lu,%p)\n", this, hwndOwner, pidl,
          debugstr_w (lpName), dwFlags, pPidlOut);

    return E_FAIL;
}

HRESULT WINAPI CPrinterFolder::GetDefaultSearchGUID(GUID *pguid)
{
    FIXME("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CPrinterFolder::EnumSearches(IEnumExtraSearch **ppenum)
{
    FIXME("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CPrinterFolder::GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    if (pSort)
        *pSort = 0;
    if (pDisplay)
        *pDisplay = 0;

    return S_OK;
}

HRESULT WINAPI CPrinterFolder::GetDefaultColumnState(UINT iColumn, DWORD *pcsFlags)
{
    if (!pcsFlags || iColumn >= PrinterSHELLVIEWCOLUMNS)
        return E_INVALIDARG;
    *pcsFlags = PrinterSFHeader[iColumn].pcsFlags;
    return S_OK;

}

HRESULT WINAPI CPrinterFolder::GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    FIXME("(%p): stub\n", this);

    return E_NOTIMPL;
}

HRESULT WINAPI CPrinterFolder::GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd)
{
    TRACE("(%p)->(%p %i %p): stub\n", this, pidl, iColumn, psd);

    if (iColumn >= PrinterSHELLVIEWCOLUMNS)
        return E_FAIL;

    psd->fmt = PrinterSFHeader[iColumn].fmt;
    psd->cxChar = PrinterSFHeader[iColumn].cxChar;
    if (pidl == NULL)
        return SHSetStrRet(&psd->str, PrinterSFHeader[iColumn].colnameid);

    if (iColumn == COLUMN_NAME)
        return GetDisplayNameOf(pidl, SHGDN_NORMAL, &psd->str);

    psd->str.uType = STRRET_CSTR;
    psd->str.cStr[0] = '\0';

    return E_NOTIMPL;
}

HRESULT WINAPI CPrinterFolder::MapColumnToSCID(UINT column, SHCOLUMNID *pscid)
{
    FIXME ("(%p): stub\n", this);
    return E_NOTIMPL;
}

/************************************************************************
 *    CPrinterFolder::GetClassID
 */
HRESULT WINAPI CPrinterFolder::GetClassID(CLSID *lpClassId)
{
    TRACE ("(%p)\n", this);

    *lpClassId = CLSID_Printers;

    return S_OK;
}

/************************************************************************
 *    CPrinterFolder::Initialize
 */
HRESULT WINAPI CPrinterFolder::Initialize(PCIDLIST_ABSOLUTE pidl)
{
    if (pidlRoot)
        SHFree((LPVOID)pidlRoot);

    pidlRoot = ILClone(pidl);
    return S_OK;
}

/**************************************************************************
 *    CPrinterFolder::GetCurFolder
 */
HRESULT WINAPI CPrinterFolder::GetCurFolder(PIDLIST_ABSOLUTE * pidl)
{
    TRACE ("(%p)->(%p)\n", this, pidl);

    *pidl = ILClone (pidlRoot);
    return S_OK;
}
