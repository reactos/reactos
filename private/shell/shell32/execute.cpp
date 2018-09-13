#include "shellprv.h"
#include "shlexec.h"
#include <newexe.h>
#include <appmgmt.h>
#include "ids.h"
#include <shstr.h>
#include "pidl.h"
#include "fstreex.h"
#include "uemapp.h"
#include "views.h"      // for SHRunControlPanelEx
#include "control.h"    // for MakeCPLCommandLine, etc
#include <lmcons.h>     // for UNLEN (max username length), GNLEN (max groupname length), PWLEN (max password length)

#define DM_MISC     0           // miscellany

#define SZWNDCLASS          TEXT("WndClass")
#define SZTERMEVENT         TEXT("TermEvent")

#ifdef WINNT
typedef PSHCREATEPROCESSINFOW PSHCREATEPROCESSINFO;

// stolen from sdk\inc\winbase.h
#define LOGON_WITH_PROFILE              0x00000001

// from dllload.c...
STDAPI_(BOOL) DelayCreateProcessWithLogon(LPCWSTR pszUser,
                                          LPCWSTR pszDomain,
                                          LPCWSTR pszPassword,
                                          DWORD dwLogonFlags,
                                          LPCWSTR lpApplicationName,
                                          LPCWSTR lpCommandLine,
                                          DWORD dwCreationFlags,
                                          LPVOID lpEnvironment,
                                          LPCWSTR lpCurrentDirectory,
                                          LPSTARTUPINFOW lpStartupInfo,
                                          LPPROCESS_INFORMATION lpProcessInformation);
#endif //WINNT

//  ASSOCAPI
//  i would like to elim these with new Assoc APIs...
const TCHAR c_szConv[] = TEXT("ddeconv");
const TCHAR c_szDDEEvent[] = TEXT("ddeevent");

#ifdef WINNT
LPVOID lpfnWowShellExecCB = NULL;
#endif

// from dllload.c
extern "C" BOOL AllowSetForegroundWindow( DWORD dwProcId );

class CShellExecute {
public:

    CShellExecute();
    ~CShellExecute();

    void ExecuteNormal(LPSHELLEXECUTEINFO pei);
    BOOL Finalize(LPSHELLEXECUTEINFO pei);

#ifdef WINNT
    BOOL Init(PSHCREATEPROCESSINFO pscpi);
    void ExecuteProcess(void);
    BOOL Finalize(PSHCREATEPROCESSINFO pscpi);
#endif // WINNT

private:
    //
    // PRIVATE METHODS
    //

    // default inits
    HRESULT _Init(LPSHELLEXECUTEINFO pei);

    //  member init methods
    BOOL _InitAssociations(LPSHELLEXECUTEINFO pei);
    HRESULT _InitClassAssociations(LPCTSTR pszClass, HKEY hkClass);
    HRESULT _InitShellAssociations(LPCTSTR pszFile, LPCITEMIDLIST pidl);
    void _SetMask(ULONG fMask);
    void _SetWorkingDir(LPCTSTR pszIn);
    void _SetFile(LPCTSTR pszIn);
    void _SetFileAndUrl(LPCTSTR pszIn);
    BOOL _SetDDEInfo(void);
    HRESULT _SetDarwinCmdTemplate(void);
    BOOL _SetAppRunAsCmdTemplate(void);
    BOOL _SetCmdTemplate(void);
    BOOL _SetCommand(void);
    void _SetStartup(LPSHELLEXECUTEINFO pei);
    void _SetImageName(void);

    //  utility methods
    HRESULT _QueryString(ASSOCF flags, ASSOCSTR str, LPTSTR psz, DWORD cch);
    BOOL _CheckForRegisteredProgram(void);
    BOOL _ProcessErrorShouldTryExecCommand(DWORD err, HWND hwnd, BOOL fCreateProcessFailed);
    LPTSTR _BuildEnvironmentForNewProcess( LPCTSTR pszNewEnvString );
    void _FixActivationStealingApps(HWND hwndOldActive, int nShow);
    DWORD _GetCreateFlags(ULONG fMask);
    BOOL _Resolve(void);

    //  DDE stuff
#ifdef FEATURE_SHELLEXECCACHE
    void _CacheDDEWindowClass(HWND hwnd);
#endif // FEATURE_SHELLEXECCACHE
    HWND _GetConversationWindow(HWND hwndDDE);
    HWND _CreateHiddenDDEWindow(HWND hwndParent, HANDLE hDDEEvent);
    HGLOBAL _CreateDDECommand(int nShow, BOOL fLFNAware, BOOL fNative);
    void _DestroyHiddenDDEWindow(HWND hwnd);
    BOOL _TryDDEShortCircuit(HWND hwnd, HGLOBAL hMem, int nShow);
    BOOL _PostDDEExecute(HWND hwndConv,
                        HGLOBAL hDDECommand,
                        HANDLE hConversationDone,
                        BOOL fWaitForDDE,
                        HWND *phwndDDE);
    BOOL _DDEExecute(BOOL fWillRetry,
                    HWND hwndParent,
                    int   nShowCmd,
                    BOOL fWaitForDDE);

    // exec methods
    BOOL _TryHooks(LPSHELLEXECUTEINFO pei);
    BOOL _TryInProcess(LPSHELLEXECUTEINFO pei);
    BOOL _TryValidateUNC(LPTSTR pszFile, LPSHELLEXECUTEINFO pei);
    void _TryOpenExe(void);
    void _TryExecCommand(void);
    void _DoExecCommand(void);
    BOOL _TryExecDDE(void);
    BOOL _TryExecPidl(LPSHELLEXECUTEINFO pei);
    BOOL _DoExecPidl(LPSHELLEXECUTEINFO pei);
    BOOL _ShellExecPidl(LPSHELLEXECUTEINFO pei, LPITEMIDLIST pidlExec);

    //  uninit/error handling methods
    BOOL _Cleanup(BOOL fSucceeded);
    BOOL _FinalMapError(HINSTANCE UNALIGNED64 *phinst);
    BOOL _ReportWin32(DWORD err);
    BOOL _ReportHinst(HINSTANCE hinst);
    DWORD _MapHINSTToWin32Err(HINSTANCE se_err);
    HINSTANCE _MapWin32ErrToHINST(UINT errWin32);

    BOOL _ShouldRetryWithNewDarwinInfo(void);
#ifdef WINNT
    BOOL _TryWowShellExec(void);
    BOOL _ShouldRetryWithNewClassKey(void);
#endif //WINNT

    // BUGBUGTODO members that arent yet
    //  BUGBUGTODO _InvokeInProcExec
    //  BUGBUGTODO _ShellExecPidl

    //
    // PRIVATE MEMBERS
    //
    TCHAR _szFile[INTERNET_MAX_URL_LENGTH];
    TCHAR _szWorkingDir[MAX_PATH];
    TCHAR _szCommand[INTERNET_MAX_URL_LENGTH];
    TCHAR _szCmdTemplate[INTERNET_MAX_URL_LENGTH];
    TCHAR _szDDECmd[MAX_PATH];
    TCHAR _szImageName[MAX_PATH];
    TCHAR _szDarwinCmdTemplate[MAX_PATH];
    DWORD _dwCreateFlags;
    STARTUPINFO _startup;
    int _nShow;
    UINT _uConnect;
    PROCESS_INFORMATION _pi;

    //  used only within restricted scope
    //  to avoid stack usage;
    WCHAR _wszTemp[INTERNET_MAX_URL_LENGTH];
    TCHAR _szTemp[MAX_PATH];

    //  we always pass a UNICODE verb to the _pqa
    WCHAR       _wszVerb[MAX_PATH];
    LPCWSTR     _pszQueryVerb;

    LPCTSTR    _lpParameters;
    LPCTSTR    _lpClass;
    LPCTSTR    _lpTitle;
    LPCITEMIDLIST _lpID;
    ATOM       _aApplication;
    ATOM       _aTopic;
    LPITEMIDLIST _pidlGlobal;
    IQueryAssociations *_pqa;

    HWND _hwndParent;
    LPSECURITY_ATTRIBUTES _pProcAttrs;
    LPSECURITY_ATTRIBUTES _pThreadAttrs;
    HANDLE _hUserToken;

    //  error state
    HINSTANCE  _hInstance; // hinstance value should only be set with ReportHinst
    DWORD      _err;   //  win32 error value should only be set with ReportWin32

    // FLAGS
    BOOL _fNoUI;                         //  dont show any UI
    BOOL _fDoEnvSubst;                   // do environment substitution on paths
    BOOL _fUseClass;
    BOOL _fRetryExecute;                 // used after querying the class store or invoking darwin
    BOOL _fNoQueryClassStore;            // blocks calling darwins class store
    BOOL _fClassStoreOnly;
    BOOL _fIsUrl;                        //_szFile is actually an URL
    BOOL _fRunAs;                        // canonical runas verb was used indicating the desire for another user context
    BOOL _fActivateHandler;
    BOOL _fTryOpenExe;
    BOOL _fDDEInfoSet;
    BOOL _fDDEWait;
    BOOL _fNoExecPidl;
    BOOL _fNoResolve;                    // unnecessary to resolve this path
    BOOL _fAlreadyQueriedDarwin;         // have we already queried for a darwin base execution?
#ifdef WINNT
    BOOL _fAlreadyQueriedClassStore;     // have we already queried the NT5 class store?
#endif
    BOOL _fInheritHandles;
    BOOL _fIsNamespaceObject;            // is namespace object like ::{GUID}, must pidlexec
    BOOL _fWaitForInputIdle;
    BOOL _fUseNullCWD;                   // should we pass NULL as the lpCurrentDirectory param to _SHCreateProcess? 
};

CShellExecute::CShellExecute()
{
    TraceMsg(TF_SHELLEXEC, "SHEX::SHEX Created [%X]", this);
}

CShellExecute::~CShellExecute()
{
    TraceMsg(TF_SHELLEXEC, "SHEX::SHEX deleted [%X]", this);
}

void CShellExecute::_SetMask(ULONG fMask)
{
    _fDoEnvSubst = (fMask & SEE_MASK_DOENVSUBST);
    _fNoUI       = (fMask & SEE_MASK_FLAG_NO_UI);
    _fNoQueryClassStore = (fMask & SEE_MASK_NOQUERYCLASSSTORE);
    _fDDEWait = fMask & SEE_MASK_FLAG_DDEWAIT;
    _fWaitForInputIdle = fMask & SEE_MASK_WAITFORINPUTIDLE;
    _fUseClass   = _UseClassName(fMask) || _UseClassKey(fMask);

    _dwCreateFlags = _GetCreateFlags(fMask);
    _uConnect = fMask & SEE_MASK_CONNECTNETDRV ? VALIDATEUNC_CONNECT : 0;
    if (_fNoUI)
        _uConnect |= VALIDATEUNC_NOUI;

    // BUGBUG (scotth): somebody should explain why these fMask flags
    // must be off for this condition to pass.

    // PARTIAANSWER (reinerf): the SEE_MASK_FILEANDURL has to be off
    // so we can wait until we find out what the associated App is and query
    // to find out whether they want the the cache filename or the URL name passed
    // on the command line.
#define NOEXECPIDLMASK   (SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_DDEWAIT | SEE_MASK_FORCENOIDLIST | SEE_MASK_FILEANDURL)
    _fNoExecPidl = BOOLIFY(fMask & NOEXECPIDLMASK);
}

HRESULT CShellExecute::_Init(LPSHELLEXECUTEINFO pei)
{
    TraceMsg(TF_SHELLEXEC, "SHEX::_Init()");

    _SetMask(pei->fMask);

    _lpParameters= pei->lpParameters;
    _lpID        = (LPITEMIDLIST)(_UseIDList(pei->fMask) ? pei->lpIDList : NULL);
    _lpTitle     = _UseTitleName(pei->fMask) ? pei->lpClass : NULL;


    //  default to TRUE;
    _fActivateHandler = TRUE;

    if (pei->lpVerb && *(pei->lpVerb))
    {
        SHTCharToUnicode(pei->lpVerb, _wszVerb, SIZECHARS(_wszVerb));
        _pszQueryVerb = _wszVerb;

        if (0 == lstrcmpi(pei->lpVerb, TEXT("runas")))
            _fRunAs = TRUE;
    }

    _hwndParent = pei->hwnd;

    pei->hProcess = 0;

    _nShow = pei->nShow;

    //  initialize the startup struct
    _SetStartup(pei);

    return S_OK;
}

void CShellExecute::_SetWorkingDir(LPCTSTR pszIn)
{
        //  if we were given a directory, we attempt to use it
    if (pszIn && *pszIn)
    {
        StrCpyN(_szWorkingDir, pszIn, SIZECHARS(_szWorkingDir));
        if (_fDoEnvSubst)
            DoEnvironmentSubst(_szWorkingDir, SIZECHARS(_szWorkingDir));

        //
        // if the passed directory is not valid (after env subst) dont
        // fail, act just like Win31 and use whatever the current dir is.
        //
        // Win31 is stranger than I could imagine, if you pass ShellExecute
        // an invalid directory, it will change the current drive.
        //
        if (!PathIsDirectory(_szWorkingDir))
        {
            if (PathGetDriveNumber(_szWorkingDir) >= 0)
            {
                TraceMsg(TF_SHELLEXEC, "SHEX::_SetWorkingDir() bad directory %s, using %c:", _szWorkingDir, _szWorkingDir[0]);
                PathStripToRoot(_szWorkingDir);
            }
            else
            {
                TraceMsg(TF_SHELLEXEC, "SHEX::_SetWorkingDir() bad directory %s, using current dir", _szWorkingDir);
                GetCurrentDirectory(SIZECHARS(_szWorkingDir), _szWorkingDir);
            }
        }
        else
        {
            return;
        }
    }
    else
    {
        // if we are doing a SHCreateProcessAsUser or a normal shellexecute w/ the "runas" verb, and
        // the caller passed NULL for lpCurrentDirectory then we we do NOT want to fall back and use 
        // the CWD because the newly logged on user might not have permissions in the current users CWD.
        // We will have better luck just passing NULL and letting the OS figure it out.
        if (_fRunAs)
        {
            _fUseNullCWD = TRUE;
            return;
        }
        else
        {
            GetCurrentDirectory(SIZECHARS(_szWorkingDir), _szWorkingDir);
        }
    }

    //  there are some lame cases where even CD is bad.
    //  and CreateProcess() will then fail.
    if (!PathIsDirectory(_szWorkingDir))
    {
        GetWindowsDirectory(_szWorkingDir, SIZECHARS(_szWorkingDir));
    }

    TraceMsg(TF_SHELLEXEC, "SHEX::_SetWorkingDir() pszIn = %s, NewDir = %s", pszIn, _szWorkingDir);

}

inline BOOL _IsNamespaceObject(LPCTSTR psz)
{
    return (psz[0] == L':' && psz[1] == L':' && psz[2] == L'{');
}

void CShellExecute::_SetFile(LPCTSTR pszIn)
{
    if (pszIn && pszIn[0])
    {
        PARSEDURL pu;

        TraceMsg(TF_SHELLEXEC, "SHEX::_SetFileName() Entered pszIn = %s", pszIn);

        // Is this a URL and is the protocol scheme "file:"?

        pu.cbSize = SIZEOF(pu);
        _fIsUrl = SUCCEEDED(ParseURL(pszIn, &pu));
        if (_fIsUrl && URL_SCHEME_FILE == pu.nScheme)
        {
            //  BUGBUG - we lose local anchors on any dospaths - zekel - 5-Feb-97
            //  if an URL comes thru as "file:///c:/foo/bar.htm#fragment"
            //  the "#fragment" will be discarded.  right now
            //  file urls with fragments are completely busted,
            //  so only losing the fragment would still be a serious improvement.
            //  i have avoided saving the fragment since there is a lot of
            //  path mucking that happens here.

            DWORD cchPath = SIZECHARS(_szFile);
            PathCreateFromUrl(pszIn, _szFile, &cchPath, 0);

            TraceMsg(TF_SHELLEXEC, "SHEX::_SetFileName() Translated local URL to \"%s\"", _szFile);

            //
            //  WARNING:  In IE4 we left the fIsUrl set to true even though it is now a path.
            //  I dont think that this was utilized, so i am now setting it to false
            _fIsUrl = FALSE;
        }
        else
        {
            StrCpyN(_szFile, pszIn, SIZECHARS(_szFile));
            _fIsNamespaceObject = (!_lpID && _IsNamespaceObject(_szFile));
        }

        if (_fDoEnvSubst)
            DoEnvironmentSubst(_szFile, SIZECHARS(_szFile));

    }
    else
    {
        //  LEGACY - to support shellexec() of directories.
        if (!_lpID)
            StrCpyN(_szFile, _szWorkingDir, SIZECHARS(_szFile));
    }

    TraceMsg(TF_SHELLEXEC, "SHEX::_SetFileName() exit:  szFile = %s", _szFile);

}

