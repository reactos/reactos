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
 * of shell32, that get either a filesystem path or a LPITEMIDLIST (shell
 * namespace) path for a given folder (CSIDL value).
 */

#define COBJMACROS

#include <stdarg.h>
#include <stdio.h>
#include "windef.h"
#include "winbase.h"
#include "shlguid.h"
#include "shlobj.h"
#include "shlwapi.h"
#include "initguid.h"
#include "wine/test.h"

/* CSIDL_MYDOCUMENTS is now the same as CSIDL_PERSONAL, but what we want
 * here is its original value.
 */
#define OLD_CSIDL_MYDOCUMENTS  0x000c

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
#ifndef PT_FOLDERW
#define PT_FOLDERW    0x35 /* has path */
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
    int folder;
    int numTypes;
    const BYTE *types;
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
static const BYTE guidType[] = { PT_GUID };
static const BYTE controlPanelType[] = { PT_SHELLEXT, PT_GUID };
static const BYTE folderType[] = { PT_FOLDER, PT_FOLDERW };
static const BYTE favoritesType[] = { PT_FOLDER, PT_FOLDERW, 0, PT_IESPECIAL2 /* Win98 */ };
static const BYTE folderOrSpecialType[] = { PT_FOLDER, PT_IESPECIAL2 };
static const BYTE personalType[] = { PT_FOLDER, PT_GUID, PT_DRIVE, 0xff /* Win9x */,
 PT_IESPECIAL2 /* Win98 */, 0 /* Vista */ };
