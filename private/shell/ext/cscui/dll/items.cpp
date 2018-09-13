//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       items.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include <shlwapip.h>   // QITAB, QISearch
#include <shsemip.h>    // ILFree(), etc

#include "folder.h"
#include "items.h"

CLIPFORMAT COfflineItemsData::m_cfHDROP;
CLIPFORMAT COfflineItemsData::m_cfFileContents;
CLIPFORMAT COfflineItemsData::m_cfFileDesc;
CLIPFORMAT COfflineItemsData::m_cfPreferedEffect;
CLIPFORMAT COfflineItemsData::m_cfPerformedEffect;
CLIPFORMAT COfflineItemsData::m_cfLogicalPerformedEffect;
CLIPFORMAT COfflineItemsData::m_cfDataSrcClsid;

COfflineItemsData::COfflineItemsData(
    LPCITEMIDLIST pidlFolder, 
    UINT cidl, 
    LPCITEMIDLIST *apidl, 
    HWND hwndParent,
    IShellFolder *psfOwner,    // Optional.  Default is NULL.
    IDataObject *pdtInner      // Optional.  Default is NULL.
    ) : CIDLData(pidlFolder,
                 cidl,
                 apidl,
                 psfOwner,
                 pdtInner),
        m_hwndParent(hwndParent),
        m_rgpolid(NULL),
        m_hrCtor(NOERROR),
        m_dwPreferredEffect(DROPEFFECT_COPY),
        m_dwPerformedEffect(DROPEFFECT_NONE),
        m_dwLogicalPerformedEffect(DROPEFFECT_NONE),
        m_cItems(0)
{
    if (0 == m_cfHDROP)
    {
        m_cfHDROP          = CF_HDROP;
        m_cfFileContents   = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILECONTENTS);
        m_cfFileDesc       = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR);
        m_cfPreferedEffect = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);
        m_cfPerformedEffect = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_PERFORMEDDROPEFFECT);
        m_cfLogicalPerformedEffect = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_LOGICALPERFORMEDDROPEFFECT);
        m_cfDataSrcClsid   = (CLIPFORMAT)RegisterClipboardFormat(c_szCFDataSrcClsid);
    }

    m_hrCtor = CIDLData::CtorResult();
    if (SUCCEEDED(m_hrCtor))
    {
        m_rgpolid = new LPCOLID[cidl];
        if (m_rgpolid)
        {
            ZeroMemory(m_rgpolid, sizeof(LPCOLID) * cidl);
            m_cItems = cidl;
            for (UINT i = 0; i < cidl; i++)
            {
                m_rgpolid[i] = (LPCOLID)ILClone(apidl[i]);
                if (!m_rgpolid[i])
                {
                    m_hrCtor = E_OUTOFMEMORY;
                    break;
                }
            }
        }
        else
            m_hrCtor = E_OUTOFMEMORY;
    }
}

COfflineItemsData::~COfflineItemsData(
    void
    )
{
    delete[] m_rgpolid;
}


HRESULT 
COfflineItemsData::CreateInstance(
    COfflineItemsData **ppOut,
    LPCITEMIDLIST pidlFolder, 
    UINT cidl, 
    LPCITEMIDLIST *apidl, 
    HWND hwndParent,
    IShellFolder *psfOwner,
    IDataObject *pdtInner
    )
{
    HRESULT hr = E_OUTOFMEMORY;
    COfflineItemsData *pNew = new COfflineItemsData(pidlFolder,
                                                    cidl,
                                                    apidl,
                                                    hwndParent,
                                                    psfOwner,
                                                    pdtInner);
    if (NULL != pNew)
    {
        hr = pNew->CtorResult();
        if (SUCCEEDED(hr))
            *ppOut = pNew;
        else
            delete pNew;
    }
    return hr;
}



HRESULT 
COfflineItemsData::CreateInstance(
    IDataObject **ppOut,
    LPCITEMIDLIST pidlFolder, 
    UINT cidl, 
    LPCITEMIDLIST *apidl, 
    HWND hwndParent,
    IShellFolder *psfOwner,
    IDataObject *pdtInner
    )
{
    COfflineItemsData *poid;
    HRESULT hr = CreateInstance(&poid,
                                pidlFolder,
                                cidl,
                                apidl,
                                hwndParent,
                                psfOwner,
                                pdtInner);
    if (SUCCEEDED(hr))
    {
        poid->AddRef();
        hr = poid->QueryInterface(IID_IDataObject, (void **)ppOut);
        poid->Release();
    }
    return hr;
}


