/**************************************************************\
    FILE: shellurl.cpp

    DESCRIPTION:
        Implements CShellUrl.
\**************************************************************/

#include "priv.h"
#include "resource.h"
#include "dbgmem.h"
#include "util.h"
#include "shellurl.h"

#include "bandprxy.h"

#include "mluisupp.h"

#ifdef UNIX
#include "unixstuff.h"
#endif

//#define FEATURE_WILDCARD_SUPPORT

#define CH_DOT                TEXT('.')
#define CH_SPACE              TEXT(' ')
#define CH_SEPARATOR          TEXT('/')
#define CH_FRAGMENT           TEXT('#')

#ifdef FEATURE_WILDCARD_SUPPORT
#define CH_ASTRISK            TEXT('*')
#define CH_QUESTIONMARK       TEXT('?')
#endif // FEATURE_WILDCARD_SUPPORT

#define SZ_SPACE              TEXT(" ")
#define SZ_SEPARATOR          TEXT("/")
#define SZ_UNC                TEXT("\\\\")

#ifndef UNIX
#define CH_FILESEPARATOR      TEXT('\\')
#define SZ_FILESEPARATOR      TEXT("\\")
#else
#define CH_FILESEPARATOR      TEXT('/')
#define SZ_FILESEPARATOR      TEXT("/")
#endif

#define CE_PATHGROW 1

#define IS_SHELL_SEPARATOR(ch) ((CH_SEPARATOR == ch) || (CH_FILESEPARATOR == ch))

// Private Functions
BOOL _FixDriveDisplayName(LPCTSTR pszStart, LPCTSTR pszCurrent, LPCITEMIDLIST pidl);

#define TF_CHECKITEM 0 // TF_BAND|TF_GENERAL


/****************************************************\
    CShellUrl Constructor
\****************************************************/
CShellUrl::CShellUrl()
{
    TraceMsg(TF_SHDLIFE, "ctor CShellUrl %x", this);

    // Don't want this object to be on the stack
    ASSERT(!m_pszURL);
    ASSERT(!m_pszArgs);
    ASSERT(!m_pstrRoot);
    ASSERT(!m_pidl);
    ASSERT(!m_pidlWorkingDir);
    ASSERT(!m_hdpaPath);
    ASSERT(!m_dwGenType);
    ASSERT(!m_hwnd);
}


/****************************************************\
    CShellUrl destructor
\****************************************************/
CShellUrl::~CShellUrl()
{
    Reset();
    if (m_pstrRoot)
    {
        LocalFree(m_pstrRoot);
        m_pstrRoot = NULL;
    }

    if (m_pidlWorkingDir)
        ILFree(m_pidlWorkingDir);

    _DeletePidlDPA(m_hdpaPath);
    TraceMsg(TF_SHDLIFE, "dtor CShellUrl %x", this);
}


/****************************************************\
    FUNCTION: Execute

    PARAMETERS
         pbp - This is the pointer to the interface
               which is needed to find a new topmost
               window or the associated browser window.
         pfDidShellExec (Out Optional) - This parameter
               can be NULL.  If not NULL, it will be set
               to TRUE if this Execute() called ShellExec.
               This is needed by callers that wait for
               DISPID_NAVIGATECOMPLETE which will never happen
               in this case.

    DESCRIPTION:
        This command will determine if the current
    shell url needs to be shell executed or navigated
    to.  If it needs to be navigated to, it will try
    to navigate to the PIDL, otherwise, it will navigate
    to the string version.
\****************************************************/
HRESULT CShellUrl::Execute(IBandProxy * pbp, BOOL * pfDidShellExec, DWORD dwExecFlags)
{
    HRESULT hr = S_FALSE;       // S_FALSE until navigation occurs.
    ULONG ulShellExecFMask = (IsFlagSet(dwExecFlags, SHURL_EXECFLAGS_SEPVDM)) ? SEE_MASK_FLAG_SEPVDM : 0;

    ASSERT(IS_VALID_CODE_PTR(pbp, IBandProxy *));
    ASSERT(!pfDidShellExec || IS_VALID_WRITE_PTR(pfDidShellExec, BOOL));

    if (!EVAL(pbp))
        return E_INVALIDARG;

#ifdef UNIX

    // When trying to execute a shellurl we will first check if it is a local file
    // url. If so check if there is a proper file association with it. If not give
    // error and bail out.
    TCHAR szTmpPath[MAX_URL_STRING];
    BOOL  bCheckForAssoc = FALSE;
    if(m_pidl)
    {
        // Get Path from pidl
        IEGetNameAndFlags(m_pidl, SHGDN_FORPARSING, szTmpPath, SIZECHARS(szTmpPath), NULL);
        //SHTCharToAnsi( szTmpPath, szTmpPathAnsi, ARRAYSIZE(szTmpPathAnsi) );
        // Path is file path only in Ansi ??
        if (PathIsFilePath(szTmpPath) && PathFileExists(szTmpPath) )
             bCheckForAssoc = TRUE;
    }
    else
    if (GetUrlScheme(m_pszURL) == URL_SCHEME_FILE )
    {
         HRESULT hr = S_FALSE;
         TCHAR szQualifiedUrl[MAX_URL_STRING];
         DWORD cchSize = ARRAYSIZE(szQualifiedUrl);
         hr = (ParseURLFromOutsideSource(m_pszURL, szQualifiedUrl, &cchSize, NULL) ? S_OK : E_FAIL);
         if (EVAL(SUCCEEDED(hr)))
         {
             cchSize = ARRAYSIZE(szTmpPath);

             hr = PathCreateFromUrl(szQualifiedUrl, szTmpPath, &cchSize, 0);
             if (EVAL(SUCCEEDED(hr)) && PathFileExists(szTmpPath))
                 bCheckForAssoc = TRUE;
         }
    }

    if (bCheckForAssoc)
    {
        // FileHasProperAssociation returns true for all known
        // file types ( even directories )
        DWORD cch;
        if (!PathIsExe( szTmpPath )
           && !FileHasProperAssociation(szTmpPath))
        {
            MLShellMessageBox(m_hwnd,
               MAKEINTRESOURCE(IDS_SHURL_ERR_NOASSOC),
               MAKEINTRESOURCE(IDS_SHURL_ERR_TITLE),
               (MB_OK | MB_ICONERROR));
            return E_FAIL;
        }
    }

#endif

    // Is the following true: 1) The caller wants other browsers to be able to handle the URLs,
    // 2) The ShellUrl is a Web Url, and 3) IE doesn't own HTML files.
    // If all of these are true, then we will just ShellExec() the URL String so the
    // default handler can handle it.
    // Also if the user wants us to browse in a new process and we are currently in the shell process,
    // we will launch IE to handle the url.

    if ((IsFlagSet(dwExecFlags, SHURL_EXECFLAGS_DONTFORCEIE) && IsWebUrl() && !IsIEDefaultBrowser())
#ifdef BROWSENEWPROCESS_STRICT // "Nav in new process" has become "Launch in new process", so this is no longer needed
    ||  (IsWebUrl() && IsBrowseNewProcessAndExplorer())
#endif
       )
    {
        hr = _UrlShellExec();
        ASSERT(S_OK == hr);
    }

    if ((S_OK != hr) && m_pidl && _CanUseAdvParsing())
    {
        // We will only Shell Exec it if:
        // 1. We want to Force IE (over other web browsers) and it's not browsable, even by non-default owners.
        // 2. It's not browsable by default owners.
        if (!ILIsBrowsable(m_pidl, NULL))
        {
            if (pfDidShellExec)
                *pfDidShellExec = TRUE;

            DEBUG_CODE(TCHAR szDbgBuffer[MAX_PATH];)
            TraceMsg(TF_BAND|TF_GENERAL, "ShellUrl: Execute() Going to _PidlShellExec(>%s<)", Dbg_PidlStr(m_pidl, szDbgBuffer, SIZECHARS(szDbgBuffer)));
            // If NULL == m_pidl, then the String will be used.
            hr = _PidlShellExec(m_pidl, ulShellExecFMask);
        }
    }

    if (S_OK != hr)
    {
        VARIANT vFlags = {0};

        vFlags.vt = VT_I4;
        vFlags.lVal = navAllowAutosearch;

        if (pfDidShellExec)
            *pfDidShellExec = FALSE;

        // We prefer pidls, thank you
        if (m_pidl)
        {
            DEBUG_CODE(TCHAR szDbgBuffer[MAX_PATH];)
            TraceMsg(TF_BAND|TF_GENERAL, "ShellUrl: Execute() Going to pbp->NavigateToPIDL(>%s<)", Dbg_PidlStr(m_pidl, szDbgBuffer, SIZECHARS(szDbgBuffer)));
            hr = pbp->NavigateToPIDL(m_pidl);
        }
        else
        {
            ASSERT(m_pszURL);
            TraceMsg(TF_BAND|TF_GENERAL, "ShellUrl: Execute() Going to pbp->NavigateToURL(%s)", m_pszURL);
#ifdef UNICODE
            hr = pbp->NavigateToURL(m_pszURL, &vFlags);
#else
            WCHAR wszURL[MAX_URL_STRING];
            SHTCharToUnicode(m_pszURL, wszURL, ARRAYSIZE(wszURL));
            hr = pbp->NavigateToURL(wszURL, &vFlags);
#endif
        }

        VariantClearLazy(&vFlags);
    }

#ifdef UNIX
    // PidlExec failed or not done at all.
    // Before passing it for navigation check if it is a shell url
    // if so, execute it.
    // The above comment is nolonger true.  This code is moved here
    // from before the navigate to fix attempt to createprocess problem
    if (S_OK != hr && m_pszURL && IsShellUrl( m_pszURL, TRUE ) )
    {
        if (pfDidShellExec)
            *pfDidShellExec = TRUE;

        hr = _UrlShellExec();
        ASSERT(S_OK == hr);
    }
#endif

    return hr;
}


/****************************************************\
    FUNCTION: _PidlShellExec

    PARAMETERS
        pidl - The Pidl to execute.

    DESCRIPTION:
        This function will call ShellExecEx() on the
    pidl specified.  It will also fill in the Current
    Working Directory and Command Line Arguments if there
    are any.
\****************************************************/
HRESULT CShellUrl::_PidlShellExec(LPCITEMIDLIST pidl, ULONG ulShellExecFMask)
{
    HRESULT hr = E_FAIL;
    SHELLEXECUTEINFO sei = {0};

    ASSERT(IS_VALID_PIDL(pidl));

    DEBUG_CODE(TCHAR szDbgBuffer[MAX_PATH];)
    TraceMsg(TF_BAND|TF_GENERAL, "ShellUrl: _PidlShellExec() Going to execute pidl=>%s<", Dbg_PidlStr(pidl, szDbgBuffer, SIZECHARS(szDbgBuffer)));

    if (m_pidlWorkingDir)
    {
        // note, this must be MAX_URL_STRING since IEGetDisplayName can return a URL.
        WCHAR szCWD[MAX_URL_STRING];

        IEGetDisplayName(m_pidlWorkingDir, szCWD, SHGDN_FORPARSING);
        if (PathIsFilePath(szCWD))
        {
            sei.lpDirectory = szCWD;
        }
    }
    /**** TODO: Get the Current Working Directory of top most window
    if (!sei.lpDirectory || !sei.lpDirectory[0])
    {
        GetCurrentDirectory(SIZECHARS(szCurrWorkDir), szCurrWorkDir);
        sei.lpDirectory = szCurrWorkDir;
    }
    *****/

    sei.cbSize          = sizeof(SHELLEXECUTEINFO);
    sei.lpIDList        = (LPVOID) pidl;
    sei.lpParameters    = m_pszArgs;
    sei.nShow           = SW_SHOWNORMAL;
    sei.fMask           = SEE_MASK_FLAG_NO_UI | (pidl ? SEE_MASK_INVOKEIDLIST : 0) | ulShellExecFMask;
    TraceMsg(TF_BAND|TF_GENERAL, "ShellUrl: _PidlShellExec() Cmd=>%s<, Args=>%s<, WorkDir=>%s<",
                GEN_DEBUGSTR(sei.lpFile), GEN_DEBUGSTR(sei.lpParameters), GEN_DEBUGSTR(sei.lpDirectory));

    if (ShellExecuteEx(&sei))
        hr = S_OK;
    else
    {
#ifdef DEBUG
        DWORD dwGetLastError = GetLastError();
        TraceMsg(TF_ERROR, "ShellUrl: _PidlShellExec() ShellExecuteEx() failed for this item. Cmd=>%s<; dwGetLastError=%lx", GEN_DEBUGSTR(sei.lpParameters), dwGetLastError);
#endif // DEBUG
        hr = E_FAIL;
    }

    return hr;
}


