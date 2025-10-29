/* Unit test suite for SHLWAPI ordinal functions
 *
 * Copyright 2004 Jon Griffiths
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

#include <stdio.h>

#define COBJMACROS
#define CONST_VTABLE
#include "wine/test.h"
#include "winbase.h"
#include "winerror.h"
#include "winuser.h"
#include "ole2.h"
#include "oaidl.h"
#include "ocidl.h"
#include "mlang.h"
#include "shlwapi.h"
#include "docobj.h"
#include "shobjidl.h"
#include "shlobj.h"

/* Function ptrs for ordinal calls */
static HMODULE hShlwapi;

static int (WINAPI *pSHSearchMapInt)(const int*,const int*,int,int);
static HRESULT (WINAPI *pGetAcceptLanguagesA)(LPSTR,LPDWORD);

static HANDLE (WINAPI *pSHAllocShared)(LPCVOID,DWORD,DWORD);
static LPVOID (WINAPI *pSHLockShared)(HANDLE,DWORD);
static BOOL   (WINAPI *pSHUnlockShared)(LPVOID);
static BOOL   (WINAPI *pSHFreeShared)(HANDLE,DWORD);
static HANDLE (WINAPI *pSHMapHandle)(HANDLE,DWORD,DWORD,DWORD,DWORD);
static HRESULT(WINAPIV *pSHPackDispParams)(DISPPARAMS*,VARIANTARG*,UINT,...);
static HRESULT(WINAPI *pIConnectionPoint_SimpleInvoke)(IConnectionPoint*,DISPID,DISPPARAMS*);
static HRESULT(WINAPI *pIConnectionPoint_InvokeWithCancel)(IConnectionPoint*,DISPID,DISPPARAMS*,DWORD,DWORD);
static HRESULT(WINAPI *pConnectToConnectionPoint)(IUnknown*,REFIID,BOOL,IUnknown*, LPDWORD,IConnectionPoint **);
static HRESULT(WINAPI *pSHPropertyBag_ReadLONG)(IPropertyBag *,LPCWSTR,LPLONG);
static LONG   (WINAPI *pSHSetWindowBits)(HWND, INT, UINT, UINT);
static INT    (WINAPI *pSHFormatDateTimeA)(const FILETIME*, DWORD*, LPSTR, UINT);
static INT    (WINAPI *pSHFormatDateTimeW)(const FILETIME*, DWORD*, LPWSTR, UINT);
static DWORD  (WINAPI *pSHGetObjectCompatFlags)(IUnknown*, const CLSID*);
static BOOL   (WINAPI *pGUIDFromStringA)(LPSTR, CLSID *);
static HRESULT (WINAPI *pIUnknown_QueryServiceExec)(IUnknown*, REFIID, const GUID*, DWORD, DWORD, VARIANT*, VARIANT*);
static HRESULT (WINAPI *pIUnknown_ProfferService)(IUnknown*, REFGUID, IServiceProvider*, DWORD*);
static HWND    (WINAPI *pSHCreateWorkerWindowA)(LONG, HWND, DWORD, DWORD, HMENU, LONG_PTR);
static HRESULT (WINAPI *pSHIShellFolder_EnumObjects)(LPSHELLFOLDER, HWND, SHCONTF, IEnumIDList**);
static DWORD   (WINAPI *pSHGetIniStringW)(LPCWSTR, LPCWSTR, LPWSTR, DWORD, LPCWSTR);
static BOOL    (WINAPI *pSHSetIniStringW)(LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR);
static HKEY    (WINAPI *pSHGetShellKey)(DWORD, LPCWSTR, BOOL);
static HRESULT (WINAPI *pSKGetValueW)(DWORD, LPCWSTR, LPCWSTR, DWORD*, void*, DWORD*);
static HRESULT (WINAPI *pSKSetValueW)(DWORD, LPCWSTR, LPCWSTR, DWORD, void*, DWORD);
static HRESULT (WINAPI *pSKDeleteValueW)(DWORD, LPCWSTR, LPCWSTR);
static HRESULT (WINAPI *pSKAllocValueW)(DWORD, LPCWSTR, LPCWSTR, DWORD*, void**, DWORD*);
static HWND    (WINAPI *pSHSetParentHwnd)(HWND, HWND);
static HRESULT (WINAPI *pIUnknown_GetClassID)(IUnknown*, CLSID*);
static HRESULT (WINAPI *pDllGetVersion)(DLLVERSIONINFO2*);

typedef struct SHELL_USER_SID {
    SID_IDENTIFIER_AUTHORITY sidAuthority;
    DWORD                    dwUserGroupID;
    DWORD                    dwUserID;
} SHELL_USER_SID, *PSHELL_USER_SID;
typedef struct SHELL_USER_PERMISSION {

    SHELL_USER_SID susID;
    DWORD          dwAccessType;
    BOOL           fInherit;
    DWORD          dwAccessMask;
    DWORD          dwInheritMask;
    DWORD          dwInheritAccessMask;
} SHELL_USER_PERMISSION, *PSHELL_USER_PERMISSION;

static SECURITY_DESCRIPTOR* (WINAPI *pGetShellSecurityDescriptor)(const SHELL_USER_PERMISSION**,int);

static const CHAR ie_international[] = {
    'S','o','f','t','w','a','r','e','\\',
    'M','i','c','r','o','s','o','f','t','\\',
    'I','n','t','e','r','n','e','t',' ','E','x','p','l','o','r','e','r','\\',
    'I','n','t','e','r','n','a','t','i','o','n','a','l',0};
static const CHAR acceptlanguage[] = {
    'A','c','c','e','p','t','L','a','n','g','u','a','g','e',0};

typedef struct {
    int id;
    const void *args[5];
} call_entry_t;

typedef struct {
    call_entry_t *calls;
    int count;
    int alloc;
} call_trace_t;

static void init_call_trace(call_trace_t *ctrace)
{
    ctrace->alloc = 10;
    ctrace->count = 0;
    ctrace->calls = HeapAlloc(GetProcessHeap(), 0, sizeof(call_entry_t) * ctrace->alloc);
}

static void free_call_trace(const call_trace_t *ctrace)
{
    HeapFree(GetProcessHeap(), 0, ctrace->calls);
}

static void add_call(call_trace_t *ctrace, int id, const void *arg0,
    const void *arg1, const void *arg2, const void *arg3, const void *arg4)
{
    call_entry_t call;

    call.id = id;
    call.args[0] = arg0;
    call.args[1] = arg1;
    call.args[2] = arg2;
    call.args[3] = arg3;
    call.args[4] = arg4;

    if (ctrace->count == ctrace->alloc)
    {
        ctrace->alloc *= 2;
        ctrace->calls = HeapReAlloc(GetProcessHeap(),0, ctrace->calls, ctrace->alloc*sizeof(call_entry_t));
    }

    ctrace->calls[ctrace->count++] = call;
}

static void ok_trace_(call_trace_t *texpected, call_trace_t *tgot, int line)
{
    if (texpected->count == tgot->count)
    {
        INT i;
        /* compare */
        for (i = 0; i < texpected->count; i++)
        {
            call_entry_t *expected = &texpected->calls[i];
            call_entry_t *got = &tgot->calls[i];
            INT j;

            ok_(__FILE__, line)(expected->id == got->id, "got different ids %d: %d, %d\n", i+1, expected->id, got->id);

            for (j = 0; j < 5; j++)
            {
                ok_(__FILE__, line)(expected->args[j] == got->args[j], "got different args[%d] for %d: %p, %p\n", j, i+1,
                   expected->args[j], got->args[j]);
            }
        }
    }
    else
        ok_(__FILE__, line)(0, "traces length mismatch\n");
}

#define ok_trace(a, b) ok_trace_(a, b, __LINE__)

/* trace of actually made calls */
static call_trace_t trace_got;

static void test_GetAcceptLanguagesA(void)
{
    static LPCSTR table[] = {"de,en-gb;q=0.7,en;q=0.3",
                             "de,en;q=0.3,en-gb;q=0.7", /* sorting is ignored */
                             "winetest",    /* content is ignored */
                             "de-de,de;q=0.5",
                             "de",
                             NULL};

    DWORD exactsize;
    char original[512];
    char language[32];
    char buffer[64];
    HKEY hroot = NULL;
    LONG res_query = ERROR_SUCCESS;
    LONG lres;
    HRESULT hr;
    DWORD maxlen = sizeof(buffer) - 2;
    DWORD len;
    LCID lcid;
    LPCSTR entry;
    INT i = 0;

    lcid = GetUserDefaultLCID();

    /* Get the original Value */
    lres = RegOpenKeyA(HKEY_CURRENT_USER, ie_international, &hroot);
    if (lres) {
        skip("RegOpenKey(%s) failed: %ld\n", ie_international, lres);
        return;
    }
    len = sizeof(original);
    original[0] = 0;
    res_query = RegQueryValueExA(hroot, acceptlanguage, 0, NULL, (PBYTE)original, &len);

    RegDeleteValueA(hroot, acceptlanguage);

    /* Some windows versions use "lang-COUNTRY" as default */
    memset(language, 0, sizeof(language));
    len = GetLocaleInfoA(lcid, LOCALE_SISO639LANGNAME, language, sizeof(language));

    if (len) {
        lstrcatA(language, "-");
        memset(buffer, 0, sizeof(buffer));
        len = GetLocaleInfoA(lcid, LOCALE_SISO3166CTRYNAME, buffer, sizeof(buffer) - len - 1);
        lstrcatA(language, buffer);
    }
    else
    {
        /* LOCALE_SNAME has additional parts in some languages. Try only as last chance */
        memset(language, 0, sizeof(language));
        len = GetLocaleInfoA(lcid, LOCALE_SNAME, language, sizeof(language));
    }

    /* get the default value */
    len = maxlen;
    memset(buffer, '#', maxlen);
    buffer[maxlen] = 0;
    hr = pGetAcceptLanguagesA( buffer, &len);

    if (hr != S_OK) {
        win_skip("GetAcceptLanguagesA failed with 0x%lx\n", hr);
        goto restore_original;
    }

    if (lstrcmpA(buffer, language)) {
        /* some windows versions use "lang" or "lang-country" as default */
        language[0] = 0;
        hr = LcidToRfc1766A(lcid, language, sizeof(language));
        ok(hr == S_OK, "LcidToRfc1766A returned 0x%lx and %s\n", hr, language);
    }

    ok(!lstrcmpA(buffer, language),
        "have '%s' (searching for '%s')\n", language, buffer);

    if (lstrcmpA(buffer, language)) {
        win_skip("no more ideas, how to build the default language '%s'\n", buffer);
        goto restore_original;
    }

    trace("detected default: %s\n", language);
    while ((entry = table[i])) {

        exactsize = lstrlenA(entry);

        lres = RegSetValueExA(hroot, acceptlanguage, 0, REG_SZ, (const BYTE *) entry, exactsize + 1);
        ok(!lres, "got %ld for RegSetValueExA: %s\n", lres, entry);

        /* len includes space for the terminating 0 before vista/w2k8 */
        len = exactsize + 2;
        memset(buffer, '#', maxlen);
        buffer[maxlen] = 0;
        hr = pGetAcceptLanguagesA( buffer, &len);
        ok(((hr == E_INVALIDARG) && (len == 0)) ||
            (SUCCEEDED(hr) &&
            ((len == exactsize) || (len == exactsize+1)) &&
            !lstrcmpA(buffer, entry)),
            "+2_#%d: got 0x%lx with %ld and %s\n", i, hr, len, buffer);

        len = exactsize + 1;
        memset(buffer, '#', maxlen);
        buffer[maxlen] = 0;
        hr = pGetAcceptLanguagesA( buffer, &len);
        ok(((hr == E_INVALIDARG) && (len == 0)) ||
            (SUCCEEDED(hr) &&
            ((len == exactsize) || (len == exactsize+1)) &&
            !lstrcmpA(buffer, entry)),
            "+1_#%d: got 0x%lx with %ld and %s\n", i, hr, len, buffer);

        len = exactsize;
        memset(buffer, '#', maxlen);
        buffer[maxlen] = 0;
        hr = pGetAcceptLanguagesA( buffer, &len);

        /* There is no space for the string in the registry.
           When the buffer is large enough, the default language is returned

           When the buffer is too small for that fallback, win7_32 and w2k8_64
           fail with E_NOT_SUFFICIENT_BUFFER, win8 fails with HRESULT_FROM_WIN32(ERROR_MORE_DATA),
           other versions succeed and return a partial result while older os succeed
           and overflow the buffer */

        ok(((hr == E_INVALIDARG) && (len == 0)) ||
            (((hr == S_OK) && !lstrcmpA(buffer, language)  && (len == lstrlenA(language))) ||
            ((hr == S_OK) && !memcmp(buffer, language, len)) ||
            ((hr == E_NOT_SUFFICIENT_BUFFER) && !len) ||
            ((hr == __HRESULT_FROM_WIN32(ERROR_MORE_DATA)) && len == exactsize)),
            "==_#%d: got 0x%lx with %ld and %s\n", i, hr, len, buffer);

        if (exactsize > 1) {
            len = exactsize - 1;
            memset(buffer, '#', maxlen);
            buffer[maxlen] = 0;
            hr = pGetAcceptLanguagesA( buffer, &len);
            ok(((hr == E_INVALIDARG) && (len == 0)) ||
                (((hr == S_OK) && !lstrcmpA(buffer, language)  && (len == lstrlenA(language))) ||
                ((hr == S_OK) && !memcmp(buffer, language, len)) ||
                ((hr == E_NOT_SUFFICIENT_BUFFER) && !len) ||
                ((hr == __HRESULT_FROM_WIN32(ERROR_MORE_DATA)) && len == exactsize - 1)),
                "-1_#%d: got 0x%lx with %ld and %s\n", i, hr, len, buffer);
        }

        len = 1;
        memset(buffer, '#', maxlen);
        buffer[maxlen] = 0;
        hr = pGetAcceptLanguagesA( buffer, &len);
        ok(((hr == E_INVALIDARG) && (len == 0)) ||
            (((hr == S_OK) && !lstrcmpA(buffer, language)  && (len == lstrlenA(language))) ||
            ((hr == S_OK) && !memcmp(buffer, language, len)) ||
            ((hr == E_NOT_SUFFICIENT_BUFFER) && !len) ||
            ((hr == __HRESULT_FROM_WIN32(ERROR_MORE_DATA)) && len == 1)),
            "=1_#%d: got 0x%lx with %ld and %s\n", i, hr, len, buffer);

        len = maxlen;
        hr = pGetAcceptLanguagesA( NULL, &len);

        /* w2k3 and below: E_FAIL and untouched len,
           since w2k8: S_OK and needed size (excluding 0), win8 S_OK and size including 0. */
        ok( ((hr == S_OK) && ((len == exactsize) || (len == exactsize + 1))) ||
            ((hr == E_FAIL) && (len == maxlen)),
            "NULL,max #%d: got 0x%lx with %ld and %s\n", i, hr, len, buffer);

        i++;
    }

    /* without a value in the registry, a default language is returned */
    RegDeleteValueA(hroot, acceptlanguage);

    len = maxlen;
    memset(buffer, '#', maxlen);
    buffer[maxlen] = 0;
    hr = pGetAcceptLanguagesA( buffer, &len);
    ok( ((hr == S_OK) && (len == lstrlenA(language))),
        "max: got 0x%lx with %ld and %s (expected S_OK with %d and '%s'\n",
        hr, len, buffer, lstrlenA(language), language);

    len = 2;
    memset(buffer, '#', maxlen);
    buffer[maxlen] = 0;
    hr = pGetAcceptLanguagesA( buffer, &len);
    ok( (((hr == S_OK) || (hr == E_INVALIDARG)) && !memcmp(buffer, language, len)) ||
        ((hr == E_NOT_SUFFICIENT_BUFFER) && !len) ||
        ((hr == __HRESULT_FROM_WIN32(ERROR_CANNOT_COPY)) && !len),
        "=2: got 0x%lx with %ld and %s\n", hr, len, buffer);

    len = 1;
    memset(buffer, '#', maxlen);
    buffer[maxlen] = 0;
    hr = pGetAcceptLanguagesA( buffer, &len);
    /* When the buffer is too small, win7_32 and w2k8_64 and above fail with
       E_NOT_SUFFICIENT_BUFFER, win8 ERROR_CANNOT_COPY,
       other versions succeed and return a partial 0 terminated result while other versions
       fail with E_INVALIDARG and return a partial unterminated result */
    ok( (((hr == S_OK) || (hr == E_INVALIDARG)) && !memcmp(buffer, language, len)) ||
        ((hr == E_NOT_SUFFICIENT_BUFFER) && !len) ||
        ((hr == __HRESULT_FROM_WIN32(ERROR_CANNOT_COPY)) && !len),
        "=1: got 0x%lx with %ld and %s\n", hr, len, buffer);

    len = 0;
    memset(buffer, '#', maxlen);
    buffer[maxlen] = 0;
    hr = pGetAcceptLanguagesA( buffer, &len);
    /* w2k3 and below: E_FAIL, since w2k8: E_INVALIDARG, win8 ERROR_CANNOT_COPY */
    ok((hr == E_FAIL) || (hr == E_INVALIDARG) || (hr == __HRESULT_FROM_WIN32(ERROR_CANNOT_COPY)),
        "got 0x%lx\n", hr);

    memset(buffer, '#', maxlen);
    buffer[maxlen] = 0;
    hr = pGetAcceptLanguagesA( buffer, NULL);
    /* w2k3 and below: E_FAIL, since w2k8: E_INVALIDARG */
    ok((hr == E_FAIL) || (hr == E_INVALIDARG),
        "got 0x%lx (expected E_FAIL or E_INVALIDARG)\n", hr);


    hr = pGetAcceptLanguagesA( NULL, NULL);
    /* w2k3 and below: E_FAIL, since w2k8: E_INVALIDARG */
    ok((hr == E_FAIL) || (hr == E_INVALIDARG),
        "got 0x%lx (expected E_FAIL or E_INVALIDARG)\n", hr);

restore_original:
    if (!res_query) {
        len = lstrlenA(original);
        lres = RegSetValueExA(hroot, acceptlanguage, 0, REG_SZ, (const BYTE *) original, len ? len + 1: 0);
        ok(!lres, "RegSetValueEx(%s) failed: %ld\n", original, lres);
    }
    else
    {
        RegDeleteValueA(hroot, acceptlanguage);
    }
    RegCloseKey(hroot);
}

