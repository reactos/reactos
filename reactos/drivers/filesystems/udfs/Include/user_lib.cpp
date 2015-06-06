////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////

/*************************************************************************
*
* File: user_lib.cpp
*
* Module: User-mode library
*
* Description: common useful user-mode functions
*
* Author: Ivan
*
*************************************************************************/


#ifndef __USER_LIB_CPP__
#define __USER_LIB_CPP__

#include "user_lib.h"

TCHAR* MediaTypeStrings[] =
{
    "CD-ROM"        ,
    "CD-R"          ,
    "CD-RW"         ,
    "DVD-ROM"       ,
    "DVD-RAM"       ,
    "DVD-R"         ,
    "DVD-RW"        ,
    "DVD+R"         ,
    "DVD+RW"        ,
    "DD CD-ROM"     ,
    "DD CD-R"       ,
    "DD CD-RW"      ,
    "BD-ROM"        ,
    "BD-RE"         ,
    "[BUSY]"        ,
    "Unknown"
};

void * __cdecl mymemchr (
    const void * buf,
    int chr,
    size_t cnt
    )
{
    while ( cnt && (*(unsigned char *)buf != (unsigned char)chr) ) {
        buf = (unsigned char *)buf + 1;
        cnt--;
    }

    return(cnt ? (void *)buf : NULL);
} // end mymemchr()

char * __cdecl  mystrrchr(
    const char * string,
    int ch
    )
{
    char *start = (char *)string;

    while (*string++) {;}

    while (--string != start && *string != (char)ch) {;}

    if (*string == (char)ch) {
        return( (char *)string );
    }

    return(NULL);
} // end mystrrchr()

char * __cdecl  mystrchr(
    const char * string,
    int ch
    )
{
    while (*string != (char)ch && *string != '\0' ) {
        string++;
    }

    if (*string == (char)ch) {
        return( (char *)string );
    }

    return(NULL);
} // end mystrchr()


int __cdecl Exist (
    PCHAR path
    )
{
    DWORD attr;

    attr = GetFileAttributes((LPTSTR)path);
    if (attr  == 0xffffffff) {
            return 0;
    }

    return 1;

} // end Exist()

ULONG
MyMessageBox(
    HINSTANCE hInst,
    HWND hWnd,
    LPCSTR pszFormat,
    LPCSTR pszTitle,
    UINT fuStyle,
    ...
    )
{
    CHAR szTitle[80];
    CHAR szFormat[1024];
    LPSTR pszMessage;
    BOOL fOk;
    int result;
    va_list ArgList;

    if (!HIWORD(pszTitle)) {
        LoadString(hInst, LOWORD(pszTitle), szTitle, sizeof(szTitle)/sizeof(szTitle[0]));
        pszTitle = szTitle;
    }

    if (!HIWORD(pszFormat)) {
        // Allow this to be a resource ID
        LoadString(hInst, LOWORD(pszFormat), szFormat, sizeof(szFormat)/sizeof(szFormat[0]));
        pszFormat = szFormat;
    }

    va_start(ArgList, fuStyle);
    fOk = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                        | FORMAT_MESSAGE_FROM_STRING,
                        pszFormat, 0, 0, (LPTSTR)&pszMessage, 0, &ArgList);

    va_end(ArgList);

    if (fOk && pszMessage) {
        result = MessageBox(hWnd, pszMessage, pszTitle, fuStyle | MB_SETFOREGROUND);
        LocalFree(pszMessage);
    } else {
        return -1;
    }

    return result;
} // end MyMessageBox()


/// Return service status by service name.
JS_SERVICE_STATE
ServiceInfo(
    LPCTSTR ServiceName
    )
{
    SC_HANDLE       schService;
    DWORD           RC;
    SERVICE_STATUS  ssStatus;
    JS_SERVICE_STATE return_value;
    SC_HANDLE       schSCManager;

    schSCManager = OpenSCManager(
                                NULL,                   // machine (NULL == local)
                                NULL,                   // database (NULL == default)
                                SC_MANAGER_ALL_ACCESS   // access required
                               );
    if (!schSCManager) {
        schSCManager = OpenSCManager(
                                    NULL,                   // machine (NULL == local)
                                    NULL,                   // database (NULL == default)
                                    SC_MANAGER_CONNECT   // access required
                                   );
    }
    if (!schSCManager)
        return JS_ERROR_STATUS;
    schService = OpenService(schSCManager, ServiceName, SERVICE_QUERY_STATUS);
    if (!schService) {
        RC = GetLastError();
        CloseServiceHandle(schSCManager);
        if (RC == ERROR_SERVICE_DOES_NOT_EXIST) return JS_SERVICE_NOT_PRESENT;
                                           else return JS_ERROR_STATUS;
    }
    QueryServiceStatus(schService, &ssStatus);
    if(ssStatus.dwCurrentState == SERVICE_RUNNING) {
        return_value = JS_SERVICE_RUNNING;
    } else {
        return_value = JS_SERVICE_NOT_RUNNING;
    }
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    return return_value;
} // end ServiceInfo()

