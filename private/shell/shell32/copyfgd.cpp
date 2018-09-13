/*****************************************************************************\
    FILE: copyfgd.cpp

    DESCRIPTION:
        Copy a FileGroupDescriptor.
\*****************************************************************************/

#include "shellprv.h"

#include <ynlist.h>
#include "ids.h"
#include "pidl.h"
#include "fstreex.h"
#include "copy.h"
#include <shldisp.h>
#include <shlwapi.h>
#include <wininet.h>    // InternetGetLastResponseInfo

#include "datautil.h"

class CCopyThread;
HRESULT CreateInstance_CopyThread(HWND hwnd, LPCTSTR pszPath, IDataObject *pdtobj, DWORD *pdwEffect, BOOL fIsBkDropTarget, CCopyThread ** ppct);

extern "C" {
    #include "undo.h"
    #include "defview.h"
};

BOOL GetWininetError(DWORD dwError, BOOL fCopy, LPTSTR pszErrorMsg, DWORD cchSize)
{
    TCHAR szErrorMsg[MAX_PATH];
    BOOL fIsWininetError = ((dwError >= INTERNET_ERROR_BASE) && (dwError <= INTERNET_ERROR_LAST));

    // Default message if FormatMessage doesn't recognize hres
    szErrorMsg[0] = 0;
    LoadString(HINST_THISDLL, (fCopy ? IDS_COPYERROR : IDS_MOVEERROR), szErrorMsg, ARRAYSIZE(szErrorMsg));

    if (fIsWininetError)
    {
        static HINSTANCE s_hinst = NULL;

        if (!s_hinst)
            s_hinst = GetModuleHandle(TEXT("WININET")); // It's okay if we leak it.


        // Can wininet give us extended error messages?
        // We ignore them because it's too late to call InternetGetLastResponseInfo.
        if (ERROR_INTERNET_EXTENDED_ERROR != dwError)
        {
            TCHAR szDetails[MAX_PATH*2];

            FormatMessage(FORMAT_MESSAGE_FROM_HMODULE, (LPCVOID)s_hinst, dwError, 0, szDetails, ARRAYSIZE(szDetails), NULL);
            StrCatBuff(szErrorMsg, TEXT("%s"), ARRAYSIZE(szErrorMsg));
            wnsprintf(pszErrorMsg, cchSize, szErrorMsg, szDetails);
        }
        else
            StrCpyN(pszErrorMsg, szErrorMsg, cchSize);
    }
    else
    {
        StrCpyN(pszErrorMsg, szErrorMsg, cchSize);
    }

    return TRUE;
}


// thunk A/W funciton to access A/W FILEGROUPDESCRIPTOR
// this relies on the fact that the first part of the A/W structures are
// identical. only the string buffer part is different. so all accesses to the
// cFileName field need to go through this function.
//

FILEDESCRIPTOR *GetFileDescriptor(FILEGROUPDESCRIPTOR *pfgd, BOOL fUnicode, int nIndex, LPTSTR pszName)
{
    if (fUnicode)
    {
        // Yes, so grab the data because it matches.
        FILEGROUPDESCRIPTORW * pfgdW = (FILEGROUPDESCRIPTORW *)pfgd;    // cast to what this really is

        // If the filename starts with a leading / we're going to be in trouble, since the rest
        // of the code assumes its going to be a \.  Web folders does the leading /, so just lop it
        // off right here.
        WCHAR *pwz;
        pwz = pfgdW->fgd[nIndex].cFileName;
        if (pfgdW->fgd[nIndex].cFileName[0] == '/')
        {
            memmove (pwz, pwz+1, sizeof(pfgdW->fgd[nIndex].cFileName)-sizeof(WCHAR));
        }

        // Now flip all the /'s to \'s.  No dbcs issues, we're unicode!
        for (; *pwz; ++pwz)
        {
            if (*pwz == '/')
            {
                *pwz = '\\';
            }
        }

        if (pszName)
            SHUnicodeToTChar(pfgdW->fgd[nIndex].cFileName, pszName, MAX_PATH);

        return (FILEDESCRIPTOR *)&pfgdW->fgd[nIndex];   // cast assume the non string parts are the same!
    }
    else
    {
        FILEGROUPDESCRIPTORA *pfgdA = (FILEGROUPDESCRIPTORA *)pfgd;     // cast to what this really is
        if (pfgdA->fgd[nIndex].cFileName[0] == '/' &&
            CharNextA(pfgdA->fgd[nIndex].cFileName) == pfgdA->fgd[nIndex].cFileName+1)
        {
            memmove (pfgdA->fgd[nIndex].cFileName, pfgdA->fgd[nIndex].cFileName+1, sizeof(pfgdA->fgd[nIndex].cFileName)-sizeof(char));
        }

        if (pszName)
            SHAnsiToTChar(pfgdA->fgd[nIndex].cFileName, pszName, MAX_PATH);

        return (FILEDESCRIPTOR *)&pfgdA->fgd[nIndex];   // cast assume the non string parts are the same!
    }
}


