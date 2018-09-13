/*****************************************************************************\
    FILE: util.cpp
    
    DESCRIPTION:
        Shared stuff that operates on all classes.
\*****************************************************************************/

#include "priv.h"
#include "util.h"
#include "ftpurl.h"
#include "view.h"
#include "statusbr.h"
#include <commctrl.h>
#include <shdocvw.h>

HINSTANCE g_hinst;              /* My instance handle */
CHAR g_szShell32[MAX_PATH];     /* Full path to shell32.dll (must be ANSI) */

DWORD GetOSVer(void);

#ifdef DEBUG
DWORD g_TLSliStopWatchStartHi = 0;
DWORD g_TLSliStopWatchStartLo = 0;
LARGE_INTEGER g_liStopWatchFreq = {0};
#endif // DEBUG

// Shell32.dll v3 (original Win95/WinNT) has so many bugs when it receives
// an IDataObject with FILEGROUPDESCRIPTOR that it doesn't make sense to allow
// users to drag from FTP with FILEGROUPDESCRIPTOR on these early shell machines.
// This #define turns this on off.
//#define BROWSERONLY_DRAGGING        1

const VARIANT c_vaEmpty = {0};
#define PVAREMPTY ((VARIANT*)&c_vaEmpty)

//////////////////////////// IE 5 vs IE 4 /////////////////////////////////
// These are functions that IE5 exposes (normally in shlwapi), but
// if we want to be compatible with IE4, we need to have our own copy.s
// If we turn on USE_IE5_UTILS, we won't work with IE4's DLLs (like shlwapi).
//
#ifndef USE_IE5_UTILS
void UnicWrapper_IUnknown_Set(IUnknown ** ppunk, IUnknown * punk)
{
    ENTERCRITICAL;

    if (*ppunk)
        (*ppunk)->Release();

    *ppunk = punk;
    if (punk)
        punk->AddRef();

    LEAVECRITICAL;
}

void UnicWrapper_IUnknown_AtomicRelease(void ** ppunk)
{
    if (ppunk && *ppunk) {
        IUnknown* punk = *(IUnknown**)ppunk;
        *ppunk = NULL;
        punk->Release();
    }
}


DWORD UnicWrapper_SHWaitForSendMessageThread(HANDLE hThread, DWORD dwTimeout)
{
    MSG msg;
    DWORD dwRet;
    DWORD dwEnd = GetTickCount() + dwTimeout;

    // We will attempt to wait up to dwTimeout for the thread to
    // terminate
    do
    {
        dwRet = MsgWaitForMultipleObjects(1, &hThread, FALSE,
                dwTimeout, QS_SENDMESSAGE);
        if (dwRet == WAIT_OBJECT_0 ||
            dwRet == WAIT_FAILED)
        {
            // The thread must have exited, so we are happy
            break;
        }

        if (dwRet == WAIT_TIMEOUT)
        {
            // The thread is taking too long to finish, so just
            // return and let the caller kill it
            break;
        }

        // There must be a pending SendMessage from either the
        // thread we are killing or some other thread/process besides
        // this one.  Do a PeekMessage to process the pending
        // SendMessage and try waiting again
        PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);

        if (dwTimeout != INFINITE)
            dwTimeout = dwEnd - GetTickCount();
    }
    while((dwTimeout == INFINITE) || ((long)dwTimeout > 0));

    return(dwRet);
}


/****************************************************\
    FUNCTION: UnicWrapper_AutoCompleteFileSysInEditbox

    DESCRIPTION:
        This function will have AutoComplete take over
    an editbox to help autocomplete DOS paths.
\****************************************************/
HRESULT UnicWrapper_AutoCompleteFileSysInEditbox(HWND hwndEdit)
{
    HRESULT hr;
    IUnknown * punkACLISF;

    hr = CoCreateInstance(CLSID_ACListISF, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void **)&punkACLISF);
    if (EVAL(SUCCEEDED(hr)))
    {
        IAutoComplete * pac;

        // Create the AutoComplete Object
        hr = CoCreateInstance(CLSID_AutoComplete, NULL, CLSCTX_INPROC_SERVER, IID_IAutoComplete, (void **)&pac);
        if (EVAL(SUCCEEDED(hr)))
        {
            hr = pac->Init(hwndEdit, punkACLISF, NULL, NULL);
            pac->Release();
        }

        punkACLISF->Release();
    }

    return hr;
}


#endif // USE_IE5_UTILS
//////////////////////////// IE 5 vs IE 4 /////////////////////////////////


void IUnknown_Set(IMalloc ** ppm, IMalloc * pm)
{
    ENTERCRITICAL;

    if (*ppm)
        (*ppm)->Release();
    
    *ppm = pm;

    if (pm)
        pm->AddRef();

    LEAVECRITICAL;
}

// TODO: This is a remnent of using C++ in stead of real COM
void IUnknown_Set(CFtpFolder ** ppff, CFtpFolder * pff)
{
    ENTERCRITICAL;

    if (*ppff)
        (*ppff)->Release();
    
    *ppff = pff;

    if (pff)
        pff->AddRef();

    LEAVECRITICAL;
}

void IUnknown_Set(CFtpDir ** ppfd, CFtpDir * pfd)
{
    ENTERCRITICAL;

    if (*ppfd)
        (*ppfd)->Release();
    
    *ppfd = pfd;

    if (pfd)
        pfd->AddRef();

    LEAVECRITICAL;
}

void IUnknown_Set(CFtpSite ** ppfs, CFtpSite * pfs)
{
    ENTERCRITICAL;

    if (*ppfs)
        (*ppfs)->Release();
    
    *ppfs = pfs;

    if (pfs)
        pfs->AddRef();

    LEAVECRITICAL;
}

void IUnknown_Set(CFtpList ** ppfl, CFtpList * pfl)
{
    ENTERCRITICAL;

    if (*ppfl)
        (*ppfl)->Release();
    
    *ppfl = pfl;

    if (pfl)
        pfl->AddRef();

    LEAVECRITICAL;
}

void IUnknown_Set(CFtpPidlList ** ppflpidl, CFtpPidlList * pflpidl)
{
    ENTERCRITICAL;

    if (*ppflpidl)
        (*ppflpidl)->Release();
    
    *ppflpidl = pflpidl;

    if (pflpidl)
        pflpidl->AddRef();

    LEAVECRITICAL;
}

void IUnknown_Set(CFtpEfe ** ppfefe, CFtpEfe * pfefe)
{
    ENTERCRITICAL;

    if (*ppfefe)
        (*ppfefe)->Release();
    
    *ppfefe = pfefe;

    if (pfefe)
        pfefe->AddRef();

    LEAVECRITICAL;
}

void IUnknown_Set(CFtpGlob ** ppfg, CFtpGlob * pfg)
{
    ENTERCRITICAL;

    if (*ppfg)
        (*ppfg)->Release();
    
    *ppfg = pfg;

    if (pfg)
        pfg->AddRef();

    LEAVECRITICAL;
}


void IUnknown_Set(CFtpMenu ** ppfcm, CFtpMenu * pfcm)
{
    ENTERCRITICAL;

    if (*ppfcm)
        (*ppfcm)->Release();
    
    *ppfcm = pfcm;

    if (pfcm)
        pfcm->AddRef();

    LEAVECRITICAL;
}


void IUnknown_Set(CFtpStm ** ppfstm, CFtpStm * pfstm)
{
    ENTERCRITICAL;

    if (*ppfstm)
        (*ppfstm)->Release();
    
    *ppfstm = pfstm;

    if (pfstm)
        pfstm->AddRef();

    LEAVECRITICAL;
}


#undef ILCombine
// Fix Shell32 bug
LPITEMIDLIST ILCombineWrapper(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    if (!pidl1)
        return ILClone(pidl2);

    if (!pidl2)
        return ILClone(pidl1);

    return ILCombine(pidl1, pidl2);
}


#undef ILClone
// Fix Shell32 bug
LPITEMIDLIST ILCloneWrapper(LPCITEMIDLIST pidl)
{
    if (!pidl)
        return NULL;

    return ILClone(pidl);
}


#undef ILFree
// Fix Shell32 bug
void ILFreeWrapper(LPITEMIDLIST pidl)
{
    if (pidl)
        ILFree(pidl);
}


// BUGBUG: Don't ship with this on.
//#define DEBUG_LEGACY

BOOL IsLegacyChangeNotifyNeeded(LONG wEventId)
{
#ifdef DEBUG_LEGACY
    return TRUE;
#endif // DEBUG_LEGACY

    // The only version that doesn't support IDelegateFolder pidls is
    // shell32 v3 (w/o IE4 Shell Intergrated)
    BOOL fResult = (SHELL_VERSION_W95NT4 == GetShellVersion());
    
    return fResult;
}



/*****************************************************************************\
    FUNCTION: LegacyChangeNotify

    DESCRIPTION:
        Browser only can't read IDelegateFolder pidls (our Pidls), so we need
    to use this function instead of SHChangeNotify that will use hacks to
    get DefView's ListView to update by using 
    SHShellFolderView_Message(HWND hwnd, UINT uMsg, LPARAM lParam).

    These are the messages to use.
    SFVM_ADDOBJECT (SHCNE_CREATE & SHCNE_MKDIR),
    SFVM_UPDATEOBJECT (SHCNE_RENAMEFOLDER, SHCNE_RENAMEITEM, SHCNE_ATTRIBUTES), or SFVM_REFRESHOBJECT(),
    SFVM_REMOVEOBJECT (SHCNE_RMDIR & SHCNE_DELETE).
\*****************************************************************************/
HRESULT LegacyChangeNotify(HWND hwnd, LONG wEventId, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    if (EVAL(hwnd))       // We can't talk to the window w/o this.
    {
        switch(wEventId)
        {
        case SHCNE_CREATE:
        case SHCNE_MKDIR:
        {
            // BUGBUG: If the item alread exists, it will create a new duplicate name.
            //         We need to skip this if it exists.
            LPCITEMIDLIST pidlRelative = ILGetLastID(pidl1);
            // For some lame reason, SFVM_ADDOBJECT frees the pidl we give them.
            EVAL(SHShellFolderView_Message(hwnd, SFVM_ADDOBJECT, (LPARAM) ILClone(pidlRelative)));
            break;
        }
        case SHCNE_RMDIR:
        case SHCNE_DELETE:
        {
            LPCITEMIDLIST pidlRelative = ILGetLastID(pidl1);
            EVAL(SHShellFolderView_Message(hwnd, SFVM_REMOVEOBJECT, (LPARAM) pidlRelative));
            break;
        }
        case SHCNE_RENAMEFOLDER:
        case SHCNE_RENAMEITEM:
        case SHCNE_ATTRIBUTES:
        {
            LPCITEMIDLIST pidlArray[2];
            
            pidlArray[0] = ILGetLastID(pidl1);
            pidlArray[1] = ILClone(ILGetLastID(pidl2));
            EVAL(SHShellFolderView_Message(hwnd, SFVM_UPDATEOBJECT, (LPARAM) pidlArray));
            break;
        }
        }
    }

    return S_OK;
}