void CShellExecute::_SetFileAndUrl(LPCTSTR pszIn)
{
    TraceMsg(TF_SHELLEXEC, "SHEX::_SetFileAndUrl() enter:  pszIn = %s", pszIn);

    if (SUCCEEDED(_QueryString(0, ASSOCSTR_EXECUTABLE, _szTemp, SIZECHARS(_szTemp)))
    &&  DoesAppWantUrl(_szTemp))
    {
        // our lpFile points to a string that contains both an Internet Cache
        // File location and the URL name that is associated with that cache file
        // (they are seperated by a single NULL). The application that we are
        // about to execute wants the URL name instead of the cache file, so
        // use it instead.
        int iLength = lstrlen(pszIn);
        LPCTSTR pszUrlPart = &pszIn[iLength + 1];

        if (IsBadStringPtr(pszUrlPart, INTERNET_MAX_URL_LENGTH) || !PathIsURL(pszUrlPart))
        {
            ASSERT(FALSE);
        }
        else
        {
            // we have a vaild URL, so use it
            lstrcpy(_szFile, pszUrlPart);
        }
    }
    TraceMsg(TF_SHELLEXEC, "SHEX::_SetFileAndUrl() exit: szFile = %s",_szFile);

}

//
//  _TryValidateUNC() has queer return values
//
BOOL CShellExecute::_TryValidateUNC(LPTSTR pszFile, LPSHELLEXECUTEINFO pei)
{
    HRESULT hr = S_FALSE;
    BOOL fRet = FALSE;

    if (PathIsUNC(pszFile))
    {
        TraceMsg(TF_SHELLEXEC, "SHEX::_TVUNC Is UNC: %s", pszFile);
        // Notes:
        //  SHValidateUNC() returns FALSE if it failed. In such a case,
        //   GetLastError will gives us the right error code.
        //
        if (!SHValidateUNC(_hwndParent, pszFile, _uConnect))
        {
            hr = E_FAIL;
            // Note that SHValidateUNC calls SetLastError() and we need
            // to preserve that so that the caller makes the right decision
            DWORD err = GetLastError();

            if (ERROR_CANCELLED == err)
            {
                // Not a print share, use the error returned from the first call
                // _ReportWin32(ERROR_CANCELLED);
                //  we dont need to report this error, it is the callers responsibility
                //  the caller should GetLastError() on E_FAIL and do a _ReportWin32()
                TraceMsg(TF_SHELLEXEC, "SHEX::_TVUNC FAILED with ERROR_CANCELLED");
            }
            else if (pei)
            {
                // Now check to see if it's a print share, if it is, we need to exec as pidl
                // Note: This call will not display "connect ui" because SHValidateUNC
                // uses the CONNECT_CURRENT_MEDIA flag for VALIDATEUNC_PRINT.
                if (SHValidateUNC(_hwndParent, pszFile, _uConnect | VALIDATEUNC_PRINT))
                {
                    hr = S_OK;
                    TraceMsg(TF_SHELLEXEC, "SHEX::TVUNC found print share");
                }
                else
                    // need to reset the orginal error ,cuz SHValidateUNC() has set it again
                    SetLastError(err);

            }
        }
        else
        {
            TraceMsg(TF_SHELLEXEC, "SHEX::_TVUNC UNC is accessible");
        }
    }

    TraceMsg(TF_SHELLEXEC, "SHEX::_TVUNC exit: hr = %X", hr);

    switch (hr)
    {
//  S_FALSE    pszFile is not a UNC or is a valid UNC according to the flags
//  S_OK       pszFile is a valid UNC to a print share
//  E_FAIL     pszFile is a UNC but cannot be validated use GetLastError() to get the real error
        case S_OK:
            //  we got a good UNC
            if(_DoExecPidl(pei))
            {
                //  we got the pidl, whether or not we could use it,
                //  so we drop through and goto Quit
                fRet = TRUE;
            }
            //  if we dont get a pidl we just try something else.
            break;

        case E_FAIL:
            _ProcessErrorShouldTryExecCommand(GetLastError(), _hwndParent, FALSE);
            //  never retry since we didnt try in the first place.
            fRet = TRUE;

        // case S_FALSE: Dropthrough
        default:
            break;
    }

    return fRet;
}


BOOL CShellExecute::_ShellExecPidl(LPSHELLEXECUTEINFO pei, LPITEMIDLIST pidlExec)
{
    HRESULT hres = E_OUTOFMEMORY;

    // I need a copy so that the bind can modify the IDList
    LPITEMIDLIST pidl = ILClone(pidlExec);

    if (pidl)
    {
        LPCITEMIDLIST pidlLast;
        IShellFolder *psf;

        hres = SHBindToIDListParent(pidl, IID_IShellFolder, (LPVOID *)&psf, &pidlLast);
        if (SUCCEEDED(hres))
        {
            IContextMenu *pcm;

            hres = psf->GetUIObjectOf(pei->hwnd, 1, &pidlLast, IID_IContextMenu, NULL, (LPVOID *)&pcm);
            if (SUCCEEDED(hres))
            {
                //  BUGBUGTODO - need to convert InvokeInProcExec to a member function
                hres = InvokeInProcExec(pcm, pei, NULL);

                pcm->Release();
            }
            psf->Release();
        }

        ILFree(pidl);
    }

    if (FAILED(hres))
    {
        // BUGBUG could we have better error mapping here? - zekel - 22-JAN-98
        //  are there any meaningful HRESULTs for ShellExec?
        switch (hres) {
        case E_OUTOFMEMORY:
            _ReportWin32(ERROR_NOT_ENOUGH_MEMORY);
            break;

        default:
            _ReportWin32(ERROR_ACCESS_DENIED);
            break;
        }
    }

    TraceMsg(TF_SHELLEXEC, "SHEX::_ShellExecPidl() exiting hres = %X", hres);

    return(SUCCEEDED(hres));
}

//
//  BUGBUGTODO CShellExecute::_ShellExecPidl() needs to be added probably, but right now
//  we work around it being all alone.
//

//
//  BOOL CShellExecute::_DoExecPidl(LPSHELLEXECUTEINFO pei)
//
//  returns TRUE if a pidl was created, FALSE otherwise
//
BOOL CShellExecute::_DoExecPidl(LPSHELLEXECUTEINFO pei)
{
    TraceMsg(TF_SHELLEXEC, "SHEX::_DoExecPidl enter: szFile = %s", _szFile);

    //BUGBUG simple PIDL?
    LPITEMIDLIST pidl;
    pidl = ILCreateFromPath(_szFile);
    if (pidl)
    {

#ifdef DEBUG
        static int panic=0;
        ASSERT(panic==0);
        panic++;
#endif

        //
        //  if _ShellExecPidl() FAILS, it does
        //  Report() for us
        //
        _ShellExecPidl(pei, pidl);

        ILFree(pidl);

#ifdef DEBUG
        panic--;
        ASSERT(panic==0);
#endif
    }
    else
    {
        TraceMsg(TF_SHELLEXEC, "SHEX::_DoExecPidl() unhandled cuz ILCreateFromPath() failed");

        return FALSE;
    }
    return TRUE;
}


/*----------------------------------------------------------
Purpose: This function looks up the given file in "HKLM\Software\
         Microsoft\Windows\CurrentVersion\App Paths" to
         see if it has an absolute path registered.

Returns: TRUE if the file has a registered path
         FALSE if it does not or if the provided filename has
               a relative path already


Cond:    !! Side effect: the szFile field may be changed by
         !! this function.

*/
BOOL CShellExecute::_CheckForRegisteredProgram(void)
{
    TCHAR szTemp[MAX_PATH];
    TraceMsg(TF_SHELLEXEC, "SHEX::CFRP entered");

    // Only supported for files with no paths specified
    if (PathFindFileName(_szFile) != _szFile)
        return FALSE;

    if (PathToAppPath(_szFile, szTemp))
    {
        TraceMsg(TF_SHELLEXEC, "SHEX::CFRP Set szFile = %s", szTemp);

        StrCpy(_szFile, szTemp);
        return TRUE;
    }

    return FALSE;
}

BOOL CShellExecute::_Resolve(void)
{
    // No; get the fully qualified path and add .exe extension
    // if needed
    LPCTSTR rgszDirs[2] =  { _szWorkingDir, NULL };
    const UINT uFlags = PRF_VERIFYEXISTS | PRF_TRYPROGRAMEXTENSIONS | PRF_FIRSTDIRDEF;

    // if the Path is not an URL
    // and the path cant be resolved
    //
    //  PathResolve() now does SetLastError() when we pass VERIFYEXISTS
    //  this means that we can be assure if all these tests fail
    //  that LastError is set.
    //
    if (!_fNoResolve && !_fIsUrl && !_fIsNamespaceObject &&
        !PathResolve(_szFile, rgszDirs, uFlags))
    {
        //  _CheckForRegisteredProgram() changes _szFile if
        //  there is a registered program in the registry
        //  so we recheck to see if it exists.
        if (!_CheckForRegisteredProgram() ||
             !PathResolve(_szFile, rgszDirs, uFlags))
        {
            // No; file not found, bail out
            //
            //  WARNING LEGACY - we must return ERROR_FILE_NOT_FOUND - ZekeL - 14-APR-99
            //  some apps, specifically Netscape Navigator 4.5, rely on this
            //  failing with ERROR_FILE_NOT_FOUND.  so even though PathResolve() does
            //  a SetLastError() to the correct error we cannot propagate that up
            //
            _ReportWin32(ERROR_FILE_NOT_FOUND);
            ASSERT(_err);
            TraceMsg(TF_SHELLEXEC, "SHEX::TryExecPidl FAILED %d", _err);

            return FALSE;
        }
    }

    return TRUE;
}


/*----------------------------------------------------------
Purpose: decide whether it is appropriate to TryExecPidl()

Returns: S_OK        if it should _DoExecPidl()
         S_FALSE     it shouldnt _DoExecPidl()
         E_FAIL      ShellExec should quit  Report*() has the real error


Cond:    !! Side effect: the szFile field may be changed by
         !! this function.

*/
BOOL CShellExecute::_TryExecPidl(LPSHELLEXECUTEINFO pei)
{
    TraceMsg(TF_SHELLEXEC, "SHEX::TryExecPidl entered szFile = %s", _szFile);
    BOOL fInvokeIdList = _InvokeIDList(pei->fMask);

    //
    // If we're explicitly given a class then we don't care if the file exists.
    // Just let the handler for the class worry about it, and _TryExecPidl()
    // will return the default of FALSE.
    //

    if ( *_szFile &&
         (!_fUseClass || fInvokeIdList || _fIsNamespaceObject) )
    {
        if (!_fNoResolve && !_Resolve())
            return TRUE;

        // The optimal execution path is to check for the default
        // verb and exec the pidl.  It is smarter than all this path
        // code (it calls the context menu handlers, etc...)

        if ((!_pszQueryVerb && !(_fNoExecPidl))
        ||  _fIsUrl                  //  BUGBUG - i dont think this is used
        ||  fInvokeIdList            //  caller told us to!
        ||  _fIsNamespaceObject      //  namespace objects can only be invoked through pidls
        ||  PathIsShortcut(_szFile)) //  to support LNK files and soon URL files
        {
            //  this means that we can tryexecpidl
            TraceMsg(TF_SHELLEXEC, "SHEX::TryExecPidl() succeeded now TEP()");

            return _DoExecPidl(pei);
        }
    }

    TraceMsg(TF_SHELLEXEC, "SHEX::TryExecPidl dont bother");

    return FALSE;
}

HRESULT CShellExecute::_InitClassAssociations(LPCTSTR pszClass, HKEY hkClass)
{
    TraceMsg(TF_SHELLEXEC, "SHEX::InitClassAssoc enter: lpClass = %s, hkClass = %X", pszClass, hkClass);

    HRESULT hr = AssocCreate(CLSID_QueryAssociations, IID_IQueryAssociations, (LPVOID*)&_pqa);
    if (SUCCEEDED(hr))
    {
        if (hkClass)
        {
            hr = _pqa->Init(0, NULL, hkClass, NULL);
        }
        else if (pszClass)
        {
            SHTCharToUnicode(pszClass, _wszTemp, SIZECHARS(_wszTemp));
            hr = _pqa->Init(0, _wszTemp, NULL, NULL);
        }
        else
        {
            //  LEGACY - they didnt pass us anything to go on so we default to folder
            //  because of the chaos of the original shellexec() we didnt even notice
            //  when we had nothing to be associated with, and just used
            //  our base key, which turns out to be explorer.
            //  this permitted ShellExecute(NULL, "explore", NULL, NULL, NULL, SW_SHOW);
            //  to succeed.  in order to support this, we will fall back to it here.
            hr = _pqa->Init(0, L"Folder", NULL, NULL);
        }
    }

    return hr;
}

HRESULT CShellExecute::_InitShellAssociations(LPCTSTR pszFile, LPCITEMIDLIST pidl)
{
    TraceMsg(TF_SHELLEXEC, "SHEX::InitShellAssoc enter: pszFile = %s, pidl = %X", pszFile, pidl);

    HRESULT hr;
    LPITEMIDLIST pidlFree = NULL;
    if (*pszFile)
    {
        hr = SHILCreateFromPath(pszFile, &pidlFree, NULL);

        if (SUCCEEDED(hr))
            pidl = pidlFree;
    }
    else if (pidl)
    {
        // Other parts of CShellExecute expect that _szFile is
        // filled in, so we may as well do it here.
        SHGetNameAndFlags(pidl, SHGDN_FORPARSING, _szFile, SIZECHARS(_szFile), NULL);
        _fNoResolve = TRUE;
    }

    if (pidl)
    {
        hr = SHGetAssociations(pidl, (LPVOID *)&_pqa);

        // NOTE: sometimes we can have the extension or even the progid in the registry, but there
        // is no "shell" subkey. An example of this is for .xls files in NT5: the index server guys
        // create HKCR\.xls and HKCR\Excel.Sheet.8 but all they put under Excel.Sheet.8 is the clsid.
        //
        //  so we need to check and make sure that we have a valid command value for
        //  this object.  if we dont, then that means that this is not valid
        //  class to shellexec with.  we need to fall back to the Unknown key
        //  so that we can query the Darwin/NT5 ClassStore and/or
        //  show the openwith dialog box.
        //
        DWORD cch;
        if (FAILED(hr) ||
        (FAILED(_pqa->GetString(0, ASSOCSTR_COMMAND, _pszQueryVerb, NULL, &cch))
        && FAILED(_pqa->GetData(0, ASSOCDATA_MSIDESCRIPTOR, _pszQueryVerb, NULL, &cch))))

        {
            if (!_pqa)
                hr = AssocCreate(CLSID_QueryAssociations, IID_IQueryAssociations, (LPVOID*)&_pqa);

            if (_pqa)
            {
                hr = _pqa->Init(0, L"Unknown", NULL, NULL);

                //  this allows us to locate something
                //  in the class store, but restricts us
                //  from using the openwith dialog if the
                //  caller instructed NOUI
                if (SUCCEEDED(hr) && _fNoUI)
                    _fClassStoreOnly = TRUE;
            }
        }

    }
    else
    {
        LPCTSTR pszExt = PathFindExtension(_szFile);
        if (*pszExt)
            hr = _InitClassAssociations(pszExt, NULL);

        RIPMSG(hr != S_OK, "SHEX::InitAssoc parsing failed, but there is a valid association for *.%s", pszExt);
    }

    if (pidlFree)
        ILFree(pidlFree);

    return hr;
}

BOOL CShellExecute::_InitAssociations(LPSHELLEXECUTEINFO pei)
{
    HRESULT hr;
    if (_fUseClass || (!_szFile[0] && !_lpID))
    {
        ASSERT(pei);
        hr = _InitClassAssociations(pei->lpClass, pei->hkeyClass);
    }
    else
    {
        hr = _InitShellAssociations(_szFile, _lpID);
    }

    TraceMsg(TF_SHELLEXEC, "SHEX::InitAssoc return %X", hr);

    if (FAILED(hr))
    {
        if (PathIsExe(_szFile))
            _fTryOpenExe = TRUE;
        else
            _ReportWin32(ERROR_NO_ASSOCIATION);
    }

    return SUCCEEDED(hr);
}

void CShellExecute::_TryOpenExe(void)
{
    //
    //  this is the last chance that a file will have
    //  we shouldnt even be here in any case
    //  unless the registry has been thrashed, and
    //  the exe classes are all deleted from HKCR
    //
    ASSERT(PathIsExe(_szFile));

    // even with no association, we know how to open an executable
    if ((!_pszQueryVerb || !StrCmpIW(_pszQueryVerb, L"open")))
    {
        //  _SetCommand() by hand here...

        // NB WinExec can handle long names so there's no need to convert it.
        StrCpy(_szCommand, _szFile);

        //
        // We need to append the parameter
        //
        if (_lpParameters && *_lpParameters)
        {
            StrCatBuff(_szCommand, c_szSpace, ARRAYSIZE(_szCommand));
            StrCatBuff(_szCommand, _lpParameters, ARRAYSIZE(_szCommand));
        }

        TraceMsg(TF_SHELLEXEC, "SHEX::TryOpenExe() command = %s", _szCommand);

        //  _TryExecCommand() sets the fSucceeded if appropriate
        _DoExecCommand();
    }
    else
    {
        TraceMsg(TF_SHELLEXEC, "SHEX::TryOpenExe() wrong verb");
        _ReportWin32(ERROR_INVALID_PARAMETER);
    }
}

