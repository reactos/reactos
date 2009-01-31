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

#include "wine/test.h"

static const char msifile[] = "winetest.msi";
char CURR_DIR[MAX_PATH];

static void get_user_sid(LPSTR *usersid)
{
    HANDLE token;
    BYTE buf[1024];
    DWORD size;
    PTOKEN_USER user;
    HMODULE hadvapi32 = GetModuleHandleA("advapi32.dll");
    static BOOL (WINAPI *pConvertSidToStringSidA)(PSID, LPSTR*);

    *usersid = NULL;
    pConvertSidToStringSidA = (void *)GetProcAddress(hadvapi32, "ConvertSidToStringSidA");
    if (!pConvertSidToStringSidA)
        return;

    OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token);
    size = sizeof(buf);
    GetTokenInformation(token, TokenUser, buf, size, &size);
    user = (PTOKEN_USER)buf;
    pConvertSidToStringSidA(user->User.Sid, usersid);
    CloseHandle(token);
}

/* RegDeleteTreeW from dlls/advapi32/registry.c */
static LSTATUS package_RegDeleteTreeW(HKEY hKey, LPCWSTR lpszSubKey)
{
    LONG ret;
    DWORD dwMaxSubkeyLen, dwMaxValueLen;
    DWORD dwMaxLen, dwSize;
    WCHAR szNameBuf[MAX_PATH], *lpszName = szNameBuf;
    HKEY hSubKey = hKey;

    if(lpszSubKey)
    {
        ret = RegOpenKeyExW(hKey, lpszSubKey, 0, KEY_READ, &hSubKey);
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

        ret = package_RegDeleteTreeW(hSubKey, lpszName);
        if (ret) goto cleanup;
    }

    if (lpszSubKey)
        ret = RegDeleteKeyW(hKey, lpszSubKey);
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

    if (FAILED(CLSIDFromString((LPOLESTR)in, &guid)))
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

    RegCreateKeyA(HKEY_LOCAL_MACHINE, comppath, &hkey);

    lstrcpyA(path, CURR_DIR);
    lstrcatA(path, "\\");
    if (!dir) lstrcatA(path, filename);

    RegSetValueExA(hkey, prod, 0, REG_SZ, (LPBYTE)path, lstrlenA(path));
    RegCloseKey(hkey);

    RegCreateKeyA(HKEY_LOCAL_MACHINE, prodpath, &hkey);
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
    package_RegDeleteTreeW(HKEY_LOCAL_MACHINE, substrW);

    MultiByteToWideChar(CP_ACP, 0, prodpath, -1, substrW, MAX_PATH);
    package_RegDeleteTreeW(HKEY_LOCAL_MACHINE, substrW);
}

static UINT do_query(MSIHANDLE hdb, const char *query, MSIHANDLE *phrec)
{
    MSIHANDLE hview = 0;
    UINT r, ret;

    /* open a select query */
    r = MsiDatabaseOpenView(hdb, query, &hview);
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

    r = MsiDatabaseOpenView(hdb, query, &hview);
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

make_add_entry(reglocator,
               "INSERT INTO `RegLocator` "
               "(`Signature_`, `Root`, `Key`, `Name`, `Type`) VALUES( %s )")

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

static UINT set_summary_info(MSIHANDLE hdb)
{
    UINT res;
    MSIHANDLE suminfo;

    /* build summary info */
    res = MsiGetSummaryInformation(hdb, NULL, 7, &suminfo);
    ok( res == ERROR_SUCCESS , "Failed to open summaryinfo\n" );

    res = MsiSummaryInfoSetProperty(suminfo,2, VT_LPSTR, 0,NULL,
                        "Installation Database");
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetProperty(suminfo,3, VT_LPSTR, 0,NULL,
                        "Installation Database");
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetProperty(suminfo,4, VT_LPSTR, 0,NULL,
                        "Wine Hackers");
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetProperty(suminfo,7, VT_LPSTR, 0,NULL,
                    ";1033");
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetProperty(suminfo,9, VT_LPSTR, 0,NULL,
                    "{913B8D18-FBB6-4CAC-A239-C74C11E3FA74}");
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetProperty(suminfo, 14, VT_I4, 100, NULL, NULL);
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetProperty(suminfo, 15, VT_I4, 0, NULL, NULL);
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

    DeleteFile(msifile);

    /* create an empty database */
    res = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb );
    ok( res == ERROR_SUCCESS , "Failed to create database\n" );
    if( res != ERROR_SUCCESS )
        return hdb;

    res = MsiDatabaseCommit( hdb );
    ok( res == ERROR_SUCCESS , "Failed to commit database\n" );

    res = set_summary_info(hdb);

    res = run_query( hdb,
            "CREATE TABLE `Directory` ( "
            "`Directory` CHAR(255) NOT NULL, "
            "`Directory_Parent` CHAR(255), "
            "`DefaultDir` CHAR(255) NOT NULL "
            "PRIMARY KEY `Directory`)" );
    ok( res == ERROR_SUCCESS , "Failed to create directory table\n" );

    return hdb;
}

static MSIHANDLE package_from_db(MSIHANDLE hdb)
{
    UINT res;
    CHAR szPackage[10];
    MSIHANDLE hPackage;

    sprintf(szPackage,"#%i",hdb);
    res = MsiOpenPackage(szPackage,&hPackage);
    if (res != ERROR_SUCCESS)
        return 0;

    res = MsiCloseHandle(hdb);
    if (res != ERROR_SUCCESS)
        return 0;

    return hPackage;
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

    GetSystemDirectory(path, MAX_PATH);
    lstrcatA(path, "\\kernel32.dll");

    CopyFileA(path, name, FALSE);

    size = GetFileVersionInfoSize(path, &handle);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);

    GetFileVersionInfoA(path, 0, size, buffer);

    pVerInfo = (VS_VERSIONINFO *)buffer;
    ofs = (BYTE *)&pVerInfo->szKey[lstrlenW(pVerInfo->szKey) + 1];
    pFixedInfo = (VS_FIXEDFILEINFO *)roundpos(pVerInfo, ofs, 4);

    pFixedInfo->dwFileVersionMS = ms;
    pFixedInfo->dwFileVersionLS = ls;
    pFixedInfo->dwProductVersionMS = ms;
    pFixedInfo->dwProductVersionLS = ls;

    resource = BeginUpdateResource(name, FALSE);
    if (!resource)
        goto done;

    if (!UpdateResource(resource, RT_VERSION, MAKEINTRESOURCE(VS_VERSION_INFO),
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), buffer, size))
        goto done;

    if (!EndUpdateResource(resource, FALSE))
        goto done;

    ret = TRUE;

done:
    HeapFree(GetProcessHeap(), 0, buffer);
    return ret;
}

static void test_createpackage(void)
{
    MSIHANDLE hPackage = 0;
    UINT res;

    hPackage = package_from_db(create_package_db());
    ok( hPackage != 0, " Failed to create package\n");

    res = MsiCloseHandle( hPackage);
    ok( res == ERROR_SUCCESS , "Failed to close package\n" );
    DeleteFile(msifile);
}