/****************************************************\
    FUNCTION: _UrlShellExec

    DESCRIPTION:
        This function will call ShellExecEx() on the
    URL.  This is so other popular browsers can handle
    the URL if they own HTML and other web files.
\****************************************************/
HRESULT CShellUrl::_UrlShellExec(void)
{
    HRESULT hr = E_FAIL;
    SHELLEXECUTEINFO sei;

    // BUGBUG (scotth): make this be = {0} when we move to the new compiler
    ZeroMemory(&sei, SIZEOF(sei));

    ASSERT(m_pszURL);
    TraceMsg(TF_BAND|TF_GENERAL, "ShellUrl: _UrlShellExec() Going to execute URL=>%s<", m_pszURL);

    sei.cbSize          = sizeof(SHELLEXECUTEINFO);
    sei.lpFile          = m_pszURL;
    sei.nShow           = SW_SHOWNORMAL;
    sei.fMask           = SEE_MASK_FLAG_NO_UI;

    if (m_pszURL && ShellExecuteEx(&sei))
        hr = S_OK;
    else
        hr = E_FAIL;

    return hr;
}

// The following function is identical to ParseURLFromOutsideSource except that it
// enables autocorrect and sets pbWasCorrected to TRUE if the string was corrected
BOOL CShellUrl::_ParseURLFromOutsideSource
(
    LPCWSTR psz,
    LPWSTR pszOut,
    LPDWORD pcchOut,
    LPBOOL pbWasSearchURL,  // if converted to a search string
    LPBOOL pbWasCorrected   // if url was autocorrected
    )
{
    // This is our hardest case.  Users and outside applications might
    // type fully-escaped, partially-escaped, or unescaped URLs at us.
    // We need to handle all these correctly.  This API will attempt to
    // determine what sort of URL we've got, and provide us a returned URL
    // that is guaranteed to be FULLY escaped.

    IURLQualify(psz, UQF_DEFAULT | UQF_AUTOCORRECT, pszOut, pbWasSearchURL, pbWasCorrected);

    //
    //  Go ahead and canonicalize this appropriately
    //
    if (FAILED(UrlCanonicalize(pszOut, pszOut, pcchOut, URL_ESCAPE_SPACES_ONLY)))
    {
        //
        //  we cant resize from here.
        //  NOTE UrlCan will return E_POINTER if it is an insufficient buffer
        //
        return FALSE;
    }

    return TRUE;
}
#ifdef UNICODE
HRESULT CShellUrl::ParseFromOutsideSource(LPCSTR pcszUrlIn, DWORD dwParseFlags, PBOOL pfWasCorrected)
{
    WCHAR wzUrl[MAX_URL_STRING];

    SHAnsiToUnicode(pcszUrlIn, wzUrl, ARRAYSIZE(wzUrl));
    return ParseFromOutsideSource(wzUrl, dwParseFlags, pfWasCorrected);
}
#endif // UNICODE


/****************************************************\
    FUNCTION: _TryQuickParse

    PARAMETERS
        pcszUrlIn - String to parse.
        dwParseFlags - Flags to modify parsing. (Defined in iedev\inc\shlobj.w)

    DESCRIPTION:
        We prefer to call g_psfDesktop->ParseDisplayName()
    and have it do the parse really quickly and without
    enumerating the name space.  We need this for things
    that are parsed but not enumerated, which includes:
    a) hidden files, b) other.

    However, we need to not parse URLs if the caller
    doesn't want to accept them.
\****************************************************/
HRESULT CShellUrl::_TryQuickParse(LPCTSTR pszUrl, DWORD dwParseFlags)
{
    HRESULT hr = E_FAIL;  // E_FAIL means we don't know yet.
    int nScheme = GetUrlScheme(pszUrl);

    // Don't parse unknown schemes because we may
    // want to "AutoCorrect" them later.
    if (URL_SCHEME_UNKNOWN != nScheme)
    {
        if ((dwParseFlags & SHURL_FLAGS_NOWEB) &&
            (URL_SCHEME_INVALID != nScheme) &&
            (URL_SCHEME_UNKNOWN != nScheme) &&
            (URL_SCHEME_MK != nScheme) &&
            (URL_SCHEME_SHELL != nScheme) &&
            (URL_SCHEME_LOCAL != nScheme) &&
            (URL_SCHEME_RES != nScheme) &&
            (URL_SCHEME_ABOUT != nScheme))
        {
            // Skip parsing this because it's a web item, and
            // the caller wants to filter those out.
        }
        else
        {
            hr = IEParseDisplayNameWithBCW(CP_ACP, pszUrl, NULL, &m_pidl);
        }
    }

    return hr;
}


/****************************************************\
    FUNCTION: ParseFromOutsideSource

    PARAMETERS
        pcszUrlIn - String to parse.
        dwParseFlags - Flags to modify parsing. (Defined in iedev\inc\shlobj.w)
        pfWasCorrected - [out] if url was autocorrected (can be null)

    DESCRIPTION:
        Convert a string to a fully qualified shell url.  Parsing
    falls into one of the following categories:

    1. If the URL starts with "\\", check if it's a UNC Path.
    2. If the URL starts something that appears to indicate that it starts
       from the root of the shell name space (Desktop), then check if it
       is an absolute ShellUrl.
    (Only do #3 and #4 if #2 was false)
    3. Check if the string is relative to the Current Working Directory.
    4. Check if the string is relative to one of the items in the
       "Shell Path".
    5. Check if the string is in the system's AppPath or DOS Path.
    6. Check if this is a URL to Navigate to.  This call will pretty much
        always succeeded, because it will suck up anything as an AutoSearch
        URL.
\****************************************************/
HRESULT CShellUrl::ParseFromOutsideSource(LPCTSTR pcszUrlIn, DWORD dwParseFlags, PBOOL pfWasCorrected)
{
    HRESULT hr = E_FAIL;  // E_FAIL means we don't know yet.
    TCHAR szUrlExpanded[MAX_URL_STRING];
    LPTSTR pszUrlInMod = (LPTSTR) szUrlExpanded; // For iteration only
    LPTSTR pszErrorURL = NULL;
    BOOL fPossibleWebUrl = FALSE;
    int nScheme;
    BOOL fDisable = SHRestricted(REST_NORUN);
    m_dwFlags = dwParseFlags;

    if (pfWasCorrected)
        *pfWasCorrected = FALSE;

    if (!pcszUrlIn[0])
        return E_FAIL;

    SHExpandEnvironmentStrings(pcszUrlIn, szUrlExpanded, SIZECHARS(szUrlExpanded));

    PathRemoveBlanks(pszUrlInMod);

    Reset(); // Empty info because we will fill it in if successful or leave empty if we fail.
    TraceMsg(TF_BAND|TF_GENERAL, "ShellUrl: ShellUrlQualify() Begin. pszUrlInMod=%s", pszUrlInMod);
    // The display Name will be exactly what the user entered.
    Str_SetPtr(&m_pszDisplayName, pszUrlInMod);

    nScheme = GetUrlScheme(pszUrlInMod);
    if ((URL_SCHEME_FILE != nScheme) || !fDisable)  // Don't parse FILE: URLs if Start->Run is disabled.
    {
        // For HTTP and FTP we can make a few minor corrections
        if (IsFlagSet(dwParseFlags, SHURL_FLAGS_AUTOCORRECT) &&
            (URL_SCHEME_HTTP == nScheme || URL_SCHEME_FTP == nScheme || URL_SCHEME_HTTPS == nScheme))
        {
            if (S_OK == UrlFixupW(szUrlExpanded, szUrlExpanded, ARRAYSIZE(szUrlExpanded)) &&
                pfWasCorrected)
            {
                *pfWasCorrected = TRUE;
            }
        }

        hr = _TryQuickParse(szUrlExpanded, dwParseFlags);
        if (FAILED(hr))
        {
            // Does this string refer to something in the shell namespace that is
            // not a standard URL AND can we do shell namespace parsing AND
            // can we use advanced parsing on it?
            if (((URL_SCHEME_UNKNOWN == nScheme) ||
                 (URL_SCHEME_SHELL == nScheme) ||
                 (URL_SCHEME_INVALID == nScheme)) &&
                !(SHURL_FLAGS_NOSNS & dwParseFlags) && _CanUseAdvParsing())
            {
                fPossibleWebUrl = TRUE;

                // Yes; is this URL absolute (e.g., "\foo" or "Desktop\foo")?
                if (IS_SHELL_SEPARATOR(pszUrlInMod[0]) ||
                    (S_OK == StrCmpIWithRoot(pszUrlInMod, FALSE, &m_pstrRoot)))
                {
                    // Yes

                    // CASE #1.
                    // It starts with "\\", so it's probably a UNC,
                    // so _ParseUNC() will call _ParseRelativePidl() with the Network
                    // Neighborhood PIDL as the relative location.  This is needed
                    // because commands like this "\\bryanst2\public\program.exe Arg1 Arg2"
                    // that need to be shell executed.
                    if (PathIsUNC(pszUrlInMod))
                    {
                        hr = _ParseUNC(pszUrlInMod, &fPossibleWebUrl, dwParseFlags, FALSE);
                        // If we got this far, don't pass off to Navigation if _ParseUNC() failed.
                        fPossibleWebUrl = FALSE;
                    }

                    if (FAILED(hr))
                    {
                        if (IS_SHELL_SEPARATOR(pszUrlInMod[0]))
                        {
                            pszErrorURL = pszUrlInMod;  // We want to keep the '\' for the error message.
                            pszUrlInMod++;    // Skip past '\'.
                        }

                        // See if we need to advance past a "Desktop".
                        if (S_OK == StrCmpIWithRoot(pszUrlInMod, FALSE, &m_pstrRoot))
                        {
                            pszUrlInMod += lstrlen(m_pstrRoot);
                            if (IS_SHELL_SEPARATOR(pszUrlInMod[0]))
                                pszUrlInMod++;
                            if (!pszUrlInMod[0])
                            {
                                // The only thing the user entered was [...]"desktop"[\]
                                // so just clone the Root pidl.
                                return _SetPidl(&s_idlNULL);
                            }
                        }

                        // CASE #2.  Passing NULL indicates that it should test relative
                        //           to the root.
                        hr = _ParseRelativePidl(pszUrlInMod, &fPossibleWebUrl, dwParseFlags, NULL, FALSE, FALSE);
                    }
                }
                else
                {
                    // No; it is relative
                    int nPathCount = 0;
                    int nPathIndex;

                    if (m_hdpaPath)
                        nPathCount = DPA_GetPtrCount(m_hdpaPath);

                    // CASE #3.  Parse relative to the Current Working Directory.
                    //           Only valid if this object's ::SetCurrentWorkingDir()
                    //           method was called.
                    if (m_pidlWorkingDir)
                    {
                        hr = _ParseRelativePidl(pszUrlInMod, &fPossibleWebUrl, dwParseFlags, m_pidlWorkingDir, TRUE, TRUE);
    #ifdef FEATURE_WILDCARD_SUPPORT
                        if (FAILED(hr) && m_pidlWorkingDir &&
                            !StrChr(pszUrlInMod, CH_SEPARATOR) && !StrChr(pszUrlInMod, CH_FILESEPARATOR))
                        {
                            LPTSTR pszWildCard = StrChr(pszUrlInMod, CH_ASTRISK);
                            if (!pszWildCard)
                                pszWildCard = StrChr(pszUrlInMod, CH_QUESTIONMARK);

                            if (pszWildCard)
                            {
                                IOleWindow * pow;
                                m_pidlWorkingDir
                            }
                        }
    #endif // FEATURE_WILDCARD_SUPPORT

                        if (FAILED(hr))
                        {
                            //
                            // Check if the place we are navigating to is the same as the current
                            // working directory.  If so then there is a good chance that the user just
                            // pressed the enter key / go button in the addressbar and we should simply
                            // refresh the current directory.
                            //
                            WCHAR szCurrentDir[MAX_URL_STRING];
                            HRESULT hr2 = IEGetNameAndFlags(m_pidlWorkingDir, SHGDN_FORPARSING | SHGDN_FORADDRESSBAR, szCurrentDir, ARRAYSIZE(szCurrentDir), NULL);
                            if (FAILED(hr2))
                            {
                                // Sometimes SHGDN_FORPARSING fails and the addressbar then tries SHGDN_NORMAL
                                hr2 = IEGetNameAndFlags(m_pidlWorkingDir, SHGDN_NORMAL | SHGDN_FORADDRESSBAR, szCurrentDir, ARRAYSIZE(szCurrentDir), NULL);
                            }
    
                            if (SUCCEEDED(hr2))
                            {
                                if (0 == StrCmpI(pszUrlInMod, szCurrentDir))
                                {
                                    // It matches so stay in the current working directory
                                    _SetPidl(m_pidlWorkingDir);
                                    hr = S_OK;
                                }
                            }

                        }
                    }
                    else
                    {
                        // TODO: Get the Current Working Directory of the top most window.
                        // hr = _ParseRelativePidl(pszUrlInMod, &fPossibleWebUrl, dwParseFlags, pshurlCWD, TRUE, TRUE);
                    }

                    // CASE #4.  Parse relative to the entries in the "Shell Path".
                    //           Only valid if this object's ::AddPath() method was
                    //           called at least once.
                    for (nPathIndex = 0; FAILED(hr) && nPathIndex < nPathCount; nPathIndex++)
                    {
                        LPITEMIDLIST pidlCurrPath = (LPITEMIDLIST) DPA_GetPtr(m_hdpaPath, nPathIndex);

                        if (EVAL(pidlCurrPath))
                        {
                            ASSERT(IS_VALID_PIDL(pidlCurrPath));
                            hr = _ParseRelativePidl(pszUrlInMod, &fPossibleWebUrl, dwParseFlags, pidlCurrPath, FALSE, FALSE);
                        }
                    }


                    // CASE #5.  We need to see if the beginning of the string matches
                    //           the entry in the AppPaths or DOS Path

                    if (FAILED(hr) && IsFlagClear(dwParseFlags, SHURL_FLAGS_NOPATHSEARCH))
                        hr = _QualifyFromPath(pszUrlInMod, dwParseFlags);
                }
            }
            else
            {
                if (URL_SCHEME_FILE != nScheme)
                    fPossibleWebUrl = TRUE;
            }
        }
    }

    if (FAILED(hr) && !fPossibleWebUrl && !fDisable)
    {
        // Did the caller want to suppress UI (Error Messages)
        if (IsFlagClear(dwParseFlags, SHURL_FLAGS_NOUI))
        {
            if (!pszErrorURL)
                pszErrorURL = pszUrlInMod;
            ASSERT(pszErrorURL);

            // We were able to parse part of it, but failed parsing the second or
            // later segment.  This means we need to inform the user of their
            // misspelling.  They can force AutoSearch with "go xxx" or "? xxx"
            // if they are trying to AutoSearch something that appears in their
            // Shell Name Space.
            MLShellMessageBox(m_hwnd, MAKEINTRESOURCE(IDS_SHURL_ERR_PARSE_FAILED),
                MAKEINTRESOURCE(IDS_SHURL_ERR_TITLE),
                (MB_OK | MB_ICONERROR), pszErrorURL);
        }
    }
    else if (S_OK != hr)
    {
        if (!(dwParseFlags & SHURL_FLAGS_NOWEB))
        {
            TCHAR szQualifiedUrl[MAX_URL_STRING];
            DWORD cchSize = SIZECHARS(szQualifiedUrl);

            SHExpandEnvironmentStrings(pcszUrlIn, szUrlExpanded, SIZECHARS(szUrlExpanded));
            PathRemoveBlanks(szUrlExpanded);

            // Unintialized szQualifiedUrl causes junk characters to appear on
            // addressbar and causes registry corruption on UNIX.
            szQualifiedUrl[0] = TEXT('\0');

            // CASE #6. Just check if this is a URL to Navigate to.  This call will
            //          pretty much always succeeded, because it will suck up
            //          anything as a search URL.
            if (IsFlagSet(dwParseFlags, SHURL_FLAGS_AUTOCORRECT))
            {
                hr = (_ParseURLFromOutsideSource(szUrlExpanded, szQualifiedUrl, &cchSize, NULL, pfWasCorrected) ? S_OK : E_FAIL);
            }
            else
            {
                hr = (ParseURLFromOutsideSource(szUrlExpanded, szQualifiedUrl, &cchSize, NULL) ? S_OK : E_FAIL);
            }
            if (SUCCEEDED(hr))
            {
                SetUrl(szQualifiedUrl, GENTYPE_FROMURL);
                Str_SetPtr(&m_pszDisplayName, szQualifiedUrl);    // The display Name will be exactly what the user entered.
            }

            ASSERT(!m_pidl);
            if (fDisable && SUCCEEDED(hr))
            {
                nScheme = GetUrlScheme(szQualifiedUrl);
                // We will allow all but the following schemes:
                if ((URL_SCHEME_SHELL != nScheme) &&
                    (URL_SCHEME_FILE != nScheme) &&
                    (URL_SCHEME_UNKNOWN != nScheme) &&
                    (URL_SCHEME_INVALID != nScheme))
                {
                    fDisable = FALSE;
                }
            }
        }
    }

    if (fDisable && ((URL_SCHEME_FILE == nScheme) || (URL_SCHEME_INVALID == nScheme) || (URL_SCHEME_UNKNOWN == nScheme))) 
    {
        if (IsFlagClear(dwParseFlags, SHURL_FLAGS_NOUI))
        {
            MLShellMessageBox(m_hwnd, MAKEINTRESOURCE(IDS_SHURL_ERR_PARSE_NOTALLOWED),
                MAKEINTRESOURCE(IDS_SHURL_ERR_TITLE),
                (MB_OK | MB_ICONERROR), pszUrlInMod);
        }
        hr = E_ACCESSDENIED;
        Reset(); // Just in case the caller ignores the return value.
    }

    return hr;
}