BOOL
CheckCdrwFilter(
    BOOL ReInstall
    )
{
    char CdromUpperFilters[1024];
    bool    found = false;

    if (LOBYTE(LOWORD(GetVersion())) < 5) {
        return true;
    }

    if (GetRegString(CDROM_CLASS_PATH,REG_UPPER_FILTER_NAME,&CdromUpperFilters[0],arraylen(CdromUpperFilters))) {
        char *token = &CdromUpperFilters[0];

        while (*token) {
            if (!strcmp(token,CDRW_SERVICE)) {
                found = true;
                break;
            }
            token += strlen(token)+1;
        }
        if (!found) {
            memcpy(token,CDRW_SERVICE,sizeof(CDRW_SERVICE));
            *(token+sizeof(CDRW_SERVICE)) = '\0';
            *(token+sizeof(CDRW_SERVICE)+1) = '\0';
            if(ReInstall) {
                RegisterString(CDROM_CLASS_PATH,REG_UPPER_FILTER_NAME,&CdromUpperFilters[0],TRUE,token-&CdromUpperFilters[0]+sizeof(CDRW_SERVICE)+1);
                found = true;
            }
        }
    } else {
        memcpy(CdromUpperFilters,CDRW_SERVICE,sizeof(CDRW_SERVICE));
        CdromUpperFilters[sizeof(CDRW_SERVICE)] = '\0';
        CdromUpperFilters[sizeof(CDRW_SERVICE)+1] = '\0';
        if(ReInstall) {
            RegisterString(CDROM_CLASS_PATH,REG_UPPER_FILTER_NAME,&CdromUpperFilters[0],TRUE,sizeof(CDRW_SERVICE)+1);
            found = true;
        }
    }
    return found;

} // end CheckCdrwFilter()

BOOL 
RegisterString(
    LPSTR pszKey, 
    LPSTR pszValue, 
    LPSTR pszData,
    BOOLEAN MultiSz,
    DWORD size
    )
{

    HKEY hKey;
    DWORD dwDisposition;

    // Create the key, if it exists it will be opened
    if (ERROR_SUCCESS != 
        RegCreateKeyEx(
          HKEY_LOCAL_MACHINE,      // handle of an open key 
          pszKey,                  // address of subkey name 
          0,                       // reserved 
          NULL,                    // address of class string 
          REG_OPTION_NON_VOLATILE, // special options flag 
          KEY_ALL_ACCESS,          // desired security access 
          NULL,                    // address of key security structure 
          &hKey,                   // address of buffer for opened handle  
          &dwDisposition))         // address of disposition value buffer 
    {
        return FALSE;
    }

    // Write the value and it's data to the key
    if (ERROR_SUCCESS != 
        RegSetValueEx(
            hKey,                                   // handle of key to set value for  
            pszValue,                               // address of value to set 
            0,                                      // reserved 
            MultiSz ? REG_MULTI_SZ : REG_SZ,        // flag for value type 
            (CONST BYTE *)pszData,                  // address of value data 
            MultiSz ? size : strlen(pszData) ))     // size of value data 
    {
        
        RegCloseKey(hKey);
        return FALSE;
    }

    // Close the key
    RegCloseKey(hKey);
    
    return TRUE;
} // end RegisterString()

