/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for dbghelp PDB functions
 * COPYRIGHT:   Copyright 2017-2019 Mark Jansen (mark.jansen@reactos.org)
 */

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <dbghelp.h>
#include <cvconst.h>    // SymTagXXX
#include <stdio.h>
#include <delayimp.h>

#include "wine/test.h"

extern PfnDliHook __pfnDliFailureHook2;

#define ok_ulonglong(expression, result) \
    do { \
        ULONG64 _value = (expression); \
        ULONG64 _result = (result); \
        ok(_value == (result), "Wrong value for '%s', expected: " #result " (%s), got: %s\n", \
           #expression, wine_dbgstr_longlong(_result), wine_dbgstr_longlong(_value)); \
    } while (0)


// data.c
void create_compressed_files();
int extract_msvc_dll(char szFile[MAX_PATH], char szPath[MAX_PATH]);
void cleanup_msvc_dll();

static HANDLE proc()
{
    return GetCurrentProcess();
}

static BOOL init_sym_imp(BOOL fInvadeProcess, const char* file, int line)
{
    if (!SymInitialize(proc(), NULL, fInvadeProcess))
    {
        DWORD err = GetLastError();
        ok_(file, line)(0, "Failed to init: 0x%x\n", err);
        return FALSE;
    }
    return TRUE;
}

static void deinit_sym()
{
    SymCleanup(proc());
}

#define init_sym(fInvadeProcess)          init_sym_imp(fInvadeProcess, __FILE__, __LINE__)

#define INIT_PSYM(buff) do { \
    memset((buff), 0, sizeof((buff))); \
    ((PSYMBOL_INFO)(buff))->SizeOfStruct = sizeof(SYMBOL_INFO); \
    ((PSYMBOL_INFO)(buff))->MaxNameLen = MAX_SYM_NAME; \
} while (0)