/*****************************************************************************\
    FUNCTION: FtpChangeNotify

    Convert the relative pidls into absolute pidls, then hand onwards
    to SHChangeNotify.  If we can't do the notification, tough.

    Issuing a change notify also invalidates the name-cache, because
    we know that something happened to the directory.

    If we wanted to be clever, we could edit the name-cache on the
    fly, but that would entail allocating a new name-cache, initializing
    it with the edited directory contents, then setting it as the new
    cache.  (We can't edit the name-cache in place because somebody
    might still be holding a reference to it.)  And all this work needs
    to be done under the critical section, so that nobody else tries
    to do the same thing simultaneously.  What's more, the only thing
    that this helps is the case where the user opens two views on
    the same folder from within the same process, which not a very
    common scenario.  Summary: It's just not worth it.

    Note that this must be done at the CFtpFolder level and not at the
    CFtpDir level, because CFtpDir doesn't know where we are rooted.
    (We might have several instances, each rooted at different places.)

    _UNDOCUMENTED_: The pidl1 and pidl2 parameters to SHChangeNotify
    are not documented.  It is also not mentioned (although it becomes
    obvious once you realize it) that the pidls passed to SHChangeNotify
    must be absolute.
\*****************************************************************************/
void FtpChangeNotify(HWND hwnd, LONG wEventId, CFtpFolder * pff, CFtpDir * pfd, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, BOOL fTopLevel)
{
    ASSERT(pfd && IsValidPIDL(pidl1));
    ASSERT(!pidl2 || IsValidPIDL(pidl2));

    // Update our local cache because SHChangeNotify will come back in later and
    // want to create a pidl from a DisplayName and will then use that pidls
    // time/date.  This is done because the shell is trying to create a 'full'
    // pidl.
    switch (wEventId)
    {
    case SHCNE_CREATE:
    case SHCNE_MKDIR:
        TraceMsg(TF_CHANGENOTIFY, ((wEventId == SHCNE_CREATE) ? "FtpChangeNotify(SHCNE_CREATE), Name=%ls" : "FtpChangeNotify(SHCNE_MKDIR), Name=%s"), FtpPidl_GetFileDisplayName(pidl1));
        EVAL(SUCCEEDED(pfd->AddItem(pidl1)));
        break;
    case SHCNE_RMDIR:
    case SHCNE_DELETE:
        TraceMsg(TF_CHANGENOTIFY, "FtpChangeNotify(SHCNE_DELETE), Name=%ls", FtpPidl_GetLastFileDisplayName(pidl1));
        pfd->DeletePidl(pidl1); // This may fail if we never populated that cache.
        break;
    case SHCNE_RENAMEFOLDER:
    {
        CFtpDir * pfdSubFolder = pfd->GetSubFtpDir(NULL, pidl1, TRUE);
        
        if (EVAL(pfdSubFolder))
        {
            LPITEMIDLIST pidlDest = pfd->GetSubPidl(NULL, pidl2, TRUE);

            if (EVAL(pidlDest))
            {
                EVAL(SUCCEEDED(pfdSubFolder->ChangeFolderName(pidlDest)));
                ILFree(pidlDest);
            }
            pfdSubFolder->Release();
        }
    }
    // break; Fall Thru so we change the pidl also.
    case SHCNE_RENAMEITEM:
    case SHCNE_ATTRIBUTES:
        TraceMsg(TF_CHANGENOTIFY, "FtpChangeNotify(SHCNE_RENAMEITEM), Name1=%ls, Name2=%ls", FtpPidl_GetLastFileDisplayName(pidl1), FtpPidl_GetLastFileDisplayName(pidl2));
        EVAL(SUCCEEDED(pfd->ReplacePidl(pidl1, pidl2)));
        break;
    }

    pidl1 = pfd->GetSubPidl(pff, pidl1, TRUE);
    if (EVAL(pidl1))
    {
        if ((pidl2 == NULL) || (EVAL(pidl2 = pfd->GetSubPidl(pff, pidl2, TRUE))) != 0)
        {
            // LRESULT SHShellFolderView_Message(HWND hwnd, UINT uMsg, LPARAM lParam)

            // Are we on something (browser only) that can't read
            // IDelegateFolder pidls (our Pidls)?
            if (IsLegacyChangeNotifyNeeded(wEventId))  // BUGBUG
            {
                // Yes, so SHChangeNotify won't work.  Use a work around.
                if (fTopLevel)  // Only top level changes are appropriate.
                    LegacyChangeNotify(hwnd, wEventId, pidl1, pidl2);
            }
            else
                SHChangeNotify(wEventId, (SHCNF_IDLIST | SHCNF_FLUSH), pidl1, pidl2);

            ILFree((LPITEMIDLIST)pidl2);
        }
        ILFree((LPITEMIDLIST)pidl1);
    }
}




/**************************************************************\
    FUNCTION: EscapeString

    DESCRIPTION:
\**************************************************************/
HRESULT EscapeString(LPCTSTR pszStrToEscape, LPTSTR pszEscapedStr, DWORD cchSize)
{
    LPCTSTR pszCopy = NULL;

    if (!pszStrToEscape)
    {
        Str_SetPtr((LPTSTR *) &pszCopy, pszEscapedStr);  // NULL pszStrToEscape means do pszEscapedStr in place.
        pszStrToEscape = pszCopy;
    }

    pszEscapedStr[0] = 0;
    if (pszStrToEscape[0])
        UrlEscape(pszStrToEscape, pszEscapedStr, &cchSize, URL_ESCAPE_SEGMENT_ONLY);

    Str_SetPtr((LPTSTR *) &pszCopy, NULL);  // NULL pszStrToEscape means do pszEscapedStr in place.
    return S_OK;
}


/**************************************************************\
    FUNCTION: UnEscapeString

    DESCRIPTION:
\**************************************************************/
HRESULT UnEscapeString(LPCTSTR pszStrToUnEscape, LPTSTR pszUnEscapedStr, DWORD cchSize)
{
    LPCTSTR pszCopy = NULL;

    if (!pszStrToUnEscape)
    {
        Str_SetPtr((LPTSTR *) &pszCopy, pszUnEscapedStr);  // NULL pszStrToEscape means do pszEscapedStr in place.
        pszStrToUnEscape = pszCopy;
    }

    pszUnEscapedStr[0] = 0;
    UrlUnescape((LPTSTR)pszStrToUnEscape, pszUnEscapedStr, &cchSize, URL_ESCAPE_SEGMENT_ONLY);
    
    Str_SetPtr((LPTSTR *) &pszCopy, NULL);  // NULL pszStrToEscape means do pszEscapedStr in place.
    return S_OK;
}


/**************************************************************\
    Since wininet errors are often very generic, this function
    will generate error message of this format:

    "An error occurred while attempted to do x and it could not
     be completed.
     
    Details:
    <Wininet error that may be specific or generic>"
\**************************************************************/
int DisplayWininetErrorEx(HWND hwnd, BOOL fAssertOnNULLHWND, DWORD dwError, UINT idTitleStr, UINT idBaseErrorStr, UINT idDetailsStr, UINT nMsgBoxType, IProgressDialog * ppd, LPCWSTR pwzDetails)
{
    TCHAR szErrMessage[MAX_PATH*3];
    TCHAR szTitle[MAX_PATH];
    BOOL fIsWininetError = ((dwError >= INTERNET_ERROR_BASE) && (dwError <= INTERNET_ERROR_LAST));
    HMODULE hmod = (fIsWininetError ? GetModuleHandle(TEXT("WININET")) : NULL);
    UINT uiType = (IDS_FTPERR_GETDIRLISTING == idBaseErrorStr) ? MB_ICONINFORMATION : MB_ICONERROR;
    
    if (ppd)
    {
        // If we have a progress dialog, we want to close it down
        // because we will display an error message and the progress
        // dialog in the background looks really dumb.
        ppd->StopProgressDialog();
    }

    // Default message if FormatMessage doesn't recognize hres
    LoadString(HINST_THISDLL, idBaseErrorStr, szErrMessage, ARRAYSIZE(szErrMessage));
    LoadString(HINST_THISDLL, idTitleStr, szTitle, ARRAYSIZE(szTitle));

    // Yes we did, so display the error.
    WCHAR szDetails[MAX_URL_STRING*2];
    TCHAR szPromptTemplate[MAX_PATH];
    TCHAR szBuffer[MAX_PATH*4];

    LoadString(HINST_THISDLL, idDetailsStr, szPromptTemplate, ARRAYSIZE(szPromptTemplate));

    // Can wininet give us extended error messages?
    // UNIX servers cancel the connection if the disk or quote is full
    // but the return a value that explains that to the user.
    if ((ERROR_INTERNET_EXTENDED_ERROR == dwError) || 
        (ERROR_INTERNET_CONNECTION_ABORTED == dwError))
    {
        if (!pwzDetails)
        {
            // BUGBUG/TODO: Strip the FTP Spec #s from the err msg.
            // StripResponseHeaders(pszMOTD);
            if (FAILED(InternetGetLastResponseInfoDisplayWrap(TRUE, &dwError, szDetails, ARRAYSIZE(szDetails))))
                szDetails[0] = 0;

            pwzDetails = (LPCWSTR) szDetails;
        }
    }
    else
    {
        if (fIsWininetError)
            FormatMessage(FORMAT_MESSAGE_FROM_HMODULE, (LPCVOID)hmod, dwError, 0, szDetails, ARRAYSIZE(szDetails), NULL);
        else
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, (LPCVOID)hmod, dwError, 0, szDetails, ARRAYSIZE(szDetails), NULL);

        pwzDetails = (LPCWSTR) szDetails;
    }

    wnsprintf(szBuffer, ARRAYSIZE(szBuffer), szPromptTemplate, pwzDetails);
    StrCatBuff(szErrMessage, szBuffer, ARRAYSIZE(szErrMessage));

    return MessageBox(hwnd, szErrMessage, szTitle, (uiType | nMsgBoxType));
}


int DisplayWininetError(HWND hwnd, BOOL fAssertOnNULLHWND, DWORD dwError, UINT idTitleStr, UINT idBaseErrorStr, UINT idDetailsStr, UINT nMsgBoxType, IProgressDialog * ppd)
{
    if (hwnd)   // Only display if HWND exists.
        return DisplayWininetErrorEx(hwnd, fAssertOnNULLHWND, dwError, idTitleStr, idBaseErrorStr, idDetailsStr, nMsgBoxType, ppd, NULL);
    else
    {
        if (fAssertOnNULLHWND)
        {
//            ASSERT(hwnd);
        }

        TraceMsg(TF_ALWAYS, "DisplayWininetError() no HWND so no Error.");
    }

    return IDCANCEL;
}

#define CCH_SIZE_ERROR_MESSAGE  6*1024
HRESULT FtpSafeCreateDirectory(HWND hwnd, HINTERNET hint, CMultiLanguageCache * pmlc, CFtpFolder * pff, CFtpDir * pfd, IProgressDialog * ppd, LPCWSTR pwzFtpPath, BOOL fRoot)
{
    FTP_FIND_DATA wfd;
    HRESULT hr = S_OK;
    WIRECHAR wFtpPath[MAX_PATH];
    CWireEncoding * pwe = pfd->GetFtpSite()->GetCWireEncoding();
    
    if (SUCCEEDED(pwe->UnicodeToWireBytes(NULL, pwzFtpPath, (pfd->IsUTF8Supported() ? WIREENC_USE_UTF8 : WIREENC_NONE), wFtpPath, ARRAYSIZE(wFtpPath))))
    {
        hr = FtpCreateDirectoryWrap(hint, TRUE, wFtpPath);

        // PERF NOTE: It is faster to just try to create the directory and then ignore
        //       error return values that indicate that they failed to create because it
        //       already exists.  The problem I worry about is that there is some FTP server
        //       impl somewhere that will return the same error as failed to create because
        //       of access violation and we don't or can't return an error value.
        if (FAILED(hr)
// BUGBUG: IE #30208: Currently broken in wininet.        
//         I want to test the attribute flags but for some reason the FILE_ATTRIBUTE_DIRECTORY bit
//         is also set for files!!!! (!@(*#!!!)
//          || !(FILE_ATTRIBUTE_DIRECTORY & wfd.dwFileAttributes)
                )
        {
            // Maybe if failed because it already exists, which is fine by me.

            // First save off the error msg in case we need it for the err dlg later.
            CHAR szErrorMsg[CCH_SIZE_ERROR_MESSAGE];
            WCHAR wzErrorMsg[CCH_SIZE_ERROR_MESSAGE];
            DWORD cchSize = ARRAYSIZE(szErrorMsg);
            InternetGetLastResponseInfoWrap(TRUE, NULL, szErrorMsg, &cchSize);
            HRESULT hrOrig = hr;

            pwe->WireBytesToUnicode(NULL, szErrorMsg, WIREENC_NONE, wzErrorMsg, ARRAYSIZE(wzErrorMsg));
            // Does it already exist?
            hr = FtpDoesFileExist(hint, TRUE, wFtpPath, &wfd, (INTERNET_NO_CALLBACK | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RESYNCHRONIZE | INTERNET_FLAG_RELOAD));

            // It's okay if we failed to create the directory because a -DIRECTORY- already exists
            // because we'll just use that directory.  However, it a file with that name exists, 
            // then we need the err msg.
            if ((S_OK != hr) || !(FILE_ATTRIBUTE_DIRECTORY & wfd.dwFileAttributes))
            {
                // No, so it was a real error, now display the error message with the original
                // server response.
                DisplayWininetErrorEx(hwnd, TRUE, HRESULT_CODE(hrOrig), IDS_FTPERR_TITLE_ERROR, IDS_FTPERR_DIRCOPY, IDS_FTPERR_WININET, MB_OK, ppd, wzErrorMsg);
                hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
            }
        }

        // Was it created successfully?
        if (SUCCEEDED(hr))
        {
            // Yes, so fire the change notify.
            LPITEMIDLIST pidlNewDir;
            FILETIME ftUTC;
            FTP_FIND_DATA wfd;

            GetSystemTimeAsFileTime(&ftUTC);   // UTC
            FileTimeToLocalFileTime(&ftUTC, &wfd.ftCreationTime);   // Need Local Time because FTP is stupid and won't work in the cross time zones case.

            // For some reason, FtpFindFirstFile needs an '*' behind the name.
            StrCpyNA(wfd.cFileName, wFtpPath, ARRAYSIZE(wfd.cFileName));
            wfd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
            wfd.ftLastWriteTime = wfd.ftCreationTime;
            wfd.ftLastAccessTime = wfd.ftCreationTime;
            wfd.nFileSizeLow = 0;
            wfd.nFileSizeHigh = 0;
            wfd.dwReserved0 = 0;
            wfd.dwReserved1 = 0;
            wfd.cAlternateFileName[0] = 0;

            hr = FtpItemID_CreateReal(&wfd, pwzFtpPath, &pidlNewDir);
            if (SUCCEEDED(hr))   // May happen on weird character set problems.
            {
                // Notify the folder of the new item so the Shell Folder updates.
                // PERF: Note that we should give SHChangeNotify() the information (time/date)
                //       from the local file system which may be different than on the server.
                //       But I don't think it's worth the perf to hit the server for the info.
                FtpChangeNotify(hwnd, SHCNE_MKDIR, pff, pfd, pidlNewDir, NULL, fRoot);
                ILFree(pidlNewDir);
            }
        }

    }

    return hr;
}


