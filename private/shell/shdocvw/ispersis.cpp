/*
 * ispersis.cpp - IPersist, IPersistFile, and IPersistStream implementations for
 *               URL class.
 */


#include "priv.h"
#include "ishcut.h"

#include "resource.h"

// Need to flush the file to prevent win95 from barfing after stuff is written in
VOID FlushFile(LPCTSTR pszFile)
{
    if (!g_fRunningOnNT)
    {
        WritePrivateProfileString(NULL, NULL, NULL, pszFile);
    }
}


// save object to file

STDMETHODIMP Intshcut::SaveToFile(LPCTSTR pszFile, BOOL bRemember)
{
    HRESULT hres = InitProp();
    if (SUCCEEDED(hres))
    {
        m_pprop->SetFileName(pszFile);

        hres = m_pprop->Commit(STGC_DEFAULT);

        // Remember file if requested 

        if (SUCCEEDED(hres) && bRemember)
        {
            Dirty(FALSE);

            if ( !Str_SetPtr(&m_pszFile, pszFile) )
                hres = E_OUTOFMEMORY;

            SHChangeNotify(SHCNE_UPDATEITEM, (SHCNF_PATH | SHCNF_FLUSH), pszFile, NULL);
#ifdef DEBUG
            Dump();
#endif
        }

        if (!bRemember)
            m_pprop->SetFileName(m_pszFile);
    }

    if(pszFile && (S_OK == hres))
        FlushFile(pszFile);
    return hres;
}

STDMETHODIMP Intshcut::LoadFromFile(LPCTSTR pszFile)
{
    HRESULT hres;

    if (Str_SetPtr(&m_pszFile, pszFile))
    {
        hres = InitProp();
#ifdef DEBUG
        Dump();
#endif
    }
    else
    {
        hres = E_OUTOFMEMORY;
    }

    return hres;
}


STDMETHODIMP Intshcut::LoadFromAsyncFileNow()
{
    HRESULT hres = S_OK;
    if (m_pszFileToLoad)
    {
        hres = LoadFromFile(m_pszFileToLoad);
        Str_SetPtr(&m_pszFileToLoad, NULL);
    }    
    return hres;
}

STDMETHODIMP Intshcut::GetCurFile(LPTSTR pszFile, UINT cchLen)
{
    HRESULT hr;

    if (m_pszFile)
    {
        StrCpyN(pszFile, m_pszFile, cchLen);
        hr = S_OK;
    }
    else
        hr = S_FALSE;

    return hr;
}

STDMETHODIMP Intshcut::Dirty(BOOL bDirty)
{
    HRESULT hres;
    
    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    
    if (bDirty)
    {
        if (IsFlagClear(m_dwFlags, ISF_DIRTY))
            TraceMsg(TF_INTSHCUT, "Intshcut now dirty.");
        
        SetFlag(m_dwFlags, ISF_DIRTY);
    }
    else
    {
        if (IsFlagSet(m_dwFlags, ISF_DIRTY))
            TraceMsg(TF_INTSHCUT, "Intshcut now clean.");
        
        ClearFlag(m_dwFlags, ISF_DIRTY);
    }
    
    hres = S_OK;
    
    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    
    return hres;
}

// IPersist::GetClassID method for Intshcut

STDMETHODIMP Intshcut::GetClassID(CLSID *pclsid)
{
    ASSERT(IS_VALID_WRITE_PTR(pclsid, CLSID));

    *pclsid = CLSID_InternetShortcut;
    return S_OK;
}


// IPersistFile::IsDirty handler for Intshcut

STDMETHODIMP Intshcut::IsDirty(void)
{
    HRESULT hres = LoadFromAsyncFileNow();

    if(SUCCEEDED(hres))
    {
        hres = InitProp();
        if (SUCCEEDED(hres))
        {
            if (IsFlagSet(m_dwFlags, ISF_DIRTY) || S_OK == m_pprop->IsDirty())
                hres = S_OK;
            else
                hres = S_FALSE;
        }
    }
    return hres;
}

// Helper function to save off Trident specific stuff 

