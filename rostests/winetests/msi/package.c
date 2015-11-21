/*
 * tests for Microsoft Installer functionality
 *
 * Copyright 2005 Mike McCormack for CodeWeavers
 * Copyright 2005 Aric Stewart for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define COBJMACROS

#include <stdio.h>
#include <windows.h>
#include <msidefs.h>
#include <msi.h>
#include <msiquery.h>
#include <srrestoreptapi.h>
#include <shlobj.h>

#include "wine/test.h"

static BOOL is_wow64;
static const char msifile[] = "winetest-package.msi";
static const WCHAR msifileW[] =
    {'w','i','n','e','t','e','s','t','-','p','a','c','k','a','g','e','.','m','s','i',0};
static char CURR_DIR[MAX_PATH];

static UINT (WINAPI *pMsiApplyMultiplePatchesA)(LPCSTR, LPCSTR, LPCSTR);
static INSTALLSTATE (WINAPI *pMsiGetComponentPathExA)(LPCSTR, LPCSTR, LPCSTR, MSIINSTALLCONTEXT, LPSTR, LPDWORD);
static HRESULT (WINAPI *pSHGetFolderPathA)(HWND, int, HANDLE, DWORD, LPSTR);

static BOOL (WINAPI *pCheckTokenMembership)(HANDLE,PSID,PBOOL);
static BOOL (WINAPI *pConvertSidToStringSidA)(PSID, LPSTR*);
static BOOL (WINAPI *pOpenProcessToken)( HANDLE, DWORD, PHANDLE );
static LONG (WINAPI *pRegDeleteKeyExA)(HKEY, LPCSTR, REGSAM, DWORD);
static LONG (WINAPI *pRegDeleteKeyExW)(HKEY, LPCWSTR, REGSAM, DWORD);
static BOOL (WINAPI *pIsWow64Process)(HANDLE, PBOOL);
static void (WINAPI *pGetSystemInfo)(LPSYSTEM_INFO);
static void (WINAPI *pGetNativeSystemInfo)(LPSYSTEM_INFO);
static UINT (WINAPI *pGetSystemWow64DirectoryA)(LPSTR, UINT);

static BOOL (WINAPI *pSRRemoveRestorePoint)(DWORD);
static BOOL (WINAPI *pSRSetRestorePointA)(RESTOREPOINTINFOA*, STATEMGRSTATUS*);

static void init_functionpointers(void)
{
    HMODULE hmsi = GetModuleHandleA("msi.dll");
    HMODULE hadvapi32 = GetModuleHandleA("advapi32.dll");
    HMODULE hkernel32 = GetModuleHandleA("kernel32.dll");
    HMODULE hshell32 = GetModuleHandleA("shell32.dll");
    HMODULE hsrclient;

#define GET_PROC(mod, func) \
    p ## func = (void*)GetProcAddress(mod, #func);

    GET_PROC(hmsi, MsiApplyMultiplePatchesA);
    GET_PROC(hmsi, MsiGetComponentPathExA);
    GET_PROC(hshell32, SHGetFolderPathA);

    GET_PROC(hadvapi32, CheckTokenMembership);
    GET_PROC(hadvapi32, ConvertSidToStringSidA);
    GET_PROC(hadvapi32, OpenProcessToken);
    GET_PROC(hadvapi32, RegDeleteKeyExA)
    GET_PROC(hadvapi32, RegDeleteKeyExW)
    GET_PROC(hkernel32, IsWow64Process)
    GET_PROC(hkernel32, GetNativeSystemInfo)
    GET_PROC(hkernel32, GetSystemInfo)
    GET_PROC(hkernel32, GetSystemWow64DirectoryA)

    hsrclient = LoadLibraryA("srclient.dll");
    GET_PROC(hsrclient, SRRemoveRestorePoint);
    GET_PROC(hsrclient, SRSetRestorePointA);
#undef GET_PROC
}

static BOOL is_process_limited(void)
{
    SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};
    PSID Group = NULL;
    BOOL IsInGroup;
    HANDLE token;

    if (!pCheckTokenMembership || !pOpenProcessToken) return FALSE;

    if (!AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &Group) ||
        !pCheckTokenMembership(NULL, Group, &IsInGroup))
    {
        trace("Could not check if the current user is an administrator\n");
        FreeSid(Group);
        return FALSE;
    }
    FreeSid(Group);

    if (!IsInGroup)
    {
        if (!AllocateAndInitializeSid(&NtAuthority, 2,
                                      SECURITY_BUILTIN_DOMAIN_RID,
                                      DOMAIN_ALIAS_RID_POWER_USERS,
                                      0, 0, 0, 0, 0, 0, &Group) ||
            !pCheckTokenMembership(NULL, Group, &IsInGroup))
        {
            trace("Could not check if the current user is a power user\n");
            return FALSE;
        }
        if (!IsInGroup)
        {
            /* Only administrators and power users can be powerful */
            return TRUE;
        }
    }

    if (pOpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
    {
        BOOL ret;
        TOKEN_ELEVATION_TYPE type = TokenElevationTypeDefault;
        DWORD size;

        ret = GetTokenInformation(token, TokenElevationType, &type, sizeof(type), &size);
        CloseHandle(token);
        return (ret && type == TokenElevationTypeLimited);
    }
    return FALSE;
}

static LONG delete_key( HKEY key, LPCSTR subkey, REGSAM access )
{
    if (pRegDeleteKeyExA)
        return pRegDeleteKeyExA( key, subkey, access, 0 );
    return RegDeleteKeyA( key, subkey );
}

static char *get_user_sid(void)
{
    HANDLE token;
    DWORD size = 0;
    TOKEN_USER *user;
    char *usersid = NULL;

    if (!pConvertSidToStringSidA)
    {
        win_skip("ConvertSidToStringSidA is not available\n");
        return NULL;
    }
    OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token);
    GetTokenInformation(token, TokenUser, NULL, size, &size);

    user = HeapAlloc(GetProcessHeap(), 0, size);
    GetTokenInformation(token, TokenUser, user, size, &size);
    pConvertSidToStringSidA(user->User.Sid, &usersid);
    HeapFree(GetProcessHeap(), 0, user);

    CloseHandle(token);
    return usersid;
}

/* RegDeleteTreeW from dlls/advapi32/registry.c */
static LSTATUS package_RegDeleteTreeW(HKEY hKey, LPCWSTR lpszSubKey, REGSAM access)
{
    LONG ret;
    DWORD dwMaxSubkeyLen, dwMaxValueLen;
    DWORD dwMaxLen, dwSize;
    WCHAR szNameBuf[MAX_PATH], *lpszName = szNameBuf;
    HKEY hSubKey = hKey;

    if(lpszSubKey)
    {
        ret = RegOpenKeyExW(hKey, lpszSubKey, 0, access, &hSubKey);
        if (ret) return ret;
    }

    ret = RegQueryInfoKeyW(hSubKey, NULL, NULL, NULL, NULL,
            &dwMaxSubkeyLen, NULL, NULL, &dwMaxValueLen, NULL, NULL, NULL);
    if (ret) goto cleanup;

    dwMaxSubkeyLen++;
    dwMaxValueLen++;
    dwMaxLen = max(dwMaxSubkeyLen, dwMaxValueLen);
    if (dwMaxLen > sizeof(szNameBuf)/sizeof(WCHAR))
    {
        /* Name too big: alloc a buffer for it */
        if (!(lpszName = HeapAlloc( GetProcessHeap(), 0, dwMaxLen*sizeof(WCHAR))))
        {
            ret = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }

    /* Recursively delete all the subkeys */
    while (TRUE)
    {
        dwSize = dwMaxLen;
        if (RegEnumKeyExW(hSubKey, 0, lpszName, &dwSize, NULL,
                          NULL, NULL, NULL)) break;

        ret = package_RegDeleteTreeW(hSubKey, lpszName, access);
        if (ret) goto cleanup;
    }

    if (lpszSubKey)
    {
        if (pRegDeleteKeyExW)
            ret = pRegDeleteKeyExW(hKey, lpszSubKey, access, 0);
        else
            ret = RegDeleteKeyW(hKey, lpszSubKey);
    }
    else
        while (TRUE)
        {
            dwSize = dwMaxLen;
            if (RegEnumValueW(hKey, 0, lpszName, &dwSize,
                  NULL, NULL, NULL, NULL)) break;

            ret = RegDeleteValueW(hKey, lpszName);
            if (ret) goto cleanup;
        }

cleanup:
    if (lpszName != szNameBuf)
        HeapFree(GetProcessHeap(), 0, lpszName);
    if(lpszSubKey)
        RegCloseKey(hSubKey);
    return ret;
}

static BOOL squash_guid(LPCWSTR in, LPWSTR out)
{
    DWORD i,n=1;
    GUID guid;

    if (FAILED(CLSIDFromString((LPCOLESTR)in, &guid)))
        return FALSE;

    for(i=0; i<8; i++)
        out[7-i] = in[n++];
    n++;
    for(i=0; i<4; i++)
        out[11-i] = in[n++];
    n++;
    for(i=0; i<4; i++)
        out[15-i] = in[n++];
    n++;
    for(i=0; i<2; i++)
    {
        out[17+i*2] = in[n++];
        out[16+i*2] = in[n++];
    }
    n++;
    for( ; i<8; i++)
    {
        out[17+i*2] = in[n++];
        out[16+i*2] = in[n++];
    }
    out[32]=0;
    return TRUE;
}

static void create_test_guid(LPSTR prodcode, LPSTR squashed)
{
    WCHAR guidW[MAX_PATH];
    WCHAR squashedW[MAX_PATH];
    GUID guid;
    HRESULT hr;
    int size;

    hr = CoCreateGuid(&guid);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);

    size = StringFromGUID2(&guid, guidW, MAX_PATH);
    ok(size == 39, "Expected 39, got %d\n", hr);

    WideCharToMultiByte(CP_ACP, 0, guidW, size, prodcode, MAX_PATH, NULL, NULL);
    squash_guid(guidW, squashedW);
    WideCharToMultiByte(CP_ACP, 0, squashedW, -1, squashed, MAX_PATH, NULL, NULL);
}

static void set_component_path(LPCSTR filename, MSIINSTALLCONTEXT context,
                               LPCSTR guid, LPSTR usersid, BOOL dir)
{
    WCHAR guidW[MAX_PATH];
    WCHAR squashedW[MAX_PATH];
    CHAR squashed[MAX_PATH];
    CHAR comppath[MAX_PATH];
    CHAR prodpath[MAX_PATH];
    CHAR path[MAX_PATH];
    LPCSTR prod = NULL;
    HKEY hkey;
    REGSAM access = KEY_ALL_ACCESS;

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    MultiByteToWideChar(CP_ACP, 0, guid, -1, guidW, MAX_PATH);
    squash_guid(guidW, squashedW);
    WideCharToMultiByte(CP_ACP, 0, squashedW, -1, squashed, MAX_PATH, NULL, NULL);

    if (context == MSIINSTALLCONTEXT_MACHINE)
    {
        prod = "3D0DAE300FACA1300AD792060BCDAA92";
        sprintf(comppath,
                "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\"
                "Installer\\UserData\\S-1-5-18\\Components\\%s", squashed);
        lstrcpyA(prodpath,
                 "SOFTWARE\\Classes\\Installer\\"
                 "Products\\3D0DAE300FACA1300AD792060BCDAA92");
    }
    else if (context == MSIINSTALLCONTEXT_USERUNMANAGED)
    {
        prod = "7D2F387510109040002000060BECB6AB";
        sprintf(comppath,
                "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\"
                "Installer\\UserData\\%s\\Components\\%s", usersid, squashed);
        sprintf(prodpath,
                "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\"
                "Installer\\%s\\Installer\\Products\\"
                "7D2F387510109040002000060BECB6AB", usersid);
    }
    else if (context == MSIINSTALLCONTEXT_USERMANAGED)
    {
        prod = "7D2F387510109040002000060BECB6AB";
        sprintf(comppath,
                "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\"
                "Installer\\UserData\\%s\\Components\\%s", usersid, squashed);
        sprintf(prodpath,
                "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\"
                "Installer\\Managed\\%s\\Installer\\Products\\"
                "7D2F387510109040002000060BECB6AB", usersid);
    }

    RegCreateKeyExA(HKEY_LOCAL_MACHINE, comppath, 0, NULL, 0, access, NULL, &hkey, NULL);

    lstrcpyA(path, CURR_DIR);
    lstrcatA(path, "\\");
    if (!dir) lstrcatA(path, filename);

    RegSetValueExA(hkey, prod, 0, REG_SZ, (LPBYTE)path, lstrlenA(path));
    RegCloseKey(hkey);

    RegCreateKeyExA(HKEY_LOCAL_MACHINE, prodpath, 0, NULL, 0, access, NULL, &hkey, NULL);
    RegCloseKey(hkey);
}

static void delete_component_path(LPCSTR guid, MSIINSTALLCONTEXT context, LPSTR usersid)
{
    WCHAR guidW[MAX_PATH];
    WCHAR squashedW[MAX_PATH];
    WCHAR substrW[MAX_PATH];
    CHAR squashed[MAX_PATH];
    CHAR comppath[MAX_PATH];
    CHAR prodpath[MAX_PATH];
    REGSAM access = KEY_ALL_ACCESS;

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    MultiByteToWideChar(CP_ACP, 0, guid, -1, guidW, MAX_PATH);
    squash_guid(guidW, squashedW);
    WideCharToMultiByte(CP_ACP, 0, squashedW, -1, squashed, MAX_PATH, NULL, NULL);

    if (context == MSIINSTALLCONTEXT_MACHINE)
    {
        sprintf(comppath,
                "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\"
                "Installer\\UserData\\S-1-5-18\\Components\\%s", squashed);
        lstrcpyA(prodpath,
                 "SOFTWARE\\Classes\\Installer\\"
                 "Products\\3D0DAE300FACA1300AD792060BCDAA92");
    }
    else if (context == MSIINSTALLCONTEXT_USERUNMANAGED)
    {
        sprintf(comppath,
                "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\"
                "Installer\\UserData\\%s\\Components\\%s", usersid, squashed);
        sprintf(prodpath,
                "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\"
                "Installer\\%s\\Installer\\Products\\"
                "7D2F387510109040002000060BECB6AB", usersid);
    }
    else if (context == MSIINSTALLCONTEXT_USERMANAGED)
    {
        sprintf(comppath,
                "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\"
                "Installer\\UserData\\%s\\Components\\%s", usersid, squashed);
        sprintf(prodpath,
                "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\"
                "Installer\\Managed\\%s\\Installer\\Products\\"
                "7D2F387510109040002000060BECB6AB", usersid);
    }

    MultiByteToWideChar(CP_ACP, 0, comppath, -1, substrW, MAX_PATH);
    package_RegDeleteTreeW(HKEY_LOCAL_MACHINE, substrW, access);

    MultiByteToWideChar(CP_ACP, 0, prodpath, -1, substrW, MAX_PATH);
    package_RegDeleteTreeW(HKEY_LOCAL_MACHINE, substrW, access);
}

static UINT do_query(MSIHANDLE hdb, const char *query, MSIHANDLE *phrec)
{
    MSIHANDLE hview = 0;
    UINT r, ret;

    /* open a select query */
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    if (r != ERROR_SUCCESS)
        return r;
    r = MsiViewExecute(hview, 0);
    if (r != ERROR_SUCCESS)
        return r;
    ret = MsiViewFetch(hview, phrec);
    r = MsiViewClose(hview);
    if (r != ERROR_SUCCESS)
        return r;
    r = MsiCloseHandle(hview);
    if (r != ERROR_SUCCESS)
        return r;
    return ret;
}

static UINT run_query( MSIHANDLE hdb, const char *query )
{
    MSIHANDLE hview = 0;
    UINT r;

    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    if( r != ERROR_SUCCESS )
        return r;

    r = MsiViewExecute(hview, 0);
    if( r == ERROR_SUCCESS )
        r = MsiViewClose(hview);
    MsiCloseHandle(hview);
    return r;
}

static UINT create_component_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `Component` ( "
            "`Component` CHAR(72) NOT NULL, "
            "`ComponentId` CHAR(38), "
            "`Directory_` CHAR(72) NOT NULL, "
            "`Attributes` SHORT NOT NULL, "
            "`Condition` CHAR(255), "
            "`KeyPath` CHAR(72) "
            "PRIMARY KEY `Component`)" );
}

static UINT create_feature_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `Feature` ( "
            "`Feature` CHAR(38) NOT NULL, "
            "`Feature_Parent` CHAR(38), "
            "`Title` CHAR(64), "
            "`Description` CHAR(255), "
            "`Display` SHORT NOT NULL, "
            "`Level` SHORT NOT NULL, "
            "`Directory_` CHAR(72), "
            "`Attributes` SHORT NOT NULL "
            "PRIMARY KEY `Feature`)" );
}

static UINT create_feature_components_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `FeatureComponents` ( "
            "`Feature_` CHAR(38) NOT NULL, "
            "`Component_` CHAR(72) NOT NULL "
            "PRIMARY KEY `Feature_`, `Component_` )" );
}

static UINT create_file_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `File` ("
            "`File` CHAR(72) NOT NULL, "
            "`Component_` CHAR(72) NOT NULL, "
            "`FileName` CHAR(255) NOT NULL, "
            "`FileSize` LONG NOT NULL, "
            "`Version` CHAR(72), "
            "`Language` CHAR(20), "
            "`Attributes` SHORT, "
            "`Sequence` SHORT NOT NULL "
            "PRIMARY KEY `File`)" );
}

static UINT create_remove_file_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `RemoveFile` ("
            "`FileKey` CHAR(72) NOT NULL, "
            "`Component_` CHAR(72) NOT NULL, "
            "`FileName` CHAR(255) LOCALIZABLE, "
            "`DirProperty` CHAR(72) NOT NULL, "
            "`InstallMode` SHORT NOT NULL "
            "PRIMARY KEY `FileKey`)" );
}

static UINT create_appsearch_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `AppSearch` ("
            "`Property` CHAR(72) NOT NULL, "
            "`Signature_` CHAR(72) NOT NULL "
            "PRIMARY KEY `Property`, `Signature_`)" );
}

static UINT create_reglocator_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `RegLocator` ("
            "`Signature_` CHAR(72) NOT NULL, "
            "`Root` SHORT NOT NULL, "
            "`Key` CHAR(255) NOT NULL, "
            "`Name` CHAR(255), "
            "`Type` SHORT "
            "PRIMARY KEY `Signature_`)" );
}

static UINT create_signature_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `Signature` ("
            "`Signature` CHAR(72) NOT NULL, "
            "`FileName` CHAR(255) NOT NULL, "
            "`MinVersion` CHAR(20), "
            "`MaxVersion` CHAR(20), "
            "`MinSize` LONG, "
            "`MaxSize` LONG, "
            "`MinDate` LONG, "
            "`MaxDate` LONG, "
            "`Languages` CHAR(255) "
            "PRIMARY KEY `Signature`)" );
}

static UINT create_launchcondition_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `LaunchCondition` ("
            "`Condition` CHAR(255) NOT NULL, "
            "`Description` CHAR(255) NOT NULL "
            "PRIMARY KEY `Condition`)" );
}

static UINT create_property_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `Property` ("
            "`Property` CHAR(72) NOT NULL, "
            "`Value` CHAR(0) "
            "PRIMARY KEY `Property`)" );
}

static UINT create_install_execute_sequence_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `InstallExecuteSequence` ("
            "`Action` CHAR(72) NOT NULL, "
            "`Condition` CHAR(255), "
            "`Sequence` SHORT "
            "PRIMARY KEY `Action`)" );
}

static UINT create_media_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `Media` ("
            "`DiskId` SHORT NOT NULL, "
            "`LastSequence` SHORT NOT NULL, "
            "`DiskPrompt` CHAR(64), "
            "`Cabinet` CHAR(255), "
            "`VolumeLabel` CHAR(32), "
            "`Source` CHAR(72) "
            "PRIMARY KEY `DiskId`)" );
}

static UINT create_ccpsearch_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `CCPSearch` ("
            "`Signature_` CHAR(72) NOT NULL "
            "PRIMARY KEY `Signature_`)" );
}

static UINT create_drlocator_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `DrLocator` ("
            "`Signature_` CHAR(72) NOT NULL, "
            "`Parent` CHAR(72), "
            "`Path` CHAR(255), "
            "`Depth` SHORT "
            "PRIMARY KEY `Signature_`, `Parent`, `Path`)" );
}

static UINT create_complocator_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `CompLocator` ("
            "`Signature_` CHAR(72) NOT NULL, "
            "`ComponentId` CHAR(38) NOT NULL, "
            "`Type` SHORT "
            "PRIMARY KEY `Signature_`)" );
}

static UINT create_inilocator_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `IniLocator` ("
            "`Signature_` CHAR(72) NOT NULL, "
            "`FileName` CHAR(255) NOT NULL, "
            "`Section` CHAR(96)NOT NULL, "
            "`Key` CHAR(128)NOT NULL, "
            "`Field` SHORT, "
            "`Type` SHORT "
            "PRIMARY KEY `Signature_`)" );
}

static UINT create_custom_action_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `CustomAction` ("
            "`Action` CHAR(72) NOT NULL, "
            "`Type` SHORT NOT NULL, "
            "`Source` CHAR(75), "
            "`Target` CHAR(255) "
            "PRIMARY KEY `Action`)" );
}

#define make_add_entry(type, qtext) \
    static UINT add##_##type##_##entry( MSIHANDLE hdb, const char *values ) \
    { \
        char insert[] = qtext; \
        char *query; \
        UINT sz, r; \
        sz = strlen(values) + sizeof insert; \
        query = HeapAlloc(GetProcessHeap(),0,sz); \
        sprintf(query,insert,values); \
        r = run_query( hdb, query ); \
        HeapFree(GetProcessHeap(), 0, query); \
        return r; \
    }

make_add_entry(component,
               "INSERT INTO `Component`  "
               "(`Component`, `ComponentId`, `Directory_`, "
               "`Attributes`, `Condition`, `KeyPath`) VALUES( %s )")

make_add_entry(directory,
               "INSERT INTO `Directory` "
               "(`Directory`,`Directory_Parent`,`DefaultDir`) VALUES( %s )")

make_add_entry(feature,
               "INSERT INTO `Feature` "
               "(`Feature`, `Feature_Parent`, `Title`, `Description`, "
               "`Display`, `Level`, `Directory_`, `Attributes`) VALUES( %s )")

make_add_entry(feature_components,
               "INSERT INTO `FeatureComponents` "
               "(`Feature_`, `Component_`) VALUES( %s )")

make_add_entry(file,
               "INSERT INTO `File` "
               "(`File`, `Component_`, `FileName`, `FileSize`, "
               "`Version`, `Language`, `Attributes`, `Sequence`) VALUES( %s )")

make_add_entry(appsearch,
               "INSERT INTO `AppSearch` "
               "(`Property`, `Signature_`) VALUES( %s )")

make_add_entry(signature,
               "INSERT INTO `Signature` "
               "(`Signature`, `FileName`, `MinVersion`, `MaxVersion`,"
               " `MinSize`, `MaxSize`, `MinDate`, `MaxDate`, `Languages`) "
               "VALUES( %s )")

make_add_entry(launchcondition,
               "INSERT INTO `LaunchCondition` "
               "(`Condition`, `Description`) VALUES( %s )")

make_add_entry(property,
               "INSERT INTO `Property` (`Property`, `Value`) VALUES( %s )")

make_add_entry(install_execute_sequence,
               "INSERT INTO `InstallExecuteSequence` "
               "(`Action`, `Condition`, `Sequence`) VALUES( %s )")

make_add_entry(media,
               "INSERT INTO `Media` "
               "(`DiskId`, `LastSequence`, `DiskPrompt`, "
               "`Cabinet`, `VolumeLabel`, `Source`) VALUES( %s )")

make_add_entry(ccpsearch,
               "INSERT INTO `CCPSearch` (`Signature_`) VALUES( %s )")

make_add_entry(drlocator,
               "INSERT INTO `DrLocator` "
               "(`Signature_`, `Parent`, `Path`, `Depth`) VALUES( %s )")

make_add_entry(complocator,
               "INSERT INTO `CompLocator` "
               "(`Signature_`, `ComponentId`, `Type`) VALUES( %s )")

make_add_entry(inilocator,
               "INSERT INTO `IniLocator` "
               "(`Signature_`, `FileName`, `Section`, `Key`, `Field`, `Type`) "
               "VALUES( %s )")

make_add_entry(custom_action,
               "INSERT INTO `CustomAction`  "
               "(`Action`, `Type`, `Source`, `Target`) VALUES( %s )")

static UINT add_reglocator_entry( MSIHANDLE hdb, const char *sig, UINT root, const char *path,
                                  const char *name, UINT type )
{
    const char insert[] =
        "INSERT INTO `RegLocator` (`Signature_`, `Root`, `Key`, `Name`, `Type`) "
        "VALUES( '%s', %u, '%s', '%s', %u )";
    char *query;
    UINT sz, r;

    sz = strlen( sig ) + 10 + strlen( path ) + strlen( name ) + 10 + sizeof( insert );
    query = HeapAlloc( GetProcessHeap(), 0, sz );
    sprintf( query, insert, sig, root, path, name, type );
    r = run_query( hdb, query );
    HeapFree( GetProcessHeap(), 0, query );
    return r;
}

static UINT set_summary_info(MSIHANDLE hdb)
{
    UINT res;
    MSIHANDLE suminfo;

    /* build summary info */
    res = MsiGetSummaryInformationA(hdb, NULL, 7, &suminfo);
    ok( res == ERROR_SUCCESS , "Failed to open summaryinfo\n" );

    res = MsiSummaryInfoSetPropertyA(suminfo,2, VT_LPSTR, 0,NULL,
                        "Installation Database");
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetPropertyA(suminfo,3, VT_LPSTR, 0,NULL,
                        "Installation Database");
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetPropertyA(suminfo,4, VT_LPSTR, 0,NULL,
                        "Wine Hackers");
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetPropertyA(suminfo,7, VT_LPSTR, 0,NULL,
                    ";1033");
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetPropertyA(suminfo,9, VT_LPSTR, 0,NULL,
                    "{913B8D18-FBB6-4CAC-A239-C74C11E3FA74}");
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetPropertyA(suminfo, 14, VT_I4, 100, NULL, NULL);
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetPropertyA(suminfo, 15, VT_I4, 0, NULL, NULL);
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoPersist(suminfo);
    ok( res == ERROR_SUCCESS , "Failed to make summary info persist\n" );

    res = MsiCloseHandle( suminfo);
    ok( res == ERROR_SUCCESS , "Failed to close suminfo\n" );

    return res;
}


static MSIHANDLE create_package_db(void)
{
    MSIHANDLE hdb = 0;
    UINT res;

    DeleteFileA(msifile);

    /* create an empty database */
    res = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb );
    ok( res == ERROR_SUCCESS , "Failed to create database %u\n", res );
    if( res != ERROR_SUCCESS )
        return hdb;

    res = MsiDatabaseCommit( hdb );
    ok( res == ERROR_SUCCESS , "Failed to commit database\n" );

    res = set_summary_info(hdb);
    ok( res == ERROR_SUCCESS, "Expected ERROR_SUCCESS got %d\n", res);

    res = run_query( hdb,
            "CREATE TABLE `Directory` ( "
            "`Directory` CHAR(255) NOT NULL, "
            "`Directory_Parent` CHAR(255), "
            "`DefaultDir` CHAR(255) NOT NULL "
            "PRIMARY KEY `Directory`)" );
    ok( res == ERROR_SUCCESS , "Failed to create directory table\n" );

    return hdb;
}

static UINT package_from_db(MSIHANDLE hdb, MSIHANDLE *handle)
{
    UINT res;
    CHAR szPackage[12];
    MSIHANDLE hPackage;

    sprintf(szPackage, "#%u", hdb);
    res = MsiOpenPackageA(szPackage, &hPackage);
    if (res != ERROR_SUCCESS)
    {
        MsiCloseHandle(hdb);
        return res;
    }

    res = MsiCloseHandle(hdb);
    if (res != ERROR_SUCCESS)
    {
        MsiCloseHandle(hPackage);
        return res;
    }

    *handle = hPackage;
    return ERROR_SUCCESS;
}

static void create_test_file(const CHAR *name)
{
    HANDLE file;
    DWORD written;

    file = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Failure to open file %s\n", name);
    WriteFile(file, name, strlen(name), &written, NULL);
    WriteFile(file, "\n", strlen("\n"), &written, NULL);
    CloseHandle(file);
}

typedef struct _tagVS_VERSIONINFO
{
    WORD wLength;
    WORD wValueLength;
    WORD wType;
    WCHAR szKey[1];
    WORD wPadding1[1];
    VS_FIXEDFILEINFO Value;
    WORD wPadding2[1];
    WORD wChildren[1];
} VS_VERSIONINFO;

#define roundoffs(a, b, r) (((BYTE *)(b) - (BYTE *)(a) + ((r) - 1)) & ~((r) - 1))
#define roundpos(a, b, r) (((BYTE *)(a)) + roundoffs(a, b, r))

static BOOL create_file_with_version(const CHAR *name, LONG ms, LONG ls)
{
    VS_VERSIONINFO *pVerInfo;
    VS_FIXEDFILEINFO *pFixedInfo;
    LPBYTE buffer, ofs;
    CHAR path[MAX_PATH];
    DWORD handle, size;
    HANDLE resource;
    BOOL ret = FALSE;

    GetSystemDirectoryA(path, MAX_PATH);
    /* Some dlls can't be updated on Vista/W2K8 */
    lstrcatA(path, "\\version.dll");

    CopyFileA(path, name, FALSE);

    size = GetFileVersionInfoSizeA(path, &handle);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);

    GetFileVersionInfoA(path, 0, size, buffer);

    pVerInfo = (VS_VERSIONINFO *)buffer;
    ofs = (BYTE *)&pVerInfo->szKey[lstrlenW(pVerInfo->szKey) + 1];
    pFixedInfo = (VS_FIXEDFILEINFO *)roundpos(pVerInfo, ofs, 4);

    pFixedInfo->dwFileVersionMS = ms;
    pFixedInfo->dwFileVersionLS = ls;
    pFixedInfo->dwProductVersionMS = ms;
    pFixedInfo->dwProductVersionLS = ls;

    resource = BeginUpdateResourceA(name, FALSE);
    if (!resource)
        goto done;

    if (!UpdateResourceA(resource, (LPCSTR)RT_VERSION, (LPCSTR)MAKEINTRESOURCE(VS_VERSION_INFO),
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), buffer, size))
        goto done;

    if (!EndUpdateResourceA(resource, FALSE))
        goto done;

    ret = TRUE;

done:
    HeapFree(GetProcessHeap(), 0, buffer);
    return ret;
}

static BOOL notify_system_change(DWORD event_type, STATEMGRSTATUS *status)
{
    RESTOREPOINTINFOA spec;

    spec.dwEventType = event_type;
    spec.dwRestorePtType = APPLICATION_INSTALL;
    spec.llSequenceNumber = status->llSequenceNumber;
    lstrcpyA(spec.szDescription, "msitest restore point");

    return pSRSetRestorePointA(&spec, status);
}

static void remove_restore_point(DWORD seq_number)
{
    DWORD res;

    res = pSRRemoveRestorePoint(seq_number);
    if (res != ERROR_SUCCESS)
        trace("Failed to remove the restore point : %08x\n", res);
}

static BOOL is_root(const char *path)
{
    return (isalpha(path[0]) && path[1] == ':' && path[2] == '\\' && !path[3]);
}

static void test_createpackage(void)
{
    MSIHANDLE hPackage = 0;
    UINT res;

    res = package_from_db(create_package_db(), &hPackage);
    if (res == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        DeleteFileA(msifile);
        return;
    }
    ok( res == ERROR_SUCCESS, " Failed to create package %u\n", res );

    res = MsiCloseHandle( hPackage);
    ok( res == ERROR_SUCCESS , "Failed to close package\n" );
    DeleteFileA(msifile);
}