/* FIXME: don't know the type of 0x71 returned by Vista/2008 for printers */
static const BYTE printersType[] = { PT_YAGUID, PT_SHELLEXT, 0x71 };
static const BYTE ieSpecialType[] = { PT_IESPECIAL2 };
static const BYTE shellExtType[] = { PT_SHELLEXT };
static const BYTE workgroupType[] = { PT_WORKGRP };
#define DECLARE_TYPE(x, y) { x, sizeof(y) / sizeof(y[0]), y }
static const struct shellExpectedValues requiredShellValues[] = {
 DECLARE_TYPE(CSIDL_BITBUCKET, guidType),
 DECLARE_TYPE(CSIDL_CONTROLS, controlPanelType),
 DECLARE_TYPE(CSIDL_COOKIES, folderType),
 DECLARE_TYPE(CSIDL_DESKTOPDIRECTORY, folderType),
 DECLARE_TYPE(CSIDL_DRIVES, guidType),
 DECLARE_TYPE(CSIDL_FAVORITES, favoritesType),
 DECLARE_TYPE(CSIDL_FONTS, folderOrSpecialType),
/* FIXME: the following fails in Wine, returns type PT_FOLDER
 DECLARE_TYPE(CSIDL_HISTORY, ieSpecialType),
 */
 DECLARE_TYPE(CSIDL_INTERNET, guidType),
 DECLARE_TYPE(CSIDL_NETHOOD, folderType),
 DECLARE_TYPE(CSIDL_NETWORK, guidType),
 DECLARE_TYPE(CSIDL_PERSONAL, personalType),
 DECLARE_TYPE(CSIDL_PRINTERS, printersType),
 DECLARE_TYPE(CSIDL_PRINTHOOD, folderType),
 DECLARE_TYPE(CSIDL_PROGRAMS, folderType),
 DECLARE_TYPE(CSIDL_RECENT, folderOrSpecialType),
 DECLARE_TYPE(CSIDL_SENDTO, folderType),
 DECLARE_TYPE(CSIDL_STARTMENU, folderType),
 DECLARE_TYPE(CSIDL_STARTUP, folderType),
 DECLARE_TYPE(CSIDL_TEMPLATES, folderType),
};
static const struct shellExpectedValues optionalShellValues[] = {
/* FIXME: the following only semi-succeed; they return NULL PIDLs on XP.. hmm.
 DECLARE_TYPE(CSIDL_ALTSTARTUP, folderType),
 DECLARE_TYPE(CSIDL_COMMON_ALTSTARTUP, folderType),
 DECLARE_TYPE(CSIDL_COMMON_OEM_LINKS, folderType),
 */
/* Windows NT-only: */
 DECLARE_TYPE(CSIDL_COMMON_DESKTOPDIRECTORY, folderType),
 DECLARE_TYPE(CSIDL_COMMON_DOCUMENTS, shellExtType),
 DECLARE_TYPE(CSIDL_COMMON_FAVORITES, folderType),
 DECLARE_TYPE(CSIDL_COMMON_PROGRAMS, folderType),
 DECLARE_TYPE(CSIDL_COMMON_STARTMENU, folderType),
 DECLARE_TYPE(CSIDL_COMMON_STARTUP, folderType),
 DECLARE_TYPE(CSIDL_COMMON_TEMPLATES, folderType),
/* first appearing in shell32 version 4.71: */
 DECLARE_TYPE(CSIDL_APPDATA, folderType),
/* first appearing in shell32 version 4.72: */
 DECLARE_TYPE(CSIDL_INTERNET_CACHE, ieSpecialType),
/* first appearing in shell32 version 5.0: */
 DECLARE_TYPE(CSIDL_ADMINTOOLS, folderType),
 DECLARE_TYPE(CSIDL_COMMON_APPDATA, folderType),
 DECLARE_TYPE(CSIDL_LOCAL_APPDATA, folderType),
 DECLARE_TYPE(OLD_CSIDL_MYDOCUMENTS, folderType),
 DECLARE_TYPE(CSIDL_MYMUSIC, folderType),
 DECLARE_TYPE(CSIDL_MYPICTURES, folderType),
 DECLARE_TYPE(CSIDL_MYVIDEO, folderType),
 DECLARE_TYPE(CSIDL_PROFILE, folderType),
 DECLARE_TYPE(CSIDL_PROGRAM_FILES, folderType),
 DECLARE_TYPE(CSIDL_PROGRAM_FILESX86, folderType),
 DECLARE_TYPE(CSIDL_PROGRAM_FILES_COMMON, folderType),
 DECLARE_TYPE(CSIDL_PROGRAM_FILES_COMMONX86, folderType),
 DECLARE_TYPE(CSIDL_SYSTEM, folderType),
 DECLARE_TYPE(CSIDL_WINDOWS, folderType),
/* first appearing in shell32 6.0: */
 DECLARE_TYPE(CSIDL_CDBURN_AREA, folderType),
 DECLARE_TYPE(CSIDL_COMMON_MUSIC, folderType),
 DECLARE_TYPE(CSIDL_COMMON_PICTURES, folderType),
 DECLARE_TYPE(CSIDL_COMMON_VIDEO, folderType),
 DECLARE_TYPE(CSIDL_COMPUTERSNEARME, workgroupType),
 DECLARE_TYPE(CSIDL_RESOURCES, folderType),
 DECLARE_TYPE(CSIDL_RESOURCES_LOCALIZED, folderType),
};
#undef DECLARE_TYPE

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
        trace("shell32 version is %d.%d\n",
              shellVersion.dwMajorVersion, shellVersion.dwMinorVersion);
    }
#undef GET_PROC
}

#ifndef CSIDL_PROFILES
#define CSIDL_PROFILES		0x003e
#endif

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

static const char *printGUID(const GUID *guid, char * guidSTR)
{
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
     "SHGetFolderLocation(NULL, 0xeeee, NULL, 0, &pidl) returned 0x%08x, expected E_INVALIDARG\n", hr);
    if (SUCCEEDED(hr))
        IMalloc_Free(pMalloc, pidl);
    /* check a bogus user token: */
    pidl = NULL;
    hr = pSHGetFolderLocation(NULL, CSIDL_FAVORITES, (HANDLE)2, 0, &pidl);
    ok(hr == E_FAIL || hr == E_HANDLE,
     "SHGetFolderLocation(NULL, CSIDL_FAVORITES, 2, 0, &pidl) returned 0x%08x, expected E_FAIL or E_HANDLE\n", hr);
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
    ok(hr == E_FAIL ||
       hr == E_HANDLE ||   /* Windows Vista and 2008 */
       broken(hr == S_OK), /* Windows 2000 and Me */
     "SHGetFolderPathA(NULL, CSIDL_DESKTOP, 2, SHGFP_TYPE_DEFAULT, path) returned 0x%08x, expected E_FAIL\n", hr);
    hr = pSHGetFolderPathA(NULL, 0xeeee, NULL, SHGFP_TYPE_DEFAULT, path);
    ok(hr == E_INVALIDARG,
     "SHGetFolderPathA(NULL, 0xeeee, NULL, SHGFP_TYPE_DEFAULT, path) returned 0x%08x, expected E_INVALIDARG\n", hr);
}

