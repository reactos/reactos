/*
 * Unit tests for shelllinks
 *
 * Copyright 2004 Mike McCormack
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
 *
 */

#define COBJMACROS

#include "initguid.h"
#include "windows.h"
#include "shlguid.h"
#include "shobjidl.h"
#include "shlobj.h"
#include "shellapi.h"
#include "wine/test.h"

#include "shell32_test.h"

#ifndef SLDF_HAS_LOGO3ID
#  define SLDF_HAS_LOGO3ID 0x00000800 /* not available in the Vista SDK */
#endif

static void (WINAPI *pILFree)(LPITEMIDLIST);
static BOOL (WINAPI *pILIsEqual)(LPCITEMIDLIST, LPCITEMIDLIST);
static HRESULT (WINAPI *pSHILCreateFromPath)(LPCWSTR, LPITEMIDLIST *,DWORD*);
static HRESULT (WINAPI *pSHDefExtractIconA)(LPCSTR, int, UINT, HICON*, HICON*, UINT);
static HRESULT (WINAPI *pSHGetStockIconInfo)(SHSTOCKICONID, UINT, SHSTOCKICONINFO *);
static DWORD (WINAPI *pGetLongPathNameA)(LPCSTR, LPSTR, DWORD);
static DWORD (WINAPI *pGetShortPathNameA)(LPCSTR, LPSTR, DWORD);
static UINT (WINAPI *pSHExtractIconsW)(LPCWSTR, int, int, int, HICON *, UINT *, UINT, UINT);

static const GUID _IID_IShellLinkDataList = {
    0x45e2b4ae, 0xb1c3, 0x11d0,
    { 0xb9, 0x2f, 0x00, 0xa0, 0xc9, 0x03, 0x12, 0xe1 }
};


/* For some reason SHILCreateFromPath does not work on Win98 and
 * SHSimpleIDListFromPathA does not work on NT4. But if we call both we
 * get what we want on all platforms.
 */
static LPITEMIDLIST (WINAPI *pSHSimpleIDListFromPathAW)(LPCVOID);

static LPITEMIDLIST path_to_pidl(const char* path)
{
    LPITEMIDLIST pidl;

    if (!pSHSimpleIDListFromPathAW)
    {
        HMODULE hdll=GetModuleHandleA("shell32.dll");
        pSHSimpleIDListFromPathAW=(void*)GetProcAddress(hdll, (char*)162);
        if (!pSHSimpleIDListFromPathAW)
            win_skip("SHSimpleIDListFromPathAW not found in shell32.dll\n");
    }

    pidl=NULL;
    /* pSHSimpleIDListFromPathAW maps to A on non NT platforms */
    if (pSHSimpleIDListFromPathAW && (GetVersion() & 0x80000000))
        pidl=pSHSimpleIDListFromPathAW(path);

    if (!pidl)
    {
        WCHAR* pathW;
        HRESULT r;
        int len;

        len=MultiByteToWideChar(CP_ACP, 0, path, -1, NULL, 0);
        pathW=HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, path, -1, pathW, len);

        r=pSHILCreateFromPath(pathW, &pidl, NULL);
        ok(r == S_OK, "SHILCreateFromPath failed (0x%08x)\n", r);
        HeapFree(GetProcessHeap(), 0, pathW);
    }
    return pidl;
}


/*
 * Test manipulation of an IShellLink's properties.
 */

