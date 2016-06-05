/*
 * Copyright 2012 Andrew Eikum for CodeWeavers
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

#include "windows.h"
#include "winnetwk.h"
#include "wine/test.h"

static void test_WNetGetUniversalName(void)
{
    DWORD ret;
    char buffer[1024];
    DWORD drive_type, info_size, fail_size;
    char driveA[] = "A:\\";
    char driveandpathA[] = "A:\\file.txt";
    WCHAR driveW[] = {'A',':','\\',0};

    for(; *driveA <= 'Z'; ++*driveA,  ++*driveandpathA, ++*driveW){
        drive_type = GetDriveTypeW(driveW);

        info_size = sizeof(buffer);
        ret = WNetGetUniversalNameA(driveA, UNIVERSAL_NAME_INFO_LEVEL,
                buffer, &info_size);

        if(drive_type == DRIVE_REMOTE)
            ok(ret == WN_NO_ERROR, "WNetGetUniversalNameA failed: %08x\n", ret);
        else
            /* WN_NO_NET_OR_BAD_PATH (DRIVE_FIXED) returned from the virtual drive (usual Q:)
               created by the microsoft application virtualization client */
            ok((ret == WN_NOT_CONNECTED) || (ret == WN_NO_NET_OR_BAD_PATH),
                "WNetGetUniversalNameA(%s, ...) returned %u (drive_type: %u)\n",
                driveA, ret, drive_type);

        ok(info_size == sizeof(buffer), "Got wrong size: %u\n", info_size);

        fail_size = 0;
        ret = WNetGetUniversalNameA(driveA, UNIVERSAL_NAME_INFO_LEVEL,
                buffer, &fail_size);
        if(drive_type == DRIVE_REMOTE)
            todo_wine ok(ret == WN_BAD_VALUE, "WNetGetUniversalNameA failed: %08x\n", ret);
        else
            ok(ret == WN_NOT_CONNECTED || ret == WN_NO_NET_OR_BAD_PATH,
                "(%s) WNetGetUniversalNameW gave wrong error: %u\n", driveA, ret);

        fail_size = sizeof(driveA) / sizeof(char) - 1;
        ret = WNetGetUniversalNameA(driveA, UNIVERSAL_NAME_INFO_LEVEL,
                buffer, &fail_size);
        if(drive_type == DRIVE_REMOTE)
            ok(ret == WN_MORE_DATA, "WNetGetUniversalNameA failed: %08x\n", ret);

        ret = WNetGetUniversalNameA(driveandpathA, UNIVERSAL_NAME_INFO_LEVEL,
                buffer, &info_size);
        if(drive_type == DRIVE_REMOTE)
            todo_wine ok(ret == WN_NO_ERROR, "WNetGetUniversalNameA failed: %08x\n", ret);

        info_size = sizeof(buffer);
        ret = WNetGetUniversalNameW(driveW, UNIVERSAL_NAME_INFO_LEVEL,
                buffer, &info_size);

        if(drive_type == DRIVE_REMOTE)
            ok(ret == WN_NO_ERROR, "WNetGetUniversalNameW failed: %08x\n", ret);
        else
            ok((ret == WN_NOT_CONNECTED) || (ret == WN_NO_NET_OR_BAD_PATH),
                "WNetGetUniversalNameW(%s, ...) returned %u (drive_type: %u)\n",
                wine_dbgstr_w(driveW), ret, drive_type);
        if(drive_type != DRIVE_REMOTE)
            ok(info_size == sizeof(buffer), "Got wrong size: %u\n", info_size);
    }
}