/****************************************************\
    FUNCTION: _QualifyFromPath

    PARAMETERS:
        pcszFilePathIn - String that may be in the Path.
        dwFlags - Parse Flags, not currently used.

    DESCRIPTION:
        This function will call _QualifyFromAppPath()
    to see if the item exists in the AppPaths.  If not,
    it will check in the DOS Path Env. variable with a
    call to _QualifyFromDOSPath().
\****************************************************/
HRESULT CShellUrl::_QualifyFromPath(LPCTSTR pcszFilePathIn, DWORD dwFlags)
{
    HRESULT hr = _QualifyFromAppPath(pcszFilePathIn, dwFlags);

    if (FAILED(hr))
        hr = _QualifyFromDOSPath(pcszFilePathIn, dwFlags);

    return hr;
}


/****************************************************\
    FUNCTION: _QualifyFromDOSPath

    PARAMETERS:
        pcszFilePathIn - String that may be in the Path.
        dwFlags - Parse Flags, not currently used.

    DESCRIPTION:
        See if pcszFilePathIn exists in the DOS Path Env
    variable.  If so, set the ShellUrl to that location.
\****************************************************/
HRESULT CShellUrl::_QualifyFromDOSPath(LPCTSTR pcszFilePathIn, DWORD dwFlags)
{
    HRESULT hr = E_FAIL;
    TCHAR szPath[MAX_PATH];
    LPTSTR pszEnd = (LPTSTR) pcszFilePathIn;
    BOOL fContinue = TRUE;

    do
    {
        hr = _GetNextPossibleFullPath(pcszFilePathIn, &pszEnd, szPath, SIZECHARS(szPath), &fContinue);
        if (SUCCEEDED(hr))
        {
            if (PathFindOnPathEx(szPath, NULL, (PFOPEX_OPTIONAL | PFOPEX_COM | PFOPEX_BAT | PFOPEX_PIF | PFOPEX_EXE)))
            {
                _GeneratePidl(szPath, GENTYPE_FROMPATH);
                if (!ILIsFileSysFolder(m_pidl))
                {
                    Str_SetPtr(&m_pszArgs, pszEnd);        // Set aside Args
                    break;
                }
            }
            if (fContinue)
                pszEnd = CharNext(pszEnd);
            hr = E_FAIL;
        }
    }
    while (FAILED(hr) && fContinue);

    return hr;
}


/****************************************************\
    FUNCTION: _QualifyFromAppPath

    PARAMETERS:
        pcszFilePathIn - String that may be in the Path.
        dwFlags - Parse Flags, not currently used.

    DESCRIPTION:
        See if pcszFilePathIn exists in the AppPaths
    Registry Section.  If so, set the ShellUrl to that location.
\****************************************************/
HRESULT CShellUrl::_QualifyFromAppPath(LPCTSTR pcszFilePathIn, DWORD dwFlags)
{
    HRESULT hr = E_FAIL;
    TCHAR szFileName[MAX_PATH];
    TCHAR szRegKey[MAX_PATH];
    DWORD dwType;
    DWORD cbData = sizeof(szFileName);
    DWORD cchNewPathSize;

    StrCpyN(szFileName, pcszFilePathIn, SIZECHARS(szFileName));
    PathRemoveArgs(szFileName);     // Get Rid of Args (Will be added later)
    cchNewPathSize = lstrlen(szFileName);   // Get size so we known where to find args in pcszFilePathIn
    PathAddExtension(szFileName, TEXT(".exe")); // Add extension if needed.

    wnsprintf(szRegKey, ARRAYSIZE(szRegKey), TEXT("%s\\%s"), STR_REGKEY_APPPATH, szFileName);
    if (NOERROR == SHGetValue(HKEY_LOCAL_MACHINE, szRegKey, TEXT(""), &dwType, (LPVOID) szFileName, &cbData))
    {
        // 1. Create Pidl from String.
        hr = _GeneratePidl(szFileName, GENTYPE_FROMPATH);

        // 2. Set aside Args
        ASSERT((DWORD)lstrlen(pcszFilePathIn) >= cchNewPathSize);
        Str_SetPtr(&m_pszArgs, &(pcszFilePathIn[cchNewPathSize]));
    }

    return hr;
}


/****************************************************\
    FUNCTION: _ParseUNC

    PARAMETERS:
        pcszUrlIn - URL, which can be a UNC path.
        pfPossibleWebUrl - Set to FALSE if we find that the user has attempted
                           to enter a Shell Url or File url but misspelled one
                           of the segments.
        dwFlags - Parse Flags
        fQualifyDispName - If TRUE when we known that we need to force the
                           URL to be fully qualified if we bind to the destination.
                           This is needed because we are using state information to
                           find the destination URL and that state information won't
                           be available later.

    DESCRIPTION:
        See if the URL passed in is a valid path
    relative to "SHELL:Desktop/Network Neighborhood".
\****************************************************/
HRESULT CShellUrl::_ParseUNC(LPCTSTR pcszUrlIn, BOOL * pfPossibleWebUrl, DWORD dwFlags, BOOL fQualifyDispName)
{
    HRESULT hr = E_FAIL;
    LPITEMIDLIST pidlNN = NULL;

    SHGetSpecialFolderLocation(NULL, CSIDL_NETWORK, &pidlNN);  // Get Pidl for "Network Neighborhood"
    if (pidlNN)
    {
        hr = _ParseRelativePidl(pcszUrlIn, pfPossibleWebUrl, dwFlags, pidlNN, FALSE, fQualifyDispName);
        ILFree(pidlNN);
    }

    return hr;
}