static void test_get_set(void)
{
    HRESULT r;
    IShellLinkA *sl;
    IShellLinkW *slW = NULL;
    char mypath[MAX_PATH];
    char buffer[INFOTIPSIZE];
    LPITEMIDLIST pidl, tmp_pidl;
    const char * str;
    int i;
    WORD w;

    r = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                         &IID_IShellLinkA, (LPVOID*)&sl);
    ok(r == S_OK, "no IID_IShellLinkA (0x%08x)\n", r);
    if (r != S_OK)
        return;

    /* Test Getting / Setting the description */
    strcpy(buffer,"garbage");
    r = IShellLinkA_GetDescription(sl, buffer, sizeof(buffer));
    ok(r == S_OK, "GetDescription failed (0x%08x)\n", r);
    ok(*buffer=='\0', "GetDescription returned '%s'\n", buffer);

    str="Some description";
    r = IShellLinkA_SetDescription(sl, str);
    ok(r == S_OK, "SetDescription failed (0x%08x)\n", r);

    strcpy(buffer,"garbage");
    r = IShellLinkA_GetDescription(sl, buffer, sizeof(buffer));
    ok(r == S_OK, "GetDescription failed (0x%08x)\n", r);
    ok(strcmp(buffer,str)==0, "GetDescription returned '%s'\n", buffer);

    r = IShellLinkA_SetDescription(sl, NULL);
    ok(r == S_OK, "SetDescription failed (0x%08x)\n", r);

    strcpy(buffer,"garbage");
    r = IShellLinkA_GetDescription(sl, buffer, sizeof(buffer));
    ok(r == S_OK, "GetDescription failed (0x%08x)\n", r);
    ok(*buffer=='\0' || broken(strcmp(buffer,str)==0), "GetDescription returned '%s'\n", buffer); /* NT4 */

    /* Test Getting / Setting the work directory */
    strcpy(buffer,"garbage");
    r = IShellLinkA_GetWorkingDirectory(sl, buffer, sizeof(buffer));
    ok(r == S_OK, "GetWorkingDirectory failed (0x%08x)\n", r);
    ok(*buffer=='\0', "GetWorkingDirectory returned '%s'\n", buffer);

    str="c:\\nonexistent\\directory";
    r = IShellLinkA_SetWorkingDirectory(sl, str);
    ok(r == S_OK, "SetWorkingDirectory failed (0x%08x)\n", r);

    strcpy(buffer,"garbage");
    r = IShellLinkA_GetWorkingDirectory(sl, buffer, sizeof(buffer));
    ok(r == S_OK, "GetWorkingDirectory failed (0x%08x)\n", r);
    ok(lstrcmpiA(buffer,str)==0, "GetWorkingDirectory returned '%s'\n", buffer);

    /* Test Getting / Setting the path */
    strcpy(buffer,"garbage");
    r = IShellLinkA_GetPath(sl, buffer, sizeof(buffer), NULL, SLGP_RAWPATH);
    todo_wine ok(r == S_FALSE || broken(r == S_OK) /* NT4/W2K */, "GetPath failed (0x%08x)\n", r);
    ok(*buffer=='\0', "GetPath returned '%s'\n", buffer);

    CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                     &IID_IShellLinkW, (LPVOID*)&slW);
    if (!slW /* Win9x */ || !pGetLongPathNameA /* NT4 */)
        skip("SetPath with NULL parameter crashes on Win9x and some NT4\n");
    else
    {
        IShellLinkW_Release(slW);
        r = IShellLinkA_SetPath(sl, NULL);
        ok(r==E_INVALIDARG ||
           broken(r==S_OK), /* Some Win95 and NT4 */
           "SetPath returned wrong error (0x%08x)\n", r);
    }

    r = IShellLinkA_SetPath(sl, "");
    ok(r==S_OK, "SetPath failed (0x%08x)\n", r);

    strcpy(buffer,"garbage");
    r = IShellLinkA_GetPath(sl, buffer, sizeof(buffer), NULL, SLGP_RAWPATH);
    todo_wine ok(r == S_FALSE, "GetPath failed (0x%08x)\n", r);
    ok(*buffer=='\0', "GetPath returned '%s'\n", buffer);

    /* Win98 returns S_FALSE, but WinXP returns S_OK */
    str="c:\\nonexistent\\file";
    r = IShellLinkA_SetPath(sl, str);
    ok(r==S_FALSE || r==S_OK, "SetPath failed (0x%08x)\n", r);

    strcpy(buffer,"garbage");
    r = IShellLinkA_GetPath(sl, buffer, sizeof(buffer), NULL, SLGP_RAWPATH);
    ok(r == S_OK, "GetPath failed (0x%08x)\n", r);
    ok(lstrcmpiA(buffer,str)==0, "GetPath returned '%s'\n", buffer);

    /* Get some real path to play with */
    GetWindowsDirectoryA( mypath, sizeof(mypath)-12 );
    strcat(mypath, "\\regedit.exe");

    /* Test the interaction of SetPath and SetIDList */
    tmp_pidl=NULL;
    r = IShellLinkA_GetIDList(sl, &tmp_pidl);
    ok(r == S_OK, "GetIDList failed (0x%08x)\n", r);
    if (r == S_OK)
    {
        BOOL ret;

        strcpy(buffer,"garbage");
        ret = SHGetPathFromIDListA(tmp_pidl, buffer);
        ok(ret, "SHGetPathFromIDListA failed\n");
        if (ret)
            ok(lstrcmpiA(buffer,str)==0, "GetIDList returned '%s'\n", buffer);
        pILFree(tmp_pidl);
    }

    pidl=path_to_pidl(mypath);
    ok(pidl!=NULL, "path_to_pidl returned a NULL pidl\n");

    if (pidl)
    {
        LPITEMIDLIST second_pidl;

        r = IShellLinkA_SetIDList(sl, pidl);
        ok(r == S_OK, "SetIDList failed (0x%08x)\n", r);

        tmp_pidl=NULL;
        r = IShellLinkA_GetIDList(sl, &tmp_pidl);
        ok(r == S_OK, "GetIDList failed (0x%08x)\n", r);
        ok(tmp_pidl && pILIsEqual(pidl, tmp_pidl),
           "GetIDList returned an incorrect pidl\n");

        r = IShellLinkA_GetIDList(sl, &second_pidl);
        ok(r == S_OK, "GetIDList failed (0x%08x)\n", r);
        ok(second_pidl && pILIsEqual(pidl, second_pidl),
           "GetIDList returned an incorrect pidl\n");
        ok(second_pidl != tmp_pidl, "pidls are the same\n");

        pILFree(second_pidl);
        pILFree(tmp_pidl);
        pILFree(pidl);

        strcpy(buffer,"garbage");
        r = IShellLinkA_GetPath(sl, buffer, sizeof(buffer), NULL, SLGP_RAWPATH);
        ok(r == S_OK, "GetPath failed (0x%08x)\n", r);
        todo_wine
        ok(lstrcmpiA(buffer, mypath)==0, "GetPath returned '%s'\n", buffer);
    }

    /* test path with quotes (IShellLinkA_SetPath returns S_FALSE on W2K and below and S_OK on XP and above */
    r = IShellLinkA_SetPath(sl, "\"c:\\nonexistent\\file\"");
    ok(r==S_FALSE || r == S_OK, "SetPath failed (0x%08x)\n", r);

    strcpy(buffer,"garbage");
    r = IShellLinkA_GetPath(sl, buffer, sizeof(buffer), NULL, SLGP_RAWPATH);
    ok(r==S_OK, "GetPath failed (0x%08x)\n", r);
    ok(!strcmp(buffer, "C:\\nonexistent\\file") ||
       broken(!strcmp(buffer, "C:\\\"c:\\nonexistent\\file\"")), /* NT4 */
       "case doesn't match\n");

    r = IShellLinkA_SetPath(sl, "\"c:\\foo");
    ok(r==S_FALSE || r == S_OK || r == E_INVALIDARG /* Vista */, "SetPath failed (0x%08x)\n", r);

    r = IShellLinkA_SetPath(sl, "\"\"c:\\foo");
    ok(r==S_FALSE || r == S_OK || r == E_INVALIDARG /* Vista */, "SetPath failed (0x%08x)\n", r);

    r = IShellLinkA_SetPath(sl, "c:\\foo\"");
    ok(r==S_FALSE || r == S_OK || r == E_INVALIDARG /* Vista */, "SetPath failed (0x%08x)\n", r);

    r = IShellLinkA_SetPath(sl, "\"\"c:\\foo\"");
    ok(r==S_FALSE || r == S_OK || r == E_INVALIDARG /* Vista */, "SetPath failed (0x%08x)\n", r);

    r = IShellLinkA_SetPath(sl, "\"\"c:\\foo\"\"");
    ok(r==S_FALSE || r == S_OK || r == E_INVALIDARG /* Vista */, "SetPath failed (0x%08x)\n", r);

    /* Test Getting / Setting the arguments */
    strcpy(buffer,"garbage");
    r = IShellLinkA_GetArguments(sl, buffer, sizeof(buffer));
    ok(r == S_OK, "GetArguments failed (0x%08x)\n", r);
    ok(*buffer=='\0', "GetArguments returned '%s'\n", buffer);

    str="param1 \"spaced param2\"";
    r = IShellLinkA_SetArguments(sl, str);
    ok(r == S_OK, "SetArguments failed (0x%08x)\n", r);

    strcpy(buffer,"garbage");
    r = IShellLinkA_GetArguments(sl, buffer, sizeof(buffer));
    ok(r == S_OK, "GetArguments failed (0x%08x)\n", r);
    ok(strcmp(buffer,str)==0, "GetArguments returned '%s'\n", buffer);

    strcpy(buffer,"garbage");
    r = IShellLinkA_SetArguments(sl, NULL);
    ok(r == S_OK, "SetArguments failed (0x%08x)\n", r);
    r = IShellLinkA_GetArguments(sl, buffer, sizeof(buffer));
    ok(r == S_OK, "GetArguments failed (0x%08x)\n", r);
    ok(!buffer[0] || strcmp(buffer,str)==0, "GetArguments returned '%s'\n", buffer);

    strcpy(buffer,"garbage");
    r = IShellLinkA_SetArguments(sl, "");
    ok(r == S_OK, "SetArguments failed (0x%08x)\n", r);
    r = IShellLinkA_GetArguments(sl, buffer, sizeof(buffer));
    ok(r == S_OK, "GetArguments failed (0x%08x)\n", r);
    ok(!buffer[0], "GetArguments returned '%s'\n", buffer);

    /* Test Getting / Setting showcmd */
    i=0xdeadbeef;
    r = IShellLinkA_GetShowCmd(sl, &i);
    ok(r == S_OK, "GetShowCmd failed (0x%08x)\n", r);
    ok(i==SW_SHOWNORMAL, "GetShowCmd returned %d\n", i);

    r = IShellLinkA_SetShowCmd(sl, SW_SHOWMAXIMIZED);
    ok(r == S_OK, "SetShowCmd failed (0x%08x)\n", r);

    i=0xdeadbeef;
    r = IShellLinkA_GetShowCmd(sl, &i);
    ok(r == S_OK, "GetShowCmd failed (0x%08x)\n", r);
    ok(i==SW_SHOWMAXIMIZED, "GetShowCmd returned %d'\n", i);

    /* Test Getting / Setting the icon */
    i=0xdeadbeef;
    strcpy(buffer,"garbage");
    r = IShellLinkA_GetIconLocation(sl, buffer, sizeof(buffer), &i);
    ok(r == S_OK, "GetIconLocation failed (0x%08x)\n", r);
    ok(*buffer=='\0', "GetIconLocation returned '%s'\n", buffer);
    ok(i==0, "GetIconLocation returned %d\n", i);

    str="c:\\nonexistent\\file";
    r = IShellLinkA_SetIconLocation(sl, str, 0xbabecafe);
    ok(r == S_OK, "SetIconLocation failed (0x%08x)\n", r);

    i=0xdeadbeef;
    r = IShellLinkA_GetIconLocation(sl, buffer, sizeof(buffer), &i);
    ok(r == S_OK, "GetIconLocation failed (0x%08x)\n", r);
    ok(lstrcmpiA(buffer,str)==0, "GetIconLocation returned '%s'\n", buffer);
    ok(i==0xbabecafe, "GetIconLocation returned %d'\n", i);

    /* Test Getting / Setting the hot key */
    w=0xbeef;
    r = IShellLinkA_GetHotkey(sl, &w);
    ok(r == S_OK, "GetHotkey failed (0x%08x)\n", r);
    ok(w==0, "GetHotkey returned %d\n", w);

    r = IShellLinkA_SetHotkey(sl, 0x5678);
    ok(r == S_OK, "SetHotkey failed (0x%08x)\n", r);

    w=0xbeef;
    r = IShellLinkA_GetHotkey(sl, &w);
    ok(r == S_OK, "GetHotkey failed (0x%08x)\n", r);
    ok(w==0x5678, "GetHotkey returned %d'\n", w);

    IShellLinkA_Release(sl);
}