BOOL CShellExecute::_ProcessErrorShouldTryExecCommand(DWORD err, HWND hwnd, BOOL fCreateProcessFailed)
{
    BOOL fRet = FALSE;

    //  insure that we dont lose this error.
    BOOL fNeedToReport = TRUE;

    TraceMsg(TF_SHELLEXEC, "SHEX::PESTEC() enter : err = %d", err);

    // special case some error returns
    switch (err)
    {
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
    case ERROR_BAD_PATHNAME:
    case ERROR_INVALID_NAME:
        if ((_szCmdTemplate[0] != TEXT('%')) && fCreateProcessFailed)
        {
            UINT uAppType = LOWORD(GetExeType(_szImageName));
            if ((uAppType == NEMAGIC))
            {
                //
                // PK16FNF only applies to 16bit modules, and only when it was an
                // implicit DLL load failure (ie, the szImageName exists).
                // this is a undoc'd kernel API that returns a dll
                // name if CreateProcess() failed because of a missing
                // dll.  this is only used in 16bit.  otherwise, kernel32
                // puts up the dialog, according to ChrisG.
                //
                PK16FNF(_szImageName);
                if (_szImageName[0])
                {
                    // do the message here so that callers of us won't need
                    // to deal with 32bit and 16bit apps separately
                    ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_CANTFINDCOMPONENT), NULL,
                                    MB_OK|MB_ICONEXCLAMATION | MB_SETFOREGROUND, _szImageName, _szFile);
                    _ReportWin32(ERROR_DLL_NOT_FOUND);
                    fNeedToReport = FALSE;
                }
            }
            else if (uAppType != PEMAGIC && !_fNoUI)   // ie, it was not found
            {
                INT iret;
                HKEY hk;

                if (_pqa)
                    _pqa->GetKey(0, ASSOCKEY_CLASS, NULL, &hk);
                else
                    hk = NULL;

                //
                // have user help us find missing exe
                //
                iret  = FindAssociatedExe(hwnd, _szCommand, _szFile, hk);

                if (hk)
                    RegCloseKey(hk);

                //
                //  We infinitely retry until either the user cancel it
                // or we find it.
                //
                if (iret == -1)
                {
                    fRet = TRUE;
                    TraceMsg(TF_SHELLEXEC, "SHEX::PESTEC() found new exe");
                }
                else
                    _ReportWin32(ERROR_CANCELLED);

                //  either way we dont need to report this error
                fNeedToReport = FALSE;
            }
        }
        break;

    case ERROR_SINGLE_INSTANCE_APP:

        // REVIEW: first we should search for windows with szFile in
        // their title (maybe sans the extension).  if that fails then
        // we should look for the exe that we tried to run (as we do now)

        // try to activate it. it would be nice if we could pass it params too...
        PathRemoveArgs(_szCommand);                   // strip off the params
        HWND hwndOld = _FindPopupFromExe(_szCommand);    // find the exe
        TraceMsg(TF_SHELLEXEC, "Single instance exe (%s), activating hwnd (%x)", (LPTSTR)_szCommand, hwndOld);
        if (hwndOld) {
            SwitchToThisWindow(hwndOld, TRUE);
            // Success - try to get it's hinstance.
            _ReportHinst(Window_GetInstance(hwndOld));
            fNeedToReport = FALSE;

            TraceMsg(TF_SHELLEXEC, "SHEX::PESTEC() found single instance app");

        }
        break;

    } // switch (errWin32)


    if (fNeedToReport)
        _ReportWin32(err);

    TraceMsg(TF_SHELLEXEC, "SHEX::PESTEC() return %d", fRet);

    return fRet;
}

void CShellExecute::_SetStartup(LPSHELLEXECUTEINFO pei)
{
    // Was zero filled by Alloc...
    ASSERT(!_startup.cb);
    _startup.cb = SIZEOF(_startup);
    _startup.dwFlags |= STARTF_USESHOWWINDOW;
    _startup.wShowWindow = (WORD) pei->nShow;
    _startup.lpTitle = (LPTSTR)_lpTitle;

#ifdef WINNT
    if ( pei->fMask & SEE_MASK_RESERVED )
    {
        _startup.lpReserved = (LPTSTR)pei->hInstApp;
    }

    if (pei->fMask & SEE_MASK_HASLINKNAME)
    {
        _startup.dwFlags |= STARTF_TITLEISLINKNAME;
    }
#endif

    if (pei->fMask & SEE_MASK_HOTKEY)
    {
        _startup.hStdInput = (HANDLE)(pei->dwHotKey);
        _startup.dwFlags |= STARTF_USEHOTKEY;
    }


// Multi-monitor support (dli) pass a hMonitor to createprocess

#ifndef STARTF_HASHMONITOR
#define STARTF_HASHMONITOR       0x00000400  // same as HASSHELLDATA
#endif

    if (pei->fMask & SEE_MASK_ICON) {
        _startup.hStdOutput = (HANDLE)pei->hIcon;
        _startup.dwFlags |= STARTF_HASSHELLDATA;
    }
    else if (pei->fMask & SEE_MASK_HMONITOR)
    {
        _startup.hStdOutput = (HANDLE)pei->hMonitor;
        _startup.dwFlags |= STARTF_HASHMONITOR;
    }
    else if (pei->hwnd)
    {
        _startup.hStdOutput = (HANDLE)MonitorFromWindow(pei->hwnd,MONITOR_DEFAULTTONEAREST);
        _startup.dwFlags |= STARTF_HASHMONITOR;
    }
    TraceMsg(TF_SHELLEXEC, "SHEX::SetStartup() called");

}




ULONG _GetEnvSizeAndFindString( LPTSTR pszEnv, LPCTSTR pFindString, LPTSTR *ppFoundString )
{
    LPTSTR psz;
    ULONG FindStringLen = 0;

    if ( pFindString )
        FindStringLen = lstrlen( pFindString );


    for (psz = pszEnv; *psz; psz += lstrlen(psz)+1)
    {
        // Well lets try to use the CompareString function to find
        // out if we found the Path... Note return of 2 is equal...

        if ( pFindString && (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, psz,
                FindStringLen, pFindString, FindStringLen) == CSTR_EQUAL) && (*(psz+FindStringLen) == TEXT('=')))
        {
            // We found the string
            *ppFoundString = psz;
        }
    }

    return (ULONG)(psz - pszEnv) + 1;
}


BOOL _AddEnvVariable( LPTSTR *ppszEnv, LPCTSTR StringToAdd )
{
    ULONG NewEnvSize;
    LPTSTR psz;
    LPTSTR pszNewEnv;
    ULONG NewStringLen;
    ULONG NewStringNameLen;
    int nCompareResult;
    TCHAR t;


    psz = (LPTSTR)StringToAdd;
    while ( (t = *psz) && (TEXT('=') != t) )
    {
        psz++;
    }

    if ( NULL == t )
    {
        // This should not happen - it's an invalid string format
        Assert( t == TEXT('=') );
        return FALSE;
    }

    NewStringNameLen = (ULONG)(psz - StringToAdd);
    NewStringLen = lstrlen(StringToAdd);

    NewEnvSize = _GetEnvSizeAndFindString( *ppszEnv, NULL, NULL );

    pszNewEnv = (LPTSTR)LocalAlloc( LPTR, (NewEnvSize + NewStringLen + 1) * sizeof(TCHAR) );

    if (!pszNewEnv)
        return FALSE;


    // Since these need to be in order, find the right place to stick it

    psz = *ppszEnv;
    while ( *psz && (CSTR_LESS_THAN == (nCompareResult = CompareString( LOCALE_SYSTEM_DEFAULT,
            NORM_IGNORECASE, StringToAdd, NewStringNameLen, psz, NewStringNameLen)) ) )
    {
        // Get to next string
        psz += lstrlen(psz) + 1;
    }

    // At this point, we're either at the end, or we found a var that's "greater than" this one,
    // or we found a match (ie: it existed already) so we insert here in any case.

    // First, copy all the strings before ours
    CopyMemory(
        pszNewEnv,
        *ppszEnv,
        (PBYTE)psz - (PBYTE)*ppszEnv
        );

    // Now bring in the new string
    lstrcpy( pszNewEnv + (psz - *ppszEnv), StringToAdd );

    // Now copy whatever's left

    // If the thing found was a match, we have to skip it
    if ( *psz && (nCompareResult == CSTR_EQUAL) && (TEXT('=') == *(psz + NewStringNameLen) ) )
    {
        // It was an exact match - skip it.
        psz += lstrlen( psz ) + 1;
    }

    // Now were either at the end of the block, or there's more strings
    if ( *psz )
    {
        // There's more
        CopyMemory(
            pszNewEnv + (psz - *ppszEnv) + (NewStringLen + 1),
            psz,
            _GetEnvSizeAndFindString( psz, NULL, NULL ) * sizeof(TCHAR)
            );
    }


    // The king is dead.  Long live the king.
    LocalFree( *ppszEnv );
    *ppszEnv = pszNewEnv;


    return TRUE;
}



LPTSTR CShellExecute::_BuildEnvironmentForNewProcess( LPCTSTR pszNewEnvString )
{
    LPTSTR pszNewEnv = NULL;
    DWORD cbTemp = SIZEOF(_szTemp);
    LPTSTR pszEnv = GetEnvBlock(_hUserToken);
    int cchOldEnv;
    int cchNewEnv = 0;


    // Use the _szTemp variable of pseem to build key to the programs specific
    // key in the registry as well as other things...
    PathToAppPathKey(_szImageName, _szTemp, SIZECHARS(_szTemp));

    // Currently only clone environment if we have path.
    if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, _szTemp, TEXT("PATH"), NULL, _szTemp, &cbTemp))
    {
        LPTSTR psz;
        LPTSTR pszPath = NULL;
        int cchT = lstrlen(c_szPATH);

        // We need to figure out how big an environment we have.
        // While we are at it find the path environment var...

        cchOldEnv = _GetEnvSizeAndFindString( pszEnv, (LPTSTR)c_szPATH, &pszPath );


        // Now lets allocate some memory to create a new environment from.

        // BUGBUG (DavePl) Why 10 and not 11?  Or 9?
        // Comment from BobDay: 2 of the 10 come from nul terminators of the
        //   pseem->_szTemp and cchT strings added on.  The additional space might
        //   come from the fact that 16-bit Windows used to pass around an
        //   environment block that had some extra stuff on the end.  The extra
        //   stuff had things like the path name (argv[0]) and a nCmdShow value.

        cchNewEnv = (cchOldEnv + lstrlen(_szTemp) + cchT + 10);
        pszNewEnv = (LPTSTR)LocalAlloc(LPTR, cchNewEnv * SIZEOF(TCHAR));

        if (pszNewEnv == NULL)
        {
            FreeEnvBlock(_hUserToken, pszEnv);
            return(NULL);
        }

        if (pszPath)
        {
            // We found a path from before, calc how many bytes to copy
            // to start off with.  This should be up till the end of the
            // current path= var

            cchT = (int)(pszPath-pszEnv) + lstrlen(c_szPATH) + 1;
            hmemcpy(pszNewEnv, pszEnv, cchT * SIZEOF(TCHAR));
            psz = pszNewEnv + cchT;
            StrCpy(psz, _szTemp);
            psz += lstrlen(_szTemp);
            *psz++ = TEXT(';');  // add a ; between old path and new things

            // and copy in the rest of the stuff.
            hmemcpy(psz, pszEnv+cchT, (cchOldEnv-cchT) * SIZEOF(TCHAR));
        }
        else
        {
            //
            // Path not found so copy entire old environment down
            // And add PATH= and the end.
            //
            hmemcpy(pszNewEnv, pszEnv, cchOldEnv * SIZEOF(TCHAR));
            psz = pszNewEnv + cchOldEnv -1; // Before last trailing NULL
            StrCpy(psz, c_szPATH);
            StrCat(psz, SZEQUALS);
            StrCat(psz, _szTemp);

            // Add the Final Null for the end of the environment.
            *(psz + lstrlen(psz) + 1) = TEXT('\0');
        }

    }


    //
    // If there was a new string passed in, add it.
    //
    if ( pszNewEnvString )
    {

        //
        // Do we have to build a new env block from scratch?
        //
        if ( NULL == pszNewEnv )
        {
            cchNewEnv = _GetEnvSizeAndFindString( pszEnv, NULL, NULL );

            pszNewEnv = (LPTSTR)LocalAlloc( LPTR, cchNewEnv * sizeof(TCHAR) );
            if ( NULL == pszNewEnv )
            {
                Assert( pszNewEnv )
                return NULL;
            }

            CopyMemory( pszNewEnv, pszEnv, cchNewEnv * sizeof(TCHAR) );
        }

        //
        // We cannot just add ours to the end (they must be in order), so use
    	// this routine to add our var to the list - berniem 7/7/99
        //
        _AddEnvVariable( &pszNewEnv, pszNewEnvString );

    }
	
    FreeEnvBlock(_hUserToken, pszEnv);

    return(pszNewEnv);

}


// Some apps when run no-active steal the focus anyway so we
// we set it back to the previously active window.

void CShellExecute::_FixActivationStealingApps(HWND hwndOldActive, int nShow)
{
    HWND hwndNew;

    if (nShow == SW_SHOWMINNOACTIVE && (hwndNew = GetForegroundWindow()) != hwndOldActive && IsIconic(hwndNew))
        SetForegroundWindow(hwndOldActive);
}


//
//  The flags that need to passed to CreateProcess()
//
DWORD CShellExecute::_GetCreateFlags(ULONG fMask)
{
    DWORD dwFlags = 0;

#ifdef WINNT
    dwFlags |= CREATE_DEFAULT_ERROR_MODE;
    if ( fMask & SEE_MASK_FLAG_SEPVDM )
    {
        dwFlags |= CREATE_SEPARATE_WOW_VDM;
    }
#else
    dwFlags |= CREATE_NEW_PROCESS_GROUP | CREATE_DEFAULT_ERROR_MODE;
#endif // WINNT

#ifdef UNICODE
    dwFlags |= CREATE_UNICODE_ENVIRONMENT;
#endif

    if ( !(fMask & SEE_MASK_NO_CONSOLE))
    {
        dwFlags |= CREATE_NEW_CONSOLE;
    }

    return dwFlags;
}

//***   GetUEMAssoc -- approximate answer to 'is path an executable' (etc.)
// ENTRY/EXIT
//  pszFile     thing we asked to run (e.g. foo.xls)
//  pszImage    thing we ultimately ran (e.g. excel.exe)
int GetUEMAssoc(LPCTSTR pszFile, LPCTSTR pszImage)
{
    LPTSTR pszExt, pszExt2;

    // .exe's and associations come thru here
    // folders go thru ???
    // links go thru ResolveLink
    pszExt = PathFindExtension(pszFile);
    if (StrCmpIC(pszExt, c_szDotExe) == 0) {
        // only check .exe (assume .com, .bat, etc. are rare)
        return UIBL_DOTEXE;
    }
    pszExt2 = PathFindExtension(pszImage);
    // BUGBUG is StrCmpC (non-I, yes-C) o.k. here?  i think so since
    // all we really care about is that they don't match
    if (StrCmpC(pszExt, pszExt2) != 0) {
        TraceMsg(DM_MISC, "gua: UIBL_DOTASSOC file=%s image=%s", pszExt, pszExt2);
        return UIBL_DOTASSOC;
    }
    if (GetFileAttributes(pszFile) & FILE_ATTRIBUTE_DIRECTORY)
        return UIBL_DOTFOLDER;

    return UIBL_DOTOTHER;   // UIBL_DOTEXE?
}



#ifdef WINNT

typedef enum
{
    RUNAS_NORMAL                = 0x0000,   // the user explicityl choose the "Run as..." verb
    RUNAS_NONADMININSTALL       = 0x0001,   // "user is not an admin" and running a setup app
    RUNAS_HYDRANOINSTALLMODE    = 0x0002,   // (hyrdra only) machine is not in install mode and the user is running a setup app
} RUNAS_TYPE;

typedef struct {
    TCHAR szAppName[MAX_PATH];
    TCHAR szUser[UNLEN + 1];
    TCHAR szDomain[GNLEN + 1];
    TCHAR szPassword[PWLEN + 1];
    RUNAS_TYPE raType;
} LOGONINFO;