/****************************************************\
    FUNCTION: _ParseSeparator

    PARAMETERS:
        pidl - PIDL to ISF that has been parsed so far.
        pcszSeg - Str of rest of Url to parse.
        pfPossibleWebUrl - Set to FALSE if we know that the user attempted
                           but failed to enter a correct Shell Url.
        fQualifyDispName - If TRUE when we known that we need to force the
                           URL to be fully qualified if we bind to the
                           destination. This is needed because we are using
                           state information to find the destination URL and
                           that state information won't be available later.

    DESCRIPTION:
        This function is called after at least one
    segment in the SHELL URL has bound to a valid
    Shell Item/Folder (i.e., ITEMID).  It is called
    each time a segment in the Shell Url binds to a PIDL.
    It will then evaluate the rest of the string and
    determine if:
        1. The URL has been completely parsed
            and is valid.  This will include getting
            the command line arguments if appropriate.
        2. More segments in the URL exist and ::_ParseNextSegment()
           needs to be called to continue the recursive parsing
           of the URL.
        3. The rest of the URL indicates that it's an invalid url.

   This function is always called by ::_ParseNextSegment() and basically
   decides if it wants to continue the recursion by calling back into
   ::_ParseNextSegment() or not.  Recursion is used because it's necessary
   to back out of parsing something and go down a path if we received
   a false positive.
\****************************************************/
HRESULT CShellUrl::_ParseSeparator(LPCITEMIDLIST pidl, LPCTSTR pcszSeg, BOOL * pfPossibleWebUrl, BOOL fAllowRelative, BOOL fQualifyDispName)
{
    HRESULT hr = S_OK;
    BOOL fIgnoreArgs = FALSE;

    ASSERT(pidl && IS_VALID_PIDL(pidl));

    // Does anything follow this separator?
    if ((CH_FRAGMENT == pcszSeg[0]) || (IS_SHELL_SEPARATOR(pcszSeg[0]) && pcszSeg[1]))
    {
        // Yes, continue parsing recursively.

        // Do we need to skip the '/' or '\' separator?
        if (CH_FRAGMENT != pcszSeg[0])
            pcszSeg++;      // Skip separator

        hr = _ParseNextSegment(pidl, pcszSeg, pfPossibleWebUrl, fAllowRelative, fQualifyDispName);
        DEBUG_CODE(TCHAR szDbgBuffer[MAX_PATH];)
        TraceMsg(TF_BAND|TF_GENERAL, "ShellUrl: _ParseSeparator() Current Level pidl=>%s<", Dbg_PidlStr(pidl, szDbgBuffer, SIZECHARS(szDbgBuffer)));

        if (FAILED(hr) && pfPossibleWebUrl)
        {
            *pfPossibleWebUrl = FALSE;
            // We bound to at least one level when parsing, so don't do a web search because
            // of a failure.
        }
    }
    else
    {
        // No, we will see if we have reached a valid Shell Item.

        // Is the remaining string args?
        if (CH_SPACE == pcszSeg[0])
        {
            // If there are still chars left in the string, we need to
            // verify the first one is a space to indicate Command line args.
            // Also, we need to make sure the PIDL isn't browsable because browsable
            // Shell folders/items don't take Cmd Line Args.

            if (ILIsBrowsable(pidl, NULL))
            {
                // No
                //
                // The remaining chars cannot be Command Line Args if the PIDL
                // doesn't point to something that is shell executable.  This
                // case actually happens often.
                // Example: (\\bryanst\... and \\bryanst2\.. both exist and
                //          user enters \\bryanst2\... but parsing attempts
                //          to use \\bryanst because it was found first.  This
                //          will cause recursion to crawl back up the stack and try \\bryanst2.
                hr = E_FAIL;
            }
        }
        else if (pcszSeg[0])
        {
            // No
            // The only time we allow a char after a folder segment is if it is a Shell Separator
            // Example: "E:\dir1\"

            if (IS_SHELL_SEPARATOR(*pcszSeg) && 0 == pcszSeg[1])
                fIgnoreArgs = TRUE;
            else
                hr = E_FAIL;    // Invalid because there is more to be parsed.
        }

        if (SUCCEEDED(hr))
        {
            DEBUG_CODE(TCHAR szDbgBuffer[MAX_PATH];)
            TraceMsg(TF_BAND|TF_GENERAL, "ShellUrl: _ParseSeparator() Parsing Finished.  pidl=>%s<", Dbg_PidlStr(pidl, szDbgBuffer, SIZECHARS(szDbgBuffer)));
            _SetPidl(pidl);

            if (!fIgnoreArgs && pcszSeg[0])
                Str_SetPtr(&m_pszArgs, pcszSeg);

            if (fQualifyDispName)
                _GenDispNameFromPidl(pidl, pcszSeg);
        }
    }

    return hr;
}


//
// Returns TRUE is the pidl is a network server
//
BOOL _IsNetworkServer(LPCITEMIDLIST pidl)
{
    BOOL fRet = FALSE;

    // First see if this is a network pidl
    if (IsSpecialFolderChild(pidl, CSIDL_NETWORK, FALSE))
    {
        // See if it ends in a share name
        WCHAR szUrl[MAX_URL_STRING];
        HRESULT hr = IEGetNameAndFlags(pidl, SHGDN_FORPARSING, szUrl, ARRAYSIZE(szUrl), NULL);
        if (FAILED(hr))
        {
            // On non-integrated browsers SHGDN_FORPARSING may fail so try
            // again without this flag.  The preceeding back slashes will be
            // missing so we add them ourselves
            szUrl[0] = CH_FILESEPARATOR;
            szUrl[1] = CH_FILESEPARATOR;
            hr = IEGetNameAndFlags(pidl, SHGDN_NORMAL | SHGDN_FORADDRESSBAR, szUrl+2, ARRAYSIZE(szUrl)-2, NULL);
        }

        fRet = SUCCEEDED(hr) && PathIsUNCServer(szUrl);
    }
    return fRet;
}


/****************************************************\
    FUNCTION: _ParseNextSegment

    PARAMETERS:
        pidlParent - Fully Qualified PIDL to ISF to find next ITEMID in pcszStrToParse.
        pcszStrToParse - pcszStrToParse will begin with either
                      a valid display name of a child ITEMID of pidlParent
                      or the Shell URL is invalid relative to pidlParent.
        fAllowRelative - Should relative moves be allowed?
        fQualifyDispName - If TRUE when we known that we need to force the
                           URL to be fully qualified if we bind to the destination.
                           This is needed because we are using state information to
                           find the destination URL and that state information won't
                           be available later.

    DESCRIPTION/PERF:
        This function exists to take the string (pcszStrToParse)
    passed in and attempt to bind to a ITEMID which
    has a DisplayName that matches the beginning of
    pcszStrToParse.  This function will check all the
    ITEMIDs under the pidlParent section of the Shell
    Name Space.

      The only two exceptions to the above method is if
    1) the string begins with "..", in which case, we
       bind to the pidlParent's Parent ITEMID. - or -
    2) The pidlParent passes the ::_IsFilePidl()
       test and we are guaranteed the item is in the
       File System or a UNC item.  This will allow us
       to call IShellFolder::ParseDisplayName() to
       find the child ITEMID of pidlParent.

    This function will iterate through the items under
    pidlParent instead of call IShellFolder::ParseDisplayName
    for two reasons: 1) The ::ParseDisplayName for "The Internet"
    will accept any string because of AutoSearch, and
    2) We never know the location of the end of one segment and
    the beginning of the next segment in pcszStrToParse.  This is
    because DisplayNames for ISFs can contain almost any character.

    If this function has successfully bind to a child ITEMID
    of pidlParent, it will call ::_ParseSeparator() with the
    rest of pcszStrToParse to parse.  _ParseSeparator() will determine
    if the end of the URL has been parsed or call back into this function
    recursively to continue parsing segments.  In the former case,
    _ParseSeparator() will set this object's PIDL and arguments which
    can be used later.  In the latter case, the recursion stack will
    unwind and my take a different path (Cases exists that require this).
\****************************************************/
HRESULT CShellUrl::_ParseNextSegment(LPCITEMIDLIST pidlParent,
            LPCTSTR pcszStrToParse, BOOL * pfPossibleWebUrl,
            BOOL fAllowRelative, BOOL fQualifyDispName)
{
    HRESULT hr = E_FAIL;

    if (!pidlParent || !pcszStrToParse)
        return E_INVALIDARG;

    // Is this ".."?
    if (fAllowRelative && CH_DOT == pcszStrToParse[0] && CH_DOT == pcszStrToParse[1])
    {
        // Yes
        LPITEMIDLIST pidl = ILClone(pidlParent);
        if (pidl && !ILIsEmpty(pidl))
        {
            ILRemoveLastID(pidl);  // pidl/psfFolder now point to the new shell item, which is the parent in this case.
            DEBUG_CODE(TCHAR szDbgBuffer[MAX_PATH];)
            TraceMsg(TF_BAND|TF_GENERAL, "ShellUrl: _ParseNextSegment() Nav '..'. PIDL=>%s<", Dbg_PidlStr(pidl, szDbgBuffer, SIZECHARS(szDbgBuffer)));

            // Parse the next segment or finish up if we reached the end
            // (we're skipping the ".." here)
            hr = _ParseSeparator(pidl, &(pcszStrToParse[2]), pfPossibleWebUrl, fAllowRelative, fQualifyDispName);
            ILFree(pidl);
        }
    }
    else
    {
        // No
        LPTSTR pszNext = NULL; // Remove const because we will iterate only.

        // Can we parse this display name quickly?
        if (!ILIsRooted(pidlParent) && _IsFilePidl(pidlParent) &&
            
            // Quick way fails for shares right off of the network server
            !_IsNetworkServer(pidlParent))
        {       
            // Yes
            TCHAR szParseChunk[MAX_PATH+1];

            do
            {
                hr = _GetNextPossibleSegment(pcszStrToParse, &pszNext, szParseChunk, SIZECHARS(szParseChunk));
                if (S_OK == hr)
                {
                    hr = _QuickParse(pidlParent, szParseChunk, pszNext, pfPossibleWebUrl, fAllowRelative, fQualifyDispName);

#ifdef FEATURE_SUPPORT_FRAGS_INFILEURLS
                    // Did we fail to parse the traditional way and the first char of this
                    // next chunk indicates it's probably a URL Fragment?
                    if (FAILED(hr) && (CH_FRAGMENT == pcszStrToParse[0]))
                    {
                        TCHAR szUrl[MAX_URL_STRING];
                        // Yes, so try parsing in another way that will work
                        // with URL fragments.

                        hr = ::IEGetDisplayName(pidlParent, szUrl, SHGDN_FORPARSING);
                        if (EVAL(SUCCEEDED(hr)))
                        {
                            TCHAR szFullUrl[MAX_URL_STRING];
                            DWORD cchFullUrlSize = ARRAYSIZE(szFullUrl);

                            hr = UrlCombine(szUrl, szParseChunk, szFullUrl, &cchFullUrlSize, 0);
                            if (EVAL(SUCCEEDED(hr)))
                            {
                                LPITEMIDLIST pidl = NULL;

                                hr = IEParseDisplayName(CP_ACP, szFullUrl, &pidl);
                                if (SUCCEEDED(hr))
                                {
                                    _SetPidl(pidl);

                                    if (fQualifyDispName)
                                        _GenDispNameFromPidl(pidl, szFullUrl);

                                    ILFree(pidl);
                                }
                                else
                                    ASSERT(!pidl);  // Verify IEParseDisplayName() didn't fail but return a pidl.
                            }
                        }
                    }
#endif // FEATURE_SUPPORT_FRAGS_INFILEURLS
                }
            }
            while (FAILED(hr));

            if (S_OK != hr)
                hr = E_FAIL;    // Not Found
        }
        else if (FAILED(hr))
        {
            // No; use the slow method
            IShellFolder * psfFolder = NULL;

            DWORD dwAttrib = SFGAO_FOLDER;
            IEGetAttributesOf(pidlParent, &dwAttrib);

            if (IsFlagSet(dwAttrib, SFGAO_FOLDER))
            {
                IEBindToObject(pidlParent, &psfFolder);
                ASSERT(psfFolder);
            }

            if (psfFolder)
            {
                LPENUMIDLIST penumIDList = NULL;
                HWND hwnd = _GetWindow();

                // Is this an FTP Pidl?
                if (IsFTPFolder(psfFolder))
                {
                    // NT #274795: Yes so, we need to NULL out the hwnd to prevent
                    // displaying UI because enumerator of that folder may need to display
                    // UI (to collect passwords, etc.).  This is not valid because pcszStrToParse
                    // may be an absolute path and psfFolder points to the current location which
                    // isn't valid.  This should probaby be done for all IShellFolder::EnumObjects()
                    // calls, but it's too risky right before ship.
                    hwnd = NULL;
                }

                // Warning Docfind returns S_FALSE to indicate no enumerator and returns NULL..
                if (SUCCEEDED(IShellFolder_EnumObjects(psfFolder, hwnd, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN, &penumIDList)) && penumIDList)
                {
                    LPITEMIDLIST pidlRelative;   // NOT a FULLY Qualified Pidl
                    LPITEMIDLIST pidlResult; // PIDL after it has been made Fully Qualified
                    ULONG cFetched;
                    LPTSTR pszRemaining = NULL;

                    while (FAILED(hr) && NOERROR == penumIDList->Next(1, &pidlRelative, &cFetched) && cFetched)
                    {
                        // The user will have entered the name in one of the three formats and they need to be
                        // checked from the longest string to the smallest.  This is necessary because the
                        // parser will check to see if the item's DisplayName is the first part of the user
                        // string.
                        //
                        // #1. (FORPARSING): This will be the full name.
                        //     Example: razzle.lnk on desktop = D:\nt\public\tools\razzle.lnk.
                        // #2. (FORPARSING | SHGDN_INFOLDER): This will be only the full name w/Extension.
                        //     Example: razzle.lnk on desktop = razzle.lnk
                        // #3. (SHGDN_INFOLDER): This will be the full name w/o extension if "Hide File Extensions for Known File Types" is on.
                        //     Example: razzle.lnk on desktop = D:\nt\public\tools\razzle.lnk.
                        // The user may have entered the "SHGDN_FORPARSING" Display Name or the "SHGDN_INFOLDER", so we need
                        // to check both.
                        hr = _CheckItem(psfFolder, pidlParent, pidlRelative, &pidlResult, pcszStrToParse, &pszRemaining, SHGDN_FORPARSING);
                        if (FAILED(hr))     // Used for file items w/extensions. (Like razzle.lnk on the Desktop)
                            hr = _CheckItem(psfFolder, pidlParent, pidlRelative, &pidlResult, pcszStrToParse, &pszRemaining, SHGDN_FORPARSING | SHGDN_INFOLDER);
                        if (FAILED(hr))
                            hr = _CheckItem(psfFolder, pidlParent, pidlRelative, &pidlResult, pcszStrToParse, &pszRemaining, SHGDN_INFOLDER);

                        if (SUCCEEDED(hr))
                        {
                            // See if the Display Name for a Drive ate the separator for the next segment.
                            if (_FixDriveDisplayName(pcszStrToParse, pszRemaining, pidlResult))
                            {
                                // FIX: "E:\dir1\dir2".  We expent display names to not claim the '\' separator between
                                //                       names.  The problem is that drive letters claim to be "E:\" instead
                                //                       of "E:".  So, we need to back up so we use the '\' as a separator.
                                pszRemaining--;
                            }
#ifndef UNIX
                            // Our root is equal to a separator, so it's N/A on UNIX.
                            ASSERT(pcszStrToParse != pszRemaining);
#endif
                            // Parse the next segment or finish up if we reached the end.
                            hr = _ParseSeparator(pidlResult, pszRemaining, pfPossibleWebUrl, fAllowRelative, fQualifyDispName);

                            if (pidlResult)
                                ILFree(pidlResult);
                        }

                        ILFree(pidlRelative);
                    }
                    penumIDList->Release();
                }
                psfFolder->Release();
            }
        }
    }

    return hr;
}


