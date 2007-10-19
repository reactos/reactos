/*
 * Unit tests for shell32 SHGet{Special}Folder{Path|Location} functions.
 *
 * Copyright 2004 Juan Lang
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
 * This is a test program for the SHGet{Special}Folder{Path|Location} functions
 * of shell32, that get either a filesytem path or a LPITEMIDLIST (shell
 * namespace) path for a given folder (CSIDL value).
 *
 * FIXME:
 * - Need to verify on more systems.
 */

#define COBJMACROS

#include <stdarg.h>
#include <stdio.h>
#include "windef.h"
#include "winbase.h"
#include "shlguid.h"
#include "shlobj.h"
#include "shlwapi.h"
#include "wine/test.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) ( sizeof(x) / sizeof((x)[0]) )
#endif

/* from pidl.h, not included here: */
#ifndef PT_GUID
#define PT_GUID       0x1f /* no path */
#endif
#ifndef PT_DRIVE
#define PT_DRIVE      0x23 /* has path */
#endif
#ifndef PT_DRIVE2
#define PT_DRIVE2     0x25 /* has path */
#endif
#ifndef PT_SHELLEXT
#define PT_SHELLEXT   0x2e /* no path */
#endif
#ifndef PT_FOLDER
#define PT_FOLDER     0x31 /* has path */
#endif
#ifndef PT_WORKGRP
#define PT_WORKGRP    0x41 /* no path */
#endif
#ifndef PT_YAGUID
#define PT_YAGUID     0x70 /* no path */
#endif
/* FIXME: this is used for history/favorites folders; what's a better name? */
#ifndef PT_IESPECIAL2
#define PT_IESPECIAL2 0xb1 /* has path */
#endif

static GUID CLSID_CommonDocuments = { 0x0000000c, 0x0000, 0x0000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x74, 0x1a } };

struct shellExpectedValues {
    int  folder;
    BYTE pidlType;
};

static HRESULT (WINAPI *pDllGetVersion)(DLLVERSIONINFO *);
static HRESULT (WINAPI *pSHGetFolderPathA)(HWND, int, HANDLE, DWORD, LPSTR);
static HRESULT (WINAPI *pSHGetFolderLocation)(HWND, int, HANDLE, DWORD,
 LPITEMIDLIST *);
static BOOL    (WINAPI *pSHGetSpecialFolderPathA)(HWND, LPSTR, int, BOOL);
static HRESULT (WINAPI *pSHGetSpecialFolderLocation)(HWND, int, LPITEMIDLIST *);
static LPITEMIDLIST (WINAPI *pILFindLastID)(LPCITEMIDLIST);
static int (WINAPI *pSHFileOperationA)(LPSHFILEOPSTRUCTA);
static HRESULT (WINAPI *pSHGetMalloc)(LPMALLOC *);
static DLLVERSIONINFO shellVersion = { 0 };
static LPMALLOC pMalloc;
static const struct shellExpectedValues requiredShellValues[] = {
 { CSIDL_BITBUCKET, PT_GUID },
 { CSIDL_CONTROLS, PT_SHELLEXT },
 { CSIDL_COOKIES, PT_FOLDER },
 { CSIDL_DESKTOPDIRECTORY, PT_FOLDER },
 { CSIDL_DRIVES, PT_GUID },
 { CSIDL_FAVORITES, PT_FOLDER },
 { CSIDL_FONTS, PT_FOLDER },
/* FIXME: the following fails in Wine, returns type PT_FOLDER
 { CSIDL_HISTORY, PT_IESPECIAL2 },
 */
 { CSIDL_INTERNET, PT_GUID },
 { CSIDL_NETHOOD, PT_FOLDER },
 { CSIDL_NETWORK, PT_GUID },
 { CSIDL_PRINTERS, PT_YAGUID },
 { CSIDL_PRINTHOOD, PT_FOLDER },
 { CSIDL_PROGRAMS, PT_FOLDER },
 { CSIDL_RECENT, PT_FOLDER },
 { CSIDL_SENDTO, PT_FOLDER },
 { CSIDL_STARTMENU, PT_FOLDER },
 { CSIDL_STARTUP, PT_FOLDER },
 { CSIDL_TEMPLATES, PT_FOLDER },
};
static const struct shellExpectedValues optionalShellValues[] = {
/* FIXME: the following only semi-succeed; they return NULL PIDLs on XP.. hmm.
 { CSIDL_ALTSTARTUP, PT_FOLDER },
 { CSIDL_COMMON_ALTSTARTUP, PT_FOLDER },
 { CSIDL_COMMON_OEM_LINKS, PT_FOLDER },
 */
/* Windows NT-only: */
 { CSIDL_COMMON_DESKTOPDIRECTORY, PT_FOLDER },
 { CSIDL_COMMON_DOCUMENTS, PT_SHELLEXT },
 { CSIDL_COMMON_FAVORITES, PT_FOLDER },
 { CSIDL_COMMON_PROGRAMS, PT_FOLDER },
 { CSIDL_COMMON_STARTMENU, PT_FOLDER },
 { CSIDL_COMMON_STARTUP, PT_FOLDER },
 { CSIDL_COMMON_TEMPLATES, PT_FOLDER },
/* first appearing in shell32 version 4.71: */
 { CSIDL_APPDATA, PT_FOLDER },
/* first appearing in shell32 version 4.72: */
 { CSIDL_INTERNET_CACHE, PT_IESPECIAL2 },
/* first appearing in shell32 version 5.0: */
 { CSIDL_ADMINTOOLS, PT_FOLDER },
 { CSIDL_COMMON_APPDATA, PT_FOLDER },
 { CSIDL_LOCAL_APPDATA, PT_FOLDER },
 { CSIDL_MYDOCUMENTS, PT_FOLDER },
 { CSIDL_MYMUSIC, PT_FOLDER },
 { CSIDL_MYPICTURES, PT_FOLDER },
 { CSIDL_MYVIDEO, PT_FOLDER },
 { CSIDL_PROFILE, PT_FOLDER },
 { CSIDL_PROGRAM_FILES, PT_FOLDER },
 { CSIDL_PROGRAM_FILESX86, PT_FOLDER },
 { CSIDL_PROGRAM_FILES_COMMON, PT_FOLDER },
 { CSIDL_PROGRAM_FILES_COMMONX86, PT_FOLDER },
 { CSIDL_SYSTEM, PT_FOLDER },
 { CSIDL_WINDOWS, PT_FOLDER },
/* first appearing in shell32 6.0: */
 { CSIDL_CDBURN_AREA, PT_FOLDER },
 { CSIDL_COMMON_MUSIC, PT_FOLDER },
 { CSIDL_COMMON_PICTURES, PT_FOLDER },
 { CSIDL_COMMON_VIDEO, PT_FOLDER },
 { CSIDL_COMPUTERSNEARME, PT_WORKGRP },
 { CSIDL_RESOURCES, PT_FOLDER },
 { CSIDL_RESOURCES_LOCALIZED, PT_FOLDER },
};