// this is the dialog that we display when we are on hydra and we are not in install mode,
// but we are launching a setup application
BOOL_PTR CALLBACK HydraNoInstallMode_DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LOGONINFO *pli= (LOGONINFO*)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pli = (LOGONINFO*)lParam;
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pli);

            Static_SetIcon(GetDlgItem(hDlg, IDD_ITEMICON), LoadIcon(NULL, MAKEINTRESOURCE(IDI_ERROR)));
            break;

        case WM_DESTROY:
        {
            HICON hIcon = (HICON)SendDlgItemMessage(hDlg, IDD_ITEMICON, STM_GETICON, 0, 0L);
            if (hIcon)
            {
                DestroyIcon(hIcon);
            }
        }
        break;

        case WM_NOTIFY:
            if (((LPNMHDR)lParam)->code == NM_CLICK || ((LPNMHDR)lParam)->code == NM_RETURN )
            {
                TCHAR szModule[MAX_PATH];
                if (GetSystemDirectory(szModule, ARRAYSIZE(szModule)))
                {
                    if (PathAppend(szModule, TEXT("appwiz.cpl")))
                    {                        
                        TCHAR szParam[1 + MAX_PATH + 2 + MAX_CCH_CPLNAME]; // See MakeCPLCommandLine function
                        TCHAR szAppwiz[64];

                        LoadString(g_hinst, IDS_APPWIZCPL, szAppwiz, ARRAYSIZE(szAppwiz));
                        MakeCPLCommandLine(szModule, szAppwiz, szParam, ARRAYSIZE(szParam));
                        SHRunControlPanelEx(szParam, NULL, FALSE);
                    }
                }
                EndDialog(hDlg, IDCANCEL);
            }
            break;

        case WM_COMMAND:
        {
            int idCmd = GET_WM_COMMAND_ID(wParam, lParam);
            switch (idCmd)
            {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL); // we always just return IDCANCLE so that the install aborts
            }
        }
        break;

        default:
            return FALSE;
    }

    return TRUE;
}


// this is what gets called in the normal runas case
void InitUserLogonDlg(LOGONINFO* pli, HWND hDlg, LPCTSTR pszFullUserName)
{
    HWNDWSPrintf(GetDlgItem(hDlg, IDD_CURRENTUSER), pszFullUserName, FALSE);
    HWNDWSPrintf(GetDlgItem(hDlg, IDC_USECURRENTACCOUNT), pszFullUserName, FALSE);

    CheckRadioButton(hDlg, IDC_USECURRENTACCOUNT, IDC_USEOTHERACCOUNT, IDC_USEOTHERACCOUNT);
    EnableOKButtonFromID(hDlg, IDC_USERNAME);
    SetFocus(GetDlgItem(hDlg, IDC_PASSWORD));
}


// this is what gets called in the install app launching as non admin case
void InitSetupLogonDlg(LOGONINFO* pli, HWND hDlg, LPCTSTR pszFullUserName)
{
    HWNDWSPrintf(GetDlgItem(hDlg, IDC_USECURRENTACCOUNT), pszFullUserName, FALSE);
    HWNDWSPrintf(GetDlgItem(hDlg, IDC_MESSAGEBOXCHECKEX), pszFullUserName, FALSE);

    CheckRadioButton(hDlg, IDC_USECURRENTACCOUNT, IDC_USEOTHERACCOUNT, IDC_USECURRENTACCOUNT);
    EnableWindow(GetDlgItem(hDlg, IDC_USERNAME), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_PASSWORD), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_DOMAIN), FALSE);

    SetFocus(GetDlgItem(hDlg, IDOK));
}


void SetInitialNameAndDomain(LOGONINFO* pli, HWND hDlg)
{
    TCHAR szName[MAX_PATH];

    //
    // BUGBUG (reinerf) - we should save off what the user typed in and use that as the default
    //
    if (LoadString(HINST_THISDLL, IDS_ADMINISTRATOR, szName, ARRAYSIZE(szName)) > 0)
    {
        // default the "User name:" to Administrator
        SetDlgItemText(hDlg, IDC_USERNAME, szName);
    }

    if (GetEnvironmentVariable(TEXT("COMPUTERNAME"), szName, ARRAYSIZE(szName)) > 0)
    {
        // default the "Domain:" to COMPUTERNAME
        SetDlgItemText(hDlg, IDC_DOMAIN, szName);
    }
}


BOOL_PTR CALLBACK UserLogon_DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TCHAR szTemp[UNLEN + 1 + GNLEN + 1];    // enough to hold "reinerf@NTDEV" or "NTDEV\reinerf"
    LPTSTR psz;
    LOGONINFO *pli= (LOGONINFO*)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            TCHAR szFullName[UNLEN + 1 + GNLEN + 1];    // enough to hold "reinerf@NTDEV" or "NTDEV\reinerf"
            TCHAR szName[UNLEN + 1 + GNLEN + 1];        
            ULONG cchName;

            pli = (LOGONINFO*)lParam;
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pli);

            // start off with "the current user" in case we fail, so we don't put garbage in the dialog
            LoadString(HINST_THISDLL, IDS_THECURRENTUSER, szFullName, ARRAYSIZE(szFullName));

            if (!GetUserNameEx(NameSamCompatible, szName, &(cchName = ARRAYSIZE(szName))))
            {
                if (GetUserNameEx(NameDisplay, szName, &(cchName = ARRAYSIZE(szName)))  ||
                    GetUserName(szName, &(cchName = ARRAYSIZE(szName)))                 ||
                    (GetEnvironmentVariable(TEXT("USERNAME"), szName, ARRAYSIZE(szName)) > 0))
                {
                    if (GetEnvironmentVariable(TEXT("USERDOMAIN"), szFullName, ARRAYSIZE(szFullName)) > 0)
                    {
                        lstrcatn(szFullName, TEXT("\\"), ARRAYSIZE(szFullName));
                        lstrcatn(szFullName, szName, ARRAYSIZE(szFullName));
                    }
                }
                else
                {
                    TraceMsg(TF_WARNING, "UserLogon_DlgProc: failed to get the user's name using various methods");
                }
            }
            else
            {
                // we got the SamCompatible name, so just use that
                lstrcpy(szFullName, szName);
            }

            SetInitialNameAndDomain(pli, hDlg);
            
            // limit the edit box lengths to prevent buffer overrun
            Edit_LimitText(GetDlgItem(hDlg, IDC_USERNAME), UNLEN + 1 + GNLEN);  // enough room for "reinerf@NTDEV" or "NTDEV\reinerf"
            Edit_LimitText(GetDlgItem(hDlg, IDC_PASSWORD), PWLEN);
            Edit_LimitText(GetDlgItem(hDlg, IDC_DOMAIN), GNLEN);

            // call the proper init function depending on whether this is a setup program launching or the normal runas case
            if (pli->raType == RUNAS_NONADMININSTALL)
            {
                InitSetupLogonDlg(pli, hDlg, szFullName);
            }
            else if (pli->raType == RUNAS_NORMAL)
            {
                InitUserLogonDlg(pli, hDlg, szFullName);
            }
            else
            {
                ASSERTMSG(FALSE, "UserLogon_DlgProc: found pli->raType that is not RUNAS_NORMAL or RUNAS_NONADMININSTALL!");
            }
            break;
        }
        break;

        case WM_COMMAND:
        {
            BOOL fNoInlineDomain;
            int idCmd = GET_WM_COMMAND_ID(wParam, lParam);
            switch (idCmd)
            {
                case IDC_USERNAME:
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_UPDATE)
                    {
                        EnableOKButtonFromID(hDlg, IDC_USERNAME);
                        GetDlgItemText(hDlg, IDC_USERNAME, szTemp, ARRAYSIZE(szTemp));

                        if (!IsDlgButtonChecked(hDlg, IDC_USECURRENTACCOUNT))
                        {
                            fNoInlineDomain = (StrChr(szTemp, TEXT('\\')) == NULL);
                            EnableWindow(GetDlgItem(hDlg, IDC_DOMAIN), fNoInlineDomain);
                            EnableWindow(GetDlgItem(hDlg, IDC_DOMAINLBL), fNoInlineDomain);
                        }
                    }
                    break;

                case IDC_USEOTHERACCOUNT:
                case IDC_USECURRENTACCOUNT:
                    if (IsDlgButtonChecked(hDlg, IDC_USECURRENTACCOUNT))
                    {
                        EnableWindow(GetDlgItem(hDlg, IDC_USERNAME), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_PASSWORD), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_DOMAIN), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_USERNAMELBL), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_PASSWORDLBL), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_DOMAINLBL), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
                    }
                    else
                    {
                        EnableWindow(GetDlgItem(hDlg, IDC_USERNAME), TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_PASSWORD), TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_USERNAMELBL), TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_PASSWORDLBL), TRUE);
                        GetDlgItemText(hDlg, IDC_USERNAME, szTemp, ARRAYSIZE(szTemp));
                        fNoInlineDomain = (StrChr(szTemp, TEXT('\\')) == NULL);
                        EnableWindow(GetDlgItem(hDlg, IDC_DOMAIN), fNoInlineDomain);
                        EnableWindow(GetDlgItem(hDlg, IDC_DOMAINLBL), fNoInlineDomain);
                        EnableOKButtonFromID(hDlg, IDC_USERNAME);
                    }
                    break;

                case IDOK:
                    if (IsDlgButtonChecked(hDlg, IDC_USEOTHERACCOUNT))
                    {
                        szTemp[0] = 0;
                        GetDlgItemText(hDlg, IDC_PASSWORD, pli->szPassword, ARRAYSIZE(pli->szPassword));
                        GetDlgItemText(hDlg, IDC_USERNAME, szTemp, ARRAYSIZE(szTemp));
                        GetDlgItemText(hDlg, IDC_DOMAIN, pli->szDomain, ARRAYSIZE(pli->szDomain));

                        // Order of domain name acquisition, the first one found wins:
                        // 1) use domain if user entered domain\username
                        // 2) use domain name if entered
                        // 3) check for username@domain
                        if (psz = StrChr(szTemp, TEXT('\\')))
                        {
                            // domain\name format
                            ASSERT(IsWindowEnabled(GetDlgItem(hDlg, IDC_DOMAIN)) == FALSE);
                            *psz = 0;
                            lstrcpyn(pli->szUser, psz + 1, ARRAYSIZE(pli->szUser));
                            lstrcpyn(pli->szDomain, szTemp, ARRAYSIZE(pli->szDomain));
                        }
                        else
                        {
                            psz = StrChr(szTemp, TEXT('@'));
                            if ((pli->szDomain[0] == 0) && (psz != NULL))
                            {
                                // name@domain format
                                *psz = 0;
                                lstrcpyn(pli->szDomain, psz + 1, ARRAYSIZE(pli->szDomain));
                                lstrcpyn(pli->szUser, szTemp, ARRAYSIZE(pli->szUser));
                            }
                            else
                            {
                                lstrcpyn(pli->szUser, szTemp, ARRAYSIZE(pli->szUser));
                            }
                        }
                    }
                    else
                    {
                        idCmd = IDNO;
                    }
                // fall through

                case IDCANCEL:
                    EndDialog(hDlg, idCmd);
                    return TRUE;
                    break;
            }
            break;
        }

        default:
            return FALSE;
    }

    if (!pli || (pli->raType == RUNAS_NONADMININSTALL))
    {
        // we want the MessageBoxCheckExDlgProc have a crack at all messages in
        // the RUNAS_NONADMININSTALL case, so return FALSE here
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

#endif // WINNT

//  implement this after we figure out what
//  errors that CreateProcessWithLogonW() will return
//  that mean the user should retry the logon.
BOOL _IsLogonError(DWORD err)
{
    static const DWORD s_aLogonErrs[] = {
        ERROR_LOGON_FAILURE,
        ERROR_ACCOUNT_RESTRICTION,
        ERROR_INVALID_LOGON_HOURS,
        ERROR_INVALID_WORKSTATION,
        ERROR_PASSWORD_EXPIRED,
        ERROR_ACCOUNT_DISABLED,
        ERROR_NONE_MAPPED,
        ERROR_NO_SUCH_USER,
        ERROR_INVALID_ACCOUNT_NAME
        };

    for (int i = 0; i < ARRAYSIZE(s_aLogonErrs); i++)
    {
        if (err == s_aLogonErrs[i])
            return TRUE;
    }
    return FALSE;
}


#ifdef WINNT
BOOL CheckForAppPathsBoolValue(LPCTSTR pszImageName, LPCTSTR pszValueName)
{
    BOOL bRet = FALSE;
    TCHAR szAppPathKeyName[MAX_PATH + ARRAYSIZE(REGSTR_PATH_APPPATHS) + 2]; // +2 = +1 for '\' and +1 for the null terminator
    DWORD cbSize = sizeof(bRet);

    PathToAppPathKey(pszImageName, szAppPathKeyName, ARRAYSIZE(szAppPathKeyName));
    SHGetValue(HKEY_LOCAL_MACHINE, szAppPathKeyName, pszValueName, NULL, &bRet, &cbSize);

    return bRet;
}

__inline BOOL IsRunAsSetupExe(LPCTSTR pszImageName)
{
    return CheckForAppPathsBoolValue(pszImageName, TEXT("RunAsOnNonAdminInstall"));
}

__inline BOOL IsTSSetupExe(LPCTSTR pszImageName)
{
    return CheckForAppPathsBoolValue(pszImageName, TEXT("BlockOnTSNonInstallMode"));
}


//
// this function checks for the different cases where we need to display a "runas" or warning dialog
// before a program is run.
//
// NOTE: pli->raType is an outparam that tells the caller what type of dialog is needed
//
// return:  TRUE    - we need to bring up a dialog
//          FALSE   - we do not need to prompt the user
//
BOOL CheckForInstallApplication(LPCTSTR pszApplicationName, LPCTSTR pszCommandLine, LOGONINFO* pli)
{
    // if we are on a TS machine, AND its not in "Remote Administration" mode, AND this is a TS setup exe (eg install.exe or setup.exe)
    // AND we aren't in install mode...
    if (IsOS(OS_WIN2000TERMINAL) && !IsOS(OS_TERMINALREMOTEADMIN) && IsTSSetupExe(pszApplicationName) && !TermsrvAppInstallMode())
    {
        TCHAR szExePath[MAX_PATH];

        lstrcpyn(szExePath, pszCommandLine, ARRAYSIZE(szExePath));
        PathRemoveArgs(szExePath);
        PathUnquoteSpaces(szExePath);
        
        // ...AND the app we are launching is not TS aware, then we block the install and tell the user to go
        // to Add/Remove Programs.
        if (!IsExeTSAware(szExePath))
        {
            TraceMsg(TF_SHELLEXEC, "_SHCreateProcess: blocking the install on TS beacuse the machine is not in install mode for %s", pszApplicationName);
            pli->raType = RUNAS_HYDRANOINSTALLMODE;
            return TRUE;
        }
    }
    
    // the hyrda case failed, so we check for the user not running as an admin but launching a setup exe (eg winnt32.exe, install.exe, or setup.exe)
    if (!SHRestricted(REST_NORUNASINSTALLPROMPT) && IsRunAsSetupExe(pszApplicationName) && !IsUserAnAdmin())
    {
        BOOL bPromptForInstall = TRUE;

        if (!SHRestricted(REST_PROMPTRUNASINSTALLNETPATH))
        {
            TCHAR szFullPathToApp[MAX_PATH];

            // we want to disable runas on unc and net shares for now since the Administrative account might not
            // have privlidges to the network path
            lstrcpyn(szFullPathToApp, pszCommandLine, ARRAYSIZE(szFullPathToApp));
            PathRemoveArgs(szFullPathToApp);
            PathUnquoteSpaces(szFullPathToApp);

            if (PathIsUNC(szFullPathToApp) || IsNetDrive(PathGetDriveNumber(szFullPathToApp)))
            {
                TraceMsg(TF_SHELLEXEC, "_SHCreateProcess: not prompting for runas install on unc/network path %s", szFullPathToApp);
                bPromptForInstall = FALSE;
            }
        }

        if (bPromptForInstall)
        {
            TraceMsg(TF_SHELLEXEC, "_SHCreateProcess: bringing up the Run As... dialog for %s", pszApplicationName);
            pli->raType = RUNAS_NONADMININSTALL;
            return TRUE;
        }
    }

    return FALSE;
}
#endif


//
//  SHCreateProcess()
//  WARNING: lpApplication is not actually passed to CreateProcess() it is
//            for internal use only.
//
BOOL _SHCreateProcess(
                     HWND hwnd,
                     HANDLE hToken,
                     LPCTSTR lpApplicationName,
                     LPTSTR lpCommandLine,
                     DWORD dwCreationFlags,
                     LPSECURITY_ATTRIBUTES  lpProcessAttributes,
                     LPSECURITY_ATTRIBUTES  lpThreadAttributes,
                     BOOL  bInheritHandles,
                     LPVOID lpEnvironment,
                     LPCTSTR lpCurrentDirectory,
                     LPSTARTUPINFO lpStartupInfo,
                     LPPROCESS_INFORMATION lpProcessInformation,
                     BOOL fUserLogon,
                     BOOL fNoUI
                    )
{
    BOOL fRet = FALSE;
    BOOL fAllowRunAs = FALSE;
    DWORD err = NOERROR;
#ifdef WINNT
    LOGONINFO li = {0};


    if (!fUserLogon)
    {
        // see if we need to put up a warning prompt either because the user is not an
        // admin or this is hydra and we are not in install mode.
        fUserLogon = CheckForInstallApplication(lpApplicationName, lpCommandLine, &li);

        // NOTE: We always do this, even if NOUI is set... We figure anyone launching a setup app needs
        // this and wants this. If this assumption is wrong, we need to introduce a new bit
        // that says "don't display error dialogs" and change shlexec.c:InvokeInProcExec to pass this flag.
        // - lamadio 6.5.99
        if (fUserLogon)
            fAllowRunAs = TRUE;
    }

    if (!hToken && fUserLogon && lpApplicationName)
    {
        AssocQueryString(ASSOCF_VERIFY | ASSOCF_INIT_BYEXENAME, ASSOCSTR_FRIENDLYAPPNAME,
            lpApplicationName, NULL, li.szAppName, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(li.szAppName)));

        //  if there is NO_UI, we cant prompt the user. Except when it's the runas dialog.
        //  default to DENIED
        if (fNoUI && !fAllowRunAs)
        {
            err = ERROR_ACCESS_DENIED;
        }
        else
        {
RetryUserLogon:
            INT_PTR iLogon;

            switch (li.raType)
            {
                case RUNAS_NORMAL:
                {
                    // this is the normal "Run as..." verb dialgo
                    iLogon = DialogBoxParam(HINST_THISDLL,
                                            MAKEINTRESOURCE(DLG_RUNUSERLOGON),
                                            hwnd,
                                            UserLogon_DlgProc,
                                            (LPARAM)&li);
                }
                break;

                case RUNAS_NONADMININSTALL:
                {
                    // in the non-administrator setup app case. we want the "dont show me
                    // this again" functionality, so we use the SHMessageBoxCheckEx function
                    iLogon = SHMessageBoxCheckEx(hwnd,
                                                 HINST_THISDLL,
                                                 MAKEINTRESOURCE(DLG_RUNSETUPLOGON),
                                                 UserLogon_DlgProc,
                                                 (LPVOID)&li,
                                                 IDNO, // if they checked the "dont show me this again", we want to just launch it as the current user
                                                 TEXT("WarnOnNonAdminInstall"));
                }
                break;

                case RUNAS_HYDRANOINSTALLMODE:
                {
                    // this is the hydra machine that is not in install mode and a setup app is running case.
                    LinkWindow_RegisterClass();
                    iLogon = DialogBoxParam(HINST_THISDLL,
                                            MAKEINTRESOURCE(DLG_TSINSTALLFAILURE),
                                            hwnd,
                                            HydraNoInstallMode_DlgProc,
                                            (LPARAM)&li);

                    ASSERT(iLogon == IDCANCEL); // we should always abort the execute in this case
                }
                break;

                default:
                {
                    ASSERTMSG(FALSE, "_SHCreateProcess: li.raType not recognized!");
                }
                break;
            }

            // check the return value from the dlg
            switch (iLogon)
            {
                case IDOK:
                    //  use the information from the dialog
                    //  to logon the user
                    ASSERT(fUserLogon);
                    break;

                case IDNO:
                    //  in this case we will call the regular
                    //  CreateProcess() and it will SLE()
                    fUserLogon = FALSE;
                    break;

                default:
                    //  user hit the cancel button
                    err = ERROR_CANCELLED;
                    break;
            }

        }
    }

#endif // WINNT

    if (err == NOERROR)
    {
        if (!fUserLogon)
        {
            // DEFAULT use CreateProcess
            fRet = CreateProcess(NULL, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles,
                                 dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo,
                                 lpProcessInformation);
        }
#ifdef WINNT
        else if (hToken)
        {
            //  use the user Token
            fRet = CreateProcessAsUser(hToken, NULL, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles,
                                 dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo,
                                 lpProcessInformation);

        }
        else
        {
            LPTSTR pszDesktop = lpStartupInfo->lpDesktop;
            // 99/08/19 #389284 vtan: clip username and domain to 125
            // characters each to avoid hitting the combined MAX_PATH
            // limit in AllowDesktopAccessToUser in advapi32.dll which
            // is invoked by CreateProcessWithLogonW.
            // This can be removed when the API is fixed. Check:
            // %_ntbindir%\private\windows\base\advapi\cseclogn.cxx
            li.szUser[125] = li.szDomain[125] = TEXT('\0');
            //  we are attempting logon the user. NOTE: pass LOGON_WITH_PROFILE so that we ensure that the profile is loaded
            fRet = DelayCreateProcessWithLogon(li.szUser, li.szDomain, li.szPassword, LOGON_WITH_PROFILE, NULL, lpCommandLine,
                                  dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo,
                                  lpProcessInformation);

            if (!fRet)
            {
                // HACKHACK: When CreateProcessWithLogon fails, it munges the desktop. This causes
                // the next call to "Appear" to fail because the app show up on another desktop...
                //     Why? I don't know...
                // I'm going to assign the bug back to them and have them fix it on their end, this is just to
                // work around their bug.

                if (lpStartupInfo)
                    lpStartupInfo->lpDesktop = pszDesktop;

                //  ShellMessageBox can alter LastError
                err = GetLastError();
                if (_IsLogonError(err))
                {
                    TCHAR szTemp[MAX_PATH];
                    LoadString(HINST_THISDLL, IDS_CANTLOGON, szTemp, SIZECHARS(szTemp));

                    SHSysErrorMessageBox(
                        hwnd,
                        li.szAppName,
                        IDS_SHLEXEC_ERROR,
                        err,
                        szTemp,
                        MB_OK | MB_ICONSTOP);

                    err = NOERROR;
                    goto RetryUserLogon;
                }
            }

        }

#endif // WINNT
    }


    // fire *after* the actual process since:
    //  - if there's a bug we at least get the process started (hopefully)
    //  - don't want to log failed events (for now at least)
    if (fRet)
    {
        if (UEMIsLoaded())
        {
            // skip the call if stuff isn't there yet.
            // the load is expensive (forces ole32.dll and browseui.dll in
            // and then pins browseui).
            UEMFireEvent(&UEMIID_SHELL, UEME_RUNPATH, UEMF_XEVENT, -1, (LPARAM)lpApplicationName);
            // we do the UIBW_RUNASSOC elsewhere.  this can cause slight
            // inaccuracies since there's no guarantees the 2 places are
            // 'paired'.  however it's way easier to do UIBW_RUNASSOC
            // elsewhere so we'll live w/ it.
        }
    }
    else if (err)
        SetLastError(err);

    return fRet;
}


