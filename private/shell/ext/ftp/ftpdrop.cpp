/*****************************************************************************\
    FILE: ftpdrop.cpp - IDropTarget interface

    Remarks:

    Note that you cannot create a shortcut on an FTP site.  Although
    there's nothing technically preventing it, it's not done because
    the shortcut won't be of much use on an FTP site.  (It points to
    your local machine, which doesn't help much for people not on the
    same network!)

    If you really want to put a shortcut file on an FTP site, create
    it on the desktop, then drag the shortcut onto the FTP site.

    The default verb for FTP sites is always "Copy".  This is true
    even if an intra-site drag-drop is being done.

    DESCRIPTION:
        DefView will cache the IDropTarget pointer (CFtpDrop) for a shell extension.
    When it calls CFtpDrop::Drop(), the work needs to be done on a background
    thread in order to not block the UI thread.  The problem is that if the user
    does another drag to the same Ftp Window, CFtpDrop::Drop() will be called again.
    For this reasons, CFtpDrop::Drop() cannot have any state after it returns.
    In order to accomplish this with the asynch background thread, we have
    CFtpDrop::Drop() call CDropOperation_Create(), and then CDropOperation->DoOperation().
    And then it will orphan (call Release()) the CDropOperation.  The CDropOperation
    will then destroy itself when the copy is finishes.  This enables subsequent calls
    to CFtpDrop::Drop() to spawn separate CDropOperation objects so each can maintain
    the state for that specifc operation and CFtpDrop remains stateless.
\*****************************************************************************/

#include "priv.h"
#include "ftpdrop.h"
#include "ftpurl.h"
#include "statusbr.h"
#include "newmenu.h"

class CDropOperation;
HRESULT CDropOperation_Create(CFtpFolder * pff, HWND hwnd, LPCTSTR pszzFSSource, LPCTSTR pszzFtpDest, CDropOperation ** ppfdt, DROPEFFECT de, OPS ops, int cobj);
HRESULT ConfirmCopy(LPCWSTR pszLocal, LPCWSTR pszFtpName, OPS * pOps, HWND hwnd, CFtpFolder * pff, CFtpDir * pfd, DROPEFFECT * pde, int nObjs, BOOL * pfFireChangeNotify);


// Declared because of recusion
HRESULT FtpCopyDirectory(HINTERNET hint, HINTPROCINFO * phpi, LPCOPYONEHDROPINFO pcohi);
HRESULT FtpCopyFile(HINTERNET hint, HINTPROCINFO * phpi, LPCOPYONEHDROPINFO pcohi);


HRESULT UpdateCopyFileName(LPCOPYONEHDROPINFO pcohi)
{
    HRESULT hr = S_OK;
    static WCHAR wzCopyTemplate[MAX_PATH] = L"";
    WCHAR wzLine1[MAX_PATH];

    if (!wzCopyTemplate[0])
        LoadStringW(HINST_THISDLL, IDS_COPYING, wzCopyTemplate, ARRAYSIZE(wzCopyTemplate));

    wnsprintfW(wzLine1, ARRAYSIZE(wzLine1), wzCopyTemplate, pcohi->pszFtpDest);

    EVAL(SUCCEEDED(pcohi->progInfo.ppd->SetLine(1, wzLine1, FALSE, NULL)));

    return hr;
}


HRESULT UpdateSrcDestDirs(LPCOPYONEHDROPINFO pcohi)
{
    HRESULT hr = S_OK;
    WCHAR wzFrom[MAX_PATH];
    WCHAR wzStatusStr[MAX_PATH];

    StrCpyN(wzFrom, pcohi->pszFSSource, ARRAYSIZE(wzFrom));
    PathRemoveFileSpecW(wzFrom);

    if (EVAL(SUCCEEDED(hr = CreateFromToStr(wzStatusStr, ARRAYSIZE(wzStatusStr), wzFrom, pcohi->pszDir))))
        EVAL(SUCCEEDED(hr = pcohi->progInfo.ppd->SetLine(2, wzStatusStr, FALSE, NULL)));    // Line one is the file being copied.

    return hr;
}


HRESULT DeleteOneFileCB(HINTERNET hint, HINTPROCINFO * phpi, LPVOID pv, BOOL * pfReleaseHint)
{
    LPCOPYONEHDROPINFO pcohi = (LPCOPYONEHDROPINFO) pv;
    WIRECHAR wFtpPath[MAX_PATH];

    phpi->pfd->GetFtpSite()->GetCWireEncoding()->UnicodeToWireBytes(pcohi->pmlc, pcohi->pszFtpDest, (phpi->pfd->IsUTF8Supported() ? WIREENC_USE_UTF8 : WIREENC_NONE), wFtpPath, ARRAYSIZE(wFtpPath));
    return FtpDeleteFileWrap(hint, TRUE, wFtpPath);
}


HRESULT UpdateProgressDialogStr(LPCOPYONEHDROPINFO pcohi)
{
    EVAL(SUCCEEDED(UpdateCopyFileName(pcohi)));
    EVAL(SUCCEEDED(UpdateSrcDestDirs(pcohi)));
    return S_OK;
}


/*****************************************************************************\
    CopyFileSysItem

    This function may cause recursion.
\*****************************************************************************/
HRESULT CopyFileSysItem(HINTERNET hint, HINTPROCINFO * phpi, LPCOPYONEHDROPINFO pcohi)
{
    HRESULT hr = S_OK;

    // Check if the user canceled.
    if (pcohi->progInfo.ppd)
    {
        if (pcohi->progInfo.ppd->HasUserCancelled())
            return HRESULT_FROM_WIN32(ERROR_CANCELLED);

        if (pcohi->dwOperation != COHDI_FILESIZE_COUNT)
            UpdateProgressDialogStr(pcohi);
    }

    if (PathIsDirectory(pcohi->pszFSSource))
    {
        hr = FtpCopyDirectory(hint, phpi, pcohi);

        if (SUCCEEDED(hr) && (pcohi->dwOperation != COHDI_FILESIZE_COUNT))
        {
            /*
            WIN32_FIND_DATA wfd;
            HANDLE handle = FindFirstFile(pcohi->pszFSSource, &wfd);

            // BUGBUG: The date is wrong doing it this way, but it's faster.  We should
            //         find out if FtpCreateDirectory always stamps the directory with
            //         the current date, and then update wfd with the current time/date.
            //         This will simulate the server entry w/o the perf hit.
            if (handle != INVALID_HANDLE_VALUE)
            {
                // If we are the root, then we need to notify the shell that
                // a folder was created so the view needs to be updated.
                // We fire the FtpChangeNotify() call for SHCNE_MKDIR in FtpCreateDirectoryWithCN().
                // FtpChangeNotify(SHCNE_MKDIR) is fired in FtpCreateDirectoryWithCN
                FindClose(handle);
            }
            */
        }
    }
    else
        hr = FtpCopyFile(hint, phpi, pcohi);

    return hr;
}


HRESULT FtpCopyItem(HINTERNET hint, HINTPROCINFO * phpi, LPCOPYONEHDROPINFO pcohi, LPWIN32_FIND_DATA pwfd, LPCWIRESTR pwCurrentDir)
{
    HRESULT hr = S_OK;
    TCHAR szFrom[MAX_PATH];
    WCHAR wzDestDir[MAX_PATH];
    TCHAR szServer[INTERNET_MAX_HOST_NAME_LENGTH];
    COPYONEHDROPINFO cohi = {pcohi->pff, szFrom, pwfd->cFileName, wzDestDir, pcohi->dwOperation, pcohi->ops, FALSE, pcohi->pmlc, pcohi->pidlServer, pcohi->fFireChangeNotify, NULL};
    CFtpDir * pfd = phpi->pfd;
    BOOL fSkipCurrentFile = FALSE;
    CWireEncoding * pwe = phpi->pfd->GetFtpSite()->GetCWireEncoding();

    cohi.progInfo.ppd = pcohi->progInfo.ppd;
    cohi.progInfo.hint = pcohi->progInfo.hint;
    cohi.progInfo.uliBytesCompleted.QuadPart = pcohi->progInfo.uliBytesCompleted.QuadPart;
    cohi.progInfo.uliBytesTotal.QuadPart = pcohi->progInfo.uliBytesTotal.QuadPart;

    EVAL(SUCCEEDED(pwe->WireBytesToUnicode(pcohi->pmlc, pwCurrentDir, (pfd->IsUTF8Supported() ? WIREENC_USE_UTF8 : WIREENC_NONE), wzDestDir, ARRAYSIZE(wzDestDir))));
    DisplayPathAppend(wzDestDir, ARRAYSIZE(wzDestDir), pcohi->pszFtpDest);

    if (EVAL(SUCCEEDED(pfd->GetFtpSite()->GetServer(szServer, ARRAYSIZE(szServer)))) &&
        SUCCEEDED(pfd->GetFtpSite()->GetFtpDir(szServer, wzDestDir, &(phpi->pfd))))
    {
        ASSERT(phpi->hwnd);
        // Make sure the user thinks it's ok to replace.  We don't care about replacing directories
        if ((pcohi->dwOperation != COHDI_FILESIZE_COUNT) &&
            !(FILE_ATTRIBUTE_DIRECTORY & pwfd->dwFileAttributes))
        {
            TCHAR szSourceFile[MAX_PATH];

            StrCpyN(szSourceFile, pcohi->pszFSSource, ARRAYSIZE(szSourceFile));
            if (PathAppend(szSourceFile, pwfd->cFileName))
            {
                // PERF: We should do the Confirm copy only if the upload fails because it's
                //       so costly.
                hr = ConfirmCopy(szSourceFile, pwfd->cFileName, &(cohi.ops), phpi->hwnd, pcohi->pff, phpi->pfd, NULL, 1, &cohi.fFireChangeNotify);
                if (S_FALSE == hr)
                {
                    // S_FALSE from ConfirmCopy() means doen't replace this specific file, but continue
                    // copying.  We need to return S_OK or we will cancel copying all the files.
                    fSkipCurrentFile = TRUE;
                    hr = S_OK;
                }
            }
            else
                hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);    // Path too long, probably.
        }

        if (!fSkipCurrentFile && (S_OK == hr) && IS_VALID_FILE(pwfd->cFileName))
        {
            StrCpyN(szFrom, pcohi->pszFSSource, ARRAYSIZE(szFrom));     // Set the source directory.
            // Specify the file/dir in that directory to copy.
            if (PathAppend(szFrom, pwfd->cFileName))
            {
                // 5. Call CopyFileSysItem() to get it copied (maybe recursively)
                //TraceMsg(TF_FTPOPERATION, "FtpCopyDirectory() calling CopyFileSysItem(From=%s. To=%s)", szFrom, pwfd->cFileName);
                hr = CopyFileSysItem(hint, phpi, &cohi);
                if (FAILED(hr) && (HRESULT_FROM_WIN32(ERROR_CANCELLED) != hr) &&
                    (pcohi->dwOperation != COHDI_FILESIZE_COUNT))
                {
                    int nResult = DisplayWininetError(phpi->hwnd, TRUE, HRESULT_CODE(hr), IDS_FTPERR_TITLE_ERROR, IDS_FTPERR_FILECOPY, IDS_FTPERR_WININET, MB_OK, pcohi->progInfo.ppd);
                    hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                }
            }
            else
                hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);    // Path too long, probably.
        }


        pcohi->progInfo.hint = cohi.progInfo.hint;    // Maybe the user cancelled.
        pcohi->progInfo.uliBytesCompleted.QuadPart = cohi.progInfo.uliBytesCompleted.QuadPart;
        pcohi->progInfo.uliBytesTotal.QuadPart = cohi.progInfo.uliBytesTotal.QuadPart;
        pcohi->ops = cohi.ops;
        phpi->pfd->Release();
    }

    phpi->pfd = pfd;
    return hr;
}