/****************************************************\
    FUNCTION: _GetNextPossibleSegment

    PARAMETERS:
        pcszFullPath - Full Path
        ppszSegIterator - Pointer to iterator to maintain state.
                          WARNING: This needs to be NULL on first call.
        pszSegOut - Of S_OK is returned, this will contain the next possible segment
        cchSegOutSize - char Size of pszSegOut buffer

    DESCRIPTION:
        Generate the next possible segment that can
    be parsed.  If "one two three/four five" is passed
    in, this function will return S_OK three times
    with these values in pszSegOut:
    1) "one two three",
    2) "one two", and
    3) "one".

    In this example, S_OK will be returned for the first
    three calls, and S_FALSE will be returned for the
    fourth to indicate that no more possible segments can be obtained
    from that string.
\****************************************************/
HRESULT CShellUrl::_GetNextPossibleSegment(LPCTSTR pcszFullPath,
        LPTSTR * ppszSegIterator, LPTSTR pszSegOut, DWORD cchSegOutSize)
{
    HRESULT hr = S_OK;
    LPTSTR szStart = (LPTSTR) pcszFullPath;

    // We need to treat UNCs Specially.
    if (PathIsUNC(szStart))
    {
        LPTSTR szUNCShare;
        // This is a UNC so we need to make the "Segment" include
        // the "\\server\share" because Network Neighborhood's
        // IShellFolder::ParseDisplayName() is increadibly slow
        // and makes mistakes when it parses "server" and then "share"
        // separately.

        // This if clause will advance szStart past the Server
        // section of the UNC path so the rest of the algorithm will
        // naturally continue working on the share section of the UNC.
        szStart += 2;   // Skip past the "\\" UNC header.

        // Is there a share?
        if (szUNCShare = StrChr(szStart, CH_FILESEPARATOR))
        {
            // Yes, so advanced to the first char in the share
            // name so the algorithm below works correctly.
            szStart = szUNCShare + 1;
        }
    }

    // Do we need to initialize the iterator?  If so, set it to the
    // largest possible segment in the string because we will be
    // working backwards.
    ASSERT(ppszSegIterator);
    if (*ppszSegIterator)
    {
        *ppszSegIterator = StrRChr(szStart, *ppszSegIterator, CH_SPACE);
        if (!*ppszSegIterator)
        {
            pszSegOut[0] = TEXT('\0');  // Make sure caller doesn't ignore return and recurse infinitely.
            return S_FALSE;
        }
    }
    else
    {
        // We have not yet started the iteration, so set the ppszSegIterator to the end of the possible
        // segment.  This will be a segment separator character ('\' || '/') or the end of the string
        // if either of those don't exist.  This will be the first segment to try.
#ifndef UNIX
        *ppszSegIterator = StrChr(szStart, CH_FILESEPARATOR);
        if (!*ppszSegIterator)
            *ppszSegIterator = StrChr(szStart, CH_SEPARATOR);
#else
    // On UNIX, we always skip the 1st "/" and go to the 2nd.
    if (szStart[0] == CH_FILESEPARATOR)
            *ppszSegIterator = StrChr(szStart+1, CH_FILESEPARATOR);
#endif

        LPTSTR pszFrag = StrChr(szStart, CH_FRAGMENT);
        // Is the next separator a fragment?
        if (pszFrag && (!*ppszSegIterator || (pszFrag < *ppszSegIterator)))
        {
            TCHAR szFile[MAX_URL_STRING];

            StrCpyN(szFile, szStart, (int)(pszFrag - szStart + 1));
            if (PathIsHTMLFile(szFile))
                *ppszSegIterator = pszFrag;
        }

        if (!*ppszSegIterator)
        {
            // Go to end of the string because this is the last seg.
            *ppszSegIterator = (LPTSTR) &((szStart)[lstrlen(szStart)]);
        }
    }

    // Fill the pszSegOut parameter.
    ASSERT(*ppszSegIterator);

    // This is weird but correct.  pszEnd - pszBeginning results count of chars, not
    // count of bytes.
    if (cchSegOutSize >= (DWORD)((*ppszSegIterator - pcszFullPath) + 1))
        StrCpyN(pszSegOut, pcszFullPath, (int)(*ppszSegIterator - pcszFullPath + 1));
    else
        StrCpyN(pszSegOut, pcszFullPath, cchSegOutSize-1);

    return hr;
}


/****************************************************\
    FUNCTION: _GetNextPossibleFullPath

    DESCRIPTION:
        This function will attempt to see if strParseChunk
    is a Parsible DisplayName under pidlParent.
\****************************************************/
HRESULT CShellUrl::_GetNextPossibleFullPath(LPCTSTR pcszFullPath,
    LPTSTR * ppszSegIterator, LPTSTR pszSegOut, DWORD cchSegOutSize,
    BOOL * pfContinue)
{
    HRESULT hr = S_OK;
    LPTSTR pszNext = StrChr(*ppszSegIterator, CH_SPACE);
    DWORD cchAmountToCopy = cchSegOutSize;

    if (TEXT('\0') == (*ppszSegIterator)[0])
    {
        if (pfContinue)
            *pfContinue = FALSE;
        return E_FAIL;  // Nothing Left.
    }

    if (!pszNext)
        pszNext = &((*ppszSegIterator)[lstrlen(*ppszSegIterator)]);   // Go to end of the string because this is the last seg.

    // Copy as much of the string as we have room for.
    // The compiler will take care of adding '/ sizeof(TCHAR)'.
    if ((cchAmountToCopy-1) > (DWORD)(pszNext - pcszFullPath + 1))
        cchAmountToCopy = (int)(pszNext - pcszFullPath + 1);

    StrCpyN(pszSegOut, pcszFullPath, cchAmountToCopy);

    if (CH_SPACE == pszNext[0])
    {
        *pfContinue = TRUE;
    }
    else
        *pfContinue = FALSE;

    *ppszSegIterator = pszNext;
    return hr;
}


/****************************************************\
    FUNCTION: _QuickParse

    PARAMETERS:
        pidlParent - Pidl to ISF to parse from.
        pszParseChunk - Display Name of item in pidlParent.
        pszNext - Rest of string to parse if we succeed at parsing pszParseChunk.
        pfPossibleWebUrl - Set to FALSE if we find that the user has attempted to enter
                           a Shell Url or File url but misspelled one of the segments.
        fAllowRelative - Allow relative parsing. ("..")
        fQualifyDispName - If TRUE when we known that we need to force the
                           URL to be fully qualified if we bind to the destination.
                           This is needed because we are using state information to
                           find the destination URL and that state information won't
                           be available later.

    DESCRIPTION:
        This function will attempt to see if strParseChunk
    is a Parsible DisplayName under pidlParent.
\****************************************************/
HRESULT CShellUrl::_QuickParse(LPCITEMIDLIST pidlParent, LPTSTR pszParseChunk,
    LPTSTR pszNext, BOOL * pfPossibleWebUrl, BOOL fAllowRelative,
    BOOL fQualifyDispName)
{
    HRESULT hr;
    IShellFolder * psfFolder;

    hr = IEBindToObject(pidlParent, &psfFolder);
    if (SUCCEEDED(hr))
    {
        ULONG ulEatten; // Not used.
        SHSTRW strParseChunkThunked;

        hr = strParseChunkThunked.SetStr(pszParseChunk);
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidl = NULL;

            // TODO: In the future, we may want to cycle through commonly used extensions in case the
            //       user doesn't add them.
            hr = psfFolder->ParseDisplayName(_GetWindow(), NULL, strParseChunkThunked.GetStr(), &ulEatten, &pidl, NULL);
            if (SUCCEEDED(hr))
            {
                // IShellFolder::ParseDisplayName() only generates PIDLs that are relative to the ISF.  We need
                // to make them Absolute.
                LPITEMIDLIST pidlFull = ILCombine(pidlParent, pidl);

                if (pidlFull)
                {
                    // Parse the next segment or finish up if we reached the end.
                    hr = _ParseSeparator(pidlFull, pszNext, pfPossibleWebUrl, fAllowRelative, fQualifyDispName);
                    ILFree(pidlFull);
                }
                ILFree(pidl);
            }
        }
        psfFolder->Release();
    }

    return hr;
}


