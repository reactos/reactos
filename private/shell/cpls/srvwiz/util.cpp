#include "priv.h"

#include <lm.h>
#include <dnsapi.h>
#include <dsgetdc.h>
#include <netcfgx.h>
#include <netcfgn.h>
#include <netcfgp.h>
#include <netcon.h>
#include <setupapi.h>
#include <devguid.h>
#include <tchar.h>
#include <iphlpapi.h>
#include <mprapi.h>
#include <debug.h>
#include "resource.h"

#define SZ_REGKEY_SRVWIZ        TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SrvWiz")

// dcpromo stuff, copied from \\kernel\razzle2\srv\admin\burnslib and \\kernel\razzle2\srv\admin\dcpromo    
// the number of bytes in a full DNS name to reserve for stuff
// netlogon pre-/appends to DNS names when registering them
#define SRV_RECORD_RESERVE      100

// the max lengths, in bytes, of strings when represented in the UTF-8
// character set.  These are the limits we expose to the user
#define MAX_NAME_LENGTH         DNS_MAX_NAME_LENGTH - SRV_RECORD_RESERVE
#define MAX_LABEL_LENGTH        DNS_MAX_LABEL_LENGTH

// TCP/IP interface prefix, copied from ipconfig.c
#define TCPIP_DEVICE_PREFIX "\\Device\\Tcpip_"

// utility function: 
// remove the leading and trailing spaces then return the modified string

LPWSTR TrimString(LPWSTR pszIn)
{
    LPWSTR pstr;

    if (!pszIn || !*pszIn)
        return pszIn;

    // strip leading spaces
    for (pstr = pszIn; *pstr && *pstr == L' '; pstr++);
    if (pstr != pszIn)
        lstrcpyW(pszIn, pstr);

    // strip trailing spaces
    pstr = pszIn + lstrlen(pszIn) - 1; 
    for (; pstr >= pszIn && *pstr == L' '; pstr--);
    *(pstr + 1) = 0;

    return pszIn;
}


BOOL ValidateDomainDNSName(LPWSTR pszDomainDNSName)
{
    // trim the name
    pszDomainDNSName = TrimString(pszDomainDNSName);
    if (!pszDomainDNSName || !*pszDomainDNSName)
        return FALSE;

    // validate DNS name syntax: length and characters
    size_t cbUTF8 = WideCharToMultiByte(CP_UTF8, 0, pszDomainDNSName, -1, 0, 0, 0, 0);
    if (cbUTF8 > MAX_NAME_LENGTH)
        return FALSE;   // name is too long

    if (ERROR_SUCCESS != DnsValidateName_W(pszDomainDNSName, DnsNameDomain))
        return FALSE;

    // check whether this name is already in use
    DOMAIN_CONTROLLER_INFO* info = 0;
    if (NO_ERROR == DsGetDcName(0, pszDomainDNSName, 0, 0, 0, &info))
    {
        NetApiBufferFree(info);
        return FALSE;
    }

    if (ERROR_DUP_NAME == NetValidateName(0, pszDomainDNSName, 0, 0, NetSetupNonExistentDomain))
        return FALSE;   // duplicate domain name

    return TRUE;
}

BOOL ValidateDomainNetBiosName(LPWSTR pszDomainNetBiosName)
{
    // trim the name
    pszDomainNetBiosName = TrimString(pszDomainNetBiosName);
    if (!pszDomainNetBiosName || !*pszDomainNetBiosName)
        return FALSE;

    // check for "."
    if (StrChrW(pszDomainNetBiosName, L'.'))
        return FALSE;

    WCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD ch = ARRAYSIZE(szComputerName);
    *szComputerName = 0;

    if (!DnsHostnameToComputerNameW(pszDomainNetBiosName, szComputerName, &ch))
        return FALSE;

    if (lstrlenW(szComputerName) < lstrlenW(pszDomainNetBiosName))
        return FALSE;   // name was truncated

    // check the name is not longer than the max bytes in te oem character set
    if (WideCharToMultiByte(CP_OEMCP, 0, szComputerName, ch - 1, 0, 0, 0, 0) > DNLEN)
        return FALSE; 

    // check whether the domain name is in use
    if (ERROR_DUP_NAME == NetValidateName(0, pszDomainNetBiosName, 0, 0, NetSetupNonExistentDomain))
        return FALSE;

    return TRUE;
}