static void test_doaction( void )
{
    MSIHANDLE hpkg;
    UINT r;

    r = MsiDoAction( -1, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    hpkg = package_from_db(create_package_db());
    ok( hpkg, "failed to create package\n");

    r = MsiDoAction(hpkg, NULL);
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiDoAction(0, "boo");
    ok( r == ERROR_INVALID_HANDLE, "wrong return val\n");

    r = MsiDoAction(hpkg, "boo");
    ok( r == ERROR_FUNCTION_NOT_CALLED, "wrong return val\n");

    MsiCloseHandle( hpkg );
    DeleteFile(msifile);
}

static void test_gettargetpath_bad(void)
{
    char buffer[0x80];
    MSIHANDLE hpkg;
    DWORD sz;
    UINT r;

    hpkg = package_from_db(create_package_db());
    ok( hpkg, "failed to create package\n");

    r = MsiGetTargetPath( 0, NULL, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiGetTargetPath( 0, NULL, NULL, &sz );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiGetTargetPath( 0, "boo", NULL, NULL );
    ok( r == ERROR_INVALID_HANDLE, "wrong return val\n");

    r = MsiGetTargetPath( 0, "boo", NULL, NULL );
    ok( r == ERROR_INVALID_HANDLE, "wrong return val\n");

    r = MsiGetTargetPath( hpkg, "boo", NULL, NULL );
    ok( r == ERROR_DIRECTORY, "wrong return val\n");

    r = MsiGetTargetPath( hpkg, "boo", buffer, NULL );
    ok( r == ERROR_DIRECTORY, "wrong return val\n");

    MsiCloseHandle( hpkg );
    DeleteFile(msifile);
}

static void query_file_path(MSIHANDLE hpkg, LPCSTR file, LPSTR buff)
{
    UINT r;
    DWORD size;
    MSIHANDLE rec;

    rec = MsiCreateRecord( 1 );
    ok(rec, "MsiCreate record failed\n");

    r = MsiRecordSetString( rec, 0, file );
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r );

    size = MAX_PATH;
    r = MsiFormatRecord( hpkg, rec, buff, &size );
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

    hpkg = package_from_db( hdb );
    ok( hpkg, "failed to create package\n");

    r = MsiDoAction( hpkg, "CostInitialize");
    ok( r == ERROR_SUCCESS, "cost init failed\n");

    r = MsiDoAction( hpkg, "FileCost");
    ok( r == ERROR_SUCCESS, "file cost failed\n");

    r = MsiDoAction( hpkg, "CostFinalize");
    ok( r == ERROR_SUCCESS, "cost finalize failed\n");

    r = MsiSetTargetPath( 0, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiSetTargetPath( 0, "boo", "C:\\bogusx" );
    ok( r == ERROR_INVALID_HANDLE, "wrong return val\n");

    r = MsiSetTargetPath( hpkg, "boo", NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiSetTargetPath( hpkg, "boo", "c:\\bogusx" );
    ok( r == ERROR_DIRECTORY, "wrong return val\n");

    sz = sizeof tempdir - 1;
    r = MsiGetTargetPath( hpkg, "TARGETDIR", tempdir, &sz );
    sprintf( file, "%srootfile.txt", tempdir );
    query_file_path( hpkg, "[#RootFile]", buffer );
    ok( r == ERROR_SUCCESS, "failed to get target path: %d\n", r);
    ok( !lstrcmp(buffer, file), "Expected %s, got %s\n", file, buffer );

    GetTempFileName( tempdir, "_wt", 0, buffer );
    sprintf( tempdir, "%s\\subdir", buffer );

    r = MsiSetTargetPath( hpkg, "TARGETDIR", buffer );
    ok( r == ERROR_SUCCESS || r == ERROR_DIRECTORY,
        "MsiSetTargetPath on file returned %d\n", r );

    r = MsiSetTargetPath( hpkg, "TARGETDIR", tempdir );
    ok( r == ERROR_SUCCESS || r == ERROR_DIRECTORY,
        "MsiSetTargetPath on 'subdir' of file returned %d\n", r );

    DeleteFile( buffer );

    r = MsiSetTargetPath( hpkg, "TARGETDIR", buffer );
    ok( r == ERROR_SUCCESS, "MsiSetTargetPath returned %d\n", r );

    r = GetFileAttributes( buffer );
    ok ( r == INVALID_FILE_ATTRIBUTES, "file/directory exists after MsiSetTargetPath. Attributes: %08X\n", r );

    r = MsiSetTargetPath( hpkg, "TARGETDIR", tempdir );
    ok( r == ERROR_SUCCESS, "MsiSetTargetPath on subsubdir returned %d\n", r );

    sz = sizeof buffer - 1;
    lstrcat( tempdir, "\\" );
    r = MsiGetTargetPath( hpkg, "TARGETDIR", buffer, &sz );
    ok( r == ERROR_SUCCESS, "failed to get target path: %d\n", r);
    ok( !lstrcmp(buffer, tempdir), "Expected %s, got %s\n", tempdir, buffer);

    sprintf( file, "%srootfile.txt", tempdir );
    query_file_path( hpkg, "[#RootFile]", buffer );
    ok( !lstrcmp(buffer, file), "Expected %s, got %s\n", file, buffer);

    r = MsiSetTargetPath( hpkg, "TestParent", "C:\\one\\two" );
    ok( r == ERROR_SUCCESS, "MsiSetTargetPath returned %d\n", r );

    query_file_path( hpkg, "[#TestFile]", buffer );
    ok( !lstrcmp(buffer, "C:\\one\\two\\TestDir\\testfile.txt"),
        "Expected C:\\one\\two\\TestDir\\testfile.txt, got %s\n", buffer );

    sz = sizeof buffer - 1;
    r = MsiGetTargetPath( hpkg, "TestParent", buffer, &sz );
    ok( r == ERROR_SUCCESS, "failed to get target path: %d\n", r);
    ok( !lstrcmp(buffer, "C:\\one\\two\\"), "Expected C:\\one\\two\\, got %s\n", buffer);

    MsiCloseHandle( hpkg );
}

static void test_condition(void)
{
    MSICONDITION r;
    MSIHANDLE hpkg;

    hpkg = package_from_db(create_package_db());
    ok( hpkg, "failed to create package\n");

    r = MsiEvaluateCondition(0, NULL);
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, NULL);
    ok( r == MSICONDITION_NONE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "");
    ok( r == MSICONDITION_NONE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "-1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 = 0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 <> 0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 = 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 > 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 ~> 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 > 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 ~> 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 >= 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 ~>= 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 >= 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 ~>= 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 < 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 ~< 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 < 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 ~< 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 <= 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 ~<= 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 <= 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 ~<= 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 >=");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " ");
    ok( r == MSICONDITION_NONE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "LicView <> \"1\"");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "LicView <> \"0\"");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "LicView <> LicView");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "not 0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "not LicView");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "not \"A\"");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "~not \"A\"");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "\"0\"");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 and 2");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "not 0 and 3");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "not 0 and 0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "not 0 or 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "(0)");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "(((((1))))))");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "(((((1)))))");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " \"A\" < \"B\" ");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " \"A\" > \"B\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " \"1\" > \"12\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " \"100\" < \"21\" ");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 < > 0");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "(1<<1) == 2");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " \"A\" = \"a\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " \"A\" ~ = \"a\" ");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " \"A\" ~= \"a\" ");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " \"A\" ~= 1 ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " \"A\" = 1 ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " 1 ~= 1 ");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " 1 ~= \"1\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " 1 = \"1\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " 0 = \"1\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " 0 < \"100\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " 100 > \"0\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 XOR 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 IMP 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 IMP 0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 IMP 0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 EQV 0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 EQV 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 IMP 1 OR 0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 IMPL 1");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "\"ASFD\" >< \"S\" ");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "\"ASFD\" ~>< \"s\" ");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "\"ASFD\" ~>< \"\" ");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "\"ASFD\" ~>< \"sss\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "mm", "5" );

    r = MsiEvaluateCondition(hpkg, "mm = 5");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "mm < 6");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "mm <= 5");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "mm > 4");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "mm < 12");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "mm = \"5\"");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 = \"\"");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 AND \"\"");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 AND \"\"");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 AND \"1\"");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "3 >< 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "3 >< 4");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "NOT 0 AND 0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "NOT 0 AND 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "NOT 1 OR 0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 AND 1 OR 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 AND 0 OR 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "NOT 0 AND 1 OR 0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "_1 = _1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "( 1 AND 1 ) = 2");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "NOT ( 1 AND 1 )");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "NOT A AND (BBBBBBBBBB=2 OR CCC=1) AND Ddddddddd");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "Installed<>\"\"");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "NOT 1 AND 0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "bandalmael<>0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "bandalmael<0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "bandalmael>0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "bandalmael>=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "bandalmael<=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "bandalmael~<>0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "0" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "asdf" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "0asdf" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "0 " );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "-0" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "0000000000000" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "--0" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "0x00" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "-" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "+0" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "0.0" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");
    r = MsiEvaluateCondition(hpkg, "bandalmael<>0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "hi");
    MsiSetProperty(hpkg, "two", "hithere");
    r = MsiEvaluateCondition(hpkg, "one >< two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "hithere");
    MsiSetProperty(hpkg, "two", "hi");
    r = MsiEvaluateCondition(hpkg, "one >< two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "hello");
    MsiSetProperty(hpkg, "two", "hi");
    r = MsiEvaluateCondition(hpkg, "one >< two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "hellohithere");
    MsiSetProperty(hpkg, "two", "hi");
    r = MsiEvaluateCondition(hpkg, "one >< two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "");
    MsiSetProperty(hpkg, "two", "hi");
    r = MsiEvaluateCondition(hpkg, "one >< two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "hi");
    MsiSetProperty(hpkg, "two", "");
    r = MsiEvaluateCondition(hpkg, "one >< two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "");
    MsiSetProperty(hpkg, "two", "");
    r = MsiEvaluateCondition(hpkg, "one >< two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "1234");
    MsiSetProperty(hpkg, "two", "1");
    r = MsiEvaluateCondition(hpkg, "one >< two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "one 1234");
    MsiSetProperty(hpkg, "two", "1");
    r = MsiEvaluateCondition(hpkg, "one >< two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "hithere");
    MsiSetProperty(hpkg, "two", "hi");
    r = MsiEvaluateCondition(hpkg, "one << two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "hi");
    MsiSetProperty(hpkg, "two", "hithere");
    r = MsiEvaluateCondition(hpkg, "one << two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "hi");
    MsiSetProperty(hpkg, "two", "hi");
    r = MsiEvaluateCondition(hpkg, "one << two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "abcdhithere");
    MsiSetProperty(hpkg, "two", "hi");
    r = MsiEvaluateCondition(hpkg, "one << two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "");
    MsiSetProperty(hpkg, "two", "hi");
    r = MsiEvaluateCondition(hpkg, "one << two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "hithere");
    MsiSetProperty(hpkg, "two", "");
    r = MsiEvaluateCondition(hpkg, "one << two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "");
    MsiSetProperty(hpkg, "two", "");
    r = MsiEvaluateCondition(hpkg, "one << two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "1234");
    MsiSetProperty(hpkg, "two", "1");
    r = MsiEvaluateCondition(hpkg, "one << two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "1234 one");
    MsiSetProperty(hpkg, "two", "1");
    r = MsiEvaluateCondition(hpkg, "one << two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "hithere");
    MsiSetProperty(hpkg, "two", "there");
    r = MsiEvaluateCondition(hpkg, "one >> two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "hithere");
    MsiSetProperty(hpkg, "two", "hi");
    r = MsiEvaluateCondition(hpkg, "one >> two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "there");
    MsiSetProperty(hpkg, "two", "hithere");
    r = MsiEvaluateCondition(hpkg, "one >> two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "there");
    MsiSetProperty(hpkg, "two", "there");
    r = MsiEvaluateCondition(hpkg, "one >> two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "abcdhithere");
    MsiSetProperty(hpkg, "two", "hi");
    r = MsiEvaluateCondition(hpkg, "one >> two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "");
    MsiSetProperty(hpkg, "two", "there");
    r = MsiEvaluateCondition(hpkg, "one >> two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "there");
    MsiSetProperty(hpkg, "two", "");
    r = MsiEvaluateCondition(hpkg, "one >> two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "");
    MsiSetProperty(hpkg, "two", "");
    r = MsiEvaluateCondition(hpkg, "one >> two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "1234");
    MsiSetProperty(hpkg, "two", "4");
    r = MsiEvaluateCondition(hpkg, "one >> two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "one 1234");
    MsiSetProperty(hpkg, "two", "4");
    r = MsiEvaluateCondition(hpkg, "one >> two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "MsiNetAssemblySupport", NULL);  /* make sure it's empty */

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"1.1.4322\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport > \"1.1.4322\"");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport >= \"1.1.4322\"");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport <= \"1.1.4322\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport <> \"1.1.4322\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport ~< \"1.1.4322\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"abcd\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"a1.1.4322\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"1.1.4322a\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"0000001.1.4322\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"1.1.4322.1\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"1.1.4322.1.1\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "\"2\" < \"1.1");
    ok( r == MSICONDITION_ERROR, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "\"2\" < \"1.1\"");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "\"2\" < \"12.1\"");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "\"02.1\" < \"2.11\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "\"02.1.1\" < \"2.1\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"1.1\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"1\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"0\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"-1\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"a\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"!\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"!\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"/\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \" \"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"azAZ_\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"a[a]\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"a[a]a\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"[a]\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"[a]a\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"{a}\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"{a\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"[a\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"a{\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"a]\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"A\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    MsiSetProperty(hpkg, "MsiNetAssemblySupport", "1.1.4322");
    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"1.1.4322\"");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"1.1.14322\"");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"1.1.5\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"1.1\"");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"1\"");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    MsiSetProperty(hpkg, "one", "1");
    r = MsiEvaluateCondition(hpkg, "one < \"1\"");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "X", "5.0");

    r = MsiEvaluateCondition(hpkg, "X != \"\"");
    ok( r == MSICONDITION_ERROR, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "X =\"5.0\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "X =\"5.1\"");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "X =\"6.0\"");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "X =\"5.0\" or X =\"5.1\" or X =\"6.0\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "(X =\"5.0\" or X =\"5.1\" or X =\"6.0\")");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "X !=\"\" and (X =\"5.0\" or X =\"5.1\" or X =\"6.0\")");
    ok( r == MSICONDITION_ERROR, "wrong return val (%d)\n", r);

    /* feature doesn't exist */
    r = MsiEvaluateCondition(hpkg, "&nofeature");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    MsiSetProperty(hpkg, "A", "2");
    MsiSetProperty(hpkg, "X", "50");

    r = MsiEvaluateCondition(hpkg, "2 <= X");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "A <= X");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "A <= 50");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    MsiSetProperty(hpkg, "X", "50val");

    r = MsiEvaluateCondition(hpkg, "2 <= X");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "A <= X");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    MsiSetProperty(hpkg, "A", "7");
    MsiSetProperty(hpkg, "X", "50");

    r = MsiEvaluateCondition(hpkg, "7 <= X");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "A <= X");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "A <= 50");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    MsiSetProperty(hpkg, "X", "50val");

    r = MsiEvaluateCondition(hpkg, "2 <= X");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "A <= X");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    MsiCloseHandle( hpkg );
    DeleteFile(msifile);
}

static BOOL check_prop_empty( MSIHANDLE hpkg, const char * prop)
{
    UINT r;
    DWORD sz;
    char buffer[2];

    sz = sizeof buffer;
    strcpy(buffer,"x");
    r = MsiGetProperty( hpkg, prop, buffer, &sz );
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

    hpkg = package_from_db( hdb );
    ok( hpkg, "failed to create package\n");

    /* test invalid values */
    r = MsiGetProperty( 0, NULL, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiGetProperty( hpkg, NULL, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiGetProperty( hpkg, "boo", NULL, NULL );
    ok( r == ERROR_SUCCESS, "wrong return val\n");

    r = MsiGetProperty( hpkg, "boo", buffer, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    /* test retrieving an empty/nonexistent property */
    sz = sizeof buffer;
    r = MsiGetProperty( hpkg, "boo", NULL, &sz );
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( sz == 0, "wrong size returned\n");

    check_prop_empty( hpkg, "boo");
    sz = 0;
    strcpy(buffer,"x");
    r = MsiGetProperty( hpkg, "boo", buffer, &sz );
    ok( r == ERROR_MORE_DATA, "wrong return val\n");
    ok( !strcmp(buffer,"x"), "buffer was changed\n");
    ok( sz == 0, "wrong size returned\n");

    sz = 1;
    strcpy(buffer,"x");
    r = MsiGetProperty( hpkg, "boo", buffer, &sz );
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( buffer[0] == 0, "buffer was not changed\n");
    ok( sz == 0, "wrong size returned\n");

    /* set the property to something */
    r = MsiSetProperty( 0, NULL, NULL );
    ok( r == ERROR_INVALID_HANDLE, "wrong return val\n");

    r = MsiSetProperty( hpkg, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiSetProperty( hpkg, "", NULL );
    ok( r == ERROR_SUCCESS, "wrong return val\n");

    /* try set and get some illegal property identifiers */
    r = MsiSetProperty( hpkg, "", "asdf" );
    ok( r == ERROR_FUNCTION_FAILED, "wrong return val\n");

    r = MsiSetProperty( hpkg, "=", "asdf" );
    ok( r == ERROR_SUCCESS, "wrong return val\n");

    r = MsiSetProperty( hpkg, " ", "asdf" );
    ok( r == ERROR_SUCCESS, "wrong return val\n");

    r = MsiSetProperty( hpkg, "'", "asdf" );
    ok( r == ERROR_SUCCESS, "wrong return val\n");

    sz = sizeof buffer;
    buffer[0]=0;
    r = MsiGetProperty( hpkg, "'", buffer, &sz );
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( !strcmp(buffer,"asdf"), "buffer was not changed\n");

    /* set empty values */
    r = MsiSetProperty( hpkg, "boo", NULL );
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( check_prop_empty( hpkg, "boo"), "prop wasn't empty\n");

    r = MsiSetProperty( hpkg, "boo", "" );
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( check_prop_empty( hpkg, "boo"), "prop wasn't empty\n");

    /* set a non-empty value */
    r = MsiSetProperty( hpkg, "boo", "xyz" );
    ok( r == ERROR_SUCCESS, "wrong return val\n");

    sz = 1;
    strcpy(buffer,"x");
    r = MsiGetProperty( hpkg, "boo", buffer, &sz );
    ok( r == ERROR_MORE_DATA, "wrong return val\n");
    ok( buffer[0] == 0, "buffer was not changed\n");
    ok( sz == 3, "wrong size returned\n");

    sz = 4;
    strcpy(buffer,"x");
    r = MsiGetProperty( hpkg, "boo", buffer, &sz );
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( !strcmp(buffer,"xyz"), "buffer was not changed\n");
    ok( sz == 3, "wrong size returned\n");

    sz = 3;
    strcpy(buffer,"x");
    r = MsiGetProperty( hpkg, "boo", buffer, &sz );
    ok( r == ERROR_MORE_DATA, "wrong return val\n");
    ok( !strcmp(buffer,"xy"), "buffer was not changed\n");
    ok( sz == 3, "wrong size returned\n");

    r = MsiSetProperty(hpkg, "SourceDir", "foo");
    ok( r == ERROR_SUCCESS, "wrong return val\n");

    sz = 4;
    r = MsiGetProperty(hpkg, "SOURCEDIR", buffer, &sz);
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( !strcmp(buffer,""), "buffer wrong\n");
    ok( sz == 0, "wrong size returned\n");

    sz = 4;
    r = MsiGetProperty(hpkg, "SOMERANDOMNAME", buffer, &sz);
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( !strcmp(buffer,""), "buffer wrong\n");
    ok( sz == 0, "wrong size returned\n");

    sz = 4;
    r = MsiGetProperty(hpkg, "SourceDir", buffer, &sz);
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( !strcmp(buffer,"foo"), "buffer wrong\n");
    ok( sz == 3, "wrong size returned\n");

    r = MsiSetProperty(hpkg, "MetadataCompName", "Photoshop.dll");
    ok( r == ERROR_SUCCESS, "wrong return val\n");

    sz = 0;
    r = MsiGetProperty(hpkg, "MetadataCompName", NULL, &sz );
    ok( r == ERROR_SUCCESS, "return wrong\n");
    ok( sz == 13, "size wrong (%d)\n", sz);

    sz = 13;
    r = MsiGetProperty(hpkg, "MetadataCompName", buffer, &sz );
    ok( r == ERROR_MORE_DATA, "return wrong\n");
    ok( !strcmp(buffer,"Photoshop.dl"), "buffer wrong\n");

    r = MsiSetProperty(hpkg, "property", "value");
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sz = 6;
    r = MsiGetProperty(hpkg, "property", buffer, &sz);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok( !strcmp(buffer, "value"), "Expected value, got %s\n", buffer);

    r = MsiSetProperty(hpkg, "property", NULL);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sz = 6;
    r = MsiGetProperty(hpkg, "property", buffer, &sz);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok( !strlen(buffer), "Expected empty string, got %s\n", buffer);

    MsiCloseHandle( hpkg );
    DeleteFile(msifile);
}

static BOOL find_prop_in_property(MSIHANDLE hdb, LPCSTR prop, LPCSTR val)
{
    MSIHANDLE hview, hrec;
    BOOL found;
    CHAR buffer[MAX_PATH];
    DWORD sz;
    UINT r;

    r = MsiDatabaseOpenView(hdb, "SELECT * FROM `_Property`", &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");

    found = FALSE;
    while (r == ERROR_SUCCESS && !found)
    {
        r = MsiViewFetch(hview, &hrec);
        if (r != ERROR_SUCCESS) break;

        sz = MAX_PATH;
        r = MsiRecordGetString(hrec, 1, buffer, &sz);
        if (r == ERROR_SUCCESS && !lstrcmpA(buffer, prop))
        {
            sz = MAX_PATH;
            r = MsiRecordGetString(hrec, 2, buffer, &sz);
            if (r == ERROR_SUCCESS && !lstrcmpA(buffer, val))
                found = TRUE;
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
    char buffer[MAX_PATH];
    DWORD sz;
    BOOL found;

    hdb = create_package_db();
    ok( hdb, "failed to create package\n");

    hpkg = package_from_db(hdb);
    ok( hpkg, "failed to create package\n");

    MsiCloseHandle(hdb);

    hdb = MsiGetActiveDatabase(hpkg);

    query = "CREATE TABLE `_Property` ( "
        "`foo` INT NOT NULL, `bar` INT LOCALIZABLE PRIMARY KEY `foo`)";
    r = run_query(hdb, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    MsiCloseHandle(hdb);
    MsiCloseHandle(hpkg);
    DeleteFile(msifile);

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

    hpkg = package_from_db(hdb);
    todo_wine
    {
        ok(!hpkg, "package should not be created\n");
    }

    MsiCloseHandle(hdb);
    MsiCloseHandle(hpkg);
    DeleteFile(msifile);

    hdb = create_package_db();
    ok (hdb, "failed to create package database\n");

    r = create_property_table(hdb);
    ok(r == ERROR_SUCCESS, "cannot create Property table: %d\n", r);

    r = add_property_entry(hdb, "'prop', 'val'");
    ok(r == ERROR_SUCCESS, "cannot add property: %d\n", r);

    hpkg = package_from_db(hdb);
    ok(hpkg, "failed to create package\n");

    MsiCloseHandle(hdb);

    sz = MAX_PATH;
    r = MsiGetProperty(hpkg, "prop", buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmp(buffer, "val"), "Expected val, got %s\n", buffer);

    hdb = MsiGetActiveDatabase(hpkg);

    found = find_prop_in_property(hdb, "prop", "val");
    ok(found, "prop should be in the _Property table\n");

    r = add_property_entry(hdb, "'dantes', 'mercedes'");
    ok(r == ERROR_SUCCESS, "cannot add property: %d\n", r);

    query = "SELECT * FROM `_Property` WHERE `Property` = 'dantes'";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    found = find_prop_in_property(hdb, "dantes", "mercedes");
    ok(found == FALSE, "dantes should not be in the _Property table\n");

    sz = MAX_PATH;
    lstrcpy(buffer, "aaa");
    r = MsiGetProperty(hpkg, "dantes", buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(lstrlenA(buffer) == 0, "Expected empty string, got %s\n", buffer);

    r = MsiSetProperty(hpkg, "dantes", "mercedes");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    found = find_prop_in_property(hdb, "dantes", "mercedes");
    ok(found == TRUE, "dantes should be in the _Property table\n");

    MsiCloseHandle(hdb);
    MsiCloseHandle(hpkg);
    DeleteFile(msifile);
}

static UINT try_query_param( MSIHANDLE hdb, LPCSTR szQuery, MSIHANDLE hrec )
{
    MSIHANDLE htab = 0;
    UINT res;

    res = MsiDatabaseOpenView( hdb, szQuery, &htab );
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
    r = MsiOpenPackage(NULL, &hpack);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* empty szPackagePath */
    r = MsiOpenPackage("", &hpack);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    }

    if (r == ERROR_SUCCESS)
        MsiCloseHandle(hpack);

    /* nonexistent szPackagePath */
    r = MsiOpenPackage("nonexistent", &hpack);
    ok(r == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", r);

    /* NULL hProduct */
    r = MsiOpenPackage(msifile, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    name[0]='#';
    name[1]=0;
    r = MsiOpenPackage(name, &hpack);
    ok(r == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %d\n", r);

    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* database exists, but is emtpy */
    sprintf(name, "#%d", hdb);
    r = MsiOpenPackage(name, &hpack);
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
    r = MsiOpenPackage(name, &hpack);
    ok(r == ERROR_INSTALL_PACKAGE_INVALID,
       "Expected ERROR_INSTALL_PACKAGE_INVALID, got %d\n", r);

    MsiCloseHandle(hdb);
    DeleteFile(msifile);

    /* start with a clean database to show what constitutes a valid package */
    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sprintf(name, "#%d", hdb);

    /* The following summary information props must exist:
     *  - PID_REVNUMBER
     *  - PID_PAGECOUNT
     */

    set_summary_dword(hdb, PID_PAGECOUNT, 100);
    r = MsiOpenPackage(name, &hpack);
    ok(r == ERROR_INSTALL_PACKAGE_INVALID,
       "Expected ERROR_INSTALL_PACKAGE_INVALID, got %d\n", r);

    set_summary_str(hdb, PID_REVNUMBER, "{004757CD-5092-49c2-AD20-28E1CE0DF5F2}");
    r = MsiOpenPackage(name, &hpack);
    ok(r == ERROR_SUCCESS,
       "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(hpack);
    MsiCloseHandle(hdb);
    DeleteFile(msifile);
}

static void test_formatrecord2(void)
{
    MSIHANDLE hpkg, hrec ;
    char buffer[0x100];
    DWORD sz;
    UINT r;

    hpkg = package_from_db(create_package_db());
    ok( hpkg, "failed to create package\n");

    r = MsiSetProperty(hpkg, "Manufacturer", " " );
    ok( r == ERROR_SUCCESS, "set property failed\n");

    hrec = MsiCreateRecord(2);
    ok(hrec, "create record failed\n");

    r = MsiRecordSetString( hrec, 0, "[ProgramFilesFolder][Manufacturer]\\asdf");
    ok( r == ERROR_SUCCESS, "format record failed\n");

    buffer[0] = 0;
    sz = sizeof buffer;
    r = MsiFormatRecord( hpkg, hrec, buffer, &sz );

    r = MsiRecordSetString(hrec, 0, "[foo][1]");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(hpkg, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"hoo"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "x[~]x");
    sz = sizeof buffer;
    r = MsiFormatRecord(hpkg, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"x"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "[foo.$%}][1]");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(hpkg, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"hoo"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "[\\[]");
    sz = sizeof buffer;
    r = MsiFormatRecord(hpkg, hrec, buffer, &sz);
    ok( sz == 1, "size wrong\n");
    ok( 0 == strcmp(buffer,"["), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    SetEnvironmentVariable("FOO", "BAR");
    r = MsiRecordSetString(hrec, 0, "[%FOO]");
    sz = sizeof buffer;
    r = MsiFormatRecord(hpkg, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"BAR"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "[[1]]");
    r = MsiRecordSetString(hrec, 1, "%FOO");
    sz = sizeof buffer;
    r = MsiFormatRecord(hpkg, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"BAR"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    MsiCloseHandle( hrec );
    MsiCloseHandle( hpkg );
    DeleteFile(msifile);
}

static void test_states(void)
{
    MSIHANDLE hpkg;
    UINT r;
    MSIHANDLE hdb;
    INSTALLSTATE state, action;

    static const CHAR msifile2[] = "winetest2.msi";
    static const CHAR msifile3[] = "winetest3.msi";

    hdb = create_package_db();
    ok ( hdb, "failed to create package database\n" );

    r = add_directory_entry( hdb, "'TARGETDIR', '', 'SourceDir'");
    ok( r == ERROR_SUCCESS, "cannot add directory: %d\n", r );

    r = create_property_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create Property table: %d\n", r );

    r = add_property_entry( hdb, "'ProductCode', '{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}'" );
    ok( r == ERROR_SUCCESS, "cannot add property entry: %d\n", r );

    r = add_property_entry( hdb, "'ProductLanguage', '1033'" );
    ok( r == ERROR_SUCCESS, "cannot add property entry: %d\n", r );

    r = add_property_entry( hdb, "'ProductName', 'MSITEST'" );
    ok( r == ERROR_SUCCESS, "cannot add property entry: %d\n", r );

    r = add_property_entry( hdb, "'ProductVersion', '1.1.1'" );
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

    MsiDatabaseCommit(hdb);

    /* these properties must not be in the saved msi file */
    r = add_property_entry( hdb, "'ADDLOCAL', 'one,four'");
    ok( r == ERROR_SUCCESS, "cannot add property: %d\n", r );

    r = add_property_entry( hdb, "'ADDSOURCE', 'two,three'");
    ok( r == ERROR_SUCCESS, "cannot add property: %d\n", r );

    r = add_property_entry( hdb, "'REMOVE', 'six,seven'");
    ok( r == ERROR_SUCCESS, "cannot add property: %d\n", r );

    hpkg = package_from_db( hdb );
    ok( hpkg, "failed to create package\n");

    MsiCloseHandle(hdb);

    CopyFileA(msifile, msifile2, FALSE);
    CopyFileA(msifile, msifile3, FALSE);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "one", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "two", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "three", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "four", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "five", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "six", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "seven", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "alpha", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "beta", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "gamma", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "theta", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "delta", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "epsilon", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "zeta", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "iota", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "eta", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "kappa", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "lambda", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "mu", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "nu", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "xi", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "omicron", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "pi", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "rho", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "sigma", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    r = MsiDoAction( hpkg, "CostInitialize");
    ok( r == ERROR_SUCCESS, "cost init failed\n");

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "one", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "two", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "three", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "four", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "five", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "six", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "seven", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "alpha", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "beta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "gamma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "theta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "delta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "epsilon", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "zeta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "iota", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "eta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "kappa", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "lambda", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "mu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "nu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "xi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "omicron", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "pi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "rho", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "sigma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    r = MsiDoAction( hpkg, "FileCost");
    ok( r == ERROR_SUCCESS, "file cost failed\n");

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "one", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "two", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "three", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "four", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "five", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "six", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "seven", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "alpha", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "beta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "gamma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "theta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "delta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "epsilon", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "zeta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "iota", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "eta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "kappa", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "lambda", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "mu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "nu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "xi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "omicron", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "pi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "rho", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "sigma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    r = MsiDoAction( hpkg, "CostFinalize");
    ok( r == ERROR_SUCCESS, "cost finalize failed: %d\n", r);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "one", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "two", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "three", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "four", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "five", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "six", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "seven", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "alpha", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "beta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "gamma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "theta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "delta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "epsilon", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "zeta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "iota", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "eta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "kappa", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "lambda", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "mu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "nu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "xi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "omicron", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "pi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "rho", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "sigma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    MsiCloseHandle( hpkg );

    /* publish the features and components */
    r = MsiInstallProduct(msifile, "ADDLOCAL=one,four ADDSOURCE=two,three REMOVE=six,seven");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiOpenDatabase(msifile, MSIDBOPEN_DIRECT, &hdb);
    ok(r == ERROR_SUCCESS, "failed to open database: %d\n", r);

    /* these properties must not be in the saved msi file */
    r = add_property_entry( hdb, "'ADDLOCAL', 'one,four'");
    ok( r == ERROR_SUCCESS, "cannot add property: %d\n", r );

    r = add_property_entry( hdb, "'ADDSOURCE', 'two,three'");
    ok( r == ERROR_SUCCESS, "cannot add property: %d\n", r );

    r = add_property_entry( hdb, "'REMOVE', 'six,seven'");
    ok( r == ERROR_SUCCESS, "cannot add property: %d\n", r );

    hpkg = package_from_db( hdb );
    ok( hpkg, "failed to create package\n");

    MsiCloseHandle(hdb);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "one", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "two", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "three", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "four", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "five", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "six", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "seven", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "alpha", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "beta", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "gamma", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "theta", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "delta", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "epsilon", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "zeta", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "iota", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "eta", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "kappa", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "lambda", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "mu", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "nu", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "xi", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "omicron", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "pi", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "rho", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "sigma", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    r = MsiDoAction( hpkg, "CostInitialize");
    ok( r == ERROR_SUCCESS, "cost init failed\n");

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "one", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "two", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "three", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "four", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "five", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "six", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "seven", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "alpha", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "beta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "gamma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "theta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "delta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "epsilon", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "zeta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "iota", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "eta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "kappa", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "lambda", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "mu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "nu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "xi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "omicron", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "pi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "rho", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "sigma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    r = MsiDoAction( hpkg, "FileCost");
    ok( r == ERROR_SUCCESS, "file cost failed\n");

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "one", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "two", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "three", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "four", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "five", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "six", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "seven", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "alpha", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "beta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "gamma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "theta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "delta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "epsilon", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "zeta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "iota", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "eta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "kappa", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "lambda", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "mu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "nu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "xi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "omicron", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "pi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "rho", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "sigma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    r = MsiDoAction( hpkg, "CostFinalize");
    ok( r == ERROR_SUCCESS, "cost finalize failed: %d\n", r);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "one", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "two", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    ok( action == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "three", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "four", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "five", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "six", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "seven", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "alpha", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "beta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    ok( action == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "gamma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "theta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "delta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "epsilon", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "zeta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "iota", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "eta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "kappa", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "lambda", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "mu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "nu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "xi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "omicron", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "pi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "rho", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "sigma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    MsiCloseHandle(hpkg);

    /* uninstall the product */
    r = MsiInstallProduct(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* all features installed locally */
    r = MsiInstallProduct(msifile2, "ADDLOCAL=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiOpenDatabase(msifile2, MSIDBOPEN_DIRECT, &hdb);
    ok(r == ERROR_SUCCESS, "failed to open database: %d\n", r);

    /* these properties must not be in the saved msi file */
    r = add_property_entry( hdb, "'ADDLOCAL', 'one,two,three,four,five,six,seven'");
    ok( r == ERROR_SUCCESS, "cannot add property: %d\n", r );

    hpkg = package_from_db( hdb );
    ok( hpkg, "failed to create package\n");

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "one", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "two", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "three", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "four", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "five", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "six", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "seven", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "alpha", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "beta", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "gamma", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "theta", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "delta", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "epsilon", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "zeta", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "iota", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "eta", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "kappa", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "lambda", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "mu", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "nu", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "xi", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "omicron", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "pi", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "rho", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "sigma", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    r = MsiDoAction( hpkg, "CostInitialize");
    ok( r == ERROR_SUCCESS, "cost init failed\n");

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "one", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "two", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "three", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "four", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "five", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "six", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "seven", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "alpha", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "beta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "gamma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "theta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "delta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "epsilon", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "zeta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "iota", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "eta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "kappa", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "lambda", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "mu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "nu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "xi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "omicron", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "pi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "rho", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "sigma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    r = MsiDoAction( hpkg, "FileCost");
    ok( r == ERROR_SUCCESS, "file cost failed\n");

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "one", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "two", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "three", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "four", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "five", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "six", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "seven", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "alpha", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "beta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "gamma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "theta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "delta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "epsilon", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "zeta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "iota", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "eta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "kappa", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "lambda", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "mu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "nu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "xi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "omicron", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "pi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "rho", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "sigma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    r = MsiDoAction( hpkg, "CostFinalize");
    ok( r == ERROR_SUCCESS, "cost finalize failed: %d\n", r);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "one", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "two", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "three", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "four", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "five", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "six", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "seven", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "alpha", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "beta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "gamma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "theta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "delta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "epsilon", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    ok( action == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "zeta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "iota", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "eta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "kappa", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "lambda", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "mu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "nu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "xi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "omicron", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "pi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    ok( action == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "rho", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "sigma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    MsiCloseHandle(hpkg);

    /* uninstall the product */
    r = MsiInstallProduct(msifile2, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* all features installed from source */
    r = MsiInstallProduct(msifile3, "ADDSOURCE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiOpenDatabase(msifile3, MSIDBOPEN_DIRECT, &hdb);
    ok(r == ERROR_SUCCESS, "failed to open database: %d\n", r);

    /* this property must not be in the saved msi file */
    r = add_property_entry( hdb, "'ADDSOURCE', 'one,two,three,four,five,six,seven'");
    ok( r == ERROR_SUCCESS, "cannot add property: %d\n", r );

    hpkg = package_from_db( hdb );
    ok( hpkg, "failed to create package\n");

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "one", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "two", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "three", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "four", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "five", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "six", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "seven", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "alpha", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "beta", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "gamma", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "theta", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "delta", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "epsilon", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "zeta", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "iota", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "eta", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "kappa", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "lambda", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "mu", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "nu", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "xi", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "omicron", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "pi", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "rho", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "sigma", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    r = MsiDoAction( hpkg, "CostInitialize");
    ok( r == ERROR_SUCCESS, "cost init failed\n");

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "one", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "two", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "three", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "four", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "five", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "six", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "seven", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "alpha", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "beta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "gamma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "theta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "delta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "epsilon", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "zeta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "iota", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "eta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "kappa", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "lambda", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "mu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "nu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "xi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "omicron", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "pi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "rho", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "sigma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    r = MsiDoAction( hpkg, "FileCost");
    ok( r == ERROR_SUCCESS, "file cost failed\n");

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "one", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "two", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "three", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "four", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "five", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "six", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "seven", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "alpha", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "beta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "gamma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "theta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "delta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "epsilon", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "zeta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "iota", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "eta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "kappa", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "lambda", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "mu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "nu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "xi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "omicron", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "pi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "rho", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "sigma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    r = MsiDoAction( hpkg, "CostFinalize");
    ok( r == ERROR_SUCCESS, "cost finalize failed: %d\n", r);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "one", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "two", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "three", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "four", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "five", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "six", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "seven", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "alpha", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "beta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "gamma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "theta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "delta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "epsilon", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "zeta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "iota", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "eta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "kappa", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "lambda", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "mu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "nu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "xi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "omicron", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "pi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "rho", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "sigma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    MsiCloseHandle(hpkg);

    /* uninstall the product */
    r = MsiInstallProduct(msifile3, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    DeleteFileA(msifile);
    DeleteFileA(msifile2);
    DeleteFileA(msifile3);
}

static void test_getproperty(void)
{
    MSIHANDLE hPackage = 0;
    char prop[100];
    static CHAR empty[] = "";
    DWORD size;
    UINT r;

    hPackage = package_from_db(create_package_db());
    ok( hPackage != 0, " Failed to create package\n");

    /* set the property */
    r = MsiSetProperty(hPackage, "Name", "Value");
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* retrieve the size, NULL pointer */
    size = 0;
    r = MsiGetProperty(hPackage, "Name", NULL, &size);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok( size == 5, "Expected 5, got %d\n", size);

    /* retrieve the size, empty string */
    size = 0;
    r = MsiGetProperty(hPackage, "Name", empty, &size);
    ok( r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok( size == 5, "Expected 5, got %d\n", size);

    /* don't change size */
    r = MsiGetProperty(hPackage, "Name", prop, &size);
    ok( r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok( size == 5, "Expected 5, got %d\n", size);
    ok( !lstrcmp(prop, "Valu"), "Expected Valu, got %s\n", prop);

    /* increase the size by 1 */
    size++;
    r = MsiGetProperty(hPackage, "Name", prop, &size);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok( size == 5, "Expected 5, got %d\n", size);
    ok( !lstrcmp(prop, "Value"), "Expected Value, got %s\n", prop);

    r = MsiCloseHandle( hPackage);
    ok( r == ERROR_SUCCESS , "Failed to close package\n" );
    DeleteFile(msifile);
}

static void test_removefiles(void)
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

    r = create_remove_file_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create Remove File table: %d\n", r);

    hpkg = package_from_db( hdb );
    ok( hpkg, "failed to create package\n");

    MsiCloseHandle( hdb );

    create_test_file( "hydrogen.txt" );
    create_test_file( "helium.txt" );
    create_test_file( "lithium.txt" );
    create_test_file( "beryllium.txt" );
    create_test_file( "boron.txt" );
    create_test_file( "carbon.txt" );

    r = MsiSetProperty( hpkg, "TARGETDIR", CURR_DIR );
    ok( r == ERROR_SUCCESS, "set property failed\n");

    r = MsiDoAction( hpkg, "CostInitialize");
    ok( r == ERROR_SUCCESS, "cost init failed\n");

    r = MsiDoAction( hpkg, "FileCost");
    ok( r == ERROR_SUCCESS, "cost finalize failed\n");

    r = MsiDoAction( hpkg, "CostFinalize");
    ok( r == ERROR_SUCCESS, "cost finalize failed\n");

    r = MsiDoAction( hpkg, "InstallValidate");
    ok( r == ERROR_SUCCESS, "cost finalize failed\n");

    r = MsiSetComponentState( hpkg, "hydrogen", INSTALLSTATE_ABSENT );
    ok( r == ERROR_SUCCESS, "failed to set component state: %d\n", r);

    r = MsiSetComponentState( hpkg, "helium", INSTALLSTATE_LOCAL );
    ok( r == ERROR_SUCCESS, "failed to set component state: %d\n", r);

    r = MsiSetComponentState( hpkg, "lithium", INSTALLSTATE_SOURCE );
    ok( r == ERROR_SUCCESS, "failed to set component state: %d\n", r);

    r = MsiSetComponentState( hpkg, "beryllium", INSTALLSTATE_ABSENT );
    ok( r == ERROR_SUCCESS, "failed to set component state: %d\n", r);

    r = MsiSetComponentState( hpkg, "boron", INSTALLSTATE_LOCAL );
    ok( r == ERROR_SUCCESS, "failed to set component state: %d\n", r);

    r = MsiSetComponentState( hpkg, "carbon", INSTALLSTATE_SOURCE );
    ok( r == ERROR_SUCCESS, "failed to set component state: %d\n", r);

    r = MsiDoAction( hpkg, "RemoveFiles");
    ok( r == ERROR_SUCCESS, "remove files failed\n");

    ok(DeleteFileA("hydrogen.txt"), "Expected hydrogen.txt to exist\n");
    ok(DeleteFileA("lithium.txt"), "Expected lithium.txt to exist\n");    
    ok(DeleteFileA("beryllium.txt"), "Expected beryllium.txt to exist\n");
    ok(DeleteFileA("carbon.txt"), "Expected carbon.txt to exist\n");
    ok(DeleteFileA("helium.txt"), "Expected helium.txt to exist\n");
    ok(DeleteFileA("boron.txt"), "Expected boron.txt to exist\n");

    MsiCloseHandle( hpkg );
    DeleteFileA(msifile);
}

static void test_appsearch(void)
{
    MSIHANDLE hpkg;
    UINT r;
    MSIHANDLE hdb;
    CHAR prop[MAX_PATH];
    DWORD size = MAX_PATH;

    hdb = create_package_db();
    ok ( hdb, "failed to create package database\n" );

    r = create_appsearch_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create AppSearch table: %d\n", r );

    r = add_appsearch_entry( hdb, "'WEBBROWSERPROG', 'NewSignature1'" );
    ok( r == ERROR_SUCCESS, "cannot add entry: %d\n", r );

    r = create_reglocator_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create RegLocator table: %d\n", r );

    r = add_reglocator_entry( hdb, "'NewSignature1', 0, 'htmlfile\\shell\\open\\command', '', 1" );
    ok( r == ERROR_SUCCESS, "cannot create RegLocator table: %d\n", r );

    r = create_signature_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create Signature table: %d\n", r );

    r = add_signature_entry( hdb, "'NewSignature1', 'FileName', '', '', '', '', '', '', ''" );
    ok( r == ERROR_SUCCESS, "cannot create Signature table: %d\n", r );

    hpkg = package_from_db( hdb );
    ok( hpkg, "failed to create package\n");

    MsiCloseHandle( hdb );

    r = MsiDoAction( hpkg, "AppSearch" );
    ok( r == ERROR_SUCCESS, "AppSearch failed: %d\n", r);

    r = MsiGetPropertyA( hpkg, "WEBBROWSERPROG", prop, &size );
    ok( r == ERROR_SUCCESS, "get property failed: %d\n", r);
    todo_wine
    {
        ok( lstrlenA(prop) != 0, "Expected non-zero length\n");
    }

    MsiCloseHandle( hpkg );
    DeleteFileA(msifile);
}

static void test_appsearch_complocator(void)
{
    MSIHANDLE hpkg, hdb;
    CHAR path[MAX_PATH];
    CHAR prop[MAX_PATH];
    LPSTR usersid;
    DWORD size;
    UINT r;

    get_user_sid(&usersid);
    if (!usersid)
    {
        skip("ConvertSidToStringSidA is not available\n");
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

    hpkg = package_from_db(hdb);
    ok(hpkg, "Expected a valid package handle\n");

    r = MsiSetPropertyA(hpkg, "SIGPROP8", "october");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiDoAction(hpkg, "AppSearch");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    sprintf(path, "%s\\FileName1", CURR_DIR);
    r = MsiGetPropertyA(hpkg, "SIGPROP1", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    sprintf(path, "%s\\FileName2", CURR_DIR);
    r = MsiGetPropertyA(hpkg, "SIGPROP2", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    sprintf(path, "%s\\FileName3", CURR_DIR);
    r = MsiGetPropertyA(hpkg, "SIGPROP3", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    sprintf(path, "%s\\FileName4", CURR_DIR);
    r = MsiGetPropertyA(hpkg, "SIGPROP4", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected \"\", got \"%s\"\n", prop);

    size = MAX_PATH;
    sprintf(path, "%s\\FileName5", CURR_DIR);
    r = MsiGetPropertyA(hpkg, "SIGPROP5", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    sprintf(path, "%s\\", CURR_DIR);
    r = MsiGetPropertyA(hpkg, "SIGPROP6", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    sprintf(path, "%s\\", CURR_DIR);
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
    sprintf(path, "%s\\FileName8.dll", CURR_DIR);
    r = MsiGetPropertyA(hpkg, "SIGPROP10", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP11", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected \"\", got \"%s\"\n", prop);

    size = MAX_PATH;
    sprintf(path, "%s\\FileName10.dll", CURR_DIR);
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
    MsiCloseHandle(hpkg);
    DeleteFileA(msifile);
}

static void test_appsearch_reglocator(void)
{
    MSIHANDLE hpkg, hdb;
    CHAR path[MAX_PATH];
    CHAR prop[MAX_PATH];
    DWORD binary[2];
    DWORD size, val;
    BOOL space, version;
    HKEY hklm, classes;
    HKEY hkcu, users;
    LPCSTR str;
    LPSTR ptr;
    LONG res;
    UINT r;

    version = TRUE;
    if (!create_file_with_version("test.dll", MAKELONG(2, 1), MAKELONG(4, 3)))
        version = FALSE;

    DeleteFileA("test.dll");

    res = RegCreateKeyA(HKEY_CLASSES_ROOT, "Software\\Wine", &classes);
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

    create_test_file("FileName1");
    sprintf(path, "%s\\FileName1", CURR_DIR);
    res = RegSetValueExA(hklm, "Value9", 0, REG_SZ,
                         (const BYTE *)path, lstrlenA(path) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    sprintf(path, "%s\\FileName2", CURR_DIR);
    res = RegSetValueExA(hklm, "Value10", 0, REG_SZ,
                         (const BYTE *)path, lstrlenA(path) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    lstrcpyA(path, CURR_DIR);
    res = RegSetValueExA(hklm, "Value11", 0, REG_SZ,
                         (const BYTE *)path, lstrlenA(path) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = RegSetValueExA(hklm, "Value12", 0, REG_SZ,
                         (const BYTE *)"", 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    create_file_with_version("FileName3.dll", MAKELONG(2, 1), MAKELONG(4, 3));
    sprintf(path, "%s\\FileName3.dll", CURR_DIR);
    res = RegSetValueExA(hklm, "Value13", 0, REG_SZ,
                         (const BYTE *)path, lstrlenA(path) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    create_file_with_version("FileName4.dll", MAKELONG(1, 2), MAKELONG(3, 4));
    sprintf(path, "%s\\FileName4.dll", CURR_DIR);
    res = RegSetValueExA(hklm, "Value14", 0, REG_SZ,
                         (const BYTE *)path, lstrlenA(path) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    create_file_with_version("FileName5.dll", MAKELONG(2, 1), MAKELONG(4, 3));
    sprintf(path, "%s\\FileName5.dll", CURR_DIR);
    res = RegSetValueExA(hklm, "Value15", 0, REG_SZ,
                         (const BYTE *)path, lstrlenA(path) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    sprintf(path, "\"%s\\FileName1\" -option", CURR_DIR);
    res = RegSetValueExA(hklm, "value16", 0, REG_SZ,
                         (const BYTE *)path, lstrlenA(path) + 1);

    space = (strchr(CURR_DIR, ' ')) ? TRUE : FALSE;
    sprintf(path, "%s\\FileName1 -option", CURR_DIR);
    res = RegSetValueExA(hklm, "value17", 0, REG_SZ,
                         (const BYTE *)path, lstrlenA(path) + 1);

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

    /* HKLM, msidbLocatorTypeRawValue, REG_SZ */
    str = "'NewSignature1', 2, 'Software\\Wine', 'Value1', 2";
    r = add_reglocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeRawValue, positive DWORD */
    str = "'NewSignature2', 2, 'Software\\Wine', 'Value2', 2";
    r = add_reglocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeRawValue, negative DWORD */
    str = "'NewSignature3', 2, 'Software\\Wine', 'Value3', 2";
    r = add_reglocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeRawValue, REG_EXPAND_SZ */
    str = "'NewSignature4', 2, 'Software\\Wine', 'Value4', 2";
    r = add_reglocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeRawValue, REG_EXPAND_SZ */
    str = "'NewSignature5', 2, 'Software\\Wine', 'Value5', 2";
    r = add_reglocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeRawValue, REG_MULTI_SZ */
    str = "'NewSignature6', 2, 'Software\\Wine', 'Value6', 2";
    r = add_reglocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeRawValue, REG_BINARY */
    str = "'NewSignature7', 2, 'Software\\Wine', 'Value7', 2";
    r = add_reglocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeRawValue, REG_SZ first char is # */
    str = "'NewSignature8', 2, 'Software\\Wine', 'Value8', 2";
    r = add_reglocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeFileName, signature, file exists */
    str = "'NewSignature9', 2, 'Software\\Wine', 'Value9', 1";
    r = add_reglocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeFileName, signature, file does not exist */
    str = "'NewSignature10', 2, 'Software\\Wine', 'Value10', 1";
    r = add_reglocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeFileName, no signature */
    str = "'NewSignature11', 2, 'Software\\Wine', 'Value9', 1";
    r = add_reglocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeDirectory, no signature, file exists */
    str = "'NewSignature12', 2, 'Software\\Wine', 'Value9', 0";
    r = add_reglocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeDirectory, no signature, directory exists */
    str = "'NewSignature13', 2, 'Software\\Wine', 'Value11', 0";
    r = add_reglocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeDirectory, signature, file exists */
    str = "'NewSignature14', 2, 'Software\\Wine', 'Value9', 0";
    r = add_reglocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKCR, msidbLocatorTypeRawValue, REG_SZ */
    str = "'NewSignature15', 0, 'Software\\Wine', 'Value1', 2";
    r = add_reglocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKCU, msidbLocatorTypeRawValue, REG_SZ */
    str = "'NewSignature16', 1, 'Software\\Wine', 'Value1', 2";
    r = add_reglocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKU, msidbLocatorTypeRawValue, REG_SZ */
    str = "'NewSignature17', 3, 'S-1-5-18\\Software\\Wine', 'Value1', 2";
    r = add_reglocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeRawValue, REG_SZ, NULL Name */
    str = "'NewSignature18', 2, 'Software\\Wine', '', 2";
    r = add_reglocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeRawValue, REG_SZ, key does not exist */
    str = "'NewSignature19', 2, 'Software\\IDontExist', '', 2";
    r = add_reglocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeRawValue, REG_SZ, value is empty */
    str = "'NewSignature20', 2, 'Software\\Wine', 'Value12', 2";
    r = add_reglocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeFileName, signature, file exists w/ version */
    str = "'NewSignature21', 2, 'Software\\Wine', 'Value13', 1";
    r = add_reglocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeFileName, file exists w/ version, version > max */
    str = "'NewSignature22', 2, 'Software\\Wine', 'Value14', 1";
    r = add_reglocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeFileName, file exists w/ version, sig->name ignored */
    str = "'NewSignature23', 2, 'Software\\Wine', 'Value15', 1";
    r = add_reglocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeFileName, no signature, directory exists */
    str = "'NewSignature24', 2, 'Software\\Wine', 'Value11', 1";
    r = add_reglocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeFileName, no signature, file does not exist */
    str = "'NewSignature25', 2, 'Software\\Wine', 'Value10', 1";
    r = add_reglocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeDirectory, signature, directory exists */
    str = "'NewSignature26', 2, 'Software\\Wine', 'Value11', 0";
    r = add_reglocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeDirectory, signature, file does not exist */
    str = "'NewSignature27', 2, 'Software\\Wine', 'Value10', 0";
    r = add_reglocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeDirectory, no signature, file does not exist */
    str = "'NewSignature28', 2, 'Software\\Wine', 'Value10', 0";
    r = add_reglocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeFile, file exists, in quotes */
    str = "'NewSignature29', 2, 'Software\\Wine', 'Value16', 1";
    r = add_reglocator_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* HKLM, msidbLocatorTypeFile, file exists, no quotes */
    str = "'NewSignature30', 2, 'Software\\Wine', 'Value17', 1";
    r = add_reglocator_entry(hdb, str);
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

    ptr = strrchr(CURR_DIR, '\\') + 1;
    sprintf(path, "'NewSignature26', '%s', '', '', '', '', '', '', ''", ptr);
    r = add_signature_entry(hdb, path);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    str = "'NewSignature27', 'FileName2', '', '', '', '', '', '', ''";
    r = add_signature_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    str = "'NewSignature29', 'FileName1', '', '', '', '', '', '', ''";
    r = add_signature_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    str = "'NewSignature30', 'FileName1', '', '', '', '', '', '', ''";
    r = add_signature_entry(hdb, str);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    hpkg = package_from_db(hdb);
    ok(hpkg, "Expected a valid package handle\n");

    r = MsiDoAction(hpkg, "AppSearch");
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

    ExpandEnvironmentStringsA("%PATH%", path, MAX_PATH);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP4", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

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
    sprintf(path, "%s\\FileName1", CURR_DIR);
    r = MsiGetPropertyA(hpkg, "SIGPROP9", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP10", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected \"\", got \"%s\"\n", prop);

    size = MAX_PATH;
    sprintf(path, "%s\\", CURR_DIR);
    r = MsiGetPropertyA(hpkg, "SIGPROP11", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP12", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected \"\", got \"%s\"\n", prop);

    size = MAX_PATH;
    sprintf(path, "%s\\", CURR_DIR);
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
        sprintf(path, "%s\\FileName3.dll", CURR_DIR);
        r = MsiGetPropertyA(hpkg, "SIGPROP21", prop, &size);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

        size = MAX_PATH;
        r = MsiGetPropertyA(hpkg, "SIGPROP22", prop, &size);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(prop, ""), "Expected \"\", got \"%s\"\n", prop);

        size = MAX_PATH;
        sprintf(path, "%s\\FileName5.dll", CURR_DIR);
        r = MsiGetPropertyA(hpkg, "SIGPROP23", prop, &size);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);
    }

    size = MAX_PATH;
    lstrcpyA(path, CURR_DIR);
    ptr = strrchr(path, '\\') + 1;
    *ptr = '\0';
    r = MsiGetPropertyA(hpkg, "SIGPROP24", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    sprintf(path, "%s\\", CURR_DIR);
    r = MsiGetPropertyA(hpkg, "SIGPROP25", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP26", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
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
    sprintf(path, "%s\\FileName1", CURR_DIR);
    r = MsiGetPropertyA(hpkg, "SIGPROP29", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    sprintf(path, "%s\\FileName1", CURR_DIR);
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
    CHAR path[MAX_PATH];
    CHAR prop[MAX_PATH];
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

    create_test_file("FileName1");
    sprintf(path, "%s\\FileName1", CURR_DIR);
    WritePrivateProfileStringA("Section", "Key2", path, "IniFile.ini");

    WritePrivateProfileStringA("Section", "Key3", CURR_DIR, "IniFile.ini");

    sprintf(path, "%s\\IDontExist", CURR_DIR);
    WritePrivateProfileStringA("Section", "Key4", path, "IniFile.ini");

    create_file_with_version("FileName2.dll", MAKELONG(2, 1), MAKELONG(4, 3));
    sprintf(path, "%s\\FileName2.dll", CURR_DIR);
    WritePrivateProfileStringA("Section", "Key5", path, "IniFile.ini");

    create_file_with_version("FileName3.dll", MAKELONG(1, 2), MAKELONG(3, 4));
    sprintf(path, "%s\\FileName3.dll", CURR_DIR);
    WritePrivateProfileStringA("Section", "Key6", path, "IniFile.ini");

    create_file_with_version("FileName4.dll", MAKELONG(2, 1), MAKELONG(4, 3));
    sprintf(path, "%s\\FileName4.dll", CURR_DIR);
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

    hpkg = package_from_db(hdb);
    ok(hpkg, "Expected a valid package handle\n");

    r = MsiDoAction(hpkg, "AppSearch");
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
    sprintf(path, "%s\\FileName1", CURR_DIR);
    r = MsiGetPropertyA(hpkg, "SIGPROP4", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP5", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected \"\", got \"%s\"\n", prop);

    size = MAX_PATH;
    sprintf(path, "%s\\", CURR_DIR);
    r = MsiGetPropertyA(hpkg, "SIGPROP6", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    sprintf(path, "%s\\", CURR_DIR);
    r = MsiGetPropertyA(hpkg, "SIGPROP7", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    lstrcpyA(path, CURR_DIR);
    ptr = strrchr(path, '\\');
    *(ptr + 1) = '\0';
    r = MsiGetPropertyA(hpkg, "SIGPROP8", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP9", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected \"\", got \"%s\"\n", prop);

    if (version)
    {
        size = MAX_PATH;
        sprintf(path, "%s\\FileName2.dll", CURR_DIR);
        r = MsiGetPropertyA(hpkg, "SIGPROP10", prop, &size);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

        size = MAX_PATH;
        r = MsiGetPropertyA(hpkg, "SIGPROP11", prop, &size);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(prop, ""), "Expected \"\", got \"%s\"\n", prop);

        size = MAX_PATH;
        sprintf(path, "%s\\FileName4.dll", CURR_DIR);
        r = MsiGetPropertyA(hpkg, "SIGPROP12", prop, &size);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);
    }

    delete_win_ini("IniFile.ini");
    DeleteFileA("FileName1");
    DeleteFileA("FileName2.dll");
    DeleteFileA("FileName3.dll");
    DeleteFileA("FileName4.dll");
    MsiCloseHandle(hpkg);
    DeleteFileA(msifile);
}

static void test_appsearch_drlocator(void)
{
    MSIHANDLE hpkg, hdb;
    CHAR path[MAX_PATH];
    CHAR prop[MAX_PATH];
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

    r = create_drlocator_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* no parent, full path, depth 0, signature */
    sprintf(path, "'NewSignature1', '', '%s', 0", CURR_DIR);
    r = add_drlocator_entry(hdb, path);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* no parent, full path, depth 0, no signature */
    sprintf(path, "'NewSignature2', '', '%s', 0", CURR_DIR);
    r = add_drlocator_entry(hdb, path);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* no parent, relative path, depth 0, no signature */
    sprintf(path, "'NewSignature3', '', '%s', 0", CURR_DIR + 3);
    r = add_drlocator_entry(hdb, path);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* no parent, full path, depth 2, signature */
    sprintf(path, "'NewSignature4', '', '%s', 2", CURR_DIR);
    r = add_drlocator_entry(hdb, path);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* no parent, full path, depth 3, signature */
    sprintf(path, "'NewSignature5', '', '%s', 3", CURR_DIR);
    r = add_drlocator_entry(hdb, path);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* no parent, full path, depth 1, signature is dir */
    sprintf(path, "'NewSignature6', '', '%s', 1", CURR_DIR);
    r = add_drlocator_entry(hdb, path);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* parent is in DrLocator, relative path, depth 0, signature */
    sprintf(path, "'NewSignature7', 'NewSignature1', 'one\\two\\three', 1");
    r = add_drlocator_entry(hdb, path);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* no parent, full path, depth 0, signature w/ version */
    sprintf(path, "'NewSignature8', '', '%s', 0", CURR_DIR);
    r = add_drlocator_entry(hdb, path);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* no parent, full path, depth 0, signature w/ version, ver > max */
    sprintf(path, "'NewSignature9', '', '%s', 0", CURR_DIR);
    r = add_drlocator_entry(hdb, path);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* no parent, full path, depth 0, signature w/ version, sig->name not ignored */
    sprintf(path, "'NewSignature10', '', '%s', 0", CURR_DIR);
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

    hpkg = package_from_db(hdb);
    ok(hpkg, "Expected a valid package handle\n");

    r = MsiDoAction(hpkg, "AppSearch");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    sprintf(path, "%s\\FileName1", CURR_DIR);
    r = MsiGetPropertyA(hpkg, "SIGPROP1", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    sprintf(path, "%s\\", CURR_DIR);
    r = MsiGetPropertyA(hpkg, "SIGPROP2", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    sprintf(path, "%s\\", CURR_DIR);
    r = MsiGetPropertyA(hpkg, "SIGPROP3", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP4", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected \"\", got \"%s\"\n", prop);

    size = MAX_PATH;
    sprintf(path, "%s\\one\\two\\three\\FileName2", CURR_DIR);
    r = MsiGetPropertyA(hpkg, "SIGPROP5", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "SIGPROP6", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected \"\", got \"%s\"\n", prop);

    size = MAX_PATH;
    sprintf(path, "%s\\one\\two\\three\\FileName2", CURR_DIR);
    r = MsiGetPropertyA(hpkg, "SIGPROP7", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, path), "Expected \"%s\", got \"%s\"\n", path, prop);

    if (version)
    {
        size = MAX_PATH;
        sprintf(path, "%s\\FileName3.dll", CURR_DIR);
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

    DeleteFileA("FileName1");
    DeleteFileA("FileName3.dll");
    DeleteFileA("FileName4.dll");
    DeleteFileA("FileName5.dll");
    DeleteFileA("one\\two\\three\\FileName2");
    RemoveDirectoryA("one\\two\\three");
    RemoveDirectoryA("one\\two");
    RemoveDirectoryA("one");
    RemoveDirectoryA("another");
    MsiCloseHandle(hpkg);
    DeleteFileA(msifile);
}

static void test_featureparents(void)
{
    MSIHANDLE hpkg;
    UINT r;
    MSIHANDLE hdb;
    INSTALLSTATE state, action;

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

    hpkg = package_from_db( hdb );
    ok( hpkg, "failed to create package\n");

    MsiCloseHandle( hdb );

    r = MsiDoAction( hpkg, "CostInitialize");
    ok( r == ERROR_SUCCESS, "cost init failed\n");

    r = MsiDoAction( hpkg, "FileCost");
    ok( r == ERROR_SUCCESS, "file cost failed\n");

    r = MsiDoAction( hpkg, "CostFinalize");
    ok( r == ERROR_SUCCESS, "cost finalize failed\n");

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "zodiac", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "perseus", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "orion", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "waters", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "bayer", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "leo", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "virgo", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected virgo INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_SOURCE, "Expected virgo INSTALLSTATE_SOURCE, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "libra", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected libra INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected libra INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "cassiopeia", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected cassiopeia INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected cassiopeia INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "cepheus", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected cepheus INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_SOURCE, "Expected cepheus INSTALLSTATE_SOURCE, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "andromeda", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected andromeda INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected andromeda INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "canis", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected canis INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected canis INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "monoceros", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected monoceros INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_SOURCE, "Expected monoceros INSTALLSTATE_SOURCE, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "lepus", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected lepus INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected lepus INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "delphinus", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected delphinus INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected delphinus INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "hydrus", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected hydrus INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected hydrus INSTALLSTATE_UNKNOWN, got %d\n", action);

    r = MsiSetFeatureState(hpkg, "orion", INSTALLSTATE_ABSENT);
    ok( r == ERROR_SUCCESS, "failed to set feature state: %d\n", r);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "zodiac", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected zodiac INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected zodiac INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "perseus", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected perseus INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_SOURCE, "Expected perseus INSTALLSTATE_SOURCE, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "orion", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected orion INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_ABSENT, "Expected orion INSTALLSTATE_ABSENT, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "leo", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected leo INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected leo INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "virgo", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected virgo INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_SOURCE, "Expected virgo INSTALLSTATE_SOURCE, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "libra", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected libra INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected libra INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "cassiopeia", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected cassiopeia INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected cassiopeia INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "cepheus", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected cepheus INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_SOURCE, "Expected cepheus INSTALLSTATE_SOURCE, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "andromeda", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected andromeda INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_SOURCE, "Expected andromeda INSTALLSTATE_SOURCE, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "canis", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected canis INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected canis INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "monoceros", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected monoceros INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected monoceros INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "lepus", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected lepus INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected lepus INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "delphinus", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected delphinus INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected delphinus INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "hydrus", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected hydrus INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected hydrus INSTALLSTATE_UNKNOWN, got %d\n", action);
    
    MsiCloseHandle(hpkg);
    DeleteFileA(msifile);
}

static void test_installprops(void)
{
    MSIHANDLE hpkg, hdb;
    CHAR path[MAX_PATH];
    CHAR buf[MAX_PATH];
    DWORD size, type;
    LANGID langid;
    HKEY hkey1, hkey2;
    int res;
    UINT r;

    GetCurrentDirectory(MAX_PATH, path);
    lstrcat(path, "\\");
    lstrcat(path, msifile);

    hdb = create_package_db();
    ok( hdb, "failed to create database\n");

    hpkg = package_from_db(hdb);
    ok( hpkg, "failed to create package\n");

    MsiCloseHandle(hdb);

    size = MAX_PATH;
    r = MsiGetProperty(hpkg, "DATABASE", buf, &size);
    ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
    ok( !lstrcmp(buf, path), "Expected %s, got %s\n", path, buf);

    RegOpenKey(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\MS Setup (ACME)\\User Info", &hkey1);

    RegOpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", &hkey2);

    size = MAX_PATH;
    type = REG_SZ;
    *path = '\0';
    if (RegQueryValueEx(hkey1, "DefName", NULL, &type, (LPBYTE)path, &size) != ERROR_SUCCESS)
    {
        size = MAX_PATH;
        type = REG_SZ;
        RegQueryValueEx(hkey2, "RegisteredOwner", NULL, &type, (LPBYTE)path, &size);
    }

    /* win9x doesn't set this */
    if (*path)
    {
        size = MAX_PATH;
        r = MsiGetProperty(hpkg, "USERNAME", buf, &size);
        ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
        ok( !lstrcmp(buf, path), "Expected %s, got %s\n", path, buf);
    }

    size = MAX_PATH;
    type = REG_SZ;
    *path = '\0';
    if (RegQueryValueEx(hkey1, "DefCompany", NULL, &type, (LPBYTE)path, &size) != ERROR_SUCCESS)
    {
        size = MAX_PATH;
        type = REG_SZ;
        RegQueryValueEx(hkey2, "RegisteredOrganization", NULL, &type, (LPBYTE)path, &size);
    }

    if (*path)
    {
        size = MAX_PATH;
        r = MsiGetProperty(hpkg, "COMPANYNAME", buf, &size);
        ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
        ok( !lstrcmp(buf, path), "Expected %s, got %s\n", path, buf);
    }

    size = MAX_PATH;
    r = MsiGetProperty(hpkg, "VersionDatabase", buf, &size);
    ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
    trace("VersionDatabase = %s\n", buf);

    size = MAX_PATH;
    r = MsiGetProperty(hpkg, "VersionMsi", buf, &size);
    ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
    trace("VersionMsi = %s\n", buf);

    size = MAX_PATH;
    r = MsiGetProperty(hpkg, "Date", buf, &size);
    ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
    trace("Date = %s\n", buf);

    size = MAX_PATH;
    r = MsiGetProperty(hpkg, "Time", buf, &size);
    ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
    trace("Time = %s\n", buf);

    size = MAX_PATH;
    r = MsiGetProperty(hpkg, "PackageCode", buf, &size);
    ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
    trace("PackageCode = %s\n", buf);

    langid = GetUserDefaultLangID();
    sprintf(path, "%d", langid);

    size = MAX_PATH;
    r = MsiGetProperty(hpkg, "UserLanguageID", buf, &size);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS< got %d\n", r);
    ok( !lstrcmpA(buf, path), "Expected \"%s\", got \"%s\"\n", path, buf);

    res = GetSystemMetrics(SM_CXSCREEN);
    size = MAX_PATH;
    r = MsiGetProperty(hpkg, "ScreenX", buf, &size);
    ok(atol(buf) == res, "Expected %d, got %ld\n", res, atol(buf));

    res = GetSystemMetrics(SM_CYSCREEN);
    size = MAX_PATH;
    r = MsiGetProperty(hpkg, "ScreenY", buf, &size);
    ok(atol(buf) == res, "Expected %d, got %ld\n", res, atol(buf));

    CloseHandle(hkey1);
    CloseHandle(hkey2);
    MsiCloseHandle(hpkg);
    DeleteFile(msifile);
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

    hpkg = package_from_db( hdb );
    ok( hpkg, "failed to create package\n");

    MsiCloseHandle( hdb );

    r = MsiSetProperty( hpkg, "X", "1" );
    ok( r == ERROR_SUCCESS, "failed to set property\n" );

    /* invalid conditions are ignored */
    r = MsiDoAction( hpkg, "LaunchConditions" );
    ok( r == ERROR_SUCCESS, "cost init failed\n" );

    /* verify LaunchConditions still does some verification */
    r = MsiSetProperty( hpkg, "X", "2" );
    ok( r == ERROR_SUCCESS, "failed to set property\n" );

    r = MsiDoAction( hpkg, "LaunchConditions" );
    ok( r == ERROR_INSTALL_FAILURE, "Expected ERROR_INSTALL_FAILURE, got %d\n", r );

    MsiCloseHandle( hpkg );
    DeleteFile( msifile );
}

static void test_ccpsearch(void)
{
    MSIHANDLE hdb, hpkg;
    CHAR prop[MAX_PATH];
    DWORD size = MAX_PATH;
    UINT r;

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

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

    r = add_reglocator_entry(hdb, "'CCP_random', 0, 'htmlfile\\shell\\open\\nonexistent', '', 1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = create_drlocator_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_drlocator_entry(hdb, "'RMCCP_random', '', 'C:\\', '0'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = create_signature_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    hpkg = package_from_db(hdb);
    ok(hpkg, "failed to create package\n");

    MsiCloseHandle(hdb);

    r = MsiDoAction(hpkg, "CCPSearch");
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

    hpkg = package_from_db(hdb);
    ok(hpkg, "failed to create package\n");

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

    r = MsiDoAction(hpkg, "AppSearch");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "ABELISAURUS", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    lstrcpyA(expected, CURR_DIR);
    lstrcatA(expected, "\\abelisaurus");
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
    lstrcatA(expected, "\\");
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
    lstrcatA(expected, "\\");
    ok(!lstrcmpA(prop, expected) || !lstrcmpA(prop, ""),
       "Expected %s or empty string, got %s\n", expected, prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "NEOSODON", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    lstrcpyA(expected, CURR_DIR);
    lstrcatA(expected, "\\neosodon\\");
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
    lstrcatA(cwd, "\\");

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

    hpkg = package_from_db(hdb);
    ok(hpkg, "failed to create package\n");

    MsiCloseHandle(hdb);

    /* invalid database handle */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(-1, "TARGETDIR", path, &size);
    ok(r == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* NULL szFolder */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, NULL, path, &size);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* empty szFolder */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* try TARGETDIR */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "TARGETDIR", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* try SourceDir */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* try SOURCEDIR */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* source path does not exist, but the property exists */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetProperty(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
    ok(size == 0, "Expected 0, got %d\n", size);

    /* try SubDir */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SubDir", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* try SubDir2 */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SubDir2", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    r = MsiDoAction(hpkg, "CostInitialize");
    ok(r == ERROR_SUCCESS, "cost init failed\n");

    /* try TARGETDIR after CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "TARGETDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SourceDir after CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SOURCEDIR after CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* source path does not exist, but the property exists */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetProperty(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    todo_wine
    {
        ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
        ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);
    }

    /* try SubDir after CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SubDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    /* try SubDir2 after CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SubDir2", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, sub2), "Expected \"%s\", got \"%s\"\n", sub2, path);
    ok(size == lstrlenA(sub2), "Expected %d, got %d\n", lstrlenA(sub2), size);

    r = MsiDoAction(hpkg, "ResolveSource");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    /* try TARGETDIR after ResolveSource */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "TARGETDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SourceDir after ResolveSource */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SOURCEDIR after ResolveSource */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* source path does not exist, but the property exists */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetProperty(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SubDir after ResolveSource */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SubDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    /* try SubDir2 after ResolveSource */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SubDir2", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, sub2), "Expected \"%s\", got \"%s\"\n", sub2, path);
    ok(size == lstrlenA(sub2), "Expected %d, got %d\n", lstrlenA(sub2), size);

    r = MsiDoAction(hpkg, "FileCost");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    /* try TARGETDIR after FileCost */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "TARGETDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SourceDir after FileCost */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SOURCEDIR after FileCost */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* try SubDir after FileCost */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SubDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    /* try SubDir2 after FileCost */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SubDir2", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, sub2), "Expected \"%s\", got \"%s\"\n", sub2, path);
    ok(size == lstrlenA(sub2), "Expected %d, got %d\n", lstrlenA(sub2), size);

    r = MsiDoAction(hpkg, "CostFinalize");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    /* try TARGETDIR after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "TARGETDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SourceDir after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SOURCEDIR after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* try SubDir after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SubDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    /* try SubDir2 after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SubDir2", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, sub2), "Expected \"%s\", got \"%s\"\n", sub2, path);
    ok(size == lstrlenA(sub2), "Expected %d, got %d\n", lstrlenA(sub2), size);

    /* nonexistent directory */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "IDontExist", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* NULL szPathBuf */
    size = MAX_PATH;
    r = MsiGetSourcePath(hpkg, "SourceDir", NULL, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* NULL pcchPathBuf */
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SourceDir", path, NULL);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);

    /* pcchPathBuf is 0 */
    size = 0;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* pcchPathBuf does not have room for NULL terminator */
    size = lstrlenA(cwd);
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(!strncmp(path, cwd, lstrlenA(cwd) - 1),
       "Expected path with no backslash, got \"%s\"\n", path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* pcchPathBuf has room for NULL terminator */
    size = lstrlenA(cwd) + 1;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    MsiCloseHandle(hpkg);

    /* compressed source */

    r = MsiOpenDatabase(msifile, MSIDBOPEN_DIRECT, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    set_suminfo_prop(hdb, PID_WORDCOUNT, msidbSumInfoSourceTypeCompressed);

    hpkg = package_from_db(hdb);
    ok(hpkg, "failed to create package\n");

    r = MsiDoAction(hpkg, "CostInitialize");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiDoAction(hpkg, "FileCost");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiDoAction(hpkg, "CostFinalize");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* try TARGETDIR after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "TARGETDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SourceDir after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SOURCEDIR after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SOURCEDIR", path, &size);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
        ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);
    }

    /* try SubDir after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SubDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* try SubDir2 after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SubDir2", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    MsiCloseHandle(hpkg);
    DeleteFile(msifile);
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
    lstrcatA(cwd, "\\");

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

    hpkg = package_from_db(hdb);
    ok(hpkg, "failed to create package\n");

    MsiCloseHandle(hdb);

    CreateDirectoryA("one", NULL);
    CreateDirectoryA("four", NULL);

    r = MsiDoAction(hpkg, "CostInitialize");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    CreateDirectory("five", NULL);
    CreateDirectory("eight", NULL);

    r = MsiDoAction(hpkg, "FileCost");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    CreateDirectory("nine", NULL);
    CreateDirectory("twelve", NULL);

    r = MsiDoAction(hpkg, "CostFinalize");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    /* neither short nor long source directories exist */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SubDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    CreateDirectoryA("short", NULL);

    /* short source directory exists */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SubDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    CreateDirectoryA("long", NULL);

    /* both short and long source directories exist */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SubDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    lstrcpyA(subsrc, cwd);
    lstrcatA(subsrc, "two");
    lstrcatA(subsrc, "\\");

    /* short dir exists before CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SubDir2", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    lstrcpyA(subsrc, cwd);
    lstrcatA(subsrc, "four");
    lstrcatA(subsrc, "\\");

    /* long dir exists before CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SubDir3", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    lstrcpyA(subsrc, cwd);
    lstrcatA(subsrc, "six");
    lstrcatA(subsrc, "\\");

    /* short dir exists before FileCost */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SubDir4", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    lstrcpyA(subsrc, cwd);
    lstrcatA(subsrc, "eight");
    lstrcatA(subsrc, "\\");

    /* long dir exists before FileCost */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SubDir5", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    lstrcpyA(subsrc, cwd);
    lstrcatA(subsrc, "ten");
    lstrcatA(subsrc, "\\");

    /* short dir exists before CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SubDir6", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    lstrcpyA(subsrc, cwd);
    lstrcatA(subsrc, "twelve");
    lstrcatA(subsrc, "\\");

    /* long dir exists before CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SubDir7", path, &size);
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

    r = MsiOpenDatabase(msifile, MSIDBOPEN_DIRECT, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    set_suminfo_prop(hdb, PID_WORDCOUNT, msidbSumInfoSourceTypeSFN);

    hpkg = package_from_db(hdb);
    ok(hpkg, "failed to create package\n");

    MsiCloseHandle(hdb);

    CreateDirectoryA("one", NULL);
    CreateDirectoryA("four", NULL);

    r = MsiDoAction(hpkg, "CostInitialize");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    CreateDirectory("five", NULL);
    CreateDirectory("eight", NULL);

    r = MsiDoAction(hpkg, "FileCost");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    CreateDirectory("nine", NULL);
    CreateDirectory("twelve", NULL);

    r = MsiDoAction(hpkg, "CostFinalize");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    lstrcpyA(subsrc, cwd);
    lstrcatA(subsrc, "short");
    lstrcatA(subsrc, "\\");

    /* neither short nor long source directories exist */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SubDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    CreateDirectoryA("short", NULL);

    /* short source directory exists */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SubDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    CreateDirectoryA("long", NULL);

    /* both short and long source directories exist */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SubDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    lstrcpyA(subsrc, cwd);
    lstrcatA(subsrc, "one");
    lstrcatA(subsrc, "\\");

    /* short dir exists before CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SubDir2", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    lstrcpyA(subsrc, cwd);
    lstrcatA(subsrc, "three");
    lstrcatA(subsrc, "\\");

    /* long dir exists before CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SubDir3", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    lstrcpyA(subsrc, cwd);
    lstrcatA(subsrc, "five");
    lstrcatA(subsrc, "\\");

    /* short dir exists before FileCost */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SubDir4", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    lstrcpyA(subsrc, cwd);
    lstrcatA(subsrc, "seven");
    lstrcatA(subsrc, "\\");

    /* long dir exists before FileCost */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SubDir5", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    lstrcpyA(subsrc, cwd);
    lstrcatA(subsrc, "nine");
    lstrcatA(subsrc, "\\");

    /* short dir exists before CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SubDir6", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, subsrc), "Expected \"%s\", got \"%s\"\n", subsrc, path);
    ok(size == lstrlenA(subsrc), "Expected %d, got %d\n", lstrlenA(subsrc), size);

    lstrcpyA(subsrc, cwd);
    lstrcatA(subsrc, "eleven");
    lstrcatA(subsrc, "\\");

    /* long dir exists before CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SubDir7", path, &size);
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
    CHAR package[10];
    CHAR path[MAX_PATH];
    CHAR cwd[MAX_PATH];
    CHAR subsrc[MAX_PATH];
    DWORD size;
    UINT r;

    lstrcpyA(cwd, CURR_DIR);
    lstrcatA(cwd, "\\");

    lstrcpyA(subsrc, cwd);
    lstrcatA(subsrc, "long");
    lstrcatA(subsrc, "\\");

    hdb = create_package_db();
    ok( hdb, "failed to create database\n");

    r = add_directory_entry(hdb, "'TARGETDIR', '', 'SourceDir'");
    ok(r == S_OK, "failed\n");

    sprintf(package, "#%i", hdb);
    r = MsiOpenPackage(package, &hpkg);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* properties only */

    /* SourceDir prop */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetProperty(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
    ok(size == 0, "Expected 0, got %d\n", size);

    /* SOURCEDIR prop */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetProperty(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
    ok(size == 0, "Expected 0, got %d\n", size);

    r = MsiDoAction(hpkg, "CostInitialize");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    /* SourceDir after CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetProperty(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
    ok(size == 0, "Expected 0, got %d\n", size);

    /* SOURCEDIR after CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetProperty(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
    ok(size == 0, "Expected 0, got %d\n", size);

    r = MsiDoAction(hpkg, "FileCost");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    /* SourceDir after FileCost */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetProperty(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
    ok(size == 0, "Expected 0, got %d\n", size);

    /* SOURCEDIR after FileCost */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetProperty(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
    ok(size == 0, "Expected 0, got %d\n", size);

    r = MsiDoAction(hpkg, "CostFinalize");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    /* SourceDir after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetProperty(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
    ok(size == 0, "Expected 0, got %d\n", size);

    /* SOURCEDIR after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetProperty(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
    ok(size == 0, "Expected 0, got %d\n", size);

    r = MsiDoAction(hpkg, "ResolveSource");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    /* SourceDir after ResolveSource */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetProperty(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* SOURCEDIR after ResolveSource */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetProperty(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* random casing */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetProperty(hpkg, "SoUrCeDiR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
    ok(size == 0, "Expected 0, got %d\n", size);

    MsiCloseHandle(hpkg);

    /* reset the package state */
    sprintf(package, "#%i", hdb);
    r = MsiOpenPackage(package, &hpkg);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* test how MsiGetSourcePath affects the properties */

    /* SourceDir prop */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetProperty(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    todo_wine
    {
        ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
        ok(size == 0, "Expected 0, got %d\n", size);
    }

    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* SourceDir after MsiGetSourcePath */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetProperty(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    todo_wine
    {
        ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
        ok(size == 0, "Expected 0, got %d\n", size);
    }

    /* SOURCEDIR prop */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetProperty(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    todo_wine
    {
        ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
        ok(size == 0, "Expected 0, got %d\n", size);
    }

    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* SOURCEDIR prop after MsiGetSourcePath */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetProperty(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    todo_wine
    {
        ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
        ok(size == 0, "Expected 0, got %d\n", size);
    }

    r = MsiDoAction(hpkg, "CostInitialize");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    /* SourceDir after CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetProperty(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    todo_wine
    {
        ok(!lstrcmpA(path, ""), "Expected \"\", got \"%s\"\n", path);
        ok(size == 0, "Expected 0, got %d\n", size);
    }

    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* SourceDir after MsiGetSourcePath */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetProperty(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* SOURCEDIR after CostInitialize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetProperty(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* SOURCEDIR source path still does not exist */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    r = MsiDoAction(hpkg, "FileCost");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    /* SourceDir after FileCost */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetProperty(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* SOURCEDIR after FileCost */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetProperty(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* SOURCEDIR source path still does not exist */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    r = MsiDoAction(hpkg, "CostFinalize");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    /* SourceDir after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetProperty(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* SOURCEDIR after CostFinalize */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetProperty(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* SOURCEDIR source path still does not exist */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    r = MsiDoAction(hpkg, "ResolveSource");
    ok(r == ERROR_SUCCESS, "file cost failed\n");

    /* SourceDir after ResolveSource */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetProperty(hpkg, "SourceDir", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* SOURCEDIR after ResolveSource */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetProperty(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(path, cwd), "Expected \"%s\", got \"%s\"\n", cwd, path);
    ok(size == lstrlenA(cwd), "Expected %d, got %d\n", lstrlenA(cwd), size);

    /* SOURCEDIR source path still does not exist */
    size = MAX_PATH;
    lstrcpyA(path, "kiwi");
    r = MsiGetSourcePath(hpkg, "SOURCEDIR", path, &size);
    ok(r == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", r);
    ok(!lstrcmpA(path, "kiwi"),
       "Expected path to be unchanged, got \"%s\"\n", path);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    MsiCloseHandle(hdb);
    MsiCloseHandle(hpkg);
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

            ok(lasterr == ares[idx].lasterr ||
               lasterr == 0xdeadbeef, /* win9x */
               "(%d, lasterr, %d): Expected %d, got %d\n",
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

    r = MsiOpenDatabaseA(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    test_file_access(msifile, create);

    r = MsiDatabaseCommit(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    test_file_access(msifile, create_commit);

    MsiCloseHandle(hdb);

    test_file_access(msifile, create_close);

    DeleteFileA(msifile);

    r = MsiOpenDatabaseA(msifile, MSIDBOPEN_CREATEDIRECT, &hdb);
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
    MSIHANDLE hpkg, hdb, hsuminfo;
    MSIHANDLE hview, hrec;
    MSICONDITION condition;
    CHAR buffer[MAX_PATH];
    DWORD size;
    UINT r;

    r = MsiOpenPackageA("", &hpkg);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    }

    hdb = MsiGetActiveDatabase(hpkg);
    todo_wine
    {
        ok(hdb != 0, "Expected a valid database handle\n");
    }

    r = MsiDatabaseOpenView(hdb, "SELECT * FROM `_Tables`", &hview);
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

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, buffer, &size);
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
    r = MsiRecordGetString(hrec, 1, buffer, &size);
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

    r = MsiDatabaseOpenView(hdb, "SELECT * FROM `_Property`", &hview);
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

    r = MsiDatabaseOpenView(hdb, "SELECT * FROM `#_FolderCache`", &hview);
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

    r = MsiDatabaseOpenView(hdb, "SELECT * FROM `_Streams`", &hview);
    todo_wine
    {
        ok(r == ERROR_BAD_QUERY_SYNTAX,
           "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);
    }

    r = MsiDatabaseOpenView(hdb, "SELECT * FROM `_Storages`", &hview);
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
    LPSTR usersid;
    DWORD size;
    LONG res;
    UINT r;
    SC_HANDLE scm;

    scm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        win_skip("Different registry keys on Win9x and WinMe\n");
        return;
    }
    CloseServiceHandle(scm);

    GetCurrentDirectoryA(MAX_PATH, path);
    lstrcatA(path, "\\");

    create_test_guid(prodcode, prod_squashed);
    get_user_sid(&usersid);

    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
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

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\S-1-5-18\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &userkey);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        RegDeleteKeyA(prodkey, "");
        RegCloseKey(prodkey);
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = RegCreateKeyA(userkey, "InstallProperties", &props);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    lstrcpyA(val, path);
    lstrcatA(val, "\\winetest.msi");
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
    RegDeleteKeyA(props, "");
    RegCloseKey(props);
    RegDeleteKeyA(userkey, "");
    RegCloseKey(userkey);
    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);
    DeleteFileA(msifile);
}

START_TEST(package)
{
    GetCurrentDirectoryA(MAX_PATH, CURR_DIR);

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
}
