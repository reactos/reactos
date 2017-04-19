
/*
 * file system folder drop target
 *
 * Copyright 1997             Marcus Meissner
 * Copyright 1998, 1999, 2002 Juergen Schmied
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

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL (shell);

/****************************************************************************
 * BuildPathsList
 *
 * Builds a list of paths like the one used in SHFileOperation from a table of
 * PIDLs relative to the given base folder
 */
WCHAR *
BuildPathsList(LPCWSTR wszBasePath, int cidl, LPCITEMIDLIST *pidls, BOOL bRelative)
{
    WCHAR *pwszPathsList;
    WCHAR *pwszListPos;
    int iPathLen, i;

    iPathLen = wcslen(wszBasePath);
    pwszPathsList = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR) * cidl + 1);
    pwszListPos = pwszPathsList;

    for (i = 0; i < cidl; i++)
    {
        if (!_ILIsFolder(pidls[i]) && !_ILIsValue(pidls[i]))
            continue;

        wcscpy(pwszListPos, wszBasePath);
        pwszListPos += iPathLen;

        if (_ILIsFolder(pidls[i]) && bRelative)
            continue;

        /* FIXME: abort if path too long */
        _ILSimpleGetTextW(pidls[i], pwszListPos, MAX_PATH - iPathLen);
        pwszListPos += wcslen(pwszListPos) + 1;
    }
    *pwszListPos = 0;
    return pwszPathsList;
}

/****************************************************************************
 * CFSDropTarget::CopyItems
 *
 * copies items to this folder
 */
HRESULT WINAPI CFSDropTarget::CopyItems(IShellFolder * pSFFrom, UINT cidl,
                                    LPCITEMIDLIST * apidl, BOOL bCopy)
{
    CComPtr<IPersistFolder2> ppf2 = NULL;
    WCHAR szSrcPath[MAX_PATH];
    WCHAR szTargetPath[MAX_PATH];
    SHFILEOPSTRUCTW op;
    LPITEMIDLIST pidl;
    LPWSTR pszSrc, pszTarget, pszSrcList, pszTargetList, pszFileName;
    int res, length;
    HRESULT hr;

    TRACE ("(%p)->(%p,%u,%p)\n", this, pSFFrom, cidl, apidl);

    hr = pSFFrom->QueryInterface (IID_PPV_ARG(IPersistFolder2, &ppf2));
    if (SUCCEEDED(hr))
    {
        hr = ppf2->GetCurFolder(&pidl);
        if (FAILED(hr))
        {
            return hr;
        }

        hr = SHGetPathFromIDListW(pidl, szSrcPath);
        SHFree(pidl);

        if (FAILED(hr))
            return hr;

        pszSrc = PathAddBackslashW(szSrcPath);

        wcscpy(szTargetPath, sPathTarget);
        pszTarget = PathAddBackslashW(szTargetPath);

        pszSrcList = BuildPathsList(szSrcPath, cidl, apidl, FALSE);
        pszTargetList = BuildPathsList(szTargetPath, cidl, apidl, TRUE);

        if (!pszSrcList || !pszTargetList)
        {
            if (pszSrcList)
                HeapFree(GetProcessHeap(), 0, pszSrcList);

            if (pszTargetList)
                HeapFree(GetProcessHeap(), 0, pszTargetList);

            SHFree(pidl);
            return E_OUTOFMEMORY;
        }

        ZeroMemory(&op, sizeof(op));
        if (!pszSrcList[0])
        {
            /* remove trailing backslash */
            pszSrc--;
            pszSrc[0] = L'\0';
            op.pFrom = szSrcPath;
        }
        else
        {
            op.pFrom = pszSrcList;
        }

        if (!pszTargetList[0])
        {
            /* remove trailing backslash */
            if (pszTarget - szTargetPath > 3)
            {
                pszTarget--;
                pszTarget[0] = L'\0';
            }
            else
            {
                pszTarget[1] = L'\0';
            }

            op.pTo = szTargetPath;
            op.fFlags = 0;
        }
        else
        {
            op.pTo = pszTargetList;
            op.fFlags = FOF_MULTIDESTFILES;
        }
        op.hwnd = GetActiveWindow();
        op.wFunc = bCopy ? FO_COPY : FO_MOVE;
        op.fFlags |= FOF_ALLOWUNDO | FOF_NOCONFIRMMKDIR;

        res = SHFileOperationW(&op);

        if (res == DE_SAMEFILE)
        {
            length = wcslen(szTargetPath);

            pszFileName = wcsrchr(pszSrcList, '\\');
            pszFileName++;

            if (LoadStringW(shell32_hInstance, IDS_COPY_OF, pszTarget, MAX_PATH - length))
            {
                wcscat(szTargetPath, L" ");
            }

            wcscat(szTargetPath, pszFileName);
            op.pTo = szTargetPath;

            res = SHFileOperationW(&op);
        }

        HeapFree(GetProcessHeap(), 0, pszSrcList);
        HeapFree(GetProcessHeap(), 0, pszTargetList);

        if (res)
            return E_FAIL;
        else
            return S_OK;
    }
    return E_FAIL;
}

