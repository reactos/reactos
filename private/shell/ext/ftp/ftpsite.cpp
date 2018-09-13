/*****************************************************************************\
 *    ftpsite.cpp - Internal object that manages a single FTP site
\*****************************************************************************/

#include "priv.h"
#include "ftpsite.h"
#include "ftpinet.h"
#include "ftpurl.h"
#include "statusbr.h"
#include "offline.h"
#include <ratings.h>
#include <wininet.h>
#include <dbgmem.h>

#ifdef DEBUG
DWORD g_dwOpenConnections = 0;      // Ref Counting Open Connections
#endif // DEBUG

/*****************************************************************************\
 *    CFtpSite
 *
 *    EEK!  RFC 1738 is really scary.  FTP sites don't necessarily
 *    start you at the root, and RFC1738 says that ftp://foo/bar asks
 *    for the file bar in the DEFAULT directory, not the root!
\*****************************************************************************/
CFtpList * g_FtpSiteCache = NULL;                /* The list of all open FTP sites */


void CFtpSite::FlushHint(void)
{
    HINTERNET hint = m_hint;

    m_hint = NULL;
    if (hint)
    {
        // Our caller needs to be holding the critical section
        // while we modify m_hint
        ASSERTCRITICAL;

        InternetCloseHandle(hint);
//        DEBUG_CODE(g_dwOpenConnections--;);
    }
}


void CFtpSite::FlushHintCritial(void)
{
    ASSERTNONCRITICAL;

    ENTERCRITICAL;
    FlushHint();
    LEAVECRITICAL;
}


void CFtpSite::FlushHintCB(LPVOID pvFtpSite)
{
    CFtpSite * pfs = (CFtpSite *) pvFtpSite;

    if (pfs)
    {
        pfs->FlushHint();
        pfs->Release();
    }
}


/*****************************************************************************\
 *    An InternetConnect has just completed.  Get the motd and cache it.
 *
 *    hint - the connected handle, possibly 0 if error
\*****************************************************************************/
void CFtpSite::CollectMotd(HINTERNET hint)
{
    CFtpGlob * pfg = GetFtpResponse(&m_cwe);
    remove_from_memlist(pfg);   // We will probably free this on a separate thread.

    ENTERCRITICAL;
    m_fMotd = m_pfgMotd ? TRUE : FALSE;            // We have a motd

    IUnknown_Set(&m_pfgMotd, NULL);
    m_pfgMotd = pfg;

    LEAVECRITICAL;
}


/*****************************************************************************\
    FUNCTION: ReleaseHint

    DESCRIPTION:
        An FtpDir client is finished with a handle to the FTP site.
    Put it into the cache, and throw away what used to be there.

    We always keep the most recent handle, because that reduces the
    likelihood that the server will close the connection due to extended
    inactivity.

    The critical section around this entire procedure is important,
    else we open up all sorts of really ugly race conditions.  E.g.,
    the timeout might trigger before we're finished initializing it.
    Or somebody might ask for the handle before we're ready.
\*****************************************************************************/
void CFtpSite::ReleaseHint(LPCITEMIDLIST pidlFtpPath, HINTERNET hint)
{
    ENTERCRITICAL;

    TriggerDelayedAction(&m_hgti);    // Kick out the old one

    _SetPidl(pidlFtpPath);
    m_hint = hint;

    if (EVAL(SUCCEEDED(SetDelayedAction(FlushHintCB, (LPVOID) this, &m_hgti))))
        AddRef();   // We just gave away a ref.
    else
        FlushHint();    // Oh well, can't cache it

    LEAVECRITICAL;
}


// NT #362108: We need to set the redirect password for the CFtpSite that
// contains the server, the user name, but a blank password to be redirected
// to the CFtpSite that does have the correct password.  This way, if a user
// logs in and doesn't save the password in the URL or the secure cache, we
// then put it in the in memory password cache so it stays valid for that
// "browser" session (defined by process lifetime).  We then need to redirect
// future navigations that go to that 
HRESULT CFtpSite::_SetRedirPassword(LPCTSTR pszServer, INTERNET_PORT ipPortNum, LPCTSTR pszUser, LPCTSTR pszPassword, LPCITEMIDLIST pidlFtpPath, LPCTSTR pszFragment)
{
    TCHAR szUrl[MAX_URL_STRING];
    HRESULT hr;

    hr = UrlCreate(pszServer, pszUser, TEXT(""), TEXT(""), pszFragment, ipPortNum, NULL, szUrl, ARRAYSIZE(szUrl));
    if (EVAL(SUCCEEDED(hr)))
    {
        LPITEMIDLIST pidlServer;

        hr = CreateFtpPidlFromUrl(szUrl, GetCWireEncoding(), NULL, &pidlServer, m_pm, TRUE);
        if (EVAL(SUCCEEDED(hr)))
        {
            LPITEMIDLIST pidl = ILCombine(pidlServer, pidlFtpPath);

            if (pidl)
            {
                CFtpSite * pfsDest = NULL;

                // The user name has changed so we need to update the
                // CFtpSite with the new user name also.
                hr = SiteCache_PidlLookup(pidl, FALSE, m_pm, &pfsDest);
                if (EVAL(SUCCEEDED(hr)))
                {
                    pfsDest->SetRedirPassword(pszPassword);
                    pfsDest->Release();
                }

                ILFree(pidl);
            }

            ILFree(pidlServer);
        }
    }

    return hr;
}