BOOL CShellExecute::_SetCommand(void)
{
    if (_szCmdTemplate[0])
    {
        // parse arguments into command line
        DWORD se_err = ReplaceParameters(_szCommand, ARRAYSIZE(_szCommand),
            _szFile, _szCmdTemplate, _lpParameters,
            _nShow, NULL, FALSE, _lpID, &_pidlGlobal);

        if (se_err)
            _ReportHinst((HINSTANCE)se_err);
        else
            return TRUE;
    }
    else if (PathIsExe(_szFile))
    {
        _fTryOpenExe = TRUE;
    }
    else
        _ReportWin32(ERROR_NO_ASSOCIATION);

    return FALSE;
}

void CShellExecute::_TryExecCommand(void)
{
    TraceMsg(TF_SHELLEXEC, "SHEX::TryExecCommand() entered CmdTemplate = %s", _szCmdTemplate);

    if (!_SetCommand())
        return;

    _DoExecCommand();
}

void CShellExecute::_SetImageName(void)
{
    if (SUCCEEDED(_QueryString(ASSOCF_VERIFY, ASSOCSTR_EXECUTABLE, _szImageName, SIZECHARS(_szImageName))))
    {
        if (0 == lstrcmp(_szImageName, TEXT("%1")))
            StrCpyN(_szImageName, _szFile, SIZECHARS(_szImageName));
    }
    else if (PathIsExe(_szFile))
    {
        StrCpyN(_szImageName, _szFile, SIZECHARS(_szImageName));
    }
}

//
//  TryExecCommand() is the most common and default way to get an app started.
//  mostly it uses CreateProcess() with a command line composed from
//  the pei and the registry.  it can also do a ddeexec afterwards.
//

void CShellExecute::_DoExecCommand(void)
{
    BOOL fCreateProcessFailed;
    TraceMsg(TF_SHELLEXEC, "SHEX::DoExecCommand() entered szCommand = %s", _szCommand);

    do
    {
        HWND hwndOld = GetForegroundWindow();
        LPTSTR pszEnv = NULL;
        LPCTSTR pszNewEnvString = NULL;
        fCreateProcessFailed = FALSE;

        _SetImageName();

        // Check exec restrictions.
        if (SHRestricted(REST_RESTRICTRUN) && RestrictedApp(_szImageName))
        {
            _ReportWin32(ERROR_RESTRICTED_APP);
            break;
        }
        if (SHRestricted(REST_DISALLOWRUN) && DisallowedApp(_szImageName))
        {
            _ReportWin32(ERROR_RESTRICTED_APP);
            break;
        }


        // Check if app is incompatible in some fashion...
        if (!CheckAppCompatibility(_szImageName, &pszNewEnvString, _fNoUI, _hwndParent))
        {
            _ReportWin32(ERROR_CANCELLED);
            break;
        }

        //  try to validate the image if it is on a UNC share
        //  we dont need to check for Print shares, so we
        //  will fail if it is on one.
        if (_TryValidateUNC(_szImageName, NULL))
        {
            // returns TRUE if it failed or handled the operation
            // Note that SHValidateUNC calls SetLastError
            // this continue will test based on GetLastError()
            continue;
        }

#ifdef WINNT
        //
        // WOWShellExecute sets a global variable
        //     The cb is only valid when we are being called from wow
        //     If valid use it
        //
        if (_TryWowShellExec())
            break;
#endif

		// See if we need to pass a new environment to the new process
        pszEnv = _BuildEnvironmentForNewProcess( pszNewEnvString );

        TraceMsg(TF_SHELLEXEC, "SHEX::DoExecCommand() CreateProcess(NULL,%s,...)", _szCommand);

        //  CreateProcess will SetLastError() if it fails
        if (_SHCreateProcess(_hwndParent,
                             _hUserToken,
                             _szImageName,
                             _szCommand,
                             _dwCreateFlags,
                             _pProcAttrs,
                             _pThreadAttrs,
                             _fInheritHandles,
                             pszEnv,
                             _fUseNullCWD ? NULL : _szWorkingDir,
                             &_startup,
                             &_pi,
                             _fRunAs,
                             _fNoUI))
        {
            // If we're doing DDE we'd better wait for the app to be up and running
            // before we try to talk to them.
            if (_fDDEInfoSet || _fWaitForInputIdle)
            {
                // Yep, How long to wait? For now, try 60 seconds to handle
                // pig-slow OLE apps.
                WaitForInputIdle(_pi.hProcess, 60*1000);
            }
#ifndef WINNT
            // For 16-bit apps, we need to wait until they've started. 32-bit
            // apps never have to wait.
            // On NT, the 16-bit app path doesn't get this far so we can avoid
            // it altogether.
            else if (GetProcessDword(GetCurrentProcessId(), GPD_FLAGS) & GPF_WIN16_PROCESS)
            {
                // NT and win3.1 16 bit callers all wait, even if the target is
                // a 32 bit guy
                WaitForInputIdle(_pi.hProcess, 10*1000);
            }
#endif

            // Find the "hinstance" of whatever we just created.
            // PEIOUT - hinst reported for pei->hInstApp
            HINSTANCE hinst = (HINSTANCE)GetProcessDword(_pi.dwProcessId, GPD_HINST);

            // Now fix the focus and do any dde stuff that we need to do
            _FixActivationStealingApps(hwndOld, _nShow);

            if (_fDDEInfoSet)
            {
                //  this will _Report() any errors for us if necessary
                _DDEExecute(NULL, _hwndParent, _nShow, _fDDEWait);
            }
            else
                _ReportHinst(hinst);

            //  clean up before the break
            if (pszEnv)
                LocalFree(pszEnv);

            break;  // out of retry loop
        }
        else
        {
            fCreateProcessFailed = TRUE;

        }

        //  clean up the loop
        if (pszEnv)
            LocalFree(pszEnv);


    // **WARNING** this assumes that SetLastError() has been called - zekel - 20-NOV-97
    //  right now we only reach here after CreateProcess() fails or
    //  SHValidateUNC() fails.  both of these do SetLastError()
    }
    while (_ProcessErrorShouldTryExecCommand(GetLastError(), _hwndParent, fCreateProcessFailed));

    // (we used to do a UIBW_RUNASSOC here, but moved it higher up)
}

HGLOBAL CShellExecute::_CreateDDECommand(int nShow, BOOL fLFNAware, BOOL fNative)
{
    // Now that we can handle ShellExec for URLs, we need to have a much bigger
    // command buffer. Explorer's DDE exec command even has two file names in
    // it. (WHY?) So the command buffer have to be a least twice the size of
    // INTERNET_MAX_URL_LENGTH plus room for the command format.
    SHSTR strTemp;
    HGLOBAL hRet = NULL;

    if (SUCCEEDED(strTemp.SetSize((2 * INTERNET_MAX_URL_LENGTH) + 64)))
    {
        if (0 == ReplaceParameters(strTemp.GetStr(), strTemp.GetSize(), _szFile,
            _szDDECmd, _lpParameters, nShow, ((DWORD*) &_startup.hStdInput), fLFNAware, _lpID, &_pidlGlobal))
        {

            TraceMsg(TF_SHELLEXEC, "SHEX::_CreateDDECommand(%d, %d) : %s", fLFNAware, fNative, strTemp.GetStr());

#ifdef UNICODE
            //  we only have to thunk on NT
            if (!fNative)
            {
                SHSTRA stra;
                if (SUCCEEDED(stra.SetStr(strTemp)))
                {
                    // Get dde memory for the command and copy the command line.

                    hRet = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, CbFromCch(lstrlenA(stra.GetStr()) + 1));

                    if (hRet)
                    {
                        LPSTR psz = (LPSTR) GlobalLock(hRet);
                        lstrcpyA(psz, stra.GetStr());
                        GlobalUnlock(hRet);
                    }
                }
            }
            else
            {
#else
                ASSERT(fNative);
#endif

                // Get dde memory for the command and copy the command line.

                hRet = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, CbFromCch(lstrlen(strTemp.GetStr()) + 1));

                if (hRet)
                {
                    LPTSTR psz = (LPTSTR) GlobalLock(hRet);
                    lstrcpy(psz, strTemp.GetStr());
                    GlobalUnlock(hRet);
                }
#ifdef UNICODE
            }
#endif

        }
    }

    return hRet;
}

// Short cut all DDE commands with a WM_NOTIFY
//  returns true if this was handled...or unrecoverable error.
BOOL CShellExecute::_TryDDEShortCircuit(HWND hwnd, HGLOBAL hMem, int nShow)
{
    if (hwnd  && IsWindowInProcess(hwnd))
    {
        HINSTANCE hret = (HINSTANCE)SE_ERR_FNF;

        // get the top most owner.
        hwnd = GetTopParentWindow(hwnd);

        if (IsWindowInProcess(hwnd))
        {
            LPNMVIEWFOLDER lpnm = (LPNMVIEWFOLDER)LocalAlloc(LPTR, SIZEOF(NMVIEWFOLDER));

            if (lpnm)
            {
                lpnm->hdr.hwndFrom = NULL;
                lpnm->hdr.idFrom = 0;
                lpnm->hdr.code = SEN_DDEEXECUTE;
                lpnm->dwHotKey = HandleToUlong(_startup.hStdInput);
                if ((_startup.dwFlags & STARTF_HASHMONITOR) != 0)
                    lpnm->hMonitor = reinterpret_cast<HMONITOR>(_startup.hStdOutput);
                else
                    lpnm->hMonitor = NULL;

                StrCpyN(lpnm->szCmd, (LPTSTR) GlobalLock(hMem), ARRAYSIZE(lpnm->szCmd));
                GlobalUnlock(hMem);

                if (SendMessage(hwnd, WM_NOTIFY, 0, (LPARAM)lpnm))
                    hret =  Window_GetInstance(hwnd);

                LocalFree(lpnm);
            }
            else
                hret = (HINSTANCE)SE_ERR_OOM;
        }

        TraceMsg(TF_SHELLEXEC, "SHEX::_TryDDEShortcut hinst = %d", hret);

        if ((UINT_PTR)hret != SE_ERR_FNF)
        {
            _ReportHinst(hret);
            return TRUE;
        }
    }

    return FALSE;
}

//----------------------------------------------------------------------------
// _WaiteForDDEMsg()
// this does a message loop until DDE msg or a timeout occurs
//
STDAPI_(void) _WaitForDDEMsg(HWND hwnd, DWORD dwTimeout, UINT wMsg)
{
    //  termination event
    HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    SetProp(hwnd, SZTERMEVENT, hEvent);

    for (;;)
    {
        MSG msg;
        DWORD dwEndTime = GetTickCount() + dwTimeout;
        LONG lWait = (LONG)dwTimeout;

        DWORD dwReturn = MsgWaitForMultipleObjects(1, &hEvent,
                FALSE, lWait, QS_POSTMESSAGE);

        //  if we time out or get an error or get our EVENT!!!
        //  we just bag out
        if (dwReturn != (WAIT_OBJECT_0 + 1))
        {
            break;
        }

        // we woke up because of messages.
        while (PeekMessage(&msg, NULL, WM_DDE_FIRST, WM_DDE_LAST, PM_REMOVE))
        {
            ASSERT(msg.message != WM_QUIT);
            DispatchMessage(&msg);

            if (msg.hwnd == hwnd && msg.message == wMsg)
                goto Quit;
        }

        // calculate new timeout value
        if (dwTimeout != INFINITE)
        {
            lWait = (LONG)dwEndTime - GetTickCount();
        }
    }

Quit:
    if (hEvent)
        CloseHandle(hEvent);
    RemoveProp(hwnd, SZTERMEVENT);

    return;
}