HRESULT 
COfflineItemsData::GetData(
    FORMATETC *pFEIn, 
    STGMEDIUM *pstm
    )
{
    HRESULT hr;

    pstm->hGlobal = NULL;
    pstm->pUnkForRelease = NULL;

    if ((pFEIn->cfFormat == m_cfHDROP) && (pFEIn->tymed & TYMED_HGLOBAL))
        hr = CreateHDROP(pstm);
    else if ((pFEIn->cfFormat == m_cfFileDesc) && (pFEIn->tymed & TYMED_HGLOBAL))
        hr = CreateFileDescriptor(pstm);
    else if ((pFEIn->cfFormat == m_cfFileContents) && (pFEIn->tymed & TYMED_ISTREAM))
        hr = CreateFileContents(pstm, pFEIn->lindex);
    else if ((pFEIn->cfFormat == m_cfPreferedEffect) && (pFEIn->tymed & TYMED_HGLOBAL))
        hr = CreatePrefDropEffect(pstm);
    else if ((pFEIn->cfFormat == m_cfPerformedEffect) && (pFEIn->tymed & TYMED_HGLOBAL))
        hr = CreatePerformedDropEffect(pstm);
    else if ((pFEIn->cfFormat == m_cfLogicalPerformedEffect) && (pFEIn->tymed & TYMED_HGLOBAL))
        hr = CreateLogicalPerformedDropEffect(pstm);
    else if ((pFEIn->cfFormat == m_cfDataSrcClsid) && (pFEIn->tymed & TYMED_HGLOBAL))
        hr = CreateDataSrcClsid(pstm);
    else
        hr = CIDLData::GetData(pFEIn, pstm);

    return hr;
}


DWORD COfflineItemsData::GetDataDWORD(
    FORMATETC *pfe, 
    STGMEDIUM *pstm, 
    DWORD *pdwOut
    )
{
    if (pfe->tymed == TYMED_HGLOBAL)
    {
        DWORD *pdw = (DWORD *)GlobalLock(pstm->hGlobal);
        if (pdw)
        {
            *pdwOut = *pdw;
            GlobalUnlock(pstm->hGlobal);
        }
    }
    return *pdwOut;
}



HRESULT
COfflineItemsData::SetData(
    FORMATETC *pFEIn, 
    STGMEDIUM *pstm, 
    BOOL fRelease
    )
{
    if (pFEIn->cfFormat == g_cfPerformedDropEffect)
    {
        GetDataDWORD(pFEIn, pstm, &m_dwPerformedEffect);
    }
    else if (pFEIn->cfFormat == g_cfLogicalPerformedDropEffect)
    {
        GetDataDWORD(pFEIn, pstm, &m_dwLogicalPerformedEffect);
    }
    else if (pFEIn->cfFormat == g_cfPreferredDropEffect)
    {
        GetDataDWORD(pFEIn, pstm, &m_dwPreferredEffect);
    }

    return CIDLData::SetData(pFEIn, pstm, fRelease);
}


HRESULT 
COfflineItemsData::QueryGetData(
    FORMATETC *pFEIn
    )
{
    if (pFEIn->cfFormat == m_cfHDROP ||
        pFEIn->cfFormat == m_cfFileDesc ||
        pFEIn->cfFormat == m_cfFileContents   ||
        pFEIn->cfFormat == m_cfPreferedEffect ||
        pFEIn->cfFormat == m_cfPerformedEffect ||
        pFEIn->cfFormat == m_cfLogicalPerformedEffect ||
        pFEIn->cfFormat == m_cfDataSrcClsid)
    {
        return S_OK;
    }
    return CIDLData::QueryGetData(pFEIn);
}


