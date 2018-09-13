#ifndef __cplusplus
#error Install stub code must be C++!
#endif

#ifndef HINST_THISDLL
#error HINST_THISDLL must be defined!
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)    (sizeof(a)/sizeof((a)[0]))
#endif

#include <ccstock.h>
#include <stubres.h>
#include <trayp.h>

BOOL CheckWebViewShell();

/* This code runs the install/uninstall stubs recorded in the local-machine
 * part of the registry, iff the current user has not had them run in his
 * context yet.  Used for populating the user's profile with things like
 * links to applications.
 */
//---------------------------------------------------------------------------


BOOL ProfilesEnabled(void)
{
    BOOL fEnabled = FALSE;

    if (staticIsOS(OS_NT)) {
        fEnabled = TRUE;
    }
    else {
        HKEY hkeyLogon;
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Network\\Logon"), 0,
                         KEY_QUERY_VALUE, &hkeyLogon) == ERROR_SUCCESS) {
            DWORD fProfiles, cbData = sizeof(fProfiles), dwType;
            if (RegQueryValueEx(hkeyLogon, TEXT("UserProfiles"), NULL, &dwType,
                                (LPBYTE)&fProfiles, &cbData) == ERROR_SUCCESS) {
                if (dwType == REG_DWORD || (dwType == REG_BINARY && cbData == sizeof(DWORD)))
                    fEnabled = fProfiles;
            }
            RegCloseKey(hkeyLogon);
        }
    }
    return fEnabled;
}


DWORD MsgWaitForMultipleObjectsLoop(HANDLE hEvent, DWORD dwTimeout)
{
    MSG msg;
    DWORD dwObject;
    // DebugMsg(DM_TRACE, "c.t_rd: Waiting for run dlg...");
    while (1)
    {
        dwObject = MsgWaitForMultipleObjects(1, &hEvent, FALSE, dwTimeout, QS_ALLINPUT);
        // Are we done waiting?
        switch (dwObject) {
        case WAIT_OBJECT_0:
        case WAIT_FAILED:
            return dwObject;

        case WAIT_OBJECT_0 + 1:
            // Almost.
            // DebugMsg(DM_TRACE, "c.t_rd: Almost done waiting.");
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            break;
        }
    }
    // never gets here
    // return dwObject;
}

// The following handles running an application and optionally waiting for it
// to terminate.
void ShellExecuteRegApp(LPTSTR szCmdLine, UINT fFlags)
{
    TCHAR szQuotedCmdLine[MAX_PATH+2];    
    SHELLEXECUTEINFO ei;
    LPTSTR lpszArgs;

    //
    // We used to call CreateProcess( NULL, szCmdLine, ...) here,
    // but thats not useful for people with apppaths stuff.
    //
    // Don't let empty strings through, they will endup doing something dumb
    // like opening a command prompt or the like
    if (!szCmdLine || !*szCmdLine)
        return;


    // Gross, but if the process command fails, copy the command line to let
    // shell execute report the errors
    if (PathProcessCommand(szCmdLine, szQuotedCmdLine, ARRAYSIZE(szQuotedCmdLine),
                           PPCF_ADDARGUMENTS|PPCF_FORCEQUALIFY) == -1)
        lstrcpyn(szQuotedCmdLine, szCmdLine, ARRAYSIZE(szQuotedCmdLine) - 2);
    
    lpszArgs = PathGetArgs(szQuotedCmdLine);
    if (*lpszArgs)
        *(lpszArgs-1) = TEXT('\0'); // Strip args

    ei.lpFile          = szQuotedCmdLine;
    int cch = lstrlen(ei.lpFile);

    // Are the first and last chars quotes?
    if (szQuotedCmdLine[0] == TEXT('"') && szQuotedCmdLine[cch-1] == TEXT('"'))
    {
        // Yep, remove them.
        szQuotedCmdLine[cch-1] = TEXT('\0');
        ei.lpFile++;
    }

    ei.cbSize          = sizeof(SHELLEXECUTEINFO);
    ei.hwnd            = NULL;
    ei.lpVerb          = NULL;
    ei.lpParameters    = lpszArgs;
    ei.lpDirectory     = NULL;
    ei.nShow           = SW_SHOWNORMAL;
    ei.fMask           = (fFlags & RRA_NOUI)? 
            (SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI) :  SEE_MASK_NOCLOSEPROCESS;

    if (ShellExecuteEx(&ei))
    {
        if ((fFlags & RRA_WAIT) && ei.hProcess != NULL)
        {
            MsgWaitForMultipleObjectsLoop(ei.hProcess, INFINITE);
        }
        CloseHandle(ei.hProcess);
    }
}