static void test_WNetGetRemoteName(void)
{
    DWORD ret;
    char buffer[1024];
    DWORD drive_type, info_size, fail_size;
    char driveA[] = "A:\\";
    char driveandpathA[] = "A:\\file.txt";
    WCHAR driveW[] = {'A',':','\\',0};

    for(; *driveA <= 'Z'; ++*driveA,  ++*driveandpathA, ++*driveW){
        drive_type = GetDriveTypeW(driveW);

        info_size = sizeof(buffer);
        ret = WNetGetUniversalNameA(driveA, REMOTE_NAME_INFO_LEVEL,
                buffer, &info_size);
        if(drive_type == DRIVE_REMOTE)
            todo_wine
            ok(ret == WN_NO_ERROR, "WNetGetUniversalNameA failed: %08x\n", ret);
        else
            ok(ret == WN_NOT_CONNECTED || ret == WN_NO_NET_OR_BAD_PATH,
                "(%s) WNetGetUniversalNameA gave wrong error: %u\n", driveA, ret);
        ok(info_size == sizeof(buffer), "Got wrong size: %u\n", info_size);

        fail_size = 0;
        ret = WNetGetUniversalNameA(driveA, REMOTE_NAME_INFO_LEVEL,
                buffer, &fail_size);
        if(drive_type == DRIVE_REMOTE)
            todo_wine
            ok(ret == WN_BAD_VALUE, "WNetGetUniversalNameA failed: %08x\n", ret);
        else
            ok(ret == WN_NOT_CONNECTED || ret == WN_NO_NET_OR_BAD_PATH,
                "(%s) WNetGetUniversalNameA gave wrong error: %u\n", driveA, ret);
        ret = WNetGetUniversalNameA(driveA, REMOTE_NAME_INFO_LEVEL,
                buffer, NULL);
        todo_wine ok(ret == WN_BAD_POINTER, "WNetGetUniversalNameA failed: %08x\n", ret);

        ret = WNetGetUniversalNameA(driveA, REMOTE_NAME_INFO_LEVEL,
                NULL, &info_size);

        if(((GetVersion() & 0x8000ffff) == 0x00000004) || /* NT40 */
           (drive_type == DRIVE_REMOTE))
            todo_wine
            ok(ret == WN_BAD_POINTER, "WNetGetUniversalNameA failed: %08x\n", ret);
        else
            ok(ret == WN_NOT_CONNECTED || ret == WN_BAD_VALUE,
                "(%s) WNetGetUniversalNameA gave wrong error: %u\n", driveA, ret);

        fail_size = sizeof(driveA) / sizeof(char) - 1;
        ret = WNetGetUniversalNameA(driveA, REMOTE_NAME_INFO_LEVEL,
                buffer, &fail_size);
        if(drive_type == DRIVE_REMOTE)
            todo_wine ok(ret == WN_MORE_DATA, "WNetGetUniversalNameA failed: %08x\n", ret);

        ret = WNetGetUniversalNameA(driveandpathA, REMOTE_NAME_INFO_LEVEL,
                buffer, &info_size);
        if(drive_type == DRIVE_REMOTE)
          todo_wine ok(ret == WN_NO_ERROR, "WNetGetUniversalNameA failed: %08x\n", ret);

        info_size = sizeof(buffer);
        ret = WNetGetUniversalNameW(driveW, REMOTE_NAME_INFO_LEVEL,
                buffer, &info_size);
        todo_wine{
        if(drive_type == DRIVE_REMOTE)
            ok(ret == WN_NO_ERROR, "WNetGetUniversalNameW failed: %08x\n", ret);
        else
            ok(ret == WN_NOT_CONNECTED || ret == WN_NO_NET_OR_BAD_PATH,
                "(%s) WNetGetUniversalNameW gave wrong error: %u\n", driveA, ret);
        }
        ok(info_size == sizeof(buffer), "Got wrong size: %u\n", info_size);
    }
}

static DWORD (WINAPI *pWNetCachePassword)( LPSTR, WORD, LPSTR, WORD, BYTE, WORD );
static DWORD (WINAPI *pWNetGetCachedPassword)( LPSTR, WORD, LPSTR, LPWORD, BYTE );
static UINT (WINAPI *pWNetEnumCachedPasswords)( LPSTR, WORD, BYTE, ENUMPASSWORDPROC, DWORD);
static UINT (WINAPI *pWNetRemoveCachedPassword)( LPSTR, WORD, BYTE );
static DWORD (WINAPI *pWNetUseConnectionA)( HWND, LPNETRESOURCEA, LPCSTR, LPCSTR, DWORD, LPSTR, LPDWORD, LPDWORD );