HRESULT CFtpSite::_RedirectAndUpdate(LPCTSTR pszServer, INTERNET_PORT ipPortNum, LPCTSTR pszUser, LPCTSTR pszPassword, LPCITEMIDLIST pidlFtpPath, LPCTSTR pszFragment, IUnknown * punkSite, CFtpFolder * pff)
{
    TCHAR szUrl[MAX_URL_STRING];
    TCHAR szUser[INTERNET_MAX_USER_NAME_LENGTH];
    HRESULT hr;

    StrCpyN(szUser, pszUser, ARRAYSIZE(szUser));    // Copy because of possible reentrancy
    EscapeString(NULL, szUser, ARRAYSIZE(szUser));
    hr = UrlCreate(pszServer, szUser, pszPassword, TEXT(""), pszFragment, ipPortNum, NULL, szUrl, ARRAYSIZE(szUrl));
    if (EVAL(SUCCEEDED(hr) && pff))
    {
        LPITEMIDLIST pidlServer;

        hr = CreateFtpPidlFromUrl(szUrl, GetCWireEncoding(), NULL, &pidlServer, m_pm, TRUE);
        if (EVAL(SUCCEEDED(hr)))
        {
            LPITEMIDLIST pidl = ILCombine(pidlServer, pidlFtpPath);

            if (pidl)
            {
                // If the user changed the password, we need to setup a redirect so
                // they can return later. (NT #362108)
                if (m_pszUser && !StrCmp(m_pszUser, szUser) && StrCmp(m_pszPassword, pszPassword))
                {
                    _SetRedirPassword(pszServer, ipPortNum, szUser, pszPassword, pidlFtpPath, pszFragment);
                }

                // If the user name changed, set a redirect.
                if (!m_pszUser || StrCmp(m_pszUser, szUser))
                {
                    CFtpSite * pfsDest = NULL;

                    // The user name has changed so we need to update the
                    // CFtpSite with the new user name also.
                    hr = SiteCache_PidlLookup(pidl, FALSE, m_pm, &pfsDest);
                    if (EVAL(SUCCEEDED(hr)))
                    {
                        pfsDest->SetRedirPassword(pszPassword);
                        pfsDest->Release();
                    }
                }

                hr = _Redirect(pidl, punkSite, pff);
                ILFree(pidl);
            }

            ILFree(pidlServer);
        }
    }

    return hr;
}