LRESULT CALLBACK DDESubClassWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndConv = (HWND) GetProp(hWnd, SZCONV);
    WPARAM nLow;
    WPARAM nHigh;
    HANDLE hEvent;

    switch (wMsg)
    {
      case WM_DDE_ACK:
        if (!hwndConv)
        {
            // this is the first ACK for our INITIATE message
            TraceMsg(TF_SHELLEXEC, "SHEX::DDEStubWnd get ACK on INITIATE");
            return SetProp(hWnd, SZCONV, (HANDLE)wParam);
        }
        else if (((UINT_PTR)hwndConv == 1) || ((HWND)wParam == hwndConv))

        {
            // this is the ACK for our EXECUTE message
            TraceMsg(TF_SHELLEXEC, "SHEX::DDEStubWnd got ACK on EXECUTE");

            UnpackDDElParam(wMsg, lParam, &nLow, &nHigh);
            GlobalFree((HGLOBAL)nHigh);
            FreeDDElParam(wMsg, lParam);

            // prevent us from destroying again....
            if ((UINT_PTR) hwndConv != 1)
                DestroyWindow(hWnd);
        }

        // This is the ACK for our INITIATE message for all servers
        // besides the first.  We return FALSE, so the conversation
        // should terminate.
        break;

      case WM_DDE_TERMINATE:
        if (hwndConv == (HANDLE)wParam)
        {
            // this TERMINATE was originated by another application
            // (otherwise, hwndConv would be 1)
            // they should have freed the memory for the exec message

            TraceMsg(TF_SHELLEXEC, "SHEX::DDEStubWnd got TERMINATE from hwndConv");

            PostMessage((HWND)wParam, WM_DDE_TERMINATE, (WPARAM)hWnd, 0L);

            RemoveProp(hWnd, SZCONV);
            DestroyWindow(hWnd);
        }
        // Signal the termination event to ensure nested dde calls will terminate the
        // appropriate _WaitForDDEMsg loop properly...
        if (hEvent = GetProp(hWnd, SZTERMEVENT))
            SetEvent(hEvent);

        // This is the TERMINATE response for our TERMINATE message
        // or a random terminate (which we don't really care about)
        break;

      case WM_TIMER:
        if ( wParam == DDE_DEATH_TIMER_ID )
        {
            // we have waited long enough, we still have no ACK, so quit the window ...
            hEvent = RemoveProp(hWnd, SZDDEEVENT);
            if ( hEvent )
                CloseHandle( hEvent );

            // The conversation will be terminated in the destroy code
            DestroyWindow(hWnd);

            TraceMsg(TF_SHELLEXEC, "SHEX::DDEStubWnd TIMER closing DDE Window due to lack of ACK");
            break;
        }
        else
          return DefWindowProc(hWnd, wMsg, wParam, lParam);

      case WM_DESTROY:
        TraceMsg(TF_SHELLEXEC, "SHEX::DDEStubWnd WM_DESTROY'd");

        // kill the timer just incase.... (this may fail if we never set the timer)
        KillTimer( hWnd, DDE_DEATH_TIMER_ID );
        if (hwndConv)
        {
            // Make sure the window is not destroyed twice
            SetProp(hWnd, SZCONV, (HANDLE)1);

            /* Post the TERMINATE message and then
             * Wait for the acknowledging TERMINATE message or a timeout
             */
            PostMessage(hwndConv, WM_DDE_TERMINATE, (WPARAM)hWnd, 0L);

            _WaitForDDEMsg(hWnd, DDE_TERMINATETIMEOUT, WM_DDE_TERMINATE);

            RemoveProp(hWnd, SZCONV);
        }

        // the DDE conversation is officially over, let ShellExec know
        if (NULL != (hEvent = GetProp(hWnd, SZDDEEVENT)))
        {
            SetEvent(hEvent);

            //  this can be picked off by the event handler...
            hEvent = RemoveProp(hWnd, SZDDEEVENT);
            if (hEvent)
                CloseHandle(hEvent);
        }

        /* Fall through */
      default:
        return DefWindowProc(hWnd, wMsg, wParam, lParam);
    }

    return 0L;
}

HWND CShellExecute::_CreateHiddenDDEWindow(HWND hwndParent, HANDLE hDDEEvent)
{
    // lets be lazy and not create a class for it
    HWND hwnd = SHCreateWorkerWindow(DDESubClassWndProc, GetTopParentWindow(hwndParent),
        0, 0, NULL, NULL);

    if (hwnd)
    {
        SetProp(hwnd, SZDDEEVENT, hDDEEvent);
    }

    TraceMsg(TF_SHELLEXEC, "SHEX::_CreateHiddenDDEWindow returning hwnd = 0x%X", hwnd);

    return hwnd;
}

void CShellExecute::_DestroyHiddenDDEWindow(HWND hwnd)
{
    if (IsWindow(hwnd))
    {
        TraceMsg(TF_SHELLEXEC, "SHEX::_DestroyHiddenDDEWindow on hwnd = 0x%X", hwnd);

        HANDLE hEvent = RemoveProp(hwnd, SZDDEEVENT);

        if (hEvent)
            CloseHandle(hEvent);

        DestroyWindow(hwnd);
    }
}

BOOL CShellExecute::_PostDDEExecute(HWND hwndConv,
                                    HGLOBAL hDDECommand,
                                    HANDLE hConversationDone,
                                    BOOL fWaitForDDE,
                                    HWND *phwndDDE)
{
    TraceMsg(TF_SHELLEXEC, "SHEX::_PostDDEExecute(0x%X, 0x%X) entered", hwndConv, *phwndDDE);

    DWORD dwProcessID = 0;
    GetWindowThreadProcessId( hwndConv, &dwProcessID );
    if ( dwProcessID )
    {
        AllowSetForegroundWindow( dwProcessID );
    }

    if (PostMessage(hwndConv, WM_DDE_EXECUTE, (WPARAM)*phwndDDE, (LPARAM)PackDDElParam(WM_DDE_EXECUTE, 0,(UINT_PTR)hDDECommand)))
    {
        TraceMsg(TF_SHELLEXEC, "SHEX::_PostDDEExecute() connected");

        // everything's going fine so far, so return to the application
        // with the instance handle of the guy, and hope he can execute our string

        _ReportHinst(Window_GetInstance(hwndConv));

        if (fWaitForDDE)
        {
            // We can't return from this call until the DDE conversation terminates.
            // Otherwise the thread may go away, nuking our hwndConv window,
            // messing up the DDE conversation, and Word drops funky error messages
            // on us.
            TraceMsg(TF_SHELLEXEC, "SHEX::_PostDDEExecute() waiting for termination");
            SHProcessMessagesUntilEvent(NULL, hConversationDone, INFINITE);
        }
        else if (IsWindow(*phwndDDE))
        {
            // set a timer to tidy up the window incase we never get a ACK....
            TraceMsg(TF_SHELLEXEC, "SHEX::_PostDDEExecute() setting DEATH timer");

            SetTimer(*phwndDDE, DDE_DEATH_TIMER_ID, DDE_DEATH_TIMEOUT, NULL);
            *phwndDDE = NULL;
        }

        return TRUE;
    }

    return FALSE;
}

#ifdef FEATURE_SHELLEXECCACHE
void CShellExecute::_CacheDDEWindowClass(HWND hwnd)
{
    ASSERT(IsWindow(hwnd));
    ASSERT(_hkDDE);

    //  if they use the DDEML stuff, then they are no good to us.
    if (GetClassName(hwnd, _szValue, SIZECHARS(_szValue)) && !StrStr(_szValue, TEXT("DDEML")))
    {
        //  we now want to cache this in the DDE key so that we can
        //  to use it first the next time we try this file type.
        //  this will avoid doing broadcasts, and thus decrease the
        //  chance of us hanging or doing other DDE trash.

        RegSetValueEx(_hkDDE, SZWNDCLASS, 0, REG_SZ, (LPBYTE) _szValue, SIZEOF(_szValue));
        TraceMsg(TF_SHELLEXEC, "SHEX::CacheDDEWndClass caching: %s", _szValue);


    }

}
#endif // FEATURE_SHELLEXECCACHE

#define DDE_TIMEOUT             30000       // 30 seconds.
#define DDE_TIMEOUT_LOW_MEM     80000       // 80 seconds - Excel takes 77.87 on 486.33 with 8mb

typedef struct {
    WORD  aName;
    HWND  hwndDDE;
    LONG  lAppTopic;
    UINT  timeout;
}INITDDECONV ;

#ifdef FEATURE_SHELLEXECCACHE

BOOL InitDDEConv(HWND hwnd, LPARAM pv)
{
    INITDDECONV *pidc = (INITDDECONV *) pv;

    ASSERT(pidc);

    //  if this is the desired window....
    if (pidc->aName == GetClassWord(hwnd, GCW_ATOM))
    {
        DWORD dwResult;
        //  we found somebody who used to like us...
        // Send the initiate message.
        // NB This doesn't need packing.
        SendMessageTimeout(hwnd, WM_DDE_INITIATE, (WPARAM)pidc->hwndDDE,
                pidc->lAppTopic, SMTO_ABORTIFHUNG,
                pidc->timeout,
                &dwResult);

        return !BOOLIFY(GetProp(pidc->hwndDDE, SZCONV));
    }

    return TRUE;
}
#endif //  FEATURE_SHELLEXECCACHE


HWND CShellExecute::_GetConversationWindow(HWND hwndDDE)
{
    ULONG_PTR dwResult;  //unused
    HWND hwnd = NULL;
    INITDDECONV idc = { NULL,
                        hwndDDE,
                        MAKELONG(_aApplication, _aTopic),
                        SHIsLowMemoryMachine(ILMM_IE4) ? DDE_TIMEOUT_LOW_MEM : DDE_TIMEOUT
                        };

#ifdef FEATURE_SHELLEXECCACHE

    //
    //  BUGBUG  it turns out that we cant use the WndClass because of DDEML
    //  with DDEML, the real window that we want to INIT with is not the
    //  one that the conversation uses.  so we need to identify the process
    //  and use the process path for searching instead.  this requires
    //  using NtQueryInformationProcess(), and i dont know how expensive that is.
    //
    DWORD cbValue = SIZEOF(_szValue), dwType = REG_SZ;
    //  see if we have a cached wndclass name
    if (ERROR_SUCCESS == SHQueryValueEx(_hkDDE, SZWNDCLASS, NULL, &dwType, (LPBYTE) _szValue, &cbValue)
        && _szValue[0])
    {
        TraceMsg(TF_SHELLEXEC, "SHEX::GetConvWnd() looking for class: %s", _szValue);
        idc.aName = GlobalAddAtom(_szValue);

        if (idc.aName)
        {

            EnumWindows(InitDDEConv, (LPARAM)&idc);
            hwnd = (HWND) GetProp(hwndDDE, SZCONV);
            TraceMsg(TF_SHELLEXEC, "SHEX::GetConvWnd found this classy window [%X]", hwnd);
            GlobalDeleteAtom(idc.aName);
        }
    }
#endif  // FEATURE_SHELLEXECCACHE

    //  if we didnt find him, then we better default to the old way...
    if (!hwnd)
    {

        //  we found somebody who used to like us...
        // Send the initiate message.
        // NB This doesn't need packing.
        SendMessageTimeout((HWND) -1, WM_DDE_INITIATE, (WPARAM)hwndDDE,
                idc.lAppTopic, SMTO_ABORTIFHUNG,
                idc.timeout,
                &dwResult);

        hwnd = (HWND) GetProp(hwndDDE, SZCONV);
    }

    TraceMsg(TF_SHELLEXEC, "SHEX::GetConvWnd returns [%X]", hwnd);
    return hwnd;
}

//----------------------------------------------------------------------------
BOOL CShellExecute::_DDEExecute(
    BOOL fWillRetry,
    HWND hwndParent,
    int   nShowCmd,
    BOOL fWaitForDDE
)
{
    LONG err = ERROR_OUTOFMEMORY;
    BOOL fReportErr = TRUE;

    // Get the actual command string.
    // NB We'll assume the guy we're going to talk to is LFN aware. If we're wrong
    // we'll rebuild the command string a bit later on.
    HGLOBAL hDDECommand = _CreateDDECommand(nShowCmd, TRUE, TRUE);

    if (hDDECommand)
    {
        //  we have a DDE command to try

        if (_TryDDEShortCircuit(hwndParent, hDDECommand, nShowCmd))
        {
            //  the shortcut tried and now we have an error reported
            fReportErr = FALSE;
        }
        else
        {
            HANDLE hConversationDone = CreateEvent(NULL, FALSE, FALSE, NULL);

            if (hConversationDone)
            {
                // Create a hidden window for the conversation
                HWND hwndDDE = _CreateHiddenDDEWindow(hwndParent, hConversationDone);

                if (hwndDDE)
                {
                    HWND hwndConv = _GetConversationWindow(hwndDDE);                    // no one responded
                    if (hwndConv)
                    {
#ifdef FEATURE_SHELLEXECCACHE
                        //  we want to set them up for next time...
                        _CacheDDEWindowClass(hwndConv);
#endif // FEATURE_SHELLEXECCACHE

                        //  somebody answered us.

                        // This doesn't work if the other guy is using ddeml.
                        if (_fActivateHandler)
                            ActivateHandler(hwndConv, (DWORD_PTR) _startup.hStdInput);

                        // Can the guy we're talking to handle LFNs?
                        BOOL fLFNAware = Window_IsLFNAware(hwndConv);
#ifdef UNICODE
                        BOOL fNative = IsWindowUnicode(hwndConv);
#else
#define fNative TRUE
#endif
                        if (!fLFNAware || !fNative)
                        {
                            //  we need to redo the command string.
                            // Nope - App isn't LFN aware - redo the command string.
                            GlobalFree(hDDECommand);

                            //  we may need a new _pidlGlobal too.
                            if (_pidlGlobal)
                            {
                                //  BUGBUG : there is one time that we may use a global...
                                SHFreeShared((HANDLE)_pidlGlobal,GetCurrentProcessId());
                                _pidlGlobal = NULL;

                            }

                            hDDECommand = _CreateDDECommand(nShowCmd, fLFNAware, fNative);

                        }


                        // Send the execute message to the application.
                        err = ERROR_DDE_FAIL;

                        if (_PostDDEExecute(hwndConv, hDDECommand, hConversationDone, fWaitForDDE, &hwndDDE))
                        {
                            fReportErr = FALSE;
                            hDDECommand = NULL;
                        }
                    }
                    else
                    {
                        err = (ERROR_FILE_NOT_FOUND);
                    }

                    //  cleanup
                    _DestroyHiddenDDEWindow(hwndDDE);

                }
                else
                //  otherwise its cleaned up in _DestroyHiddenDDEWindow()
                    CloseHandle(hConversationDone);
            }

        }

        //  cleanup
        if (hDDECommand)
            GlobalFree(hDDECommand);
    }


    if (fReportErr)
    {
        if (fWillRetry && ERROR_FILE_NOT_FOUND == err)
        {
            //  this means that we need to update the
            //  command so that we can try DDE again after
            //  starting the app up...
            // if it wasn't found, determine the correct command

            _QueryString(0, ASSOCSTR_DDEIFEXEC, _szDDECmd, SIZECHARS(_szDDECmd));

            return FALSE;
        }
        else
        {

            _ReportWin32(err);
        }
    }

    return TRUE;
}

BOOL CShellExecute::_SetDDEInfo(void)
{
    ASSERT(_pqa);

    if (SUCCEEDED(_QueryString(0, ASSOCSTR_DDECOMMAND, _szDDECmd, SIZECHARS(_szDDECmd))))
    {
        TraceMsg(TF_SHELLEXEC, "SHEX::SetDDEInfo command: %s", _szDDECmd);

        // Any activation info?
        _fActivateHandler = FAILED(_pqa->GetData(0, ASSOCDATA_NOACTIVATEHANDLER, _pszQueryVerb, NULL, NULL));

        if (SUCCEEDED(_QueryString(0, ASSOCSTR_DDEAPPLICATION, _szTemp, SIZECHARS(_szTemp))))
        {
            TraceMsg(TF_SHELLEXEC, "SHEX::SetDDEInfo application: %s", _szTemp);

            if (_aApplication)
                GlobalDeleteAtom(_aApplication);

            _aApplication = GlobalAddAtom(_szTemp);

            if (SUCCEEDED(_QueryString(0, ASSOCSTR_DDETOPIC, _szTemp, SIZECHARS(_szTemp))))
            {
                TraceMsg(TF_SHELLEXEC, "SHEX::SetDDEInfo topic: %s", _szTemp);

                if (_aTopic)
                    GlobalDeleteAtom(_aTopic);
                _aTopic = GlobalAddAtom(_szTemp);

                _fDDEInfoSet = TRUE;
            }
        }
    }

    TraceMsg(TF_SHELLEXEC, "SHEX::SetDDEInfo returns %d", _fDDEInfoSet);

    return _fDDEInfoSet;
}

