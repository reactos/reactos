/*
 *
 * Copyright 2012 Alistair Leslie-Hughes
 * Copyright 2014 Dmitry Timoshkov
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
#include <limits.h>

#include "windows.h"
#include "ole2.h"
#include "olectl.h"
#include "oleauto.h"
#include "dispex.h"

#include "wine/test.h"

#include "initguid.h"
#include "scrrun.h"

static IFileSystem3 *fs3;

/* w2k and 2k3 error code. */
#define E_VAR_NOT_SET 0x800a005b

static inline ULONG get_refcount(IUnknown *iface)
{
    IUnknown_AddRef(iface);
    return IUnknown_Release(iface);
}

static const WCHAR crlfW[] = {'\r','\n',0};
static const char utf16bom[] = {0xff,0xfe,0};
static const WCHAR testfileW[] = {'t','e','s','t','.','t','x','t',0};

#define GET_REFCOUNT(iface) \
    get_refcount((IUnknown*)iface)

static inline void get_temp_path(const WCHAR *prefix, WCHAR *path)
{
    WCHAR buffW[MAX_PATH];

    GetTempPathW(MAX_PATH, buffW);
    GetTempFileNameW(buffW, prefix, 0, path);
    DeleteFileW(path);
}

static IDrive *get_fixed_drive(void)
{
    IDriveCollection *drives;
    IEnumVARIANT *iter;
    IDrive *drive;
    HRESULT hr;

    hr = IFileSystem3_get_Drives(fs3, &drives);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDriveCollection_get__NewEnum(drives, (IUnknown**)&iter);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDriveCollection_Release(drives);

    while (1) {
        DriveTypeConst type;
        VARIANT var;

        hr = IEnumVARIANT_Next(iter, 1, &var, NULL);
        if (hr == S_FALSE) {
            drive = NULL;
            break;
        }
        ok(hr == S_OK, "got 0x%08x\n", hr);

        hr = IDispatch_QueryInterface(V_DISPATCH(&var), &IID_IDrive, (void**)&drive);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        VariantClear(&var);

        hr = IDrive_get_DriveType(drive, &type);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        if (type == Fixed)
            break;

        IDrive_Release(drive);
    }

    IEnumVARIANT_Release(iter);
    return drive;
}