HRESULT CFtpSite::_Redirect(LPITEMIDLIST pidl, IUnknown * punkSite, CFtpFolder * pff)
{
    LPITEMIDLIST pidlFull = pff->CreateFullPublicPidl(pidl);
    HRESULT hr = E_INVALIDARG;

    if (EVAL(pidlFull))
    {
        hr = IUnknown_PidlNavigate(punkSite, pidlFull, FALSE);

        ASSERT(SUCCEEDED(hr));
        ILFree(pidlFull);
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: _SetDirectory

    DESCRIPTION:
        When the caller wants a handle to the server, they often want a different
    directory than what's in the cache.  This function needs to change into
    the new directory.
\*****************************************************************************/
HRESULT CFtpSite::_SetDirectory(HINTERNET hint, HWND hwnd, LPCITEMIDLIST pidlNewDir, CStatusBar * psb, int * pnTriesLeft)
{
    HRESULT hr = S_OK;

    if (pidlNewDir && FtpID_IsServerItemID(pidlNewDir))
        pidlNewDir = _ILNext(pidlNewDir);   // Skip the server.

    ASSERT(m_pidl);
    // NT #300889: I would like to cache the dir but sometimes it gets
    //             out of wack and m_pidl doesn't match the HINTERNET's
    //             cwd.  PERF: This could be fixed in the future but
    //             this perf tweak isn't work the work now (small gain).
//  if (m_pidl && !FtpPidl_IsPathEqual(_ILNext(m_pidl), pidlNewDir))
    {
        LPITEMIDLIST pidlWithVirtualRoot;

        if (psb)
        {
            WCHAR wzDisplayPath[MAX_PATH];  // For Statusbar.
            
            if (pidlNewDir && SUCCEEDED(GetDisplayPathFromPidl(pidlNewDir, wzDisplayPath, ARRAYSIZE(wzDisplayPath), TRUE)))
                psb->SetStatusMessage(IDS_CHDIR, wzDisplayPath);
            else
                psb->SetStatusMessage(IDS_CHDIR, L"\\");
        }

        hr = PidlInsertVirtualRoot(pidlNewDir, &pidlWithVirtualRoot);
        if (EVAL(SUCCEEDED(hr)))
        {
            hr = FtpSetCurrentDirectoryPidlWrap(hint, TRUE, pidlWithVirtualRoot, TRUE, TRUE);
            if (SUCCEEDED(hr))  // Ok if failed. (No Access?)
            {
                hr = _SetPidl(pidlNewDir);
            }
            else
            {

                ReleaseHint(NULL, hint); // Nowhere
                if (hr == HRESULT_FROM_WIN32(ERROR_FTP_DROPPED))
                    FlushHintCritial(); // Don't cache dead hint
                else
                {
                    DisplayWininetError(hwnd, TRUE, HRESULT_CODE(hr), IDS_FTPERR_TITLE_ERROR, IDS_FTPERR_CHANGEDIR, IDS_FTPERR_WININET, MB_OK, NULL);
                    *pnTriesLeft = 0;   // Make sure we don't keep display UI.
                    hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                }

                hint = 0;
            }

            ILFree(pidlWithVirtualRoot);
        }

        if (psb)
            psb->SetStatusMessage(IDS_EMPTY, 0);
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: _LoginToTheServer

    DESCRIPTION:
        We want an HINTERNET to do some FTP operation but we don't have one
    cached.  So, login to create it.

    WARNING: This function will be called in a critical section and needs to 
             return in one.  However, it may leave the critical section for a
             while.
\*****************************************************************************/
HRESULT CFtpSite::_LoginToTheServer(HWND hwnd, HINTERNET hintDll, HINTERNET * phint, LPCITEMIDLIST pidlFtpPath, CStatusBar * psb, IUnknown * punkSite, CFtpFolder * pff)
{
    HRESULT hr = S_OK;

    ASSERTCRITICAL;
    BOOL fKeepTryingToLogin = FALSE;
    BOOL fTryOldPassword = TRUE;

    LEAVECRITICALNOASSERT;
    TCHAR szUser[INTERNET_MAX_USER_NAME_LENGTH];
    TCHAR szPassword[INTERNET_MAX_PASSWORD_LENGTH];

    StrCpyN(szUser, m_pszUser, ARRAYSIZE(szUser));
    StrCpyN(szPassword, m_pszPassword, ARRAYSIZE(szPassword));

    ASSERT(m_pszServer);
    if (psb)
        psb->SetStatusMessage(IDS_CONNECTING, m_pszServer);

    do
    {
        hr = InternetConnectWrap(hintDll, TRUE, HANDLE_NULLSTR(m_pszServer), m_ipPortNum, NULL_FOR_EMPTYSTR(szUser), NULL_FOR_EMPTYSTR(szPassword), INTERNET_SERVICE_FTP, 0, 0, phint);
        if (*phint)
            fKeepTryingToLogin = FALSE; // Move up.
        else
        {
            BOOL fSkipLoginDialog = FALSE;

            // Display Login dialog to get new user name/password to try again or cancel login.
            // fKeepTryingToLogin = TRUE if Dialog said [LOGIN].
            if (((ERROR_INTERNET_LOGIN_FAILURE == HRESULT_CODE(hr)) ||
                (ERROR_INTERNET_INCORRECT_USER_NAME == HRESULT_CODE(hr)) ||
                (ERROR_INTERNET_INCORRECT_PASSWORD == HRESULT_CODE(hr))) && hwnd)
            {
                BOOL fIsAnonymous = (!szUser[0] || !StrCmpI(szUser, TEXT("anonymous")) ? TRUE : FALSE);
                DWORD dwLoginFlags = (fIsAnonymous ? LOGINFLAGS_ANON_LOGINJUSTFAILED : LOGINFLAGS_USER_LOGINJUSTFAILED);

                if (fTryOldPassword)
                {
                    hr = m_cAccount.GetUserName(HANDLE_NULLSTR(m_pszServer), szUser, ARRAYSIZE(szUser));
                    if (S_OK == hr)
                    {
                        hr = m_cAccount.GetPassword(HANDLE_NULLSTR(m_pszServer), szUser, szPassword, ARRAYSIZE(szPassword));
                        if (S_OK == hr)
                        {
                            fKeepTryingToLogin = TRUE;
                            fSkipLoginDialog = TRUE;
                        }
                    }
                }
            
                if (!fSkipLoginDialog)
                {
                    // If the user tried to log in anonymously and failed, we want to try
                    // logging in with a password.  If the user tried logging in with a password
                    // and failed, we want to keep trying to log in with a password.
                    // 
                    // DisplayLoginDialog returns S_OK for OK pressed, S_FALSE for Cancel button, and
                    //       FAILED() for something is really messed up.
                    hr = m_cAccount.DisplayLoginDialog(hwnd, dwLoginFlags, HANDLE_NULLSTR(m_pszServer),
                                szUser, ARRAYSIZE(szUser), szPassword, ARRAYSIZE(szPassword));
                }

                // S_FALSE means the user cancelled out of the Login dialog.
                // We need to turn this into an error value so the caller,
                // CFtpDir::WithHint() won't call the callback.
                if (S_FALSE == hr)
                    hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);

                fKeepTryingToLogin = (SUCCEEDED(hr) ? TRUE : FALSE);
                if (fKeepTryingToLogin)
                {
                    // We need to set the cancelled error so we don't display the
                    // error message after this.
                    hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                }

                fTryOldPassword = FALSE;
            }
            else
                fKeepTryingToLogin = FALSE;
        }
    }
    while (fKeepTryingToLogin);

    if (!*phint)
    {
        ASSERT(2 != HRESULT_CODE(hr));        // error 2 = wininet not configured


#ifdef DEBUG
        // Gee, I wonder why I couldn't connect, let's find out.
        TCHAR szBuff[1500];
        InternetGetLastResponseInfoDisplayWrap(FALSE, NULL, szBuff, ARRAYSIZE(szBuff));
        // This may happen if the server has too many connections.  We may want to sniff
        // for this and offer to keep trying.  These are the response from the various
        // FTP Servers in this case:
        // IIS v5: 421 Too many people are connected.  Please come back when the server is less busy.
        // UNIX: ???
#endif // DEBUG
    }
    // Was a different login name or password needed in order to login successfully?
    else
    {
        LPITEMIDLIST pidlVirtualDir;

        CollectMotd(*phint);
        _QueryServerFeatures(*phint);
        // Ref Count the open connections.
//                  DEBUG_CODE(g_dwOpenConnections++;);

        // Is it a VMS Server?
        if (m_fIsServerVMS)
        {
            // Yes, so skip getting pidlVirtualDir because wininet gives us
            // garbage for FtpGetCurrentDirectoryA().
        }
        else
        {
            // NOTE: If the connection isn't annonymous, the server may put the user
            //   into a sub directory called a virtual root.  We need to squirel that
            //   directory away because it may be needed when going into sub directories
            //   relative to this virtual root.
            //     Example: ftp://user1:password@server/ puts you into /users/user1/
            //     Then: ftp://user1:password@server/dir1 really should be /users/user1/dir1/
            hr = FtpGetCurrentDirectoryPidlWrap(*phint, TRUE, GetCWireEncoding(), &pidlVirtualDir);
            if (SUCCEEDED(hr))
            {
                // Are we rooted at '/'? (Meaning no virtual root)
                Pidl_Set(&m_pidlVirtualDir, pidlVirtualDir);
                ILFree(pidlVirtualDir);
            }
        }

        //DEBUG_CODE(TraceMsg(TF_WININET_DEBUG, "CFtpSite::GetHint() FtpGetCurrentDirectory() returned %#08lx", hr));
        if (StrCmp(HANDLE_NULLSTR(m_pszUser), szUser) || StrCmp(HANDLE_NULLSTR(m_pszPassword), szPassword))
        {
            // Yes, so redirect so the AddressBand and User Status Bar pane update.
            // We normally log in with m_pidl because normally we login with
            // a default directory ('\') and then change directories to the final location.
            // we do this so isolate access denied to the server and access denied to the
            // directory.
            //
            // We pass pidlFtpPath instead in this case because it will tell the browser
            // to re-direct and we won't get a chance to do the ChangeDir later.

            Str_SetPtr(&m_pszRedirPassword, szPassword);

            _RedirectAndUpdate(m_pszServer, m_ipPortNum, szUser, szPassword, pidlFtpPath, m_pszFragment, punkSite, pff);
            hr = HRESULT_FROM_WIN32(ERROR_NETWORK_ACCESS_DENIED);
        }
    }

    // Can we assume annonymous logins don't use virtual roots?
    ASSERT(FAILED(hr) || (m_pidlVirtualDir && szUser[0]) || !(m_pidlVirtualDir && szUser[0]));

    if (psb)
        psb->SetStatusMessage(IDS_EMPTY, NULL);
    ENTERCRITICALNOASSERT;

    // The directory is empty.
    _SetPidl(NULL);

    return hr;
}


/*****************************************************************************\
    FUNCTION: GetHint

    DESCRIPTION:
        An IShellFolder client wants a handle to the FTP site.
    Pull it from the cache if possible.

    The caller should have marked the IShellFolder as busy.

    EEK!  RFC 1738 is really scary.  FTP sites don't necessarily
    start you at the root, and RFC1738 says that ftp://foo/bar asks
    for the file bar in the DEFAULT directory, not the root!
\*****************************************************************************/
HRESULT CFtpSite::GetHint(HWND hwnd, LPCITEMIDLIST pidlFtpPath, CStatusBar * psb, HINTERNET * phint, IUnknown * punkSite, CFtpFolder * pff)
{
    HINTERNET hint = NULL;
    HINTERNET hintDll = GetWininetSessionHandle();
    HRESULT hr = S_OK;

    if (!hintDll)
    {
        // No point in retrying if we can't init Wininet
        hr = HRESULT_FROM_WIN32(GetLastError());    // Save error code
    }
    else
    {
        int cTriesLeft = 1; // This is a feature that would be cool to implement.
        hr = AssureNetConnection(NULL, hwnd, m_pszServer, NULL, TRUE);

        if (ILIsEmpty(pidlFtpPath))
            pidlFtpPath = NULL;

        if (SUCCEEDED(hr))
        {
            // BUGBUG -- I don't remember exactly what the CS is protecting
            ASSERTNONCRITICAL;
            ENTERCRITICALNOASSERT;

            do
            {
                BOOL fReuseExistingConnection = FALSE;
                hr = E_FAIL;    // We don't have our hint yet...

                ASSERTCRITICAL;
                hint = (HINTERNET) InterlockedExchangePointer(&m_hint, 0);
                if (hint)
                {
                    HINTERNET hintResponse;

                    TriggerDelayedAction(&m_hgti);      // Nothing will happen
                    fReuseExistingConnection = TRUE;    // We will need to change it for the current user.

                    // We want (S_OK == hr) if our login session is still good.  Else, we want to
                    // re-login.
                    hr = FtpCommandWrap(hint, FALSE, FALSE, FTP_TRANSFER_TYPE_ASCII, FTP_CMD_NO_OP, NULL, &hintResponse);
                    if (SUCCEEDED(hr))
                    {
                        TraceMsg(TF_FTPOPERATION, "CFtpSite::GetHint() We are going to use a cached HINTERNET.");
                        InternetCloseHandleWrap(hintResponse, TRUE);
                    }
                    else
                    {
                        TraceMsg(TF_FTPOPERATION, "CFtpSite::GetHint() Can't used cached HINTERNET because server didn't respond to NOOP.");
                        InternetCloseHandleWrap(hint, TRUE);
                    }
                }
                
                if (FAILED(hr))
                {
                    hr = _LoginToTheServer(hwnd, hintDll, &hint, pidlFtpPath, psb, punkSite, pff);
                    TraceMsg(TF_FTPOPERATION, "CFtpSite::GetHint() We had to login because we didn't have a cached HINTERNET.");
                }

                ASSERTCRITICAL;
                // BUGBUG -- is it safe to do this outside the crst?
                LEAVECRITICALNOASSERT;

                // Do we need to CD into a specific directory?  Yes, if...
                // 1. We succeeded above, AND
                // 2. We are already using a connection so the dir may be incorrect, OR
                // 3. We need a non-default dir.
                if (SUCCEEDED(hr) && (fReuseExistingConnection || pidlFtpPath))   // pidlFtpPath may be NULL.
                    hr = _SetDirectory(hint, hwnd, pidlFtpPath, psb, &cTriesLeft);

                ENTERCRITICALNOASSERT;
                ASSERTCRITICAL;
            }
            while (hr == HRESULT_FROM_WIN32(ERROR_FTP_DROPPED) && --cTriesLeft);

            LEAVECRITICALNOASSERT;
        }
    }

    *phint = hint;
    return hr;
}


HRESULT CFtpSite::_CheckToEnableCHMOD(LPCWIRESTR pwResponse)
{
    HRESULT hr = S_FALSE;
    // TODO: We should probably be more restictive in how we parse the
    //       response.  We should probably verify there is some kind of
    //       white space before and after the command.
    LPCWIRESTR pwCommand = StrStrIA(pwResponse, FTP_UNIXCMD_CHMODA);

    // Does this FTP server support the "SITE CHMOD" command?
    if (pwCommand)
    {
        // Yes, so we may want to use it later.
        m_fIsCHMODSupported = TRUE;

        // We can later respond with:
        // "SITE chmod xyz FileName.txt"
        // x is for Owner, (4=Read, 2=Write, 1=Execute)
        // y is for Owner, (4=Read, 2=Write, 1=Execute)
        // z is for Owner, (4=Read, 2=Write, 1=Execute)
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: _QueryServerFeatures

    DESCRIPTION:
        Find out what the server is and isn't capable of.  Information we could
    use:
        SITE: Find out OS specific commands that may be useful.  "chmod" is one
              of them.
        HELP SITE: Find out what the OS supports.
        SYST: Find out the OS type.
        NOOP: See if the connection is still alive.
        MLST: Unambiguous Directory listing with dates in UTC.
        MLSD: 
        FEAT: Features supported. UTF8 is the one we care about. 

    Response to "SITE HELP" for these servers:
        UNIX Type: L8 Version: BSD-199506
        UNIX Type: L8
           UMASK   CHMOD   GROUP   NEWER   INDEX   ALIAS   GROUPS 
           IDLE    HELP    GPASS   MINFO   EXEC    CDPATH 

        Windows_NT version 4.0
           CKM DIRSTYLE HELP STATS    
\*****************************************************************************/
HRESULT CFtpSite::_QueryServerFeatures(HINTERNET hint)
{
    HRESULT hr = E_FAIL;
    HINTERNET hintResponse;

    // Can we turn on 'UTF8' encoding?
    if (SUCCEEDED(FtpCommandWrap(hint, FALSE, FALSE, FTP_TRANSFER_TYPE_ASCII, FTP_CMD_UTF8, NULL, &hintResponse)))
    {
        m_fInUTF8Mode = TRUE;
        m_cwe.SetUTF8Support(TRUE);
        TraceMsg(TF_FTP_OTHER, "_QueryServerFeatures() in UTF8 Mode");

        InternetCloseHandleWrap(hintResponse, TRUE);
    }
    else
    {
        TraceMsg(TF_FTP_OTHER, "_QueryServerFeatures() NOT in UTF8 Mode");
        m_fInUTF8Mode = FALSE;
    }

    if (!m_fFeaturesQueried)
    {
        // Is type of server software is running?  We want to know if we are running
        // on VMS, because in that case we want to fall back to HTML view (URLMON).
        // This is because the wininet guys don't want to support it.
        if (SUCCEEDED(FtpCommandWrap(hint, FALSE, FALSE, FTP_TRANSFER_TYPE_ASCII, FTP_CMD_SYSTEM, NULL, &hintResponse)))
        {
            DWORD dwError;
            WIRECHAR wResponse[MAX_URL_STRING];
            DWORD cchSize = ARRAYSIZE(wResponse);

            if (SUCCEEDED(InternetGetLastResponseInfoWrap(TRUE, &dwError, wResponse, &cchSize)))
            {
                // Is this a VMS server?
                if (StrStrIA(wResponse, FTP_SYST_VMS))
                    m_fIsServerVMS = TRUE;

                TraceMsg(TF_FTP_OTHER, "_QueryServerFeatures() SYSTM returned %hs.", wResponse);
            }

            InternetCloseHandleWrap(hintResponse, TRUE);
        }


#ifdef FEATURE_CHANGE_PERMISSIONS
        // Is the server capable of supporting the UNIX "chmod" command
        // to change permissions on the file?
        if (SUCCEEDED(FtpCommandWrap(hint, FALSE, FALSE, FTP_TRANSFER_TYPE_ASCII, FTP_CMD_SITE_HELP, NULL, &hintResponse)))
        {
            DWORD dwError;
            WIRECHAR wResponse[MAX_URL_STRING];
            DWORD cchSize = ARRAYSIZE(wResponse);

            if (SUCCEEDED(InternetGetLastResponseInfoWrap(TRUE, &dwError, wResponse, &cchSize)))
            {
                _CheckToEnableCHMOD(wResponse);
//                TraceMsg(TF_FTP_OTHER, "_QueryServerFeatures() SITE HELP returned success");
            }

            InternetCloseHandleWrap(hintResponse, TRUE);
        }
#endif // FEATURE_CHANGE_PERMISSIONS

/*
        // Is the server capable of supporting the UNIX "chmod" command
        // to change permissions on the file?
        if (SUCCEEDED(FtpCommandWrap(hint, FALSE, FALSE, FTP_TRANSFER_TYPE_ASCII, FTP_CMD_SITE, NULL, &hintResponse)))
        {
            DWORD dwError;
            WIRECHAR wResponse[MAX_URL_STRING];
            DWORD cchSize = ARRAYSIZE(wResponse);

            if (SUCCEEDED(InternetGetLastResponseInfoWrap(TRUE, &dwError, wResponse, &cchSize)))
            {
                TraceMsg(TF_FTP_OTHER, "_QueryServerFeatures() SITE returned succeess");
            }

            InternetCloseHandleWrap(hintResponse, TRUE);
        }
*/
    
        m_fFeaturesQueried = TRUE;
    }

    return S_OK;    // This shouldn't fail.
}


LPITEMIDLIST CFtpSite::GetPidl(void)
{
    return ILClone(m_pidl);
}


/*****************************************************************************\
    FUNCTION: _SetPidl

    DESCRIPTION:
        m_pidl contains the ServerID and the ItemIDs making up the path of where
    m_hint is currently located.  This function will take a new path in pidlFtpPath
    and update m_pidl so it still has the server.
\*****************************************************************************/
HRESULT CFtpSite::_SetPidl(LPCITEMIDLIST pidlFtpPath)
{
    HRESULT hr = E_FAIL;
    LPITEMIDLIST pidlServer = FtpCloneServerID(m_pidl);

    if (pidlServer)
    {
        LPITEMIDLIST pidlNew = ILCombine(pidlServer, pidlFtpPath);

        if (pidlNew)
        {
            ILFree(m_pidl);
            m_pidl = pidlNew;

            hr = S_OK;
        }

        ILFree(pidlServer);
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: QueryMotd

    DESCRIPTION:
        Determine whether there is a motd at all.
\*****************************************************************************/
BOOL CFtpSite::QueryMotd(void)
{
    return m_fMotd;
}


HRESULT CFtpSite::GetVirtualRoot(LPITEMIDLIST * ppidl)
{
    HRESULT hr = S_FALSE;
    *ppidl = NULL;

    if (m_pidlVirtualDir)
    {
        *ppidl = ILClone(m_pidlVirtualDir);
        hr = S_OK;
    }

    return S_OK;
}


HRESULT CFtpSite::PidlInsertVirtualRoot(LPCITEMIDLIST pidlFtpPath, LPITEMIDLIST * ppidl)
{
    HRESULT hr = S_OK;

    if (!m_pidlVirtualDir)
        *ppidl = ILClone(pidlFtpPath);
    else
    {
        LPITEMIDLIST pidlTemp = NULL;

        if (pidlFtpPath && FtpID_IsServerItemID(pidlFtpPath))
        {
            pidlTemp = FtpCloneServerID(pidlFtpPath);
            pidlFtpPath = _ILNext(pidlFtpPath);
        }

        LPITEMIDLIST pidlWithVRoot = ILCombine(pidlTemp, m_pidlVirtualDir);
        if (pidlWithVRoot)
        {
            *ppidl = ILCombine(pidlWithVRoot, pidlFtpPath);
            ILFree(pidlWithVRoot);
        }
        
        ILFree(pidlTemp);
    }

    return S_OK;
}


BOOL CFtpSite::HasVirtualRoot(void)
{
    return (m_pidlVirtualDir ? TRUE : FALSE);
}


/*****************************************************************************\
      GetMotd
  
      Returns the HFGLOB that babysits the motd.  The refcount has been
      incremented.
\*****************************************************************************/
CFtpGlob * CFtpSite::GetMotd(void)
{
    if (m_pfgMotd)
        m_pfgMotd->AddRef();

    return m_pfgMotd;
}


/*****************************************************************************\
      GetCFtpList
  
      Return the CFtpList * that remembers which folders live in this CFtpSite *.
  
      WARNING!  The caller must own the critical section when calling
      this routine, because the returned CFtpList * is not refcounted!
\*****************************************************************************/
CFtpList * CFtpSite::GetCFtpList(void)
{
    return m_FtpDirList;
}


/*****************************************************************************\
      _CompareSites
  
      Callback during SiteCache_PrivSearch to see if the site is already in the
      list.
\*****************************************************************************/
int CALLBACK _CompareSites(LPVOID pvStrSite, LPVOID pvFtpSite, LPARAM lParam)
{
    CFtpSite * pfs = (CFtpSite *) pvFtpSite;
    LPCTSTR pszLookupStrNew = (LPCTSTR) pvStrSite;
    LPCTSTR pszLookupStr = (pfs->m_pszLookupStr ? pfs->m_pszLookupStr : TEXT(""));

    ASSERT(pszLookupStr && pszLookupStr);
    return StrCmpI(pszLookupStr, pszLookupStrNew);
}


/*****************************************************************************\
    FUNCTION: SiteCache_PrivSearch

    DESCRIPTION:
        We cache information about an FTP Server to prevent hitting the net all
    the time.  This state is stored in CFtpSite objects and we use 'lookup strings'
    to find them.  This is what makes one server different from another.  Since
    we store password state in a CFtpSite object, we need to have one per
    user/password combo.
\*****************************************************************************/
HRESULT SiteCache_PrivSearch(LPCTSTR pszLookup, LPCITEMIDLIST pidl, IMalloc * pm, CFtpSite ** ppfs)
{
    CFtpSite * pfs = NULL;
    HRESULT hr = S_OK;

    ENTERCRITICAL;

    // CFtpSite_Init() can fail in low memory
    if (SUCCEEDED(CFtpSite_Init()))
    {
        pfs = (CFtpSite *) g_FtpSiteCache->Find(_CompareSites, (LPVOID)pszLookup);   // Add CFtpSite:: ?
        if (!pfs)
        {
            //  We need to hold the critical section while setting up
            //  the new CFtpSite structure, lest somebody else come in
            //  and try to create the same CFtpSite while we are busy.
            hr = CFtpSite_Create(pidl, pszLookup, pm, &pfs);
            if (EVAL(SUCCEEDED(hr)))
            {
                hr = g_FtpSiteCache->AppendItem(pfs);
                if (!(EVAL(SUCCEEDED(hr))))
                    IUnknown_Set(&pfs, NULL);
            }
        }
    }

    LEAVECRITICAL;
    *ppfs = pfs;
    if (pfs)
        pfs->AddRef();

    ASSERT_POINTER_MATCHES_HRESULT(*ppfs, hr);
    return hr;
}



/*****************************************************************************\
    FUNCTION: SiteCache_PidlLookupPrivHelper

    DESCRIPTION:
        We cache information about an FTP Server to prevent hitting the net all
    the time.  This state is stored in CFtpSite objects and we use 'lookup strings'
    to find them.  This is what makes one server different from another.  Since
    we store password state in a CFtpSite object, we need to have one per
    user/password combo.
    
        SiteCache_PidlLookup() does the high level work of deciding if we want
    to do a password redirect.  This function just wraps the creating of the
    lookup string and the fetching of the site.
\*****************************************************************************/
HRESULT SiteCache_PidlLookupPrivHelper(LPCITEMIDLIST pidl, IMalloc * pm, CFtpSite ** ppfs)
{
    HRESULT hr = E_FAIL;
    TCHAR szLookup[MAX_PATH];

    *ppfs = NULL;
    hr = PidlGenerateSiteLookupStr(pidl, szLookup, ARRAYSIZE(szLookup));
    // May fail w/Outofmemory

    if (SUCCEEDED(hr))
        hr = SiteCache_PrivSearch((pidl ? szLookup : TEXT('\0')), pidl, pm, ppfs);

    ASSERT_POINTER_MATCHES_HRESULT(*ppfs, hr);
    return hr;
}


/*****************************************************************************\
    FUNCTION: SiteCache_PidlLookupPrivHelper

    DESCRIPTION:
        We cache information about an FTP Server to prevent hitting the net all
    the time.  This state is stored in CFtpSite objects and we use 'lookup strings'
    to find them.  This is what makes one server different from another.  Since
    we store password state in a CFtpSite object, we need to have one per
    user/password combo.
\*****************************************************************************/
HRESULT SiteCache_PidlLookup(LPCITEMIDLIST pidl, BOOL fPasswordRedir, IMalloc * pm, CFtpSite ** ppfs)
{
    HRESULT hr = E_FAIL;

    if (pidl && !ILIsEmpty(pidl))
    {
        hr = SiteCache_PidlLookupPrivHelper(pidl, pm, ppfs);

        // Okay, we found a site but we may need to redirect to another site
        // because the password is wrong.  This happens if a user goes to
        // ServerA w/UserA and PasswordA but PasswordA is invalid.  So,
        // PasswordB is entered and the navigation completes successfully.
        // Now either the navigation occurs again with PasswordA or w/o
        // a password (because the addrbar removes it), then we need to
        // look it up again and get it.
        if (SUCCEEDED(hr) && (*ppfs)->m_pszRedirPassword && fPasswordRedir)
        {
            LPITEMIDLIST pidlNew;   // with new (redirected) password

            if (FtpPidl_IsAnonymous(pidl))
            {
                pidlNew = ILClone(pidl);
                if (!pidlNew)
                    hr = E_OUTOFMEMORY;
            }
            else
            {
                // We need to redirect to get that CFtpSite.
                hr = PidlReplaceUserPassword(pidl, &pidlNew, pm, NULL, (*ppfs)->m_pszRedirPassword);
            }

            (*ppfs)->Release();
            *ppfs = NULL;
            if (SUCCEEDED(hr))
            {
                hr = SiteCache_PidlLookupPrivHelper(pidlNew, pm, ppfs);
                ILFree(pidlNew);
            }
        }
    }

    ASSERT_POINTER_MATCHES_HRESULT(*ppfs, hr);
    return hr;
}


/*****************************************************************************\
     FUNCTION: UpdateHiddenPassword

     DESCRIPTION:
        Since our IShellFolder::GetDisplayNameOf() will hide the password in some
     cases, we need to 'patch' display names that come thru our
     IShellFolder::GetDisplayName().  If a display name is coming in, we will
     see if the CFtpSite has a m_pszRedirPassword.  If it did, then the user entered
     a password via the 'Login As...' dialog in place of the empty password,
     which made it hidden.  If this is the case, we then have IShellFolder::ParseDisplayName()
     patch back in the password.
\*****************************************************************************/
HRESULT CFtpSite::UpdateHiddenPassword(LPITEMIDLIST pidl)
{
    HRESULT hr = S_FALSE;
    TCHAR szUserName[INTERNET_MAX_USER_NAME_LENGTH];
    TCHAR szPassword[INTERNET_MAX_PASSWORD_LENGTH];

    // Is it a candidate to a password to be inserted?
    if (m_pszPassword && 
        EVAL(SUCCEEDED(FtpPidl_GetUserName(pidl, szUserName, ARRAYSIZE(szUserName)))) &&
        szUserName[0] &&
        SUCCEEDED(FtpPidl_GetPassword(pidl, szPassword, ARRAYSIZE(szPassword), TRUE)) &&
        !szPassword[0]) 
    {
        // Yes...
        hr = FtpServerID_SetHiddenPassword(pidl, m_pszPassword);
    }

    return hr;
}


/*****************************************************************************\
     CFtpSite::GetFtpDir
\*****************************************************************************/
HRESULT CFtpSite::GetFtpDir(LPCTSTR pszServer, LPCWSTR pszUrlPath, CFtpDir ** ppfd)
{
    HRESULT hr = S_OK;
    TCHAR szUrl[MAX_URL_STRING];

    *ppfd = NULL;
    hr = UrlCreate(pszServer, NULL, NULL, pszUrlPath, NULL, INTERNET_DEFAULT_FTP_PORT, NULL, szUrl, ARRAYSIZE(szUrl));
    if (EVAL(SUCCEEDED(hr)))
    {
        LPITEMIDLIST pidl;

        // We know this is a path.
        hr = CreateFtpPidlFromUrlEx(szUrl, GetCWireEncoding(), NULL, &pidl, m_pm, FALSE, TRUE, TRUE);
        if (EVAL(SUCCEEDED(hr)))
        {
            hr = GetFtpDir(pidl, ppfd);
            ILFree(pidl);
        }
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: GetFtpDir

    DESCRIPTION:
        Obtain the FtpDir structure for an FTP site, creating one if
    necessary.  It is the caller's responsibility to Release the
    FtpDir when finished.
\*****************************************************************************/
HRESULT CFtpSite::GetFtpDir(LPCITEMIDLIST pidl, CFtpDir ** ppfd)
{
    HRESULT hr = S_OK;
    CFtpDir * pfd = NULL;

    ENTERCRITICAL;
    ASSERT(ppfd && m_FtpDirList);

    pfd = (CFtpDir *) m_FtpDirList->Find(_CompareDirs, (LPVOID) pidl);
    if (!pfd)
    {
        // We need to hold the critical section while setting up
        // the new FtpDir structure, lest somebody else come in
        // and try to create the same FtpDir while we are busy.
        hr = CFtpDir_Create(this, pidl, &pfd);
        if (EVAL(SUCCEEDED(hr)))
        {
            // NOTE: REF-COUNTING
            //      Note that CFtpDir has a pointer (m_pfs) to a CFtpSite.
            //      We just added a back pointer in CFtpSite's list of CFtpDir(s),
            //      so it's necessary for that back pointer to not have a ref.
            //      This will not be a problem because the back pointers will
            //      always be valid because: 1) CFtpDir's destructor removes the backpointer,
            //      and 2) CFtpDir holds a ref on CFtpSite, so it won't go away until
            //      all the CFtpDir(s) are good and ready.  -BryanSt
            hr = m_FtpDirList->AppendItem(pfd);
            if (FAILED(hr))
                IUnknown_Set(&pfd, NULL);
        }
    }
    LEAVECRITICAL;

    *ppfd = pfd;
    if (pfd)
        pfd->AddRef();

    return hr;
}


/*****************************************************************************\
    FUNCTION: FlushSubDirs

    DESCRIPTION:
        Every subdir of pidl is no longer valid so flush them.  This is done
    because the parent dir may have changed names so they are invalid.

    PARAMETERS:
        pidl: Path of ItemIDs (no-ServerID) that includes the full path w/o
              the virtual root.  This matches CFtpDir::m_pidlFtpDir
\*****************************************************************************/
HRESULT CFtpSite::FlushSubDirs(LPCITEMIDLIST pidl)
{
    HRESULT hr = S_OK;
    CFtpDir * pfd = NULL;
    int nIndex;

    ENTERCRITICAL;

    // Count down so deleting items won't screw up the indicies.
    for (nIndex = (m_FtpDirList->GetCount() - 1); nIndex >= 0; nIndex--)
    {
        pfd = (CFtpDir *) m_FtpDirList->GetItemPtr(nIndex);
        if (pfd)
        {
            // Is this a child?
            if (FtpItemID_IsParent(pidl, pfd->GetPathPidlReference()))
            {
                // Yes, pfd is a child of pidl so delete it.
                m_FtpDirList->DeletePtrByIndex(nIndex);
                pfd->Release();
            }
        }
    }
    LEAVECRITICAL;

    return hr;
}


BOOL CFtpSite::IsSiteBlockedByRatings(HWND hwndDialogOwner)
{
    if (!m_fRatingsChecked)
    {
        void * pvRatingDetails = NULL;
        TCHAR szUrl[MAX_URL_STRING];
        CHAR szUrlAnsi[MAX_URL_STRING];
        HRESULT hr = S_OK;  // Assume allowed (in case no ratings)

        EVAL(SUCCEEDED(UrlCreateFromPidlW(m_pidl, SHGDN_FORPARSING, szUrl, ARRAYSIZE(szUrl), (ICU_ESCAPE | ICU_USERNAME), FALSE)));
        SHTCharToAnsi(szUrl, szUrlAnsi, ARRAYSIZE(szUrlAnsi));

        if (IS_RATINGS_ENABLED())
        {
            // S_OK - Allowed, S_FALSE - Not Allowed, FAILED() - not rated.
            hr = RatingCheckUserAccess(NULL, szUrlAnsi, NULL, NULL, 0, &pvRatingDetails);
            if (S_OK != hr)    // Does user want to override with parent password in dialog?
                hr = RatingAccessDeniedDialog2(hwndDialogOwner, NULL, pvRatingDetails);

            if (pvRatingDetails)
                RatingFreeDetails(pvRatingDetails);
        }

        if (S_OK == hr)     // It's off by default.
            m_fRatingsAllow = TRUE;

        m_fRatingsChecked = TRUE;
    }

    return !m_fRatingsAllow;
}


/*****************************************************************************\
      CFtpSite_Init
  
      Initialize the global list of FTP sites.
  
      Note that the DLL refcount is decremented after this is created,
      so that this internal list doesn't prevent us from unloading.
\*****************************************************************************/
HRESULT CFtpSite_Init(void)
{
    HRESULT hr = S_OK;

    if (!g_FtpSiteCache)
        hr = CFtpList_Create(10, NULL, 10, &g_FtpSiteCache);

    return hr;
}


/*****************************************************************************\
      FtpSitePurge_CallBack
  
      Purge the global list of FTP sites.
\*****************************************************************************/
int FtpSitePurge_CallBack(LPVOID pvPunk, LPVOID pv)
{
    IUnknown * punk = (IUnknown *) pvPunk;

    if (punk)
        punk->Release();

    return 1;
}


/*****************************************************************************\
      CFtpPunkList_Purge
  
      Purge the global list of FTP sites.
\*****************************************************************************/
HRESULT CFtpPunkList_Purge(CFtpList ** pfl)
{
    TraceMsg(TF_FTP_DLLLOADING, "CFtpPunkList_Purge() Purging our cache.");
    if (*pfl)
    {
        (*pfl)->Enum(FtpSitePurge_CallBack, NULL);
        IUnknown_Set(pfl, NULL);
    }

    return S_OK;
}


/*****************************************************************************\
      CFtpSite_Create
  
      Create a brand new CFtpSite given a name.
\*****************************************************************************/
HRESULT CFtpSite_Create(LPCITEMIDLIST pidl, LPCTSTR pszLookupStr, IMalloc * pm, CFtpSite ** ppfs)
{
    CFtpSite * pfs = new CFtpSite();
    HRESULT hr = E_OUTOFMEMORY;

    ASSERT(pidl && pszLookupStr && ppfs);
    *ppfs = NULL;
    if (EVAL(pfs))
    {
        Str_SetPtr(&pfs->m_pszLookupStr, pszLookupStr);

        IUnknown_Set((IUnknown **) &(pfs->m_pm), pm);
        hr = CFtpList_Create(10, NULL, 10, &pfs->m_FtpDirList);
        if (EVAL(SUCCEEDED(hr)))
        {
            // Did someone give us an empty URL?
            if (EVAL(pidl) && EVAL(FtpPidl_IsValid(pidl)))
            {
                TCHAR szServer[INTERNET_MAX_HOST_NAME_LENGTH];
                TCHAR szUser[INTERNET_MAX_USER_NAME_LENGTH];
                TCHAR szPassword[INTERNET_MAX_PASSWORD_LENGTH];
                TCHAR szFragment[INTERNET_MAX_PASSWORD_LENGTH];

                EVAL(SUCCEEDED(FtpPidl_GetServer(pidl, szServer, ARRAYSIZE(szServer))));
                Str_SetPtr(&pfs->m_pszServer, szServer);

                Pidl_Set(&pfs->m_pidl, pidl);

                EVAL(SUCCEEDED(FtpPidl_GetUserName(pidl, szUser, ARRAYSIZE(szUser))));
                Str_SetPtr(&pfs->m_pszUser, szUser);
                
                if (FAILED(FtpPidl_GetPassword(pidl, szPassword, ARRAYSIZE(szPassword), TRUE)))
                {
                       // Password expired
                    szPassword[0] = 0;
                }

                Str_SetPtr(&pfs->m_pszPassword, szPassword);
                FtpPidl_GetFragment(pidl, szFragment, ARRAYSIZE(szFragment));
                Str_SetPtr(&pfs->m_pszFragment, szFragment);

                pfs->m_ipPortNum = FtpPidl_GetPortNum(pidl);

                switch (FtpPidl_GetDownloadType(pidl))
                {
                case FTP_TRANSFER_TYPE_UNKNOWN:
                    pfs->m_fDLTypeSpecified = FALSE;
                    pfs->m_fASCIIDownload = FALSE;
                    break;
                case FTP_TRANSFER_TYPE_ASCII:
                    pfs->m_fDLTypeSpecified = TRUE;
                    pfs->m_fASCIIDownload = TRUE;
                    break;
                case FTP_TRANSFER_TYPE_BINARY:
                    pfs->m_fDLTypeSpecified = TRUE;
                    pfs->m_fASCIIDownload = FALSE;
                    break;
                default:
                    ASSERT(0);
                }
            }
            else
            {
                Str_SetPtr(&pfs->m_pszServer, NULL);
                Str_SetPtr(&pfs->m_pszUser, NULL);
                Str_SetPtr(&pfs->m_pszPassword, NULL);
                Str_SetPtr(&pfs->m_pszFragment, NULL);

                Pidl_Set(&pfs->m_pidl, NULL);
                pfs->m_fDLTypeSpecified = FALSE;
            }
            *ppfs = pfs;
        }
        else
        {
            hr = E_FAIL;
            pfs->Release();
        }
    }

    return hr;
}



/****************************************************\
    Constructor
\****************************************************/
CFtpSite::CFtpSite() : m_cRef(1)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!m_pszServer);
    ASSERT(!m_pidl);
    ASSERT(!m_pszUser);
    ASSERT(!m_pszPassword);
    ASSERT(!m_pszFragment);
    ASSERT(!m_pszLookupStr);
    ASSERT(!m_pidlVirtualDir);

    ASSERT(!m_fMotd);
    ASSERT(!m_hint);
    ASSERT(!m_hgti);
    ASSERT(!m_FtpDirList);
    ASSERT(!m_fRatingsChecked);
    ASSERT(!m_fRatingsAllow);

    LEAK_ADDREF(LEAK_CFtpSite);
}


/****************************************************\
    Destructor
\****************************************************/
CFtpSite::~CFtpSite()
{
    FlushHint();        // Frees m_hgti

    Str_SetPtr(&m_pszServer, NULL);
    Str_SetPtr(&m_pszUser, NULL);
    Str_SetPtr(&m_pszPassword, NULL);
    Str_SetPtr(&m_pszFragment, NULL);
    Str_SetPtr(&m_pszLookupStr, NULL);
    Str_SetPtr(&m_pszRedirPassword, NULL);

    Pidl_Set(&m_pidlVirtualDir, NULL);
    Pidl_Set(&m_pidl, NULL);

    IUnknown_Set(&m_pfgMotd, NULL);

    ASSERTCRITICAL;

    CFtpPunkList_Purge(&m_FtpDirList);

    TriggerDelayedAction(&m_hgti);    // Out goes the cached handle
    ASSERT(m_hint == 0);        // Make sure he's gone
    ATOMICRELEASE(m_pm);

    DllRelease();
    LEAK_DELREF(LEAK_CFtpSite);
}


//===========================
// *** IUnknown Interface ***
ULONG CFtpSite::AddRef()
{
    m_cRef++;
    return m_cRef;
}

ULONG CFtpSite::Release()
{
    ASSERT(m_cRef > 0);
    m_cRef--;

    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}


HRESULT CFtpSite::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = SAFECAST(this, IUnknown*);
    }
    else
    {
        TraceMsg(TF_FTPQI, "CFtpSite::QueryInterface() failed.");
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}