/* modified copy of function from apitests/apphelp/apitest.c */
BOOL get_module_version(
    _In_ HMODULE mod,
    _Out_ VS_FIXEDFILEINFO *fileinfo)
{
    BOOL res = FALSE;
    HRSRC hResInfo;
    char *errmsg;
    DWORD dwSize, errcode = 0;
    UINT uLen;
    HGLOBAL hResData = 0;
    LPVOID pRes = NULL;
    HLOCAL pResCopy = 0;
    VS_FIXEDFILEINFO *lpFfi;

    if (fileinfo == NULL)
    {
        errmsg = "fileinfo is NULL.\n";
        goto cleanup;
    }

    hResInfo = FindResource(mod, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
    if (hResInfo == 0)
    {
        errmsg = "FindResource failed";
        errcode = GetLastError();
        goto cleanup;
    }

    dwSize = SizeofResource(mod, hResInfo);
    if (dwSize == 0)
    {
        errmsg = "SizeofResource failed";
        errcode = GetLastError();
        goto cleanup;
    }

    hResData = LoadResource(mod, hResInfo);
    if (hResData == 0)
    {
        errmsg = "LoadResource failed";
        errcode = GetLastError();
        goto cleanup;
    }

    pRes = LockResource(hResData);
    if (pRes == NULL)
    {
        errmsg = "LockResource failed";
        errcode = GetLastError();
        goto cleanup;
    }

    pResCopy = LocalAlloc(LMEM_FIXED, dwSize);
    if (pResCopy == NULL)
    {
        errmsg = "LocalAlloc failed";
        errcode = GetLastError();
        goto cleanup;
    }

    CopyMemory(pResCopy, pRes, dwSize);

    if (VerQueryValueW(pResCopy, L"\\", (LPVOID*)&lpFfi, &uLen))
    {
        *fileinfo = *lpFfi;
        res = TRUE;
    }

cleanup:
    /* cleanup */
    if (hResData != 0)
        FreeResource(hResData);
    if (pResCopy != NULL)
        LocalFree(pResCopy);
    /* if it was good */
    if (res == TRUE)
        return TRUE;
    /* failure path */
    if (errcode == 0)
        trace("get_module_version - %s.\n", errmsg);
    else
        trace("get_module_version - %s (lasterror %d).\n", errmsg, errcode);
    return FALSE;
}

static VS_FIXEDFILEINFO dbghelpFileVer;
static void init_dbghelp_version()
{
    LPAPI_VERSION v;
    WCHAR filenameW[MAX_PATH + 1];
    HMODULE hDLL;
    DWORD fileLen;
    VS_FIXEDFILEINFO fileInfo;

    memset(&dbghelpFileVer, 0, sizeof(dbghelpFileVer));

    /* get internal file version */
    v = ImagehlpApiVersion();
    if (v == NULL)
        return;

    /* get module file version */
    hDLL = GetModuleHandleW(L"dbghelp.dll");
    if (hDLL == 0)
    {
        ok(FALSE, "Dbghelp.dll is not loaded!\n");
        return;
    }
    if (!get_module_version(hDLL, &fileInfo))
        memset(&fileInfo, 0, sizeof(fileInfo));
    dbghelpFileVer = fileInfo;

    /* get full file path */
    fileLen = GetModuleFileNameW(hDLL, filenameW, MAX_PATH + 1);
    if (fileLen == 0)
    {
        ok(FALSE, "GetModuleFileNameW for dbghelp.dll failed!\n");
        return;
    }

    trace("Using %S\n", filenameW);
    trace("  API-Version: %hu.%hu.%hu (%hu)\n",
          v->MajorVersion, v->MinorVersion, v->Revision, v->Reserved);

    trace("  Fileversion: %hu.%hu.%hu.%hu\n",
          HIWORD(fileInfo.dwProductVersionMS),
          LOWORD(fileInfo.dwProductVersionMS),
          HIWORD(fileInfo.dwProductVersionLS),
          LOWORD(fileInfo.dwProductVersionLS));
}

static
int g_SymRegisterCallbackW64NotFound = 0;

static
BOOL WINAPI SymRegisterCallbackW64_Stub(HANDLE hProcess, PSYMBOL_REGISTERED_CALLBACK64 CallbackFunction, ULONG64 UserContext)
{
    g_SymRegisterCallbackW64NotFound++;
    return FALSE;
}

/* A delay-load failure hook will be called when resolving a delay-load dependency (dll or function) fails */
FARPROC WINAPI DliFailHook(unsigned dliNotify, PDelayLoadInfo pdli)
{
    /* Was the failure a function, and did we get info */
    if (dliNotify == dliFailGetProc && pdli)
    {
        /* Is it our function? */
        if (pdli->dlp.fImportByName && !strcmp(pdli->dlp.szProcName, "SymRegisterCallbackW64"))
        {
            /* Redirect execution to the stub */
            return (FARPROC)SymRegisterCallbackW64_Stub;
        }
    }
    /* This is not the function you are looking for, continue default behavior (throw exception) */
    return NULL;
}

/* Maybe our dbghelp.dll is too old? */
static BOOL supports_pdb(HANDLE hProc, DWORD64 BaseAddress)
{
    IMAGEHLP_MODULE64 ModuleInfo;
    BOOL Ret;

    memset(&ModuleInfo, 0, sizeof(ModuleInfo));
    ModuleInfo.SizeOfStruct = sizeof(ModuleInfo);
    Ret = SymGetModuleInfo64(hProc, BaseAddress, &ModuleInfo);

    return Ret && ModuleInfo.SymType == SymPdb;
}


static void test_SymFromName(HANDLE hProc, DWORD64 BaseAddress)
{
    BOOL Ret;
    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
    PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;

    if (!supports_pdb(hProc, BaseAddress))
    {
        skip("dbghelp.dll too old or cannot enumerate symbols!\n");
    }
    else
    {
        INIT_PSYM(buffer);
        Ret = SymFromName(hProc, "DllMain", pSymbol);
        ok_int(Ret, TRUE);
        ok_ulonglong(pSymbol->ModBase, BaseAddress);
        ok_hex(pSymbol->Flags, 0);
        ok_ulonglong(pSymbol->Address, BaseAddress + 0x1010);
        ok_hex(pSymbol->Tag, SymTagFunction);
        ok_str(pSymbol->Name, "DllMain");

        INIT_PSYM(buffer);
        Ret = SymFromName(hProc, "_DllMain@12", pSymbol);
        ok_int(Ret, TRUE);
        ok_ulonglong(pSymbol->ModBase, BaseAddress);
        ok_hex(pSymbol->Flags, 0x400000);   // ??
        ok_ulonglong(pSymbol->Address, BaseAddress + 0x1010);
        ok_hex(pSymbol->Tag, SymTagPublicSymbol);
        ok_str(pSymbol->Name, "_DllMain@12");

        INIT_PSYM(buffer);
        Ret = SymFromName(hProc, "FfsChkdsk", pSymbol);
        ok_int(Ret, TRUE);
        ok_ulonglong(pSymbol->ModBase, BaseAddress);
        ok_hex(pSymbol->Flags, 0);
        ok_ulonglong(pSymbol->Address, BaseAddress + 0x1040);
        ok_hex(pSymbol->Tag, SymTagFunction);
        ok_str(pSymbol->Name, "FfsChkdsk");

        INIT_PSYM(buffer);
        Ret = SymFromName(hProc, "_FfsChkdsk@24", pSymbol);
        ok_int(Ret, TRUE);
        ok_ulonglong(pSymbol->ModBase, BaseAddress);
        ok_hex(pSymbol->Flags, 0x400000);   // ??
        ok_ulonglong(pSymbol->Address, BaseAddress + 0x1040);
        ok_hex(pSymbol->Tag, SymTagPublicSymbol);
        ok_str(pSymbol->Name, "_FfsChkdsk@24");

        INIT_PSYM(buffer);
        Ret = SymFromName(hProc, "FfsFormat", pSymbol);
        ok_int(Ret, TRUE);
        ok_ulonglong(pSymbol->ModBase, BaseAddress);
        ok_hex(pSymbol->Flags, 0);
        ok_ulonglong(pSymbol->Address, BaseAddress + 0x1070);
        ok_hex(pSymbol->Tag, SymTagFunction);
        ok_str(pSymbol->Name, "FfsFormat");

        INIT_PSYM(buffer);
        Ret = SymFromName(hProc, "_FfsFormat@24", pSymbol);
        ok_int(Ret, TRUE);
        ok_ulonglong(pSymbol->ModBase, BaseAddress);
        ok_hex(pSymbol->Flags, 0x400000);   // ??
        ok_ulonglong(pSymbol->Address, BaseAddress + 0x1070);
        ok_hex(pSymbol->Tag, SymTagPublicSymbol);
        ok_str(pSymbol->Name, "_FfsFormat@24");
    }
}

static void test_SymFromAddr(HANDLE hProc, DWORD64 BaseAddress)
{
    BOOL Ret;
    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
    PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;

    DWORD64 Displacement;
    DWORD dwErr;

    /* No address found before load address of module */
    Displacement = 0;
    INIT_PSYM(buffer);
    Ret = SymFromAddr(hProc, BaseAddress -1, &Displacement, pSymbol);
    dwErr = GetLastError();
    ok_int(Ret, FALSE);
    ok_hex(dwErr, ERROR_MOD_NOT_FOUND);

    /* Right at the start of the module is recognized as the first symbol found */
    Displacement = 0;
    INIT_PSYM(buffer);
    Ret = SymFromAddr(hProc, BaseAddress, &Displacement, pSymbol);
    ok_int(Ret, TRUE);
    ok_ulonglong(Displacement, 0xffffffffffffffff);
    ok_ulonglong(pSymbol->ModBase, BaseAddress);
    ok_hex(pSymbol->Flags, 0);
    ok_ulonglong(pSymbol->Address, BaseAddress + 0x1010);
    ok_hex(pSymbol->Tag, SymTagFunction);
    ok_str(pSymbol->Name, "DllMain");

    /* The actual first instruction of the function */
    Displacement = 0;
    INIT_PSYM(buffer);
    Ret = SymFromAddr(hProc, BaseAddress + 0x1010, &Displacement, pSymbol);
    ok_int(Ret, TRUE);
    ok_ulonglong(Displacement, 0);
    ok_ulonglong(pSymbol->ModBase, BaseAddress);
    ok_hex(pSymbol->Flags, 0);
    ok_ulonglong(pSymbol->Address, BaseAddress + 0x1010);
    ok_hex(pSymbol->Tag, SymTagFunction);
    ok_str(pSymbol->Name, "DllMain");

    /* The last instruction in the function */
    Displacement = 0;
    INIT_PSYM(buffer);
    Ret = SymFromAddr(hProc, BaseAddress + 0x102D, &Displacement, pSymbol);
    ok_int(Ret, TRUE);
    ok_ulonglong(Displacement, 0x1d);
    ok_ulonglong(pSymbol->ModBase, BaseAddress);
    ok_hex(pSymbol->Flags, 0);
    ok_ulonglong(pSymbol->Address, BaseAddress + 0x1010);
    ok_hex(pSymbol->Tag, SymTagFunction);
    ok_str(pSymbol->Name, "DllMain");

    /* The padding below the function */
    Displacement = 0;
    INIT_PSYM(buffer);
    Ret = SymFromAddr(hProc, BaseAddress + 0x102E, &Displacement, pSymbol);
    ok_int(Ret, TRUE);
    ok_ulonglong(Displacement, 0x1e);
    ok_ulonglong(pSymbol->ModBase, BaseAddress);
    ok_hex(pSymbol->Flags, 0);
    ok_ulonglong(pSymbol->Address, BaseAddress + 0x1010);
    ok_hex(pSymbol->Tag, SymTagFunction);
    ok_str(pSymbol->Name, "DllMain");

    /* One byte before the next function */
    Displacement = 0;
    INIT_PSYM(buffer);
    Ret = SymFromAddr(hProc, BaseAddress + 0x103f, &Displacement, pSymbol);
    ok_int(Ret, TRUE);
    ok_ulonglong(Displacement, 0x2f);
    ok_ulonglong(pSymbol->ModBase, BaseAddress);
    ok_hex(pSymbol->Flags, 0);
    ok_ulonglong(pSymbol->Address, BaseAddress + 0x1010);
    ok_hex(pSymbol->Tag, SymTagFunction);
    ok_str(pSymbol->Name, "DllMain");

    /* First byte of the next function */
    Displacement = 0;
    INIT_PSYM(buffer);
    Ret = SymFromAddr(hProc, BaseAddress + 0x1040, &Displacement, pSymbol);
    ok_int(Ret, TRUE);
    ok_ulonglong(Displacement, 0);
    ok_ulonglong(pSymbol->ModBase, BaseAddress);
    ok_hex(pSymbol->Flags, 0);
    ok_ulonglong(pSymbol->Address, BaseAddress + 0x1040);
    ok_hex(pSymbol->Tag, SymTagFunction);
    ok_str(pSymbol->Name, "FfsChkdsk");

    if (!supports_pdb(hProc, BaseAddress))
    {
        skip("dbghelp.dll too old or cannot read this symbol!\n");
    }
    else
    {
        /* .idata */
        Displacement = 0;
        INIT_PSYM(buffer);
        Ret = SymFromAddr(hProc, BaseAddress + 0x2000, &Displacement, pSymbol);
        ok_int(Ret, TRUE);
        ok_ulonglong(Displacement, 0);
        ok_ulonglong(pSymbol->ModBase, BaseAddress);
        ok_hex(pSymbol->Flags, 0);
        ok_ulonglong(pSymbol->Address, BaseAddress + 0x2000);
        ok_hex(pSymbol->Tag, SymTagPublicSymbol);
        ok_str(pSymbol->Name, "__imp__DbgPrint");
    }
}

typedef struct _test_context
{
    DWORD64 BaseAddress;
    SIZE_T Index;
} test_context;

static struct _test_data {
    DWORD64 AddressOffset;
    ULONG Size;
    ULONG Tag;
    const char* Name;
} test_data[] = {
    /* TODO: Order is based on magic, should find entries based on name, and mark as 'seen' */
    { 0x1070, 36, SymTagFunction, "FfsFormat" },
    { 0x1010, 32, SymTagFunction, "DllMain" },
    { 0x1040, 36, SymTagFunction, "FfsChkdsk" },

    { 0x2100, 0, SymTagPublicSymbol, "__IMPORT_DESCRIPTOR_ntdll" },
    { 0x109a, 0, SymTagPublicSymbol, "_DbgPrint" },
    { 0x2004, 0, SymTagPublicSymbol, "\x7fntdll_NULL_THUNK_DATA" },
    { 0x2000, 0, SymTagPublicSymbol, "__imp__DbgPrint" },
    { 0x2114, 0, SymTagPublicSymbol, "__NULL_IMPORT_DESCRIPTOR" },
};

static BOOL CALLBACK EnumSymProc(PSYMBOL_INFO pSymInfo, ULONG SymbolSize, PVOID UserContext)
{
    test_context* ctx = UserContext;

    if (ctx->Index < ARRAYSIZE(test_data))
    {
        ok_ulonglong(pSymInfo->ModBase, ctx->BaseAddress);
        ok_ulonglong(pSymInfo->Address, ctx->BaseAddress + test_data[ctx->Index].AddressOffset);
        ok_hex(pSymInfo->Tag, test_data[ctx->Index].Tag);
        ok_str(pSymInfo->Name, test_data[ctx->Index].Name);

        ctx->Index++;
    }
    else
    {
        ok(0, "Out of bounds (%lu), max is: %i!\n", ctx->Index, ARRAYSIZE(test_data));
    }

    return TRUE;
}

static void test_SymEnumSymbols(HANDLE hProc, DWORD64 BaseAddress)
{
    BOOL Ret;
    test_context ctx;

    ctx.Index = 0;
    ctx.BaseAddress = BaseAddress;

    if (!supports_pdb(hProc, ctx.BaseAddress))
    {
        skip("dbghelp.dll too old or cannot enumerate symbols!\n");
    }
    else
    {
        Ret = SymEnumSymbols(hProc, ctx.BaseAddress, NULL, EnumSymProc, &ctx);
        ok_int(Ret, TRUE);
        ok_int(ctx.Index, ARRAYSIZE(test_data));
    }
}

typedef struct _symregcallback_context
{
    UINT idx;
    BOOL isANSI;
} symregcallback_context;

static struct _symregcallback_test_data {
    ULONG ActionCode;
} symregcallback_test_data[] = {
    { CBA_DEFERRED_SYMBOL_LOAD_CANCEL },
    { CBA_DEFERRED_SYMBOL_LOAD_START },
    { CBA_READ_MEMORY },
    { CBA_DEFERRED_SYMBOL_LOAD_PARTIAL },
    { CBA_DEFERRED_SYMBOL_LOAD_COMPLETE }
};

static BOOL CALLBACK SymRegisterCallback64Proc(
    HANDLE hProcess,
    ULONG ActionCode,
    ULONG64 CallbackData,
    ULONG64 UserContext)
{
    symregcallback_context *ctx;
    ctx = (symregcallback_context*)(ULONG_PTR)UserContext;

    if (ctx->idx > sizeof(symregcallback_test_data))
    {
        ok(FALSE, "SymRegisterCallback64Proc: Too many calls.\n");
    }
    else
    {
        ok(ActionCode == symregcallback_test_data[ctx->idx].ActionCode,
            "ActionCode (idx %u) expected %u, got %u\n",
            ctx->idx, symregcallback_test_data[ctx->idx].ActionCode, ActionCode);
    }
    ctx->idx++;

    return FALSE;
}

static void test_SymRegCallback(HANDLE hProc, const char* szModuleName, BOOL testANSI)
{
    BOOL Ret;
    DWORD dwErr;
    ULONG64 BaseAddress;
    symregcallback_context ctx;

    ctx.idx = 0;
    ctx.isANSI = testANSI;

    if (!init_sym(FALSE))
        return;

    if (testANSI)
    {
        Ret = SymRegisterCallback64(hProc, SymRegisterCallback64Proc, (ULONG_PTR)&ctx);
    }
    else
    {
        Ret = SymRegisterCallbackW64(hProc, SymRegisterCallback64Proc, (ULONG_PTR)&ctx);
        if (g_SymRegisterCallbackW64NotFound)
        {
            skip("SymRegisterCallbackW64 not found in dbghelp.dll\n");
            return;
        }
    }

    ok_int(Ret, TRUE);
    if (!Ret)
        return;

    SetLastError(ERROR_SUCCESS);
    BaseAddress = SymLoadModule64(hProc, NULL, szModuleName, NULL, 0x600000, 0);
    dwErr = GetLastError();

    ok_ulonglong(BaseAddress, 0x600000);
    ok_hex(dwErr, ERROR_SUCCESS);

    /* this is what we want to test ... we expect 5 calls */
    ok_int(ctx.idx, 5);

    deinit_sym();
}

START_TEST(pdb)
{
    char szDllName[MAX_PATH];
    char szDllPath[MAX_PATH], szOldDir[MAX_PATH];
    HMODULE hMod;
    DWORD64 BaseAddress;
    DWORD dwErr, Options;

    Options = SymGetOptions();
    Options &= ~(SYMOPT_UNDNAME);
    //Options |= SYMOPT_DEBUG;
    SymSetOptions(Options);

    if (!extract_msvc_dll(szDllName, szDllPath))
    {
        ok(0, "Failed extracting files\n");
        return;
    }

    init_dbghelp_version();

    /* Register the failure hook using the magic name '__pfnDliFailureHook2'. */
    __pfnDliFailureHook2 = DliFailHook;

    if (init_sym(FALSE))
    {
        SetLastError(ERROR_SUCCESS);
        BaseAddress = SymLoadModule64(proc(), NULL, szDllName, NULL, 0x600000, 0);
        dwErr = GetLastError();

        ok_ulonglong(BaseAddress, 0x600000);
        ok_hex(dwErr, ERROR_SUCCESS);

        if (BaseAddress == 0x600000)
        {
            trace("Module loaded by SymLoadModule64\n");
            test_SymFromName(proc(), BaseAddress);
            test_SymFromAddr(proc(), BaseAddress);
            test_SymEnumSymbols(proc(), BaseAddress);
        }

        deinit_sym();
    }

    /* This needs to load the module by itself */
    test_SymRegCallback(proc(), szDllName, TRUE);
    test_SymRegCallback(proc(), szDllName, FALSE);

    hMod = LoadLibraryA(szDllName);
    if (hMod)
    {
        BaseAddress = (DWORD64)(DWORD_PTR)hMod;
        /* Make sure we can find the pdb */
        GetCurrentDirectoryA(_countof(szOldDir), szOldDir);
        SetCurrentDirectoryA(szDllPath);
        /* Invade process */
        if (init_sym(TRUE))
        {
            trace("Module loaded by LoadLibraryA\n");
            test_SymFromName(proc(), BaseAddress);
            test_SymFromAddr(proc(), BaseAddress);
            test_SymEnumSymbols(proc(), BaseAddress);

            deinit_sym();
        }
        /* Restore working dir */
        SetCurrentDirectoryA(szOldDir);

        FreeLibrary(hMod);
    }

    cleanup_msvc_dll();
}