// ---------------------------------------------------------------------------
// %%Function: GetVersionFromString
//
//  Snarfed from urlmon\download\helpers.cxx.
//
//    converts version in text format (a,b,c,d) into two dwords (a,b), (c,d)
//    The printed version number is of format a.b.d (but, we don't care)
// ---------------------------------------------------------------------------
HRESULT
GetVersionFromString(LPCTSTR szBuf, LPDWORD pdwFileVersionMS, LPDWORD pdwFileVersionLS)
{
    LPCTSTR pch = szBuf;
    TCHAR ch;
    USHORT n = 0;

    USHORT a = 0;
    USHORT b = 0;
    USHORT c = 0;
    USHORT d = 0;

    enum HAVE { HAVE_NONE, HAVE_A, HAVE_B, HAVE_C, HAVE_D } have = HAVE_NONE;


    *pdwFileVersionMS = 0;
    *pdwFileVersionLS = 0;

    if (!pch)            // default to zero if none provided
        return S_OK;

    if (lstrcmp(pch, TEXT("-1,-1,-1,-1")) == 0) {
        *pdwFileVersionMS = 0xffffffff;
        *pdwFileVersionLS = 0xffffffff;
    }


    for (ch = *pch++;;ch = *pch++) {

        if ((ch == ',') || (ch == '\0')) {

            switch (have) {

            case HAVE_NONE:
                a = n;
                have = HAVE_A;
                break;

            case HAVE_A:
                b = n;
                have = HAVE_B;
                break;

            case HAVE_B:
                c = n;
                have = HAVE_C;
                break;

            case HAVE_C:
                d = n;
                have = HAVE_D;
                break;

            case HAVE_D:
                return E_INVALIDARG; // invalid arg
            }

            if (ch == '\0') {
                // all done convert a,b,c,d into two dwords of version

                *pdwFileVersionMS = ((a << 16)|b);
                *pdwFileVersionLS = ((c << 16)|d);

                return S_OK;
            }

            n = 0; // reset

        } else if ( (ch < '0') || (ch > '9'))
            return E_INVALIDARG;    // invalid arg
        else
            n = n*10 + (ch - '0');


    } /* end forever */

    // NEVERREACHED
}


// Reg keys and values for install/uninstall stub list.  Each subkey under
// HKLM\Software\InstalledComponents is a component identifier (GUID).
// Each subkey has values "Path" for the EXE to run to install or uninstall;
// IsInstalled (dword) indicating whether the component has been installed
// or uninstalled;  and an optional Revision (dword) used to refresh a
// component without changing its GUID.  Locale (string) is used to describe
// the language/locale for the component;  this string is not interpreted by
// the install stub code, it is just compared between the HKLM and HKCU keys.
// If it's different between the two, the stub is re-run.
//
// HKCU\Software\InstalledComponents contains similar GUID subkeys, but the
// only values under each subkey are the optional Revision and Locale values,
// and an optional DontAsk value (also DWORD).  Presence of the subkey indicates
// that the component is installed for that user.
//
// If the DontAsk value is present under an HKCU subkey and is non-zero, that
// means that the user has decided to keep their settings for that component
// on all machines, even those that have had the component uninstalled, and
// that they don't want to be asked if they want to run the uninstall stub
// every time they log on.  This implies that for that user, the uninstall
// stub will never be run for that component unless the user somehow clears
// the flag.
//
// NOTE: mslocusr.dll also knows these registry paths.