#define MPR_GET_PROC(func) \
    p ## func = (void*)GetProcAddress(hmpr, #func)

static void InitFunctionPtrs(void)
{
    HMODULE hmpr = GetModuleHandleA("mpr.dll");

    MPR_GET_PROC(WNetCachePassword);
    MPR_GET_PROC(WNetGetCachedPassword);
    MPR_GET_PROC(WNetEnumCachedPasswords);
    MPR_GET_PROC(WNetRemoveCachedPassword);
    MPR_GET_PROC(WNetUseConnectionA);
}

static const char* m_resource = "wine-test-resource";
static const char* m_password = "wine-test-password";
static const BYTE m_type = 1;
static const DWORD m_param = 8;
static BOOL m_callback_reached;

static BOOL CALLBACK enum_password_proc(PASSWORD_CACHE_ENTRY* pce, DWORD param)
{
    WORD size = 0;
    char* buf;

    ok(param == m_param, "param, got %d, got %d\n", param, m_param);

    size = offsetof( PASSWORD_CACHE_ENTRY, abResource[pce->cbResource + pce->cbPassword] );
    ok(pce->cbEntry == size, "cbEntry, got %d, expected %d\n", pce->cbEntry, size);
    ok(pce->cbResource == strlen(m_resource), "cbResource, got %d\n", pce->cbResource);
    ok(pce->cbPassword == strlen(m_password), "cbPassword, got %d\n", pce->cbPassword);
    ok(pce->iEntry == 0, "iEntry, got %d, got %d\n", pce->iEntry, 0);
    ok(pce->nType == m_type, "nType, got %d, got %d\n", pce->nType, m_type);

    buf = (char*)pce->abResource;
    ok(strncmp(buf, m_resource, pce->cbResource)==0, "enumerated resource differs, got %.*s, expected %s\n", pce->cbResource, buf, m_resource);

    buf += pce->cbResource;
    ok(strncmp(buf, m_password, pce->cbPassword)==0, "enumerated resource differs, got %.*s, expected %s\n", pce->cbPassword, buf, m_password);

    m_callback_reached = 1;
    return TRUE;
}

static void test_WNetCachePassword(void)
{
    char resource_buf[32];
    char password_buf[32];
    char prefix_buf[32];
    WORD resource_len;
    WORD password_len;
    WORD prefix_len;
    DWORD ret;

    InitFunctionPtrs();

    if (pWNetCachePassword &&
        pWNetGetCachedPassword &&
        pWNetEnumCachedPasswords &&
        pWNetRemoveCachedPassword)
    {
        strcpy(resource_buf, m_resource);
        resource_len = strlen(m_resource);
        strcpy(password_buf, m_password);
        password_len = strlen(m_password);
        ret = pWNetCachePassword(resource_buf, resource_len, password_buf, password_len, m_type, 0);
        ok(ret == WN_SUCCESS, "WNetCachePassword failed: got %d, expected %d\n", ret, WN_SUCCESS);

        strcpy(resource_buf, m_resource);
        resource_len = strlen(m_resource);
        strcpy(password_buf, "------");
        password_len = sizeof(password_buf);
        ret = pWNetGetCachedPassword(resource_buf, resource_len, password_buf, &password_len, m_type);
        ok(ret == WN_SUCCESS, "WNetGetCachedPassword failed: got %d, expected %d\n", ret, WN_SUCCESS);
        ok(password_len == strlen(m_password), "password length different, got %d\n", password_len);
        ok(strncmp(password_buf, m_password, password_len)==0, "passwords different, got %.*s, expected %s\n", password_len, password_buf, m_password);

        prefix_len = 9;
        strcpy(prefix_buf, m_resource);
        prefix_buf[prefix_len] = '0';
        ret = pWNetEnumCachedPasswords(prefix_buf, prefix_len, m_type, enum_password_proc, m_param);
        ok(ret == WN_SUCCESS, "WNetEnumCachedPasswords failed: got %d, expected %d\n", ret, WN_SUCCESS);
        ok(m_callback_reached == 1, "callback was not reached\n");

        strcpy(resource_buf, m_resource);
        resource_len = strlen(m_resource);
        ret = pWNetRemoveCachedPassword(resource_buf, resource_len, m_type);
        ok(ret == WN_SUCCESS, "WNetRemoveCachedPassword failed: got %d, expected %d\n", ret, WN_SUCCESS);
    } else {
        win_skip("WNetCachePassword() is not supported.\n");
    }
}