static void testSHGetSpecialFolderPathInvalidArgs(void)
{
    char path[MAX_PATH];
    BOOL ret;

    if (!pSHGetSpecialFolderPathA) return;

#if 0
    ret = pSHGetSpecialFolderPathA(NULL, NULL, CSIDL_BITBUCKET, FALSE);
    ok(!ret,
     "SHGetSpecialFolderPathA(NULL, NULL, CSIDL_BITBUCKET, FALSE) returned TRUE, expected FALSE\n");
#endif
    /* odd but true: calling with a NULL path still succeeds if it's a real
     * dir (on some windows platform).  on winME it generates exception.
     */
    ret = pSHGetSpecialFolderPathA(NULL, path, CSIDL_PROGRAMS, FALSE);
    ok(ret,
     "SHGetSpecialFolderPathA(NULL, path, CSIDL_PROGRAMS, FALSE) returned FALSE, expected TRUE\n");
    ret = pSHGetSpecialFolderPathA(NULL, path, 0xeeee, FALSE);
    ok(!ret,
     "SHGetSpecialFolderPathA(NULL, path, 0xeeee, FALSE) returned TRUE, expected FALSE\n");
}

static void testApiParameters(void)
{
    testSHGetFolderLocationInvalidArgs();
    testSHGetSpecialFolderLocationInvalidArgs();
    testSHGetFolderPathInvalidArgs();
    testSHGetSpecialFolderPathInvalidArgs();
}

/* Returns the folder's PIDL type, or 0xff if one can't be found. */
static BYTE testSHGetFolderLocation(int folder)
{
    LPITEMIDLIST pidl;
    HRESULT hr;
    BYTE ret = 0xff;

    /* treat absence of function as success */
    if (!pSHGetFolderLocation) return TRUE;

    pidl = NULL;
    hr = pSHGetFolderLocation(NULL, folder, NULL, 0, &pidl);
    if (SUCCEEDED(hr))
    {
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
static BYTE testSHGetSpecialFolderLocation(int folder)
{
    LPITEMIDLIST pidl;
    HRESULT hr;
    BYTE ret = 0xff;

    /* treat absence of function as success */
    if (!pSHGetSpecialFolderLocation) return TRUE;

    pidl = NULL;
    hr = pSHGetSpecialFolderLocation(NULL, folder, &pidl);
    if (SUCCEEDED(hr))
    {
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
     "SHGetFolderPathA(NULL, %s, NULL, SHGFP_TYPE_CURRENT, path) failed: 0x%08x\n", getFolderName(folder), hr);
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
        int j;
        BOOL foundTypeMatch = FALSE;

        if (pSHGetFolderLocation)
        {
            type = testSHGetFolderLocation(testEntries[i].folder);
            for (j = 0; !foundTypeMatch && j < testEntries[i].numTypes; j++)
                if (testEntries[i].types[j] == type)
                    foundTypeMatch = TRUE;
            ok(foundTypeMatch || optional || broken(type == 0xff) /* Win9x */,
             "%s has unexpected type %d (0x%02x)\n",
             getFolderName(testEntries[i].folder), type, type);
        }
        type = testSHGetSpecialFolderLocation(testEntries[i].folder);
        for (j = 0, foundTypeMatch = FALSE; !foundTypeMatch &&
         j < testEntries[i].numTypes; j++)
            if (testEntries[i].types[j] == type)
                foundTypeMatch = TRUE;
        ok(foundTypeMatch || optional || broken(type == 0xff) /* Win9x */,
         "%s has unexpected type %d (0x%02x)\n",
         getFolderName(testEntries[i].folder), type, type);
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
static void matchGUID(int folder, const GUID *guid, const GUID *guid_alt)
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
            char shellGuidStr[39], guidStr[39], guid_altStr[39];

            if (!guid_alt)
             ok(IsEqualIID(shellGuid, guid),
              "%s: got GUID %s, expected %s\n", getFolderName(folder),
              printGUID(shellGuid, shellGuidStr), printGUID(guid, guidStr));
            else
             ok(IsEqualIID(shellGuid, guid) ||
              IsEqualIID(shellGuid, guid_alt),
              "%s: got GUID %s, expected %s or %s\n", getFolderName(folder),
              printGUID(shellGuid, shellGuidStr), printGUID(guid, guidStr),
              printGUID(guid_alt, guid_altStr));
        }
        IMalloc_Free(pMalloc, pidl);
    }
}