#define test_provideclassinfo(a, b) _test_provideclassinfo((IDispatch*)a, b, __LINE__)
static void _test_provideclassinfo(IDispatch *disp, const GUID *guid, int line)
{
    IProvideClassInfo *classinfo;
    TYPEATTR *attr;
    ITypeInfo *ti;
    IUnknown *unk;
    HRESULT hr;

    hr = IDispatch_QueryInterface(disp, &IID_IProvideClassInfo, (void **)&classinfo);
    ok_(__FILE__,line) (hr == S_OK, "Failed to get IProvideClassInfo, %#x.\n", hr);

    hr = IProvideClassInfo_GetClassInfo(classinfo, &ti);
    ok_(__FILE__,line) (hr == S_OK, "GetClassInfo() failed, %#x.\n", hr);

    hr = ITypeInfo_GetTypeAttr(ti, &attr);
    ok_(__FILE__,line) (hr == S_OK, "GetTypeAttr() failed, %#x.\n", hr);

    ok_(__FILE__,line) (IsEqualGUID(&attr->guid, guid), "Unexpected typeinfo %s, expected %s\n", wine_dbgstr_guid(&attr->guid),
        wine_dbgstr_guid(guid));

    hr = IProvideClassInfo_QueryInterface(classinfo, &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Failed to QI for IUnknown.\n");
    ok(unk == (IUnknown *)disp, "Got unk %p, original %p.\n", unk, disp);
    IUnknown_Release(unk);

    IProvideClassInfo_Release(classinfo);
    ITypeInfo_ReleaseTypeAttr(ti, attr);
    ITypeInfo_Release(ti);
}

static void test_interfaces(void)
{
    static const WCHAR nonexistent_dirW[] = {
        'c', ':', '\\', 'N', 'o', 'n', 'e', 'x', 'i', 's', 't', 'e', 'n', 't', 0};
    static const WCHAR pathW[] = {'p','a','t','h',0};
    static const WCHAR file_kernel32W[] = {
        '\\', 'k', 'e', 'r', 'n', 'e', 'l', '3', '2', '.', 'd', 'l', 'l', 0};
    HRESULT hr;
    IDispatch *disp;
    IDispatchEx *dispex;
    IObjectWithSite *site;
    VARIANT_BOOL b;
    BSTR path;
    WCHAR windows_path[MAX_PATH];
    WCHAR file_path[MAX_PATH];

    IFileSystem3_QueryInterface(fs3, &IID_IDispatch, (void**)&disp);

    GetSystemDirectoryW(windows_path, MAX_PATH);
    lstrcpyW(file_path, windows_path);
    lstrcatW(file_path, file_kernel32W);

    test_provideclassinfo(disp, &CLSID_FileSystemObject);

    hr = IDispatch_QueryInterface(disp, &IID_IObjectWithSite, (void**)&site);
    ok(hr == E_NOINTERFACE, "got 0x%08x, expected 0x%08x\n", hr, E_NOINTERFACE);

    hr = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    ok(hr == E_NOINTERFACE, "got 0x%08x, expected 0x%08x\n", hr, E_NOINTERFACE);

    b = VARIANT_TRUE;
    hr = IFileSystem3_FileExists(fs3, NULL, &b);
    ok(hr == S_OK, "got 0x%08x, expected 0x%08x\n", hr, S_OK);
    ok(b == VARIANT_FALSE, "got %x\n", b);

    hr = IFileSystem3_FileExists(fs3, NULL, NULL);
    ok(hr == E_POINTER, "got 0x%08x, expected 0x%08x\n", hr, E_POINTER);

    path = SysAllocString(pathW);
    b = VARIANT_TRUE;
    hr = IFileSystem3_FileExists(fs3, path, &b);
    ok(hr == S_OK, "got 0x%08x, expected 0x%08x\n", hr, S_OK);
    ok(b == VARIANT_FALSE, "got %x\n", b);
    SysFreeString(path);

    path = SysAllocString(file_path);
    b = VARIANT_FALSE;
    hr = IFileSystem3_FileExists(fs3, path, &b);
    ok(hr == S_OK, "got 0x%08x, expected 0x%08x\n", hr, S_OK);
    ok(b == VARIANT_TRUE, "got %x\n", b);
    SysFreeString(path);

    path = SysAllocString(windows_path);
    b = VARIANT_TRUE;
    hr = IFileSystem3_FileExists(fs3, path, &b);
    ok(hr == S_OK, "got 0x%08x, expected 0x%08x\n", hr, S_OK);
    ok(b == VARIANT_FALSE, "got %x\n", b);
    SysFreeString(path);

    /* Folder Exists */
    hr = IFileSystem3_FolderExists(fs3, NULL, NULL);
    ok(hr == E_POINTER, "got 0x%08x, expected 0x%08x\n", hr, E_POINTER);

    path = SysAllocString(windows_path);
    hr = IFileSystem3_FolderExists(fs3, path, &b);
    ok(hr == S_OK, "got 0x%08x, expected 0x%08x\n", hr, S_OK);
    ok(b == VARIANT_TRUE, "Folder doesn't exists\n");
    SysFreeString(path);

    path = SysAllocString(nonexistent_dirW);
    hr = IFileSystem3_FolderExists(fs3, path, &b);
    ok(hr == S_OK, "got 0x%08x, expected 0x%08x\n", hr, S_OK);
    ok(b == VARIANT_FALSE, "Folder exists\n");
    SysFreeString(path);

    path = SysAllocString(file_path);
    hr = IFileSystem3_FolderExists(fs3, path, &b);
    ok(hr == S_OK, "got 0x%08x, expected 0x%08x\n", hr, S_OK);
    ok(b == VARIANT_FALSE, "Folder exists\n");
    SysFreeString(path);

    IDispatch_Release(disp);
}

static void test_createfolder(void)
{
    WCHAR buffW[MAX_PATH];
    HRESULT hr;
    BSTR path;
    IFolder *folder;
    BOOL ret;

    get_temp_path(NULL, buffW);
    ret = CreateDirectoryW(buffW, NULL);
    ok(ret, "got %d, %d\n", ret, GetLastError());

    /* create existing directory */
    path = SysAllocString(buffW);
    folder = (void*)0xdeabeef;
    hr = IFileSystem3_CreateFolder(fs3, path, &folder);
    ok(hr == CTL_E_FILEALREADYEXISTS, "got 0x%08x\n", hr);
    ok(folder == NULL, "got %p\n", folder);
    SysFreeString(path);
    RemoveDirectoryW(buffW);
}

static void test_textstream(void)
{
    ITextStream *stream;
    VARIANT_BOOL b;
    DWORD written;
    HANDLE file;
    HRESULT hr;
    BSTR name, data;
    BOOL ret;

    file = CreateFileW(testfileW, GENERIC_READ, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    CloseHandle(file);

    name = SysAllocString(testfileW);
    b = VARIANT_FALSE;
    hr = IFileSystem3_FileExists(fs3, name, &b);
    ok(hr == S_OK, "got 0x%08x, expected 0x%08x\n", hr, S_OK);
    ok(b == VARIANT_TRUE, "got %x\n", b);

    /* different mode combinations */
    hr = IFileSystem3_OpenTextFile(fs3, name, ForWriting | ForAppending, VARIANT_FALSE, TristateFalse, &stream);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IFileSystem3_OpenTextFile(fs3, name, ForReading | ForAppending, VARIANT_FALSE, TristateFalse, &stream);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IFileSystem3_OpenTextFile(fs3, name, ForWriting | ForReading, VARIANT_FALSE, TristateFalse, &stream);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IFileSystem3_OpenTextFile(fs3, name, ForAppending, VARIANT_FALSE, TristateFalse, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = ITextStream_Read(stream, 1, &data);
    ok(hr == CTL_E_BADFILEMODE, "got 0x%08x\n", hr);
    ITextStream_Release(stream);

    hr = IFileSystem3_OpenTextFile(fs3, name, ForWriting, VARIANT_FALSE, TristateFalse, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = ITextStream_Read(stream, 1, &data);
    ok(hr == CTL_E_BADFILEMODE, "got 0x%08x\n", hr);
    ITextStream_Release(stream);

    hr = IFileSystem3_OpenTextFile(fs3, name, ForReading, VARIANT_FALSE, TristateFalse, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* try to write when open for reading */
    hr = ITextStream_WriteLine(stream, name);
    ok(hr == CTL_E_BADFILEMODE, "got 0x%08x\n", hr);

    hr = ITextStream_Write(stream, name);
    ok(hr == CTL_E_BADFILEMODE, "got 0x%08x\n", hr);

    hr = ITextStream_get_AtEndOfStream(stream, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    b = 10;
    hr = ITextStream_get_AtEndOfStream(stream, &b);
    ok(hr == S_OK || broken(hr == S_FALSE), "got 0x%08x\n", hr);
    ok(b == VARIANT_TRUE, "got 0x%x\n", b);

    ITextStream_Release(stream);

    hr = IFileSystem3_OpenTextFile(fs3, name, ForWriting, VARIANT_FALSE, TristateFalse, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    b = 10;
    hr = ITextStream_get_AtEndOfStream(stream, &b);
    ok(hr == CTL_E_BADFILEMODE, "got 0x%08x\n", hr);
    ok(b == VARIANT_TRUE || broken(b == 10), "got 0x%x\n", b);

    b = 10;
    hr = ITextStream_get_AtEndOfLine(stream, &b);
todo_wine {
    ok(hr == CTL_E_BADFILEMODE, "got 0x%08x\n", hr);
    ok(b == VARIANT_FALSE || broken(b == 10), "got 0x%x\n", b);
}
    hr = ITextStream_Read(stream, 1, &data);
    ok(hr == CTL_E_BADFILEMODE, "got 0x%08x\n", hr);

    hr = ITextStream_ReadLine(stream, &data);
    ok(hr == CTL_E_BADFILEMODE, "got 0x%08x\n", hr);

    hr = ITextStream_ReadAll(stream, &data);
    ok(hr == CTL_E_BADFILEMODE, "got 0x%08x\n", hr);

    ITextStream_Release(stream);

    hr = IFileSystem3_OpenTextFile(fs3, name, ForAppending, VARIANT_FALSE, TristateFalse, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    b = 10;
    hr = ITextStream_get_AtEndOfStream(stream, &b);
    ok(hr == CTL_E_BADFILEMODE, "got 0x%08x\n", hr);
    ok(b == VARIANT_TRUE || broken(b == 10), "got 0x%x\n", b);

    b = 10;
    hr = ITextStream_get_AtEndOfLine(stream, &b);
todo_wine {
    ok(hr == CTL_E_BADFILEMODE, "got 0x%08x\n", hr);
    ok(b == VARIANT_FALSE || broken(b == 10), "got 0x%x\n", b);
}
    hr = ITextStream_Read(stream, 1, &data);
    ok(hr == CTL_E_BADFILEMODE, "got 0x%08x\n", hr);

    hr = ITextStream_ReadLine(stream, &data);
    ok(hr == CTL_E_BADFILEMODE, "got 0x%08x\n", hr);

    hr = ITextStream_ReadAll(stream, &data);
    ok(hr == CTL_E_BADFILEMODE, "got 0x%08x\n", hr);

    ITextStream_Release(stream);

    /* now with non-empty file */
    file = CreateFileW(testfileW, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ret = WriteFile(file, testfileW, sizeof(testfileW), &written, NULL);
    ok(ret && written == sizeof(testfileW), "got %d\n", ret);
    CloseHandle(file);

    hr = IFileSystem3_OpenTextFile(fs3, name, ForReading, VARIANT_FALSE, TristateFalse, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    b = 10;
    hr = ITextStream_get_AtEndOfStream(stream, &b);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(b == VARIANT_FALSE, "got 0x%x\n", b);
    ITextStream_Release(stream);

    SysFreeString(name);
    DeleteFileW(testfileW);
}

static void test_GetFileVersion(void)
{
    static const WCHAR k32W[] = {'\\','k','e','r','n','e','l','3','2','.','d','l','l',0};
    static const WCHAR k33W[] = {'\\','k','e','r','n','e','l','3','3','.','d','l','l',0};
    WCHAR pathW[MAX_PATH], filenameW[MAX_PATH];
    BSTR path, version;
    HRESULT hr;

    GetSystemDirectoryW(pathW, ARRAY_SIZE(pathW));

    lstrcpyW(filenameW, pathW);
    lstrcatW(filenameW, k32W);

    path = SysAllocString(filenameW);
    hr = IFileSystem3_GetFileVersion(fs3, path, &version);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(*version != 0, "got %s\n", wine_dbgstr_w(version));
    SysFreeString(version);
    SysFreeString(path);

    lstrcpyW(filenameW, pathW);
    lstrcatW(filenameW, k33W);

    path = SysAllocString(filenameW);
    version = (void*)0xdeadbeef;
    hr = IFileSystem3_GetFileVersion(fs3, path, &version);
    ok(broken(hr == S_OK) || hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "got 0x%08x\n", hr);
    if (hr == S_OK)
    {
        ok(*version == 0, "got %s\n", wine_dbgstr_w(version));
        SysFreeString(version);
    }
    else
        ok(version == (void*)0xdeadbeef, "got %p\n", version);
    SysFreeString(path);
}

static void test_GetParentFolderName(void)
{
    static const WCHAR path1[] = {'a',0};
    static const WCHAR path2[] = {'a','/','a','/','a',0};
    static const WCHAR path3[] = {'a','\\','a','\\','a',0};
    static const WCHAR path4[] = {'a','/','a','/','/','\\','\\',0};
    static const WCHAR path5[] = {'c',':','\\','\\','a',0};
    static const WCHAR path6[] = {'a','c',':','\\','a',0};
    static const WCHAR result2[] = {'a','/','a',0};
    static const WCHAR result3[] = {'a','\\','a',0};
    static const WCHAR result4[] = {'a',0};
    static const WCHAR result5[] = {'c',':','\\',0};
    static const WCHAR result6[] = {'a','c',':',0};

    static const struct {
        const WCHAR *path;
        const WCHAR *result;
    } tests[] = {
        {NULL, NULL},
        {path1, NULL},
        {path2, result2},
        {path3, result3},
        {path4, result4},
        {path5, result5},
        {path6, result6}
    };

    BSTR path, result;
    HRESULT hr;
    int i;

    hr = IFileSystem3_GetParentFolderName(fs3, NULL, NULL);
    ok(hr == E_POINTER, "GetParentFolderName returned %x, expected E_POINTER\n", hr);

    for(i=0; i < ARRAY_SIZE(tests); i++) {
        result = (BSTR)0xdeadbeef;
        path = tests[i].path ? SysAllocString(tests[i].path) : NULL;
        hr = IFileSystem3_GetParentFolderName(fs3, path, &result);
        ok(hr == S_OK, "%d) GetParentFolderName returned %x, expected S_OK\n", i, hr);
        if(!tests[i].result)
            ok(!result, "%d) result = %s\n", i, wine_dbgstr_w(result));
        else
            ok(!lstrcmpW(result, tests[i].result), "%d) result = %s\n", i, wine_dbgstr_w(result));
        SysFreeString(path);
        SysFreeString(result);
    }
}

static void test_GetFileName(void)
{
    static const WCHAR path1[] = {'a',0};
    static const WCHAR path2[] = {'a','/','a','.','b',0};
    static const WCHAR path3[] = {'a','\\',0};
    static const WCHAR path4[] = {'c',':',0};
    static const WCHAR path5[] = {'/','\\',0};
    static const WCHAR result2[] = {'a','.','b',0};
    static const WCHAR result3[] = {'a',0};

    static const struct {
        const WCHAR *path;
        const WCHAR *result;
    } tests[] = {
        {NULL, NULL},
        {path1, path1},
        {path2, result2},
        {path3, result3},
        {path4, NULL},
        {path5, NULL}
    };

    BSTR path, result;
    HRESULT hr;
    int i;

    hr = IFileSystem3_GetFileName(fs3, NULL, NULL);
    ok(hr == E_POINTER, "GetFileName returned %x, expected E_POINTER\n", hr);

    for(i=0; i < ARRAY_SIZE(tests); i++) {
        result = (BSTR)0xdeadbeef;
        path = tests[i].path ? SysAllocString(tests[i].path) : NULL;
        hr = IFileSystem3_GetFileName(fs3, path, &result);
        ok(hr == S_OK, "%d) GetFileName returned %x, expected S_OK\n", i, hr);
        if(!tests[i].result)
            ok(!result, "%d) result = %s\n", i, wine_dbgstr_w(result));
        else
            ok(!lstrcmpW(result, tests[i].result), "%d) result = %s\n", i, wine_dbgstr_w(result));
        SysFreeString(path);
        SysFreeString(result);
    }
}

static void test_GetBaseName(void)
{
    static const WCHAR path1[] = {'a',0};
    static const WCHAR path2[] = {'a','/','a','.','b','.','c',0};
    static const WCHAR path3[] = {'a','.','b','\\',0};
    static const WCHAR path4[] = {'c',':',0};
    static const WCHAR path5[] = {'/','\\',0};
    static const WCHAR path6[] = {'.','a',0};
    static const WCHAR result1[] = {'a',0};
    static const WCHAR result2[] = {'a','.','b',0};
    static const WCHAR result6[] = {0};

    static const struct {
        const WCHAR *path;
        const WCHAR *result;
    } tests[] = {
        {NULL, NULL},
        {path1, result1},
        {path2, result2},
        {path3, result1},
        {path4, NULL},
        {path5, NULL},
        {path6, result6}
    };

    BSTR path, result;
    HRESULT hr;
    int i;

    hr = IFileSystem3_GetBaseName(fs3, NULL, NULL);
    ok(hr == E_POINTER, "GetBaseName returned %x, expected E_POINTER\n", hr);

    for(i=0; i < ARRAY_SIZE(tests); i++) {
        result = (BSTR)0xdeadbeef;
        path = tests[i].path ? SysAllocString(tests[i].path) : NULL;
        hr = IFileSystem3_GetBaseName(fs3, path, &result);
        ok(hr == S_OK, "%d) GetBaseName returned %x, expected S_OK\n", i, hr);
        if(!tests[i].result)
            ok(!result, "%d) result = %s\n", i, wine_dbgstr_w(result));
        else
            ok(!lstrcmpW(result, tests[i].result), "%d) result = %s\n", i, wine_dbgstr_w(result));
        SysFreeString(path);
        SysFreeString(result);
    }
}

static void test_GetAbsolutePathName(void)
{
    static const WCHAR dir1[] = {'t','e','s','t','_','d','i','r','1',0};
    static const WCHAR dir2[] = {'t','e','s','t','_','d','i','r','2',0};
    static const WCHAR dir_match1[] = {'t','e','s','t','_','d','i','r','*',0};
    static const WCHAR dir_match2[] = {'t','e','s','t','_','d','i','*',0};
    static const WCHAR cur_dir[] = {'.',0};

    WIN32_FIND_DATAW fdata;
    HANDLE find;
    WCHAR buf[MAX_PATH], buf2[MAX_PATH];
    BSTR path, result;
    HRESULT hr;

    hr = IFileSystem3_GetAbsolutePathName(fs3, NULL, NULL);
    ok(hr == E_POINTER, "GetAbsolutePathName returned %x, expected E_POINTER\n", hr);

    hr = IFileSystem3_GetAbsolutePathName(fs3, NULL, &result);
    ok(hr == S_OK, "GetAbsolutePathName returned %x, expected S_OK\n", hr);
    GetFullPathNameW(cur_dir, MAX_PATH, buf, NULL);
    ok(!lstrcmpiW(buf, result), "result = %s, expected %s\n", wine_dbgstr_w(result), wine_dbgstr_w(buf));
    SysFreeString(result);

    find = FindFirstFileW(dir_match2, &fdata);
    if(find != INVALID_HANDLE_VALUE) {
        skip("GetAbsolutePathName tests\n");
        FindClose(find);
        return;
    }

    path = SysAllocString(dir_match1);
    hr = IFileSystem3_GetAbsolutePathName(fs3, path, &result);
    ok(hr == S_OK, "GetAbsolutePathName returned %x, expected S_OK\n", hr);
    GetFullPathNameW(dir_match1, MAX_PATH, buf2, NULL);
    ok(!lstrcmpiW(buf2, result), "result = %s, expected %s\n", wine_dbgstr_w(result), wine_dbgstr_w(buf2));
    SysFreeString(result);

    ok(CreateDirectoryW(dir1, NULL), "CreateDirectory(%s) failed\n", wine_dbgstr_w(dir1));
    hr = IFileSystem3_GetAbsolutePathName(fs3, path, &result);
    ok(hr == S_OK, "GetAbsolutePathName returned %x, expected S_OK\n", hr);
    GetFullPathNameW(dir1, MAX_PATH, buf, NULL);
    ok(!lstrcmpiW(buf, result) || broken(!lstrcmpiW(buf2, result)), "result = %s, expected %s\n",
                wine_dbgstr_w(result), wine_dbgstr_w(buf));
    SysFreeString(result);

    ok(CreateDirectoryW(dir2, NULL), "CreateDirectory(%s) failed\n", wine_dbgstr_w(dir2));
    hr = IFileSystem3_GetAbsolutePathName(fs3, path, &result);
    ok(hr == S_OK, "GetAbsolutePathName returned %x, expected S_OK\n", hr);
    if(!lstrcmpiW(buf, result) || !lstrcmpiW(buf2, result)) {
        ok(!lstrcmpiW(buf, result) || broken(!lstrcmpiW(buf2, result)), "result = %s, expected %s\n",
                wine_dbgstr_w(result), wine_dbgstr_w(buf));
    }else {
        GetFullPathNameW(dir2, MAX_PATH, buf, NULL);
        ok(!lstrcmpiW(buf, result), "result = %s, expected %s\n",
                wine_dbgstr_w(result), wine_dbgstr_w(buf));
    }
    SysFreeString(result);

    SysFreeString(path);
    path = SysAllocString(dir_match2);
    hr = IFileSystem3_GetAbsolutePathName(fs3, path, &result);
    ok(hr == S_OK, "GetAbsolutePathName returned %x, expected S_OK\n", hr);
    GetFullPathNameW(dir_match2, MAX_PATH, buf, NULL);
    ok(!lstrcmpiW(buf, result), "result = %s, expected %s\n", wine_dbgstr_w(result), wine_dbgstr_w(buf));
    SysFreeString(result);
    SysFreeString(path);

    RemoveDirectoryW(dir1);
    RemoveDirectoryW(dir2);
}

static void test_GetFile(void)
{
    static const WCHAR slW[] = {'\\',0};
    BSTR path, str;
    WCHAR pathW[MAX_PATH];
    FileAttribute fa;
    VARIANT size;
    DWORD gfa, new_gfa;
    IFile *file;
    HRESULT hr;
    HANDLE hf;
    BOOL ret;
    DATE date;

    get_temp_path(NULL, pathW);

    path = SysAllocString(pathW);
    hr = IFileSystem3_GetFile(fs3, path, NULL);
    ok(hr == E_POINTER, "GetFile returned %x, expected E_POINTER\n", hr);
    hr = IFileSystem3_GetFile(fs3, NULL, &file);
    ok(hr == E_INVALIDARG, "GetFile returned %x, expected E_INVALIDARG\n", hr);

    file = (IFile*)0xdeadbeef;
    hr = IFileSystem3_GetFile(fs3, path, &file);
    ok(!file, "file != NULL\n");
    ok(hr == CTL_E_FILENOTFOUND, "GetFile returned %x, expected CTL_E_FILENOTFOUND\n", hr);

    hf = CreateFileW(pathW, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_READONLY, NULL);
    if(hf == INVALID_HANDLE_VALUE) {
        skip("Can't create temporary file\n");
        SysFreeString(path);
        return;
    }
    CloseHandle(hf);

    hr = IFileSystem3_GetFile(fs3, path, &file);
    ok(hr == S_OK, "GetFile returned %x, expected S_OK\n", hr);

    hr = IFile_get_DateLastModified(file, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    date = 0.0;
    hr = IFile_get_DateLastModified(file, &date);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(date > 0.0, "got %f\n", date);

    hr = IFile_get_Path(file, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    hr = IFile_get_Path(file, &str);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(!lstrcmpiW(str, pathW), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

#define FILE_ATTR_MASK (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | \
        FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_ARCHIVE | \
        FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_COMPRESSED)

    hr = IFile_get_Attributes(file, &fa);
    gfa = GetFileAttributesW(pathW) & FILE_ATTR_MASK;
    ok(hr == S_OK, "get_Attributes returned %x, expected S_OK\n", hr);
    ok(fa == gfa, "fa = %x, expected %x\n", fa, gfa);

    hr = IFile_put_Attributes(file, gfa | FILE_ATTRIBUTE_READONLY);
    ok(hr == S_OK, "put_Attributes failed: %08x\n", hr);
    new_gfa = GetFileAttributesW(pathW) & FILE_ATTR_MASK;
    ok(new_gfa == (gfa|FILE_ATTRIBUTE_READONLY), "new_gfa = %x, expected %x\n", new_gfa, gfa|FILE_ATTRIBUTE_READONLY);

    hr = IFile_get_Attributes(file, &fa);
    ok(hr == S_OK, "get_Attributes returned %x, expected S_OK\n", hr);
    ok(fa == new_gfa, "fa = %x, expected %x\n", fa, new_gfa);

    hr = IFile_put_Attributes(file, gfa);
    ok(hr == S_OK, "put_Attributes failed: %08x\n", hr);
    new_gfa = GetFileAttributesW(pathW) & FILE_ATTR_MASK;
    ok(new_gfa == gfa, "new_gfa = %x, expected %x\n", new_gfa, gfa);

    hr = IFile_get_Attributes(file, &fa);
    ok(hr == S_OK, "get_Attributes returned %x, expected S_OK\n", hr);
    ok(fa == gfa, "fa = %x, expected %x\n", fa, gfa);

    hr = IFile_get_Size(file, &size);
    ok(hr == S_OK, "get_Size returned %x, expected S_OK\n", hr);
    ok(V_VT(&size) == VT_I4, "V_VT(&size) = %d, expected VT_I4\n", V_VT(&size));
    ok(V_I4(&size) == 0, "V_I4(&size) = %d, expected 0\n", V_I4(&size));
    IFile_Release(file);

    hr = IFileSystem3_DeleteFile(fs3, path, FALSE);
    ok(hr==CTL_E_PERMISSIONDENIED || broken(hr==S_OK),
            "DeleteFile returned %x, expected CTL_E_PERMISSIONDENIED\n", hr);
    if(hr != S_OK) {
        hr = IFileSystem3_DeleteFile(fs3, path, TRUE);
        ok(hr == S_OK, "DeleteFile returned %x, expected S_OK\n", hr);
    }
    hr = IFileSystem3_DeleteFile(fs3, path, TRUE);
    ok(hr == CTL_E_FILENOTFOUND, "DeleteFile returned %x, expected CTL_E_FILENOTFOUND\n", hr);

    SysFreeString(path);

    /* try with directory */
    lstrcatW(pathW, slW);
    ret = CreateDirectoryW(pathW, NULL);
    ok(ret, "got %d, error %d\n", ret, GetLastError());

    path = SysAllocString(pathW);
    hr = IFileSystem3_GetFile(fs3, path, &file);
    ok(hr == CTL_E_FILENOTFOUND, "GetFile returned %x, expected S_OK\n", hr);
    SysFreeString(path);

    RemoveDirectoryW(pathW);
}

static inline BOOL create_file(const WCHAR *name)
{
    HANDLE f = CreateFileW(name, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
    CloseHandle(f);
    return f != INVALID_HANDLE_VALUE;
}

static inline void create_path(const WCHAR *folder, const WCHAR *name, WCHAR *ret)
{
    DWORD len = lstrlenW(folder);
    memmove(ret, folder, len*sizeof(WCHAR));
    ret[len] = '\\';
    memmove(ret+len+1, name, (lstrlenW(name)+1)*sizeof(WCHAR));
}

static void test_CopyFolder(void)
{
    static const WCHAR filesystem3_dir[] = {'f','i','l','e','s','y','s','t','e','m','3','_','t','e','s','t',0};
    static const WCHAR s1[] = {'s','r','c','1',0};
    static const WCHAR s[] = {'s','r','c','*',0};
    static const WCHAR d[] = {'d','s','t',0};
    static const WCHAR empty[] = {0};

    WCHAR tmp[MAX_PATH];
    BSTR bsrc, bdst;
    HRESULT hr;

    if(!CreateDirectoryW(filesystem3_dir, NULL)) {
        skip("can't create temporary directory\n");
        return;
    }

    create_path(filesystem3_dir, s1, tmp);
    bsrc = SysAllocString(tmp);
    create_path(filesystem3_dir, d, tmp);
    bdst = SysAllocString(tmp);
    hr = IFileSystem3_CopyFile(fs3, bsrc, bdst, VARIANT_TRUE);
    ok(hr == CTL_E_FILENOTFOUND, "CopyFile returned %x, expected CTL_E_FILENOTFOUND\n", hr);

    hr = IFileSystem3_CopyFolder(fs3, bsrc, bdst, VARIANT_TRUE);
    ok(hr == CTL_E_PATHNOTFOUND, "CopyFolder returned %x, expected CTL_E_PATHNOTFOUND\n", hr);

    ok(create_file(bsrc), "can't create %s file\n", wine_dbgstr_w(bsrc));
    hr = IFileSystem3_CopyFile(fs3, bsrc, bdst, VARIANT_TRUE);
    ok(hr == S_OK, "CopyFile returned %x, expected S_OK\n", hr);

    hr = IFileSystem3_CopyFolder(fs3, bsrc, bdst, VARIANT_TRUE);
    ok(hr == CTL_E_PATHNOTFOUND, "CopyFolder returned %x, expected CTL_E_PATHNOTFOUND\n", hr);

    hr = IFileSystem3_DeleteFile(fs3, bsrc, VARIANT_FALSE);
    ok(hr == S_OK, "DeleteFile returned %x, expected S_OK\n", hr);

    ok(CreateDirectoryW(bsrc, NULL), "can't create %s\n", wine_dbgstr_w(bsrc));
    hr = IFileSystem3_CopyFile(fs3, bsrc, bdst, VARIANT_TRUE);
    ok(hr == CTL_E_FILENOTFOUND, "CopyFile returned %x, expected CTL_E_FILENOTFOUND\n", hr);

    hr = IFileSystem3_CopyFolder(fs3, bsrc, bdst, VARIANT_TRUE);
    ok(hr == CTL_E_FILEALREADYEXISTS, "CopyFolder returned %x, expected CTL_E_FILEALREADYEXISTS\n", hr);

    hr = IFileSystem3_DeleteFile(fs3, bdst, VARIANT_TRUE);
    ok(hr == S_OK, "DeleteFile returned %x, expected S_OK\n", hr);

    hr = IFileSystem3_CopyFolder(fs3, bsrc, bdst, VARIANT_TRUE);
    ok(hr == S_OK, "CopyFolder returned %x, expected S_OK\n", hr);

    hr = IFileSystem3_CopyFolder(fs3, bsrc, bdst, VARIANT_TRUE);
    ok(hr == S_OK, "CopyFolder returned %x, expected S_OK\n", hr);
    create_path(tmp, s1, tmp);
    ok(GetFileAttributesW(tmp) == INVALID_FILE_ATTRIBUTES,
            "%s file exists\n", wine_dbgstr_w(tmp));

    create_path(filesystem3_dir, d, tmp);
    create_path(tmp, empty, tmp);
    SysFreeString(bdst);
    bdst = SysAllocString(tmp);
    hr = IFileSystem3_CopyFolder(fs3, bsrc, bdst, VARIANT_TRUE);
    ok(hr == S_OK, "CopyFolder returned %x, expected S_OK\n", hr);
    create_path(tmp, s1, tmp);
    ok(GetFileAttributesW(tmp) != INVALID_FILE_ATTRIBUTES,
            "%s directory doesn't exist\n", wine_dbgstr_w(tmp));
    ok(RemoveDirectoryW(tmp), "can't remove %s directory\n", wine_dbgstr_w(tmp));
    create_path(filesystem3_dir, d, tmp);
    SysFreeString(bdst);
    bdst = SysAllocString(tmp);


    create_path(filesystem3_dir, s, tmp);
    SysFreeString(bsrc);
    bsrc = SysAllocString(tmp);
    hr = IFileSystem3_CopyFolder(fs3, bsrc, bdst, VARIANT_TRUE);
    ok(hr == S_OK, "CopyFolder returned %x, expected S_OK\n", hr);
    create_path(filesystem3_dir, d, tmp);
    create_path(tmp, s1, tmp);
    ok(GetFileAttributesW(tmp) != INVALID_FILE_ATTRIBUTES,
            "%s directory doesn't exist\n", wine_dbgstr_w(tmp));

    hr = IFileSystem3_DeleteFolder(fs3, bdst, VARIANT_FALSE);
    ok(hr == S_OK, "DeleteFolder returned %x, expected S_OK\n", hr);

    hr = IFileSystem3_CopyFolder(fs3, bsrc, bdst, VARIANT_TRUE);
    ok(hr == CTL_E_PATHNOTFOUND, "CopyFolder returned %x, expected CTL_E_PATHNOTFOUND\n", hr);

    create_path(filesystem3_dir, s1, tmp);
    SysFreeString(bsrc);
    bsrc = SysAllocString(tmp);
    create_path(tmp, s1, tmp);
    ok(create_file(tmp), "can't create %s file\n", wine_dbgstr_w(tmp));
    hr = IFileSystem3_CopyFolder(fs3, bsrc, bdst, VARIANT_FALSE);
    ok(hr == S_OK, "CopyFolder returned %x, expected S_OK\n", hr);

    hr = IFileSystem3_CopyFolder(fs3, bsrc, bdst, VARIANT_FALSE);
    ok(hr == CTL_E_FILEALREADYEXISTS, "CopyFolder returned %x, expected CTL_E_FILEALREADYEXISTS\n", hr);

    hr = IFileSystem3_CopyFolder(fs3, bsrc, bdst, VARIANT_TRUE);
    ok(hr == S_OK, "CopyFolder returned %x, expected S_OK\n", hr);
    SysFreeString(bsrc);
    SysFreeString(bdst);

    bsrc = SysAllocString(filesystem3_dir);
    hr = IFileSystem3_DeleteFolder(fs3, bsrc, VARIANT_FALSE);
    ok(hr == S_OK, "DeleteFolder returned %x, expected S_OK\n", hr);
    SysFreeString(bsrc);
}

static BSTR bstr_from_str(const char *str)
{
    int len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    BSTR ret = SysAllocStringLen(NULL, len - 1);  /* NUL character added automatically */
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    return ret;
}

struct buildpath_test
{
    const char *path;
    const char *name;
    const char *result;
};

static struct buildpath_test buildpath_data[] =
{
    { "C:\\path", "..\\name.tmp", "C:\\path\\..\\name.tmp" },
    { "C:\\path", "\\name.tmp", "C:\\path\\name.tmp" },
    { "C:\\path", "name.tmp", "C:\\path\\name.tmp" },
    { "C:\\path\\", "name.tmp", "C:\\path\\name.tmp" },
    { "C:\\path", "\\\\name.tmp", "C:\\path\\\\name.tmp" },
    { "C:\\path\\", "\\name.tmp", "C:\\path\\name.tmp" },
    { "C:\\path\\", "\\\\name.tmp", "C:\\path\\\\name.tmp" },
    { "C:\\path\\\\", "\\\\name.tmp", "C:\\path\\\\\\name.tmp" },
    { "C:\\\\", "\\name.tmp", "C:\\\\name.tmp" },
    { "C:", "name.tmp", "C:name.tmp" },
    { "C:", "\\\\name.tmp", "C:\\\\name.tmp" },
    { NULL }
};

static void test_BuildPath(void)
{
    struct buildpath_test *ptr = buildpath_data;
    BSTR ret, path;
    HRESULT hr;
    int i = 0;

    hr = IFileSystem3_BuildPath(fs3, NULL, NULL, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    ret = (BSTR)0xdeadbeef;
    hr = IFileSystem3_BuildPath(fs3, NULL, NULL, &ret);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(*ret == 0, "got %p\n", ret);
    SysFreeString(ret);

    ret = (BSTR)0xdeadbeef;
    path = bstr_from_str("path");
    hr = IFileSystem3_BuildPath(fs3, path, NULL, &ret);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(!lstrcmpW(ret, path), "got %s\n", wine_dbgstr_w(ret));
    SysFreeString(ret);
    SysFreeString(path);

    ret = (BSTR)0xdeadbeef;
    path = bstr_from_str("path");
    hr = IFileSystem3_BuildPath(fs3, NULL, path, &ret);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(!lstrcmpW(ret, path), "got %s\n", wine_dbgstr_w(ret));
    SysFreeString(ret);
    SysFreeString(path);

    while (ptr->path)
    {
        BSTR name, result;

        ret = NULL;
        path = bstr_from_str(ptr->path);
        name = bstr_from_str(ptr->name);
        result = bstr_from_str(ptr->result);
        hr = IFileSystem3_BuildPath(fs3, path, name, &ret);
        ok(hr == S_OK, "%d: got 0x%08x\n", i, hr);
        if (hr == S_OK)
        {
            ok(!lstrcmpW(ret, result), "%d: got wrong path %s, expected %s\n", i, wine_dbgstr_w(ret),
                wine_dbgstr_w(result));
            SysFreeString(ret);
        }
        SysFreeString(path);
        SysFreeString(name);
        SysFreeString(result);

        i++;
        ptr++;
    }
}

static void test_GetFolder(void)
{
    static const WCHAR dummyW[] = {'d','u','m','m','y',0};
    WCHAR buffW[MAX_PATH];
    IFolder *folder;
    HRESULT hr;
    BSTR str;

    folder = (void*)0xdeadbeef;
    hr = IFileSystem3_GetFolder(fs3, NULL, &folder);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    ok(folder == NULL, "got %p\n", folder);

    hr = IFileSystem3_GetFolder(fs3, NULL, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    /* something that doesn't exist */
    str = SysAllocString(dummyW);

    hr = IFileSystem3_GetFolder(fs3, str, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    folder = (void*)0xdeadbeef;
    hr = IFileSystem3_GetFolder(fs3, str, &folder);
    ok(hr == CTL_E_PATHNOTFOUND, "got 0x%08x\n", hr);
    ok(folder == NULL, "got %p\n", folder);
    SysFreeString(str);

    GetWindowsDirectoryW(buffW, MAX_PATH);
    str = SysAllocString(buffW);
    hr = IFileSystem3_GetFolder(fs3, str, &folder);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    SysFreeString(str);
    test_provideclassinfo(folder, &CLSID_Folder);
    IFolder_Release(folder);
}

static void _test_clone(IEnumVARIANT *enumvar, BOOL position_inherited, LONG count, int line)
{
    HRESULT hr;
    IEnumVARIANT *clone;
    ULONG fetched;
    VARIANT var, var2;

    hr = IEnumVARIANT_Reset(enumvar);
    ok(hr == S_OK, "%d: got 0x%08x\n", line, hr);

    VariantInit(&var);
    fetched = -1;
    hr = IEnumVARIANT_Next(enumvar, 1, &var, &fetched);
    ok(hr == S_OK, "%d: got 0x%08x\n", line, hr);
    ok(fetched == 1, "%d: got %d\n", line, fetched);

    /* clone enumerator */
    hr = IEnumVARIANT_Clone(enumvar, &clone);
    ok(hr == S_OK, "%d: got 0x%08x\n", line, hr);
    ok(clone != enumvar, "%d: got %p, %p\n", line, enumvar, clone);

    /* check if clone inherits position */
    VariantInit(&var2);
    fetched = -1;
    hr = IEnumVARIANT_Next(clone, 1, &var2, &fetched);
    if (position_inherited && count == 1)
    {
        ok(hr == S_FALSE, "%d: got 0x%08x\n", line, hr);
        ok(fetched == 0, "%d: got %d\n", line, fetched);
    }
    else
    {
        ok(hr == S_OK, "%d: got 0x%08x\n", line, hr);
        ok(fetched == 1, "%d: got %d\n", line, fetched);
        if (!position_inherited)
            todo_wine ok(V_DISPATCH(&var) == V_DISPATCH(&var2), "%d: values don't match\n", line);
        else
        {
            fetched = -1;
            hr = IEnumVARIANT_Next(enumvar, 1, &var, &fetched);
            ok(hr == S_OK, "%d: got 0x%08x\n", line, hr);
            ok(fetched == 1, "%d: got %d\n", line, fetched);
            todo_wine ok(V_DISPATCH(&var) == V_DISPATCH(&var2), "%d: values don't match\n", line);
        }
    }

    VariantClear(&var2);
    VariantClear(&var);
    IEnumVARIANT_Release(clone);

    hr = IEnumVARIANT_Reset(enumvar);
    ok(hr == S_OK, "%d: got 0x%08x\n", line, hr);
}
#define test_clone(a, b, c) _test_clone(a, b, c, __LINE__)

/* Please keep the tests for IFolderCollection and IFileCollection in sync */
static void test_FolderCollection(void)
{
    static const WCHAR fooW[] = {'f','o','o',0};
    static const WCHAR aW[] = {'\\','a',0};
    static const WCHAR bW[] = {'\\','b',0};
    static const WCHAR cW[] = {'\\','c',0};
    IFolderCollection *folders;
    WCHAR buffW[MAX_PATH], pathW[MAX_PATH];
    IEnumVARIANT *enumvar;
    LONG count, ref, ref2, i;
    IUnknown *unk, *unk2;
    IFolder *folder;
    ULONG fetched;
    VARIANT var, var2[2];
    HRESULT hr;
    BSTR str;
    int found_a = 0, found_b = 0, found_c = 0;

    get_temp_path(fooW, buffW);
    CreateDirectoryW(buffW, NULL);

    str = SysAllocString(buffW);
    hr = IFileSystem3_GetFolder(fs3, str, &folder);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    SysFreeString(str);

    hr = IFolder_get_SubFolders(folder, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    hr = IFolder_get_Path(folder, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    hr = IFolder_get_Path(folder, &str);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(!lstrcmpiW(buffW, str), "got %s, expected %s\n", wine_dbgstr_w(str), wine_dbgstr_w(buffW));
    SysFreeString(str);

    lstrcpyW(pathW, buffW);
    lstrcatW(pathW, aW);
    CreateDirectoryW(pathW, NULL);

    lstrcpyW(pathW, buffW);
    lstrcatW(pathW, bW);
    CreateDirectoryW(pathW, NULL);

    hr = IFolder_get_SubFolders(folder, &folders);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    test_provideclassinfo(folders, &CLSID_Folders);
    IFolder_Release(folder);

    count = 0;
    hr = IFolderCollection_get_Count(folders, &count);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(count == 2, "got %d\n", count);

    lstrcpyW(pathW, buffW);
    lstrcatW(pathW, cW);
    CreateDirectoryW(pathW, NULL);

    /* every time property is requested it scans directory */
    count = 0;
    hr = IFolderCollection_get_Count(folders, &count);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(count == 3, "got %d\n", count);

    hr = IFolderCollection_get__NewEnum(folders, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    hr = IFolderCollection_QueryInterface(folders, &IID_IEnumVARIANT, (void**)&unk);
    ok(hr == E_NOINTERFACE, "got 0x%08x\n", hr);

    /* NewEnum creates new instance each time it's called */
    ref = GET_REFCOUNT(folders);

    unk = NULL;
    hr = IFolderCollection_get__NewEnum(folders, &unk);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    ref2 = GET_REFCOUNT(folders);
    ok(ref2 == ref + 1, "got %d, %d\n", ref2, ref);

    unk2 = NULL;
    hr = IFolderCollection_get__NewEnum(folders, &unk2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(unk != unk2, "got %p, %p\n", unk2, unk);
    IUnknown_Release(unk2);

    /* now get IEnumVARIANT */
    ref = GET_REFCOUNT(folders);
    hr = IUnknown_QueryInterface(unk, &IID_IEnumVARIANT, (void**)&enumvar);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ref2 = GET_REFCOUNT(folders);
    ok(ref2 == ref, "got %d, %d\n", ref2, ref);

    test_clone(enumvar, FALSE, count);

    for (i = 0; i < 3; i++)
    {
        VariantInit(&var);
        fetched = 0;
        hr = IEnumVARIANT_Next(enumvar, 1, &var, &fetched);
        ok(hr == S_OK, "%d: got 0x%08x\n", i, hr);
        ok(fetched == 1, "%d: got %d\n", i, fetched);
        ok(V_VT(&var) == VT_DISPATCH, "%d: got type %d\n", i, V_VT(&var));

        hr = IDispatch_QueryInterface(V_DISPATCH(&var), &IID_IFolder, (void**)&folder);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        str = NULL;
        hr = IFolder_get_Name(folder, &str);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        if (!lstrcmpW(str, aW + 1))
            found_a++;
        else if (!lstrcmpW(str, bW + 1))
            found_b++;
        else if (!lstrcmpW(str, cW + 1))
            found_c++;
        else
            ok(0, "unexpected folder %s was found\n", wine_dbgstr_w(str));
        SysFreeString(str);

        IFolder_Release(folder);
        VariantClear(&var);
    }

    ok(found_a == 1 && found_b == 1 && found_c == 1,
       "each folder should be found 1 time instead of %d/%d/%d\n",
       found_a, found_b, found_c);

    VariantInit(&var);
    fetched = -1;
    hr = IEnumVARIANT_Next(enumvar, 1, &var, &fetched);
    ok(hr == S_FALSE, "got 0x%08x\n", hr);
    ok(fetched == 0, "got %d\n", fetched);

    hr = IEnumVARIANT_Reset(enumvar);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IEnumVARIANT_Skip(enumvar, 2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IEnumVARIANT_Skip(enumvar, 0);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    VariantInit(&var2[0]);
    VariantInit(&var2[1]);
    fetched = -1;
    hr = IEnumVARIANT_Next(enumvar, 0, var2, &fetched);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(fetched == 0, "got %d\n", fetched);
    fetched = -1;
    hr = IEnumVARIANT_Next(enumvar, 2, var2, &fetched);
    ok(hr == S_FALSE, "got 0x%08x\n", hr);
    ok(fetched == 1, "got %d\n", fetched);
    ok(V_VT(&var2[0]) == VT_DISPATCH, "got type %d\n", V_VT(&var2[0]));
    VariantClear(&var2[0]);
    VariantClear(&var2[1]);

    IEnumVARIANT_Release(enumvar);
    IUnknown_Release(unk);

    lstrcpyW(pathW, buffW);
    lstrcatW(pathW, aW);
    RemoveDirectoryW(pathW);
    lstrcpyW(pathW, buffW);
    lstrcatW(pathW, bW);
    RemoveDirectoryW(pathW);
    lstrcpyW(pathW, buffW);
    lstrcatW(pathW, cW);
    RemoveDirectoryW(pathW);
    RemoveDirectoryW(buffW);

    IFolderCollection_Release(folders);
}

/* Please keep the tests for IFolderCollection and IFileCollection in sync */
static void test_FileCollection(void)
{
    static const WCHAR fooW[] = {'\\','f','o','o',0};
    static const WCHAR aW[] = {'\\','a',0};
    static const WCHAR bW[] = {'\\','b',0};
    static const WCHAR cW[] = {'\\','c',0};
    WCHAR buffW[MAX_PATH], pathW[MAX_PATH];
    IFolder *folder;
    IFileCollection *files;
    IFile *file;
    IEnumVARIANT *enumvar;
    LONG count, ref, ref2, i;
    IUnknown *unk, *unk2;
    ULONG fetched;
    VARIANT var, var2[2];
    HRESULT hr;
    BSTR str;
    HANDLE file_a, file_b, file_c;
    int found_a = 0, found_b = 0, found_c = 0;

    get_temp_path(fooW, buffW);
    CreateDirectoryW(buffW, NULL);

    str = SysAllocString(buffW);
    hr = IFileSystem3_GetFolder(fs3, str, &folder);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    SysFreeString(str);

    hr = IFolder_get_Files(folder, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    lstrcpyW(pathW, buffW);
    lstrcatW(pathW, aW);
    file_a = CreateFileW(pathW, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                         FILE_FLAG_DELETE_ON_CLOSE, 0);
    lstrcpyW(pathW, buffW);
    lstrcatW(pathW, bW);
    file_b = CreateFileW(pathW, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                         FILE_FLAG_DELETE_ON_CLOSE, 0);

    hr = IFolder_get_Files(folder, &files);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    test_provideclassinfo(files, &CLSID_Files);
    IFolder_Release(folder);

    count = 0;
    hr = IFileCollection_get_Count(files, &count);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(count == 2, "got %d\n", count);

    lstrcpyW(pathW, buffW);
    lstrcatW(pathW, cW);
    file_c = CreateFileW(pathW, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                         FILE_FLAG_DELETE_ON_CLOSE, 0);

    /* every time property is requested it scans directory */
    count = 0;
    hr = IFileCollection_get_Count(files, &count);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(count == 3, "got %d\n", count);

    hr = IFileCollection_get__NewEnum(files, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    hr = IFileCollection_QueryInterface(files, &IID_IEnumVARIANT, (void**)&unk);
    ok(hr == E_NOINTERFACE, "got 0x%08x\n", hr);

    /* NewEnum creates new instance each time it's called */
    ref = GET_REFCOUNT(files);

    unk = NULL;
    hr = IFileCollection_get__NewEnum(files, &unk);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    ref2 = GET_REFCOUNT(files);
    ok(ref2 == ref + 1, "got %d, %d\n", ref2, ref);

    unk2 = NULL;
    hr = IFileCollection_get__NewEnum(files, &unk2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(unk != unk2, "got %p, %p\n", unk2, unk);
    IUnknown_Release(unk2);

    /* now get IEnumVARIANT */
    ref = GET_REFCOUNT(files);
    hr = IUnknown_QueryInterface(unk, &IID_IEnumVARIANT, (void**)&enumvar);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ref2 = GET_REFCOUNT(files);
    ok(ref2 == ref, "got %d, %d\n", ref2, ref);

    test_clone(enumvar, FALSE, count);

    for (i = 0; i < 3; i++)
    {
        VariantInit(&var);
        fetched = 0;
        hr = IEnumVARIANT_Next(enumvar, 1, &var, &fetched);
        ok(hr == S_OK, "%d: got 0x%08x\n", i, hr);
        ok(fetched == 1, "%d: got %d\n", i, fetched);
        ok(V_VT(&var) == VT_DISPATCH, "%d: got type %d\n", i, V_VT(&var));

        hr = IDispatch_QueryInterface(V_DISPATCH(&var), &IID_IFile, (void **)&file);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        test_provideclassinfo(file, &CLSID_File);

        str = NULL;
        hr = IFile_get_Name(file, &str);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        if (!lstrcmpW(str, aW + 1))
            found_a++;
        else if (!lstrcmpW(str, bW + 1))
            found_b++;
        else if (!lstrcmpW(str, cW + 1))
            found_c++;
        else
            ok(0, "unexpected file %s was found\n", wine_dbgstr_w(str));
        SysFreeString(str);

        IFile_Release(file);
        VariantClear(&var);
    }

    ok(found_a == 1 && found_b == 1 && found_c == 1,
       "each file should be found 1 time instead of %d/%d/%d\n",
       found_a, found_b, found_c);

    VariantInit(&var);
    fetched = -1;
    hr = IEnumVARIANT_Next(enumvar, 1, &var, &fetched);
    ok(hr == S_FALSE, "got 0x%08x\n", hr);
    ok(fetched == 0, "got %d\n", fetched);

    hr = IEnumVARIANT_Reset(enumvar);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IEnumVARIANT_Skip(enumvar, 2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IEnumVARIANT_Skip(enumvar, 0);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    VariantInit(&var2[0]);
    VariantInit(&var2[1]);
    fetched = -1;
    hr = IEnumVARIANT_Next(enumvar, 0, var2, &fetched);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(fetched == 0, "got %d\n", fetched);
    fetched = -1;
    hr = IEnumVARIANT_Next(enumvar, 2, var2, &fetched);
    ok(hr == S_FALSE, "got 0x%08x\n", hr);
    ok(fetched == 1, "got %d\n", fetched);
    ok(V_VT(&var2[0]) == VT_DISPATCH, "got type %d\n", V_VT(&var2[0]));
    VariantClear(&var2[0]);
    VariantClear(&var2[1]);

    IEnumVARIANT_Release(enumvar);
    IUnknown_Release(unk);

    CloseHandle(file_a);
    CloseHandle(file_b);
    CloseHandle(file_c);
    RemoveDirectoryW(buffW);

    IFileCollection_Release(files);
}

static void test_DriveCollection(void)
{
    IDriveCollection *drives;
    IEnumVARIANT *enumvar;
    ULONG fetched;
    VARIANT var;
    HRESULT hr;
    LONG count;

    hr = IFileSystem3_get_Drives(fs3, &drives);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    test_provideclassinfo(drives, &CLSID_Drives);

    hr = IDriveCollection_get__NewEnum(drives, (IUnknown**)&enumvar);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDriveCollection_get_Count(drives, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    count = 0;
    hr = IDriveCollection_get_Count(drives, &count);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(count > 0, "got %d\n", count);

    V_VT(&var) = VT_EMPTY;
    fetched = -1;
    hr = IEnumVARIANT_Next(enumvar, 0, &var, &fetched);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(fetched == 0, "got %d\n", fetched);

    hr = IEnumVARIANT_Skip(enumvar, 0);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IEnumVARIANT_Skip(enumvar, count);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IEnumVARIANT_Skip(enumvar, 1);
    ok(hr == S_FALSE, "got 0x%08x\n", hr);

    test_clone(enumvar, TRUE, count);

    while (IEnumVARIANT_Next(enumvar, 1, &var, &fetched) == S_OK) {
        IDrive *drive = (IDrive*)V_DISPATCH(&var);
        DriveTypeConst type;
        BSTR str;

        hr = IDrive_get_DriveType(drive, &type);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        hr = IDrive_get_DriveLetter(drive, NULL);
        ok(hr == E_POINTER, "got 0x%08x\n", hr);

        hr = IDrive_get_DriveLetter(drive, &str);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(SysStringLen(str) == 1, "got string %s\n", wine_dbgstr_w(str));
        SysFreeString(str);

        hr = IDrive_get_IsReady(drive, NULL);
        ok(hr == E_POINTER, "got 0x%08x\n", hr);

        hr = IDrive_get_TotalSize(drive, NULL);
        ok(hr == E_POINTER, "got 0x%08x\n", hr);

        hr = IDrive_get_AvailableSpace(drive, NULL);
        ok(hr == E_POINTER, "got 0x%08x\n", hr);

        hr = IDrive_get_FreeSpace(drive, NULL);
        ok(hr == E_POINTER, "got 0x%08x\n", hr);

        if (type == Fixed) {
            VARIANT_BOOL ready = VARIANT_FALSE;
            VARIANT size;

            hr = IDrive_get_IsReady(drive, &ready);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            ok(ready == VARIANT_TRUE, "got %x\n", ready);

            if (ready != VARIANT_TRUE) {
                hr = IDrive_get_DriveLetter(drive, &str);
                ok(hr == S_OK, "got 0x%08x\n", hr);

                skip("Drive %s is not ready, skipping some tests\n", wine_dbgstr_w(str));

                VariantClear(&var);
                SysFreeString(str);
                continue;
            }

            V_VT(&size) = VT_EMPTY;
            hr = IDrive_get_TotalSize(drive, &size);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            ok(V_VT(&size) == VT_R8 || V_VT(&size) == VT_I4, "got %d\n", V_VT(&size));
            if (V_VT(&size) == VT_R8)
                ok(V_R8(&size) > 0, "got %f\n", V_R8(&size));
            else
                ok(V_I4(&size) > 0, "got %d\n", V_I4(&size));

            V_VT(&size) = VT_EMPTY;
            hr = IDrive_get_AvailableSpace(drive, &size);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            ok(V_VT(&size) == VT_R8 || V_VT(&size) == VT_I4, "got %d\n", V_VT(&size));
            if (V_VT(&size) == VT_R8)
                ok(V_R8(&size) > (double)INT_MAX, "got %f\n", V_R8(&size));
            else
                ok(V_I4(&size) > 0, "got %d\n", V_I4(&size));

            V_VT(&size) = VT_EMPTY;
            hr = IDrive_get_FreeSpace(drive, &size);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            ok(V_VT(&size) == VT_R8 || V_VT(&size) == VT_I4, "got %d\n", V_VT(&size));
            if (V_VT(&size) == VT_R8)
                ok(V_R8(&size) > 0, "got %f\n", V_R8(&size));
            else
                ok(V_I4(&size) > 0, "got %d\n", V_I4(&size));
        }
        VariantClear(&var);
    }

    IEnumVARIANT_Release(enumvar);
    IDriveCollection_Release(drives);
}

static void get_temp_filepath(const WCHAR *filename, WCHAR *path, WCHAR *dir)
{
    static const WCHAR scrrunW[] = {'s','c','r','r','u','n','\\',0};

    GetTempPathW(MAX_PATH, path);
    lstrcatW(path, scrrunW);
    lstrcpyW(dir, path);
    lstrcatW(path, filename);
}

static void test_CreateTextFile(void)
{
    WCHAR pathW[MAX_PATH], dirW[MAX_PATH], buffW[10];
    ITextStream *stream;
    BSTR nameW, str;
    HANDLE file;
    HRESULT hr;
    BOOL ret;

    get_temp_filepath(testfileW, pathW, dirW);

    /* dir doesn't exist */
    nameW = SysAllocString(pathW);
    hr = IFileSystem3_CreateTextFile(fs3, nameW, VARIANT_FALSE, VARIANT_FALSE, &stream);
    ok(hr == CTL_E_PATHNOTFOUND, "got 0x%08x\n", hr);

    ret = CreateDirectoryW(dirW, NULL);
    ok(ret, "got %d, %d\n", ret, GetLastError());

    hr = IFileSystem3_CreateTextFile(fs3, nameW, VARIANT_FALSE, VARIANT_FALSE, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    test_provideclassinfo(stream, &CLSID_TextStream);

    hr = ITextStream_Read(stream, 1, &str);
    ok(hr == CTL_E_BADFILEMODE, "got 0x%08x\n", hr);

    hr = ITextStream_Close(stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = ITextStream_Read(stream, 1, &str);
    ok(hr == CTL_E_BADFILEMODE || hr == E_VAR_NOT_SET, "got 0x%08x\n", hr);

    hr = ITextStream_Close(stream);
    ok(hr == S_FALSE || hr == E_VAR_NOT_SET, "got 0x%08x\n", hr);

    ITextStream_Release(stream);

    /* check it's created */
    file = CreateFileW(pathW, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "got %p\n", file);
    CloseHandle(file);

    /* try to create again with no-overwrite mode */
    hr = IFileSystem3_CreateTextFile(fs3, nameW, VARIANT_FALSE, VARIANT_FALSE, &stream);
    ok(hr == CTL_E_FILEALREADYEXISTS, "got 0x%08x\n", hr);

    /* now overwrite */
    hr = IFileSystem3_CreateTextFile(fs3, nameW, VARIANT_TRUE, VARIANT_FALSE, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ITextStream_Release(stream);

    /* overwrite in Unicode mode, check for BOM */
    hr = IFileSystem3_CreateTextFile(fs3, nameW, VARIANT_TRUE, VARIANT_TRUE, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ITextStream_Release(stream);

    /* File was created in Unicode mode, it contains 0xfffe BOM. Opening it in non-Unicode mode
       treats BOM like a valuable data with appropriate CP_ACP -> WCHAR conversion. */
    buffW[0] = 0;
    MultiByteToWideChar(CP_ACP, 0, utf16bom, -1, buffW, ARRAY_SIZE(buffW));

    hr = IFileSystem3_OpenTextFile(fs3, nameW, ForReading, VARIANT_FALSE, TristateFalse, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = ITextStream_ReadAll(stream, &str);
    ok(hr == S_FALSE || broken(hr == S_OK) /* win2k */, "got 0x%08x\n", hr);
    ok(!lstrcmpW(str, buffW), "got %s, expected %s\n", wine_dbgstr_w(str), wine_dbgstr_w(buffW));
    SysFreeString(str);
    ITextStream_Release(stream);

    DeleteFileW(nameW);
    RemoveDirectoryW(dirW);
    SysFreeString(nameW);
}

static void test_WriteLine(void)
{
    WCHAR pathW[MAX_PATH], dirW[MAX_PATH];
    WCHAR buffW[MAX_PATH], buff2W[MAX_PATH];
    char buffA[MAX_PATH];
    ITextStream *stream;
    DWORD r, len;
    HANDLE file;
    BSTR nameW;
    HRESULT hr;
    BOOL ret;

    get_temp_filepath(testfileW, pathW, dirW);

    ret = CreateDirectoryW(dirW, NULL);
    ok(ret, "got %d, %d\n", ret, GetLastError());

    /* create as ASCII file first */
    nameW = SysAllocString(pathW);
    hr = IFileSystem3_CreateTextFile(fs3, nameW, VARIANT_FALSE, VARIANT_FALSE, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = ITextStream_WriteLine(stream, nameW);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ITextStream_Release(stream);

    /* check contents */
    file = CreateFileW(pathW, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "got %p\n", file);
    r = 0;
    ret = ReadFile(file, buffA, sizeof(buffA), &r, NULL);
    ok(ret && r, "read %d, got %d, %d\n", r, ret, GetLastError());

    len = MultiByteToWideChar(CP_ACP, 0, buffA, r, buffW, ARRAY_SIZE(buffW));
    buffW[len] = 0;
    lstrcpyW(buff2W, nameW);
    lstrcatW(buff2W, crlfW);
    ok(!lstrcmpW(buff2W, buffW), "got %s, expected %s\n", wine_dbgstr_w(buffW), wine_dbgstr_w(buff2W));
    CloseHandle(file);
    DeleteFileW(nameW);

    /* same for unicode file */
    hr = IFileSystem3_CreateTextFile(fs3, nameW, VARIANT_FALSE, VARIANT_TRUE, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = ITextStream_WriteLine(stream, nameW);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ITextStream_Release(stream);

    /* check contents */
    file = CreateFileW(pathW, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "got %p\n", file);
    r = 0;
    ret = ReadFile(file, buffW, sizeof(buffW), &r, NULL);
    ok(ret && r, "read %d, got %d, %d\n", r, ret, GetLastError());
    buffW[r/sizeof(WCHAR)] = 0;

    buff2W[0] = 0xfeff;
    buff2W[1] = 0;
    lstrcatW(buff2W, nameW);
    lstrcatW(buff2W, crlfW);
    ok(!lstrcmpW(buff2W, buffW), "got %s, expected %s\n", wine_dbgstr_w(buffW), wine_dbgstr_w(buff2W));
    CloseHandle(file);
    DeleteFileW(nameW);

    RemoveDirectoryW(dirW);
    SysFreeString(nameW);
}

static void test_ReadAll(void)
{
    static const WCHAR secondlineW[] = {'s','e','c','o','n','d',0};
    static const WCHAR aW[] = {'A',0};
    WCHAR pathW[MAX_PATH], dirW[MAX_PATH], buffW[500];
    ITextStream *stream;
    BSTR nameW;
    HRESULT hr;
    BOOL ret;
    BSTR str;

    get_temp_filepath(testfileW, pathW, dirW);

    ret = CreateDirectoryW(dirW, NULL);
    ok(ret, "got %d, %d\n", ret, GetLastError());

    /* Unicode file -> read with ascii stream */
    nameW = SysAllocString(pathW);
    hr = IFileSystem3_CreateTextFile(fs3, nameW, VARIANT_FALSE, VARIANT_TRUE, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = ITextStream_WriteLine(stream, nameW);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    str = SysAllocString(secondlineW);
    hr = ITextStream_WriteLine(stream, str);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    SysFreeString(str);

    hr = ITextStream_ReadAll(stream, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    str = (void*)0xdeadbeef;
    hr = ITextStream_ReadAll(stream, &str);
    ok(hr == CTL_E_BADFILEMODE, "got 0x%08x\n", hr);
    ok(str == NULL || broken(str == (void*)0xdeadbeef) /* win2k */, "got %p\n", str);

    ITextStream_Release(stream);

    hr = IFileSystem3_OpenTextFile(fs3, nameW, ForReading, VARIANT_FALSE, TristateFalse, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = ITextStream_ReadAll(stream, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    /* Buffer content is not interpreted - BOM is kept, all data is converted to WCHARs */
    str = NULL;
    hr = ITextStream_ReadAll(stream, &str);
    ok(hr == S_FALSE || broken(hr == S_OK) /* win2k */, "got 0x%08x\n", hr);
    buffW[0] = 0;
    MultiByteToWideChar(CP_ACP, 0, utf16bom, -1, buffW, ARRAY_SIZE(buffW));
    ok(str[0] == buffW[0] && str[1] == buffW[1], "got %s, %d\n", wine_dbgstr_w(str), SysStringLen(str));
    SysFreeString(str);
    ITextStream_Release(stream);

    /* Unicode file -> read with unicode stream */
    hr = IFileSystem3_OpenTextFile(fs3, nameW, ForReading, VARIANT_FALSE, TristateTrue, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    lstrcpyW(buffW, nameW);
    lstrcatW(buffW, crlfW);
    lstrcatW(buffW, secondlineW);
    lstrcatW(buffW, crlfW);
    str = NULL;
    hr = ITextStream_ReadAll(stream, &str);
    ok(hr == S_FALSE || broken(hr == S_OK) /* win2k */, "got 0x%08x\n", hr);
    ok(!lstrcmpW(buffW, str), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    /* ReadAll one more time */
    str = (void*)0xdeadbeef;
    hr = ITextStream_ReadAll(stream, &str);
    ok(hr == CTL_E_ENDOFFILE, "got 0x%08x\n", hr);
    ok(str == NULL || broken(str == (void*)0xdeadbeef) /* win2k */, "got %p\n", str);

    /* ReadLine fails the same way */
    str = (void*)0xdeadbeef;
    hr = ITextStream_ReadLine(stream, &str);
    ok(hr == CTL_E_ENDOFFILE, "got 0x%08x\n", hr);
    ok(str == NULL || broken(str == (void*)0xdeadbeef) /* win2k */, "got %p\n", str);
    ITextStream_Release(stream);

    /* Open again and skip first line before ReadAll */
    hr = IFileSystem3_OpenTextFile(fs3, nameW, ForReading, VARIANT_FALSE, TristateTrue, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    str = NULL;
    hr = ITextStream_ReadLine(stream, &str);
todo_wine {
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(str != NULL, "got %p\n", str);
}
    SysFreeString(str);

    lstrcpyW(buffW, secondlineW);
    lstrcatW(buffW, crlfW);
    str = NULL;
    hr = ITextStream_ReadAll(stream, &str);
    ok(hr == S_FALSE || broken(hr == S_OK) /* win2k */, "got 0x%08x\n", hr);
todo_wine
    ok(!lstrcmpW(buffW, str), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);
    ITextStream_Release(stream);

    /* ASCII file, read with Unicode stream */
    /* 1. one byte content, not enough for Unicode read */
    hr = IFileSystem3_CreateTextFile(fs3, nameW, VARIANT_TRUE, VARIANT_FALSE, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    str = SysAllocString(aW);
    hr = ITextStream_Write(stream, str);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    SysFreeString(str);
    ITextStream_Release(stream);

    hr = IFileSystem3_OpenTextFile(fs3, nameW, ForReading, VARIANT_FALSE, TristateTrue, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    str = (void*)0xdeadbeef;
    hr = ITextStream_ReadAll(stream, &str);
    ok(hr == CTL_E_ENDOFFILE, "got 0x%08x\n", hr);
    ok(str == NULL || broken(str == (void*)0xdeadbeef) /* win2k */, "got %p\n", str);

    ITextStream_Release(stream);

    DeleteFileW(nameW);
    RemoveDirectoryW(dirW);
    SysFreeString(nameW);
}

static void test_Read(void)
{
    static const WCHAR secondlineW[] = {'s','e','c','o','n','d',0};
    static const WCHAR aW[] = {'A',0};
    WCHAR pathW[MAX_PATH], dirW[MAX_PATH], buffW[500];
    ITextStream *stream;
    BSTR nameW;
    HRESULT hr;
    BOOL ret;
    BSTR str;

    get_temp_filepath(testfileW, pathW, dirW);

    ret = CreateDirectoryW(dirW, NULL);
    ok(ret, "got %d, %d\n", ret, GetLastError());

    /* Unicode file -> read with ascii stream */
    nameW = SysAllocString(pathW);
    hr = IFileSystem3_CreateTextFile(fs3, nameW, VARIANT_FALSE, VARIANT_TRUE, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = ITextStream_WriteLine(stream, nameW);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    str = SysAllocString(secondlineW);
    hr = ITextStream_WriteLine(stream, str);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    SysFreeString(str);

    hr = ITextStream_Read(stream, 0, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    hr = ITextStream_Read(stream, 1, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    hr = ITextStream_Read(stream, -1, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    str = (void*)0xdeadbeef;
    hr = ITextStream_Read(stream, 1, &str);
    ok(hr == CTL_E_BADFILEMODE, "got 0x%08x\n", hr);
    ok(str == NULL, "got %p\n", str);

    ITextStream_Release(stream);

    hr = IFileSystem3_OpenTextFile(fs3, nameW, ForReading, VARIANT_FALSE, TristateFalse, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = ITextStream_Read(stream, 1, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    str = (void*)0xdeadbeef;
    hr = ITextStream_Read(stream, -1, &str);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    ok(str == NULL, "got %p\n", str);

    str = (void*)0xdeadbeef;
    hr = ITextStream_Read(stream, 0, &str);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(str == NULL, "got %p\n", str);

    /* Buffer content is not interpreted - BOM is kept, all data is converted to WCHARs */
    str = NULL;
    hr = ITextStream_Read(stream, 2, &str);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    buffW[0] = 0;
    MultiByteToWideChar(CP_ACP, 0, utf16bom, -1, buffW, ARRAY_SIZE(buffW));

    ok(!lstrcmpW(str, buffW), "got %s, expected %s\n", wine_dbgstr_w(str), wine_dbgstr_w(buffW));
    ok(SysStringLen(str) == 2, "got %d\n", SysStringLen(str));
    SysFreeString(str);
    ITextStream_Release(stream);

    /* Unicode file -> read with unicode stream */
    hr = IFileSystem3_OpenTextFile(fs3, nameW, ForReading, VARIANT_FALSE, TristateTrue, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    lstrcpyW(buffW, nameW);
    lstrcatW(buffW, crlfW);
    lstrcatW(buffW, secondlineW);
    lstrcatW(buffW, crlfW);
    str = NULL;
    hr = ITextStream_Read(stream, 500, &str);
    ok(hr == S_FALSE || broken(hr == S_OK) /* win2k */, "got 0x%08x\n", hr);
    ok(!lstrcmpW(buffW, str), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    /* ReadAll one more time */
    str = (void*)0xdeadbeef;
    hr = ITextStream_Read(stream, 10, &str);
    ok(hr == CTL_E_ENDOFFILE, "got 0x%08x\n", hr);
    ok(str == NULL, "got %p\n", str);

    /* ReadLine fails the same way */
    str = (void*)0xdeadbeef;
    hr = ITextStream_ReadLine(stream, &str);
    ok(hr == CTL_E_ENDOFFILE, "got 0x%08x\n", hr);
    ok(str == NULL || broken(str == (void*)0xdeadbeef), "got %p\n", str);
    ITextStream_Release(stream);

    /* Open again and skip first line before ReadAll */
    hr = IFileSystem3_OpenTextFile(fs3, nameW, ForReading, VARIANT_FALSE, TristateTrue, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    str = NULL;
    hr = ITextStream_ReadLine(stream, &str);
todo_wine {
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(str != NULL, "got %p\n", str);
}
    SysFreeString(str);

    lstrcpyW(buffW, secondlineW);
    lstrcatW(buffW, crlfW);
    str = NULL;
    hr = ITextStream_Read(stream, 100, &str);
    ok(hr == S_FALSE || broken(hr == S_OK) /* win2k */, "got 0x%08x\n", hr);
todo_wine
    ok(!lstrcmpW(buffW, str), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);
    ITextStream_Release(stream);

    /* default read will use Unicode */
    hr = IFileSystem3_OpenTextFile(fs3, nameW, ForReading, VARIANT_FALSE, TristateUseDefault, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    lstrcpyW(buffW, nameW);
    lstrcatW(buffW, crlfW);
    lstrcatW(buffW, secondlineW);
    lstrcatW(buffW, crlfW);
    str = NULL;
    hr = ITextStream_Read(stream, 500, &str);
    ok(hr == S_FALSE || broken(hr == S_OK) /* win2003 */, "got 0x%08x\n", hr);
    ok(!lstrcmpW(buffW, str), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    ITextStream_Release(stream);

    /* default append will use Unicode */
    hr = IFileSystem3_OpenTextFile(fs3, nameW, ForAppending, VARIANT_FALSE, TristateUseDefault, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    str = SysAllocString(L"123");
    hr = ITextStream_Write(stream, str);
    ok(hr == S_OK, "got %08x\n", hr);
    SysFreeString(str);

    ITextStream_Release(stream);

    hr = IFileSystem3_OpenTextFile(fs3, nameW, ForReading, VARIANT_FALSE, TristateTrue, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    lstrcatW(buffW, L"123");
    str = NULL;
    hr = ITextStream_Read(stream, 500, &str);
    ok(hr == S_FALSE || broken(hr == S_OK) /* win2003 */, "got 0x%08x\n", hr);
    ok(!lstrcmpW(buffW, str), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    ITextStream_Release(stream);

    /* default write will use ASCII */
    hr = IFileSystem3_OpenTextFile(fs3, nameW, ForWriting, VARIANT_FALSE, TristateUseDefault, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    str = SysAllocString(L"123");
    hr = ITextStream_Write(stream, str);
    ok(hr == S_OK, "got %08x\n", hr);
    SysFreeString(str);

    ITextStream_Release(stream);

    hr = IFileSystem3_OpenTextFile(fs3, nameW, ForReading, VARIANT_FALSE, TristateFalse, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    str = (void*)0xdeadbeef;
    hr = ITextStream_Read(stream, 500, &str);
    ok(hr == S_FALSE || broken(hr == S_OK) /* win2003 */, "got 0x%08x\n", hr);
    ok(!wcscmp(str, L"123"), "got %s\n", wine_dbgstr_w(str));

    ITextStream_Release(stream);
    /* ASCII file, read with default stream */
    hr = IFileSystem3_CreateTextFile(fs3, nameW, VARIANT_TRUE, VARIANT_FALSE, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    str = SysAllocString(L"test");
    hr = ITextStream_Write(stream, str);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    SysFreeString(str);
    ITextStream_Release(stream);

    hr = IFileSystem3_OpenTextFile(fs3, nameW, ForReading, VARIANT_FALSE, TristateUseDefault, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    str = (void*)0xdeadbeef;
    hr = ITextStream_Read(stream, 500, &str);
    ok(hr == S_FALSE || broken(hr == S_OK) /* win2003 */, "got 0x%08x\n", hr);
    ok(!wcscmp(str, L"test"), "got %s\n", wine_dbgstr_w(str));

    ITextStream_Release(stream);

    /* default append will use Unicode */
    hr = IFileSystem3_OpenTextFile(fs3, nameW, ForAppending, VARIANT_FALSE, TristateUseDefault, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    str = SysAllocString(L"123");
    hr = ITextStream_Write(stream, str);
    ok(hr == S_OK, "got %08x\n", hr);
    SysFreeString(str);

    ITextStream_Release(stream);

    hr = IFileSystem3_OpenTextFile(fs3, nameW, ForReading, VARIANT_FALSE, TristateFalse, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    str = NULL;
    hr = ITextStream_Read(stream, 500, &str);
    ok(hr == S_FALSE || broken(hr == S_OK) /* win2003 */, "got 0x%08x\n", hr);
    ok(!lstrcmpW(L"test123", str), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    ITextStream_Release(stream);

    /* default write will use ASCII as well */
    hr = IFileSystem3_OpenTextFile(fs3, nameW, ForWriting, VARIANT_FALSE, TristateUseDefault, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    str = SysAllocString(L"test string");
    hr = ITextStream_Write(stream, str);
    ok(hr == S_OK, "got %08x\n", hr);
    SysFreeString(str);

    ITextStream_Release(stream);

    hr = IFileSystem3_OpenTextFile(fs3, nameW, ForReading, VARIANT_FALSE, TristateFalse, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    str = (void*)0xdeadbeef;
    hr = ITextStream_Read(stream, 500, &str);
    ok(hr == S_FALSE || broken(hr == S_OK) /* win2003 */, "got 0x%08x\n", hr);
    ok(!wcscmp(str, L"test string"), "got %s\n", wine_dbgstr_w(str));

    ITextStream_Release(stream);

    /* ASCII file, read with Unicode stream */
    /* 1. one byte content, not enough for Unicode read */
    hr = IFileSystem3_CreateTextFile(fs3, nameW, VARIANT_TRUE, VARIANT_FALSE, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    str = SysAllocString(aW);
    hr = ITextStream_Write(stream, str);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    SysFreeString(str);
    ITextStream_Release(stream);

    hr = IFileSystem3_OpenTextFile(fs3, nameW, ForReading, VARIANT_FALSE, TristateTrue, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    str = (void*)0xdeadbeef;
    hr = ITextStream_Read(stream, 500, &str);
    ok(hr == CTL_E_ENDOFFILE, "got 0x%08x\n", hr);
    ok(str == NULL, "got %p\n", str);

    ITextStream_Release(stream);

    DeleteFileW(nameW);
    RemoveDirectoryW(dirW);
    SysFreeString(nameW);
}

struct driveexists_test {
    const WCHAR drivespec[10];
    const INT drivetype;
    const VARIANT_BOOL expected_ret;
};

/* If 'drivetype' != -1, the first character of 'drivespec' will be replaced
 * with the drive letter of a drive of this type. If such a drive does not exist,
 * the test will be skipped. */
static const struct driveexists_test driveexiststestdata[] = {
    { {'N',':','\\',0}, DRIVE_NO_ROOT_DIR, VARIANT_FALSE },
    { {'R',':','\\',0}, DRIVE_REMOVABLE, VARIANT_TRUE },
    { {'F',':','\\',0}, DRIVE_FIXED, VARIANT_TRUE },
    { {'F',':',0}, DRIVE_FIXED, VARIANT_TRUE },
    { {'F','?',0}, DRIVE_FIXED, VARIANT_FALSE },
    { {'F',0}, DRIVE_FIXED, VARIANT_TRUE },
    { {'?',0}, -1, VARIANT_FALSE },
    { { 0 } }
};

static void test_DriveExists(void)
{
    const struct driveexists_test *ptr = driveexiststestdata;
    HRESULT hr;
    VARIANT_BOOL ret;
    BSTR drivespec;
    WCHAR root[] = {'?',':','\\',0};

    hr = IFileSystem3_DriveExists(fs3, NULL, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    ret = VARIANT_TRUE;
    hr = IFileSystem3_DriveExists(fs3, NULL, &ret);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(ret == VARIANT_FALSE, "got %x\n", ret);

    drivespec = SysAllocString(root);
    hr = IFileSystem3_DriveExists(fs3, drivespec, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);
    SysFreeString(drivespec);

    for (; *ptr->drivespec; ptr++) {
        drivespec = SysAllocString(ptr->drivespec);
        if (ptr->drivetype != -1) {
            for (root[0] = 'A'; root[0] <= 'Z'; root[0]++)
                if (GetDriveTypeW(root) == ptr->drivetype)
                    break;
            if (root[0] > 'Z') {
                skip("No drive with type 0x%x found, skipping test %s.\n",
                        ptr->drivetype, wine_dbgstr_w(ptr->drivespec));
                SysFreeString(drivespec);
                continue;
            }

            /* Test both upper and lower case drive letters. */
            drivespec[0] = root[0];
            ret = ptr->expected_ret == VARIANT_TRUE ? VARIANT_FALSE : VARIANT_TRUE;
            hr = IFileSystem3_DriveExists(fs3, drivespec, &ret);
            ok(hr == S_OK, "got 0x%08x for drive spec %s (%s)\n",
                    hr, wine_dbgstr_w(drivespec), wine_dbgstr_w(ptr->drivespec));
            ok(ret == ptr->expected_ret, "got %d, expected %d for drive spec %s (%s)\n",
                    ret, ptr->expected_ret, wine_dbgstr_w(drivespec), wine_dbgstr_w(ptr->drivespec));

            drivespec[0] = tolower(root[0]);
        }

        ret = ptr->expected_ret == VARIANT_TRUE ? VARIANT_FALSE : VARIANT_TRUE;
        hr = IFileSystem3_DriveExists(fs3, drivespec, &ret);
        ok(hr == S_OK, "got 0x%08x for drive spec %s (%s)\n",
                hr, wine_dbgstr_w(drivespec), wine_dbgstr_w(ptr->drivespec));
        ok(ret == ptr->expected_ret, "got %d, expected %d for drive spec %s (%s)\n",
                ret, ptr->expected_ret, wine_dbgstr_w(drivespec), wine_dbgstr_w(ptr->drivespec));

        SysFreeString(drivespec);
    }
}

struct getdrivename_test {
    const WCHAR path[10];
    const WCHAR drive[5];
};

static const struct getdrivename_test getdrivenametestdata[] = {
    { {'C',':','\\','1','.','t','s','t',0}, {'C',':',0} },
    { {'O',':','\\','1','.','t','s','t',0}, {'O',':',0} },
    { {'O',':',0}, {'O',':',0} },
    { {'o',':',0}, {'o',':',0} },
    { {'O','O',':',0} },
    { {':',0} },
    { {'O',0} },
    { { 0 } }
};

static void test_GetDriveName(void)
{
    const struct getdrivename_test *ptr = getdrivenametestdata;
    HRESULT hr;
    BSTR name;

    hr = IFileSystem3_GetDriveName(fs3, NULL, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    name = (void*)0xdeadbeef;
    hr = IFileSystem3_GetDriveName(fs3, NULL, &name);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(name == NULL, "got %p\n", name);

    while (*ptr->path) {
        BSTR path = SysAllocString(ptr->path);
        name = (void*)0xdeadbeef;
        hr = IFileSystem3_GetDriveName(fs3, path, &name);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        if (name)
            ok(!lstrcmpW(ptr->drive, name), "got %s, expected %s\n", wine_dbgstr_w(name), wine_dbgstr_w(ptr->drive));
        else
            ok(!*ptr->drive, "got %s, expected %s\n", wine_dbgstr_w(name), wine_dbgstr_w(ptr->drive));
        SysFreeString(path);
        SysFreeString(name);
        ptr++;
    }
}

struct getdrive_test {
    const WCHAR drivespec[12];
    HRESULT res;
    const WCHAR driveletter[2];
};

static void test_GetDrive(void)
{
    HRESULT hr;
    IDrive *drive_fixed, *drive;
    BSTR dl_fixed, drivespec;
    WCHAR root[] = {'?',':','\\',0};

    drive = (void*)0xdeadbeef;
    hr = IFileSystem3_GetDrive(fs3, NULL, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);
    ok(drive == (void*)0xdeadbeef, "got %p\n", drive);

    for (root[0] = 'A'; root[0] <= 'Z'; root[0]++)
        if (GetDriveTypeW(root) == DRIVE_NO_ROOT_DIR)
            break;

    if (root[0] > 'Z')
        skip("All drive letters are occupied, skipping test for nonexisting drive.\n");
    else {
        drivespec = SysAllocString(root);
        drive = (void*)0xdeadbeef;
        hr = IFileSystem3_GetDrive(fs3, drivespec, &drive);
        ok(hr == CTL_E_DEVICEUNAVAILABLE, "got 0x%08x\n", hr);
        ok(drive == NULL, "got %p\n", drive);
        SysFreeString(drivespec);
    }

    drive_fixed = get_fixed_drive();
    if (!drive_fixed) {
        skip("No fixed drive found, skipping test.\n");
        return;
    }

    hr = IDrive_get_DriveLetter(drive_fixed, &dl_fixed);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    if (FAILED(hr))
        skip("Could not retrieve drive letter of fixed drive, skipping test.\n");
    else {
        WCHAR dl_upper = toupper(dl_fixed[0]);
        WCHAR dl_lower = tolower(dl_fixed[0]);
        struct getdrive_test testdata[] = {
            { {dl_upper,0}, S_OK, {dl_upper,0} },
            { {dl_upper,':',0}, S_OK, {dl_upper,0} },
            { {dl_upper,':','\\',0}, S_OK, {dl_upper,0} },
            { {dl_lower,':','\\',0}, S_OK, {dl_upper,0} },
            { {dl_upper,'\\',0}, E_INVALIDARG, { 0 } },
            { {dl_lower,'\\',0}, E_INVALIDARG, { 0 } },
            { {'$',':','\\',0}, E_INVALIDARG, { 0 } },
            { {'\\','h','o','s','t','\\','s','h','a','r','e',0}, E_INVALIDARG, { 0 } },
            { {'h','o','s','t','\\','s','h','a','r','e',0}, E_INVALIDARG, { 0 } },
            { { 0 } },
        };
        struct getdrive_test *ptr = &testdata[0];

        for (; *ptr->drivespec; ptr++) {
            drivespec = SysAllocString(ptr->drivespec);
            drive = (void*)0xdeadbeef;
            hr = IFileSystem3_GetDrive(fs3, drivespec, &drive);
            ok(hr == ptr->res, "got 0x%08x, expected 0x%08x for drive spec %s\n",
                    hr, ptr->res, wine_dbgstr_w(ptr->drivespec));
            ok(!lstrcmpW(ptr->drivespec, drivespec), "GetDrive modified its DriveSpec argument\n");
            SysFreeString(drivespec);

            if (*ptr->driveletter) {
                BSTR driveletter;
                hr = IDrive_get_DriveLetter(drive, &driveletter);
                ok(hr == S_OK, "got 0x%08x for drive spec %s\n", hr, wine_dbgstr_w(ptr->drivespec));
                if (SUCCEEDED(hr)) {
                    ok(!lstrcmpW(ptr->driveletter, driveletter), "got %s, expected %s for drive spec %s\n",
                            wine_dbgstr_w(driveletter), wine_dbgstr_w(ptr->driveletter),
                            wine_dbgstr_w(ptr->drivespec));
                    SysFreeString(driveletter);
                }
                test_provideclassinfo(drive, &CLSID_Drive);
                IDrive_Release(drive);
            } else
                ok(drive == NULL, "got %p for drive spec %s\n", drive, wine_dbgstr_w(ptr->drivespec));
        }
        SysFreeString(dl_fixed);
    }
}

static void test_SerialNumber(void)
{
    IDrive *drive;
    LONG serial;
    HRESULT hr;
    BSTR name;

    drive = get_fixed_drive();
    if (!drive) {
        skip("No fixed drive found, skipping test.\n");
        return;
    }

    hr = IDrive_get_SerialNumber(drive, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    serial = 0xdeadbeef;
    hr = IDrive_get_SerialNumber(drive, &serial);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(serial != 0xdeadbeef, "got %x\n", serial);

    hr = IDrive_get_FileSystem(drive, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    name = NULL;
    hr = IDrive_get_FileSystem(drive, &name);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(name != NULL, "got %p\n", name);
    SysFreeString(name);

    hr = IDrive_get_VolumeName(drive, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    name = NULL;
    hr = IDrive_get_VolumeName(drive, &name);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(name != NULL, "got %p\n", name);
    SysFreeString(name);

    IDrive_Release(drive);
}

static const struct extension_test {
    WCHAR path[20];
    WCHAR ext[10];
} extension_tests[] = {
    { {'n','o','e','x','t',0}, {0} },
    { {'n','.','o','.','e','x','t',0}, {'e','x','t',0} },
    { {'n','.','o','.','e','X','t',0}, {'e','X','t',0} },
    { { 0 } }
};

static void test_GetExtensionName(void)
{
    BSTR path, ext;
    HRESULT hr;
    int i;

    for (i = 0; i < ARRAY_SIZE(extension_tests); i++) {

       path = SysAllocString(extension_tests[i].path);
       ext = NULL;
       hr = IFileSystem3_GetExtensionName(fs3, path, &ext);
       ok(hr == S_OK, "got 0x%08x\n", hr);
       if (*extension_tests[i].ext)
           ok(!lstrcmpW(ext, extension_tests[i].ext), "%d: path %s, got %s, expected %s\n", i,
               wine_dbgstr_w(path), wine_dbgstr_w(ext), wine_dbgstr_w(extension_tests[i].ext));
       else
           ok(ext == NULL, "%d: path %s, got %s, expected %s\n", i,
               wine_dbgstr_w(path), wine_dbgstr_w(ext), wine_dbgstr_w(extension_tests[i].ext));

       SysFreeString(path);
       SysFreeString(ext);
    }
}

static void test_GetSpecialFolder(void)
{
    WCHAR pathW[MAX_PATH];
    IFolder *folder;
    HRESULT hr;
    DWORD ret;
    BSTR path;

    hr = IFileSystem3_GetSpecialFolder(fs3, WindowsFolder, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    hr = IFileSystem3_GetSpecialFolder(fs3, TemporaryFolder+1, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    hr = IFileSystem3_GetSpecialFolder(fs3, TemporaryFolder+1, &folder);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IFileSystem3_GetSpecialFolder(fs3, WindowsFolder, &folder);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFolder_get_Path(folder, &path);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    GetWindowsDirectoryW(pathW, ARRAY_SIZE(pathW));
    ok(!lstrcmpiW(pathW, path), "got %s, expected %s\n", wine_dbgstr_w(path), wine_dbgstr_w(pathW));
    SysFreeString(path);
    IFolder_Release(folder);

    hr = IFileSystem3_GetSpecialFolder(fs3, SystemFolder, &folder);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFolder_get_Path(folder, &path);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    GetSystemDirectoryW(pathW, ARRAY_SIZE(pathW));
    ok(!lstrcmpiW(pathW, path), "got %s, expected %s\n", wine_dbgstr_w(path), wine_dbgstr_w(pathW));
    SysFreeString(path);
    IFolder_Release(folder);

    hr = IFileSystem3_GetSpecialFolder(fs3, TemporaryFolder, &folder);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFolder_get_Path(folder, &path);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ret = GetTempPathW(ARRAY_SIZE(pathW), pathW);
    if (ret && pathW[ret-1] == '\\')
        pathW[ret-1] = 0;

    ok(!lstrcmpiW(pathW, path), "got %s, expected %s\n", wine_dbgstr_w(path), wine_dbgstr_w(pathW));
    SysFreeString(path);
    IFolder_Release(folder);
}

START_TEST(filesystem)
{
    HRESULT hr;

    CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_FileSystemObject, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IFileSystem3, (void**)&fs3);
    if(FAILED(hr)) {
        win_skip("Could not create FileSystem object: %08x\n", hr);
        return;
    }

    test_interfaces();
    test_createfolder();
    test_textstream();
    test_GetFileVersion();
    test_GetParentFolderName();
    test_GetFileName();
    test_GetBaseName();
    test_GetAbsolutePathName();
    test_GetFile();
    test_CopyFolder();
    test_BuildPath();
    test_GetFolder();
    test_FolderCollection();
    test_FileCollection();
    test_DriveCollection();
    test_CreateTextFile();
    test_WriteLine();
    test_ReadAll();
    test_Read();
    test_DriveExists();
    test_GetDriveName();
    test_GetDrive();
    test_SerialNumber();
    test_GetExtensionName();
    test_GetSpecialFolder();

    IFileSystem3_Release(fs3);

    CoUninitialize();
}