const TCHAR c_szRegInstalledComponentsKey[] = TEXT("Software\\Microsoft\\Active Setup\\Installed Components");
const TCHAR c_szRegInstallStubValue[] = TEXT("StubPath");
const TCHAR c_szRegIsInstalledValue[] = TEXT("IsInstalled");
const TCHAR c_szRegInstallSequenceValue[] = TEXT("Version");
const TCHAR c_szRegDontAskValue[] = TEXT("DontAsk");
const TCHAR c_szRegLocaleValue[] = TEXT("Locale");


UINT ConfirmUninstall(LPCTSTR pszDescription)
{
    /* The only case where the user wouldn't want settings cleaned up on
     * uninstall would be if they'd roamed to a machine that had had this
     * component uninstalled.  If user profiles aren't enabled (which is
     * the case on a fair number of customers' machines), they're certainly
     * not roaming, so there's not much point in asking them.  Just pretend
     * they said YES, they want to clean up the settings.
     */
    if (!ProfilesEnabled())
        return IDYES;

    /* BUGBUG - change to a dialog with a checkbox for
     * the don't-ask value.
     */

    TCHAR szTitle[MAX_PATH];
#ifdef USERSTUB
    LoadString(HINST_THISDLL, IDS_DESKTOP, szTitle, ARRAYSIZE(szTitle));
#else
    MLLoadString(IDS_DESKTOP, szTitle, ARRAYSIZE(szTitle));
#endif

    TCHAR szMessageTemplate[MAX_PATH];
    LPTSTR pszMessage = NULL;
    int   cchMessage;

#ifdef USERSTUB
    LoadString(HINST_THISDLL, IDS_UNINSTALL, szMessageTemplate, ARRAYSIZE(szMessageTemplate));
#else
    MLLoadString(IDS_UNINSTALL, szMessageTemplate, ARRAYSIZE(szMessageTemplate));
#endif


    cchMessage = (lstrlen(szMessageTemplate)+lstrlen(pszDescription)+4)*sizeof(TCHAR);
    pszMessage = (LPTSTR)LocalAlloc(LPTR, cchMessage);
    if (pszMessage)
    {
#ifdef USERSTUB
        wsprintf(pszMessage, szMessageTemplate, pszDescription);
#else
        wnsprintf(pszMessage, cchMessage, szMessageTemplate, pszDescription);
#endif
    }
    else
    {
        pszMessage = szMessageTemplate;
    }

    // due to build in UNICODE the following call is broken under win95, user wsprintf above
    //if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING |
    //                   FORMAT_MESSAGE_ARGUMENT_ARRAY,
    //                   (LPVOID)szMessageTemplate,
    //                   0,
    //                   0,
    //                   (LPTSTR)&pszMessage,
    //                   0,        /* min chars to allocate */
    //                   (va_list *)&pszDescription)) {
    //    pszMessage = szMessageTemplate;
    //}


    UINT idRet = MessageBox(NULL, pszMessage, szTitle,
                            MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_SETFOREGROUND);

    if (pszMessage != szMessageTemplate)
        LocalFree(pszMessage);

    return idRet;
}


HWND hwndProgress = NULL;
BOOL fTriedProgressDialog = FALSE;

INT_PTR ProgressDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_INITDIALOG:
        return TRUE;

    case WM_SETCURSOR:
        SetCursor(LoadCursor(NULL, IDC_WAIT));
        return TRUE;


    default:
        return FALSE;
    }

    return TRUE;
}