HRESULT _FtpSetCurrentDirectory(HINTERNET hint, HINTPROCINFO * phpi, LPCWSTR pwzFtpPath)
{
    HRESULT hr;
    WIRECHAR wFtpPath[MAX_PATH];
    CWireEncoding * pwe = phpi->pfd->GetFtpSite()->GetCWireEncoding();

    hr = pwe->UnicodeToWireBytes(NULL, pwzFtpPath, (phpi->pfd->IsUTF8Supported() ? WIREENC_USE_UTF8 : WIREENC_NONE), wFtpPath, ARRAYSIZE(wFtpPath));
    if (SUCCEEDED(hr))
        hr = FtpSetCurrentDirectoryWrap(hint, TRUE, wFtpPath);

    return hr;
}

/*****************************************************************************\
     FtpCopyDirectory
 
    DESCRIPTION:
        This function will need to copy all the items in the directory to the
    FTP server if the item is a folder, it will need to recurse.

    Recursion algorithm:
    // 1. Create Directory
    // 2. Get Current Directory (To save for later).
    // 3. Change Directory Into new Directory.
    // 4. Find Next item (file/dir) in file system
    // 5. Call CopyFileSysItem() to get it copied (maybe recursively)
    // 6. Go to Step 4 if there are any left.
    // 7. Go back to original directory (Step 2)
\*****************************************************************************/
HRESULT FtpCopyDirectory(HINTERNET hint, HINTPROCINFO * phpi, LPCOPYONEHDROPINFO pcohi)
{
    HRESULT hr = S_OK;

    if (phpi->psb && (pcohi->dwOperation != COHDI_FILESIZE_COUNT))
        phpi->psb->SetStatusMessage(IDS_COPYING, pcohi->pszFSSource);

    //TraceMsg(TF_FTPOPERATION, "FtpCopyDirectory() calling FtpCreateDirectoryA(%s)", pcohi->pszFSSource);

    // Create the directories on the first pass when we calculate file sizes.
    // We then skip creating them on the copy pass.
    if (pcohi->dwOperation == COHDI_FILESIZE_COUNT)
    {
        hr = FtpSafeCreateDirectory(phpi->hwnd, hint, pcohi->pmlc, pcohi->pff, phpi->pfd, pcohi->progInfo.ppd, pcohi->pszFtpDest, pcohi->fIsRoot);
    }

    // 1. Create Directory
    if (SUCCEEDED(hr))
    {
        WIRECHAR wCurrentDir[MAX_PATH];

        hr = FtpGetCurrentDirectoryWrap(hint, TRUE, wCurrentDir, ARRAYSIZE(wCurrentDir));
        if (EVAL(SUCCEEDED(hr)))
        {
            // NOTE: At this point, pcohi->pszFSSource is the DIRECTORY on the local
            //       file system that is being copied.
            hr = _FtpSetCurrentDirectory(hint, phpi, pcohi->pszFtpDest);
            if (SUCCEEDED(hr))
            {
                WCHAR szSearchStr[MAX_PATH*2];
                WIN32_FIND_DATA wfd;
                HANDLE handle = NULL;

                StrCpyN(szSearchStr, pcohi->pszFSSource, ARRAYSIZE(szSearchStr));
                // We need to copy the entire directory.
                if (PathAppend(szSearchStr, SZ_ALL_FILES))
                {
                    // 4. Find Next item (file/dir) in file system
                    handle = FindFirstFile(szSearchStr, &wfd);
                    if (handle != INVALID_HANDLE_VALUE)
                    {
                        do
                        {
                            //TraceMsg(TF_WININET_DEBUG, "FindFirstFileNext() returned %s", wfd.cFileName);
                            hr = FtpCopyItem(hint, phpi, pcohi, &wfd, wCurrentDir);

                            // 6. Check if the user canceled.
                            if ((pcohi->progInfo.ppd) && (pcohi->progInfo.ppd->HasUserCancelled()))
                            {
                                hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                                break;
                            }

                            // 7. Repeat if there are any left and it wasn't cancelled (S_FALSE)
                        }
                        while ((S_OK == hr) && FindNextFile(handle, &wfd));

                        FindClose(handle);
                    }
                }
                else
                    hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);    // Path too long, probably.
            }

            // 7. Go back to original directory (from Step 2)
            // The only time we don't want to return to the original directory is if
            // the hinst was freed in an wininet callback function.  We may cache the hinst
            // so we need the directory to be valid later.
            if (pcohi->progInfo.hint)
            {
                EVAL(SUCCEEDED(FtpSetCurrentDirectoryWrap(hint, TRUE, wCurrentDir)));
            }
        }
    }
    else
    {
        if (HRESULT_FROM_WIN32(ERROR_CANCELLED) != hr)
        {
            DisplayWininetError(phpi->hwnd, TRUE, HRESULT_CODE(hr), IDS_FTPERR_TITLE_ERROR, IDS_FTPERR_DIRCOPY, IDS_FTPERR_WININET, MB_OK, pcohi->progInfo.ppd);
            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
        }
    }

    return hr;
}