static void test_SHSearchMapInt(void)
{
  int keys[8], values[8];
  int i = 0;

  if (!pSHSearchMapInt)
    return;

  memset(keys, 0, sizeof(keys));
  memset(values, 0, sizeof(values));
  keys[0] = 99; values[0] = 101;

  /* NULL key/value lists crash native, so skip testing them */

  /* 1 element */
  i = pSHSearchMapInt(keys, values, 1, keys[0]);
  ok(i == values[0], "Len 1, expected %d, got %d\n", values[0], i);

  /* Key doesn't exist */
  i = pSHSearchMapInt(keys, values, 1, 100);
  ok(i == -1, "Len 1 - bad key, expected -1, got %d\n", i);

  /* Len = 0 => not found */
  i = pSHSearchMapInt(keys, values, 0, keys[0]);
  ok(i == -1, "Len 1 - passed len 0, expected -1, got %d\n", i);

  /* 2 elements, len = 1 */
  keys[1] = 98; values[1] = 102;
  i = pSHSearchMapInt(keys, values, 1, keys[1]);
  ok(i == -1, "Len 1 - array len 2, expected -1, got %d\n", i);

  /* 2 elements, len = 2 */
  i = pSHSearchMapInt(keys, values, 2, keys[1]);
  ok(i == values[1], "Len 2, expected %d, got %d\n", values[1], i);

  /* Searches forward */
  keys[2] = 99; values[2] = 103;
  i = pSHSearchMapInt(keys, values, 3, keys[0]);
  ok(i == values[0], "Len 3, expected %d, got %d\n", values[0], i);
}

struct shared_struct
{
    DWORD value;
    HANDLE handle;
};

static void test_alloc_shared(int argc, char **argv)
{
    char cmdline[MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = { 0 };
    DWORD procid;
    HANDLE hmem, hmem2 = 0;
    struct shared_struct val, *p;
    BOOL ret;

    procid=GetCurrentProcessId();
    hmem=pSHAllocShared(NULL,10,procid);
    ok(hmem!=NULL,"SHAllocShared(NULL...) failed: %lu\n", GetLastError());
    ret = pSHFreeShared(hmem, procid);
    ok( ret, "SHFreeShared failed: %lu\n", GetLastError());

    val.value = 0x12345678;
    val.handle = 0;
    hmem = pSHAllocShared(&val, sizeof(val), procid);
    ok(hmem!=NULL,"SHAllocShared(NULL...) failed: %lu\n", GetLastError());

    p=pSHLockShared(hmem,procid);
    ok(p!=NULL,"SHLockShared failed: %lu\n", GetLastError());
    if (p!=NULL)
        ok(p->value == 0x12345678, "Wrong value in shared memory: %ld instead of %d\n", p->value, 0x12345678);
    ret = pSHUnlockShared(p);
    ok( ret, "SHUnlockShared failed: %lu\n", GetLastError());

    sprintf(cmdline, "%s %s %ld %p", argv[0], argv[1], procid, hmem);
    ret = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "could not create child process error: %lu\n", GetLastError());
    if (ret)
    {
        wait_child_process(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);

        p = pSHLockShared(hmem, procid);
        ok(p != NULL,"SHLockShared failed: %lu\n", GetLastError());
        if (p != NULL && p->value != 0x12345678)
        {
            ok(p->value == 0x12345679, "Wrong value in shared memory: %ld instead of %d\n", p->value, 0x12345679);
            hmem2 = p->handle;
            ok(hmem2 != NULL, "Expected handle in shared memory\n");
        }
        ret = pSHUnlockShared(p);
        ok(ret, "SHUnlockShared failed: %lu\n", GetLastError());
    }

    ret = pSHFreeShared(hmem, procid);
    ok( ret, "SHFreeShared failed: %lu\n", GetLastError());

    if (hmem2)
    {
        p = pSHLockShared(hmem2, procid);
        ok(p != NULL,"SHLockShared failed: %lu\n", GetLastError());
        if (p != NULL)
            ok(p->value == 0xDEADBEEF, "Wrong value in shared memory: %ld instead of %d\n", p->value, 0xDEADBEEF);
        ret = pSHUnlockShared(p);
        ok(ret, "SHUnlockShared failed: %lu\n", GetLastError());

        ret = pSHFreeShared(hmem2, procid);
        ok(ret, "SHFreeShared failed: %lu\n", GetLastError());
    }

    SetLastError(0xdeadbeef);
    ret = pSHFreeShared(NULL, procid);
    ok(ret, "SHFreeShared failed: %lu\n", GetLastError());
    ok(GetLastError() == 0xdeadbeef, "last error should not have changed, got %lu\n", GetLastError());
}

static void test_alloc_shared_remote(DWORD procid, HANDLE hmem)
{
    struct shared_struct val, *p;
    HANDLE hmem2;
    BOOL ret;

    /* test directly accessing shared memory of a remote process */
    p = pSHLockShared(hmem, procid);
    ok(p != NULL || broken(p == NULL) /* Windows 7/8 */, "SHLockShared failed: %lu\n", GetLastError());
    if (p == NULL)
    {
        win_skip("Subprocess failed to modify shared memory, skipping test\n");
        return;
    }

    ok(p->value == 0x12345678, "Wrong value in shared memory: %ld instead of %d\n", p->value, 0x12345678);
    p->value++;

    val.value = 0xDEADBEEF;
    val.handle = 0;
    p->handle = pSHAllocShared(&val, sizeof(val), procid);
    ok(p->handle != NULL, "SHAllocShared failed: %lu\n", GetLastError());

    ret = pSHUnlockShared(p);
    ok(ret, "SHUnlockShared failed: %lu\n", GetLastError());

    /* test SHMapHandle */
    SetLastError(0xdeadbeef);
    hmem2 = pSHMapHandle(NULL, procid, GetCurrentProcessId(), 0, 0);
    ok(hmem2 == NULL, "expected NULL, got new handle\n");
    ok(GetLastError() == 0xdeadbeef, "last error should not have changed, got %lu\n", GetLastError());

    hmem2 = pSHMapHandle(hmem, procid, GetCurrentProcessId(), 0, 0);

    /* It seems like Windows Vista/2008 uses a different internal implementation
     * for shared memory, and calling SHMapHandle fails. */
    ok(hmem2 != NULL || broken(hmem2 == NULL),
       "SHMapHandle failed: %lu\n", GetLastError());
    if (hmem2 == NULL)
    {
        win_skip("Subprocess failed to map shared memory, skipping test\n");
        return;
    }

    p = pSHLockShared(hmem2, GetCurrentProcessId());
    ok(p != NULL, "SHLockShared failed: %lu\n", GetLastError());

    if (p != NULL)
        ok(p->value == 0x12345679, "Wrong value in shared memory: %ld instead of %d\n", p->value, 0x12345679);

    ret = pSHUnlockShared(p);
    ok(ret, "SHUnlockShared failed: %lu\n", GetLastError());

    ret = pSHFreeShared(hmem2, GetCurrentProcessId());
    ok(ret, "SHFreeShared failed: %lu\n", GetLastError());
}

static void test_fdsa(void)
{
    typedef struct
    {
        DWORD num_items;       /* Number of elements inserted */
        void *mem;             /* Ptr to array */
        DWORD blocks_alloced;  /* Number of elements allocated */
        BYTE inc;              /* Number of elements to grow by when we need to expand */
        BYTE block_size;       /* Size in bytes of an element */
        BYTE flags;            /* Flags */
    } FDSA_info;

    BOOL (WINAPI *pFDSA_Initialize)(DWORD block_size, DWORD inc, FDSA_info *info, void *mem,
                                    DWORD init_blocks);
    BOOL (WINAPI *pFDSA_Destroy)(FDSA_info *info);
    DWORD (WINAPI *pFDSA_InsertItem)(FDSA_info *info, DWORD where, const void *block);
    BOOL (WINAPI *pFDSA_DeleteItem)(FDSA_info *info, DWORD where);

    FDSA_info info;
    int block_size = 10, init_blocks = 4, inc = 2;
    DWORD ret;
    char *mem;

    pFDSA_Initialize = (void *)GetProcAddress(hShlwapi, (LPSTR)208);
    pFDSA_Destroy    = (void *)GetProcAddress(hShlwapi, (LPSTR)209);
    pFDSA_InsertItem = (void *)GetProcAddress(hShlwapi, (LPSTR)210);
    pFDSA_DeleteItem = (void *)GetProcAddress(hShlwapi, (LPSTR)211);

    mem = HeapAlloc(GetProcessHeap(), 0, block_size * init_blocks);
    memset(&info, 0, sizeof(info));

    ok(pFDSA_Initialize(block_size, inc, &info, mem, init_blocks), "FDSA_Initialize rets FALSE\n");
    ok(info.num_items == 0, "num_items = %ld\n", info.num_items);
    ok(info.mem == mem, "mem = %p\n", info.mem);
    ok(info.blocks_alloced == init_blocks, "blocks_alloced = %ld\n", info.blocks_alloced);
    ok(info.inc == inc, "inc = %d\n", info.inc);
    ok(info.block_size == block_size, "block_size = %d\n", info.block_size);
    ok(info.flags == 0, "flags = %d\n", info.flags);

    ret = pFDSA_InsertItem(&info, 1234, "1234567890");
    ok(ret == 0, "ret = %ld\n", ret);
    ok(info.num_items == 1, "num_items = %ld\n", info.num_items);
    ok(info.mem == mem, "mem = %p\n", info.mem);
    ok(info.blocks_alloced == init_blocks, "blocks_alloced = %ld\n", info.blocks_alloced);
    ok(info.inc == inc, "inc = %d\n", info.inc);
    ok(info.block_size == block_size, "block_size = %d\n", info.block_size);
    ok(info.flags == 0, "flags = %d\n", info.flags);

    ret = pFDSA_InsertItem(&info, 1234, "abcdefghij");
    ok(ret == 1, "ret = %ld\n", ret);

    ret = pFDSA_InsertItem(&info, 1, "klmnopqrst");
    ok(ret == 1, "ret = %ld\n", ret);

    ret = pFDSA_InsertItem(&info, 0, "uvwxyzABCD");
    ok(ret == 0, "ret = %ld\n", ret);
    ok(info.mem == mem, "mem = %p\n", info.mem);
    ok(info.flags == 0, "flags = %d\n", info.flags);

    /* This next InsertItem will cause shlwapi to allocate its own mem buffer */
    ret = pFDSA_InsertItem(&info, 0, "EFGHIJKLMN");
    ok(ret == 0, "ret = %ld\n", ret);
    ok(info.mem != mem, "mem = %p\n", info.mem);
    ok(info.blocks_alloced == init_blocks + inc, "blocks_alloced = %ld\n", info.blocks_alloced);
    ok(info.flags == 0x1, "flags = %d\n", info.flags);

    ok(!memcmp(info.mem, "EFGHIJKLMNuvwxyzABCD1234567890klmnopqrstabcdefghij", 50), "mem %s\n", (char*)info.mem);

    ok(pFDSA_DeleteItem(&info, 2), "rets FALSE\n");
    ok(info.mem != mem, "mem = %p\n", info.mem);
    ok(info.blocks_alloced == init_blocks + inc, "blocks_alloced = %ld\n", info.blocks_alloced);
    ok(info.flags == 0x1, "flags = %d\n", info.flags);

    ok(!memcmp(info.mem, "EFGHIJKLMNuvwxyzABCDklmnopqrstabcdefghij", 40), "mem %s\n", (char*)info.mem);

    ok(pFDSA_DeleteItem(&info, 3), "rets FALSE\n");
    ok(info.mem != mem, "mem = %p\n", info.mem);
    ok(info.blocks_alloced == init_blocks + inc, "blocks_alloced = %ld\n", info.blocks_alloced);
    ok(info.flags == 0x1, "flags = %d\n", info.flags);

    ok(!memcmp(info.mem, "EFGHIJKLMNuvwxyzABCDklmnopqrst", 30), "mem %s\n", (char*)info.mem);

    ok(!pFDSA_DeleteItem(&info, 4), "does not ret FALSE\n");

    /* As shlwapi has allocated memory internally, Destroy will ret FALSE */
    ok(!pFDSA_Destroy(&info), "FDSA_Destroy does not ret FALSE\n");


    /* When Initialize is called with inc = 0, set it to 1 */
    ok(pFDSA_Initialize(block_size, 0, &info, mem, init_blocks), "FDSA_Initialize rets FALSE\n");
    ok(info.inc == 1, "inc = %d\n", info.inc);

    /* This time, because shlwapi hasn't had to allocate memory
       internally, Destroy rets non-zero */
    ok(pFDSA_Destroy(&info), "FDSA_Destroy rets FALSE\n");


    HeapFree(GetProcessHeap(), 0, mem);
}

static void test_GetShellSecurityDescriptor(void)
{
    static const SHELL_USER_PERMISSION supCurrentUserFull = {
        { {SECURITY_NULL_SID_AUTHORITY}, 0, 0 },
        ACCESS_ALLOWED_ACE_TYPE, FALSE,
        GENERIC_ALL, 0, 0 };
#define MY_INHERITANCE 0xBE /* invalid value to proof behavior */
    static const SHELL_USER_PERMISSION supEveryoneDenied = {
        { {SECURITY_WORLD_SID_AUTHORITY}, SECURITY_WORLD_RID, 0 },
        ACCESS_DENIED_ACE_TYPE, TRUE,
        GENERIC_WRITE, MY_INHERITANCE | 0xDEADBA00, GENERIC_READ };
    const SHELL_USER_PERMISSION* rgsup[2] = {
        &supCurrentUserFull, &supEveryoneDenied,
    };
    SECURITY_DESCRIPTOR* psd;

    if(!pGetShellSecurityDescriptor) /* vista and later */
    {
        win_skip("GetShellSecurityDescriptor not available\n");
        return;
    }

    psd = pGetShellSecurityDescriptor(NULL, 2);
    ok(psd==NULL, "GetShellSecurityDescriptor should fail\n");
    psd = pGetShellSecurityDescriptor(rgsup, 0);
    ok(psd==NULL, "GetShellSecurityDescriptor should fail, got %p\n", psd);

    SetLastError(0xdeadbeef);
    psd = pGetShellSecurityDescriptor(rgsup, 2);
    ok(psd!=NULL, "GetShellSecurityDescriptor failed\n");
    if (psd!=NULL)
    {
        BOOL bHasDacl = FALSE, bDefaulted, ret;
        PACL pAcl;
        DWORD dwRev;
        SECURITY_DESCRIPTOR_CONTROL control;

        ok(IsValidSecurityDescriptor(psd), "returned value is not valid SD\n");

        ret = GetSecurityDescriptorControl(psd, &control, &dwRev);
        ok(ret, "GetSecurityDescriptorControl failed with error %lu\n", GetLastError());
        ok(0 == (control & SE_SELF_RELATIVE), "SD should be absolute\n");

        ret = GetSecurityDescriptorDacl(psd, &bHasDacl, &pAcl, &bDefaulted);
        ok(ret, "GetSecurityDescriptorDacl failed with error %lu\n", GetLastError());

        ok(bHasDacl, "SD has no DACL\n");
        if (bHasDacl)
        {
            ok(!bDefaulted, "DACL should not be defaulted\n");

            ok(pAcl != NULL, "NULL DACL!\n");
            if (pAcl != NULL)
            {
                ACL_SIZE_INFORMATION asiSize;

                ok(IsValidAcl(pAcl), "DACL is not valid\n");

                ret = GetAclInformation(pAcl, &asiSize, sizeof(asiSize), AclSizeInformation);
                ok(ret, "GetAclInformation failed with error %lu\n", GetLastError());

                ok(asiSize.AceCount == 3, "Incorrect number of ACEs: %ld entries\n", asiSize.AceCount);
                if (asiSize.AceCount == 3)
                {
                    ACCESS_ALLOWED_ACE *paaa; /* will use for DENIED too */

                    ret = GetAce(pAcl, 0, (LPVOID*)&paaa);
                    ok(ret, "GetAce failed with error %lu\n", GetLastError());
                    ok(paaa->Header.AceType == ACCESS_ALLOWED_ACE_TYPE, 
                            "Invalid ACE type %d\n", paaa->Header.AceType); 
                    ok(paaa->Header.AceFlags == 0, "Invalid ACE flags %x\n", paaa->Header.AceFlags);
                    ok(paaa->Mask == GENERIC_ALL, "Invalid ACE mask %lx\n", paaa->Mask);

                    ret = GetAce(pAcl, 1, (LPVOID*)&paaa);
                    ok(ret, "GetAce failed with error %lu\n", GetLastError());
                    ok(paaa->Header.AceType == ACCESS_DENIED_ACE_TYPE, 
                            "Invalid ACE type %d\n", paaa->Header.AceType); 
                    /* first one of two ACEs generated from inheritable entry - without inheritance */
                    ok(paaa->Header.AceFlags == 0, "Invalid ACE flags %x\n", paaa->Header.AceFlags);
                    ok(paaa->Mask == GENERIC_WRITE, "Invalid ACE mask %lx\n", paaa->Mask);

                    ret = GetAce(pAcl, 2, (LPVOID*)&paaa);
                    ok(ret, "GetAce failed with error %lu\n", GetLastError());
                    ok(paaa->Header.AceType == ACCESS_DENIED_ACE_TYPE, 
                            "Invalid ACE type %d\n", paaa->Header.AceType); 
                    /* second ACE - with inheritance */
                    ok(paaa->Header.AceFlags == MY_INHERITANCE,
                            "Invalid ACE flags %x\n", paaa->Header.AceFlags);
                    ok(paaa->Mask == GENERIC_READ, "Invalid ACE mask %lx\n", paaa->Mask);
                }
            }
        }

        LocalFree(psd);
    }
}