HWND GetProgressHWnd(IProgressDialog * ppd, HWND hwndDefault)
{
    if (ppd)
    {
        HWND hwndProgress = NULL;

        IUnknown_GetWindow(ppd, &hwndProgress);
        if (hwndProgress)
            hwndDefault = hwndProgress;
    }

    return hwndDefault;
}


// Returns FALSE if out of memory
int SHMessageBox(HWND hwnd, LPCTSTR pszMessage, UINT uMessageID, UINT uTitleID, UINT uType)
{
    int nResult = IDCANCEL;
    TCHAR szMessage[MAX_PATH];
    TCHAR szTitle[MAX_PATH];

    if (LoadString(HINST_THISDLL, uTitleID, szTitle, ARRAYSIZE(szTitle)) &&
        (pszMessage || 
         (uMessageID && LoadString(HINST_THISDLL, uMessageID, szMessage, ARRAYSIZE(szMessage)))))
    {
        nResult = MessageBox(hwnd, pszMessage ? pszMessage : szMessage, szTitle, uType);
    }

    return nResult;
}


DWORD GetOSVer(void)
{
    OSVERSIONINFOA osVerInfoA;

    osVerInfoA.dwOSVersionInfoSize = sizeof(osVerInfoA);
    if (!GetVersionExA(&osVerInfoA))
        return VER_PLATFORM_WIN32_WINDOWS;   // Default to this.
    
    return osVerInfoA.dwPlatformId;
}


LPITEMIDLIST SHILCreateFromPathWrapper(LPCTSTR pszPath)
{
    LPITEMIDLIST pidl;

    if (VER_PLATFORM_WIN32_NT == GetOSVer())
    {
        WCHAR wzPath[MAX_PATH];

        SHTCharToUnicode(pszPath, wzPath, ARRAYSIZE(wzPath));
        SHILCreateFromPath((LPCTSTR)wzPath, &pidl, NULL);
    }
    else
    {
        CHAR szPath[MAX_PATH];

        SHTCharToAnsi(pszPath, szPath, ARRAYSIZE(szPath));
        SHILCreateFromPath((LPCTSTR)szPath, &pidl, NULL);
    }

    return pidl;
}


LPCITEMIDLIST ILGetLastID(LPCITEMIDLIST pidlIn)
{
    LPITEMIDLIST pidl = (LPITEMIDLIST) pidlIn;

    while (!ILIsEmpty(_ILNext(pidl)))
        pidl = _ILNext(pidl);

    return pidl;
}


LPCITEMIDLIST ILGetLastNonFragID(LPCITEMIDLIST pidlIn)
{
    LPITEMIDLIST pidl = (LPITEMIDLIST) pidlIn;

    while (!ILIsEmpty(_ILNext(pidl)) && !FtpItemID_IsFragment(_ILNext(pidl)))
        pidl = _ILNext(pidl);

    return pidl;
}



SAFEARRAY * MakeSafeArrayFromData(LPCBYTE pData,DWORD cbData)
{
    SAFEARRAY * psa;

    if (!pData || 0 == cbData)
        return NULL;  // nothing to do

    // create a one-dimensional safe array
    psa = SafeArrayCreateVector(VT_UI1,0,cbData);
    ASSERT(psa);

    if (psa) {
        // copy data into the area in safe array reserved for data
        // Note we party directly on the pointer instead of using locking/
        // unlocking functions.  Since we just created this and no one
        // else could possibly know about it or be using it, this is OK.

        ASSERT(psa->pvData);
        memcpy(psa->pvData,pData,cbData);
    }

    return psa;
}


//
// PARAMETER:
//    pvar - Allocated by caller and filled in by this function.
//    pidl - Allocated by caller and caller needs to free.
//
// This function will take the PIDL parameter and COPY it
// into the Variant data structure.  This allows the pidl
// to be freed and the pvar to be used later, however, it
// is necessary to call VariantClear(pvar) to free memory
// that this function allocates.

BOOL InitVariantFromIDList(VARIANT* pvar, LPCITEMIDLIST pidl)
{
    UINT cb = ILGetSize(pidl);
    SAFEARRAY* psa = MakeSafeArrayFromData((LPCBYTE)pidl, cb);
    if (psa) {
        ASSERT(psa->cDims == 1);
        // ASSERT(psa->cbElements == cb);
        ASSERT(ILGetSize((LPCITEMIDLIST)psa->pvData)==cb);
        VariantInit(pvar);
        pvar->vt = VT_ARRAY|VT_UI1;
        pvar->parray = psa;
        return TRUE;
    }

    return FALSE;
}



BSTR BStrFromStr(LPCTSTR pszStr)
{
    BSTR bStr = NULL;

#ifdef UNICODE
    bStr = SysAllocString(pszStr);

#else // UNICODE
    DWORD cchSize = (lstrlen(pszStr) + 2);
    bStr = SysAllocStringLen(NULL, cchSize);
    if (EVAL(bStr))
        SHAnsiToUnicode(pszStr, bStr, cchSize);

#endif // UNICODE

    return bStr;
}


HRESULT IUnknown_IWebBrowserNavigate2(IUnknown * punk, LPCITEMIDLIST pidl, BOOL fHistoryEntry)
{
    HRESULT hr = E_FAIL;
    IWebBrowser2 * pwb2;

    // punk will be NULL on Browser Only installs because the old
    // shell32 doesn't do ::SetSite().
    IUnknown_QueryService(punk, SID_SWebBrowserApp, IID_IWebBrowser2, (LPVOID *) &pwb2);
    if (pwb2)
    {
        VARIANT varThePidl;

        if (InitVariantFromIDList(&varThePidl, pidl))
        {
            VARIANT varFlags;
            VARIANT * pvarFlags = PVAREMPTY;

            if (!fHistoryEntry)
            {
                varFlags.vt = VT_I4;
                varFlags.lVal = navNoHistory;
                pvarFlags = &varFlags;
            }

            hr = pwb2->Navigate2(&varThePidl, pvarFlags, PVAREMPTY, PVAREMPTY, PVAREMPTY);
            VariantClear(&varThePidl);
        }
        pwb2->Release();
    }
    else
    {
        IShellBrowser * psb;

        // Maybe we are in comdlg32.
        hr = IUnknown_QueryService(punk, SID_SCommDlgBrowser, IID_IShellBrowser, (LPVOID *) &psb);
        if (SUCCEEDED(hr))
        {
            CFtpView * pfv = GetCFtpViewFromDefViewSite(punk);

            AssertMsg((NULL != pfv), TEXT("IUnknown_IWebBrowserNavigate2() defview gave us our IShellFolderViewCB so it needs to support this interface."));
            if (pfv)
            {
                // Are we on the forground thread?
                if (pfv->IsForegroundThread())
                {
                    // Yes, so this will be easy.  This is the case
                    // where "Login As..." was chosen from the background context menu item.
                    hr = psb->BrowseObject(pidl, 0);
                }
                else
                {
                    // No, so this is the case where we failed to login with the original
                    // UserName/Password and we will try again with the corrected Username/Password.

                    // Okay, we are talking to the ComDlg code but we don't want to use
                    // IShellBrowse::BrowseObject() because we are on a background thread. (NT #297732)
                    // Therefore, we want to have the IShellFolderViewCB (CFtpView) cause
                    // the redirect on the forground thread.  Let's inform
                    // CFtpView now to do this.
                    hr = pfv->SetRedirectPidl(pidl);
                }

                pfv->Release();
            }
            
            AssertMsg(SUCCEEDED(hr), TEXT("IUnknown_IWebBrowserNavigate2() defview needs to support QS(SID_ShellFolderViewCB) on all platforms that hit this point"));
            psb->Release();
        }
    }

    return hr;
}


HRESULT IUnknown_PidlNavigate(IUnknown * punk, LPCITEMIDLIST pidl, BOOL fHistoryEntry)
{
    HRESULT hrOle = SHCoInitialize();
    HRESULT hr = IUnknown_IWebBrowserNavigate2(punk, pidl, fHistoryEntry);

    // Try a pre-NT5 work around.
    // punk will be NULL on Browser Only installs because the old
    // shell32 doesn't do ::SetSite().
    if (FAILED(hr))
    {
        IWebBrowserApp * pauto = NULL;
        
        hr = SHGetIDispatchForFolder(pidl, &pauto);
        if (EVAL(pauto))
        {
            hr = IUnknown_IWebBrowserNavigate2(pauto, pidl, fHistoryEntry);
            ASSERT(SUCCEEDED(hr));
            pauto->Release();
        }
    }

    ASSERT(SUCCEEDED(hrOle));
    SHCoUninitialize(hrOle);
    return hr;
}


/*****************************************************************************\

    HIDACREATEINFO

    Structure that collects all information needed when building
    an ID List Array.

\*****************************************************************************/

typedef struct tagHIDACREATEINFO
{
    HIDA hida;            /* The HIDA being built */
    UINT ipidl;            /* Who we are */
    UINT ib;            /* Where we are */
    UINT cb;            /* Where we're going */
    UINT cpidl;            /* How many we're doing */
    LPCITEMIDLIST pidlFolder;        /* The parent all these LPITEMIDLISTs live in */
    CFtpPidlList * pflHfpl;            /* The pidl list holding all the kids */
} HIDACREATEINFO, * LPHIDACREATEINFO;

#define pidaPhci(phci) ((LPIDA)(phci)->hida)    /* no need to lock */


/*****************************************************************************\
    Misc_SfgaoFromFileAttributes

    AIGH!

    UNIX and Win32 semantics on file permissions are different.

    On UNIX, the ability to rename or delete a file depends on
    your permissions on the parent folder.

    On Win32, the ability to rename or delete a file depends on
    your permissions on the file itself.

    Note that there is no such thing as "deny-read" attributes
    on Win32...  I wonder how WinINet handles that...

    I'm going to hope that WinINet does the proper handling of this,
    so I'll just proceed with Win32 semantics... I'm probably assuming too much...
\*****************************************************************************/
DWORD Misc_SfgaoFromFileAttributes(DWORD dwFAFLFlags)
{
    DWORD sfgao = SFGAO_CANLINK;    // You can always link

    sfgao |= SFGAO_HASPROPSHEET;    // You can always view properties

    sfgao |= SFGAO_CANCOPY;        // Deny-read?  No such thing! (Yet)

    if (dwFAFLFlags & FILE_ATTRIBUTE_READONLY)
    {        /* Can't delete it, sorry */
#ifdef _SOMEDAY_ASK_FRANCISH_WHAT_THIS_IS
        if (SHELL_VERSION_NT5 == GetShellVersion())
            sfgao |= SFGAO_READONLY;
#endif
    }
    else
    {
        sfgao |= (SFGAO_CANRENAME | SFGAO_CANDELETE);
#ifdef FEATURE_CUT_MOVE
        sfgao |= SFGAO_CANMOVE;
#endif // FEATURE_CUT_MOVE
    }

    if (dwFAFLFlags & FILE_ATTRIBUTE_DIRECTORY)
    {
        //Since FTP connections are expensive, assume SFGAO_HASSUBFOLDER
        sfgao |= SFGAO_DROPTARGET | SFGAO_FOLDER | SFGAO_HASSUBFOLDER;
    }
    else
    {
        // We always return the
        // SFGAO_BROWSABLE because we always want to do the navigation
        // using our IShellFolder::CreateViewObject().  In the case of
        // files, the CreateViewObject() that we create is for URLMON
        // which will do the download.  This is especially true for
        // Folder Shortcuts.
        sfgao |= SFGAO_BROWSABLE;
    }

    return sfgao;
}

