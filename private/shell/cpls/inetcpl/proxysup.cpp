/*++


Copyright (c) 1996  Microsoft Corporation

Module Name:

    proxysup.c

Abstract:

    Contains implementation for proxy server and proxy bypass list
    dialog interface.

    WARNING: Changes in this code need to be syncronizated
    with changes in proxysup.cxx in the WININET project.

    Contents:

Author:

    Arthur L Bierer (arthurbi) 18-Apr-1996

Revision History:

    18-Apr-1996 arthurbi
    Created

--*/

#include "inetcplp.h"

#include <mluisupp.h>

// Disable warning for VC6 (ASSERT macro causing the problem)
#pragma warning(4:4509) // nonstandard extension used: 'ftn' uses SEH and 'object' has destructor

//
// Don't use CRTs so define our own isdigit
//

#undef isdigit
#define isdigit(ch) (ch >= '0' && ch <= '9')

//
// ARRAY_ELEMENTS - returns number of elements in array
//

#define ARRAY_ELEMENTS(array)   (sizeof(array)/sizeof(array[0]))

#define GET_TERMINATOR(string)  while(*string != '\0') string++

#define IS_BLANK(string)        (*string == '\0')


//
// private types
//
typedef enum {
    STATE_START,
    STATE_PROTOCOL,
    STATE_SCHEME,
    STATE_SERVER,
    STATE_PORT,
    STATE_END,
    STATE_ERROR
} PARSER_STATE;

typedef struct tagMY_URL_SCHEME
{
    LPSTR           SchemeName;
    DWORD           SchemeLength;
    INTERNET_SCHEME SchemeType;
    DWORD           dwControlId;
    DWORD           dwPortControlId;
} MY_URL_SCHEME;

const MY_URL_SCHEME UrlSchemeList[] =
{
    NULL,       0,  INTERNET_SCHEME_DEFAULT,IDC_NOTUSED,                  IDC_NOTUSED,
    "ftp",      3,  INTERNET_SCHEME_FTP,    IDC_PROXY_FTP_ADDRESS,        IDC_PROXY_FTP_PORT,
    "gopher",   6,  INTERNET_SCHEME_GOPHER, IDC_PROXY_GOPHER_ADDRESS,     IDC_PROXY_GOPHER_PORT,
    "http",     4,  INTERNET_SCHEME_HTTP,   IDC_PROXY_HTTP_ADDRESS,       IDC_PROXY_HTTP_PORT,
    "https",    5,  INTERNET_SCHEME_HTTPS,  IDC_PROXY_SECURITY_ADDRESS,   IDC_PROXY_SECURITY_PORT,
    "socks",    5,  INTERNET_SCHEME_SOCKS,  IDC_PROXY_SOCKS_ADDRESS,      IDC_PROXY_SOCKS_PORT,
};

#define INTERNET_MAX_PORT_LENGTH    sizeof("123456789")

typedef struct tagBEFOREUSESAME
{
    // addresses
    TCHAR szFTP      [ INTERNET_MAX_URL_LENGTH + 1 ];
    TCHAR szGOPHER   [ INTERNET_MAX_URL_LENGTH + 1 ];
    TCHAR szSECURE   [ INTERNET_MAX_URL_LENGTH + 1 ];
    TCHAR szSOCKS    [ INTERNET_MAX_URL_LENGTH + 1 ];

    // ports
    TCHAR szFTPport      [ INTERNET_MAX_PORT_LENGTH + 1 ];
    TCHAR szGOPHERport   [ INTERNET_MAX_PORT_LENGTH + 1 ];
    TCHAR szSECUREport   [ INTERNET_MAX_PORT_LENGTH + 1 ];
    TCHAR szSOCKSport    [ INTERNET_MAX_PORT_LENGTH + 1 ];

} BEFOREUSESAME, *LPBEFOREUSESAME;

static const int g_iProxies[] = {IDC_PROXY_HTTP_ADDRESS, IDC_PROXY_FTP_ADDRESS, IDC_PROXY_GOPHER_ADDRESS, IDC_PROXY_SECURITY_ADDRESS, IDC_PROXY_SOCKS_ADDRESS};


typedef struct tagPROXYPAGE
{
    LPBEFOREUSESAME lpOldSettings;
    BOOL            fNT;
    LPPROXYINFO     lpi;
    HINSTANCE       hinstUrlMon;    // runtime load URLMON.DLL
} PROXYPAGE, *LPPROXYPAGE;

extern const TCHAR cszLocalString[] = TEXT("<local>");

#define MAX_SCHEME_NAME_LENGTH  sizeof("gopher")

#define MAX_TITLE           80
#define MAX_DIALOG_MESSAGE  300


//
// private function prototypes
//


LPBEFOREUSESAME SaveCurrentSettings(HWND hDlg);
void RestorePreviousSettings(HWND hDlg, LPBEFOREUSESAME lpSave);

BOOL
ProxyDlgInitProxyServers(
    IN HWND hDlg
    );

BOOL
ProxyDlgOK(
    IN HWND hDlg
    );

BOOL
ProxyDlgInit(
    IN HWND hDlg, LPARAM lParam
    );

VOID
EnableProxyControls(
    IN HWND hDlg
    );

BOOL
IsProxyValid(
    IN HWND     hDlg
    );


BOOL
ParseEditCtlForPort(
    IN OUT LPSTR   lpszProxyName,
    IN HWND        hDlg,
    IN DWORD       dwProxyNameCtlId,
    IN DWORD       dwProxyPortCtlId
    );

BOOL
FormatOutProxyEditCtl(
    IN HWND        hDlg,
    IN DWORD       dwProxyNameCtlId,
    IN DWORD       dwProxyPortCtlId,
    OUT LPSTR     lpszOutputStr,
    IN OUT LPDWORD lpdwOutputStrSize,
    IN DWORD       dwOutputStrLength,
    IN BOOL        fDefaultProxy
    );

INTERNET_SCHEME
MapUrlSchemeName(
    IN LPSTR lpszSchemeName,
    IN DWORD dwSchemeNameLength
    );

DWORD
MapUrlSchemeTypeToCtlId(
    IN INTERNET_SCHEME SchemeType,
    IN BOOL        fIdForPortCtl
    );