static void test_SHPackDispParams(void)
{
    DISPPARAMS params;
    VARIANT vars[10];
    HRESULT hres;

    memset(&params, 0xc0, sizeof(params));
    memset(vars, 0xc0, sizeof(vars));
    hres = pSHPackDispParams(&params, vars, 1, VT_I4, 0xdeadbeef);
    ok(hres == S_OK, "SHPackDispParams failed: %08lx\n", hres);
    ok(params.cArgs == 1, "params.cArgs = %d\n", params.cArgs);
    ok(params.cNamedArgs == 0, "params.cNamedArgs = %d\n", params.cArgs);
    ok(params.rgdispidNamedArgs == NULL, "params.rgdispidNamedArgs = %p\n", params.rgdispidNamedArgs);
    ok(params.rgvarg == vars, "params.rgvarg = %p\n", params.rgvarg);
    ok(V_VT(vars) == VT_I4, "V_VT(var) = %d\n", V_VT(vars));
    ok(V_I4(vars) == 0xdeadbeef, "failed %lx\n", V_I4(vars));

    memset(&params, 0xc0, sizeof(params));
    hres = pSHPackDispParams(&params, NULL, 0, 0);
    ok(hres == S_OK, "SHPackDispParams failed: %08lx\n", hres);
    ok(params.cArgs == 0, "params.cArgs = %d\n", params.cArgs);
    ok(params.cNamedArgs == 0, "params.cNamedArgs = %d\n", params.cArgs);
    ok(params.rgdispidNamedArgs == NULL, "params.rgdispidNamedArgs = %p\n", params.rgdispidNamedArgs);
    ok(params.rgvarg == NULL, "params.rgvarg = %p\n", params.rgvarg);

    memset(vars, 0xc0, sizeof(vars));
    memset(&params, 0xc0, sizeof(params));
    hres = pSHPackDispParams(&params, vars, 4, VT_BSTR, (void*)0xdeadbeef, VT_EMPTY, 10,
            VT_I4, 100, VT_DISPATCH, (void*)0xdeadbeef);
    ok(hres == S_OK, "SHPackDispParams failed: %08lx\n", hres);
    ok(params.cArgs == 4, "params.cArgs = %d\n", params.cArgs);
    ok(params.cNamedArgs == 0, "params.cNamedArgs = %d\n", params.cArgs);
    ok(params.rgdispidNamedArgs == NULL, "params.rgdispidNamedArgs = %p\n", params.rgdispidNamedArgs);
    ok(params.rgvarg == vars, "params.rgvarg = %p\n", params.rgvarg);
    ok(V_VT(vars) == VT_DISPATCH, "V_VT(vars[0]) = %x\n", V_VT(vars));
    ok(V_I4(vars) == 0xdeadbeef, "V_I4(vars[0]) = %lx\n", V_I4(vars));
    ok(V_VT(vars+1) == VT_I4, "V_VT(vars[1]) = %d\n", V_VT(vars+1));
    ok(V_I4(vars+1) == 100, "V_I4(vars[1]) = %lx\n", V_I4(vars+1));
    ok(V_VT(vars+2) == VT_I4, "V_VT(vars[2]) = %d\n", V_VT(vars+2));
    ok(V_I4(vars+2) == 10, "V_I4(vars[2]) = %lx\n", V_I4(vars+2));
    ok(V_VT(vars+3) == VT_BSTR, "V_VT(vars[3]) = %d\n", V_VT(vars+3));
    ok(V_BSTR(vars+3) == (void*)0xdeadbeef, "V_BSTR(vars[3]) = %p\n", V_BSTR(vars+3));
}

typedef struct _disp
{
    IDispatch IDispatch_iface;
    LONG   refCount;
} Disp;

static inline Disp *impl_from_IDispatch(IDispatch *iface)
{
    return CONTAINING_RECORD(iface, Disp, IDispatch_iface);
}

typedef struct _contain
{
    IConnectionPointContainer IConnectionPointContainer_iface;
    LONG   refCount;

    UINT  ptCount;
    IConnectionPoint **pt;
} Contain;

static inline Contain *impl_from_IConnectionPointContainer(IConnectionPointContainer *iface)
{
    return CONTAINING_RECORD(iface, Contain, IConnectionPointContainer_iface);
}

typedef struct _cntptn
{
    IConnectionPoint IConnectionPoint_iface;
    LONG refCount;

    Contain *container;
    GUID  id;
    UINT  sinkCount;
    IUnknown **sink;
} ConPt;

static inline ConPt *impl_from_IConnectionPoint(IConnectionPoint *iface)
{
    return CONTAINING_RECORD(iface, ConPt, IConnectionPoint_iface);
}

typedef struct _enum
{
    IEnumConnections IEnumConnections_iface;
    LONG   refCount;

    UINT idx;
    ConPt *pt;
} EnumCon;

static inline EnumCon *impl_from_IEnumConnections(IEnumConnections *iface)
{
    return CONTAINING_RECORD(iface, EnumCon, IEnumConnections_iface);
}

typedef struct _enumpt
{
    IEnumConnectionPoints IEnumConnectionPoints_iface;
    LONG   refCount;

    int idx;
    Contain *container;
} EnumPt;

static inline EnumPt *impl_from_IEnumConnectionPoints(IEnumConnectionPoints *iface)
{
    return CONTAINING_RECORD(iface, EnumPt, IEnumConnectionPoints_iface);
}


static HRESULT WINAPI Disp_QueryInterface(
        IDispatch* This,
        REFIID riid,
        void **ppvObject)
{
    *ppvObject = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDispatch))
    {
        *ppvObject = This;
    }

    if (*ppvObject)
    {
        IDispatch_AddRef(This);
        return S_OK;
    }

    trace("no interface\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI Disp_AddRef(IDispatch* This)
{
    Disp *iface = impl_from_IDispatch(This);
    return InterlockedIncrement(&iface->refCount);
}

static ULONG WINAPI Disp_Release(IDispatch* This)
{
    Disp *iface = impl_from_IDispatch(This);
    ULONG ret;

    ret = InterlockedDecrement(&iface->refCount);
    if (ret == 0)
        HeapFree(GetProcessHeap(),0,This);
    return ret;
}

static HRESULT WINAPI Disp_GetTypeInfoCount(
        IDispatch* This,
        UINT *pctinfo)
{
    return ERROR_SUCCESS;
}

static HRESULT WINAPI Disp_GetTypeInfo(
        IDispatch* This,
        UINT iTInfo,
        LCID lcid,
        ITypeInfo **ppTInfo)
{
    return ERROR_SUCCESS;
}

static HRESULT WINAPI Disp_GetIDsOfNames(
        IDispatch* This,
        REFIID riid,
        LPOLESTR *rgszNames,
        UINT cNames,
        LCID lcid,
        DISPID *rgDispId)
{
    return ERROR_SUCCESS;
}

static HRESULT WINAPI Disp_Invoke(
        IDispatch* This,
        DISPID dispIdMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS *pDispParams,
        VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo,
        UINT *puArgErr)
{
    trace("%p %lx %s %lx %x %p %p %p %p\n", This, dispIdMember, wine_dbgstr_guid(riid), lcid, wFlags,
          pDispParams, pVarResult, pExcepInfo, puArgErr);

    ok(dispIdMember == 0xa0 || dispIdMember == 0xa1, "Unknown dispIdMember\n");
    ok(pDispParams != NULL, "Invoked with NULL pDispParams\n");
    ok(wFlags == DISPATCH_METHOD, "Wrong flags %x\n",wFlags);
    ok(lcid == 0,"Wrong lcid %lx\n",lcid);
    if (dispIdMember == 0xa0)
    {
        ok(pDispParams->cArgs == 0, "params.cArgs = %d\n", pDispParams->cArgs);
        ok(pDispParams->cNamedArgs == 0, "params.cNamedArgs = %d\n", pDispParams->cArgs);
        ok(pDispParams->rgdispidNamedArgs == NULL, "params.rgdispidNamedArgs = %p\n", pDispParams->rgdispidNamedArgs);
        ok(pDispParams->rgvarg == NULL, "params.rgvarg = %p\n", pDispParams->rgvarg);
    }
    else if (dispIdMember == 0xa1)
    {
        ok(pDispParams->cArgs == 2, "params.cArgs = %d\n", pDispParams->cArgs);
        ok(pDispParams->cNamedArgs == 0, "params.cNamedArgs = %d\n", pDispParams->cArgs);
        ok(pDispParams->rgdispidNamedArgs == NULL, "params.rgdispidNamedArgs = %p\n", pDispParams->rgdispidNamedArgs);
        ok(V_VT(pDispParams->rgvarg) == VT_BSTR, "V_VT(var) = %d\n", V_VT(pDispParams->rgvarg));
        ok(V_I4(pDispParams->rgvarg) == 0xdeadcafe , "failed %p\n", V_BSTR(pDispParams->rgvarg));
        ok(V_VT(pDispParams->rgvarg+1) == VT_I4, "V_VT(var) = %d\n", V_VT(pDispParams->rgvarg+1));
        ok(V_I4(pDispParams->rgvarg+1) == 0xdeadbeef, "failed %lx\n", V_I4(pDispParams->rgvarg+1));
    }

    return ERROR_SUCCESS;
}

static const IDispatchVtbl disp_vtbl = {
    Disp_QueryInterface,
    Disp_AddRef,
    Disp_Release,

    Disp_GetTypeInfoCount,
    Disp_GetTypeInfo,
    Disp_GetIDsOfNames,
    Disp_Invoke
};

static HRESULT WINAPI Enum_QueryInterface(
        IEnumConnections* This,
        REFIID riid,
        void **ppvObject)
{
    *ppvObject = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IEnumConnections))
    {
        *ppvObject = This;
    }

    if (*ppvObject)
    {
        IEnumConnections_AddRef(This);
        return S_OK;
    }

    trace("no interface\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI Enum_AddRef(IEnumConnections* This)
{
    EnumCon *iface = impl_from_IEnumConnections(This);
    return InterlockedIncrement(&iface->refCount);
}

static ULONG WINAPI Enum_Release(IEnumConnections* This)
{
    EnumCon *iface = impl_from_IEnumConnections(This);
    ULONG ret;

    ret = InterlockedDecrement(&iface->refCount);
    if (ret == 0)
        HeapFree(GetProcessHeap(),0,This);
    return ret;
}

static HRESULT WINAPI Enum_Next(
        IEnumConnections* This,
        ULONG cConnections,
        LPCONNECTDATA rgcd,
        ULONG *pcFetched)
{
    EnumCon *iface = impl_from_IEnumConnections(This);

    if (cConnections > 0 && iface->idx < iface->pt->sinkCount)
    {
        rgcd->pUnk = iface->pt->sink[iface->idx];
        IUnknown_AddRef(iface->pt->sink[iface->idx]);
        rgcd->dwCookie=0xff;
        if (pcFetched)
            *pcFetched = 1;
        iface->idx++;
        return S_OK;
    }

    return E_FAIL;
}

static HRESULT WINAPI Enum_Skip(
        IEnumConnections* This,
        ULONG cConnections)
{
    return E_FAIL;
}

static HRESULT WINAPI Enum_Reset(
        IEnumConnections* This)
{
    return E_FAIL;
}

static HRESULT WINAPI Enum_Clone(
        IEnumConnections* This,
        IEnumConnections **ppEnum)
{
    return E_FAIL;
}

static const IEnumConnectionsVtbl enum_vtbl = {

    Enum_QueryInterface,
    Enum_AddRef,
    Enum_Release,
    Enum_Next,
    Enum_Skip,
    Enum_Reset,
    Enum_Clone
};

static HRESULT WINAPI ConPt_QueryInterface(
        IConnectionPoint* This,
        REFIID riid,
        void **ppvObject)
{
    *ppvObject = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IConnectionPoint))
    {
        *ppvObject = This;
    }

    if (*ppvObject)
    {
        IConnectionPoint_AddRef(This);
        return S_OK;
    }

    trace("no interface\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ConPt_AddRef(
        IConnectionPoint* This)
{
    ConPt *iface = impl_from_IConnectionPoint(This);
    return InterlockedIncrement(&iface->refCount);
}

static ULONG WINAPI ConPt_Release(
        IConnectionPoint* This)
{
    ConPt *iface = impl_from_IConnectionPoint(This);
    ULONG ret;

    ret = InterlockedDecrement(&iface->refCount);
    if (ret == 0)
    {
        if (iface->sinkCount > 0)
        {
            int i;
            for (i = 0; i < iface->sinkCount; i++)
            {
                if (iface->sink[i])
                    IUnknown_Release(iface->sink[i]);
            }
            HeapFree(GetProcessHeap(),0,iface->sink);
        }
        HeapFree(GetProcessHeap(),0,This);
    }
    return ret;
}