/*
 * Test saving and loading .lnk files
 */

#define lok                   ok_(__FILE__, line)
#define lok_todo_4(todo_flag,a,b,c,d) \
    if ((todo & todo_flag) == 0) lok((a), (b), (c), (d)); \
    else todo_wine lok((a), (b), (c), (d));
#define lok_todo_2(todo_flag,a,b) \
    if ((todo & todo_flag) == 0) lok((a), (b)); \
    else todo_wine lok((a), (b));
#define check_lnk(a,b,c)        check_lnk_(__LINE__, (a), (b), (c))

void create_lnk_(int line, const WCHAR* path, lnk_desc_t* desc, int save_fails)
{
    HRESULT r;
    IShellLinkA *sl;
    IPersistFile *pf;

    r = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                         &IID_IShellLinkA, (LPVOID*)&sl);
    lok(r == S_OK, "no IID_IShellLinkA (0x%08x)\n", r);
    if (r != S_OK)
        return;

    if (desc->description)
    {
        r = IShellLinkA_SetDescription(sl, desc->description);
        lok(r == S_OK, "SetDescription failed (0x%08x)\n", r);
    }
    if (desc->workdir)
    {
        r = IShellLinkA_SetWorkingDirectory(sl, desc->workdir);
        lok(r == S_OK, "SetWorkingDirectory failed (0x%08x)\n", r);
    }
    if (desc->path)
    {
        r = IShellLinkA_SetPath(sl, desc->path);
        lok(SUCCEEDED(r), "SetPath failed (0x%08x)\n", r);
    }
    if (desc->pidl)
    {
        r = IShellLinkA_SetIDList(sl, desc->pidl);
        lok(r == S_OK, "SetIDList failed (0x%08x)\n", r);
    }
    if (desc->arguments)
    {
        r = IShellLinkA_SetArguments(sl, desc->arguments);
        lok(r == S_OK, "SetArguments failed (0x%08x)\n", r);
    }
    if (desc->showcmd)
    {
        r = IShellLinkA_SetShowCmd(sl, desc->showcmd);
        lok(r == S_OK, "SetShowCmd failed (0x%08x)\n", r);
    }
    if (desc->icon)
    {
        r = IShellLinkA_SetIconLocation(sl, desc->icon, desc->icon_id);
        lok(r == S_OK, "SetIconLocation failed (0x%08x)\n", r);
    }
    if (desc->hotkey)
    {
        r = IShellLinkA_SetHotkey(sl, desc->hotkey);
        lok(r == S_OK, "SetHotkey failed (0x%08x)\n", r);
    }

    r = IShellLinkA_QueryInterface(sl, &IID_IPersistFile, (void**)&pf);
    lok(r == S_OK, "no IID_IPersistFile (0x%08x)\n", r);
    if (r == S_OK)
    {
        LPOLESTR str;

    if (0)
    {
        /* crashes on XP */
        IPersistFile_GetCurFile(pf, NULL);
    }

        /* test GetCurFile before ::Save */
        str = (LPWSTR)0xdeadbeef;
        r = IPersistFile_GetCurFile(pf, &str);
        lok(r == S_FALSE ||
            broken(r == S_OK), /* shell32 < 5.0 */
            "got 0x%08x\n", r);
        lok(str == NULL, "got %p\n", str);

        r = IPersistFile_Save(pf, path, TRUE);
        if (save_fails)
        {
            todo_wine {
            lok(r == S_OK, "save failed (0x%08x)\n", r);
            }
        }
        else
        {
            lok(r == S_OK, "save failed (0x%08x)\n", r);
        }

        /* test GetCurFile after ::Save */
        r = IPersistFile_GetCurFile(pf, &str);
        lok(r == S_OK, "got 0x%08x\n", r);
        lok(str != NULL ||
            broken(str == NULL), /* shell32 < 5.0 */
            "Didn't expect NULL\n");
        if (str != NULL)
        {
            IMalloc *pmalloc;

            lok(!winetest_strcmpW(path, str), "Expected %s, got %s\n",
                wine_dbgstr_w(path), wine_dbgstr_w(str));

            SHGetMalloc(&pmalloc);
            IMalloc_Free(pmalloc, str);
        }
        else
            win_skip("GetCurFile fails on shell32 < 5.0\n");

        IPersistFile_Release(pf);
    }

    IShellLinkA_Release(sl);
}