BOOL IsTerminalServicesRunning(void)
{
    OSVERSIONINFOEX osVersionInfo;
    DWORDLONG dwlConditionMask = 0;

    ZeroMemory(&osVersionInfo, sizeof(OSVERSIONINFOEX));
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osVersionInfo.wSuiteMask = VER_SUITE_TERMINAL;

    VER_SET_CONDITION( dwlConditionMask, VER_SUITENAME, VER_AND );

    return VerifyVersionInfo(
        &osVersionInfo,
        VER_SUITENAME,
        dwlConditionMask
        );
}

// copied from \nt\private\cluster\setup\common\VerifyClusteros.cpp

/////////////////////////////////////////////////////////////////////////////
//++
//
// VerifyClusterOS
//
// Routine Description:
//    This function determines whether the OS is correct for installing Clusteringi
//    Service. The current requirements are that the OS be NT 5.0 and the product
//    Suite be Enterprise and not Terminal Server.
//
// Arguments:
//    None
//
// Return Value:
//    TRUE - indicates that the OS is correct and Clustering Service may be installed.
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL VerifyClusterOS( void )
{
   BOOL              fReturnValue;

   OSVERSIONINFOEX   osiv;

   ZeroMemory( &osiv, sizeof( OSVERSIONINFOEX ) );

   osiv.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );

   // The first call to VerifyVersionInfo will test the OS level and that the
   // product suite is Enterprise.
   
   osiv.dwMajorVersion = 5;
   osiv.dwMinorVersion = 0;
   osiv.wServicePackMajor = 0;
   osiv.wSuiteMask = VER_SUITE_ENTERPRISE;

   DWORDLONG   dwlConditionMask;

   dwlConditionMask = (DWORDLONG) 0L;

   VER_SET_CONDITION( dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL );
   VER_SET_CONDITION( dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL );
   VER_SET_CONDITION( dwlConditionMask, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL );
   VER_SET_CONDITION( dwlConditionMask, VER_SUITENAME, VER_AND );

   fReturnValue = VerifyVersionInfo( &osiv,
                                     VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR |
                                     VER_SUITENAME,
                                     dwlConditionMask );
   
   return ( fReturnValue );
}


// copied from \nt\private\cluster\setup\common\IsterminalServiceInstalled.cpp

/////////////////////////////////////////////////////////////////////////////
//++
//
// IsTerminalServicesInstalled
//
// Routine Description:
//    This function determines whether Terminal Services is installed.
//
// Arguments:
//    None
//
// Return Value:
//    TRUE - indicates that Terminal Services is installed.
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL IsTerminalServicesInstalled( void )
{
   BOOL              fReturnValue;

   OSVERSIONINFOEX   osiv;

   ZeroMemory( &osiv, sizeof( OSVERSIONINFOEX ) );

   osiv.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );

   osiv.wSuiteMask = VER_SUITE_TERMINAL;

   DWORDLONG   dwlConditionMask;

   dwlConditionMask = (DWORDLONG) 0L;

   VER_SET_CONDITION( dwlConditionMask, VER_SUITENAME, VER_AND );

   fReturnValue = VerifyVersionInfo( &osiv,
                                     VER_SUITENAME,
                                     dwlConditionMask );
   
   return ( fReturnValue );
}