HRESULT UpdateCopyProgressInfo(IProgressDialog * ppd, LPCTSTR pszFileName)
{
    HRESULT hr = E_FAIL;
    TCHAR szTemplate[MAX_PATH];

    if (EVAL(LoadString(HINST_THISDLL, IDS_COPYING, szTemplate, ARRAYSIZE(szTemplate))))
    {
        TCHAR szStatusStr[MAX_PATH];
        WCHAR wzStatusStr[MAX_PATH];

        wnsprintf(szStatusStr, ARRAYSIZE(szStatusStr), szTemplate, pszFileName);
        SHTCharToUnicode(szStatusStr, wzStatusStr, ARRAYSIZE(wzStatusStr));
        EVAL(SUCCEEDED(hr = ppd->SetLine(1, wzStatusStr, FALSE, NULL)));
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: _FireChangeNotify

    DESCRIPTION:
        asd
\*****************************************************************************/
HRESULT _FireChangeNotify(HINTPROCINFO * phpi, LPCOPYONEHDROPINFO pcohi)
{
    HRESULT hr = S_OK;
    WIN32_FIND_DATA wfd;
    HANDLE handle = FindFirstFile(pcohi->pszFSSource, &wfd);

    TraceMsg(TF_WININET_DEBUG, "_FireChangeNotify() FtpPutFileEx(%s -> %s) succeeded", pcohi->pszFSSource, pcohi->pszFtpDest);
    if (handle != INVALID_HANDLE_VALUE)
    {
        ULARGE_INTEGER uliFileSize;
        FTP_FIND_DATA ffd;
        CWireEncoding * pwe = pcohi->pff->GetCWireEncoding();

        uliFileSize.LowPart = wfd.nFileSizeLow;
        uliFileSize.HighPart = wfd.nFileSizeHigh;
        pcohi->progInfo.uliBytesCompleted.QuadPart += uliFileSize.QuadPart;

        hr = pwe->UnicodeToWireBytes(pcohi->pmlc, wfd.cFileName, (phpi->pfd->IsUTF8Supported() ? WIREENC_USE_UTF8 : WIREENC_NONE), ffd.cFileName, ARRAYSIZE(ffd.cFileName));
        if (EVAL(SUCCEEDED(hr)))
        {
            LPITEMIDLIST pidlFtpFile;
            SYSTEMTIME st;
            FILETIME ftUTC;

            ffd.dwFileAttributes = wfd.dwFileAttributes;
            ffd.dwReserved0 = wfd.dwReserved0;
            ffd.dwReserved1 = wfd.dwReserved1;
            ffd.nFileSizeHigh = wfd.nFileSizeHigh;
            ffd.nFileSizeLow = wfd.nFileSizeLow;

            // wfd.ft*Time is in UTF and FtpItemID_CreateReal wants
            // it in LocalTime, so we need to convert here.
            GetSystemTime(&st);
            SystemTimeToFileTime(&st, &ftUTC);
            FileTimeToLocalFileTime(&ftUTC, &ffd.ftLastWriteTime);   // UTC->LocalTime
            ffd.ftCreationTime = ffd.ftLastWriteTime;
            ffd.ftLastAccessTime = ffd.ftLastWriteTime;

            hr = FtpItemID_CreateReal(&ffd, pcohi->pszFtpDest, &pidlFtpFile);
            if (SUCCEEDED(hr))
            {
                // Note that we created the mapped name
                // PERF: Note that we give the time/date stamp to SHChangeNotify that comes from the source
                //       file, not from the FTP server, so it may be inforrect.  However, it's perf prohibitive
                //       to do the right thing.
                FtpChangeNotify(phpi->hwnd, SHCNE_CREATE, pcohi->pff, phpi->pfd, pidlFtpFile, NULL, pcohi->fIsRoot);
                ILFree(pidlFtpFile);
            }
        }

        FindClose(handle);
    }

    return hr;
}


#define CCH_SIZE_ERROR_MESSAGE  6*1024

/*****************************************************************************\
    FtpCopyFile

    Callback procedure that copies a single hdrop / map.
    Should I try to make the name unique in case of collision?
    Naah, just prompt, but! no way to tell if destination is case-sensitive...
\*****************************************************************************/
HRESULT FtpCopyFile(HINTERNET hint, HINTPROCINFO * phpi, LPCOPYONEHDROPINFO pcohi)
{
    HRESULT hr = S_OK;

    if (pcohi->dwOperation != COHDI_FILESIZE_COUNT)
    {
        WIRECHAR wWireName[MAX_PATH];

        EVAL(SUCCEEDED(pcohi->pff->GetCWireEncoding()->UnicodeToWireBytes(pcohi->pmlc, pcohi->pszFtpDest, (pcohi->pff->IsUTF8Supported() ? WIREENC_USE_UTF8 : WIREENC_NONE), wWireName, ARRAYSIZE(wWireName))));

        if (phpi->psb)
            phpi->psb->SetStatusMessage(IDS_COPYING, pcohi->pszFSSource);

        if (pcohi->progInfo.ppd)
        {
            EVAL(SUCCEEDED(UpdateCopyProgressInfo(pcohi->progInfo.ppd, pcohi->pszFtpDest)));
            EVAL(SUCCEEDED(pcohi->progInfo.ppd->SetProgress64(pcohi->progInfo.uliBytesCompleted.QuadPart, pcohi->progInfo.uliBytesTotal.QuadPart)));
        }

        pcohi->progInfo.dwCompletedInCurFile = 0;
        pcohi->progInfo.dwLastDisplayed = 0;

        // BUGBUG: We need to pass the FTP_TRANSFER_TYPE (_ASCII vs. _BINARY)
        hr = FtpPutFileExWrap(hint, TRUE, pcohi->pszFSSource, wWireName, FTP_TRANSFER_TYPE_UNKNOWN, (DWORD_PTR)&(pcohi->progInfo));
        if (SUCCEEDED(hr))
        {
            // We don't fire change notify on browser only if we
            // are replacing a file because ChangeNotify really
            // just hacks ListView and doen't know how to handle
            // duplicates (file replace).
            if (pcohi->fFireChangeNotify)
                hr = _FireChangeNotify(phpi, pcohi);
        }
        else
        {
            if (HRESULT_FROM_WIN32(ERROR_INTERNET_OPERATION_CANCELLED) == hr)
            {
                // Clean up the file.
                EVAL(SUCCEEDED(phpi->pfd->WithHint(NULL, phpi->hwnd, DeleteOneFileCB, pcohi, NULL, pcohi->pff)));
                hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
            }
            else
            {
                // We still want to delete the file, but we need to save the error message
                // so the dialog is correct.
                CHAR szErrorMsg[CCH_SIZE_ERROR_MESSAGE];
                WCHAR wzErrorMsg[CCH_SIZE_ERROR_MESSAGE];
                DWORD cchSize = ARRAYSIZE(szErrorMsg);
                InternetGetLastResponseInfoWrap(TRUE, NULL, szErrorMsg, &cchSize);
                HRESULT hrOrig = hr;
                CWireEncoding * pwe = phpi->pfd->GetFtpSite()->GetCWireEncoding();

                pwe->WireBytesToUnicode(NULL, szErrorMsg, WIREENC_NONE, wzErrorMsg, ARRAYSIZE(wzErrorMsg));
                // Does it already exist?  This may fail.
                SUCCEEDED(phpi->pfd->WithHint(NULL, phpi->hwnd, DeleteOneFileCB, pcohi, NULL, pcohi->pff));

                // No, so it was a real error, now display the error message with the original
                // server response.
                DisplayWininetErrorEx(phpi->hwnd, TRUE, HRESULT_CODE(hrOrig), IDS_FTPERR_TITLE_ERROR, IDS_FTPERR_FILECOPY, IDS_FTPERR_WININET, MB_OK, pcohi->progInfo.ppd, wzErrorMsg);
                hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
            }
        }
    }
    else
    {
        // Just get the file size.   
        WIN32_FIND_DATA wfd;
        HANDLE handle = FindFirstFile(pcohi->pszFSSource, &wfd);

        if (handle && (handle != INVALID_HANDLE_VALUE))
        {
            ULARGE_INTEGER uliFileSize;
            uliFileSize.LowPart = wfd.nFileSizeLow;
            uliFileSize.HighPart = wfd.nFileSizeHigh;
            pcohi->progInfo.uliBytesTotal.QuadPart += uliFileSize.QuadPart;
            FindClose(handle);
        }
    }

    //TraceMsg(TF_FTPOPERATION, "FtpPutFileA(From=%ls, To=%s) hr=%#08lX", pcohi->pszFSSource, pcohi->pszFtpDest, hr);
    return hr;
}


/*****************************************************************************\
    _EnumOneHdropW

    Handle one hdrop and corresponding filemap.

    This is annoying because we need to convert from UNICODE to ANSI.
\*****************************************************************************/
#define OleStrToStrA(a, b) OleStrToStrN(a, ARRAYSIZE(a), b, -1)

HRESULT _EnumOneHdropW(LPCWSTR * ppwzzFSSources, LPCWSTR * ppwzzFtpDest, LPTSTR pszFSSourceOut, DWORD cchFSSourceOut, LPTSTR pszFtpDestOut, DWORD cchFtpDestOut)
{
    HRESULT hres;
    int cwch;

    if (*ppwzzFSSources && (*ppwzzFSSources)[0])
    {
        cwch = SHUnicodeToTChar(*ppwzzFSSources, pszFSSourceOut, cchFSSourceOut);
        if (EVAL(cwch))
        {
            *ppwzzFSSources += cwch;
            if (EVAL((*ppwzzFtpDest)[0]))
            {
                cwch = SHUnicodeToTChar(*ppwzzFtpDest, pszFtpDestOut, cchFtpDestOut);
                if (EVAL(cwch))
                {
                    *ppwzzFtpDest += cwch;
                    hres = S_OK;    // Both strings converted okay
                }
                else
                    hres = E_UNEXPECTED; // File name too long
            }
            else
                hres = E_UNEXPECTED;    // Premature EOF in map
        }
        else
            hres = E_UNEXPECTED;    // File name too long
    }
    else
        hres = S_FALSE;            // End of buffer

    return hres;
}


/*****************************************************************************\
    _EnumOneHdropA

    Handle one hdrop and corresponding filemap.
\*****************************************************************************/
HRESULT _EnumOneHdropA(LPCSTR * ppszzFSSource, LPCSTR * ppszzFtpDest, LPTSTR pszFSSourceOut, DWORD cchFSSourceOut, LPTSTR pszFtpDestOut, DWORD cchFtpDestOut)
{
    HRESULT hres;

    if ((*ppszzFSSource)[0])
    {
        SHAnsiToTChar(*ppszzFSSource, pszFSSourceOut, cchFSSourceOut);
        *ppszzFSSource += lstrlenA(*ppszzFSSource) + 1;
        if (EVAL((*ppszzFtpDest)[0]))
        {
            SHAnsiToTChar(*ppszzFtpDest, pszFtpDestOut, cchFtpDestOut);
            *ppszzFtpDest += lstrlenA(*ppszzFtpDest) + 1;
            hres = S_OK;        // No problemo
        }
        else
            hres = E_UNEXPECTED;    // Premature EOF in map
    }
    else
        hres = S_FALSE;            // No more files

    return hres;
}


/*****************************************************************************\
    ConfirmCopy

    Callback procedure that checks if this file really ought to be
    copied.

    Returns S_OK if the file should be copied.
    Returns S_FALSE if the file should not be copied.

    - If the user cancelled, then say S_FALSE from now on.
    - If the user said Yes to All, then say S_OK.
    - If there is no conflict, then say S_OK.
    - If the user said No to All, then say S_FALSE.
    - Else, ask the user what to do.

    Note that the order of the tests above means that if you say
    "Yes to All", then we don't waste our time doing overwrite checks.

    _GROSS_:  NOTE! that we don't try to uniquify the name, because
    WinINet doesn't support the STOU (store unique) command, and
    there is no way to know what filenames are valid on the server.
\*****************************************************************************/
HRESULT ConfirmCopy(LPCWSTR pszLocal, LPCWSTR pszFtpName, OPS * pOps, HWND hwnd, CFtpFolder * pff, CFtpDir * pfd, DROPEFFECT * pde, int nObjs, BOOL * pfFireChangeNotify)
{
    HRESULT hr = S_OK;

    *pfFireChangeNotify = TRUE;
    if (*pOps == opsCancel)
        hr = S_FALSE;
    else 
    {
        HANDLE hfind;
        WIN32_FIND_DATA wfdSrc;
        hfind = FindFirstFile(pszLocal, &wfdSrc);
        if (hfind != INVALID_HANDLE_VALUE)
        {
            FindClose(hfind);

            // Is it a file?  We don't care about confirming the replacement
            // of directories.
            if (!(FILE_ATTRIBUTE_DIRECTORY & wfdSrc.dwFileAttributes))
            {
                FTP_FIND_DATA wfd;
                hr = pfd->GetFindDataForDisplayPath(hwnd, pszFtpName, &wfd, pff);
                if (*pOps == opsYesToAll)
                {
                    // If the file exists (S_OK) and it's browser only, 
                    // then don't fire the change notify.
                    if ((S_OK == hr) && (SHELL_VERSION_NT5 != GetShellVersion()))
                        *pfFireChangeNotify = FALSE;

                    hr = S_OK;
                }
                else
                {
                    switch (hr)
                    {
                    case S_OK:            // File exists; worry
                        if (*pOps == opsNoToAll)
                            hr = S_FALSE;
                        else
                        {
                            FILETIME ftUTC = wfdSrc.ftLastWriteTime;
    
                            FileTimeToLocalFileTime(&ftUTC, &wfdSrc.ftLastWriteTime);   // UTC->LocalTime
                            // BUGBUG/TODO: Do we need to set modal?
                            switch (FtpConfirmReplaceDialog(hwnd, &wfdSrc, &wfd, nObjs, pff))
                            {
                            case IDC_REPLACE_YESTOALL:
                                *pOps = opsYesToAll;
                                // FALLTHROUGH

                            case IDC_REPLACE_YES:
                                // pre-NT5 doesn't work 
                                if (SHELL_VERSION_NT5 != GetShellVersion())
                                    *pfFireChangeNotify = FALSE;

                                hr = S_OK;
                                break;

                            case IDC_REPLACE_NOTOALL:
                                *pOps = opsNoToAll;
                                // FALLTHROUGH

                            case IDC_REPLACE_NO:
                                hr = S_FALSE;
                                break;

                            default:
                                ASSERT(0);        // Huh?
                                // FALLTHROUGH

                            case IDC_REPLACE_CANCEL:
                                if (pde)
                                    *pde = 0;

                                *pOps = opsCancel;
                                hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                                break;
                            }
                        }
                        break;

                    case S_FALSE:
                    default:
                        // Assume the file doesn't exist; no problemo
                        hr = S_OK;
                        break;
                    }
                }
            }
        }
        else
        {                   // File doesn't exist
            hr = S_OK;    // The open will raise the error
        }

    }

    //TraceMsg(TF_FTPDRAGDROP, "ConfirmCopy(%s) -> %08x", pszFtpName, hr);
    return hr;
}




/*****************************************************************************\
    CLASS: CDropOperation

    DESCRIPTION:
        DefView will cache the IDropTarget pointer (CFtpDrop) for a shell extension.
    When it calls CFtpDrop::Drop(), the work needs to be done on a background
    thread in order to not block the UI thread.  The problem is that if the user
    does another drag to the same Ftp Window, CFtpDrop::Drop() will be called again.
    For this reasons, CFtpDrop::Drop() cannot have any state after it returns.
    In order to accomplish this with the asynch background thread, we have
    CFtpDrop::Drop() call CDropOperation_Create(), and then CDropOperation->DoOperation().
    And then it will orphan (call Release()) the CDropOperation.  The CDropOperation
    will then destroy itself when the copy is finishes.  This enables subsequent calls
    to CFtpDrop::Drop() to spawn separate CDropOperation objects so each can maintain
    the state for that specifc operation and CFtpDrop remains stateless.
\*****************************************************************************/
class CDropOperation          : public IUnknown
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

public:
    CDropOperation();
    ~CDropOperation(void);

    // Public Member Functions
    HRESULT DoOperation(BOOL fAsync);

    static HRESULT CopyCB(HINTERNET hint, HINTPROCINFO * phpi, LPVOID pv, BOOL * pfReleaseHint);

    // Friend Functions
    friend HRESULT CDropOperation_Create(CFtpFolder * pff, HWND hwnd, LPCTSTR pszzFSSource, LPCTSTR pszzFtpDest, CDropOperation ** ppfdt, DROPEFFECT de, OPS ops, int cobj);

protected:
    // Protected Member Variables
    int                     m_cRef;

    CFtpFolder *            m_pff;          // The owner
    CFtpDir *               m_pfd;          // The FtpDir of the owner
    HWND                    m_hwnd;         // The window being drug over

    DROPEFFECT              m_de;           // Effect being performed
    OPS                     m_ops;          // Overwrite prompting state
    int                     m_cobj;         // Number of objects being dropped
    ULARGE_INTEGER          m_uliBytesCompleted;
    ULARGE_INTEGER          m_uliBytesTotal;


    // Private Member Functions
    HRESULT _ConfirmCopy(LPCWSTR pszLocal, LPCWSTR psz, BOOL * pfFireChangeNotify);
    HRESULT _CalcSizeOneHdrop(LPCWSTR pszFSSource, LPCWSTR pszFtpDest, IProgressDialog * ppd);
    HRESULT _ThreadProcCB(void);
    HRESULT _CopyOneHdrop(LPCWSTR pszFSSource, LPCWSTR pszFtpDest);

    HRESULT _StartBackgroundInteration(void);
    HRESULT _DoCopyIteration(void);
    HRESULT _CalcUploadProgress(void);

private:
    // Private Member Variables
    IProgressDialog *       m_ppd;
    LPCWSTR                 m_pszzFSSource;            // Paths
    LPCWSTR                 m_pszzFtpDest;              // Map
    CMultiLanguageCache     m_mlc;          // Cache for fast str thunking.

    static DWORD CALLBACK _ThreadProc(LPVOID pThis) {return ((CDropOperation *)pThis)->_ThreadProcCB();};
};


HRESULT CDropOperation_Create(CFtpFolder * pff, HWND hwnd, LPCTSTR pszzFSSource, LPCTSTR pszzFtpDest, CDropOperation ** ppfdt, 
                              DROPEFFECT de, OPS ops, int cobj)
{
    HRESULT hr = E_OUTOFMEMORY;
    CDropOperation * pfdt = new CDropOperation();
    *ppfdt = pfdt;

    if (pfdt)
    {
        pfdt->m_hwnd = hwnd;

        // Copy the CFtpFolder * value
        pfdt->m_pff = pff;
        if (pff)
            pff->AddRef();

        // Copy the CFtpDir * value
        ASSERT(!pfdt->m_pfd);
        pfdt->m_pfd = pff->GetFtpDir();
        ASSERT(pfdt->m_pfd);

        ASSERT(!pfdt->m_pszzFSSource);
        pfdt->m_pszzFSSource = pszzFSSource;

        ASSERT(!pfdt->m_pszzFtpDest);
        pfdt->m_pszzFtpDest = pszzFtpDest;

        pfdt->m_de = de;           // Effect being performed
        pfdt->m_ops = ops;          // Overwrite prompting state
        pfdt->m_cobj = cobj;         // Number of objects being dropped

        hr = S_OK;
    }

    ASSERT_POINTER_MATCHES_HRESULT(*ppfdt, hr);
    return hr;
}


/****************************************************\
    Constructor
\****************************************************/
CDropOperation::CDropOperation() : m_cRef(1)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!m_pff);
    ASSERT(!m_pfd);
    ASSERT(!m_hwnd);
    ASSERT(!m_cobj);

    LEAK_ADDREF(LEAK_CDropOperation);
}