void SetProgressInfo(HWND hwndProgress, LPCTSTR pszFriendlyName, BOOL fInstalling)
{
    HWND hwndInstalling = GetDlgItem(hwndProgress, IDC_RUNNING_INSTALL_STUB);
    HWND hwndUninstalling = GetDlgItem(hwndProgress, IDC_RUNNING_UNINSTALL_STUB);

    ShowWindow(hwndInstalling, fInstalling ? SW_SHOW : SW_HIDE);
    EnableWindow(hwndInstalling, fInstalling);
    ShowWindow(hwndUninstalling, fInstalling ? SW_HIDE : SW_SHOW);
    EnableWindow(hwndUninstalling, !fInstalling);
    SetDlgItemText(hwndProgress, IDC_INSTALL_STUB_NAME, pszFriendlyName);
}


void IndicateProgress(LPCTSTR pszFriendlyName, BOOL fInstalling)
{
    if (hwndProgress == NULL && !fTriedProgressDialog) {
        hwndProgress = CreateDialog(HINST_THISDLL, MAKEINTRESOURCE(IDD_InstallStubProgress),
                                    NULL, ProgressDialogProc);
    }

    if (hwndProgress != NULL) {
        SetProgressInfo(hwndProgress, pszFriendlyName, fInstalling);
        if (!fTriedProgressDialog) {
            ShowWindow(hwndProgress, SW_RESTORE);
            SetForegroundWindow(hwndProgress);
        }
    }
    fTriedProgressDialog = TRUE;
}


void CleanupProgressDialog(void)
{
    if (hwndProgress != NULL) {
        DestroyWindow(hwndProgress);
        hwndProgress = NULL;
    }
}