static HRESULT WINAPI ConPt_GetConnectionInterface(
        IConnectionPoint* This,
        IID *pIID)
{
    static int i = 0;
    ConPt *iface = impl_from_IConnectionPoint(This);
    if (i==0)
    {
        i++;
        return E_FAIL;
    }
    else
        memcpy(pIID,&iface->id,sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI ConPt_GetConnectionPointContainer(
        IConnectionPoint* This,
        IConnectionPointContainer **ppCPC)
{
    ConPt *iface = impl_from_IConnectionPoint(This);

    *ppCPC = &iface->container->IConnectionPointContainer_iface;
    return S_OK;
}

static HRESULT WINAPI ConPt_Advise(
        IConnectionPoint* This,
        IUnknown *pUnkSink,
        DWORD *pdwCookie)
{
    ConPt *iface = impl_from_IConnectionPoint(This);

    if (iface->sinkCount == 0)
        iface->sink = HeapAlloc(GetProcessHeap(),0,sizeof(IUnknown*));
    else
        iface->sink = HeapReAlloc(GetProcessHeap(),0,iface->sink,sizeof(IUnknown*)*(iface->sinkCount+1));
    iface->sink[iface->sinkCount] = pUnkSink;
    IUnknown_AddRef(pUnkSink);
    iface->sinkCount++;
    *pdwCookie = iface->sinkCount;
    return S_OK;
}

static HRESULT WINAPI ConPt_Unadvise(
        IConnectionPoint* This,
        DWORD dwCookie)
{
    ConPt *iface = impl_from_IConnectionPoint(This);

    if (dwCookie > iface->sinkCount)
        return E_FAIL;
    else
    {
        IUnknown_Release(iface->sink[dwCookie-1]);
        iface->sink[dwCookie-1] = NULL;
    }
    return S_OK;
}

static HRESULT WINAPI ConPt_EnumConnections(
        IConnectionPoint* This,
        IEnumConnections **ppEnum)
{
    EnumCon *ec;

    ec = HeapAlloc(GetProcessHeap(),0,sizeof(EnumCon));
    ec->IEnumConnections_iface.lpVtbl = &enum_vtbl;
    ec->refCount = 1;
    ec->pt = impl_from_IConnectionPoint(This);
    ec->idx = 0;
    *ppEnum = &ec->IEnumConnections_iface;

    return S_OK;
}

static const IConnectionPointVtbl point_vtbl = {
    ConPt_QueryInterface,
    ConPt_AddRef,
    ConPt_Release,

    ConPt_GetConnectionInterface,
    ConPt_GetConnectionPointContainer,
    ConPt_Advise,
    ConPt_Unadvise,
    ConPt_EnumConnections
};

static HRESULT WINAPI EnumPt_QueryInterface(
        IEnumConnectionPoints* This,
        REFIID riid,
        void **ppvObject)
{
    *ppvObject = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IEnumConnectionPoints))
    {
        *ppvObject = This;
    }

    if (*ppvObject)
    {
        IEnumConnectionPoints_AddRef(This);
        return S_OK;
    }

    trace("no interface\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI EnumPt_AddRef(IEnumConnectionPoints* This)
{
    EnumPt *iface = impl_from_IEnumConnectionPoints(This);
    return InterlockedIncrement(&iface->refCount);
}

static ULONG WINAPI EnumPt_Release(IEnumConnectionPoints* This)
{
    EnumPt *iface = impl_from_IEnumConnectionPoints(This);
    ULONG ret;

    ret = InterlockedDecrement(&iface->refCount);
    if (ret == 0)
        HeapFree(GetProcessHeap(),0,This);
    return ret;
}

static HRESULT WINAPI EnumPt_Next(
        IEnumConnectionPoints* This,
        ULONG cConnections,
        IConnectionPoint **rgcd,
        ULONG *pcFetched)
{
    EnumPt *iface = impl_from_IEnumConnectionPoints(This);

    if (cConnections > 0 && iface->idx < iface->container->ptCount)
    {
        *rgcd = iface->container->pt[iface->idx];
        IConnectionPoint_AddRef(iface->container->pt[iface->idx]);
        if (pcFetched)
            *pcFetched = 1;
        iface->idx++;
        return S_OK;
    }

    return E_FAIL;
}

static HRESULT WINAPI EnumPt_Skip(
        IEnumConnectionPoints* This,
        ULONG cConnections)
{
    return E_FAIL;
}

static HRESULT WINAPI EnumPt_Reset(
        IEnumConnectionPoints* This)
{
    return E_FAIL;
}

static HRESULT WINAPI EnumPt_Clone(
        IEnumConnectionPoints* This,
        IEnumConnectionPoints **ppEnumPt)
{
    return E_FAIL;
}

static const IEnumConnectionPointsVtbl enumpt_vtbl = {

    EnumPt_QueryInterface,
    EnumPt_AddRef,
    EnumPt_Release,
    EnumPt_Next,
    EnumPt_Skip,
    EnumPt_Reset,
    EnumPt_Clone
};

static HRESULT WINAPI Contain_QueryInterface(
        IConnectionPointContainer* This,
        REFIID riid,
        void **ppvObject)
{
    *ppvObject = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IConnectionPointContainer))
    {
        *ppvObject = This;
    }

    if (*ppvObject)
    {
        IConnectionPointContainer_AddRef(This);
        return S_OK;
    }

    trace("no interface\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI Contain_AddRef(
        IConnectionPointContainer* This)
{
    Contain *iface = impl_from_IConnectionPointContainer(This);
    return InterlockedIncrement(&iface->refCount);
}

static ULONG WINAPI Contain_Release(
        IConnectionPointContainer* This)
{
    Contain *iface = impl_from_IConnectionPointContainer(This);
    ULONG ret;

    ret = InterlockedDecrement(&iface->refCount);
    if (ret == 0)
    {
        if (iface->ptCount > 0)
        {
            int i;
            for (i = 0; i < iface->ptCount; i++)
                IConnectionPoint_Release(iface->pt[i]);
            HeapFree(GetProcessHeap(),0,iface->pt);
        }
        HeapFree(GetProcessHeap(),0,This);
    }
    return ret;
}

static HRESULT WINAPI Contain_EnumConnectionPoints(
        IConnectionPointContainer* This,
        IEnumConnectionPoints **ppEnum)
{
    EnumPt *ec;

    ec = HeapAlloc(GetProcessHeap(),0,sizeof(EnumPt));
    ec->IEnumConnectionPoints_iface.lpVtbl = &enumpt_vtbl;
    ec->refCount = 1;
    ec->idx= 0;
    ec->container = impl_from_IConnectionPointContainer(This);
    *ppEnum = &ec->IEnumConnectionPoints_iface;

    return S_OK;
}

static HRESULT WINAPI Contain_FindConnectionPoint(
        IConnectionPointContainer* This,
        REFIID riid,
        IConnectionPoint **ppCP)
{
    Contain *iface = impl_from_IConnectionPointContainer(This);
    ConPt *pt;

    if (!IsEqualIID(riid, &IID_NULL) || iface->ptCount ==0)
    {
        pt = HeapAlloc(GetProcessHeap(),0,sizeof(ConPt));
        pt->IConnectionPoint_iface.lpVtbl = &point_vtbl;
        pt->refCount = 1;
        pt->sinkCount = 0;
        pt->sink = NULL;
        pt->container = iface;
        pt->id = IID_IDispatch;

        if (iface->ptCount == 0)
            iface->pt =HeapAlloc(GetProcessHeap(),0,sizeof(IUnknown*));
        else
            iface->pt = HeapReAlloc(GetProcessHeap(),0,iface->pt,sizeof(IUnknown*)*(iface->ptCount+1));
        iface->pt[iface->ptCount] = &pt->IConnectionPoint_iface;
        iface->ptCount++;

        *ppCP = &pt->IConnectionPoint_iface;
    }
    else
    {
        *ppCP = iface->pt[0];
        IConnectionPoint_AddRef(*ppCP);
    }

    return S_OK;
}

static const IConnectionPointContainerVtbl contain_vtbl = {
    Contain_QueryInterface,
    Contain_AddRef,
    Contain_Release,

    Contain_EnumConnectionPoints,
    Contain_FindConnectionPoint
};

static void test_IConnectionPoint(void)
{
    HRESULT rc;
    ULONG ref;
    IConnectionPoint *point;
    Contain *container;
    Disp *dispatch;
    DWORD cookie = 0xffffffff;
    DISPPARAMS params;
    VARIANT vars[10];

    container = HeapAlloc(GetProcessHeap(),0,sizeof(Contain));
    container->IConnectionPointContainer_iface.lpVtbl = &contain_vtbl;
    container->refCount = 1;
    container->ptCount = 0;
    container->pt = NULL;

    dispatch = HeapAlloc(GetProcessHeap(),0,sizeof(Disp));
    dispatch->IDispatch_iface.lpVtbl = &disp_vtbl;
    dispatch->refCount = 1;

    rc = pConnectToConnectionPoint((IUnknown*)&dispatch->IDispatch_iface, &IID_NULL, TRUE,
                                   (IUnknown*)&container->IConnectionPointContainer_iface, &cookie, &point);
    ok(rc == S_OK, "pConnectToConnectionPoint failed with %lx\n",rc);
    ok(point != NULL, "returned ConnectionPoint is NULL\n");
    ok(cookie != 0xffffffff, "invalid cookie returned\n");

    rc = pIConnectionPoint_SimpleInvoke(point,0xa0,NULL);
    ok(rc == S_OK, "pConnectToConnectionPoint failed with %lx\n",rc);

    memset(&params, 0xc0, sizeof(params));
    memset(vars, 0xc0, sizeof(vars));
    rc = pSHPackDispParams(&params, vars, 2, VT_I4, 0xdeadbeef, VT_BSTR, 0xdeadcafe);
    ok(rc == S_OK, "SHPackDispParams failed: %08lx\n", rc);

    rc = pIConnectionPoint_SimpleInvoke(point,0xa1,&params);
    ok(rc == S_OK, "pConnectToConnectionPoint failed with %lx\n",rc);

    rc = pConnectToConnectionPoint(NULL, &IID_NULL, FALSE,
                                   (IUnknown*)&container->IConnectionPointContainer_iface, &cookie, NULL);
    ok(rc == S_OK, "pConnectToConnectionPoint failed with %lx\n",rc);

/* MSDN says this should be required but it crashes on XP
    IUnknown_Release(point);
*/
    ref = IConnectionPointContainer_Release(&container->IConnectionPointContainer_iface);
    ok(ref == 0, "leftover IConnectionPointContainer reference %li\n",ref);
    ref = IDispatch_Release(&dispatch->IDispatch_iface);
    ok(ref == 0, "leftover IDispatch reference %li\n",ref);
}

typedef struct _propbag
{
    IPropertyBag IPropertyBag_iface;
    LONG   refCount;

} PropBag;

static inline PropBag *impl_from_IPropertyBag(IPropertyBag *iface)
{
    return CONTAINING_RECORD(iface, PropBag, IPropertyBag_iface);
}


static HRESULT WINAPI Prop_QueryInterface(
        IPropertyBag* This,
        REFIID riid,
        void **ppvObject)
{
    *ppvObject = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IPropertyBag))
    {
        *ppvObject = This;
    }

    if (*ppvObject)
    {
        IPropertyBag_AddRef(This);
        return S_OK;
    }

    trace("no interface\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI Prop_AddRef(
        IPropertyBag* This)
{
    PropBag *iface = impl_from_IPropertyBag(This);
    return InterlockedIncrement(&iface->refCount);
}

static ULONG WINAPI Prop_Release(
        IPropertyBag* This)
{
    PropBag *iface = impl_from_IPropertyBag(This);
    ULONG ret;

    ret = InterlockedDecrement(&iface->refCount);
    if (ret == 0)
        HeapFree(GetProcessHeap(),0,This);
    return ret;
}

static HRESULT WINAPI Prop_Read(
        IPropertyBag* This,
        LPCOLESTR pszPropName,
        VARIANT *pVar,
        IErrorLog *pErrorLog)
{
    V_VT(pVar) = VT_BLOB|VT_BYREF;
    V_BYREF(pVar) = (LPVOID)0xdeadcafe;
    return S_OK;
}

static HRESULT WINAPI Prop_Write(
        IPropertyBag* This,
        LPCOLESTR pszPropName,
        VARIANT *pVar)
{
    return S_OK;
}


static const IPropertyBagVtbl prop_vtbl = {
    Prop_QueryInterface,
    Prop_AddRef,
    Prop_Release,

    Prop_Read,
    Prop_Write
};

static void test_SHPropertyBag_ReadLONG(void)
{
    PropBag *pb;
    HRESULT rc;
    LONG out;
    static const WCHAR szName1[] = {'n','a','m','e','1',0};

    pb = HeapAlloc(GetProcessHeap(),0,sizeof(PropBag));
    pb->refCount = 1;
    pb->IPropertyBag_iface.lpVtbl = &prop_vtbl;

    out = 0xfeedface;
    rc = pSHPropertyBag_ReadLONG(NULL, szName1, &out);
    ok(rc == E_INVALIDARG || broken(rc == S_OK), "incorrect return %lx\n",rc);
    ok(out == 0xfeedface, "value should not have changed\n");
    rc = pSHPropertyBag_ReadLONG(&pb->IPropertyBag_iface, NULL, &out);
    ok(rc == E_INVALIDARG || broken(rc == S_OK) || broken(rc == S_FALSE), "incorrect return %lx\n",rc);
    ok(out == 0xfeedface, "value should not have changed\n");
    rc = pSHPropertyBag_ReadLONG(&pb->IPropertyBag_iface, szName1, NULL);
    ok(rc == E_INVALIDARG || broken(rc == S_OK) || broken(rc == S_FALSE), "incorrect return %lx\n",rc);
    rc = pSHPropertyBag_ReadLONG(&pb->IPropertyBag_iface, szName1, &out);
    ok(rc == DISP_E_BADVARTYPE || broken(rc == S_OK) || broken(rc == S_FALSE), "incorrect return %lx\n",rc);
    ok(out == 0xfeedface  || broken(out == 0xfeedfa00), "value should not have changed %lx\n",out);
    IPropertyBag_Release(&pb->IPropertyBag_iface);
}

static void test_SHSetWindowBits(void)
{
    HWND hwnd;
    DWORD style, styleold;
    WNDCLASSA clsA;

    clsA.style = 0;
    clsA.lpfnWndProc = DefWindowProcA;
    clsA.cbClsExtra = 0;
    clsA.cbWndExtra = 0;
    clsA.hInstance = GetModuleHandleA(NULL);
    clsA.hIcon = 0;
    clsA.hCursor = LoadCursorA(0, (LPSTR)IDC_ARROW);
    clsA.hbrBackground = NULL;
    clsA.lpszMenuName = NULL;
    clsA.lpszClassName = "Shlwapi test class";
    RegisterClassA(&clsA);

    hwnd = CreateWindowA("Shlwapi test class", "Test", WS_VISIBLE, 0, 0, 100, 100,
                          NULL, NULL, GetModuleHandleA(NULL), 0);
    ok(IsWindow(hwnd), "failed to create window\n");

    /* null window */
    SetLastError(0xdeadbeef);
    style = pSHSetWindowBits(NULL, GWL_STYLE, 0, 0);
    ok(style == 0, "expected 0 retval, got %ld\n", style);
    ok(GetLastError() == ERROR_INVALID_WINDOW_HANDLE,
        "expected ERROR_INVALID_WINDOW_HANDLE, got %ld\n", GetLastError());

    /* zero mask, zero flags */
    styleold = GetWindowLongA(hwnd, GWL_STYLE);
    style = pSHSetWindowBits(hwnd, GWL_STYLE, 0, 0);
    ok(styleold == style, "expected old style\n");
    ok(styleold == GetWindowLongA(hwnd, GWL_STYLE), "expected to keep old style\n");

    /* test mask */
    styleold = GetWindowLongA(hwnd, GWL_STYLE);
    ok(styleold & WS_VISIBLE, "expected WS_VISIBLE\n");
    style = pSHSetWindowBits(hwnd, GWL_STYLE, WS_VISIBLE, 0);

    ok(style == styleold, "expected previous style, got %lx\n", style);
    ok((GetWindowLongA(hwnd, GWL_STYLE) & WS_VISIBLE) == 0, "expected updated style\n");

    /* test mask, unset style bit used */
    styleold = GetWindowLongA(hwnd, GWL_STYLE);
    style = pSHSetWindowBits(hwnd, GWL_STYLE, WS_VISIBLE, 0);
    ok(style == styleold, "expected previous style, got %lx\n", style);
    ok(styleold == GetWindowLongA(hwnd, GWL_STYLE), "expected to keep old style\n");

    /* set back with flags */
    styleold = GetWindowLongA(hwnd, GWL_STYLE);
    style = pSHSetWindowBits(hwnd, GWL_STYLE, WS_VISIBLE, WS_VISIBLE);
    ok(style == styleold, "expected previous style, got %lx\n", style);
    ok(GetWindowLongA(hwnd, GWL_STYLE) & WS_VISIBLE, "expected updated style\n");

    /* reset and try to set without a mask */
    pSHSetWindowBits(hwnd, GWL_STYLE, WS_VISIBLE, 0);
    ok((GetWindowLongA(hwnd, GWL_STYLE) & WS_VISIBLE) == 0, "expected updated style\n");
    styleold = GetWindowLongA(hwnd, GWL_STYLE);
    style = pSHSetWindowBits(hwnd, GWL_STYLE, 0, WS_VISIBLE);
    ok(style == styleold, "expected previous style, got %lx\n", style);
    ok((GetWindowLongA(hwnd, GWL_STYLE) & WS_VISIBLE) == 0, "expected updated style\n");

    DestroyWindow(hwnd);

    UnregisterClassA("Shlwapi test class", GetModuleHandleA(NULL));
}