/*****************************************************************************\
    FUNCTION: Misc_StringFromFileTime

    DESCRIPTION:
        Get the date followed by the time.  flType can be DATE_SHORTDATE
    (for defview's details list) or DATE_LONGDATE for the property sheet.
\*****************************************************************************/
HRESULT Misc_StringFromFileTime(LPTSTR pszDateTime, DWORD cchSize, LPFILETIME pft, DWORD flType)
{
    if (EVAL(pft && pft->dwHighDateTime))
    {
	SHFormatDateTime(pft, &flType, pszDateTime, cchSize);
    }
    else
        pszDateTime[0] = 0;

    return S_OK;
}


LPITEMIDLIST GetPidlFromFtpFolderAndPidlList(CFtpFolder * pff, CFtpPidlList * pflHfpl)
{
    LPCITEMIDLIST pidlBase = pff->GetPrivatePidlReference();
    LPCITEMIDLIST pidlRelative = ((0 == pflHfpl->GetCount()) ? c_pidlNil : pflHfpl->GetPidl(0));

    return ILCombine(pidlBase, pidlRelative);
}


IProgressDialog * CProgressDialog_CreateInstance(UINT idTitle, UINT idAnimation)
{
    IProgressDialog * ppd = NULL;
    
    if (EVAL(SUCCEEDED(CoCreateInstance(CLSID_ProgressDialog, NULL, CLSCTX_INPROC_SERVER, IID_IProgressDialog, (void **)&ppd))))
    {
        WCHAR wzTitle[MAX_PATH];

        if (EVAL(LoadStringW(HINST_THISDLL, idTitle, wzTitle, ARRAYSIZE(wzTitle))))
            EVAL(SUCCEEDED(ppd->SetTitle(wzTitle)));

        EVAL(SUCCEEDED(ppd->SetAnimation(HINST_THISDLL, idAnimation)));
    }

    return ppd;
}


BOOL_PTR CALLBACK ProxyDlgWarningWndProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_INITDIALOG)
    {
        LPCTSTR pszUrl = (LPCTSTR)lParam;
        TCHAR szMessage[MAX_PATH*3];
        TCHAR szTemplate[MAX_PATH*3];

        ASSERT(pszUrl);

        EVAL(LoadString(HINST_THISDLL, IDS_FTP_PROXY_WARNING, szTemplate, ARRAYSIZE(szTemplate)));
        wnsprintf(szMessage, ARRAYSIZE(szMessage), szTemplate, pszUrl);
        EVAL(SetWindowText(GetDlgItem(hDlg, IDC_PROXY_MESSAGE), szMessage));
    }

    return FALSE;
}


/*****************************************************************************\
    FUNCTION:   DisplayBlockingProxyDialog

    DESCRIPTION:
        Inform user that their CERN style proxy is blocking real FTP access so
    they can do something about it.

    Inform the user so they can: 
    A) Change proxies,
    B) Annoy their administrator to install real proxies,
    C) Install Remote WinSock themselves,
    D) or settle for their sorry situation in life and use the
       limited CERN proxy support and dream about the abilitity
       to rename, delete, and upload.

    This will be a no-op if the user clicks "Don't display this
    message again" check box.
\*****************************************************************************/
HRESULT DisplayBlockingProxyDialog(LPCITEMIDLIST pidl, HWND hwnd)
{
    // Did the IBindCtx provide information to allow us to do UI?
    if (hwnd)
    {
        TCHAR szUrl[MAX_PATH];

        UrlCreateFromPidl(pidl, SHGDN_FORPARSING, szUrl, ARRAYSIZE(szUrl), 0, TRUE);

        // Make it modal while the dialog is being displayed.
//        IUnknown_EnableModless(punkSite, FALSE);
        SHMessageBoxCheckEx(hwnd, HINST_THISDLL, MAKEINTRESOURCE(IDD_PROXYDIALOG), ProxyDlgWarningWndProc, (LPVOID) szUrl, IDOK, SZ_REGVALUE_WARN_ABOUT_PROXY);
//        IUnknown_EnableModless(punkSite, TRUE);
    }

    return S_OK;
}


HRESULT CreateFromToStr(LPWSTR pwzStrOut, DWORD cchSize, ...)
{
    CHAR szStatusText[MAX_PATH];
    CHAR szTemplate[MAX_PATH];
    va_list vaParamList;
    
    va_start(vaParamList, cchSize);
    // Generate the string "From <SrcFtpUrlDir> to <DestFileDir>" status string
    EVAL(LoadStringA(HINST_THISDLL, IDS_DL_SRC_DEST, szTemplate, ARRAYSIZE(szTemplate)));
    if (EVAL(FormatMessageA(FORMAT_MESSAGE_FROM_STRING, szTemplate, 0, 0, szStatusText, ARRAYSIZE(szStatusText), &vaParamList)))
        SHAnsiToUnicode(szStatusText, pwzStrOut, cchSize);

    va_end(vaParamList);
    return S_OK;
}

/****************************************************\
    FUNCTION: FtpProgressInternetStatusCB

    DESCRIPTION: 
        This function is exists to be called back during
    long FTP operations so we can update the progress
    dialog during FtpPutFile or FtpGetFile.

    A pointer to our PROGRESSINFO struct is passed in
    dwContext.
\****************************************************/
void FtpProgressInternetStatusCB(IN HINTERNET hInternet, IN DWORD_PTR pdwContext, IN DWORD dwInternetStatus, IN LPVOID lpwStatusInfo, IN DWORD dwStatusInfoLen)
{
    LPPROGRESSINFO pProgInfo = (LPPROGRESSINFO) pdwContext;
    if (EVAL(pProgInfo))
    {
        switch (dwInternetStatus)
        {
        case INTERNET_STATUS_RESPONSE_RECEIVED:
        case INTERNET_STATUS_REQUEST_SENT:
            if (EVAL(lpwStatusInfo && (sizeof(DWORD) == dwStatusInfoLen)
                && pProgInfo))
            {
                if (pProgInfo->hint && pProgInfo->ppd->HasUserCancelled())
                {
                    EVAL(InternetCloseHandle(pProgInfo->hint));
                    pProgInfo->hint = NULL;
                }

                pProgInfo->dwCompletedInCurFile += *(LPDWORD)lpwStatusInfo;

                // Has a big enough chunck of the file completed that we need
                // to update the progress?  We only want to update the progress
                // every SIZE_PROGRESS_AFTERBYTES (50k) chunck.
                if (pProgInfo->dwLastDisplayed < (pProgInfo->dwCompletedInCurFile / SIZE_PROGRESS_AFTERBYTES))
                {
                    ULARGE_INTEGER uliBytesCompleted;

                    pProgInfo->dwLastDisplayed = (pProgInfo->dwCompletedInCurFile / SIZE_PROGRESS_AFTERBYTES);

                    uliBytesCompleted.HighPart = 0;
                    uliBytesCompleted.LowPart = pProgInfo->dwCompletedInCurFile;
                    uliBytesCompleted.QuadPart += pProgInfo->uliBytesCompleted.QuadPart;

                    if (pProgInfo->ppd)
                        EVAL(SUCCEEDED(pProgInfo->ppd->SetProgress64(uliBytesCompleted.QuadPart, pProgInfo->uliBytesTotal.QuadPart)));
                }
            }
            break;
        }
    }
}


/*****************************************************************************\
    Misc_CreateHglob

    Allocate an hglobal of the indicated size, initialized from the
    specified buffer.
\*****************************************************************************/
HRESULT Misc_CreateHglob(SIZE_T cb, LPVOID pv, HGLOBAL *phglob)
{
    HRESULT hres = E_OUTOFMEMORY;

    *phglob = 0;            // Rules are rules
    if (cb)
    {
        *phglob = (HGLOBAL) LocalAlloc(LPTR, cb);
        if (phglob)
        {
            hres = S_OK;
            CopyMemory(*phglob, pv, cb);
        }
    }
    else
        hres = E_INVALIDARG;    // Can't clone a discardable block

    return hres;
}


/*****************************************************************************\
    _HIDA_Create_Tally

    Worker function for HIDA_Create which tallies up the total size.
\*****************************************************************************/
int _HIDA_Create_Tally(LPVOID pvPidl, LPVOID pv)
{
    LPCITEMIDLIST pidl = (LPCITEMIDLIST) pvPidl;
    UINT *pcb = (UINT *) pv;
    int nContinue = (pv ? TRUE : FALSE);

    *pcb += ILGetSize(pidl);
    return nContinue;
}


/*****************************************************************************\
    _HIDA_Create_AddIdl

    Worker function for HIDA_Create which appends another ID List
    to the growing HIDA.
\*****************************************************************************/
int _HIDA_Create_AddIdl(LPVOID pvPidl, LPVOID pv)
{
    LPCITEMIDLIST pidl = (LPCITEMIDLIST) pvPidl;
    LPHIDACREATEINFO phci = (LPHIDACREATEINFO) pv;
    UINT cb = ILGetSize(pidl);

    pidaPhci(phci)->aoffset[phci->ipidl++] = phci->ib;
    CopyMemory(pvByteIndexCb(pidaPhci(phci), phci->ib), pidl, cb);
    phci->ib += cb;

    return phci ? TRUE : FALSE;
}


/*****************************************************************************\
    _Misc_HIDA_Init

    Once we've allocated the memory for a HIDA, fill it with stuff.
\*****************************************************************************/
BOOL _Misc_HIDA_Init(LPVOID hida, LPVOID pv, LPCVOID pvParam2, BOOL fUnicode)
{
    LPHIDACREATEINFO phci = (LPHIDACREATEINFO) pv;

    phci->hida = hida;
    pidaPhci(phci)->cidl = phci->cpidl;
    phci->ipidl = 0;

    phci->pflHfpl->TraceDump(_ILNext(phci->pidlFolder), TEXT("_Misc_HIDA_Init() TraceDump Before"));

    _HIDA_Create_AddIdl((LPVOID) phci->pidlFolder, (LPVOID) phci);
    phci->pflHfpl->Enum(_HIDA_Create_AddIdl, (LPVOID) phci);

    phci->pflHfpl->TraceDump(_ILNext(phci->pidlFolder), TEXT("_Misc_HIDA_Init() TraceDump After"));

    return 1;
}


/*****************************************************************************\
    HIDA_Create

    Swiped from idlist.c in the shell because they didn't    ;Internal
    export it.                        ;Internal
\*****************************************************************************/
HIDA Misc_HIDA_Create(LPCITEMIDLIST pidlFolder, CFtpPidlList * pflHfpl)
{
    HIDACREATEINFO hci;
    LPHIDACREATEINFO phci = &hci;
    HIDA hida;

    pflHfpl->TraceDump(_ILNext(pidlFolder), TEXT("Misc_HIDA_Create() TraceDump Before"));
    phci->pidlFolder = pidlFolder;
    phci->pflHfpl = pflHfpl;
    phci->cpidl = pflHfpl->GetCount();
    phci->ib = sizeof(CIDA) + sizeof(UINT) * phci->cpidl;
    phci->cb = phci->ib + ILGetSize(pidlFolder);

    pflHfpl->Enum(_HIDA_Create_Tally, (LPVOID) &phci->cb);

    hida = AllocHGlob(phci->cb, _Misc_HIDA_Init, phci, NULL, FALSE);
    pflHfpl->TraceDump(_ILNext(pidlFolder), TEXT("Misc_HIDA_Create() TraceDump Before"));

    return hida;
}