BOOL
MapCtlIdUrlSchemeName(
    IN DWORD    dwEditCtlId,
    OUT LPSTR   lpszSchemeOut
    );


DWORD
MapAddrCtlIdToPortCtlId(
    IN DWORD    dwEditCtlId
    );

BOOL
RemoveLocalFromExceptionList(
    IN LPTSTR lpszExceptionList
    );



//
// functions
//


/*******************************************************************

    NAME:       ProxyDlgProc

    SYNOPSIS:   Proxy property sheet dialog proc.

********************************************************************/
INT_PTR CALLBACK ProxyDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
    LPARAM lParam)
{
    LPPROXYPAGE pPxy= (LPPROXYPAGE) GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg)
    {
        
        case WM_INITDIALOG:
        {
            BOOL fInited;
            
            fInited = ProxyDlgInit(hDlg, lParam);
            return fInited;
        }
        
        case WM_NOTIFY:
        {
            NMHDR * lpnm = (NMHDR *) lParam;
            switch (lpnm->code)
            {
                case PSN_APPLY:
                {
                    BOOL fRet = ProxyDlgOK(hDlg);
                    SetPropSheetResult(hDlg,!fRet);
                    return !fRet;
                    break;
                }
                
                case PSN_RESET:
                    SetPropSheetResult(hDlg,FALSE);
                    break;
            }
            break;
        }
        
        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDC_PROXY_ENABLE:
                    EnableProxyControls(hDlg);
                    break;

                case IDC_PROXY_HTTP_ADDRESS:
                case IDC_PROXY_GOPHER_ADDRESS:
                case IDC_PROXY_SECURITY_ADDRESS:
                case IDC_PROXY_FTP_ADDRESS:
                case IDC_PROXY_SOCKS_ADDRESS:

                    if ( GET_WM_COMMAND_CMD(wParam, lParam) == EN_KILLFOCUS )
                    {
                        ParseEditCtlForPort(NULL, hDlg, (GET_WM_COMMAND_ID(wParam, lParam)), 0);
                        EnableProxyControls(hDlg);
                    }
                    break;

                case IDC_PROXY_HTTP_PORT:
                case IDC_PROXY_GOPHER_PORT:
                case IDC_PROXY_SECURITY_PORT:
                case IDC_PROXY_FTP_PORT:
                case IDC_PROXY_SOCKS_PORT:

                    if ( GET_WM_COMMAND_CMD(wParam, lParam) == EN_KILLFOCUS )
                    {
                        EnableProxyControls(hDlg);
                    }
                    break;


                case IDC_PROXY_USE_SAME_SERVER:

                    if ( GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED )
                    {
                        if (IsDlgButtonChecked(hDlg, IDC_PROXY_USE_SAME_SERVER))
                            pPxy->lpOldSettings = SaveCurrentSettings(hDlg);
                        else if (pPxy->lpOldSettings !=NULL)
                        {
                            RestorePreviousSettings(hDlg, pPxy->lpOldSettings);
                            pPxy->lpOldSettings = NULL;
                        }

                        EnableProxyControls(hDlg);
                    }
                    break;

                case IDOK:

                    if ( GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED )
                    {
                        BOOL fLeaveDialog = TRUE;

                        if (!IsProxyValid(hDlg))
                        {
                            // The proxy is invalid, so we need to ask the user if they want to turn it off.
                            TCHAR szTitle[MAX_TITLE];
                            TCHAR szMessage[MAX_DIALOG_MESSAGE];
                            int nUserResponce;

                            MLLoadShellLangString(IDS_INVALID_PROXY_TITLE, szTitle, ARRAYSIZE(szTitle));
                            MLLoadShellLangString(IDS_INVALID_PROXY, szMessage, ARRAYSIZE(szMessage));

                            // Ask the user if they want to turn off the proxy.
                            nUserResponce = MessageBox(hDlg, szMessage, szTitle, MB_YESNOCANCEL | MB_ICONERROR);
                            if (IDYES == nUserResponce)
                                pPxy->lpi->fEnable = FALSE;
                            else if (IDCANCEL == nUserResponce)
                                fLeaveDialog = FALSE;   // The user hit cancel, so let's not leave the dialog.
                        }

                        if (fLeaveDialog)
                        {
                            //
                            // Read the Ctls and Save out to the proxy..
                            //
                            ProxyDlgOK(hDlg);
                            EndDialog(hDlg, IDOK);
                        }
                    }
                    break;

                case IDCANCEL:
                    if ( GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED )
                    {
                        EndDialog(hDlg, IDCANCEL);
                    }
                    break;

            }
            break;

        case WM_DESTROY:
            if (pPxy->lpOldSettings)
                LocalFree(pPxy->lpOldSettings);

            if (pPxy->hinstUrlMon)
                FreeLibrary(pPxy->hinstUrlMon);

            LocalFree(pPxy);
            return TRUE;

        case WM_HELP:      // F1
            ResWinHelp((HWND) ((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
                HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_CONTEXTMENU:      // right mouse click
            ResWinHelp((HWND)wParam, IDS_HELPFILE,
                HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        default:
            return FALSE;
    }
    return TRUE;
}


//
// Private Functions.
//

VOID
EnableProxyControls(HWND hDlg)

/*++

Routine Description:

    Enables controls appropriately depending on what
    checkboxes are checked.

Arguments:

    hDlg        - Page Dialog Box.

Return Value:

    VOID

--*/


{
    BOOL fNT = ((LPPROXYPAGE)GetWindowLongPtr(hDlg, DWLP_USER))->fNT;
    
    BOOL fEnable = !g_restrict.fProxy;
            
    BOOL fUseOneProxy = IsDlgButtonChecked(hDlg,IDC_PROXY_USE_SAME_SERVER);
    
    EnableDlgItem(hDlg,IDC_GRP_SETTINGS2, fEnable);
    EnableDlgItem(hDlg,IDC_PROXY_EXCEPTIONS_GROUPBOX, fEnable);

    EnableDlgItem(hDlg,IDC_TYPE_TEXT, fEnable);
    EnableDlgItem(hDlg,IDC_ADDR_TEXT, fEnable);
    EnableDlgItem(hDlg,IDC_PORT_TEXT, fEnable);
    EnableDlgItem(hDlg,IDC_EXCEPT_TEXT, fEnable);
    EnableDlgItem(hDlg,IDC_EXCEPT2_TEXT, fEnable);
    EnableDlgItem(hDlg,IDC_PROXY_ICON1, fEnable);
    EnableDlgItem(hDlg,IDC_PROXY_ICON2, fEnable);
    
    EnableDlgItem(hDlg,IDC_PROXY_HTTP_CAPTION, fEnable);
    EnableDlgItem(hDlg,IDC_PROXY_SECURITY_CAPTION, fEnable);
    EnableDlgItem(hDlg,IDC_PROXY_FTP_CAPTION, fEnable);
    EnableDlgItem(hDlg,IDC_PROXY_GOPHER_CAPTION, fEnable);
    EnableDlgItem(hDlg,IDC_PROXY_SOCKS_CAPTION, fEnable);

    EnableDlgItem(hDlg, IDC_PROXY_USE_SAME_SERVER, fEnable);

    EnableDlgItem(hDlg,IDC_PROXY_HTTP_ADDRESS,fEnable);
    EnableDlgItem(hDlg,IDC_PROXY_HTTP_PORT,fEnable);
           
    EnableDlgItem(hDlg,IDC_PROXY_OVERRIDE,fEnable);

    //
    // If we only want one Proxy, then make all others use the same
    //  proxy.
    //

    EnableDlgItem(hDlg,IDC_PROXY_SECURITY_ADDRESS,!fUseOneProxy && fEnable);
    EnableDlgItem(hDlg,IDC_PROXY_SECURITY_PORT,!fUseOneProxy && fEnable);

    EnableDlgItem(hDlg,IDC_PROXY_FTP_ADDRESS,!fUseOneProxy && fEnable);
    EnableDlgItem(hDlg,IDC_PROXY_FTP_PORT,!fUseOneProxy && fEnable);

    EnableDlgItem(hDlg,IDC_PROXY_GOPHER_ADDRESS,!fUseOneProxy && fEnable);
    EnableDlgItem(hDlg,IDC_PROXY_GOPHER_PORT,!fUseOneProxy && fEnable);

    EnableDlgItem(hDlg,IDC_PROXY_SOCKS_ADDRESS,!fUseOneProxy && fEnable);
    EnableDlgItem(hDlg,IDC_PROXY_SOCKS_PORT,!fUseOneProxy && fEnable);

    //
    // If we only want one proxy, prepopulate the other fields
    //  so they use the the mirror of the first one.
    //

    if (fUseOneProxy)
    {
        TCHAR szProxyName[MAX_URL_STRING+1];
        TCHAR szProxyPort[INTERNET_MAX_PORT_LENGTH];

        GetDlgItemText(hDlg,
            IDC_PROXY_HTTP_ADDRESS,
            szProxyName,
            ARRAYSIZE(szProxyName));

        GetDlgItemText(hDlg,
            IDC_PROXY_HTTP_PORT,
            szProxyPort,
            ARRAYSIZE(szProxyPort));

        SetDlgItemText(hDlg,IDC_PROXY_SECURITY_ADDRESS,szProxyName);
        SetDlgItemText(hDlg,IDC_PROXY_SECURITY_PORT,szProxyPort);

        SetDlgItemText(hDlg,IDC_PROXY_FTP_ADDRESS,szProxyName);
        SetDlgItemText(hDlg,IDC_PROXY_FTP_PORT,szProxyPort);

        SetDlgItemText(hDlg,IDC_PROXY_GOPHER_ADDRESS,szProxyName);
        SetDlgItemText(hDlg,IDC_PROXY_GOPHER_PORT,szProxyPort);

        SetDlgItemText(hDlg,IDC_PROXY_SOCKS_ADDRESS,TEXT(""));
        SetDlgItemText(hDlg,IDC_PROXY_SOCKS_PORT,TEXT(""));
    }
}

//
// SaveCurrentSettings()
//
// Saves current settings... just in case user changes their mind.
//
// Returns a pointer to a structure filled with current settings.
//
LPBEFOREUSESAME SaveCurrentSettings(HWND hDlg)
{
    LPBEFOREUSESAME lpSave = (LPBEFOREUSESAME)LocalAlloc(LPTR, sizeof(*lpSave));

    if (!lpSave)
        return lpSave; // if NULL return NULL

    GetDlgItemText(hDlg, IDC_PROXY_FTP_ADDRESS,      lpSave->szFTP,    INTERNET_MAX_URL_LENGTH);
    GetDlgItemText(hDlg, IDC_PROXY_GOPHER_ADDRESS,   lpSave->szGOPHER, INTERNET_MAX_URL_LENGTH);
    GetDlgItemText(hDlg, IDC_PROXY_SECURITY_ADDRESS, lpSave->szSECURE, INTERNET_MAX_URL_LENGTH);
    GetDlgItemText(hDlg, IDC_PROXY_SOCKS_ADDRESS,    lpSave->szSOCKS,  INTERNET_MAX_URL_LENGTH);

    GetDlgItemText(hDlg, IDC_PROXY_FTP_PORT,      lpSave->szFTPport,    INTERNET_MAX_URL_LENGTH);
    GetDlgItemText(hDlg, IDC_PROXY_GOPHER_PORT,   lpSave->szGOPHERport, INTERNET_MAX_URL_LENGTH);
    GetDlgItemText(hDlg, IDC_PROXY_SECURITY_PORT, lpSave->szSECUREport, INTERNET_MAX_URL_LENGTH);
    GetDlgItemText(hDlg, IDC_PROXY_SOCKS_PORT,    lpSave->szSOCKSport,  INTERNET_MAX_URL_LENGTH);

    return lpSave;

} // SaveCurrentSettings()


//
// RestorePreviousSettings()
//
// Restores settings... just in case user changes their mind.
//
void RestorePreviousSettings(HWND hDlg, LPBEFOREUSESAME lpSave)
{

    if (!lpSave)
    return; // nothing to do

    SetDlgItemText(hDlg, IDC_PROXY_FTP_ADDRESS,      lpSave->szFTP    );
    SetDlgItemText(hDlg, IDC_PROXY_GOPHER_ADDRESS,   lpSave->szGOPHER );
    SetDlgItemText(hDlg, IDC_PROXY_SECURITY_ADDRESS, lpSave->szSECURE );
    SetDlgItemText(hDlg, IDC_PROXY_SOCKS_ADDRESS,    lpSave->szSOCKS  );

    SetDlgItemText(hDlg, IDC_PROXY_FTP_PORT,      lpSave->szFTPport    );
    SetDlgItemText(hDlg, IDC_PROXY_GOPHER_PORT,   lpSave->szGOPHERport );
    SetDlgItemText(hDlg, IDC_PROXY_SECURITY_PORT, lpSave->szSECUREport );
    SetDlgItemText(hDlg, IDC_PROXY_SOCKS_PORT,    lpSave->szSOCKSport  );

    LocalFree(lpSave);  // give back the memory

} // RestorePreviousSettings()


BOOL
ProxyDlgInit(
    IN HWND hDlg, LPARAM lParam
    )

/*++

Routine Description:

    Initialization proc for proxy prop page

Arguments:

    hDlg        - Page Dialog Box.

Return Value:

    BOOL
    Success - TRUE

    Failure - FALSE

--*/


{
    BOOL fSuccess;
    LPPROXYPAGE pPxy;

    pPxy = (LPPROXYPAGE)LocalAlloc(LPTR, sizeof(*pPxy));
    // NOTE: this NULLS lpOldSettings

    if (!pPxy)
        return FALSE;   // no memory?

    OSVERSIONINFOA osvi;
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionExA(&osvi);
    
    pPxy->fNT = (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT);
    ASSERT(pPxy->lpOldSettings == NULL);
        
    // tell dialog where to get info
    SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pPxy);

    pPxy->lpi = (LPPROXYINFO)lParam;
    //
    // Begin by reading and setting the list of proxy
    //  servers we have.
    //

    fSuccess = ProxyDlgInitProxyServers( hDlg );

    if (!fSuccess)
        return FALSE;

    //
    // read settings from registry, for the list of Exclusion Hosts.
    //

    RegEntry re(REGSTR_PATH_INTERNETSETTINGS,HKEY_CURRENT_USER);

    if (re.GetError() == ERROR_SUCCESS)
    {
        BUFFER bufProxyString(MAX_URL_STRING+1);
        
        if (!bufProxyString)
        {
            MsgBox(NULL,IDS_ERROutOfMemory,MB_ICONEXCLAMATION,MB_OK);
            return FALSE;
        }

        //  
        // get proxy override settings from registry and stuff fields
        //
        if(pPxy->lpi->fReadProxyFromRegistry) {
            re.GetString(REGSTR_VAL_PROXYOVERRIDE,bufProxyString.QueryPtr(),
                     bufProxyString.QuerySize());

            //
            // Parse Overide list, remove "<local>"
            RemoveLocalFromExceptionList(bufProxyString.QueryPtr());

            SetDlgItemText(hDlg,IDC_PROXY_OVERRIDE,bufProxyString.QueryPtr());
        } else {
            SetDlgItemText(hDlg, IDC_PROXY_OVERRIDE, pPxy->lpi->szOverride);
        }
    }

    //
    // initialize the UI appropriately
    //

    EnableProxyControls(hDlg);
    
    return TRUE;
}

BOOL
ProxyDlgOK(
    IN HWND hDlg
    )

/*++

Routine Description:

    OK button handler for proxy prop page

Arguments:

    hDlg        - Page Dialog Box.

Return Value:

    BOOL
    Success - TRUE

    Failure - FALSE

--*/


{
    LPPROXYPAGE pPxy = (LPPROXYPAGE)GetWindowLongPtr(hDlg, DWLP_USER);       
    TCHAR szProxyListOutputBuffer[MAX_URL_STRING];
    CHAR  szProxyListOutputBufferA[MAX_URL_STRING];
    DWORD dwBufferOffset = 0;

    //
    // Get the state of our two check boxes.
    //

    BOOL fUseOneProxy =
        IsDlgButtonChecked(hDlg,IDC_PROXY_USE_SAME_SERVER);
    //
    // Open our Registry Key.
    //
    
    RegEntry re(REGSTR_PATH_INTERNETSETTINGS,HKEY_CURRENT_USER);
    if (re.GetError() == ERROR_SUCCESS)
    {       
        //
        //  Now Format, and write out the list of proxies to
        //  the registry.  We special case the case of
        //  only proxy.
        //

        szProxyListOutputBufferA[dwBufferOffset] = '\0';

        if ( fUseOneProxy )
        {
            FormatOutProxyEditCtl(
                                  hDlg,
                                  IDC_PROXY_HTTP_ADDRESS,
                                  IDC_PROXY_HTTP_PORT,
                                  szProxyListOutputBufferA,
                                  &dwBufferOffset,
                                  ARRAY_ELEMENTS(szProxyListOutputBufferA),
                                  TRUE
                                 );
        }
        else
        {
            for (int i = 1; i < ARRAY_ELEMENTS(UrlSchemeList); ++i)
            {
                FormatOutProxyEditCtl(
                                      hDlg,
                                      UrlSchemeList[i].dwControlId,
                                      UrlSchemeList[i].dwPortControlId,
                                      szProxyListOutputBufferA,
                                      &dwBufferOffset,
                                      ARRAY_ELEMENTS(szProxyListOutputBufferA),
                                      FALSE
                                     );

            }
        }

        szProxyListOutputBufferA[dwBufferOffset] = '\0';

#ifdef UNICODE
        SHAnsiToUnicode(szProxyListOutputBufferA, szProxyListOutputBuffer, MAX_URL_STRING);
#else
        lstrcpy(szProxyListOutputBuffer, szProxyListOutputBufferA);
#endif
        if(pPxy->lpi->fReadProxyFromRegistry) {
            re.SetValue(REGSTR_VAL_PROXYSERVER, szProxyListOutputBuffer);
        } else {
            StrCpyN(pPxy->lpi->szProxy, szProxyListOutputBuffer, MAX_URL_STRING);
        }

        //
        // Now Write out the Proxy Exception List
        //  (list of addresses to use for local connections)
        //

        szProxyListOutputBuffer[0] = '\0';
        
        GetDlgItemText(hDlg,
                       IDC_PROXY_OVERRIDE,
                       szProxyListOutputBuffer,
                       ARRAY_ELEMENTS(szProxyListOutputBuffer));
                
        if(pPxy->lpi->fReadProxyFromRegistry) {
            re.SetValue(REGSTR_VAL_PROXYOVERRIDE, szProxyListOutputBuffer);            
        } else {
            StrCpyN(pPxy->lpi->szOverride, szProxyListOutputBuffer, MAX_URL_STRING);
        }
    }
    
    else    
    {
        AssertMsg(0, TEXT("Couldn't save settings to registry!"));
    }


    // let any active (participating) wininet's get notified
    //
    InternetSetOption(NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);

    return TRUE;
}


INTERNET_SCHEME
MapUrlSchemeName(
    IN LPSTR lpszSchemeName,
    IN DWORD dwSchemeNameLength
    )

/*++

Routine Description:

    Maps a scheme name/length to a scheme name type

Arguments:

    lpszSchemeName  - pointer to name of scheme to map

    dwSchemeNameLength  - length of scheme (if -1, lpszSchemeName is ASCIZ)

Return Value:

    INTERNET_SCHEME

--*/

{
    if (dwSchemeNameLength == (DWORD)-1)
    {
        dwSchemeNameLength = (DWORD)lstrlenA(lpszSchemeName);
    }
    
    for (int i = 0; i < ARRAY_ELEMENTS(UrlSchemeList); ++i)
    {
        if (UrlSchemeList[i].SchemeLength == dwSchemeNameLength)
        {   
            CHAR chBackup = lpszSchemeName[dwSchemeNameLength];
            lpszSchemeName[dwSchemeNameLength] = '\0';
            
            if(StrCmpIA(UrlSchemeList[i].SchemeName,lpszSchemeName) == 0)
            {
                lpszSchemeName[dwSchemeNameLength] = chBackup;
                return UrlSchemeList[i].SchemeType;
            }
            
            lpszSchemeName[dwSchemeNameLength] = chBackup;
        }
    }
    return INTERNET_SCHEME_UNKNOWN;
}



DWORD
MapUrlSchemeTypeToCtlId(
    IN INTERNET_SCHEME SchemeType,
    IN BOOL        fIdForPortCtl
    )

/*++

Routine Description:

    Maps a scheme to a dlg child control id.

Arguments:

    Scheme    - Scheme to Map

    fIdForPortCtl - If TRUE, means we really want the ID for a the PORT control
            not the ADDRESS control.

Return Value:

    DWORD

--*/

{
    for (int i = 0; i < ARRAY_ELEMENTS(UrlSchemeList); ++i)
    {
        if (SchemeType == UrlSchemeList[i].SchemeType)
        {
            return (fIdForPortCtl ? UrlSchemeList[i].dwPortControlId :
                    UrlSchemeList[i].dwControlId );
        }
    }
    return IDC_NOTUSED;
}

BOOL
MapCtlIdUrlSchemeName(
    IN DWORD    dwEditCtlId,
    OUT LPSTR   lpszSchemeOut
    )

/*++

Routine Description:

    Maps a dlg child control id to String represnting
    the name of the scheme.

Arguments:

    dwEditCtlId   - Edit Control to Map Out.

    lpszSchemeOut - Scheme to Map Out.
            WARNING: ASSUMED to be size of largest scheme type.

Return Value:

    BOOL
    Success - TRUE

    Failure - FALSE

--*/

{
    ASSERT(lpszSchemeOut);

    for (int i = 0; i < ARRAY_ELEMENTS(UrlSchemeList); ++i)
    {
        if (dwEditCtlId == UrlSchemeList[i].dwControlId )
        {
            StrCpyA(lpszSchemeOut, UrlSchemeList[i].SchemeName);
            return TRUE;
        }
    }
    return FALSE;
}


DWORD
MapAddrCtlIdToPortCtlId(
    IN DWORD    dwEditCtlId
    )

/*++

Routine Description:

    Maps a dlg child control id for addresses to
    a dlg control id for ports.

Arguments:

    dwEditCtlId   - Edit Control to Map Out.

Return Value:

    DWORD
    Success - Correctly mapped ID.

    Failure - 0.

--*/

{

    for (int i = 0; i < ARRAY_ELEMENTS(UrlSchemeList); ++i)
    {
        if (dwEditCtlId == UrlSchemeList[i].dwControlId )
        {
            return UrlSchemeList[i].dwPortControlId ;
        }
    }
    return FALSE;
}


BOOL
ProxyDlgInitProxyServers(
    IN HWND hDlg
    )

/*++

Routine Description:

    Parses a list of proxy servers and sets them into the newly created
    Proxy Dialog.

    Ruthlessly stolen from RFirth's proxysup.cxx in WININET by
    ArthurBi.

Arguments:

    hDlg    -      HWin to add our stuff to.


Return Value:

    BOOL
    Success - TRUE

    Failure - FALSE

Comments:

    Designed to handle Proxy string entry of the Form:

      pointer to list of proxies of the form:

      [<protocol>=][<scheme>"://"]<server>[":"<port>][";"*]

      The list can is read from the registry.


--*/

{
    DWORD error = !ERROR_SUCCESS;
    DWORD entryLength;
    LPSTR protocolName;
    DWORD protocolLength;
    LPSTR schemeName;
    DWORD schemeLength;
    LPSTR serverName;
    DWORD serverLength;
    PARSER_STATE state;
    DWORD nSlashes;
    INTERNET_PORT port;
    BOOL done;
    LPSTR lpszList;

    entryLength = 0;
    protocolLength = 0;
    schemeName = NULL;
    schemeLength = 0;
    serverName = NULL;
    serverLength = 0;
    state = STATE_PROTOCOL;
    nSlashes = 0;
    port = 0;
    done = FALSE;


    //
    // Open the Reg Key.
    //

    RegEntry re(REGSTR_PATH_INTERNETSETTINGS,HKEY_CURRENT_USER);

    if (re.GetError() != ERROR_SUCCESS)
        return FALSE; // no REG values..


    //
    // Crack the Registry values, read the proxy list
    //


    BUFFER bufProxyString(MAX_URL_STRING+1);
    BOOL   fProxyEnabled;

    if (!bufProxyString)
    {
        MsgBox(NULL,IDS_ERROutOfMemory,MB_ICONEXCLAMATION,MB_OK);
        return FALSE;
    }

    //
    // is proxy enabled?
    // It should if we got into this dialog.
    //

    fProxyEnabled = (BOOL)re.GetNumber(REGSTR_VAL_PROXYENABLE,0);

    //
    // get proxy server and override settings from registry and stuff fields
    //

    re.GetString(REGSTR_VAL_PROXYSERVER,bufProxyString.QueryPtr(),
        bufProxyString.QuerySize());


    LPPROXYPAGE pPxy = (LPPROXYPAGE)GetWindowLongPtr(hDlg, DWLP_USER);

    // if there's a proxy passed in from the main page, then use it; otherwise use the registry val
#ifndef UNICODE
    lpszList = pPxy->lpi->fReadProxyFromRegistry ? bufProxyString.QueryPtr() : pPxy->lpi->szProxy;
#else
    char*  szList = NULL;
    LPTSTR lpTmp = pPxy->lpi->fReadProxyFromRegistry ? bufProxyString.QueryPtr() : pPxy->lpi->szProxy;
    DWORD  cch = lstrlen(lpTmp) + 1;
    szList = new char[2 * cch];
    if (szList)
    {
        SHUnicodeToAnsi(lpTmp, szList, 2 * cch);
        lpszList = szList;
    }
    else
    {
        MsgBox(NULL,IDS_ERROutOfMemory,MB_ICONEXCLAMATION,MB_OK);
        return FALSE;
    }
#endif

    protocolName = lpszList;

    //
    // walk the list, pulling out the various scheme parts
    //

    do
    {
        char ch = *lpszList++;

        if ((nSlashes == 1) && (ch != '/'))
        {
            state = STATE_ERROR;
            break;
        }
        
        switch (ch)
        {
            case '=':
                if ((state == STATE_PROTOCOL) && (entryLength != 0))
                {
                    protocolLength = entryLength;
                    entryLength = 0;
                    state = STATE_SCHEME;
                    schemeName = lpszList;
                }
                else
                {
                    //
                    // '=' can't legally appear anywhere else
                    //
                    state = STATE_ERROR;
                }
                break;
                
            case ':':
                switch (state)
                {
                    case STATE_PROTOCOL:
                        if (*lpszList == '/')
                        {
                            schemeName = protocolName;
                            protocolName = NULL;
                            schemeLength = entryLength;
                            protocolLength = 0;
                            state = STATE_SCHEME;
                        }
                        else if (*lpszList != '\0')
                        {
                            serverName = protocolName;
                            serverLength = entryLength;
                            state = STATE_PORT;
                        }
                        else
                        {
                            state = STATE_ERROR;
                        }
                        entryLength = 0;
                        break;
                        
                    case STATE_SCHEME:
                        if (*lpszList == '/')
                        {
                            schemeLength = entryLength;
                        }
                        else if (*lpszList != '\0')
                        {
                            serverName = schemeName;
                            serverLength = entryLength;
                            state = STATE_PORT;
                        }
                        else
                        {
                            state = STATE_ERROR;
                        }
                        entryLength = 0;
                        break;
                        
                    case STATE_SERVER:
                        serverLength = entryLength;
                        state = STATE_PORT;
                        entryLength = 0;
                        break;

                    default:
                        state = STATE_ERROR;
                        break;
                }
                break;

            case '/':
                if ((state == STATE_SCHEME) && (nSlashes < 2) && (entryLength == 0))
                {
                    if (++nSlashes == 2)
                    {
                        state = STATE_SERVER;
                        serverName = lpszList;
                    }
                }
                else
                {
                    state = STATE_ERROR;
                }
                break;

            case '\v':  // vertical tab, 0x0b
            case '\f':  // form feed, 0x0c
                if (!((state == STATE_PROTOCOL) && (entryLength == 0)))
                {
                    //
                    // can't have embedded whitespace
                    //

                    state = STATE_ERROR;
                }
                break;

            default:
                if (state != STATE_PORT)
                {
                    ++entryLength;
                }
                else if (isdigit(ch))
                {
                    //
                    // BUGBUG - we will overflow if >65535
                    //
                    port = port * 10 + (ch - '0');
                }
                else
                {                   
                    //
                    // STATE_PORT && non-digit character - error
                    //
                    state = STATE_ERROR;
                }
                break;

            case '\0':
                done = TRUE;

                //
                // fall through
                //
            case ' ':
            case '\t':
            case '\n':
            case '\r':
            case ';':
            case ',':
                if (serverLength == 0)
                {
                    serverLength = entryLength;
                }
                if (serverLength != 0)
                {
                    if (serverName == NULL)
                    {
                        serverName = (schemeName != NULL)
                            ? schemeName : protocolName;
                    }

                    ASSERT(serverName != NULL);

                    INTERNET_SCHEME protocol;

                    if (protocolLength != 0)
                    {
                        protocol = MapUrlSchemeName(protocolName, protocolLength);
                    }
                    else
                    {
                        protocol = INTERNET_SCHEME_DEFAULT;
                    }

                    INTERNET_SCHEME scheme;

                    if (schemeLength != 0)
                    {
                        scheme = MapUrlSchemeName(schemeName, schemeLength);
                    }
                    else
                    {
                        scheme = INTERNET_SCHEME_DEFAULT;
                    }

                    //
                    // add an entry if this is a protocol we handle and we don't
                    // already have an entry for it
                    //

                    if ((protocol != INTERNET_SCHEME_UNKNOWN)
                        && (scheme != INTERNET_SCHEME_UNKNOWN))
                    {
                        DWORD dwCtlId = IDC_NOTUSED;
                        DWORD dwPortCtlId = IDC_NOTUSED;
                        CHAR chBackup;

                        error = ERROR_SUCCESS;
                        //
                        // we can only currently handle CERN proxies (unsecure or
                        // secure) so kick out anything that wants to go via a different
                        // proxy scheme
                        //

                        if (protocol == INTERNET_SCHEME_DEFAULT)
                        {
                            CheckDlgButton( hDlg, IDC_PROXY_USE_SAME_SERVER, TRUE );
                            dwCtlId     = IDC_PROXY_HTTP_ADDRESS;
                            dwPortCtlId = IDC_PROXY_HTTP_PORT;
                        }
                        else
                        {
                            dwCtlId     = MapUrlSchemeTypeToCtlId(protocol,FALSE);
                            dwPortCtlId = MapUrlSchemeTypeToCtlId(protocol,TRUE);
                        }

                        //
                        // Set the Field Entry.
                        //

                        LPSTR lpszProxyNameText;

                        if (scheme != INTERNET_SCHEME_DEFAULT)
                        {
                            ASSERT(schemeLength != 0);
                            lpszProxyNameText = schemeName;
                        }
                        else
                            lpszProxyNameText = serverName;

                        chBackup = serverName[serverLength];
                        serverName[serverLength] = '\0';
                        
                        SetDlgItemTextA( hDlg, dwCtlId, lpszProxyNameText );
                        if ( port )
                            SetDlgItemInt( hDlg, dwPortCtlId, port, FALSE );

                        serverName[serverLength] = chBackup;
                        
                    }
                    
                    else
                    {                      
                        //
                        // bad/unrecognised protocol or scheme. Treat it as error
                        // for now
                        //
                        error = !ERROR_SUCCESS;
                    }
                }

                entryLength = 0;
                protocolName = lpszList;
                protocolLength = 0;
                schemeName = NULL;
                schemeLength = 0;
                serverName = NULL;
                serverLength = 0;
                nSlashes = 0;
                port = 0;
                if (error == ERROR_SUCCESS)
                {
                    state = STATE_PROTOCOL;
                }
                else
                {
                    state = STATE_ERROR;
                }
                break;
        }

        if (state == STATE_ERROR)
        {
            break;
        }
        
    } while (!done);

#ifdef UNICODE
    delete [] szList;
#endif

    if (state == STATE_ERROR)
    {
        error = ERROR_INVALID_PARAMETER;
    }

    if ( error == ERROR_SUCCESS )
        error = TRUE;
    else
        error = FALSE;

    return error;
}


BOOL
IsProxyValid(
    IN HWND     hDlg
    )

/*++

Routine Description:

    Determines if the Proxy is valid.  The proxy is invalid if
    all of the proxy entries are empty.

Arguments:

    hDlg      - HWIN of the dialog to play with.

Return Value:

    BOOL
    Success TRUE - Valid.

    FALSE - Invalid.


--*/

{
    BOOL  fProxyIsValid = FALSE;
    TCHAR szProxyUrl[MAX_URL_STRING+1];
    int   iCurrentProxy = 0;

    ASSERT(IsWindow(hDlg));

    for (int iIndex = 0; iIndex < ARRAYSIZE(g_iProxies); iIndex++)
    {
        szProxyUrl[0] = '\0';
        GetDlgItemText(hDlg,
                       g_iProxies[iIndex],
                       szProxyUrl,
                       sizeof(szProxyUrl));
        
        if (szProxyUrl[0])
        {
            fProxyIsValid = TRUE;
            break;
        }
    }

    return fProxyIsValid;
}



BOOL
ParseEditCtlForPort(
    IN OUT LPSTR   lpszProxyName,
    IN HWND        hDlg,
    IN DWORD       dwProxyNameCtlId,
    IN DWORD       dwProxyPortCtlId
    )

/*++

Routine Description:

    Parses a Port Number off then end of a Proxy Server URL that is
    located either in the Proxy Name Edit Box, or passed in as
    a string pointer.

Arguments:

    lpszProxyName - (OPTIONAL) string pointer with Proxy Name to parse, and
            set into the Proxy Name edit ctl field.

    hDlg      - HWIN of the dialog to play with.

    dwProxyNameCtlId -  Res Ctl Id to play with.

    dwProxyPortCtlId -  Res Ctl Id of Port Number Edit Box.

Return Value:

    BOOL
    Success TRUE -

    Failure FALSE


--*/

{
    CHAR   szProxyUrl[MAX_URL_STRING+1];
    LPSTR  lpszPort;
    LPSTR  lpszProxyUrl;

    ASSERT(IsWindow(hDlg));

    if ( dwProxyPortCtlId == 0 )
    {
        dwProxyPortCtlId = MapAddrCtlIdToPortCtlId(dwProxyNameCtlId);
        ASSERT(dwProxyPortCtlId);
    }

    //
    // Get the Proxy String from the Edit Control
    //  (OR) from the Registry [passed in]
    //

    if ( lpszProxyName )
        lpszProxyUrl = lpszProxyName;
    else
    {
    //
    // Need to Grab it out of the edit control.
    //
        GetDlgItemTextA(hDlg,
            dwProxyNameCtlId,
            szProxyUrl,
            sizeof(szProxyUrl));

        lpszProxyUrl = szProxyUrl;
    }

    //
    // Now find the port.
    //

    lpszPort = lpszProxyUrl;

    GET_TERMINATOR(lpszPort);

    lpszPort--;

    //
    // Walk backwards from the end of url looking
    //  for a port number sitting on the end like this
    //  http://proxy:1234
    //

    while ( (lpszPort > lpszProxyUrl) &&
        (*lpszPort != ':')         &&
        (isdigit(*lpszPort))  )
    {
        lpszPort--;
    }

    //
    // If we found a match for our rules
    //  then set the port, otherwise
    //  we assume the user knows what he's
    //  doing.
    //

    if ( *lpszPort == ':'   &&   isdigit(*(lpszPort+1)) )
    {
        *lpszPort = '\0';

        SetDlgItemTextA(hDlg, dwProxyPortCtlId, (lpszPort+1));
    }

    SetDlgItemTextA(hDlg, dwProxyNameCtlId, lpszProxyUrl);
    return TRUE;
}


BOOL
FormatOutProxyEditCtl(
    IN HWND    hDlg,
    IN DWORD       dwProxyNameCtlId,
    IN DWORD       dwProxyPortCtlId,
    OUT LPSTR      lpszOutputStr,
    IN OUT LPDWORD lpdwOutputStrSize,
    IN DWORD       dwOutputStrLength,
    IN BOOL    fDefaultProxy
    )

/*++

Routine Description:

    Combines Proxy URL components into a string that can be saved
    in the registry.  Can be called multiple times to build
    a list of proxy servers, or once to special case a "default"
    proxy.

Arguments:

    hDlg      - HWIN of the dialog to play with.

    dwProxyNameCtlId -  Res Ctl Id to play with.

    dwProxyPortCtlId -  Res Ctl Id of Port Number Edit Box.

    lpszOutputStr    -  The start of the output string to send
            the product of this function.

    lpdwOutputStrSize - The amount of used space in lpszOutputStr
            that is already used.  New output should
            start from (lpszOutputStr + *lpdwOutputStrSize)

    fDefaultProxy     - Default Proxy, don't add scheme= in front of the proxy
            just use plop one proxy into the registry.


Return Value:

    BOOL
    Success TRUE

    Failure FALSE


--*/

{
    LPSTR lpszOutput;
    LPSTR lpszEndOfOutputStr;

    ASSERT(IsWindow(hDlg));
    ASSERT(lpdwOutputStrSize);

    lpszOutput = lpszOutputStr + *lpdwOutputStrSize;
    lpszEndOfOutputStr = lpszOutputStr + dwOutputStrLength;

    ASSERT( lpszEndOfOutputStr > lpszOutput );

    if ( lpszEndOfOutputStr <= lpszOutput )
        return FALSE; // bail out, ran out of space

    //
    // Plop ';' if we're not the first in this string buffer.
    //

    if (*lpdwOutputStrSize != 0  )
    {
        *lpszOutput = ';';

        lpszOutput++;

        if ( lpszEndOfOutputStr <= lpszOutput )
            return FALSE; // bail out, ran out of space
    }

    //
    // Put the schemetype= into the string
    //  ex:  http=
    //

    if ( ! fDefaultProxy )
    {
        if ( lpszEndOfOutputStr <= (MAX_SCHEME_NAME_LENGTH + lpszOutput + 1) )
            return FALSE; // bail out, ran out of space
        
        if (!MapCtlIdUrlSchemeName(dwProxyNameCtlId,lpszOutput))
            return FALSE;
        
        lpszOutput += lstrlenA(lpszOutput);
    
        *lpszOutput = '=';
        lpszOutput++;
    }
    
    //
    // Need to Grab ProxyUrl out of the edit control.
    //
    
    GetDlgItemTextA(hDlg, dwProxyNameCtlId, lpszOutput, (int)(lpszEndOfOutputStr - lpszOutput));

    if ( IS_BLANK(lpszOutput) )
        return FALSE;

    //
    // Now seperate out the port so we can save them seperately.
    //   But go past the Proxy Url while we're at it.
    //      ex: http=http://netscape-proxy
    //

    if (!ParseEditCtlForPort(lpszOutput, hDlg, dwProxyNameCtlId, dwProxyPortCtlId))
        return FALSE;

    lpszOutput += lstrlenA(lpszOutput);

    //
    // Now, add in a ':" for the port number, if we don't
    //  have a port we'll remove it.
    //
    {
        *lpszOutput = ':';
        
        lpszOutput++;
        
        if ( lpszEndOfOutputStr <= lpszOutput )
            return FALSE; // bail out, ran out of space
    }

    //
    // Grab Proxy Port if its around.
    //  Back out the ':' if its not.
    //

    GetDlgItemTextA(hDlg, dwProxyPortCtlId,lpszOutput, (int)(lpszEndOfOutputStr - lpszOutput));

    if ( IS_BLANK(lpszOutput) )
    {
        lpszOutput--;
        
        ASSERT(*lpszOutput == ':');

        *lpszOutput = '\0';
    }
    
    lpszOutput += lstrlenA(lpszOutput);
    
    //
    // Now we're done return our final sizes.
    //
    
    *lpdwOutputStrSize = (DWORD)(lpszOutput - lpszOutputStr);

    return TRUE;
}

BOOL
RemoveLocalFromExceptionList(
    IN LPTSTR lpszExceptionList
    )

/*++

Routine Description:

    Scans a delimited list of entries, and removed "<local>
    if found.  If <local> is found we return TRUE.

Arguments:

    lpszExceptionList - String List of proxy excepion entries.


Return Value:

    BOOL
    TRUE - If found <local>

    FALSE - If local was not found.


--*/

{
    LPTSTR lpszLocalInstToRemove;
    BOOL   fFoundLocal;

    if ( !lpszExceptionList || ! *lpszExceptionList )
    return FALSE;

    fFoundLocal = FALSE;
    lpszLocalInstToRemove = lpszExceptionList;

    //
    // Loop looking "<local>" entries in the list.
    //

    do {

        lpszLocalInstToRemove =
                               StrStr(lpszLocalInstToRemove,cszLocalString);
        
        
        if ( lpszLocalInstToRemove )
        {
            
            fFoundLocal = TRUE;
            
            //
            // Nuke <local> out of the string. <local>;otherstuff\0
            //  Dest is: '<'local>;otherstuff\0
            //     ??? (OR) ';'<local> if the ; is the first character.???
            //  Src  is: >'o'therstuff\0
            //  size is: sizeof(';otherstuff\0')
            //
            
            MoveMemory(
                       lpszLocalInstToRemove,
                       (lpszLocalInstToRemove+lstrlen(cszLocalString)),
                       lstrlen(lpszLocalInstToRemove+lstrlen(cszLocalString))*sizeof(TCHAR)+sizeof(TCHAR)
                      );
            
        }
        
    } while (lpszLocalInstToRemove && *lpszLocalInstToRemove);

    //
    // If we produced a ; on the end, nuke it.
    //

    lpszLocalInstToRemove = lpszExceptionList;

    GET_TERMINATOR(lpszLocalInstToRemove);

    if ( lpszLocalInstToRemove != lpszExceptionList &&
         *(lpszLocalInstToRemove-1) == ';' )
    {
        *(lpszLocalInstToRemove-1) = '\0';
    }
    
    return fFoundLocal;
}




/***
*char *strstr(string1, string2) - search for string2 in string1
*
*Purpose:
*   finds the first occurrence of string2 in string1
*   (STOLEN from MSDEV CRT Library)
*
*Entry:
*   char *string1 - string to search in
*   char *string2 - string to search for
*
*Exit:
*   returns a pointer to the first occurrence of string2 in
*   string1, or NULL if string2 does not occur in string1
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

char * __cdecl strstr (
    const char * str1,
    const char * str2
    )
{
    char *cp = (char *) str1;
    char *s1, *s2;

    if ( !*str2 )
        return((char *)str1);

    while (*cp)
    {
        s1 = cp;
        s2 = (char *) str2;

        while ( *s1 && *s2 && !(*s1-*s2) )
            s1++, s2++;

        if (!*s2)
            return(cp);

        cp++;
    }

    return(NULL);

}