static void check_lnk_(int line, const WCHAR* path, lnk_desc_t* desc, int todo)
{
    HRESULT r;
    IShellLinkA *sl;
    IPersistFile *pf;
    char buffer[INFOTIPSIZE];
    LPOLESTR str;

    r = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                         &IID_IShellLinkA, (LPVOID*)&sl);
    lok(r == S_OK, "no IID_IShellLinkA (0x%08x)\n", r);
    if (r != S_OK)
        return;

    r = IShellLinkA_QueryInterface(sl, &IID_IPersistFile, (LPVOID*)&pf);
    lok(r == S_OK, "no IID_IPersistFile (0x%08x)\n", r);
    if (r != S_OK)
    {
        IShellLinkA_Release(sl);
        return;
    }

    /* test GetCurFile before ::Load */
    str = (LPWSTR)0xdeadbeef;
    r = IPersistFile_GetCurFile(pf, &str);
    lok(r == S_FALSE ||
        broken(r == S_OK), /* shell32 < 5.0 */
        "got 0x%08x\n", r);
    lok(str == NULL, "got %p\n", str);

    r = IPersistFile_Load(pf, path, STGM_READ);
    lok(r == S_OK, "load failed (0x%08x)\n", r);

    /* test GetCurFile after ::Save */
    r = IPersistFile_GetCurFile(pf, &str);
    lok(r == S_OK, "got 0x%08x\n", r);
    lok(str != NULL ||
        broken(str == NULL), /* shell32 < 5.0 */
        "Didn't expect NULL\n");
    if (str != NULL)
    {
        IMalloc *pmalloc;

        lok(!winetest_strcmpW(path, str), "Expected %s, got %s\n",
            wine_dbgstr_w(path), wine_dbgstr_w(str));

        SHGetMalloc(&pmalloc);
        IMalloc_Free(pmalloc, str);
    }
    else
        win_skip("GetCurFile fails on shell32 < 5.0\n");

    IPersistFile_Release(pf);
    if (r != S_OK)
    {
        IShellLinkA_Release(sl);
        return;
    }

    if (desc->description)
    {
        strcpy(buffer,"garbage");
        r = IShellLinkA_GetDescription(sl, buffer, sizeof(buffer));
        lok(r == S_OK, "GetDescription failed (0x%08x)\n", r);
        lok_todo_4(0x1, strcmp(buffer, desc->description)==0,
           "GetDescription returned '%s' instead of '%s'\n",
           buffer, desc->description);
    }
    if (desc->workdir)
    {
        strcpy(buffer,"garbage");
        r = IShellLinkA_GetWorkingDirectory(sl, buffer, sizeof(buffer));
        lok(r == S_OK, "GetWorkingDirectory failed (0x%08x)\n", r);
        lok_todo_4(0x2, lstrcmpiA(buffer, desc->workdir)==0,
           "GetWorkingDirectory returned '%s' instead of '%s'\n",
           buffer, desc->workdir);
    }
    if (desc->path)
    {
        strcpy(buffer,"garbage");
        r = IShellLinkA_GetPath(sl, buffer, sizeof(buffer), NULL, SLGP_RAWPATH);
        lok(SUCCEEDED(r), "GetPath failed (0x%08x)\n", r);
        lok_todo_4(0x4, lstrcmpiA(buffer, desc->path)==0,
           "GetPath returned '%s' instead of '%s'\n",
           buffer, desc->path);
    }
    if (desc->pidl)
    {
        LPITEMIDLIST pidl=NULL;
        r = IShellLinkA_GetIDList(sl, &pidl);
        lok(r == S_OK, "GetIDList failed (0x%08x)\n", r);
        lok_todo_2(0x8, pILIsEqual(pidl, desc->pidl),
           "GetIDList returned an incorrect pidl\n");
    }
    if (desc->showcmd)
    {
        int i=0xdeadbeef;
        r = IShellLinkA_GetShowCmd(sl, &i);
        lok(r == S_OK, "GetShowCmd failed (0x%08x)\n", r);
        lok_todo_4(0x10, i==desc->showcmd,
           "GetShowCmd returned 0x%0x instead of 0x%0x\n",
           i, desc->showcmd);
    }
    if (desc->icon)
    {
        int i=0xdeadbeef;
        strcpy(buffer,"garbage");
        r = IShellLinkA_GetIconLocation(sl, buffer, sizeof(buffer), &i);
        lok(r == S_OK, "GetIconLocation failed (0x%08x)\n", r);
        lok_todo_4(0x20, lstrcmpiA(buffer, desc->icon)==0,
           "GetIconLocation returned '%s' instead of '%s'\n",
           buffer, desc->icon);
        lok_todo_4(0x20, i==desc->icon_id,
           "GetIconLocation returned 0x%0x instead of 0x%0x\n",
           i, desc->icon_id);
    }
    if (desc->hotkey)
    {
        WORD i=0xbeef;
        r = IShellLinkA_GetHotkey(sl, &i);
        lok(r == S_OK, "GetHotkey failed (0x%08x)\n", r);
        lok_todo_4(0x40, i==desc->hotkey,
           "GetHotkey returned 0x%04x instead of 0x%04x\n",
           i, desc->hotkey);
    }

    IShellLinkA_Release(sl);
}

