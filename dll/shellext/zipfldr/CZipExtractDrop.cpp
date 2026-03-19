/*
 * PROJECT:     ReactOS Zip Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     IDataObject for dragging/copying files out of a ZIP folder
 * COPYRIGHT:   Copyright 2024-2026 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

/*
 * Design notes
 * ------------
 * When the shell asks for IDataObject from items inside a virtual folder it
 * uses CF_HDROP to actually copy/move the data to a real directory.  Because
 * ZIP entries are not real files we must extract them on-demand.
 *
 * Strategy
 * ~~~~~~~~
 * 1.  CZipExtractDrop holds a list of ZipPidlEntry names (relative paths
 *     inside the ZIP) and a back-pointer to the CZipFolder (for m_ZipFile,
 *     m_ZipDir and getZip()).
 * 2.  The first time GetData(CF_HDROP) is called we
 *     a. create a unique temporary directory under %TEMP%\zipfldr_XXXXXX\
 *     b. enumerate all entries whose path starts with one of the requested
 *        names (handles both single files and whole sub-trees),
 *     c. extract each one while translating its in-ZIP name via
 *        CZipEnumerator (so UTF-8 / code-page names are handled correctly),
 *     d. build an HDROP containing all extracted paths, and cache it.
 * 3.  The temporary directory is removed in the destructor (recursive delete).
 * 4.  All other clipboard formats are delegated to a standard
 *     CIDLData_CreateFromIDArray data-object so that shell operations that
 *     rely on PIDLs (e.g. properties) continue to work.
 */

#include "precomp.h"
#include "CZipExtractDrop.hpp"

// --------------------------------------------------------------------------
// Helpers
// --------------------------------------------------------------------------

// Recursively delete a directory tree (like SHFileOperation with FO_DELETE,
// but without any UI and without depending on shell32 internals).
static void RecursiveDeleteDirectory(PCWSTR pszDir)
{
    WCHAR szFind[MAX_PATH];
    StringCbCopyW(szFind, sizeof(szFind), pszDir);
    PathAppendW(szFind, L"*");

    WIN32_FIND_DATAW wfd;
    HANDLE hFind = FindFirstFileW(szFind, &wfd);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        RemoveDirectoryW(pszDir);
        return;
    }
    do
    {
        if (wcscmp(wfd.cFileName, L".") == 0 || wcscmp(wfd.cFileName, L"..") == 0)
            continue;

        WCHAR szChild[MAX_PATH];
        StringCbCopyW(szChild, sizeof(szChild), pszDir);
        PathAppendW(szChild, wfd.cFileName);

        if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            RecursiveDeleteDirectory(szChild);
        else
            DeleteFileW(szChild);
    }
    while (FindNextFileW(hFind, &wfd));
    FindClose(hFind);

    RemoveDirectoryW(pszDir);
}

// Build an HDROP from a list of NUL-terminated wide strings.
// HDROP layout: DROPFILES header + double-NUL-terminated list of wchar paths.
static HGLOBAL BuildHDrop(const CAtlList<CStringW>& paths)
{
    // Calculate required buffer size.
    SIZE_T cbPaths = 0;
    POSITION pos = paths.GetHeadPosition();
    while (pos)
        cbPaths += (paths.GetNext(pos).GetLength() + 1) * sizeof(WCHAR);
    cbPaths += sizeof(WCHAR); // final double NUL

    SIZE_T cbTotal = sizeof(DROPFILES) + cbPaths;
    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, cbTotal);
    if (!hGlobal)
        return NULL;

    DROPFILES* pDrop = static_cast<DROPFILES*>(GlobalLock(hGlobal));
    if (!pDrop)
    {
        GlobalFree(hGlobal);
        return NULL;
    }

    pDrop->pFiles = sizeof(DROPFILES);
    pDrop->fWide  = TRUE;

    WCHAR* pszDst = reinterpret_cast<WCHAR*>(pDrop + 1);
    pos = paths.GetHeadPosition();
    while (pos)
    {
        const CStringW& s = paths.GetNext(pos);
        INT cch = s.GetLength();
        CopyMemory(pszDst, s.GetString(), cch * sizeof(WCHAR));
        pszDst += cch;
        *pszDst++ = L'\0';
    }
    *pszDst = L'\0'; // double NUL terminator

    GlobalUnlock(hGlobal);
    return hGlobal;
}