BOOL CShellExecute::_TryExecDDE(void)
{
    BOOL fRet = FALSE;
    TraceMsg(TF_SHELLEXEC, "SHEX::TryExecDDE entered ");

    if (_SetDDEInfo())
    {
        //  try the real deal here.  we pass TRUE for fWillRetry because
        //  if this fails to find the app, we will attempt to start
        //  the app and then use DDE again.
        fRet = _DDEExecute(TRUE, _hwndParent, _nShow, _fDDEWait);
    }

    TraceMsg(TF_SHELLEXEC, "SHEX::TryDDEExec() returning %d", fRet);

    return fRet;
}


HRESULT CShellExecute::_SetDarwinCmdTemplate()
{
    HRESULT hr = S_FALSE;

    if (_fAlreadyQueriedDarwin && _szDarwinCmdTemplate[0])
    {
        // we already queried darwin once and got a value, so just use the value
        // that was returned last time and return S_OK to avoid calling darwin twice.
        lstrcpy(_szCmdTemplate, _szDarwinCmdTemplate);

        return S_OK;
    }

    if (SUCCEEDED(_pqa->GetData(0, ASSOCDATA_MSIDESCRIPTOR, _pszQueryVerb, (void *)_wszTemp, (LPDWORD)MAKEINTRESOURCE(SIZEOF(_wszTemp)))))
    {
        SHUnicodeToTChar(_wszTemp, _szTemp, SIZECHARS(_szTemp));

        // call darwin to give us the real location of the app.
        //
        // Note: this call could possibly fault the application in thus
        // installing it on the users machine.
        if (SUCCEEDED(hr = ParseDarwinID(_szTemp, _szDarwinCmdTemplate, SIZECHARS(_szDarwinCmdTemplate))))
        {
            lstrcpy(_szCmdTemplate, _szDarwinCmdTemplate);
            hr = S_OK;
        }
        else
        {
            _ReportWin32(HRESULT_CODE(hr));
        }
    }

    return hr;
}

HRESULT CShellExecute::_QueryString(ASSOCF flags, ASSOCSTR str, LPTSTR psz, DWORD cch)
{
    if (_pqa)
    {
        HRESULT hr = _pqa->GetString(flags, str, _pszQueryVerb, _wszTemp, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(_wszTemp)));

        if (SUCCEEDED(hr))
            SHUnicodeToTChar(_wszTemp, psz, cch);

        return hr;
    }
    return E_FAIL;
}

BOOL CShellExecute::_SetAppRunAsCmdTemplate(void)
{
    DWORD cb = SIZEOF(_szCmdTemplate);
    //  we want to use a special command
    PathToAppPathKey(_szFile, _szTemp, SIZECHARS(_szTemp));

    return (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, _szTemp, TEXT("RunAsCommand"), NULL, _szCmdTemplate, &cb) && *_szCmdTemplate);
}

#if DBG && defined(_X86_)
#pragma optimize("", off) // work around compiler bug
#endif

BOOL CShellExecute::_SetCmdTemplate(void)
{
    HRESULT hr = S_FALSE;

    // we check darwin first since it should override everything else
    if (IsDarwinEnabled())
    {
        // if darwin is enabled, then check for the darwin ID in
        // the registry and set the value based on that.
        hr = _SetDarwinCmdTemplate();
    }

    if (S_OK == hr)
    {
        // darwin was successful, check to see if we need to retry
        // with the new darwin info
        _fRetryExecute =  _ShouldRetryWithNewDarwinInfo();
    }
    else if (S_FALSE == hr)
    {
        // no darwin information in the registry
#ifdef WINNT
        // so now we have to check to see if the NT5 class store will populate our registry
        // with some helpful information (darwin or otherwise)
        _fRetryExecute = _ShouldRetryWithNewClassKey();
#endif
        if (!_fRetryExecute)
        {
            //
            //  both darwin and the class store were unsucessful, so fall back to
            //  the good ole' default command value.
            //
            //  but if we the caller requested NOUI and we
            //  decided to use Unknown as the class
            //  then we should fail here so that
            //  we dont popup the OpenWith dialog box.
            //
            if (!_fClassStoreOnly)
            {
                ASSERT(hr == S_FALSE);

                if (!_fRunAs
                || !PathIsExe(_szFile)
                || !_SetAppRunAsCmdTemplate())
                    hr = _QueryString(0, ASSOCSTR_COMMAND, _szCmdTemplate, SIZECHARS(_szCmdTemplate));
            }
            else
            {
                hr = E_FAIL;
            }
        }
    }

    TraceMsg(TF_SHELLEXEC, "SHEX::SetCmdTemplate() value = %s", _szCmdTemplate);


    if (SUCCEEDED(hr))
        return TRUE;

    //  else
    _ReportWin32(ERROR_NO_ASSOCIATION);
    return FALSE;
}

#if DBG && defined(_X86_)
#pragma optimize("", on) // return to previous optimization level
#endif

BOOL CShellExecute::_ShouldRetryWithNewDarwinInfo()
{
    if (!_fAlreadyQueriedDarwin)
    {
        // darwin was successful and this was the first time during this execute we envoked darwin
        _fAlreadyQueriedDarwin = TRUE;

        // so retry with the new information that darwin could have installed
        // (e.g. darwin apps that use DDE)
        return TRUE;
    }

    // we already envoked darwin during this execute, so dont retry
    return FALSE;
}


#ifdef WINNT
BOOL CShellExecute::_TryWowShellExec(void)
{
    //
    // WOWShellExecute sets this global variable
    //     The cb is only valid when we are being called from wow
    //     If valid use it
    //

    if (lpfnWowShellExecCB)
    {
        SHSTRA strCmd;
        SHSTRA strDir;

        HINSTANCE hinst = (HINSTANCE)SE_ERR_OOM;
        if (SUCCEEDED(strCmd.SetStr(_szCommand)) && SUCCEEDED(strDir.SetStr(_szWorkingDir)))
        {
           if (g_bRunOnNT5)
              hinst = (HINSTANCE)(*(LPFNWOWSHELLEXECCB)lpfnWowShellExecCB)(strCmd.GetStr(), _startup.wShowWindow, strDir.GetStr());
           else
            hinst = (HINSTANCE)(*(LPFNWOWSHELLEXECCB_NT4)lpfnWowShellExecCB)(strCmd.GetStr(), _startup.wShowWindow);
        }

        if (!_ReportHinst(hinst))
        {
            //  SUCCESS!

            //
            // If we were doing DDE, then retry now that the app has been
            // exec'd.  Note we don't keep HINSTANCE returned from _DDEExecute
            // because it will be constant 33 instead of the valid WOW HINSTANCE
            // returned from *lpfnWowShellExecCB above.
            //
            if (_fDDEInfoSet)
            {
                _DDEExecute(NULL, _hwndParent, _nShow, _fDDEWait);
            }
        }

        TraceMsg(TF_SHELLEXEC, "SHEX::TryWowShellExec() used Wow");

        return TRUE;
    }
    return FALSE;
}


BOOL CShellExecute::_ShouldRetryWithNewClassKey(void)
{
    BOOL fRet = FALSE;
    // If this is an app who's association is unknown, we might need to query the ClassStore if
    // we have not already done so.

    // The easiest way we can tell if the file we are going to execute is "Unknown" is by looking for
    // the "QueryClassStore" string value under the hkey we have. DllInstall in shell32 writes this key
    // so that we know when we are dealing with HKCR\Unknown (or any other progid that always wants to
    // do a classtore lookup)
    if (!_fAlreadyQueriedClassStore && !_fNoQueryClassStore &&
        SUCCEEDED(_pqa->GetData(0, ASSOCDATA_QUERYCLASSSTORE, NULL, NULL, NULL)))
    {
        // go hit the NT5 Directory Services class store
        if (_szFile[0])
        {
            INSTALLDATA id;
            LPTSTR pszExtPart;
            WCHAR szFileExt[MAX_PATH];

            // all we have is a filename so whatever PathFindExtension
            // finds, we will use
            pszExtPart = PathFindExtension(_szFile);
            lstrcpy(szFileExt, pszExtPart);

            // Need to zero init id (can't do a = {0} when we declated it, because it has a non-zero enum type)
            ZeroMemory(&id, SIZEOF(INSTALLDATA));

            id.Type = FILEEXT;
            id.Spec.FileExt = szFileExt;

            // call the DS to lookup the file type in the class store
            if (ERROR_SUCCESS == InstallApplication(&id))
            {
                // Since InstallApplication succeeded, it could have possibly installed and app
                // or munged the registry so that we now have the necesssary reg info to
                // launch the app. So basically re-read the class association to see if there is any
                // new darwin info or new normal info, and jump back up and retry to execute.
                LPITEMIDLIST pidlUnkFile = ILCreateFromPath(_szFile);

                if (pidlUnkFile)
                {
                    IQueryAssociations *pqa;
                    if (SUCCEEDED(SHGetAssociations(pidlUnkFile, (void **)&pqa)))
                    {
                        _pqa->Release();
                        _pqa = pqa;

                        if (_pszQueryVerb && (lstrcmpi(_pszQueryVerb, TEXT("openas")) == 0))
                        {
                            // Since we just sucessfully queried the class store, if our verb was "openas" (meaning
                            // that we used the Unknown key to do the execute) we always reset the verb to the default.
                            // If we do not do this, then we could fail the execute since "openas" is most likely not a
                            // supported verb of the application
                            _pszQueryVerb = NULL;
                        }
                    }

                    ILFree(pidlUnkFile);

                    _fAlreadyQueriedClassStore = TRUE;
                    _fClassStoreOnly = FALSE;
                    fRet = TRUE;
                }

            } // CoGetClassInfo

        } // _szFile[0]

    }

    TraceMsg(TF_SHELLEXEC, "SHEX::ShouldRWNCK() returning %d", fRet);

    return fRet;
}

#endif //WINNT

BOOL CShellExecute::_TryInProcess(LPSHELLEXECUTEINFO pei)
{
    BOOL fRet = FALSE;
    //
    //  BUGBUGREMOVE - this is just to handle internet shortcuts - ZekeL 28-SEP-98
    //  if we can setup internet shortcuts to work the same as
    //  normal LNK files, then we can obviate this check.  this is
    //  done in TryExecPidl() but right now the IntShCut object doesnt
    //  do any default verbs, so we dont notice...
    //

    //  thunk the values to get pass to the real TryInProcess...
    HKEY hk;
    if (SUCCEEDED(_pqa->GetKey(0, ASSOCKEY_SHELLEXECCLASS, _pszQueryVerb, &hk)))
    {
        StrCpy(_szTemp, TEXT("shell\\"));
        StrCatBuff(_szTemp, _pszQueryVerb ? pei->lpVerb : TEXT("open"), SIZECHARS(_szTemp));

        if (S_FALSE != TryInProcess(pei, hk, _szTemp, pei->lpVerb))
        {
            _ReportHinst(pei->hInstApp);
            fRet = TRUE;
        }

        RegCloseKey(hk);
    }

    return fRet;
}

BOOL CShellExecute::_TryHooks(LPSHELLEXECUTEINFO pei)
{
    BOOL fRet = FALSE;
    if (_UseHooks(pei->fMask))
    {
        //  BUGBUG the only client of this are URLs.
        //  if we change psfInternet to return IID_IQueryAssociations,
        //  then we can kill the urlexechook  (our only client)

        if (S_FALSE != TryShellExecuteHooks(pei))
        {
            //  either way we always exit.  should get TryShellhook to use SetLastError()
            _ReportHinst(pei->hInstApp);
            fRet = TRUE;;
        }
    }

    return fRet;
}

void CShellExecute::ExecuteNormal(LPSHELLEXECUTEINFO pei)
{
    SetAppStartingCursor(pei->hwnd, TRUE);

    _Init(pei);

    //
    //  Copy the specified directory in _szWorkingDir if the working
    // directory is specified; otherwise, get the current directory there.
    //
    _SetWorkingDir(pei->lpDirectory);

    //
    //  Copy the file name to _szFile, if it is specified. Then,
    // perform environment substitution.
    //
    _SetFile(pei->lpFile);

    //
    //  If the specified filename is a UNC path, validate it now.
    //
    if (_TryValidateUNC(_szFile, pei))
        goto Quit;


    if (_TryHooks(pei))
        goto Quit;

    //
    // If we're explicitly given a class then we don't care if the file exists.
    // Just let the handler for the class worry about it, and _TryExecPidl()
    // will return S_FALSE.
    //

    if(_TryExecPidl(pei))
        goto Quit;

    // Is the class key provided?
    if (!_InitAssociations(pei))
        goto Quit;

    do
    {
        // check for both the CacheFilename and URL being passed to us,
        // if this is the case, we need to check to see which one the App
        // wants us to pass to it.
        if (pei->fMask & SEE_MASK_FILEANDURL)
            _SetFileAndUrl(pei->lpFile);

        if (_TryInProcess(pei))
            goto Quit;

        //  Try using DDE stuff
        if (_TryExecDDE())
            goto Quit;


        //  SetCmdTemplate() will set this to true
        //  if the pqa has been redirected...
        _fRetryExecute = FALSE;

        // check to see if darwin is enabled on the machine
        if (!_SetCmdTemplate())
            goto Quit;

    } while (_fRetryExecute);

    // At this point, the _szFile should have been determined one way
    // or another.
    ASSERT(_szFile[0] || _szCmdTemplate[0]);

    // do we have the necessary RegDB info to do an exec?

    _TryExecCommand();

Quit:

    //
    //  we should only see this if the registry is corrupted.
    //  but we still want to be able to open EXE's
#ifdef DEBUG
    if (_fTryOpenExe)
        TraceMsg(TF_WARNING, "SHEX - trying EXE with no Associations - %s", _szFile);
#endif // DEBUG

    if (_fTryOpenExe)
        _TryOpenExe();

    if (_err == ERROR_SUCCESS && UEMIsLoaded()) {
        int i;
        // skip the call if stuff isn't there yet.
        // the load is expensive (forces ole32.dll and browseui.dll in
        // and then pins browseui).

        // however we ran the app (exec, dde, etc.), we succeeded.  do our
        // best to guess the association etc. and log it.
        i = GetUEMAssoc(_szFile, _szImageName);
        TraceMsg(DM_MISC, "cse.e: GetUEMAssoc()=%d", i);
        UEMFireEvent(&UEMIID_SHELL, UEME_INSTRBROWSER, UEMF_INSTRUMENT, UIBW_RUNASSOC, (LPARAM)i);
    }

    SetAppStartingCursor(pei->hwnd, FALSE);

}

BOOL CShellExecute::_Cleanup(BOOL fSucceeded)
{
    // Clean this up if the exec failed
    if(!fSucceeded && _pidlGlobal)
        SHFreeShared((HANDLE)_pidlGlobal,GetCurrentProcessId());

    if (_aTopic)
        GlobalDeleteAtom(_aTopic);
    if (_aApplication)
        GlobalDeleteAtom(_aApplication);

    if (_pqa)
        _pqa->Release();

    return fSucceeded;
}

BOOL CShellExecute::_FinalMapError(HINSTANCE UNALIGNED64 *phinst)
{

    if (_err != ERROR_SUCCESS)
    {
        // REVIEW: if errWin32 == ERROR_CANCELLED, we may want to
        // set hInstApp to 42 so silly people who don't check the return
        // code properly won't put up bogus messages. We should still
        // return FALSE. But this won't help everything and we should
        // really evangelize the proper use of ShellExecuteEx. In fact,
        // if we do want to do this, we should do it in ShellExecute
        // only. (This will force new people to do it right.)


        // Map FNF for drives to something slightly more sensible.
        if (_err == ERROR_FILE_NOT_FOUND && PathIsRoot(_szFile) &&
            !PathIsUNC(_szFile))
        {
            // NB CD-Rom drives with disk missing will hit this.
            if ((DriveType(DRIVEID(_szFile)) == DRIVE_CDROM) ||
                (DriveType(DRIVEID(_szFile)) == DRIVE_REMOVABLE))
                _err = ERROR_NOT_READY;
            else
                _err = ERROR_BAD_UNIT;
        }

        SetLastError(_err);

        if (phinst)
            *phinst = _MapWin32ErrToHINST(_err);

    }
    else if (phinst)
    {
        if (!_hInstance)
        {
            *phinst = (HINSTANCE) 42;
        }
        else
            *phinst = _hInstance;

        ASSERT(ISSHELLEXECSUCCEEDED(*phinst));
    }

    TraceMsg(TF_SHELLEXEC, "SHEX::FinalMapError() returning err = %d, hinst = %d", _err, _hInstance);

    return (_err == ERROR_SUCCESS);
}

