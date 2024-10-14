/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Tests for wscript.exe
 * COPYRIGHT:   Whindmar Saksit (whindsaks@proton.me)
 */

#include <apitest.h>
#include <windows.h>
#include <shlwapi.h>
#include <stdio.h>

#define MYGUID "{898AC78E-BFC7-41FF-937D-EDD01E666707}"

static DWORD getregdw(HKEY hKey, LPCSTR sub, LPCSTR name, DWORD *out, DWORD defval)
{
    LRESULT e;
    DWORD size = sizeof(*out);
    *out = 0;
    e = SHGetValueA(hKey, sub, name, NULL, out, &size);
    if (e)
        *out = defval;
    return e;
}

static BOOL makestringfile(LPWSTR path, SIZE_T cchpath, LPCSTR ext, LPCSTR string, const BYTE *map)
{
    UINT cch = GetTempPathW(cchpath, path);
    UINT16 i = 0;
    if (!cch || cch > cchpath)
        return FALSE;
    while (++i)
    {
        HANDLE hFile;
        if (_snwprintf(path + cch, cchpath - cch, L"~%u.%hs", i, ext ? ext : "tmp") >= cchpath - cch)
            return FALSE;
        hFile = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_NEW, 0, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            BOOL succ = TRUE;
            for (; *string && succ; ++string)
            {
                BYTE ch = *string;
                DWORD j;
                for (j = 0; map && map[j + 0]; j += 2)
                {
                    if (ch == map[j + 0])
                        ch = map[j + 1];
                }
                succ = WriteFile(hFile, &ch, 1, &j, NULL);
            }
            CloseHandle(hFile);
            return succ;
        }
    }
    return FALSE;
}

static DWORD runscriptfile(LPCWSTR path, LPCWSTR engine)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    LPCWSTR exe = engine ? engine : L"wscript.exe";
    WCHAR cmd[MAX_PATH * 2];

    if (_snwprintf(cmd, _countof(cmd), L"\"%s\" //nologo \"%s\"", exe, path) >= _countof(cmd))
        return ERROR_BUFFER_OVERFLOW;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    if (CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        DWORD code = ERROR_INTERNAL_ERROR;
        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &code);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return code;
    }
    return GetLastError();
}

static DWORD runscript(LPCSTR ext, LPCSTR script, const BYTE *map)
{
    DWORD code;
    WCHAR file[MAX_PATH];
    if (!makestringfile(file, _countof(file), ext, script, map))
    {
        skip("Unable to create script\n");
        return ERROR_FILE_NOT_FOUND;
    }
    code = runscriptfile(file, NULL);
    DeleteFileW(file);
    return code;
}

static void test_defaultscriptisjs(void)
{
    LPCSTR script = ""
    "<job>"
    "<script>" /* No language attribute should default to Javascript */
    "var x = 42;"
    "WScript.Quit(x);"
    "</script>"
    "</job>";

    ok(runscript("wsf", script, NULL) == 42, "Script failed\n");
}

static void test_simplevb(void)
{
    LPCSTR script = ""
    "<job>"
    "<script language=\"VBScript\">"
    "Dim x\n"
    "x = 42\n"
    "WScript.Quit x\n"
    "</script>"
    "</job>";

    ok(runscript("wsf", script, NULL) == 42, "Script failed\n");
}

static void test_defpackagejob(void)
{
    LPCSTR script = ""
    "<package>"
    "<job id=\"PickMePlease\">"
    "<script language=\"VBScript\">"
    "WScript.Quit 42"
    "</script>"
    "</job>"
    "<job id=\"DontExecuteMe\">"
    "<script language=\"VBScript\">"
    "WScript.Quit 33"
    "</script>"
    "</job>"
    "</package>";

    ok(runscript("wsf", script, NULL) == 42, "Script failed\n");
}

static void test_objecttag(void)
{
    DWORD dw;
    static const BYTE map[] = { '#', '\"', '$', '\\', 0, 0 };
    LPCSTR script = ""
    "<job>"
    "<object id=#ws1# clsid=#{72C24DD5-D70A-438B-8A42-98424B88AFB8}# />"
    "<script language=#JScript#>"
    "var dontcare = ws1.ExpandEnvironmentStrings(#SystemRoot#);"
    "var p = #HKCU/Software/" MYGUID "#.replace(/$//g,'$$');"
    "ws2.RegWrite(p, 42, #REG_DWORD#);"
    "</script>"
    "<object id=#ws2# progid=#WScript.Shell# />" /* Placing the object tag after the script just for fun */
    "</job>";

    ok(runscript("wsf", script, map) == 0, "Script failed\n");

    getregdw(HKEY_CURRENT_USER, "Software", MYGUID, &dw, 0);
    ok(dw == 42, "Value does not match\n");
    SHDeleteValueA(HKEY_CURRENT_USER, "Software", MYGUID);
}

START_TEST(wsf)
{
    test_defaultscriptisjs();
    test_simplevb();
    test_defpackagejob();
    test_objecttag();
}
