#include "shellprv.h"
#pragma  hdrstop

#include "printer.h"
#include <shguidp.h>
#include "fstreex.h"
#include "netview.h"

#include "deskfldr.h"
#include "datautil.h"

// Internal function prototypes

DWORD _OpenEnumRetry(HWND hwnd, DWORD dwType, LPNETRESOURCE pnr, HANDLE *phEnum);

HRESULT CNETIDLData_GetNetResource(IDataObject *pdtobj, STGMEDIUM *pmedium);
HRESULT CNETIDLData_GetHDrop(IDataObject *pdtobj, FORMATETC* pformatetcIn, STGMEDIUM *pmedium);

LPTSTR NET_GetProviderFromRes(LPCIDNETRESOURCE pidn, LPTSTR pszBuff, UINT cchBuff);

//
// By the way...Win95 shipped with the below provider
// names.  Since the name can be changed and be localized,
// we have to try and map these correctly for net pidl
// interop.
//

const NETPROVIDERS c_rgProviderMap[] = 
{
    { TEXT("Microsoft Network"), HIWORD(WNNC_NET_LANMAN) },
    { TEXT("NetWare"),           HIWORD(WNNC_NET_NETWARE) }
};
const int c_cProviders = ARRAYSIZE(c_rgProviderMap);

#define WNNC_NET_LARGEST WNNC_NET_SYMFONET

//===========================================================================
// CNetwork: Some private macro
//===========================================================================

#define PTROFFSET(pBase, p)     ((int) ((LPBYTE)(p) - (LPBYTE)(pBase)))

extern const IDropTargetVtbl c_CNetRootTargetVtbl;

// This should only be called during process detach
void NetRoot_Terminate(void)
{
    // we can't release this as DLL detatch time since it calls other DLLs
    // that may be unloaded now. we just leak instead of crash
    // ATOMICRELEASE(g_punkNetRoot);
}

//
// get the comment if there is one from the net item
//
LPTSTR NET_CopyComment(LPCIDNETRESOURCE pidn, LPTSTR pszBuff, UINT cchBuff)
{
    LPCSTR pszT;
    VDATEINPUTBUF(pszBuff, TCHAR, cchBuff);

    *pszBuff = 0;

    pszT = pidn->szNetResName + lstrlenA(pidn->szNetResName) + 1;
    if (NET_FHasComment(pidn))
    {
        if (NET_FHasProvider(pidn))
            pszT += lstrlenA(pszT) + 1;
#ifdef UNICODE
        if (NET_IsUnicode(pidn))
        {
            pszT += lstrlenA(pszT) + 1;      // Skip Ansi comment
            pszT += (ualstrlen((LPNWSTR)pszT) + 1) * SIZEOF(TCHAR);     // Skip Unicode Name
            if (NET_FHasProvider(pidn))
                pszT += (ualstrlen((LPNWSTR)pszT) + 1) * SIZEOF(TCHAR); // Skip Unicode Provider
            ualstrcpyn(pszBuff,(LPNWSTR)pszT,cchBuff);
        }
        else
        {
            SHAnsiToUnicode(pszT, pszBuff, cchBuff);
        }
#else
        lstrcpyn(pszBuff, pszT, cchBuff);
#endif
    }
    return pszBuff;
}

//
// get the network resource name from an item. this is not a file system path!
//
// example:
//      server      \\server or strike/sys
//      share       \\server\share or strike/sys
//      printer     \\server\printer
//      provider    "provider name"
//      entire net  "Entire Network"
//
// in:
//   pidn       the item
//   cchBuff    size of buffer in chars.
//
// out:
//   pszBuff    return buffer
//
// returns:
//   address of the input buffer (pszBuff)
//
LPTSTR NET_CopyResName(LPCIDNETRESOURCE pidn, LPTSTR pszBuff, UINT cchBuff)
{
    VDATEINPUTBUF(pszBuff, TCHAR, cchBuff);

#ifdef UNICODE
    if (NET_IsUnicode(pidn))
    {
        LPBYTE pb = (LPBYTE)pidn->szNetResName;
        pb += lstrlenA((LPSTR)pb) + 1;      // Skip over ansi net name
        if (NET_FHasProvider(pidn))
            pb += lstrlenA((LPSTR)pb) + 1;  // Skip over ansi provider
        if (NET_FHasComment(pidn))
            pb += lstrlenA((LPSTR)pb) + 1;  // Skip over comment
        ualstrcpyn(pszBuff, (LPNWSTR)pb, cchBuff);
    }
    else
#endif
    {
        SHAnsiToTChar(pidn->szNetResName, pszBuff, cchBuff);
    }
    return pszBuff;
}