BOOL 
RegDelString(
    LPSTR pszKey, 
    LPSTR pszValue
    )
{

    HKEY hKey;
    DWORD dwDisposition;

    // Create the key, if it exists it will be opened
    if (ERROR_SUCCESS != 
        RegCreateKeyEx(
          HKEY_LOCAL_MACHINE,      // handle of an open key 
          pszKey,                  // address of subkey name 
          0,                       // reserved 
          NULL,                    // address of class string 
          REG_OPTION_NON_VOLATILE, // special options flag 
          KEY_ALL_ACCESS,          // desired security access 
          NULL,                    // address of key security structure 
          &hKey,                   // address of buffer for opened handle  
          &dwDisposition))         // address of disposition value buffer 
    {
        return FALSE;
    }

    // Write the value and it's data to the key
    if (ERROR_SUCCESS != 
        RegDeleteValue(
            hKey,                 // handle of key to set value for  
            pszValue))     
    {
        
        RegCloseKey(hKey);
        return FALSE;
    }

    // Close the key
    RegCloseKey(hKey);
    
    return TRUE;
} // end RegDelString()

/// Get string from registry by Key path and Value name
BOOL
GetRegString(
    LPSTR pszKey,
    LPSTR pszValue,
    LPSTR pszData,
    DWORD dwBufSize
    )
{

    HKEY hKey;
    DWORD dwDataSize = dwBufSize;
    DWORD dwValueType = REG_SZ;

    if(!dwBufSize)
        return FALSE;

    RegOpenKeyEx(
       HKEY_LOCAL_MACHINE,    // handle of open key 
       pszKey,                // address of name of subkey to open 
       0,                     // reserved 
       KEY_QUERY_VALUE,       // security access mask 
       &hKey                  // address of handle of open key 
       );

    if (ERROR_SUCCESS != RegQueryValueEx(
        hKey,               // handle of key to query 
        pszValue,           // address of name of value to query 
        0,                  // reserved 
        &dwValueType,       // address of buffer for value type 
        (BYTE *)pszData,    // address of data buffer 
        &dwDataSize         // address of data buffer size 
        )) return FALSE;

    if (pszData[dwDataSize-1] != '\0')
         pszData[dwDataSize-1] = '\0';

    return TRUE;
} // end GetRegString()

BOOL 
RegisterDword(
   LPSTR pszKey, 
   LPSTR pszValue, 
   DWORD dwData
   )
{

    HKEY hKey;
    DWORD dwDisposition;

    // Create the key, if it exists it will be opened
    if (ERROR_SUCCESS != 
        RegCreateKeyEx(
          HKEY_LOCAL_MACHINE,      // handle of an open key 
          pszKey,                  // address of subkey name 
          0,                       // reserved 
          NULL,                    // address of class string 
          REG_OPTION_NON_VOLATILE, // special options flag 
          KEY_ALL_ACCESS,          // desired security access 
          NULL,                    // address of key security structure 
          &hKey,                   // address of buffer for opened handle  
          &dwDisposition))         // address of disposition value buffer 
    {
        return FALSE;
    }

    // Write the value and it's data to the key
    if (ERROR_SUCCESS != 
        RegSetValueEx(
            hKey,                   // handle of key to set value for  
            pszValue,               // address of value to set 
            0,                      // reserved 
            REG_DWORD,              // flag for value type 
            (CONST BYTE *)&dwData,  // address of value data 
            4 ))                    // size of value data 
    {
        
        RegCloseKey(hKey);
        return FALSE;
    }

    // Close the key
    RegCloseKey(hKey);
    
    return TRUE;
} // end RegisterDword()

BOOL
GetRegUlong(
    LPSTR pszKey,
    LPSTR pszValue,
    PULONG pszData
    )
{

    HKEY hKey;
    DWORD dwDataSize = 4;
    DWORD dwValueType = REG_DWORD;
    ULONG origValue = *pszData;

    if(RegOpenKeyEx(
       HKEY_LOCAL_MACHINE,    // handle of open key
       pszKey,                // address of name of subkey to open
       0,                    // reserved
       KEY_QUERY_VALUE,        // security access mask
       &hKey                 // address of handle of open key
        ) != ERROR_SUCCESS) {
        (*pszData) = origValue;
        return FALSE;
    }

    if(RegQueryValueEx(
        hKey,         // handle of key to query
        pszValue,     // address of name of value to query
        0,             // reserved
        &dwValueType,// address of buffer for value type
        (BYTE *)pszData,     // address of data buffer
        &dwDataSize  // address of data buffer size
        ) != ERROR_SUCCESS) {
        (*pszData) = origValue;
    }

    RegCloseKey(hKey);

    return TRUE;
} // end GetRegUlong()