CFSDropTarget::CFSDropTarget():
    cfShellIDList(0),
    fAcceptFmt(FALSE),
    sPathTarget(NULL)
{
}

HRESULT WINAPI CFSDropTarget::Initialize(LPWSTR PathTarget)
{
    cfShellIDList = RegisterClipboardFormatW(CFSTR_SHELLIDLIST);
    if (!cfShellIDList)
        return E_FAIL;

    sPathTarget = (WCHAR *)SHAlloc((wcslen(PathTarget) + 1) * sizeof(WCHAR));
    if (!sPathTarget)
        return E_OUTOFMEMORY;
    wcscpy(sPathTarget, PathTarget);

    return S_OK;
}

CFSDropTarget::~CFSDropTarget()
{
    SHFree(sPathTarget);
}

BOOL
CFSDropTarget::GetUniqueFileName(LPWSTR pwszBasePath, LPCWSTR pwszExt, LPWSTR pwszTarget, BOOL bShortcut)
{
    WCHAR wszLink[40];

    if (!bShortcut)
    {
        if (!LoadStringW(shell32_hInstance, IDS_LNK_FILE, wszLink, _countof(wszLink)))
            wszLink[0] = L'\0';
    }

    if (!bShortcut)
        swprintf(pwszTarget, L"%s%s%s", wszLink, pwszBasePath, pwszExt);
    else
        swprintf(pwszTarget, L"%s%s", pwszBasePath, pwszExt);

    for (UINT i = 2; PathFileExistsW(pwszTarget); ++i)
    {
        if (!bShortcut)
            swprintf(pwszTarget, L"%s%s (%u)%s", wszLink, pwszBasePath, i, pwszExt);
        else
            swprintf(pwszTarget, L"%s (%u)%s", pwszBasePath, i, pwszExt);
    }

    return TRUE;
}

/****************************************************************************
 * IDropTarget implementation
 */
BOOL CFSDropTarget::QueryDrop(DWORD dwKeyState, LPDWORD pdwEffect)
{
    /* TODO Windows does different drop effects if dragging across drives. 
    i.e., it will copy instead of move if the directories are on different disks. */

    DWORD dwEffect = DROPEFFECT_MOVE;

    *pdwEffect = DROPEFFECT_NONE;

    if (fAcceptFmt) { /* Does our interpretation of the keystate ... */
        *pdwEffect = KeyStateToDropEffect (dwKeyState);
        
        if (*pdwEffect == DROPEFFECT_NONE)
            *pdwEffect = dwEffect;

        /* ... matches the desired effect ? */
        if (dwEffect & *pdwEffect) {
            return TRUE;
        }
    }
    return FALSE;
}

