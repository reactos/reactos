#include "headers.hxx"
#ifndef X_INETREG_H_
#define X_INETREG_H_
#include "inetreg.h"
#endif

#ifndef X_MLANG_H_
#define X_MLANG_H_
#include <mlang.h>
#endif

#ifndef X_MAPI_H_
#define X_MAPI_H_
#include <mapi.h>
#endif

#ifndef X_MAILPROT_HXX_
#define X_MAILPROT_HXX_
#include "mailprot.hxx"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"
#endif

#ifndef X_SHELLAPI_H_
#define X_SHELLAPI_H_
#include <shellapi.h>  // for the definition of ShellExecuteA (for AXP)
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#define URL_KEYWL_MAILTO      7           // length of "mailto:"
#define MAILTO_OPTION         TEXT(" mailto ")

#ifndef NO_IME
extern IMultiLanguage *g_pMultiLanguage;
#endif

#define OE_MAIL_COMMAND_KEY TEXT("Software\\Clients\\Mail\\Outlook Express\\shell\\open\\command")
#define OE_URL_COMMAND_NAME TEXT("URLCommand")
#define MAILTO_URL_HEADER TEXT("mailto:")
#define IE_HOME_ENVIRONMENT TEXT("MWDEV")

HRESULT _UnixLaunchOE(TCHAR *pszRecips, UINT nRecips)
{
    HRESULT     hr = S_OK;
    TCHAR       *tszCommand = NULL;
    TCHAR       tszIEHome[MAX_PATH];
    DWORD       cchIEHome;
    DWORD       cchCommand;
    DWORD       dwDisposition;
    TCHAR       *pchPos;
    BOOL        bMailed;
    STARTUPINFO stInfo;
    HKEY        hkey = NULL;
    int         i;

    cchIEHome = GetEnvironmentVariable(IE_HOME_ENVIRONMENT, tszIEHome, MAX_PATH);
    if (cchIEHome)
    {
        _tcscat(tszIEHome, TEXT("/bin"));
    }
    else
    {
        return E_FAIL;
    }

    hr = RegCreateKeyEx(HKEY_LOCAL_MACHINE, OE_MAIL_COMMAND_KEY, 0, NULL, 0, KEY_READ, NULL, &hkey, &dwDisposition);
    if (hr != ERROR_SUCCESS) 
    {
        goto Cleanup;
    }

    hr = RegQueryValueEx(hkey, OE_URL_COMMAND_NAME, NULL, NULL, (LPBYTE)NULL, &cchCommand);
    if (hr != ERROR_SUCCESS)
    {
        goto Cleanup;
    }
    cchCommand += _tcslen(tszIEHome) + _tcslen(MAILTO_URL_HEADER) + _tcslen(pszRecips) + 1;
    tszCommand = new TCHAR[cchCommand];

    _tcscpy(tszCommand, tszIEHome);
    _tcscat(tszCommand, TEXT("/"));
    dwDisposition = _tcslen(tszCommand);

    hr = RegQueryValueEx(hkey, OE_URL_COMMAND_NAME, NULL, NULL, (LPBYTE)(&tszCommand[dwDisposition]), &cchCommand);
    if (hr != ERROR_SUCCESS)
    {
        goto Cleanup;
    }
    _tcscat(tszCommand, MAILTO_URL_HEADER);

    for (i = 1; i < nRecips; i ++)
        *(pszRecips + _tcslen(pszRecips)) = TEXT(';');
    _tcscat(tszCommand, pszRecips);


    memset(&stInfo, 0, sizeof(stInfo));
    stInfo.cb = sizeof(stInfo);
    bMailed = CreateProcess(NULL, tszCommand, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &stInfo, NULL);

Cleanup:
    if ( hkey != NULL )
        RegCloseKey(hkey);

    if (tszCommand)
        delete(tszCommand);

    return hr;
}

HRESULT
CMailtoProtocol::LaunchUnixClient(TCHAR *aRecips, UINT nRecips)
{
    HRESULT         hr = S_OK;

    TCHAR           tszCommand[pdlUrlLen];
    TCHAR           tszExpandedCommand[pdlUrlLen];
    UINT            nCommandSize;
    int             i;
    HKEY    hkey = NULL;
    DWORD   dw;
    TCHAR *pchPos;
     BOOL bMailed;
    STARTUPINFO stInfo;

    DWORD dwType;
    DWORD dwSize = sizeof(DWORD);
    DWORD dwUseOEMail;

    hr = SHGetValue(IE_USE_OE_MAIL_HKEY, IE_USE_OE_MAIL_KEY, IE_USE_OE_MAIL_VALUE, 
                    &dwType, (void*)&dwUseOEMail, &dwSize);
    if ((hr) && (dwType != REG_DWORD))
    {
        // The default value for mail is FALSE
        dwUseOEMail = FALSE;
    }
    if (dwUseOEMail)
    {
        return _UnixLaunchOE(aRecips, nRecips);
    }

    tszCommand[0] = 0;

    hr = RegCreateKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_MAILCLIENTS, 0, NULL, 0, KEY_READ, NULL, &hkey, &dw);
    if (hr != ERROR_SUCCESS)
        goto Cleanup;
    dw = pdlUrlLen;
    hr = RegQueryValueEx(hkey, REGSTR_PATH_CURRENT, NULL, NULL, (LPBYTE)tszCommand, &dw);
    if (hr != ERROR_SUCCESS)
    {
        RegCloseKey(hkey);
        goto Cleanup;
    }

    dw = ExpandEnvironmentStrings(tszCommand, tszExpandedCommand, pdlUrlLen);
    if (!dw)
     {
        _tcscpy(tszExpandedCommand, tszCommand);
     }
    _tcscpy(tszCommand, TEXT("X "));
    _tcscat(tszCommand, tszExpandedCommand);
    for (i = _tcslen(tszCommand); i > 0; i--)
	if (tszCommand[i] == '/')
	{
	    tszCommand[i] = '\0';
	    break;
	}
    _tcscat(tszCommand, MAILTO_OPTION);
    nCommandSize = _tcslen(tszCommand);
    for (i = 1; i < nRecips; i ++)
	*(aRecips + _tcslen(aRecips)) = ' ';
    _tcscat(tszCommand, aRecips);

    memset(&stInfo, 0, sizeof(stInfo));
    stInfo.cb = sizeof(stInfo);
    bMailed = CreateProcess(tszExpandedCommand, tszCommand, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &stInfo, NULL);
 
Cleanup:

    return hr;
}
