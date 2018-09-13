/*
 * isurl.cpp - IUniformResourceLocator implementation for Intshcut class.
 */
#include "priv.h"
#include "ishcut.h"
#include "urlprop.h"
#include "assocurl.h"
#include "shlwapi.h"
#include "infotip.h"
#include "resource.h"

#include <mluisupp.h>

#define DM_PLUGGABLE DM_TRACE
#define DM_SHELLEXECOBJECT         0x80000000

extern HRESULT CreateTargetFrame(LPCOLESTR pszTargetName, LPUNKNOWN /*IN,OUT*/ *ppunk);

const TCHAR c_szDefaultVerbSubKeyFmt[]  = TEXT("%s\\Shell");

const TCHAR c_szAppCmdLineFmt[]         = TEXT("\"%s\" %s");
const TCHAR c_szQuotesAppCmdLineFmt[]   = TEXT("\"%s\" \"%s\"");


/***************************** Private Functions *****************************/


/* input flags to MyExecute() */

typedef enum myexecute_in_flags
{
    /*
     * Adds double quotes around the given argument string on the generated
     * command line if the argument string contains any white space.
     */

    ME_IFL_QUOTE_ARGS    = 0x0001,

    /* flag combinations */

    ALL_ME_IN_FLAGS      = ME_IFL_QUOTE_ARGS
}
MYEXECUTE_IN_FLAGS;