//
// get the provider name from an item. some items do not have providers stored
// in them. for example the "*" indicates where the provider is stored in the
// two different forms of network pidls.
//      [entire net] [provider *] [server] [share] [... file system]
//      [server *] [share] [... file system]
// in:
//   pidn       item (single item PIDL) to try to get the provider name from
//   cchBuff    size in chars.
// out:
//   pszBuff    output
//
LPTSTR NET_CopyProviderName(LPCIDNETRESOURCE pidn, LPTSTR pszBuff, UINT cchBuff)
{
    const BYTE *pb;
    DWORD dwNetType;

    VDATEINPUTBUF(pszBuff, TCHAR, cchBuff);

    *pszBuff = 0;

    if (!NET_FHasProvider(pidn))
        return NULL;

    // try the wNetType at the end of the pidl

    pb = (LPBYTE)pidn + pidn->cb - SIZEOF(WORD);
    dwNetType = *((UNALIGNED WORD *)pb) << 16;

    if (dwNetType && (dwNetType <= WNNC_NET_LARGEST) &&
        (WNetGetProviderName(dwNetType, pszBuff, &cchBuff) == WN_SUCCESS))
    {
        return pszBuff;
    }

    // Try the old way...

    pb = pidn->szNetResName + lstrlenA(pidn->szNetResName) + 1;      // Skip over ansi net name

#ifdef UNICODE
    if (NET_IsUnicode(pidn))
    {
        pb += lstrlenA((LPSTR)pb) + 1;      // Skip over ansi provider
        if (NET_FHasComment(pidn))
            pb += lstrlenA((LPSTR)pb) + 1;  // Skip over comment
        pb += (ualstrlen((LPNWSTR)pb) + 1) * SIZEOF(WCHAR); // skip over unicode net name
        ualstrcpyn(pszBuff, (LPNWSTR)pb, cchBuff);
    }
    else
#endif
    {
        SHAnsiToTChar(pb, pszBuff, cchBuff);
    }

    // Map from Win95 net provider name if possible...
    {
        int i;

        for (i = 0; i < ARRAYSIZE(c_rgProviderMap); i++)
        {
            if (lstrcmp(pszBuff, c_rgProviderMap[i].lpName)==0)
            {
                DWORD dwNetType = c_rgProviderMap[i].wNetType << 16;
                if (dwNetType && (dwNetType <= WNNC_NET_LARGEST))
                {
                    *pszBuff = 0;
                    WNetGetProviderName(dwNetType, pszBuff, (LPDWORD)&cchBuff);
                }
                break;
            }
        }
    }
    return pszBuff;
}


STDMETHODIMP CNETIDLData_QueryGetData(IDataObject *pdtobj, LPFORMATETC pformatetc)
{
    if (pformatetc->tymed & TYMED_HGLOBAL)
    {
        if (pformatetc->cfFormat == g_cfNetResource)
            return CNETIDLData_GetNetResource(pdtobj, NULL);

        if (pformatetc->cfFormat == CF_HDROP)
            return CNETIDLData_GetHDrop(pdtobj, NULL, NULL);
    }

    return CIDLData_QueryGetData(pdtobj, pformatetc);
}

STDMETHODIMP CNETIDLData_GetData(IDataObject *pdtobj, LPFORMATETC pformatetc, STGMEDIUM *pmedium)
{
    if (pformatetc->tymed & TYMED_HGLOBAL)
    {
        if (pformatetc->cfFormat == g_cfNetResource)
            return CNETIDLData_GetNetResource(pdtobj, pmedium);

        if (pformatetc->cfFormat == CF_HDROP)
            return CNETIDLData_GetHDrop(pdtobj, pformatetc, pmedium);
    }

    return CIDLData_GetData(pdtobj, pformatetc, pmedium);
}

const IDataObjectVtbl c_CNETIDLDataVtbl = {
    CIDLData_QueryInterface, CIDLData_AddRef, CIDLData_Release,
    CNETIDLData_GetData,
    CIDLData_GetDataHere,
    CNETIDLData_QueryGetData,
    CIDLData_GetCanonicalFormatEtc,
    CIDLData_SetData,
    CIDLData_EnumFormatEtc,
    CIDLData_Advise,
    CIDLData_Unadvise,
    CIDLData_EnumAdvise
};


//---------------------------------------------------------------------------
//
// IDropTarget stuff  (I guess we're subclassing the CPrintObjs IDropTarget)
//

