/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for dbghelp PDB functions
 * PROGRAMMER:      Mark Jansen
 */

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <dbghelp.h>
#include <cvconst.h>    // SymTagXXX
#include <stdio.h>

#include "wine/test.h"

#define ok_ulonglong(expression, result) \
    do { \
        ULONG64 _value = (expression); \
        ULONG64 _result = (result); \
        ok(_value == (result), "Wrong value for '%s', expected: " #result " (%s), got: %s\n", \
           #expression, wine_dbgstr_longlong(_result), wine_dbgstr_longlong(_value)); \
    } while (0)


// data.c
void create_compressed_files();
int extract_msvc_exe(char szFile[MAX_PATH]);
void cleanup_msvc_exe();

static HANDLE proc()
{
    return GetCurrentProcess();
}

static BOOL init_sym_imp(const char* file, int line)
{
    if (!SymInitialize(proc(), NULL, FALSE))
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

#define init_sym()          init_sym_imp(__FILE__, __LINE__)

#define INIT_PSYM(buff) do { \
    memset((buff), 0, sizeof((buff))); \
    ((PSYMBOL_INFO)(buff))->SizeOfStruct = sizeof(SYMBOL_INFO); \
    ((PSYMBOL_INFO)(buff))->MaxNameLen = MAX_SYM_NAME; \
} while (0)


/* Maybe our dbghelp.dll is too old? */
static BOOL can_enumerate(HANDLE hProc, DWORD64 BaseAddress)
{
    IMAGEHLP_MODULE64 ModuleInfo;
    BOOL Ret;

    memset(&ModuleInfo, 0, sizeof(ModuleInfo));
    ModuleInfo.SizeOfStruct = sizeof(ModuleInfo);
    Ret = SymGetModuleInfo64(hProc, BaseAddress, &ModuleInfo);

    return Ret && ModuleInfo.SymType == SymPdb;
}


static void test_SymFromName(HANDLE hProc, const char* szModuleName)
{
    BOOL Ret;
    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
    PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;

    DWORD64 BaseAddress;
    DWORD dwErr;

    if (!init_sym())
        return;

    SetLastError(ERROR_SUCCESS);
    BaseAddress = SymLoadModule64(hProc, NULL, szModuleName, NULL, 0x600000, 0);
    dwErr = GetLastError();

    ok_ulonglong(BaseAddress, 0x600000);
    ok_hex(dwErr, ERROR_SUCCESS);

    if (!can_enumerate(hProc, BaseAddress))
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

    deinit_sym();
}

static void test_SymFromAddr(HANDLE hProc, const char* szModuleName)
{
    BOOL Ret;
    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
    PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;

    DWORD64 BaseAddress, Displacement;
    DWORD dwErr;

    if (!init_sym())
        return;

    SetLastError(ERROR_SUCCESS);
    BaseAddress = SymLoadModule64(hProc, NULL, szModuleName, NULL, 0x600000, 0);
    dwErr = GetLastError();

    ok_ulonglong(BaseAddress, 0x600000);
    ok_hex(dwErr, ERROR_SUCCESS);

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

    if (!can_enumerate(hProc, BaseAddress))
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

    deinit_sym();
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

static void test_SymEnumSymbols(HANDLE hProc, const char* szModuleName)
{
    BOOL Ret;
    DWORD dwErr;

    test_context ctx;

    if (!init_sym())
        return;

    ctx.Index = 0;
    SetLastError(ERROR_SUCCESS);
    ctx.BaseAddress = SymLoadModule64(hProc, NULL, szModuleName, NULL, 0x600000, 0);
    dwErr = GetLastError();

    ok_ulonglong(ctx.BaseAddress, 0x600000);
    ok_hex(dwErr, ERROR_SUCCESS);

    if (!can_enumerate(hProc, ctx.BaseAddress))
    {
        skip("dbghelp.dll too old or cannot enumerate symbols!\n");
    }
    else
    {
        Ret = SymEnumSymbols(hProc, ctx.BaseAddress, NULL, EnumSymProc, &ctx);
        ok_int(Ret, TRUE);
        ok_int(ctx.Index, ARRAYSIZE(test_data));
    }

    deinit_sym();
}


START_TEST(pdb)
{
    char szDllName[MAX_PATH];
    //create_compressed_files();

    DWORD Options = SymGetOptions();
    Options &= ~(SYMOPT_UNDNAME);
    //Options |= SYMOPT_DEBUG;
    SymSetOptions(Options);

    if (!extract_msvc_exe(szDllName))
    {
        ok(0, "Failed extracting files\n");
        return;
    }

    test_SymFromName(proc(), szDllName);
    test_SymFromAddr(proc(), szDllName);
    test_SymEnumSymbols(proc(), szDllName);

    cleanup_msvc_exe();
}
