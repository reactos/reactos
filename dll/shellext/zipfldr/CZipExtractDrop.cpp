/*
 * PROJECT:     ReactOS Zip Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     IDataObject for dragging/copying files out of a ZIP folder
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"
#include "CZipExtractDrop.hpp"

static HRESULT GlobalClone(HGLOBAL* phDest, HGLOBAL hSrc)
{
    SIZE_T cb = GlobalSize(hSrc);
    HGLOBAL hDest = GlobalAlloc(GMEM_MOVEABLE, cb);
    if (!hDest)
        return E_OUTOFMEMORY;

    PVOID pSrc = GlobalLock(hSrc), pDest = GlobalLock(hDest);
    if (!pSrc || !pDest)
    {
        if (pSrc)
            GlobalUnlock(hSrc);
        if (pDest)
            GlobalUnlock(hDest);
        GlobalFree(hDest);
        return E_OUTOFMEMORY;
    }

    CopyMemory(pDest, pSrc, cb);
    GlobalUnlock(hSrc);
    GlobalUnlock(hDest);
    *phDest = hDest;
    return S_OK;
}

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
    // Calculate required buffer size
    SIZE_T cbPaths = 0;
    POSITION pos = paths.GetHeadPosition();
    while (pos)
        cbPaths += (paths.GetNext(pos).GetLength() + 1) * sizeof(WCHAR);
    cbPaths += sizeof(UNICODE_NULL); // final double NUL

    SIZE_T cbTotal = sizeof(DROPFILES) + cbPaths;
    HGLOBAL hGlobal = GlobalAlloc(GHND, cbTotal);
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

    PWCHAR pszDst = reinterpret_cast<PWCHAR>(pDrop + 1);
    pos = paths.GetHeadPosition();
    while (pos)
    {
        CStringW s = paths.GetNext(pos);
        INT cch = s.GetLength();
        CopyMemory(pszDst, s.GetString(), cch * sizeof(WCHAR));
        pszDst += cch;
        *pszDst++ = UNICODE_NULL;
    }
    *pszDst = UNICODE_NULL; // double NUL terminator

    GlobalUnlock(hGlobal);

    WCHAR szPath[MAX_PATH];
    DragQueryFileW((HDROP)hGlobal, 0, szPath, _countof(szPath));

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

    BYTE buf[2048];
    INT read;
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

class CZipExtractDrop :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDataObject
{
    CComPtr<CZipFolder> m_pFolder; // The ZIP folder we belong to
    CAtlArray<CStringW> m_selectedNames; // Names of the top-level items (relative to m_ZipDir)
    HGLOBAL m_hDropCache; // Cached HDROP after first extraction
    CStringW  m_tempDir; // Temporary directory created during extraction
    CComAutoCriticalSection m_csExtract;

    // Create a unique temporary directory and store it in m_tempDir.
    HRESULT CreateTempDir()
    {
        WCHAR szTempBase[MAX_PATH];
        if (!GetTempPathW(_countof(szTempBase), szTempBase))
            return E_FAIL;

        WCHAR szTempFile[MAX_PATH];
        if (!GetTempFileNameW(szTempBase, L"zfd", 0, szTempFile))
            return E_FAIL;

        DeleteFileW(szTempFile);
        if (!CreateDirectoryW(szTempFile, NULL))
            return E_FAIL;

        GetLongPathNameW(szTempFile, szTempFile, _countof(szTempFile));
        m_tempDir = szTempFile;
        return S_OK;
    }

    // Given the in-ZIP wide path (e.g. "src/foo/bar.c"), strip the ZipDir
    // prefix and return the part relative to the selected items, split into
    // the "top-level name" and the "rest" of the path.
    BOOL MatchesSelection(const CStringW& zipRelPath, CStringW& outTopName, CStringW& outRest) const
    {
        for (SIZE_T i = 0; i < m_selectedNames.GetCount(); ++i)
        {
            const CStringW& sel = m_selectedNames[i];
            BOOL isDir = (!sel.IsEmpty() && sel[sel.GetLength() - 1] == L'/');
            if (isDir)
            {
                // sel = "src/"
                // match if zipRelPath starts with "src/"
                if (StrCmpNIW(zipRelPath, sel, sel.GetLength()) == 0)
                {
                    outTopName = sel.Left(sel.GetLength() - 1); // "src"
                    outRest    = zipRelPath.Mid(sel.GetLength()); // "foo/bar.c"
                    return TRUE;
                }
            }
            else
            {
                if (StrCmpIW(zipRelPath, sel) == 0)
                {
                    outTopName = sel;
                    outRest    = L"";
                    return TRUE;
                }
            }
        }
        return FALSE;
    }

    // Do the actual extraction into m_tempDir.
    // Returns S_OK and fills m_hDropCache on success.
    HRESULT DoExtract()
    {
        HRESULT hr = CreateTempDir();
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        // We need a fresh unzFile because CZipFolder's handle may be in use.
        unzFile uf = unzOpen2_64(m_pFolder->GetZipFilePath(), &g_FFunc);
        if (!uf)
        {
            DPRINT1("!uf\n");
            return E_FAIL;
        }

        CAtlList<CStringW> extractedPaths; // top-level paths added to HDROP
        CAtlList<CStringW> topLevelAdded;  // de-dup guard for top-level dirs

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
            DPRINT1("!zipEnum.Initialize\n");
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
            if (StrCmpNIW(entryName, prefix, (INT)cchPrefix) != 0)
                continue;

            CStringW relPath = entryName.Mid((INT)cchPrefix);
            CStringW topName, rest;
            if (relPath.IsEmpty() || !MatchesSelection(relPath, topName, rest))
                continue;

            BOOL isZipDir = (!entryName.IsEmpty() && entryName[entryName.GetLength() - 1] == L'/');

            // Construct full destination
            CStringW destRel = relPath; // "src/foo/bar.c"
            destRel.Replace(L'/', L'\\');

            // SECURITY: Reject absolute, drive-letter, or traversal paths
            if (!PathIsRelativeW(destRel) || destRel.Find(L':') >= 0 ||
                destRel == L"." || destRel == L".." || destRel.Find(L"..\\") == 0 ||
                destRel.Find(L"\\..\\") >= 0 || destRel.Right(3) == L"\\..")
            {
                continue;
            }

            WCHAR destFull[MAX_PATH];
            PathCombineW(destFull, m_tempDir, destRel);

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
                }
                else
                {
                    PathRemoveFileSpecW(parentDir.GetBuffer(MAX_PATH));
                    parentDir.ReleaseBuffer();
                    SHPathPrepareForWriteW(NULL, NULL, parentDir, SHPPFW_DIRCREATE);
                }
            }

            // Open + extract the file
            if (unzOpenCurrentFile(uf) == UNZ_OK)
            {
                BOOL ok = WriteZipEntryToFile(uf, destFull);
                unzCloseCurrentFile(uf);
                if (!ok)
                    continue;

                // Update file timestamp
                FILETIME ftLocal, ftUtc;
                DosDateTimeToFileTime(HIWORD(info.dosDate), LOWORD(info.dosDate), &ftLocal);
                LocalFileTimeToFileTime(&ftLocal, &ftUtc);
                HANDLE hFile = CreateFileW(destFull, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                                           OPEN_EXISTING, 0, NULL);
                if (hFile != INVALID_HANDLE_VALUE)
                {
                    SetFileTime(hFile, &ftUtc, &ftUtc, &ftUtc);
                    CloseHandle(hFile);
                }
            }

            // Add the top-level item to the HDROP list (once).
            WCHAR topDest[MAX_PATH];
            PathCombineW(topDest, m_tempDir, topName);

            // Check duplicate
            BOOL found = FALSE;
            POSITION pos = topLevelAdded.GetHeadPosition();
            while (pos)
            {
                if (StrCmpIW(topLevelAdded.GetNext(pos), topName) == 0)
                {
                    found = TRUE;
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
    }

    HRESULT Init(CZipFolder* pFolder, UINT cidl, PCUITEMID_CHILD_ARRAY apidl)
    {
        ATLASSERT(pFolder);
        m_pFolder = pFolder;

        for (UINT i = 0; i < cidl; ++i)
        {
            const ZipPidlEntry* pEntry = _ZipFromIL(apidl[i]);
            if (!pEntry)
                continue;

            CStringW name = pEntry->Name;
            name.Replace(L'\\', L'/');
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

    DECLARE_NOT_AGGREGATABLE(CZipExtractDrop)
    BEGIN_COM_MAP(CZipExtractDrop)
        COM_INTERFACE_ENTRY_IID(IID_IDataObject, IDataObject)
    END_COM_MAP()

    STDMETHODIMP GetData(FORMATETC* pfe, STGMEDIUM* pstm) override
    {
        if (!pfe || !pstm)
            return E_INVALIDARG;

        ZeroMemory(pstm, sizeof(*pstm));

        // Handle CF_HDROP ourselves
        if (pfe->cfFormat == CF_HDROP && (pfe->tymed & TYMED_HGLOBAL) &&
            pfe->dwAspect == DVASPECT_CONTENT)
        {
            CComCritSecLock<CComAutoCriticalSection> lock(m_csExtract);

            // Extract on first request
            if (!m_hDropCache)
            {
                HRESULT hr = DoExtract();
                if (FAILED_UNEXPECTEDLY(hr))
                    return hr;
            }

            // Duplicate the cached HGLOBAL
            HGLOBAL hCopy;
            HRESULT hr = GlobalClone(&hCopy, m_hDropCache);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            pstm->tymed   = TYMED_HGLOBAL;
            pstm->hGlobal = hCopy;
            pstm->pUnkForRelease = NULL;
            return S_OK;
        }

        return DV_E_FORMATETC;
    }

    STDMETHODIMP GetDataHere(FORMATETC* pfe, STGMEDIUM* pstm) override
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP QueryGetData(FORMATETC* pfe) override
    {
        if (!pfe)
            return E_INVALIDARG;

        if (pfe->cfFormat == CF_HDROP && (pfe->tymed & TYMED_HGLOBAL) &&
            pfe->dwAspect == DVASPECT_CONTENT)
        {
            return S_OK;
        }

        return DV_E_FORMATETC;
    }

    STDMETHODIMP GetCanonicalFormatEtc(FORMATETC* pfeIn, FORMATETC* pfeOut) override
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP SetData(FORMATETC* pfe, STGMEDIUM* pstm, BOOL fRelease) override
    {
        if (!pfe || !pstm)
            return E_INVALIDARG;

        if (pstm->tymed == TYMED_HGLOBAL && pstm->hGlobal)
        {
            if (fRelease)
                ReleaseStgMedium(pstm);
            return S_OK;
        }

        return E_NOTIMPL;
    }

    STDMETHODIMP EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppenum) override
    {
        if (dwDirection != DATADIR_GET)
            return E_NOTIMPL;

        FORMATETC format[] = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        return SHCreateStdEnumFmtEtc(1, format, ppenum);
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

// Factory function
HRESULT CZipExtractDrop_CreateInstance(
    CZipFolder*           pFolder,
    UINT                  cidl,
    PCUITEMID_CHILD_ARRAY apidl,
    IDataObject**         ppDataObject)
{
    if (!ppDataObject)
        return E_POINTER;

    *ppDataObject = NULL;

    if (!pFolder || !cidl || !apidl)
        return E_INVALIDARG;

    CComObject<CZipExtractDrop>* pObj = NULL;
    HRESULT hr = CComObject<CZipExtractDrop>::CreateInstance(&pObj);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    pObj->AddRef();
    hr = pObj->Init(pFolder, cidl, apidl);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        pObj->Release();
        return hr;
    }

    *ppDataObject = static_cast<IDataObject*>(pObj);
    return S_OK;
}