static void test_SHFormatDateTimeA(const SYSTEMTIME *st)
{
    FILETIME filetime;
    FILETIME filetimeCheck;
    SYSTEMTIME universalSystemTime;
    CHAR buff[100], buff2[100], buff3[100];
    BOOL dstMatch;
    DWORD flags;
    INT ret;

    /* SHFormatDateTime expects input as utc */
    TzSpecificLocalTimeToSystemTime(NULL, st, &universalSystemTime);
    SystemTimeToFileTime(&universalSystemTime, &filetime);

    SystemTimeToFileTime(st, &filetimeCheck);
    LocalFileTimeToFileTime(&filetimeCheck, &filetimeCheck);
    dstMatch = (filetime.dwHighDateTime == filetimeCheck.dwHighDateTime) &&
               (filetime.dwLowDateTime == filetimeCheck.dwLowDateTime);

    /* no way to get required buffer length here */
    ret = pSHFormatDateTimeA(&filetime, NULL, NULL, 0);
    ok(ret == 0, "got %d\n", ret);

    SetLastError(0xdeadbeef);
    buff[0] = 'a'; buff[1] = 0;
    ret = pSHFormatDateTimeA(&filetime, NULL, buff, 0);
    ok(ret == 0, "got %d\n", ret);
    ok(GetLastError() == 0xdeadbeef, "expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(buff[0] == 'a', "expected same string, got %s\n", buff);

    /* flags needs to have FDTF_NOAUTOREADINGORDER for these tests to succeed on Vista+ */

    /* all combinations documented as invalid succeeded */
    flags = FDTF_NOAUTOREADINGORDER | FDTF_SHORTTIME | FDTF_LONGTIME;
    SetLastError(0xdeadbeef);
    ret = pSHFormatDateTimeA(&filetime, &flags, buff, sizeof(buff));
    ok(ret == lstrlenA(buff)+1, "got %d\n", ret);
    ok(GetLastError() == 0xdeadbeef, "expected 0xdeadbeef, got %ld\n", GetLastError());

    flags = FDTF_NOAUTOREADINGORDER | FDTF_SHORTDATE | FDTF_LONGDATE;
    SetLastError(0xdeadbeef);
    ret = pSHFormatDateTimeA(&filetime, &flags, buff, sizeof(buff));
    ok(ret == lstrlenA(buff)+1, "got %d\n", ret);
    ok(GetLastError() == 0xdeadbeef, "expected 0xdeadbeef, got %ld\n", GetLastError());

    flags =  FDTF_SHORTDATE | FDTF_LTRDATE | FDTF_RTLDATE;
    SetLastError(0xdeadbeef);
    ret = pSHFormatDateTimeA(&filetime, &flags, buff, sizeof(buff));
    ok(ret == lstrlenA(buff)+1, "got %d\n", ret);
    ok(GetLastError() == 0xdeadbeef,
        "expected 0xdeadbeef, got %ld\n", GetLastError());

    flags = FDTF_DEFAULT;
    ret = pSHFormatDateTimeA(&filetime, &flags, buff, sizeof(buff));
    ok(ret == lstrlenA(buff)+1, "got %d\n", ret);

    buff2[0] = '\0';
    flags = FDTF_SHORTDATE | FDTF_SHORTTIME;
    ret = pSHFormatDateTimeA(&filetime, &flags, buff2, sizeof(buff2));
    ok(ret == lstrlenA(buff2)+1, "got %d\n", ret);
    ok(lstrcmpA(buff, buff2) == 0, "expected (%s), got (%s)\n", buff, buff2);

    buff2[0] = '\0';
    ret = pSHFormatDateTimeA(&filetime, NULL, buff2, sizeof(buff2));
    ok(ret == lstrlenA(buff2)+1, "got %d\n", ret);
    ok(lstrcmpA(buff, buff2) == 0, "expected (%s), got (%s)\n", buff, buff2);

    /* now check returned strings */
    flags = FDTF_NOAUTOREADINGORDER | FDTF_SHORTTIME;
    ret = pSHFormatDateTimeA(&filetime, &flags, buff, sizeof(buff));
    ok(ret == lstrlenA(buff)+1, "got %d\n", ret);
    ret = GetTimeFormatA(LOCALE_USER_DEFAULT, TIME_NOSECONDS | LOCALE_USE_CP_ACP, st, NULL, buff2, sizeof(buff2));
    ok(ret == lstrlenA(buff2)+1, "got %d\n", ret);
    ok(lstrcmpA(buff, buff2) == 0 || broken(!dstMatch) /* pre Windows 7 */,
        "expected (%s), got (%s)\n", buff2, buff);

    flags = FDTF_NOAUTOREADINGORDER | FDTF_LONGTIME;
    ret = pSHFormatDateTimeA(&filetime, &flags, buff, sizeof(buff));
    ok(ret == lstrlenA(buff)+1, "got %d\n", ret);
    ret = GetTimeFormatA(LOCALE_USER_DEFAULT, LOCALE_USE_CP_ACP, st, NULL, buff2, sizeof(buff2));
    ok(ret == lstrlenA(buff2)+1, "got %d\n", ret);
    ok(lstrcmpA(buff, buff2) == 0 || broken(!dstMatch) /* pre Windows 7 */,
        "expected (%s), got (%s)\n", buff2, buff);

    /* both time flags */
    flags = FDTF_NOAUTOREADINGORDER | FDTF_LONGTIME | FDTF_SHORTTIME;
    ret = pSHFormatDateTimeA(&filetime, &flags, buff, sizeof(buff));
    ok(ret == lstrlenA(buff)+1, "got %d\n", ret);
    ret = GetTimeFormatA(LOCALE_USER_DEFAULT, LOCALE_USE_CP_ACP, st, NULL, buff2, sizeof(buff2));
    ok(ret == lstrlenA(buff2)+1, "got %d\n", ret);
    ok(lstrcmpA(buff, buff2) == 0 || broken(!dstMatch) /* pre Windows 7 */,
        "expected (%s), got (%s)\n", buff2, buff);

    flags = FDTF_NOAUTOREADINGORDER | FDTF_SHORTDATE;
    ret = pSHFormatDateTimeA(&filetime, &flags, buff, sizeof(buff));
    ok(ret == lstrlenA(buff)+1, "got %d\n", ret);
    ret = GetDateFormatA(LOCALE_USER_DEFAULT, DATE_SHORTDATE | LOCALE_USE_CP_ACP, st, NULL, buff2, sizeof(buff2));
    ok(ret == lstrlenA(buff2)+1, "got %d\n", ret);
    ok(lstrcmpA(buff, buff2) == 0, "expected (%s), got (%s)\n", buff2, buff);

    flags = FDTF_NOAUTOREADINGORDER | FDTF_LONGDATE;
    ret = pSHFormatDateTimeA(&filetime, &flags, buff, sizeof(buff));
    ok(ret == lstrlenA(buff)+1, "got %d\n", ret);
    ret = GetDateFormatA(LOCALE_USER_DEFAULT, DATE_LONGDATE | LOCALE_USE_CP_ACP, st, NULL, buff2, sizeof(buff2));
    ok(ret == lstrlenA(buff2)+1, "got %d\n", ret);
    ok(lstrcmpA(buff, buff2) == 0, "expected (%s), got (%s)\n", buff2, buff);

    /* both date flags */
    flags = FDTF_NOAUTOREADINGORDER | FDTF_LONGDATE | FDTF_SHORTDATE;
    ret = pSHFormatDateTimeA(&filetime, &flags, buff, sizeof(buff));
    ok(ret == lstrlenA(buff)+1, "got %d\n", ret);
    ret = GetDateFormatA(LOCALE_USER_DEFAULT, DATE_LONGDATE | LOCALE_USE_CP_ACP, st, NULL, buff2, sizeof(buff2));
    ok(ret == lstrlenA(buff2)+1, "got %d\n", ret);
    ok(lstrcmpA(buff, buff2) == 0, "expected (%s), got (%s)\n", buff2, buff);

    /* various combinations of date/time flags */
    flags = FDTF_NOAUTOREADINGORDER | FDTF_LONGDATE | FDTF_SHORTTIME;
    ret = pSHFormatDateTimeA(&filetime, &flags, buff, sizeof(buff));
    ok(ret == lstrlenA(buff)+1, "got %d, length %d\n", ret, lstrlenA(buff)+1);
    ret = GetTimeFormatA(LOCALE_USER_DEFAULT, TIME_NOSECONDS | LOCALE_USE_CP_ACP, st, NULL, buff3, sizeof(buff3));
    ok(ret == lstrlenA(buff3)+1, "got %d\n", ret);
    ok(lstrcmpA(buff3, buff + lstrlenA(buff) - lstrlenA(buff3)) == 0 || broken(!dstMatch) /* pre Windows 7 */,
       "expected (%s), got (%s) for time part\n",
       buff3, buff + lstrlenA(buff) - lstrlenA(buff3));
    ret = GetDateFormatA(LOCALE_USER_DEFAULT, DATE_LONGDATE | LOCALE_USE_CP_ACP, st, NULL, buff2, sizeof(buff2));
    ok(ret == lstrlenA(buff2)+1, "got %d\n", ret);
    buff[lstrlenA(buff2)] = '\0';
    ok(lstrcmpA(buff2, buff) == 0, "expected (%s) got (%s) for date part\n",
       buff2, buff);

    flags = FDTF_NOAUTOREADINGORDER | FDTF_LONGDATE | FDTF_LONGTIME;
    ret = pSHFormatDateTimeA(&filetime, &flags, buff, sizeof(buff));
    ok(ret == lstrlenA(buff)+1, "got %d\n", ret);
    ret = GetTimeFormatA(LOCALE_USER_DEFAULT, LOCALE_USE_CP_ACP, st, NULL, buff3, sizeof(buff3));
    ok(ret == lstrlenA(buff3)+1, "got %d\n", ret);
    ok(lstrcmpA(buff3, buff + lstrlenA(buff) - lstrlenA(buff3)) == 0 || broken(!dstMatch) /* pre Windows 7 */,
       "expected (%s), got (%s) for time part\n",
       buff3, buff + lstrlenA(buff) - lstrlenA(buff3));
    ret = GetDateFormatA(LOCALE_USER_DEFAULT, DATE_LONGDATE | LOCALE_USE_CP_ACP, st, NULL, buff2, sizeof(buff2));
    ok(ret == lstrlenA(buff2)+1, "got %d\n", ret);
    buff[lstrlenA(buff2)] = '\0';
    ok(lstrcmpA(buff2, buff) == 0, "expected (%s) got (%s) for date part\n",
       buff2, buff);

    flags = FDTF_NOAUTOREADINGORDER | FDTF_SHORTDATE | FDTF_SHORTTIME;
    ret = pSHFormatDateTimeA(&filetime, &flags, buff, sizeof(buff));
    ok(ret == lstrlenA(buff)+1, "got %d\n", ret);
    ret = GetDateFormatA(LOCALE_USER_DEFAULT, DATE_SHORTDATE | LOCALE_USE_CP_ACP, st, NULL, buff2, sizeof(buff2));
    ok(ret == lstrlenA(buff2)+1, "got %d\n", ret);
    strcat(buff2, " ");
    ret = GetTimeFormatA(LOCALE_USER_DEFAULT, TIME_NOSECONDS | LOCALE_USE_CP_ACP, st, NULL, buff3, sizeof(buff3));
    ok(ret == lstrlenA(buff3)+1, "got %d\n", ret);
    strcat(buff2, buff3);
    ok(lstrcmpA(buff, buff2) == 0 || broken(!dstMatch) /* pre Windows 7 */,
        "expected (%s), got (%s)\n", buff2, buff);

    flags = FDTF_NOAUTOREADINGORDER | FDTF_SHORTDATE | FDTF_LONGTIME;
    ret = pSHFormatDateTimeA(&filetime, &flags, buff, sizeof(buff));
    ok(ret == lstrlenA(buff)+1, "got %d\n", ret);
    ret = GetDateFormatA(LOCALE_USER_DEFAULT, DATE_SHORTDATE | LOCALE_USE_CP_ACP, st, NULL, buff2, sizeof(buff2));
    ok(ret == lstrlenA(buff2)+1, "got %d\n", ret);
    strcat(buff2, " ");
    ret = GetTimeFormatA(LOCALE_USER_DEFAULT, LOCALE_USE_CP_ACP, st, NULL, buff3, sizeof(buff3));
    ok(ret == lstrlenA(buff3)+1, "got %d\n", ret);
    strcat(buff2, buff3);
    ok(lstrcmpA(buff, buff2) == 0 || broken(!dstMatch) /* pre Windows 7 */,
        "expected (%s), got (%s)\n", buff2, buff);
}

static void test_SHFormatDateTimeW(void)
{
    FILETIME filetime;
    SYSTEMTIME universalSystemTime;
    WCHAR buff[100], buff2[100], buff3[100], *p1, *p2;
    SYSTEMTIME st;
    DWORD flags;
    INT ret;
    static const WCHAR spaceW[] = {' ',0};
#define UNICODE_LTR_MARK 0x200e
#define UNICODE_RTL_MARK 0x200f

if (0)
{
    /* crashes on native */
    pSHFormatDateTimeW(NULL, NULL, NULL, 0);
}

    GetLocalTime(&st);
    /* SHFormatDateTime expects input as utc */
    TzSpecificLocalTimeToSystemTime(NULL, &st, &universalSystemTime);
    SystemTimeToFileTime(&universalSystemTime, &filetime);

    /* no way to get required buffer length here */
    SetLastError(0xdeadbeef);
    ret = pSHFormatDateTimeW(&filetime, NULL, NULL, 0);
    ok(ret == 0, "expected 0, got %d\n", ret);
    ok(GetLastError() == 0xdeadbeef, "expected 0xdeadbeef, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    buff[0] = 'a'; buff[1] = 0;
    ret = pSHFormatDateTimeW(&filetime, NULL, buff, 0);
    ok(ret == 0, "expected 0, got %d\n", ret);
    ok(GetLastError() == 0xdeadbeef, "expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(buff[0] == 'a', "expected same string\n");

    /* all combinations documented as invalid succeeded */
    flags = FDTF_SHORTTIME | FDTF_LONGTIME;
    SetLastError(0xdeadbeef);
    ret = pSHFormatDateTimeW(&filetime, &flags, buff, ARRAY_SIZE(buff));
    ok(ret == lstrlenW(buff)+1 || ret == lstrlenW(buff),
       "expected %d or %d, got %d\n", lstrlenW(buff)+1, lstrlenW(buff), ret);
    ok(GetLastError() == 0xdeadbeef, "expected 0xdeadbeef, got %ld\n", GetLastError());

    flags = FDTF_SHORTDATE | FDTF_LONGDATE;
    SetLastError(0xdeadbeef);
    ret = pSHFormatDateTimeW(&filetime, &flags, buff, ARRAY_SIZE(buff));
    ok(ret == lstrlenW(buff)+1 || ret == lstrlenW(buff),
       "expected %d or %d, got %d\n", lstrlenW(buff)+1, lstrlenW(buff), ret);
    ok(GetLastError() == 0xdeadbeef, "expected 0xdeadbeef, got %ld\n", GetLastError());

    flags = FDTF_SHORTDATE | FDTF_LTRDATE | FDTF_RTLDATE;
    SetLastError(0xdeadbeef);
    buff[0] = 0; /* NT4 doesn't clear the buffer on failure */
    ret = pSHFormatDateTimeW(&filetime, &flags, buff, ARRAY_SIZE(buff));
    ok(ret == lstrlenW(buff)+1 || ret == lstrlenW(buff),
       "expected %d or %d, got %d\n", lstrlenW(buff)+1, lstrlenW(buff), ret);
    ok(GetLastError() == 0xdeadbeef,
        "expected 0xdeadbeef, got %ld\n", GetLastError());

    /* now check returned strings */
    flags = FDTF_SHORTTIME;
    ret = pSHFormatDateTimeW(&filetime, &flags, buff, ARRAY_SIZE(buff));
    ok(ret == lstrlenW(buff)+1 || ret == lstrlenW(buff),
       "expected %d or %d, got %d\n", lstrlenW(buff)+1, lstrlenW(buff), ret);
    SetLastError(0xdeadbeef);
    ret = GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, buff2, ARRAY_SIZE(buff2));
    ok(ret == lstrlenW(buff2)+1, "expected %d, got %d\n", lstrlenW(buff2)+1, ret);
    ok(lstrcmpW(buff, buff2) == 0, "expected equal strings\n");

    flags = FDTF_LONGTIME;
    ret = pSHFormatDateTimeW(&filetime, &flags, buff, ARRAY_SIZE(buff));
    ok(ret == lstrlenW(buff)+1 || ret == lstrlenW(buff),
       "expected %d or %d, got %d\n", lstrlenW(buff)+1, lstrlenW(buff), ret);
    ret = GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &st, NULL, buff2, ARRAY_SIZE(buff2));
    ok(ret == lstrlenW(buff2)+1, "expected %d, got %d\n", lstrlenW(buff2)+1, ret);
    ok(lstrcmpW(buff, buff2) == 0, "expected equal strings\n");

    /* both time flags */
    flags = FDTF_LONGTIME | FDTF_SHORTTIME;
    ret = pSHFormatDateTimeW(&filetime, &flags, buff, ARRAY_SIZE(buff));
    ok(ret == lstrlenW(buff)+1 || ret == lstrlenW(buff),
       "expected %d or %d, got %d\n", lstrlenW(buff)+1, lstrlenW(buff), ret);
    ret = GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &st, NULL, buff2, ARRAY_SIZE(buff2));
    ok(ret == lstrlenW(buff2)+1, "expected %d, got %d\n", lstrlenW(buff2)+1, ret);
    ok(lstrcmpW(buff, buff2) == 0, "expected equal string\n");

    flags = FDTF_SHORTDATE;
    ret = pSHFormatDateTimeW(&filetime, &flags, buff, ARRAY_SIZE(buff));
    ok(ret == lstrlenW(buff)+1 || ret == lstrlenW(buff),
       "expected %d or %d, got %d\n", lstrlenW(buff)+1, lstrlenW(buff), ret);
    ret = GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, buff2, ARRAY_SIZE(buff2));
    ok(ret == lstrlenW(buff2)+1, "expected %d, got %d\n", lstrlenW(buff2)+1, ret);
    ok(lstrcmpW(buff, buff2) == 0, "expected equal strings\n");

    flags = FDTF_LONGDATE;
    ret = pSHFormatDateTimeW(&filetime, &flags, buff, ARRAY_SIZE(buff));
    ok(ret == lstrlenW(buff)+1 || ret == lstrlenW(buff),
       "expected %d or %d, got %d\n", lstrlenW(buff)+1, lstrlenW(buff), ret);
    ret = GetDateFormatW(LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL, buff2, ARRAY_SIZE(buff2));
    ok(ret == lstrlenW(buff2)+1, "expected %d, got %d\n", lstrlenW(buff2)+1, ret);
    ok(lstrcmpW(buff, buff2) == 0, "expected equal strings\n");

    /* both date flags */
    flags = FDTF_LONGDATE | FDTF_SHORTDATE;
    ret = pSHFormatDateTimeW(&filetime, &flags, buff, ARRAY_SIZE(buff));
    ok(ret == lstrlenW(buff)+1 || ret == lstrlenW(buff),
       "expected %d or %d, got %d\n", lstrlenW(buff)+1, lstrlenW(buff), ret);
    ret = GetDateFormatW(LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL, buff2, ARRAY_SIZE(buff2));
    ok(ret == lstrlenW(buff2)+1, "expected %d, got %d\n", lstrlenW(buff2)+1, ret);
    ok(lstrcmpW(buff, buff2) == 0, "expected equal strings\n");

    /* various combinations of date/time flags */
    flags = FDTF_LONGDATE | FDTF_SHORTTIME;
    ret = pSHFormatDateTimeW(&filetime, &flags, buff, ARRAY_SIZE(buff));
    ok(ret == lstrlenW(buff)+1 || ret == lstrlenW(buff),
       "expected %d or %d, got %d\n", lstrlenW(buff)+1, lstrlenW(buff), ret);
    ret = GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, buff3, ARRAY_SIZE(buff3));
    ok(ret == lstrlenW(buff3)+1, "expected %d, got %d\n", lstrlenW(buff3)+1, ret);
    ok(lstrcmpW(buff3, buff + lstrlenW(buff) - lstrlenW(buff3)) == 0,
       "expected (%s), got (%s) for time part\n",
       wine_dbgstr_w(buff3), wine_dbgstr_w(buff + lstrlenW(buff) - lstrlenW(buff3)));
    ret = GetDateFormatW(LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL, buff2, ARRAY_SIZE(buff2));
    ok(ret == lstrlenW(buff2)+1, "expected %d, got %d\n", lstrlenW(buff2)+1, ret);
    p1 = buff;
    p2 = buff2;
    while (*p2 != '\0')
    {
        while (*p1 == UNICODE_LTR_MARK || *p1 == UNICODE_RTL_MARK)
            p1++;
        while (*p2 == UNICODE_LTR_MARK || *p2 == UNICODE_RTL_MARK)
            p2++;
        p1++;
        p2++;
    }
    *p1 = '\0';
    ok(lstrcmpW(buff2, buff) == 0, "expected (%s) got (%s) for date part\n",
       wine_dbgstr_w(buff2), wine_dbgstr_w(buff));

    flags = FDTF_LONGDATE | FDTF_LONGTIME;
    ret = pSHFormatDateTimeW(&filetime, &flags, buff, ARRAY_SIZE(buff));
    ok(ret == lstrlenW(buff)+1 || ret == lstrlenW(buff),
       "expected %d or %d, got %d\n", lstrlenW(buff)+1, lstrlenW(buff), ret);
    ret = GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &st, NULL, buff3, ARRAY_SIZE(buff3));
    ok(ret == lstrlenW(buff3)+1, "expected %d, got %d\n", lstrlenW(buff3)+1, ret);
    ok(lstrcmpW(buff3, buff + lstrlenW(buff) - lstrlenW(buff3)) == 0,
       "expected (%s), got (%s) for time part\n",
       wine_dbgstr_w(buff3), wine_dbgstr_w(buff + lstrlenW(buff) - lstrlenW(buff3)));
    ret = GetDateFormatW(LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL, buff2, ARRAY_SIZE(buff2));
    ok(ret == lstrlenW(buff2)+1, "expected %d, got %d\n", lstrlenW(buff2)+1, ret);
    p1 = buff;
    p2 = buff2;
    while (*p2 != '\0')
    {
        while (*p1 == UNICODE_LTR_MARK || *p1 == UNICODE_RTL_MARK)
            p1++;
        while (*p2 == UNICODE_LTR_MARK || *p2 == UNICODE_RTL_MARK)
            p2++;
        p1++;
        p2++;
    }
    *p1 = '\0';
    ok(lstrcmpW(buff2, buff) == 0, "expected (%s) got (%s) for date part\n",
       wine_dbgstr_w(buff2), wine_dbgstr_w(buff));

    flags = FDTF_SHORTDATE | FDTF_SHORTTIME;
    ret = pSHFormatDateTimeW(&filetime, &flags, buff, ARRAY_SIZE(buff));
    ok(ret == lstrlenW(buff)+1 || ret == lstrlenW(buff),
       "expected %d or %d, got %d\n", lstrlenW(buff)+1, lstrlenW(buff), ret);
    ret = GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, buff2, ARRAY_SIZE(buff2));
    ok(ret == lstrlenW(buff2)+1, "expected %d, got %d\n", lstrlenW(buff2)+1, ret);
    lstrcatW(buff2, spaceW);
    ret = GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, buff3, ARRAY_SIZE(buff3));
    ok(ret == lstrlenW(buff3)+1, "expected %d, got %d\n", lstrlenW(buff3)+1, ret);
    lstrcatW(buff2, buff3);
    ok(lstrcmpW(buff, buff2) == 0, "expected equal strings\n");

    flags = FDTF_SHORTDATE | FDTF_LONGTIME;
    ret = pSHFormatDateTimeW(&filetime, &flags, buff, ARRAY_SIZE(buff));
    ok(ret == lstrlenW(buff)+1 || ret == lstrlenW(buff),
       "expected %d or %d, got %d\n", lstrlenW(buff)+1, lstrlenW(buff), ret);
    ret = GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, buff2, ARRAY_SIZE(buff2));
    ok(ret == lstrlenW(buff2)+1, "expected %d, got %d\n", lstrlenW(buff2)+1, ret);
    lstrcatW(buff2, spaceW);
    ret = GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &st, NULL, buff3, ARRAY_SIZE(buff3));
    ok(ret == lstrlenW(buff3)+1, "expected %d, got %d\n", lstrlenW(buff3)+1, ret);
    lstrcatW(buff2, buff3);
    ok(lstrcmpW(buff, buff2) == 0, "expected equal strings\n");
}