STDMETHODIMP Intshcut::_SaveOffPersistentDataFromSite()
{
    IOleCommandTarget *pcmdt = NULL;
    HRESULT hr = S_OK;
    if (_punkSite)
    {
        if(S_OK == _CreateTemporaryBackingFile())
        {
            ASSERT(m_pszTempFileName);
            hr = _punkSite->QueryInterface(IID_IOleCommandTarget, (LPVOID *)&pcmdt);
            if((S_OK == hr))
            {
                ASSERT(pcmdt);
                VARIANT varIn = {0};
                varIn.vt = VT_UNKNOWN;
                varIn.punkVal = (LPUNKNOWN)(SAFECAST(this, IUniformResourceLocator *));
                
                // Tell the site to save off it's persistent stuff
                hr = pcmdt->Exec(&CGID_ShortCut, CMDID_INTSHORTCUTCREATE, 0, &varIn, NULL);
                
                pcmdt->Release();
            }
            FlushFile(m_pszTempFileName);
        }
    }
    return hr;
}

// IPersistFile::Save handler for Intshcut

STDMETHODIMP Intshcut::Save(LPCOLESTR pwszFile, BOOL bRemember)
{
    HRESULT hres = LoadFromAsyncFileNow();
    if (SUCCEEDED(hres))
    {
        TCHAR szFile[MAX_PATH];

        if (pwszFile)
            SHUnicodeToTChar(pwszFile, szFile, SIZECHARS(szFile));
        else if (m_pszFile)
            StrCpyN(szFile, m_pszFile, ARRAYSIZE(szFile));
        else
            return E_FAIL;
        
        // Perhaps there is a site which wants to save off stuff ?
        // However, the site may end up calling via intefaces 
        hres = _SaveOffPersistentDataFromSite();

        if ((S_OK == hres) && (m_pszTempFileName) && (StrCmp(m_pszTempFileName, szFile) != 0))
        {
            // Copy contents of the temp file to the destination
            // if they are different files
            EVAL(CopyFile(m_pszTempFileName, szFile, FALSE));
        }

        // Then save off in memory stuff to this file
        hres = SaveToFile(szFile, bRemember);
    }
    return hres;
}

STDMETHODIMP Intshcut::SaveCompleted(LPCOLESTR pwszFile)
{
    return S_OK;
}

// IPersistFile::Load()

STDMETHODIMP Intshcut::Load(LPCOLESTR pwszFile, DWORD dwMode)
{
    TCHAR szFile[MAX_PATH];
    
    SHUnicodeToTChar(pwszFile, szFile, SIZECHARS(szFile));

    HRESULT hres;
    
    if (m_fMustLoadSync)
        hres = LoadFromFile(szFile);
    else
    {
        if (Str_SetPtr(&m_pszFileToLoad, szFile))
            hres = S_OK;
        else
            hres = E_OUTOFMEMORY;

        // we can only do this once, to protect against people
        // re-using the same object for different files, 
        // (multiple calls to IPersistFile::Load())

        m_fMustLoadSync = TRUE; 
    }
    return hres;
}

// IPersistFile::GetCurFile method for Intshcut

STDMETHODIMP Intshcut::GetCurFile(WCHAR **ppwszFile)
{
    HRESULT hr;
    TCHAR szTempFile[MAX_PATH];

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    ASSERT(IS_VALID_WRITE_PTR(ppwszFile, LPOLESTR));

    hr = LoadFromAsyncFileNow();
    if (FAILED(hr))
        return hr;

    if (m_pszFile)
    {
        StrCpyN(szTempFile, m_pszFile, SIZECHARS(szTempFile));
        hr = S_OK;
    }
    else
    {
        StrCpyN(szTempFile, TEXT("*.url"), ARRAYSIZE(szTempFile));
        hr = S_FALSE;
    }

    HRESULT hrTemp = SHStrDup(szTempFile, ppwszFile);
    if (FAILED(hrTemp))
        hr = hrTemp;

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));

    return(hr);
}

// IPersistStream::Load method for Intshcut

STDMETHODIMP Intshcut::Load(IStream *pstm)
{
    // to implement this: 
    //      save stream to temp.ini
    //      IPersistFile::Load() from that
    //      delete temp file
    return E_NOTIMPL;
}