typedef struct tagURL_FILEGROUP
{
    LPFILEGROUPDESCRIPTORA   pfgdA;
    LPFILEGROUPDESCRIPTORW   pfgdW;
    LPCITEMIDLIST            pidlParent;
} URL_FILEGROUP;

/*****************************************************************************\
    Misc_HFGD_Create

    Build a file group descriptor based on an pflHfpl.

    CFtpObj::_DelayRender_FGD() did the recursive walk to expand the list 
    of pidls, so we don't have to.
\*****************************************************************************/
#define cbFgdCfdW(cfd) FIELD_OFFSET(FILEGROUPDESCRIPTORW, fgd[cfd])
#define cbFgdCfdA(cfd) FIELD_OFFSET(FILEGROUPDESCRIPTORA, fgd[cfd])

int _Misc_HFGD_Create(LPVOID pvPidl, LPVOID pv)
{
    BOOL fSucceeded = TRUE;
    URL_FILEGROUP * pUrlFileGroup = (URL_FILEGROUP *) pv;
    LPCITEMIDLIST pidlFull = (LPCITEMIDLIST) pvPidl;
    LPCITEMIDLIST pidl;

    LPFILEGROUPDESCRIPTORA pfgdA = pUrlFileGroup->pfgdA;
    LPFILEGROUPDESCRIPTORW pfgdW = pUrlFileGroup->pfgdW;
    LPFILEDESCRIPTORA pfdA = (pfgdA ? &pfgdA->fgd[pfgdA->cItems++] : NULL);
    LPFILEDESCRIPTORW pfdW = (pfgdW ? &pfgdW->fgd[pfgdW->cItems++] : NULL);

    pidl = ILGetLastID(pidlFull);
    if (pfdA)
    {
#if !DEBUG_LEGACY_PROGRESS
        pfdA->dwFlags = (FD_ATTRIBUTES | FD_FILESIZE | FD_CREATETIME | FD_ACCESSTIME | FD_WRITESTIME | FD_PROGRESSUI);
#else // !DEBUG_LEGACY_PROGRESS
        pfdA->dwFlags = (FD_ATTRIBUTES | FD_FILESIZE | FD_CREATETIME | FD_ACCESSTIME | FD_WRITESTIME);
#endif // !DEBUG_LEGACY_PROGRESS
        pfdA->dwFileAttributes = FtpItemID_GetAttributes(pidl);
        pfdA->nFileSizeLow = FtpItemID_GetFileSizeLo(pidl);
        pfdA->nFileSizeHigh = FtpItemID_GetFileSizeHi(pidl);

        // This sucks but all WIN32_FIND_DATA want to be stored in TimeZone independent
        // ways, except for WININET's FTP.  Also note that we only store Modified
        // time and use if for everything because of another UNIX/Wininet issue.
        // See priv.h on more FTP Time/Date issues.
        pfdA->ftCreationTime = FtpPidl_GetFileTime(ILFindLastID(pidl));
        pfdA->ftLastWriteTime = pfdA->ftCreationTime;
        pfdA->ftLastAccessTime = pfdA->ftCreationTime;
    }
    else
    {
#if !DEBUG_LEGACY_PROGRESS
        pfdW->dwFlags = (FD_ATTRIBUTES | FD_FILESIZE | FD_CREATETIME | FD_ACCESSTIME | FD_WRITESTIME | FD_PROGRESSUI);
#else // !DEBUG_LEGACY_PROGRESS
        pfdW->dwFlags = (FD_ATTRIBUTES | FD_FILESIZE | FD_CREATETIME | FD_ACCESSTIME | FD_WRITESTIME);
#endif // !DEBUG_LEGACY_PROGRESS
        pfdW->dwFileAttributes = FtpItemID_GetAttributes(pidl);
        pfdW->nFileSizeLow = FtpItemID_GetFileSizeLo(pidl);
        pfdW->nFileSizeHigh = FtpItemID_GetFileSizeHi(pidl);

        // This sucks but all WIN32_FIND_DATA want to be stored in TimeZone independent
        // ways, except for WININET's FTP.  Also note that we only store Modified
        // time and use if for everything because of another UNIX/Wininet issue.
        // See priv.h on more FTP Time/Date issues.
        pfdW->ftCreationTime = FtpPidl_GetFileTime(ILFindLastID(pidl));
        pfdW->ftLastWriteTime = pfdW->ftCreationTime;
        pfdW->ftLastAccessTime = pfdW->ftCreationTime;
    }

    LPCITEMIDLIST pidlDiff = FtpItemID_FindDifference(pUrlFileGroup->pidlParent, pidlFull);

    if (pfdA)
    {
        GetWirePathFromPidl(pidlDiff, pfdA->cFileName, ARRAYSIZE(pfdA->cFileName), FALSE);
        UrlPathRemoveSlashA(pfdA->cFileName);
        UrlPathRemoveFrontSlashA(pfdA->cFileName);
        UrlPathToFilePathA(pfdA->cFileName);
    }
    else
    {
        GetDisplayPathFromPidl(pidlDiff, pfdW->cFileName, ARRAYSIZE(pfdW->cFileName), FALSE);
        UrlPathRemoveSlashW(pfdW->cFileName);
        UrlPathRemoveFrontSlashW(pfdW->cFileName);
        UrlPathToFilePathW(pfdW->cFileName);
    }

    TraceMsg(TF_FTPURL_UTILS, "_Misc_HFGD_Create() pfd(A/W)->dwFileAttributes=%#08lX", (pfdW ? pfdW->dwFileAttributes : pfdA->dwFileAttributes));

    return fSucceeded;
}


BOOL _Misc_HFGD_Init(LPVOID pv, LPVOID pvHFPL, LPCVOID pvParam2, BOOL fUnicode)
{
    CFtpPidlList * pflHfpl = (CFtpPidlList *) pvHFPL;
    URL_FILEGROUP urlFG = {0};

    urlFG.pidlParent = (LPCITEMIDLIST) pvParam2;
    if (fUnicode)
        urlFG.pfgdW = (LPFILEGROUPDESCRIPTORW) pv;
    else
        urlFG.pfgdA = (LPFILEGROUPDESCRIPTORA) pv;

    TraceMsg(TF_PIDLLIST_DUMP, "_Misc_HFGD_Init() TraceDump Before");
    pflHfpl->TraceDump(NULL, TEXT("_Misc_HFGD_Init() TraceDump before"));

    pflHfpl->Enum(_Misc_HFGD_Create, (LPVOID) &urlFG);

    pflHfpl->TraceDump(NULL, TEXT("_Misc_HFGD_Init() TraceDump after"));

    return 1;
}


HGLOBAL Misc_HFGD_Create(CFtpPidlList * pflHfpl, LPCITEMIDLIST pidlItem, BOOL fUnicode)
{
    DWORD dwCount = pflHfpl->GetCount();
    DWORD cbAllocSize = (fUnicode ? cbFgdCfdW(dwCount) : cbFgdCfdA(dwCount));

    return AllocHGlob(cbAllocSize, _Misc_HFGD_Init, pflHfpl, (LPCVOID) pidlItem, fUnicode);
}


// Returns the submenu of the given menu and ID.  Returns NULL if there
// is no submenu
int _MergePopupMenus(HMENU hmDest, HMENU hmSource, int idCmdFirst, int idCmdLast)
{
    int i, idFinal = idCmdFirst;

    for (i = GetMenuItemCount(hmSource) - 1; i >= 0; --i)
    {
        MENUITEMINFO mii;

        mii.cbSize = SIZEOF(mii);
        mii.fMask = MIIM_ID|MIIM_SUBMENU;
        mii.cch = 0;     // just in case

        if (EVAL(GetMenuItemInfo(hmSource, i, TRUE, &mii)))
        {
            HMENU hmDestSub = GetMenuFromID(hmDest, mii.wID);
            if (hmDestSub)
            {
                int idTemp = Shell_MergeMenus(hmDestSub, mii.hSubMenu, (UINT)0, idCmdFirst, idCmdLast, MM_ADDSEPARATOR | MM_SUBMENUSHAVEIDS);

                if (idFinal < idTemp)
                    idFinal = idTemp;
            }
        }
    }

    return idFinal;
}


/*****************************************************************************\
    FUNCTION: AddToPopupMenu

    DESCRIPTION:
      Swiped from utils.c in RNAUI, in turn swiped from the    ;Internal
      shell.                            ;Internal
                                  ;Internal
      Takes a destination menu and a (menu id, submenu index) pair,
      and inserts the items from the (menu id, submenu index) at location
      imi in the destination menu, with a separator, returning the number
      of items added.  (imi = index to menu item)
  
      Returns the first the number of items added.
  
      hmenuDst        - destination menu
      idMenuToAdd        - menu resource identifier
      idSubMenuIndex    - submenu from menu resource to act as template
      indexMenu        - location at which menu items should be inserted
      idCmdFirst        - first available menu identifier
      idCmdLast       - first unavailable menu identifier
      uFlags            - flags for Shell_MergeMenus
\*****************************************************************************/
#define FLAGS_MENUMERGE                 (MM_SUBMENUSHAVEIDS | MM_DONTREMOVESEPS)

UINT AddToPopupMenu(HMENU hmenuDst, UINT idMenuToAdd, UINT idSubMenuIndex, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    UINT nLastItem = 0;

    HMENU hmenuSrc = LoadMenu(g_hinst, MAKEINTRESOURCE(idMenuToAdd));
    if (hmenuSrc)
    {
        nLastItem = Shell_MergeMenus(hmenuDst, GetSubMenu(hmenuSrc, idSubMenuIndex), indexMenu, idCmdFirst, idCmdLast, (uFlags | FLAGS_MENUMERGE));
        DestroyMenu(hmenuSrc);
    }

    return nLastItem;
}


UINT MergeInToPopupMenu(HMENU hmenuDst, UINT idMenuToMerge, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    UINT nLastItem = 0;

    HMENU hmenuSrc = LoadMenu(g_hinst, MAKEINTRESOURCE(idMenuToMerge));
    if (hmenuSrc)
    {
        nLastItem = _MergePopupMenus(hmenuDst, hmenuSrc, idCmdFirst, idCmdLast);
        DestroyMenu(hmenuSrc);
    }

    return nLastItem;
}


/*****************************************************************************\

    GetMenuFromID

    Swiped from defviewx.c in the shell.            ;Internal
                                ;Internal
    Given an actual menu and a menu identifier which corresponds
    to a submenu, return the submenu handle.

    hmenu - source menu
    idm   - menu identifier

\*****************************************************************************/
HMENU GetMenuFromID(HMENU hmenu, UINT idm)
{
    HMENU hmenuRet = NULL;
    if (!hmenu)
        return NULL;

    MENUITEMINFO mii;
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_SUBMENU;
    mii.cch = 0;             // just in case
    mii.hSubMenu = 0;        // in case GetMenuItemInfo fails

    if (GetMenuItemInfo(hmenu, idm, 0, &mii))
        hmenuRet = mii.hSubMenu;

    return hmenuRet;
}


/*****************************************************************************\
    MergeMenuHierarchy

    Swiped from defcm.c in the shell.            ;Internal
                                ;Internal
    Given an actual menu (hmenuDst), iterate over its submenus
    and merge corresponding submenus whose IDs match the IDs of
    actuals.

    hmenuDst - menu being adjusted
    hmenuSrc - template menu
    idcMin     - first available index
    idcMax     - first unavailable index
\*****************************************************************************/
UINT MergeMenuHierarchy(HMENU hmenuDst, HMENU hmenuSrc, UINT idcMin, UINT idcMax)
{
    int imi;
    UINT idcMaxUsed = idcMin;

    imi = GetMenuItemCount(hmenuSrc);
    while (--imi >= 0)
    {
        UINT idcT;
        MENUITEMINFO mii;

        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_ID|MIIM_SUBMENU;
        mii.cch = 0;     /* just in case */

        if (GetMenuItemInfo(hmenuSrc, imi, 1, &mii))
        {
            idcT = Shell_MergeMenus(GetMenuFromID(hmenuDst, mii.wID),
                mii.hSubMenu, (UINT)0, idcMin, idcMax,
                MM_ADDSEPARATOR | MM_SUBMENUSHAVEIDS);
            idcMaxUsed = max(idcMaxUsed, idcT);
        }
    }

    return idcMaxUsed;
}