BOOL CShellExecute::Finalize(LPSHELLEXECUTEINFO pei)
{
    _Cleanup(_err == ERROR_SUCCESS);

    if (_pi.hProcess)
    {
        //
        //  BUGBUGLEGACY - change from win95 behavior - zekel 3-APR-98
        //  in win95 we would close the proces but return a handle.
        //  the handle was invalid of course, but some crazy app could be
        //  using this value to test for success.  i am assuming that they
        //  are using one of the other three ways to determine success,
        //  and we can follow the spec and return NULL if we close it.
        //
        //  PEIOUT - set the hProcess if they are going to use it.
        if (_err == ERROR_SUCCESS
        && (pei->fMask & SEE_MASK_NOCLOSEPROCESS))
        {
            pei->hProcess = _pi.hProcess;
        }
        else
        {
            CloseHandle(_pi.hProcess);
        }

        CloseHandle(_pi.hThread);
    }

    //
    //  NOTE:  _FinalMapError() actually calls SetLastError() with our best error
    //  if any win32 apis are called after this, they can reset LastError!!
    //
    return _FinalMapError(&(pei->hInstApp));
}

//
//  Both the Reports return back TRUE if there was an error
//  or FALSE if it was a Success.
//
BOOL CShellExecute::_ReportWin32(DWORD err)
{
    ASSERT(!_err);
    TraceMsg(TF_SHELLEXEC, "SHEX::ReportWin32 reporting err = %d", err);

    _err = err;
    return (err != ERROR_SUCCESS);
}

BOOL CShellExecute::_ReportHinst(HINSTANCE hinst)
{
    ASSERT(!_hInstance);
    TraceMsg(TF_SHELLEXEC, "SHEX::ReportHinst reporting hinst = %d", hinst);
    if(ISSHELLEXECSUCCEEDED(hinst) || !hinst)
    {
        _hInstance = hinst;
        return FALSE;
    }
    else
        return _ReportWin32(_MapHINSTToWin32Err(hinst));
}

typedef struct {
    DWORD errWin32;
    UINT se_err;
} SHEXERR;

// one to one errs
//  ERROR_FILE_NOT_FOUND             SE_ERR_FNF              2       // file not found
//  ERROR_PATH_NOT_FOUND             SE_ERR_PNF              3       // path not found
//  ERROR_ACCESS_DENIED              SE_ERR_ACCESSDENIED     5       // access denied
//  ERROR_NOT_ENOUGH_MEMORY          SE_ERR_OOM              8       // out of memory
#define ISONE2ONE(e)   (e == SE_ERR_FNF || e == SE_ERR_PNF || e == SE_ERR_ACCESSDENIED || e == SE_ERR_OOM)

//  no win32 mapping SE_ERR_DDETIMEOUT               28
//  no win32 mapping SE_ERR_DDEBUSY                  30
//  but i dont see any places where this is returned.
//  before they became the win32 equivalent...ERROR_OUT_OF_PAPER or ERROR_READ_FAULT
//  now they become ERROR_DDE_FAIL.
//  BUGBUG but we wont preserve these errors in the pei->hInstApp
#define ISUNMAPPEDHINST(e)   (e == 28 || e == 30)

//  **WARNING** .  ORDER is IMPORTANT.
//  if there is more than one mapping for an error,
//  (like SE_ERR_PNF) then the first
const SHEXERR c_rgShexErrs[] = {
    {ERROR_SHARING_VIOLATION, SE_ERR_SHARE},
    {ERROR_OUTOFMEMORY, SE_ERR_OOM},
    {ERROR_BAD_PATHNAME,SE_ERR_PNF},
    {ERROR_BAD_NETPATH,SE_ERR_PNF},
    {ERROR_PATH_BUSY,SE_ERR_PNF},
    {ERROR_NO_NET_OR_BAD_PATH,SE_ERR_PNF},
    {ERROR_OLD_WIN_VERSION,10},
    {ERROR_APP_WRONG_OS,12},
    {ERROR_RMODE_APP,15},
    {ERROR_SINGLE_INSTANCE_APP,16},
    {ERROR_INVALID_DLL,20},
    {ERROR_NO_ASSOCIATION,SE_ERR_NOASSOC},
    {ERROR_DDE_FAIL,SE_ERR_DDEFAIL},
    {ERROR_DDE_FAIL,SE_ERR_DDEBUSY},
    {ERROR_DDE_FAIL,SE_ERR_DDETIMEOUT},
    {ERROR_DLL_NOT_FOUND,SE_ERR_DLLNOTFOUND}
};

DWORD CShellExecute::_MapHINSTToWin32Err(HINSTANCE hinst)
{
    DWORD errWin32 = 0;
    UINT_PTR se_err = (UINT_PTR) hinst;

    ASSERT(se_err);
    ASSERT(!ISSHELLEXECSUCCEEDED(se_err));

    // i actually handle these, but it used to be that these
    // became mutant win32s.  now they will be lost
    // i dont think these occur anymore
    AssertMsg(!ISUNMAPPEDHINST(se_err), TEXT("SHEX::COMPATIBILITY SE_ERR = %d, Get ZekeL!!!"), se_err);

    if (ISONE2ONE(se_err))
    {
        errWin32 = (DWORD) se_err;
    }
    else for (int i = 0; i < ARRAYSIZE(c_rgShexErrs) ; i++)
    {
        if (se_err == c_rgShexErrs[i].se_err)
        {
            errWin32= c_rgShexErrs[i].errWin32;
            break;
        }
    }

    ASSERT(errWin32);

    return errWin32;
}


HINSTANCE CShellExecute::_MapWin32ErrToHINST(UINT errWin32)
{
    ASSERT(errWin32);

    UINT se_err = 0;
    if (ISONE2ONE(errWin32))
    {
        se_err = errWin32;
    }
    else for (int i = 0; i < ARRAYSIZE(c_rgShexErrs) ; i++)
    {
        if (errWin32 == c_rgShexErrs[i].errWin32)
        {
            se_err = c_rgShexErrs[i].se_err;
            break;
        }
    }

    if (!se_err)
    {
        //  NOTE legacy error handling  - zekel - 20-NOV-97
        //  for any unhandled win32 errors, we default to ACCESS_DENIED
        ASSERT(errWin32 >= SE_ERR_SHARE);
        se_err = SE_ERR_ACCESSDENIED;
    }

    return (HINSTANCE) se_err;
}


BOOL ShellExecuteNormal(LPSHELLEXECUTEINFO pei)
{
    BOOL fRet = FALSE;
    TraceMsg(TF_SHELLEXEC, "ShellExecuteNormal Using CShellExecute");

    //  WARNING Dont use up Stack Space
    //  we allocate because of win16 stack issues
    //  and the shex is a big object
    CShellExecute *shex = new CShellExecute();

    if (!shex)
    {
        pei->hInstApp = (HINSTANCE)SE_ERR_OOM;
        SetLastError(ERROR_OUTOFMEMORY);
    }
    else
    {
        shex->ExecuteNormal(pei);
        fRet = shex->Finalize(pei);
        delete shex;
    }

    TraceMsg(TF_SHELLEXEC, "ShellExecuteNormal returning %d, win32 = %d, hinst = %d", fRet, GetLastError(), pei->hInstApp);

    return fRet;
}

#ifdef WINNT

BOOL CShellExecute::Init(PSHCREATEPROCESSINFO pscpi)
{
    TraceMsg(TF_SHELLEXEC, "SHEX::Init(pscpi)");

    _SetMask(pscpi->fMask);

    _lpParameters= pscpi->pszParameters;

    //  we always do "runas"
    StrCpyN(_wszVerb, TEXT("runas"), SIZECHARS(_wszVerb));
    _pszQueryVerb = _wszVerb;
    _fRunAs = TRUE;

    if (pscpi->lpStartupInfo)
    {
        _nShow = pscpi->lpStartupInfo->wShowWindow;
        _startup = *(pscpi->lpStartupInfo);
    }
    else    // require startupinfo
        return !(_ReportWin32(ERROR_INVALID_PARAMETER));

    //
    //  Copy the specified directory in _szWorkingDir if the working
    // directory is specified; otherwise, get the current directory there.
    //
    _SetWorkingDir(pscpi->pszCurrentDirectory);

    //
    //  Copy the file name to _szFile, if it is specified. Then,
    // perform environment substitution.
    //
    _SetFile(pscpi->pszFile);

    _pProcAttrs = pscpi->lpProcessAttributes;
    _pThreadAttrs = pscpi->lpThreadAttributes;
    _fInheritHandles = pscpi->bInheritHandles;
    _hUserToken = pscpi->hUserToken;
    //  createflags already inited by _SetMask() just
    //  add the users in.
    _dwCreateFlags |= pscpi->dwCreationFlags;
    _hwndParent = pscpi->hwnd;

    return TRUE;
}


void CShellExecute::ExecuteProcess(void)
{
    SetAppStartingCursor(_hwndParent, TRUE);

    //
    //  If the specified filename is a UNC path, validate it now.
    //
    if (_TryValidateUNC(_szFile, NULL))
        goto Quit;

    if (!_Resolve())
        goto Quit;

    if (!_InitAssociations(NULL))
        goto Quit;

    do
    {
        // check for both the CacheFilename and URL being passed to us,
        // if this is the case, we need to check to see which one the App
        // wants us to pass to it.
        // if (pei->fMask & SEE_MASK_FILEANDURL)
        //    _SetFileAndUrl(pei->lpFile);

        //  SetCmdTemplate() will set this to true
        //  if the pqa has been redirected...
        _fRetryExecute = FALSE;

        // check to see if darwin is enabled on the machine
        if (!_SetCmdTemplate())
            goto Quit;

    } while (_fRetryExecute);

    // At this point, the _szFile should have been determined one way
    // or another.
    ASSERT(_szFile[0] || _szCmdTemplate[0]);

    // do we have the necessary RegDB info to do an exec?

    _TryExecCommand();

Quit:

    //
    //  we should only see this if the registry is corrupted.
    //  but we still want to be able to open EXE's
    RIP(!_fTryOpenExe);
    if (_fTryOpenExe)
        _TryOpenExe();

    if (_err == ERROR_SUCCESS && UEMIsLoaded())
    {
        int i;
        // skip the call if stuff isn't there yet.
        // the load is expensive (forces ole32.dll and browseui.dll in
        // and then pins browseui).

        // however we ran the app (exec, dde, etc.), we succeeded.  do our
        // best to guess the association etc. and log it.
        i = GetUEMAssoc(_szFile, _szImageName);
        TraceMsg(DM_MISC, "cse.e: GetUEMAssoc()=%d", i);
        UEMFireEvent(&UEMIID_SHELL, UEME_INSTRBROWSER, UEMF_INSTRUMENT, UIBW_RUNASSOC, (LPARAM)i);
    }

    SetAppStartingCursor(_hwndParent, FALSE);

}

BOOL CShellExecute::Finalize(PSHCREATEPROCESSINFO pscpi)
{
    _Cleanup(_err == ERROR_SUCCESS);

    if (_pi.hProcess)
    {
        if (!(pscpi->fMask & SEE_MASK_NOCLOSEPROCESS))
        {
            CloseHandle(_pi.hProcess);
            _pi.hProcess = NULL;
            CloseHandle(_pi.hThread);
            _pi.hThread = NULL;
        }

        if (_err == ERROR_SUCCESS
        && pscpi->lpProcessInformation)
        {
            *(pscpi->lpProcessInformation) = _pi;
        }

    }
    else if (pscpi->lpProcessInformation)
        ZeroMemory(pscpi->lpProcessInformation, SIZEOF(_pi));

    //
    //  NOTE:  _FinalMapError() actually calls SetLastError() with our best error
    //  if any win32 apis are called after this, they can reset LastError!!
    //
    return _FinalMapError(NULL);
}

#if 0 // we only support the UNICODE export
//  if we want to add the ANSI export, then we need to change
//  shellapi.w to have these declarations instead
//
typedef struct _SHCREATEPROCESSINFO%
{
        DWORD cbSize;
        ULONG fMask;
        HWND hwnd;
        LPCTSTR% pszFile;
        LPCTSTR% pszParameters;
        LPCTSTR% pszCurrentDirectory;
        IN HANDLE hUserToken;
        IN LPSECURITY_ATTRIBUTES lpProcessAttributes;
        IN LPSECURITY_ATTRIBUTES lpThreadAttributes;
        IN BOOL bInheritHandles;
        IN DWORD dwCreationFlags;
        IN LPSTARTUPINFO% lpStartupInfo;
        OUT LPPROCESS_INFORMATION lpProcessInformation;
} SHCREATEPROCESSINFO%, *PSHCREATEPROCESSINFO%;

SHSTDAPI_(BOOL) SHCreateProcessAsUser%(PSHCREATEPROCESSINFO% pscpi);

#endif // 0

SHSTDAPI_(BOOL) SHCreateProcessAsUserW(PSHCREATEPROCESSINFOW pscpi)
{
    BOOL fRet = FALSE;
    TraceMsg(TF_SHELLEXEC, "SHCreateProcess using CShellExecute");

    //  WARNING Dont use up Stack Space
    //  we allocate because of win16 stack issues
    //  and the shex is a big object
    CShellExecute *pshex = new CShellExecute();

    if (pshex)
    {
        if (pshex->Init(pscpi))
            pshex->ExecuteProcess();

        fRet = pshex->Finalize(pscpi);

        delete pshex;
    }
    else
        SetLastError(ERROR_OUTOFMEMORY);

    TraceMsg(TF_SHELLEXEC, "SHCreateProcess returning %d, win32 = %d", fRet, GetLastError());

    if (!fRet)
        _DisplayShellExecError(pscpi->fMask, pscpi->hwnd, pscpi->pszFile, NULL, GetLastError());

    return fRet;
}

#define STR_BLANK ""

HINSTANCE  APIENTRY WOWShellExecute(
    HWND  hwnd,
    LPCSTR lpOperation,
    LPCSTR lpFile,
    LPSTR lpParameters,
    LPCSTR lpDirectory,
    INT nShowCmd,
    LPVOID lpfnCBWinExec)
{
   HINSTANCE hinstRet;

   lpfnWowShellExecCB = lpfnCBWinExec;

   if (!lpParameters)
       lpParameters = STR_BLANK;

   hinstRet = RealShellExecuteExA(hwnd,lpOperation,lpFile,lpParameters,
      lpDirectory,NULL, (LPCSTR)STR_BLANK,NULL,(WORD)nShowCmd, NULL, 0);

   lpfnWowShellExecCB = NULL;

   return(hinstRet);
}

#endif //WINNT

void _ShellExec_RunDLL(HWND hwnd, HINSTANCE hAppInstance, LPTSTR pszCmdLine, int nCmdShow)
{
    TCHAR szQuotedCmdLine[MAX_PATH * 2];
    SHELLEXECUTEINFO ei = {0};
    ULONG fMask = SEE_MASK_FLAG_DDEWAIT;
    LPTSTR pszArgs;

    // Don't let empty strings through, they will endup doing something dumb
    // like opening a command prompt or the like
    if (!pszCmdLine || !*pszCmdLine)
        return;

    //
    //   the flags are prepended to the command line like:
    //   "?0x00000001?" "cmd line"
    //
    if (pszCmdLine[0] == TEXT('?'))
    {
        //  these are the fMask flags
        int i;
        if (StrToIntEx(++pszCmdLine, STIF_SUPPORT_HEX, &i))
        {
            fMask |= i;
        }

        pszCmdLine = StrChr(pszCmdLine, TEXT('?'));

        if (!pszCmdLine)
            return;

        pszCmdLine++;
    }

    // Gross, but if the process command fails, copy the command line to let
    // shell execute report the errors
    if (PathProcessCommand(pszCmdLine, szQuotedCmdLine, ARRAYSIZE(szQuotedCmdLine),
                           PPCF_ADDARGUMENTS|PPCF_FORCEQUALIFY) == -1)
        StrCpyN(szQuotedCmdLine, pszCmdLine, SIZECHARS(szQuotedCmdLine));

    pszArgs = PathGetArgs(szQuotedCmdLine);
    if (*pszArgs)
        *(pszArgs - 1) = 0; // Strip args

    PathUnquoteSpaces(szQuotedCmdLine);

    ei.cbSize          = sizeof(SHELLEXECUTEINFO);
    ei.hwnd            = hwnd;
    ei.lpFile          = szQuotedCmdLine;
    ei.lpParameters    = pszArgs;
    ei.nShow           = nCmdShow;
    ei.fMask           = fMask;

    //  if shellexec() fails we want to pass back the error.
    if (!ShellExecuteEx(&ei))
    {
        DWORD err = GetLastError();

        if (InRunDllProcess())
            ExitProcess(err);
    }

}

STDAPI_(void) ShellExec_RunDLLA(HWND hwnd, HINSTANCE hAppInstance, LPSTR pszCmdLine, int nCmdShow)
{
    SHSTR str;
    if (SUCCEEDED(str.SetStr(pszCmdLine)))
        _ShellExec_RunDLL(hwnd, hAppInstance, str, nCmdShow);
}


STDAPI_(void) ShellExec_RunDLLW(HWND hwnd, HINSTANCE hAppInstance, LPWSTR pszCmdLine, int nCmdShow)
{
    SHSTR str;
    if (SUCCEEDED(str.SetStr(pszCmdLine)))
        _ShellExec_RunDLL(hwnd, hAppInstance, str, nCmdShow);
}



