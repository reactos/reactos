/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for dbghelp rsym functions
 * PROGRAMMER:      Mark Jansen
 *
 *                  These tests are based on the PDB tests.
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

#define ok_ulonglong_(file, line, expression, result) \
    do { \
        ULONG64 _value = (expression); \
        ULONG64 _result = (result); \
        ok_(file, line)(_value == (result), "Wrong value for '%s', expected: " #result " (%s), got: %s\n", \
           #expression, wine_dbgstr_longlong(_result), wine_dbgstr_longlong(_value)); \
    } while (0)

#define ok_hex_(file, line, expression, result) \
    do { \
        int _value = (expression); \
        ok_(file, line)(_value == (result), "Wrong value for '%s', expected: " #result " (0x%x), got: 0x%x\n", \
           #expression, (int)(result), _value); \
    } while (0)

#define ok_str_(file, line, x, y) \
    ok_(file, line)(strcmp(x, y) == 0, "Wrong string. Expected '%s', got '%s'\n", y, x)


// data.c
void dump_rsym(const char* filename);
int extract_gcc_exe(char szFile[MAX_PATH]);
void cleanup_gcc_exe();

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

static BOOL supports_rsym(HANDLE hProc, DWORD64 BaseAddress)
{
    IMAGEHLP_MODULE64 ModuleInfo;
    BOOL Ret;

    memset(&ModuleInfo, 0, sizeof(ModuleInfo));
    ModuleInfo.SizeOfStruct = sizeof(ModuleInfo);
    Ret = SymGetModuleInfo64(hProc, BaseAddress, &ModuleInfo);

    return Ret &&
        ModuleInfo.SymType == SymDia &&
        ModuleInfo.CVSig == ('R' | ('S' << 8) | ('Y' << 16) | ('M' << 24));
}

#define init_sym()          init_sym_imp(__FILE__, __LINE__)

#define INIT_PSYM(buff) do { \
    memset((buff), 0, sizeof((buff))); \
    ((PSYMBOL_INFO)(buff))->SizeOfStruct = sizeof(SYMBOL_INFO); \
    ((PSYMBOL_INFO)(buff))->MaxNameLen = MAX_SYM_NAME; \
} while (0)


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

    if (supports_rsym(hProc, BaseAddress))
    {
        ok_ulonglong(BaseAddress, 0x600000);
        ok_hex(dwErr, ERROR_SUCCESS);

        INIT_PSYM(buffer);
        Ret = SymFromName(hProc, "DllMain", pSymbol);
        ok_int(Ret, TRUE);
        ok_ulonglong(pSymbol->ModBase, BaseAddress);
        ok_hex(pSymbol->Flags, SYMFLAG_FUNCTION);
        ok_ulonglong(pSymbol->Address, BaseAddress + 0x1000);
        ok_hex(pSymbol->Tag, SymTagFunction);
        ok_str(pSymbol->Name, "DllMain");

        INIT_PSYM(buffer);
        Ret = SymFromName(hProc, "FfsChkdsk", pSymbol);
        ok_int(Ret, TRUE);
        ok_ulonglong(pSymbol->ModBase, BaseAddress);
        ok_hex(pSymbol->Flags, SYMFLAG_FUNCTION);
        ok_ulonglong(pSymbol->Address, BaseAddress + 0x103F);
        ok_hex(pSymbol->Tag, SymTagFunction);
        ok_str(pSymbol->Name, "FfsChkdsk");

        INIT_PSYM(buffer);
        Ret = SymFromName(hProc, "FfsFormat", pSymbol);
        ok_int(Ret, TRUE);
        ok_ulonglong(pSymbol->ModBase, BaseAddress);
        ok_hex(pSymbol->Flags, SYMFLAG_FUNCTION);
        ok_ulonglong(pSymbol->Address, BaseAddress + 0x100C);
        ok_hex(pSymbol->Tag, SymTagFunction);
        ok_str(pSymbol->Name, "FfsFormat");
    }
    else
    {
        skip("dbghelp.dll cannot parse rsym\n");
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

    if (supports_rsym(hProc, BaseAddress))
    {
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
        /* Our dbghelp.dll does not recognize this yet */
        todo_if(!Ret)
        {
            ok_int(Ret, TRUE);
            ok_ulonglong(Displacement, 0xffffffffffffffff);
            ok_ulonglong(pSymbol->ModBase, BaseAddress);
            ok_hex(pSymbol->Flags, SYMFLAG_FUNCTION);
            ok_ulonglong(pSymbol->Address, BaseAddress + 0x1000);
            ok_hex(pSymbol->Tag, SymTagFunction);
            ok_str(pSymbol->Name, "DllMain");
        }

        /* The actual first instruction of the function */
        Displacement = 0;
        INIT_PSYM(buffer);
        Ret = SymFromAddr(hProc, BaseAddress + 0x1000, &Displacement, pSymbol);
        ok_int(Ret, TRUE);
        ok_ulonglong(Displacement, 0);
        ok_ulonglong(pSymbol->ModBase, BaseAddress);
        ok_hex(pSymbol->Flags, SYMFLAG_FUNCTION);
        ok_ulonglong(pSymbol->Address, BaseAddress + 0x1000);
        ok_hex(pSymbol->Tag, SymTagFunction);
        ok_str(pSymbol->Name, "DllMain");

        /* The last instruction in the function */
        Displacement = 0;
        INIT_PSYM(buffer);
        Ret = SymFromAddr(hProc, BaseAddress + 0x1009, &Displacement, pSymbol);
        ok_int(Ret, TRUE);
        ok_ulonglong(Displacement, 0x9);
        ok_ulonglong(pSymbol->ModBase, BaseAddress);
        ok_hex(pSymbol->Flags, SYMFLAG_FUNCTION);
        ok_ulonglong(pSymbol->Address, BaseAddress + 0x1000);
        ok_hex(pSymbol->Tag, SymTagFunction);
        ok_str(pSymbol->Name, "DllMain");

        /* First byte of the next function */
        Displacement = 0;
        INIT_PSYM(buffer);
        Ret = SymFromAddr(hProc, BaseAddress + 0x103F, &Displacement, pSymbol);
        ok_int(Ret, TRUE);
        ok_ulonglong(Displacement, 0);
        ok_ulonglong(pSymbol->ModBase, BaseAddress);
        ok_hex(pSymbol->Flags, SYMFLAG_FUNCTION);
        ok_ulonglong(pSymbol->Address, BaseAddress + 0x103F);
        ok_hex(pSymbol->Tag, SymTagFunction);
        ok_str(pSymbol->Name, "FfsChkdsk");

        /* .idata */
        Displacement = 0;
        INIT_PSYM(buffer);
        Ret = SymFromAddr(hProc, BaseAddress + 0x4000, &Displacement, pSymbol);
        ok_int(Ret, TRUE);
        ok_ulonglong(Displacement, 0);
        ok_ulonglong(pSymbol->ModBase, BaseAddress);
        ok_hex(pSymbol->Flags, SYMFLAG_EXPORT);
        ok_ulonglong(pSymbol->Address, BaseAddress + 0x4000);
        ok_hex(pSymbol->Tag, SymTagPublicSymbol);
        ok_str(pSymbol->Name, "_head_dll_ntdll_libntdll_a");
    }
    else
    {
        skip("dbghelp.dll cannot parse rsym\n");
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
    int Line;
} test_data[] = {

    /* TODO: Order is based on magic, should find entries based on name, and mark as 'seen' */
    { 0x107c, 0, SymTagPublicSymbol, "__CTOR_LIST__", __LINE__ },
    { 0x2074, 0, SymTagPublicSymbol, "__RUNTIME_PSEUDO_RELOC_LIST_END__", __LINE__ },
    { 0x1000, 12, SymTagPublicSymbol, "EntryPoint", __LINE__ },
    { 0x100c, 51, SymTagFunction, "FfsFormat", __LINE__ },
    { 0x4030, 0, SymTagPublicSymbol, "_imp__DbgPrint", __LINE__ },
    { 0x1084, 0, SymTagPublicSymbol, "__DTOR_LIST__", __LINE__ },
    { 0x103f, 53, SymTagFunction, "FfsChkdsk", __LINE__ },
    { 0x2074, 0, SymTagPublicSymbol, "_rt_psrelocs_end", __LINE__ },
    { 0x103f, 53, SymTagPublicSymbol, "ChkdskEx", __LINE__ },
    { 0x4048, 0, SymTagPublicSymbol, "_dll_ntdll_libntdll_a_iname", __LINE__ },



    { 0x2074, 0, SymTagPublicSymbol, "_rt_psrelocs_start", __LINE__ },
    { 0x1000, 12, SymTagFunction, "DllMain", __LINE__ },
    { 0x100c, 0, SymTagPublicSymbol, "FormatEx", __LINE__ },
    { 0x1074, 0, SymTagPublicSymbol, "DbgPrint", __LINE__ },
    { 0x68900000, 0, SymTagPublicSymbol, "__ImageBase", __LINE__ },
    { 0x68902074, 0, SymTagPublicSymbol, "__RUNTIME_PSEUDO_RELOC_LIST__", __LINE__ },
    { 0x4000, 0, SymTagPublicSymbol, "_head_dll_ntdll_libntdll_a", __LINE__ },
};

BOOL CALLBACK EnumSymProc(PSYMBOL_INFO pSymInfo, ULONG SymbolSize, PVOID UserContext)
{
    test_context* ctx = UserContext;

    if (ctx->Index < ARRAYSIZE(test_data))
    {
        ok_ulonglong_(__FILE__, test_data[ctx->Index].Line, pSymInfo->ModBase, ctx->BaseAddress);
        if (test_data[ctx->Index].AddressOffset > 0x100000)
            ok_ulonglong_(__FILE__, test_data[ctx->Index].Line, pSymInfo->Address, test_data[ctx->Index].AddressOffset);
        else
            ok_ulonglong_(__FILE__, test_data[ctx->Index].Line, pSymInfo->Address, ctx->BaseAddress + test_data[ctx->Index].AddressOffset);
        ok_hex_(__FILE__, test_data[ctx->Index].Line, pSymInfo->Tag, test_data[ctx->Index].Tag);
        ok_str_(__FILE__, test_data[ctx->Index].Line, pSymInfo->Name, test_data[ctx->Index].Name);

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

    if (supports_rsym(hProc, ctx.BaseAddress))
    {
        ok_ulonglong(ctx.BaseAddress, 0x600000);
        ok_hex(dwErr, ERROR_SUCCESS);

        Ret = SymEnumSymbols(hProc, ctx.BaseAddress, NULL, EnumSymProc, &ctx);
        ok_int(Ret, TRUE);
        ok_int(ctx.Index, ARRAYSIZE(test_data));
    }
    else
    {
        skip("dbghelp.dll cannot parse rsym\n");
    }

    deinit_sym();
}




START_TEST(rsym)
{
    char szDllName[MAX_PATH];
    //dump_rsym("R:\\src\\trunk\\reactos\\modules\\rostests\\apitests\\dbghelp\\testdata\\gcc_uffs.dll");

    DWORD Options = SymGetOptions();
    Options &= ~(SYMOPT_UNDNAME);
    //Options |= SYMOPT_DEBUG;
    SymSetOptions(Options);

    if (!extract_gcc_exe(szDllName))
    {
        ok(0, "Failed extracting files\n");
        return;
    }

    test_SymFromName(proc(), szDllName);
    test_SymFromAddr(proc(), szDllName);
    test_SymEnumSymbols(proc(), szDllName);

    cleanup_gcc_exe();
}