BOOL
SetRegUlong(
  LPSTR pszKey,
  LPSTR pszValue,
  PULONG pszData
  )
{

    HKEY    hKey;
    DWORD   dwDataSize = 4;
    DWORD   dwValueType = REG_DWORD;
    LPVOID  lpMsgBuf;
    LONG    RC;

    if (!(ERROR_SUCCESS == (RC = RegOpenKeyEx(
       HKEY_LOCAL_MACHINE,    // handle of open key
       pszKey,                // address of name of subkey to open
       0,                    // reserved
       KEY_ALL_ACCESS ,        // security access mask
       &hKey                 // address of handle of open key
       )))) {
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            RC,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &lpMsgBuf,
            0,
            NULL);

        // Display the string.
        MessageBox( NULL, (CCHAR*)lpMsgBuf, "Error", MB_OK|MB_ICONHAND );

        // Free the buffer.
        LocalFree( lpMsgBuf );
        return FALSE;
    }

    if (!(ERROR_SUCCESS == (RC = RegSetValueEx(
        hKey,         // handle of key to query
        pszValue,     // address of name of value to query
        0,             // reserved
        REG_DWORD,// address of buffer for value type
        (BYTE *)pszData,     // address of data buffer
        sizeof(ULONG)  // data buffer size
        )))) {
        FormatMessage( 
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            RC,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &lpMsgBuf,
            0,
            NULL);

        // Display the string.
        MessageBox( NULL, (CCHAR*)lpMsgBuf, "Error", MB_OK|MB_ICONHAND );

        // Free the buffer.
        LocalFree( lpMsgBuf );

    }
    RegCloseKey(hKey);
    return TRUE;
} // end SetRegUlong()

BOOL
Privilege(
    LPTSTR pszPrivilege,
    BOOL bEnable
    )
{
   
    HANDLE           hToken;
    TOKEN_PRIVILEGES tp;

    //
    // obtain the token, first check the thread and then the process
    //
    if (!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, TRUE, &hToken)){
        if (GetLastError() == ERROR_NO_TOKEN) {
            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
                return FALSE;
        }
        else return FALSE;
    }

    //
    // get the luid for the privilege
    //
    if (!LookupPrivilegeValue(NULL, pszPrivilege, &tp.Privileges[0].Luid))
         return FALSE;

    tp.PrivilegeCount = 1;

    if (bEnable)
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    else
        tp.Privileges[0].Attributes = 0;

    //
    // enable or disable the privilege
    //
    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, 0, (PTOKEN_PRIVILEGES)NULL, 0))
        return FALSE;

    if (!CloseHandle(hToken))
        return FALSE;

    return TRUE;
} // end Privilege()

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

 
BOOL IsWow64(VOID)
{
    BOOL bIsWow64 = FALSE;
    LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle("kernel32"),"IsWow64Process");
 
    if (NULL != fnIsWow64Process)
    {
        if (!fnIsWow64Process(GetCurrentProcess(),&bIsWow64))
        {
            return FALSE;
        }
    }
    return bIsWow64;
} // end IsWow64()


HANDLE
CreatePublicEvent(
    PWCHAR EventName
    )
{
    SECURITY_DESCRIPTOR sdPublic;
    SECURITY_ATTRIBUTES saPublic;

    InitializeSecurityDescriptor(
        &sdPublic,
        SECURITY_DESCRIPTOR_REVISION
        );

    SetSecurityDescriptorDacl(
        &sdPublic,
        TRUE,
        NULL,
        FALSE
        );

    saPublic.nLength = sizeof(saPublic);
    saPublic.lpSecurityDescriptor = &sdPublic;

    return CreateEventW(
        &saPublic,
        TRUE,
        FALSE,
        EventName);

} // end CreatePublicEvent()

/// Send Device IO Controls to undelaying level via handle
ULONG
UDFPhSendIOCTL(
    IN ULONG IoControlCode,
    IN HANDLE DeviceObject,
    IN PVOID InputBuffer ,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer ,
    IN ULONG OutputBufferLength,
    IN BOOLEAN OverrideVerify,
    IN PVOID Dummy
    )
{
    ULONG real_read;
    ULONG ret;
    LONG offh=0;
    ULONG RC = DeviceIoControl(DeviceObject,IoControlCode,
                                InputBuffer,InputBufferLength,
                                OutputBuffer,OutputBufferLength,
                                &real_read,NULL);

    if (!RC) {
        ret = GetLastError();
    }
    return RC ? 1 : -1;
} // end UDFPhSendIOCTL()