/****************************************************\
    Destructor
\****************************************************/
CDropOperation::~CDropOperation()
{
    // use ATOMICRELEASE
    IUnknown_Set(&m_pff, NULL);
    IUnknown_Set(&m_pfd, NULL);
    IUnknown_Set((IUnknown **)&m_ppd, NULL);
    Str_SetPtr((LPTSTR *) &m_pszzFSSource, NULL);
    Str_SetPtr((LPTSTR *) &m_pszzFtpDest, NULL);

    DllRelease();
    LEAK_DELREF(LEAK_CDropOperation);
}


//===========================
// *** IUnknown Interface ***
//===========================

ULONG CDropOperation::AddRef()
{
    m_cRef++;
    return m_cRef;
}

ULONG CDropOperation::Release()
{
    ASSERT(m_cRef > 0);
    m_cRef--;

    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}

HRESULT CDropOperation::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = SAFECAST(this, IUnknown*);
    }
    else
    {
        TraceMsg(TF_FTPQI, "CDropOperation::QueryInterface() failed.");
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}




/****************************************************\
    FUNCTION: _ThreadProcCB

    DESCRIPTION:
\****************************************************/
HRESULT CDropOperation::_ThreadProcCB(void)
{
    HRESULT hr;
    HRESULT hrOleInit = SHCoInitialize();
    
    // WARNING: Init OLE if you plan to do COM.
    m_ppd = CProgressDialog_CreateInstance(IDS_COPY_TITLE, IDA_FTPUPLOAD);
    if (EVAL(m_ppd))
    {
        ASSERT(m_hwnd);
        // We give a NULL punkEnableModless because we don't want to go modal.
        EVAL(SUCCEEDED(m_ppd->StartProgressDialog(m_hwnd, NULL, PROGDLG_AUTOTIME, NULL)));
    }

    hr = _CalcUploadProgress();
    // Did we succeed creating the directories and counting the
    // size we need to copy?
    if (SUCCEEDED(hr))
    {
        if (m_ppd)
        {
            EVAL(SUCCEEDED(m_ppd->SetProgress64(m_uliBytesCompleted.QuadPart, m_uliBytesTotal.QuadPart)));

            // Reset because _CalcUploadProgress() can take a long time and the estimated time
            // is based on the time between ::StartProgressDialog() and the first
            // ::SetProgress() call.
            EVAL(SUCCEEDED(m_ppd->Timer(PDTIMER_RESET, NULL)));
        }

        hr = _DoCopyIteration();
    }

    if (m_ppd)
    {
        EVAL(SUCCEEDED(m_ppd->StopProgressDialog()));
        ATOMICRELEASE(m_ppd);
    }

    SHCoUninitialize(hrOleInit);
    Release();
    return hr;
}


HRESULT CDropOperation::DoOperation(BOOL fAsync)
{
    HRESULT hr = S_OK;

    AddRef();
    if (fAsync)
    {
        HANDLE hThread;
        DWORD dwThreadId;

        hThread = CreateThread(NULL, 0, CDropOperation::_ThreadProc, this, 0, &dwThreadId);
        if (hThread)
            CloseHandle(hThread);
        else
        {
            TraceMsg(TF_ERROR, "CDropOperation::DoOperation() CreateThread() failed and GetLastError()=%lu.", GetLastError());
            Release();
        }
    }
    else
        hr = _ThreadProcCB();

    return hr;
}



/****************************************************\
    FUNCTION: _CalcUploadProgress

    DESCRIPTION:
\****************************************************/
HRESULT CDropOperation::_CalcUploadProgress(void)
{
    HRESULT hr = S_OK;
    LPCWSTR pszzFSSource = m_pszzFSSource;
    LPCWSTR pszzFtpDest = m_pszzFtpDest;
    WCHAR wzProgressDialogStr[MAX_PATH];

    m_uliBytesCompleted.QuadPart = 0;
    m_uliBytesTotal.QuadPart = 0;
    
    // Tell the user we are calculating how long it will take.
    if (EVAL(LoadStringW(HINST_THISDLL, IDS_PROGRESS_UPLOADTIMECALC, wzProgressDialogStr, ARRAYSIZE(wzProgressDialogStr))))
        EVAL(SUCCEEDED(m_ppd->SetLine(2, wzProgressDialogStr, FALSE, NULL)));

    while (S_OK == hr)
    {
        WCHAR szFSSource[MAX_PATH];
        WCHAR szFtpDest[MAX_PATH];

        hr = _EnumOneHdrop(&pszzFSSource, &pszzFtpDest, szFSSource, ARRAYSIZE(szFSSource), szFtpDest, ARRAYSIZE(szFtpDest));
        if (S_OK == hr)
            hr = _CalcSizeOneHdrop(szFSSource, szFtpDest, m_ppd);
    }

    if (FAILED(hr))
        TraceMsg(TF_ALWAYS, "CDropOperation::_CalcUploadProgress() Calculating the upload time failed, but oh well.");

    return hr;
}


HRESULT CDropOperation::_CalcSizeOneHdrop(LPCWSTR pszFSSource, LPCWSTR pszFtpDest, IProgressDialog * ppd)
{
    HRESULT hr;
    WCHAR wzTo[MAX_PATH];

    EVAL(SUCCEEDED(m_pfd->GetDisplayPath(wzTo, ARRAYSIZE(wzTo))));
    pszFtpDest = PathFindFileName(pszFtpDest);

    COPYONEHDROPINFO cohi = {0};

    cohi.pff = m_pff;
    cohi.pszFSSource = pszFSSource;
    cohi.pszFtpDest = pszFtpDest;
    cohi.pszDir = wzTo;
    cohi.dwOperation = COHDI_FILESIZE_COUNT;
    cohi.ops = opsPrompt;
    cohi.fIsRoot = TRUE;
    cohi.pmlc = &m_mlc;
    cohi.pidlServer = FtpCloneServerID(m_pff->GetPrivatePidlReference());
    cohi.progInfo.ppd = ppd;
    cohi.fFireChangeNotify = TRUE;

    cohi.progInfo.uliBytesCompleted.QuadPart = m_uliBytesCompleted.QuadPart;
    cohi.progInfo.uliBytesTotal.QuadPart = m_uliBytesTotal.QuadPart;

    hr = m_pfd->WithHint(NULL, m_hwnd, CopyCB, &cohi, NULL, m_pff);
    if (SUCCEEDED(hr))
    {
        m_uliBytesCompleted = cohi.progInfo.uliBytesCompleted;
        m_uliBytesTotal = cohi.progInfo.uliBytesTotal;
    }

    ILFree(cohi.pidlServer);
    return hr;
}


/****************************************************\
    FUNCTION: CDropOperation

    DESCRIPTION:
\****************************************************/
HRESULT CDropOperation::_DoCopyIteration()
{
    HRESULT hr = S_OK;
    LPCTSTR pszzFSSource = m_pszzFSSource;
    LPCTSTR pszzFtpDest = m_pszzFtpDest;

    m_ops = opsPrompt;
    while (S_OK == hr)
    {
        WCHAR szFSSource[MAX_PATH];
        WCHAR szFtpDest[MAX_PATH];

        hr = _EnumOneHdrop(&pszzFSSource, &pszzFtpDest, szFSSource, ARRAYSIZE(szFSSource), szFtpDest, ARRAYSIZE(szFtpDest));
        if (S_OK == hr)
        {
            szFSSource[lstrlenW(szFSSource)+1] = 0;   // Double terminate for SHFileOperation(Delete) in move case
            hr = _CopyOneHdrop(szFSSource, szFtpDest);
            if (EVAL(m_ppd))
                EVAL(SUCCEEDED(m_ppd->SetProgress64(m_uliBytesCompleted.QuadPart, m_uliBytesTotal.QuadPart)));

            // Did we fail to copy the file?
            if (FAILED(hr) && (HRESULT_FROM_WIN32(ERROR_CANCELLED) != hr))
            {
                if (!IsValidFtpAnsiFileName(szFSSource) || !IsValidFtpAnsiFileName(szFtpDest))
                    int nResult = DisplayWininetError(m_hwnd, TRUE, HRESULT_CODE(hr), IDS_FTPERR_TITLE_ERROR, IDS_FTPERR_INVALIDFTPNAME, IDS_FTPERR_WININET, MB_OK, m_ppd);
                else
                    int nResult = DisplayWininetError(m_hwnd, TRUE, HRESULT_CODE(hr), IDS_FTPERR_TITLE_ERROR, IDS_FTPERR_FILECOPY, IDS_FTPERR_WININET, MB_OK, m_ppd);
                hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
            }
            if (S_FALSE == hr)
            {
                // _CopyOneHdrop() returning S_FALSE means we hit the end of the iteration,
                // in this case, _ConfirmCopy() only meant to skip this one file, so
                // change to S_OK to continue with the rest of the files.
                hr = S_OK;
            }
        }
    }

    Str_SetPtr((LPTSTR *) &m_pszzFSSource, NULL);
    Str_SetPtr((LPTSTR *) &m_pszzFtpDest, NULL);

    return hr;
}