// IPersistStream::Save method for Intshcut
STDMETHODIMP Intshcut::Save(IStream *pstm, BOOL bClearDirty)
{
    HRESULT hr = InitProp();
    if (SUCCEEDED(hr))
    {
        TCHAR szURL[INTERNET_MAX_URL_LENGTH];
        hr = m_pprop->GetProp(PID_IS_URL, szURL, SIZECHARS(szURL));
        if (SUCCEEDED(hr))
        {
            LPSTR pszContents;
            hr = CreateURLFileContents(szURL, &pszContents);
            if (SUCCEEDED(hr)) {
                ASSERT(hr == lstrlenA(pszContents));
                hr = pstm->Write(pszContents, hr + 1, NULL);
                GlobalFree(pszContents);
            } else {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    
    return hr;
}

// IPersistStream::GetSizeMax method for Intshcut

STDMETHODIMP Intshcut::GetSizeMax(PULARGE_INTEGER puliSize)
{
    puliSize->LowPart = 0;
    puliSize->HighPart = 0;

    HRESULT hr = InitProp();
    if (SUCCEEDED(hr))
    {
        puliSize->LowPart = GetFileContentsAndSize(NULL);
        hr = S_OK;
    }
    return hr;
}

STDMETHODIMP Intshcut::_SetTempFileName(TCHAR *pszTempFileName)
{
    ASSERT(NULL == m_pszTempFileName);
    if (m_pszTempFileName)
        DeleteFile(m_pszTempFileName);
        
    Str_SetPtr(&m_pszTempFileName, pszTempFileName);
    return (m_pszTempFileName ? S_OK : E_OUTOFMEMORY);
}

STDMETHODIMP Intshcut::_CreateTemporaryBackingFile()
{
    HRESULT hres = E_FAIL;

    if (m_pszTempFileName)
        return S_OK;

    TCHAR szTempFileName[MAX_PATH];
    TCHAR szDirectory[MAX_PATH];
    
    DWORD dwRet = GetTempPath(ARRAYSIZE(szDirectory),  szDirectory);

    if ((FALSE == dwRet) || (FALSE == PathFileExists(szDirectory)))
    {
        szDirectory[0] = TEXT('\\');
        szDirectory[1] = TEXT('\0');
        dwRet = TRUE;
    }

    dwRet =  GetTempFileName(szDirectory, TEXT("www"), 0, szTempFileName);
    if (dwRet)
    {
        hres = _SetTempFileName(szTempFileName);
        // Now copy over the current file from which this was loaded and then save off
        // any changes
        if (S_OK == hres)
        {
            if (m_pszFile)
            {
                EVAL(CopyFile(m_pszFile, m_pszTempFileName, FALSE));
                SaveToFile(m_pszTempFileName, FALSE); // this flushes the file
            }
        }
    }

    return hres;
}

// Calculate the size of the contents to be transferred in a block.
STDMETHODIMP_(DWORD) Intshcut::GetFileContentsAndSize(LPSTR *ppszBuf)
{
    DWORD cbSize = 0;    // this is in bytes, not characters
    TCHAR szURL[INTERNET_MAX_URL_LENGTH];
    BOOL fSuccess = FALSE;
    HRESULT hres;
    
    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    ASSERT(m_pprop);
    
    // Create a temporary backing File here and save off everything that needs to be 
    // saved off there and use that to satisfy this request
    if (S_OK == _CreateTemporaryBackingFile())
    {
        ASSERT(m_pszTempFileName);
        
        WCHAR wszTemp[MAX_PATH];
        SHTCharToUnicode(m_pszTempFileName, wszTemp, ARRAYSIZE(wszTemp));
        
        hres = Save(wszTemp, FALSE); // So our temp file is now up to date
        
        // Just copy the file
        HANDLE hFile = CreateFile(m_pszTempFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            DWORD dwTemp = 0;
            cbSize = GetFileSize(hFile, &dwTemp);
            if (ppszBuf)
            {
                *ppszBuf = NULL;
                if (0xFFFFFFFF != cbSize)
                {
                    ASSERT(0 == dwTemp);
                    *ppszBuf = (LPSTR)LocalAlloc(LPTR, cbSize);
                    if (*ppszBuf)
                    {
                        dwTemp = 0;
                        if(ReadFile(hFile, *ppszBuf, cbSize, &dwTemp, NULL))
                        {
                            ASSERT(cbSize >= dwTemp);
                            fSuccess = TRUE;
                        }
                    }
                }
            }
            else
            {
                fSuccess = TRUE; // Just want the size - not contents
            }
            CloseHandle(hFile);
        }
        
        if (FALSE == fSuccess)
        {
            cbSize = 0;
            if(ppszBuf && (*ppszBuf))
            {
                LocalFree(*ppszBuf);
                *ppszBuf = NULL;
            }
        }
    }
    
    if (FALSE == fSuccess)
    {
        // if you couldn't read the file, then perhaps atleast this will work ?
        if (SUCCEEDED(m_pprop->GetProp(PID_IS_URL, szURL, SIZECHARS(szURL))))
        {
            HRESULT hr = CreateURLFileContents(szURL, ppszBuf);

            // IEUNIX-This function should return the strlen not including the
            // null characters as this causes the shortcut file  having a null
            // character causing a crash in the execution of the link.
            // Fortunately, that's what CreateURLFileContents returns
            
            cbSize = SUCCEEDED(hr) ? hr : 0;
        }
    }
    
    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    
    return cbSize;
}

// transfer the URL data in URL clipboard 
STDMETHODIMP Intshcut::TransferUniformResourceLocator(FORMATETC *pfmtetc, STGMEDIUM *pstgmed)
{
    HRESULT hr;

    ASSERT(pfmtetc->dwAspect == DVASPECT_CONTENT);
    ASSERT(pfmtetc->lindex == -1);

    if (pfmtetc->tymed & TYMED_HGLOBAL)
    {
        TCHAR szURL[INTERNET_MAX_URL_LENGTH];

        hr = m_pprop->GetProp(PID_IS_URL, szURL, SIZECHARS(szURL));
        if (SUCCEEDED(hr))
        {
            int cch = (lstrlen(szURL) + 1) * 2; // For multi byte chars
            LPSTR pszURL = (LPSTR)GlobalAlloc(GPTR, cch);
            if (pszURL)
            {
                SHUnicodeToAnsi(szURL, pszURL, cch);
                pstgmed->tymed = TYMED_HGLOBAL;
                pstgmed->hGlobal = pszURL;
            }
        }
    }
    else
        hr = DV_E_TYMED;

    return hr;
}

// transfer the URL data in text
STDMETHODIMP Intshcut::TransferText(FORMATETC *pfmtetc, STGMEDIUM *pstgmed)
{
    return TransferUniformResourceLocator(pfmtetc, pstgmed);
}

// assumes the current seek pos in the stream is at the start

BOOL GetStreamMimeAndExt(LPCWSTR pszURL, IStream *pstm, 
                         LPTSTR pszMime, UINT cchMime, LPTSTR pszExt, UINT cchExt)
{
    BYTE buf[256];
    ULONG cbRead;
    pstm->Read(buf, SIZEOF(buf), &cbRead);

    WCHAR *pwszMimeOut;
    if (SUCCEEDED(FindMimeFromData(NULL, pszURL, buf, cbRead, L"text/html", 0, &pwszMimeOut, 0)))
    {
        TCHAR szMimeTemp[MAX_PATH];

        if (pszMime == NULL)
        {
            pszMime = szMimeTemp;
            cchMime = ARRAYSIZE(szMimeTemp);
        }

        SHUnicodeToTChar(pwszMimeOut, pszMime, cchMime);
        CoTaskMemFree(pwszMimeOut);

        if (pszExt)
            MIME_GetExtension(pszMime, pszExt, cchExt);
    }

    // const LARGE_INTEGER c_li0 = {0, 0};
    pstm->Seek(c_li0, STREAM_SEEK_SET, NULL);

    return TRUE;
}

// pszName is assumed to be MAX_PATH

STDMETHODIMP Intshcut::GetDocumentName(LPTSTR pszName)
{
    GetDescription(pszName, MAX_PATH);
                
    WCHAR *pszURL;
    if (S_OK == GetURLW(&pszURL))
    {
        IStream *pstm;
        if (SUCCEEDED(URLOpenBlockingStreamW(NULL, pszURL, &pstm, 0, NULL)))
        {
            TCHAR szExt[MAX_PATH];
            GetStreamMimeAndExt(pszURL, pstm, NULL, 0, szExt, ARRAYSIZE(szExt));

            PathRenameExtension(pszName, szExt);
            
            pstm->Release();
        }
        SHFree(pszURL);
    }
    return S_OK;
}

// transfer URL data in file-group-descriptor clipboard format.
STDMETHODIMP Intshcut::TransferFileGroupDescriptorA(FORMATETC *pfmtetc, STGMEDIUM *pstgmed)
{
    HRESULT hr;

    if (pfmtetc->dwAspect != DVASPECT_COPY  &&
        pfmtetc->dwAspect != DVASPECT_LINK  &&
        pfmtetc->dwAspect != DVASPECT_CONTENT)
    {
        hr = DV_E_DVASPECT;
    }
    else if (pfmtetc->tymed & TYMED_HGLOBAL)
    {
        FILEGROUPDESCRIPTORA * pfgd = (FILEGROUPDESCRIPTORA *)GlobalAlloc(GPTR, SIZEOF(FILEGROUPDESCRIPTORA));
        if (pfgd)
        {
            FILEDESCRIPTORA * pfd = &(pfgd->fgd[0]);
            TCHAR szTemp[MAX_PATH]; 

            if (pfmtetc->dwAspect == DVASPECT_COPY)
            {
                pfd->dwFlags = FD_FILESIZE;
                GetDocumentName(szTemp);
            }
            else
            {
                pfd->dwFlags = FD_FILESIZE | FD_LINKUI;
                GetDescription(szTemp, ARRAYSIZE(szTemp));
            }
            SHTCharToAnsi(PathFindFileName(szTemp), pfd->cFileName, SIZECHARS(pfd->cFileName));

            pfd->nFileSizeHigh = 0;
            pfd->nFileSizeLow = GetFileContentsAndSize(NULL);

            pfgd->cItems = 1;

            pstgmed->tymed = TYMED_HGLOBAL;
            pstgmed->hGlobal = pfgd;

            hr = S_OK;
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else
        hr = DV_E_TYMED;

    return hr;
}

// transfer URL data in file-group-descriptor clipboard format.
STDMETHODIMP Intshcut::TransferFileGroupDescriptorW(FORMATETC *pfmtetc, STGMEDIUM *pstgmed)
{
    HRESULT hr;

    if (pfmtetc->dwAspect != DVASPECT_COPY  &&
        pfmtetc->dwAspect != DVASPECT_LINK  &&
        pfmtetc->dwAspect != DVASPECT_CONTENT)
    {
        hr = DV_E_DVASPECT;
    }
    else if (pfmtetc->tymed & TYMED_HGLOBAL)
    {
        FILEGROUPDESCRIPTORW * pfgd = (FILEGROUPDESCRIPTORW *)GlobalAlloc(GPTR, SIZEOF(FILEGROUPDESCRIPTORW));
        if (pfgd)
        {
            FILEDESCRIPTORW * pfd = &(pfgd->fgd[0]);
            TCHAR szTemp[MAX_PATH];
            
            if (pfmtetc->dwAspect == DVASPECT_COPY)
            {
                pfd->dwFlags = FD_FILESIZE;
                GetDocumentName(szTemp);
            }
            else
            {
                pfd->dwFlags = FD_FILESIZE | FD_LINKUI;
                GetDescription(szTemp, ARRAYSIZE(szTemp));
            }

            SHTCharToUnicode(PathFindFileName(szTemp), pfd->cFileName, SIZECHARS(pfd->cFileName));

            pfd->nFileSizeHigh = 0;
            pfd->nFileSizeLow = GetFileContentsAndSize(NULL);

            pfgd->cItems = 1;

            pstgmed->tymed = TYMED_HGLOBAL;
            pstgmed->hGlobal = pfgd;

            hr = S_OK;
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else
        hr = DV_E_TYMED;

    return hr;
}

#if defined(BIG_ENDIAN) && defined(BYTE_ORDER)
#if BYTE_ORDER != BIG_ENDIAN
#undef BIG_ENDIAN
#endif
#endif

#ifdef BIG_ENDIAN
#define BOM 0xfffe
#else
#define BOM 0xfeff
#endif

STDMETHODIMP Intshcut::GetDocumentStream(IStream **ppstm)
{
    *ppstm = NULL;

    WCHAR *pszURL;
    HRESULT hres = GetURLW(&pszURL);
    if (S_OK == hres)
    {
        IStream *pstm;
        hres = URLOpenBlockingStreamW(NULL, pszURL, &pstm, 0, NULL);
        if (SUCCEEDED(hres))
        {
            TCHAR szMime[80];

            if (GetStreamMimeAndExt(pszURL, pstm, szMime, ARRAYSIZE(szMime), NULL, 0) &&
                StrCmpI(szMime, TEXT("text/html")) == 0)
            {
                IStream *aStreams[2];

                if(m_uiCodePage == 1200)    // Unicode
                {
                    WCHAR wzBaseTag[INTERNET_MAX_URL_LENGTH + 20];

                    wnsprintfW(wzBaseTag, ARRAYSIZE(wzBaseTag), TEXT("%wc<BASE HREF=\"%ws\">\n"), (WCHAR)BOM, pszURL);
                    aStreams[0] = SHCreateMemStream((BYTE *)wzBaseTag, lstrlenW(wzBaseTag) * SIZEOF(wzBaseTag[0]));
                }
                else
                {
                    CHAR szURL[INTERNET_MAX_URL_LENGTH], szBaseTag[INTERNET_MAX_URL_LENGTH + 20];

                    SHUnicodeToAnsi(pszURL, szURL, ARRAYSIZE(szURL));
                    wnsprintfA(szBaseTag, ARRAYSIZE(szBaseTag), "<BASE HREF=\"%s\">\n", szURL);

                    // NOTE: this is an ANSI stream

                    aStreams[0] = SHCreateMemStream((BYTE *)szBaseTag, lstrlenA(szBaseTag) * SIZEOF(szBaseTag[0]));
                }
                if (aStreams[0])
                {
                    aStreams[1] = pstm;
                    hres = SHCreateStreamWrapperCP(aStreams, ARRAYSIZE(aStreams), STGM_READ, m_uiCodePage, ppstm);
                    aStreams[0]->Release();
                }
                else
                    hres = E_OUTOFMEMORY;
                pstm->Release();
            }
            else
                *ppstm = pstm;
        }
        SHFree(pszURL);
    }
    else
        hres = E_FAIL;
    return hres;
}

// transfer URL data in file-contents clipboard format.
STDMETHODIMP Intshcut::TransferFileContents(FORMATETC *pfmtetc, STGMEDIUM *pstgmed)
{
    HRESULT hr;

    if (pfmtetc->lindex != 0)
        return DV_E_LINDEX;

    if ((pfmtetc->dwAspect == DVASPECT_CONTENT ||
         pfmtetc->dwAspect == DVASPECT_LINK) && 
         (pfmtetc->tymed & TYMED_HGLOBAL))
    {
        LPSTR pszFileContents;
        DWORD cbSize = GetFileContentsAndSize(&pszFileContents);
        if (pszFileContents)
        {
            pstgmed->tymed = TYMED_HGLOBAL;
            pstgmed->hGlobal = pszFileContents;
            hr = S_OK;
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else if ((pfmtetc->dwAspect == DVASPECT_COPY) && (pfmtetc->tymed & TYMED_ISTREAM))
    {
        hr = GetDocumentStream(&pstgmed->pstm);
        if (SUCCEEDED(hr))
        {
            pstgmed->tymed = TYMED_ISTREAM;
            hr = S_OK;
        }
    }
    else
        hr = DV_E_TYMED;

    return hr;
}



#ifdef DEBUG

STDMETHODIMP_(void) Intshcut::Dump(void)
{
    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));

#define INDENT_STRING   "    "

    if (IsFlagSet(g_dwDumpFlags, DF_INTSHCUT))
    {
        TraceMsg(TF_ALWAYS, "%sm_dwFlags = %#08lx",
                   INDENT_STRING,
                   m_dwFlags);
        TraceMsg(TF_ALWAYS, "%sm_pszFile = \"%s\"",
                   INDENT_STRING,
                   Dbg_SafeStr(m_pszFile));

        if (m_pprop)
            m_pprop->Dump();
    }
}

#endif  // DEBUG