CHAR    RealDeviceName[MAX_PATH+1];

PCHAR
UDFGetDeviceName(
    PCHAR szDeviceName
    )
{
    HANDLE      hDevice;
    WCHAR       DeviceName[MAX_PATH+1];
    ULONG       RC;

    ODS("  UDFGetDeviceName\r\n");
    hDevice = CreateFile(szDeviceName, GENERIC_READ ,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,  NULL);
    
    if (hDevice == ((HANDLE)-1)) {
        strcpy(RealDeviceName,"");
        return (PCHAR)&RealDeviceName;
    }

    RC = UDFPhSendIOCTL(IOCTL_CDRW_GET_DEVICE_NAME,hDevice,
                        &DeviceName,(MAX_PATH+1)*sizeof(WCHAR),
                        &DeviceName,(MAX_PATH+1)*sizeof(WCHAR), FALSE,NULL);

    if(RC == 1) {
        wcstombs((PCHAR)&RealDeviceName,&DeviceName[1],(USHORT)DeviceName[0]);
        RealDeviceName[(USHORT)DeviceName[0]/sizeof(USHORT)] = '\0';
    } else {
        strcpy(RealDeviceName, szDeviceName+4);
    }

    CloseHandle(hDevice);

    return (PCHAR)(strrchr(RealDeviceName, '\\')+1);
} // end UDFGetDeviceName()

BOOL
GetOptUlong(
    PCHAR Path,
    PCHAR OptName,
    PULONG OptVal
    )
{
    if(!Path) {
        return FALSE;
    }
    if(Path[0] && Path[1] == ':') { 
        CHAR SettingFile[MAX_PATH];
        CHAR Setting[16];

        sprintf(SettingFile, "%s\\%s", Path, UDF_CONFIG_STREAM_NAME);
        GetPrivateProfileString("DiskSettings", OptName, "d", &Setting[0], 10, SettingFile);
        Setting[15]=0;
        if (Setting[0] != 'd') {
            if(Setting[0] == '0' && Setting[1] == 'x') {
                sscanf(Setting+2, "%x", OptVal);
            } else {
                sscanf(Setting, "%d", OptVal);
            }
            return TRUE;
        }
        return FALSE;
    }
    return GetRegUlong(Path, OptName, OptVal);
} // end GetOptUlong()

BOOL
SetOptUlong(
    PCHAR Path,
    PCHAR OptName,
    PULONG OptVal
    )
{
    if(!Path) {
        return FALSE;
    }
    if(Path[0] && Path[1] == ':') { 
        CHAR SettingFile[MAX_PATH];
        CHAR Setting[16];
        if(Path[2] != '\\') {
            sprintf(SettingFile, "%s\\%s", Path, UDF_CONFIG_STREAM_NAME);
        } else {
            sprintf(SettingFile, "%s%s", Path, UDF_CONFIG_STREAM_NAME);
        }
        sprintf(Setting, "%d\n", (*OptVal));
        return WritePrivateProfileString("DiskSettings", OptName, Setting, SettingFile);
    }
    return SetRegUlong(Path, OptName, OptVal);
} // end SetOptUlong()

ULONG
UDFGetOptUlongInherited(
    PCHAR Drive,
    PCHAR OptName,
    PULONG OptVal,
    ULONG CheckDepth
    )
{
    CHAR    LocalPath[1024];
    ULONG   retval = 0;

    ODS("  UDFGetOptUlongInherited\r\n");

    if(GetOptUlong(UDF_SERVICE_PARAM_PATH, OptName, OptVal)) {
        retval = UDF_OPTION_GLOBAL;
    }
    if(CheckDepth <= UDF_OPTION_GLOBAL) {
        return retval;
    }
    strcpy(LocalPath,UDF_SERVICE_PARAM_PATH);
    strcat(LocalPath,"\\");
    strcat(LocalPath,UDFGetDeviceName(Drive));
    if(GetOptUlong(LocalPath, OptName, OptVal)) {
        retval = UDF_OPTION_DEVSPEC;
    }
    if(CheckDepth <= UDF_OPTION_DEVSPEC) {
        return retval;
    }
    if(GetOptUlong(Drive, OptName, OptVal))
        retval = UDF_OPTION_DISKSPEC;
    return retval;
} // end UDFGetOptUlongInherited()