HRESULT _SetStatusBarZone(CStatusBar * psb, CFtpSite * pfs)
{
    if (EVAL(psb && pfs))
    {
        LPITEMIDLIST pidl = pfs->GetPidl();

        if (pidl)
        {
            TCHAR szUrl[MAX_URL_STRING];

            UrlCreateFromPidl(pidl, SHGDN_FORPARSING, szUrl, ARRAYSIZE(szUrl), 0, TRUE);
            psb->UpdateZonesPane(szUrl);
            ILFree(pidl);
        }
    }

    return S_OK;
}


/*****************************************************************************\

    Misc_CopyPidl

    I wrote this on my own, and discovered months later    ;Internal
    that this is the same as SHILClone...            ;Internal
                                ;Internal
\*****************************************************************************/
HRESULT Misc_CopyPidl(LPCITEMIDLIST pidl, LPITEMIDLIST * ppidlOut)
{
    *ppidlOut = ILClone(pidl);
    return *ppidlOut ? S_OK : E_OUTOFMEMORY;
}


/*****************************************************************************\

    Misc_CloneHglobal

\*****************************************************************************/
HRESULT Misc_CloneHglobal(HGLOBAL hglob, HGLOBAL *phglob)
{
    LPVOID pv;
    HRESULT hres;

    ASSERT(hglob);
    *phglob = 0;            /* Rules are rules */
    pv = GlobalLock(hglob);
    if (EVAL(pv))
    {
        hres = Misc_CreateHglob(GlobalSize(hglob), pv, phglob);
        GlobalUnlock(hglob);
    }
    else
    {                /* Not a valid global handle */
        hres = E_INVALIDARG;
    }
    return hres;
}


#define FTP_PROPPAGES_FROM_INETCPL          (INET_PAGE_SECURITY | INET_PAGE_CONTENT | INET_PAGE_CONNECTION)

HRESULT AddFTPPropertyPages(LPFNADDPROPSHEETPAGE pfnAddPropSheetPage, LPARAM lParam, HINSTANCE * phinstInetCpl, IUnknown * punkSite)
{
    HRESULT hr = E_FAIL;

    if (NULL == *phinstInetCpl)
        *phinstInetCpl = LoadLibrary(TEXT("inetcpl.cpl"));

    // First add the pages from the Internet Control Panel.
    if (EVAL(*phinstInetCpl))
    {
        PFNADDINTERNETPROPERTYSHEETSEX pfnAddSheet = (PFNADDINTERNETPROPERTYSHEETSEX)GetProcAddress(*phinstInetCpl, STR_ADDINTERNETPROPSHEETSEX);
        if (EVAL(pfnAddSheet))
        {
            IEPROPPAGEINFO iepi = {0};

            iepi.cbSize = sizeof(iepi);
            iepi.dwFlags = (DWORD)-1;       // all pages

            hr = pfnAddSheet(pfnAddPropSheetPage, lParam, 0, 0, &iepi);
        }
        // Don't FreeLibrary here, otherwise PropertyPage will GP-fault!
    }

    ASSERT(SUCCEEDED(hr));

    if (((LPPROPSHEETHEADER)lParam)->nPages > 0)
        return hr;
    else
        return S_FALSE;

}


#if 0
/*****************************************************************************\

    Misc_SetDataDword

\*****************************************************************************/
HRESULT Misc_SetDataDword(IDataObject *pdto, FORMATETC *pfe, DWORD dw)
{
    HRESULT hres;
    HGLOBAL hglob;

    hres = Misc_CreateHglob(sizeof(dw), &dw, &hglob);
    if (SUCCEEDED(hres))
    {
        STGMEDIUM stg = { TYMED_HGLOBAL, hglob, 0 };
        hres = pdto->SetData(&fe, &stg, 1);

        if (!(EVAL(SUCCEEDED(hres))))
            GlobalFree(hglob);
    }
    else
        hres = E_OUTOFMEMORY;

    return hres;
}
#endif


CFtpPidlList * CreateRelativePidlList(CFtpFolder * pff, CFtpPidlList * pPidlListFull)
{
    int nSize = pPidlListFull->GetCount();
    CFtpPidlList * pPidlListNew = NULL;

    if (nSize > 0)
    {
        LPCITEMIDLIST pidlFirst = pff->GetPrivatePidlReference();
        int nCount = 0;

        while (!ILIsEmpty(pidlFirst))
        {
            pidlFirst = _ILNext(pidlFirst);
            nCount++;
        }

        if (nSize > 0)
        {
            for (int nIndex = 0; nIndex < nSize; nIndex++)
            {
                int nLeft = nCount;
                LPITEMIDLIST pidl = pPidlListFull->GetPidl(nIndex);

                while (nLeft--)
                    pidl = _ILNext(pidl);

                AssertMsg((pidl ? TRUE : FALSE), TEXT("CreateRelativePidlList() pPidlListFull->GetPidl() should never fail because we got the size and no mem allocation is needed."));
                if (0 == nIndex)
                {
                    CFtpPidlList_Create(1, (LPCITEMIDLIST *)&pidl, &pPidlListNew);
                    if (!pPidlListNew)
                        break;
                }
                else
                {
                    // We only want to add top level nodes.
                    // ftp://s/d1/d2/         <- Root of copy.
                    // ftp://s/d1/d2/d3a/     <- First Top Level Item
                    // ftp://s/d1/d2/d3a/f1   <- Skip non-top level items
                    // ftp://s/d1/d2/d3b/     <- Second Top Level Item
                    if (pidl && !ILIsEmpty(pidl) && ILIsEmpty(_ILNext(pidl)))
                        pPidlListNew->InsertSorted(pidl);
                }
            }
        }
    }

    return pPidlListNew;
}


#define SZ_VERB_DELETEA             "delete"
/*****************************************************************************\
    FUNCTION: Misc_DeleteHfpl

    DESCRIPTION:
        Delete the objects described by a pflHfpl.
\*****************************************************************************/
HRESULT Misc_DeleteHfpl(CFtpFolder * pff, HWND hwnd, CFtpPidlList * pflHfpl)
{
    IContextMenu * pcm;
    HRESULT hr = pff->GetUIObjectOfHfpl(hwnd, pflHfpl, IID_IContextMenu, (LPVOID *)&pcm, FALSE);

    if (EVAL(SUCCEEDED(hr)))
    {
        CMINVOKECOMMANDINFO ici = {
            sizeof(ici),            // cbSize
            CMIC_MASK_FLAG_NO_UI,    // fMask
            hwnd,                    // hwnd
            SZ_VERB_DELETEA,        // lpVerb
            0,                        // lpParameters
            0,                        // lpDirectory
            0,                        // nShow
            0,                        // dwHotKey
            0,                        // hIcon
        };
        hr = pcm->InvokeCommand(&ici);
        pcm->Release();
    }
    else
    {
        // Couldn't delete source; oh well.  Don't need UI.
        // BUGBUG -- Actually, maybe we ought to do UI after all.
    }

    return hr;
}

/*****************************************************************************\

    Misc_FindStatusBar

    Get the status bar from a browser window.

    _UNDOCUMENTED_: The following quirks are not documented.

    Note that we need to be very paranoid about the way GetControlWindow
    works.  Some people (Desktop) properly return error if the window
    does not exist.  Others (Explorer) return S_OK when the window
    does not exist, but they kindly set *lphwndOut = 0.  Still others
    (Find File) return S_OK but leave *lphwndOut unchanged!

    In order to work with all these bozos, we must manually set hwnd = 0
    before calling, and continue only if GetControlWindow returns success
    *and* the outgoing hwnd is nonzero.

    Furthermore, the documentation for GetControlWindow says that we
    have to check the window class before trusting the hwnd.

\*****************************************************************************/

#pragma BEGIN_CONST_DATA

TCHAR c_tszStatusBarClass[] = STATUSCLASSNAME;

#pragma END_CONST_DATA

HWND Misc_FindStatusBar(HWND hwndOwner)
{
    HWND hwnd = 0;    // Must preinit in case GetControlWindow fails

    if (EVAL(hwndOwner))
    {
        IShellBrowser * psb = FileCabinet_GetIShellBrowser(hwndOwner);

        if (EVAL(psb))
        {
            if (SUCCEEDED(psb->GetControlWindow(FCW_STATUS, &hwnd)) && hwnd) // This won't work when hosted in an IFRAME
            {
                //  Make sure it really is a status bar...
                TCHAR tszClass[ARRAYSIZE(c_tszStatusBarClass)+1];

                if (GetClassName(hwnd, tszClass, ARRAYSIZE(tszClass)) &&
                    !StrCmpI(tszClass, c_tszStatusBarClass))
                {
                    // We have a winner
                }
                else
                    hwnd = 0;        // False positive
            }
        }
    }

    return hwnd;
}

#ifdef DEBUG
void TraceMsgWithCurrentDir(DWORD dwTFOperation, LPCSTR pszMessage, HINTERNET hint)
{
    // For debugging...
    TCHAR szCurrentDir[MAX_PATH];
    DWORD cchDebugSize = ARRAYSIZE(szCurrentDir);

    DEBUG_CODE(DebugStartWatch());
    // PERF: Status FtpGetCurrentDirectory/FtpSetCurrentDirectory() takes
    //  180-280ms on ftp.microsoft.com on average.
    //  500-2000ms on ftp://ftp.tu-clausthal.de/ on average
    //  0-10ms on ftp://shapitst/ on average
    EVAL(FtpGetCurrentDirectory(hint, szCurrentDir, &cchDebugSize));
    DEBUG_CODE(TraceMsg(TF_WININET_DEBUG, "TraceMsgWithCurrentDir() FtpGetCurrentDirectory() returned %ls and took %lu milliseconds", szCurrentDir, DebugStopWatch()));
    TraceMsg(dwTFOperation, pszMessage, szCurrentDir);
}


void DebugStartWatch(void)
{
    LARGE_INTEGER liStopWatchStart;
    
    liStopWatchStart.HighPart = PtrToUlong(TlsGetValue(g_TLSliStopWatchStartHi));
    liStopWatchStart.LowPart = PtrToUlong(TlsGetValue(g_TLSliStopWatchStartLo));

    ASSERT(!liStopWatchStart.QuadPart); // If you hit this, then the stopwatch is nested.
    QueryPerformanceFrequency(&g_liStopWatchFreq);
    QueryPerformanceCounter(&liStopWatchStart);

    TlsSetValue(g_TLSliStopWatchStartHi, (LPVOID) liStopWatchStart.HighPart);
    TlsSetValue(g_TLSliStopWatchStartLo, (LPVOID) liStopWatchStart.LowPart);
}

DWORD DebugStopWatch(void)
{
    LARGE_INTEGER liDiff;
    LARGE_INTEGER liStopWatchStart;
    
    QueryPerformanceCounter(&liDiff);
    liStopWatchStart.HighPart = PtrToUlong(TlsGetValue(g_TLSliStopWatchStartHi));
    liStopWatchStart.LowPart = PtrToUlong(TlsGetValue(g_TLSliStopWatchStartLo));
    liDiff.QuadPart -= liStopWatchStart.QuadPart;

    ASSERT(0 != g_liStopWatchFreq.QuadPart);    // I don't like to fault with div 0.
    DWORD dwTime = (DWORD)((liDiff.QuadPart * 1000) / g_liStopWatchFreq.QuadPart);
    
    TlsSetValue(g_TLSliStopWatchStartHi, (LPVOID) 0);
    TlsSetValue(g_TLSliStopWatchStartLo, (LPVOID) 0);

    return dwTime;
}
#endif // DEBUG


/*****************************************************************************\

    GetCfBuf

    Convert a clipboard format name to something stringable.

\*****************************************************************************/
void GetCfBufA(UINT cf, LPSTR pszOut, int cchOut)
{
    if (!GetClipboardFormatNameA(cf, pszOut, cchOut))
       wnsprintfA(pszOut, cchOut, "[%04x]", cf);
}

/*****************************************************************************\

    AllocHGlob

    Allocate a moveable HGLOBAL of the requested size, lock it, then call
    the callback.  On return, unlock it and get out.

    Returns the allocated HGLOBAL, or 0.

\*****************************************************************************/