static void testDesktop(void)
{
    testSHGetFolderPath(FALSE, CSIDL_DESKTOP);
    testSHGetSpecialFolderPath(FALSE, CSIDL_DESKTOP);
}

/* Checks the PIDL type of all the known values. */
static void testPidlTypes(void)
{
    testDesktop();
    testShellValues(requiredShellValues, ARRAY_SIZE(requiredShellValues),
     FALSE);
    testShellValues(optionalShellValues, ARRAY_SIZE(optionalShellValues),
     TRUE);
}

/* FIXME: Should be in shobjidl.idl */
DEFINE_GUID(CLSID_NetworkExplorerFolder, 0xF02C1A0D, 0xBE21, 0x4350, 0x88, 0xB0, 0x73, 0x67, 0xFC, 0x96, 0xEF, 0x3C);

/* Verifies various shell virtual folders have the correct well-known GUIDs. */
static void testGUIDs(void)
{
    matchGUID(CSIDL_BITBUCKET, &CLSID_RecycleBin, NULL);
    matchGUID(CSIDL_CONTROLS, &CLSID_ControlPanel, NULL);
    matchGUID(CSIDL_DRIVES, &CLSID_MyComputer, NULL);
    matchGUID(CSIDL_INTERNET, &CLSID_Internet, NULL);
    matchGUID(CSIDL_NETWORK, &CLSID_NetworkPlaces, &CLSID_NetworkExplorerFolder); /* Vista and higher */
    matchGUID(CSIDL_PERSONAL, &CLSID_MyDocuments, NULL);
    matchGUID(CSIDL_COMMON_DOCUMENTS, &CLSID_CommonDocuments, NULL);
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

/* Loosely based on PathRemoveBackslashA from dlls/shlwapi/path.c */
static BOOL myPathIsRootA(LPCSTR lpszPath)
{
  if (lpszPath && *lpszPath &&
      lpszPath[1] == ':' && lpszPath[2] == '\\' && lpszPath[3] == '\0')
      return TRUE; /* X:\ */
  return FALSE;
}
static LPSTR myPathRemoveBackslashA( LPSTR lpszPath )
{
  LPSTR szTemp = NULL;

  if(lpszPath)
  {
    szTemp = CharPrevA(lpszPath, lpszPath + strlen(lpszPath));
    if (!myPathIsRootA(lpszPath) && *szTemp == '\\')
      *szTemp = '\0';
  }
  return szTemp;
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
        myPathRemoveBackslashA(windowsShellPath);
        GetWindowsDirectoryA(windowsDir, sizeof(windowsDir));
        myPathRemoveBackslashA(windowsDir);
        ok(!lstrcmpiA(windowsDir, windowsShellPath),
         "GetWindowsDirectory returns %s SHGetSpecialFolderPath returns %s\n",
         windowsDir, windowsShellPath);
    }
}

/* Verifies the shell path for CSIDL_SYSTEM matches the return from
 * GetSystemDirectory.  If SHGetSpecialFolderPath fails, no harm,
 * no foul--not every shell32 version supports CSIDL_SYSTEM.
 */