//
// This is the entry of "drop thread"
//
DWORD CALLBACK CNetPrint_DropThreadProc(void *pv)
{
    PRNTHREADPARAM *pthp = (PRNTHREADPARAM *)pv;
    LPCIDNETRESOURCE pidn = (LPCIDNETRESOURCE)pthp->pidl;
    LPITEMIDLIST pidl;
    TCHAR szName[MAX_PATH];

    NET_CopyResName(pidn, szName, ARRAYSIZE(szName));

    // we need to convert pthp->pidl from a network share to
    // a printer driver connected to that network share

    pidl = Printers_GetInstalledNetPrinter(szName);
    if (!pidl)
    {
        LPCTSTR pTitle;

        if (pthp->hwnd)
        {
            SetForegroundWindow(pthp->hwnd);
            pTitle = NULL;
        }
        else
        {
            pTitle = MAKEINTRESOURCE(IDS_PRINTERS);
        }

        if (IDYES == ShellMessageBox(HINST_THISDLL, pthp->hwnd,
                    MAKEINTRESOURCE(IDS_INSTALLNETPRINTER), pTitle,
                    MB_YESNO|MB_ICONINFORMATION, szName))
        {
            pidl = Printers_PrinterSetup(pthp->hwnd, MSP_NETPRINTER, szName, NULL);
        }
    }

    if (pidl)
    {
        ILFree(pthp->pidl);
        pthp->pidl = pidl;

        // now that pthp->pidl points to a printer driver,
        // CPrintObjs can handle the rest
        return CPrintObj_DropThreadProc(pv);
    }
    FreePrinterThreadParam(pthp);
    return 0;
}

STDMETHODIMP CNetPrint_Drop(IDropTarget *pdt, IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    return CPrintObjs_DropCallback(pdt, pDataObj, grfKeyState, pt, pdwEffect, CNetPrint_DropThreadProc);
}


const IDropTargetVtbl c_CPrintDropTargetVtbl =
{
    CIDLDropTarget_QueryInterface, CIDLDropTarget_AddRef, CIDLDropTarget_Release,
    CPrintObjs_DragEnter,
    CIDLDropTarget_DragOver,
    CIDLDropTarget_DragLeave,
    CNetPrint_Drop,
};