static void test_SHGetObjectCompatFlags(void)
{
    struct compat_value {
        CHAR nameA[30];
        DWORD value;
    };

    struct compat_value values[] = {
        { "OTNEEDSSFCACHE", 0x1 },
        { "NO_WEBVIEW", 0x2 },
        { "UNBINDABLE", 0x4 },
        { "PINDLL", 0x8 },
        { "NEEDSFILESYSANCESTOR", 0x10 },
        { "NOTAFILESYSTEM", 0x20 },
        { "CTXMENU_NOVERBS", 0x40 },
        { "CTXMENU_LIMITEDQI", 0x80 },
        { "COCREATESHELLFOLDERONLY", 0x100 },
        { "NEEDSSTORAGEANCESTOR", 0x200 },
        { "NOLEGACYWEBVIEW", 0x400 },
        { "CTXMENU_XPQCMFLAGS", 0x1000 },
        { "NOIPROPERTYSTORE", 0x2000 }
    };

    static const char compat_path[] = "Software\\Microsoft\\Windows\\CurrentVersion\\ShellCompatibility\\Objects";
    CHAR keyA[39]; /* {CLSID} */
    HKEY root;
    DWORD ret;
    int i;

    /* null args */
    ret = pSHGetObjectCompatFlags(NULL, NULL);
    ok(ret == 0, "got %ld\n", ret);

    ret = RegOpenKeyA(HKEY_LOCAL_MACHINE, compat_path, &root);
    if (ret != ERROR_SUCCESS)
    {
        skip("No compatibility class data found\n");
        return;
    }

    for (i = 0; RegEnumKeyA(root, i, keyA, sizeof(keyA)) == ERROR_SUCCESS; i++)
    {
        HKEY clsid_key;

        if (RegOpenKeyA(root, keyA, &clsid_key) == ERROR_SUCCESS)
        {
            CHAR valueA[30];
            DWORD expected = 0, got, length = sizeof(valueA);
            CLSID clsid;
            int v;

            for (v = 0; RegEnumValueA(clsid_key, v, valueA, &length, NULL, NULL, NULL, NULL) == ERROR_SUCCESS; v++)
            {
                int j;

                for (j = 0; j < ARRAY_SIZE(values); j++)
                    if (lstrcmpA(values[j].nameA, valueA) == 0)
                    {
                        expected |= values[j].value;
                        break;
                    }

                length = sizeof(valueA);
            }

            pGUIDFromStringA(keyA, &clsid);
            got = pSHGetObjectCompatFlags(NULL, &clsid);
            ok(got == expected, "got 0x%08lx, expected 0x%08lx. Key %s\n", got, expected, keyA);

            RegCloseKey(clsid_key);
        }
    }

    RegCloseKey(root);
}

typedef struct {
    IOleCommandTarget IOleCommandTarget_iface;
    LONG ref;
} IOleCommandTargetImpl;

static inline IOleCommandTargetImpl *impl_from_IOleCommandTarget(IOleCommandTarget *iface)
{
    return CONTAINING_RECORD(iface, IOleCommandTargetImpl, IOleCommandTarget_iface);
}

static const IOleCommandTargetVtbl IOleCommandTargetImpl_Vtbl;

static IOleCommandTarget* IOleCommandTargetImpl_Construct(void)
{
    IOleCommandTargetImpl *obj;

    obj = HeapAlloc(GetProcessHeap(), 0, sizeof(*obj));
    obj->IOleCommandTarget_iface.lpVtbl = &IOleCommandTargetImpl_Vtbl;
    obj->ref = 1;

    return &obj->IOleCommandTarget_iface;
}