HANDLE
OpenOurVolume(
    PCHAR szDeviceName
    )
{
    HANDLE hDevice;

    // Open device volume
    hDevice = CreateFile(szDeviceName, GENERIC_READ ,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING, 
                             FILE_ATTRIBUTE_NORMAL,  NULL);

    if (hDevice == ((HANDLE)-1)) {   
        hDevice = CreateFile(szDeviceName, GENERIC_READ ,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL,
                                 OPEN_EXISTING, 
                                 FILE_ATTRIBUTE_NORMAL,  NULL);

    }
    return hDevice;
} // end OpenOurVolume()

ULONG
drv_letter_to_index(
    WCHAR a
    )
{
    if(a >= 'a' && a <= 'z') {
        return a - 'a';
    }
    if(a >= 'A' && a <= 'Z') {
        return a - 'A';
    }
    return -1;
} // end drv_letter_to_index()

/// Start app with desired parameters
DWORD
WINAPI
LauncherRoutine2(
    LPVOID lpParameter
    )
{
    PCHAR ParamStr = (PCHAR)lpParameter;
    STARTUPINFO proc_startup_info;
    PROCESS_INFORMATION proc_info;
    CHAR szLaunchPath[MAX_PATH],ErrMes[50];
    INT  index;
    ULONG MkUdfRetCode;
    CHAR szTemp[256];

    GetRegString(UDF_KEY,"ToolsPath",szLaunchPath, sizeof(szLaunchPath));
    SetCurrentDirectory(szLaunchPath);

    strcat(szLaunchPath,"\\");
    //strcat(szLaunchPath,UDFFMT);
    strcat(szLaunchPath,ParamStr);

    //strcpy(MkUdfStatus,"");

#ifndef TESTMODE
    proc_startup_info.cb = sizeof(proc_startup_info);
    proc_startup_info.lpReserved = 0;
    proc_startup_info.lpReserved2 = 0;
    proc_startup_info.cbReserved2 = 0;
    proc_startup_info.lpDesktop = 0;
    proc_startup_info.lpTitle = 0;
    proc_startup_info.dwFlags = 0;

    proc_startup_info.hStdInput = NULL;
    proc_startup_info.hStdOutput = NULL;
    proc_startup_info.hStdError = NULL;

    if(CreateProcess(NULL, szLaunchPath, 0,0, TRUE, CREATE_NO_WINDOW | NORMAL_PRIORITY_CLASS,
                  0,0, &proc_startup_info, &proc_info)) {

        //hFmtProc[i] = proc_info.hProcess;
        WaitForSingleObject(proc_info.hProcess, -1);
        GetExitCodeProcess(proc_info.hProcess, &MkUdfRetCode);
        index=0;
/*
        while (mkudf_err_msg[index].err_code != 0xffffffff){
          if (mkudf_err_msg[index].err_code == MkUdfRetCode) break;
          index++;
        } 
*/
        //strcpy(MkUdfStatus,mkudf_err_msg[index].err_msg);
        
        CloseHandle(proc_info.hThread);
        CloseHandle(proc_info.hProcess);
    } else {
        strcpy(ErrMes,"Stop: Cannot launch ");
        strcat(ErrMes,szLaunchPath);
        sprintf(szTemp," error %d",GetLastError());
        strcat(ErrMes,szTemp);
        MessageBox(NULL,ErrMes,"UDFFormat",MB_OK | MB_ICONHAND);

    }
#else
        MessageBox(NULL,szLaunchPath,"Message",MB_OK);
        Sleep(500);
        MkUdfRetCode = MKUDF_OK;
//        MkUdfRetCode = MKUDF_FORMAT_REQUIRED;
//        MkUdfRetCode = MKUDF_CANT_BLANK;

        index = 0;
        while (mkudf_err_msg[index].err_code != 0xffffffff){
          if (mkudf_err_msg[index].err_code == MkUdfRetCode) break;
          index++;
        } 
        //strcpy(MkUdfStatus,mkudf_err_msg[index].err_msg);

#endif
    return MkUdfRetCode;
} // end LauncherRoutine2()


#endif // __USER_LIB_CPP__