BOOL RunOneInstallStub( HKEY hklmList, HKEY hkcuList, LPCTSTR pszKeyName,
                        LPCTSTR pszCurrentUsername, int iPass )
{
     BOOL bNextPassNeeded = FALSE;
     /* See if this component is installed or an uninstall tombstone. */
     HKEY hkeyComponent;

     DWORD err = RegOpenKeyEx(hklmList, pszKeyName, 0, KEY_QUERY_VALUE,
                               &hkeyComponent);
     if (err == ERROR_SUCCESS) {
        TCHAR szCmdLine[MAX_PATH];
        DWORD fIsInstalled;
        DWORD dwType;
        DWORD cbData = sizeof(fIsInstalled);
        HKEY hkeyUser = NULL;

        /* Must have the stub path;  if not there, skip this entry. */
        cbData = sizeof(szCmdLine);
        if (SHQueryValueEx(hkeyComponent, c_szRegInstallStubValue,
                           NULL, &dwType, (LPBYTE)szCmdLine,
                           &cbData) != ERROR_SUCCESS || ((dwType != REG_SZ) && (dwType != REG_EXPAND_SZ)) ) {
            RegCloseKey(hkeyComponent);
            return bNextPassNeeded;
        }

        TCHAR szDescription[MAX_PATH];
        LPTSTR pszDescription = szDescription;
        cbData = sizeof(szDescription);
        if (SHQueryValueEx(hkeyComponent, TEXT(""),
                           NULL, &dwType, (LPBYTE)szDescription,
                           &cbData) != ERROR_SUCCESS || dwType != REG_SZ) {
            pszDescription = szCmdLine;
        }


        if (RegQueryValueEx(hkeyComponent, c_szRegIsInstalledValue,
                            NULL, &dwType, (LPBYTE)&fIsInstalled,
                            &cbData) != ERROR_SUCCESS ||
            (dwType != REG_DWORD && (dwType != REG_BINARY || cbData != sizeof(DWORD))))
            fIsInstalled = TRUE;

        /* If it's installed, check the user's profile, and if the
         * component (or its current revision) isn't installed there,
         * run it.
         */
        if (fIsInstalled) {
            DWORD dwRevisionHi, dwRevisionLo;
            DWORD dwUserRevisionHi = 0;
            DWORD dwUserRevisionLo = 0;
            BOOL fSetRevision;
            TCHAR szRevision[24], szUserRevision[24];   /* 65535,65535,65535,65535\0 */
            TCHAR szLocale[10], szUserLocale[10];       /* usually not very big strings */
            TCHAR szInstallUsername[128+1];  /* 128 is the win95 system username limit */

            DWORD fIsCloneUser;
            cbData = sizeof(fIsCloneUser);
            if (RegQueryValueEx(hkeyComponent, TEXT("CloneUser"),
                                NULL, &dwType, (LPBYTE)&fIsCloneUser,
                                &cbData) != ERROR_SUCCESS ||
                (dwType != REG_DWORD && (dwType != REG_BINARY || cbData != sizeof(DWORD))))
                fIsCloneUser = FALSE;

            cbData = sizeof(szRevision);
            if (RegQueryValueEx(hkeyComponent, c_szRegInstallSequenceValue,
                                NULL, &dwType, (LPBYTE)szRevision,
                                &cbData) != ERROR_SUCCESS ||
                dwType != REG_SZ ||
                FAILED(GetVersionFromString(szRevision, &dwRevisionHi, &dwRevisionLo))) {
                fSetRevision = FALSE;
                dwRevisionHi = 0;
                dwRevisionLo = 0;
            }
            else {
                fSetRevision = TRUE;
            }

            cbData = sizeof(szLocale);
            err = RegQueryValueEx(hkeyComponent, c_szRegLocaleValue,
                                  NULL, &dwType, (LPBYTE)szLocale,
                                  &cbData);
            if (err != ERROR_SUCCESS || dwType != REG_SZ) {
                szLocale[0] = '\0';
            }

            err = RegOpenKeyEx(hkcuList, pszKeyName, 0,
                               KEY_QUERY_VALUE | KEY_SET_VALUE, &hkeyUser);
            if (err == ERROR_SUCCESS) {
                cbData = sizeof(szUserRevision);
                if (RegQueryValueEx(hkeyUser, c_szRegInstallSequenceValue,
                                    NULL, &dwType, (LPBYTE)szUserRevision,
                                    &cbData) != ERROR_SUCCESS ||
                    dwType != REG_SZ ||
                    FAILED(GetVersionFromString(szUserRevision, &dwUserRevisionHi, &dwUserRevisionLo))) {
                    dwUserRevisionHi = 0;
                    dwUserRevisionLo = 0;
                }

                if (szLocale[0] != '\0') {
                    cbData = sizeof(szUserLocale);
                    err = RegQueryValueEx(hkeyUser, c_szRegLocaleValue,
                                          NULL, &dwType, (LPBYTE)szUserLocale,
                                          &cbData);
                    /* If there's a locale string under the user key
                     * and it's the same as the machine one, then we
                     * blank out the machine one so we won't consider
                     * that when running the stub.
                     */
                    if (err == ERROR_SUCCESS && dwType == REG_SZ &&
                        !lstrcmp(szLocale, szUserLocale)) {
                        szLocale[0] = '\0';
                    }
                }
                if (fIsCloneUser) {
                    /* Clone-user install stub.  We need to re-run it if the
                     * username we used when we last installed to this profile,
                     * or the one it was copied from, is different from the
                     * current username.
                     */
                    cbData = sizeof(szInstallUsername);
                    if (RegQueryValueEx(hkeyUser, TEXT("Username"),
                                        NULL, &dwType, (LPBYTE)szInstallUsername,
                                        &cbData) != ERROR_SUCCESS ||
                        dwType != REG_SZ) {
                        szInstallUsername[0] = '\0';
                    }
                }
            }
            else {
                hkeyUser = NULL;
            }

            /* Install if:
             *
             * - User doesn't have component installed, OR
             *   - Component installed on machine has a revision AND
             *   - Machine component revision greater than user's
             * - OR
             *   - Component installed on machine has a locale AND
             *   - Machine component locale different than user's
             *     (this is actually checked above)
             * - OR
             *   - Component is a clone-user install stub and the username
             *     recorded for the stub is different from the current username
             */
            if ((hkeyUser == NULL) ||
                (fSetRevision &&
                 ((dwRevisionHi > dwUserRevisionHi) ||
                  ((dwRevisionHi == dwUserRevisionHi) &&
                   (dwRevisionLo > dwUserRevisionLo)
                  )
                 )
                ) ||
                (szLocale[0] != '\0') ||
#ifdef UNICODE
                (fIsCloneUser && StrCmpI(szInstallUsername, pszCurrentUsername))
#else
                (fIsCloneUser && lstrcmpi(szInstallUsername, pszCurrentUsername))
#endif
                ) {

                if ( (iPass == -1 ) ||
                     ((iPass == 0) && (*pszKeyName == '<')) ||
                     ((iPass == 1) && (*pszKeyName != '<') && (*pszKeyName != '>')) ||
                     ((iPass == 2) && (*pszKeyName == '>')) )
                {
                    // the condition meets, run it now.
#ifdef TraceMsg
                    TraceMsg(TF_REGCHECK, "Running install stub ( %s )", szCmdLine);
#endif
                    IndicateProgress(pszDescription, TRUE);
                    ShellExecuteRegApp(szCmdLine, RRA_WAIT | RRA_NOUI);
                    if (hkeyUser == NULL) {
                        RegCreateKey(hkcuList, pszKeyName, &hkeyUser);
                    }
                    if (hkeyUser != NULL) {
                        if (fSetRevision) {
                            RegSetValueEx(hkeyUser, c_szRegInstallSequenceValue,
                                          0, REG_SZ,
                                          (LPBYTE)szRevision,
                                          (lstrlen(szRevision)+1)*sizeof(TCHAR));
                        }
                        if (szLocale[0]) {
                            RegSetValueEx(hkeyUser, c_szRegLocaleValue,
                                          0, REG_SZ,
                                          (LPBYTE)szLocale,
                                          (lstrlen(szLocale)+1)*sizeof(TCHAR));
                        }
                        if (fIsCloneUser) {
                            RegSetValueEx(hkeyUser, TEXT("Username"),
                                          0, REG_SZ,
                                          (LPBYTE)pszCurrentUsername,
                                          (lstrlen(pszCurrentUsername)+1)*sizeof(TCHAR));
                        }
                    }
                }
                else
                {
                    // decide if this belong to the next pass
                    // if it is in Pass 2, should never get here
                    if ( iPass == 0 )
                        bNextPassNeeded = TRUE;
                    else if ( (iPass == 1 ) && (*pszKeyName == '>') )
                        bNextPassNeeded = TRUE;
                }
            }
        }
        else {
            /* Component is an uninstall stub. */

            err = RegOpenKeyEx(hkcuList, pszKeyName, 0,
                               KEY_QUERY_VALUE, &hkeyUser);
            if (err == ERROR_SUCCESS) {
                DWORD fDontAsk = 0;

                /* Check the "Don't Ask" value.  If it's present, its value
                 * is interpreted as follows:
                 *
                 * 0 --> ask the user
                 * 1 --> do not run the stub
                 * 2 --> always run the stub
                 */
                cbData = sizeof(fDontAsk);
                if (RegQueryValueEx(hkeyComponent, c_szRegDontAskValue,
                                    NULL, &dwType, (LPBYTE)&fDontAsk,
                                    &cbData) != ERROR_SUCCESS ||
                    (dwType != REG_DWORD && (dwType != REG_BINARY || cbData != sizeof(DWORD))) ||
                    fDontAsk != 1) 
                {

                    if ( (iPass == -1 ) ||
                         ((iPass == 0) && (*pszKeyName == '>')) ||
                         ((iPass == 1) && (*pszKeyName != '<') && (*pszKeyName != '>')) ||
                         ((iPass == 2) && (*pszKeyName == '<')) )
                    {
                        // uninstall stub has the reversed order comparing with install stub
                        if (fDontAsk == 2 || ConfirmUninstall(pszDescription) == IDYES) {

#ifdef TraceMsg
                            TraceMsg(TF_REGCHECK, "Running uninstall stub ( %s )", szCmdLine);
#endif
                            IndicateProgress(pszDescription, FALSE);
                            ShellExecuteRegApp(szCmdLine, RRA_WAIT | RRA_NOUI);
    
                            /* Component has been uninstalled.  Forget that the
                             * user ever had it installed.
                             */
                            RegCloseKey(hkeyUser);
                            hkeyUser = NULL;
                            RegDeleteKey(hkcuList, pszKeyName);
                        }

                    }
                    else
                    {
                        // decide if this belong to the next pass
                        // if it is in Pass 2, should never get here
                        if ( iPass == 0 )
                            bNextPassNeeded = TRUE;
                        else if ( (iPass == 1 ) && (*pszKeyName == '<') )
                            bNextPassNeeded = TRUE;
                    }
                }
            }
        }

        if (hkeyUser != NULL) {
            RegCloseKey(hkeyUser);
        }
        RegCloseKey(hkeyComponent);
    }

    return bNextPassNeeded;
}