HRESULT WINAPI CFSDropTarget::DragEnter(IDataObject *pDataObject,
                                        DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
    TRACE("(%p)->(DataObject=%p)\n", this, pDataObject);
    FORMATETC fmt;
    FORMATETC fmt2;
    fAcceptFmt = FALSE;

    InitFormatEtc (fmt, cfShellIDList, TYMED_HGLOBAL);
    InitFormatEtc (fmt2, CF_HDROP, TYMED_HGLOBAL);

    if (SUCCEEDED(pDataObject->QueryGetData(&fmt)))
        fAcceptFmt = TRUE;
    else if (SUCCEEDED(pDataObject->QueryGetData(&fmt2)))
        fAcceptFmt = TRUE;

    QueryDrop(dwKeyState, pdwEffect);
    return S_OK;
}

HRESULT WINAPI CFSDropTarget::DragOver(DWORD dwKeyState, POINTL pt,
                                       DWORD *pdwEffect)
{
    TRACE("(%p)\n", this);

    if (!pdwEffect)
        return E_INVALIDARG;

    QueryDrop(dwKeyState, pdwEffect);

    return S_OK;
}

HRESULT WINAPI CFSDropTarget::DragLeave()
{
    TRACE("(%p)\n", this);

    fAcceptFmt = FALSE;

    return S_OK;
}

HRESULT WINAPI CFSDropTarget::Drop(IDataObject *pDataObject,
                                   DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
    TRACE("(%p) object dropped, effect %u\n", this, *pdwEffect);
    
    if (!pdwEffect)
        return E_INVALIDARG;

    QueryDrop(dwKeyState, pdwEffect);

    BOOL fIsOpAsync = FALSE;
    CComPtr<IAsyncOperation> pAsyncOperation;

    if (SUCCEEDED(pDataObject->QueryInterface(IID_PPV_ARG(IAsyncOperation, &pAsyncOperation))))
    {
        if (SUCCEEDED(pAsyncOperation->GetAsyncMode(&fIsOpAsync)) && fIsOpAsync)
        {
            _DoDropData *data = static_cast<_DoDropData*>(HeapAlloc(GetProcessHeap(), 0, sizeof(_DoDropData)));
            data->This = this;
            // Need to maintain this class in case the window is closed or the class exists temporarily (when dropping onto a folder).
            pDataObject->AddRef();
            pAsyncOperation->StartOperation(NULL);
            CoMarshalInterThreadInterfaceInStream(IID_IDataObject, pDataObject, &data->pStream);
            this->AddRef();
            data->dwKeyState = dwKeyState;
            data->pt = pt;
            // Need to dereference as pdweffect gets freed.
            data->pdwEffect = *pdwEffect;
            SHCreateThread(CFSDropTarget::_DoDropThreadProc, data, NULL, NULL);
            return S_OK;
        }
    }
    return this->_DoDrop(pDataObject, dwKeyState, pt, pdwEffect);
}