static void test_load_save(void)
{
    WCHAR lnkfile[MAX_PATH];
    char lnkfileA[MAX_PATH];
    static const char lnkfileA_name[] = "\\test.lnk";

    lnk_desc_t desc;
    char mypath[MAX_PATH];
    char mydir[MAX_PATH];
    char realpath[MAX_PATH];
    char* p;
    HANDLE hf;
    DWORD r;

    if (!pGetLongPathNameA)
    {
        win_skip("GetLongPathNameA is not available\n");
        return;
    }

    /* Don't used a fixed path for the test.lnk file */
    GetTempPathA(MAX_PATH, lnkfileA);
    lstrcatA(lnkfileA, lnkfileA_name);
    MultiByteToWideChar(CP_ACP, 0, lnkfileA, -1, lnkfile, MAX_PATH);

    /* Save an empty .lnk file */
    memset(&desc, 0, sizeof(desc));
    create_lnk(lnkfile, &desc, 0);

    /* It should come back as a bunch of empty strings */
    desc.description="";
    desc.workdir="";
    desc.path="";
    desc.arguments="";
    desc.icon="";
    check_lnk(lnkfile, &desc, 0x0);

    /* Point a .lnk file to nonexistent files */
    desc.description="";
    desc.workdir="c:\\Nonexitent\\work\\directory";
    desc.path="c:\\nonexistent\\path";
    desc.pidl=NULL;
    desc.arguments="";
    desc.showcmd=0;
    desc.icon="c:\\nonexistent\\icon\\file";
    desc.icon_id=1234;
    desc.hotkey=0;
    create_lnk(lnkfile, &desc, 0);
    check_lnk(lnkfile, &desc, 0x0);

    r=GetModuleFileNameA(NULL, mypath, sizeof(mypath));
    ok(r<sizeof(mypath), "GetModuleFileName failed (%d)\n", r);
    strcpy(mydir, mypath);
    p=strrchr(mydir, '\\');
    if (p)
        *p='\0';

    /* IShellLink returns path in long form */
    if (!pGetLongPathNameA(mypath, realpath, MAX_PATH)) strcpy( realpath, mypath );

    /* Overwrite the existing lnk file and point it to existing files */
    desc.description="test 2";
    desc.workdir=mydir;
    desc.path=realpath;
    desc.pidl=NULL;
    desc.arguments="/option1 /option2 \"Some string\"";
    desc.showcmd=SW_SHOWNORMAL;
    desc.icon=mypath;
    desc.icon_id=0;
    desc.hotkey=0x1234;
    create_lnk(lnkfile, &desc, 0);
    check_lnk(lnkfile, &desc, 0x0);

    /* Test omitting .exe from an absolute path */
    p=strrchr(realpath, '.');
    if (p)
        *p='\0';

    desc.description="absolute path without .exe";
    desc.workdir=mydir;
    desc.path=realpath;
    desc.pidl=NULL;
    desc.arguments="/option1 /option2 \"Some string\"";
    desc.showcmd=SW_SHOWNORMAL;
    desc.icon=mypath;
    desc.icon_id=0;
    desc.hotkey=0x1234;
    create_lnk(lnkfile, &desc, 0);
    strcat(realpath, ".exe");
    check_lnk(lnkfile, &desc, 0x4);

    /* Overwrite the existing lnk file and test link to a command on the path */
    desc.description="command on path";
    desc.workdir=mypath;
    desc.path="rundll32.exe";
    desc.pidl=NULL;
    desc.arguments="/option1 /option2 \"Some string\"";
    desc.showcmd=SW_SHOWNORMAL;
    desc.icon=mypath;
    desc.icon_id=0;
    desc.hotkey=0x1234;
    create_lnk(lnkfile, &desc, 0);
    /* Check that link is created to proper location */
    SearchPathA( NULL, desc.path, NULL, MAX_PATH, realpath, NULL);
    desc.path=realpath;
    check_lnk(lnkfile, &desc, 0x0);

    /* Test omitting .exe from a command on the path */
    desc.description="command on path without .exe";
    desc.workdir=mypath;
    desc.path="rundll32";
    desc.pidl=NULL;
    desc.arguments="/option1 /option2 \"Some string\"";
    desc.showcmd=SW_SHOWNORMAL;
    desc.icon=mypath;
    desc.icon_id=0;
    desc.hotkey=0x1234;
    create_lnk(lnkfile, &desc, 0);
    /* Check that link is created to proper location */
    SearchPathA( NULL, "rundll32", NULL, MAX_PATH, realpath, NULL);
    desc.path=realpath;
    check_lnk(lnkfile, &desc, 0x4);

    /* Create a temporary non-executable file */
    r=GetTempPathA(sizeof(mypath), mypath);
    ok(r<sizeof(mypath), "GetTempPath failed (%d), err %d\n", r, GetLastError());
    r=pGetLongPathNameA(mypath, mydir, sizeof(mydir));
    ok(r<sizeof(mydir), "GetLongPathName failed (%d), err %d\n", r, GetLastError());
    p=strrchr(mydir, '\\');
    if (p)
        *p='\0';

    strcpy(mypath, mydir);
    strcat(mypath, "\\test.txt");
    hf = CreateFileA(mypath, GENERIC_WRITE, 0, NULL,
                     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    CloseHandle(hf);

    /* Overwrite the existing lnk file and test link to an existing non-executable file */
    desc.description="non-executable file";
    desc.workdir=mydir;
    desc.path=mypath;
    desc.pidl=NULL;
    desc.arguments="";
    desc.showcmd=SW_SHOWNORMAL;
    desc.icon=mypath;
    desc.icon_id=0;
    desc.hotkey=0x1234;
    create_lnk(lnkfile, &desc, 0);
    check_lnk(lnkfile, &desc, 0x0);

    r=pGetShortPathNameA(mydir, mypath, sizeof(mypath));
    ok(r<sizeof(mypath), "GetShortPathName failed (%d), err %d\n", r, GetLastError());

    strcpy(realpath, mypath);
    strcat(realpath, "\\test.txt");
    strcat(mypath, "\\\\test.txt");

    /* Overwrite the existing lnk file and test link to a short path with double backslashes */
    desc.description="non-executable file";
    desc.workdir=mydir;
    desc.path=mypath;
    desc.pidl=NULL;
    desc.arguments="";
    desc.showcmd=SW_SHOWNORMAL;
    desc.icon=mypath;
    desc.icon_id=0;
    desc.hotkey=0x1234;
    create_lnk(lnkfile, &desc, 0);
    desc.path=realpath;
    check_lnk(lnkfile, &desc, 0x0);

    r = DeleteFileA(mypath);
    ok(r, "failed to delete file %s (%d)\n", mypath, GetLastError());

    /* Create a temporary .bat file */
    strcpy(mypath, mydir);
    strcat(mypath, "\\test.bat");
    hf = CreateFileA(mypath, GENERIC_WRITE, 0, NULL,
                     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    CloseHandle(hf);

    strcpy(realpath, mypath);

    p=strrchr(mypath, '.');
    if (p)
        *p='\0';

    /* Try linking to the .bat file without the extension */
    desc.description="batch file";
    desc.workdir=mydir;
    desc.path=mypath;
    desc.pidl=NULL;
    desc.arguments="";
    desc.showcmd=SW_SHOWNORMAL;
    desc.icon=mypath;
    desc.icon_id=0;
    desc.hotkey=0x1234;
    create_lnk(lnkfile, &desc, 0);
    desc.path = realpath;
    check_lnk(lnkfile, &desc, 0x4);

    r = DeleteFileA(realpath);
    ok(r, "failed to delete file %s (%d)\n", realpath, GetLastError());

    /* FIXME: Also test saving a .lnk pointing to a pidl that cannot be
     * represented as a path.
     */

    /* DeleteFileW is not implemented on Win9x */
    r=DeleteFileA(lnkfileA);
    ok(r, "failed to delete link '%s' (%d)\n", lnkfileA, GetLastError());
}

static void test_datalink(void)
{
    static const WCHAR lnk[] = {
      ':',':','{','9','d','b','1','1','8','6','e','-','4','0','d','f','-','1',
      '1','d','1','-','a','a','8','c','-','0','0','c','0','4','f','b','6','7',
      '8','6','3','}',':','2','6',',','!','!','g','x','s','f','(','N','g',']',
      'q','F','`','H','{','L','s','A','C','C','E','S','S','F','i','l','e','s',
      '>','p','l','T',']','j','I','{','j','f','(','=','1','&','L','[','-','8',
      '1','-',']',':',':',0 };
    static const WCHAR comp[] = {
      '2','6',',','!','!','g','x','s','f','(','N','g',']','q','F','`','H','{',
      'L','s','A','C','C','E','S','S','F','i','l','e','s','>','p','l','T',']',
      'j','I','{','j','f','(','=','1','&','L','[','-','8','1','-',']',0 };
    IShellLinkDataList *dl = NULL;
    IShellLinkW *sl = NULL;
    HRESULT r;
    DWORD flags = 0;
    EXP_DARWIN_LINK *dar;

    r = CoCreateInstance( &CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                            &IID_IShellLinkW, (LPVOID*)&sl );
    ok( r == S_OK ||
        broken(r == E_NOINTERFACE), /* Win9x */
        "CoCreateInstance failed (0x%08x)\n", r);
    if (!sl)
    {
        win_skip("no shelllink\n");
        return;
    }

    r = IShellLinkW_QueryInterface( sl, &_IID_IShellLinkDataList, (LPVOID*) &dl );
    ok( r == S_OK ||
        broken(r == E_NOINTERFACE), /* NT4 */
        "IShellLinkW_QueryInterface failed (0x%08x)\n", r);

    if (!dl)
    {
        win_skip("no datalink interface\n");
        IShellLinkW_Release( sl );
        return;
    }

    flags = 0;
    r = IShellLinkDataList_GetFlags( dl, &flags );
    ok( r == S_OK, "GetFlags failed\n");
    ok( flags == 0, "GetFlags returned wrong flags\n");

    dar = (void*)-1;
    r = IShellLinkDataList_CopyDataBlock( dl, EXP_DARWIN_ID_SIG, (LPVOID*) &dar );
    ok( r == E_FAIL, "CopyDataBlock failed\n");
    ok( dar == NULL, "should be null\n");

    if (!pGetLongPathNameA /* NT4 */)
        skip("SetPath with NULL parameter crashes on NT4\n");
    else
    {
        r = IShellLinkW_SetPath(sl, NULL);
        ok(r == E_INVALIDARG, "SetPath returned wrong error (0x%08x)\n", r);
    }

    r = IShellLinkW_SetPath(sl, lnk);
    ok(r == S_OK, "SetPath failed\n");

if (0)
{
    /* the following crashes */
    IShellLinkDataList_GetFlags( dl, NULL );
}

    flags = 0;
    r = IShellLinkDataList_GetFlags( dl, &flags );
    ok( r == S_OK, "GetFlags failed\n");
    /* SLDF_HAS_LOGO3ID is no longer supported on Vista+, filter it out */
    ok( (flags & (~ SLDF_HAS_LOGO3ID)) == SLDF_HAS_DARWINID,
        "GetFlags returned wrong flags\n");

    dar = NULL;
    r = IShellLinkDataList_CopyDataBlock( dl, EXP_DARWIN_ID_SIG, (LPVOID*) &dar );
    ok( r == S_OK, "CopyDataBlock failed\n");

    ok( dar && ((DATABLOCK_HEADER*)dar)->dwSignature == EXP_DARWIN_ID_SIG, "signature wrong\n");
    ok( dar && 0==lstrcmpW(dar->szwDarwinID, comp ), "signature wrong\n");

    LocalFree( dar );

    IShellLinkDataList_Release( dl );
    IShellLinkW_Release( sl );
}

static void test_shdefextracticon(void)
{
    HICON hiconlarge=NULL, hiconsmall=NULL;
    HRESULT res;

    if (!pSHDefExtractIconA)
    {
        win_skip("SHDefExtractIconA is unavailable\n");
        return;
    }

    res = pSHDefExtractIconA("shell32.dll", 0, 0, &hiconlarge, &hiconsmall, MAKELONG(16,24));
    ok(SUCCEEDED(res), "SHDefExtractIconA failed, res=%x\n", res);
    ok(hiconlarge != NULL, "got null hiconlarge\n");
    ok(hiconsmall != NULL, "got null hiconsmall\n");
    DestroyIcon(hiconlarge);
    DestroyIcon(hiconsmall);

    hiconsmall = NULL;
    res = pSHDefExtractIconA("shell32.dll", 0, 0, NULL, &hiconsmall, MAKELONG(16,24));
    ok(SUCCEEDED(res), "SHDefExtractIconA failed, res=%x\n", res);
    ok(hiconsmall != NULL, "got null hiconsmall\n");
    DestroyIcon(hiconsmall);

    res = pSHDefExtractIconA("shell32.dll", 0, 0, NULL, NULL, MAKELONG(16,24));
    ok(SUCCEEDED(res), "SHDefExtractIconA failed, res=%x\n", res);
}

static void test_GetIconLocation(void)
{
    IShellLinkA *sl;
    const char *str;
    char buffer[INFOTIPSIZE], mypath[MAX_PATH];
    int i;
    HRESULT r;
    LPITEMIDLIST pidl;

    r = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
            &IID_IShellLinkA, (LPVOID*)&sl);
    ok(r == S_OK, "no IID_IShellLinkA (0x%08x)\n", r);
    if(r != S_OK)
        return;

    i = 0xdeadbeef;
    strcpy(buffer, "garbage");
    r = IShellLinkA_GetIconLocation(sl, buffer, sizeof(buffer), &i);
    ok(r == S_OK, "GetIconLocation failed (0x%08x)\n", r);
    ok(*buffer == '\0', "GetIconLocation returned '%s'\n", buffer);
    ok(i == 0, "GetIconLocation returned %d\n", i);

    str = "c:\\some\\path";
    r = IShellLinkA_SetPath(sl, str);
    ok(r == S_FALSE || r == S_OK, "SetPath failed (0x%08x)\n", r);

    i = 0xdeadbeef;
    strcpy(buffer, "garbage");
    r = IShellLinkA_GetIconLocation(sl, buffer, sizeof(buffer), &i);
    ok(r == S_OK, "GetIconLocation failed (0x%08x)\n", r);
    ok(*buffer == '\0', "GetIconLocation returned '%s'\n", buffer);
    ok(i == 0, "GetIconLocation returned %d\n", i);

    GetWindowsDirectoryA(mypath, sizeof(mypath) - 12);
    strcat(mypath, "\\regedit.exe");
    pidl = path_to_pidl(mypath);
    r = IShellLinkA_SetIDList(sl, pidl);
    ok(r == S_OK, "SetPath failed (0x%08x)\n", r);
    pILFree(pidl);

    i = 0xdeadbeef;
    strcpy(buffer, "garbage");
    r = IShellLinkA_GetIconLocation(sl, buffer, sizeof(buffer), &i);
    ok(r == S_OK, "GetIconLocation failed (0x%08x)\n", r);
    ok(*buffer == '\0', "GetIconLocation returned '%s'\n", buffer);
    ok(i == 0, "GetIconLocation returned %d\n", i);

    str = "c:\\nonexistent\\file";
    r = IShellLinkA_SetIconLocation(sl, str, 0xbabecafe);
    ok(r == S_OK, "SetIconLocation failed (0x%08x)\n", r);

    i = 0xdeadbeef;
    r = IShellLinkA_GetIconLocation(sl, buffer, sizeof(buffer), &i);
    ok(r == S_OK, "GetIconLocation failed (0x%08x)\n", r);
    ok(lstrcmpiA(buffer,str) == 0, "GetIconLocation returned '%s'\n", buffer);
    ok(i == 0xbabecafe, "GetIconLocation returned %d'\n", i);

    IShellLinkA_Release(sl);
}

static void test_SHGetStockIconInfo(void)
{
    BYTE buffer[sizeof(SHSTOCKICONINFO) + 16];
    SHSTOCKICONINFO *sii = (SHSTOCKICONINFO *) buffer;
    HRESULT hr;
    INT i;

    /* not present before vista */
    if (!pSHGetStockIconInfo)
    {
        win_skip("SHGetStockIconInfo not available\n");
        return;
    }

    /* negative values are handled */
    memset(buffer, '#', sizeof(buffer));
    sii->cbSize = sizeof(SHSTOCKICONINFO);
    hr = pSHGetStockIconInfo(SIID_INVALID, SHGSI_ICONLOCATION, sii);
    ok(hr == E_INVALIDARG, "-1: got 0x%x (expected E_INVALIDARG)\n", hr);

    /* max. id for vista is 140 (no definition exists for this value) */
    for (i = SIID_DOCNOASSOC; i <= SIID_CLUSTEREDDRIVE; i++)
    {
        memset(buffer, '#', sizeof(buffer));
        sii->cbSize = sizeof(SHSTOCKICONINFO);
        hr = pSHGetStockIconInfo(i, SHGSI_ICONLOCATION, sii);

        ok(hr == S_OK,
            "%3d: got 0x%x, iSysImageIndex: 0x%x, iIcon: 0x%x (expected S_OK)\n",
            i, hr, sii->iSysImageIndex, sii->iIcon);

        if ((hr == S_OK) && (winetest_debug > 1))
            trace("%3d: got iSysImageIndex %3d, iIcon %3d and %s\n", i, sii->iSysImageIndex,
                  sii->iIcon, wine_dbgstr_w(sii->szPath));
    }

    /* test invalid icons indices that are invalid for all platforms */
    for (i = SIID_MAX_ICONS; i < (SIID_MAX_ICONS + 25) ; i++)
    {
        memset(buffer, '#', sizeof(buffer));
        sii->cbSize = sizeof(SHSTOCKICONINFO);
        hr = pSHGetStockIconInfo(i, SHGSI_ICONLOCATION, sii);
        ok(hr == E_INVALIDARG, "%3d: got 0x%x (expected E_INVALIDARG)\n", i, hr);
    todo_wine {
        ok(sii->iSysImageIndex == -1, "%d: got iSysImageIndex %d\n", i, sii->iSysImageIndex);
        ok(sii->iIcon == -1, "%d: got iIcon %d\n", i, sii->iIcon);
    }
    }

    /* test more returned SHSTOCKICONINFO elements without extra flags */
    memset(buffer, '#', sizeof(buffer));
    sii->cbSize = sizeof(SHSTOCKICONINFO);
    hr = pSHGetStockIconInfo(SIID_FOLDER, SHGSI_ICONLOCATION, sii);
    ok(hr == S_OK, "got 0x%x (expected S_OK)\n", hr);
    ok(!sii->hIcon, "got %p (expected NULL)\n", sii->hIcon);
    ok(sii->iSysImageIndex == -1, "got %d (expected -1)\n", sii->iSysImageIndex);

    /* the exact size is required of the struct */
    memset(buffer, '#', sizeof(buffer));
    sii->cbSize = sizeof(SHSTOCKICONINFO) + 2;
    hr = pSHGetStockIconInfo(SIID_FOLDER, SHGSI_ICONLOCATION, sii);
    ok(hr == E_INVALIDARG, "+2: got 0x%x, iSysImageIndex: 0x%x, iIcon: 0x%x\n", hr, sii->iSysImageIndex, sii->iIcon);

    memset(buffer, '#', sizeof(buffer));
    sii->cbSize = sizeof(SHSTOCKICONINFO) + 1;
    hr = pSHGetStockIconInfo(SIID_FOLDER, SHGSI_ICONLOCATION, sii);
    ok(hr == E_INVALIDARG, "+1: got 0x%x, iSysImageIndex: 0x%x, iIcon: 0x%x\n", hr, sii->iSysImageIndex, sii->iIcon);

    memset(buffer, '#', sizeof(buffer));
    sii->cbSize = sizeof(SHSTOCKICONINFO) - 1;
    hr = pSHGetStockIconInfo(SIID_FOLDER, SHGSI_ICONLOCATION, sii);
    ok(hr == E_INVALIDARG, "-1: got 0x%x, iSysImageIndex: 0x%x, iIcon: 0x%x\n", hr, sii->iSysImageIndex, sii->iIcon);

    memset(buffer, '#', sizeof(buffer));
    sii->cbSize = sizeof(SHSTOCKICONINFO) - 2;
    hr = pSHGetStockIconInfo(SIID_FOLDER, SHGSI_ICONLOCATION, sii);
    ok(hr == E_INVALIDARG, "-2: got 0x%x, iSysImageIndex: 0x%x, iIcon: 0x%x\n", hr, sii->iSysImageIndex, sii->iIcon);

    /* there is a NULL check for the struct  */
    hr = pSHGetStockIconInfo(SIID_FOLDER, SHGSI_ICONLOCATION, NULL);
    ok(hr == E_INVALIDARG, "NULL: got 0x%x\n", hr);
}

static void test_SHExtractIcons(void)
{
    static const WCHAR notepadW[] = {'n','o','t','e','p','a','d','.','e','x','e',0};
    static const WCHAR shell32W[] = {'s','h','e','l','l','3','2','.','d','l','l',0};
    static const WCHAR emptyW[] = {0};
    UINT ret, ret2;
    HICON icons[256];
    UINT ids[256], i;

    if (!pSHExtractIconsW)
    {
        win_skip("SHExtractIconsW not available\n");
        return;
    }

    ret = pSHExtractIconsW(emptyW, 0, 16, 16, icons, ids, 1, 0);
    ok(ret == ~0u, "got %u\n", ret);

    ret = pSHExtractIconsW(notepadW, 0, 16, 16, NULL, NULL, 1, 0);
    ok(ret == 1 || broken(ret == 2) /* win2k */, "got %u\n", ret);

    icons[0] = (HICON)0xdeadbeef;
    ret = pSHExtractIconsW(notepadW, 0, 16, 16, icons, NULL, 1, 0);
    ok(ret == 1, "got %u\n", ret);
    ok(icons[0] != (HICON)0xdeadbeef, "icon not set\n");
    DestroyIcon(icons[0]);

    icons[0] = (HICON)0xdeadbeef;
    ids[0] = 0xdeadbeef;
    ret = pSHExtractIconsW(notepadW, 0, 16, 16, icons, ids, 1, 0);
    ok(ret == 1, "got %u\n", ret);
    ok(icons[0] != (HICON)0xdeadbeef, "icon not set\n");
    ok(ids[0] != 0xdeadbeef, "id not set\n");
    DestroyIcon(icons[0]);

    ret = pSHExtractIconsW(shell32W, 0, 16, 16, NULL, NULL, 0, 0);
    ret2 = pSHExtractIconsW(shell32W, 4, MAKELONG(32,16), MAKELONG(32,16), NULL, NULL, 256, 0);
    ok(ret && ret == ret2,
       "icon count should be independent of requested icon sizes and base icon index\n");

    ret = pSHExtractIconsW(shell32W, 0, 16, 16, icons, ids, 0, 0);
    ok(ret == ~0u || !ret /* < vista */, "got %u\n", ret);

    ret = pSHExtractIconsW(shell32W, 0, 16, 16, icons, ids, 3, 0);
    ok(ret == 3, "got %u\n", ret);
    for (i = 0; i < ret; i++) DestroyIcon(icons[i]);

    /* count must be a multiple of two when getting two sizes */
    ret = pSHExtractIconsW(shell32W, 0, MAKELONG(16,32), MAKELONG(16,32), icons, ids, 3, 0);
    ok(!ret /* vista */ || ret == 4, "got %u\n", ret);
    for (i = 0; i < ret; i++) DestroyIcon(icons[i]);

    ret = pSHExtractIconsW(shell32W, 0, MAKELONG(16,32), MAKELONG(16,32), icons, ids, 4, 0);
    ok(ret == 4, "got %u\n", ret);
    for (i = 0; i < ret; i++) DestroyIcon(icons[i]);
}

static void test_propertystore(void)
{
    IShellLinkA *linkA;
    IShellLinkW *linkW;
    IPropertyStore *ps;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                         &IID_IShellLinkA, (void**)&linkA);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IShellLinkA_QueryInterface(linkA, &IID_IShellLinkW, (void**)&linkW);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IShellLinkA_QueryInterface(linkA, &IID_IPropertyStore, (void**)&ps);
    if (hr == S_OK) {
        IPropertyStoreCache *pscache;

        IPropertyStore_Release(ps);

        hr = IShellLinkW_QueryInterface(linkW, &IID_IPropertyStore, (void**)&ps);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        hr = IPropertyStore_QueryInterface(ps, &IID_IPropertyStoreCache, (void**)&pscache);
        ok(hr == E_NOINTERFACE, "got 0x%08x\n", hr);

        IPropertyStore_Release(ps);
    }
    else
        win_skip("IShellLink doesn't support IPropertyStore.\n");

    IShellLinkA_Release(linkA);
    IShellLinkW_Release(linkW);
}