BOOL GetPathFromDataObject(IDataObject *pDataObj, DWORD dwData, LPTSTR pszFileName)
{
    BOOL bRet = FALSE;
    BOOL fUnicode = FALSE;
    HRESULT hr;

    if (dwData & (DTID_FDESCW | DTID_FDESCA))
    {
        FORMATETC fmteW = {g_cfFileGroupDescriptorW, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        STGMEDIUM medium = {0};

        hr = pDataObj->lpVtbl->GetData(pDataObj, &fmteW, &medium);

        if (SUCCEEDED(hr))
        {
            fUnicode = TRUE;
        }
        else
        {
            FORMATETC fmteA = {g_cfFileGroupDescriptorA, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
            hr = pDataObj->lpVtbl->GetData(pDataObj, &fmteA, &medium);
        }


        if (SUCCEEDED(hr))
        {
            if (fUnicode)
            {
                FILEGROUPDESCRIPTORW *pfgdW = GlobalLock(medium.hGlobal);

                if (pfgdW)
                {
                    if (pfgdW->cItems == 1)
                    {
                        SHUnicodeToTChar(pfgdW->fgd[0].cFileName, pszFileName, MAX_PATH);
                    }
                    bRet = TRUE;
                    GlobalUnlock(medium.hGlobal);
                }
            }
            else
            {
                FILEGROUPDESCRIPTORA *pfgdA = GlobalLock(medium.hGlobal);

                if (pfgdA)
                {
                    if (pfgdA->cItems == 1)
                    {
                        SHAnsiToTChar(pfgdA->fgd[0].cFileName, pszFileName, MAX_PATH);
                    }
                    bRet = TRUE;
                    GlobalUnlock(medium.hGlobal);
                }
            }
            ReleaseStgMedium(&medium);
        }
    }

    return bRet;
}
STDMETHODIMP CNetRootDropTarget_DragEnter(IDropTarget *pdropt, IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    CIDLDropTarget *this = IToClass(CIDLDropTarget, dropt, pdropt);

    // let the base-class process it first.
    CIDLDropTarget_DragEnter(pdropt, pDataObj, grfKeyState, pt, pdwEffect);

    if ((this->dwData & (DTID_NETRES | DTID_HIDA)) == (DTID_NETRES | DTID_HIDA))
    {
        *pdwEffect &= DROPEFFECT_LINK;
    }

    //Does data object contain file descriptor as well as file contente ?
    else if (((this->dwData & (DTID_FDESCW | DTID_CONTENTS)) == (DTID_FDESCW | DTID_CONTENTS)) ||
             ((this->dwData & (DTID_FDESCA | DTID_CONTENTS)) == (DTID_FDESCA | DTID_CONTENTS))  )
    {
        //Yes. See if this file is internet shortcut file. if so allow dropping 
        LPTSTR pszExt = NULL;
        TCHAR szFileName[MAX_PATH];
    
        //Get the file name that is being dragged ?
        if (GetPathFromDataObject(pDataObj, this->dwData, szFileName))
        {
            pszExt = PathFindExtension(szFileName);

            //Does the file represent a Internet Shortcut (.url) ?
            *pdwEffect &=  lstrcmpi(pszExt, TEXT(".url")) ? 0 : DROPEFFECT_LINK;
        }
        else
        {
            *pdwEffect = DROPEFFECT_NONE;
        }
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
    }

    this->dwEffectLastReturned = *pdwEffect;
    return S_OK;
}

STDMETHODIMP CNetRootDropTarget_Drop(IDropTarget *pdropt, IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    CIDLDropTarget *this = IToClass(CIDLDropTarget, dropt, pdropt);
    HRESULT hres = S_OK;

    if ((this->dwData & (DTID_NETRES | DTID_HIDA)) == (DTID_NETRES | DTID_HIDA))
    {
        *pdwEffect &= DROPEFFECT_LINK;

        hres = CIDLDropTarget_DragDropMenu(this, DROPEFFECT_LINK, pdtobj,
                                pt, pdwEffect, NULL, NULL, POPUP_NONDEFAULTDD, grfKeyState);

        if (*pdwEffect)
        {

            if (!this->pdtgAgr)
            {
                LPITEMIDLIST pidl = SHCloneSpecialIDList(NULL, CSIDL_NETHOOD, FALSE);
                if (pidl)
                {
                    IShellFolder *psfFiles;
                    hres = SHBindToObject(NULL, &IID_IShellFolder, pidl, (void **)&psfFiles);
                    if (SUCCEEDED(hres))
                    {
                        hres = psfFiles->lpVtbl->CreateViewObject(psfFiles, this->hwndOwner, &IID_IDropTarget, &this->pdtgAgr);
                        psfFiles->lpVtbl->Release(psfFiles);
                    }
                    ILFree(pidl);
                }
            }
            
            if (this->pdtgAgr)
            {
                // force link through the dwEffect and keyboard
                *pdwEffect &= DROPEFFECT_LINK;
                grfKeyState = MK_LBUTTON | MK_CONTROL | MK_SHIFT | MK_FAKEDROP;
                hres = SHSimulateDrop(this->pdtgAgr, pdtobj, grfKeyState, NULL, pdwEffect);
            }
            else 
                *pdwEffect = 0;
        }

    }
    //Does data object contain file descriptor as well as file contente ?
    else if (((this->dwData & (DTID_FDESCW | DTID_CONTENTS)) == (DTID_FDESCW | DTID_CONTENTS)) ||
             ((this->dwData & (DTID_FDESCA | DTID_CONTENTS)) == (DTID_FDESCA | DTID_CONTENTS))  )
    {
        //Yes 
        TCHAR szPath[MAX_PATH];
        TCHAR szFileName[MAX_PATH];

        szFileName[0] = 0;
        //Get the Nethood location.
        SHGetSpecialFolderPath(NULL, szPath, CSIDL_NETHOOD,0);

        //Get File Name from Data Object.
        GetPathFromDataObject(pdtobj, DTID_FDESCW | DTID_FDESCA, szFileName);


        PathAddBackslash(szPath);

        //Get Full Path of the file to  be created.
        lstrcat(szPath, szFileName);

        //Make sure the file name being created is unique.
        PathYetAnotherMakeUniqueName(szFileName, szPath, NULL, NULL);

        //Save the file contents from Data Object to the file.
        hres = DataObj_SaveToFile(pdtobj, g_cfFileContents, 0, szFileName, 0, NULL);

        if (SUCCEEDED(hres))
        {
            *pdwEffect &= DROPEFFECT_LINK;
        }
        else
        {
            *pdwEffect = 0;
        }

    }
    else
    {
        *pdwEffect = 0;
    }

    CIDLDropTarget_DragLeave(pdropt);
    return hres;
}

const IDropTargetVtbl c_CNetRootTargetVtbl =
{
    CIDLDropTarget_QueryInterface, CIDLDropTarget_AddRef, CIDLDropTarget_Release,
    CNetRootDropTarget_DragEnter,
    CIDLDropTarget_DragOver,
    CIDLDropTarget_DragLeave,
    CNetRootDropTarget_Drop,
};

//
// Get the provider name from a relative IDLIST.
// in:
//  pidn    potentially multi level item to try to get the resource from
//
LPTSTR NET_GetProviderFromRes(LPCIDNETRESOURCE pidn, LPTSTR pszBuffer, UINT cchBuffer)
{
    VDATEINPUTBUF(pszBuffer, TCHAR, cchBuffer);

    // If this guy is the REST of network item, we increment to the
    // next IDLIST - If at root return NULL
    if (pidn->cb == 0)
        return NULL;

    //
    // If the IDLIST starts with a ROOT_REGITEM, then skip to the
    // next item in the list...
    if (pidn->bFlags == SHID_ROOT_REGITEM)
    {
        pidn = (LPIDNETRESOURCE)_ILNext((LPITEMIDLIST)pidn);
        if (pidn->cb == 0)
            return NULL;
    }

    // If the IDLIST includes Entire Network, the provider will be
    // part of the next component.
    if (NET_GetDisplayType(pidn) == RESOURCEDISPLAYTYPE_ROOT)
    {
        pidn = (LPIDNETRESOURCE)_ILNext((LPITEMIDLIST)pidn);
        if (pidn->cb == 0)
            return NULL;
    }

    // If the next component after the 'hood or Entire Network is
    // a network object, its name is the provider name, else the
    // provider name comes after the remote name.
    if (NET_GetDisplayType(pidn) == RESOURCEDISPLAYTYPE_NETWORK)
    {
        // Simply return the name field back for the item.
        return NET_CopyResName(pidn, pszBuffer, cchBuffer);
    }
    else
    {
        // Nope one of the items in the neighborhood view was selected
        // The Provider name is stored after ther resource name
        return NET_CopyProviderName(pidn, pszBuffer, cchBuffer);
    }
}

//
// Get the provider name from an absolute IDLIST.
// Parameters:
//  pidlAbs -- Specifies the Absolute IDList to the file system object
//
LPTSTR NET_GetProviderFromIDList(LPCITEMIDLIST pidlAbs, LPTSTR pszBuff, UINT cchBuff)
{
    return NET_GetProviderFromRes((LPCIDNETRESOURCE)_ILNext(pidlAbs), pszBuff, cchBuff);
}

//===========================================================================
// HNRES related stuff
//===========================================================================

extern HRESULT CFSIDLData_GetHDrop(IDataObject *pdtobj, FORMATETC* pformatetcIn, STGMEDIUM *pmedium);

//
// pmedium and pformatetcIn == NULL if we are handling QueryGetData
//
HRESULT CNETIDLData_GetHDrop(IDataObject *pdtobj, FORMATETC* pformatetcIn, STGMEDIUM *pmedium)
{
    STGMEDIUM medium = { 0, NULL, NULL };
    HRESULT hres = E_INVALIDARG;        // assume error
    LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
    if (pida)
    {
        // Get the first one to see the type.
        LPCIDNETRESOURCE pidn = (LPCIDNETRESOURCE)IDA_GetIDListPtr(pida, 0);

        if ((NET_GetFlags(pidn) & SHID_JUNCTION) && 
            (NET_GetType(pidn) == RESOURCETYPE_DISK))
        {
            //
            // Get HDrop only if we are handling IDataObject::GetData (pmedium != NULL)
            //
            if (pmedium != NULL) 
            {
                // We have are handling GetData. Make sure we have a FORMATETC
                if (pformatetcIn == NULL)
                {
                    // Bad; a NULL FORMATETC with a non-null STGMEDIUM
                    hres = E_INVALIDARG;
                }
                else
                {
                    // We have non-null FORMATETC and STGMEDIUM - get the HDrop
                    hres = CFSIDLData_GetHDrop(pdtobj, pformatetcIn, pmedium);
                }
            }
            else
            {
                // We were handling QueryGetData
                hres = S_OK;
            }
        }

        HIDA_ReleaseStgMedium(pida, &medium);
    }
    return hres;
}

//
// fill in pmedium with a NRESARRAY
//
// pmedium == NULL if we are handling QueryGetData
//
HRESULT CNETIDLData_GetNetResource(IDataObject *pdtobj, STGMEDIUM *pmedium)
{
    HRESULT hres = E_OUTOFMEMORY;
    LPITEMIDLIST pidl;
    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);

    ASSERT(pida && pida->cidl);

    // First, get the provider name from the first one (assuming they are common).
    pidl = IDA_ILClone(pida, 0);
    if (pidl)
    {
        TCHAR szProvider[MAX_PATH];
        LPCTSTR pszProvider = NET_GetProviderFromIDList(pidl, szProvider, ARRAYSIZE(szProvider));
        if (pmedium)
        {
            TCHAR szName[MAX_PATH];
            UINT cbHeader = SIZEOF(NRESARRAY) - SIZEOF(NETRESOURCE) + SIZEOF(NETRESOURCE) * pida->cidl;
            UINT cbRequired, cchName, iItem;

            // Calculate required size
            cbRequired = cbHeader;
            if (pszProvider)
                cbRequired += lstrlen(pszProvider) * SIZEOF(TCHAR) + SIZEOF(TCHAR);

            for (iItem = 0; iItem < pida->cidl; iItem++)
            {
                LPCIDNETRESOURCE pidn = (LPCIDNETRESOURCE)IDA_GetIDListPtr(pida, iItem);
                NET_CopyResName(pidn, szName, ARRAYSIZE(szName));
                cchName = lstrlen(szName) + 1;
                cbRequired += cchName * SIZEOF(TCHAR);
            }

            //
            // Indicate that the caller should release hmem.
            //
            pmedium->pUnkForRelease = NULL;
            pmedium->tymed = TYMED_HGLOBAL;
            pmedium->hGlobal = GlobalAlloc(GPTR, cbRequired);
            if (pmedium->hGlobal)
            {
                LPNRESARRAY panr = (LPNRESARRAY)pmedium->hGlobal;
                LPTSTR pszT = (LPTSTR)((LPBYTE)panr + cbHeader);
                UINT offProvider = 0;

                panr->cItems = pida->cidl;

                // Copy the provider name. This is not necessary,
                // if we are dragging providers.
                if (pszProvider)
                {
                    lstrcpy(pszT, pszProvider);
                    offProvider = PTROFFSET(panr, pszT);
                    pszT += lstrlen(pszT) + 1;
                }

                //
                // For each item, fill each NETRESOURCE and append resource
                // name at the end. Note that we should put offsets in
                // lpProvider and lpRemoteName.
                //
                for (iItem = 0; iItem < pida->cidl; iItem++)
                {
                    LPNETRESOURCE pnr = &panr->nr[iItem];
                    LPCIDNETRESOURCE pidn = (LPCIDNETRESOURCE)IDA_GetIDListPtr(pida, iItem);

                    ASSERT(pnr->dwScope == 0);
                    ASSERT(pnr->lpLocalName==NULL);
                    ASSERT(pnr->lpComment==NULL);

                    pnr->dwType = NET_GetType(pidn);
                    pnr->dwDisplayType = NET_GetDisplayType(pidn);
                    pnr->dwUsage = NET_GetUsage(pidn);
                    NET_CopyResName(pidn, pszT, cchName);

                    if (pnr->dwDisplayType == RESOURCEDISPLAYTYPE_ROOT)
                    {
                        pnr->lpProvider = NULL;
                        pnr->lpRemoteName = NULL;
                    }
                    else if (pnr->dwDisplayType == RESOURCEDISPLAYTYPE_NETWORK)
                    {
                        *((UINT *) &pnr->lpProvider) = PTROFFSET(panr, pszT);
                        ASSERT(pnr->lpRemoteName == NULL);
                    }
                    else
                    {
                        *((UINT *) &pnr->lpProvider) = offProvider;
                        *((UINT *) &pnr->lpRemoteName) = PTROFFSET(panr, pszT);
                    }
                    pszT += lstrlen(pszT) + 1;
                }

                ASSERT((LPTSTR)((LPBYTE)panr + cbRequired) == pszT);
                hres = S_OK;
            }
        }
        else
        {
            hres = S_OK;    // handing QueryGetData, yes, we have it
        }
        ILFree(pidl);
    }

    HIDA_ReleaseStgMedium(pida, &medium);

    return hres;
}

// fill in pmedium with an HGLOBAL version of a NRESARRAY

HRESULT CNETIDLData_GetNetResourceForFS(IDataObject *pdtobj, STGMEDIUM *pmedium)
{
    HRESULT hres = E_OUTOFMEMORY;
    LPITEMIDLIST pidlAbs;
    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);

    ASSERT(pida && medium.hGlobal);     // we created this...

    //
    // NOTES: Even though we may have multiple FS objects in this HIDA,
    //  we know that they share the root. Therefore, getting the pidl for
    //  the first item is always sufficient.
    //

    pidlAbs = IDA_ILClone(pida, 0);
    if (pidlAbs)
    {
#if 0
        IShellFolder2 *psf;

        hres = SHBindToIDListParent(pidlAbs, IID_IShellFolder2, &psf, &pidlChild);
        if (SUCCEEDED(hres))
        {
            UINT cbRequired = SIZEOF(NETRESOURCE) + SIZEOF(TCHAR) * 1024;
            pmedium->pUnkForRelease = NULL;
            pmedium->tymed = TYMED_HGLOBAL;
            pmedium->hGlobal = GlobalAlloc(GPTR, cbRequired);
            if (pmedium->hGlobal)
            {
                hres = psf->lpVtbl->GetItemData(psf, SFGID_NETRESOURCE, pmedium->hGlobal, cbRequired);
                if (FAILED(hres))
                {
                    GlobalFree(pemdium->hGlobal);
                    pemdium->hGlobal = NULL;
                }
            }
            psf->lpVtbl->Release(psf);
        }
#endif
        LPITEMIDLIST pidl;

        ASSERT(IsIDListInNameSpace(pidlAbs, &CLSID_NetworkPlaces));

        //
        // Look for the JUNCTION point (starting from the second ID)
        //
        for (pidl = _ILNext(pidlAbs); !ILIsEmpty(pidl); pidl = _ILNext(pidl))
        {
            LPIDNETRESOURCE pidn = (LPIDNETRESOURCE)pidl;
            if (NET_GetFlags(pidn) & SHID_JUNCTION)
            {
                //
                // We found the JUNCTION point (which is s share).
                // Return the HNRES to it.
                //
                TCHAR szProvider[MAX_PATH];
                TCHAR szRemote[MAX_PATH];
                UINT cbRequired;
                LPCTSTR pszProvider = NET_GetProviderFromIDList(pidlAbs, szProvider, ARRAYSIZE(szProvider));
                LPCTSTR pszRemoteName = NET_CopyResName(pidn, szRemote, ARRAYSIZE(szRemote));
                UINT   cbProvider = lstrlen(pszProvider) * SIZEOF(TCHAR) + SIZEOF(TCHAR);

                //
                // This should not be a provider node.
                // This should not be the last ID in pidlAbs.
                //
                ASSERT(pszProvider != pszRemoteName);
                ASSERT(!ILIsEmpty(_ILNext(pidl)));

                cbRequired = SIZEOF(NRESARRAY) + cbProvider + lstrlen(pszRemoteName) * SIZEOF(TCHAR) + SIZEOF(TCHAR);

                pmedium->pUnkForRelease = NULL;
                pmedium->tymed = TYMED_HGLOBAL;
                pmedium->hGlobal = GlobalAlloc(GPTR, cbRequired);
                if (pmedium->hGlobal)
                {
                    LPNRESARRAY panr = (LPNRESARRAY)pmedium->hGlobal;
                    LPNETRESOURCE pnr = &panr->nr[0];
                    LPTSTR pszT = (LPTSTR)(panr + 1);

                    ASSERT(pnr->dwScope == 0);
                    ASSERT(pnr->lpLocalName == NULL);
                    ASSERT(pnr->lpComment == NULL);

                    panr->cItems = 1;

                    pnr->dwType = NET_GetType(pidn);
                    pnr->dwDisplayType = NET_GetDisplayType(pidn);
                    pnr->dwUsage = NET_GetUsage(pidn);

                    *((UINT *) &pnr->lpProvider) = SIZEOF(NRESARRAY);
                    lstrcpy(pszT, pszProvider);
                    ASSERT(PTROFFSET(panr, pszT) == SIZEOF(NRESARRAY));
                    pszT += cbProvider / SIZEOF(TCHAR);

                    *((UINT *) &pnr->lpRemoteName) = SIZEOF(NRESARRAY) + cbProvider;
                    ASSERT(PTROFFSET(panr, pszT) == (int)SIZEOF(NRESARRAY) + (int)cbProvider);
                    lstrcpy(pszT, pszRemoteName);

                    ASSERT(((LPBYTE)panr) + cbRequired == (LPBYTE)pszT + (lstrlen(pszT) + 1) * SIZEOF(TCHAR));
                    hres = S_OK;
                }
                else
                {
                    hres = E_OUTOFMEMORY;
                }
                break;
            }
        }
        ASSERT(!ILIsEmpty(pidl));   // We should have found the junction point.
        ILFree(pidlAbs);
    }
    HIDA_ReleaseStgMedium(pida, &medium);
    return hres;
}