HRESULT CDropOperation::_ConfirmCopy(LPCWSTR pszLocal, LPCWSTR pszFtpName, BOOL * pfFireChangeNotify)
{
    return ConfirmCopy(pszLocal, pszFtpName, &m_ops, m_hwnd, m_pff, m_pfd, NULL, m_cobj, pfFireChangeNotify);
}


/*****************************************************************************\
    CopyCB

    Callback procedure that copies a single hdrop / map.
\*****************************************************************************/
HRESULT CDropOperation::CopyCB(HINTERNET hint, HINTPROCINFO * phpi, LPVOID pv, BOOL * pfReleaseHint)
{
    LPCOPYONEHDROPINFO pcohi = (LPCOPYONEHDROPINFO) pv;
    pcohi->progInfo.hint = hint;
    HRESULT hr;

    InternetSetStatusCallbackWrap(hint, TRUE, FtpProgressInternetStatusCB);
    hr = CopyFileSysItem(hint, phpi, pcohi);
    if (!pcohi->progInfo.hint)
        *pfReleaseHint = FALSE;     // We had to close hint to get the cancel.

    return hr;
}


HRESULT CDropOperation::_CopyOneHdrop(LPCWSTR pszFSSource, LPCWSTR pszFtpDest)
{
    HRESULT hr;
    BOOL fFireChangeNotify = TRUE;

    pszFtpDest = PathFindFileName(pszFtpDest);

    hr = _ConfirmCopy(pszFSSource, pszFtpDest, &fFireChangeNotify);
    if (S_OK == hr)
    {
        WCHAR wzTo[MAX_PATH];
        COPYONEHDROPINFO cohi = {0};

        cohi.pff = m_pff;
        cohi.pszFSSource = pszFSSource;
        cohi.pszFtpDest = pszFtpDest;
        cohi.pszDir = wzTo;
        cohi.dwOperation = COHDI_COPY_FILES;
        cohi.ops = m_ops;
        cohi.fIsRoot = TRUE;
        cohi.pmlc = &m_mlc;
        cohi.pidlServer = FtpCloneServerID(m_pff->GetPrivatePidlReference());
        cohi.fFireChangeNotify = fFireChangeNotify;
        cohi.progInfo.ppd = m_ppd;

        cohi.progInfo.uliBytesCompleted.QuadPart = m_uliBytesCompleted.QuadPart;
        cohi.progInfo.uliBytesTotal.QuadPart = m_uliBytesTotal.QuadPart;
        EVAL(SUCCEEDED(m_pfd->GetDisplayPath(wzTo, ARRAYSIZE(wzTo))));

        // TODO: have CopyCB also update the dialog.
        hr = m_pfd->WithHint(NULL, m_hwnd, CopyCB, &cohi, NULL, m_pff);

        if (SUCCEEDED(hr) && (m_de == DROPEFFECT_MOVE))
        {
            //  We delete the file with SHFileOperation to keep the
            //  disk free space statistics up to date.
            //
            //  BUGBUG -- If coming from a file name map, maybe it's
            //  being dragged from the recycle bin, in which case, doing
            //  an FO_DELETE will put it back in!
            SHFILEOPSTRUCT sfo = {0};
            
            sfo.hwnd = NULL,                // No HWND so NO UI.
            sfo.wFunc  = FO_DELETE;
            sfo.pFrom  = pszFSSource;       // Multiple files in list.
            sfo.fFlags = (FOF_SILENT | FOF_NOCONFIRMATION /*| FOF_MULTIDESTFILES*/);  // No HWND so NO UI.

            int nResult = SHFileOperation(&sfo);
            if (0 != nResult)
                TraceMsg(TF_ALWAYS, "In CDropOperation::_CopyOneHdrop() and caller wanted MOVE but we couldn't delete the files after the copy.");
        }
        m_uliBytesCompleted = cohi.progInfo.uliBytesCompleted;
        m_uliBytesTotal = cohi.progInfo.uliBytesTotal;
        m_ops = cohi.ops;
    }
    else
    {
        if (S_FALSE == hr)
        {
            // _CopyOneHdrop() returning S_FALSE means we hit the end of the iteration,
            // in this case, _ConfirmCopy() only meant to skip this one file, so
            // change to S_OK to continue with the rest of the files.
            hr = S_OK;
        }
    }

    return hr;
}


/*****************************************************************************
    FUNCTION: SetEffect

    DESCRIPTION:
        Set the appropriate drop effect feedback.

    In the absence of keyboard modifiers, use CTRL (copy), unless
    DROPEFFECT_COPY is not available, in which case we use SHIFT (move).

    If anything else is set, then panic out to DROPEFFECT_NONE.

    Note that we do *not* use g_cfPreferredDe.  The only things
    we support are DROPEFFECT_COPY and DROPEFFECT_MOVE, and we always prefer DROPEFFECT_COPY.

    BUGBUG -- ignoring g_cfPreferredDe messes up cut/paste, though.
 *****************************************************************************/

HRESULT CFtpDrop::SetEffect(DROPEFFECT * pde)
{
    DWORD de;            // Preferred drop effect

    // Don't even think about effects that we don't support
    *pde &= m_grfksAvail;

    switch (m_grfks & (MK_SHIFT | MK_CONTROL))
    {
    case 0:            // No modifier, use COPY if possible
        if (*pde & DROPEFFECT_COPY)
        {
    case MK_CONTROL:
            de = DROPEFFECT_COPY;
        }
        else
        {
    case MK_SHIFT:
            de = DROPEFFECT_MOVE;
        }
        break;

    default:
        de = 0;
        break;        // Cannot link
    }
    *pde &= de;

    TraceMsg(TF_FTPDRAGDROP, "CFtpDrop::SetEffect(DROPEFFECT=%08x) m_grfksAvail=%08x", *pde, m_grfksAvail);
    return S_OK;
}


BOOL CFtpDrop::_IsFTPOperationAllowed(IDataObject * pdto)
{
#ifdef FEATURE_FTP_TO_FTP_COPY
    BOOL fIsFTPOperationAllowed = TRUE;

    // There are a few things we don't allow.
    // Is the Drop FTP Location the same
    // folder that the dragged items are already in?
    if (0)
    {
        // TODO:
    }
    
    return fIsFTPOperationAllowed;
#else // FEATURE_FTP_TO_FTP_COPY

    // Disallow all FTP Operations
    return !_HasData(pdto, &g_dropTypes[DROP_FTP_PRIVATE]);
#endif // FEATURE_FTP_TO_FTP_COPY
}


/*****************************************************************************\
    GetEffectsAvail

    Look at the object to see what drop effects are available.

    If we have a file group descriptor or an HDROP,
    then file contents are available.  (We assume that if you have
    a FGD, then a Contents isn't far behind.)

    In a perfect world, we would also validate the contents of
    each file in the group descriptor, to ensure that the contents
    are droppable.  We skimp on that because it's too expensive.
\*****************************************************************************/
DWORD CFtpDrop::GetEffectsAvail(IDataObject * pdto)
{
    DWORD grfksAvail = 0;

    // Is this from an Ftp Shell Extension?
    if (_IsFTPOperationAllowed(pdto))
    {
        // No or it's allowed, then we will accept it.  We reject everything
        // else because we can't do Ftp1->Ftp2 copying without
        // using the local machine as a temp location. (Ftp1->Local->Ftp2)

        if (_HasData(pdto, &g_dropTypes[DROP_Hdrop]) ||
            _HasData(pdto, &g_dropTypes[DROP_FGDW]) ||
            _HasData(pdto, &g_dropTypes[DROP_FGDA]))
        {
            TraceMsg(TF_FTPDRAGDROP, "CFtpDrop::GetEffectsAvail() SUCCEEDED");
            grfksAvail = DROPEFFECT_COPY + DROPEFFECT_MOVE;
        }
        else
        {
            TraceMsg(TF_FTPDRAGDROP, "CFtpDrop::GetEffectsAvail() FAILED");
#ifdef DEBUG
            STGMEDIUM sm;
            HRESULT hres = pdto->GetData(&g_dropTypes[DROP_URL], &sm);
            if (SUCCEEDED(hres))
            {
                TraceMsg(TF_FTPDRAGDROP, "CFtpDrop::GetEffectsAvail(%08x) URL: %hs", pdto, GlobalLock(sm.hGlobal));
                GlobalUnlock(sm.hGlobal);
                ReleaseStgMedium(&sm);
            }
            else
            {
                TraceMsg(TF_FTPDRAGDROP, "CFtpDrop::GetEffectsAvail(%08x) No URL", pdto);
            }
#endif // DEBUG
        }
    }

    return grfksAvail;
}


/*****************************************************************************\
    GetEffect

    Return the drop effect to use.

    If this is a nondefault drag/drop, then put up a menu.  Else,
    just go with the default.

    m_de = default effect
    m_pde -> possible effects (and receives result)
\*****************************************************************************/
DROPEFFECT CFtpDrop::GetEffect(POINTL pt)
{
    TraceMsg(TF_FTPDRAGDROP, "CFtpDrop::GetEffect() m_de=%08x. m_grfks=%08x", m_de, m_grfks);

    if (m_de && (m_grfks & MK_RBUTTON))
    {
        HMENU hmenuMain = LoadMenu(g_hinst, MAKEINTRESOURCE(IDM_DROPCONTEXT));
        HMENU hmenu = GetSubMenu(hmenuMain, 0);
        DROPEFFECT de;

        ASSERT(*m_pde & m_de);
        SetMenuDefaultItem(hmenu, m_de, 0);
        if (!(*m_pde & DROPEFFECT_COPY))
            DeleteMenu(hmenu, DROPEFFECT_COPY, MF_BYCOMMAND);

        if (!(*m_pde & DROPEFFECT_MOVE))
            DeleteMenu(hmenu, DROPEFFECT_MOVE, MF_BYCOMMAND);

        // _UNOBVIOUS_:  Defview is incestuous with itself.
        // If the drop target originated from Shell32.dll, then
        // it leaves the image of the dropped object on the screen
        // while the menu is up, which is nice.  Otherwise, it removes
        // the image of the dropped object before the drop target
        // receives its IDropTarget::Drop.
        // Which means that outside shell extensions can't take
        // advantage of the "pretty drop UI" feature.

        // _UNOBVIOUS_:  Have to force foregroundness, else the input
        // gets screwed up.
        if (m_hwnd)
            SetForegroundWindow(m_hwnd);

        de = TrackPopupMenuEx(hmenu,
                      TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_VERTICAL |
                      TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y,
                      m_hwnd, 0);

        DestroyMenu(hmenuMain);
        m_de = de;
    }
    *m_pde = m_de;

    TraceMsg(TF_FTPDRAGDROP, "CFtpDrop::GetEffect(%08x) -> %08x", this, m_de);
    return m_de;
}