START_TEST(shelllink)
{
    HRESULT r;
    HMODULE hmod = GetModuleHandleA("shell32.dll");
    HMODULE hkernel32 = GetModuleHandleA("kernel32.dll");

    pILFree = (void *)GetProcAddress(hmod, (LPSTR)155);
    pILIsEqual = (void *)GetProcAddress(hmod, (LPSTR)21);
    pSHILCreateFromPath = (void *)GetProcAddress(hmod, (LPSTR)28);
    pSHDefExtractIconA = (void *)GetProcAddress(hmod, "SHDefExtractIconA");
    pSHGetStockIconInfo = (void *)GetProcAddress(hmod, "SHGetStockIconInfo");
    pGetLongPathNameA = (void *)GetProcAddress(hkernel32, "GetLongPathNameA");
    pGetShortPathNameA = (void *)GetProcAddress(hkernel32, "GetShortPathNameA");
    pSHExtractIconsW = (void *)GetProcAddress(hmod, "SHExtractIconsW");

    r = CoInitialize(NULL);
    ok(r == S_OK, "CoInitialize failed (0x%08x)\n", r);
    if (r != S_OK)
        return;

    test_get_set();
    test_load_save();
    test_datalink();
    test_shdefextracticon();
    test_GetIconLocation();
    test_SHGetStockIconInfo();
    test_SHExtractIcons();
    test_propertystore();

    CoUninitialize();
}