static void testSystemDir(void)
{
    char systemShellPath[MAX_PATH], systemDir[MAX_PATH] = { 0 };

    if (!pSHGetSpecialFolderPathA) return;

    GetSystemDirectoryA(systemDir, sizeof(systemDir));
    myPathRemoveBackslashA(systemDir);
    if (pSHGetSpecialFolderPathA(NULL, systemShellPath, CSIDL_SYSTEM, FALSE))
    {
        myPathRemoveBackslashA(systemShellPath);
        ok(!lstrcmpiA(systemDir, systemShellPath),
         "GetSystemDirectory returns %s SHGetSpecialFolderPath returns %s\n",
         systemDir, systemShellPath);
    }
    /* CSIDL_SYSTEMX86 isn't checked in the same way, since it's different
     * on Win64 (and non-x86 Windows systems, if there are any still in
     * existence) than on Win32.
     */
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
    char *p, path[MAX_PATH];

    /* test some failure cases first: */
    hr = pSHGetFolderPathA(NULL, CSIDL_FAVORITES, NULL,
     SHGFP_TYPE_CURRENT, path);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
     "SHGetFolderPath returned 0x%08x, expected 0x80070002\n", hr);
    pidl = NULL;
    hr = pSHGetFolderLocation(NULL, CSIDL_FAVORITES, NULL, 0,
     &pidl);
    ok(hr == E_FAIL || hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
     "SHGetFolderLocation returned 0x%08x\n", hr);
    if (SUCCEEDED(hr) && pidl)
        IMalloc_Free(pMalloc, pidl);
    ok(!pSHGetSpecialFolderPathA(NULL, path, CSIDL_FAVORITES, FALSE),
     "SHGetSpecialFolderPath succeeded, expected failure\n");
    pidl = NULL;
    hr = pSHGetSpecialFolderLocation(NULL, CSIDL_FAVORITES, &pidl);
    ok(hr == E_FAIL || hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
       "SHGetFolderLocation returned 0x%08x\n", hr);
    if (SUCCEEDED(hr) && pidl)
        IMalloc_Free(pMalloc, pidl);
    /* now test success: */
    hr = pSHGetFolderPathA(NULL, CSIDL_FAVORITES | CSIDL_FLAG_CREATE, NULL,
     SHGFP_TYPE_CURRENT, path);
    if (SUCCEEDED(hr))
    {
        BOOL ret;

        trace("CSIDL_FAVORITES was changed to %s\n", path);
        ret = CreateDirectoryA(path, NULL);
        ok(!ret,
         "CreateDirectoryA succeeded but should have failed "
         "with ERROR_ALREADY_EXISTS\n");
        if (!ret)
            ok(GetLastError() == ERROR_ALREADY_EXISTS,
             "CreateDirectoryA failed with %d, "
             "expected ERROR_ALREADY_EXISTS\n",
             GetLastError());
        p = path + strlen(path);
        strcpy(p, "\\desktop.ini");
        DeleteFileA(path);
        *p = 0;
        SetFileAttributesA( path, FILE_ATTRIBUTE_NORMAL );
        ret = RemoveDirectoryA(path);
        ok( ret, "failed to remove %s error %u\n", path, GetLastError() );
    }
    ok(SUCCEEDED(hr),
     "SHGetFolderPath(NULL, CSIDL_FAVORITES | CSIDL_FLAG_CREATE, "
     "NULL, SHGFP_TYPE_CURRENT, path) failed: 0x%08x\n", hr);
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
            trace("Changing CSIDL_FAVORITES to %s\n", modifiedPath);
            if (!RegSetValueExA(key, "Favorites", 0, type,
             (LPBYTE)modifiedPath, len))
            {
                char buffer[MAX_PATH+20];
                STARTUPINFOA startup;
                PROCESS_INFORMATION info;

                sprintf(buffer, "%s tests/shellpath.c 1", selfname);
                memset(&startup, 0, sizeof(startup));
                startup.cb = sizeof(startup);
                startup.dwFlags = STARTF_USESHOWWINDOW;
                startup.dwFlags = SW_SHOWNORMAL;
                CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL,
                 &startup, &info);
                winetest_wait_child_process( info.hProcess );

                /* restore original values: */
                trace("Restoring CSIDL_FAVORITES to %s\n", originalPath);
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
            }
        }
        else skip("RegQueryValueExA(key, Favorites, ...) failed\n");
        if (key)
            RegCloseKey(key);
    }
    else skip("RegOpenKeyExA(HKEY_CURRENT_USER, %s, ...) failed\n", userShellFolders);
}

START_TEST(shellpath)
{
    if (!init()) return;

    loadShell32();

    if (myARGC >= 3)
        doChild(myARGV[2]);
    else
    {
        /* Report missing functions once */
        if (!pSHGetFolderLocation)
            win_skip("SHGetFolderLocation is not available\n");

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