HRESULT WINAPI CFSDropTarget::_DoDrop(IDataObject *pDataObject,
                                      DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
    TRACE("(%p) performing drop, effect %u\n", this, *pdwEffect);
    FORMATETC fmt;
    FORMATETC fmt2;
    STGMEDIUM medium;

    InitFormatEtc (fmt, cfShellIDList, TYMED_HGLOBAL);
    InitFormatEtc (fmt2, CF_HDROP, TYMED_HGLOBAL);

    HRESULT hr;
    bool bCopy = TRUE;
    bool bLinking = FALSE;

    /* Figure out what drop operation we're doing */
    if (pdwEffect)
    {
        TRACE("Current drop effect flag %i\n", *pdwEffect);
        if ((*pdwEffect & DROPEFFECT_MOVE) == DROPEFFECT_MOVE)
            bCopy = FALSE;
        if ((*pdwEffect & DROPEFFECT_LINK) == DROPEFFECT_LINK)
            bLinking = TRUE;
    }

    if (SUCCEEDED(pDataObject->QueryGetData(&fmt)))
    {
        hr = pDataObject->GetData(&fmt, &medium);
        TRACE("CFSTR_SHELLIDLIST.\n");

        /* lock the handle */
        LPIDA lpcida = (LPIDA)GlobalLock(medium.hGlobal);
        if (!lpcida)
        {
            ReleaseStgMedium(&medium);
            return E_FAIL;
        }

        /* convert the data into pidl */
        LPITEMIDLIST pidl;
        LPITEMIDLIST *apidl = _ILCopyCidaToaPidl(&pidl, lpcida);
        if (!apidl)
        {
            ReleaseStgMedium(&medium);
            return E_FAIL;
        }

        CComPtr<IShellFolder> psfDesktop;
        CComPtr<IShellFolder> psfFrom = NULL;

        /* Grab the desktop shell folder */
        hr = SHGetDesktopFolder(&psfDesktop);
        if (FAILED(hr))
        {
            ERR("SHGetDesktopFolder failed\n");
            SHFree(pidl);
            _ILFreeaPidl(apidl, lpcida->cidl);
            ReleaseStgMedium(&medium);
            return E_FAIL;
        }

        /* Find source folder, this is where the clipboard data was copied from */
        if (_ILIsDesktop(pidl))
        {
            /* use desktop shell folder */
            psfFrom = psfDesktop;
        }
        else 
        {
            hr = psfDesktop->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder, &psfFrom));
            if (FAILED(hr))
            {
                ERR("no IShellFolder\n");
                SHFree(pidl);
                _ILFreeaPidl(apidl, lpcida->cidl);
                ReleaseStgMedium(&medium);
                return E_FAIL;
            }
        }

        if (bLinking)
        {
            WCHAR wszTargetPath[MAX_PATH];
            WCHAR wszPath[MAX_PATH];
            WCHAR wszTarget[MAX_PATH];

            wcscpy(wszTargetPath, sPathTarget);

            TRACE("target path = %s", debugstr_w(wszTargetPath));

            /* We need to create a link for each pidl in the copied items, so step through the pidls from the clipboard */
            for (UINT i = 0; i < lpcida->cidl; i++)
            {
                //Find out which file we're copying
                STRRET strFile;
                hr = psfFrom->GetDisplayNameOf(apidl[i], SHGDN_FORPARSING, &strFile);
                if (FAILED(hr)) 
                {
                    ERR("Error source obtaining path");
                    break;
                }

                hr = StrRetToBufW(&strFile, apidl[i], wszPath, _countof(wszPath));
                if (FAILED(hr)) 
                {
                    ERR("Error putting source path into buffer");
                    break;
                }
                TRACE("source path = %s", debugstr_w(wszPath));

                // Creating a buffer to hold the combined path
                WCHAR buffer_1[MAX_PATH] = L"";
                WCHAR *lpStr1;
                lpStr1 = buffer_1;

                LPWSTR pwszFileName = PathFindFileNameW(wszPath);
                LPWSTR pwszExt = PathFindExtensionW(wszPath);
                LPWSTR placementPath = PathCombineW(lpStr1, sPathTarget, pwszFileName);
                CComPtr<IPersistFile> ppf;

                //Check to see if it's already a link. 
                if (!wcsicmp(pwszExt, L".lnk"))
                {
                    //It's a link so, we create a new one which copies the old.
                    if(!GetUniqueFileName(placementPath, pwszExt, wszTarget, TRUE)) 
                    {
                        ERR("Error getting unique file name");
                        hr = E_FAIL;
                        break;
                    }
                    hr = IShellLink_ConstructFromPath(wszPath, IID_PPV_ARG(IPersistFile, &ppf));
                    if (FAILED(hr)) {
                        ERR("Error constructing link from file");
                        break;
                    }

                    hr = ppf->Save(wszTarget, FALSE);
					if (FAILED(hr))
						break;
					SHChangeNotify(SHCNE_CREATE, SHCNF_PATHW, wszTarget, NULL);
                }
                else
                {
                    //It's not a link, so build a new link using the creator class and fill it in.
                    //Create a file name for the link
                    if (!GetUniqueFileName(placementPath, L".lnk", wszTarget, TRUE))
                    {
                        ERR("Error creating unique file name");
                        hr = E_FAIL;
                        break;
                    }

                    CComPtr<IShellLinkW> pLink;
                    hr = CShellLink::_CreatorClass::CreateInstance(NULL, IID_PPV_ARG(IShellLinkW, &pLink));
                    if (FAILED(hr)) {
                        ERR("Error instantiating IShellLinkW");
                        break;
                    }

                    WCHAR szDirPath[MAX_PATH], *pwszFile;
                    GetFullPathName(wszPath, MAX_PATH, szDirPath, &pwszFile);
                    if (pwszFile) pwszFile[0] = 0;

                    hr = pLink->SetPath(wszPath);
                    if(FAILED(hr))
                        break;

                    hr = pLink->SetWorkingDirectory(szDirPath);
                    if(FAILED(hr))
                        break;

                    hr = pLink->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf));
                    if(FAILED(hr))
                        break;

                    hr = ppf->Save(wszTarget, TRUE);
					if (FAILED(hr))
						break;
					SHChangeNotify(SHCNE_CREATE, SHCNF_PATHW, wszTarget, NULL);
                }
            }
        }
        else 
        {
            hr = this->CopyItems(psfFrom, lpcida->cidl, (LPCITEMIDLIST*)apidl, bCopy);
        }

        SHFree(pidl);
        _ILFreeaPidl(apidl, lpcida->cidl);
        ReleaseStgMedium(&medium);
    }
    else if (SUCCEEDED(pDataObject->QueryGetData(&fmt2)))
    {
        FORMATETC fmt2;
        InitFormatEtc (fmt2, CF_HDROP, TYMED_HGLOBAL);
        if (SUCCEEDED(pDataObject->GetData(&fmt2, &medium)) /* && SUCCEEDED(pDataObject->GetData(&fmt2, &medium))*/)
        {
            WCHAR wszTargetPath[MAX_PATH + 1];
            LPWSTR pszSrcList;

            wcscpy(wszTargetPath, sPathTarget);
            //Double NULL terminate.
            wszTargetPath[wcslen(wszTargetPath) + 1] = '\0';
            
            LPDROPFILES lpdf = (LPDROPFILES) GlobalLock(medium.hGlobal);
            if (!lpdf)
            {
                ERR("Error locking global\n");
                return E_FAIL;
            }
            pszSrcList = (LPWSTR) (((byte*) lpdf) + lpdf->pFiles);
            TRACE("Source file (just the first) = %s\n", debugstr_w(pszSrcList));
            TRACE("Target path = %s\n", debugstr_w(wszTargetPath));

            SHFILEOPSTRUCTW op;
            ZeroMemory(&op, sizeof(op));
            op.pFrom = pszSrcList;
            op.pTo = wszTargetPath;
            op.hwnd = GetActiveWindow();
            op.wFunc = bCopy ? FO_COPY : FO_MOVE;
            op.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMMKDIR;
            hr = SHFileOperationW(&op);
            return hr;
        }
        ERR("Error calling GetData\n");
        hr = E_FAIL;
    }
    else 
    {
        ERR("No viable drop format.\n");
        hr = E_FAIL;
    }    
    return hr;
}