static void loadShell32(void)
{
    HMODULE hShell32 = GetModuleHandleA("shell32");

#define GET_PROC(func) \
    p ## func = (void*)GetProcAddress(hShell32, #func); \
    if(!p ## func) \
      trace("GetProcAddress(%s) failed\n", #func);

    GET_PROC(DllGetVersion)
    GET_PROC(SHGetFolderPathA)
    GET_PROC(SHGetFolderLocation)
    GET_PROC(SHGetSpecialFolderPathA)
    GET_PROC(SHGetSpecialFolderLocation)
    GET_PROC(ILFindLastID)
    if (!pILFindLastID)
        pILFindLastID = (void *)GetProcAddress(hShell32, (LPCSTR)16);
    GET_PROC(SHFileOperationA)
    GET_PROC(SHGetMalloc)

    ok(pSHGetMalloc != NULL, "shell32 is missing SHGetMalloc\n");
    if (pSHGetMalloc)
    {
        HRESULT hr = pSHGetMalloc(&pMalloc);

        ok(SUCCEEDED(hr), "SHGetMalloc failed: 0x%08x\n", hr);
        ok(pMalloc != NULL, "SHGetMalloc returned a NULL IMalloc\n");
    }

    if (pDllGetVersion)
    {
        shellVersion.cbSize = sizeof(shellVersion);
        pDllGetVersion(&shellVersion);
        if (winetest_interactive)
            printf("shell32 version is %d.%d\n",
             shellVersion.dwMajorVersion, shellVersion.dwMinorVersion);
    }
#undef GET_PROC
}

#ifndef CSIDL_PROFILES
#define CSIDL_PROFILES		0x003e
#endif

/* CSIDL_MYDOCUMENTS is now the same as CSIDL_PERSONAL, but what we want
 * here is its original value.
 */
#define OLD_CSIDL_MYDOCUMENTS  0x000c

/* A couple utility printing functions */
static const char *getFolderName(int folder)
{
    static char unknown[32];

#define CSIDL_TO_STR(x) case x: return#x;
    switch (folder)
    {
    CSIDL_TO_STR(CSIDL_DESKTOP);
    CSIDL_TO_STR(CSIDL_INTERNET);
    CSIDL_TO_STR(CSIDL_PROGRAMS);
    CSIDL_TO_STR(CSIDL_CONTROLS);
    CSIDL_TO_STR(CSIDL_PRINTERS);
    CSIDL_TO_STR(CSIDL_PERSONAL);
    CSIDL_TO_STR(CSIDL_FAVORITES);
    CSIDL_TO_STR(CSIDL_STARTUP);
    CSIDL_TO_STR(CSIDL_RECENT);
    CSIDL_TO_STR(CSIDL_SENDTO);
    CSIDL_TO_STR(CSIDL_BITBUCKET);
    CSIDL_TO_STR(CSIDL_STARTMENU);
    CSIDL_TO_STR(OLD_CSIDL_MYDOCUMENTS);
    CSIDL_TO_STR(CSIDL_MYMUSIC);
    CSIDL_TO_STR(CSIDL_MYVIDEO);
    CSIDL_TO_STR(CSIDL_DESKTOPDIRECTORY);
    CSIDL_TO_STR(CSIDL_DRIVES);
    CSIDL_TO_STR(CSIDL_NETWORK);
    CSIDL_TO_STR(CSIDL_NETHOOD);
    CSIDL_TO_STR(CSIDL_FONTS);
    CSIDL_TO_STR(CSIDL_TEMPLATES);
    CSIDL_TO_STR(CSIDL_COMMON_STARTMENU);
    CSIDL_TO_STR(CSIDL_COMMON_PROGRAMS);
    CSIDL_TO_STR(CSIDL_COMMON_STARTUP);
    CSIDL_TO_STR(CSIDL_COMMON_DESKTOPDIRECTORY);
    CSIDL_TO_STR(CSIDL_APPDATA);
    CSIDL_TO_STR(CSIDL_PRINTHOOD);
    CSIDL_TO_STR(CSIDL_LOCAL_APPDATA);
    CSIDL_TO_STR(CSIDL_ALTSTARTUP);
    CSIDL_TO_STR(CSIDL_COMMON_ALTSTARTUP);
    CSIDL_TO_STR(CSIDL_COMMON_FAVORITES);
    CSIDL_TO_STR(CSIDL_INTERNET_CACHE);
    CSIDL_TO_STR(CSIDL_COOKIES);
    CSIDL_TO_STR(CSIDL_HISTORY);
    CSIDL_TO_STR(CSIDL_COMMON_APPDATA);
    CSIDL_TO_STR(CSIDL_WINDOWS);
    CSIDL_TO_STR(CSIDL_SYSTEM);
    CSIDL_TO_STR(CSIDL_PROGRAM_FILES);
    CSIDL_TO_STR(CSIDL_MYPICTURES);
    CSIDL_TO_STR(CSIDL_PROFILE);
    CSIDL_TO_STR(CSIDL_SYSTEMX86);
    CSIDL_TO_STR(CSIDL_PROGRAM_FILESX86);
    CSIDL_TO_STR(CSIDL_PROGRAM_FILES_COMMON);
    CSIDL_TO_STR(CSIDL_PROGRAM_FILES_COMMONX86);
    CSIDL_TO_STR(CSIDL_COMMON_TEMPLATES);
    CSIDL_TO_STR(CSIDL_COMMON_DOCUMENTS);
    CSIDL_TO_STR(CSIDL_COMMON_ADMINTOOLS);
    CSIDL_TO_STR(CSIDL_ADMINTOOLS);
    CSIDL_TO_STR(CSIDL_CONNECTIONS);
    CSIDL_TO_STR(CSIDL_PROFILES);
    CSIDL_TO_STR(CSIDL_COMMON_MUSIC);
    CSIDL_TO_STR(CSIDL_COMMON_PICTURES);
    CSIDL_TO_STR(CSIDL_COMMON_VIDEO);
    CSIDL_TO_STR(CSIDL_RESOURCES);
    CSIDL_TO_STR(CSIDL_RESOURCES_LOCALIZED);
    CSIDL_TO_STR(CSIDL_COMMON_OEM_LINKS);
    CSIDL_TO_STR(CSIDL_CDBURN_AREA);
    CSIDL_TO_STR(CSIDL_COMPUTERSNEARME);
#undef CSIDL_TO_STR
    default:
        sprintf(unknown, "unknown (0x%04x)", folder);
        return unknown;
    }
}

static const char *printGUID(const GUID *guid)
{
    static char guidSTR[39];

    if (!guid) return NULL;

    sprintf(guidSTR, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
     guid->Data1, guid->Data2, guid->Data3,
     guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
     guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
    return guidSTR;
}

static void testSHGetFolderLocationInvalidArgs(void)
{
    LPITEMIDLIST pidl;
    HRESULT hr;

    if (!pSHGetFolderLocation) return;

    /* check a bogus CSIDL: */
    pidl = NULL;
    hr = pSHGetFolderLocation(NULL, 0xeeee, NULL, 0, &pidl);
    ok(hr == E_INVALIDARG,
     "SHGetFolderLocation(NULL, 0xeeee, NULL, 0, &pidl)\n"
     "returned 0x%08x, expected E_INVALIDARG\n", hr);
    if (SUCCEEDED(hr))
        IMalloc_Free(pMalloc, pidl);
    /* check a bogus user token: */
    pidl = NULL;
    hr = pSHGetFolderLocation(NULL, CSIDL_FAVORITES, (HANDLE)2, 0, &pidl);
    ok(hr == E_FAIL,
     "SHGetFolderLocation(NULL, CSIDL_FAVORITES, 2, 0, &pidl)\n"
     "returned 0x%08x, expected E_FAIL\n", hr);
    if (SUCCEEDED(hr))
        IMalloc_Free(pMalloc, pidl);
    /* check reserved is not zero: */
    pidl = NULL;
    hr = pSHGetFolderLocation(NULL, CSIDL_DESKTOP, NULL, 1, &pidl);
    ok(hr == E_INVALIDARG,
     "SHGetFolderLocation(NULL, CSIDL_DESKTOP, NULL, 1, &pidl)\n"
     "returned 0x%08x, expected E_INVALIDARG\n", hr);
    if (SUCCEEDED(hr))
        IMalloc_Free(pMalloc, pidl);
    /* a NULL pidl pointer crashes, so don't test it */
}

static void testSHGetSpecialFolderLocationInvalidArgs(void)
{
    LPITEMIDLIST pidl = NULL;
    HRESULT hr;

    if (!pSHGetSpecialFolderLocation) return;

    /* SHGetSpecialFolderLocation(NULL, 0, NULL) crashes */
    hr = pSHGetSpecialFolderLocation(NULL, 0xeeee, &pidl);
    ok(hr == E_INVALIDARG,
     "SHGetSpecialFolderLocation(NULL, 0xeeee, &pidl) returned 0x%08x, "
     "expected E_INVALIDARG\n", hr);
}

static void testSHGetFolderPathInvalidArgs(void)
{
    char path[MAX_PATH];
    HRESULT hr;

    if (!pSHGetFolderPathA) return;

    /* expect 2's a bogus handle, especially since we didn't open it */
    hr = pSHGetFolderPathA(NULL, CSIDL_DESKTOP, (HANDLE)2,
     SHGFP_TYPE_DEFAULT, path);
    ok(hr == E_FAIL,
     "SHGetFolderPathA(NULL, CSIDL_DESKTOP, 2, SHGFP_TYPE_DEFAULT, path)\n"
     "returned 0x%08x, expected E_FAIL\n", hr);
    hr = pSHGetFolderPathA(NULL, 0xeeee, NULL, SHGFP_TYPE_DEFAULT, path);
    ok(hr == E_INVALIDARG,
     "SHGetFolderPathA(NULL, 0xeeee, NULL, SHGFP_TYPE_DEFAULT, path)\n"
     "returned 0x%08x, expected E_INVALIDARG\n", hr);
}

static void testSHGetSpecialFolderPathInvalidArgs(void)
{
    char path[MAX_PATH];
    BOOL ret;

    if (!pSHGetSpecialFolderPathA) return;

#if 0
    ret = pSHGetSpecialFolderPathA(NULL, NULL, CSIDL_BITBUCKET, FALSE);
    ok(!ret,
     "SHGetSpecialFolderPathA(NULL, NULL, CSIDL_BITBUCKET, FALSE)\n"
     "returned TRUE, expected FALSE\n");
#endif
    /* odd but true: calling with a NULL path still succeeds if it's a real
     * dir (on some windows platform).  on winME it generates exception.
     */
    ret = pSHGetSpecialFolderPathA(NULL, path, CSIDL_PROGRAMS, FALSE);
    ok(ret,
     "SHGetSpecialFolderPathA(NULL, path, CSIDL_PROGRAMS, FALSE)\n"
     "returned FALSE, expected TRUE\n");
    ret = pSHGetSpecialFolderPathA(NULL, path, 0xeeee, FALSE);
    ok(!ret,
     "SHGetSpecialFolderPathA(NULL, path, 0xeeee, FALSE)\n"
     "returned TRUE, expected FALSE\n");
}

static void testApiParameters(void)
{
    testSHGetFolderLocationInvalidArgs();
    testSHGetSpecialFolderLocationInvalidArgs();
    testSHGetFolderPathInvalidArgs();
    testSHGetSpecialFolderPathInvalidArgs();
}

/* Returns the folder's PIDL type, or 0xff if one can't be found. */
static BYTE testSHGetFolderLocation(BOOL optional, int folder)
{
    LPITEMIDLIST pidl;
    HRESULT hr;
    BYTE ret = 0xff;

    /* treat absence of function as success */
    if (!pSHGetFolderLocation) return TRUE;

    pidl = NULL;
    hr = pSHGetFolderLocation(NULL, folder, NULL, 0, &pidl);
    ok(SUCCEEDED(hr) || optional,
     "SHGetFolderLocation(NULL, %s, NULL, 0, &pidl)\n"
     "failed: 0x%08x\n", getFolderName(folder), hr);
    if (SUCCEEDED(hr))
    {
        ok(pidl != NULL,
         "SHGetFolderLocation(NULL, %s, NULL, 0, &pidl)\n"
         "succeeded, but returned pidl is NULL\n", getFolderName(folder));
        if (pidl)
        {
            LPITEMIDLIST pidlLast = pILFindLastID(pidl);

            ok(pidlLast != NULL, "%s: ILFindLastID failed\n",
             getFolderName(folder));
            if (pidlLast)
                ret = pidlLast->mkid.abID[0];
            IMalloc_Free(pMalloc, pidl);
        }
    }
    return ret;
}

/* Returns the folder's PIDL type, or 0xff if one can't be found. */
static BYTE testSHGetSpecialFolderLocation(BOOL optional, int folder)
{
    LPITEMIDLIST pidl;
    HRESULT hr;
    BYTE ret = 0xff;

    /* treat absence of function as success */
    if (!pSHGetSpecialFolderLocation) return TRUE;

    pidl = NULL;
    hr = pSHGetSpecialFolderLocation(NULL, folder, &pidl);
    ok(SUCCEEDED(hr) || optional,
     "SHGetSpecialFolderLocation(NULL, %s, &pidl)\n"
     "failed: 0x%08x\n", getFolderName(folder), hr);
    if (SUCCEEDED(hr))
    {
        ok(pidl != NULL,
         "SHGetSpecialFolderLocation(NULL, %s, &pidl)\n"
         "succeeded, but returned pidl is NULL\n", getFolderName(folder));
        if (pidl)
        {
            LPITEMIDLIST pidlLast = pILFindLastID(pidl);

            ok(pidlLast != NULL,
                "%s: ILFindLastID failed\n", getFolderName(folder));
            if (pidlLast)
                ret = pidlLast->mkid.abID[0];
            IMalloc_Free(pMalloc, pidl);
        }
    }
    return ret;
}

static void testSHGetFolderPath(BOOL optional, int folder)
{
    char path[MAX_PATH];
    HRESULT hr;

    if (!pSHGetFolderPathA) return;

    hr = pSHGetFolderPathA(NULL, folder, NULL, SHGFP_TYPE_CURRENT, path);
    ok(SUCCEEDED(hr) || optional,
     "SHGetFolderPathA(NULL, %s, NULL, SHGFP_TYPE_CURRENT, path)\n"
     "failed: 0x%08x\n", getFolderName(folder), hr);
}

static void testSHGetSpecialFolderPath(BOOL optional, int folder)
{
    char path[MAX_PATH];
    BOOL ret;

    if (!pSHGetSpecialFolderPathA) return;

    ret = pSHGetSpecialFolderPathA(NULL, path, folder, FALSE);
    if (ret && winetest_interactive)
        printf("%s: %s\n", getFolderName(folder), path);
    ok(ret || optional,
     "SHGetSpecialFolderPathA(NULL, path, %s, FALSE) failed\n",
     getFolderName(folder));
}

static void testShellValues(const struct shellExpectedValues testEntries[],
 int numEntries, BOOL optional)
{
    int i;

    for (i = 0; i < numEntries; i++)
    {
        BYTE type;

        type = testSHGetFolderLocation(optional, testEntries[i].folder);
        ok(type == testEntries[i].pidlType || optional,
         "%s has type %d (0x%02x), expected %d (0x%02x)\n",
         getFolderName(testEntries[i].folder), type, type,
         testEntries[i].pidlType, testEntries[i].pidlType);
        type = testSHGetSpecialFolderLocation(optional, testEntries[i].folder);
        ok(type == testEntries[i].pidlType || optional,
         "%s has type %d (0x%02x), expected %d (0x%02x)\n",
         getFolderName(testEntries[i].folder), type, type,
         testEntries[i].pidlType, testEntries[i].pidlType);
        switch (type)
        {
            case PT_FOLDER:
            case PT_DRIVE:
            case PT_DRIVE2:
            case PT_IESPECIAL2:
                testSHGetFolderPath(optional, testEntries[i].folder);
                testSHGetSpecialFolderPath(optional, testEntries[i].folder);
                break;
        }
    }
}

/* Attempts to verify that the folder path corresponding to the folder CSIDL
 * value has the same value as the environment variable with name envVar.
 * Doesn't mind if SHGetSpecialFolderPath fails for folder or if envVar isn't
 * set in this environment; different OS and shell version behave differently.
 * However, if both are present, fails if envVar's value is not the same
 * (byte-for-byte) as what SHGetSpecialFolderPath returns.
 */
static void matchSpecialFolderPathToEnv(int folder, const char *envVar)
{
    char path[MAX_PATH];

    if (!pSHGetSpecialFolderPathA) return;

    if (pSHGetSpecialFolderPathA(NULL, path, folder, FALSE))
    {
        char *envVal = getenv(envVar);

        ok(!envVal || !lstrcmpiA(envVal, path),
         "%%%s%% does not match SHGetSpecialFolderPath:\n"
         "%%%s%% is %s\nSHGetSpecialFolderPath returns %s\n",
         envVar, envVar, envVal, path);
    }
}

/* Attempts to match the GUID returned by SHGetFolderLocation for folder with
 * GUID.  Assumes the type of the returned PIDL is in fact a GUID, but doesn't
 * fail if it isn't--that check should already have been done.
 * Fails if the returned PIDL is a GUID whose value does not match guid.
 */
static void matchGUID(int folder, const GUID *guid)
{
    LPITEMIDLIST pidl;
    HRESULT hr;

    if (!pSHGetFolderLocation) return;
    if (!guid) return;

    pidl = NULL;
    hr = pSHGetFolderLocation(NULL, folder, NULL, 0, &pidl);
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidlLast = pILFindLastID(pidl);

        if (pidlLast && (pidlLast->mkid.abID[0] == PT_SHELLEXT ||
         pidlLast->mkid.abID[0] == PT_GUID))
        {
            GUID *shellGuid = (GUID *)(pidlLast->mkid.abID + 2);

            ok(IsEqualIID(shellGuid, guid),
             "%s: got GUID %s, expected %s\n", getFolderName(folder),
             printGUID(shellGuid), printGUID(guid));
        }
        IMalloc_Free(pMalloc, pidl);
    }
}

static void testDesktop(void)
{
    testSHGetFolderPath(FALSE, CSIDL_DESKTOP);
    testSHGetSpecialFolderPath(FALSE, CSIDL_DESKTOP);
    /* Test the desktop; even though SHITEMID should always contain abID of at
     * least one type, when cb is 0 its value is undefined.  So don't check
     * what the returned type is, just make sure it exists.
     */
    testSHGetFolderLocation(FALSE, CSIDL_DESKTOP);
    testSHGetSpecialFolderLocation(FALSE, CSIDL_DESKTOP);
}

static void testPersonal(void)
{
    BYTE type;

    /* The pidl may be a real folder, or a virtual directory, or a drive if the
     * home directory is set to the root directory of a drive.
     */
    type = testSHGetFolderLocation(FALSE, CSIDL_PERSONAL);
    ok(type == PT_FOLDER || type == PT_GUID || type == PT_DRIVE,
     "CSIDL_PERSONAL returned invalid type 0x%02x, "
     "expected PT_FOLDER or PT_GUID\n", type);
    if (type == PT_FOLDER)
        testSHGetFolderPath(FALSE, CSIDL_PERSONAL);
    type = testSHGetSpecialFolderLocation(FALSE, CSIDL_PERSONAL);
    ok(type == PT_FOLDER || type == PT_GUID || type == PT_DRIVE,
     "CSIDL_PERSONAL returned invalid type 0x%02x, "
     "expected PT_FOLDER or PT_GUID\n", type);
    if (type == PT_FOLDER)
        testSHGetSpecialFolderPath(FALSE, CSIDL_PERSONAL);
}

/* Checks the PIDL type of all the known values. */
static void testPidlTypes(void)
{
    testDesktop();
    testPersonal();
    testShellValues(requiredShellValues, ARRAY_SIZE(requiredShellValues),
     FALSE);
    testShellValues(optionalShellValues, ARRAY_SIZE(optionalShellValues),
     TRUE);
}

/* Verifies various shell virtual folders have the correct well-known GUIDs. */
static void testGUIDs(void)
{
    matchGUID(CSIDL_BITBUCKET, &CLSID_RecycleBin);
    matchGUID(CSIDL_CONTROLS, &CLSID_ControlPanel);
    matchGUID(CSIDL_DRIVES, &CLSID_MyComputer);
    matchGUID(CSIDL_INTERNET, &CLSID_Internet);
    matchGUID(CSIDL_NETWORK, &CLSID_NetworkPlaces);
    matchGUID(CSIDL_PERSONAL, &CLSID_MyDocuments);
    matchGUID(CSIDL_COMMON_DOCUMENTS, &CLSID_CommonDocuments);
}

/* Verifies various shell paths match the environment variables to which they
 * correspond.
 */
static void testEnvVars(void)
{
    matchSpecialFolderPathToEnv(CSIDL_PROGRAM_FILES, "ProgramFiles");
    matchSpecialFolderPathToEnv(CSIDL_APPDATA, "APPDATA");
    matchSpecialFolderPathToEnv(CSIDL_PROFILE, "USERPROFILE");
    matchSpecialFolderPathToEnv(CSIDL_WINDOWS, "SystemRoot");
    matchSpecialFolderPathToEnv(CSIDL_WINDOWS, "windir");
    matchSpecialFolderPathToEnv(CSIDL_PROGRAM_FILES_COMMON,
     "CommonProgramFiles");
    /* this is only set on Wine, but can't hurt to verify it: */
    matchSpecialFolderPathToEnv(CSIDL_SYSTEM, "winsysdir");
}

/* Verifies the shell path for CSIDL_WINDOWS matches the return from
 * GetWindowsDirectory.  If SHGetSpecialFolderPath fails, no harm, no foul--not
 * every shell32 version supports CSIDL_WINDOWS.
 */
static void testWinDir(void)
{
    char windowsShellPath[MAX_PATH], windowsDir[MAX_PATH] = { 0 };

    if (!pSHGetSpecialFolderPathA) return;

    if (pSHGetSpecialFolderPathA(NULL, windowsShellPath, CSIDL_WINDOWS, FALSE))
    {
        PathRemoveBackslashA(windowsShellPath);
        GetWindowsDirectoryA(windowsDir, sizeof(windowsDir));
        PathRemoveBackslashA(windowsDir);
        ok(!lstrcmpiA(windowsDir, windowsShellPath),
         "GetWindowsDirectory does not match SHGetSpecialFolderPath:\n"
         "GetWindowsDirectory returns %s\nSHGetSpecialFolderPath returns %s\n",
         windowsDir, windowsShellPath);
    }
}

/* Verifies the shell path for CSIDL_SYSTEM and CSIDL_SYSTEMX86 matches the
 * return from GetSystemDirectory.  If SHGetSpecialFolderPath fails, no harm,
 * no foul--not every shell32 version supports CSIDL_SYSTEM.
 */
static void testSystemDir(void)
{
    char systemShellPath[MAX_PATH], systemDir[MAX_PATH] = { 0 };

    if (!pSHGetSpecialFolderPathA) return;

    GetSystemDirectoryA(systemDir, sizeof(systemDir));
    PathRemoveBackslashA(systemDir);
    if (pSHGetSpecialFolderPathA(NULL, systemShellPath, CSIDL_SYSTEM, FALSE))
    {
        PathRemoveBackslashA(systemShellPath);
        ok(!lstrcmpiA(systemDir, systemShellPath),
         "GetSystemDirectory does not match SHGetSpecialFolderPath:\n"
         "GetSystemDirectory returns %s\nSHGetSpecialFolderPath returns %s\n",
         systemDir, systemShellPath);
    }
    /* check CSIDL_SYSTEMX86; note that this isn't always present, so don't
     * worry if it fails
     */
    if (pSHGetSpecialFolderPathA(NULL, systemShellPath, CSIDL_SYSTEMX86, FALSE))
    {
        PathRemoveBackslashA(systemShellPath);
        ok(!lstrcmpiA(systemDir, systemShellPath),
         "GetSystemDirectory does not match SHGetSpecialFolderPath:\n"
         "GetSystemDirectory returns %s\nSHGetSpecialFolderPath returns %s\n",
         systemDir, systemShellPath);
    }
}

/* Globals used by subprocesses */
static int    myARGC;
static char **myARGV;
static char   base[MAX_PATH];
static char   selfname[MAX_PATH];

static int init(void)
{
    myARGC = winetest_get_mainargs(&myARGV);
    if (!GetCurrentDirectoryA(sizeof(base), base)) return 0;
    strcpy(selfname, myARGV[0]);
    return 1;
}

/* Subprocess helper 1: test what happens when CSIDL_FAVORITES is set to a
 * nonexistent directory.
 */
static void testNonExistentPath1(void)
{
    HRESULT hr;
    LPITEMIDLIST pidl;
    char path[MAX_PATH];

    /* test some failure cases first: */
    hr = pSHGetFolderPathA(NULL, CSIDL_FAVORITES, NULL,
     SHGFP_TYPE_CURRENT, path);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
     "SHGetFolderPath returned 0x%08x, expected 0x80070002\n", hr);
    pidl = NULL;
    hr = pSHGetFolderLocation(NULL, CSIDL_FAVORITES, NULL, 0,
     &pidl);
    ok(hr == E_FAIL,
     "SHGetFolderLocation returned 0x%08x, expected E_FAIL\n", hr);
    if (SUCCEEDED(hr) && pidl)
        IMalloc_Free(pMalloc, pidl);
    ok(!pSHGetSpecialFolderPathA(NULL, path, CSIDL_FAVORITES, FALSE),
     "SHGetSpecialFolderPath succeeded, expected failure\n");
    pidl = NULL;
    hr = pSHGetSpecialFolderLocation(NULL, CSIDL_FAVORITES, &pidl);
    ok(hr == E_FAIL, "SHGetFolderLocation returned 0x%08x, expected E_FAIL\n",
     hr);
    if (SUCCEEDED(hr) && pidl)
        IMalloc_Free(pMalloc, pidl);
    /* now test success: */
    hr = pSHGetFolderPathA(NULL, CSIDL_FAVORITES | CSIDL_FLAG_CREATE, NULL,
     SHGFP_TYPE_CURRENT, path);
    if (SUCCEEDED(hr))
    {
        BOOL ret;

        if (winetest_interactive)
            printf("CSIDL_FAVORITES was changed to %s\n", path);
        ret = CreateDirectoryA(path, NULL);
        ok(!ret,
         "CreateDirectoryA succeeded but should have failed "
         "with ERROR_ALREADY_EXISTS\n");
        if (!ret)
            ok(GetLastError() == ERROR_ALREADY_EXISTS,
             "CreateDirectoryA failed with %d, "
             "expected ERROR_ALREADY_EXISTS\n",
             GetLastError());
    }
    ok(SUCCEEDED(hr),
     "SHGetFolderPath(NULL, CSIDL_FAVORITES | CSIDL_FLAG_CREATE, "
     "NULL, SHGFP_TYPE_CURRENT, path)\nfailed: 0x%08x\n", hr);
}

/* Subprocess helper 2: make sure SHGetFolderPath still succeeds when the
 * original value of CSIDL_FAVORITES is restored.
 */
static void testNonExistentPath2(void)
{
    HRESULT hr;
    char path[MAX_PATH];

    hr = pSHGetFolderPathA(NULL, CSIDL_FAVORITES | CSIDL_FLAG_CREATE, NULL,
     SHGFP_TYPE_CURRENT, path);
    ok(SUCCEEDED(hr), "SHGetFolderPath failed: 0x%08x\n", hr);
}

static void doChild(const char *arg)
{
    if (arg[0] == '1')
        testNonExistentPath1();
    else if (arg[0] == '2')
        testNonExistentPath2();
}

/* Tests the return values from the various shell functions both with and
 * without the use of the CSIDL_FLAG_CREATE flag.  This flag only appeared in
 * version 5 of the shell, so don't test unless it's at least version 5.
 * The test reads a value from the registry, modifies it, calls
 * SHGetFolderPath once with the CSIDL_FLAG_CREATE flag, and immediately
 * afterward without it.  Then it restores the registry and deletes the folder
 * that was created.
 * One oddity with respect to restoration: shell32 caches somehow, so it needs
 * to be reloaded in order to see the correct (restored) value.
 * Some APIs unrelated to the ones under test may fail, but I expect they're
 * covered by other unit tests; I just print out something about failure to
 * help trace what's going on.
 */
static void testNonExistentPath(void)
{
    static const char userShellFolders[] =
     "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders";
    char originalPath[MAX_PATH], modifiedPath[MAX_PATH];
    HKEY key;

    if (!pSHGetFolderPathA) return;
    if (!pSHGetFolderLocation) return;
    if (!pSHGetSpecialFolderPathA) return;
    if (!pSHGetSpecialFolderLocation) return;
    if (!pSHFileOperationA) return;
    if (shellVersion.dwMajorVersion < 5) return;

    if (!RegOpenKeyExA(HKEY_CURRENT_USER, userShellFolders, 0, KEY_ALL_ACCESS,
     &key))
    {
        DWORD len, type;

        len = sizeof(originalPath);
        if (!RegQueryValueExA(key, "Favorites", NULL, &type,
         (LPBYTE)&originalPath, &len))
        {
            size_t len = strlen(originalPath);

            memcpy(modifiedPath, originalPath, len);
            modifiedPath[len++] = '2';
            modifiedPath[len++] = '\0';
            if (winetest_interactive)
                printf("Changing CSIDL_FAVORITES to %s\n", modifiedPath);
            if (!RegSetValueExA(key, "Favorites", 0, type,
             (LPBYTE)modifiedPath, len))
            {
                char buffer[MAX_PATH+20];
                STARTUPINFOA startup;
                PROCESS_INFORMATION info;
                HRESULT hr;

                sprintf(buffer, "%s tests/shellpath.c 1", selfname);
                memset(&startup, 0, sizeof(startup));
                startup.cb = sizeof(startup);
                startup.dwFlags = STARTF_USESHOWWINDOW;
                startup.dwFlags = SW_SHOWNORMAL;
                CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL,
                 &startup, &info);
                ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0,
                 "child process termination\n");

                /* Query the path to be able to delete it below */
                hr = pSHGetFolderPathA(NULL, CSIDL_FAVORITES, NULL,
                 SHGFP_TYPE_CURRENT, modifiedPath);
                ok(SUCCEEDED(hr), "SHGetFolderPathA failed: 0x%08x\n", hr);

                /* restore original values: */
                if (winetest_interactive)
                    printf("Restoring CSIDL_FAVORITES to %s\n", originalPath);
                RegSetValueExA(key, "Favorites", 0, type, (LPBYTE) originalPath,
                 strlen(originalPath) + 1);
                RegFlushKey(key);

                sprintf(buffer, "%s tests/shellpath.c 2", selfname);
                memset(&startup, 0, sizeof(startup));
                startup.cb = sizeof(startup);
                startup.dwFlags = STARTF_USESHOWWINDOW;
                startup.dwFlags = SW_SHOWNORMAL;
                CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL,
                 &startup, &info);
                ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0,
                 "child process termination\n");

                strcpy(buffer, modifiedPath);
                strcat(buffer, "\\desktop.ini");
                DeleteFileA(buffer);
                RemoveDirectoryA(modifiedPath);
            }
        }
        else if (winetest_interactive)
            printf("RegQueryValueExA(key, Favorites, ...) failed\n");
        if (key)
            RegCloseKey(key);
    }
    else if (winetest_interactive)
        printf("RegOpenKeyExA(HKEY_CURRENT_USER, %s, ...) failed\n",
         userShellFolders);
}

START_TEST(shellpath)
{
    if (!init()) return;

    loadShell32();

    if (myARGC >= 3)
        doChild(myARGV[2]);
    else
    {
        /* first test various combinations of parameters: */
        testApiParameters();

        /* check known values: */
        testPidlTypes();
        testGUIDs();
        testEnvVars();
        testWinDir();
        testSystemDir();
        testNonExistentPath();
    }
}