#ifdef WINNT

//
//  pidlRemainder will be filled in (only in the TRUE return case) with a
//  pointer to the part of the IDL (if any) past the remote regitem.
//  This value may be used, for example, to differentiate between a remote
//  printer folder and a printer under a remote printer folder
//

BOOL NET_IsRemoteRegItem(LPCITEMIDLIST pidl, REFCLSID rclsid, LPITEMIDLIST* ppidlRemainder)
{
    BOOL bRet = FALSE;
    // in "My Network Places"
    if (pidl && IsIDListInNameSpace(pidl, &CLSID_NetworkPlaces))
    {
        LPCITEMIDLIST pidlStart = pidl; // save this

        // Now, search for a server item. HACKHACK: this assume everything from
        // the NetHood to the server item is a shell pidl with a bFlags field!!

        for (pidl = _ILNext(pidl); !ILIsEmpty(pidl); pidl = _ILNext(pidl))
        {
            if ((SIL_GetType(pidl) & SHID_TYPEMASK) == SHID_NET_SERVER)
            {
                LPITEMIDLIST pidlToTest;

                // Found a server. Is the thing after it a remote registry item?
                pidl = _ILNext(pidl);

                *ppidlRemainder = _ILNext(pidl);

                pidlToTest = ILCloneUpTo(pidlStart, *ppidlRemainder);
                if (pidlToTest)
                {
                    CLSID clsid;
                    bRet = SUCCEEDED(GetCLSIDFromIDList(pidlToTest, &clsid)) && IsEqualCLSID(rclsid, &clsid);
                    ILFree(pidlToTest);
                }
                break;  // done
            }
        }
    }
    return bRet;
}
#endif // WINNT