/****************************************************\
    FUNCTION: _StartBackgroundInteration

    DESCRIPTION:
\****************************************************/
HRESULT CFtpDrop::_StartBackgroundInteration(void)
{
    CDropOperation * pDropOperation;
    HRESULT hr = CDropOperation_Create(m_pff, m_hwnd, m_pszzFSSource, m_pszzFtpDest, &pDropOperation, m_de, m_ops, m_cobj);
    
    // Did it succeed?
    if (EVAL(SUCCEEDED(hr)))
    {
        // Yes, so NULL out m_pszzFSSource, m_pszzFtpDest because we gave them our copies.
        //  Ugly but allocation is uglier.
        m_pszzFSSource = NULL;
        m_pszzFtpDest = NULL;

        EVAL(SUCCEEDED(hr = pDropOperation->DoOperation(TRUE)));
        pDropOperation->Release();
    }

    return hr;
}


/****************************************************\
    FUNCTION: _DoCountIteration

    DESCRIPTION:
\****************************************************/
HRESULT CFtpDrop::_DoCountIteration(void)
{
    HRESULT hr = S_OK;
    LPCTSTR pszzFSSource = m_pszzFSSource;
    LPCTSTR pszzFtpDest = m_pszzFtpDest;

    while (S_OK == hr)
    {
        TCHAR szFSSource[MAX_PATH];
        TCHAR szFtpDest[MAX_PATH];

        hr = _EnumOneHdrop(&pszzFSSource, &pszzFtpDest, szFSSource, ARRAYSIZE(szFSSource), szFtpDest, ARRAYSIZE(szFtpDest));
        if (S_OK == hr)
            m_cobj++;
    }

    if (hr == S_FALSE)
        hr = S_OK;        // Enumerated to completion

    return hr;
}


/****************************************************\
    FUNCTION: _GetFSSourcePaths

    DESCRIPTION:
\****************************************************/
HRESULT CFtpDrop::_GetFSSourcePaths(HGLOBAL hdrop, BOOL * pfAnsi)
{
    LPDROPFILES pdrop = (LPDROPFILES) GlobalLock(hdrop);
    HRESULT hr = E_INVALIDARG;

    *pfAnsi = TRUE;
    if (EVAL(pdrop))
    {
        //  Now to decide whether it is an old-style drop or a new-style
        // drop.  And if it's a new-style drop, to get the character set.
        if (LOWORD(pdrop->pFiles) == sizeof(DROPFILES16))
        {
            // Old style
            Str_StrAndThunkA((LPTSTR *) &m_pszzFSSource, (LPCSTR) pvByteIndexCb(pdrop, LOWORD(pdrop->pFiles)), TRUE);
        }
        else
        {
            if (pdrop->fWide)
            {
                Str_StrAndThunkW((LPTSTR *) &m_pszzFSSource, (LPCWSTR) pvByteIndexCb(pdrop, pdrop->pFiles), TRUE);
                *pfAnsi = FALSE;
            }
            else
                Str_StrAndThunkA((LPTSTR *) &m_pszzFSSource, (LPCSTR) pvByteIndexCb(pdrop, pdrop->pFiles), TRUE);
        }
        GlobalUnlock(pdrop);
        hr = S_OK;
    }

    return hr;
}


/****************************************************\
    FUNCTION: _GetFtpDestPaths

    DESCRIPTION:
\****************************************************/
HRESULT CFtpDrop::_GetFtpDestPaths(HGLOBAL hmap, BOOL fAnsi)
{
    HRESULT hr = E_INVALIDARG;
    LPVOID pmap = NULL;

    //  If we can't get a map, then just use the source file names.
    ASSERT(!m_pszzFtpDest);
    if (hmap)
    {
        pmap = GlobalLock(hmap);

        if (pmap)
        {
            if (fAnsi)
                Str_StrAndThunkA((LPTSTR *) &m_pszzFtpDest, (LPCSTR) pmap, TRUE);
            else
                Str_StrAndThunkW((LPTSTR *) &m_pszzFtpDest, (LPCWSTR) pmap, TRUE);

            GlobalUnlock(pmap);
        }
    }

    if (!m_pszzFtpDest)
    {
        // Just copy the Paths
        Str_StrAndThunk((LPTSTR *) &m_pszzFtpDest, m_pszzFSSource, TRUE);
    }

    if (m_pszzFtpDest)
        hr = S_OK;

    return hr;
}



/*****************************************************************************\
    CopyHdrop

    Copy an HDROP data object.

    Note also that when we use HDROP, we must also consult the
    FileNameMap otherwise dragging out of the recycle bin directly
    into an FTP folder will create files with the wrong name!

    Note further that the returned effect of an HDROP is always
    DROPEFFECT_COPY, because we will do the work of deleting the
    source files when finished.
\*****************************************************************************/
HRESULT CFtpDrop::CopyHdrop(IDataObject * pdto, STGMEDIUM *psm)
{
    BOOL fAnsi;
    HRESULT hr = _GetFSSourcePaths(psm->hGlobal, &fAnsi);

    if (EVAL(SUCCEEDED(hr)))
    {
        STGMEDIUM sm;

        // ZIP fails this.
        // Get the File name map, too, if one exists
        if (fAnsi)
            hr = pdto->GetData(&g_dropTypes[DROP_FNMA], &sm);
        else
            hr = pdto->GetData(&g_dropTypes[DROP_FNMW], &sm);

        if (FAILED(hr))       // Failure is ok
            sm.hGlobal = 0;

        hr = _GetFtpDestPaths(sm.hGlobal, fAnsi);
        if (EVAL(SUCCEEDED(hr)))
        {
            *m_pde = DROPEFFECT_COPY;
            // Count up how many things there are in the hdrop,
            // so that our confirmation dialog knows what the deal is.
            // We can ignore the error; it'll show up again when we copy.
            m_cobj = 0;
            hr = _DoCountIteration();
            ASSERT(SUCCEEDED(hr));
            TraceMsg(TF_FTPDRAGDROP, "CFtpDrop_CopyHdrop: %d file(s)", m_cobj);

            //  Now walk the lists with the appropriate enumerator.
            hr = _StartBackgroundInteration();
            ASSERT(SUCCEEDED(hr));
        }
        if (sm.hGlobal)
            ReleaseStgMedium(&sm);
    }

    return hr;
}


/*****************************************************************************\
    _CopyHglobal

    Copy a file contents received as an hglobal.

    If a FD_FILESIZE is provided, use it.  Otherwise, just use the size
    of the hglobal.
\*****************************************************************************/
HRESULT CFtpDrop::_CopyHglobal(IStream * pstm, DWORD dwFlags, DWORD dwFileSizeHigh, DWORD dwFileSizeLow, LPVOID pvSrc, ULARGE_INTEGER *pqw)
{
    LPVOID pv;
    HGLOBAL hglob = pvSrc;
    HRESULT hres;

    pqw->HighPart = 0;
    pv = GlobalLock(hglob);
    if (EVAL(pv))
    {
        UINT cb = (UINT) GlobalSize(hglob);
        if (dwFlags & FD_FILESIZE)
        {
            if (cb > dwFileSizeLow)
                cb = dwFileSizeHigh;
        }
        hres = pstm->Write(pv, cb, &pqw->LowPart);
        if (SUCCEEDED(hres))
        {
            if (pqw->LowPart != cb)
                hres = STG_E_MEDIUMFULL;
        }
        GlobalUnlock(pv);
    }
    else
        hres = E_INVALIDARG;

    return hres;
}


/*****************************************************************************
    FUNCTION: _GetRelativePidl

    DESCRIPTION:
        pszFullPath may come in this format: "dir1\dir2\dir3\file.txt".  We
    need to create *ppidl such that it will contain 4 itemIDs in this case and
    the last one (file.txt) will have the correct attributes and file size.
\*****************************************************************************/
CFtpDir * CFtpDrop::_GetRelativePidl(LPCWSTR pszFullPath, DWORD dwFileAttributes, DWORD dwFileSizeHigh, DWORD dwFileSizeLow, LPITEMIDLIST * ppidl)
{
    HRESULT hr = S_OK;
    WCHAR szFullPath[MAX_PATH];
    LPWSTR pszFileName;
    LPITEMIDLIST pidlFull;
    CFtpDir * pfd = m_pfd;  // Assume the Dir to create isn't in a subdir.

    // Find the File Name
    StrCpyNW(szFullPath, pszFullPath, ARRAYSIZE(szFullPath));   // Make a copy because the caller's is read only.
    pszFileName = PathFindFileName(szFullPath);                 // Find where the file begins.
    FilePathToUrlPathW(szFullPath);                             // Convert from "dir1\dir2\file.txt" to "dir1/dir2/file.txt"

    *ppidl = NULL;
    hr = CreateFtpPidlFromDisplayPath(szFullPath, m_pff->GetCWireEncoding(), NULL, &pidlFull, TRUE, FALSE);
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidlFile = ILFindLastID(pidlFull);
        SYSTEMTIME st;
        FILETIME ft;

        GetSystemTime(&st);
        SystemTimeToFileTime(&st, &ft);
        FtpPidl_SetAttributes(pidlFile, dwFileAttributes);
        FtpPidl_SetFileSize(pidlFile, dwFileSizeHigh, dwFileSizeLow);
        FtpItemID_SetFileTime(pidlFile, ft);

        // Is the file in a subdir?
        if (!ILIsEmpty(pidlFull) && !ILIsEmpty(_ILNext(pidlFull)))
        {
            // Yes, so generate a CFtpDir to the subdir.
            LPITEMIDLIST pidlPath = ILClone(pidlFull);

            if (pidlPath)
            {
                ILRemoveLastID(pidlPath);
                pfd = m_pfd->GetSubFtpDir(m_pff, pidlPath, FALSE);
                ILFree(pidlPath);
            }
        }

        if (pfd)
            *ppidl = ILClone(pidlFile);
        ILFree(pidlFull);
    }

    return pfd;
}