HRESULT 
COfflineItemsData::ProvideFormats(
    CEnumFormatEtc *pEnumFmtEtc
    )
{
    FORMATETC rgFmtEtc[] = {
        { m_cfHDROP,                  NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        { m_cfFileContents,           NULL, DVASPECT_CONTENT, -1, TYMED_ISTREAM },
        { m_cfFileDesc,               NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        { m_cfPreferedEffect,         NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        { m_cfPerformedEffect,        NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        { m_cfLogicalPerformedEffect, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        { m_cfDataSrcClsid,           NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL }
    };
    //
    // Add our formats to the CIDLData format enumerator.
    //
    return pEnumFmtEtc->AddFormats(ARRAYSIZE(rgFmtEtc), rgFmtEtc);
}

HRESULT 
COfflineItemsData::CreateFileDescriptor(
    STGMEDIUM *pstm
    )
{
    HRESULT hr;
    pstm->tymed = TYMED_HGLOBAL;
    pstm->pUnkForRelease = NULL;
    
    // render the file descriptor
    // we only allocate for m_cItems-1 file descriptors because the filegroup
    // descriptor has already allocated space for 1.
    FILEGROUPDESCRIPTOR *pfgd = (FILEGROUPDESCRIPTOR *)GlobalAlloc(GPTR, sizeof(FILEGROUPDESCRIPTOR) + (m_cItems - 1) * sizeof(FILEDESCRIPTOR));
    if (pfgd)
    {
        pfgd->cItems = m_cItems;                     // set the number of items
        pfgd->fgd[0].dwFlags = FD_PROGRESSUI;       // turn on progress UI

        for (int i = 0; i < m_cItems; i++)
        {
            FILEDESCRIPTOR *pfd = &(pfgd->fgd[i]);
            TCHAR szName[MAX_PATH];

            StrCpyN(pfd->cFileName, 
                    COfflineFilesFolder::OLID_GetFileName(m_rgpolid[i], szName, ARRAYSIZE(szName)), 
                    ARRAYSIZE(pfd->cFileName));
        }

        pstm->hGlobal = pfgd;
        hr = S_OK;
    }
    else
        hr = E_OUTOFMEMORY;
    
    return hr;
}

HRESULT 
COfflineItemsData::CreatePrefDropEffect(
    STGMEDIUM *pstm
    )
{
    return CreateDWORD(pstm, m_dwPreferredEffect);
}

HRESULT 
COfflineItemsData::CreatePerformedDropEffect(
    STGMEDIUM *pstm
    )
{
    return CreateDWORD(pstm, m_dwPerformedEffect);
}

HRESULT 
COfflineItemsData::CreateLogicalPerformedDropEffect(
    STGMEDIUM *pstm
    )
{
    return CreateDWORD(pstm, m_dwLogicalPerformedEffect);
}


HRESULT
COfflineItemsData::CreateDWORD(
    STGMEDIUM *pstm,
    DWORD dwEffect
    )
{
    pstm->tymed = TYMED_HGLOBAL;
    pstm->pUnkForRelease = NULL;
    pstm->hGlobal = GlobalAlloc(GPTR, sizeof(DWORD));
    if (pstm->hGlobal)
    {
        *((DWORD *)pstm->hGlobal) = dwEffect;
        return S_OK;
    }

    return E_OUTOFMEMORY;    
}

HRESULT 
COfflineItemsData::CreateFileContents(
    STGMEDIUM *pstm, 
    LONG lindex
    )
{
    HRESULT hr;
    
    // here's a partial fix for when ole sometimes passes in -1 for lindex
    if (lindex == -1)
    {
        if (m_cItems == 1)
            lindex = 0;
        else
            return E_FAIL;
    }
    
    pstm->tymed = TYMED_ISTREAM;
    pstm->pUnkForRelease = NULL;

    TCHAR szPath[MAX_PATH];
    COfflineFilesFolder::OLID_GetFullPath(m_rgpolid[lindex], szPath, ARRAYSIZE(szPath));

    hr = SHCreateStreamOnFile(szPath, STGM_READ, &pstm->pstm);

    return hr;
}


HRESULT 
COfflineItemsData::CreateHDROP(
    STGMEDIUM *pstm
    )
{
    HRESULT hr = E_OUTOFMEMORY;

    int i;
    //
    // The extra MAX_PATH is so that the damned SHLWAPI functions (i.e.
    // PathAppend) won't complain about a too-small buffer.  They require
    // that the destination buffer be AT LEAST MAX_PATH.  So much for letting
    // code being smart about buffer sizes.
    //
    int cbHdrop = sizeof(DROPFILES) + MAX_PATH + sizeof(TCHAR); // +1 == final nul.
    TCHAR szPath[MAX_PATH];

    pstm->tymed = TYMED_HGLOBAL;
    pstm->pUnkForRelease = NULL;

    //
    // Calculate required buffer size.
    //
    for (i = 0; i < m_cItems; i++)
    {
        szPath[0] = TEXT('\0');
        COfflineFilesFolder::OLID_GetFullPath(m_rgpolid[i], szPath, ARRAYSIZE(szPath));
        cbHdrop += (lstrlen(szPath) + 1) * sizeof(TCHAR);
    }
    pstm->hGlobal = GlobalAlloc(GPTR, cbHdrop);
    if (NULL != pstm->hGlobal)
    {
        //
        // Fill out the header and append the file paths in a 
        // double-nul term list.
        //
        LPDROPFILES pdfHdr = (LPDROPFILES)pstm->hGlobal;
        pdfHdr->pFiles = sizeof(DROPFILES);
        pdfHdr->fWide  = TRUE;

        LPTSTR pszWrite = (LPTSTR)((LPBYTE)pdfHdr + sizeof(DROPFILES));
        LPTSTR pszEnd   = (LPTSTR)((LPBYTE)pstm->hGlobal + cbHdrop - sizeof(TCHAR));
        for (i = 0; i < m_cItems; i++)
        {
            COfflineFilesFolder::OLID_GetFullPath(m_rgpolid[i], pszWrite, (UINT)(pszEnd - pszWrite));
            pszWrite += lstrlen(pszWrite) + 1;
        }
        hr = S_OK;
    }

    return hr;    
}


HRESULT
COfflineItemsData::CreateDataSrcClsid(
    STGMEDIUM *pstm
    )
{
    HRESULT hr = E_OUTOFMEMORY;

    pstm->tymed = TYMED_HGLOBAL;
    pstm->pUnkForRelease = NULL;
    pstm->hGlobal = GlobalAlloc(GPTR, sizeof(CLSID));
    if (pstm->hGlobal)
    {
        *((CLSID *)pstm->hGlobal) = CLSID_OfflineFilesFolder;
        return S_OK;
    }

    return E_OUTOFMEMORY;    
}


COfflineItems::COfflineItems(
    COfflineFilesFolder *pFolder, 
    HWND hwnd
    ) : m_cRef(1),
        m_hwndBrowser(hwnd),
        m_pFolder(pFolder),
        m_ppolid(NULL),
        m_cItems(0)
{
    DllAddRef();
    if (m_pFolder)
        m_pFolder->AddRef();
}        

COfflineItems::~COfflineItems()
{
    if (m_pFolder)
        m_pFolder->Release();

    if (m_ppolid)
    {
        for (UINT i = 0; i < m_cItems; i++) 
        {
            if (m_ppolid[i])
                ILFree((LPITEMIDLIST)m_ppolid[i]);
        }
        LocalFree((HLOCAL)m_ppolid);
    }
    
    DllRelease();
}


HRESULT 
COfflineItems::Initialize(
    UINT cidl, 
    LPCITEMIDLIST *ppidl
    )
{
    HRESULT hr;
    m_ppolid = (LPCOLID *)LocalAlloc(LPTR, cidl * sizeof(LPCOLID));
    if (m_ppolid)
    {
        m_cItems = cidl;
        hr = S_OK;
        for (UINT i = 0; i < cidl; i++)
        {
            m_ppolid[i] = (LPCOLID)ILClone(ppidl[i]);
            if (!m_ppolid[i])
            {
                hr = E_OUTOFMEMORY;
                break;
            }
        }
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}        



HRESULT 
COfflineItems::CreateInstance(
    COfflineFilesFolder *pfolder, 
    HWND hwnd,
    UINT cidl, 
    LPCITEMIDLIST *ppidl, 
    REFIID riid, 
    void **ppv)
{
    HRESULT hr;

    *ppv = NULL;                 // null the out param

    COfflineItems *pitems = new COfflineItems(pfolder, hwnd);
    if (pitems)
    {
        hr = pitems->Initialize(cidl, ppidl);
        if (SUCCEEDED(hr))
        {
            hr = pitems->QueryInterface(riid, ppv);
        }
        pitems->Release();
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}


HRESULT 
COfflineItems::QueryInterface(
    REFIID iid, 
    void **ppv
    )
{
    static const QITAB qit[] = {
        QITABENT(COfflineItems, IContextMenu),
        QITABENT(COfflineItems, IQueryInfo),
         { 0 },
    };
    return QISearch(this, qit, iid, ppv);
}


ULONG 
COfflineItems::AddRef(
    void
    )
{
    return InterlockedIncrement(&m_cRef);
}


ULONG 
COfflineItems::Release(
    void
    )
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;   
}

//
// IQueryInfo Methods -------------------------------------------------------
//
HRESULT 
COfflineItems::GetInfoTip(
    DWORD dwFlags, 
    WCHAR **ppwszTip
    )
{
    TCHAR szPath[MAX_PATH];
    return SHStrDup(COfflineFilesFolder::OLID_GetFullPath(m_ppolid[0], szPath, ARRAYSIZE(szPath)), ppwszTip);
}


HRESULT 
COfflineItems::GetInfoFlags(
    DWORD *pdwFlags
    )
{
    *pdwFlags = 0;  // BUGBUG: set this QIF_CACHED; 
    return S_OK;
}

//
// IContextMenu Methods -------------------------------------------------------
//
HRESULT 
COfflineItems::QueryContextMenu(
    HMENU hmenu, 
    UINT indexMenu, 
    UINT idCmdFirst,
    UINT idCmdLast, 
    UINT uFlags
    )
{
    USHORT cItems = 0;

    return ResultFromShort(cItems);    // number of menu items    
}

HRESULT
COfflineItems::InvokeCommand(
    LPCMINVOKECOMMANDINFO pici
    )
{
    HRESULT hr = S_OK;
    return hr;
}


HRESULT
COfflineItems::GetCommandString(
    UINT_PTR idCmd, 
    UINT uFlags, 
    UINT *pwReserved,
    LPSTR pszName, 
    UINT cchMax
    )
{
    HRESULT hr = E_FAIL;
    return hr;
}