void WINAPI CopyEnumElement(void* pDest, const void* pSource, DWORD dwSize)
{
    if (!pDest)
        return;
        
    memcpy(pDest, pSource, dwSize);
}

//==========================================================================
// This part is psuedo bogus.  Basically we have problems at times doing a
// translation from things like \\pyrex\user to the appropriate PIDL,
// especially if you want to avoid the overhead of hitting the network and
// also problems of knowing if the server is in the "HOOD"

typedef struct _NPTItem
{
    struct _NPTItem *pnptNext;  // Pointer to next item;
    LPCITEMIDLIST   pidl;       // The pidl
    USHORT          cchName;     // size of the name in characters.
    TCHAR            szName[1];  // The name to translate from
} NPTItem, *PNPTItem;

// Each process will maintain their own list.
PNPTItem    g_pnptHead = NULL;

//
//  Function to register translations from Path to IDList translations.
//
void NPTRegisterNameToPidlTranslation(LPCTSTR pszPath, LPCITEMIDLIST pidl)
{
    PNPTItem pnpt;
    int cItemsRemoved = 0;
    TCHAR szPath[MAX_PATH];

    // We currently are only interested in UNC Roots
    // If the table becomes large we can reduce this to only servers...

    if (!PathIsUNC(pszPath) || !IsIDListInNameSpace(pidl, &CLSID_NetworkPlaces))
        return;     // Not interested.

    //
    // If this item is not a root we need to count how many items to remove
    //
    lstrcpy(szPath, pszPath);
    while (!PathIsRoot(szPath))
    {
        cItemsRemoved++;
        if (!PathRemoveFileSpec(szPath))
            return;     // Did not get back to a valid root
    }

    ENTERCRITICAL;

    // We don't want to add duplicates
    for (pnpt = g_pnptHead; pnpt != NULL ; pnpt = pnpt->pnptNext)
    {
        if (lstrcmpi(szPath, pnpt->szName) == 0)
            break;
    }

    if (pnpt == NULL)
    {
        UINT cch = lstrlen(szPath);

        pnpt = (PNPTItem)LocalAlloc(LPTR, SIZEOF(NPTItem) + cch * SIZEOF(TCHAR));
        if (pnpt)
        {
            pnpt->pidl = ILClone(pidl);
            if (pnpt->pidl)
            {
                while (cItemsRemoved--)
                {
                    ILRemoveLastID((LPITEMIDLIST)pnpt->pidl);
                }
                pnpt->pnptNext = g_pnptHead;
                g_pnptHead = pnpt;
                pnpt->cchName = (USHORT) cch;
                lstrcpy(pnpt->szName, szPath);
            }
            else
            {
                LocalFree((HLOCAL)pnpt);
            }
        }
    }
    LEAVECRITICAL;
}


//--------------------------------------------------------------------------
// The main function to attemp to map a portion of the name into an idlist
// Right now limit it to UNC roots
//
LPCTSTR NPTMapNameToPidl(LPCTSTR pszPath, LPCITEMIDLIST *ppidl)
{
    PNPTItem pnpt;

    *ppidl = NULL;

    ENTERCRITICAL;

    // See if we can find the item in the list.
    for (pnpt = g_pnptHead; pnpt != NULL ; pnpt = pnpt->pnptNext)
    {
        if (IntlStrEqNI(pszPath, pnpt->szName, pnpt->cchName) && 
            (*(pszPath + pnpt->cchName) == TEXT('\\')))
            break;
    }
    LEAVECRITICAL;

    // See if we found a match
    if (pnpt == NULL)
        return NULL;

    // Found a match
    *ppidl = pnpt->pidl;
    return pszPath + pnpt->cchName;     // points to slash
}