DWORD WINAPI CFSDropTarget::_DoDropThreadProc(LPVOID lpParameter)
{
    CoInitialize(NULL);
    _DoDropData *data = static_cast<_DoDropData*>(lpParameter);
    CComPtr<IDataObject> pDataObject;
    HRESULT hr = CoGetInterfaceAndReleaseStream (data->pStream, IID_PPV_ARG(IDataObject, &pDataObject));

    if (SUCCEEDED(hr))
    {
        CComPtr<IAsyncOperation> pAsyncOperation;
        hr = data->This->_DoDrop(pDataObject, data->dwKeyState, data->pt, &data->pdwEffect);
        if (SUCCEEDED(pDataObject->QueryInterface(IID_PPV_ARG(IAsyncOperation, &pAsyncOperation))))
        {
            pAsyncOperation->EndOperation(hr, NULL, data->pdwEffect);
        }
    }
    //Release the CFSFolder and data object holds in the copying thread.
    data->This->Release();
    //Release the parameter from the heap.
    HeapFree(GetProcessHeap(), 0, data);
    CoUninitialize();
    return 0;
}

HRESULT CFSDropTarget_CreateInstance(LPWSTR sPathTarget, REFIID riid, LPVOID * ppvOut)
{
    return ShellObjectCreatorInit<CFSDropTarget>(sPathTarget, riid, ppvOut);
}