const TCHAR  c_szIE40GUID_STUB[] = TEXT("{89820200-ECBD-11cf-8B85-00AA005B4383}");
const TCHAR  c_szBlockIE4Stub[]  = TEXT("NoIE4StubProcessing");

extern "C" void RunInstallUninstallStubs2(LPCTSTR pszStubToRun)
{
    HKEY hklmList = NULL, hkcuList = NULL;
    LONG err;

    TCHAR szUsername[128+1];        /* 128 is the win95 limit, good default */
    LPTSTR pszCurrentUser = szUsername;

    /* As far as clone-user install stubs are concerned, we only want profile
     * usernames.
     */
    if (!ProfilesEnabled()) {
        *pszCurrentUser = '\0';
    }
    else {
        DWORD cbData = sizeof(szUsername);
        if (!GetUserName(szUsername, &cbData)) {
            if (cbData > sizeof(szUsername)) {
                cbData++;   /* allow for null char just in case */
                pszCurrentUser = (LPTSTR)LocalAlloc(LPTR, cbData+1);
                if (pszCurrentUser == NULL || !GetUserName(pszCurrentUser, &cbData)) {
                    if (pszCurrentUser != NULL)
                        LocalFree(pszCurrentUser);
                    pszCurrentUser = szUsername;
                    *pszCurrentUser = '\0';
                }
            }
            else {
                szUsername[0] = '\0';
            }
        }
    }

#ifdef TraceMsg
    TraceMsg(TF_REGCHECK, "Running install/uninstall stubs.");
#endif

    err = RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegInstalledComponentsKey, 0,
                       KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &hklmList);

    if (err == ERROR_SUCCESS) {
        DWORD dwDisp;
        err = RegCreateKeyEx(HKEY_CURRENT_USER, c_szRegInstalledComponentsKey, 0,
                             TEXT(""), REG_OPTION_NON_VOLATILE,
                             KEY_READ | KEY_WRITE, NULL, &hkcuList, &dwDisp);
    }

    if (err == ERROR_SUCCESS) {
        if (pszStubToRun != NULL) {
            // here we call with pass number -1 means no pass order enforced
            RunOneInstallStub(hklmList, hkcuList, pszStubToRun, pszCurrentUser, -1);
        }
        else {
            DWORD cbKeyName, iKey, iPass;
            TCHAR szKeyName[80];
            BOOL  bNextPassNeeded = TRUE;
            HANDLE hMutex;

            // This mutex check is to ensure if explore restarted due abnormal active desktop shutdown, and setup resume
            // per-user stubs should not be processed till setup is done.
            if (CheckWebViewShell())
            {
                hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, TEXT("Ie4Setup.Mutext"));
                if (hMutex)
                {
                    CloseHandle(hMutex);                
                    goto done;
                }
            }

	        // check if we want to block the stub processing
            cbKeyName = sizeof(szKeyName);
            if ((RegQueryValueEx(hklmList, c_szBlockIE4Stub, NULL, NULL, 
                                (LPBYTE)szKeyName, &cbKeyName) == ERROR_SUCCESS) && 
                (*CharUpper(szKeyName) == 'Y') )
	        {
                 goto done;
            }                       

            /* we will do TWO passes to meet the ordering requirement when run component stubs.
               Any KeyName with '*' as the first char, get run in the 1st Pass.  the rest run 2nd pass */
            for ( iPass = 0; ((iPass<3) && bNextPassNeeded); iPass++ )
            {
                bNextPassNeeded = FALSE;
                
                // BUGBUG: in 2nd pass, we do want to special case of IE4.0 base browser stub
                // to run first.  The reason we did not use '<' for this is to 1) reserve < stuff
                // for pre-ie4 stubs and this whole thing should redo in sorted fashion.  For now,
                // we hard code this IE4.0 base browser GUID
                if ( iPass == 1 )
                {
                    if ( RunOneInstallStub(hklmList, hkcuList, c_szIE40GUID_STUB, pszCurrentUser, iPass) )                    
                        bNextPassNeeded = TRUE;
                }                    

                /* Enumerate components that are installed on the local machine. */
                for (iKey = 0; ; iKey++)
                {
                    LONG lEnum;

                    cbKeyName = ARRAYSIZE(szKeyName);

                    // BUGBUG (Unicode, Davepl) I'm assuming that the data is UNICODE,
                    // but I'm not sure who put it there yet... double check.

                    if ((lEnum = RegEnumKey(hklmList, iKey, szKeyName, cbKeyName)) == ERROR_MORE_DATA)
                    {
                        // ERROR_MORE_DATA means the value name or data was too large
                        // skip to the next item
#ifdef TraceMsg
                        TraceMsg( DM_ERROR, "Cannot run oversize entry in InstalledComponents");
#endif
                        continue;
                    }
                    else if( lEnum != ERROR_SUCCESS )
                    {
                        // could be ERROR_NO_MORE_ENTRIES, or some kind of failure
                        // we can't recover from any other registry problem, anyway
                        break;
                    }

                    // in case the user say NO when we try to run the IE4 stub first time,
                    // we should not re-process this stub again.
		    if ( (iPass == 1) && (!lstrcmpi(szKeyName, c_szIE40GUID_STUB)) )
                        continue;
                    
                    if ( RunOneInstallStub(hklmList, hkcuList, szKeyName, pszCurrentUser, iPass) )                    
                        bNextPassNeeded = TRUE;
                }
            }
        }
    }

done:

    if (hklmList != NULL)
        RegCloseKey(hklmList);
    if (hkcuList != NULL)
        RegCloseKey(hkcuList);

    if (pszCurrentUser != szUsername)
        LocalFree(pszCurrentUser);

    CleanupProgressDialog();
}

// Check shell32.dll's version and see if it is the one which supports the integrated WebView
BOOL CheckWebViewShell()
{
    HINSTANCE           hInstShell32;
    DLLGETVERSIONPROC   fpGetDllVersion;    
    BOOL                pWebViewShell = FALSE;
           
    hInstShell32 = LoadLibrary(TEXT("Shell32.dll"));
    if (hInstShell32)
    {
        fpGetDllVersion = (DLLGETVERSIONPROC)GetProcAddress(hInstShell32, "DllGetVersion");
        pWebViewShell = (fpGetDllVersion != NULL);
        FreeLibrary(hInstShell32);
    }
    return pWebViewShell;
}