void CreateProgressStatusStr(LPCTSTR pszDirTo, LPWSTR pwzProgressStr, DWORD cchSize)
{
    // IDS_COPYTO also works in move operations. (It doesn't use the work "Copy")
    LPTSTR pszMsg = ShellConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(IDS_COPYTO), pszDirTo);

    if (pszMsg)
    {
        SHTCharToUnicode(pszMsg, pwzProgressStr, cchSize);
        LocalFree(pszMsg);
    }
    else
        pwzProgressStr[0] = 0;
}


void CalcBytesInFileGrpDescriptor(FILEGROUPDESCRIPTOR *pfgd, BOOL fUnicode, ULARGE_INTEGER * puliTotal)
{
    puliTotal->QuadPart = 0; // Init.
    for (UINT i = 0; i < pfgd->cItems; i++)
    {
        // WARNING: This may point to a FILEDESCRIPTOR *A or W, but that's ok as long as we ignore the filename.
        ULARGE_INTEGER uliFileSize;
        FILEDESCRIPTOR *pfd = GetFileDescriptor(pfgd, fUnicode, i, NULL);

        uliFileSize.HighPart = pfd->nFileSizeHigh;
        uliFileSize.LowPart = pfd->nFileSizeLow;
        puliTotal->QuadPart += uliFileSize.QuadPart;
    }
}

BOOL IsNameInDescriptor(FILEGROUPDESCRIPTOR *pfgd, BOOL fUnicode, LPCTSTR pszName, UINT iMax)
{
    for (UINT i = 0; i < iMax; i++)
    {
        TCHAR szName[MAX_PATH];
        // WARNING: This may point to a FILEDESCRIPTOR *A or W, but that's ok as long as we ignore the filename.
        FILEDESCRIPTOR *pfd = GetFileDescriptor(pfgd, fUnicode, i, szName);
        if (lstrcmpi(szName, pszName) == 0)
            return TRUE;
    }
    return FALSE;
}


BOOL ShowProgressUI(FILEGROUPDESCRIPTOR *pfgd)
{
    return (0 < pfgd->cItems) && (FD_PROGRESSUI & pfgd->fgd->dwFlags);
}


class CCopyThread
                : public IUnknown
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    HRESULT DoCopy(void) {return _DoCopy(_pdtobj);};
    HRESULT DoAsynchCopy(void);
    HRESULT GetEffect(DWORD *pdwEffect) {*pdwEffect = _dwEffect; return S_OK;}; // For synchronous case only.

    friend HRESULT CreateInstance_CopyThread(HWND hwnd, LPCTSTR pszPath, IDataObject *pdtobj, DWORD *pdwEffect, BOOL fIsBkDropTarget, CCopyThread ** ppct);

protected:
    CCopyThread(HWND hwnd, LPCTSTR pszPath, IDataObject *pdtobj, DWORD *pdwEffect, BOOL fIsBkDropTarget);
    ~CCopyThread();