// Write raw data from the current open zip entry to a file.
// Returns TRUE on success.
static BOOL WriteZipEntryToFile(unzFile uf, PCWSTR pszDestFile)
{
    HANDLE hFile = CreateFileW(pszDestFile, GENERIC_WRITE, 0, NULL,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    BYTE buf[65536];
    int read;
    BOOL ok = TRUE;
    while ((read = unzReadCurrentFile(uf, buf, sizeof(buf))) > 0)
    {
        DWORD written;
        if (!WriteFile(hFile, buf, (DWORD)read, &written, NULL) || written != (DWORD)read)
        {
            ok = FALSE;
            break;
        }
    }
    if (read < 0)
        ok = FALSE;

    CloseHandle(hFile);
    if (!ok)
        DeleteFileW(pszDestFile);
    return ok;
}

// --------------------------------------------------------------------------
// CZipExtractDrop
// --------------------------------------------------------------------------

class CZipExtractDrop :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDataObject
{
    // The ZIP folder we belong to (not AddRef'd – the shell keeps both alive).
    CZipFolder*           m_pFolder;

    // Names of the top-level items that were selected (relative to m_ZipDir).
    // e.g. { L"readme.txt", L"src/" }
    CAtlArray<CStringW>   m_selectedNames;

    // Fallback data-object (for PIDL-based formats).
    CComPtr<IDataObject>  m_spInner;

    // Cached HDROP after first extraction.
    HGLOBAL               m_hDropCache;

    // Temporary directory created during extraction.
    CStringW              m_tempDir;

    // Storage for arbitrary SetData() calls (e.g. DataObjectAttributes cache
    // written by SHGetAttributesFromDataObject in shldataobject.cpp).
    // Only TYMED_HGLOBAL entries are stored.
    struct ExtraEntry
    {
        CLIPFORMAT cfFormat;
        HGLOBAL    hData;   // owned by us
    };
    CAtlArray<ExtraEntry> m_extraData;

    // ---------- extraction helpers ----------

    // Create a unique temporary directory and store it in m_tempDir.
    HRESULT CreateTempDir()
    {
        WCHAR szTempBase[MAX_PATH];
        if (!GetTempPathW(_countof(szTempBase), szTempBase))
            return HRESULT_FROM_WIN32(GetLastError());

        // Try up to 100 unique names.
        for (int i = 0; i < 100; ++i)
        {
            CStringW candidate;
            candidate.Format(L"%szipfldr_%08X", szTempBase, GetTickCount() + i);
            if (CreateDirectoryW(candidate, NULL))
            {
                m_tempDir = candidate;
                return S_OK;
            }
        }
        return E_FAIL;
    }

    // Given the in-ZIP wide path (e.g. "src/foo/bar.c"), strip the ZipDir
    // prefix and return the part relative to the selected items, split into
    // the "top-level name" and the "rest" of the path.
    //
    // zip path:  "subdir/src/foo/bar.c"   ZipDir: "subdir/"
    // rel path:  "src/foo/bar.c"
    // top name:  "src"  (matches selected name "src/")
    // rest:      "foo/bar.c"
    //
    // Returns FALSE if the entry does not belong to any selected item.
    bool MatchesSelection(const CStringW& zipRelPath,
                          CStringW& outTopName,
                          CStringW& outRest) const
    {
        for (SIZE_T i = 0; i < m_selectedNames.GetCount(); ++i)
        {
            const CStringW& sel = m_selectedNames[i];
            // sel may end with '/' (directory) or not (file).
            // For files: exact match.
            // For directories: prefix match followed by '/'.
            bool isDir = (!sel.IsEmpty() && sel[sel.GetLength() - 1] == L'/');

            if (isDir)
            {
                // sel = "src/"
                // match if zipRelPath starts with "src/"
                if (StrCmpNIW(zipRelPath, sel, sel.GetLength()) == 0)
                {
                    outTopName = sel.Left(sel.GetLength() - 1); // "src"
                    outRest    = zipRelPath.Mid(sel.GetLength()); // "foo/bar.c"
                    return true;
                }
            }
            else
            {
                if (StrCmpIW(zipRelPath, sel) == 0)
                {
                    outTopName = sel;
                    outRest    = L"";
                    return true;
                }
            }
        }
        return false;
    }

    // Do the actual extraction into m_tempDir.
    // Returns S_OK and fills m_hDropCache on success.
    HRESULT DoExtract()
    {
        HRESULT hr = CreateTempDir();
        if (FAILED(hr))
            return hr;

        // We need a fresh unzFile because CZipFolder's handle may be in use.
        unzFile uf = unzOpen2_64(
            // CZipFolder exposes m_ZipFile through a public accessor; we reach
            // it via the friend declaration added in CZipFolder.hpp.
            m_pFolder->GetZipFilePath(),
            &g_FFunc);
        if (!uf)
            return E_FAIL;

        CAtlList<CStringW>  extractedPaths; // top-level paths added to HDROP
        CAtlList<CStringW>  topLevelAdded;  // de-dup guard for top-level dirs

        CZipEnumerator zipEnum;
        // CZipEnumerator::Initialize wants an IZip*; CZipFolder implements IZip.
        // Use a thin wrapper so we don't disturb the folder's own unzFile.
        struct LocalIZip : IZip
        {
            unzFile m_uf;
            LocalIZip(unzFile uf) : m_uf(uf) {}
            STDMETHODIMP QueryInterface(REFIID, void**) { return E_NOINTERFACE; }
            STDMETHODIMP_(ULONG) AddRef()  { return 1; }
            STDMETHODIMP_(ULONG) Release() { return 1; }
            STDMETHODIMP_(unzFile) getZip() { return m_uf; }
        } localZip(uf);

        if (!zipEnum.Initialize(&localZip))
        {
            unzClose(uf);
            return E_FAIL;
        }

        // Prefix = m_ZipDir (e.g. "" for root, "subdir/" for sub-folder view)
        const CStringW& prefix = m_pFolder->GetZipDir();
        SIZE_T cchPrefix = prefix.GetLength();

        CStringW entryName;
        unz_file_info64 info;
        while (zipEnum.Next(entryName, info))
        {
            // entryName is the full path inside the ZIP (UTF-8/CP decoded).
            // Strip the current folder prefix.
            if (StrCmpNIW(entryName, prefix, (int)cchPrefix) != 0)
                continue;

            CStringW relPath = entryName.Mid((int)cchPrefix);
            if (relPath.IsEmpty())
                continue;

            CStringW topName, rest;
            if (!MatchesSelection(relPath, topName, rest))
                continue;

            // Build the destination path inside m_tempDir.
            // top-level files/dirs go directly into m_tempDir.
            // e.g.  m_tempDir\readme.txt
            //        m_tempDir\src\foo\bar.c
            bool isZipDir = (!entryName.IsEmpty() &&
                              entryName[entryName.GetLength() - 1] == L'/');

            // Construct full destination
            CStringW destRel = relPath; // "src/foo/bar.c"
            destRel.Replace(L'/', L'\\');

            CStringW destFull = m_tempDir + L'\\' + destRel;

            // Ensure parent directories exist
            {
                CStringW parentDir = destFull;
                // PathRemoveFileSpec won't work for dirs ending with \; handled below.
                if (isZipDir)
                {
                    // Entry itself is a directory – just create it.
                    SHPathPrepareForWriteW(NULL, NULL, destFull,
                                           SHPPFW_DIRCREATE | SHPPFW_IGNOREFILENAME);
                    // Create the directory itself (last component).
                    CreateDirectoryW(destFull, NULL);
                    goto next_entry;
                }
                else
                {
                    PathRemoveFileSpecW(parentDir.GetBuffer(MAX_PATH));
                    parentDir.ReleaseBuffer();
                    SHPathPrepareForWriteW(NULL, NULL, parentDir,
                                           SHPPFW_DIRCREATE);
                }
            }

            // Open + extract the file
            if (unzOpenCurrentFile(uf) == UNZ_OK)
            {
                BOOL ok = WriteZipEntryToFile(uf, destFull);
                unzCloseCurrentFile(uf);

                if (ok)
                {
                    // Update file timestamp
                    FILETIME ftLocal, ftUtc;
                    DosDateTimeToFileTime((WORD)(info.dosDate >> 16),
                                          (WORD)info.dosDate, &ftLocal);
                    LocalFileTimeToFileTime(&ftLocal, &ftUtc);
                    HANDLE hFile = CreateFileW(destFull, GENERIC_WRITE,
                                               FILE_SHARE_READ, NULL,
                                               OPEN_EXISTING, 0, NULL);
                    if (hFile != INVALID_HANDLE_VALUE)
                    {
                        SetFileTime(hFile, &ftUtc, &ftUtc, &ftUtc);
                        CloseHandle(hFile);
                    }
                }
            }

        next_entry:
            // Add the top-level item to the HDROP list (once).
            CStringW topDest = m_tempDir + L'\\' + topName;
            // Check duplicate
            bool found = false;
            POSITION pos = topLevelAdded.GetHeadPosition();
            while (pos)
            {
                if (StrCmpIW(topLevelAdded.GetNext(pos), topName) == 0)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                topLevelAdded.AddTail(topName);
                extractedPaths.AddTail(topDest);
            }
        }

        unzClose(uf);

        if (extractedPaths.IsEmpty())
            return E_FAIL;

        m_hDropCache = BuildHDrop(extractedPaths);
        return m_hDropCache ? S_OK : E_OUTOFMEMORY;
    }

public:
    CZipExtractDrop()
        : m_pFolder(NULL)
        , m_hDropCache(NULL)
    {
    }

    ~CZipExtractDrop()
    {
        if (m_hDropCache)
        {
            GlobalFree(m_hDropCache);
            m_hDropCache = NULL;
        }

        if (!m_tempDir.IsEmpty())
            RecursiveDeleteDirectory(m_tempDir);

        for (SIZE_T i = 0; i < m_extraData.GetCount(); ++i)
            GlobalFree(m_extraData[i].hData);
    }

    HRESULT Init(CZipFolder* pFolder,
                 UINT cidl,
                 PCUITEMID_CHILD_ARRAY apidl,
                 IDataObject* pInner)
    {
        ATLASSERT(pFolder);
        m_pFolder = pFolder;
        m_spInner = pInner;

        for (UINT i = 0; i < cidl; ++i)
        {
            const ZipPidlEntry* pEntry = _ZipFromIL(apidl[i]);
            if (!pEntry)
                continue;

            CStringW name = pEntry->Name;
            if (pEntry->ZipType == ZIP_PIDL_DIRECTORY)
            {
                // Ensure trailing slash so MatchesSelection works as prefix.
                if (name.IsEmpty() || name[name.GetLength() - 1] != L'/')
                    name += L'/';
            }
            m_selectedNames.Add(name);
        }
        return S_OK;
    }

    // ---- IUnknown (via CComObjectRootEx) ----
    DECLARE_NOT_AGGREGATABLE(CZipExtractDrop)
    BEGIN_COM_MAP(CZipExtractDrop)
        COM_INTERFACE_ENTRY_IID(IID_IDataObject, IDataObject)
    END_COM_MAP()

    // ---- IDataObject ----
    STDMETHODIMP GetData(FORMATETC* pfe, STGMEDIUM* pstm) override
    {
        if (!pfe || !pstm)
            return E_INVALIDARG;

        ZeroMemory(pstm, sizeof(*pstm));

        // Handle CF_HDROP ourselves.
        if (pfe->cfFormat == CF_HDROP &&
            (pfe->tymed & TYMED_HGLOBAL) &&
            pfe->dwAspect == DVASPECT_CONTENT)
        {
            // Extract on first request.
            if (!m_hDropCache)
            {
                HRESULT hr = DoExtract();
                if (FAILED(hr))
                    return hr;
            }

            // Duplicate the cached HGLOBAL.
            SIZE_T cb = GlobalSize(m_hDropCache);
            HGLOBAL hCopy = GlobalAlloc(GMEM_MOVEABLE, cb);
            if (!hCopy)
                return E_OUTOFMEMORY;

            void* pSrc = GlobalLock(m_hDropCache);
            void* pDst = GlobalLock(hCopy);
            if (!pSrc || !pDst)
            {
                if (pSrc) GlobalUnlock(m_hDropCache);
                if (pDst) GlobalUnlock(hCopy);
                GlobalFree(hCopy);
                return E_OUTOFMEMORY;
            }
            CopyMemory(pDst, pSrc, cb);
            GlobalUnlock(m_hDropCache);
            GlobalUnlock(hCopy);

            pstm->tymed   = TYMED_HGLOBAL;
            pstm->hGlobal = hCopy;
            pstm->pUnkForRelease = NULL;
            return S_OK;
        }

        // Check our own extra-data store (for formats cached via SetData).
        if (pfe->tymed & TYMED_HGLOBAL)
        {
            for (SIZE_T i = 0; i < m_extraData.GetCount(); ++i)
            {
                if (m_extraData[i].cfFormat == pfe->cfFormat)
                {
                    SIZE_T cb = GlobalSize(m_extraData[i].hData);
                    HGLOBAL hCopy = GlobalAlloc(GMEM_MOVEABLE, cb);
                    if (!hCopy)
                        return E_OUTOFMEMORY;
                    void* pSrc = GlobalLock(m_extraData[i].hData);
                    void* pDst = GlobalLock(hCopy);
                    if (pSrc && pDst)
                        CopyMemory(pDst, pSrc, cb);
                    if (pSrc) GlobalUnlock(m_extraData[i].hData);
                    if (pDst) GlobalUnlock(hCopy);
                    if (!pSrc || !pDst) { GlobalFree(hCopy); return E_OUTOFMEMORY; }

                    pstm->tymed   = TYMED_HGLOBAL;
                    pstm->hGlobal = hCopy;
                    pstm->pUnkForRelease = NULL;
                    return S_OK;
                }
            }
        }

        // Delegate everything else to the inner (PIDL-based) data object.
        if (m_spInner)
            return m_spInner->GetData(pfe, pstm);

        return DV_E_FORMATETC;
    }

    STDMETHODIMP GetDataHere(FORMATETC* pfe, STGMEDIUM* pstm) override
    {
        if (m_spInner)
            return m_spInner->GetDataHere(pfe, pstm);
        return E_NOTIMPL;
    }

    STDMETHODIMP QueryGetData(FORMATETC* pfe) override
    {
        if (!pfe)
            return E_INVALIDARG;

        if (pfe->cfFormat == CF_HDROP &&
            (pfe->tymed & TYMED_HGLOBAL) &&
            pfe->dwAspect == DVASPECT_CONTENT)
        {
            return S_OK;
        }

        // Check our own extra-data store.
        if (pfe->tymed & TYMED_HGLOBAL)
        {
            for (SIZE_T i = 0; i < m_extraData.GetCount(); ++i)
                if (m_extraData[i].cfFormat == pfe->cfFormat)
                    return S_OK;
        }

        if (m_spInner)
            return m_spInner->QueryGetData(pfe);
        return DV_E_FORMATETC;
    }

    STDMETHODIMP GetCanonicalFormatEtc(FORMATETC* pfeIn, FORMATETC* pfeOut) override
    {
        if (m_spInner)
            return m_spInner->GetCanonicalFormatEtc(pfeIn, pfeOut);
        return E_NOTIMPL;
    }

    STDMETHODIMP SetData(FORMATETC* pfe, STGMEDIUM* pstm, BOOL fRelease) override
    {
        if (!pfe || !pstm)
            return E_INVALIDARG;

        // We only handle HGLOBAL storage (covers DataObjectAttributes and similar).
        if (pstm->tymed == TYMED_HGLOBAL && pstm->hGlobal)
        {
            // Copy the data into our own HGLOBAL so we own it regardless of fRelease.
            SIZE_T cb = GlobalSize(pstm->hGlobal);
            HGLOBAL hCopy = GlobalAlloc(GMEM_MOVEABLE, cb);
            if (!hCopy)
                return E_OUTOFMEMORY;

            void* pSrc = GlobalLock(pstm->hGlobal);
            void* pDst = GlobalLock(hCopy);
            if (pSrc && pDst)
                CopyMemory(pDst, pSrc, cb);
            if (pSrc) GlobalUnlock(pstm->hGlobal);
            if (pDst) GlobalUnlock(hCopy);
            if (!pSrc || !pDst) { GlobalFree(hCopy); return E_OUTOFMEMORY; }

            // Release source medium if requested.
            if (fRelease)
                ReleaseStgMedium(pstm);

            // Update existing slot or append new one.
            for (SIZE_T i = 0; i < m_extraData.GetCount(); ++i)
            {
                if (m_extraData[i].cfFormat == pfe->cfFormat)
                {
                    GlobalFree(m_extraData[i].hData);
                    m_extraData[i].hData = hCopy;
                    return S_OK;
                }
            }
            ExtraEntry entry = { pfe->cfFormat, hCopy };
            m_extraData.Add(entry);
            return S_OK;
        }

        // For non-HGLOBAL types, try the inner object.
        if (m_spInner)
            return m_spInner->SetData(pfe, pstm, fRelease);

        return E_NOTIMPL;
    }

    STDMETHODIMP EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppenum) override
    {
        if (dwDirection == DATADIR_GET)
        {
            // Advertise CF_HDROP in addition to whatever the inner object offers.
            FORMATETC hdropFmt = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

            if (m_spInner)
            {
                // Prepend CF_HDROP then let the inner enumerator take over.
                // Simple approach: collect inner formats, prepend ours.
                CComPtr<IEnumFORMATETC> spInnerEnum;
                if (SUCCEEDED(m_spInner->EnumFormatEtc(DATADIR_GET, &spInnerEnum)))
                {
                    CAtlArray<FORMATETC> fmts;
                    fmts.Add(hdropFmt);
                    FORMATETC fe;
                    while (spInnerEnum->Next(1, &fe, NULL) == S_OK)
                        fmts.Add(fe);

                    return SHCreateStdEnumFmtEtc((UINT)fmts.GetCount(),
                                                  fmts.GetData(), ppenum);
                }
            }

            return SHCreateStdEnumFmtEtc(1, &hdropFmt, ppenum);
        }
        return E_NOTIMPL;
    }

    STDMETHODIMP DAdvise(FORMATETC*, DWORD, IAdviseSink*, DWORD*) override
    {
        return OLE_E_ADVISENOTSUPPORTED;
    }

    STDMETHODIMP DUnadvise(DWORD) override
    {
        return OLE_E_ADVISENOTSUPPORTED;
    }

    STDMETHODIMP EnumDAdvise(IEnumSTATDATA**) override
    {
        return OLE_E_ADVISENOTSUPPORTED;
    }
};

// --------------------------------------------------------------------------
// Factory function
// --------------------------------------------------------------------------

HRESULT CZipExtractDrop_CreateInstance(
    CZipFolder*           pFolder,
    UINT                  cidl,
    PCUITEMID_CHILD_ARRAY apidl,
    IDataObject**         ppDataObject)
{
    if (!ppDataObject)
        return E_POINTER;
    *ppDataObject = NULL;

    if (!pFolder || cidl == 0 || !apidl)
        return E_INVALIDARG;

    // Build the inner (PIDL-based) data object first.
    CComPtr<IDataObject> spInner;
    CIDLData_CreateFromIDArray(pFolder->GetCurDirPidl(), cidl, apidl, &spInner);
    // Inner may legitimately fail (no PIDL); we carry on regardless.

    CComObject<CZipExtractDrop>* pObj = NULL;
    HRESULT hr = CComObject<CZipExtractDrop>::CreateInstance(&pObj);
    if (FAILED(hr))
        return hr;

    pObj->AddRef();
    hr = pObj->Init(pFolder, cidl, apidl, spInner);
    if (FAILED(hr))
    {
        pObj->Release();
        return hr;
    }

    *ppDataObject = static_cast<IDataObject*>(pObj);
    return S_OK;
}