/****************************************************\
    FUNCTION: _CheckItem

    DESCRIPTION:
        This function will obtain the Display Name
    of the ITEMID (pidlRelative) which is a child of
    psfFolder.  If it's Display Name matches the first
    part of pcszStrToParse, we will return successful
    and set ppszRemaining to the section of pcszStrToParse
    after the segment just parsed.

    This function will also see if the Display Name ends
    in something that would indicate it's executable.
    (.EXE, .BAT, .COM, ...).  If so, we will match if
    pcszStrToParse matches the Display Name without the
    Extension.
\****************************************************/
HRESULT CShellUrl::_CheckItem(IShellFolder * psfFolder,
    LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidlRelative,
    LPITEMIDLIST * ppidlChild, LPCTSTR pcszStrToParse,
    LPTSTR * ppszRemaining, DWORD dwFlags)
{
    HRESULT hr = E_FAIL;
    STRRET sr;

    *ppidlChild = NULL;

    if (SUCCEEDED(psfFolder->GetDisplayNameOf(pidlRelative, dwFlags, &sr)))
    {
        TCHAR szISFName[MAX_URL_STRING];
        DWORD cchISFLen;
        BOOL fEqual = FALSE;

        StrRetToStrN(szISFName, SIZECHARS(szISFName), &sr, pidlRelative);
        cchISFLen = lstrlen(szISFName);
        // Either the item needs to match exactly, or it needs to do a partial match
        // if the Shell Object is an executable file.  For Example: "msdev" should match the
        // "msdev.exe" file object.
        if (cchISFLen > 0)
        {
            // We want to see if pcszStrToParse is a match to the first part of szISFName.

            // First we will try to see if it's a direct match.
            // Example: User="file.exe" Shell Item="file.exe"
            if (0 == StrCmpNI(szISFName, pcszStrToParse, cchISFLen))
            {
                fEqual = TRUE;
            }
            else
            {
                int cchRoot = (int)((PathFindExtension(szISFName)-szISFName));
                // If that failed, we try to see if the Shell Item is
                // executable (.EXE, .COM, .BAT, .CMD, ...) and if so,
                // we will see if pcszStrToParse matches Shell Item w/o the file
                // extension.

                // BUGBUG this will match if there happens to be a space in the user's
                //  filename that doesn't denote commandline arguments.
                //  Example: User="foo file.doc" Shell Item="foo.exe"

                if (PathIsExe(szISFName) &&                         // shell object is executable
                    ((lstrlen(pcszStrToParse) >= cchRoot) &&        // and user entered at least root chars
                     ((pcszStrToParse[cchRoot] == TEXT('\0')) ||    // and user entered exact root
                      (pcszStrToParse[cchRoot] == TEXT(' ')))) &&   //     or possible commandline args
                    (0 == StrCmpNI(szISFName, pcszStrToParse, cchRoot)))    // and the root matches
                {
                    // This wasn't a direct match, but we found that the segment entered
                    // by the user (pcszStrToParse) matched

                    // We found that the ISF item is an executable object and the
                    // string matched w/o the extension.
                    fEqual = TRUE;
                    cchISFLen = cchRoot;        // So that we generate *ppszRemaining correctly
                }
            }
        }

        if (fEqual)
        {
            hr = S_OK;    // We were able to navigate to this shell item token.
            *ppszRemaining = (LPTSTR) &(pcszStrToParse[cchISFLen]); // We will only iterate over the string, so it's ok that we loose the const.
            *ppidlChild = ILCombine(pidlParent, pidlRelative);
            TraceMsg(TF_CHECKITEM, "ShellUrl: _CheckItem() PIDL=>%s< IS EQUAL TO StrIn=>%s<", pcszStrToParse, szISFName);
        }
        else
            TraceMsg(TF_CHECKITEM, "ShellUrl: _CheckItem() PIDL=>%s< not equal to StrIn=>%s<", pcszStrToParse, szISFName);
    }

    return hr;
}


/****************************************************\
    FUNCTION: _IsFilePidl

    PARAMETERS:
        pidl (IN) - Pidl to check if it is a File Pidl

    DESCRIPTION:
        The PIDL is a file pidl if:
    1. The pidl equals "Network Neighborhood" or descendent
    2. The pidl's grandparent or farther removed from "My Computer".

    This algorithm only allows "Network Neighborhood" because
    that ISF contains a huge number of PIDLs and takes for ever
    to enumerate.  The second clause will work in any part of the
    file system except for the root drive (A:\, C:\).  This is
    because we need to allow other direct children of "My Computer"
    to use the other parsing.
\****************************************************/
BOOL CShellUrl::_IsFilePidl(LPCITEMIDLIST pidl)
{
    BOOL fResult = FALSE;
    BOOL fNeedToSkip = FALSE;

    if (!pidl || ILIsEmpty(pidl))
        return fResult;

    // Test for Network Neighborhood because it will take forever to enum.
    fResult = IsSpecialFolderChild(pidl, CSIDL_NETWORK, FALSE);

    if (!fResult)
    {
        // We only want to do this if we are not the immediate
        // child.
        if (IsSpecialFolderChild(pidl, CSIDL_DRIVES, FALSE))
        {
            TCHAR szActualPath[MAX_URL_STRING];        // IEGetDisplayName() needs the buffer to be this large.
            IEGetNameAndFlags(pidl, SHGDN_FORPARSING, szActualPath, SIZECHARS(szActualPath), NULL);

            DWORD dwOutSize = MAX_URL_STRING;
            if(SUCCEEDED(PathCreateFromUrl(szActualPath, szActualPath, &dwOutSize, 0)))
            {
                PathStripToRoot(szActualPath);
                fResult = PathIsRoot(szActualPath);
            }
#ifdef UNIX
            else
            {
                fResult = (szActualPath[0]==TEXT('/'));
            }
#endif
        }
    }

    return fResult;
}



/****************************************************\
    FUNCTION: IsWebUrl

    PARAMETERS
         none.

    DESCRIPTION:
         Return TRUE if the URL is a Web Url (http,
    ftp, other, ...).  Return FALSE if it's a Shell Url
    or File Url.
\****************************************************/
BOOL CShellUrl::IsWebUrl(void)
{
    if (m_pidl)
    {
        if (!IsURLChild(m_pidl, TRUE))
            return FALSE;
    }
    else
    {
        ASSERT(m_pszURL);   // This CShellUrl hasn't been set.
        if (m_pszURL && IsShellUrl(m_pszURL, TRUE))
            return FALSE;
    }

    return TRUE;
}


/****************************************************\
    FUNCTION: SetCurrentWorkingDir

    PARAMETERS
         pShellUrlNew - Pointer to a CShellUrl that will
                        be the "Current Working Directory"

    DESCRIPTION:
         This Shell Url will have a new current working
    directory, which will be the CShellUrl passed in.

    MEMORY ALLOCATION:
         The caller needs to Allocate pShellUrlNew and
    this object will take care of freeing it.  WARNING:
    this means it cannot be on the stack.
\****************************************************/
HRESULT CShellUrl::SetCurrentWorkingDir(LPCITEMIDLIST pidlCWD)
{
    Pidl_Set(&m_pidlWorkingDir, pidlCWD);

    DEBUG_CODE(TCHAR szDbgBuffer[MAX_PATH];)
    TraceMsg(TF_BAND|TF_GENERAL, "ShellUrl: SetCurrentWorkingDir() pidl=>%s<", Dbg_PidlStr(m_pidlWorkingDir, szDbgBuffer, SIZECHARS(szDbgBuffer)));
    return S_OK;
}


/****************************************************\
    PARAMETERS
         pvPidl1 - First pidl to compare
         pvPidl2 - Second pidl to compare

    DESCRIPTION:
         Return if the pidl matches.  This doesn't work
    for sorted lists (because we can't determine less
    than or greater than).
\****************************************************/
int DPAPidlCompare(LPVOID pvPidl1, LPVOID pvPidl2, LPARAM lParam)
{
    // return < 0 for pvPidl1 before pvPidl2.
    // return == 0 for pvPidl1 equals pvPidl2.
    // return > 0 for pvPidl1 after pvPidl2.
    return (ILIsEqual((LPCITEMIDLIST)pvPidl1, (LPCITEMIDLIST)pvPidl2) ? 0 : 1);
}


/****************************************************\
    PARAMETERS
         pShellUrlNew - Pointer to a CShellUrl that will
                        be added to the "Shell Path"

    DESCRIPTION:
         This Shell Url will have the ShellUrl that's
    passed in added to the "Shell Path", which will be
    searched when trying to qualify the Shell Url during
    parsing.

    MEMORY ALLOCATION:
         The caller needs to Allocate pShellUrlNew and
    this object will take care of freeing it.  WARNING:
    this means it cannot be on the stack.
\****************************************************/
HRESULT CShellUrl::AddPath(LPCITEMIDLIST pidl)
{
    ASSERT(IS_VALID_PIDL(pidl));

    //  we dont want to add any paths that arent derived from
    //  our root.
    if (ILIsRooted(m_pidlWorkingDir) && !ILIsParent(m_pidlWorkingDir, pidl, FALSE))
        return S_FALSE;

    if (!m_hdpaPath)
    {
        m_hdpaPath = DPA_Create(CE_PATHGROW);
        if (!m_hdpaPath)
            return E_OUTOFMEMORY;
    }

    // Does the path already exist in our list?
    if (-1 == DPA_Search(m_hdpaPath, (void *)pidl, 0, DPAPidlCompare, NULL, 0))
    {
        // No, so let's add it.
        LPITEMIDLIST pidlNew = ILClone(pidl);
        if (pidlNew)
            DPA_AppendPtr(m_hdpaPath, pidlNew);
    }

    DEBUG_CODE(TCHAR szDbgBuffer[MAX_PATH];)
    TraceMsg(TF_BAND|TF_GENERAL, "ShellUrl: SetCurrentWorkingDir() pidl=>%s<", Dbg_PidlStr(pidl, szDbgBuffer, SIZECHARS(szDbgBuffer)));
    return S_OK;
}


/****************************************************\
    FUNCTION: Reset

    PARAMETERS:
        none.

    DESCRIPTION:
        This function will "Clean" out the object and
    reset it.  Normally called when the caller is about
    to set new values.
\****************************************************/
HRESULT CShellUrl::Reset(void)
{
    Pidl_Set(&m_pidl, NULL);
    Str_SetPtr(&m_pszURL, NULL);
    Str_SetPtr(&m_pszArgs, NULL);
    Str_SetPtr(&m_pszDisplayName, NULL);
    m_dwGenType = 0;

    return S_OK;
}


/****************************************************\
    FUNCTION: _CanUseAdvParsing

    PARAMETERS:
        none.

    DESCRIPTION:
        This function will return TRUE if Advanced
    Parsing (Shell URLs) should be supported.  This
    function will keep track of whether the user
    has turn off Shell Parsing from the Control Panel.
\****************************************************/
#define REGSTR_USEADVPARSING_PATH  TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Band\\Address")
#define REGSTR_USEADVPARSING_VALUE TEXT("UseShellParsing")

BOOL CShellUrl::_CanUseAdvParsing(void)
{
    // WARNING: Since this is static, changes to the registry entry won't be
    //          read in until the time the process is launched.  This is okay,
    //          because this feature will probably be removed from the released
    //          product and can be added back in as power toy.
    static TRI_STATE fCanUseAdvParsing = TRI_UNKNOWN;

    if (TRI_UNKNOWN == fCanUseAdvParsing)
        fCanUseAdvParsing = (TRI_STATE) SHRegGetBoolUSValue(REGSTR_USEADVPARSING_PATH, REGSTR_USEADVPARSING_VALUE, FALSE, TRUE);

    return fCanUseAdvParsing;
}