HGLOBAL AllocHGlob(UINT cb, HGLOBWITHPROC pfn, LPVOID pvRef, LPCVOID pvParam2, BOOL fUnicode)
{
    HGLOBAL hglob = GlobalAlloc(GHND, cb);
    if (hglob)
    {
        LPVOID pv = GlobalLock(hglob);
        if (pv)
        {
            BOOL fRc = pfn(pv, pvRef, pvParam2, fUnicode);
            GlobalUnlock(hglob);
            if (!fRc)
            {
                GlobalFree(hglob);
                hglob = 0;
            }
        }
        else
        {
            GlobalFree(hglob);
            hglob = 0;
        }
    }

    return hglob;
}


SHELL_VERSION g_ShellVersion = SHELL_VERSION_UNKNOWN;
#define SHELL_VERSION_FOR_WIN95_AND_NT4     4


SHELL_VERSION GetShellVersion(void)
{
    if (SHELL_VERSION_UNKNOWN == g_ShellVersion)
    {
        g_ShellVersion = SHELL_VERSION_W95NT4;
        HINSTANCE hInst = LoadLibrary(TEXT("shell32.dll"));

        if (EVAL(hInst))
        {
            DLLGETVERSIONPROC pfnDllGetVersion = (DLLGETVERSIONPROC) GetProcAddress(hInst, "DllGetVersion");

            if (pfnDllGetVersion)
            {
                DLLVERSIONINFO dllVersionInfo;

                g_ShellVersion = SHELL_VERSION_IE4;      // Assume this.
                dllVersionInfo.cbSize = sizeof(dllVersionInfo);
                if (SUCCEEDED(pfnDllGetVersion(&dllVersionInfo)))
                {
                    if (SHELL_VERSION_FOR_WIN95_AND_NT4 < dllVersionInfo.dwMajorVersion)
                        g_ShellVersion = SHELL_VERSION_NT5;      // Assume this.
                }
            }
            FreeLibrary(hInst);
        }
    }
    
    return g_ShellVersion;
}

DWORD GetShdocvwVersion(void)
{
    static DWORD majorVersion=0;  // cache for perf

    if (majorVersion)
        return majorVersion;
    
    HINSTANCE hInst = LoadLibrary(TEXT("shdocvw.dll"));
    if (EVAL(hInst))
    {
        DLLGETVERSIONPROC pfnDllGetVersion = (DLLGETVERSIONPROC) GetProcAddress(hInst, "DllGetVersion");

        if (pfnDllGetVersion)
        {
            DLLVERSIONINFO dllVersionInfo;

            dllVersionInfo.cbSize = sizeof(dllVersionInfo);
            if (SUCCEEDED(pfnDllGetVersion(&dllVersionInfo)))
            {
                majorVersion = dllVersionInfo.dwMajorVersion;
            }
        }

        FreeLibrary(hInst);
    }

    return majorVersion;
}


BOOL ShouldSkipDropFormat(int nIndex)
{
    // Allow DROP_IDList or repositioning items withing
    // ftp windows won't work.
/*
    // We want to skip DROP_IDList on Win95 and WinNT4's shell
    // because it will cause the old shell to only offer DROPEFFECT_LINK
    // so download isn't available.
    if (((DROP_IDList == nIndex)) &&
        (SHELL_VERSION_W95NT4 == GetShellVersion()))
    {
        return TRUE;
    }
*/

#ifndef BROWSERONLY_DRAGGING
    if (((DROP_FGDW == nIndex) || (DROP_FGDA == nIndex)) &&
        (SHELL_VERSION_NT5 != GetShellVersion()))
    {
        return TRUE;
    }
#endif // BROWSERONLY_DRAGGING

    return FALSE;
}


void SetWindowBits(HWND hWnd, int iWhich, DWORD dwBits, DWORD dwValue)
{
    DWORD dwStyle;
    DWORD dwNewStyle;

    dwStyle = GetWindowLong(hWnd, iWhich);
    dwNewStyle = ( dwStyle & ~dwBits ) | (dwValue & dwBits);
    if (dwStyle != dwNewStyle) {
        SetWindowLong(hWnd, iWhich, dwNewStyle);
    }
}


void InitComctlForNaviteFonts(void)
{
    // hinst is ignored because we set it at our LibMain()
    INITCOMMONCONTROLSEX icex = {0};

    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_USEREX_CLASSES|ICC_NATIVEFNTCTL_CLASS;
    InitCommonControlsEx(&icex);
}


BOOL DoesUrlContainNTDomainName(LPCTSTR pszUrl)
{
    BOOL fResult = FALSE;
    LPCTSTR pszPointer = pszUrl;

    if (lstrlen(pszPointer) > ARRAYSIZE(SZ_FTPURL))
    {
        pszPointer += ARRAYSIZE(SZ_FTPURL); // Skip past the scheme.
        pszPointer = StrChr(pszPointer, CH_URL_SLASH);
        if (pszPointer)
        {
            pszPointer = StrChr(CharNext(pszPointer), CH_URL_PASSWORD_SEPARATOR);
            if (pszPointer)
            {
                pszPointer = StrChr(CharNext(pszPointer), CH_URL_LOGON_SEPARATOR);
                if (pszPointer)
                    fResult = TRUE;
            }
        }
    }

    return fResult;
}


HRESULT CharReplaceWithStrW(LPWSTR pszLocToInsert, DWORD cchSize, DWORD cchChars, LPWSTR pszStrToInsert)
{
    WCHAR szTemp[MAX_URL_STRING];

    StrCpyNW(szTemp, pszLocToInsert, ARRAYSIZE(szTemp));

    pszLocToInsert[0] = 0; // Terminate String here to kill char.
    StrCatBuffW(pszLocToInsert, pszStrToInsert, cchSize);
    StrCatBuffW(pszLocToInsert, &szTemp[cchChars], cchSize);

    return S_OK;
}


HRESULT CharReplaceWithStrA(LPSTR pszLocToInsert, DWORD cchSize, DWORD cchChars, LPSTR pszStrToInsert)
{
    CHAR szTemp[MAX_URL_STRING];

    StrCpyNA(szTemp, pszLocToInsert, ARRAYSIZE(szTemp));

    pszLocToInsert[0] = 0; // Terminate String here to kill char.
    StrCatBuffA(pszLocToInsert, pszStrToInsert, cchSize);
    StrCatBuffA(pszLocToInsert, &szTemp[cchChars], cchSize);

    return S_OK;
}


HRESULT RemoveCharsFromString(LPTSTR pszLocToRemove, DWORD cchSizeToRemove)
{
    LPTSTR pszRest = &pszLocToRemove[cchSizeToRemove];

    MoveMemory((LPVOID) pszLocToRemove, (LPVOID) pszRest, (lstrlen(pszRest) + 1) * sizeof(TCHAR));
    return S_OK;
}


HRESULT RemoveCharsFromStringA(LPSTR pszLocToRemove, DWORD cchSizeToRemove)
{
    LPSTR pszRest = &pszLocToRemove[cchSizeToRemove];

    MoveMemory((LPVOID) pszLocToRemove, (LPVOID) pszRest, (lstrlenA(pszRest) + 1) * sizeof(CHAR));
    return S_OK;
}


// Helper function to convert Ansi string to allocated BSTR
#ifndef UNICODE
BSTR AllocBStrFromString(LPCTSTR psz)
{
    OLECHAR wsz[INFOTIPSIZE];  // assumes INFOTIPSIZE number of chars max

    SHAnsiToUnicode(psz, wsz, ARRAYSIZE(wsz));
    return SysAllocString(wsz);

}
#endif // UNICODE


/****************************************************\
    FUNCTION: StrListLength

    DESCRIPTION:
\****************************************************/
DWORD StrListLength(LPCTSTR ppszStrList)
{
    LPTSTR pszStr = (LPTSTR) ppszStrList;
    DWORD cchLength = 0;

    while (pszStr[0])
    {
        pszStr += (lstrlen(pszStr) + 1);
        cchLength++;
    }

    return cchLength;
}


/****************************************************\
    FUNCTION: CalcStrListSizeA

    DESCRIPTION:
\****************************************************/
DWORD CalcStrListSizeA(LPCSTR ppszStrList)
{
    LPSTR pszStr = (LPSTR) ppszStrList;
    DWORD cchSize = 1;

    while (pszStr[0])
    {
        DWORD cchSizeCurr = lstrlenA(pszStr) + 1;

        cchSize += cchSizeCurr;
        pszStr += cchSizeCurr;
    }

    return cchSize;
}


/****************************************************\
    FUNCTION: CalcStrListSizeW

    DESCRIPTION:
\****************************************************/
DWORD CalcStrListSizeW(LPCWSTR ppwzStrList)
{
    LPWSTR pwzStr = (LPWSTR) ppwzStrList;
    DWORD cchSize = 1;

    while (pwzStr[0])
    {
        DWORD cchSizeCurr = lstrlenW(pwzStr) + 1;

        cchSize += cchSizeCurr;
        pwzStr += cchSizeCurr;
    }

    return cchSize;
}


/****************************************************\
    FUNCTION: AnsiToUnicodeStrList

    DESCRIPTION:
\****************************************************/
void AnsiToUnicodeStrList(LPCSTR ppszStrListIn, LPCWSTR ppwzStrListOut, DWORD cchSize)
{
    LPWSTR pwzStrOut = (LPWSTR) ppwzStrListOut;
    LPSTR pszStrIn = (LPSTR) ppszStrListIn;

    while (pszStrIn[0])
    {
        SHAnsiToUnicode(pszStrIn, pwzStrOut, lstrlenA(pszStrIn) + 2);

        pszStrIn += lstrlenA(pszStrIn) + 1;
        pwzStrOut += lstrlenW(pwzStrOut) + 1;
    }

    pwzStrOut[0] = L'\0';
}


/****************************************************\
    FUNCTION: UnicodeToAnsiStrList

    DESCRIPTION:
\****************************************************/
void UnicodeToAnsiStrList(LPCWSTR ppwzStrListIn, LPCSTR ppszStrListOut, DWORD cchSize)
{
    LPSTR pszStrOut = (LPSTR) ppszStrListOut;
    LPWSTR pwzStrIn = (LPWSTR) ppwzStrListIn;

    while (pwzStrIn[0])
    {
        SHUnicodeToAnsi(pwzStrIn, pszStrOut, lstrlenW(pwzStrIn) + 2);

        pwzStrIn += lstrlenW(pwzStrIn) + 1;
        pszStrOut += lstrlenA(pszStrOut) + 1;
    }

    pszStrOut[0] = '\0';
}


/****************************************************\
    FUNCTION: Str_StrAndThunkA

    DESCRIPTION:
\****************************************************/
HRESULT Str_StrAndThunkA(LPTSTR * ppszOut, LPCSTR pszIn, BOOL fStringList)
{
#ifdef UNICODE
    if (!fStringList)
    {
        DWORD cchSize = (lstrlenA(pszIn) + 2);
        LPWSTR pwzBuffer = (LPWSTR) LocalAlloc(LPTR, cchSize * SIZEOF(WCHAR));

        if (!pwzBuffer)
            return E_OUTOFMEMORY;

        SHAnsiToUnicode(pszIn, pwzBuffer, cchSize);
        Str_SetPtrW(ppszOut, pwzBuffer);
    }
    else
    {
        DWORD cchSize = CalcStrListSizeA(pszIn);
        Str_SetPtrW(ppszOut, NULL); // Free

        *ppszOut = (LPTSTR) LocalAlloc(LPTR, cchSize * sizeof(WCHAR));
        if (*ppszOut)
            AnsiToUnicodeStrList(pszIn, *ppszOut, cchSize);
    }

#else // UNICODE

    if (!fStringList)
    {
        // No thunking needed.
        Str_SetPtrA(ppszOut, pszIn);
    }
    else
    {
        DWORD cchSize = CalcStrListSizeA(pszIn);
        Str_SetPtrA(ppszOut, NULL); // Free

        *ppszOut = (LPTSTR) LocalAlloc(LPTR, cchSize * sizeof(CHAR));
        if (*ppszOut)
            CopyMemory(*ppszOut, pszIn, cchSize * sizeof(CHAR));
    }
#endif // UNICODE

    return S_OK;
}


BOOL IsValidFtpAnsiFileName(LPCTSTR pszString)
{
#ifdef UNICODE
    // TODO:
#endif // UNICODE
    return TRUE;
}