/*----------------------------------------------------------
Purpose: Calls CreateProcess() politely

Returns:
Cond:    --
*/
HRESULT
MyExecute(
    LPCTSTR pcszApp,
    LPCTSTR pcszArgs,
    DWORD dwInFlags)
{
    HRESULT hr;
    TCHAR szFullApp[MAX_PATH];

    ASSERT(IS_VALID_STRING_PTR(pcszApp, -1));
    ASSERT(IS_VALID_STRING_PTR(pcszArgs, -1));
    ASSERT(FLAGS_ARE_VALID(dwInFlags, ALL_ME_IN_FLAGS));

    hr = PathSearchAndQualify(pcszApp, szFullApp, SIZECHARS(szFullApp));

    if (hr == S_OK)
    {
        DWORD cbSize;
        LPTSTR pszCmdLine;

        /* (+ 1) for null terminator. */
        cbSize = max(SIZEOF(c_szAppCmdLineFmt),
                         SIZEOF(c_szQuotesAppCmdLineFmt)) +
                         + CbFromCch(lstrlen(szFullApp) + lstrlen(pcszArgs) + 1);

        pszCmdLine = (LPTSTR)LocalAlloc(LPTR, cbSize);

        if (pszCmdLine)
        {
            LPCTSTR pcszFmt;
            STARTUPINFO si;
            PROCESS_INFORMATION pi;

            /* Execute URL via one-shot app. */

            pcszFmt = (IsFlagSet(dwInFlags, ME_IFL_QUOTE_ARGS) &&
                       StrPBrk(pcszArgs, TEXT(" \t")) != NULL)
                       ? c_szQuotesAppCmdLineFmt : c_szAppCmdLineFmt;

            wnsprintf(pszCmdLine, cbSize / sizeof(TCHAR), pcszFmt, szFullApp, pcszArgs);

            ZeroMemory(&si, SIZEOF(si));
            si.cb = SIZEOF(si);

            /* Specify command line exactly as given to app. */

            if (CreateProcess(NULL, pszCmdLine, NULL, NULL, FALSE, 0, NULL, NULL,
                              &si, &pi))
            {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);

                hr = S_OK;

                TraceMsg(TF_INTSHCUT, "MyExecute(): CreateProcess() \"%s\" succeeded.",
                           pszCmdLine);
            }
            else
            {
                hr = E_FAIL;

                TraceMsg(TF_WARNING, "MyExecute(): CreateProcess() \"%s\" failed.",
                             pszCmdLine);
            }

            LocalFree(pszCmdLine);
            pszCmdLine = NULL;
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else
        TraceMsg(TF_WARNING, "MyExecute(): Unable to find app %s.",
                     pcszApp);

    return(hr);
}


/*----------------------------------------------------------
Purpose: Returns TRUE if the given internet shortcut points
         to a website (as opposed to an ftp site, etc).

Returns: see above
*/
BOOL IsWebsite(IN Intshcut * pintshcut)
{
    ASSERT(pintshcut);
    
    //  (BUGBUG (scotth): we are assuming that file: schemes are
    //   generally web pages.  This is not true.  For file: schemes,
    //   we should first verify that it is an htm filetype.)
    
    return (URL_SCHEME_HTTP == pintshcut->GetScheme() ||
        URL_SCHEME_FILE == pintshcut->GetScheme());
}



BOOL
GetClassDefaultVerb(
    LPCTSTR pcszClass,
    LPTSTR  pszDefaultVerbBuf,
    UINT    cchBufLen)
{
    // No; get the default verb
    TCHAR szKey[MAX_PATH];

    StrCpyN(szKey, pcszClass, SIZECHARS(szKey));
    StrCatBuff(szKey, TEXT("\\"), SIZECHARS(szKey));
    StrCatBuff(szKey, TEXT("shell"), SIZECHARS(szKey));
    DWORD cbSize = CbFromCch(cchBufLen);

    if (NO_ERROR != SHGetValue(HKEY_CLASSES_ROOT, szKey, NULL, NULL, pszDefaultVerbBuf, &cbSize) 
    || !*pszDefaultVerbBuf)
    {
        // Default to "open" if the registry doesn't specify one
        StrCpyN(pszDefaultVerbBuf, TEXT("open"), cchBufLen);
    }

    return TRUE;
}


#ifdef DEBUG

BOOL
IsValidPCPARSEDURL(
    LPCTSTR pcszURL,
    PCPARSEDURL pcpu)
{
    return(IS_VALID_READ_PTR(pcpu, CPARSEDURL) &&
           (IS_VALID_STRING_PTR(pcpu->pszProtocol, -1) &&
            EVAL(IsStringContained(pcszURL, pcpu->pszProtocol)) &&
            EVAL(pcpu->cchProtocol < (UINT)lstrlen(pcpu->pszProtocol))) &&
           (IS_VALID_STRING_PTR(pcpu->pszSuffix, -1) &&
            EVAL(IsStringContained(pcszURL, pcpu->pszSuffix)) &&
            EVAL(pcpu->cchSuffix <= (UINT)lstrlen(pcpu->pszSuffix))) &&
           EVAL(pcpu->cchProtocol + pcpu->cchSuffix < (UINT)lstrlen(pcszURL)));
}


BOOL
IsValidPCURLINVOKECOMMANDINFO(
    PCURLINVOKECOMMANDINFO pcurlici)
{
    return(IS_VALID_READ_PTR(pcurlici, CURLINVOKECOMMANDINFO) &&
           EVAL(pcurlici->dwcbSize >= SIZEOF(*pcurlici)) &&
           FLAGS_ARE_VALID(pcurlici->dwFlags, ALL_IURL_INVOKECOMMAND_FLAGS) &&
           (IsFlagClear(pcurlici->dwFlags, IURL_INVOKECOMMAND_FL_ALLOW_UI) ||
            NULL == pcurlici->hwndParent || 
            IS_VALID_HANDLE(pcurlici->hwndParent, WND)) &&
           (IsFlagSet(pcurlici->dwFlags, IURL_INVOKECOMMAND_FL_USE_DEFAULT_VERB) ||
            IS_VALID_STRING_PTR(pcurlici->pcszVerb, -1)));
}

#endif

BOOL IsValidProtocolChar(TCHAR ch)
{
    if ((ch>=TEXT('a') && ch<=TEXT('z')) ||
        (ch>=TEXT('A') && ch<=TEXT('Z')) ||
        (ch>=TEXT('0') && ch<=TEXT('9')) ||
        (ch == TEXT('+')) ||
        (ch == TEXT('-')) ||
        (ch == TEXT('.'))   )
    {
        return TRUE;
    }
    return FALSE;
}


/********************************** Methods **********************************/

typedef struct
{
    UINT idsVerb;
    UINT idsMenuHelp;
    LPCTSTR pszVerb;
} ISCM;

const static ISCM g_rgiscm[] =
{
    { IDS_MENUOPEN,         IDS_MH_OPEN,            TEXT("open") },         //  IDCMD_ISCM_OPEN 
    { IDS_SYNCHRONIZE,      IDS_MH_SYNCHRONIZE,     TEXT("update now")},    //  IDCMD_ISCM_SYNC 
    { IDS_MAKE_OFFLINE,     IDS_MH_MAKE_OFFLINE,    TEXT("subscribe")},     //  IDCMD_ISCM_SUB  
};

//  WARNING - these must match their index into g_rgiscm
#define IDCMD_ISCM_OPEN   0
#define IDCMD_ISCM_SYNC   1
#define IDCMD_ISCM_SUB    2

BOOL _IsSubscribed(LPCWSTR pszUrl, BOOL *pfSubscribable)
{
    BOOL fRet = FALSE;
    ISubscriptionMgr * pMgr;
    
    *pfSubscribable = FALSE;
    
    if (SUCCEEDED(CoCreateInstance(CLSID_SubscriptionMgr, NULL, CLSCTX_INPROC_SERVER, IID_ISubscriptionMgr, (void **)& pMgr)))
    {
        pMgr->IsSubscribed(pszUrl, &fRet);

                        
        pMgr->Release();
    }

    if (!fRet)
    {
        //test if we CAN subscribe to this thing
        if (!SHRestricted2W(REST_NoAddingSubscriptions, pszUrl, 0) &&
            IsFeaturePotentiallyAvailable(CLSID_SubscriptionMgr))
        {
            *pfSubscribable = IsSubscribableW(pszUrl);
        }
    }
    else
        *pfSubscribable = TRUE;
    
    return fRet;
}

void _InsertISCM(UINT indexISCM, HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT uFlags)
{
    TCHAR szMenu[CCH_MENUMAX];
    uFlags |= MF_BYPOSITION | MF_STRING;

    MLLoadShellLangString(g_rgiscm[indexISCM].idsVerb, szMenu, SIZECHARS(szMenu));
    InsertMenu_PrivateNoMungeW(hmenu, indexMenu, uFlags, idCmdFirst + indexISCM, szMenu);
}

// IContextMenu::QueryContextMenu handler for Intshcut
// The context menu handler adds the open verb for .url
// files.  This is because we remove the shell\open\command
// key in Nashville for this file type.

STDMETHODIMP Intshcut::QueryContextMenu(
    IN HMENU hmenu,
    IN UINT  indexMenu,
    IN UINT  idCmdFirst,
    IN UINT  idCmdLast,
    IN UINT  uFlags)
{
    //
    //  BUGBUGLEGACY - .URL files have to maintain an open verb in the registry - ZekeL - 14-APR-99
    //  we would like to just use the "open" verb here in the context menu extension,
    //  but we need to not duplicate the open verb that is added by DefCM
    //  on NT5+ shell32 we disable that verb so we can add it here.
    //  on earlier shell32 we want to add "open" any time we arent
    //  initialized by DefCM.  if we think that DefCM added us, 
    //  then we go ahead and allow the DefCM's open from the registry.
    //
    if (!m_fProbablyDefCM || GetUIVersion() >= 5)
    {
        _InsertISCM(IDCMD_ISCM_OPEN, hmenu, indexMenu, idCmdFirst, 0);
        if (-1 == GetMenuDefaultItem(hmenu, MF_BYCOMMAND, 0))
            SetMenuDefaultItem(hmenu, indexMenu, MF_BYPOSITION);
        indexMenu++;
    }

#ifndef UNIX
    /* v-sriran: 12/8/97
     * disabling the context menu item for subscribe, separators etc.
     * because we are not supporting subscriptions right now
     */

    // skip this if we only want default or if there is no room for more.
    if (!(uFlags & CMF_DEFAULTONLY) && (idCmdLast - idCmdFirst >= ARRAYSIZE(g_rgiscm)))
    {
        WCHAR *pwszURL;
        if (SUCCEEDED(GetURLW(&pwszURL)))
        {
            BOOL bSubscribable = FALSE;             //can be subscribed to
            BOOL bSub = _IsSubscribed(pwszURL, &bSubscribable);
            m_bCheckForDelete = bSub && m_pszFile;

            if (bSubscribable || bSub)
            {
                //  add a separator for our subscription stuff
                InsertMenu(hmenu, indexMenu++, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
                UINT uMenuFlags = 0;

                if (bSub)
                {
                    uMenuFlags |= MF_CHECKED;

                    if (SHRestricted2W(REST_NoRemovingSubscriptions, pwszURL, 0))
                    {
                        uMenuFlags |= MF_GRAYED;
                    }
                }

                _InsertISCM(IDCMD_ISCM_SUB, hmenu, indexMenu++, idCmdFirst, uMenuFlags);

                if (bSub)
                {
                    uMenuFlags = 0;

                    if (SHRestricted2W(REST_NoManualUpdates, NULL, 0))
                    {
                        uMenuFlags |= MF_GRAYED;
                    }
                    _InsertISCM(IDCMD_ISCM_SYNC, hmenu, indexMenu++, idCmdFirst, uMenuFlags);
                } 
            }
            
            SHFree(pwszURL);
        }
    }

#endif /* UNIX */

    return ResultFromShort(ARRAYSIZE(g_rgiscm));
}

STDMETHODIMP Intshcut::InvokeCommand(IN LPCMINVOKECOMMANDINFO pici)
{
    HRESULT hres = E_INVALIDARG;

    ASSERT(pici);

    if (pici && SIZEOF(*pici) <= pici->cbSize)
    {
        UINT idCmd;

        if (0 == HIWORD(pici->lpVerb))      // Is the ID cmd given?
        {
            idCmd = LOWORD(pici->lpVerb);   // Yes

            //  BUGBUG - old versions of ShellExec() didnt get the right default command - Zekel - 15-MAR-99
            //  since our QCM implementation doesnt add anything to the menu
            //  if we fix the QCM to work correctly, then this problem will go away.
            //  it sent 0xfffe instead.  so just adjust here.
            if (idCmd == 0xfffe && GetUIVersion() <= 4)
                idCmd = IDCMD_ISCM_OPEN;
        }
        else
        {
            // No; a language-independent verb was supplied
            int i;
            LPCTSTR pszVerb;
            LPCMINVOKECOMMANDINFOEX piciex = (LPCMINVOKECOMMANDINFOEX)pici;
            ASSERT(SIZEOF(*piciex) <= piciex->cbSize);

            WCHAR szVerb[40];

            if (piciex->lpVerbW)
            {
                pszVerb = piciex->lpVerbW;
            }
            else
            {
                if (piciex->lpVerb)
                {
                    ASSERT(lstrlenA(piciex->lpVerb) < ARRAYSIZE(szVerb));
                    SHAnsiToUnicode(piciex->lpVerb, szVerb, ARRAYSIZE(szVerb));    
                }
                else
                {
                    szVerb[0] = L'\0';
                }
                    
                pszVerb = szVerb;
            }

            idCmd = (UINT)-1;
            for (i = 0; i < ARRAYSIZE(g_rgiscm); i++)
            {
                if (0 == StrCmpI(g_rgiscm[i].pszVerb, pszVerb))
                {
                    idCmd = i;
                    break;
                }
            }
        }

        switch (idCmd)
        {
        case IDCMD_ISCM_OPEN: 
            {
                URLINVOKECOMMANDINFO urlici;

                urlici.dwcbSize = SIZEOF(urlici);
                urlici.hwndParent = pici->hwnd;
                urlici.pcszVerb = NULL;
                urlici.dwFlags = IURL_INVOKECOMMAND_FL_USE_DEFAULT_VERB;

                if (IsFlagClear(pici->fMask, CMIC_MASK_FLAG_NO_UI))
                {
                    SetFlag(urlici.dwFlags, IURL_INVOKECOMMAND_FL_ALLOW_UI);
                }
                hres = InvokeCommand(&urlici);
                m_bCheckForDelete = FALSE;
            }
            break;

        case IDCMD_ISCM_SUB:
        case IDCMD_ISCM_SYNC:
        {
            hres = S_OK;

            WCHAR *pwszURL;
            if (SUCCEEDED(GetURLW(&pwszURL)))
            {
                ISubscriptionMgr * pMgr;
                if (SUCCEEDED(JITCoCreateInstance(CLSID_SubscriptionMgr, 
                                                  NULL, 
                                                  CLSCTX_INPROC_SERVER, 
                                                  IID_ISubscriptionMgr, 
                                                  (void **)&pMgr,
                                                  pici->hwnd,
                                                  FIEF_FLAG_FORCE_JITUI))) 
                {
                    if (idCmd == IDCMD_ISCM_SUB)  
                    {
                        BOOL bSubscribed;

                        pMgr->IsSubscribed(pwszURL, &bSubscribed);

                        if (!bSubscribed)
                        {
                            SHFILEINFO  sfi = {0};
                            WCHAR wszName[MAX_PATH];
                            wszName[0] = 0;
                            if (SHGetFileInfo(m_pszFile, 0, &sfi, sizeof(sfi), SHGFI_DISPLAYNAME))
                            {
                                SHTCharToUnicode(sfi.szDisplayName, wszName, ARRAYSIZE(wszName));
                            }

                            if (!wszName[0])
                                StrCpyNW(wszName, pwszURL, ARRAYSIZE(wszName));

                            //all subscriptions to local .urls are treated as subscribing something
                            //that's already in Favorites, so user isn't forced to add it to their
                            //favorites as they subscribe.
                            if (SUCCEEDED(pMgr->CreateSubscription(pici->hwnd, pwszURL, wszName,
                                                                   CREATESUBS_FROMFAVORITES, 
                                                                   SUBSTYPE_URL, 
                                                                   NULL)))
                            {
                                pMgr->UpdateSubscription(pwszURL);
                            }
                        }
                        else
                        {
                            pMgr->DeleteSubscription(pwszURL, pici->hwnd);
                        }
                    } 
                    else if (idCmd == IDCMD_ISCM_SYNC)
                    {
                        pMgr->UpdateSubscription(pwszURL);
                    }
                    pMgr->Release();    
                }
                SHFree(pwszURL);
                m_bCheckForDelete = FALSE;
            }
            break;
        }

        default:
            hres = E_INVALIDARG;
            break;
        }
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: IContextMenu::GetCommandString handler for Intshcut

*/
STDMETHODIMP Intshcut::GetCommandString(
    IN     UINT_PTR idCmd,
    IN     UINT     uType,
    IN OUT UINT*    puReserved,
    IN OUT LPSTR    pszName,
    IN     UINT     cchMax)
{
    HRESULT hres;
    TCHAR szMenu[CCH_MENUMAX];

    ASSERT(NULL == puReserved);
    ASSERT(IS_VALID_WRITE_BUFFER(pszName, char, cchMax));

    switch (uType)
    {
    case GCS_HELPTEXTA:
    case GCS_HELPTEXTW:
        if (idCmd < ARRAYSIZE(g_rgiscm))
        {
            MLLoadString(g_rgiscm[idCmd].idsMenuHelp, szMenu, SIZECHARS(szMenu));

            if (GCS_HELPTEXTA == uType)
            {
                UnicodeToAnsi(szMenu, pszName, cchMax);
            }
            else
            {
                StrCpyN((LPWSTR)pszName, szMenu, cchMax);
            }
            hres = NOERROR;
        }
        else
        {
            ASSERT(0);
            hres = E_INVALIDARG;
        }
        break;

    case GCS_VALIDATEA:
    case GCS_VALIDATEW:
        hres = idCmd < ARRAYSIZE(g_rgiscm) ? S_OK : S_FALSE;
        break;

    case GCS_VERBA:
    case GCS_VERBW:
        if (idCmd < ARRAYSIZE(g_rgiscm))
        {
            LPCTSTR pszVerb = g_rgiscm[idCmd].pszVerb;

            if (GCS_VERBA == uType)
            {
                UnicodeToAnsi(pszVerb, pszName, cchMax);
            }
            else
            {
                StrCpyN((LPWSTR)pszName, pszVerb, cchMax);
            }
            hres = NOERROR;
        }
        else
        {
            ASSERT(0);
            hres = E_INVALIDARG;
        }
        break;

    default:
        hres = E_NOTIMPL;
        break;
    }

    return hres;
}


// IContextMenu2::HandleMenuMsg handler for Intshcut
STDMETHODIMP Intshcut::HandleMenuMsg(IN UINT uMsg, IN WPARAM wParam, IN LPARAM lParam)
{
    return S_OK;
}


/*----------------------------------------------------------
Purpose: Bring up UI to ask the user what to associate this 
         URL protocol to.

*/
STDMETHODIMP
Intshcut::RegisterProtocolHandler(
    HWND hwndParent,
    LPTSTR pszAppBuf,
    UINT cchBuf)
{
    HRESULT hr;
    DWORD dwFlags = 0;
    TCHAR szURL[MAX_URL_STRING];

    ASSERT(! hwndParent ||
           IS_VALID_HANDLE(hwndParent, WND));
    ASSERT(IS_VALID_WRITE_BUFFER(pszAppBuf, TCHAR, cchBuf));
    ASSERT(m_pprop);

    hr = m_pprop->GetProp(PID_IS_URL, szURL, SIZECHARS(szURL));
    ASSERT(S_OK == hr);

    SetFlag(dwFlags, URLASSOCDLG_FL_REGISTER_ASSOC);

    if (! m_pszFile)
        SetFlag(dwFlags, URLASSOCDLG_FL_USE_DEFAULT_NAME);

    hr = AssociateURL(hwndParent, dwFlags, m_pszFile, szURL, pszAppBuf, cchBuf);

    switch (hr)
    {
    case S_FALSE:
        TraceMsg(TF_INTSHCUT, "Intshcut::RegisterProtocolHandler(): One time execution of %s via %s requested.",
                    szURL, pszAppBuf);
        break;

    case S_OK:
        TraceMsg(TF_INTSHCUT, "Intshcut::RegisterProtocolHandler(): Protocol handler registered for %s.",
                    szURL);
        break;

    default:
        ASSERT(FAILED(hr));
        break;
    }

    ASSERT(! cchBuf ||
           (IS_VALID_STRING_PTR(pszAppBuf, -1) &&
            (UINT)lstrlen(pszAppBuf) < cchBuf));

    return(hr);
}


// Returns the protocol scheme value (URL_SCHEME_*).

STDMETHODIMP_(DWORD)
Intshcut::GetScheme(void)
{
    DWORD dwScheme;

    ASSERT(m_pprop);

    m_pprop->GetProp(PID_IS_SCHEME, &dwScheme);
    return dwScheme;
}


// IUniformResourceLocator::SetURL handler for Intshcut
//
// Note:
//    1. SetURL clears the IDList, so that when we launch this shortcut,
//        we will use the URL.

STDMETHODIMP
Intshcut::SetURL(
    IN LPCTSTR pszURL,      OPTIONAL
    IN DWORD   dwFlags)
{
    HRESULT hres = E_FAIL;

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    ASSERT(! pszURL ||
           IS_VALID_STRING_PTR(pszURL, -1));
    ASSERT(FLAGS_ARE_VALID(dwFlags, ALL_IURL_SETURL_FLAGS));

    hres = InitProp();
    if (SUCCEEDED(hres))
    {
        hres = m_pprop->SetURLProp(pszURL, dwFlags);
        if (SUCCEEDED(hres))
        {
            // if the path was set successfully, clear the pidl.
            m_pprop->SetIDListProp(NULL);
        }
    }

    return hres;
}



/*----------------------------------------------------------
Purpose: IUniformResourceLocatorA::SetURL handler for Intshcut

         Ansi version

*/
STDMETHODIMP
Intshcut::SetURL(
    IN LPCSTR pcszURL,      OPTIONAL
    IN DWORD  dwInFlags)
{
    if ( !pcszURL )
    {
        return SetURL((LPCTSTR)NULL, dwInFlags);
    }
    else
    {
        WCHAR wszURL[MAX_URL_STRING];

        ASSERT(IS_VALID_STRING_PTRA(pcszURL, -1));

        AnsiToUnicode(pcszURL, wszURL, SIZECHARS(wszURL));

        return SetURL(wszURL, dwInFlags);
    }
}


STDMETHODIMP Intshcut::GetURLW(WCHAR **ppwsz)
{
    LPTSTR  pszURL;
    HRESULT hres = GetURL(&pszURL);
    if (S_OK == hres)
    {
        hres = SHStrDup(pszURL, ppwsz);
        SHFree(pszURL);
    }
    else
        hres = E_FAIL;  // map S_FALSE to FAILED()
    return hres;
}

// IUniformResourceLocator::GetURL handler for Intshcut

STDMETHODIMP Intshcut::GetURL(LPTSTR * ppszURL)
{
    HRESULT hres;
    TCHAR szURL[MAX_URL_STRING];

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    ASSERT(IS_VALID_WRITE_PTR(ppszURL, PTSTR));

    *ppszURL = NULL;

    hres = InitProp();
    if (SUCCEEDED(hres))
    {
        hres = m_pprop->GetProp(PID_IS_URL, szURL, SIZECHARS(szURL));
        if (S_OK == hres)
        {
            // (+ 1) for null terminator.
            int cch = lstrlen(szURL) + 1;
            *ppszURL = (PTSTR)SHAlloc(CbFromCch(cch));
            if (*ppszURL)
                StrCpyN(*ppszURL, szURL, cch);
            else
                hres = E_OUTOFMEMORY;
        }
    }

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    ASSERT((hres == S_OK &&
            IS_VALID_STRING_PTR(*ppszURL, -1)) ||
           ((hres == S_FALSE ||
             hres == E_OUTOFMEMORY) &&
            ! *ppszURL));

    return hres;
}



/*----------------------------------------------------------
Purpose: IUniformResourceLocatorA::GetURL handler for Intshcut

         Ansi version

*/
STDMETHODIMP Intshcut::GetURL(LPSTR * ppszURL)
{
    HRESULT hres;
    TCHAR szURL[MAX_URL_STRING];

    ASSERT(IS_VALID_WRITE_PTR(ppszURL, PSTR));

    *ppszURL = NULL;

    hres = InitProp();
    if (SUCCEEDED(hres))
    {
        hres = m_pprop->GetProp(PID_IS_URL, szURL, SIZECHARS(szURL));

        if (S_OK == hres)
        {
            DWORD cch = WideCharToMultiByte(CP_ACP, 0, szURL, -1, NULL, 0, NULL, NULL);
            *ppszURL = (LPSTR)SHAlloc(CbFromCchA(cch + 1));

            if (*ppszURL)
                UnicodeToAnsi(szURL, *ppszURL, cch);
            else
                hres = E_OUTOFMEMORY;
        }
    }

    return hres;
}


HRESULT HandlePluggableProtocol(LPCTSTR pszURL, LPCTSTR pszProtocol)
{
    HRESULT hres = E_UNEXPECTED;
    HKEY hkey;
    TraceMsg(DM_PLUGGABLE, "HandlePluggableProtocol called");

    if (RegOpenKey(HKEY_CLASSES_ROOT, TEXT("PROTOCOLS\\Handler"), &hkey) == ERROR_SUCCESS) {
        HKEY hkeyProtocol;
        if (RegOpenKey(hkey, pszProtocol, &hkeyProtocol) == ERROR_SUCCESS) {
            TraceMsg(DM_PLUGGABLE, "HandlePluggableProtocol found %s", pszProtocol);
            IUnknown* punk = NULL; // CreateTargetFrame's ppunk is [IN][OUT]
            hres = CreateTargetFrame(NULL, &punk);
            if (SUCCEEDED(hres)) {
                IWebBrowser2* pauto;
                hres = punk->QueryInterface(IID_IWebBrowser2, (LPVOID*)&pauto);
                if (SUCCEEDED(hres)) {
                    TraceMsg(DM_PLUGGABLE, "HandlePluggableProtocol calling navigate with %s", pszURL);
                    SA_BSTR str;
                    SHTCharToUnicode(pszURL, str.wsz, ARRAYSIZE(str.wsz));
                    str.cb = lstrlenW(str.wsz) * SIZEOF(WCHAR);
                    pauto->Navigate(str.wsz, PVAREMPTY, PVAREMPTY, PVAREMPTY, PVAREMPTY);
                    pauto->put_Visible(TRUE);
                    pauto->Release();
                }
                punk->Release();
            }
            RegCloseKey(hkeyProtocol);
        } else {
            TraceMsg(DM_WARNING, "HandlePluggableProtocol can't find %s", pszProtocol);
        }
        RegCloseKey(hkey);
    } else {
        ASSERT(0);
    }
    return hres;
}

extern "C" HRESULT CoAllowSetForegroundWindow(IUnknown *punk, void *pvReserved);

HRESULT _IEExecFile_TryRunningWindow(VARIANT *pvarIn, DWORD cid)
{
    HRESULT hr = E_FAIL;
    ASSERT(pvarIn);

    IShellWindows *psw = WinList_GetShellWindows(TRUE);
    if (psw)
    {
        IUnknown *punk;
        if (SUCCEEDED(psw->_NewEnum(&punk)))
        {
            VARIANT var = {0};
            IEnumVARIANT *penum;

            //
            //  its too bad _NewEnum doesnt return an penum....
            //  this should never fail.
            //
            punk->QueryInterface(IID_IEnumVARIANT, (LPVOID *)&penum);
            ASSERT(penum);

            //
            //  this can be super spendy since every one of these
            //  items is marshalled.
            //
            //  should we clone the stream here??
            //
            while (FAILED(hr) && S_OK == penum->Next(1, &var, NULL))
            {
                
                ASSERT(var.vt == VT_DISPATCH);
                ASSERT(var.pdispVal);
                IOleCommandTarget *poct;
                
                if (SUCCEEDED(var.pdispVal->QueryInterface(IID_IOleCommandTarget, (LPVOID *)&poct)))
                {
                    CoAllowSetForegroundWindow(poct, NULL);
                    
                    hr = poct->Exec(&CGID_Explorer, cid, 0, pvarIn, NULL);

                    poct->Release();
                }
                
                //  this should release the pdisp
                VariantClear(&var);
            }

            punk->Release();
            penum->Release();
        }

        
        psw->Release();
    }


    TraceMsgW(DM_SHELLEXECOBJECT, "IEExecFile_Running returns 0x%X", hr);
    return hr;
}


BOOL
IsIESchemeHandler(LPTSTR pszVerb, LPTSTR pszScheme)
{
    //  if we fail to get any value at all, the we must assume that it
    //  is some protocol like about: or res: that is not in the registry
    //  so we default to success.
    BOOL fRet = FALSE;
    TCHAR szExe[MAX_PATH];

    if (SUCCEEDED(AssocQueryString(0, ASSOCSTR_EXECUTABLE, pszScheme, pszVerb, szExe, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(szExe)))))
    {
        //  if we find something and it aint us, then fail.
        if ((StrStrI(szExe, TEXT("iexplore.exe")) || StrStrI(szExe, TEXT("explorer.exe"))))
        {
            fRet = TRUE;

            TraceMsg(DM_SHELLEXECOBJECT, "IsIEScheme() found %s", szExe);
        }
    }
    else
    {
        // these are unregistered schemes, we are the only ones that 
        //  should ever even use the unregistered schemes like
        //  res: or shell: so return TRUE here too.
        fRet = TRUE;
    }
    
    TraceMsg(DM_SHELLEXECOBJECT, "IsIEScheme() returns %d for %s", fRet, pszScheme);
    return fRet;
}    

HRESULT IEExecFile(LPTSTR pszVerb, LPTSTR pszScheme, DWORD cid, LPTSTR pszPath)
{
    HRESULT hr = E_FAIL;
    ASSERT(pszVerb);
    ASSERT(pszScheme);
    ASSERT(pszPath);
    
    if (IsIESchemeHandler(pszVerb, pszScheme))
    {
        VARIANT varIn = {0};
        varIn.vt = VT_BSTR;

        SHSTRW str;
        str.SetStr(pszPath);
        varIn.bstrVal = SysAllocString(str.GetStr());
        if (varIn.bstrVal)
        {

            if (!SHRegGetBoolUSValue(REGSTR_PATH_MAIN, TEXT("AllowWindowReuse"), FALSE, TRUE)
            || FAILED(hr = _IEExecFile_TryRunningWindow(&varIn, cid)))
            {
                IOleCommandTarget *poct;
    
                if (SUCCEEDED(CoCreateInstance(CLSID_InternetExplorer, NULL, CLSCTX_ALL, 
                        IID_IOleCommandTarget, (LPVOID *)&poct)))
                {
                    hr = poct->Exec(&CGID_Explorer, cid, 0, &varIn, NULL);
                    poct->Release();
                }
            }

            SysFreeString(varIn.bstrVal);

        }

    }

    TraceMsg(DM_SHELLEXECOBJECT, "IEExecFile returns 0x%X for %s", hr, pszPath);

    return hr;
}
                
            
/*----------------------------------------------------------
Purpose: IUniformResourceLocator::InvokeCommand for Intshcut

Note:
    1. If the internet shortcut comes with a pidl, use it to ShellExec,
        otherwise use the URL.

*/
STDMETHODIMP Intshcut::InvokeCommand(PURLINVOKECOMMANDINFO purlici)
{
    HRESULT hr = E_INVALIDARG;
    BOOL bExecFailedWhine = FALSE;

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    ASSERT(IS_VALID_STRUCT_PTR(purlici, CURLINVOKECOMMANDINFO));

    if (purlici && EVAL(SIZEOF(*purlici) == purlici->dwcbSize))
    {
        //
        // App compat.  Don't use stack space for the URL.  We use up 16-bit app
        // stack space when we they shell exec urls.
        //

        LPWSTR pszURL = (LPWSTR)LocalAlloc(LPTR, MAX_URL_STRING * sizeof(WCHAR));

        if (pszURL)
        {
            hr = InitProp();
            if (SUCCEEDED(hr))
            {
                //
                // App Compat: Don't use up stack space.
                //

                LPWSTR pszT = (LPWSTR)LocalAlloc(LPTR, MAX_PATH * sizeof(WCHAR));

                if (pszT)
                {
                    SHELLEXECUTEINFO sei = {0};
                    LPITEMIDLIST pidl = NULL;
                    LPTSTR pszProtocol = NULL;
                    PARSEDURL pu;
                    pu.nScheme = 0; // init to avoid bogus C4701 warning

                    sei.fMask = SEE_MASK_NO_HOOKS;


                    // check if we have a pidl for the target.        
                    hr = GetIDListInternal(&pidl);
                    if ((hr == S_OK) && pidl)
                    {
                        // yse, use the pidl to ShellExec.
                        sei.fMask |= SEE_MASK_INVOKEIDLIST;
                        sei.lpIDList = pidl;
                    }
                    else
                    {
                        // no, get the URL and invoke class handler.
                        sei.fMask |= SEE_MASK_CLASSNAME;
                        hr = m_pprop->GetProp(PID_IS_URL, pszURL, MAX_URL_STRING);
                        if (S_OK == hr)
                        {
                            hr = CopyURLProtocol(pszURL, &pszProtocol, &pu);
               
                            if (hr == S_OK)
                            {
                                hr = IsProtocolRegistered(pszProtocol);
               
                                if (hr == URL_E_UNREGISTERED_PROTOCOL &&
                                    IsFlagSet(purlici->dwFlags, IURL_INVOKECOMMAND_FL_ALLOW_UI))
                                {
                                    TraceMsg(TF_INTSHCUT, "Intshcut::InvokeCommand(): Unregistered URL protocol %s.  Invoking URL protocol handler association dialog.",
                                               pszProtocol);
               
                                    hr = RegisterProtocolHandler(purlici->hwndParent, pszT,
                                                                 MAX_PATH);
                                    if (FAILED(hr))
                                        hr = URL_E_UNREGISTERED_PROTOCOL;
                                }

                                //
                                // I have no idea what this RegisterProtocolHandler
                                // does (it looks too complicated). I, however, know
                                // that we come here if the user type one of pluggable
                                // protocol. And RegisterProtocolHandler returns E_FAIL.
                                // (SatoNa)
                                //
                                if (FAILED(hr)) {
                                    if (SUCCEEDED(HandlePluggableProtocol(pszURL, pszProtocol))) {
                                        hr = S_OK;
                                        goto done;
                                    }
                                }
                            }
                        }
                    }
                
                    switch (hr)
                    {
                        case S_OK:
                        {
                            //
                            // App Compat: Don't use up stack space.
                            //

                            LPWSTR pszVerb = (LPWSTR)LocalAlloc(LPTR, MAX_PATH * sizeof(WCHAR));

                            if (pszVerb)
                            {
                                int nShowCmd;
   
                                // Execute URL via registered protocol handler.
   
                                if (IsFlagClear(purlici->dwFlags,
                                                IURL_INVOKECOMMAND_FL_ALLOW_UI))
                                    SetFlag(sei.fMask, SEE_MASK_FLAG_NO_UI);

                                if (purlici->dwFlags & IURL_INVOKECOMMAND_FL_DDEWAIT)
                                    SetFlag(sei.fMask, SEE_MASK_FLAG_DDEWAIT);
                        
                                if (IsFlagClear(purlici->dwFlags,
                                                IURL_INVOKECOMMAND_FL_USE_DEFAULT_VERB))
                                {
                                    sei.lpVerb = purlici->pcszVerb;
                                }
                                else
                                {
                                    if (pszProtocol &&
                                        GetClassDefaultVerb(pszProtocol, pszVerb,
                                                            MAX_PATH))
                                        sei.lpVerb = pszVerb;
                                    else
                                        ASSERT(! sei.lpVerb);
                                }
   
                                m_pprop->GetProp(PID_IS_WORKINGDIR, pszT, MAX_PATH);
                                m_pprop->GetProp(PID_IS_SHOWCMD, &nShowCmd); // inits to zero if not found

                                //  if we have a file try using a direct connection
                                //  to the shell to give the whole shortcut
                                if (m_pszFile && ((IsIEDefaultBrowser()) || (_IsInFavoritesFolder()) ) )
                                    hr = IEExecFile(pszVerb, pszProtocol, SBCMDID_IESHORTCUT, m_pszFile);
                                else 
                                    hr = E_FAIL;

                                //  if we failed to pass it to IE, then we should just default 
                                //  to the old behavior
                                if (FAILED(hr))
                                {

                                    sei.cbSize = SIZEOF(sei);
                                    sei.hwnd = purlici->hwndParent;
                                    sei.lpFile = pszURL;
                                    sei.lpDirectory = pszT;
                                    sei.nShow = nShowCmd ? nShowCmd : SW_NORMAL;
                                    sei.lpClass = pszProtocol;
       
                                    // We have to special case "file:" URLs,
                                    // because Nashville's Explorer typically handles 
                                    // file: URLs via DDE, which fails for executables
                                    // (eg, "file://c:\windows\notepad.exe") and
                                    // non-hostable docs (like text files).
                                    //
                                    // So in this case, we remove the protocol class
                                    // and execute the suffix.

                                    // App Compat: Don't use up stack space.
                                    DWORD cchPath = MAX_PATH;
                                    LPWSTR  pszPath = (LPWSTR)LocalAlloc(LPTR, cchPath * sizeof(WCHAR));

                                    if (pszPath)
                                    {
                                        if (IsFlagSet(sei.fMask, SEE_MASK_CLASSNAME) &&
                                            (URL_SCHEME_FILE == pu.nScheme) &&
                                            SUCCEEDED(PathCreateFromUrl(pszURL, pszPath, &cchPath, 0)))
                                        {
                                            sei.lpClass = NULL;
                                            ClearFlag(sei.fMask, SEE_MASK_CLASSNAME);
                                            sei.lpFile = pszPath;
                                        }

                                        TraceMsg(TF_INTSHCUT, "Intshcut::InvokeCommand(): Invoking %s verb on URL %s.",
                                                   sei.lpVerb ? sei.lpVerb : TEXT("open"),
                                                   sei.lpFile);
       
                                        hr = ShellExecuteEx(&sei) ? S_OK : IS_E_EXEC_FAILED;

                                        LocalFree(pszPath);
                                    }
                                    else
                                    {
                                        hr = E_OUTOFMEMORY;
                                    }
                                }
                    
                                if (hr != S_OK)
                                    TraceMsg(TF_WARNING, "Intshcut::InvokeCommand(): ShellExecuteEx() via registered protcol handler failed for %s.",
                                             pszURL);

                                LocalFree(pszVerb);
                            }
                            else
                            {
                                hr = E_OUTOFMEMORY;
                            }
   
                            break;
                        }
   
                        case S_FALSE:
                            hr = MyExecute(pszT, pszURL, 0);
                            switch (hr)
                            {
                                case E_FAIL:
                                    bExecFailedWhine = TRUE;
                                    hr = IS_E_EXEC_FAILED;
                                    break;
   
                                default:
                                    break;
                            }
                            break;
   
                        default:
                            ASSERT(FAILED(hr));
                            break;
                    }

        done:
                    if (pszProtocol)
                        LocalFree(pszProtocol);
                
                    if (pidl)
                        ILFree(pidl);
                    
                    if (FAILED(hr) &&
                        IsFlagSet(purlici->dwFlags, IURL_INVOKECOMMAND_FL_ALLOW_UI))
                    {
                        switch (hr)
                        {
                            case IS_E_EXEC_FAILED:
                                if (bExecFailedWhine)
                                {
                                    ASSERT(IS_VALID_STRING_PTR(pszT, -1));

                                    MLShellMessageBox(
                                                    purlici->hwndParent,
                                                    MAKEINTRESOURCE(IDS_IS_EXEC_FAILED),
                                                    MAKEINTRESOURCE(IDS_SHORTCUT_ERROR_TITLE),
                                                    (MB_OK | MB_ICONEXCLAMATION),
                                                    pszT);
                                }
                                break;
            
                            case URL_E_INVALID_SYNTAX:
                                MLShellMessageBox(
                                                purlici->hwndParent,
                                                MAKEINTRESOURCE(IDS_IS_EXEC_INVALID_SYNTAX),
                                                MAKEINTRESOURCE(IDS_SHORTCUT_ERROR_TITLE),
                                                (MB_OK | MB_ICONEXCLAMATION),
                                                pszURL);
            
                                break;
            
                            case URL_E_UNREGISTERED_PROTOCOL:
                            {
                                LPTSTR pszProtocol;
            
                                if (CopyURLProtocol(pszURL, &pszProtocol, NULL) == S_OK)
                                {
                                    MLShellMessageBox(
                                                    purlici->hwndParent,
                                                    MAKEINTRESOURCE(IDS_IS_EXEC_UNREGISTERED_PROTOCOL),
                                                    MAKEINTRESOURCE(IDS_SHORTCUT_ERROR_TITLE),
                                                    (MB_OK | MB_ICONEXCLAMATION),
                                                    pszProtocol);
            
                                    LocalFree(pszProtocol);
                                }
            
                                break;
                            }
            
                            case E_OUTOFMEMORY:
                                MLShellMessageBox(
                                                purlici->hwndParent,
                                                MAKEINTRESOURCE(IDS_IS_EXEC_OUT_OF_MEMORY),
                                                MAKEINTRESOURCE(IDS_SHORTCUT_ERROR_TITLE),
                                                (MB_OK | MB_ICONEXCLAMATION));
                                break;
            
                            default:
                                ASSERT(hr == E_ABORT);
                                break;
                        }
                    }

                    LocalFree(pszT);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }

           LocalFree(pszURL);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    ASSERT(hr == S_OK ||
           hr == E_ABORT ||
           hr == E_OUTOFMEMORY ||
           hr == URL_E_INVALID_SYNTAX ||
           hr == URL_E_UNREGISTERED_PROTOCOL ||
           hr == IS_E_EXEC_FAILED ||
           hr == E_INVALIDARG);
    
    return(hr);
}



/*----------------------------------------------------------
Purpose: IUniformResourceLocatorA::InvokeCommand for Intshcut

         Ansi version

*/
STDMETHODIMP
Intshcut::InvokeCommand(
    IN PURLINVOKECOMMANDINFOA purlici)

{
    HRESULT hres = E_INVALIDARG;

    ASSERT(purlici);
    ASSERT(SIZEOF(*purlici) == purlici->dwcbSize);

    if (SIZEOF(*purlici) == purlici->dwcbSize)
    {
        URLINVOKECOMMANDINFOW ici;

        ici.dwcbSize = SIZEOF(ici);
        ici.dwFlags  = purlici->dwFlags;
        ici.hwndParent = purlici->hwndParent;

        ici.pcszVerb = NULL;

        if (purlici->pcszVerb)
        {
            //
            // App compat hack.
            //
            // Note: use local alloc here instead of the stack since 16-bit code
            // can shell exec urls and we don't want to use up their stack.
            //

            int cch = lstrlenA(purlici->pcszVerb) + 1;

            ici.pcszVerb = (LPWSTR)LocalAlloc(LPTR, cch * sizeof(WCHAR));

            if (ici.pcszVerb)
            {
                AnsiToUnicode(purlici->pcszVerb, (LPWSTR)ici.pcszVerb, cch);
            }
        }

        hres = InvokeCommand(&ici);

        if (ici.pcszVerb)
            LocalFree((void*)ici.pcszVerb);
    }

    return hres;
}



STDMETHODIMP Intshcut::Create(REFFMTID fmtid, const CLSID *pclsid,
                              DWORD grfFlags, DWORD grfMode, IPropertyStorage **pppropstg)
{
    *pppropstg = NULL;
    return E_NOTIMPL;
}


STDMETHODIMP Intshcut::Open(REFFMTID fmtid, DWORD grfMode, IPropertyStorage **pppropstg)
{
    HRESULT hres = E_FAIL;      // assume failure

    *pppropstg = NULL;

    if (IsEqualGUID(fmtid, FMTID_Intshcut))
    {
        // Create a URLProp object for this format ID
        hres = CIntshcutProp_CreateInstance(NULL, IID_IPropertyStorage, (void **)pppropstg);
        if (SUCCEEDED(hres))
        {
            // Initialize this object
            IntshcutProp * pisprop = (IntshcutProp *)*pppropstg;
            hres = pisprop->InitFromFile(m_pszFile);
        }
    }
    else if (IsEqualGUID(fmtid, FMTID_InternetSite))
    {
        // Create a URLProp object for this format ID
        hres = CIntsiteProp_CreateInstance(NULL, IID_IPropertyStorage, (void **)pppropstg);
        if (SUCCEEDED(hres))
        {
            hres = InitProp();
            if (SUCCEEDED(hres))
            {
                TCHAR szURL[MAX_URL_STRING];
                hres = m_pprop->GetProp(PID_IS_URL, szURL, SIZECHARS(szURL));
                if (SUCCEEDED(hres))
                {
                    IntsiteProp * pisprop = (IntsiteProp *)*pppropstg;
                    hres = pisprop->InitFromDB(szURL, this, FALSE);
                }
            }

            if (FAILED(hres))
            {
                (*pppropstg)->Release();
                *pppropstg = NULL;
            }
        }
    }

    return hres;
}


STDMETHODIMP Intshcut::Delete(REFFMTID fmtid)
{
    return STG_E_ACCESSDENIED;
}


STDMETHODIMP Intshcut::Enum(OUT IEnumSTATPROPSETSTG ** ppenum)
{
    *ppenum = NULL;
    return E_NOTIMPL;
}

// get the required propery that indicates the item has been changed

STDAPI GetRecentlyChanged(IPropertyStorage *ppropstg, PROPID propid, LPTSTR pszBuf, DWORD cchBuf)
{
    PROPVARIANT propvar;
    HRESULT hres = E_FAIL;  // assume not there
    PROPSPEC prspec = { PRSPEC_PROPID, propid };

    if (S_OK == ppropstg->ReadMultiple(1, &prspec, &propvar))
    {
        if ((VT_UI4 == propvar.vt) && (PIDISF_RECENTLYCHANGED & propvar.lVal))
            hres = S_FALSE;     // we've got it, skip this property

        PropVariantClear(&propvar);
    }
    return hres;
}

STDAPI GetStringPropURL(IPropertyStorage *ppropstg, PROPID propid, LPTSTR pszBuf, DWORD cchBuf)
{
    HRESULT hres = GetStringProp(ppropstg, propid, pszBuf, cchBuf);
    if (SUCCEEDED(hres))
    {
        // get rid of the query string for display
        if (UrlIs(pszBuf, URLIS_HASQUERY))
            UrlCombine(pszBuf, TEXT("?..."), pszBuf, &cchBuf, 0);
    }
    return hres;
}

BOOL Intshcut::_TryLink(REFIID riid, void **ppvOut)
{
    HRESULT hr = InitProp();

    if (SUCCEEDED(hr) && URL_SCHEME_FILE == GetScheme())
    {
        // This shortcut is not in the favorites folder as far as we know 
        TCHAR szURL[INTERNET_MAX_URL_LENGTH];
        DWORD cch = SIZECHARS(szURL);

        *szURL = 0;

        m_pprop->GetProp(PID_IS_URL, szURL, SIZECHARS(szURL));

        if (*szURL && SUCCEEDED(PathCreateFromUrl(szURL, szURL, &cch, 0)))
        {
            if (!_punkLink)
            {
                hr = _CreateShellLink(szURL, &_punkLink);
            }

            if (_punkLink)
            {
                if (SUCCEEDED(_punkLink->QueryInterface(riid, ppvOut)))
                    return TRUE;
            }
        }

        if (FAILED(hr))
            ATOMICRELEASE(_punkLink);
    }

    return FALSE;
}

STDMETHODIMP Intshcut::GetInfoTip(DWORD dwFlags, WCHAR **ppwszTip)
{
    HRESULT hr = E_FAIL;
    IQueryInfo *pqi;

    if (_TryLink(IID_IQueryInfo, (void **)&pqi))
    {
        hr = pqi->GetInfoTip(dwFlags, ppwszTip);
        pqi->Release();
    }
    
    if (FAILED(hr))
    {
        static const ITEM_PROP c_rgTitleAndURL[] = {
            { &FMTID_InternetSite, PID_INTSITE_TITLE,   GetStringProp, IDS_FAV_STRING },
            { &FMTID_Intshcut, PID_IS_URL,              GetStringPropURL, IDS_FAV_STRING },
            { NULL, 0, 0, 0 },
        };

        hr = GetInfoTipFromStorage(SAFECAST(this, IPropertySetStorage *), c_rgTitleAndURL, ppwszTip);
    }

    return hr;

}

STDMETHODIMP Intshcut::GetInfoFlags(DWORD *pdwFlags)
{
    *pdwFlags = 0;
#if 0    
// This Function is commented out since it has not been tested.
// It can be uncommented if we provide support for providing offline cursor
// for shortucts. I think this needs updates to listview in comctl -- BharatS
        
    LPSTR pszURL;
    if (S_OK == GetURL(&pszURL))
    {
        BOOL fCached = UrlIsCached(pszUrl);
        if (!fCached)
        {
            CHAR szCanonicalizedUrlA[MAX_URL_STRING];
            DWORD dwLen = ARRAYSIZE(szCanonicalizedUrlA);
            InternetCanonicalizeUrlA(pszURL, szCanonicalizedUrlA, &dwLen, 0);
            fCached = UrlIsMappedOrInCache(szCanonicalizedUrlA);
        }
        if (fCached)
            *pdwFlags |= QIF_CACHED;
        SHFree(pszURL);
    }
    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

/*----------------------------------------------------------
IQueryCodePage:
*/
STDMETHODIMP Intshcut::GetCodePage(UINT * puiCodePage)
{
    HRESULT hres = E_FAIL;
    *puiCodePage = 0;     // NULL out the code page. 
    if (IsFlagSet(m_dwFlags, ISF_CODEPAGE))
    {
        *puiCodePage = m_uiCodePage;
        hres = S_OK;
    }

    return hres;
}

STDMETHODIMP Intshcut::SetCodePage(UINT uiCodePage)
{
    SetFlag(m_dwFlags, ISF_CODEPAGE);
    m_uiCodePage = uiCodePage;
    return S_OK;
}

/***************************** Exported Functions ****************************/


// This function was ported from URL.DLL.  Normally, since our
// internet shortcut object has a context menu handler, we don't
// call this function.
//
// Only one thing needs this entry point: Exchange.  Imagine the
// following phrase pasted a million times:
//
//      Exchange is stupid!
//
// Instead of simply calling ShellExecuteEx to handle opening file
// attachments, they grovel thru the registry themselves. Of course,
// their code is incomplete and thinks a file-association needs to
// have an explicit \shell\open\command that works before it executes
// it.  Hmm, it brings to mind a phrase, like:
//
//      Exchange is stupid!
//
// So, we export this API so they will work.  But really the invoke
// occurs in the context menu handler for normal cases.
//


STDAPI_(void) OpenURL(HWND hwndParent, HINSTANCE hinst, LPSTR pszCmdLine, int nShowCmd)
{
   HRESULT hr;
   HRESULT hrCoInit;

   

   Intshcut * pIntshcut = new Intshcut;     // This must be a 0 INITed memory allocation
   WCHAR wszPath[MAX_PATH];

    if (!pIntshcut)
        return;

   hrCoInit = SHCoInitialize(); // gets called from rundll32 in browser only mode - hence we need to
                                // make sure that OLE has been init'ed

 

   ASSERT(IS_VALID_HANDLE(hwndParent, WND));
   ASSERT(IS_VALID_HANDLE(hinst, INSTANCE));
   ASSERT(IS_VALID_STRING_PTRA(pszCmdLine, -1));
   ASSERT(IsValidShowCmd(nShowCmd));

   // Assume the entire command line is an Internet Shortcut file path.

   TrimWhiteSpaceA(pszCmdLine);

   TraceMsgA(TF_INTSHCUT, "OpenURL(): Trying to open Internet Shortcut %s.",
              pszCmdLine);

#ifndef UNIX

   AnsiToUnicode(pszCmdLine, wszPath, SIZECHARS(wszPath));
   hr = pIntshcut->LoadFromFile(wszPath);

#else /* UNIX */

#ifndef ANSI_SHELL32_ON_UNIX
   // IEUNIX : Our Shell32 calls this function with unicode command line
   hr = pIntshcut->LoadFromFile((LPWSTR)pszCmdLine);
#else
   hr = pIntshcut->LoadFromFile(pszCmdLine);
#endif

#endif /* !UNIX */

   if (hr == S_OK)
   {
      URLINVOKECOMMANDINFO urlici;

      urlici.dwcbSize = SIZEOF(urlici);
      urlici.hwndParent = hwndParent;
      urlici.pcszVerb = NULL;
      urlici.dwFlags = (IURL_INVOKECOMMAND_FL_ALLOW_UI |
                        IURL_INVOKECOMMAND_FL_USE_DEFAULT_VERB);

      hr = pIntshcut->InvokeCommand(&urlici);
   }

   if (hr != S_OK)
   {
      MLShellMessageBox(
                      hwndParent,
                      MAKEINTRESOURCE(IDS_IS_LOADFROMFILE_FAILED),
                      MAKEINTRESOURCE(IDS_SHORTCUT_ERROR_TITLE),
                      (MB_OK | MB_ICONEXCLAMATION),
                      wszPath);
   }

   pIntshcut->Release();

   SHCoUninitialize(hrCoInit);

}

// INamedPropertyBag Methods
//
// Reads & writes properties from a section in the shortcut ini file


const TCHAR  c_szSizeSuffix[] = TEXT("__Size");


STDMETHODIMP Intshcut::WritePropertyNPB(
                                       LPCOLESTR pszSectionNameW, 
                            /* [in] */ LPCOLESTR pszPropNameW, 
                       /* [out][in] */ PROPVARIANT  *pVar)
{
    const TCHAR *pszSectionName;
    const TCHAR *pszPropName;
    HRESULT hr;
    if((NULL == pszSectionNameW) || (NULL == pszPropNameW) || (NULL == pVar))
    {
        return E_FAIL;
    }


    if(S_OK != _CreateTemporaryBackingFile())
    {
        ASSERT(NULL == m_pszTempFileName);
        return E_FAIL;
    }


    ASSERT(m_pszTempFileName);
    
    pszSectionName = pszSectionNameW;
    pszPropName = pszPropNameW;
    // Write the appropriate value in depending on the type

    switch(pVar->vt)
    {
        // NOTE: (andrewgu) these types we also can round-trip using the same code pass as for
        // unsigned types, except bharats in a codereview recommended we comment these out because
        // they'll look goofy in the *.ini file (you wrote -5 but see 4294967290 junk instead).
        // VT_UINT is not listed as "may appear in an OLE property set" in <wtypes.h>.
     /* case VT_I1:
        case VT_I2:
        case VT_I4:
        case VT_INT:
        case VT_UINT: */

        case VT_UI1:
        case VT_UI2:
        case VT_UI4:
            hr = WriteUnsignedToFile(m_pszTempFileName, pszSectionName, pszPropName, pVar->ulVal);
            break;

        case VT_BSTR:
            hr = WriteGenericString(m_pszTempFileName, pszSectionName, pszPropName, pVar->bstrVal);
            break;

        case VT_BLOB:
            {
                TCHAR *pszSizePropName = NULL;
                int  cchPropName = lstrlen(pszPropName) + ARRAYSIZE(c_szSizeSuffix) + 1;
                DWORD dwAllocSize = cchPropName * sizeof(TCHAR);
                
                pszSizePropName = (TCHAR *)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, dwAllocSize);
                if(pszSizePropName)
                {
                    DWORD dwBufferSize;
                    StrCpyN(pszSizePropName, pszPropName, cchPropName);
                    StrCatBuff(pszSizePropName, c_szSizeSuffix, cchPropName);

                    // OK Now - we have the name for the size
                    // we write it out

                    dwBufferSize = pVar->blob.cbSize;
                    hr = WriteBinaryToFile(m_pszTempFileName, pszSectionName, pszSizePropName, 
                                                (LPVOID)(&dwBufferSize), sizeof(DWORD));

                    if(S_OK == hr)
                    {
                        // Write out the buffer
                        hr = WriteBinaryToFile(m_pszTempFileName, pszSectionName, pszPropName, 
                                                (LPVOID)(pVar->blob.pBlobData), dwBufferSize);
                    }

                    LocalFree((LPVOID)pszSizePropName);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
                
                break;
            }
        default:
            hr = WriteBinaryToFile(m_pszTempFileName, pszSectionName, pszPropName, (LPVOID)pVar, sizeof(PROPVARIANT));
            break;
    }

   
    return hr;
}

STDMETHODIMP Intshcut::ReadPropertyNPB(
                       /* [in] */ LPCOLESTR pszSectionNameW,
                       /* [in] */ LPCOLESTR pszPropNameW,
                       /* [out][in] */ PROPVARIANT  *pVar)
{
    const TCHAR *pszSectionName;
    const TCHAR *pszPropName;
    TCHAR       *pszFileToReadFrom;
    HRESULT hr;

    if((NULL == pszSectionNameW) || (NULL == pszPropNameW) || (NULL == pVar))
    {
        if (NULL != pVar)
            pVar->vt = VT_ERROR;

        return E_FAIL;
    }


    if(m_pszTempFileName)
    {
        pszFileToReadFrom = m_pszTempFileName;
    } 
    else if(m_pszFile)
    {
        pszFileToReadFrom = m_pszFile;
    }
    else
    {
        pVar->vt = VT_EMPTY;
        return S_FALSE;
    }

    pszSectionName = pszSectionNameW;
    pszPropName = pszPropNameW;

    switch(pVar->vt)
    {
        // NOTE: (andrewgu) these types we also can round-trip using the same code pass as for
        // unsigned types, except bharats in a codereview recommended we comment these out because
        // they'll look goofy in the *.ini file (you wrote -5 but see 4294967290 junk instead).
        // VT_UINT is not listed as "may appear in an OLE property set" in <wtypes.h>.
     /* case VT_I1:
        case VT_I2:
        case VT_I4:
        case VT_INT:
        case VT_UINT: */

        case VT_UI1:
        case VT_UI2:
        case VT_UI4:
            pVar->ulVal = 0;
            hr          = ReadUnsignedFromFile(pszFileToReadFrom, pszSectionName, pszPropName, &(pVar->ulVal));
            break;

        case VT_BSTR:   
             // It is a string
           pVar->vt = VT_BSTR;
           pVar->bstrVal = NULL;
           hr = ReadBStrFromFile(pszFileToReadFrom, pszSectionName, pszPropName, &(pVar->bstrVal));            
           break;

        case VT_BLOB:
            {
                TCHAR *pszSizePropName = NULL;
                int  cchPropName = lstrlen(pszPropName) + ARRAYSIZE(c_szSizeSuffix) + 1;
                DWORD dwAllocSize = cchPropName * sizeof(TCHAR);
                
                pszSizePropName = (TCHAR *)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, dwAllocSize);
                if(pszSizePropName)
                {
                    DWORD dwBufferSize;
                    StrCpyN(pszSizePropName, pszPropName, cchPropName);
                    StrCatBuff(pszSizePropName, c_szSizeSuffix, cchPropName);
                    // Read the Size first
                    hr = ReadBinaryFromFile(pszFileToReadFrom, pszSectionName, pszSizePropName, 
                                            &dwBufferSize, sizeof(DWORD));
                    if(S_OK == hr)
                    {
                        
                        pVar->blob.pBlobData = (unsigned char *)CoTaskMemAlloc(dwBufferSize);
                        if(pVar->blob.pBlobData)
                        {
                            hr = ReadBinaryFromFile(pszFileToReadFrom, pszSectionName, pszPropName, 
                                            pVar->blob.pBlobData, dwBufferSize);

                            if(S_OK == hr)
                            {
                                pVar->blob.cbSize = dwBufferSize;
                            }
                            else
                            {
                                CoTaskMemFree(pVar->blob.pBlobData);
                            }
                        }
                    }
                    LocalFree(pszSizePropName);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }

               break;
            }
        default:
            {
                // all else
                PROPVARIANT tmpPropvar = {0};
                
                hr = ReadBinaryFromFile(pszFileToReadFrom, pszSectionName, pszPropName, &tmpPropvar, sizeof(PROPVARIANT));
                if((S_OK == hr) && (tmpPropvar.vt == pVar->vt))
                {
                    memcpy(pVar, &tmpPropvar, sizeof(PROPVARIANT));
                }
                else
                {
                    pVar->vt = VT_ERROR;
                }
                break;
            }

    }

   if(hr != S_OK)
   {
        memset(pVar, 0, sizeof(PROPVARIANT));
        pVar->vt = VT_EMPTY;
   }   

   return hr;
}

STDMETHODIMP Intshcut::RemovePropertyNPB (
                            /* [in] */ LPCOLESTR pszSectionNameW,
                            /* [in] */ LPCOLESTR pszPropNameW)
{
    const TCHAR *pszSectionName;
    const TCHAR *pszPropName;
    HRESULT hr;
    TCHAR *pszFileToDeleteFrom;

    // Return if there is no file name
    if((NULL == pszSectionNameW) || (NULL == pszPropNameW)) 
    {
        return E_FAIL;
    }

     if(m_pszTempFileName)
     {
        pszFileToDeleteFrom = m_pszTempFileName;
     }
     else if(m_pszFile)
     {
        pszFileToDeleteFrom = m_pszFile;
     }
     else
     {
        return E_FAIL;
     }
     
    
    // Just delete the key corresponding to this property name
    pszSectionName = pszSectionNameW;
    pszPropName = pszPropNameW;

    hr = SHDeleteIniString(pszSectionName, pszPropName, pszFileToDeleteFrom)? S_OK : E_FAIL;

    return hr;
}