static void test_doaction( void )
{
    MSIHANDLE hpkg;
    UINT r;

    r = MsiDoActionA( -1, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = package_from_db(create_package_db(), &hpkg);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        DeleteFileA(msifile);
        return;
    }
    ok( r == ERROR_SUCCESS, "failed to create package %u\n", r);

    r = MsiDoActionA(hpkg, NULL);
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiDoActionA(0, "boo");
    ok( r == ERROR_INVALID_HANDLE, "wrong return val\n");

    r = MsiDoActionA(hpkg, "boo");
    ok( r == ERROR_FUNCTION_NOT_CALLED, "wrong return val\n");

    MsiCloseHandle( hpkg );
    DeleteFileA(msifile);
}

static void test_gettargetpath_bad(void)
{
    static const WCHAR boo[] = {'b','o','o',0};
    static const WCHAR empty[] = {0};
    char buffer[0x80];
    WCHAR bufferW[0x80];
    MSIHANDLE hpkg;
    DWORD sz;
    UINT r;

    r = package_from_db(create_package_db(), &hpkg);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        DeleteFileA(msifile);
        return;
    }
    ok( r == ERROR_SUCCESS, "failed to create package %u\n", r);

    r = MsiGetTargetPathA( 0, NULL, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiGetTargetPathA( 0, NULL, NULL, &sz );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiGetTargetPathA( 0, "boo", NULL, NULL );
    ok( r == ERROR_INVALID_HANDLE, "wrong return val\n");

    r = MsiGetTargetPathA( 0, "boo", NULL, NULL );
    ok( r == ERROR_INVALID_HANDLE, "wrong return val\n");

    r = MsiGetTargetPathA( hpkg, "boo", NULL, NULL );
    ok( r == ERROR_DIRECTORY, "wrong return val\n");

    r = MsiGetTargetPathA( hpkg, "boo", buffer, NULL );
    ok( r == ERROR_DIRECTORY, "wrong return val\n");

    sz = 0;
    r = MsiGetTargetPathA( hpkg, "", buffer, &sz );
    ok( r == ERROR_DIRECTORY, "wrong return val\n");

    r = MsiGetTargetPathW( 0, NULL, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiGetTargetPathW( 0, NULL, NULL, &sz );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiGetTargetPathW( 0, boo, NULL, NULL );
    ok( r == ERROR_INVALID_HANDLE, "wrong return val\n");

    r = MsiGetTargetPathW( 0, boo, NULL, NULL );
    ok( r == ERROR_INVALID_HANDLE, "wrong return val\n");

    r = MsiGetTargetPathW( hpkg, boo, NULL, NULL );
    ok( r == ERROR_DIRECTORY, "wrong return val\n");

    r = MsiGetTargetPathW( hpkg, boo, bufferW, NULL );
    ok( r == ERROR_DIRECTORY, "wrong return val\n");

    sz = 0;
    r = MsiGetTargetPathW( hpkg, empty, bufferW, &sz );
    ok( r == ERROR_DIRECTORY, "wrong return val\n");

    MsiCloseHandle( hpkg );
    DeleteFileA(msifile);
}

static void query_file_path(MSIHANDLE hpkg, LPCSTR file, LPSTR buff)
{
    UINT r;
    DWORD size;
    MSIHANDLE rec;

    rec = MsiCreateRecord( 1 );
    ok(rec, "MsiCreate record failed\n");

    r = MsiRecordSetStringA( rec, 0, file );
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r );

    size = MAX_PATH;
    r = MsiFormatRecordA( hpkg, rec, buff, &size );
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r );

    MsiCloseHandle( rec );
}