static HRESULT WINAPI IOleCommandTargetImpl_QueryInterface(IOleCommandTarget *iface, REFIID riid, void **ppvObj)
{
    IOleCommandTargetImpl *This = impl_from_IOleCommandTarget(iface);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IOleCommandTarget))
    {
        *ppvObj = This;
    }

    if(*ppvObj)
    {
        IOleCommandTarget_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI IOleCommandTargetImpl_AddRef(IOleCommandTarget *iface)
{
    IOleCommandTargetImpl *This = impl_from_IOleCommandTarget(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IOleCommandTargetImpl_Release(IOleCommandTarget *iface)
{
    IOleCommandTargetImpl *This = impl_from_IOleCommandTarget(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This);
        return 0;
    }
    return ref;
}

static HRESULT WINAPI IOleCommandTargetImpl_QueryStatus(
    IOleCommandTarget *iface, const GUID *group, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI IOleCommandTargetImpl_Exec(
    IOleCommandTarget *iface,
    const GUID *CmdGroup,
    DWORD nCmdID,
    DWORD nCmdexecopt,
    VARIANT *pvaIn,
    VARIANT *pvaOut)
{
    add_call(&trace_got, 3, CmdGroup, (void*)(DWORD_PTR)nCmdID, (void*)(DWORD_PTR)nCmdexecopt, pvaIn, pvaOut);
    return S_OK;
}

static const IOleCommandTargetVtbl IOleCommandTargetImpl_Vtbl =
{
    IOleCommandTargetImpl_QueryInterface,
    IOleCommandTargetImpl_AddRef,
    IOleCommandTargetImpl_Release,
    IOleCommandTargetImpl_QueryStatus,
    IOleCommandTargetImpl_Exec
};

typedef struct {
    IServiceProvider IServiceProvider_iface;
    LONG ref;
} IServiceProviderImpl;

static inline IServiceProviderImpl *impl_from_IServiceProvider(IServiceProvider *iface)
{
    return CONTAINING_RECORD(iface, IServiceProviderImpl, IServiceProvider_iface);
}

typedef struct {
    IProfferService IProfferService_iface;
    LONG ref;
} IProfferServiceImpl;

static inline IProfferServiceImpl *impl_from_IProfferService(IProfferService *iface)
{
    return CONTAINING_RECORD(iface, IProfferServiceImpl, IProfferService_iface);
}


static const IServiceProviderVtbl IServiceProviderImpl_Vtbl;
static const IProfferServiceVtbl IProfferServiceImpl_Vtbl;

static IServiceProvider* IServiceProviderImpl_Construct(void)
{
    IServiceProviderImpl *obj;

    obj = HeapAlloc(GetProcessHeap(), 0, sizeof(*obj));
    obj->IServiceProvider_iface.lpVtbl = &IServiceProviderImpl_Vtbl;
    obj->ref = 1;

    return &obj->IServiceProvider_iface;
}

static IProfferService* IProfferServiceImpl_Construct(void)
{
    IProfferServiceImpl *obj;

    obj = HeapAlloc(GetProcessHeap(), 0, sizeof(*obj));
    obj->IProfferService_iface.lpVtbl = &IProfferServiceImpl_Vtbl;
    obj->ref = 1;

    return &obj->IProfferService_iface;
}

static HRESULT WINAPI IServiceProviderImpl_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppvObj)
{
    IServiceProviderImpl *This = impl_from_IServiceProvider(iface);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IServiceProvider))
    {
        *ppvObj = This;
    }

    if(*ppvObj)
    {
        IServiceProvider_AddRef(iface);
        /* native uses redefined IID_IServiceProvider symbol, so we can't compare pointers */
        if (IsEqualIID(riid, &IID_IServiceProvider))
            add_call(&trace_got, 1, iface, &IID_IServiceProvider, 0, 0, 0);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI IServiceProviderImpl_AddRef(IServiceProvider *iface)
{
    IServiceProviderImpl *This = impl_from_IServiceProvider(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IServiceProviderImpl_Release(IServiceProvider *iface)
{
    IServiceProviderImpl *This = impl_from_IServiceProvider(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This);
        return 0;
    }
    return ref;
}

static HRESULT WINAPI IServiceProviderImpl_QueryService(
    IServiceProvider *iface, REFGUID service, REFIID riid, void **ppv)
{
    /* native uses redefined pointer for IID_IOleCommandTarget, not one from uuid.lib */
    if (IsEqualIID(riid, &IID_IOleCommandTarget))
    {
        add_call(&trace_got, 2, iface, service, &IID_IOleCommandTarget, 0, 0);
        *ppv = IOleCommandTargetImpl_Construct();
    }
    if (IsEqualIID(riid, &IID_IProfferService))
    {
        if (IsEqualIID(service, &IID_IProfferService))
            add_call(&trace_got, 2, &IID_IProfferService, &IID_IProfferService, 0, 0, 0);
        *ppv = IProfferServiceImpl_Construct();
    }
    return S_OK;
}

static const IServiceProviderVtbl IServiceProviderImpl_Vtbl =
{
    IServiceProviderImpl_QueryInterface,
    IServiceProviderImpl_AddRef,
    IServiceProviderImpl_Release,
    IServiceProviderImpl_QueryService
};

static void test_IUnknown_QueryServiceExec(void)
{
    IServiceProvider *provider;
    static const GUID dummy_serviceid = { 0xdeadbeef };
    static const GUID dummy_groupid = { 0xbeefbeef };
    call_trace_t trace_expected;
    HRESULT hr;

    provider = IServiceProviderImpl_Construct();

    /* null source pointer */
    hr = pIUnknown_QueryServiceExec(NULL, &dummy_serviceid, &dummy_groupid, 0, 0, 0, 0);
    ok(hr == E_FAIL ||
       hr == E_NOTIMPL, /* win 8 */
       "got 0x%08lx\n", hr);

    /* expected trace:
       IUnknown_QueryServiceExec( ptr1, serviceid, groupid, arg1, arg2, arg3, arg4);
         -> IUnknown_QueryInterface( ptr1, &IID_IServiceProvider, &prov );
         -> IServiceProvider_QueryService( prov, serviceid, &IID_IOleCommandTarget, &obj );
         -> IOleCommandTarget_Exec( obj, groupid, arg1, arg2, arg3, arg4 );
    */
    init_call_trace(&trace_expected);

    add_call(&trace_expected, 1, provider, &IID_IServiceProvider, 0, 0, 0);
    add_call(&trace_expected, 2, provider, &dummy_serviceid, &IID_IOleCommandTarget, 0, 0);
    add_call(&trace_expected, 3, &dummy_groupid, (void*)0x1, (void*)0x2, (void*)0x3, (void*)0x4);

    init_call_trace(&trace_got);
    hr = pIUnknown_QueryServiceExec((IUnknown*)provider, &dummy_serviceid, &dummy_groupid, 0x1, 0x2, (void*)0x3, (void*)0x4);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    ok_trace(&trace_expected, &trace_got);

    free_call_trace(&trace_expected);
    free_call_trace(&trace_got);

    IServiceProvider_Release(provider);
}


static HRESULT WINAPI IProfferServiceImpl_QueryInterface(IProfferService *iface, REFIID riid, void **ppvObj)
{
    IProfferServiceImpl *This = impl_from_IProfferService(iface);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IProfferService))
    {
        *ppvObj = This;
    }
    else if (IsEqualIID(riid, &IID_IServiceProvider))
    {
        *ppvObj = IServiceProviderImpl_Construct();
        add_call(&trace_got, 1, iface, &IID_IServiceProvider, 0, 0, 0);
        return S_OK;
    }

    if(*ppvObj)
    {
        IProfferService_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI IProfferServiceImpl_AddRef(IProfferService *iface)
{
    IProfferServiceImpl *This = impl_from_IProfferService(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IProfferServiceImpl_Release(IProfferService *iface)
{
    IProfferServiceImpl *This = impl_from_IProfferService(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This);
        return 0;
    }
    return ref;
}

static HRESULT WINAPI IProfferServiceImpl_ProfferService(IProfferService *iface,
    REFGUID service, IServiceProvider *pService, DWORD *pCookie)
{
    *pCookie = 0xdeadbeef;
    add_call(&trace_got, 3, service, pService, pCookie, 0, 0);
    return S_OK;
}

static HRESULT WINAPI IProfferServiceImpl_RevokeService(IProfferService *iface, DWORD cookie)
{
    add_call(&trace_got, 4, (void*)(DWORD_PTR)cookie, 0, 0, 0, 0);
    return S_OK;
}

static const IProfferServiceVtbl IProfferServiceImpl_Vtbl =
{
    IProfferServiceImpl_QueryInterface,
    IProfferServiceImpl_AddRef,
    IProfferServiceImpl_Release,
    IProfferServiceImpl_ProfferService,
    IProfferServiceImpl_RevokeService
};

static void test_IUnknown_ProfferService(void)
{
    IServiceProvider *provider;
    IProfferService *proff;
    static const GUID dummy_serviceid = { 0xdeadbeef };
    call_trace_t trace_expected;
    HRESULT hr;
    DWORD cookie;

    provider = IServiceProviderImpl_Construct();
    proff = IProfferServiceImpl_Construct();

    /* null source pointer */
    hr = pIUnknown_ProfferService(NULL, &dummy_serviceid, 0, 0);
    ok(hr == E_FAIL ||
       hr == E_NOTIMPL, /* win 8 */
       "got 0x%08lx\n", hr);

    /* expected trace:
       IUnknown_ProfferService( ptr1, serviceid, arg1, arg2);
         -> IUnknown_QueryInterface( ptr1, &IID_IServiceProvider, &provider );
         -> IServiceProvider_QueryService( provider, &IID_IProfferService, &IID_IProfferService, &proffer );

         if (service pointer not null):
             -> IProfferService_ProfferService( proffer, serviceid, arg1, arg2 );
         else
             -> IProfferService_RevokeService( proffer, *arg2 );
    */
    init_call_trace(&trace_expected);

    add_call(&trace_expected, 1, proff, &IID_IServiceProvider, 0, 0, 0);
    add_call(&trace_expected, 2, &IID_IProfferService, &IID_IProfferService, 0, 0, 0);
    add_call(&trace_expected, 3, &dummy_serviceid, provider, &cookie, 0, 0);

    init_call_trace(&trace_got);
    cookie = 0;
    hr = pIUnknown_ProfferService((IUnknown*)proff, &dummy_serviceid, provider, &cookie);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(cookie == 0xdeadbeef, "got %lx\n", cookie);

    ok_trace(&trace_expected, &trace_got);
    free_call_trace(&trace_got);
    free_call_trace(&trace_expected);

    /* same with ::Revoke path */
    init_call_trace(&trace_expected);

    add_call(&trace_expected, 1, proff, &IID_IServiceProvider, 0, 0, 0);
    add_call(&trace_expected, 2, &IID_IProfferService, &IID_IProfferService, 0, 0, 0);
    add_call(&trace_expected, 4, (void*)(DWORD_PTR)cookie, 0, 0, 0, 0);

    init_call_trace(&trace_got);
    ok(cookie != 0, "got %lx\n", cookie);
    hr = pIUnknown_ProfferService((IUnknown*)proff, &dummy_serviceid, 0, &cookie);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(cookie == 0, "got %lx\n", cookie);
    ok_trace(&trace_expected, &trace_got);
    free_call_trace(&trace_got);
    free_call_trace(&trace_expected);

    IServiceProvider_Release(provider);
    IProfferService_Release(proff);
}

static void test_SHCreateWorkerWindowA(void)
{
    WNDCLASSA cliA;
    char classA[20];
    HWND hwnd;
    LONG_PTR ret;
    BOOL res;

    hwnd = pSHCreateWorkerWindowA(0, NULL, 0, 0, 0, 0);
    ok(hwnd != 0, "expected window\n");

    GetClassNameA(hwnd, classA, 20);
    ok(lstrcmpA(classA, "WorkerA") == 0, "expected WorkerA class, got %s\n", classA);

    ret = GetWindowLongPtrA(hwnd, 0);
    ok(ret == 0, "got %Id\n", ret);

    /* class info */
    memset(&cliA, 0, sizeof(cliA));
    res = GetClassInfoA(GetModuleHandleA("shlwapi.dll"), "WorkerA", &cliA);
    ok(res, "failed to get class info\n");
    ok(cliA.style == 0, "got 0x%08x\n", cliA.style);
    ok(cliA.cbClsExtra == 0, "got %d\n", cliA.cbClsExtra);
    ok(cliA.cbWndExtra == sizeof(LONG_PTR), "got %d\n", cliA.cbWndExtra);
    ok(cliA.lpszMenuName == 0, "got %s\n", cliA.lpszMenuName);

    DestroyWindow(hwnd);

    /* set extra bytes */
    hwnd = pSHCreateWorkerWindowA(0, NULL, 0, 0, 0, 0xdeadbeef);
    ok(hwnd != 0, "expected window\n");

    GetClassNameA(hwnd, classA, 20);
    ok(lstrcmpA(classA, "WorkerA") == 0, "expected WorkerA class, got %s\n", classA);

    ret = GetWindowLongPtrA(hwnd, 0);
    ok(ret == 0xdeadbeef, "got %Id\n", ret);

    /* test exstyle */
    ret = GetWindowLongA(hwnd, GWL_EXSTYLE);
    ok(ret == WS_EX_WINDOWEDGE ||
       ret == (WS_EX_WINDOWEDGE|WS_EX_LAYOUTRTL) /* systems with RTL locale */, "0x%08Ix\n", ret);

    DestroyWindow(hwnd);

    hwnd = pSHCreateWorkerWindowA(0, NULL, WS_EX_TOOLWINDOW, 0, 0, 0);
    ret = GetWindowLongA(hwnd, GWL_EXSTYLE);
    ok(ret == (WS_EX_WINDOWEDGE|WS_EX_TOOLWINDOW) ||
       ret == (WS_EX_WINDOWEDGE|WS_EX_TOOLWINDOW|WS_EX_LAYOUTRTL) /* systems with RTL locale */, "0x%08Ix\n", ret);
    DestroyWindow(hwnd);
}

static HRESULT WINAPI SF_QueryInterface(IShellFolder *iface,
        REFIID riid, void **ppv)
{
    /* SHIShellFolder_EnumObjects doesn't QI the object for IShellFolder */
    ok(!IsEqualGUID(&IID_IShellFolder, riid),
            "Unexpected QI for IShellFolder\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI SF_AddRef(IShellFolder *iface)
{
    return 2;
}

static ULONG WINAPI SF_Release(IShellFolder *iface)
{
    return 1;
}

static HRESULT WINAPI SF_ParseDisplayName(IShellFolder *iface,
        HWND owner, LPBC reserved, LPOLESTR displayName, ULONG *eaten,
        LPITEMIDLIST *idl, ULONG *attr)
{
    ok(0, "Didn't expect ParseDisplayName\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI SF_EnumObjects(IShellFolder *iface,
        HWND owner, SHCONTF flags, IEnumIDList **enm)
{
    *enm = (IEnumIDList*)0xcafebabe;
    return S_OK;
}

static HRESULT WINAPI SF_BindToObject(IShellFolder *iface,
        LPCITEMIDLIST idl, LPBC reserved, REFIID riid, void **obj)
{
    ok(0, "Didn't expect BindToObject\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI SF_BindToStorage(IShellFolder *iface,
        LPCITEMIDLIST idl, LPBC reserved, REFIID riid, void **obj)
{
    ok(0, "Didn't expect BindToStorage\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI SF_CompareIDs(IShellFolder *iface,
        LPARAM lparam, LPCITEMIDLIST idl1, LPCITEMIDLIST idl2)
{
    ok(0, "Didn't expect CompareIDs\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI SF_CreateViewObject(IShellFolder *iface,
        HWND owner, REFIID riid, void **out)
{
    ok(0, "Didn't expect CreateViewObject\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI SF_GetAttributesOf(IShellFolder *iface,
        UINT cidl, LPCITEMIDLIST *idl, SFGAOF *inOut)
{
    ok(0, "Didn't expect GetAttributesOf\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI SF_GetUIObjectOf(IShellFolder *iface,
        HWND owner, UINT cidl, LPCITEMIDLIST *idls, REFIID riid, UINT *inOut,
        void **out)
{
    ok(0, "Didn't expect GetUIObjectOf\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI SF_GetDisplayNameOf(IShellFolder *iface,
        LPCITEMIDLIST idl, SHGDNF flags, STRRET *name)
{
    ok(0, "Didn't expect GetDisplayNameOf\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI SF_SetNameOf(IShellFolder *iface,
        HWND hwnd, LPCITEMIDLIST idl, LPCOLESTR name, SHGDNF flags,
        LPITEMIDLIST *idlOut)
{
    ok(0, "Didn't expect SetNameOf\n");
    return E_NOTIMPL;
}

static IShellFolderVtbl ShellFolderVtbl = {
    SF_QueryInterface,
    SF_AddRef,
    SF_Release,
    SF_ParseDisplayName,
    SF_EnumObjects,
    SF_BindToObject,
    SF_BindToStorage,
    SF_CompareIDs,
    SF_CreateViewObject,
    SF_GetAttributesOf,
    SF_GetUIObjectOf,
    SF_GetDisplayNameOf,
    SF_SetNameOf
};

static IShellFolder ShellFolder = { &ShellFolderVtbl };

static void test_SHIShellFolder_EnumObjects(void)
{
    IEnumIDList *enm;
    HRESULT hres;
    IShellFolder *folder;

    if(!pSHIShellFolder_EnumObjects){ /* win7 and later */
        win_skip("SHIShellFolder_EnumObjects not available\n");
        return;
    }

    if(0){
        /* NULL object crashes on Windows */
        pSHIShellFolder_EnumObjects(NULL, NULL, 0, NULL);
    }

    /* SHIShellFolder_EnumObjects doesn't QI the object for IShellFolder */
    enm = (IEnumIDList*)0xdeadbeef;
    hres = pSHIShellFolder_EnumObjects(&ShellFolder, NULL, 0, &enm);
    ok(hres == S_OK, "SHIShellFolder_EnumObjects failed: 0x%08lx\n", hres);
    ok(enm == (IEnumIDList*)0xcafebabe, "Didn't get expected enumerator location, instead: %p\n", enm);

    /* SHIShellFolder_EnumObjects isn't strict about the IShellFolder object */
    hres = SHGetDesktopFolder(&folder);
    ok(hres == S_OK, "SHGetDesktopFolder failed: 0x%08lx\n", hres);

    enm = NULL;
    hres = pSHIShellFolder_EnumObjects(folder, NULL, 0, &enm);
    ok(hres == S_OK, "SHIShellFolder_EnumObjects failed: 0x%08lx\n", hres);
    ok(enm != NULL, "Didn't get an enumerator\n");
    if(enm)
        IEnumIDList_Release(enm);

    IShellFolder_Release(folder);
}

static BOOL write_inifile(LPCWSTR filename)
{
    DWORD written;
    HANDLE file;

    static const char data[] =
        "[TestApp]\r\n"
        "AKey=1\r\n"
        "AnotherKey=asdf\r\n";

    file = CreateFileW(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if(file == INVALID_HANDLE_VALUE) {
        win_skip("failed to create ini file at %s\n", wine_dbgstr_w(filename));
        return FALSE;
    }

    WriteFile(file, data, sizeof(data), &written, NULL);

    CloseHandle(file);

    return TRUE;
}

#define verify_inifile(f, e) r_verify_inifile(__LINE__, f, e)
static void r_verify_inifile(unsigned l, LPCWSTR filename, LPCSTR exp)
{
    HANDLE file;
    CHAR buf[1024];
    DWORD read;

    file = CreateFileW(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

    if(file == INVALID_HANDLE_VALUE)
        return;

    ReadFile(file, buf, sizeof(buf) * sizeof(CHAR), &read, NULL);
    buf[read] = '\0';

    CloseHandle(file);

    ok_(__FILE__,l)(!strcmp(buf, exp), "Expected:\n%s\nGot:\n%s\n", exp,
            buf);
}

static void test_SHGetIniString(void)
{
    DWORD ret;
    WCHAR out[64] = {0};

    static const WCHAR TestAppW[] = {'T','e','s','t','A','p','p',0};
    static const WCHAR AKeyW[] = {'A','K','e','y',0};
    static const WCHAR AnotherKeyW[] = {'A','n','o','t','h','e','r','K','e','y',0};
    static const WCHAR JunkKeyW[] = {'J','u','n','k','K','e','y',0};
    static const WCHAR testpathW[] = {'C',':','\\','t','e','s','t','.','i','n','i',0};
    WCHAR pathW[MAX_PATH];

    lstrcpyW(pathW, testpathW);

    if (!write_inifile(pathW))
        return;

    if(0){
        /* these crash on Windows */
        pSHGetIniStringW(NULL, NULL, NULL, 0, NULL);
        pSHGetIniStringW(NULL, AKeyW, out, ARRAY_SIZE(out), pathW);
        pSHGetIniStringW(TestAppW, AKeyW, NULL, ARRAY_SIZE(out), pathW);
    }

    ret = pSHGetIniStringW(TestAppW, AKeyW, out, 0, pathW);
    ok(ret == 0, "SHGetIniStringW should have given 0, instead: %ld\n", ret);

    /* valid arguments */
    out[0] = 0;
    SetLastError(0xdeadbeef);
    ret = pSHGetIniStringW(TestAppW, NULL, out, ARRAY_SIZE(out), pathW);
    ok(ret == 4, "SHGetIniStringW should have given 4, instead: %ld\n", ret);
    ok(!lstrcmpW(out, AKeyW), "Expected %s, got: %s, %ld\n",
                wine_dbgstr_w(AKeyW), wine_dbgstr_w(out), GetLastError());

    ret = pSHGetIniStringW(TestAppW, AKeyW, out, ARRAY_SIZE(out), pathW);
    ok(ret == 1, "SHGetIniStringW should have given 1, instead: %ld\n", ret);
    ok(!lstrcmpW(out, L"1"), "Expected L\"1\", got: %s\n", wine_dbgstr_w(out));

    ret = pSHGetIniStringW(TestAppW, AnotherKeyW, out, ARRAY_SIZE(out), pathW);
    ok(ret == 4, "SHGetIniStringW should have given 4, instead: %ld\n", ret);
    ok(!lstrcmpW(out, L"asdf"), "Expected L\"asdf\", got: %s\n", wine_dbgstr_w(out));

    out[0] = 1;
    ret = pSHGetIniStringW(TestAppW, JunkKeyW, out, ARRAY_SIZE(out), pathW);
    ok(ret == 0, "SHGetIniStringW should have given 0, instead: %ld\n", ret);
    ok(*out == 0, "Expected L\"\", got: %s\n", wine_dbgstr_w(out));

    DeleteFileW(pathW);
}

static void test_SHSetIniString(void)
{
    BOOL ret;

    static const WCHAR TestAppW[] = {'T','e','s','t','A','p','p',0};
    static const WCHAR AnotherAppW[] = {'A','n','o','t','h','e','r','A','p','p',0};
    static const WCHAR TestIniW[] = {'C',':','\\','t','e','s','t','.','i','n','i',0};
    static const WCHAR AKeyW[] = {'A','K','e','y',0};
    static const WCHAR NewKeyW[] = {'N','e','w','K','e','y',0};
    static const WCHAR AValueW[] = {'A','V','a','l','u','e',0};

    if (!write_inifile(TestIniW))
        return;

    ret = pSHSetIniStringW(TestAppW, AKeyW, AValueW, TestIniW);
    ok(ret == TRUE, "SHSetIniStringW should not have failed\n");
    todo_wine /* wine sticks an extra \r\n at the end of the file */
        verify_inifile(TestIniW, "[TestApp]\r\nAKey=AValue\r\nAnotherKey=asdf\r\n");

    ret = pSHSetIniStringW(TestAppW, AKeyW, NULL, TestIniW);
    ok(ret == TRUE, "SHSetIniStringW should not have failed\n");
    verify_inifile(TestIniW, "[TestApp]\r\nAnotherKey=asdf\r\n");

    ret = pSHSetIniStringW(AnotherAppW, NewKeyW, AValueW, TestIniW);
    ok(ret == TRUE, "SHSetIniStringW should not have failed\n");
    verify_inifile(TestIniW, "[TestApp]\r\nAnotherKey=asdf\r\n[AnotherApp]\r\nNewKey=AValue\r\n");

    ret = pSHSetIniStringW(TestAppW, NULL, AValueW, TestIniW);
    ok(ret == TRUE, "SHSetIniStringW should not have failed\n");
    verify_inifile(TestIniW, "[AnotherApp]\r\nNewKey=AValue\r\n");

    DeleteFileW(TestIniW);
}

enum _shellkey_flags {
    SHKEY_Root_HKCU = 0x1,
    SHKEY_Root_HKLM = 0x2,
    SHKEY_Key_Explorer  = 0x00,
    SHKEY_Key_Shell = 0x10,
    SHKEY_Key_ShellNoRoam = 0x20,
    SHKEY_Key_Classes = 0x30,
    SHKEY_Subkey_Default = 0x0000,
    SHKEY_Subkey_ResourceName = 0x1000,
    SHKEY_Subkey_Handlers = 0x2000,
    SHKEY_Subkey_Associations = 0x3000,
    SHKEY_Subkey_Volatile = 0x4000,
    SHKEY_Subkey_MUICache = 0x5000,
    SHKEY_Subkey_FileExts = 0x6000
};

static void test_SHGetShellKey(void)
{
    static const WCHAR ShellFoldersW[] = { 'S','h','e','l','l',' ','F','o','l','d','e','r','s',0 };
    static const WCHAR WineTestW[] = { 'W','i','n','e','T','e','s','t',0 };

    DWORD *alloc_data, data, size;
    HKEY hkey;
    HRESULT hres;

    /* Vista+ limits SHKEY enumeration values */
    SetLastError(0xdeadbeef);
    hkey = pSHGetShellKey(SHKEY_Key_Explorer, ShellFoldersW, FALSE);
    if (hkey)
    {
        /* Tests not working on Vista+ */
        RegCloseKey(hkey);

        hkey = pSHGetShellKey(SHKEY_Root_HKLM|SHKEY_Key_Classes, NULL, FALSE);
        ok(hkey != NULL, "hkey = NULL\n");
        RegCloseKey(hkey);
    }

    hkey = pSHGetShellKey(SHKEY_Root_HKCU|SHKEY_Key_Explorer, ShellFoldersW, FALSE);
    ok(hkey != NULL, "hkey = NULL\n");
    RegCloseKey(hkey);

    hkey = pSHGetShellKey(SHKEY_Root_HKLM|SHKEY_Key_Explorer, ShellFoldersW, FALSE);
    ok(hkey != NULL, "hkey = NULL\n");
    RegCloseKey(hkey);

    hkey = pSHGetShellKey(SHKEY_Root_HKLM, WineTestW, FALSE);
    ok(hkey == NULL, "hkey != NULL\n");

    hkey = pSHGetShellKey(SHKEY_Root_HKLM, NULL, FALSE);
    ok(hkey != NULL, "Can't open key\n");
    ok(SUCCEEDED(RegDeleteKeyW(hkey, WineTestW)), "Can't delete key\n");
    RegCloseKey(hkey);

    hkey = pSHGetShellKey(SHKEY_Root_HKLM, WineTestW, TRUE);
    if (!hkey && GetLastError() == ERROR_ACCESS_DENIED)
    {
        skip("Not authorized to create keys\n");
        return;
    }
    ok(hkey != NULL, "Can't create key\n");
    RegCloseKey(hkey);

    size = sizeof(data);
    hres = pSKGetValueW(SHKEY_Root_HKLM, WineTestW, NULL, NULL, &data, &size);
    ok(hres == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "hres = %lx\n", hres);

    data = 1234;
    hres = pSKSetValueW(SHKEY_Root_HKLM, WineTestW, NULL, REG_DWORD, &data, sizeof(DWORD));
    ok(hres == S_OK, "hres = %lx\n", hres);

    size = 1;
    hres = pSKGetValueW(SHKEY_Root_HKLM, WineTestW, NULL, NULL, NULL, &size);
    ok(hres == S_OK, "hres = %lx\n", hres);
    ok(size == sizeof(DWORD), "size = %ld\n", size);

    data = 0xdeadbeef;
    hres = pSKGetValueW(SHKEY_Root_HKLM, WineTestW, NULL, NULL, &data, &size);
    ok(hres == S_OK, "hres = %lx\n", hres);
    ok(size == sizeof(DWORD), "size = %ld\n", size);
    ok(data == 1234, "data = %ld\n", data);

    hres = pSKAllocValueW(SHKEY_Root_HKLM, WineTestW, NULL, NULL, (void**)&alloc_data, &size);
    ok(hres == S_OK, "hres= %lx\n", hres);
    ok(size == sizeof(DWORD), "size = %ld\n", size);
    if (SUCCEEDED(hres))
    {
        ok(*alloc_data == 1234, "*alloc_data = %ld\n", *alloc_data);
        LocalFree(alloc_data);
    }

    hres = pSKDeleteValueW(SHKEY_Root_HKLM, WineTestW, NULL);
    ok(hres == S_OK, "hres = %lx\n", hres);

    hres = pSKDeleteValueW(SHKEY_Root_HKLM, WineTestW, NULL);
    ok(hres == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "hres = %lx\n", hres);

    hres = pSKGetValueW(SHKEY_Root_HKLM, WineTestW, NULL, NULL, &data, &size);
    ok(hres == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "hres = %lx\n", hres);

    hkey = pSHGetShellKey(SHKEY_Root_HKLM, NULL, FALSE);
    ok(hkey != NULL, "Can't create key\n");
    ok(SUCCEEDED(RegDeleteKeyW(hkey, WineTestW)), "Can't delete key\n");
    RegCloseKey(hkey);
}

static void init_pointers(void)
{
#define MAKEFUNC(f, ord) (p##f = (void*)GetProcAddress(hShlwapi, (LPSTR)(ord)))
    MAKEFUNC(SHAllocShared, 7);
    MAKEFUNC(SHLockShared, 8);
    MAKEFUNC(SHUnlockShared, 9);
    MAKEFUNC(SHFreeShared, 10);
    MAKEFUNC(SHMapHandle, 11);
    MAKEFUNC(GetAcceptLanguagesA, 14);
    MAKEFUNC(SHSetWindowBits, 165);
    MAKEFUNC(SHSetParentHwnd, 167);
    MAKEFUNC(ConnectToConnectionPoint, 168);
    MAKEFUNC(IUnknown_GetClassID, 175);
    MAKEFUNC(SHSearchMapInt, 198);
    MAKEFUNC(SHCreateWorkerWindowA, 257);
    MAKEFUNC(GUIDFromStringA, 269);
    MAKEFUNC(SHPackDispParams, 282);
    MAKEFUNC(IConnectionPoint_InvokeWithCancel, 283);
    MAKEFUNC(IConnectionPoint_SimpleInvoke, 284);
    MAKEFUNC(SHGetIniStringW, 294);
    MAKEFUNC(SHSetIniStringW, 295);
    MAKEFUNC(SHFormatDateTimeA, 353);
    MAKEFUNC(SHFormatDateTimeW, 354);
    MAKEFUNC(SHIShellFolder_EnumObjects, 404);
    MAKEFUNC(GetShellSecurityDescriptor, 475);
    MAKEFUNC(SHGetObjectCompatFlags, 476);
    MAKEFUNC(IUnknown_QueryServiceExec, 484);
    MAKEFUNC(SHGetShellKey, 491);
    MAKEFUNC(SHPropertyBag_ReadLONG, 496);
    MAKEFUNC(IUnknown_ProfferService, 514);
    MAKEFUNC(SKGetValueW, 516);
    MAKEFUNC(SKSetValueW, 517);
    MAKEFUNC(SKDeleteValueW, 518);
    MAKEFUNC(SKAllocValueW, 519);
#undef MAKEFUNC

    pDllGetVersion = (void*)GetProcAddress(hShlwapi, "DllGetVersion");
}

static void test_SHSetParentHwnd(void)
{
    HWND hwnd, hwnd2, ret;
    DWORD style;

    hwnd = CreateWindowA("Button", "", WS_VISIBLE, 0, 0, 10, 10, NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "got %p\n", hwnd);

    hwnd2 = CreateWindowA("Button", "", WS_VISIBLE | WS_CHILD, 0, 0, 10, 10, hwnd, NULL, NULL, NULL);
    ok(hwnd2 != NULL, "got %p\n", hwnd2);

    /* null params */
    ret = pSHSetParentHwnd(NULL, NULL);
    ok(ret == NULL, "got %p\n", ret);

    /* set to no parent while already no parent present */
    ret = GetParent(hwnd);
    ok(ret == NULL, "got %p\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok((style & (WS_POPUP|WS_CHILD)) == 0, "got style 0x%08lx\n", style);
    ret = pSHSetParentHwnd(hwnd, NULL);
    ok(ret == NULL, "got %p\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok((style & (WS_POPUP|WS_CHILD)) == 0, "got style 0x%08lx\n", style);

    /* reset to null parent from not null */
    ret = GetParent(hwnd2);
    ok(ret == hwnd, "got %p\n", ret);
    style = GetWindowLongA(hwnd2, GWL_STYLE);
    ok((style & (WS_POPUP|WS_CHILD)) == WS_CHILD, "got style 0x%08lx\n", style);
    ret = pSHSetParentHwnd(hwnd2, NULL);
    ok(ret == NULL, "got %p\n", ret);
    style = GetWindowLongA(hwnd2, GWL_STYLE);
    ok((style & (WS_POPUP|WS_CHILD)) == WS_POPUP, "got style 0x%08lx\n", style);
    ret = GetParent(hwnd2);
    ok(ret == NULL, "got %p\n", ret);

    /* set parent back */
    style = GetWindowLongA(hwnd2, GWL_STYLE);
    SetWindowLongA(hwnd2, GWL_STYLE, style & ~(WS_CHILD|WS_POPUP));
    style = GetWindowLongA(hwnd2, GWL_STYLE);
    ok((style & (WS_CHILD|WS_POPUP)) == 0, "got 0x%08lx\n", style);

    ret = pSHSetParentHwnd(hwnd2, hwnd);
    todo_wine ok(ret == NULL, "got %p\n", ret);

    style = GetWindowLongA(hwnd2, GWL_STYLE);
    ok((style & (WS_POPUP|WS_CHILD)) == WS_CHILD, "got style 0x%08lx\n", style);
    ret = GetParent(hwnd2);
    ok(ret == hwnd, "got %p\n", ret);

    /* try to set same parent again */
    /* with WS_POPUP */
    style = GetWindowLongA(hwnd2, GWL_STYLE);
    SetWindowLongA(hwnd2, GWL_STYLE, style | WS_POPUP);
    ret = pSHSetParentHwnd(hwnd2, hwnd);
    todo_wine ok(ret == NULL, "got %p\n", ret);
    style = GetWindowLongA(hwnd2, GWL_STYLE);
    ok((style & (WS_CHILD|WS_POPUP)) == WS_CHILD, "got 0x%08lx\n", style);
    ret = GetParent(hwnd2);
    ok(ret == hwnd, "got %p\n", ret);

    /* without WS_POPUP */
    style = GetWindowLongA(hwnd2, GWL_STYLE);
    SetWindowLongA(hwnd2, GWL_STYLE, style | ~WS_POPUP);
    ret = pSHSetParentHwnd(hwnd2, hwnd);
    todo_wine ok(ret == hwnd, "got %p\n", ret);
    style = GetWindowLongA(hwnd2, GWL_STYLE);
    ok((style & (WS_CHILD|WS_POPUP)) == WS_CHILD, "got 0x%08lx\n", style);
    ret = GetParent(hwnd2);
    ok(ret == hwnd, "got %p\n", ret);

    DestroyWindow(hwnd);
    DestroyWindow(hwnd2);
}

static HRESULT WINAPI testpersist_QI(IPersist *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IPersist)) {
        *obj = iface;
        IPersist_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static HRESULT WINAPI testpersist_QI2(IPersist *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IPersistFolder)) {
        *obj = iface;
        IPersist_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI testpersist_AddRef(IPersist *iface)
{
    return 2;
}

static ULONG WINAPI testpersist_Release(IPersist *iface)
{
    return 1;
}

static HRESULT WINAPI testpersist_GetClassID(IPersist *iface, CLSID *clsid)
{
    memset(clsid, 0xab, sizeof(*clsid));
    return 0x8fff2222;
}

static IPersistVtbl testpersistvtbl = {
    testpersist_QI,
    testpersist_AddRef,
    testpersist_Release,
    testpersist_GetClassID
};

static IPersistVtbl testpersist2vtbl = {
    testpersist_QI2,
    testpersist_AddRef,
    testpersist_Release,
    testpersist_GetClassID
};

static IPersist testpersist = { &testpersistvtbl };
static IPersist testpersist2 = { &testpersist2vtbl };

static void test_IUnknown_GetClassID(void)
{
    CLSID clsid, clsid2, clsid3;
    HRESULT hr;

    if (0) /* crashes on native systems */
        hr = pIUnknown_GetClassID(NULL, NULL);

    memset(&clsid, 0xcc, sizeof(clsid));
    memset(&clsid3, 0xcc, sizeof(clsid3));
    hr = pIUnknown_GetClassID(NULL, &clsid);
    ok(hr == E_FAIL, "got 0x%08lx\n", hr);
    ok(IsEqualCLSID(&clsid, &CLSID_NULL) || broken(IsEqualCLSID(&clsid, &clsid3)) /* win2k, winxp, win2k3 */,
        "got wrong clsid %s\n", wine_dbgstr_guid(&clsid));

    memset(&clsid, 0xcc, sizeof(clsid));
    memset(&clsid2, 0xab, sizeof(clsid2));
    hr = pIUnknown_GetClassID((IUnknown*)&testpersist, &clsid);
    ok(hr == 0x8fff2222, "got 0x%08lx\n", hr);
    ok(IsEqualCLSID(&clsid, &clsid2) || broken(IsEqualCLSID(&clsid, &clsid3)) /* win2k3 */,
        "got wrong clsid %s\n", wine_dbgstr_guid(&clsid));

    /* IPersistFolder is also supported */
    memset(&clsid, 0xcc, sizeof(clsid));
    memset(&clsid2, 0xab, sizeof(clsid2));
    memset(&clsid3, 0xcc, sizeof(clsid3));
    hr = pIUnknown_GetClassID((IUnknown*)&testpersist2, &clsid);
    ok(hr == 0x8fff2222, "got 0x%08lx\n", hr);
    ok(IsEqualCLSID(&clsid, &clsid2) || broken(IsEqualCLSID(&clsid, &clsid3)) /* win2k3 */,
        "got wrong clsid %s\n", wine_dbgstr_guid(&clsid));
}

static void test_DllGetVersion(void)
{
    HRESULT hr;

    hr = pDllGetVersion(NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);
}

START_TEST(ordinal)
{
    SYSTEMTIME st;
    static const SYSTEMTIME february = {2023, 2, 2, 14, 12, 0, 0, 0};
    char **argv;
    int argc;

    hShlwapi = GetModuleHandleA("shlwapi.dll");

    init_pointers();

    argc = winetest_get_mainargs(&argv);
    if (argc >= 4)
    {
        DWORD procid;
        HANDLE hmem;
        sscanf(argv[2], "%ld", &procid);
        sscanf(argv[3], "%p", &hmem);
        test_alloc_shared_remote(procid, hmem);
        return;
    }

    test_GetAcceptLanguagesA();
    test_SHSearchMapInt();
    test_alloc_shared(argc, argv);
    test_fdsa();
    test_GetShellSecurityDescriptor();
    test_SHPackDispParams();
    test_IConnectionPoint();
    test_SHPropertyBag_ReadLONG();
    test_SHSetWindowBits();

    GetLocalTime(&st);
    test_SHFormatDateTimeA(&st);
    /* Test how the locale and code page interact for date formatting by
     * repeating the tests with a February date which in French contains an
     * e-acute that can only be represented in some code pages.
     */
    test_SHFormatDateTimeA(&february);

    test_SHFormatDateTimeW();
    test_SHGetObjectCompatFlags();
    test_IUnknown_QueryServiceExec();
    test_IUnknown_ProfferService();
    test_SHCreateWorkerWindowA();
    test_SHIShellFolder_EnumObjects();
    test_SHGetIniString();
    test_SHSetIniString();
    test_SHGetShellKey();
    test_SHSetParentHwnd();
    test_IUnknown_GetClassID();
    test_DllGetVersion();
}