/*****************************************************************************
    FUNCTION: CopyAsStream

    DESCRIPTION:
        Copy a file contents received as a <mumble> to a stream.
\*****************************************************************************/
HRESULT CFtpDrop::CopyAsStream(LPCWSTR pszName, DWORD dwFileAttributes, DWORD dwFlags, DWORD dwFileSizeHigh, DWORD dwFileSizeLow, STREAMCOPYPROC pfn, LPVOID pv)
{
    BOOL fFireChangeNotify;
    HRESULT hr = ConfirmCopy(pszName, pszName, &m_ops, m_hwnd, m_pff, m_pfd, m_pde, m_cobj, &fFireChangeNotify);

    if (EVAL(SUCCEEDED(hr)))
    {
        LPITEMIDLIST pidlRelative;
        CFtpDir * pfd = _GetRelativePidl(pszName, dwFileAttributes, dwFileSizeHigh, dwFileSizeLow, &pidlRelative);

        if (EVAL(pfd))
        {
            LPITEMIDLIST pidlFull = ILCombine(pfd->GetPidlReference(), pidlRelative);

            if (pidlFull)
            {
                IStream * pstm;
                ULARGE_INTEGER uliTemp = {0};

                hr = CFtpStm_Create(pfd, pidlFull, GENERIC_WRITE, &pstm, uliTemp, uliTemp, NULL, FALSE);
                if (SUCCEEDED(hr))
                {
                    ULARGE_INTEGER uli = {dwFileSizeLow, dwFileSizeHigh};

                    hr = pfn(pstm, dwFlags, dwFileSizeHigh, dwFileSizeLow, pv, &uli);
                    if (SUCCEEDED(hr))
                    {
                        // Only fire change notify if we didn't replace a file on 
                        // browser only. (Because we hack the defview and it doesn't
                        // check for duplicates)
                        if (fFireChangeNotify)
                        {
                            FtpPidl_SetFileSize(pidlRelative, uli.HighPart, uli.LowPart);

                            // This time date stamp may be incorrect.
                            FtpChangeNotify(m_hwnd, SHCNE_CREATE, m_pff, pfd, pidlRelative, NULL, TRUE);
                        }
                    }
                    else
                    {
                        ASSERT(0);      // BUGBUG - Is there an orphaned file we need to delete?
                        DisplayWininetError(m_hwnd, TRUE, HRESULT_CODE(hr), IDS_FTPERR_TITLE_ERROR, IDS_FTPERR_DROPFAIL, IDS_FTPERR_WININET, MB_OK, NULL);
                        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                    }

                    pstm->Release();
                }
                else
                {
                    DisplayWininetError(m_hwnd, TRUE, HRESULT_CODE(hr), IDS_FTPERR_TITLE_ERROR, IDS_FTPERR_DROPFAIL, IDS_FTPERR_WININET, MB_OK, NULL);
                    hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                }

                ILFree(pidlFull);
            }
            else
                hr = E_OUTOFMEMORY;

            if (pfd != m_pfd)
                pfd->Release();

            ILFree(pidlRelative);
        }
        else
            hr = E_FAIL;
    }
    else
    {
        // BUGBUG -- need to stat the file to generate a local WFDA
        // BUGBUG -- check the return value and do something
        ASSERT(0);      // Handle appropriately.
    }

    return hr;
}


/*****************************************************************************\
    CopyStream

    Copy a file contents received as a stream.
    We ignore the file size in the fgd.
\*****************************************************************************/
HRESULT CFtpDrop::CopyStream(IStream * pstm, DWORD dwFlags, DWORD dwFileSizeHigh, DWORD dwFileSizeLow, LPVOID pvSrc, ULARGE_INTEGER *pqw)
{
    IStream * pstmSrc = (IStream *) pvSrc;
    ULARGE_INTEGER qwMax = {0xFFFFFFFF, 0xFFFFFFFF};
    HRESULT hres;

    hres = pstmSrc->CopyTo(pstm, qwMax, 0, pqw);
    ASSERT(SUCCEEDED(hres));

    return hres;
}


/*****************************************************************************\
    FUNCTION: CFtpDrop::CopyStorage

    DESCRIPTION:
        Copy a file contents provided as an IStorage.  Gack.
    We have to do this only because Exchange is a moron.

    Since there is no way to tell OLE to create a .doc file
    into an existing stream, we need to create the .doc file
    on disk, and then copy the file into the stream, then delete
    the .doc file.

    Note that CDropOperation::DoOperation() (_CopyOneHdrop) will do the ConfirmCopy
    and the FtpDropNotifyCreate(), too!  However, we want to fake
    it out and fool it into thinking we are doing a DROPEFFECT_COPY,
    so that it doesn't delete the "source" file.  *We* will delete
    the source file, because we created it.  (No need to tell the
    shell about disk size changes that don't affect it.)
\*****************************************************************************/
HRESULT CFtpDrop::CopyStorage(LPCWSTR pszFile, IStorage * pstgIn)
{
    IStorage * pstgOut;
    HRESULT hr = StgCreateDocfile(0, (STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_CREATE), 0, &pstgOut);

    if (EVAL(SUCCEEDED(hr)))
    {
        STATSTG stat;
        hr = pstgOut->Stat(&stat, STATFLAG_DEFAULT);
        if (EVAL(SUCCEEDED(hr)))
        {
            TCHAR szFSSource[MAX_PATH+3];
            TCHAR szFtpDest[MAX_PATH+3];

            SHUnicodeToTChar(stat.pwcsName, szFSSource, ARRAYSIZE(szFSSource));
            StrCpyN(szFtpDest, pszFile, ARRAYSIZE(szFtpDest));
            szFSSource[lstrlen(szFSSource)+1] = 0;    // Add the termination of the list of strings.
            szFtpDest[lstrlen(szFtpDest)+1] = 0;    // Add the termination of the list of strings.

            hr = pstgIn->CopyTo(0, 0, 0, pstgOut);
            pstgOut->Commit(STGC_OVERWRITE);
            pstgOut->Release();     // Must release before copying
            pstgOut = NULL;
            if (EVAL(SUCCEEDED(hr)))
            {
                DROPEFFECT deTrue = m_de;
                m_de = DROPEFFECT_COPY;
                CDropOperation * pDropOperation;
                hr = CDropOperation_Create(m_pff, m_hwnd, szFSSource, szFtpDest, &pDropOperation, m_de, m_ops, m_ops);
    
                // Did it succeed?
                if (EVAL(SUCCEEDED(hr)))
                {
                    // Do the operation asynchroniously because the caller may call
                    // this over an over.
                    EVAL(SUCCEEDED(hr = pDropOperation->DoOperation(FALSE)));
                    pDropOperation->Release();
                }

                // Did an error occure and no UI has been displayed yet?
                if (FAILED(hr) && (HRESULT_FROM_WIN32(ERROR_CANCELLED) != hr))
                {
                    DisplayWininetError(m_hwnd, TRUE, HRESULT_CODE(hr), IDS_FTPERR_TITLE_ERROR, IDS_FTPERR_FILECOPY, IDS_FTPERR_WININET, MB_OK, NULL);
                }

                m_de = deTrue;

                DeleteFile(szFSSource);
            }
            else
                DisplayWininetError(m_hwnd, TRUE, HRESULT_CODE(hr), IDS_FTPERR_TITLE_ERROR, IDS_FTPERR_DROPFAIL, IDS_FTPERR_WININET, MB_OK, NULL);

            SHFree(stat.pwcsName);
        }
        else
            DisplayWininetError(m_hwnd, TRUE, HRESULT_CODE(hr), IDS_FTPERR_TITLE_ERROR, IDS_FTPERR_DROPFAIL, IDS_FTPERR_WININET, MB_OK, NULL);
    }
    else
        DisplayWininetError(m_hwnd, TRUE, HRESULT_CODE(hr), IDS_FTPERR_TITLE_ERROR, IDS_FTPERR_DROPFAIL, IDS_FTPERR_WININET, MB_OK, NULL);

    return hr;
}


/*****************************************************************************\
    CopyFCont

    Copy a file contents.
\*****************************************************************************/
HRESULT CFtpDrop::CopyFCont(LPCWSTR pszName, DWORD dwFileAttributes, DWORD dwFlags, DWORD dwFileSizeHigh, DWORD dwFileSizeLow, STGMEDIUM *psm)
{
    HRESULT hres;

    switch (psm->tymed)
    {
    case TYMED_HGLOBAL:
        hres = CopyAsStream(pszName, dwFileAttributes, dwFlags, dwFileSizeHigh, dwFileSizeLow, _CopyHglobal, psm->hGlobal);
        break;

    case TYMED_ISTREAM:
        hres = CopyAsStream(pszName, dwFileAttributes, dwFlags, dwFileSizeHigh, dwFileSizeLow, CopyStream, psm->pstm);
        break;

    case TYMED_ISTORAGE:        // Stupid Exchange
        hres = CopyStorage(pszName, psm->pstg);
        break;

    default:
        ASSERT(0);
        // Shouldn't have gotten this - BUGBUG -- UI?
        hres = E_INVALIDARG;
        break;
    }

    return hres;
}


HRESULT CFtpDrop::_GetFileDescriptor(LONG nIndex, LPFILEGROUPDESCRIPTORW pfgdW, LPFILEGROUPDESCRIPTORA pfgdA, BOOL fUnicode, LPFILEDESCRIPTOR pfd)
{
    if (fUnicode)
    {
        LPFILEDESCRIPTORW pfdW = &pfgdW->fgd[nIndex];
    
        CopyMemory(pfd, pfdW, (sizeof(*pfdW) - sizeof(pfdW->cFileName)));   // Copy Everything except the name.
        SHUnicodeToTChar(pfdW->cFileName, pfd->cFileName, ARRAYSIZE(pfd->cFileName));
    }
    else
    {
        LPFILEDESCRIPTORA pfdA = &pfgdA->fgd[nIndex];
        
        CopyMemory(pfd, pfdA, (sizeof(*pfdA) - sizeof(pfdA->cFileName)));   // Copy Everything except the name.
        SHAnsiToTChar(pfdA->cFileName, pfd->cFileName, ARRAYSIZE(pfd->cFileName));
    }

    return S_OK;
}


HRESULT CFtpDrop::_CreateFGDDirectory(LPFILEDESCRIPTOR pFileDesc)
{
    HRESULT hr = S_OK;
    WCHAR szDirName[MAX_PATH];
    LPTSTR pszDirToCreate = PathFindFileName(pFileDesc->cFileName);
    FTPCREATEFOLDERSTRUCT fcfs = {szDirName, m_pff};
    CFtpDir * pfd = m_pfd;  // Assume the Dir to create isn't in a subdir.

    SHTCharToUnicode(pszDirToCreate, szDirName, ARRAYSIZE(szDirName));
    pszDirToCreate[0] = 0;  // Separate Dir to create from SubDir where to create it.

    // Is the dir to create in subdir?
    if (pFileDesc->cFileName[0])
    {
        // Yes, so let's get that CFtpDir pointer so WithHint below will get us there.
        LPITEMIDLIST pidlPath;
        
        FilePathToUrlPathW(pFileDesc->cFileName);
        hr = CreateFtpPidlFromDisplayPath(pFileDesc->cFileName, m_pff->GetCWireEncoding(), NULL, &pidlPath, TRUE, TRUE);
        if (SUCCEEDED(hr))
        {
            pfd = m_pfd->GetSubFtpDir(m_pff, pidlPath, FALSE);
            ILFree(pidlPath);
        }
    }
    
    if (SUCCEEDED(hr))
    {
        hr = pfd->WithHint(NULL, m_hwnd, CreateNewFolderCB, (LPVOID) &fcfs, NULL, m_pff);
        if (SUCCEEDED(hr))
        {
        }
        else
        {
            // TODO: Display error UI?
        }
    }

    if (m_pfd != pfd)
    {
        // We allocated pfd, so now let's free it.
        pfd->Release();
    }

    return hr;
}