LPTSTR WINAPI MyFormatString(UINT resId, ...)
{
    TCHAR szFormat[4096];
    LPTSTR pszRet = NULL;
    va_list ArgList;

    // load the format string from resource

    if (!LoadString(HINST_THISDLL, resId, szFormat, ARRAYSIZE(szFormat)))
        return NULL;

    // generate the string from format and arguments

    va_start(ArgList, resId);
    FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER, szFormat, 0, 0, (LPTSTR)&pszRet, 0, &ArgList);
    va_end(ArgList);

    return pszRet;
}

LPSTR WINAPI MyFormatStringAnsi(UINT resId, ...)
{
    TCHAR szFormat[1024];
    LPTSTR pszRet = NULL;
    va_list ArgList;

    // load the format string from resource

    if (!LoadString(HINST_THISDLL, resId, szFormat, ARRAYSIZE(szFormat)))
        return NULL;

    // generate the string from format and arguments

    va_start(ArgList, resId);
    FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER, szFormat, 0, 0, (LPTSTR)&pszRet, 0, &ArgList);
    va_end(ArgList);

#ifdef UNICODE

    // turn pseRet to an Ansi string

    int cch = lstrlenW(pszRet) + 1;
    LPSTR pszTemp = (LPSTR)LocalAlloc(LPTR, cch * sizeof(CHAR));

    if ( pszTemp )
    {
        WideCharToMultiByte(CP_ACP, 0, pszRet, -1, pszTemp, cch, NULL, NULL);
        LocalFree(pszRet);
    }

    return pszTemp;
#else
    return pszRet;
#endif // UNICODE
}