/****************************************************\
    FUNCTION: _FixDriveDisplayName

    PARAMETERS:
        pszStart - Pointer to the beginning of the URL string.
        pszCurrent - Pointer into current location in the URL string.
        pidl - PIDL pointing to location of Shell Name space that
               has been parsed so far.

    DESCRIPTION:
        This function exists to check if we are parsing
    a drive letter.  This is necessary because the Display
    Name of drive letters end in '\', which will is needed
    later to determine the start of the next segment.
\****************************************************/
#ifndef UNIX
#define DRIVE_STRENDING     TEXT(":\\")
#define DRIVE_STRSIZE       3 // "C:\"
#else
#define DRIVE_STRSIZE       1 // "/"
#endif

BOOL _FixDriveDisplayName(LPCTSTR pszStart, LPCTSTR pszCurrent, LPCITEMIDLIST pidl)
{
    BOOL fResult = FALSE;

    ASSERT(pszCurrent >= pszStart);

#ifndef UNIX
    // The compiler will take care of adding '/ sizeof(TCHAR)'.
    if (((pszCurrent - pszStart) == DRIVE_STRSIZE) &&
        (0 == StrCmpN(&(pszStart[1]), DRIVE_STRENDING, SIZECHARS(DRIVE_STRENDING)-1)))
#else
    if ((((pszCurrent - pszStart)/SIZEOF(TCHAR)) == DRIVE_STRSIZE))
#endif
    {
        if (IsSpecialFolderChild(pidl, CSIDL_DRIVES, TRUE))
            fResult = TRUE;
    }

    return fResult;
}



/****************************************************\
    FUNCTION: _ParseRelativePidl

    PARAMETERS:
        pcszUrlIn - Pointer to URL to Parse.
        dwFlags - Flags to modify the way the string is parsed.
        pidl - This function will see if pcszUrlIn is a list of display names
               relative to this pidl.
        fAllowRelative - Do we allow relative parsing, which
                         means strings containing "..".
        fQualifyDispName - If TRUE when we known that we need to force the
                           URL to be fully qualified if we bind to the destination.
                           This is needed because we are using state information to
                           find the destination URL and that state information won't
                           be available later.

    DESCRIPTION:
        Start the parsing by getting the pidl of ShellUrlRelative
    and call _ParseNextSegment().  _ParseNextSegment() will
    recursively parse each segment of the PIDL until either
    it fails to fully parse of it finishes.
\****************************************************/
HRESULT CShellUrl::_ParseRelativePidl(LPCTSTR pcszUrlIn,
    BOOL * pfPossibleWebUrl, DWORD dwFlags, LPCITEMIDLIST pidl,
    BOOL fAllowRelative, BOOL fQualifyDispName)
{
    HRESULT hr;
    BOOL fFreePidl = FALSE;

    if (!pcszUrlIn)
        return E_INVALIDARG;

    TraceMsg(TF_BAND|TF_GENERAL, "ShellUrl: _ParseRelativePidl() Begin. pcszUrlIn=%s", pcszUrlIn);

    hr = _ParseNextSegment(pidl, pcszUrlIn, pfPossibleWebUrl, fAllowRelative, fQualifyDispName);

    if (pidl && fFreePidl)
        ILFree((LPITEMIDLIST)pidl);

    DEBUG_CODE(TCHAR szDbgBuffer[MAX_PATH];)
    TraceMsg(TF_BAND|TF_GENERAL, "ShellUrl: _ParseRelativePidl() m_pidl=>%s<", Dbg_PidlStr(m_pidl, szDbgBuffer, SIZECHARS(szDbgBuffer)));
    return hr;
}



/****************************************************\
    FUNCTION: IsShellUrl

    PARAMETERS:
        LPCTSTR szUrl - URL from Outside Source.
        return - Whether the URL is an Internet URL.

    DESCRIPTION:
        This function will determine if the URL is
    a shell URL which includes the following:
    1. File Urls (E;\dir1\dir2)
    2. Shell Urls (shell:desktop)
\****************************************************/
BOOL IsShellUrl(LPCTSTR pcszUrl, BOOL fIncludeFileUrls)
{
    int nSchemeBefore, nSchemeAfter;
    TCHAR szParsedUrl[MAX_URL_STRING];

    nSchemeBefore = GetUrlScheme(pcszUrl);
    IURLQualifyT(pcszUrl, UQF_GUESS_PROTOCOL, szParsedUrl, NULL, NULL);
    nSchemeAfter = GetUrlScheme(szParsedUrl);

    // This is a "shell url" if it is a file: (and fIncludeFileUrls is
    // set), or it is a shell:, or it is an invalid scheme (which
    // occurs for things like "My Computer" and "Control Panel").

    return ((fIncludeFileUrls && URL_SCHEME_FILE == nSchemeAfter) ||
            URL_SCHEME_SHELL == nSchemeAfter ||
            URL_SCHEME_INVALID == nSchemeBefore);
}


/****************************************************\
    FUNCTION: IsSpecialFolderChild

    PARAMETERS:
        pidlToTest (In) - Is this PIDL to test and see if it's
                     a child of SpecialFolder(nFolder).
        psfParent (In Optional)- The psf passed to
                     SHGetSpecialFolderLocation() if needed.
        nFolder (In) - Special Folder Number (CSIDL_INTERNET, CSIDL_DRIVES, ...).
        pdwLevels (In Optional) - Pointer to DWORD to receive levels between
                    pidlToTest and it's parent (nFolder) if S_OK is returned.

    DESCRIPTION:
        This function will see if pidlToTest is a child
    of the Special Folder nFolder.
\****************************************************/
BOOL IsSpecialFolderChild(LPCITEMIDLIST pidlToTest, int nFolder, BOOL fImmediate)
{
    LPITEMIDLIST pidlThePidl = NULL;
    BOOL fResult = FALSE;

    if (!pidlToTest)
        return FALSE;

    ASSERT(IS_VALID_PIDL(pidlToTest));
    if (NOERROR == SHGetSpecialFolderLocation(NULL, nFolder, &pidlThePidl))
    {
        fResult = ILIsParent(pidlThePidl, pidlToTest, fImmediate);
        ILFree(pidlThePidl);
    }
    return fResult;        // Shell Items (My Computer, Control Panel)
}


/****************************************************\
    FUNCTION: GetPidl

    PARAMETERS
         ppidl - Pointer that will receive the current PIDL.

    DESCRIPTION:
         This function will retrieve the pidl that the
    Shell Url is set to.

    MEMORY ALLOCATION:
         This function will allocate the PIDL that ppidl
    points to, and the caller needs to free the PIDL when
    done with it.
\****************************************************/
HRESULT CShellUrl::GetPidl(LPITEMIDLIST * ppidl)
{
    HRESULT hr = S_OK;

    if (ppidl)
        *ppidl = NULL;
    if (!m_pidl)
        hr = _GeneratePidl(m_pszURL, m_dwGenType);

    if (ppidl)
    {
        if (m_pidl)
        {
            *ppidl = ILClone(m_pidl);
            if (!*ppidl)
                hr = E_FAIL;
        }
        else
            hr = E_FAIL;
    }

    // Callers only free *ppidl if SUCCEDED(hr), so assert we act this way.
    ASSERT((*ppidl && SUCCEEDED(hr)) || (!*ppidl && FAILED(hr)));

    DEBUG_CODE(TCHAR szDbgBuffer[MAX_PATH];)
    TraceMsg(TF_BAND|TF_GENERAL, "ShellUrl: GetPidl() *ppidl=>%s<", Dbg_PidlStr(*ppidl, szDbgBuffer, SIZECHARS(szDbgBuffer)));
    return hr;
}

//
// This is a wacky class!  If GetPidl member of this class is called and
// a m_pidl is generated from and url and then Execute() assumes we have
// a valid location in our namespace and  calls code that will not autoscan.
// This hacky function is used to return a pidl only if we have one to
// avoid the above problem.
//
HRESULT CShellUrl::GetPidlNoGenerate(LPITEMIDLIST * ppidl)
{
    HRESULT hr = E_FAIL;
    if (m_pidl && ppidl)
    {
        *ppidl = ILClone(m_pidl);
        if (*ppidl)
        {
            hr = S_OK;
        }
    }

    return hr;
}

/****************************************************\
    FUNCTION: _GeneratePidl

    PARAMETERS
         pcszUrl - This URL will be used to generate the m_pidl.
         dwGenType - This is needed to know how to parse pcszUrl
                     to generate the PIDL.

    DESCRIPTION:
        This CShellUrl maintains a pointer to the object
    in the Shell Name Space by using either the string URL
    or the PIDL.  When this CShellUrl is set to one, we
    delay generating the other one for PERF reasons.
    This function generates the PIDL from the string URL
    when we do need the string.
\****************************************************/

HRESULT CShellUrl::_GeneratePidl(LPCTSTR pcszUrl, DWORD dwGenType)
{
    HRESULT hr;

    if (!pcszUrl && m_pidl)
        return S_OK;      // The caller only wants the PIDL to be created if it doesn't exist.

    if (pcszUrl && m_pidl)
    {
        ILFree(m_pidl);
        m_pidl = NULL;
    }

    switch (dwGenType)
    {
        case GENTYPE_FROMURL:
            if (ILIsRooted(m_pidlWorkingDir))
                hr = E_FAIL;    // MSN Displays error dialogs on IShellFolder::ParseDisplayName()
            // fall through
        case GENTYPE_FROMPATH:
            hr = IECreateFromPath(pcszUrl, &m_pidl);
            // This may fail if it's something like "ftp:/" and not yet valid".
            break;

        default:
            hr = E_INVALIDARG;
            break;
    }

    if (!m_pidl && SUCCEEDED(hr))
        hr = E_FAIL;

    return hr;
}


/****************************************************\
    FUNCTION: SetPidl

    PARAMETERS
         pidl - New pidl to use.

    DESCRIPTION:
         The shell url will now consist of the new pidl
    passed in.

    MEMORY ALLOCATION:
         The caller is responsible for Allocating and Freeing
    the PIDL parameter.
\****************************************************/
HRESULT CShellUrl::SetPidl(LPCITEMIDLIST pidl)
{
    HRESULT hr = S_OK;
    ASSERT(!pidl || IS_VALID_PIDL(pidl));

    Reset();        // External Calls to this will reset the entire CShellUrl.
    return _SetPidl(pidl);
}


/****************************************************\
    FUNCTION: _SetPidl

    PARAMETERS
         pidl - New pidl to use.

    DESCRIPTION:
         This function will reset the m_pidl member
    variable without modifying m_szURL.  This is only used
    internally, and callers that want to reset the entire
    CShellUrl to a PIDL should call the public method
    SetPidl().

    MEMORY ALLOCATION:
         The caller is responsible for Allocating and Freeing
    the PIDL parameter.
\****************************************************/
HRESULT CShellUrl::_SetPidl(LPCITEMIDLIST pidl)
{
    HRESULT hr = S_OK;
    DEBUG_CODE(TCHAR szDbgBuffer[MAX_PATH];)
    TraceMsg(TF_BAND|TF_GENERAL, "ShellUrl: SetPidl() pidl=>%s<", Dbg_PidlStr(pidl, szDbgBuffer, SIZECHARS(szDbgBuffer)));

    Pidl_Set(&m_pidl, pidl);
    if (!m_pidl)
        hr = E_FAIL;

    return hr;
}


/****************************************************\
    FUNCTION: GetUrl

    PARAMETERS
         pszUrlOut (Out Optional) - If the caller wants the string.
         cchUrlOutSize (In) - Size of String Buffer Passed in.

    DESCRIPTION:
         This function will retrieve the string value of
    the shell url.  This will not include the command line
    arguments or other information needed for correct navigation
    (AutoSearch=On/Off, ...).  Note that this may be of the
    form "Shell:/desktop/My Computer/...".
\****************************************************/
HRESULT CShellUrl::GetUrl(LPTSTR pszUrlOut, DWORD cchUrlOutSize)
{
    HRESULT hr = S_OK;

    if (!m_pszURL)
    {
        if (m_pidl)
            hr = _GenerateUrl(m_pidl);
        else
            hr = E_FAIL;  // User never set the CShellUrl.
    }

    if (SUCCEEDED(hr) && pszUrlOut)
        StrCpyN(pszUrlOut, m_pszURL, cchUrlOutSize);

    return hr;
}