private:
    HRESULT _CopyThreadProc(void);
    static DWORD CALLBACK CopyThreadProc(LPVOID pvThis) { return ((CCopyThread *) pvThis)->_CopyThreadProc(); };
    HRESULT _DoCopy(IDataObject * pdo);

    LONG                _cRef;

    HWND                _hwnd;
    LPCTSTR             _pszPath;
    IDataObject *       _pdtobj;        // Unmarshalled
    IStream *           _pstmDataObjMarshal;  // Carrying the IDataObject across threads
    DWORD               _dwEffect;
    BOOL                _fWindowIsTarget;
};


CCopyThread::CCopyThread(HWND hwnd, LPCTSTR pszPath, IDataObject *pdtobj, DWORD *pdwEffect, BOOL fIsBkDropTarget) : _cRef(1)
{
    DllAddRef();

    // Assert this class was zero inited.
    ASSERT(!_pdtobj);
    ASSERT(!_pstmDataObjMarshal);
    ASSERT(!_dwEffect);
    ASSERT(!_hwnd);
    ASSERT(!_pszPath);

    _hwnd = hwnd;

    // If we are dropping onto the background of the window, we can assume that the window we
    // are passed is the target window of the copy.
    _fWindowIsTarget = fIsBkDropTarget;

    Str_SetPtr((LPTSTR *) &_pszPath, pszPath);
    IUnknown_Set((IUnknown **)&_pdtobj, (IUnknown *)pdtobj);

    // The caller doesn't get the return value in pdwEffect because it happens on a background thread.
    // These the hell out of this because we don't want a cancel operation to not make it back to a caller
    // and have them delete the files anyway.  We also need to make sure moves are done in such a way that the
    // destination (us) moves the files and not the caller.  This is the caller will return from ::Drop() before
    // the files finish copying (moving).
    _dwEffect = *pdwEffect;
}


CCopyThread::~CCopyThread()
{
    IUnknown_Set((IUnknown **)&_pdtobj, NULL);
    IUnknown_Set((IUnknown **)&_pstmDataObjMarshal, NULL);
    Str_SetPtr((LPTSTR *) &_pszPath, NULL);

    DllRelease();
}

HRESULT CCopyThread::DoAsynchCopy(void)
{
    HRESULT hr = CoMarshalInterThreadInterfaceInStream(IID_IDataObject, _pdtobj, &_pstmDataObjMarshal);
    if (SUCCEEDED(hr))
    {
        IUnknown_Set((IUnknown **)&_pdtobj, NULL);

        AddRef();   // pass to thread

        if (SHCreateThread(CCopyThread::CopyThreadProc, this, CTF_COINIT, NULL))
        {
            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
            Release();  // thread did not take, we need to release
        }
    }

    return hr;
}

HRESULT CCopyThread::_CopyThreadProc(void)
{
    IDataObject * pdo;
    HRESULT hr = CoGetInterfaceAndReleaseStream(_pstmDataObjMarshal, IID_IDataObject, (void **)&pdo);

    _pstmDataObjMarshal = NULL; // CoGetInterfaceAndReleaseStream() released the ref.
    if (EVAL(S_OK == hr))
    {
        hr = _DoCopy(pdo);

        IAsyncOperation * pao;
        if (EVAL(SUCCEEDED(pdo->QueryInterface(IID_IAsyncOperation, (void **) &pao))))
        {
            EVAL(SUCCEEDED(pao->EndOperation(hr, NULL, _dwEffect)));
            pao->Release();
        }

        pdo->Release();
    }

    Release();      // Releae the background thread's ref.
    return hr;
}