/****************************************************\
    FUNCTION: Str_StrAndThunkW

    DESCRIPTION:
\****************************************************/
HRESULT Str_StrAndThunkW(LPTSTR * ppszOut, LPCWSTR pwzIn, BOOL fStringList)
{
#ifdef UNICODE
    if (!fStringList)
    {
        // No thunking needed.
        Str_SetPtrW(ppszOut, pwzIn);
    }
    else
    {
        DWORD cchSize = CalcStrListSizeW(pwzIn);
        Str_SetPtrW(ppszOut, NULL); // Free

        *ppszOut = (LPTSTR) LocalAlloc(LPTR, cchSize * sizeof(WCHAR));
        if (*ppszOut)
            CopyMemory(*ppszOut, pwzIn, cchSize * sizeof(WCHAR));
    }

#else // UNICODE

    if (!fStringList)
    {
        DWORD cchSize = (lstrlenW(pwzIn) + 2);
        LPSTR pszBuffer = (LPSTR) LocalAlloc(LPTR, cchSize * SIZEOF(CHAR));

        if (!pszBuffer)
            return E_OUTOFMEMORY;

        SHUnicodeToAnsi(pwzIn, pszBuffer, cchSize);
        Str_SetPtrA(ppszOut, pszBuffer);
    }
    else
    {
        DWORD cchSize = CalcStrListSizeW(pwzIn);
        Str_SetPtrA(ppszOut, NULL); // Free

        *ppszOut = (LPTSTR) LocalAlloc(LPTR, cchSize * sizeof(CHAR));
        if (*ppszOut)
            UnicodeToAnsiStrList(pwzIn, *ppszOut, cchSize * sizeof(CHAR));
    }
#endif // UNICODE

    return S_OK;
}


#ifndef UNICODE
// TruncateString
//
// purpose: cut a string at the given length in dbcs safe manner.
//          the string may be truncated at cch-2 if the sz[cch] points
//          to a lead byte that would result in cutting in the middle
//          of double byte character.
//
// update: made it faster for sbcs environment (5/26/97)
//         now returns adjusted cch            (6/20/97)
//
void  TruncateString(char *sz, int cchBufferSize)
{
    if (!sz || cchBufferSize <= 0) return;

    int cch = cchBufferSize - 1; // get index position to NULL out
    
    LPSTR psz = &sz[cch];
    
    while (psz >sz)
    {
        psz--;
        if (!IsDBCSLeadByte(*psz))
        {
            // Found non-leadbyte for the first time.
            // This is either a trail byte of double byte char
            // or a single byte character we've first seen.
            // Thus, the next pointer must be at either of a leadbyte
            // or &sz[cch]
            psz++;
            break;
        }
    }
    if (((&sz[cch] - psz) & 1) && cch > 0)
    {
        // we're truncating the string in the middle of dbcs
        cch--;
    }
    sz[cch] = '\0';
    return;
}

#endif // UNICODE



HRESULT CopyStgMediumWrap(const STGMEDIUM * pcstgmedSrc, STGMEDIUM * pstgmedDest)
{
    HRESULT hr = CopyStgMedium(pcstgmedSrc, pstgmedDest);

    // if pstgmedDest->pUnkForElease is NULL,
    //  then we need to free hglobal because we own freeing the memory.
    //  else someone else owns the lifetime of the memory and releasing
    //  pUnkForElease is the way to indicate that we won't use it anymore.
    //
    // The problem is that urlmon's CopyStgMedium() ERRouniously copies the
    // pUnkForElease param in addition to cloning the memory.  This means
    // that we own freeing the memory but the pointer being non-NULL would
    // indicate that we don't own freeing the memory.

    // ASSERT(NULL == pstgmedDest->pUnkForElease);
    pstgmedDest->pUnkForRelease = NULL;

    return hr;
}


HRESULT SHBindToIDList(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    IShellFolder * psf;
    HRESULT hr = SHGetDesktopFolder(&psf);

    if (EVAL(SUCCEEDED(hr)))
    {
        hr = psf->BindToObject(pidl, pbc, riid, ppv);
        psf->Release();
    }

    return hr;
}


STDAPI DataObj_SetGlobal(IDataObject *pdtobj, UINT cf, HGLOBAL hGlobal)
{
    FORMATETC fmte = {(CLIPFORMAT) cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium;

    medium.tymed = TYMED_HGLOBAL;
    medium.hGlobal = hGlobal;
    medium.pUnkForRelease = NULL;

    // give the data object ownership of ths
    return pdtobj->SetData(&fmte, &medium, TRUE);
}


STDAPI DataObj_SetDWORD(IDataObject *pdtobj, UINT cf, DWORD dw)
{
    HRESULT hr = E_OUTOFMEMORY;
    DWORD *pdw = (DWORD *)GlobalAlloc(GPTR, sizeof(DWORD));
    if (pdw)
    {
        *pdw = dw;
        hr = DataObj_SetGlobal(pdtobj, cf, pdw);

        if (FAILED(hr))
            GlobalFree((HGLOBAL)pdw);
    }

    return hr;
}


STDAPI DataObj_GetDWORD(IDataObject *pdtobj, UINT cf, DWORD *pdwOut)
{
    STGMEDIUM medium;
    FORMATETC fmte = {(CLIPFORMAT) cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    HRESULT hr = pdtobj->GetData(&fmte, &medium);
    if (SUCCEEDED(hr))
    {
        DWORD *pdw = (DWORD *)GlobalLock(medium.hGlobal);
        if (pdw)
        {
            *pdwOut = *pdw;
            GlobalUnlock(medium.hGlobal);
        }
        else
        {
            hr = E_UNEXPECTED;
        }

        ReleaseStgMedium(&medium);
    }

    return hr;
}


STDAPI DataObj_GetDropTarget(IDataObject *pdtobj, CLSID *pclsid)
{
    STGMEDIUM medium;
    FORMATETC fmte = {(CLIPFORMAT) g_cfTargetCLSID, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    HRESULT hr = pdtobj->GetData(&fmte, &medium);
    if (SUCCEEDED(hr))
    {
        CLSID *pdw = (CLSID *)GlobalLock(medium.hGlobal);
        if (pdw)
        {
            *pclsid = *pdw;
            GlobalUnlock(medium.hGlobal);
        }
        else
        {
            hr = E_UNEXPECTED;
        }
        ReleaseStgMedium(&medium);
    }
    return hr;
}


STDAPI DataObj_SetPreferredEffect(IDataObject *pdtobj, DWORD dwEffect)
{
    return DataObj_SetDWORD(pdtobj, g_dropTypes[DROP_PrefDe].cfFormat, dwEffect);
}


STDAPI DataObj_SetPasteSucceeded(IDataObject *pdtobj, DWORD dwEffect)
{
    return DataObj_SetDWORD(pdtobj, g_formatPasteSucceeded.cfFormat, dwEffect);
}




/****************************************************\
    FUNCTION: ShowEnableWindow

    DESCRIPTION:
        If you don't want a window to be visible or
    usable by the user, you need to call both
    ShowWindow(SW_HIDE) and EnableWindow(FALSE) or
    the window may be hidden but still accessible via
    the keyboard.
\****************************************************/
void ShowEnableWindow(HWND hwnd, BOOL fShow)
{
    ShowWindow(hwnd, (fShow ? SW_SHOW : SW_HIDE));
    EnableWindow(hwnd, fShow);
}


STDAPI StringToStrRetW(LPCWSTR pwzString, STRRET *pstrret)
{
    HRESULT hr = SHStrDupW(pwzString, &pstrret->pOleStr);
    if (SUCCEEDED(hr))
    {
        pstrret->uType = STRRET_WSTR;
    }
    return hr;
}


#define BIT_8_SET       0x80

BOOL Is7BitAnsi(LPCWIRESTR pwByteStr)
{
    BOOL fIs7BitAnsi = TRUE;

    if (pwByteStr)
    {
        while (pwByteStr[0]) 
        {
            if (BIT_8_SET & pwByteStr[0])
            {
                fIs7BitAnsi = FALSE;
                break;
            }

            pwByteStr++;
        }
    }

    return fIs7BitAnsi;
}


HRESULT LoginAs(HWND hwnd, CFtpFolder * pff, CFtpDir * pfd, IUnknown * punkSite)
{
    HRESULT hr = E_FAIL;
    CFtpSite * pfs = pfd->GetFtpSite();

    ASSERT(hwnd && pff);
    if (EVAL(pfs))
    {
        CAccounts cAccounts;
        TCHAR szServer[INTERNET_MAX_HOST_NAME_LENGTH];
        TCHAR szUser[INTERNET_MAX_USER_NAME_LENGTH];
        TCHAR szPassword[INTERNET_MAX_PASSWORD_LENGTH];
        LPCITEMIDLIST pidlPrevious = pfd->GetPidlReference();

        pfs->GetServer(szServer, ARRAYSIZE(szServer));
        pfs->GetUser(szUser, ARRAYSIZE(szUser));
        pfs->GetPassword(szPassword, ARRAYSIZE(szPassword));

        hr = cAccounts.DisplayLoginDialog(hwnd, LOGINFLAGS_DEFAULT, szServer, szUser, ARRAYSIZE(szUser), szPassword, ARRAYSIZE(szPassword));
        if (S_OK == hr)
        {
            LPITEMIDLIST pidlNew;

            ASSERT(pff->GetItemAllocatorDirect());
            hr = PidlReplaceUserPassword(pidlPrevious, &pidlNew, pff->GetItemAllocatorDirect(), szUser, szPassword);
            if (EVAL(SUCCEEDED(hr)))
            {
                CFtpSite * pfs;
                LPITEMIDLIST pidlRedirect;

                // We need to update the password in the site to redirect to the correct or new one.
                if (EVAL(SUCCEEDED(PidlReplaceUserPassword(pidlNew, &pidlRedirect, pff->GetItemAllocatorDirect(), szUser, TEXT(""))) &&
                         SUCCEEDED(SiteCache_PidlLookup(pidlRedirect, TRUE, pff->GetItemAllocatorDirect(), &pfs))))
                {
                    EVAL(SUCCEEDED(pfs->SetRedirPassword(szPassword)));
                    pfs->Release();
                    ILFree(pidlRedirect);
                }

                // pidl is a full private pidl.  pidlFull will be a full public pidl because
                // that's what the browser needs to get back from the root of THE public
                // name space back to and into us.
                LPITEMIDLIST pidlFull = pff->CreateFullPublicPidl(pidlNew);
                if (pidlFull)
                {
                    hr = IUnknown_PidlNavigate(punkSite, pidlFull, TRUE);
                    ILFree(pidlFull);
                }
                else
                    hr = E_FAIL;

                ILFree(pidlNew);
            }
        }
    }

    return hr;
}



HRESULT LoginAsViaFolder(HWND hwnd, CFtpFolder * pff, IUnknown * punkSite)
{
    HRESULT hr = E_FAIL;
    CFtpDir * pfd = pff->GetFtpDir();

    if (EVAL(pfd))
    {
        hr = LoginAs(hwnd, pff, pfd, punkSite);
        pfd->Release();
    }

    return hr;
}


#define PATH_IS_DRIVE(wzPath)      (-1 != PathGetDriveNumberW(wzPath))

HRESULT SHPathPrepareForWriteWrapW(HWND hwnd, IUnknown *punkEnableModless, LPCWSTR pwzPath, UINT wFunc, DWORD dwFlags)
{
    HRESULT hr;

    if (SHELL_VERSION_NT5 == GetShellVersion())
    {
        // NT5's version of the API is better.
        hr = _SHPathPrepareForWriteW(hwnd, punkEnableModless, pwzPath, dwFlags);
    }
    else
    {
        if (PATH_IS_DRIVE(pwzPath))
        {
            hr = (SHCheckDiskForMediaW(hwnd, punkEnableModless, pwzPath, wFunc) ? S_OK : E_FAIL);
        }
        else
        {
            if (PathIsUNCW(pwzPath))
            {
                hr = (PathFileExistsW(pwzPath) ? S_OK : E_FAIL);
            }
        }
    }

    return hr;
}


// Since the reg setting doesn't take effect until the browser is restarted,
// let's stuff the reg query into a static variable, so we don't hit the registry
// all the time.
BOOL IsWebBasedFtpEnabled()
{
    static int nWebBased = 42;

    if (nWebBased == 42)
    {
        nWebBased = SHRegGetBoolUSValue(SZ_REGKEY_FTPFOLDER, SZ_REGKEY_USE_OLD_UI, FALSE, FALSE) ? 1 : 0;
    }
    return (nWebBased ? TRUE : FALSE);
}