HRESULT SHGetDisplayName(IShellFolder *psf, LPCITEMIDLIST pidl, UINT shgdnf, LPTSTR pszName, DWORD cchName)
{
    STRRET str;
    HRESULT hr = psf->GetDisplayNameOf(pidl, shgdnf, &str);
    if (SUCCEEDED(hr))
        hr = StrRetToBuf(&str, pidl, pszName, cchName);

    return hr;
}

/****************************************************\
    FUNCTION: MutantGDNForShellUrl

!!! WARNING - extremely specific to the ShellUrl/AddressBar - ZekeL - 18-NOV-98
!!!           it depends on the bizarre pathology of the ShellUrl in order
!!!           to be reparsed into a pidl later.  cannot be used for anything else

    PARAMETERS:
        pidlIn - Pointer to PIDL to generate Display Names.
        pszUrlOut - String Buffer to store list of Display Names for ITEMIDs
                    in pidlIn.
        cchUrlOutSize - Size of Buffer in characters.

    DESCRIPTION:
        This function will take the PIDL passed in and
    generate a string containing the ILGDN_ITEMONLY Display names
    of each ITEMID in the pidl separated by '\'.
\****************************************************/
#define SZ_SEPARATOR TEXT("/")

HRESULT MutantGDNForShellUrl(LPCITEMIDLIST pidlIn, LPTSTR pszUrlOut, int cchUrlOutSize)
{
    HRESULT hr = S_OK;
    LPCITEMIDLIST pidlCur;
    IShellFolder *psfCur = NULL;

    if (ILIsRooted(pidlIn))
    {
        //  need to start off with our virtual root
        LPITEMIDLIST pidlFirst = ILCloneFirst(pidlIn);
        if (pidlFirst)
        {
            IEBindToObject(pidlFirst, &psfCur);
            ILFree(pidlFirst);
        }

        pidlCur = _ILNext(pidlIn);
    }
    else
    {
        SHGetDesktopFolder(&psfCur);
        pidlCur = pidlIn;
    }

    ASSERT(pidlCur && IS_VALID_PIDL(pidlCur));
    while (psfCur && SUCCEEDED(hr) && !ILIsEmpty(pidlCur) && (cchUrlOutSize > 0))
    {
        LPITEMIDLIST pidlCopy = ILCloneFirst(pidlCur);
        if (pidlCopy)
        {
            StrCpyN(pszUrlOut, SZ_SEPARATOR, cchUrlOutSize);
            cchUrlOutSize -= SIZECHARS(SZ_SEPARATOR);

            TCHAR szCurrDispName[MAX_PATH];
            hr = SHGetDisplayName(psfCur, pidlCopy, SHGDN_NORMAL, szCurrDispName, SIZECHARS(szCurrDispName));

            if (TBOOL((int)cchUrlOutSize > lstrlen(szCurrDispName)))
            {
                StrCatBuff(pszUrlOut, szCurrDispName, cchUrlOutSize);
                cchUrlOutSize -= lstrlen(szCurrDispName);
            }

            if (SUCCEEDED(hr))
            {
                // may fail, in that case we terminate the loop
                IShellFolder *psfCurNew = NULL; // for buggy BindToObject impls
                hr = psfCur->BindToObject(pidlCopy, NULL, IID_IShellFolder, (void **)&psfCurNew);

                psfCur->Release();
                psfCur = psfCurNew;
            }
            pidlCur = _ILNext(pidlCur);
            ILFree(pidlCopy);
        }
        else
            hr = E_FAIL;
    }
    if (psfCur)
        psfCur->Release();

    TraceMsg(TF_BAND|TF_GENERAL, "ShellUrl: MutantGDNForShellUrl() End. pszUrlOut=%s", pszUrlOut);
    return hr;
}


/****************************************************\
    FUNCTION: _GenerateUrl

    PARAMETERS
         pidl - This PIDL will be used to generate the m_pszURL, string URL.

    DESCRIPTION:
        This CShellUrl maintains a pointer to the object
    in the Shell Name Space by using either the string URL
    or the PIDL.  When this CShellUrl is set to one, we
    delay generating the other one for PERF reasons.
    This function generates the string URL from the PIDL
    when we do need the string.
\****************************************************/
#define SZ_THEINTERNET_PARSENAME         TEXT("::{")

HRESULT CShellUrl::_GenerateUrl(LPCITEMIDLIST pidl)
{
    HRESULT hr = S_OK;
    TCHAR szUrl[MAX_URL_STRING];

    ASSERT(IS_VALID_PIDL(pidl));
    if (IsURLChild(pidl, TRUE) || _IsFilePidl(pidl))
    {
        hr = IEGetNameAndFlags(pidl, SHGDN_FORPARSING, szUrl, SIZECHARS(szUrl), NULL);
        if (SUCCEEDED(hr))
        {
            // Was the pidl pointing to "The Internet"?
            if (0 == StrCmpN(szUrl, SZ_THEINTERNET_PARSENAME, (ARRAYSIZE(SZ_THEINTERNET_PARSENAME) - 1)))
            {
                // Yes, so we don't want the SHGDN_FORPARSING name
                // because the user doesn't know what the heck it is.  Since we
                // navigate to the home page, let's display that.
                hr = IEGetNameAndFlags(pidl, SHGDN_NORMAL, szUrl, SIZECHARS(szUrl), NULL);
            }
        }
    }
    else
        hr = MutantGDNForShellUrl(pidl, szUrl, SIZECHARS(szUrl));

    if (SUCCEEDED(hr))
        Str_SetPtr(&m_pszURL, szUrl);

    if (!m_pszURL)
        hr = E_OUTOFMEMORY;

    if (FAILED(hr))
        Str_SetPtr(&m_pszURL, NULL);        // Clear it

    return hr;
}


/****************************************************\
    FUNCTION: SetUrl

    PARAMETERS
         szUrlOut (Out) - Url

    DESCRIPTION:
         Set the ShellUrl from a string that is parsible from
    the root (desktop) ISF.  This is normally used for
    File Paths.
\****************************************************/
HRESULT CShellUrl::SetUrl(LPCTSTR pcszUrlIn, DWORD dwGenType)
{
    Reset();        // External Calls to this will reset the entire CShellUrl.
    return _SetUrl(pcszUrlIn, dwGenType);
}


/****************************************************\
    FUNCTION: _SetUrl

    PARAMETERS
         pcszUrlIn (In) - The string URL for this CShellUrl
         dwGenType (In) - Method to use when generating the PIDL
                          from pcszUrlIn.

    DESCRIPTION:
         This function will reset the m_pszURL member
    variable without modifying m_pidl.  This is only used
    internally, and callers that want to reset the entire
    CShellUrl to an URL should call the public method
    SetUrl().
\****************************************************/
HRESULT CShellUrl::_SetUrl(LPCTSTR pcszUrlIn, DWORD dwGenType)
{
    m_dwGenType = dwGenType;

    return Str_SetPtr(&m_pszURL, pcszUrlIn) ? S_OK : E_OUTOFMEMORY;
}


/****************************************************\
    FUNCTION: GetDisplayName

    PARAMETERS
         pszUrlOut (Out) - Get the Shell Url in String Form.
         cchUrlOutSize (In) - Size of String Buffer Passed in.

    DESCRIPTION:
         This function will Fill in pszUrlOut with nice
    versions of the Shell Url that can be displayed in
    the AddressBar or in the Titles of windows.
\****************************************************/
HRESULT CShellUrl::GetDisplayName(LPTSTR pszUrlOut, DWORD cchUrlOutSize)
{
    HRESULT hr = S_OK;

    if (!m_pszDisplayName)
    {
        if (m_pidl)
        {
            LPITEMIDLIST pidl = NULL;

            hr = GetPidl(&pidl);
            if (SUCCEEDED(hr))
            {
                hr = _GenDispNameFromPidl(pidl, NULL);
                ILFree(pidl);
            }
        }
        else if (m_pszURL)
        {
            // In this case, we will just give back the URL.
            Str_SetPtr(&m_pszDisplayName, m_pszURL);
        }
    }

    if (SUCCEEDED(hr) && pszUrlOut && m_pszDisplayName)
        StrCpyN(pszUrlOut, m_pszDisplayName, cchUrlOutSize);

    return hr;
}


/****************************************************\
    FUNCTION: _GenDispNameFromPidl

    PARAMETERS
         pidl (In) - This will be used to generate the Display Name.
         pcszArgs (In) - These will be added to the end of the Display Name

    DESCRIPTION:
        This function will generate the Display Name
    from the pidl and pcszArgs parameters.  This is
    normally not needed when this CShellUrl was parsed
    from an outside source, because the Display Name
    was generated at that time.
\****************************************************/
HRESULT CShellUrl::_GenDispNameFromPidl(LPCITEMIDLIST pidl, LPCTSTR pcszArgs)
{
    HRESULT hr;
    TCHAR szDispName[MAX_URL_STRING];

    hr = GetUrl(szDispName, SIZECHARS(szDispName));
    if (SUCCEEDED(hr))
    {
        if (pcszArgs)
            StrCatBuff(szDispName, pcszArgs, ARRAYSIZE(szDispName));
        PathMakePretty(szDispName);

        hr = Str_SetPtr(&m_pszDisplayName, szDispName) ? S_OK : E_OUTOFMEMORY;
    }

    return hr;
}


/****************************************************\
    FUNCTION: GetArgs

    PARAMETERS
         pszArgsOut - The arguments to the Shell Url. (Only
                     for ShellExec().
         cchArgsOutSize - Size of pszArgsOut in chars.

    DESCRIPTION:
         Get the arguments that will be passed to
    ShellExec() if 1) the Pidl is navigated to, 2) it's
    a File URL, and 3) it's not navigatable.
\****************************************************/
HRESULT CShellUrl::GetArgs(LPTSTR pszArgsOut, DWORD cchArgsOutSize)
{
    ASSERT(pszArgsOut);

    if (m_pszArgs)
        StrCpyN(pszArgsOut, m_pszArgs, cchArgsOutSize);
    else
        *pszArgsOut = 0;

    TraceMsg(TF_BAND|TF_GENERAL, "ShellUrl: GetArgs() pszArgsOut=%s", pszArgsOut);
    return S_OK;
}


/****************************************************\
    FUNCTION: SetDefaultShellPath

    PARAMETERS
         psu - CShellUrl to set path.

    DESCRIPTION:
         "Desktop";"Desktop/My Computer" is the
    most frequently used Shell Path for parsing.  This
    function will add those two items to the CShellUrl
    passed in the paramter.
\****************************************************/
HRESULT SetDefaultShellPath(CShellUrl * psu)
{
    ASSERT(psu);
    LPITEMIDLIST pidl;

    // We need to set the "Shell Path" which will allow
    // the user to enter Display Names of items in Shell
    // Folders that are frequently used.  We add "Desktop"
    // and "Desktop/My Computer" to the Shell Path because
    // that is what users use most often.

    // _pshuUrl will free pshuPath, so we can't.
    psu->AddPath(&s_idlNULL);

    SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &pidl);  // Get Pidl for "My Computer"
    if (pidl)
    {
        // psu will free pshuPath, so we can't.
        psu->AddPath(pidl);
        ILFree(pidl);
    }

    // Add favorites folder too
    SHGetSpecialFolderLocation(NULL, CSIDL_FAVORITES, &pidl);
    if (pidl)
    {
        // psu will free pshuPath, so we can't.
        psu->AddPath(pidl);
        ILFree(pidl);
    }

    return S_OK;
}

void CShellUrl::SetMessageBoxParent(HWND hwnd)
{
    // Find the topmost window so that the messagebox disables
    // the entire frame
    HWND hwndTopmost = NULL;
    while (hwnd)
    {
        hwndTopmost = hwnd;
        hwnd = GetParent(hwnd);
    }

    m_hwnd = hwndTopmost;
};