HRESULT CCopyThread::_DoCopy(IDataObject * pdo)
{
    HRESULT hres = S_OK;
    HRESULT hresOffset;
    FORMATETC fmteA = {g_cfFileGroupDescriptorA, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    FORMATETC fmteW = {g_cfFileGroupDescriptorW, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    FORMATETC fmteOffset = {g_cfOFFSETS, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM mediumFGD = {0}, mediumOffset = {0};
    BOOL fUnicode = FALSE;
    PROGRESSINFO progInfo = {0};

    // We should have only one bit set.
    ASSERT(_dwEffect==DROPEFFECT_COPY || _dwEffect==DROPEFFECT_LINK || _dwEffect==DROPEFFECT_MOVE);

    // Display the progress now because pdo->GetData may be slow, especially if we have to call it
    // twice.IDA_FILEMOVE
    progInfo.ppd = CProgressDialog_CreateInstance(((DROPEFFECT_COPY == _dwEffect) ? IDS_ACTIONTITLECOPY : IDS_ACTIONTITLEMOVE), ((DROPEFFECT_COPY == _dwEffect) ? IDA_FILECOPY : IDA_FILEMOVE), g_hinst);
    if (EVAL(progInfo.ppd))
    {
        WCHAR wzCalcTime[MAX_PATH];

        progInfo.uliBytesCompleted.QuadPart = progInfo.uliBytesTotal.QuadPart = 0;
        EVAL(SUCCEEDED(progInfo.ppd->StartProgressDialog(_hwnd, NULL, PROGDLG_AUTOTIME, NULL)));
        
        EVAL(LoadStringW(HINST_THISDLL, ((DROPEFFECT_COPY == _dwEffect) ? IDS_CALCCOPYTIME : IDS_CALCMOVETIME), wzCalcTime, ARRAYSIZE(wzCalcTime)));
        EVAL(SUCCEEDED(progInfo.ppd->SetLine(2, wzCalcTime, FALSE, NULL)));
    }

    // Try for UNICODE group descriptor first.  If that succeeds, we won't bother trying to
    // ASCII since UNICODE is the "preferred" format.  For ANSI builds, we only try for ANSI
    hres = pdo->GetData(&fmteW, &mediumFGD);
    if (SUCCEEDED(hres))
        fUnicode = TRUE;
    else
        hres = pdo->GetData(&fmteA, &mediumFGD);

    if (SUCCEEDED(hres))
    {
        UINT i, iConflict = 1;
        UINT iTopLevelItem = 0;
        YNLIST ynl;
        DROPHISTORY dh = {0};

        // WARNING: pfgd is really an A or W struct. to deal with this all code needs to use
        // the GetFileDescriptor() function

        FILEGROUPDESCRIPTOR *pfgd = (FILEGROUPDESCRIPTOR *)GlobalLock(mediumFGD.hGlobal);  
        DECLAREWAITCURSOR;

        SetWaitCursor();
        if (progInfo.ppd)
        {
            CalcBytesInFileGrpDescriptor(pfgd, fUnicode, &progInfo.uliBytesTotal);
            // We displayed progress above because pdo->GetData() and CalcBytesInFileGrpDescriptor are slow, but most likely it
            // was just eating into the delay time before the progress appears.  If the caller
            // didn't want UI, we will close it down now.
            if (!ShowProgressUI(pfgd))
                EVAL(SUCCEEDED(progInfo.ppd->StopProgressDialog()));
            else
                EVAL(SUCCEEDED(progInfo.ppd->Timer(PDTIMER_RESET, NULL)));
        }

        CreateYesNoList(&ynl);

        // Try & get the offsets too.
        hresOffset = pdo->GetData(&fmteOffset, &mediumOffset);
        if (SUCCEEDED(hresOffset))
        {
            dh.pptOffset = (POINT *)GlobalLock(mediumOffset.hGlobal);
            dh.pptOffset++;  // First item is the anchor
        }

        for (i = 0; i < pfgd->cItems; i++)
        {
            BOOL fTopLevel;
            TCHAR szFullPath[MAX_PATH], szFileName[MAX_PATH];
            FILEDESCRIPTOR *pfd = GetFileDescriptor(pfgd, fUnicode, i, szFileName);

            StrCpyN(szFullPath, _pszPath, ARRAYSIZE(szFullPath));

            // if the source gave us duplicate file names we make them unique here
            // foo (1).txt, foo (2).txt, etc
            // name conflicts with targets still get the replace file confirm UI
            if (IsNameInDescriptor(pfgd, fUnicode, szFileName, i))
            {
                TCHAR szBuf[MAX_PATH], *pszExt = PathFindExtension(szFileName);
                wnsprintf(szBuf, ARRAYSIZE(szBuf), TEXT(" (%d)%s"), iConflict++, pszExt);  // " (xxx).msg"
                // make sure it will fit
                if (((int)ARRAYSIZE(szFileName) - lstrlen(szFileName)) > (lstrlen(szBuf) - lstrlen(pszExt))) 
                    lstrcpy(pszExt, szBuf);
            }

            // do PathCleanupSpec on the filespec part of the filename because names
            // can be relative paths "Folder\foo.txt", "Folder\Folder2\foo.txt"
            PathCleanupSpec(szFullPath, PathFindFileName(szFileName));

            // the filename in the descriptor should not be a fully qualified path
            if (PathIsRelative(szFileName))
            {
                PathAppend(szFullPath, szFileName);
                fTopLevel = (StrChr(szFileName, TEXT('\\')) == NULL &&
                                StrChr(szFileName, TEXT('/')) == NULL);
            }
            else
            {
                TraceMsg(TF_WARNING, "CopyFGD: FGD contains full path - ignoring path");
                PathAppend(szFullPath, PathFindFileName(szFileName));
                fTopLevel = TRUE;
            }

            if (IsInNoList(&ynl, szFullPath))
            {
                continue;
            }

            HWND hwndDlgParent;
            if (FAILED(IUnknown_GetWindow(progInfo.ppd, &hwndDlgParent)))
            {
                hwndDlgParent = _hwnd;
            }

            BOOL fDirectory = (pfd->dwFlags & FD_ATTRIBUTES) && (pfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

            // NOTE: SHPathPrepareForWrite() was moved here to insure that we check for replace operations
            // against a real directory (not an empty a: drive, for example).  However, the result is not checked
            // until later - make sure that we don't overwrite 'hres' between here and there.
            hres = SHPathPrepareForWrite(hwndDlgParent, NULL, szFullPath, SHPPFW_DEFAULT | SHPPFW_IGNOREFILENAME);

            switch (ValidateCreateFileFromClip(hwndDlgParent, pfd, szFullPath, &ynl))
            {
            case IDYES:
                break;

            case IDNO:
                continue;

            case IDCANCEL:
                // NOTE: This doesn't do anything because the caller never gets this back
                //       in the asynch case.
                _dwEffect = 0;
                i = (int)pfgd->cItems - 1;
                hres = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                continue;
            }

            if (progInfo.ppd)
            {
                WCHAR wzFileName[MAX_PATH];
                WCHAR wzTemplateStr[MAX_PATH];
                WCHAR wzProgressStr[MAX_PATH];

                if (DROPEFFECT_COPY == _dwEffect)
                    EVAL(LoadStringW(HINST_THISDLL, IDS_COPYING, wzTemplateStr, ARRAYSIZE(wzTemplateStr)));
                else
                    EVAL(LoadStringW(HINST_THISDLL, IDS_MOVING, wzTemplateStr, ARRAYSIZE(wzTemplateStr)));

                // Display "Copying 'filename'" or "Moving 'filename'" on line 1
                SHTCharToUnicode(szFileName, wzFileName, ARRAYSIZE(wzFileName));
                wnsprintfW(wzProgressStr, ARRAYSIZE(wzProgressStr), wzTemplateStr, wzFileName);
                progInfo.ppd->SetLine(1, wzProgressStr, FALSE, NULL);

                // Display the dir on line 2
                CreateProgressStatusStr(_pszPath, wzProgressStr, ARRAYSIZE(wzProgressStr));
                progInfo.ppd->SetLine(2, wzProgressStr, FALSE, NULL);
            }


            if (fDirectory)
            {
                // Call SHPathPrepareForWrite() again without SHPPFW_IGNOREFILENAME so that it
                // will create the directory if it doesn't already exist.
                hres = SHPathPrepareForWrite(hwndDlgParent, NULL, szFullPath, SHPPFW_DEFAULT);
                
                if (FAILED(hres))
                {
                    // NOTE: This doesn't do anything because the caller never gets this back
                    //       in the asynch case.
                    _dwEffect = 0;
                    break;
                }
            }
            else
            {
                // We want to prepare the path both before and after errors in order to catch different cases.
                
                // NOTE: We should be checking the result of SHPathPrepareForWrite() here
                if (SUCCEEDED(hres))
                {
                    hres = DataObj_SaveToFile(pdo, g_cfFileContents, i, szFullPath,
                            ((pfd->dwFlags & FD_FILESIZE) ? pfd->nFileSizeLow : 0), &progInfo);
                }

                if (FAILED(hres))
                {
                    // Display an error if it wasn't caused by a user cancel.
                    if (HRESULT_FROM_WIN32(ERROR_CANCELLED) != hres)
                    {
                        TCHAR szTitle[MAX_PATH];

                        if (EVAL(LoadString(HINST_THISDLL, ((DROPEFFECT_COPY == _dwEffect) ? IDS_FILEERRORCOPY : IDS_FILEERRORCOPY), szTitle, ARRAYSIZE(szTitle))))
                        {
                            TCHAR szErrorMsg[MAX_PATH];

                            if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, HRESULT_CODE(hres), 0L, szErrorMsg, ARRAYSIZE(szErrorMsg), NULL) ||
                                GetWininetError(HRESULT_CODE(hres), (DROPEFFECT_COPY == _dwEffect), szErrorMsg, ARRAYSIZE(szErrorMsg)))
                            {
                                MessageBox(hwndDlgParent, szErrorMsg, szTitle, (MB_ICONERROR | MB_OK | MB_SYSTEMMODAL));
                            }
                        }
                    }

                    // NOTE: This doesn't do anything because the caller never gets this back
                    //       in the asynch case.
                    _dwEffect = 0;
                    break;

                }

                if (pfd->dwFlags & (FD_CREATETIME | FD_ACCESSTIME | FD_WRITESTIME))
                {
                    // BUGBUG: We already fired the SHChangeNotify() in DataObj_SaveToFile() so it won't have the new date. BAD BAD BAD
                    HANDLE hFile = CreateFile(szFullPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
                    if (hFile != INVALID_HANDLE_VALUE)
                    {
                        SetFileTime(hFile,
                                    pfd->dwFlags & FD_CREATETIME ? &pfd->ftCreationTime : NULL,
                                    pfd->dwFlags & FD_ACCESSTIME ? &pfd->ftLastAccessTime : NULL,
                                    pfd->dwFlags & FD_WRITESTIME ? &pfd->ftLastWriteTime : NULL);
                        CloseHandle(hFile);
                    }
                }
            }

            // Only position item if it was created successfully
            // and it is not tucked in a subdir.

            // The last condition is because there is some confusion about whether _hwnd is the
            // target window of the copy or not. If it is, we should tell the window
            // to position the item we just dropped.
            if (SUCCEEDED(hres) && fTopLevel && _fWindowIsTarget)
            {
                dh.iItem = iTopLevelItem;
                FS_PositionFileFromDrop(_hwnd, szFullPath, &dh);
                iTopLevelItem ++;
            }

            if ( SUCCEEDED( hres ) && (pfd->dwFlags & FD_ATTRIBUTES))
            {
                // set the rest of the attributes if passed...
                DWORD dwAttrs= (pfd->dwFileAttributes & ~FILE_ATTRIBUTE_DIRECTORY);
                if ( dwAttrs )
                    SetFileAttributes( szFullPath, dwAttrs );
            }

            if (progInfo.ppd)
            {
                ULARGE_INTEGER uliFileSize;
                uliFileSize.HighPart = pfd->nFileSizeHigh;
                uliFileSize.LowPart = pfd->nFileSizeLow;
                progInfo.uliBytesCompleted.QuadPart += uliFileSize.QuadPart;
                progInfo.ppd->SetProgress64(progInfo.uliBytesCompleted.QuadPart, progInfo.uliBytesTotal.QuadPart);

                if (progInfo.ppd->HasUserCancelled())
                    break;   // Cancel the copy.
            }
        }

        DestroyYesNoList(&ynl);

        if (SUCCEEDED(hresOffset))
            ReleaseStgMediumHGLOBAL(NULL, &mediumOffset);

        if (SUCCEEDED(hres))
        {
            // Inform the caller of what we did.  We don't do optimized moves
            // so the caller is responsible for the delete half of the move and
            // this is now we notify them of that.
            DataObj_SetDWORD(pdo, g_cfPerformedDropEffect, _dwEffect);
            DataObj_SetDWORD(pdo, g_cfLogicalPerformedDropEffect, _dwEffect);
        }

        ResetWaitCursor();
        ReleaseStgMediumHGLOBAL(pfgd, &mediumFGD);
    }

    if (progInfo.ppd)
    {
        progInfo.ppd->StopProgressDialog();
        progInfo.ppd->Release();
    }

    return hres;
}


//===========================
// *** IUnknown Interface ***
HRESULT CCopyThread::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CCopyThread, IUnknown),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}


ULONG CCopyThread::AddRef(void)
{
    return InterlockedIncrement(&_cRef);
}

ULONG CCopyThread::Release(void)
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}