static void test_settargetpath(void)
{
    char tempdir[MAX_PATH+8], buffer[MAX_PATH], file[MAX_PATH];
    DWORD sz;
    MSIHANDLE hpkg;
    UINT r;
    MSIHANDLE hdb;

    hdb = create_package_db();
    ok ( hdb, "failed to create package database\n" );

    r = add_directory_entry( hdb, "'TARGETDIR', '', 'SourceDir'" );
    ok( r == S_OK, "failed to add directory entry: %d\n" , r );

    r = create_component_table( hdb );
    ok( r == S_OK, "cannot create Component table: %d\n", r );

    r = add_component_entry( hdb, "'RootComp', '{83e2694d-0864-4124-9323-6d37630912a1}', 'TARGETDIR', 8, '', 'RootFile'" );
    ok( r == S_OK, "cannot add dummy component: %d\n", r );

    r = add_component_entry( hdb, "'TestComp', '{A3FB59C8-C293-4F7E-B8C5-F0E1D8EEE4E5}', 'TestDir', 0, '', 'TestFile'" );
    ok( r == S_OK, "cannot add test component: %d\n", r );

    r = create_feature_table( hdb );
    ok( r == S_OK, "cannot create Feature table: %d\n", r );

    r = add_feature_entry( hdb, "'TestFeature', '', '', '', 0, 1, '', 0" );
    ok( r == ERROR_SUCCESS, "cannot add TestFeature to Feature table: %d\n", r );

    r = create_feature_components_table( hdb );
    ok( r == S_OK, "cannot create FeatureComponents table: %d\n", r );

    r = add_feature_components_entry( hdb, "'TestFeature', 'RootComp'" );
    ok( r == S_OK, "cannot insert component into FeatureComponents table: %d\n", r );

    r = add_feature_components_entry( hdb, "'TestFeature', 'TestComp'" );
    ok( r == S_OK, "cannot insert component into FeatureComponents table: %d\n", r );

    add_directory_entry( hdb, "'TestParent', 'TARGETDIR', 'TestParent'" );
    add_directory_entry( hdb, "'TestDir', 'TestParent', 'TestDir'" );

    r = create_file_table( hdb );
    ok( r == S_OK, "cannot create File table: %d\n", r );

    r = add_file_entry( hdb, "'RootFile', 'RootComp', 'rootfile.txt', 0, '', '1033', 8192, 1" );
    ok( r == S_OK, "cannot add file to the File table: %d\n", r );

    r = add_file_entry( hdb, "'TestFile', 'TestComp', 'testfile.txt', 0, '', '1033', 8192, 1" );
    ok( r == S_OK, "cannot add file to the File table: %d\n", r );

    r = package_from_db( hdb, &hpkg );
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        DeleteFileA(msifile);
        return;
    }
    ok( r == ERROR_SUCCESS, "failed to create package %u\n", r);

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiDoActionA( hpkg, "CostInitialize");
    ok( r == ERROR_SUCCESS, "cost init failed\n");

    r = MsiDoActionA( hpkg, "FileCost");
    ok( r == ERROR_SUCCESS, "file cost failed\n");

    r = MsiDoActionA( hpkg, "CostFinalize");
    ok( r == ERROR_SUCCESS, "cost finalize failed\n");

    buffer[0] = 0;
    sz = sizeof(buffer);
    r = MsiGetPropertyA( hpkg, "OutOfNoRbDiskSpace", buffer, &sz );
    ok( r == ERROR_SUCCESS, "MsiGetProperty returned %u\n", r );
    trace( "OutOfNoRbDiskSpace = \"%s\"\n", buffer );

    r = MsiSetTargetPathA( 0, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiSetTargetPathA( 0, "boo", "C:\\bogusx" );
    ok( r == ERROR_INVALID_HANDLE, "wrong return val\n");

    r = MsiSetTargetPathA( hpkg, "boo", NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiSetTargetPathA( hpkg, "boo", "c:\\bogusx" );
    ok( r == ERROR_DIRECTORY, "wrong return val\n");

    sz = sizeof tempdir - 1;
    r = MsiGetTargetPathA( hpkg, "TARGETDIR", tempdir, &sz );
    sprintf( file, "%srootfile.txt", tempdir );
    buffer[0] = 0;
    query_file_path( hpkg, "[#RootFile]", buffer );
    ok( r == ERROR_SUCCESS, "failed to get target path: %d\n", r);
    ok( !lstrcmpA(buffer, file), "Expected %s, got %s\n", file, buffer );

    GetTempFileNameA( tempdir, "_wt", 0, buffer );
    sprintf( tempdir, "%s\\subdir", buffer );

    r = MsiSetTargetPathA( hpkg, "TARGETDIR", buffer );
    ok( r == ERROR_SUCCESS || r == ERROR_DIRECTORY,
        "MsiSetTargetPath on file returned %d\n", r );

    r = MsiSetTargetPathA( hpkg, "TARGETDIR", tempdir );
    ok( r == ERROR_SUCCESS || r == ERROR_DIRECTORY,
        "MsiSetTargetPath on 'subdir' of file returned %d\n", r );

    DeleteFileA( buffer );

    r = MsiSetTargetPathA( hpkg, "TARGETDIR", buffer );
    ok( r == ERROR_SUCCESS, "MsiSetTargetPath returned %d\n", r );

    r = GetFileAttributesA( buffer );
    ok ( r == INVALID_FILE_ATTRIBUTES, "file/directory exists after MsiSetTargetPath. Attributes: %08X\n", r );

    r = MsiSetTargetPathA( hpkg, "TARGETDIR", tempdir );
    ok( r == ERROR_SUCCESS, "MsiSetTargetPath on subsubdir returned %d\n", r );

    buffer[0] = 0;
    sz = sizeof buffer - 1;
    lstrcatA( tempdir, "\\" );
    r = MsiGetTargetPathA( hpkg, "TARGETDIR", buffer, &sz );
    ok( r == ERROR_SUCCESS, "failed to get target path: %d\n", r);
    ok( !lstrcmpA(buffer, tempdir), "Expected %s, got %s\n", tempdir, buffer);

    sprintf( file, "%srootfile.txt", tempdir );
    query_file_path( hpkg, "[#RootFile]", buffer );
    ok( !lstrcmpA(buffer, file), "Expected %s, got %s\n", file, buffer);

    buffer[0] = 0;
    sz = sizeof(buffer);
    r = MsiGetPropertyA( hpkg, "TestParent", buffer, &sz );
    ok( r == ERROR_SUCCESS, "MsiGetProperty returned %u\n", r );
    lstrcatA( tempdir, "TestParent\\" );
    ok( !lstrcmpiA(buffer, tempdir), "Expected \"%s\", got \"%s\"\n", tempdir, buffer );

    r = MsiSetTargetPathA( hpkg, "TestParent", "C:\\one\\two" );
    ok( r == ERROR_SUCCESS, "MsiSetTargetPath returned %d\n", r );

    buffer[0] = 0;
    sz = sizeof(buffer);
    r = MsiGetPropertyA( hpkg, "TestParent", buffer, &sz );
    ok( r == ERROR_SUCCESS, "MsiGetProperty returned %u\n", r );
    ok( lstrcmpiA(buffer, "C:\\one\\two\\TestDir\\"),
        "Expected \"C:\\one\\two\\TestDir\\\", got \"%s\"\n", buffer );

    buffer[0] = 0;
    query_file_path( hpkg, "[#TestFile]", buffer );
    ok( !lstrcmpiA(buffer, "C:\\one\\two\\TestDir\\testfile.txt"),
        "Expected C:\\one\\two\\TestDir\\testfile.txt, got %s\n", buffer );

    buffer[0] = 0;
    sz = sizeof buffer - 1;
    r = MsiGetTargetPathA( hpkg, "TestParent", buffer, &sz );
    ok( r == ERROR_SUCCESS, "failed to get target path: %d\n", r);
    ok( !lstrcmpiA(buffer, "C:\\one\\two\\"), "Expected C:\\one\\two\\, got %s\n", buffer);

    r = MsiSetTargetPathA( hpkg, "TestParent", "C:\\one\\two\\three" );
    ok( r == ERROR_SUCCESS, "MsiSetTargetPath returned %d\n", r );

    buffer[0] = 0;
    sz = sizeof buffer - 1;
    r = MsiGetTargetPathA( hpkg, "TestParent", buffer, &sz );
    ok( r == ERROR_SUCCESS, "failed to get target path: %d\n", r);
    ok( !lstrcmpiA(buffer, "C:\\one\\two\\three\\"), "Expected C:\\one\\two\\three\\, got %s\n", buffer);

    r = MsiSetTargetPathA( hpkg, "TestParent", "C:\\\\one\\\\two  " );
    ok( r == ERROR_SUCCESS, "MsiSetTargetPath returned %d\n", r );

    buffer[0] = 0;
    sz = sizeof buffer - 1;
    r = MsiGetTargetPathA( hpkg, "TestParent", buffer, &sz );
    ok( r == ERROR_SUCCESS, "failed to get target path: %d\n", r);
    ok( !lstrcmpiA(buffer, "C:\\one\\two\\"), "Expected \"C:\\one\\two\\\", got %s\n", buffer);

    r = MsiSetTargetPathA( hpkg, "TestParent", "C:\\\\ Program Files \\\\ " );
    ok( r == ERROR_SUCCESS, "MsiSetTargetPath returned %d\n", r );

    buffer[0] = 0;
    sz = sizeof buffer - 1;
    r = MsiGetTargetPathA( hpkg, "TestParent", buffer, &sz );
    ok( r == ERROR_SUCCESS, "failed to get target path: %d\n", r);
    ok( !lstrcmpiA(buffer, "C:\\Program Files\\"), "Expected \"C:\\Program Files\\\", got %s\n", buffer);

    MsiCloseHandle( hpkg );
}

static void test_condition(void)
{
    static const WCHAR cond1[] = {'\"','a',0x30a,'\"','<','\"',0xe5,'\"',0};
    static const WCHAR cond2[] = {'\"','a',0x30a,'\"','>','\"',0xe5,'\"',0};
    static const WCHAR cond3[] = {'\"','a',0x30a,'\"','<','>','\"',0xe5,'\"',0};
    static const WCHAR cond4[] = {'\"','a',0x30a,'\"','=','\"',0xe5,'\"',0};
    MSICONDITION r;
    MSIHANDLE hpkg;

    r = package_from_db(create_package_db(), &hpkg);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        DeleteFileA(msifile);
        return;
    }
    ok( r == ERROR_SUCCESS, "failed to create package %u\n", r);

    r = MsiEvaluateConditionA(0, NULL);
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, NULL);
    ok( r == MSICONDITION_NONE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "");
    ok( r == MSICONDITION_NONE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "-1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "0 = 0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "0 <> 0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "0 = 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "0 > 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "0 ~> 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "1 > 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "1 ~> 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "0 >= 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "0 ~>= 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "1 >= 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "1 ~>= 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "0 < 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "0 ~< 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "1 < 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "1 ~< 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "0 <= 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "0 ~<= 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "1 <= 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "1 ~<= 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "0 >=");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, " ");
    ok( r == MSICONDITION_NONE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "LicView <> \"1\"");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "LicView <> \"0\"");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "LicView <> LicView");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "not 0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "not LicView");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "\"Testing\" ~<< \"Testing\"");
    ok (r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "LicView ~<< \"Testing\"");
    ok (r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "Not LicView ~<< \"Testing\"");
    ok (r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "not \"A\"");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "~not \"A\"");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "\"0\"");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "1 and 2");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "not 0 and 3");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "not 0 and 0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "not 0 or 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "(0)");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "(((((1))))))");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "(((((1)))))");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, " \"A\" < \"B\" ");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, " \"A\" > \"B\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, " \"1\" > \"12\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, " \"100\" < \"21\" ");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "0 < > 0");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "(1<<1) == 2");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, " \"A\" = \"a\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, " \"A\" ~ = \"a\" ");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, " \"A\" ~= \"a\" ");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, " \"A\" ~= 1 ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, " \"A\" = 1 ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, " 1 ~= 1 ");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, " 1 ~= \"1\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, " 1 = \"1\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, " 0 = \"1\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, " 0 < \"100\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, " 100 > \"0\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "1 XOR 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "1 IMP 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "1 IMP 0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "0 IMP 0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "0 EQV 0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "0 EQV 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "1 IMP 1 OR 0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "1 IMPL 1");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "\"ASFD\" >< \"S\" ");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "\"ASFD\" ~>< \"s\" ");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "\"ASFD\" ~>< \"\" ");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "\"ASFD\" ~>< \"sss\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "mm", "5" );

    r = MsiEvaluateConditionA(hpkg, "mm = 5");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "mm < 6");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "mm <= 5");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "mm > 4");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "mm < 12");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "mm = \"5\"");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "0 = \"\"");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "0 AND \"\"");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "1 AND \"\"");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "1 AND \"1\"");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "3 >< 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "3 >< 4");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "NOT 0 AND 0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "NOT 0 AND 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "NOT 1 OR 0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "0 AND 1 OR 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "0 AND 0 OR 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "NOT 0 AND 1 OR 0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "_1 = _1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "( 1 AND 1 ) = 2");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "NOT ( 1 AND 1 )");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "NOT A AND (BBBBBBBBBB=2 OR CCC=1) AND Ddddddddd");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "Installed<>\"\"");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "NOT 1 AND 0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "bandalmael<>0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "bandalmael<0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "bandalmael>0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "bandalmael>=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "bandalmael<=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateConditionA(hpkg, "bandalmael~<>0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "bandalmael", "0" );
    r = MsiEvaluateConditionA(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "bandalmael", "" );
    r = MsiEvaluateConditionA(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "bandalmael", "asdf" );
    r = MsiEvaluateConditionA(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "bandalmael", "0asdf" );
    r = MsiEvaluateConditionA(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "bandalmael", "0 " );
    r = MsiEvaluateConditionA(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "bandalmael", "-0" );
    r = MsiEvaluateConditionA(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "bandalmael", "0000000000000" );
    r = MsiEvaluateConditionA(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "bandalmael", "--0" );
    r = MsiEvaluateConditionA(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "bandalmael", "0x00" );
    r = MsiEvaluateConditionA(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "bandalmael", "-" );
    r = MsiEvaluateConditionA(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "bandalmael", "+0" );
    r = MsiEvaluateConditionA(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "bandalmael", "0.0" );
    r = MsiEvaluateConditionA(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");
    r = MsiEvaluateConditionA(hpkg, "bandalmael<>0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "one", "hi");
    MsiSetPropertyA(hpkg, "two", "hithere");
    r = MsiEvaluateConditionA(hpkg, "one >< two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "one", "hithere");
    MsiSetPropertyA(hpkg, "two", "hi");
    r = MsiEvaluateConditionA(hpkg, "one >< two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "one", "hello");
    MsiSetPropertyA(hpkg, "two", "hi");
    r = MsiEvaluateConditionA(hpkg, "one >< two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "one", "hellohithere");
    MsiSetPropertyA(hpkg, "two", "hi");
    r = MsiEvaluateConditionA(hpkg, "one >< two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "one", "");
    MsiSetPropertyA(hpkg, "two", "hi");
    r = MsiEvaluateConditionA(hpkg, "one >< two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "one", "hi");
    MsiSetPropertyA(hpkg, "two", "");
    r = MsiEvaluateConditionA(hpkg, "one >< two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "one", "");
    MsiSetPropertyA(hpkg, "two", "");
    r = MsiEvaluateConditionA(hpkg, "one >< two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "one", "1234");
    MsiSetPropertyA(hpkg, "two", "1");
    r = MsiEvaluateConditionA(hpkg, "one >< two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "one", "one 1234");
    MsiSetPropertyA(hpkg, "two", "1");
    r = MsiEvaluateConditionA(hpkg, "one >< two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "one", "hithere");
    MsiSetPropertyA(hpkg, "two", "hi");
    r = MsiEvaluateConditionA(hpkg, "one << two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "one", "hi");
    MsiSetPropertyA(hpkg, "two", "hithere");
    r = MsiEvaluateConditionA(hpkg, "one << two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "one", "hi");
    MsiSetPropertyA(hpkg, "two", "hi");
    r = MsiEvaluateConditionA(hpkg, "one << two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "one", "abcdhithere");
    MsiSetPropertyA(hpkg, "two", "hi");
    r = MsiEvaluateConditionA(hpkg, "one << two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "one", "");
    MsiSetPropertyA(hpkg, "two", "hi");
    r = MsiEvaluateConditionA(hpkg, "one << two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "one", "hithere");
    MsiSetPropertyA(hpkg, "two", "");
    r = MsiEvaluateConditionA(hpkg, "one << two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "one", "");
    MsiSetPropertyA(hpkg, "two", "");
    r = MsiEvaluateConditionA(hpkg, "one << two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "one", "1234");
    MsiSetPropertyA(hpkg, "two", "1");
    r = MsiEvaluateConditionA(hpkg, "one << two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "one", "1234 one");
    MsiSetPropertyA(hpkg, "two", "1");
    r = MsiEvaluateConditionA(hpkg, "one << two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "one", "hithere");
    MsiSetPropertyA(hpkg, "two", "there");
    r = MsiEvaluateConditionA(hpkg, "one >> two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "one", "hithere");
    MsiSetPropertyA(hpkg, "two", "hi");
    r = MsiEvaluateConditionA(hpkg, "one >> two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "one", "there");
    MsiSetPropertyA(hpkg, "two", "hithere");
    r = MsiEvaluateConditionA(hpkg, "one >> two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "one", "there");
    MsiSetPropertyA(hpkg, "two", "there");
    r = MsiEvaluateConditionA(hpkg, "one >> two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "one", "abcdhithere");
    MsiSetPropertyA(hpkg, "two", "hi");
    r = MsiEvaluateConditionA(hpkg, "one >> two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "one", "");
    MsiSetPropertyA(hpkg, "two", "there");
    r = MsiEvaluateConditionA(hpkg, "one >> two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "one", "there");
    MsiSetPropertyA(hpkg, "two", "");
    r = MsiEvaluateConditionA(hpkg, "one >> two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "one", "");
    MsiSetPropertyA(hpkg, "two", "");
    r = MsiEvaluateConditionA(hpkg, "one >> two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "one", "1234");
    MsiSetPropertyA(hpkg, "two", "4");
    r = MsiEvaluateConditionA(hpkg, "one >> two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "one", "one 1234");
    MsiSetPropertyA(hpkg, "two", "4");
    r = MsiEvaluateConditionA(hpkg, "one >> two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "MsiNetAssemblySupport", NULL);  /* make sure it's empty */

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"1.1.4322\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport > \"1.1.4322\"");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport >= \"1.1.4322\"");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport <= \"1.1.4322\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport <> \"1.1.4322\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport ~< \"1.1.4322\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"abcd\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"a1.1.4322\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"1.1.4322a\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"0000001.1.4322\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"1.1.4322.1\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"1.1.4322.1.1\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "\"2\" < \"1.1");
    ok( r == MSICONDITION_ERROR, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "\"2\" < \"1.1\"");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "\"2\" < \"12.1\"");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "\"02.1\" < \"2.11\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "\"02.1.1\" < \"2.1\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"1.1\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"1\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"0\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"-1\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"a\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"!\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"!\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"/\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \" \"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"azAZ_\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"a[a]\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"a[a]a\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"[a]\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"[a]a\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"{a}\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"{a\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"[a\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"a{\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"a]\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"A\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    MsiSetPropertyA(hpkg, "MsiNetAssemblySupport", "1.1.4322");
    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"1.1.4322\"");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"1.1.14322\"");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"1.1.5\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"1.1\"");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "MsiNetAssemblySupport < \"1\"");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    MsiSetPropertyA(hpkg, "one", "1");
    r = MsiEvaluateConditionA(hpkg, "one < \"1\"");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetPropertyA(hpkg, "X", "5.0");

    r = MsiEvaluateConditionA(hpkg, "X != \"\"");
    ok( r == MSICONDITION_ERROR, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "X =\"5.0\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "X =\"5.1\"");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "X =\"6.0\"");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "X =\"5.0\" or X =\"5.1\" or X =\"6.0\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "(X =\"5.0\" or X =\"5.1\" or X =\"6.0\")");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "X !=\"\" and (X =\"5.0\" or X =\"5.1\" or X =\"6.0\")");
    ok( r == MSICONDITION_ERROR, "wrong return val (%d)\n", r);

    /* feature doesn't exist */
    r = MsiEvaluateConditionA(hpkg, "&nofeature");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    MsiSetPropertyA(hpkg, "A", "2");
    MsiSetPropertyA(hpkg, "X", "50");

    r = MsiEvaluateConditionA(hpkg, "2 <= X");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "A <= X");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "A <= 50");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    MsiSetPropertyA(hpkg, "X", "50val");

    r = MsiEvaluateConditionA(hpkg, "2 <= X");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "A <= X");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    MsiSetPropertyA(hpkg, "A", "7");
    MsiSetPropertyA(hpkg, "X", "50");

    r = MsiEvaluateConditionA(hpkg, "7 <= X");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "A <= X");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "A <= 50");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    MsiSetPropertyA(hpkg, "X", "50val");

    r = MsiEvaluateConditionA(hpkg, "2 <= X");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionA(hpkg, "A <= X");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionW(hpkg, cond1);
    ok( r == MSICONDITION_TRUE || broken(r == MSICONDITION_FALSE),
        "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionW(hpkg, cond2);
    ok( r == MSICONDITION_FALSE || broken(r == MSICONDITION_TRUE),
        "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionW(hpkg, cond3);
    ok( r == MSICONDITION_TRUE || broken(r == MSICONDITION_FALSE),
        "wrong return val (%d)\n", r);

    r = MsiEvaluateConditionW(hpkg, cond4);
    ok( r == MSICONDITION_FALSE || broken(r == MSICONDITION_TRUE),
        "wrong return val (%d)\n", r);

    MsiCloseHandle( hpkg );
    DeleteFileA(msifile);
}

static BOOL check_prop_empty( MSIHANDLE hpkg, const char * prop)
{
    UINT r;
    DWORD sz;
    char buffer[2];

    sz = sizeof buffer;
    strcpy(buffer,"x");
    r = MsiGetPropertyA( hpkg, prop, buffer, &sz );
    return r == ERROR_SUCCESS && buffer[0] == 0 && sz == 0;
}

static void test_props(void)
{
    MSIHANDLE hpkg, hdb;
    UINT r;
    DWORD sz;
    char buffer[0x100];

    hdb = create_package_db();
    r = run_query( hdb,
            "CREATE TABLE `Property` ( "
            "`Property` CHAR(255) NOT NULL, "
            "`Value` CHAR(255) "
            "PRIMARY KEY `Property`)" );
    ok( r == ERROR_SUCCESS , "Failed\n" );

    r = run_query(hdb,
            "INSERT INTO `Property` "
            "(`Property`, `Value`) "
            "VALUES( 'MetadataCompName', 'Photoshop.dll' )");
    ok( r == ERROR_SUCCESS , "Failed\n" );

    r = package_from_db( hdb, &hpkg );
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        DeleteFileA(msifile);
        return;
    }
    ok( r == ERROR_SUCCESS, "failed to create package %u\n", r);

    /* test invalid values */
    r = MsiGetPropertyA( 0, NULL, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiGetPropertyA( hpkg, NULL, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiGetPropertyA( hpkg, "boo", NULL, NULL );
    ok( r == ERROR_SUCCESS, "wrong return val\n");

    r = MsiGetPropertyA( hpkg, "boo", buffer, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    /* test retrieving an empty/nonexistent property */
    sz = sizeof buffer;
    r = MsiGetPropertyA( hpkg, "boo", NULL, &sz );
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( sz == 0, "wrong size returned\n");

    check_prop_empty( hpkg, "boo");
    sz = 0;
    strcpy(buffer,"x");
    r = MsiGetPropertyA( hpkg, "boo", buffer, &sz );
    ok( r == ERROR_MORE_DATA, "wrong return val\n");
    ok( !strcmp(buffer,"x"), "buffer was changed\n");
    ok( sz == 0, "wrong size returned\n");

    sz = 1;
    strcpy(buffer,"x");
    r = MsiGetPropertyA( hpkg, "boo", buffer, &sz );
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( buffer[0] == 0, "buffer was not changed\n");
    ok( sz == 0, "wrong size returned\n");

    /* set the property to something */
    r = MsiSetPropertyA( 0, NULL, NULL );
    ok( r == ERROR_INVALID_HANDLE, "wrong return val\n");

    r = MsiSetPropertyA( hpkg, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiSetPropertyA( hpkg, "", NULL );
    ok( r == ERROR_SUCCESS, "wrong return val\n");

    /* try set and get some illegal property identifiers */
    r = MsiSetPropertyA( hpkg, "", "asdf" );
    ok( r == ERROR_FUNCTION_FAILED, "wrong return val\n");

    r = MsiSetPropertyA( hpkg, "=", "asdf" );
    ok( r == ERROR_SUCCESS, "wrong return val\n");

    r = MsiSetPropertyA( hpkg, " ", "asdf" );
    ok( r == ERROR_SUCCESS, "wrong return val\n");

    r = MsiSetPropertyA( hpkg, "'", "asdf" );
    ok( r == ERROR_SUCCESS, "wrong return val\n");

    sz = sizeof buffer;
    buffer[0]=0;
    r = MsiGetPropertyA( hpkg, "'", buffer, &sz );
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( !strcmp(buffer,"asdf"), "buffer was not changed\n");

    /* set empty values */
    r = MsiSetPropertyA( hpkg, "boo", NULL );
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( check_prop_empty( hpkg, "boo"), "prop wasn't empty\n");

    r = MsiSetPropertyA( hpkg, "boo", "" );
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( check_prop_empty( hpkg, "boo"), "prop wasn't empty\n");

    /* set a non-empty value */
    r = MsiSetPropertyA( hpkg, "boo", "xyz" );
    ok( r == ERROR_SUCCESS, "wrong return val\n");

    sz = 1;
    strcpy(buffer,"x");
    r = MsiGetPropertyA( hpkg, "boo", buffer, &sz );
    ok( r == ERROR_MORE_DATA, "wrong return val\n");
    ok( buffer[0] == 0, "buffer was not changed\n");
    ok( sz == 3, "wrong size returned\n");

    sz = 4;
    strcpy(buffer,"x");
    r = MsiGetPropertyA( hpkg, "boo", buffer, &sz );
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( !strcmp(buffer,"xyz"), "buffer was not changed\n");
    ok( sz == 3, "wrong size returned\n");

    sz = 3;
    strcpy(buffer,"x");
    r = MsiGetPropertyA( hpkg, "boo", buffer, &sz );
    ok( r == ERROR_MORE_DATA, "wrong return val\n");
    ok( !strcmp(buffer,"xy"), "buffer was not changed\n");
    ok( sz == 3, "wrong size returned\n");

    r = MsiSetPropertyA(hpkg, "SourceDir", "foo");
    ok( r == ERROR_SUCCESS, "wrong return val\n");

    sz = 4;
    r = MsiGetPropertyA(hpkg, "SOURCEDIR", buffer, &sz);
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( !strcmp(buffer,""), "buffer wrong\n");
    ok( sz == 0, "wrong size returned\n");

    sz = 4;
    r = MsiGetPropertyA(hpkg, "SOMERANDOMNAME", buffer, &sz);
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( !strcmp(buffer,""), "buffer wrong\n");
    ok( sz == 0, "wrong size returned\n");

    sz = 4;
    r = MsiGetPropertyA(hpkg, "SourceDir", buffer, &sz);
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( !strcmp(buffer,"foo"), "buffer wrong\n");
    ok( sz == 3, "wrong size returned\n");

    r = MsiSetPropertyA(hpkg, "MetadataCompName", "Photoshop.dll");
    ok( r == ERROR_SUCCESS, "wrong return val\n");

    sz = 0;
    r = MsiGetPropertyA(hpkg, "MetadataCompName", NULL, &sz );
    ok( r == ERROR_SUCCESS, "return wrong\n");
    ok( sz == 13, "size wrong (%d)\n", sz);

    sz = 13;
    r = MsiGetPropertyA(hpkg, "MetadataCompName", buffer, &sz );
    ok( r == ERROR_MORE_DATA, "return wrong\n");
    ok( !strcmp(buffer,"Photoshop.dl"), "buffer wrong\n");

    r = MsiSetPropertyA(hpkg, "property", "value");
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sz = 6;
    r = MsiGetPropertyA(hpkg, "property", buffer, &sz);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok( !strcmp(buffer, "value"), "Expected value, got %s\n", buffer);

    r = MsiSetPropertyA(hpkg, "property", NULL);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sz = 6;
    r = MsiGetPropertyA(hpkg, "property", buffer, &sz);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!buffer[0], "Expected empty string, got %s\n", buffer);

    MsiCloseHandle( hpkg );
    DeleteFileA(msifile);
}

static BOOL find_prop_in_property(MSIHANDLE hdb, LPCSTR prop, LPCSTR val, int len)
{
    MSIHANDLE hview, hrec;
    BOOL found = FALSE;
    CHAR buffer[MAX_PATH];
    DWORD sz;
    UINT r;

    r = MsiDatabaseOpenViewA(hdb, "SELECT * FROM `_Property`", &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");

    if (len < 0) len = lstrlenA(val);

    while (r == ERROR_SUCCESS && !found)
    {
        r = MsiViewFetch(hview, &hrec);
        if (r != ERROR_SUCCESS) break;

        sz = MAX_PATH;
        r = MsiRecordGetStringA(hrec, 1, buffer, &sz);
        if (r == ERROR_SUCCESS && !lstrcmpA(buffer, prop))
        {
            sz = MAX_PATH;
            r = MsiRecordGetStringA(hrec, 2, buffer, &sz);
            if (r == ERROR_SUCCESS && !memcmp(buffer, val, len) && !buffer[len])
            {
                ok(sz == len, "wrong size %u\n", sz);
                found = TRUE;
            }
        }

        MsiCloseHandle(hrec);
    }
    MsiViewClose(hview);
    MsiCloseHandle(hview);
    return found;
}

static void test_property_table(void)
{
    const char *query;
    UINT r;
    MSIHANDLE hpkg, hdb, hrec;
    char buffer[MAX_PATH], package[10];
    DWORD sz;
    BOOL found;

    hdb = create_package_db();
    ok( hdb, "failed to create package\n");

    r = package_from_db(hdb, &hpkg);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        DeleteFileA(msifile);
        return;
    }
    ok( r == ERROR_SUCCESS, "failed to create package %u\n", r);

    MsiCloseHandle(hdb);

    hdb = MsiGetActiveDatabase(hpkg);

    query = "CREATE TABLE `_Property` ( "
        "`foo` INT NOT NULL, `bar` INT LOCALIZABLE PRIMARY KEY `foo`)";
    r = run_query(hdb, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    MsiCloseHandle(hdb);
    MsiCloseHandle(hpkg);
    DeleteFileA(msifile);

    hdb = create_package_db();
    ok( hdb, "failed to create package\n");

    query = "CREATE TABLE `_Property` ( "
        "`foo` INT NOT NULL, `bar` INT LOCALIZABLE PRIMARY KEY `foo`)";
    r = run_query(hdb, query);
    ok(r == ERROR_SUCCESS, "failed to create table\n");

    query = "ALTER `_Property` ADD `foo` INTEGER";
    r = run_query(hdb, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "failed to add column\n");

    query = "ALTER TABLE `_Property` ADD `foo` INTEGER";
    r = run_query(hdb, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "failed to add column\n");

    query = "ALTER TABLE `_Property` ADD `extra` INTEGER";
    r = run_query(hdb, query);
    ok(r == ERROR_SUCCESS, "failed to add column\n");

    sprintf(package, "#%i", hdb);
    r = MsiOpenPackageA(package, &hpkg);
    ok(r != ERROR_SUCCESS, "MsiOpenPackage succeeded\n");
    if (r == ERROR_SUCCESS)
        MsiCloseHandle(hpkg);

    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed %u\n", r);

    hdb = create_package_db();
    ok (hdb, "failed to create package database\n");

    r = create_property_table(hdb);
    ok(r == ERROR_SUCCESS, "cannot create Property table: %d\n", r);

    r = add_property_entry(hdb, "'prop', 'val'");
    ok(r == ERROR_SUCCESS, "cannot add property: %d\n", r);

    r = create_custom_action_table(hdb);
    ok(r == ERROR_SUCCESS, "cannot create CustomAction table: %d\n", r);

    r = add_custom_action_entry( hdb, "'EmbedNull', 51, 'prop2', '[~]np'" );
    ok( r == ERROR_SUCCESS, "cannot add custom action: %d\n", r);

    r = package_from_db(hdb, &hpkg);
    ok(r == ERROR_SUCCESS, "failed to create package %u\n", r);

    MsiCloseHandle(hdb);

    sz = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "prop", buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buffer, "val"), "Expected val, got %s\n", buffer);

    hdb = MsiGetActiveDatabase(hpkg);

    found = find_prop_in_property(hdb, "prop", "val", -1);
    ok(found, "prop should be in the _Property table\n");

    r = add_property_entry(hdb, "'dantes', 'mercedes'");
    ok(r == ERROR_SUCCESS, "cannot add property: %d\n", r);

    query = "SELECT * FROM `_Property` WHERE `Property` = 'dantes'";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    found = find_prop_in_property(hdb, "dantes", "mercedes", -1);
    ok(found == FALSE, "dantes should not be in the _Property table\n");

    sz = MAX_PATH;
    lstrcpyA(buffer, "aaa");
    r = MsiGetPropertyA(hpkg, "dantes", buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!buffer[0], "Expected empty string, got %s\n", buffer);

    r = MsiSetPropertyA(hpkg, "dantes", "mercedes");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    found = find_prop_in_property(hdb, "dantes", "mercedes", -1);
    ok(found == TRUE, "dantes should be in the _Property table\n");

    r = MsiDoActionA( hpkg, "EmbedNull" );
    ok( r == ERROR_SUCCESS, "EmbedNull failed: %d\n", r);

    sz = MAX_PATH;
    memset( buffer, 'a', sizeof(buffer) );
    r = MsiGetPropertyA( hpkg, "prop2", buffer, &sz );
    ok( r == ERROR_SUCCESS, "get property failed: %d\n", r);
    ok( !memcmp( buffer, "\0np", sizeof("\0np") ), "wrong value\n");
    ok( sz == sizeof("\0np") - 1, "got %u\n", sz );

    found = find_prop_in_property(hdb, "prop2", "\0np", 3);
    ok(found == TRUE, "prop2 should be in the _Property table\n");

    MsiCloseHandle(hdb);
    MsiCloseHandle(hpkg);
    DeleteFileA(msifile);
}

static UINT try_query_param( MSIHANDLE hdb, LPCSTR szQuery, MSIHANDLE hrec )
{
    MSIHANDLE htab = 0;
    UINT res;

    res = MsiDatabaseOpenViewA( hdb, szQuery, &htab );
    if( res == ERROR_SUCCESS )
    {
        UINT r;

        r = MsiViewExecute( htab, hrec );
        if( r != ERROR_SUCCESS )
        {
            res = r;
            fprintf(stderr,"MsiViewExecute failed %08x\n", res);
        }

        r = MsiViewClose( htab );
        if( r != ERROR_SUCCESS )
            res = r;

        r = MsiCloseHandle( htab );
        if( r != ERROR_SUCCESS )
            res = r;
    }
    return res;
}

static UINT try_query( MSIHANDLE hdb, LPCSTR szQuery )
{
    return try_query_param( hdb, szQuery, 0 );
}

static void set_summary_str(MSIHANDLE hdb, DWORD pid, LPCSTR value)
{
    MSIHANDLE summary;
    UINT r;

    r = MsiGetSummaryInformationA(hdb, NULL, 1, &summary);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiSummaryInfoSetPropertyA(summary, pid, VT_LPSTR, 0, NULL, value);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiSummaryInfoPersist(summary);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    MsiCloseHandle(summary);
}

static void set_summary_dword(MSIHANDLE hdb, DWORD pid, DWORD value)
{
    MSIHANDLE summary;
    UINT r;

    r = MsiGetSummaryInformationA(hdb, NULL, 1, &summary);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiSummaryInfoSetPropertyA(summary, pid, VT_I4, value, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiSummaryInfoPersist(summary);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    MsiCloseHandle(summary);
}

static void test_msipackage(void)
{
    MSIHANDLE hdb = 0, hpack = 100;
    UINT r;
    const char *query;
    char name[10];

    /* NULL szPackagePath */
    r = MsiOpenPackageA(NULL, &hpack);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* empty szPackagePath */
    r = MsiOpenPackageA("", &hpack);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        return;
    }
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    }

    if (r == ERROR_SUCCESS)
        MsiCloseHandle(hpack);

    /* nonexistent szPackagePath */
    r = MsiOpenPackageA("nonexistent", &hpack);
    ok(r == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", r);

    /* NULL hProduct */
    r = MsiOpenPackageA(msifile, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    name[0]='#';
    name[1]=0;
    r = MsiOpenPackageA(name, &hpack);
    ok(r == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %d\n", r);

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* database exists, but is emtpy */
    sprintf(name, "#%d", hdb);
    r = MsiOpenPackageA(name, &hpack);
    ok(r == ERROR_INSTALL_PACKAGE_INVALID,
       "Expected ERROR_INSTALL_PACKAGE_INVALID, got %d\n", r);

    query = "CREATE TABLE `Property` ( "
            "`Property` CHAR(72), `Value` CHAR(0) "
            "PRIMARY KEY `Property`)";
    r = try_query(hdb, query);
    ok(r == ERROR_SUCCESS, "failed to create Properties table\n");

    query = "CREATE TABLE `InstallExecuteSequence` ("
            "`Action` CHAR(72), `Condition` CHAR(0), `Sequence` INTEGER "
            "PRIMARY KEY `Action`)";
    r = try_query(hdb, query);
    ok(r == ERROR_SUCCESS, "failed to create InstallExecuteSequence table\n");

    /* a few key tables exist */
    sprintf(name, "#%d", hdb);
    r = MsiOpenPackageA(name, &hpack);
    ok(r == ERROR_INSTALL_PACKAGE_INVALID,
       "Expected ERROR_INSTALL_PACKAGE_INVALID, got %d\n", r);

    MsiCloseHandle(hdb);
    DeleteFileA(msifile);

    /* start with a clean database to show what constitutes a valid package */
    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sprintf(name, "#%d", hdb);

    /* The following summary information props must exist:
     *  - PID_REVNUMBER
     *  - PID_PAGECOUNT
     */

    set_summary_dword(hdb, PID_PAGECOUNT, 100);
    r = MsiOpenPackageA(name, &hpack);
    ok(r == ERROR_INSTALL_PACKAGE_INVALID,
       "Expected ERROR_INSTALL_PACKAGE_INVALID, got %d\n", r);

    set_summary_str(hdb, PID_REVNUMBER, "{004757CD-5092-49C2-AD20-28E1CE0DF5F2}");
    r = MsiOpenPackageA(name, &hpack);
    ok(r == ERROR_SUCCESS,
       "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(hpack);
    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static void test_formatrecord2(void)
{
    MSIHANDLE hpkg, hrec ;
    char buffer[0x100];
    DWORD sz;
    UINT r;

    r = package_from_db(create_package_db(), &hpkg);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        DeleteFileA(msifile);
        return;
    }
    ok( r == ERROR_SUCCESS, "failed to create package %u\n", r);

    r = MsiSetPropertyA(hpkg, "Manufacturer", " " );
    ok( r == ERROR_SUCCESS, "set property failed\n");

    hrec = MsiCreateRecord(2);
    ok(hrec, "create record failed\n");

    r = MsiRecordSetStringA( hrec, 0, "[ProgramFilesFolder][Manufacturer]\\asdf");
    ok( r == ERROR_SUCCESS, "format record failed\n");

    buffer[0] = 0;
    sz = sizeof buffer;
    r = MsiFormatRecordA( hpkg, hrec, buffer, &sz );
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS got %d\n", r);

    r = MsiRecordSetStringA(hrec, 0, "[foo][1]");
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS got %d\n", r);
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS got %d\n", r);
    sz = sizeof buffer;
    r = MsiFormatRecordA(hpkg, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"hoo"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "x[~]x");
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS got %d\n", r);
    sz = sizeof buffer;
    r = MsiFormatRecordA(hpkg, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"x"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "[foo.$%}][1]");
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS got %d\n", r);
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS got %d\n", r);
    sz = sizeof buffer;
    r = MsiFormatRecordA(hpkg, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"hoo"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "[\\[]");
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS got %d\n", r);
    sz = sizeof buffer;
    r = MsiFormatRecordA(hpkg, hrec, buffer, &sz);
    ok( sz == 1, "size wrong\n");
    ok( 0 == strcmp(buffer,"["), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    SetEnvironmentVariableA("FOO", "BAR");
    r = MsiRecordSetStringA(hrec, 0, "[%FOO]");
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS got %d\n", r);
    sz = sizeof buffer;
    r = MsiFormatRecordA(hpkg, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"BAR"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "[[1]]");
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS got %d\n", r);
    r = MsiRecordSetStringA(hrec, 1, "%FOO");
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS got %d\n", r);
    sz = sizeof buffer;
    r = MsiFormatRecordA(hpkg, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"BAR"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    MsiCloseHandle( hrec );
    MsiCloseHandle( hpkg );
    DeleteFileA(msifile);
}

static void test_feature_states( UINT line, MSIHANDLE package, const char *feature, UINT error,
                                 INSTALLSTATE expected_state, INSTALLSTATE expected_action, BOOL todo )
{
    UINT r;
    INSTALLSTATE state = 0xdeadbee;
    INSTALLSTATE action = 0xdeadbee;

    r = MsiGetFeatureStateA( package, feature, &state, &action );
    ok( r == error, "%u: expected %d got %d\n", line, error, r );
    if (r == ERROR_SUCCESS)
    {
        ok( state == expected_state, "%u: expected state %d got %d\n",
            line, expected_state, state );
        if (todo) todo_wine
            ok( action == expected_action, "%u: expected action %d got %d\n",
                line, expected_action, action );
        else
            ok( action == expected_action, "%u: expected action %d got %d\n",
                line, expected_action, action );
    }
    else
    {
        ok( state == 0xdeadbee, "%u: expected state 0xdeadbee got %d\n", line, state );
        if (todo) todo_wine
            ok( action == 0xdeadbee, "%u: expected action 0xdeadbee got %d\n", line, action );
        else
            ok( action == 0xdeadbee, "%u: expected action 0xdeadbee got %d\n", line, action );

    }
}

static void test_component_states( UINT line, MSIHANDLE package, const char *component, UINT error,
                                   INSTALLSTATE expected_state, INSTALLSTATE expected_action, BOOL todo )
{
    UINT r;
    INSTALLSTATE state = 0xdeadbee;
    INSTALLSTATE action = 0xdeadbee;

    r = MsiGetComponentStateA( package, component, &state, &action );
    ok( r == error, "%u: expected %d got %d\n", line, error, r );
    if (r == ERROR_SUCCESS)
    {
        ok( state == expected_state, "%u: expected state %d got %d\n",
            line, expected_state, state );
        if (todo) todo_wine
            ok( action == expected_action, "%u: expected action %d got %d\n",
                line, expected_action, action );
        else
            ok( action == expected_action, "%u: expected action %d got %d\n",
                line, expected_action, action );
    }
    else
    {
        ok( state == 0xdeadbee, "%u: expected state 0xdeadbee got %d\n",
            line, state );
        if (todo) todo_wine
            ok( action == 0xdeadbee, "%u: expected action 0xdeadbee got %d\n",
                line, action );
        else
            ok( action == 0xdeadbee, "%u: expected action 0xdeadbee got %d\n",
                line, action );
    }
}

static void test_states(void)
{
    static const char msifile2[] = "winetest2-package.msi";
    static const char msifile3[] = "winetest3-package.msi";
    static const char msifile4[] = "winetest4-package.msi";
    static const WCHAR msifile2W[] =
        {'w','i','n','e','t','e','s','t','2','-','p','a','c','k','a','g','e','.','m','s','i',0};
    static const WCHAR msifile3W[] =
        {'w','i','n','e','t','e','s','t','3','-','p','a','c','k','a','g','e','.','m','s','i',0};
    static const WCHAR msifile4W[] =
        {'w','i','n','e','t','e','s','t','4','-','p','a','c','k','a','g','e','.','m','s','i',0};
    INSTALLSTATE state;
    MSIHANDLE hpkg;
    UINT r;
    MSIHANDLE hdb;

    if (is_process_limited())
    {
        skip("process is limited\n");
        return;
    }

    hdb = create_package_db();
    ok ( hdb, "failed to create package database\n" );

    r = add_directory_entry( hdb, "'TARGETDIR', '', 'SourceDir'");
    ok( r == ERROR_SUCCESS, "cannot add directory: %d\n", r );

    r = create_property_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create Property table: %d\n", r );

    r = add_property_entry( hdb, "'ProductCode', '{7262AC98-EEBD-4364-8CE3-D654F6A425B9}'" );
    ok( r == ERROR_SUCCESS, "cannot add property entry: %d\n", r );

    r = add_property_entry( hdb, "'ProductLanguage', '1033'" );
    ok( r == ERROR_SUCCESS, "cannot add property entry: %d\n", r );

    r = add_property_entry( hdb, "'ProductName', 'MSITEST'" );
    ok( r == ERROR_SUCCESS, "cannot add property entry: %d\n", r );

    r = add_property_entry( hdb, "'ProductVersion', '1.1.1'" );
    ok( r == ERROR_SUCCESS, "cannot add property entry: %d\n", r );

    r = add_property_entry( hdb, "'MSIFASTINSTALL', '1'" );
    ok( r == ERROR_SUCCESS, "cannot add property entry: %d\n", r );

    r = create_install_execute_sequence_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create InstallExecuteSequence table: %d\n", r );

    r = add_install_execute_sequence_entry( hdb, "'CostInitialize', '', '800'" );
    ok( r == ERROR_SUCCESS, "cannot add install execute sequence entry: %d\n", r );

    r = add_install_execute_sequence_entry( hdb, "'FileCost', '', '900'" );
    ok( r == ERROR_SUCCESS, "cannot add install execute sequence entry: %d\n", r );

    r = add_install_execute_sequence_entry( hdb, "'CostFinalize', '', '1000'" );
    ok( r == ERROR_SUCCESS, "cannot add install execute sequence entry: %d\n", r );

    r = add_install_execute_sequence_entry( hdb, "'InstallValidate', '', '1400'" );
    ok( r == ERROR_SUCCESS, "cannot add install execute sequence entry: %d\n", r );

    r = add_install_execute_sequence_entry( hdb, "'InstallInitialize', '', '1500'" );
    ok( r == ERROR_SUCCESS, "cannot add install execute sequence entry: %d\n", r );

    r = add_install_execute_sequence_entry( hdb, "'ProcessComponents', '', '1600'" );
    ok( r == ERROR_SUCCESS, "cannot add install execute sequence entry: %d\n", r );

    r = add_install_execute_sequence_entry( hdb, "'UnpublishFeatures', '', '1800'" );
    ok( r == ERROR_SUCCESS, "cannot add install execute sequence entry: %d\n", r );

    r = add_install_execute_sequence_entry( hdb, "'RegisterProduct', '', '6100'" );
    ok( r == ERROR_SUCCESS, "cannot add install execute sequence entry: %d\n", r );

    r = add_install_execute_sequence_entry( hdb, "'PublishFeatures', '', '6300'" );
    ok( r == ERROR_SUCCESS, "cannot add install execute sequence entry: %d\n", r );

    r = add_install_execute_sequence_entry( hdb, "'PublishProduct', '', '6400'" );
    ok( r == ERROR_SUCCESS, "cannot add install execute sequence entry: %d\n", r );

    r = add_install_execute_sequence_entry( hdb, "'InstallFinalize', '', '6600'" );
    ok( r == ERROR_SUCCESS, "cannot add install execute sequence entry: %d\n", r );

    r = create_media_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create media table: %d\n", r );

    r = add_media_entry( hdb, "'1', '3', '', '', 'DISK1', ''");
    ok( r == ERROR_SUCCESS, "cannot add media entry: %d\n", r );

    r = create_feature_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create Feature table: %d\n", r );

    r = create_component_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create Component table: %d\n", r );

    /* msidbFeatureAttributesFavorLocal */
    r = add_feature_entry( hdb, "'one', '', '', '', 2, 1, '', 0" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    /* msidbFeatureAttributesFavorLocal:msidbComponentAttributesLocalOnly */
    r = add_component_entry( hdb, "'alpha', '{467EC132-739D-4784-A37B-677AA43DBC94}', 'TARGETDIR', 0, '', 'alpha_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorLocal:msidbComponentAttributesSourceOnly */
    r = add_component_entry( hdb, "'beta', '{2C1F189C-24A6-4C34-B26B-994A6C026506}', 'TARGETDIR', 1, '', 'beta_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorLocal:msidbComponentAttributesOptional */
    r = add_component_entry( hdb, "'gamma', '{C271E2A4-DE2E-4F70-86D1-6984AF7DE2CA}', 'TARGETDIR', 2, '', 'gamma_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorLocal:msidbComponentAttributesSharedDllRefCount */
    r = add_component_entry( hdb, "'theta', '{4EB3129D-81A8-48D5-9801-75600FED3DD9}', 'TARGETDIR', 8, '', 'theta_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorSource */
    r = add_feature_entry( hdb, "'two', '', '', '', 2, 1, '', 1" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    /* msidbFeatureAttributesFavorSource:msidbComponentAttributesLocalOnly */
    r = add_component_entry( hdb, "'delta', '{938FD4F2-C648-4259-A03C-7AA3B45643F3}', 'TARGETDIR', 0, '', 'delta_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorSource:msidbComponentAttributesSourceOnly */
    r = add_component_entry( hdb, "'epsilon', '{D59713B6-C11D-47F2-A395-1E5321781190}', 'TARGETDIR', 1, '', 'epsilon_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorSource:msidbComponentAttributesOptional */
    r = add_component_entry( hdb, "'zeta', '{377D33AB-2FAA-42B9-A629-0C0DAE9B9C7A}', 'TARGETDIR', 2, '', 'zeta_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorSource:msidbComponentAttributesSharedDllRefCount */
    r = add_component_entry( hdb, "'iota', '{5D36F871-B5ED-4801-9E0F-C46B9E5C9669}', 'TARGETDIR', 8, '', 'iota_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorSource */
    r = add_feature_entry( hdb, "'three', '', '', '', 2, 1, '', 1" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    /* msidbFeatureAttributesFavorLocal */
    r = add_feature_entry( hdb, "'four', '', '', '', 2, 1, '', 0" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    /* disabled */
    r = add_feature_entry( hdb, "'five', '', '', '', 2, 0, '', 1" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    /* msidbFeatureAttributesFavorSource:msidbComponentAttributesSourceOnly */
    r = add_component_entry( hdb, "'eta', '{DD89003F-0DD4-41B8-81C0-3411A7DA2695}', 'TARGETDIR', 1, '', 'eta_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* no feature parent:msidbComponentAttributesLocalOnly */
    r = add_component_entry( hdb, "'kappa', '{D6B93DC3-8DA5-4769-9888-42BFE156BB8B}', 'TARGETDIR', 1, '', 'kappa_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorLocal:removed */
    r = add_feature_entry( hdb, "'six', '', '', '', 2, 1, '', 0" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    /* msidbFeatureAttributesFavorLocal:removed:msidbComponentAttributesLocalOnly */
    r = add_component_entry( hdb, "'lambda', '{6528C5E4-02A4-4636-A214-7A66A6C35B64}', 'TARGETDIR', 0, '', 'lambda_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorLocal:removed:msidbComponentAttributesSourceOnly */
    r = add_component_entry( hdb, "'mu', '{97014BAB-6C56-4013-9A63-2BF913B42519}', 'TARGETDIR', 1, '', 'mu_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorLocal:removed:msidbComponentAttributesOptional */
    r = add_component_entry( hdb, "'nu', '{943DD0D8-5808-4954-8526-3B8493FEDDCD}', 'TARGETDIR', 2, '', 'nu_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorLocal:removed:msidbComponentAttributesSharedDllRefCount */
    r = add_component_entry( hdb, "'xi', '{D6CF9EF7-6FCF-4930-B34B-F938AEFF9BDB}', 'TARGETDIR', 8, '', 'xi_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorSource:removed */
    r = add_feature_entry( hdb, "'seven', '', '', '', 2, 1, '', 1" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    /* msidbFeatureAttributesFavorSource:removed:msidbComponentAttributesLocalOnly */
    r = add_component_entry( hdb, "'omicron', '{7B57521D-15DB-4141-9AA6-01D934A4433F}', 'TARGETDIR', 0, '', 'omicron_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorSource:removed:msidbComponentAttributesSourceOnly */
    r = add_component_entry( hdb, "'pi', '{FB85346B-378E-4492-8769-792305471C81}', 'TARGETDIR', 1, '', 'pi_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorSource:removed:msidbComponentAttributesOptional */
    r = add_component_entry( hdb, "'rho', '{798F2047-7B0C-4783-8BB0-D703E554114B}', 'TARGETDIR', 2, '', 'rho_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorSource:removed:msidbComponentAttributesSharedDllRefCount */
    r = add_component_entry( hdb, "'sigma', '{5CE9DDA8-B67B-4736-9D93-99D61C5B93E7}', 'TARGETDIR', 8, '', 'sigma_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorLocal */
    r = add_feature_entry( hdb, "'eight', '', '', '', 2, 1, '', 0" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    r = add_component_entry( hdb, "'tau', '{07DEB510-677C-4A6F-A0A6-7CD8EFEA77ED}', 'TARGETDIR', 1, '', 'tau_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorSource */
    r = add_feature_entry( hdb, "'nine', '', '', '', 2, 1, '', 1" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    r = add_component_entry( hdb, "'phi', '{9F0594C5-35AD-43EA-94DD-8DF73FAA664D}', 'TARGETDIR', 1, '', 'phi_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorAdvertise */
    r = add_feature_entry( hdb, "'ten', '', '', '', 2, 1, '', 4" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    r = add_component_entry( hdb, "'chi', '{E6B539AB-5DA9-4236-A2D2-E341A50B4C38}', 'TARGETDIR', 1, '', 'chi_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesUIDisallowAbsent */
    r = add_feature_entry( hdb, "'eleven', '', '', '', 2, 1, '', 16" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    r = add_component_entry( hdb, "'psi', '{A06B23B5-746B-427A-8A6E-FD6AC8F46A95}', 'TARGETDIR', 1, '', 'psi_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* high install level */
    r = add_feature_entry( hdb, "'twelve', '', '', '', 2, 2, '', 0" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    r = add_component_entry( hdb, "'upsilon', '{557e0c04-ceba-4c58-86a9-4a73352e8cf6}', 'TARGETDIR', 1, '', 'upsilon_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFollowParent */
    r = add_feature_entry( hdb, "'thirteen', '', '', '', 2, 2, '', 2" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    r = create_feature_components_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create FeatureComponents table: %d\n", r );

    r = add_feature_components_entry( hdb, "'one', 'alpha'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'one', 'beta'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'one', 'gamma'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'one', 'theta'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'two', 'delta'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'two', 'epsilon'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'two', 'zeta'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'two', 'iota'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'three', 'eta'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'four', 'eta'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'five', 'eta'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'six', 'lambda'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'six', 'mu'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'six', 'nu'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'six', 'xi'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'seven', 'omicron'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'seven', 'pi'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'seven', 'rho'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'seven', 'sigma'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'eight', 'tau'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'nine', 'phi'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'ten', 'chi'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'eleven', 'psi'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'twelve', 'upsilon'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'thirteen', 'upsilon'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = create_file_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create File table: %d\n", r );

    r = add_file_entry( hdb, "'alpha_file', 'alpha', 'alpha.txt', 100, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'beta_file', 'beta', 'beta.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'gamma_file', 'gamma', 'gamma.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'theta_file', 'theta', 'theta.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'delta_file', 'delta', 'delta.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'epsilon_file', 'epsilon', 'epsilon.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'zeta_file', 'zeta', 'zeta.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'iota_file', 'iota', 'iota.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    /* compressed file */
    r = add_file_entry( hdb, "'eta_file', 'eta', 'eta.txt', 0, '', '1033', 16384, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'kappa_file', 'kappa', 'kappa.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'lambda_file', 'lambda', 'lambda.txt', 100, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'mu_file', 'mu', 'mu.txt', 100, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'nu_file', 'nu', 'nu.txt', 100, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'xi_file', 'xi', 'xi.txt', 100, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'omicron_file', 'omicron', 'omicron.txt', 100, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'pi_file', 'pi', 'pi.txt', 100, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'rho_file', 'rho', 'rho.txt', 100, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'sigma_file', 'sigma', 'sigma.txt', 100, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'tau_file', 'tau', 'tau.txt', 100, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'phi_file', 'phi', 'phi.txt', 100, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'chi_file', 'chi', 'chi.txt', 100, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'psi_file', 'psi', 'psi.txt', 100, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'upsilon_file', 'upsilon', 'upsilon.txt', 0, '', '1033', 16384, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    MsiDatabaseCommit(hdb);

    /* these properties must not be in the saved msi file */
    r = add_property_entry( hdb, "'ADDLOCAL', 'one,four'");
    ok( r == ERROR_SUCCESS, "cannot add property: %d\n", r );

    r = add_property_entry( hdb, "'ADDSOURCE', 'two,three'");
    ok( r == ERROR_SUCCESS, "cannot add property: %d\n", r );

    r = add_property_entry( hdb, "'REMOVE', 'six,seven'");
    ok( r == ERROR_SUCCESS, "cannot add property: %d\n", r );

    r = add_property_entry( hdb, "'REINSTALL', 'eight,nine,ten'");
    ok( r == ERROR_SUCCESS, "cannot add property: %d\n", r );

    r = add_property_entry( hdb, "'REINSTALLMODE', 'omus'");
    ok( r == ERROR_SUCCESS, "cannot add property: %d\n", r );

    r = package_from_db( hdb, &hpkg );
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        DeleteFileA(msifile);
        return;
    }
    ok( r == ERROR_SUCCESS, "failed to create package %u\n", r );

    MsiCloseHandle(hdb);

    CopyFileA(msifile, msifile2, FALSE);
    CopyFileA(msifile, msifile3, FALSE);
    CopyFileA(msifile, msifile4, FALSE);

    test_feature_states( __LINE__, hpkg, "one", ERROR_UNKNOWN_FEATURE, 0, 0, FALSE );
    test_component_states( __LINE__, hpkg, "alpha", ERROR_UNKNOWN_COMPONENT, 0, 0, FALSE );

    r = MsiDoActionA( hpkg, "CostInitialize");
    ok( r == ERROR_SUCCESS, "cost init failed\n");

    test_feature_states( __LINE__, hpkg, "one", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "alpha", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_UNKNOWN, FALSE );

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiDoActionA( hpkg, "FileCost");
    ok( r == ERROR_SUCCESS, "file cost failed\n");

    test_feature_states( __LINE__, hpkg, "one", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "alpha", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_UNKNOWN, FALSE );

    r = MsiDoActionA( hpkg, "CostFinalize");
    ok( r == ERROR_SUCCESS, "cost finalize failed: %d\n", r);

    test_feature_states( __LINE__, hpkg, "one", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_LOCAL, FALSE );
    test_feature_states( __LINE__, hpkg, "two", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_SOURCE, FALSE );
    test_feature_states( __LINE__, hpkg, "three", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_LOCAL, FALSE );
    test_feature_states( __LINE__, hpkg, "four", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_LOCAL, FALSE );
    test_feature_states( __LINE__, hpkg, "five", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_feature_states( __LINE__, hpkg, "six", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_feature_states( __LINE__, hpkg, "seven", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_feature_states( __LINE__, hpkg, "eight", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_feature_states( __LINE__, hpkg, "nine", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_feature_states( __LINE__, hpkg, "ten", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_feature_states( __LINE__, hpkg, "eleven", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_feature_states( __LINE__, hpkg, "twelve", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_feature_states( __LINE__, hpkg, "thirteen", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );

    test_component_states( __LINE__, hpkg, "alpha", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "beta", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_SOURCE, FALSE );
    test_component_states( __LINE__, hpkg, "gamma", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "theta", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "delta", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "epsilon", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_SOURCE, FALSE );
    test_component_states( __LINE__, hpkg, "zeta", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_SOURCE, FALSE );
    test_component_states( __LINE__, hpkg, "iota", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "eta", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "kappa", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "lambda", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "mu", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "nu", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "xi", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "omicron", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "pi", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "rho", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "sigma", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "tau", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "phi", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "chi", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "psi", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "upsilon", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );

    MsiCloseHandle( hpkg );

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    /* publish the features and components */
    r = MsiInstallProductA(msifile, "ADDLOCAL=one,four ADDSOURCE=two,three REMOVE=six,seven REINSTALL=eight,nine,ten");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_DIRECT, &hdb);
    ok(r == ERROR_SUCCESS, "failed to open database: %d\n", r);

    /* these properties must not be in the saved msi file */
    r = add_property_entry( hdb, "'ADDLOCAL', 'one,four'");
    ok( r == ERROR_SUCCESS, "cannot add property: %d\n", r );

    r = add_property_entry( hdb, "'ADDSOURCE', 'two,three'");
    ok( r == ERROR_SUCCESS, "cannot add property: %d\n", r );

    r = add_property_entry( hdb, "'REMOVE', 'six,seven'");
    ok( r == ERROR_SUCCESS, "cannot add property: %d\n", r );

    r = add_property_entry( hdb, "'REINSTALL', 'eight,nine,ten'");
    ok( r == ERROR_SUCCESS, "cannot add property: %d\n", r );

    r = package_from_db( hdb, &hpkg );
    ok( r == ERROR_SUCCESS, "failed to create package %u\n", r );

    MsiCloseHandle(hdb);

    test_feature_states( __LINE__, hpkg, "one", ERROR_UNKNOWN_FEATURE, 0, 0, FALSE );
    test_component_states( __LINE__, hpkg, "alpha", ERROR_UNKNOWN_COMPONENT, 0, 0, FALSE );

    r = MsiDoActionA( hpkg, "CostInitialize");
    ok( r == ERROR_SUCCESS, "cost init failed\n");

    test_feature_states( __LINE__, hpkg, "one", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "alpha", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_UNKNOWN, FALSE );

    r = MsiDoActionA( hpkg, "FileCost");
    ok( r == ERROR_SUCCESS, "file cost failed\n");

    test_feature_states( __LINE__, hpkg, "one", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "alpha", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_UNKNOWN, FALSE );

    r = MsiDoActionA( hpkg, "CostFinalize");
    ok( r == ERROR_SUCCESS, "cost finalize failed: %d\n", r);

    test_feature_states( __LINE__, hpkg, "one", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_LOCAL, FALSE );
    test_feature_states( __LINE__, hpkg, "two", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_SOURCE, FALSE );
    test_feature_states( __LINE__, hpkg, "three", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_feature_states( __LINE__, hpkg, "four", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_feature_states( __LINE__, hpkg, "five", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_feature_states( __LINE__, hpkg, "six", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_feature_states( __LINE__, hpkg, "seven", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_feature_states( __LINE__, hpkg, "eight", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_feature_states( __LINE__, hpkg, "nine", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_feature_states( __LINE__, hpkg, "ten", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_feature_states( __LINE__, hpkg, "eleven", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_feature_states( __LINE__, hpkg, "twelve", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_feature_states( __LINE__, hpkg, "thirteen", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );

    test_component_states( __LINE__, hpkg, "alpha", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "beta", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_SOURCE, FALSE );
    test_component_states( __LINE__, hpkg, "gamma", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "theta", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "delta", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "epsilon", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "zeta", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "iota", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "eta", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "kappa", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "lambda", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "mu", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "nu", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "xi", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "omicron", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "pi", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "rho", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "sigma", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "tau", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "phi", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "chi", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "psi", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "upsilon", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );

    MsiCloseHandle(hpkg);

    /* uninstall the product */
    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    state = MsiQueryFeatureStateA("{7262AC98-EEBD-4364-8CE3-D654F6A425B9}", "five");
    ok(state == INSTALLSTATE_UNKNOWN, "state = %d\n", state);
    state = MsiQueryFeatureStateA("{7262AC98-EEBD-4364-8CE3-D654F6A425B9}", "twelve");
    ok(state == INSTALLSTATE_UNKNOWN, "state = %d\n", state);

    /* all features installed locally */
    r = MsiInstallProductA(msifile2, "ADDLOCAL=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    state = MsiQueryFeatureStateA("{7262AC98-EEBD-4364-8CE3-D654F6A425B9}", "five");
    ok(state == INSTALLSTATE_UNKNOWN, "state = %d\n", state);
    state = MsiQueryFeatureStateA("{7262AC98-EEBD-4364-8CE3-D654F6A425B9}", "twelve");
    ok(state == INSTALLSTATE_LOCAL, "state = %d\n", state);

    r = MsiOpenDatabaseW(msifile2W, MSIDBOPEN_DIRECT, &hdb);
    ok(r == ERROR_SUCCESS, "failed to open database: %d\n", r);

    /* these properties must not be in the saved msi file */
    r = add_property_entry( hdb, "'ADDLOCAL', 'one,two,three,four,five,six,seven,eight,nine,ten,twelve'");
    ok( r == ERROR_SUCCESS, "cannot add property: %d\n", r );

    r = package_from_db( hdb, &hpkg );
    ok( r == ERROR_SUCCESS, "failed to create package %u\n", r );

    test_feature_states( __LINE__, hpkg, "one", ERROR_UNKNOWN_FEATURE, 0, 0, FALSE );
    test_component_states( __LINE__, hpkg, "alpha", ERROR_UNKNOWN_COMPONENT, 0, 0, FALSE );

    r = MsiDoActionA( hpkg, "CostInitialize");
    ok( r == ERROR_SUCCESS, "cost init failed\n");

    test_feature_states( __LINE__, hpkg, "one", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "alpha", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_UNKNOWN, FALSE );

    r = MsiDoActionA( hpkg, "CostFinalize");
    ok( r == ERROR_SUCCESS, "cost finalize failed: %d\n", r);

    test_feature_states( __LINE__, hpkg, "one", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_LOCAL, FALSE );
    test_feature_states( __LINE__, hpkg, "two", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_LOCAL, FALSE );
    test_feature_states( __LINE__, hpkg, "three", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_feature_states( __LINE__, hpkg, "four", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_feature_states( __LINE__, hpkg, "five", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_feature_states( __LINE__, hpkg, "six", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_LOCAL, FALSE );
    test_feature_states( __LINE__, hpkg, "seven", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_LOCAL, FALSE );
    test_feature_states( __LINE__, hpkg, "eight", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_SOURCE, TRUE );
    test_feature_states( __LINE__, hpkg, "nine", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_SOURCE, TRUE );
    test_feature_states( __LINE__, hpkg, "ten", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_SOURCE, TRUE );
    test_feature_states( __LINE__, hpkg, "eleven", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, TRUE );
    test_feature_states( __LINE__, hpkg, "twelve", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_feature_states( __LINE__, hpkg, "thirteen", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_UNKNOWN, FALSE );

    test_component_states( __LINE__, hpkg, "alpha", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "beta", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_SOURCE, FALSE );
    test_component_states( __LINE__, hpkg, "gamma", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "theta", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "delta", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "epsilon", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_SOURCE, FALSE );
    test_component_states( __LINE__, hpkg, "zeta", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "iota", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "eta", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "kappa", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "lambda", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "mu", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_SOURCE, FALSE );
    test_component_states( __LINE__, hpkg, "nu", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "xi", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "omicron", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "pi", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_SOURCE, FALSE );
    test_component_states( __LINE__, hpkg, "rho", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "sigma", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "tau", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, TRUE );
    test_component_states( __LINE__, hpkg, "phi", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, TRUE );
    test_component_states( __LINE__, hpkg, "chi", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, TRUE );
    test_component_states( __LINE__, hpkg, "psi", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "upsilon", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );

    MsiCloseHandle(hpkg);

    /* uninstall the product */
    r = MsiInstallProductA(msifile2, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* all features installed from source */
    r = MsiInstallProductA(msifile3, "ADDSOURCE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    state = MsiQueryFeatureStateA("{7262AC98-EEBD-4364-8CE3-D654F6A425B9}", "five");
    ok(state == INSTALLSTATE_UNKNOWN, "state = %d\n", state);
    state = MsiQueryFeatureStateA("{7262AC98-EEBD-4364-8CE3-D654F6A425B9}", "twelve");
    ok(state == INSTALLSTATE_LOCAL, "state = %d\n", state);

    r = MsiOpenDatabaseW(msifile3W, MSIDBOPEN_DIRECT, &hdb);
    ok(r == ERROR_SUCCESS, "failed to open database: %d\n", r);

    /* this property must not be in the saved msi file */
    r = add_property_entry( hdb, "'ADDSOURCE', 'one,two,three,four,five,six,seven,eight,nine,ten'");
    ok( r == ERROR_SUCCESS, "cannot add property: %d\n", r );

    r = package_from_db( hdb, &hpkg );
    ok( r == ERROR_SUCCESS, "failed to create package %u\n", r );

    test_feature_states( __LINE__, hpkg, "one", ERROR_UNKNOWN_FEATURE, 0, 0, FALSE );
    test_component_states( __LINE__, hpkg, "alpha", ERROR_UNKNOWN_COMPONENT, 0, 0, FALSE );

    r = MsiDoActionA( hpkg, "CostInitialize");
    ok( r == ERROR_SUCCESS, "cost init failed\n");

    test_feature_states( __LINE__, hpkg, "one", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "alpha", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_UNKNOWN, FALSE );

    r = MsiDoActionA( hpkg, "FileCost");
    ok( r == ERROR_SUCCESS, "file cost failed\n");

    test_feature_states( __LINE__, hpkg, "one", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "alpha", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_UNKNOWN, FALSE );

    r = MsiDoActionA( hpkg, "CostFinalize");
    ok( r == ERROR_SUCCESS, "cost finalize failed: %d\n", r);

    test_feature_states( __LINE__, hpkg, "one", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_SOURCE, FALSE );
    test_feature_states( __LINE__, hpkg, "two", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_SOURCE, FALSE );
    test_feature_states( __LINE__, hpkg, "three", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_feature_states( __LINE__, hpkg, "four", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_feature_states( __LINE__, hpkg, "five", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_feature_states( __LINE__, hpkg, "six", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_SOURCE, FALSE );
    test_feature_states( __LINE__, hpkg, "seven", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_SOURCE, FALSE );
    test_feature_states( __LINE__, hpkg, "eight", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_SOURCE, FALSE );
    test_feature_states( __LINE__, hpkg, "nine", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_SOURCE, FALSE );
    test_feature_states( __LINE__, hpkg, "ten", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_SOURCE, FALSE );
    test_feature_states( __LINE__, hpkg, "eleven", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, TRUE );
    test_feature_states( __LINE__, hpkg, "twelve", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_UNKNOWN, FALSE );
    test_feature_states( __LINE__, hpkg, "thirteen", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_UNKNOWN, FALSE );

    test_component_states( __LINE__, hpkg, "alpha", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "beta", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "gamma", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "theta", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "delta", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "epsilon", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "zeta", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "iota", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "eta", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "kappa", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "lambda", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "mu", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "nu", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "xi", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "omicron", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "pi", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "rho", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "sigma", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "tau", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "phi", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "chi", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "psi", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "upsilon", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, TRUE );

    MsiCloseHandle(hpkg);

    /* reinstall the product */
    r = MsiInstallProductA(msifile3, "REINSTALL=ALL");
    ok(r == ERROR_SUCCESS || broken(r == ERROR_INSTALL_FAILURE) /* win2k3 */, "Expected ERROR_SUCCESS, got %d\n", r);

    state = MsiQueryFeatureStateA("{7262AC98-EEBD-4364-8CE3-D654F6A425B9}", "five");
    ok(state == INSTALLSTATE_UNKNOWN, "state = %d\n", state);
    state = MsiQueryFeatureStateA("{7262AC98-EEBD-4364-8CE3-D654F6A425B9}", "twelve");
    ok(state == INSTALLSTATE_LOCAL, "state = %d\n", state);

    r = MsiOpenDatabaseW(msifile4W, MSIDBOPEN_DIRECT, &hdb);
    ok(r == ERROR_SUCCESS, "failed to open database: %d\n", r);

    /* this property must not be in the saved msi file */
    r = add_property_entry( hdb, "'ADDSOURCE', 'one,two,three,four,five,six,seven,eight,nine,ten'");
    ok( r == ERROR_SUCCESS, "cannot add property: %d\n", r );

    r = package_from_db( hdb, &hpkg );
    ok( r == ERROR_SUCCESS, "failed to create package %u\n", r );

    test_feature_states( __LINE__, hpkg, "one", ERROR_UNKNOWN_FEATURE, 0, 0, FALSE );
    test_component_states( __LINE__, hpkg, "alpha", ERROR_UNKNOWN_COMPONENT, 0, 0, FALSE );

    r = MsiDoActionA( hpkg, "CostInitialize");
    ok( r == ERROR_SUCCESS, "cost init failed\n");

    test_feature_states( __LINE__, hpkg, "one", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "alpha", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_UNKNOWN, FALSE );

    r = MsiDoActionA( hpkg, "FileCost");
    ok( r == ERROR_SUCCESS, "file cost failed\n");

    test_feature_states( __LINE__, hpkg, "one", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "alpha", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_UNKNOWN, FALSE );

    r = MsiDoActionA( hpkg, "CostFinalize");
    ok( r == ERROR_SUCCESS, "cost finalize failed: %d\n", r);

    test_feature_states( __LINE__, hpkg, "one", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_SOURCE, FALSE );
    test_feature_states( __LINE__, hpkg, "two", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_SOURCE, FALSE );
    test_feature_states( __LINE__, hpkg, "three", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_feature_states( __LINE__, hpkg, "four", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_feature_states( __LINE__, hpkg, "five", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_feature_states( __LINE__, hpkg, "six", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_SOURCE, FALSE );
    test_feature_states( __LINE__, hpkg, "seven", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_SOURCE, FALSE );
    test_feature_states( __LINE__, hpkg, "eight", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_SOURCE, FALSE );
    test_feature_states( __LINE__, hpkg, "nine", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_SOURCE, FALSE );
    test_feature_states( __LINE__, hpkg, "ten", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_SOURCE, FALSE );
    test_feature_states( __LINE__, hpkg, "eleven", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, TRUE );
    test_feature_states( __LINE__, hpkg, "twelve", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_UNKNOWN, FALSE );
    test_feature_states( __LINE__, hpkg, "thirteen", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_UNKNOWN, FALSE );

    test_component_states( __LINE__, hpkg, "alpha", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "beta", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "gamma", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "theta", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "delta", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "epsilon", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "zeta", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "iota", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "eta", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "kappa", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "lambda", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "mu", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "nu", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "xi", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "omicron", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "pi", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "rho", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "sigma", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "tau", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "phi", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "chi", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "psi", ERROR_SUCCESS, INSTALLSTATE_SOURCE, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "upsilon", ERROR_SUCCESS, INSTALLSTATE_LOCAL, INSTALLSTATE_LOCAL, TRUE );

    MsiCloseHandle(hpkg);

    /* uninstall the product */
    r = MsiInstallProductA(msifile4, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    DeleteFileA(msifile);
    DeleteFileA(msifile2);
    DeleteFileA(msifile3);
    DeleteFileA(msifile4);
}

static void test_getproperty(void)
{
    MSIHANDLE hPackage = 0;
    char prop[100];
    static CHAR empty[] = "";
    DWORD size;
    UINT r;

    r = package_from_db(create_package_db(), &hPackage);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        DeleteFileA(msifile);
        return;
    }
    ok( r == ERROR_SUCCESS, "Failed to create package %u\n", r );

    /* set the property */
    r = MsiSetPropertyA(hPackage, "Name", "Value");
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* retrieve the size, NULL pointer */
    size = 0;
    r = MsiGetPropertyA(hPackage, "Name", NULL, &size);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok( size == 5, "Expected 5, got %d\n", size);

    /* retrieve the size, empty string */
    size = 0;
    r = MsiGetPropertyA(hPackage, "Name", empty, &size);
    ok( r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok( size == 5, "Expected 5, got %d\n", size);

    /* don't change size */
    r = MsiGetPropertyA(hPackage, "Name", prop, &size);
    ok( r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok( size == 5, "Expected 5, got %d\n", size);
    ok( !lstrcmpA(prop, "Valu"), "Expected Valu, got %s\n", prop);

    /* increase the size by 1 */
    size++;
    r = MsiGetPropertyA(hPackage, "Name", prop, &size);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok( size == 5, "Expected 5, got %d\n", size);
    ok( !lstrcmpA(prop, "Value"), "Expected Value, got %s\n", prop);

    r = MsiCloseHandle( hPackage);
    ok( r == ERROR_SUCCESS , "Failed to close package\n" );
    DeleteFileA(msifile);
}

static void test_removefiles(void)
{
    MSIHANDLE hpkg;
    UINT r;
    MSIHANDLE hdb;
    INSTALLSTATE installed, action;

    hdb = create_package_db();
    ok ( hdb, "failed to create package database\n" );

    r = add_directory_entry( hdb, "'TARGETDIR', '', 'SourceDir'");
    ok( r == ERROR_SUCCESS, "cannot add directory: %d\n", r );

    r = create_feature_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create Feature table: %d\n", r );

    r = create_component_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create Component table: %d\n", r );

    r = add_feature_entry( hdb, "'one', '', '', '', 2, 1, '', 0" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    r = add_component_entry( hdb, "'hydrogen', '', 'TARGETDIR', 0, '', 'hydrogen_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    r = add_component_entry( hdb, "'helium', '', 'TARGETDIR', 0, '', 'helium_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    r = add_component_entry( hdb, "'lithium', '', 'TARGETDIR', 0, '', 'lithium_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    r = add_component_entry( hdb, "'beryllium', '', 'TARGETDIR', 0, '', 'beryllium_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    r = add_component_entry( hdb, "'boron', '', 'TARGETDIR', 0, '', 'boron_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    r = add_component_entry( hdb, "'carbon', '', 'TARGETDIR', 0, '', 'carbon_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    r = add_component_entry( hdb, "'oxygen', '', 'TARGETDIR', 0, '0', 'oxygen_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    r = create_feature_components_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create FeatureComponents table: %d\n", r );

    r = add_feature_components_entry( hdb, "'one', 'hydrogen'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'one', 'helium'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'one', 'lithium'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'one', 'beryllium'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'one', 'boron'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'one', 'carbon'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'one', 'oxygen'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = create_file_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create File table: %d\n", r );

    r = add_file_entry( hdb, "'hydrogen_file', 'hydrogen', 'hydrogen.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'helium_file', 'helium', 'helium.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'lithium_file', 'lithium', 'lithium.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'beryllium_file', 'beryllium', 'beryllium.txt', 0, '', '1033', 16384, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'boron_file', 'boron', 'boron.txt', 0, '', '1033', 16384, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'carbon_file', 'carbon', 'carbon.txt', 0, '', '1033', 16384, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'oxygen_file', 'oxygen', 'oxygen.txt', 0, '', '1033', 16384, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = create_remove_file_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create Remove File table: %d\n", r);

    r = package_from_db( hdb, &hpkg );
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        DeleteFileA(msifile);
        return;
    }
    ok( r == ERROR_SUCCESS, "failed to create package %u\n", r );

    MsiCloseHandle( hdb );

    create_test_file( "hydrogen.txt" );
    create_test_file( "helium.txt" );
    create_test_file( "lithium.txt" );
    create_test_file( "beryllium.txt" );
    create_test_file( "boron.txt" );
    create_test_file( "carbon.txt" );
    create_test_file( "oxygen.txt" );

    r = MsiSetPropertyA( hpkg, "TARGETDIR", CURR_DIR );
    ok( r == ERROR_SUCCESS, "set property failed\n");

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiGetComponentStateA( hpkg, "oxygen", &installed, &action );
    ok( r == ERROR_UNKNOWN_COMPONENT, "expected ERROR_UNKNOWN_COMPONENT, got %u\n", r );

    r = MsiDoActionA( hpkg, "CostInitialize");
    ok( r == ERROR_SUCCESS, "cost init failed\n");

    r = MsiDoActionA( hpkg, "FileCost");
    ok( r == ERROR_SUCCESS, "file cost failed\n");

    installed = action = 0xdeadbeef;
    r = MsiGetComponentStateA( hpkg, "oxygen", &installed, &action );
    ok( r == ERROR_SUCCESS, "failed to get component state %u\n", r );
    ok( installed == INSTALLSTATE_UNKNOWN, "expected INSTALLSTATE_UNKNOWN, got %d\n", installed );
    ok( action == INSTALLSTATE_UNKNOWN, "expected INSTALLSTATE_UNKNOWN, got %d\n", action );

    r = MsiDoActionA( hpkg, "CostFinalize");
    ok( r == ERROR_SUCCESS, "cost finalize failed\n");

    r = MsiDoActionA( hpkg, "InstallValidate");
    ok( r == ERROR_SUCCESS, "install validate failed\n");

    r = MsiSetComponentStateA( hpkg, "hydrogen", INSTALLSTATE_ABSENT );
    ok( r == ERROR_SUCCESS, "failed to set component state: %d\n", r);

    installed = action = 0xdeadbeef;
    r = MsiGetComponentStateA( hpkg, "hydrogen", &installed, &action );
    ok( r == ERROR_SUCCESS, "failed to get component state %u\n", r );
    ok( installed == INSTALLSTATE_UNKNOWN, "expected INSTALLSTATE_UNKNOWN, got %d\n", installed );
    todo_wine ok( action == INSTALLSTATE_UNKNOWN, "expected INSTALLSTATE_UNKNOWN, got %d\n", action );

    r = MsiSetComponentStateA( hpkg, "helium", INSTALLSTATE_LOCAL );
    ok( r == ERROR_SUCCESS, "failed to set component state: %d\n", r);

    r = MsiSetComponentStateA( hpkg, "lithium", INSTALLSTATE_SOURCE );
    ok( r == ERROR_SUCCESS, "failed to set component state: %d\n", r);

    r = MsiSetComponentStateA( hpkg, "beryllium", INSTALLSTATE_ABSENT );
    ok( r == ERROR_SUCCESS, "failed to set component state: %d\n", r);

    r = MsiSetComponentStateA( hpkg, "boron", INSTALLSTATE_LOCAL );
    ok( r == ERROR_SUCCESS, "failed to set component state: %d\n", r);

    r = MsiSetComponentStateA( hpkg, "carbon", INSTALLSTATE_SOURCE );
    ok( r == ERROR_SUCCESS, "failed to set component state: %d\n", r);

    installed = action = 0xdeadbeef;
    r = MsiGetComponentStateA( hpkg, "oxygen", &installed, &action );
    ok( r == ERROR_SUCCESS, "failed to get component state %u\n", r );
    ok( installed == INSTALLSTATE_UNKNOWN, "expected INSTALLSTATE_UNKNOWN, got %d\n", installed );
    ok( action == INSTALLSTATE_UNKNOWN, "expected INSTALLSTATE_UNKNOWN, got %d\n", action );

    r = MsiSetComponentStateA( hpkg, "oxygen", INSTALLSTATE_ABSENT );
    ok( r == ERROR_SUCCESS, "failed to set component state: %d\n", r);

    installed = action = 0xdeadbeef;
    r = MsiGetComponentStateA( hpkg, "oxygen", &installed, &action );
    ok( r == ERROR_SUCCESS, "failed to get component state %u\n", r );
    ok( installed == INSTALLSTATE_UNKNOWN, "expected INSTALLSTATE_UNKNOWN, got %d\n", installed );
    ok( action == INSTALLSTATE_UNKNOWN, "expected INSTALLSTATE_UNKNOWN, got %d\n", action );

    r = MsiDoActionA( hpkg, "RemoveFiles");
    ok( r == ERROR_SUCCESS, "remove files failed\n");

    installed = action = 0xdeadbeef;
    r = MsiGetComponentStateA( hpkg, "oxygen", &installed, &action );
    ok( r == ERROR_SUCCESS, "failed to get component state %u\n", r );
    ok( installed == INSTALLSTATE_UNKNOWN, "expected INSTALLSTATE_UNKNOWN, got %d\n", installed );
    ok( action == INSTALLSTATE_UNKNOWN, "expected INSTALLSTATE_UNKNOWN, got %d\n", action );

    ok(DeleteFileA("hydrogen.txt"), "Expected hydrogen.txt to exist\n");
    ok(DeleteFileA("lithium.txt"), "Expected lithium.txt to exist\n");    
    ok(DeleteFileA("beryllium.txt"), "Expected beryllium.txt to exist\n");
    ok(DeleteFileA("carbon.txt"), "Expected carbon.txt to exist\n");
    ok(DeleteFileA("helium.txt"), "Expected helium.txt to exist\n");
    ok(DeleteFileA("boron.txt"), "Expected boron.txt to exist\n");
    ok(DeleteFileA("oxygen.txt"), "Expected oxygen.txt to exist\n");

    MsiCloseHandle( hpkg );
    DeleteFileA(msifile);
}

static void test_appsearch(void)
{
    MSIHANDLE hpkg;
    UINT r;
    MSIHANDLE hdb;
    CHAR prop[MAX_PATH];
    DWORD size;
    HKEY hkey;
    const char reg_expand_value[] = "%systemroot%\\system32\\notepad.exe";

    hdb = create_package_db();
    ok ( hdb, "failed to create package database\n" );

    r = create_appsearch_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create AppSearch table: %d\n", r );

    r = add_appsearch_entry( hdb, "'WEBBROWSERPROG', 'NewSignature1'" );
    ok( r == ERROR_SUCCESS, "cannot add entry: %d\n", r );

    r = add_appsearch_entry( hdb, "'NOTEPAD', 'NewSignature2'" );
    ok( r == ERROR_SUCCESS, "cannot add entry: %d\n", r );

    r = add_appsearch_entry( hdb, "'REGEXPANDVAL', 'NewSignature3'" );
    ok( r == ERROR_SUCCESS, "cannot add entry: %d\n", r );

    r = create_reglocator_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create RegLocator table: %d\n", r );

    r = add_reglocator_entry( hdb, "NewSignature1", 0, "htmlfile\\shell\\open\\command", "", 1 );
    ok( r == ERROR_SUCCESS, "cannot create RegLocator table: %d\n", r );

    r = RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Winetest_msi", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkey, NULL);
    ok( r == ERROR_SUCCESS, "Could not create key: %d.\n", r );
    r = RegSetValueExA(hkey, NULL, 0, REG_EXPAND_SZ, (const BYTE*)reg_expand_value, strlen(reg_expand_value) + 1);
    ok( r == ERROR_SUCCESS, "Could not set key value: %d.\n", r);
    RegCloseKey(hkey);
    r = add_reglocator_entry( hdb, "NewSignature3", 1, "Software\\Winetest_msi", "", 1 );
    ok( r == ERROR_SUCCESS, "cannot create RegLocator table: %d\n", r );

    r = create_drlocator_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create DrLocator table: %d\n", r );

    r = add_drlocator_entry( hdb, "'NewSignature2', 0, 'c:\\windows\\system32', 0" );
    ok( r == ERROR_SUCCESS, "cannot create RegLocator table: %d\n", r );

    r = create_signature_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create Signature table: %d\n", r );

    r = add_signature_entry( hdb, "'NewSignature1', 'FileName', '', '', '', '', '', '', ''" );
    ok( r == ERROR_SUCCESS, "cannot add signature: %d\n", r );

    r = add_signature_entry( hdb, "'NewSignature2', 'NOTEPAD.EXE|notepad.exe', '', '', '', '', '', '', ''" );
    ok( r == ERROR_SUCCESS, "cannot add signature: %d\n", r );

    r = add_signature_entry( hdb, "'NewSignature3', 'NOTEPAD.EXE|notepad.exe', '', '', '', '', '', '', ''" );
    ok( r == ERROR_SUCCESS, "cannot add signature: %d\n", r );

    r = package_from_db( hdb, &hpkg );
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        DeleteFileA(msifile);
        return;
    }
    ok( r == ERROR_SUCCESS, "failed to create package %u\n", r );
    MsiCloseHandle( hdb );
    if (r != ERROR_SUCCESS)
        goto done;

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiDoActionA( hpkg, "AppSearch" );
    ok( r == ERROR_SUCCESS, "AppSearch failed: %d\n", r);

    size = sizeof(prop);
    r = MsiGetPropertyA( hpkg, "WEBBROWSERPROG", prop, &size );
    ok( r == ERROR_SUCCESS, "get property failed: %d\n", r);
    ok( lstrlenA(prop) != 0, "Expected non-zero length\n");

    size = sizeof(prop);
    r = MsiGetPropertyA( hpkg, "NOTEPAD", prop, &size );
    ok( r == ERROR_SUCCESS, "get property failed: %d\n", r);

    size = sizeof(prop);
    r = MsiGetPropertyA( hpkg, "REGEXPANDVAL", prop, &size );
    ok( r == ERROR_SUCCESS, "get property failed: %d\n", r);
    ok( lstrlenA(prop) != 0, "Expected non-zero length\n");

done:
    MsiCloseHandle( hpkg );
    DeleteFileA(msifile);
    RegDeleteKeyA(HKEY_CURRENT_USER, "Software\\Winetest_msi");
}

static void test_appsearch_complocator(void)
{
    MSIHANDLE hpkg, hdb;
    char path[MAX_PATH], expected[MAX_PATH], prop[MAX_PATH];
    LPSTR usersid;
    DWORD size;
    UINT r;

    if (!(usersid = get_user_sid()))
        return;

    if (is_process_limited())
    {
        skip("process is limited\n");
        return;
    }

    create_test_file("FileName1");
    create_test_file("FileName4");
    set_component_path("FileName1", MSIINSTALLCONTEXT_MACHINE,
                       "{A8AE6692-96BA-4198-8399-145D7D1D0D0E}", NULL, FALSE);

    create_test_file("FileName2");
    set_component_path("FileName2", MSIINSTALLCONTEXT_USERUNMANAGED,
                       "{1D2CE6F3-E81C-4949-AB81-78D7DAD2AF2E}", usersid, FALSE);

    create_test_file("FileName3");
    set_component_path("FileName3", MSIINSTALLCONTEXT_USERMANAGED,
                       "{19E0B999-85F5-4973-A61B-DBE4D66ECB1D}", usersid, FALSE);

    create_test_file("FileName5");
    set_component_path("FileName5", MSIINSTALLCONTEXT_MACHINE,
                       "{F0CCA976-27A3-4808-9DDD-1A6FD50A0D5A}", NULL, TRUE);

    create_test_file("FileName6");
    set_component_path("FileName6", MSIINSTALLCONTEXT_MACHINE,
                       "{C0ECD96F-7898-4410-9667-194BD8C1B648}", NULL, TRUE);

    create_test_file("FileName7");
    set_component_path("FileName7", MSIINSTALLCONTEXT_MACHINE,
                       "{DB20F535-9C26-4127-9C2B-CC45A8B51DA1}", NULL, FALSE);

    /* dir is FALSE, but we're pretending it's a directory */
    set_component_path("IDontExist\\", MSIINSTALLCONTEXT_MACHINE,
                       "{91B7359B-07F2-4221-AA8D-DE102BB87A5F}", NULL, FALSE);

    create_file_with_version("FileName8.dll", MAKELONG(2, 1), MAKELONG(4, 3));
    set_component_path("FileName8.dll", MSIINSTALLCONTEXT_MACHINE,
                       "{4A2E1B5B-4034-4177-833B-8CC35F1B3EF1}", NULL, FALSE);

    create_file_with_version("FileName9.dll", MAKELONG(1, 2), MAKELONG(3, 4));
    set_component_path("FileName9.dll", MSIINSTALLCONTEXT_MACHINE,
                       "{A204DF48-7346-4635-BA2E-66247DBAC9DF}", NULL, FALSE);

    create_file_with_version("FileName10.dll", MAKELONG(2, 1), MAKELONG(4, 3));
    set_component_path("FileName10.dll", MSIINSTALLCONTEXT_MACHINE,
                       "{EC30CE73-4CF9-4908-BABD-1ED82E1515FD}", NULL, FALSE);

    hdb = create_package_db();
    ok(hdb, "Expected a valid database handle\n");

    r = create_appsearch_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP1', 'NewSignature1'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP2', 'NewSignature2'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP3', 'NewSignature3'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP4', 'NewSignature4'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP5', 'NewSignature5'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP6', 'NewSignature6'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP7', 'NewSignature7'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP8', 'NewSignature8'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP9', 'NewSignature9'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP10', 'NewSignature10'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP11', 'NewSignature11'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP12', 'NewSignature12'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = create_complocator_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* published component, machine, file, signature, misdbLocatorTypeFile */
    r = add_complocator_entry(hdb, "'NewSignature1', '{A8AE6692-96BA-4198-8399-145D7D1D0D0E}', 1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* published component, user-unmanaged, file, signature, misdbLocatorTypeFile */
    r = add_complocator_entry(hdb, "'NewSignature2', '{1D2CE6F3-E81C-4949-AB81-78D7DAD2AF2E}', 1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* published component, user-managed, file, signature, misdbLocatorTypeFile */
    r = add_complocator_entry(hdb, "'NewSignature3', '{19E0B999-85F5-4973-A61B-DBE4D66ECB1D}', 1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* published component, machine, file, signature, misdbLocatorTypeDirectory */
    r = add_complocator_entry(hdb, "'NewSignature4', '{A8AE6692-96BA-4198-8399-145D7D1D0D0E}', 0");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* published component, machine, dir, signature, misdbLocatorTypeDirectory */
    r = add_complocator_entry(hdb, "'NewSignature5', '{F0CCA976-27A3-4808-9DDD-1A6FD50A0D5A}', 0");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* published component, machine, dir, no signature, misdbLocatorTypeDirectory */
    r = add_complocator_entry(hdb, "'NewSignature6', '{C0ECD96F-7898-4410-9667-194BD8C1B648}', 0");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* published component, machine, file, no signature, misdbLocatorTypeFile */
    r = add_complocator_entry(hdb, "'NewSignature7', '{DB20F535-9C26-4127-9C2B-CC45A8B51DA1}', 1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* unpublished component, no signature, misdbLocatorTypeDir */
    r = add_complocator_entry(hdb, "'NewSignature8', '{FB671D5B-5083-4048-90E0-481C48D8F3A5}', 0");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* published component, no signature, dir does not exist misdbLocatorTypeDir */
    r = add_complocator_entry(hdb, "'NewSignature9', '{91B7359B-07F2-4221-AA8D-DE102BB87A5F}', 0");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* published component, signature w/ ver, misdbLocatorTypeFile */
    r = add_complocator_entry(hdb, "'NewSignature10', '{4A2E1B5B-4034-4177-833B-8CC35F1B3EF1}', 1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* published component, signature w/ ver, ver > max, misdbLocatorTypeFile */
    r = add_complocator_entry(hdb, "'NewSignature11', '{A204DF48-7346-4635-BA2E-66247DBAC9DF}', 1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* published component, signature w/ ver, sig->name ignored, misdbLocatorTypeFile */
    r = add_complocator_entry(hdb, "'NewSignature12', '{EC30CE73-4CF9-4908-BABD-1ED82E1515FD}', 1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = create_signature_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_signature_entry(hdb, "'NewSignature1', 'FileName1', '', '', '', '', '', '', ''");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_signature_entry(hdb, "'NewSignature2', 'FileName2', '', '', '', '', '', '', ''");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_signature_entry(hdb, "'NewSignature3', 'FileName3', '', '', '', '', '', '', ''");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_signature_entry(hdb, "'NewSignature4', 'FileName4', '', '', '', '', '', '', ''");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_signature_entry(hdb, "'NewSignature5', 'FileName5', '', '', '', '', '', '', ''");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_signature_entry(hdb, "'NewSignature10', 'FileName8.dll', '1.1.1.1', '2.1.1.1', '', '', '', '', ''");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_signature_entry(hdb, "'NewSignature11', 'FileName9.dll', '1.1.1.1', '2.1.1.1', '', '', '', '', ''");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_signature_entry(hdb, "'NewSignature12', 'ignored', '1.1.1.1', '2.1.1.1', '', '', '', '', ''");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = package_from_db(hdb, &hpkg);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected a valid package handle %u\n", r);

    r = MsiSetPropertyA(hpkg, "SIGPROP8", "october");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiDoActionA(hpkg, "AppSearch");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    strcpy(expected, CURR_DIR);
    if (is_root(CURR_DIR)) expected[2] = 0;

    size = MAX_PATH;
    sprintf(path, "%s\\FileName1", expected);
    r = MsiGetPropertyA(hpkg, "SIGPROP1", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    sprintf(path, "%s\\FileName2", expected);
    r = MsiGetPropertyA(hpkg, "SIGPROP2", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    sprintf(path, "%s\\FileName3", expected);
    r = MsiGetPropertyA(hpkg, "SIGPROP3", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    sprintf(path, "%s\\FileName4", expected);
    r = MsiGetPropertyA(hpkg, "SIGPROP4", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected \"\", got \"%s\"\n", prop);

    size = MAX_PATH;
    sprintf(path, "%s\\FileName5", expected);
    r = MsiGetPropertyA(hpkg, "SIGPROP5", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    sprintf(path, "%s\\", expected);
    r = MsiGetPropertyA(hpkg, "SIGPROP6", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    sprintf(path, "%s\\", expected);
    r = MsiGetPropertyA(hpkg, "SIGPROP7", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP8", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, "october"), "Expected \"october\", got \"%s\"\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP9", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected \"\", got \"%s\"\n", prop);

    size = MAX_PATH;
    sprintf(path, "%s\\FileName8.dll", expected);
    r = MsiGetPropertyA(hpkg, "SIGPROP10", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP11", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected \"\", got \"%s\"\n", prop);

    size = MAX_PATH;
    sprintf(path, "%s\\FileName10.dll", expected);
    r = MsiGetPropertyA(hpkg, "SIGPROP12", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    delete_component_path("{A8AE6692-96BA-4198-8399-145D7D1D0D0E}",
                          MSIINSTALLCONTEXT_MACHINE, NULL);
    delete_component_path("{1D2CE6F3-E81C-4949-AB81-78D7DAD2AF2E}",
                          MSIINSTALLCONTEXT_USERUNMANAGED, usersid);
    delete_component_path("{19E0B999-85F5-4973-A61B-DBE4D66ECB1D}",
                          MSIINSTALLCONTEXT_USERMANAGED, usersid);
    delete_component_path("{F0CCA976-27A3-4808-9DDD-1A6FD50A0D5A}",
                          MSIINSTALLCONTEXT_MACHINE, NULL);
    delete_component_path("{C0ECD96F-7898-4410-9667-194BD8C1B648}",
                          MSIINSTALLCONTEXT_MACHINE, NULL);
    delete_component_path("{DB20F535-9C26-4127-9C2B-CC45A8B51DA1}",
                          MSIINSTALLCONTEXT_MACHINE, NULL);
    delete_component_path("{91B7359B-07F2-4221-AA8D-DE102BB87A5F}",
                          MSIINSTALLCONTEXT_MACHINE, NULL);
    delete_component_path("{4A2E1B5B-4034-4177-833B-8CC35F1B3EF1}",
                          MSIINSTALLCONTEXT_MACHINE, NULL);
    delete_component_path("{A204DF48-7346-4635-BA2E-66247DBAC9DF}",
                          MSIINSTALLCONTEXT_MACHINE, NULL);
    delete_component_path("{EC30CE73-4CF9-4908-BABD-1ED82E1515FD}",
                          MSIINSTALLCONTEXT_MACHINE, NULL);

    MsiCloseHandle(hpkg);

error:
    DeleteFileA("FileName1");
    DeleteFileA("FileName2");
    DeleteFileA("FileName3");
    DeleteFileA("FileName4");
    DeleteFileA("FileName5");
    DeleteFileA("FileName6");
    DeleteFileA("FileName7");
    DeleteFileA("FileName8.dll");
    DeleteFileA("FileName9.dll");
    DeleteFileA("FileName10.dll");
    DeleteFileA(msifile);
    LocalFree(usersid);
}

static void test_appsearch_reglocator(void)
{
    MSIHANDLE hpkg, hdb;
    char path[MAX_PATH], expected[MAX_PATH], prop[MAX_PATH];
    DWORD binary[2], size, val;
    BOOL space, version, is_64bit = sizeof(void *) > sizeof(int);
    HKEY hklm, classes, hkcu, users;
    LPSTR pathdata, pathvar, ptr;
    LPCSTR str;
    LONG res;
    UINT r, type = 0;
    SYSTEM_INFO si;

    version = TRUE;
    if (!create_file_with_version("test.dll", MAKELONG(2, 1), MAKELONG(4, 3)))
        version = FALSE;

    DeleteFileA("test.dll");

    res = RegCreateKeyA(HKEY_CLASSES_ROOT, "Software\\Wine", &classes);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = RegSetValueExA(classes, "Value1", 0, REG_SZ,
                         (const BYTE *)"regszdata", 10);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = RegCreateKeyA(HKEY_CURRENT_USER, "Software\\Wine", &hkcu);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = RegSetValueExA(hkcu, "Value1", 0, REG_SZ,
                         (const BYTE *)"regszdata", 10);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    users = 0;
    res = RegCreateKeyA(HKEY_USERS, "S-1-5-18\\Software\\Wine", &users);
    ok(res == ERROR_SUCCESS ||
       broken(res == ERROR_INVALID_PARAMETER),
       "Expected ERROR_SUCCESS, got %d\n", res);

    if (res == ERROR_SUCCESS)
    {
        res = RegSetValueExA(users, "Value1", 0, REG_SZ,
                             (const BYTE *)"regszdata", 10);
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    }

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, "Software\\Wine", &hklm);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = RegSetValueA(hklm, NULL, REG_SZ, "defvalue", 8);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = RegSetValueExA(hklm, "Value1", 0, REG_SZ,
                         (const BYTE *)"regszdata", 10);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    val = 42;
    res = RegSetValueExA(hklm, "Value2", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    val = -42;
    res = RegSetValueExA(hklm, "Value3", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = RegSetValueExA(hklm, "Value4", 0, REG_EXPAND_SZ,
                         (const BYTE *)"%PATH%", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = RegSetValueExA(hklm, "Value5", 0, REG_EXPAND_SZ,
                         (const BYTE *)"my%NOVAR%", 10);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = RegSetValueExA(hklm, "Value6", 0, REG_MULTI_SZ,
                         (const BYTE *)"one\0two\0", 9);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    binary[0] = 0x1234abcd;
    binary[1] = 0x567890ef;
    res = RegSetValueExA(hklm, "Value7", 0, REG_BINARY,
                         (const BYTE *)binary, sizeof(binary));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = RegSetValueExA(hklm, "Value8", 0, REG_SZ,
                         (const BYTE *)"#regszdata", 11);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    strcpy(expected, CURR_DIR);
    if (is_root(CURR_DIR)) expected[2] = 0;

    create_test_file("FileName1");
    sprintf(path, "%s\\FileName1", expected);
    res = RegSetValueExA(hklm, "Value9", 0, REG_SZ,
                         (const BYTE *)path, lstrlenA(path) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    sprintf(path, "%s\\FileName2", expected);
    res = RegSetValueExA(hklm, "Value10", 0, REG_SZ,
                         (const BYTE *)path, lstrlenA(path) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    lstrcpyA(path, expected);
    res = RegSetValueExA(hklm, "Value11", 0, REG_SZ,
                         (const BYTE *)path, lstrlenA(path) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = RegSetValueExA(hklm, "Value12", 0, REG_SZ,
                         (const BYTE *)"", 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    create_file_with_version("FileName3.dll", MAKELONG(2, 1), MAKELONG(4, 3));
    sprintf(path, "%s\\FileName3.dll", expected);
    res = RegSetValueExA(hklm, "Value13", 0, REG_SZ,
                         (const BYTE *)path, lstrlenA(path) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    create_file_with_version("FileName4.dll", MAKELONG(1, 2), MAKELONG(3, 4));
    sprintf(path, "%s\\FileName4.dll", expected);
    res = RegSetValueExA(hklm, "Value14", 0, REG_SZ,
                         (const BYTE *)path, lstrlenA(path) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    create_file_with_version("FileName5.dll", MAKELONG(2, 1), MAKELONG(4, 3));
    sprintf(path, "%s\\FileName5.dll", expected);
    res = RegSetValueExA(hklm, "Value15", 0, REG_SZ,
                         (const BYTE *)path, lstrlenA(path) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    sprintf(path, "\"%s\\FileName1\" -option", expected);
    res = RegSetValueExA(hklm, "value16", 0, REG_SZ,
                         (const BYTE *)path, lstrlenA(path) + 1);
    ok( res == ERROR_SUCCESS, "Expected ERROR_SUCCESS got %d\n", res);

    space = strchr(expected, ' ') != NULL;
    sprintf(path, "%s\\FileName1 -option", expected);
    res = RegSetValueExA(hklm, "value17", 0, REG_SZ,
                         (const BYTE *)path, lstrlenA(path) + 1);
    ok( res == ERROR_SUCCESS, "Expected ERROR_SUCCESS got %d\n", res);

    hdb = create_package_db();
    ok(hdb, "Expected a valid database handle\n");

    r = create_appsearch_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP1', 'NewSignature1'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP2', 'NewSignature2'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP3', 'NewSignature3'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP4', 'NewSignature4'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP5', 'NewSignature5'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP6', 'NewSignature6'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP7', 'NewSignature7'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP8', 'NewSignature8'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP9', 'NewSignature9'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP10', 'NewSignature10'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP11', 'NewSignature11'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP12', 'NewSignature12'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP13', 'NewSignature13'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP14', 'NewSignature14'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP15', 'NewSignature15'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP16', 'NewSignature16'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP17', 'NewSignature17'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP18', 'NewSignature18'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP19', 'NewSignature19'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP20', 'NewSignature20'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP21', 'NewSignature21'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP22', 'NewSignature22'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP23', 'NewSignature23'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP24', 'NewSignature24'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP25', 'NewSignature25'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP26', 'NewSignature26'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP27', 'NewSignature27'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP28', 'NewSignature28'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP29', 'NewSignature29'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP30', 'NewSignature30'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = create_reglocator_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    type = msidbLocatorTypeRawValue;
    if (is_64bit)
        type |= msidbLocatorType64bit;

    /* HKLM, msidbLocatorTypeRawValue, REG_SZ */
    r = add_reglocator_entry(hdb, "NewSignature1", 2, "Software\\Wine", "Value1", type);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeRawValue, positive DWORD */
    r = add_reglocator_entry(hdb, "NewSignature2", 2, "Software\\Wine", "Value2", type);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeRawValue, negative DWORD */
    r = add_reglocator_entry(hdb, "NewSignature3", 2, "Software\\Wine", "Value3", type);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeRawValue, REG_EXPAND_SZ */
    r = add_reglocator_entry(hdb, "NewSignature4", 2, "Software\\Wine", "Value4", type);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeRawValue, REG_EXPAND_SZ */
    r = add_reglocator_entry(hdb, "NewSignature5", 2, "Software\\Wine", "Value5", type);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeRawValue, REG_MULTI_SZ */
    r = add_reglocator_entry(hdb, "NewSignature6", 2, "Software\\Wine", "Value6", type);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeRawValue, REG_BINARY */
    r = add_reglocator_entry(hdb, "NewSignature7", 2, "Software\\Wine", "Value7", type);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeRawValue, REG_SZ first char is # */
    r = add_reglocator_entry(hdb, "NewSignature8", 2, "Software\\Wine", "Value8", type);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    type = msidbLocatorTypeFileName;
    if (is_64bit)
        type |= msidbLocatorType64bit;

    /* HKLM, msidbLocatorTypeFileName, signature, file exists */
    r = add_reglocator_entry(hdb, "NewSignature9", 2, "Software\\Wine", "Value9", type);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeFileName, signature, file does not exist */
    r = add_reglocator_entry(hdb, "NewSignature10", 2, "Software\\Wine", "Value10", type);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeFileName, no signature */
    r = add_reglocator_entry(hdb, "NewSignature11", 2, "Software\\Wine", "Value9", type);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    type = msidbLocatorTypeDirectory;
    if (is_64bit)
        type |= msidbLocatorType64bit;

    /* HKLM, msidbLocatorTypeDirectory, no signature, file exists */
    r = add_reglocator_entry(hdb, "NewSignature12", 2, "Software\\Wine", "Value9", type);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeDirectory, no signature, directory exists */
    r = add_reglocator_entry(hdb, "NewSignature13", 2, "Software\\Wine", "Value11", type);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeDirectory, signature, file exists */
    r = add_reglocator_entry(hdb, "NewSignature14", 2, "Software\\Wine", "Value9", type);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    type = msidbLocatorTypeRawValue;
    if (is_64bit)
        type |= msidbLocatorType64bit;

    /* HKCR, msidbLocatorTypeRawValue, REG_SZ */
    r = add_reglocator_entry(hdb, "NewSignature15", 0, "Software\\Wine", "Value1", type);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKCU, msidbLocatorTypeRawValue, REG_SZ */
    r = add_reglocator_entry(hdb, "NewSignature16", 1, "Software\\Wine", "Value1", type);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKU, msidbLocatorTypeRawValue, REG_SZ */
    r = add_reglocator_entry(hdb, "NewSignature17", 3, "S-1-5-18\\Software\\Wine", "Value1", type);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeRawValue, REG_SZ, NULL Name */
    r = add_reglocator_entry(hdb, "NewSignature18", 2, "Software\\Wine", "", type);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeRawValue, REG_SZ, key does not exist */
    r = add_reglocator_entry(hdb, "NewSignature19", 2, "Software\\IDontExist", "", type);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeRawValue, REG_SZ, value is empty */
    r = add_reglocator_entry(hdb, "NewSignature20", 2, "Software\\Wine", "Value12", type);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    type = msidbLocatorTypeFileName;
    if (is_64bit)
        type |= msidbLocatorType64bit;

    /* HKLM, msidbLocatorTypeFileName, signature, file exists w/ version */
    r = add_reglocator_entry(hdb, "NewSignature21", 2, "Software\\Wine", "Value13", type);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeFileName, file exists w/ version, version > max */
    r = add_reglocator_entry(hdb, "NewSignature22", 2, "Software\\Wine", "Value14", type);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeFileName, file exists w/ version, sig->name ignored */
    r = add_reglocator_entry(hdb, "NewSignature23", 2, "Software\\Wine", "Value15", type);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeFileName, no signature, directory exists */
    r = add_reglocator_entry(hdb, "NewSignature24", 2, "Software\\Wine", "Value11", type);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeFileName, no signature, file does not exist */
    r = add_reglocator_entry(hdb, "NewSignature25", 2, "Software\\Wine", "Value10", type);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    type = msidbLocatorTypeDirectory;
    if (is_64bit)
        type |= msidbLocatorType64bit;

    /* HKLM, msidbLocatorTypeDirectory, signature, directory exists */
    r = add_reglocator_entry(hdb, "NewSignature26", 2, "Software\\Wine", "Value11", type);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeDirectory, signature, file does not exist */
    r = add_reglocator_entry(hdb, "NewSignature27", 2, "Software\\Wine", "Value10", type);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeDirectory, no signature, file does not exist */
    r = add_reglocator_entry(hdb, "NewSignature28", 2, "Software\\Wine", "Value10", type);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    type = msidbLocatorTypeFileName;
    if (is_64bit)
        type |= msidbLocatorType64bit;

    /* HKLM, msidbLocatorTypeFile, file exists, in quotes */
    r = add_reglocator_entry(hdb, "NewSignature29", 2, "Software\\Wine", "Value16", type);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeFile, file exists, no quotes */
    r = add_reglocator_entry(hdb, "NewSignature30", 2, "Software\\Wine", "Value17", type);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = create_signature_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    str = "'NewSignature9', 'FileName1', '', '', '', '', '', '', ''";
    r = add_signature_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    str = "'NewSignature10', 'FileName2', '', '', '', '', '', '', ''";
    r = add_signature_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    str = "'NewSignature14', 'FileName1', '', '', '', '', '', '', ''";
    r = add_signature_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    str = "'NewSignature21', 'FileName3.dll', '1.1.1.1', '2.1.1.1', '', '', '', '', ''";
    r = add_signature_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    str = "'NewSignature22', 'FileName4.dll', '1.1.1.1', '2.1.1.1', '', '', '', '', ''";
    r = add_signature_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    str = "'NewSignature23', 'ignored', '1.1.1.1', '2.1.1.1', '', '', '', '', ''";
    r = add_signature_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    if (!is_root(CURR_DIR))
    {
        ptr = strrchr(expected, '\\') + 1;
        sprintf(path, "'NewSignature26', '%s', '', '', '', '', '', '', ''", ptr);
        r = add_signature_entry(hdb, path);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    }
    str = "'NewSignature27', 'FileName2', '', '', '', '', '', '', ''";
    r = add_signature_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    str = "'NewSignature29', 'FileName1', '', '', '', '', '', '', ''";
    r = add_signature_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    str = "'NewSignature30', 'FileName1', '', '', '', '', '', '', ''";
    r = add_signature_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = package_from_db(hdb, &hpkg);
    ok(r == ERROR_SUCCESS, "Expected a valid package handle %u\n", r);

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiDoActionA(hpkg, "AppSearch");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP1", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, "regszdata"),
       "Expected \"regszdata\", got \"%s\"\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP2", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, "#42"), "Expected \"#42\", got \"%s\"\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP3", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, "#-42"), "Expected \"#-42\", got \"%s\"\n", prop);

    memset(&si, 0, sizeof(si));
    if (pGetNativeSystemInfo) pGetNativeSystemInfo(&si);

    if (S(U(si)).wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
    {
        size = ExpandEnvironmentStringsA("%PATH%", NULL, 0);
        pathvar = HeapAlloc(GetProcessHeap(), 0, size);
        ExpandEnvironmentStringsA("%PATH%", pathvar, size);

        size = 0;
        r = MsiGetPropertyA(hpkg, "SIGPROP4", NULL, &size);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

        pathdata = HeapAlloc(GetProcessHeap(), 0, ++size);
        r = MsiGetPropertyA(hpkg, "SIGPROP4", pathdata, &size);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(pathdata, pathvar),
            "Expected \"%s\", got \"%s\"\n", pathvar, pathdata);

        HeapFree(GetProcessHeap(), 0, pathvar);
        HeapFree(GetProcessHeap(), 0, pathdata);
    }

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP5", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop,
       "my%NOVAR%"), "Expected \"my%%NOVAR%%\", got \"%s\"\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP6", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    todo_wine
    {
        ok(!memcmp(prop, "\0one\0two\0\0", 10),
           "Expected \"\\0one\\0two\\0\\0\"\n");
    }

    size = MAX_PATH;
    lstrcpyA(path, "#xCDAB3412EF907856");
    r = MsiGetPropertyA(hpkg, "SIGPROP7", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP8", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, "##regszdata"),
       "Expected \"##regszdata\", got \"%s\"\n", prop);

    size = MAX_PATH;
    sprintf(path, "%s\\FileName1", expected);
    r = MsiGetPropertyA(hpkg, "SIGPROP9", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP10", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected \"\", got \"%s\"\n", prop);

    size = MAX_PATH;
    sprintf(path, "%s\\", expected);
    r = MsiGetPropertyA(hpkg, "SIGPROP11", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP12", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected \"\", got \"%s\"\n", prop);

    size = MAX_PATH;
    sprintf(path, "%s\\", expected);
    r = MsiGetPropertyA(hpkg, "SIGPROP13", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP14", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected \"\", got \"%s\"\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP15", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, "regszdata"),
       "Expected \"regszdata\", got \"%s\"\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP16", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, "regszdata"),
       "Expected \"regszdata\", got \"%s\"\n", prop);

    if (users)
    {
        size = MAX_PATH;
        r = MsiGetPropertyA(hpkg, "SIGPROP17", prop, &size);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(prop, "regszdata"),
           "Expected \"regszdata\", got \"%s\"\n", prop);
    }

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP18", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, "defvalue"),
       "Expected \"defvalue\", got \"%s\"\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP19", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected \"\", got \"%s\"\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP20", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected \"\", got \"%s\"\n", prop);

    if (version)
    {
        size = MAX_PATH;
        sprintf(path, "%s\\FileName3.dll", expected);
        r = MsiGetPropertyA(hpkg, "SIGPROP21", prop, &size);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

        size = MAX_PATH;
        r = MsiGetPropertyA(hpkg, "SIGPROP22", prop, &size);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(prop, ""), "Expected \"\", got \"%s\"\n", prop);

        size = MAX_PATH;
        sprintf(path, "%s\\FileName5.dll", expected);
        r = MsiGetPropertyA(hpkg, "SIGPROP23", prop, &size);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);
    }

    if (!is_root(CURR_DIR))
    {
        size = MAX_PATH;
        lstrcpyA(path, expected);
        ptr = strrchr(path, '\\') + 1;
        *ptr = '\0';
        r = MsiGetPropertyA(hpkg, "SIGPROP24", prop, &size);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);
    }
    size = MAX_PATH;
    sprintf(path, "%s\\", expected);
    r = MsiGetPropertyA(hpkg, "SIGPROP25", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP26", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    if (is_root(CURR_DIR))
        ok(!lstrcmpA(prop, CURR_DIR), "Expected \"%s\", got \"%s\"\n", CURR_DIR, prop);
    else
        ok(!lstrcmpA(prop, ""), "Expected \"\", got \"%s\"\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP27", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected \"\", got \"%s\"\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP28", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected \"\", got \"%s\"\n", prop);

    size = MAX_PATH;
    sprintf(path, "%s\\FileName1", expected);
    r = MsiGetPropertyA(hpkg, "SIGPROP29", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    sprintf(path, "%s\\FileName1", expected);
    r = MsiGetPropertyA(hpkg, "SIGPROP30", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    if (space)
        ok(!lstrcmpA(prop, ""), "Expected \"\", got \"%s\"\n", prop);
    else
        todo_wine ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    RegSetValueA(hklm, NULL, REG_SZ, "", 0);
    RegDeleteValueA(hklm, "Value1");
    RegDeleteValueA(hklm, "Value2");
    RegDeleteValueA(hklm, "Value3");
    RegDeleteValueA(hklm, "Value4");
    RegDeleteValueA(hklm, "Value5");
    RegDeleteValueA(hklm, "Value6");
    RegDeleteValueA(hklm, "Value7");
    RegDeleteValueA(hklm, "Value8");
    RegDeleteValueA(hklm, "Value9");
    RegDeleteValueA(hklm, "Value10");
    RegDeleteValueA(hklm, "Value11");
    RegDeleteValueA(hklm, "Value12");
    RegDeleteValueA(hklm, "Value13");
    RegDeleteValueA(hklm, "Value14");
    RegDeleteValueA(hklm, "Value15");
    RegDeleteValueA(hklm, "Value16");
    RegDeleteValueA(hklm, "Value17");
    RegDeleteKeyA(hklm, "");
    RegCloseKey(hklm);

    RegDeleteValueA(classes, "Value1");
    RegDeleteKeyA(classes, "");
    RegCloseKey(classes);

    RegDeleteValueA(hkcu, "Value1");
    RegDeleteKeyA(hkcu, "");
    RegCloseKey(hkcu);

    RegDeleteValueA(users, "Value1");
    RegDeleteKeyA(users, "");
    RegCloseKey(users);

    DeleteFileA("FileName1");
    DeleteFileA("FileName3.dll");
    DeleteFileA("FileName4.dll");
    DeleteFileA("FileName5.dll");
    MsiCloseHandle(hpkg);
    DeleteFileA(msifile);
}

static void delete_win_ini(LPCSTR file)
{
    CHAR path[MAX_PATH];

    GetWindowsDirectoryA(path, MAX_PATH);
    lstrcatA(path, "\\");
    lstrcatA(path, file);

    DeleteFileA(path);
}

static void test_appsearch_inilocator(void)
{
    MSIHANDLE hpkg, hdb;
    char path[MAX_PATH], expected[MAX_PATH], prop[MAX_PATH];
    BOOL version;
    LPCSTR str;
    LPSTR ptr;
    DWORD size;
    UINT r;

    version = TRUE;
    if (!create_file_with_version("test.dll", MAKELONG(2, 1), MAKELONG(4, 3)))
        version = FALSE;

    DeleteFileA("test.dll");

    WritePrivateProfileStringA("Section", "Key", "keydata,field2", "IniFile.ini");

    strcpy(expected, CURR_DIR);
    if (is_root(CURR_DIR)) expected[2] = 0;

    create_test_file("FileName1");
    sprintf(path, "%s\\FileName1", expected);
    WritePrivateProfileStringA("Section", "Key2", path, "IniFile.ini");

    WritePrivateProfileStringA("Section", "Key3", expected, "IniFile.ini");

    sprintf(path, "%s\\IDontExist", expected);
    WritePrivateProfileStringA("Section", "Key4", path, "IniFile.ini");

    create_file_with_version("FileName2.dll", MAKELONG(2, 1), MAKELONG(4, 3));
    sprintf(path, "%s\\FileName2.dll", expected);
    WritePrivateProfileStringA("Section", "Key5", path, "IniFile.ini");

    create_file_with_version("FileName3.dll", MAKELONG(1, 2), MAKELONG(3, 4));
    sprintf(path, "%s\\FileName3.dll", expected);
    WritePrivateProfileStringA("Section", "Key6", path, "IniFile.ini");

    create_file_with_version("FileName4.dll", MAKELONG(2, 1), MAKELONG(4, 3));
    sprintf(path, "%s\\FileName4.dll", expected);
    WritePrivateProfileStringA("Section", "Key7", path, "IniFile.ini");

    hdb = create_package_db();
    ok(hdb, "Expected a valid database handle\n");

    r = create_appsearch_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP1', 'NewSignature1'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP2', 'NewSignature2'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP3', 'NewSignature3'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP4', 'NewSignature4'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP5', 'NewSignature5'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP6', 'NewSignature6'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP7', 'NewSignature7'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP8', 'NewSignature8'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP9', 'NewSignature9'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP10', 'NewSignature10'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP11', 'NewSignature11'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP12', 'NewSignature12'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = create_inilocator_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* msidbLocatorTypeRawValue, field 1 */
    str = "'NewSignature1', 'IniFile.ini', 'Section', 'Key', 1, 2";
    r = add_inilocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* msidbLocatorTypeRawValue, field 2 */
    str = "'NewSignature2', 'IniFile.ini', 'Section', 'Key', 2, 2";
    r = add_inilocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* msidbLocatorTypeRawValue, entire field */
    str = "'NewSignature3', 'IniFile.ini', 'Section', 'Key', 0, 2";
    r = add_inilocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* msidbLocatorTypeFile */
    str = "'NewSignature4', 'IniFile.ini', 'Section', 'Key2', 1, 1";
    r = add_inilocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* msidbLocatorTypeDirectory, file */
    str = "'NewSignature5', 'IniFile.ini', 'Section', 'Key2', 1, 0";
    r = add_inilocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* msidbLocatorTypeDirectory, directory */
    str = "'NewSignature6', 'IniFile.ini', 'Section', 'Key3', 1, 0";
    r = add_inilocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* msidbLocatorTypeFile, file, no signature */
    str = "'NewSignature7', 'IniFile.ini', 'Section', 'Key2', 1, 1";
    r = add_inilocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* msidbLocatorTypeFile, dir, no signature */
    str = "'NewSignature8', 'IniFile.ini', 'Section', 'Key3', 1, 1";
    r = add_inilocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* msidbLocatorTypeFile, file does not exist */
    str = "'NewSignature9', 'IniFile.ini', 'Section', 'Key4', 1, 1";
    r = add_inilocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* msidbLocatorTypeFile, signature with version */
    str = "'NewSignature10', 'IniFile.ini', 'Section', 'Key5', 1, 1";
    r = add_inilocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* msidbLocatorTypeFile, signature with version, ver > max */
    str = "'NewSignature11', 'IniFile.ini', 'Section', 'Key6', 1, 1";
    r = add_inilocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* msidbLocatorTypeFile, signature with version, sig->name ignored */
    str = "'NewSignature12', 'IniFile.ini', 'Section', 'Key7', 1, 1";
    r = add_inilocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = create_signature_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_signature_entry(hdb, "'NewSignature4', 'FileName1', '', '', '', '', '', '', ''");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_signature_entry(hdb, "'NewSignature9', 'IDontExist', '', '', '', '', '', '', ''");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_signature_entry(hdb, "'NewSignature10', 'FileName2.dll', '1.1.1.1', '2.1.1.1', '', '', '', '', ''");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_signature_entry(hdb, "'NewSignature11', 'FileName3.dll', '1.1.1.1', '2.1.1.1', '', '', '', '', ''");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_signature_entry(hdb, "'NewSignature12', 'ignored', '1.1.1.1', '2.1.1.1', '', '', '', '', ''");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = package_from_db(hdb, &hpkg);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected a valid package handle %u\n", r);

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiDoActionA(hpkg, "AppSearch");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP1", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, "keydata"), "Expected \"keydata\", got \"%s\"\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP2", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, "field2"), "Expected \"field2\", got \"%s\"\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP3", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, "keydata,field2"),
       "Expected \"keydata,field2\", got \"%s\"\n", prop);

    size = MAX_PATH;
    sprintf(path, "%s\\FileName1", expected);
    r = MsiGetPropertyA(hpkg, "SIGPROP4", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP5", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected \"\", got \"%s\"\n", prop);

    size = MAX_PATH;
    sprintf(path, "%s\\", expected);
    r = MsiGetPropertyA(hpkg, "SIGPROP6", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    sprintf(path, "%s\\", expected);
    r = MsiGetPropertyA(hpkg, "SIGPROP7", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    if (!is_root(CURR_DIR))
    {
        size = MAX_PATH;
        lstrcpyA(path, expected);
        ptr = strrchr(path, '\\');
        *(ptr + 1) = 0;
        r = MsiGetPropertyA(hpkg, "SIGPROP8", prop, &size);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);
    }
    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP9", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected \"\", got \"%s\"\n", prop);

    if (version)
    {
        size = MAX_PATH;
        sprintf(path, "%s\\FileName2.dll", expected);
        r = MsiGetPropertyA(hpkg, "SIGPROP10", prop, &size);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

        size = MAX_PATH;
        r = MsiGetPropertyA(hpkg, "SIGPROP11", prop, &size);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(prop, ""), "Expected \"\", got \"%s\"\n", prop);

        size = MAX_PATH;
        sprintf(path, "%s\\FileName4.dll", expected);
        r = MsiGetPropertyA(hpkg, "SIGPROP12", prop, &size);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);
    }

    MsiCloseHandle(hpkg);

error:
    delete_win_ini("IniFile.ini");
    DeleteFileA("FileName1");
    DeleteFileA("FileName2.dll");
    DeleteFileA("FileName3.dll");
    DeleteFileA("FileName4.dll");
    DeleteFileA(msifile);
}

/*
 * MSI AppSearch action on DrLocator table always returns absolute paths.
 * If a relative path was set, it returns the first absolute path that
 * matches or an empty string if it didn't find anything.
 * This helper function replicates this behaviour.
 */
static void search_absolute_directory(LPSTR absolute, LPCSTR relative)
{
    int i, size;
    DWORD attr, drives;

    size = lstrlenA(relative);
    drives = GetLogicalDrives();
    lstrcpyA(absolute, "A:\\");
    for (i = 0; i < 26; absolute[0] = '\0', i++)
    {
        if (!(drives & (1 << i)))
            continue;

        absolute[0] = 'A' + i;
        if (GetDriveTypeA(absolute) != DRIVE_FIXED)
            continue;

        lstrcpynA(absolute + 3, relative, size + 1);
        attr = GetFileAttributesA(absolute);
        if (attr != INVALID_FILE_ATTRIBUTES &&
            (attr & FILE_ATTRIBUTE_DIRECTORY))
        {
            if (absolute[3 + size - 1] != '\\')
                lstrcatA(absolute, "\\");
            break;
        }
        absolute[3] = '\0';
    }
}

static void test_appsearch_drlocator(void)
{
    MSIHANDLE hpkg, hdb;
    char path[MAX_PATH], expected[MAX_PATH], prop[MAX_PATH];
    BOOL version;
    LPCSTR str;
    DWORD size;
    UINT r;

    version = TRUE;
    if (!create_file_with_version("test.dll", MAKELONG(2, 1), MAKELONG(4, 3)))
        version = FALSE;

    DeleteFileA("test.dll");

    create_test_file("FileName1");
    CreateDirectoryA("one", NULL);
    CreateDirectoryA("one\\two", NULL);
    CreateDirectoryA("one\\two\\three", NULL);
    create_test_file("one\\two\\three\\FileName2");
    CreateDirectoryA("another", NULL);
    create_file_with_version("FileName3.dll", MAKELONG(2, 1), MAKELONG(4, 3));
    create_file_with_version("FileName4.dll", MAKELONG(1, 2), MAKELONG(3, 4));
    create_file_with_version("FileName5.dll", MAKELONG(2, 1), MAKELONG(4, 3));

    hdb = create_package_db();
    ok(hdb, "Expected a valid database handle\n");

    r = create_appsearch_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP1', 'NewSignature1'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP2', 'NewSignature2'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP3', 'NewSignature3'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP4', 'NewSignature4'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP5', 'NewSignature5'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP6', 'NewSignature6'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP7', 'NewSignature7'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP8', 'NewSignature8'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP9', 'NewSignature9'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP10', 'NewSignature10'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP11', 'NewSignature11'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'SIGPROP13', 'NewSignature13'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = create_drlocator_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    strcpy(expected, CURR_DIR);
    if (is_root(CURR_DIR)) expected[2] = 0;

    /* no parent, full path, depth 0, signature */
    sprintf(path, "'NewSignature1', '', '%s', 0", expected);
    r = add_drlocator_entry(hdb, path);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* no parent, full path, depth 0, no signature */
    sprintf(path, "'NewSignature2', '', '%s', 0", expected);
    r = add_drlocator_entry(hdb, path);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* no parent, relative path, depth 0, no signature */
    sprintf(path, "'NewSignature3', '', '%s', 0", expected + 3);
    r = add_drlocator_entry(hdb, path);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* no parent, full path, depth 2, signature */
    sprintf(path, "'NewSignature4', '', '%s', 2", expected);
    r = add_drlocator_entry(hdb, path);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* no parent, full path, depth 3, signature */
    sprintf(path, "'NewSignature5', '', '%s', 3", expected);
    r = add_drlocator_entry(hdb, path);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* no parent, full path, depth 1, signature is dir */
    sprintf(path, "'NewSignature6', '', '%s', 1", expected);
    r = add_drlocator_entry(hdb, path);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* parent is in DrLocator, relative path, depth 0, signature */
    sprintf(path, "'NewSignature7', 'NewSignature1', 'one\\two\\three', 1");
    r = add_drlocator_entry(hdb, path);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* no parent, full path, depth 0, signature w/ version */
    sprintf(path, "'NewSignature8', '', '%s', 0", expected);
    r = add_drlocator_entry(hdb, path);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* no parent, full path, depth 0, signature w/ version, ver > max */
    sprintf(path, "'NewSignature9', '', '%s', 0", expected);
    r = add_drlocator_entry(hdb, path);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* no parent, full path, depth 0, signature w/ version, sig->name not ignored */
    sprintf(path, "'NewSignature10', '', '%s', 0", expected);
    r = add_drlocator_entry(hdb, path);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* no parent, relative empty path, depth 0, no signature */
    sprintf(path, "'NewSignature11', '', '', 0");
    r = add_drlocator_entry(hdb, path);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = create_reglocator_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* parent */
    r = add_reglocator_entry(hdb, "NewSignature12", 2, "htmlfile\\shell\\open\\nonexistent", "", 1);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* parent is in RegLocator, no path, depth 0, no signature */
    sprintf(path, "'NewSignature13', 'NewSignature12', '', 0");
    r = add_drlocator_entry(hdb, path);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = create_signature_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    str = "'NewSignature1', 'FileName1', '', '', '', '', '', '', ''";
    r = add_signature_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    str = "'NewSignature4', 'FileName2', '', '', '', '', '', '', ''";
    r = add_signature_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    str = "'NewSignature5', 'FileName2', '', '', '', '', '', '', ''";
    r = add_signature_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    str = "'NewSignature6', 'another', '', '', '', '', '', '', ''";
    r = add_signature_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    str = "'NewSignature7', 'FileName2', '', '', '', '', '', '', ''";
    r = add_signature_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    str = "'NewSignature8', 'FileName3.dll', '1.1.1.1', '2.1.1.1', '', '', '', '', ''";
    r = add_signature_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    str = "'NewSignature9', 'FileName4.dll', '1.1.1.1', '2.1.1.1', '', '', '', '', ''";
    r = add_signature_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    str = "'NewSignature10', 'necessary', '1.1.1.1', '2.1.1.1', '', '', '', '', ''";
    r = add_signature_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = package_from_db(hdb, &hpkg);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected a valid package handle %u\n", r);

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiDoActionA(hpkg, "AppSearch");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    sprintf(path, "%s\\FileName1", expected);
    r = MsiGetPropertyA(hpkg, "SIGPROP1", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    sprintf(path, "%s\\", expected);
    r = MsiGetPropertyA(hpkg, "SIGPROP2", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    search_absolute_directory(path, expected + 3);
    r = MsiGetPropertyA(hpkg, "SIGPROP3", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpiA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP4", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected \"\", got \"%s\"\n", prop);

    size = MAX_PATH;
    sprintf(path, "%s\\one\\two\\three\\FileName2", expected);
    r = MsiGetPropertyA(hpkg, "SIGPROP5", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP6", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected \"\", got \"%s\"\n", prop);

    size = MAX_PATH;
    sprintf(path, "%s\\one\\two\\three\\FileName2", expected);
    r = MsiGetPropertyA(hpkg, "SIGPROP7", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    if (version)
    {
        size = MAX_PATH;
        sprintf(path, "%s\\FileName3.dll", expected);
        r = MsiGetPropertyA(hpkg, "SIGPROP8", prop, &size);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

        size = MAX_PATH;
        r = MsiGetPropertyA(hpkg, "SIGPROP9", prop, &size);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(prop, ""), "Expected \"\", got \"%s\"\n", prop);

        size = MAX_PATH;
        r = MsiGetPropertyA(hpkg, "SIGPROP10", prop, &size);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(prop, ""), "Expected \"\", got \"%s\"\n", prop);
    }

    size = MAX_PATH;
    search_absolute_directory(path, "");
    r = MsiGetPropertyA(hpkg, "SIGPROP11", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpiA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    strcpy(path, "c:\\");
    r = MsiGetPropertyA(hpkg, "SIGPROP13", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!prop[0], "Expected \"\", got \"%s\"\n", prop);

    MsiCloseHandle(hpkg);

error:
    DeleteFileA("FileName1");
    DeleteFileA("FileName3.dll");
    DeleteFileA("FileName4.dll");
    DeleteFileA("FileName5.dll");
    DeleteFileA("one\\two\\three\\FileName2");
    RemoveDirectoryA("one\\two\\three");
    RemoveDirectoryA("one\\two");
    RemoveDirectoryA("one");
    RemoveDirectoryA("another");
    DeleteFileA(msifile);
}

static void test_featureparents(void)
{
    MSIHANDLE hpkg;
    UINT r;
    MSIHANDLE hdb;

    hdb = create_package_db();
    ok ( hdb, "failed to create package database\n" );

    r = add_directory_entry( hdb, "'TARGETDIR', '', 'SourceDir'");
    ok( r == ERROR_SUCCESS, "cannot add directory: %d\n", r );

    r = create_feature_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create Feature table: %d\n", r );

    r = create_component_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create Component table: %d\n", r );

    r = create_feature_components_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create FeatureComponents table: %d\n", r );

    r = create_file_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create File table: %d\n", r );

    /* msidbFeatureAttributesFavorLocal */
    r = add_feature_entry( hdb, "'zodiac', '', '', '', 2, 1, '', 0" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    /* msidbFeatureAttributesFavorSource */
    r = add_feature_entry( hdb, "'perseus', '', '', '', 2, 1, '', 1" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    /* msidbFeatureAttributesFavorLocal */
    r = add_feature_entry( hdb, "'orion', '', '', '', 2, 1, '', 0" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    /* msidbFeatureAttributesUIDisallowAbsent */
    r = add_feature_entry( hdb, "'lyra', '', '', '', 2, 1, '', 16" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    /* disabled because of install level */
    r = add_feature_entry( hdb, "'waters', '', '', '', 15, 101, '', 9" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    /* child feature of disabled feature */
    r = add_feature_entry( hdb, "'bayer', 'waters', '', '', 14, 1, '', 9" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    /* component of disabled feature (install level) */
    r = add_component_entry( hdb, "'delphinus', '', 'TARGETDIR', 0, '', 'delphinus_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* component of disabled child feature (install level) */
    r = add_component_entry( hdb, "'hydrus', '', 'TARGETDIR', 0, '', 'hydrus_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorLocal:msidbComponentAttributesLocalOnly */
    r = add_component_entry( hdb, "'leo', '', 'TARGETDIR', 0, '', 'leo_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorLocal:msidbComponentAttributesSourceOnly */
    r = add_component_entry( hdb, "'virgo', '', 'TARGETDIR', 1, '', 'virgo_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorLocal:msidbComponentAttributesOptional */
    r = add_component_entry( hdb, "'libra', '', 'TARGETDIR', 2, '', 'libra_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorSource:msidbComponentAttributesLocalOnly */
    r = add_component_entry( hdb, "'cassiopeia', '', 'TARGETDIR', 0, '', 'cassiopeia_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorSource:msidbComponentAttributesSourceOnly */
    r = add_component_entry( hdb, "'cepheus', '', 'TARGETDIR', 1, '', 'cepheus_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorSource:msidbComponentAttributesOptional */
    r = add_component_entry( hdb, "'andromeda', '', 'TARGETDIR', 2, '', 'andromeda_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorLocal:msidbComponentAttributesLocalOnly */
    r = add_component_entry( hdb, "'canis', '', 'TARGETDIR', 0, '', 'canis_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorLocal:msidbComponentAttributesSourceOnly */
    r = add_component_entry( hdb, "'monoceros', '', 'TARGETDIR', 1, '', 'monoceros_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorLocal:msidbComponentAttributesOptional */
    r = add_component_entry( hdb, "'lepus', '', 'TARGETDIR', 2, '', 'lepus_file'" );
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS got %d\n", r);

    r = add_feature_components_entry( hdb, "'zodiac', 'leo'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'zodiac', 'virgo'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'zodiac', 'libra'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'perseus', 'cassiopeia'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'perseus', 'cepheus'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'perseus', 'andromeda'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'orion', 'leo'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'orion', 'virgo'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'orion', 'libra'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'orion', 'cassiopeia'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'orion', 'cepheus'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'orion', 'andromeda'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'orion', 'canis'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'orion', 'monoceros'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'orion', 'lepus'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'waters', 'delphinus'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'bayer', 'hydrus'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_file_entry( hdb, "'leo_file', 'leo', 'leo.txt', 100, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'virgo_file', 'virgo', 'virgo.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'libra_file', 'libra', 'libra.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'cassiopeia_file', 'cassiopeia', 'cassiopeia.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'cepheus_file', 'cepheus', 'cepheus.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'andromeda_file', 'andromeda', 'andromeda.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'canis_file', 'canis', 'canis.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'monoceros_file', 'monoceros', 'monoceros.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'lepus_file', 'lepus', 'lepus.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'delphinus_file', 'delphinus', 'delphinus.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'hydrus_file', 'hydrus', 'hydrus.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = package_from_db( hdb, &hpkg );
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        DeleteFileA(msifile);
        return;
    }
    ok( r == ERROR_SUCCESS, "failed to create package %u\n", r );

    MsiCloseHandle( hdb );

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiDoActionA( hpkg, "CostInitialize");
    ok( r == ERROR_SUCCESS, "cost init failed\n");

    r = MsiDoActionA( hpkg, "FileCost");
    ok( r == ERROR_SUCCESS, "file cost failed\n");

    r = MsiDoActionA( hpkg, "CostFinalize");
    ok( r == ERROR_SUCCESS, "cost finalize failed\n");

    test_feature_states( __LINE__, hpkg, "zodiac", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_LOCAL, FALSE );
    test_feature_states( __LINE__, hpkg, "perseus", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_SOURCE, FALSE );
    test_feature_states( __LINE__, hpkg, "orion", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_LOCAL, FALSE );
    test_feature_states( __LINE__, hpkg, "lyra", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_LOCAL, FALSE );
    test_feature_states( __LINE__, hpkg, "waters", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_feature_states( __LINE__, hpkg, "bayer", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );

    test_component_states( __LINE__, hpkg, "leo", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "virgo", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_SOURCE, FALSE );
    test_component_states( __LINE__, hpkg, "libra", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "cassiopeia", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "cepheus", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_SOURCE, FALSE );
    test_component_states( __LINE__, hpkg, "andromeda", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "canis", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "monoceros", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_SOURCE, FALSE );
    test_component_states( __LINE__, hpkg, "lepus", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "delphinus", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "hydrus", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_UNKNOWN, FALSE );

    r = MsiSetFeatureStateA(hpkg, "orion", INSTALLSTATE_ABSENT);
    ok( r == ERROR_SUCCESS, "failed to set feature state: %d\n", r);

    r = MsiSetFeatureStateA(hpkg, "lyra", INSTALLSTATE_ABSENT);
    ok( r == ERROR_SUCCESS, "failed to set feature state: %d\n", r);

    r = MsiSetFeatureStateA(hpkg, "nosuchfeature", INSTALLSTATE_ABSENT);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %u\n", r);

    test_feature_states( __LINE__, hpkg, "zodiac", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_LOCAL, FALSE );
    test_feature_states( __LINE__, hpkg, "perseus", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_SOURCE, FALSE );
    test_feature_states( __LINE__, hpkg, "orion", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_ABSENT, FALSE );
    test_feature_states( __LINE__, hpkg, "lyra", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_ABSENT, FALSE );
    test_feature_states( __LINE__, hpkg, "waters", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );
    test_feature_states( __LINE__, hpkg, "bayer", ERROR_SUCCESS, INSTALLSTATE_ABSENT, INSTALLSTATE_UNKNOWN, FALSE );

    test_component_states( __LINE__, hpkg, "leo", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "virgo", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_SOURCE, FALSE );
    test_component_states( __LINE__, hpkg, "libra", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "cassiopeia", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_LOCAL, FALSE );
    test_component_states( __LINE__, hpkg, "cepheus", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_SOURCE, FALSE );
    test_component_states( __LINE__, hpkg, "andromeda", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_SOURCE, FALSE );
    test_component_states( __LINE__, hpkg, "canis", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "monoceros", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "lepus", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "delphinus", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_UNKNOWN, FALSE );
    test_component_states( __LINE__, hpkg, "hydrus", ERROR_SUCCESS, INSTALLSTATE_UNKNOWN, INSTALLSTATE_UNKNOWN, FALSE );

    MsiCloseHandle(hpkg);
    DeleteFileA(msifile);
}

static void test_installprops(void)
{
    MSIHANDLE hpkg, hdb;
    CHAR path[MAX_PATH], buf[MAX_PATH];
    DWORD size, type;
    LANGID langid;
    HKEY hkey1, hkey2;
    int res;
    UINT r;
    REGSAM access = KEY_ALL_ACCESS;
    SYSTEM_INFO si;
    INSTALLUILEVEL uilevel;

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    lstrcpyA(path, CURR_DIR);
    if (!is_root(CURR_DIR)) lstrcatA(path, "\\");
    lstrcatA(path, msifile);

    uilevel = MsiSetInternalUI(INSTALLUILEVEL_BASIC|INSTALLUILEVEL_SOURCERESONLY, NULL);

    hdb = create_package_db();
    ok( hdb, "failed to create database\n");

    r = package_from_db(hdb, &hpkg);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        MsiSetInternalUI(uilevel, NULL);
        DeleteFileA(msifile);
        return;
    }
    ok( r == ERROR_SUCCESS, "failed to create package %u\n", r );

    MsiCloseHandle(hdb);

    buf[0] = 0;
    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "UILevel", buf, &size);
    ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
    ok( !lstrcmpA(buf, "3"), "Expected \"3\", got \"%s\"\n", buf);

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    buf[0] = 0;
    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "UILevel", buf, &size);
    ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
    ok( !lstrcmpA(buf, "3"), "Expected \"3\", got \"%s\"\n", buf);

    buf[0] = 0;
    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "DATABASE", buf, &size);
    ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
    ok( !lstrcmpA(buf, path), "Expected %s, got %s\n", path, buf);

    RegOpenKeyA(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\MS Setup (ACME)\\User Info", &hkey1);
    RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, access, &hkey2);

    size = MAX_PATH;
    type = REG_SZ;
    *path = '\0';
    if (RegQueryValueExA(hkey1, "DefName", NULL, &type, (LPBYTE)path, &size) != ERROR_SUCCESS)
    {
        size = MAX_PATH;
        type = REG_SZ;
        RegQueryValueExA(hkey2, "RegisteredOwner", NULL, &type, (LPBYTE)path, &size);
    }

    buf[0] = 0;
    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "USERNAME", buf, &size);
    ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
    ok( !lstrcmpA(buf, path), "Expected %s, got %s\n", path, buf);

    size = MAX_PATH;
    type = REG_SZ;
    *path = '\0';
    if (RegQueryValueExA(hkey1, "DefCompany", NULL, &type, (LPBYTE)path, &size) != ERROR_SUCCESS)
    {
        size = MAX_PATH;
        type = REG_SZ;
        RegQueryValueExA(hkey2, "RegisteredOrganization", NULL, &type, (LPBYTE)path, &size);
    }

    if (*path)
    {
        buf[0] = 0;
        size = MAX_PATH;
        r = MsiGetPropertyA(hpkg, "COMPANYNAME", buf, &size);
        ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
        ok( !lstrcmpA(buf, path), "Expected %s, got %s\n", path, buf);
    }

    buf[0] = 0;
    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "VersionDatabase", buf, &size);
    ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
    trace("VersionDatabase = %s\n", buf);

    buf[0] = 0;
    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "VersionMsi", buf, &size);
    ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
    trace("VersionMsi = %s\n", buf);

    buf[0] = 0;
    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "Date", buf, &size);
    ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
    trace("Date = %s\n", buf);

    buf[0] = 0;
    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "Time", buf, &size);
    ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
    trace("Time = %s\n", buf);

    buf[0] = 0;
    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "PackageCode", buf, &size);
    ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
    trace("PackageCode = %s\n", buf);

    buf[0] = 0;
    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "ComputerName", buf, &size);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS got %d\n", r);
    trace("ComputerName = %s\n", buf);

    langid = GetUserDefaultLangID();
    sprintf(path, "%d", langid);

    buf[0] = 0;
    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "UserLanguageID", buf, &size);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS got %d\n", r);
    ok( !lstrcmpA(buf, path), "Expected \"%s\", got \"%s\"\n", path, buf);

    res = GetSystemMetrics(SM_CXSCREEN);
    buf[0] = 0;
    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "ScreenX", buf, &size);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS got %d\n", r);
    ok(atol(buf) == res, "Expected %d, got %ld\n", res, atol(buf));

    res = GetSystemMetrics(SM_CYSCREEN);
    buf[0] = 0;
    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "ScreenY", buf, &size);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS got %d\n", r);
    ok(atol(buf) == res, "Expected %d, got %ld\n", res, atol(buf));

    if (pGetSystemInfo && pSHGetFolderPathA)
    {
        pGetSystemInfo(&si);
        if (S(U(si)).wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
        {
            buf[0] = 0;
            size = MAX_PATH;
            r = MsiGetPropertyA(hpkg, "Intel", buf, &size);
            ok(r == ERROR_SUCCESS, "failed to get property: %d\n", r);
            ok(buf[0], "property not set\n");

            buf[0] = 0;
            size = MAX_PATH;
            r = MsiGetPropertyA(hpkg, "MsiAMD64", buf, &size);
            ok(r == ERROR_SUCCESS, "failed to get property: %d\n", r);
            ok(buf[0], "property not set\n");

            buf[0] = 0;
            size = MAX_PATH;
            r = MsiGetPropertyA(hpkg, "Msix64", buf, &size);
            ok(r == ERROR_SUCCESS, "failed to get property: %d\n", r);
            ok(buf[0], "property not set\n");

            buf[0] = 0;
            size = MAX_PATH;
            r = MsiGetPropertyA(hpkg, "System64Folder", buf, &size);
            ok(r == ERROR_SUCCESS, "failed to get property: %d\n", r);
            GetSystemDirectoryA(path, MAX_PATH);
            if (size) buf[size - 1] = 0;
            ok(!lstrcmpiA(path, buf), "expected \"%s\", got \"%s\"\n", path, buf);

            buf[0] = 0;
            size = MAX_PATH;
            r = MsiGetPropertyA(hpkg, "SystemFolder", buf, &size);
            ok(r == ERROR_SUCCESS, "failed to get property: %d\n", r);
            pGetSystemWow64DirectoryA(path, MAX_PATH);
            if (size) buf[size - 1] = 0;
            ok(!lstrcmpiA(path, buf), "expected \"%s\", got \"%s\"\n", path, buf);

            buf[0] = 0;
            size = MAX_PATH;
            r = MsiGetPropertyA(hpkg, "ProgramFiles64Folder", buf, &size);
            ok(r == ERROR_SUCCESS, "failed to get property: %d\n", r);
            pSHGetFolderPathA(NULL, CSIDL_PROGRAM_FILES, NULL, 0, path);
            if (size) buf[size - 1] = 0;
            ok(!lstrcmpiA(path, buf), "expected \"%s\", got \"%s\"\n", path, buf);

            buf[0] = 0;
            size = MAX_PATH;
            r = MsiGetPropertyA(hpkg, "ProgramFilesFolder", buf, &size);
            ok(r == ERROR_SUCCESS, "failed to get property: %d\n", r);
            pSHGetFolderPathA(NULL, CSIDL_PROGRAM_FILESX86, NULL, 0, path);
            if (size) buf[size - 1] = 0;
            ok(!lstrcmpiA(path, buf), "expected \"%s\", got \"%s\"\n", path, buf);

            buf[0] = 0;
            size = MAX_PATH;
            r = MsiGetPropertyA(hpkg, "CommonFiles64Folder", buf, &size);
            ok(r == ERROR_SUCCESS, "failed to get property: %d\n", r);
            pSHGetFolderPathA(NULL, CSIDL_PROGRAM_FILES_COMMON, NULL, 0, path);
            if (size) buf[size - 1] = 0;
            ok(!lstrcmpiA(path, buf), "expected \"%s\", got \"%s\"\n", path, buf);

            buf[0] = 0;
            size = MAX_PATH;
            r = MsiGetPropertyA(hpkg, "CommonFilesFolder", buf, &size);
            ok(r == ERROR_SUCCESS, "failed to get property: %d\n", r);
            pSHGetFolderPathA(NULL, CSIDL_PROGRAM_FILES_COMMONX86, NULL, 0, path);
            if (size) buf[size - 1] = 0;
            ok(!lstrcmpiA(path, buf), "expected \"%s\", got \"%s\"\n", path, buf);

            buf[0] = 0;
            size = MAX_PATH;
            r = MsiGetPropertyA(hpkg, "VersionNT64", buf, &size);
            ok(r == ERROR_SUCCESS, "failed to get property: %d\n", r);
            ok(buf[0], "property not set\n");
        }
        else if (S(U(si)).wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
        {
            if (!is_wow64)
            {
                buf[0] = 0;
                size = MAX_PATH;
                r = MsiGetPropertyA(hpkg, "Intel", buf, &size);
                ok(r == ERROR_SUCCESS, "failed to get property: %d\n", r);
                ok(buf[0], "property not set\n");

                buf[0] = 0;
                size = MAX_PATH;
                r = MsiGetPropertyA(hpkg, "MsiAMD64", buf, &size);
                ok(r == ERROR_SUCCESS, "failed to get property: %d\n", r);
                ok(!buf[0], "property set\n");

                buf[0] = 0;
                size = MAX_PATH;
                r = MsiGetPropertyA(hpkg, "Msix64", buf, &size);
                ok(r == ERROR_SUCCESS, "failed to get property: %d\n", r);
                ok(!buf[0], "property set\n");

                buf[0] = 0;
                size = MAX_PATH;
                r = MsiGetPropertyA(hpkg, "System64Folder", buf, &size);
                ok(r == ERROR_SUCCESS, "failed to get property: %d\n", r);
                ok(!buf[0], "property set\n");

                buf[0] = 0;
                size = MAX_PATH;
                r = MsiGetPropertyA(hpkg, "SystemFolder", buf, &size);
                ok(r == ERROR_SUCCESS, "failed to get property: %d\n", r);
                GetSystemDirectoryA(path, MAX_PATH);
                if (size) buf[size - 1] = 0;
                ok(!lstrcmpiA(path, buf), "expected \"%s\", got \"%s\"\n", path, buf);

                buf[0] = 0;
                size = MAX_PATH;
                r = MsiGetPropertyA(hpkg, "ProgramFiles64Folder", buf, &size);
                ok(r == ERROR_SUCCESS, "failed to get property: %d\n", r);
                ok(!buf[0], "property set\n");

                buf[0] = 0;
                size = MAX_PATH;
                r = MsiGetPropertyA(hpkg, "ProgramFilesFolder", buf, &size);
                ok(r == ERROR_SUCCESS, "failed to get property: %d\n", r);
                pSHGetFolderPathA(NULL, CSIDL_PROGRAM_FILES, NULL, 0, path);
                if (size) buf[size - 1] = 0;
                ok(!lstrcmpiA(path, buf), "expected \"%s\", got \"%s\"\n", path, buf);

                buf[0] = 0;
                size = MAX_PATH;
                r = MsiGetPropertyA(hpkg, "CommonFiles64Folder", buf, &size);
                ok(r == ERROR_SUCCESS, "failed to get property: %d\n", r);
                ok(!buf[0], "property set\n");

                buf[0] = 0;
                size = MAX_PATH;
                r = MsiGetPropertyA(hpkg, "CommonFilesFolder", buf, &size);
                ok(r == ERROR_SUCCESS, "failed to get property: %d\n", r);
                pSHGetFolderPathA(NULL, CSIDL_PROGRAM_FILES_COMMON, NULL, 0, path);
                if (size) buf[size - 1] = 0;
                ok(!lstrcmpiA(path, buf), "expected \"%s\", got \"%s\"\n", path, buf);

                buf[0] = 0;
                size = MAX_PATH;
                r = MsiGetPropertyA(hpkg, "VersionNT64", buf, &size);
                ok(r == ERROR_SUCCESS, "failed to get property: %d\n", r);
                ok(!buf[0], "property set\n");
            }
            else
            {
                buf[0] = 0;
                size = MAX_PATH;
                r = MsiGetPropertyA(hpkg, "Intel", buf, &size);
                ok(r == ERROR_SUCCESS, "failed to get property: %d\n", r);
                ok(buf[0], "property not set\n");

                buf[0] = 0;
                size = MAX_PATH;
                r = MsiGetPropertyA(hpkg, "MsiAMD64", buf, &size);
                ok(r == ERROR_SUCCESS, "failed to get property: %d\n", r);
                ok(buf[0], "property not set\n");

                buf[0] = 0;
                size = MAX_PATH;
                r = MsiGetPropertyA(hpkg, "Msix64", buf, &size);
                ok(r == ERROR_SUCCESS, "failed to get property: %d\n", r);
                ok(buf[0], "property not set\n");

                buf[0] = 0;
                size = MAX_PATH;
                r = MsiGetPropertyA(hpkg, "System64Folder", buf, &size);
                ok(r == ERROR_SUCCESS, "failed to get property: %d\n", r);
                GetSystemDirectoryA(path, MAX_PATH);
                if (size) buf[size - 1] = 0;
                ok(!lstrcmpiA(path, buf), "expected \"%s\", got \"%s\"\n", path, buf);

                buf[0] = 0;
                size = MAX_PATH;
                r = MsiGetPropertyA(hpkg, "SystemFolder", buf, &size);
                ok(r == ERROR_SUCCESS, "failed to get property: %d\n", r);
                pGetSystemWow64DirectoryA(path, MAX_PATH);
                if (size) buf[size - 1] = 0;
                ok(!lstrcmpiA(path, buf), "expected \"%s\", got \"%s\"\n", path, buf);

                buf[0] = 0;
                size = MAX_PATH;
                r = MsiGetPropertyA(hpkg, "ProgramFilesFolder64", buf, &size);
                ok(r == ERROR_SUCCESS, "failed to get property: %d\n", r);
                ok(!buf[0], "property set\n");

                buf[0] = 0;
                size = MAX_PATH;
                r = MsiGetPropertyA(hpkg, "ProgramFilesFolder", buf, &size);
                ok(r == ERROR_SUCCESS, "failed to get property: %d\n", r);
                pSHGetFolderPathA(NULL, CSIDL_PROGRAM_FILESX86, NULL, 0, path);
                if (size) buf[size - 1] = 0;
                ok(!lstrcmpiA(path, buf), "expected \"%s\", got \"%s\"\n", path, buf);

                buf[0] = 0;
                size = MAX_PATH;
                r = MsiGetPropertyA(hpkg, "CommonFilesFolder64", buf, &size);
                ok(r == ERROR_SUCCESS, "failed to get property: %d\n", r);
                ok(!buf[0], "property set\n");

                buf[0] = 0;
                size = MAX_PATH;
                r = MsiGetPropertyA(hpkg, "CommonFilesFolder", buf, &size);
                ok(r == ERROR_SUCCESS, "failed to get property: %d\n", r);
                pSHGetFolderPathA(NULL, CSIDL_PROGRAM_FILES_COMMONX86, NULL, 0, path);
                if (size) buf[size - 1] = 0;
                ok(!lstrcmpiA(path, buf), "expected \"%s\", got \"%s\"\n", path, buf);

                buf[0] = 0;
                size = MAX_PATH;
                r = MsiGetPropertyA(hpkg, "VersionNT64", buf, &size);
                ok(r == ERROR_SUCCESS, "failed to get property: %d\n", r);
                ok(buf[0], "property not set\n");
            }
        }
    }

    CloseHandle(hkey1);
    CloseHandle(hkey2);
    MsiCloseHandle(hpkg);
    DeleteFileA(msifile);
    MsiSetInternalUI(uilevel, NULL);
}

static void test_launchconditions(void)
{
    MSIHANDLE hpkg;
    MSIHANDLE hdb;
    UINT r;

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    hdb = create_package_db();
    ok( hdb, "failed to create package database\n" );

    r = create_launchcondition_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create LaunchCondition table: %d\n", r );

    r = add_launchcondition_entry( hdb, "'X = \"1\"', 'one'" );
    ok( r == ERROR_SUCCESS, "cannot add launch condition: %d\n", r );

    /* invalid condition */
    r = add_launchcondition_entry( hdb, "'X != \"1\"', 'one'" );
    ok( r == ERROR_SUCCESS, "cannot add launch condition: %d\n", r );

    r = package_from_db( hdb, &hpkg );
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        DeleteFileA(msifile);
        return;
    }
    ok( r == ERROR_SUCCESS, "failed to create package %u\n", r );

    MsiCloseHandle( hdb );

    r = MsiSetPropertyA( hpkg, "X", "1" );
    ok( r == ERROR_SUCCESS, "failed to set property\n" );

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    /* invalid conditions are ignored */
    r = MsiDoActionA( hpkg, "LaunchConditions" );
    ok( r == ERROR_SUCCESS, "cost init failed\n" );

    /* verify LaunchConditions still does some verification */
    r = MsiSetPropertyA( hpkg, "X", "2" );
    ok( r == ERROR_SUCCESS, "failed to set property\n" );

    r = MsiDoActionA( hpkg, "LaunchConditions" );
    ok( r == ERROR_INSTALL_FAILURE, "Expected ERROR_INSTALL_FAILURE, got %d\n", r );

    MsiCloseHandle( hpkg );
    DeleteFileA( msifile );
}

static void test_ccpsearch(void)
{
    MSIHANDLE hdb, hpkg;
    CHAR prop[MAX_PATH];
    DWORD size = MAX_PATH;
    UINT r;

    hdb = create_package_db();
    ok(hdb, "failed to create package database\n");

    r = create_ccpsearch_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_ccpsearch_entry(hdb, "'CCP_random'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_ccpsearch_entry(hdb, "'RMCCP_random'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = create_reglocator_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_reglocator_entry(hdb, "CCP_random", 0, "htmlfile\\shell\\open\\nonexistent", "", 1);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = create_drlocator_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_drlocator_entry(hdb, "'RMCCP_random', '', 'C:\\', '0'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = create_signature_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = package_from_db(hdb, &hpkg);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        DeleteFileA(msifile);
        return;
    }
    ok(r == ERROR_SUCCESS, "failed to create package %u\n", r);

    MsiCloseHandle(hdb);

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiDoActionA(hpkg, "CCPSearch");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiGetPropertyA(hpkg, "CCP_Success", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, "1"), "Expected 1, got %s\n", prop);

    MsiCloseHandle(hpkg);
    DeleteFileA(msifile);
}

static void test_complocator(void)
{
    MSIHANDLE hdb, hpkg;
    UINT r;
    CHAR prop[MAX_PATH];
    CHAR expected[MAX_PATH];
    DWORD size = MAX_PATH;

    hdb = create_package_db();
    ok(hdb, "failed to create package database\n");

    r = create_appsearch_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'ABELISAURUS', 'abelisaurus'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'BACTROSAURUS', 'bactrosaurus'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'CAMELOTIA', 'camelotia'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'DICLONIUS', 'diclonius'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'ECHINODON', 'echinodon'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'FALCARIUS', 'falcarius'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'GALLIMIMUS', 'gallimimus'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'HAGRYPHUS', 'hagryphus'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'IGUANODON', 'iguanodon'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'JOBARIA', 'jobaria'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'KAKURU', 'kakuru'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'LABOCANIA', 'labocania'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'MEGARAPTOR', 'megaraptor'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'NEOSODON', 'neosodon'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'OLOROTITAN', 'olorotitan'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'PANTYDRACO', 'pantydraco'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = create_complocator_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'abelisaurus', '{E3619EED-305A-418C-B9C7-F7D7377F0934}', 1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'bactrosaurus', '{D56B688D-542F-42Ef-90FD-B6DA76EE8119}', 0");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'camelotia', '{8211BE36-2466-47E3-AFB7-6AC72E51AED2}', 1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'diclonius', '{5C767B20-A33C-45A4-B80B-555E512F01AE}', 0");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'echinodon', '{A19E16C5-C75D-4699-8111-C4338C40C3CB}', 1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'falcarius', '{17762FA1-A7AE-4CC6-8827-62873C35361D}', 0");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'gallimimus', '{75EBF568-C959-41E0-A99E-9050638CF5FB}', 1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'hagrphus', '{D4969B72-17D9-4AB6-BE49-78F2FEE857AC}', 0");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'iguanodon', '{8E0DA02E-F6A7-4A8F-B25D-6F564C492308}', 1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'jobaria', '{243C22B1-8C51-4151-B9D1-1AE5265E079E}', 0");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'kakuru', '{5D0F03BA-50BC-44F2-ABB1-72C972F4E514}', 1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'labocania', '{C7DDB60C-7828-4046-A6F8-699D5E92F1ED}', 0");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'megaraptor', '{8B1034B7-BD5E-41ac-B52C-0105D3DFD74D}', 1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'neosodon', '{0B499649-197A-48EF-93D2-AF1C17ED6E90}', 0");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'olorotitan', '{54E9E91F-AED2-46D5-A25A-7E50AFA24513}', 1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'pantydraco', '{2A989951-5565-4FA7-93A7-E800A3E67D71}', 0");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = create_signature_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_signature_entry(hdb, "'abelisaurus', 'abelisaurus', '', '', '', '', '', '', ''");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_signature_entry(hdb, "'bactrosaurus', 'bactrosaurus', '', '', '', '', '', '', ''");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_signature_entry(hdb, "'camelotia', 'camelotia', '', '', '', '', '', '', ''");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_signature_entry(hdb, "'diclonius', 'diclonius', '', '', '', '', '', '', ''");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_signature_entry(hdb, "'iguanodon', 'iguanodon', '', '', '', '', '', '', ''");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_signature_entry(hdb, "'jobaria', 'jobaria', '', '', '', '', '', '', ''");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_signature_entry(hdb, "'kakuru', 'kakuru', '', '', '', '', '', '', ''");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_signature_entry(hdb, "'labocania', 'labocania', '', '', '', '', '', '', ''");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = package_from_db(hdb, &hpkg);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        DeleteFileA(msifile);
        return;
    }
    ok(r == ERROR_SUCCESS, "failed to create package %u\n", r);

    MsiCloseHandle(hdb);

    create_test_file("abelisaurus");
    create_test_file("bactrosaurus");
    create_test_file("camelotia");
    create_test_file("diclonius");
    create_test_file("echinodon");
    create_test_file("falcarius");
    create_test_file("gallimimus");
    create_test_file("hagryphus");
    CreateDirectoryA("iguanodon", NULL);
    CreateDirectoryA("jobaria", NULL);
    CreateDirectoryA("kakuru", NULL);
    CreateDirectoryA("labocania", NULL);
    CreateDirectoryA("megaraptor", NULL);
    CreateDirectoryA("neosodon", NULL);
    CreateDirectoryA("olorotitan", NULL);
    CreateDirectoryA("pantydraco", NULL);

    set_component_path("abelisaurus", MSIINSTALLCONTEXT_MACHINE,
                       "{E3619EED-305A-418C-B9C7-F7D7377F0934}", NULL, FALSE);
    set_component_path("bactrosaurus", MSIINSTALLCONTEXT_MACHINE,
                       "{D56B688D-542F-42Ef-90FD-B6DA76EE8119}", NULL, FALSE);
    set_component_path("echinodon", MSIINSTALLCONTEXT_MACHINE,
                       "{A19E16C5-C75D-4699-8111-C4338C40C3CB}", NULL, FALSE);
    set_component_path("falcarius", MSIINSTALLCONTEXT_MACHINE,
                       "{17762FA1-A7AE-4CC6-8827-62873C35361D}", NULL, FALSE);
    set_component_path("iguanodon", MSIINSTALLCONTEXT_MACHINE,
                       "{8E0DA02E-F6A7-4A8F-B25D-6F564C492308}", NULL, FALSE);
    set_component_path("jobaria", MSIINSTALLCONTEXT_MACHINE,
                       "{243C22B1-8C51-4151-B9D1-1AE5265E079E}", NULL, FALSE);
    set_component_path("megaraptor", MSIINSTALLCONTEXT_MACHINE,
                       "{8B1034B7-BD5E-41ac-B52C-0105D3DFD74D}", NULL, FALSE);
    set_component_path("neosodon", MSIINSTALLCONTEXT_MACHINE,
                       "{0B499649-197A-48EF-93D2-AF1C17ED6E90}", NULL, FALSE);

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiDoActionA(hpkg, "AppSearch");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "ABELISAURUS", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    lstrcpyA(expected, CURR_DIR);
    if (!is_root(CURR_DIR)) lstrcatA(expected, "\\");
    lstrcatA(expected, "abelisaurus");
    ok(!lstrcmpA(prop, expected) || !lstrcmpA(prop, ""),
       "Expected %s or empty string, got %s\n", expected, prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "BACTROSAURUS", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected , got %s\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "CAMELOTIA", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected , got %s\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "DICLONIUS", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected , got %s\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "ECHINODON", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    lstrcpyA(expected, CURR_DIR);
    if (!is_root(CURR_DIR)) lstrcatA(expected, "\\");
    ok(!lstrcmpA(prop, expected) || !lstrcmpA(prop, ""),
       "Expected %s or empty string, got %s\n", expected, prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "FALCARIUS", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected , got %s\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "GALLIMIMUS", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected , got %s\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "HAGRYPHUS", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected , got %s\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "IGUANODON", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected , got %s\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "JOBARIA", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected , got %s\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "KAKURU", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected , got %s\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "LABOCANIA", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected , got %s\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "MEGARAPTOR", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    lstrcpyA(expected, CURR_DIR);
    if (!is_root(CURR_DIR)) lstrcatA(expected, "\\");
    ok(!lstrcmpA(prop, expected) || !lstrcmpA(prop, ""),
       "Expected %s or empty string, got %s\n", expected, prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "NEOSODON", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    lstrcpyA(expected, CURR_DIR);
    if (!is_root(CURR_DIR)) lstrcatA(expected, "\\");
    lstrcatA(expected, "neosodon\\");
    ok(!lstrcmpA(prop, expected) || !lstrcmpA(prop, ""),
       "Expected %s or empty string, got %s\n", expected, prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "OLOROTITAN", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected , got %s\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "PANTYDRACO", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected , got %s\n", prop);

    MsiCloseHandle(hpkg);
    DeleteFileA("abelisaurus");
    DeleteFileA("bactrosaurus");
    DeleteFileA("camelotia");
    DeleteFileA("diclonius");
    DeleteFileA("echinodon");
    DeleteFileA("falcarius");
    DeleteFileA("gallimimus");
    DeleteFileA("hagryphus");
    RemoveDirectoryA("iguanodon");
    RemoveDirectoryA("jobaria");
    RemoveDirectoryA("kakuru");
    RemoveDirectoryA("labocania");
    RemoveDirectoryA("megaraptor");
    RemoveDirectoryA("neosodon");
    RemoveDirectoryA("olorotitan");
    RemoveDirectoryA("pantydraco");
    delete_component_path("{E3619EED-305A-418C-B9C7-F7D7377F0934}",
                          MSIINSTALLCONTEXT_MACHINE, NULL);
    delete_component_path("{D56B688D-542F-42Ef-90FD-B6DA76EE8119}",
                          MSIINSTALLCONTEXT_MACHINE, NULL);
    delete_component_path("{A19E16C5-C75D-4699-8111-C4338C40C3CB}",
                          MSIINSTALLCONTEXT_MACHINE, NULL);
    delete_component_path("{17762FA1-A7AE-4CC6-8827-62873C35361D}",
                          MSIINSTALLCONTEXT_MACHINE, NULL);
    delete_component_path("{8E0DA02E-F6A7-4A8F-B25D-6F564C492308}",
                          MSIINSTALLCONTEXT_MACHINE, NULL);
    delete_component_path("{243C22B1-8C51-4151-B9D1-1AE5265E079E}",
                          MSIINSTALLCONTEXT_MACHINE, NULL);
    delete_component_path("{8B1034B7-BD5E-41ac-B52C-0105D3DFD74D}",
                          MSIINSTALLCONTEXT_MACHINE, NULL);
    delete_component_path("{0B499649-197A-48EF-93D2-AF1C17ED6E90}",
                          MSIINSTALLCONTEXT_MACHINE, NULL);
    DeleteFileA(msifile);
}

static void set_suminfo_prop(MSIHANDLE db, DWORD prop, DWORD val)
{
    MSIHANDLE summary;
    UINT r;

    r = MsiGetSummaryInformationA(db, NULL, 1, &summary);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiSummaryInfoSetPropertyA(summary, prop, VT_I4, val, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiSummaryInfoPersist(summary);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    MsiCloseHandle(summary);
}

static void test_MsiGetSourcePath(void)
{
    MSIHANDLE hdb, hpkg;
    CHAR path[MAX_PATH];
    CHAR cwd[MAX_PATH];
    CHAR subsrc[MAX_PATH];
    CHAR sub2[MAX_PATH];
    DWORD size;
    UINT r;

    lstrcpyA(cwd, CURR_DIR);
    if (!is_root(CURR_DIR)) lstrcatA(cwd, "\\");

    lstrcpyA(subsrc, cwd);
    lstrcatA(subsrc, "subsource");
    lstrcatA(subsrc, "\\");

    lstrcpyA(sub2, subsrc);
    lstrcatA(sub2, "sub2");
    lstrcatA(sub2, "\\");

    /* uncompressed source */

    hdb = create_package_db();
    ok( hdb, "failed to create database\n");

    set_suminfo_prop(hdb, PID_WORDCOUNT, 0);

    r = add_directory_entry(hdb, "'TARGETDIR', '', 'SourceDir'");
    ok(r == S_OK, "failed\n");

    r = add_directory_entry(hdb, "'SubDir', 'TARGETDIR', 'subtarget:subsource'");
    ok(r == S_OK, "failed\n");

    r = add_directory_entry(hdb, "'SubDir2', 'SubDir', 'sub2'");
    ok(r == S_OK, "failed\n");

    r = MsiDatabaseCommit(hdb);
    ok(r == ERROR_SUCCESS , "Failed to commit database\n");

    r = package_from_db(hdb, &hpkg);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        DeleteFileA(msifile);
        return;
    }
    ok(r == ERROR_SUCCESS, "failed to create package %u\n", r);

    MsiCloseHandle(hdb);

    /* invalid database handle */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(-1, "TARGETDIR", path, &size);
    ok(r == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* NULL szFolder */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, NULL, path, &size);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* empty szFolder */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* try TARGETDIR */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "TARGETDIR", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
    ok(size == 0, "Expected 0, got %d\n", size);

    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
    ok(size == 0, "Expected 0, got %d\n", size);

    /* try SourceDir */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* try SOURCEDIR */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* source path does not exist, but the property exists */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
    ok(size == 0, "Expected 0, got %d\n", size);

    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
    ok(size == 0, "Expected 0, got %d\n", size);

    /* try SubDir */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* try SubDir2 */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir2", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiDoActionA(hpkg, "CostInitialize");
    ok(r == ERROR_SUCCESS, "cost init failed\n");

    /* try TARGETDIR after CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "TARGETDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SourceDir after CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SOURCEDIR after CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* source path does not exist, but the property exists */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SubDir after CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    /* try SubDir2 after CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir2", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, sub2), "Expected \"%s\", got \"%s\"\n", sub2, path);
    ok(size == lstrlenA(sub2), "Expected %d, got %d\n", lstrlenA(sub2), size);

    r = MsiDoActionA(hpkg, "ResolveSource");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    /* try TARGETDIR after ResolveSource */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "TARGETDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SourceDir after ResolveSource */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SOURCEDIR after ResolveSource */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* source path does not exist, but the property exists */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SubDir after ResolveSource */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    /* try SubDir2 after ResolveSource */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir2", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, sub2), "Expected \"%s\", got \"%s\"\n", sub2, path);
    ok(size == lstrlenA(sub2), "Expected %d, got %d\n", lstrlenA(sub2), size);

    r = MsiDoActionA(hpkg, "FileCost");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    /* try TARGETDIR after FileCost */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "TARGETDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SourceDir after FileCost */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SOURCEDIR after FileCost */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* source path does not exist, but the property exists */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SubDir after FileCost */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    /* try SubDir2 after FileCost */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir2", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, sub2), "Expected \"%s\", got \"%s\"\n", sub2, path);
    ok(size == lstrlenA(sub2), "Expected %d, got %d\n", lstrlenA(sub2), size);

    r = MsiDoActionA(hpkg, "CostFinalize");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    /* try TARGETDIR after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "TARGETDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SourceDir after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SOURCEDIR after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* source path does not exist, but the property exists */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SubDir after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    /* try SubDir2 after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir2", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, sub2), "Expected \"%s\", got \"%s\"\n", sub2, path);
    ok(size == lstrlenA(sub2), "Expected %d, got %d\n", lstrlenA(sub2), size);

    /* nonexistent directory */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "IDontExist", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* NULL szPathBuf */
    size = MAX_PATH;
    r = MsiGetSourcePathA(hpkg, "SourceDir", NULL, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* NULL pcchPathBuf */
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SourceDir", path, NULL);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);

    /* pcchPathBuf is 0 */
    size = 0;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* pcchPathBuf does not have room for NULL terminator */
    size = lstrlenA(cwd);
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(!strncmp(path, cwd, lstrlenA(cwd) - 1),
       "Expected path with no backslash, got \"%s\"\n", path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* pcchPathBuf has room for NULL terminator */
    size = lstrlenA(cwd) + 1;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* remove property */
    r = MsiSetPropertyA(hpkg, "SourceDir", NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* try SourceDir again */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* set property to a valid directory */
    r = MsiSetPropertyA(hpkg, "SOURCEDIR", cwd);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* try SOURCEDIR again */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    MsiCloseHandle(hpkg);

    /* compressed source */

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_DIRECT, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    set_suminfo_prop(hdb, PID_WORDCOUNT, msidbSumInfoSourceTypeCompressed);

    r = package_from_db(hdb, &hpkg);
    ok(r == ERROR_SUCCESS, "failed to create package %u\n", r);

    /* try TARGETDIR */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "TARGETDIR", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* try SourceDir */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* try SOURCEDIR */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* source path nor the property exist */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
    ok(size == 0, "Expected 0, got %d\n", size);

    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
    ok(size == 0, "Expected 0, got %d\n", size);

    /* try SubDir */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* try SubDir2 */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir2", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    r = MsiDoActionA(hpkg, "CostInitialize");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* try TARGETDIR after CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "TARGETDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SourceDir after CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SOURCEDIR after CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SOURCEDIR", path, &size);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
        ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);
    }

    /* source path does not exist, but the property exists */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SubDir after CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SubDir2 after CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir2", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    r = MsiDoActionA(hpkg, "ResolveSource");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    /* try TARGETDIR after ResolveSource */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "TARGETDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SourceDir after ResolveSource */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SOURCEDIR after ResolveSource */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SOURCEDIR", path, &size);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
        ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);
    }

    /* source path and the property exist */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SubDir after ResolveSource */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SubDir2 after ResolveSource */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir2", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    r = MsiDoActionA(hpkg, "FileCost");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* try TARGETDIR after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "TARGETDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SourceDir after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SOURCEDIR after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SOURCEDIR", path, &size);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
        ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);
    }

    /* source path and the property exist */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SubDir after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SubDir2 after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir2", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    r = MsiDoActionA(hpkg, "CostFinalize");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* try TARGETDIR after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "TARGETDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SourceDir after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SOURCEDIR after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SOURCEDIR", path, &size);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
        ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);
    }

    /* source path and the property exist */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SubDir after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SubDir2 after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir2", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    MsiCloseHandle(hpkg);
    DeleteFileA(msifile);
}

static void test_shortlongsource(void)
{
    MSIHANDLE hdb, hpkg;
    CHAR path[MAX_PATH];
    CHAR cwd[MAX_PATH];
    CHAR subsrc[MAX_PATH];
    DWORD size;
    UINT r;

    lstrcpyA(cwd, CURR_DIR);
    if (!is_root(CURR_DIR)) lstrcatA(cwd, "\\");

    lstrcpyA(subsrc, cwd);
    lstrcatA(subsrc, "long");
    lstrcatA(subsrc, "\\");

    /* long file names */

    hdb = create_package_db();
    ok( hdb, "failed to create database\n");

    set_suminfo_prop(hdb, PID_WORDCOUNT, 0);

    r = add_directory_entry(hdb, "'TARGETDIR', '', 'SourceDir'");
    ok(r == S_OK, "failed\n");

    r = add_directory_entry(hdb, "'SubDir', 'TARGETDIR', 'short|long'");
    ok(r == S_OK, "failed\n");

    /* CostInitialize:short */
    r = add_directory_entry(hdb, "'SubDir2', 'TARGETDIR', 'one|two'");
    ok(r == S_OK, "failed\n");

    /* CostInitialize:long */
    r = add_directory_entry(hdb, "'SubDir3', 'TARGETDIR', 'three|four'");
    ok(r == S_OK, "failed\n");

    /* FileCost:short */
    r = add_directory_entry(hdb, "'SubDir4', 'TARGETDIR', 'five|six'");
    ok(r == S_OK, "failed\n");

    /* FileCost:long */
    r = add_directory_entry(hdb, "'SubDir5', 'TARGETDIR', 'seven|eight'");
    ok(r == S_OK, "failed\n");

    /* CostFinalize:short */
    r = add_directory_entry(hdb, "'SubDir6', 'TARGETDIR', 'nine|ten'");
    ok(r == S_OK, "failed\n");

    /* CostFinalize:long */
    r = add_directory_entry(hdb, "'SubDir7', 'TARGETDIR', 'eleven|twelve'");
    ok(r == S_OK, "failed\n");

    MsiDatabaseCommit(hdb);

    r = package_from_db(hdb, &hpkg);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        DeleteFileA(msifile);
        return;
    }
    ok(r == ERROR_SUCCESS, "failed to create package %u\n", r);

    MsiCloseHandle(hdb);

    CreateDirectoryA("one", NULL);
    CreateDirectoryA("four", NULL);

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiDoActionA(hpkg, "CostInitialize");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    CreateDirectoryA("five", NULL);
    CreateDirectoryA("eight", NULL);

    r = MsiDoActionA(hpkg, "FileCost");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    CreateDirectoryA("nine", NULL);
    CreateDirectoryA("twelve", NULL);

    r = MsiDoActionA(hpkg, "CostFinalize");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    /* neither short nor long source directories exist */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    CreateDirectoryA("short", NULL);

    /* short source directory exists */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    CreateDirectoryA("long", NULL);

    /* both short and long source directories exist */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    lstrcpyA(subsrc, cwd);
    lstrcatA(subsrc, "two");
    lstrcatA(subsrc, "\\");

    /* short dir exists before CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir2", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    lstrcpyA(subsrc, cwd);
    lstrcatA(subsrc, "four");
    lstrcatA(subsrc, "\\");

    /* long dir exists before CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir3", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    lstrcpyA(subsrc, cwd);
    lstrcatA(subsrc, "six");
    lstrcatA(subsrc, "\\");

    /* short dir exists before FileCost */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir4", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    lstrcpyA(subsrc, cwd);
    lstrcatA(subsrc, "eight");
    lstrcatA(subsrc, "\\");

    /* long dir exists before FileCost */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir5", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    lstrcpyA(subsrc, cwd);
    lstrcatA(subsrc, "ten");
    lstrcatA(subsrc, "\\");

    /* short dir exists before CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir6", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    lstrcpyA(subsrc, cwd);
    lstrcatA(subsrc, "twelve");
    lstrcatA(subsrc, "\\");

    /* long dir exists before CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir7", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    MsiCloseHandle(hpkg);
    RemoveDirectoryA("short");
    RemoveDirectoryA("long");
    RemoveDirectoryA("one");
    RemoveDirectoryA("four");
    RemoveDirectoryA("five");
    RemoveDirectoryA("eight");
    RemoveDirectoryA("nine");
    RemoveDirectoryA("twelve");

    /* short file names */

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_DIRECT, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    set_suminfo_prop(hdb, PID_WORDCOUNT, msidbSumInfoSourceTypeSFN);

    r = package_from_db(hdb, &hpkg);
    ok(r == ERROR_SUCCESS, "failed to create package %u\n", r);

    MsiCloseHandle(hdb);

    CreateDirectoryA("one", NULL);
    CreateDirectoryA("four", NULL);

    r = MsiDoActionA(hpkg, "CostInitialize");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    CreateDirectoryA("five", NULL);
    CreateDirectoryA("eight", NULL);

    r = MsiDoActionA(hpkg, "FileCost");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    CreateDirectoryA("nine", NULL);
    CreateDirectoryA("twelve", NULL);

    r = MsiDoActionA(hpkg, "CostFinalize");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    lstrcpyA(subsrc, cwd);
    lstrcatA(subsrc, "short");
    lstrcatA(subsrc, "\\");

    /* neither short nor long source directories exist */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    CreateDirectoryA("short", NULL);

    /* short source directory exists */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    CreateDirectoryA("long", NULL);

    /* both short and long source directories exist */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    lstrcpyA(subsrc, cwd);
    lstrcatA(subsrc, "one");
    lstrcatA(subsrc, "\\");

    /* short dir exists before CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir2", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    lstrcpyA(subsrc, cwd);
    lstrcatA(subsrc, "three");
    lstrcatA(subsrc, "\\");

    /* long dir exists before CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir3", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    lstrcpyA(subsrc, cwd);
    lstrcatA(subsrc, "five");
    lstrcatA(subsrc, "\\");

    /* short dir exists before FileCost */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir4", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    lstrcpyA(subsrc, cwd);
    lstrcatA(subsrc, "seven");
    lstrcatA(subsrc, "\\");

    /* long dir exists before FileCost */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir5", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    lstrcpyA(subsrc, cwd);
    lstrcatA(subsrc, "nine");
    lstrcatA(subsrc, "\\");

    /* short dir exists before CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir6", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    lstrcpyA(subsrc, cwd);
    lstrcatA(subsrc, "eleven");
    lstrcatA(subsrc, "\\");

    /* long dir exists before CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SubDir7", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    MsiCloseHandle(hpkg);
    RemoveDirectoryA("short");
    RemoveDirectoryA("long");
    RemoveDirectoryA("one");
    RemoveDirectoryA("four");
    RemoveDirectoryA("five");
    RemoveDirectoryA("eight");
    RemoveDirectoryA("nine");
    RemoveDirectoryA("twelve");
    DeleteFileA(msifile);
}

static void test_sourcedir(void)
{
    MSIHANDLE hdb, hpkg;
    CHAR package[12];
    CHAR path[MAX_PATH];
    CHAR cwd[MAX_PATH];
    CHAR subsrc[MAX_PATH];
    DWORD size;
    UINT r;

    lstrcpyA(cwd, CURR_DIR);
    if (!is_root(CURR_DIR)) lstrcatA(cwd, "\\");

    lstrcpyA(subsrc, cwd);
    lstrcatA(subsrc, "long");
    lstrcatA(subsrc, "\\");

    hdb = create_package_db();
    ok( hdb, "failed to create database\n");

    r = add_directory_entry(hdb, "'TARGETDIR', '', 'SourceDir'");
    ok(r == S_OK, "failed\n");

    sprintf(package, "#%u", hdb);
    r = MsiOpenPackageA(package, &hpkg);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* properties only */

    /* SourceDir prop */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
    ok(size == 0, "Expected 0, got %d\n", size);

    /* SOURCEDIR prop */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
    ok(size == 0, "Expected 0, got %d\n", size);

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiDoActionA(hpkg, "CostInitialize");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    /* SourceDir after CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
    ok(size == 0, "Expected 0, got %d\n", size);

    /* SOURCEDIR after CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
    ok(size == 0, "Expected 0, got %d\n", size);

    r = MsiDoActionA(hpkg, "FileCost");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    /* SourceDir after FileCost */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
    ok(size == 0, "Expected 0, got %d\n", size);

    /* SOURCEDIR after FileCost */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
    ok(size == 0, "Expected 0, got %d\n", size);

    r = MsiDoActionA(hpkg, "CostFinalize");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    /* SourceDir after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
    ok(size == 0, "Expected 0, got %d\n", size);

    /* SOURCEDIR after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
    ok(size == 0, "Expected 0, got %d\n", size);

    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"), "Expected \"kiwi\", got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected %d, got %d\n", MAX_PATH, size);

    /* SOURCEDIR after calling MsiGetSourcePath */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    todo_wine {
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);
    }

    r = MsiDoActionA(hpkg, "ResolveSource");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    /* SourceDir after ResolveSource */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* SOURCEDIR after ResolveSource */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* random casing */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SoUrCeDiR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
    ok(size == 0, "Expected 0, got %d\n", size);

    MsiCloseHandle(hpkg);

    /* reset the package state */
    sprintf(package, "#%i", hdb);
    r = MsiOpenPackageA(package, &hpkg);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* test how MsiGetSourcePath affects the properties */

    /* SourceDir prop */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    todo_wine
    {
        ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
        ok(size == 0, "Expected 0, got %d\n", size);
    }

    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* SourceDir after MsiGetSourcePath */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    todo_wine
    {
        ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
        ok(size == 0, "Expected 0, got %d\n", size);
    }

    /* SOURCEDIR prop */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    todo_wine
    {
        ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
        ok(size == 0, "Expected 0, got %d\n", size);
    }

    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* SOURCEDIR prop after MsiGetSourcePath */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    todo_wine
    {
        ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
        ok(size == 0, "Expected 0, got %d\n", size);
    }

    r = MsiDoActionA(hpkg, "CostInitialize");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    /* SourceDir after CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    todo_wine
    {
        ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
        ok(size == 0, "Expected 0, got %d\n", size);
    }

    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* SourceDir after MsiGetSourcePath */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* SOURCEDIR after CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* SOURCEDIR source path still does not exist */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    r = MsiDoActionA(hpkg, "FileCost");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    /* SourceDir after FileCost */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* SOURCEDIR after FileCost */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* SOURCEDIR source path still does not exist */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    r = MsiDoActionA(hpkg, "CostFinalize");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    /* SourceDir after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* SOURCEDIR after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* SOURCEDIR source path still does not exist */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    r = MsiDoActionA(hpkg, "ResolveSource");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    /* SourceDir after ResolveSource */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* SOURCEDIR after ResolveSource */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetPropertyA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* SOURCEDIR source path still does not exist */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePathA(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    MsiCloseHandle(hpkg);

error:
    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

struct access_res
{
    BOOL gothandle;
    DWORD lasterr;
    BOOL ignore;
};

static const struct access_res create[16] =
{
    { TRUE, ERROR_SUCCESS, TRUE },
    { TRUE, ERROR_SUCCESS, TRUE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { TRUE, ERROR_SUCCESS, TRUE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { TRUE, ERROR_SUCCESS, TRUE }
};

static const struct access_res create_commit[16] =
{
    { TRUE, ERROR_SUCCESS, TRUE },
    { TRUE, ERROR_SUCCESS, TRUE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { TRUE, ERROR_SUCCESS, TRUE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { TRUE, ERROR_SUCCESS, TRUE }
};

static const struct access_res create_close[16] =
{
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS }
};

static void _test_file_access(LPCSTR file, const struct access_res *ares, DWORD line)
{
    DWORD access = 0, share = 0;
    DWORD lasterr;
    HANDLE hfile;
    int i, j, idx = 0;

    for (i = 0; i < 4; i++)
    {
        if (i == 0) access = 0;
        if (i == 1) access = GENERIC_READ;
        if (i == 2) access = GENERIC_WRITE;
        if (i == 3) access = GENERIC_READ | GENERIC_WRITE;

        for (j = 0; j < 4; j++)
        {
            if (ares[idx].ignore)
                continue;

            if (j == 0) share = 0;
            if (j == 1) share = FILE_SHARE_READ;
            if (j == 2) share = FILE_SHARE_WRITE;
            if (j == 3) share = FILE_SHARE_READ | FILE_SHARE_WRITE;

            SetLastError(0xdeadbeef);
            hfile = CreateFileA(file, access, share, NULL, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, 0);
            lasterr = GetLastError();

            ok((hfile != INVALID_HANDLE_VALUE) == ares[idx].gothandle,
               "(%d, handle, %d): Expected %d, got %d\n",
               line, idx, ares[idx].gothandle,
               (hfile != INVALID_HANDLE_VALUE));

            ok(lasterr == ares[idx].lasterr, "(%d, lasterr, %d): Expected %d, got %d\n",
               line, idx, ares[idx].lasterr, lasterr);

            CloseHandle(hfile);
            idx++;
        }
    }
}

#define test_file_access(file, ares) _test_file_access(file, ares, __LINE__)

static void test_access(void)
{
    MSIHANDLE hdb;
    UINT r;

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    test_file_access(msifile, create);

    r = MsiDatabaseCommit(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    test_file_access(msifile, create_commit);
    MsiCloseHandle(hdb);

    test_file_access(msifile, create_close);
    DeleteFileA(msifile);

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATEDIRECT, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    test_file_access(msifile, create);

    r = MsiDatabaseCommit(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    test_file_access(msifile, create_commit);
    MsiCloseHandle(hdb);

    test_file_access(msifile, create_close);
    DeleteFileA(msifile);
}

static void test_emptypackage(void)
{
    MSIHANDLE hpkg = 0, hdb = 0, hsuminfo = 0;
    MSIHANDLE hview = 0, hrec = 0;
    MSICONDITION condition;
    CHAR buffer[MAX_PATH];
    DWORD size;
    UINT r;

    r = MsiOpenPackageA("", &hpkg);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        return;
    }
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    }

    hdb = MsiGetActiveDatabase(hpkg);
    todo_wine
    {
        ok(hdb != 0, "Expected a valid database handle\n");
    }

    r = MsiDatabaseOpenViewA(hdb, "SELECT * FROM `_Tables`", &hview);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    }
    r = MsiViewExecute(hview, 0);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    }

    r = MsiViewFetch(hview, &hrec);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    }

    buffer[0] = 0;
    size = MAX_PATH;
    r = MsiRecordGetStringA(hrec, 1, buffer, &size);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(buffer, "_Property"),
           "Expected \"_Property\", got \"%s\"\n", buffer);
    }

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    }

    size = MAX_PATH;
    r = MsiRecordGetStringA(hrec, 1, buffer, &size);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(buffer, "#_FolderCache"),
           "Expected \"_Property\", got \"%s\"\n", buffer);
    }

    MsiCloseHandle(hrec);
    MsiViewClose(hview);
    MsiCloseHandle(hview);

    condition = MsiDatabaseIsTablePersistentA(hdb, "_Property");
    todo_wine
    {
        ok(condition == MSICONDITION_FALSE,
           "Expected MSICONDITION_FALSE, got %d\n", condition);
    }

    r = MsiDatabaseOpenViewA(hdb, "SELECT * FROM `_Property`", &hview);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    }
    r = MsiViewExecute(hview, 0);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    }

    /* _Property table is not empty */
    r = MsiViewFetch(hview, &hrec);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    }

    MsiCloseHandle(hrec);
    MsiViewClose(hview);
    MsiCloseHandle(hview);

    condition = MsiDatabaseIsTablePersistentA(hdb, "#_FolderCache");
    todo_wine
    {
        ok(condition == MSICONDITION_FALSE,
           "Expected MSICONDITION_FALSE, got %d\n", condition);
    }

    r = MsiDatabaseOpenViewA(hdb, "SELECT * FROM `#_FolderCache`", &hview);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    }
    r = MsiViewExecute(hview, 0);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    }

    /* #_FolderCache is not empty */
    r = MsiViewFetch(hview, &hrec);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    }

    MsiCloseHandle(hrec);
    MsiViewClose(hview);
    MsiCloseHandle(hview);

    r = MsiDatabaseOpenViewA(hdb, "SELECT * FROM `_Streams`", &hview);
    todo_wine
    {
        ok(r == ERROR_BAD_QUERY_SYNTAX,
           "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);
    }

    r = MsiDatabaseOpenViewA(hdb, "SELECT * FROM `_Storages`", &hview);
    todo_wine
    {
        ok(r == ERROR_BAD_QUERY_SYNTAX,
           "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);
    }

    r = MsiGetSummaryInformationA(hdb, NULL, 0, &hsuminfo);
    todo_wine
    {
        ok(r == ERROR_INSTALL_PACKAGE_INVALID,
           "Expected ERROR_INSTALL_PACKAGE_INVALID, got %d\n", r);
    }

    MsiCloseHandle(hsuminfo);

    r = MsiDatabaseCommit(hdb);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    }

    MsiCloseHandle(hdb);
    MsiCloseHandle(hpkg);
}

static void test_MsiGetProductProperty(void)
{
    MSIHANDLE hprod, hdb;
    CHAR val[MAX_PATH];
    CHAR path[MAX_PATH];
    CHAR query[MAX_PATH];
    CHAR keypath[MAX_PATH*2];
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    HKEY prodkey, userkey, props;
    DWORD size;
    LONG res;
    UINT r;
    REGSAM access = KEY_ALL_ACCESS;

    GetCurrentDirectoryA(MAX_PATH, path);
    lstrcatA(path, "\\");

    create_test_guid(prodcode, prod_squashed);

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiDatabaseCommit(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = set_summary_info(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = run_query(hdb,
            "CREATE TABLE `Directory` ( "
            "`Directory` CHAR(255) NOT NULL, "
            "`Directory_Parent` CHAR(255), "
            "`DefaultDir` CHAR(255) NOT NULL "
            "PRIMARY KEY `Directory`)");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = run_query(hdb,
            "CREATE TABLE `Property` ( "
            "`Property` CHAR(72) NOT NULL, "
            "`Value` CHAR(255) "
            "PRIMARY KEY `Property`)");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sprintf(query, "INSERT INTO `Property` "
            "(`Property`, `Value`) "
            "VALUES( 'ProductCode', '%s' )", prodcode);
    r = run_query(hdb, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiDatabaseCommit(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(hdb);

    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        DeleteFileA(msifile);
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\S-1-5-18\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &userkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        RegDeleteKeyA(prodkey, "");
        RegCloseKey(prodkey);
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = RegCreateKeyExA(userkey, "InstallProperties", 0, NULL, 0, access, NULL, &props, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    lstrcpyA(val, path);
    lstrcatA(val, "\\");
    lstrcatA(val, msifile);
    res = RegSetValueExA(props, "LocalPackage", 0, REG_SZ,
                         (const BYTE *)val, lstrlenA(val) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    hprod = 0xdeadbeef;
    r = MsiOpenProductA(prodcode, &hprod);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(hprod != 0 && hprod != 0xdeadbeef, "Expected a valid product handle\n");

    /* hProduct is invalid */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetProductPropertyA(0xdeadbeef, "ProductCode", val, &size);
    ok(r == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", r);
    ok(!lstrcmpA(val, "apple"),
       "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* szProperty is NULL */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetProductPropertyA(hprod, NULL, val, &size);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(val, "apple"),
       "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* szProperty is empty */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetProductPropertyA(hprod, "", val, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(val, ""), "Expected \"\", got \"%s\"\n", val);
    ok(size == 0, "Expected 0, got %d\n", size);

    /* get the property */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetProductPropertyA(hprod, "ProductCode", val, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(val, prodcode),
       "Expected \"%s\", got \"%s\"\n", prodcode, val);
    ok(size == lstrlenA(prodcode),
       "Expected %d, got %d\n", lstrlenA(prodcode), size);

    /* lpValueBuf is NULL */
    size = MAX_PATH;
    r = MsiGetProductPropertyA(hprod, "ProductCode", NULL, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(size == lstrlenA(prodcode),
       "Expected %d, got %d\n", lstrlenA(prodcode), size);

    /* pcchValueBuf is NULL */
    lstrcpyA(val, "apple");
    r = MsiGetProductPropertyA(hprod, "ProductCode", val, NULL);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(val, "apple"),
       "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == lstrlenA(prodcode),
       "Expected %d, got %d\n", lstrlenA(prodcode), size);

    /* pcchValueBuf is too small */
    size = 4;
    lstrcpyA(val, "apple");
    r = MsiGetProductPropertyA(hprod, "ProductCode", val, &size);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(!strncmp(val, prodcode, 3),
       "Expected first 3 chars of \"%s\", got \"%s\"\n", prodcode, val);
    ok(size == lstrlenA(prodcode),
       "Expected %d, got %d\n", lstrlenA(prodcode), size);

    /* pcchValueBuf does not leave room for NULL terminator */
    size = lstrlenA(prodcode);
    lstrcpyA(val, "apple");
    r = MsiGetProductPropertyA(hprod, "ProductCode", val, &size);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(!strncmp(val, prodcode, lstrlenA(prodcode) - 1),
       "Expected first 37 chars of \"%s\", got \"%s\"\n", prodcode, val);
    ok(size == lstrlenA(prodcode),
       "Expected %d, got %d\n", lstrlenA(prodcode), size);

    /* pcchValueBuf has enough room for NULL terminator */
    size = lstrlenA(prodcode) + 1;
    lstrcpyA(val, "apple");
    r = MsiGetProductPropertyA(hprod, "ProductCode", val, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(val, prodcode),
       "Expected \"%s\", got \"%s\"\n", prodcode, val);
    ok(size == lstrlenA(prodcode),
       "Expected %d, got %d\n", lstrlenA(prodcode), size);

    /* nonexistent property */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetProductPropertyA(hprod, "IDontExist", val, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(val, ""), "Expected \"\", got \"%s\"\n", val);
    ok(size == 0, "Expected 0, got %d\n", size);

    r = MsiSetPropertyA(hprod, "NewProperty", "value");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* non-product property set */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetProductPropertyA(hprod, "NewProperty", val, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(val, ""), "Expected \"\", got \"%s\"\n", val);
    ok(size == 0, "Expected 0, got %d\n", size);

    r = MsiSetPropertyA(hprod, "ProductCode", "value");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* non-product property that is also a product property set */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetProductPropertyA(hprod, "ProductCode", val, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(val, prodcode),
       "Expected \"%s\", got \"%s\"\n", prodcode, val);
    ok(size == lstrlenA(prodcode),
       "Expected %d, got %d\n", lstrlenA(prodcode), size);

    MsiCloseHandle(hprod);

    RegDeleteValueA(props, "LocalPackage");
    delete_key(props, "", access);
    RegCloseKey(props);
    delete_key(userkey, "", access);
    RegCloseKey(userkey);
    delete_key(prodkey, "", access);
    RegCloseKey(prodkey);
    DeleteFileA(msifile);
}

static void test_MsiSetProperty(void)
{
    MSIHANDLE hpkg, hdb, hrec;
    CHAR buf[MAX_PATH];
    LPCSTR query;
    DWORD size;
    UINT r;

    r = package_from_db(create_package_db(), &hpkg);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        DeleteFileA(msifile);
        return;
    }
    ok(r == ERROR_SUCCESS, "Expected a valid package %u\n", r);

    /* invalid hInstall */
    r = MsiSetPropertyA(0, "Prop", "Val");
    ok(r == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", r);

    /* invalid hInstall */
    r = MsiSetPropertyA(0xdeadbeef, "Prop", "Val");
    ok(r == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", r);

    /* szName is NULL */
    r = MsiSetPropertyA(hpkg, NULL, "Val");
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* both szName and szValue are NULL */
    r = MsiSetPropertyA(hpkg, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* szName is empty */
    r = MsiSetPropertyA(hpkg, "", "Val");
    ok(r == ERROR_FUNCTION_FAILED,
       "Expected ERROR_FUNCTION_FAILED, got %d\n", r);

    /* szName is empty and szValue is NULL */
    r = MsiSetPropertyA(hpkg, "", NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* set a property */
    r = MsiSetPropertyA(hpkg, "Prop", "Val");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* get the property */
    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "Prop", buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "Val"), "Expected \"Val\", got \"%s\"\n", buf);
    ok(size == 3, "Expected 3, got %d\n", size);

    /* update the property */
    r = MsiSetPropertyA(hpkg, "Prop", "Nuvo");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* get the property */
    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "Prop", buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "Nuvo"), "Expected \"Nuvo\", got \"%s\"\n", buf);
    ok(size == 4, "Expected 4, got %d\n", size);

    hdb = MsiGetActiveDatabase(hpkg);
    ok(hdb != 0, "Expected a valid database handle\n");

    /* set prop is not in the _Property table */
    query = "SELECT * FROM `_Property` WHERE `Property` = 'Prop'";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* set prop is not in the Property table */
    query = "SELECT * FROM `Property` WHERE `Property` = 'Prop'";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    MsiCloseHandle(hdb);

    /* szValue is an empty string */
    r = MsiSetPropertyA(hpkg, "Prop", "");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* try to get the property */
    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "Prop", buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(size == 0, "Expected 0, got %d\n", size);

    /* reset the property */
    r = MsiSetPropertyA(hpkg, "Prop", "BlueTap");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* delete the property */
    r = MsiSetPropertyA(hpkg, "Prop", NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* try to get the property */
    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "Prop", buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(size == 0, "Expected 0, got %d\n", size);

    MsiCloseHandle(hpkg);
    DeleteFileA(msifile);
}

static void test_MsiApplyMultiplePatches(void)
{
    UINT r, type = GetDriveTypeW(NULL);

    if (!pMsiApplyMultiplePatchesA) {
        win_skip("MsiApplyMultiplePatchesA not found\n");
        return;
    }

    r = pMsiApplyMultiplePatchesA(NULL, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", r);

    r = pMsiApplyMultiplePatchesA("", NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", r);

    r = pMsiApplyMultiplePatchesA(";", NULL, NULL);
    if (type == DRIVE_FIXED)
        todo_wine ok(r == ERROR_PATH_NOT_FOUND, "Expected ERROR_PATH_NOT_FOUND, got %u\n", r);
    else
        ok(r == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %u\n", r);

    r = pMsiApplyMultiplePatchesA("  ;", NULL, NULL);
    if (type == DRIVE_FIXED)
        todo_wine ok(r == ERROR_PATCH_PACKAGE_OPEN_FAILED, "Expected ERROR_PATCH_PACKAGE_OPEN_FAILED, got %u\n", r);
    else
        ok(r == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %u\n", r);

    r = pMsiApplyMultiplePatchesA(";;", NULL, NULL);
    if (type == DRIVE_FIXED)
        todo_wine ok(r == ERROR_PATH_NOT_FOUND, "Expected ERROR_PATH_NOT_FOUND, got %u\n", r);
    else
        ok(r == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %u\n", r);

    r = pMsiApplyMultiplePatchesA("nosuchpatchpackage;", NULL, NULL);
    todo_wine ok(r == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %u\n", r);

    r = pMsiApplyMultiplePatchesA(";nosuchpatchpackage", NULL, NULL);
    if (type == DRIVE_FIXED)
        todo_wine ok(r == ERROR_PATH_NOT_FOUND, "Expected ERROR_PATH_NOT_FOUND, got %u\n", r);
    else
        ok(r == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %u\n", r);

    r = pMsiApplyMultiplePatchesA("nosuchpatchpackage;nosuchpatchpackage", NULL, NULL);
    todo_wine ok(r == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %u\n", r);

    r = pMsiApplyMultiplePatchesA("  nosuchpatchpackage  ;  nosuchpatchpackage  ", NULL, NULL);
    todo_wine ok(r == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %u\n", r);
}

static void test_MsiApplyPatch(void)
{
    UINT r;

    r = MsiApplyPatchA(NULL, NULL, INSTALLTYPE_DEFAULT, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", r);

    r = MsiApplyPatchA("", NULL, INSTALLTYPE_DEFAULT, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", r);
}

static void test_MsiEnumComponentCosts(void)
{
    MSIHANDLE hdb, hpkg;
    char package[12], drive[3];
    DWORD len;
    UINT r;
    int cost, temp;

    hdb = create_package_db();
    ok( hdb, "failed to create database\n" );

    r = create_property_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create Property table %u\n", r );

    r = add_property_entry( hdb, "'ProductCode', '{379B1C47-40C1-42FA-A9BB-BEBB6F1B0172}'" );
    ok( r == ERROR_SUCCESS, "cannot add property entry %u\n", r );

    r = add_property_entry( hdb, "'MSIFASTINSTALL', '1'" );
    ok( r == ERROR_SUCCESS, "cannot add property entry %u\n", r );

    r = add_directory_entry( hdb, "'TARGETDIR', '', 'SourceDir'" );
    ok( r == ERROR_SUCCESS, "failed to add directory entry %u\n" , r );

    r = create_media_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create Media table %u\n", r );

    r = add_media_entry( hdb, "'1', '2', 'cabinet', '', '', ''");
    ok( r == ERROR_SUCCESS, "cannot add media entry %u\n", r );

    r = create_file_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create File table %u\n", r );

    r = add_file_entry( hdb, "'one.txt', 'one', 'one.txt', 4096, '', '', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file %u\n", r );

    r = create_component_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create Component table %u\n", r );

    r = add_component_entry( hdb, "'one', '{B2F86B9D-8447-4BC5-8883-750C45AA31CA}', 'TARGETDIR', 0, '', 'one.txt'" );
    ok( r == ERROR_SUCCESS, "cannot add component %u\n", r );

    r = add_component_entry( hdb, "'two', '{62A09F6E-0B74-4829-BDB7-CAB66F42CCE8}', 'TARGETDIR', 0, '', ''" );
    ok( r == ERROR_SUCCESS, "cannot add component %u\n", r );

    r = create_feature_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create Feature table %u\n", r );

    r = add_feature_entry( hdb, "'one', '', '', '', 0, 1, '', 0" );
    ok( r == ERROR_SUCCESS, "cannot add feature %u\n", r );

    r = add_feature_entry( hdb, "'two', '', '', '', 0, 1, '', 0" );
    ok( r == ERROR_SUCCESS, "cannot add feature %u\n", r );

    r = create_feature_components_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create FeatureComponents table %u\n", r );

    r = add_feature_components_entry( hdb, "'one', 'one'" );
    ok( r == ERROR_SUCCESS, "cannot add feature/component pair %u\n", r );

    r = add_feature_components_entry( hdb, "'two', 'two'" );
    ok( r == ERROR_SUCCESS, "cannot add feature/component pair %u\n", r );

    r = create_install_execute_sequence_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create InstallExecuteSequence table %u\n", r );

    r = add_install_execute_sequence_entry( hdb, "'CostInitialize', '', '800'" );
    ok( r == ERROR_SUCCESS, "cannot add install execute sequence entry %u\n", r );

    r = add_install_execute_sequence_entry( hdb, "'FileCost', '', '900'" );
    ok( r == ERROR_SUCCESS, "cannot add install execute sequence entry %u\n", r );

    r = add_install_execute_sequence_entry( hdb, "'CostFinalize', '', '1000'" );
    ok( r == ERROR_SUCCESS, "cannot add install execute sequence entry %u\n", r );

    r = add_install_execute_sequence_entry( hdb, "'InstallValidate', '', '1100'" );
    ok( r == ERROR_SUCCESS, "cannot add install execute sequence entry %u\n", r );

    MsiDatabaseCommit( hdb );

    sprintf( package, "#%u", hdb );
    r = MsiOpenPackageA( package, &hpkg );
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r );

    r = MsiEnumComponentCostsA( 0, NULL, 0, INSTALLSTATE_UNKNOWN, NULL, NULL, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", r );

    r = MsiEnumComponentCostsA( hpkg, NULL, 0, INSTALLSTATE_UNKNOWN, NULL, NULL, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", r );

    r = MsiEnumComponentCostsA( hpkg, NULL, 0, INSTALLSTATE_UNKNOWN, NULL, NULL, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", r );

    r = MsiEnumComponentCostsA( hpkg, "", 0, INSTALLSTATE_UNKNOWN, NULL, NULL, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", r );

    r = MsiEnumComponentCostsA( hpkg, "one", 0, INSTALLSTATE_UNKNOWN, NULL, NULL, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", r );

    r = MsiEnumComponentCostsA( hpkg, "one", 0, INSTALLSTATE_LOCAL, NULL, NULL, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", r );

    r = MsiEnumComponentCostsA( hpkg, "one", 0, INSTALLSTATE_LOCAL, drive, NULL, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", r );

    len = sizeof(drive);
    r = MsiEnumComponentCostsA( hpkg, "one", 0, INSTALLSTATE_LOCAL, drive, &len, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", r );

    len = sizeof(drive);
    r = MsiEnumComponentCostsA( hpkg, "one", 0, INSTALLSTATE_LOCAL, drive, &len, &cost, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", r );

    len = sizeof(drive);
    r = MsiEnumComponentCostsA( hpkg, "one", 0, INSTALLSTATE_LOCAL, drive, &len, &cost, &temp );
    todo_wine ok( r == ERROR_INVALID_HANDLE_STATE, "Expected ERROR_INVALID_HANDLE_STATE, got %u\n", r );

    len = sizeof(drive);
    r = MsiEnumComponentCostsA( hpkg, "one", 0, INSTALLSTATE_LOCAL, NULL, &len, &cost, &temp );
    ok( r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", r );

    MsiSetInternalUI( INSTALLUILEVEL_NONE, NULL );

    r = MsiDoActionA( hpkg, "CostInitialize" );
    ok( r == ERROR_SUCCESS, "CostInitialize failed %u\n", r );

    r = MsiDoActionA( hpkg, "FileCost" );
    ok( r == ERROR_SUCCESS, "FileCost failed %u\n", r );

    len = sizeof(drive);
    r = MsiEnumComponentCostsA( hpkg, "one", 0, INSTALLSTATE_LOCAL, drive, &len, &cost, &temp );
    ok( r == ERROR_FUNCTION_NOT_CALLED, "Expected ERROR_FUNCTION_NOT_CALLED, got %u\n", r );

    r = MsiDoActionA( hpkg, "CostFinalize" );
    ok( r == ERROR_SUCCESS, "CostFinalize failed %u\n", r );

    /* contrary to what msdn says InstallValidate must be called too */
    len = sizeof(drive);
    r = MsiEnumComponentCostsA( hpkg, "one", 0, INSTALLSTATE_LOCAL, drive, &len, &cost, &temp );
    todo_wine ok( r == ERROR_FUNCTION_NOT_CALLED, "Expected ERROR_FUNCTION_NOT_CALLED, got %u\n", r );

    r = MsiDoActionA( hpkg, "InstallValidate" );
    ok( r == ERROR_SUCCESS, "InstallValidate failed %u\n", r );

    len = 0;
    r = MsiEnumComponentCostsA( hpkg, "three", 0, INSTALLSTATE_LOCAL, drive, &len, &cost, &temp );
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %u\n", r );

    len = 0;
    r = MsiEnumComponentCostsA( hpkg, "one", 0, INSTALLSTATE_LOCAL, drive, &len, &cost, &temp );
    ok( r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %u\n", r );
    ok( len == 2, "expected len == 2, got %u\n", len );

    len = 2;
    r = MsiEnumComponentCostsA( hpkg, "one", 0, INSTALLSTATE_LOCAL, drive, &len, &cost, &temp );
    ok( r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %u\n", r );
    ok( len == 2, "expected len == 2, got %u\n", len );

    len = 2;
    r = MsiEnumComponentCostsA( hpkg, "one", 0, INSTALLSTATE_UNKNOWN, drive, &len, &cost, &temp );
    ok( r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %u\n", r );
    ok( len == 2, "expected len == 2, got %u\n", len );

    /* install state doesn't seem to matter */
    len = sizeof(drive);
    r = MsiEnumComponentCostsA( hpkg, "one", 0, INSTALLSTATE_UNKNOWN, drive, &len, &cost, &temp );
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r );

    len = sizeof(drive);
    r = MsiEnumComponentCostsA( hpkg, "one", 0, INSTALLSTATE_ABSENT, drive, &len, &cost, &temp );
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r );

    len = sizeof(drive);
    r = MsiEnumComponentCostsA( hpkg, "one", 0, INSTALLSTATE_SOURCE, drive, &len, &cost, &temp );
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r );

    len = sizeof(drive);
    drive[0] = 0;
    cost = temp = 0xdead;
    r = MsiEnumComponentCostsA( hpkg, "one", 0, INSTALLSTATE_LOCAL, drive, &len, &cost, &temp );
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r );
    ok( len == 2, "expected len == 2, got %u\n", len );
    ok( drive[0], "expected a drive\n" );
    ok( cost && cost != 0xdead, "expected cost > 0, got %d\n", cost );
    ok( !temp, "expected temp == 0, got %d\n", temp );

    len = sizeof(drive);
    drive[0] = 0;
    cost = temp = 0xdead;
    r = MsiEnumComponentCostsA( hpkg, "two", 0, INSTALLSTATE_LOCAL, drive, &len, &cost, &temp );
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r );
    ok( len == 2, "expected len == 2, got %u\n", len );
    ok( drive[0], "expected a drive\n" );
    ok( !cost, "expected cost == 0, got %d\n", cost );
    ok( !temp, "expected temp == 0, got %d\n", temp );

    len = sizeof(drive);
    drive[0] = 0;
    cost = temp = 0xdead;
    r = MsiEnumComponentCostsA( hpkg, "", 0, INSTALLSTATE_UNKNOWN, drive, &len, &cost, &temp );
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r );
    ok( len == 2, "expected len == 2, got %u\n", len );
    ok( drive[0], "expected a drive\n" );
    ok( !cost, "expected cost == 0, got %d\n", cost );
    ok( temp && temp != 0xdead, "expected temp > 0, got %d\n", temp );

    /* increased index */
    len = sizeof(drive);
    r = MsiEnumComponentCostsA( hpkg, "one", 1, INSTALLSTATE_LOCAL, drive, &len, &cost, &temp );
    ok( r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %u\n", r );

    len = sizeof(drive);
    r = MsiEnumComponentCostsA( hpkg, "", 1, INSTALLSTATE_UNKNOWN, drive, &len, &cost, &temp );
    ok( r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %u\n", r );

    MsiCloseHandle( hpkg );
error:
    MsiCloseHandle( hdb );
    DeleteFileA( msifile );
}

static void test_MsiDatabaseCommit(void)
{
    UINT r;
    MSIHANDLE hdb, hpkg = 0;
    char buf[32], package[12];
    DWORD sz;

    hdb = create_package_db();
    ok( hdb, "failed to create database\n" );

    r = create_property_table( hdb );
    ok( r == ERROR_SUCCESS, "can't create Property table %u\n", r );

    sprintf( package, "#%u", hdb );
    r = MsiOpenPackageA( package, &hpkg );
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok( r == ERROR_SUCCESS, "got %u\n", r );

    r = MsiSetPropertyA( hpkg, "PROP", "value" );
    ok( r == ERROR_SUCCESS, "got %u\n", r );

    buf[0] = 0;
    sz = sizeof(buf);
    r = MsiGetPropertyA( hpkg, "PROP", buf, &sz );
    ok( r == ERROR_SUCCESS, "MsiGetPropertyA returned %u\n", r );
    ok( !lstrcmpA( buf, "value" ), "got \"%s\"\n", buf );

    r = MsiDatabaseCommit( hdb );
    ok( r == ERROR_SUCCESS, "MsiDatabaseCommit returned %u\n", r );

    buf[0] = 0;
    sz = sizeof(buf);
    r = MsiGetPropertyA( hpkg, "PROP", buf, &sz );
    ok( r == ERROR_SUCCESS, "MsiGetPropertyA returned %u\n", r );
    ok( !lstrcmpA( buf, "value" ), "got \"%s\"\n", buf );

    MsiCloseHandle( hpkg );
error:
    MsiCloseHandle( hdb );
    DeleteFileA( msifile );
}

START_TEST(package)
{
    STATEMGRSTATUS status;
    BOOL ret = FALSE;

    init_functionpointers();

    if (pIsWow64Process)
        pIsWow64Process(GetCurrentProcess(), &is_wow64);

    GetCurrentDirectoryA(MAX_PATH, CURR_DIR);

    /* Create a restore point ourselves so we circumvent the multitude of restore points
     * that would have been created by all the installation and removal tests.
     *
     * This is not needed on version 5.0 where setting MSIFASTINSTALL prevents the
     * creation of restore points.
     */
    if (pSRSetRestorePointA && !pMsiGetComponentPathExA)
    {
        memset(&status, 0, sizeof(status));
        ret = notify_system_change(BEGIN_NESTED_SYSTEM_CHANGE, &status);
    }

    test_createpackage();
    test_doaction();
    test_gettargetpath_bad();
    test_settargetpath();
    test_props();
    test_property_table();
    test_condition();
    test_msipackage();
    test_formatrecord2();
    test_states();
    test_getproperty();
    test_removefiles();
    test_appsearch();
    test_appsearch_complocator();
    test_appsearch_reglocator();
    test_appsearch_inilocator();
    test_appsearch_drlocator();
    test_featureparents();
    test_installprops();
    test_launchconditions();
    test_ccpsearch();
    test_complocator();
    test_MsiGetSourcePath();
    test_shortlongsource();
    test_sourcedir();
    test_access();
    test_emptypackage();
    test_MsiGetProductProperty();
    test_MsiSetProperty();
    test_MsiApplyMultiplePatches();
    test_MsiApplyPatch();
    test_MsiEnumComponentCosts();
    test_MsiDatabaseCommit();

    if (pSRSetRestorePointA && !pMsiGetComponentPathExA && ret)
    {
        ret = notify_system_change(END_NESTED_SYSTEM_CHANGE, &status);
        if (ret)
            remove_restore_point(status.llSequenceNumber);
    }
}