static void test_WNetUseConnection(void)
{
    DWORD ret;
    DWORD bufSize;
    DWORD outRes;
    LPNETRESOURCEA netRes;
    CHAR outBuf[4];

    if (pWNetUseConnectionA)
    {
        netRes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(NETRESOURCEA) + sizeof("\\\\127.0.0.1\\c$") + sizeof("J:"));
        netRes->dwType = RESOURCETYPE_DISK;
        netRes->dwDisplayType = RESOURCEDISPLAYTYPE_SHARE;
        netRes->dwUsage = RESOURCEUSAGE_CONNECTABLE;
        netRes->lpLocalName = (LPSTR)((LPBYTE)netRes + sizeof(NETRESOURCEA));
        netRes->lpRemoteName = (LPSTR)((LPBYTE)netRes + sizeof(NETRESOURCEA) + sizeof("J:"));
        strcpy(netRes->lpLocalName, "J:");
        strcpy(netRes->lpRemoteName, "\\\\127.0.0.1\\c$");
        bufSize = 0;
        ret = pWNetUseConnectionA(NULL, netRes, NULL, NULL, 0, NULL, &bufSize, &outRes);
        todo_wine
        ok(ret == WN_SUCCESS, "Unexpected return: %u\n", ret);
        ok(bufSize == 0, "Unexpected buffer size: %u\n", bufSize);
        if (ret == WN_SUCCESS)
            WNetCancelConnectionA("J:", TRUE);
        bufSize = 0;
        ret = pWNetUseConnectionA(NULL, netRes, NULL, NULL, 0, outBuf, &bufSize, &outRes);
        todo_wine
        ok(ret == ERROR_INVALID_PARAMETER, "Unexpected return: %u\n", ret);
        ok(bufSize == 0, "Unexpected buffer size: %u\n", bufSize);
        if (ret == WN_SUCCESS)
            WNetCancelConnectionA("J:", TRUE);
        bufSize = 1;
        todo_wine {
        ret = pWNetUseConnectionA(NULL, netRes, NULL, NULL, 0, outBuf, &bufSize, &outRes);
        ok(ret == ERROR_MORE_DATA, "Unexpected return: %u\n", ret);
        ok(bufSize == 3, "Unexpected buffer size: %u\n", bufSize);
        if (ret == WN_SUCCESS)
            WNetCancelConnectionA("J:", TRUE);
        bufSize = 4;
        ret = pWNetUseConnectionA(NULL, netRes, NULL, NULL, 0, outBuf, &bufSize, &outRes);
        ok(ret == WN_SUCCESS, "Unexpected return: %u\n", ret);
        }
        ok(bufSize == 4, "Unexpected buffer size: %u\n", bufSize);
        if (ret == WN_SUCCESS)
            WNetCancelConnectionA("J:", TRUE);
        HeapFree(GetProcessHeap(), 0, netRes);
    } else {
        win_skip("WNetUseConnection() is not supported.\n");
    }
}

START_TEST(mpr)
{
    test_WNetGetUniversalName();
    test_WNetGetRemoteName();
    test_WNetCachePassword();
    test_WNetUseConnection();
}