HRESULT CreateInstance_CopyThread(HWND hwnd, LPCTSTR pszPath, IDataObject *pdtobj, DWORD *pdwEffect, BOOL fIsBkDropTarget, CCopyThread ** ppct)
{
    *ppct = new CCopyThread(hwnd, pszPath, pdtobj, pdwEffect, fIsBkDropTarget);
    return (*ppct ? S_OK : E_FAIL);
}


HRESULT FS_CreateFileFromClip(HWND hwnd, LPCTSTR pszPath, IDataObject *pdtobj, POINTL pt, DWORD *pdwEffect, BOOL fIsBkDropTarget)
{
    HRESULT hr = E_FAIL;
    CCopyThread * pct;

    hr = CreateInstance_CopyThread(hwnd, pszPath, pdtobj, pdwEffect, fIsBkDropTarget, &pct);
    if (SUCCEEDED(hr))
    {
        hr = pct->DoCopy();
        pct->Release();
    }

    return hr;
}


/*****************************************************************************\
    DESCRIPTION:
        We know that the IDataObject (pdo) supports the CF_FILEGROUPDESCRIPTOR
    clipboard format, so copy that data to the file system directory pszPath.
    The caller will want to know if this completed or if it was cancelled or
    errored out.  This result will come in the DROPEFFECT out param (pdwEffect).
    Zero (0) will indicate either error or cancel and this code will take care
    of displaying error messages.
\*****************************************************************************/
HRESULT FS_AsyncCreateFileFromClip(HWND hwnd, LPCTSTR pszPath, IDataObject * pdo, POINTL pt, DWORD *pdwEffect, BOOL fIsBkDropTarget)
{
    CCopyThread * pct;

    HRESULT hr = CreateInstance_CopyThread(hwnd, pszPath, pdo, pdwEffect, fIsBkDropTarget, &pct);
    if (SUCCEEDED(hr))
    {
        if (DataObj_CanGoAsync(pdo))
            hr = pct->DoAsynchCopy();
        else
            hr = pct->DoCopy();

        pct->Release();
    }

    if (FAILED(hr))
        *pdwEffect = DROPEFFECT_NONE;
    return hr;
}