/*****************************************************************************\
    CopyFGD

    Copy a file group descriptor.

    File group descriptors are used to source gizmos that are file-like
    but aren't stored on disk as such.  E.g., an embedded file in a
    mail message, a GIF image in a web page, an OLE scrap, or a file
    on a remote FTP site.

    _UNOBVIOUS_:  If you do a GetData on TYMED_HGLOBAL | TYMED_ISTREAM,
    Exchange will nonetheless give you a TYMED_ISTORAGE even though
    you didn't ask for it.  So we need to support IStorage in order
    to make Exchange look less broken.  (Maybe I shouldn't cover for
    them.  Or maybe I should send them a bill.)
\*****************************************************************************/
HRESULT CFtpDrop::CopyFGD(IDataObject * pdto, STGMEDIUM *psm, BOOL fUnicode)
{
    LPFILEGROUPDESCRIPTORA pfgdA = NULL;
    LPFILEGROUPDESCRIPTORW pfgdW = NULL;
    HRESULT hr = E_INVALIDARG;

    // WARNING:
    //      shell32.dll from Win95, WinNT 4, IE 3, IE 4, and IE 4.01 have
    //      a bug that cause recursive file download not to work for
    //      subdirectories on WinNT unless we implement FILEGROUPDESCRIPTORW.

    if (fUnicode)
        pfgdW = (LPFILEGROUPDESCRIPTORW) GlobalLock((LPFILEGROUPDESCRIPTORW *) psm->hGlobal);
    else
        pfgdA = (LPFILEGROUPDESCRIPTORA) GlobalLock((FILEGROUPDESCRIPTORA *) psm->hGlobal);

    if (EVAL(pfgdA || pfgdW))
    {
        FORMATETC fe = {g_dropTypes[DROP_FCont].cfFormat, 0, DVASPECT_CONTENT, 0, (TYMED_ISTREAM | TYMED_HGLOBAL | TYMED_ISTORAGE)};
        
        // Stupid Exchange
        DWORD dwSize = m_cobj = (pfgdW ? pfgdW->cItems : pfgdA->cItems);

        TraceMsg(TF_FTPDRAGDROP, "CFtpDrop::CopyFGD: %d files", m_cobj);
        hr = S_OK;        // Watch out for vacuous null case

        for (; ((UINT)fe.lindex < dwSize); fe.lindex++)
        {
            FILEDESCRIPTOR fileDescriptor = {0};

            if (EVAL(SUCCEEDED(_GetFileDescriptor(fe.lindex, pfgdW, pfgdA, fUnicode, &fileDescriptor))))
            {
                // Is this a folder?
                if ((FD_ATTRIBUTES & fileDescriptor.dwFlags) &&
                    FILE_ATTRIBUTE_DIRECTORY & fileDescriptor.dwFileAttributes)
                {
                    // Yes, so let's create it.  We currently don't copy folder
                    // info. (ACLs or other attributes)
                    hr = _CreateFGDDirectory(&fileDescriptor);
                }
                else
                {
                    // No, so it's a file.  Let's get the stream and then upload that to the FTP server.
                    STGMEDIUM sm;
                    
                    hr = pdto->GetData(&fe, &sm);
                    if (SUCCEEDED(hr))
                    {

                        hr = CopyFCont(fileDescriptor.cFileName, fileDescriptor.dwFileAttributes, fileDescriptor.dwFlags, fileDescriptor.nFileSizeHigh, fileDescriptor.nFileSizeLow, &sm);
                        ReleaseStgMedium(&sm);
                        if (FAILED(hr))
                        {
                            break;
                        }
                    }
                    else
                    {
                        ASSERT(0);
                        break;
                    }
                }
            }
        }

        if (pfgdW)
            GlobalUnlock(pfgdW);
        if (pfgdA)
            GlobalUnlock(pfgdA);
    }

    return hr;
}


/*****************************************************************************\
    _Copy

    Copy the data object into the shell folder.

    HDROPs are preferred, because we can use FtpPutFile to shove
    them onto the FTP site without getting our hands dirty.

    Failing that, we use FileGroupDescriptor, which lets us
    get at pseudo-files.

    Note also that if you use HDROP, you need to support FileNameMap
    otherwise dragging out of the recycle bin directly into an FTP
    folder will create files with the wrong name!

    BUGBUG -- Ask FrancisH how to handle the multi-drag case + move.
    If a single file is cancelled, should I return DROPEFFECT_NONE?
\*****************************************************************************/
HRESULT CFtpDrop::_Copy(IDataObject * pdto)
{
    STGMEDIUM sm;
    HRESULT hr;

    if (SUCCEEDED(hr = pdto->GetData(&g_dropTypes[DROP_Hdrop], &sm)))
    {
        hr = CopyHdrop(pdto, &sm);
        ReleaseStgMedium(&sm);
    }
    else
    {
        BOOL fSupportsUnicode = SUCCEEDED(hr = pdto->GetData(&g_dropTypes[DROP_FGDW], &sm));

        if (fSupportsUnicode || EVAL(SUCCEEDED(hr = pdto->GetData(&g_dropTypes[DROP_FGDA], &sm))))
        {
            hr = CopyFGD(pdto, &sm, fSupportsUnicode);
            ReleaseStgMedium(&sm);
        }
    }

    // Normally we would set the PASTESUCCEEDED info back into 
    // the IDataObject but we don't because we do an optimized
    // MOVE by doing a DELETE after the COPY operation.
    // We do this because we do the operation on a background thread
    // in order to be asynch and we don't want to extend the lifetime
    // of the IDataObject that long.  Therefore we push
    // DROPEFFECT_COPY back into the caller to tell them that
    // we did an optimized move and to not delete the items.
    //
    // TODO: We need to test the CopyFGD() code above and
    //       maybe use PasteSucceeded(DROPEFFECT_MOVE) in
    //       that case.
    if (SUCCEEDED(hr) && (m_de == DROPEFFECT_MOVE))
    {
        // Always set "Copy" because we did an optimized move
        // because we deleted the files our selfs.
        DataObj_SetPasteSucceeded(pdto, DROPEFFECT_COPY);
    }

    return hr;
}

//===========================
// *** IDropTarget Interface ***
//===========================

/*****************************************************************************

    IDropTarget::DragEnter

 *****************************************************************************/
HRESULT CFtpDrop::DragEnter(IDataObject * pdto, DWORD grfKeyState, POINTL pt, DROPEFFECT * pde)
{
    HRESULT hr;

    m_grfks = grfKeyState;    // Remember last key state
    m_grfksAvail = GetEffectsAvail(pdto);

    hr = SetEffect(pde);
    ASSERT(SUCCEEDED(hr));

    TraceMsg(TF_FTPDRAGDROP, "CFtpDrop::DragEnter(grfKeyState=%08x, DROPEFFECT=%08x) m_grfks=%08x. m_grfksAvail=%08x hres=%#08lx", grfKeyState, *pde, m_grfks, m_grfksAvail, hr);

    return hr;
}

/*****************************************************************************

    IDropTarget::DragOver

 *****************************************************************************/

HRESULT CFtpDrop::DragOver(DWORD grfKeyState, POINTL pt, DROPEFFECT * pde)
{
    HRESULT hr;

    m_grfks = grfKeyState;    // Remember last key state
    hr = SetEffect(pde);
    ASSERT(SUCCEEDED(hr));

    TraceMsg(TF_FTPDRAGDROP, "CFtpDrop::DragOver(grfKeyState=%08x, DROPEFFECT=%08x) m_grfks=%08x. SetEffect() returned hres=%#08lx", grfKeyState, *pde, m_grfks, hr);

    return hr;
}


/*****************************************************************************

    IDropTarget::DragLeave

 *****************************************************************************/

HRESULT CFtpDrop::DragLeave(void)
{
    TraceMsg(TF_FTPDRAGDROP, "CFtpDrop::DragLeave() ");

    return S_OK;
}


/*****************************************************************************\
    IDropTarget::Drop 

    Note that the incoming pdto is not necessarily the same as the
    one we saw on DragEnter.  OLE will first give us a "preliminary"
    data object to play with, but on the drop, it will give us a
    fully marshalled object.

    Fortunately, we don't care, because we didn't cache the object.

    Note that we don't pass the real pde to SetEffect, because
    we don't want to lose the list of all possible effects before
    GetEffect uses it.
\*****************************************************************************/
HRESULT CFtpDrop::Drop(IDataObject * pdo, DWORD grfKeyState, POINTL pt, DROPEFFECT * pde)
{
    HRESULT hr;

    m_ops = opsPrompt;        // Start out in prompt mode
    m_grfksAvail = GetEffectsAvail(pdo);
    
    m_pde = pde;
    m_de = *pde;

    hr = SetEffect(&m_de);
    TraceMsg(TF_FTPDRAGDROP, "CFtpDrop::Drop(grfKeyState=%08x, DROPEFFECT=%08x) m_grfksAvail=%08x. m_de=%08x. SetEffect() returned hres=%#08lx", grfKeyState, *pde, m_grfksAvail, m_de, hr);

    if (EVAL(SUCCEEDED(hr)))
    {
        if (GetEffect(pt))
        {
            hr = _Copy(pdo);
        }
        else
            hr = S_FALSE;   // Indicate cancel.
    }

    if (!(SUCCEEDED(hr)))
    {
        // Error message already has been displayed.
        *pde = 0;
    }

    return hr;
}


/*****************************************************************************\
    CFtpDrop_Create
\*****************************************************************************/
HRESULT CFtpDrop_Create(CFtpFolder * pff, HWND hwnd, CFtpDrop ** ppfdt)
{
    HRESULT hres = E_OUTOFMEMORY;
    CFtpDrop * pfdt = new CFtpDrop();
    *ppfdt = pfdt;

    if (EVAL(pfdt))
    {
        pfdt->m_hwnd = hwnd;

        // Copy the CFtpFolder * value
        pfdt->m_pff = pff;
        if (pff)
            pff->AddRef();

        // Copy the CFtpDir * value
        ASSERT(!pfdt->m_pfd);
        pfdt->m_pfd = pff->GetFtpDir();
        hres = pfdt->m_pfd ? S_OK : E_FAIL;
        if (FAILED(hres))   // Will fail if the caller is CFtpMenu::_RemoveContextMenuItems() and it's OK.
            ATOMICRELEASE(*ppfdt);
    }

    ASSERT_POINTER_MATCHES_HRESULT(*ppfdt, hres);
    return hres;
}


/****************************************************\
    Constructor
\****************************************************/
CFtpDrop::CFtpDrop() : m_cRef(1)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!m_pff);
    ASSERT(!m_pfd);
    ASSERT(!m_hwnd);
    ASSERT(!m_grfks);
    ASSERT(!m_grfksAvail);
    ASSERT(!m_pde);
    ASSERT(!m_cobj);

    LEAK_ADDREF(LEAK_CFtpDrop);
}


/****************************************************\
    Destructor
\****************************************************/
CFtpDrop::~CFtpDrop()
{
    IUnknown_Set(&m_pff, NULL);
    IUnknown_Set(&m_pfd, NULL);

    DllRelease();
    LEAK_DELREF(LEAK_CFtpDrop);
}


//===========================
// *** IUnknown Interface ***
//===========================

ULONG CFtpDrop::AddRef()
{
    m_cRef++;
    return m_cRef;
}

ULONG CFtpDrop::Release()
{
    ASSERT(m_cRef > 0);
    m_cRef--;

    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}

HRESULT CFtpDrop::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDropTarget))
    {
        *ppvObj = SAFECAST(this, IDropTarget*);
    }
    else
    {
        TraceMsg(TF_FTPQI, "CFtpDrop::QueryInterface() failed.");
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}