HRESULT CreateTempFile(LPCTSTR szPath, LPSTR szText)
{
   HRESULT hr = S_OK;

   DWORD cbText = lstrlenA(szText) + 1;
   DWORD cbWritten = 0;
   HANDLE h = CreateFile(szPath, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

   if (!h || h == INVALID_HANDLE_VALUE)
      return HRESULT_FROM_WIN32(GetLastError());

   // write eof to file
   szText[cbText-1] = '\032';

   if (!WriteFile(h, szText, cbText, &cbWritten, NULL) || cbText != cbWritten)
      hr = HRESULT_FROM_WIN32(GetLastError());

   szText[cbText-1] = 0;

   CloseHandle(h);

   return hr;
}

BOOL InstallDNS(BOOL bWait)
{
    TCHAR szSysDir[MAX_PATH];
    TCHAR szInfFile[MAX_PATH], szUnattendFile[MAX_PATH];
    LPSTR pszText = NULL;
    LPTSTR pszCmdLine = NULL;

    DWORD dwRet = 0;
    PROCESS_INFORMATION pi = { 0 };
    STARTUPINFO si = { 0 };
    si.cb = sizeof(si);
    
    // initialize directories and file paths

    GetSystemDirectory(szSysDir, ARRAYSIZE(szSysDir));
    GetWindowsDirectory(szInfFile, ARRAYSIZE(szInfFile));
    StrCatN(szInfFile, TEXT("\\inf"), ARRAYSIZE(szInfFile));
    StrCpy(szUnattendFile, szInfFile);

    // create inf file

    StrCatN(szInfFile, TEXT("\\cysdnsi.inf"), ARRAYSIZE(szInfFile));

    pszText = MyFormatStringAnsi(IDS_INSTALL_DNS_INF_TEXT);
    if (!pszText)
        goto ERROR_RETURN;

    if (S_OK != CreateTempFile(szInfFile, pszText))
        goto ERROR_RETURN;
        
    LocalFree(pszText);
    pszText = NULL;

    // create unattend file

    StrCatN(szUnattendFile, TEXT("\\cysdnsu.inf"), 
            ARRAYSIZE(szUnattendFile));

    pszText = MyFormatStringAnsi(IDS_INSTALL_DNS_UNATTEND_TEXT);
    if (!pszText)
        goto ERROR_RETURN;

    if (S_OK != CreateTempFile(szUnattendFile, pszText))
        goto ERROR_RETURN;

    LocalFree(pszText);
    pszText = NULL;

    // create command line

    pszCmdLine = MyFormatString(IDS_INSTALL_DNS_COMMAND_LINE, 
                                szSysDir, szInfFile, szUnattendFile);
    if (!pszCmdLine)
        goto ERROR_RETURN;

    // execute the command 

    if (!CreateProcess(NULL, pszCmdLine, NULL, NULL, 
                       FALSE, 0, NULL, NULL, &si, &pi))
        goto ERROR_RETURN;

    if (bWait)
    {
        dwRet = SHWaitForSendMessageThread(pi.hProcess, INFINITE);
        ASSERT(dwRet == WAIT_OBJECT_0);

        // get the process's return value
        GetExitCodeProcess(pi.hProcess, &dwRet);
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    LocalFree(pszCmdLine);
    pszCmdLine = NULL;

    // return dwRet != 0;    // sysocmgr returns non zero on success   
    return TRUE;


ERROR_RETURN:

    if (pszText)
        LocalFree(pszText);

    if (pszCmdLine)
        LocalFree(pszCmdLine);

    return FALSE;
}

BOOL InstallDHCP(BOOL bWait)
{
    TCHAR szSysDir[MAX_PATH];
    TCHAR szInfFile[MAX_PATH], szUnattendFile[MAX_PATH];
    LPSTR pszText = NULL;
    LPTSTR pszCmdLine = NULL;

    TCHAR szDomainName[MAX_PATH];
    DWORD cbSize;
    DWORD dwRet;

    PROCESS_INFORMATION pi = { 0 };
    STARTUPINFO si = { 0 };
    si.cb = sizeof(si);
    
    // initialize directories and file paths

    GetSystemDirectory(szSysDir, ARRAYSIZE(szSysDir));
    GetWindowsDirectory(szInfFile, ARRAYSIZE(szInfFile));
    StrCatN(szInfFile, TEXT("\\inf"), ARRAYSIZE(szInfFile));
    StrCpy(szUnattendFile, szInfFile);

    // get domain name from registry

    cbSize = sizeof(szDomainName);
    dwRet = SHGetValue(HKEY_LOCAL_MACHINE, SZ_REGKEY_SRVWIZ, 
                       TEXT("DomainDNSName"), NULL, 
                       (LPVOID)szDomainName, &cbSize);
    if (NO_ERROR != dwRet || cbSize <= sizeof(TCHAR))
        goto ERROR_RETURN;

    // get inf file

    StrCatN(szInfFile, TEXT("\\sysoc.inf"), ARRAYSIZE(szInfFile));

    // create unattend file

    StrCatN(szUnattendFile, TEXT("\\cysdhcpu.inf"), 
            ARRAYSIZE(szUnattendFile));
    pszText = MyFormatStringAnsi(IDS_INSTALL_DHCP_UNATTEND_TEXT,
                                 szDomainName);
    if (!pszText)
        goto ERROR_RETURN;

    if (S_OK != CreateTempFile(szUnattendFile, pszText))
        goto ERROR_RETURN;

    LocalFree(pszText);
    pszText = NULL;

    // create command line

    pszCmdLine = MyFormatString(IDS_INSTALL_DHCP_COMMAND_LINE, 
                                szSysDir, szInfFile, szUnattendFile);
    if (!pszCmdLine)
        goto ERROR_RETURN;

    // execute the command 

    if (!CreateProcess(NULL, pszCmdLine, NULL, NULL, 
                       FALSE, 0, NULL, NULL, &si, &pi))
        goto ERROR_RETURN;

    if (bWait)
    {
        dwRet = SHWaitForSendMessageThread(pi.hProcess, INFINITE);
        ASSERT(dwRet == WAIT_OBJECT_0);

        // get the process's return value
        GetExitCodeProcess(pi.hProcess, &dwRet);
    }
        
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    LocalFree(pszCmdLine);
    pszCmdLine = NULL;

    // return dwRet != 0;    // sysocmgr returns non zero on success   
    return TRUE;

ERROR_RETURN:

    if (pszText)
        LocalFree(pszText);

    if (pszCmdLine)
        LocalFree(pszCmdLine);

    return FALSE;
}

BOOL InstallAD(BOOL bWait)
{
    TCHAR szSysDir[MAX_PATH];
    TCHAR szAnswerFile[MAX_PATH];
    LPSTR pszText = NULL;
    LPTSTR pszCmdLine = NULL;

    TCHAR szDomainNetBiosName[MAX_PATH], szDomainDNSName[MAX_PATH];
    DWORD cbSize;
    DWORD dwRet;

    PROCESS_INFORMATION pi = { 0 };
    STARTUPINFO si = { 0 };
    si.cb = sizeof(si);
    
    // initialize directories and file paths

    GetSystemDirectory(szSysDir, ARRAYSIZE(szSysDir));
    GetWindowsDirectory(szAnswerFile, ARRAYSIZE(szAnswerFile));
    StrCatN(szAnswerFile, TEXT("\\inf"), ARRAYSIZE(szAnswerFile));

    // get NetBios name from registry

    cbSize = sizeof(szDomainNetBiosName);
    dwRet = SHGetValue(HKEY_LOCAL_MACHINE, SZ_REGKEY_SRVWIZ,
                       TEXT("DomainNetBiosName"), NULL, 
                       (LPVOID)szDomainNetBiosName, &cbSize);
    if (NO_ERROR != dwRet || cbSize <= sizeof(TCHAR))
        goto ERROR_RETURN;

    // get domain DNS name from registry

    cbSize = sizeof(szDomainDNSName);
    dwRet = SHGetValue(HKEY_LOCAL_MACHINE, SZ_REGKEY_SRVWIZ,
                       TEXT("DomainDNSName"), NULL,
                       (LPVOID)szDomainDNSName, &cbSize);
    if (NO_ERROR != dwRet || cbSize <= sizeof(TCHAR))
        goto ERROR_RETURN;

    // create answer file

    StrCatN(szAnswerFile, TEXT("\\cysad.inf"), ARRAYSIZE(szAnswerFile));
    pszText = MyFormatStringAnsi(IDS_INSTALL_AD_ANSWER_TEXT, 
                                 szDomainNetBiosName, szDomainDNSName);
    if (!pszText)
        goto  ERROR_RETURN;

    if (ERROR_SUCCESS != CreateTempFile(szAnswerFile, pszText))
        goto ERROR_RETURN;

    LocalFree(pszText);
    pszText = NULL;

    // create command line

    pszCmdLine = MyFormatString(IDS_INSTALL_AD_COMMAND_LINE, 
                                szSysDir, szAnswerFile);
    if (!pszCmdLine)
        goto ERROR_RETURN;

    // execute the command 

    if (!CreateProcess(NULL, pszCmdLine, NULL, NULL, 
                       FALSE, 0, NULL, NULL, &si, &pi))
        goto ERROR_RETURN;

    if (bWait)
    {
        dwRet = SHWaitForSendMessageThread(pi.hProcess, INFINITE);
        ASSERT(dwRet == WAIT_OBJECT_0);

        // get the process's return value
        GetExitCodeProcess(pi.hProcess, &dwRet);
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    LocalFree(pszCmdLine);
    pszCmdLine = NULL;

    // return dwRet != 0;    // dcpromo returns non zero on success   
    return TRUE;

ERROR_RETURN:

    if (pszText)
        LocalFree(pszText);

    if (pszCmdLine)
        LocalFree(pszCmdLine);

    return FALSE;
}

// get guid name of the first tcp/ip interface we enum
HRESULT GetTcpIpInterfaceGuidName(LPWSTR wszGuidName, DWORD cchSize)
{
    GUID guid;
    DWORD dwError;
    TCHAR szGuid[128];
    ULONG ulSize = 0;
    PIP_INTERFACE_INFO pInfo = NULL;

    ASSERT(wszGuidName && cchSize);
    
    while( 1 ) {

        dwError = GetInterfaceInfo( pInfo, &ulSize );
        if( ERROR_INSUFFICIENT_BUFFER != dwError ) break;

        if( NULL != pInfo ) LocalFree(pInfo);
        if( 0 == ulSize ) return E_FAIL;

        pInfo = (PIP_INTERFACE_INFO)LocalAlloc(LPTR, ulSize);
        if( NULL == pInfo ) return E_OUTOFMEMORY;

    }

    if( ERROR_SUCCESS != dwError || 0 == pInfo->NumAdapters ) {
        if( NULL != pInfo ) LocalFree(pInfo);
        return E_FAIL;
    }

    StrCpyNW(wszGuidName, pInfo->Adapter[0].Name + strlen(TCPIP_DEVICE_PREFIX), cchSize);

    // check whether this is a valid GUID
    SHUnicodeToTChar(wszGuidName, szGuid, ARRAYSIZE(szGuid));
    if (!GUIDFromString(szGuid, &guid))
    {
        // we failed to get a valid tcp/ip interface
        *wszGuidName = 0;
        LocalFree(pInfo);
        return E_FAIL;
    }

    LocalFree(pInfo);

    return NOERROR;
}

// get friendly name of the first tcp/ip interface we enum
VOID GetTcpIpInterfaceFriendlyName(LPWSTR wszFriendlyName, DWORD cchSize)
{
    DWORD dwRet;
    HRESULT hr;
    WCHAR wszGuidName[128];
    HANDLE hMprConfig;

    *wszFriendlyName = 0;
    
    hr = GetTcpIpInterfaceGuidName(wszGuidName, ARRAYSIZE(wszGuidName));
    if (NOERROR == hr)
    {
        dwRet = MprConfigServerConnect(NULL, &hMprConfig);
        if (NO_ERROR == dwRet)
        {
            dwRet = MprConfigGetFriendlyName(hMprConfig, wszGuidName, 
                            wszFriendlyName, sizeof(WCHAR) * cchSize);
            if (NO_ERROR != dwRet)
            {
                *wszFriendlyName = 0;
            }
        }
    }

    if (!*wszFriendlyName)
    {
        // BUGBUG: we failed to get a friendly name, so use the default one
        StrCpyNW(wszFriendlyName, L"Local Area Connection", cchSize);
    }
}
    

VOID SetStaticIPAddressAndSubnetMask()
{
    DWORD dwRet;
    
    WCHAR wszFriendlyName[128];
    TCHAR szFriendlyName[128];
    TCHAR szCmdLine[256];
    
    PROCESS_INFORMATION pi = { 0 };
    STARTUPINFO si = { 0 };
    si.cb = sizeof(si);

    // get friendly name of tcp/ip interface
    GetTcpIpInterfaceFriendlyName(wszFriendlyName, ARRAYSIZE(wszFriendlyName));
    ASSERT(*wszFriendlyName);

    SHUnicodeToTChar(wszFriendlyName, szFriendlyName, ARRAYSIZE(szFriendlyName));

    // set static IP address and subnet mask
    wsprintf(szCmdLine, TEXT("netsh interface ip set address \"%s\" static 10.10.1.1 255.0.0.0 NONE"), szFriendlyName);
    if (CreateProcess(NULL, szCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        dwRet = SHWaitForSendMessageThread(pi.hProcess, INFINITE);
        ASSERT(dwRet == WAIT_OBJECT_0);

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }

    // set DNS server address for safety
    ZeroMemory(&pi, sizeof(pi));
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    wsprintf(szCmdLine, TEXT("netsh interface ip set dns \"%s\" 10.10.1.1"), szFriendlyName);
    if (CreateProcess(NULL, szCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        dwRet = SHWaitForSendMessageThread(pi.hProcess, INFINITE);
        ASSERT(dwRet == WAIT_OBJECT_0);

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
}
