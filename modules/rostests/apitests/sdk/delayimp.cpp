/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Tests for delayload
 * PROGRAMMER:      Mark Jansen
 */

#include <apitest.h>

#include <apitest.h>
#include <strsafe.h>
#include <delayimp.h>

/* Some libraries to test against */
#include <mmsystem.h>
#include <winver.h>
#include <shlwapi.h>
#include <intshcut.h>
#include <sfc.h>
#include <imagehlp.h>
#include <mmddk.h>

#include <pseh/pseh2.h>

/* Compatibility with the MS defines */

#ifndef FACILITY_VISUALCPP
#define FACILITY_VISUALCPP  ((LONG)0x6d)
#endif

#ifndef VcppException
#define VcppException(sev,err)  ((sev) | (FACILITY_VISUALCPP<<16) | err)
#endif

#ifdef __REACTOS__
#define WINMM_DLLNAME   "winmm.dll"
#else
#define WINMM_DLLNAME   "WINMM.dll"
#endif

bool g_BreakFunctionName = false;
bool g_BrokenFunctionName = false;
bool g_BypassMost = false;
bool g_ExceptionIsModule = false;
bool g_ImportByName = true;
const char* g_ExpectedDll = NULL;
const char* g_ExpectedName = NULL;
char g_Target[100] = { 0 };

char* target(PDelayLoadInfo pdli)
{
    if (g_Target[0] == '\0' && pdli)
    {
        if (pdli->dlp.fImportByName)
            sprintf(g_Target, "%s!%s", pdli->szDll, pdli->dlp.szProcName);
        else
            sprintf(g_Target, "%s!#%lu", pdli->szDll, pdli->dlp.dwOrdinal);
    }
    return g_Target;
}


struct UnProtect
{
    UnProtect(PVOID addr)
        :mAddr(NULL), mProt(0)
    {
        if (IsBadWritePtr(addr, 1))
        {
            mAddr = addr;
            VirtualProtect(addr, 1, PAGE_EXECUTE_READWRITE, &mProt);
        }
    }
    ~UnProtect()
    {
        DWORD dwOld;
        if (mAddr)
            VirtualProtect(mAddr, 1, mProt, &dwOld);
    }

    PVOID mAddr;
    DWORD mProt;
};


unsigned* g_DliHookExpected = NULL;
size_t g_DliHookIndex = 0;
#define LAST_DLI    333

static void SetExpectedDli(unsigned* order)
{
    g_DliHookExpected = order;
    g_DliHookIndex = 0;
    g_Target[0] = '\0';
}

static void CheckDli_imp(unsigned dliNotify, PDelayLoadInfo pdli, BOOL ErrorHandler)
{
    if (!g_DliHookExpected) return;

    winetest_ok(dliNotify == g_DliHookExpected[g_DliHookIndex], "Expected dliNotify to be %u, was: %u for %s\n",
        g_DliHookExpected[g_DliHookIndex], dliNotify, target(pdli));
    if (ErrorHandler)
    {
        winetest_ok(dliNotify == dliFailGetProc || dliNotify == dliFailLoadLib,
            "Expected code %u to be processed by the Hook, not the ErrorHandler for %s\n", dliNotify, target(pdli));
    }
    else
    {
        winetest_ok(dliNotify == dliStartProcessing || dliNotify == dliNotePreLoadLibrary ||
            dliNotify == dliNotePreGetProcAddress || dliNotify == dliNoteEndProcessing,
            "Expected code %u to be processed by the ErrorHandler, not the Hook for %s\n", dliNotify, target(pdli));
    }
    if (g_DliHookExpected[g_DliHookIndex] != LAST_DLI)
        g_DliHookIndex++;
}

static void CheckDliDone_imp()
{
    if (!g_DliHookExpected) return;
    winetest_ok(LAST_DLI == g_DliHookExpected[g_DliHookIndex],
        "Expected g_DliHookExpected[g_DliHookIndex] to be %u, was: %u for %s\n",
        LAST_DLI, g_DliHookExpected[g_DliHookIndex], target(NULL));
    g_DliHookExpected = NULL;
    g_Target[0] = '\0';
}

#define CheckDli        (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : CheckDli_imp
#define CheckDliDone    (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : CheckDliDone_imp


/* Replacement functions */
INT_PTR WINAPI MyFunction()
{
    return 123;
}

BOOL WINAPI MySfcIsKeyProtected(HKEY KeyHandle, LPCWSTR SubKeyName, REGSAM KeySam)
{
    return 12345;
}


static HMODULE g_VersionDll;
FARPROC WINAPI DliHook(unsigned dliNotify, PDelayLoadInfo pdli)
{
    ok(pdli && pdli->cb >= 36, "Expected a valid pointer with a struct that is big enough: %p, %lu\n",
        pdli, pdli ? pdli->cb : 0u);
    if (!pdli || pdli->cb < 36) return NULL;

    CheckDli(dliNotify, pdli, FALSE);

    if (g_BreakFunctionName && pdli->dlp.fImportByName)
    {
        g_BreakFunctionName = false;
        g_BrokenFunctionName = true;
        char* procname = (char*)pdli->dlp.szProcName;
        UnProtect prot(procname);
        char c = procname[0];
        procname[0] = isupper(c) ? tolower(c) : toupper(c);
    }

    /* Validate dll name when required */
    if (g_ExpectedDll && g_ExpectedName && !g_BrokenFunctionName)
    {
        ok(!strcmp(g_ExpectedDll, pdli->szDll), "Expected szDll to be '%s', but was: '%s'\n", g_ExpectedDll, pdli->szDll);
        ok(pdli->dlp.fImportByName, "Expected import by name (%s!%s)\n", g_ExpectedDll, g_ExpectedName);
        if (pdli->dlp.fImportByName)
        {
            ok(!strcmp(g_ExpectedName, pdli->dlp.szProcName), "Expected szProcName to be '%s', but was: '%s'\n",
                g_ExpectedName, pdli->dlp.szProcName);
        }
    }


    if (dliNotify == dliStartProcessing)
    {
        /* Test loadlib fail */
        if (!_stricmp(pdli->szDll, "sfc_os.dll"))
        {
            char* dll = (char*)pdli->szDll;
            UnProtect u(dll);
            dll[0] = 'l'; dll[1] = '_'; dll[2] = 'm';
        }
        if (!_stricmp(pdli->szDll, "imagehlp.dll"))
        {
            char* dll = (char*)pdli->szDll;
            UnProtect u(dll);
            dll[0] = 'x'; dll[1] = 'x'; dll[2] = 'x'; dll[3] = 'x'; dll[4] = 'x';
        }
        /* Test bypass */
        if (!_stricmp(pdli->szDll, "dbghelp.dll"))
            return MyFunction;
    }
    else if (dliNotify == dliNotePreLoadLibrary)
    {
        /* Show that this value is actually used! */
        if (!_stricmp(pdli->szDll, "version.dll"))
        {
            g_VersionDll = LoadLibraryA("version.dll");
            return (FARPROC)1;
        }

    }
    else if (dliNotify == dliNotePreGetProcAddress)
    {
        if (pdli->dlp.fImportByName && !strcmp(pdli->dlp.szProcName, "SfcIsKeyProtected"))
        {
            return (FARPROC)MySfcIsKeyProtected;
        }
    }

    /* Parameter validation */
    ok(pdli->ppfn != NULL, "Expected ppfn to be valid, was NULL for %s\n", target(pdli));
    ok(pdli->szDll != NULL, "Expected szDll to be valid, was NULL for %s\n", target(pdli));
    ok(pdli->dwLastError == ERROR_SUCCESS,
        "Expected dwLastError to be ERROR_SUCCESS, was: %lu for %s\n", pdli->dwLastError, target(pdli));
    ok(g_ImportByName == !!pdli->dlp.fImportByName, "Expected pdli->dlp.fImportByName to equal g_ImportByname\n");
    if (pdli->dlp.fImportByName)
        ok(pdli->dlp.szProcName != NULL, "Expected szProcName to be valid, was NULL for %s\n", target(pdli));
    else
        ok(pdli->dlp.dwOrdinal != 0, "Expected dwOrdinal to be valid, was NULL for %s\n", target(pdli));
    switch(dliNotify)
    {
        case dliStartProcessing:
            ok(pdli->hmodCur == NULL, "Expected hmodCur to be NULL, was: %p for %s\n", pdli->hmodCur, target(pdli));
            ok(pdli->pfnCur == NULL, "Expected pfnCur to be NULL, was: %p for %s\n", pdli->pfnCur, target(pdli));
            break;
        case dliNotePreLoadLibrary:
            ok(pdli->hmodCur == NULL, "Expected hmodCur to be NULL, was: %p for %s\n", pdli->hmodCur, target(pdli));
            ok(pdli->pfnCur == NULL, "Expected pfnCur to be NULL, was: %p for %s\n", pdli->pfnCur, target(pdli));
            break;
        case dliNotePreGetProcAddress:
            ok(pdli->hmodCur != NULL, "Expected hmodCur to be valid, was NULL for %s\n", target(pdli));
            ok(pdli->pfnCur == NULL, "Expected pfnCur to be NULL, was: %p for %s\n", pdli->pfnCur, target(pdli));
            break;
        case dliNoteEndProcessing:
            if (!g_BypassMost)
                ok(pdli->hmodCur != NULL, "Expected hmodCur to be valid, was NULL for %s\n", target(pdli));
            ok(pdli->pfnCur != NULL, "Expected pfnCur to be a valid pointer, was NULL for %s\n", target(pdli));
            if (g_ExpectedDll && g_ExpectedName && !g_BrokenFunctionName)
            {
                FARPROC targetProc = GetProcAddress(GetModuleHandleA(g_ExpectedDll), g_ExpectedName);
                ok(targetProc != NULL, "This should not happen, the function i need is unavail! (%s!%s)\n",
                    g_ExpectedDll, g_ExpectedName);
                ok(targetProc == pdli->pfnCur, "Expected pfnCur to be %p, was %p for %s\n", targetProc, pdli->pfnCur, target(pdli));
                ok(pdli->ppfn && targetProc == *pdli->ppfn,
                    "Expected ppfn to be valid and to result in %p, was: %p(%p) for %s\n",
                    target, pdli->ppfn, pdli->ppfn ? *pdli->ppfn : NULL, target(pdli));
            }
            break;
        default:
            break;
    }
    return NULL;
}

FARPROC WINAPI DliFailHook(unsigned dliNotify, PDelayLoadInfo pdli)
{
    ok(pdli && pdli->cb >= 36,
        "Expected a valid pointer with a struct that is big enough: %p, %lu\n", pdli, pdli ? pdli->cb : 0u);
    if (!pdli || pdli->cb < 36) return NULL;

    CheckDli(dliNotify, pdli, TRUE);

    /* Redirections / fixes */
    if (dliNotify == dliFailLoadLib)
    {
        if (!_stricmp(pdli->szDll, "l_m_os.dll"))
            return (FARPROC)LoadLibraryA("sfc_os.dll");
    }
    else if (dliNotify == dliFailGetProc)
    {
        if (pdli->dlp.fImportByName && pdli->hmodCur == (HMODULE)1)
        {
            return GetProcAddress(g_VersionDll, pdli->dlp.szProcName);
        }
    }

    /* Parameter validation */
    ok(pdli->ppfn != NULL, "Expected ppfn to be valid, was NULL for %s\n", target(pdli));
    ok(pdli->szDll != NULL, "Expected szDll to be valid, was NULL for %s\n", target(pdli));
    if (pdli->dlp.fImportByName)
        ok(pdli->dlp.szProcName != NULL, "Expected szProcName to be valid, was NULL for %s\n", target(pdli));
    else
        ok(pdli->dlp.dwOrdinal != 0, "Expected dwOrdinal to be valid, was NULL for %s\n", target(pdli));
    switch(dliNotify)
    {
        case dliFailLoadLib:
            ok(pdli->hmodCur == NULL, "Expected hmodCur to be NULL, was: %p for %s\n", pdli->hmodCur, target(pdli));
            ok(pdli->pfnCur == NULL, "Expected pfnCur to be NULL, was: %p for %s\n", pdli->pfnCur, target(pdli));
            ok(pdli->dwLastError == ERROR_MOD_NOT_FOUND,
                "Expected dwLastError to be ERROR_MOD_NOT_FOUND, was: %lu for %s\n", pdli->dwLastError, target(pdli));
            break;
        case dliFailGetProc:
            ok(pdli->hmodCur != NULL, "Expected hmodCur to be valid, was NULL for %s\n", target(pdli));
            ok(pdli->pfnCur == NULL, "Expected pfnCur to be NULL, was: %p for %s\n", pdli->pfnCur, target(pdli));
            ok(pdli->dwLastError == ERROR_PROC_NOT_FOUND,
                "Expected dwLastError to be ERROR_PROC_NOT_FOUND, was: %lu for %s\n", pdli->dwLastError, target(pdli));
            break;
    }

    return NULL;
}


LONG ExceptionFilter(IN PEXCEPTION_POINTERS ExceptionInfo, ULONG ExceptionCode)
{
    DWORD expected = VcppException(ERROR_SEVERITY_ERROR, (g_ExceptionIsModule ? ERROR_MOD_NOT_FOUND : ERROR_PROC_NOT_FOUND));
    ok(ExceptionCode == expected, "Expected code to be 0x%lx, was: 0x%lx\n", expected, ExceptionCode);
    ok(ExceptionInfo != NULL, "Expected to get exception info\n");
    ok(ExceptionInfo->ExceptionRecord != NULL, "Expected to get a valid record info\n");

    if (ExceptionCode != expected)
    {
        skip("Skipping other checks, this was not the exception we expected!\n");
        return EXCEPTION_EXECUTE_HANDLER;
    }

    if (ExceptionInfo && ExceptionInfo->ExceptionRecord)
    {
        PEXCEPTION_RECORD ExceptionRecord = ExceptionInfo->ExceptionRecord;
        ok(ExceptionRecord->ExceptionCode == expected, "Expected ExceptionCode to be 0x%lx, was 0x%lx\n",
            expected, ExceptionRecord->ExceptionCode);
        /* We can still continue. */
        ok(ExceptionRecord->ExceptionFlags == 0, "Expected ExceptionFlags to be 0, was: 0x%lx\n",
            ExceptionRecord->ExceptionFlags);
        ok(ExceptionRecord->NumberParameters == 1, "Expected 1 parameter, got %lu\n",
            ExceptionRecord->NumberParameters);
        if (ExceptionRecord->NumberParameters == 1)
        {
            PDelayLoadInfo LoadInfo = (PDelayLoadInfo)ExceptionRecord->ExceptionInformation[0];
            ok(LoadInfo && LoadInfo->cb >= 36, "Expected a valid pointer with a struct that is big enough: %p, %lu\n",
                LoadInfo, LoadInfo ? LoadInfo->cb : 0);

            if (g_ExpectedDll)
                ok(!strcmp(g_ExpectedDll, LoadInfo->szDll), "Expected szDll to be '%s', but was: '%s'\n",
                g_ExpectedDll, LoadInfo->szDll);
            if (g_ExpectedName)
            {
                ok(LoadInfo->dlp.fImportByName, "Expected import by name\n");
                if (LoadInfo->dlp.fImportByName)
                {
                    ok(!strcmp(g_ExpectedName, LoadInfo->dlp.szProcName),
                    "Expected szProcName to be '%s', but was: '%s'\n", g_ExpectedName, LoadInfo->dlp.szProcName);

                    if (g_ExceptionIsModule)
                    {
                        HMODULE mod = LoadLibraryA("imagehlp.dll");
                        LoadInfo->pfnCur = GetProcAddress(mod, g_ExpectedName);
                    }
                    else
                    {
                        char buf[100];
                        char first = isupper(g_ExpectedName[0]) ? tolower(g_ExpectedName[0]) : toupper(g_ExpectedName[0]);
                        sprintf(buf, "%c%s", first, g_ExpectedName + 1);
                        LoadInfo->pfnCur = GetProcAddress(GetModuleHandleA(g_ExpectedDll), buf);
                    }
                    return LoadInfo->pfnCur ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_EXECUTE_HANDLER;
                }
            }
        }
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

/* We register one hook the 'default' way and one manually,
so that we can check that both fallback and registration work*/
extern "C"
{
    extern PfnDliHook __pfnDliNotifyHook2;
    //PfnDliHook __pfnDliFailureHook2 = DliFailHook;
}


bool g_UsePointers = false;

template<typename PTR>
PTR Rva2Addr(PIMAGE_DOS_HEADER dos, RVA rva)
{
    /* Old delayload type */
    if (g_UsePointers)
        return reinterpret_cast<PTR>(rva);
    return reinterpret_cast<PTR>((reinterpret_cast<PBYTE>(dos) + rva));
}


unsigned g_winmm_get_cur_task[] = { dliStartProcessing, dliNotePreLoadLibrary, dliNotePreGetProcAddress, dliNoteEndProcessing, LAST_DLI };
unsigned g_winmm_midi_out_close[] = { dliStartProcessing, dliNotePreGetProcAddress, dliNoteEndProcessing, LAST_DLI };
unsigned g_winmm_mide_in_close[] = { dliStartProcessing, dliNotePreGetProcAddress, dliFailGetProc, dliNoteEndProcessing, LAST_DLI };
unsigned g_sfc_key[] = { dliStartProcessing, dliNotePreLoadLibrary, dliFailLoadLib, dliNotePreGetProcAddress, dliNoteEndProcessing, LAST_DLI };
unsigned g_sfc_file[] = { dliStartProcessing, dliNotePreGetProcAddress, dliNoteEndProcessing, LAST_DLI };
unsigned g_version_a[] = { dliStartProcessing, dliNotePreLoadLibrary, dliNotePreGetProcAddress, dliFailGetProc, dliNoteEndProcessing, LAST_DLI };
unsigned g_version_w[] = { dliStartProcessing, dliNotePreGetProcAddress, dliFailGetProc, dliNoteEndProcessing, LAST_DLI };
unsigned g_scard[] = { dliStartProcessing, dliNoteEndProcessing, LAST_DLI };
unsigned g_shlwapi[] = { dliStartProcessing, dliNotePreLoadLibrary, dliNotePreGetProcAddress, dliNoteEndProcessing, LAST_DLI };
unsigned g_imagehlp[] = { dliStartProcessing, dliNotePreLoadLibrary, dliFailLoadLib, LAST_DLI };    /* This exception does not fire EndProcessing! */


//#define DELAYLOAD_SUPPORTS_UNLOADING
START_TEST(delayimp)
{
    __pfnDliNotifyHook2 = DliHook;
    /* Verify that both scenario's work */
    ok(__pfnDliNotifyHook2 == DliHook, "Expected __pfnDliNotifyHook2 to be DliHook(%p), but was: %p\n",
        DliHook, __pfnDliNotifyHook2);
    ok(__pfnDliFailureHook2 == NULL, "Expected __pfnDliFailureHook2 to be NULL, but was: %p\n",
        __pfnDliFailureHook2);

    __pfnDliFailureHook2 = DliFailHook;


    PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)GetModuleHandle(NULL);

    ok(dos->e_magic == IMAGE_DOS_SIGNATURE, "Expected a DOS header\n");
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return;

    PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((PBYTE)dos + dos->e_lfanew);
    PIMAGE_DATA_DIRECTORY delaydir = nt->OptionalHeader.DataDirectory + IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT;

    /* Test some advanced features (loading / unloading) */
    if (delaydir->Size != 0)
    {
#if defined(_DELAY_IMP_VER) && _DELAY_IMP_VER == 2 && defined(DELAYLOAD_SUPPORTS_UNLOADING)
        /* First, before mangling the delayload stuff, let's try some v2 functions */
        HMODULE mod = GetModuleHandleA(WINMM_DLLNAME);
        ok(mod == NULL, "Expected mod to be NULL, was %p\n", mod);
        /* Now, a mistyped module (case sensitive!) */
        HRESULT hr = __HrLoadAllImportsForDll("WiNmM.DlL");
        ok(hr == HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND), "Expected hr to be HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND), was %lu\n", hr);
        mod = GetModuleHandleA(WINMM_DLLNAME);
        ok(mod == NULL, "Expected mod to be NULL, was %p\n", mod);

        /* Let's load it */
        hr = __HrLoadAllImportsForDll(WINMM_DLLNAME);
        ok(hr == S_OK, "Expected hr to be S_OK, was %lu\n", hr);
        mod = GetModuleHandleA(WINMM_DLLNAME);
        ok(mod != NULL, "Expected mod to be valid, was NULL\n");

        BOOL status = __FUnloadDelayLoadedDLL2(WINMM_DLLNAME);
        ok(status == TRUE, "Expected __FUnloadDelayLoadedDLL2 to succeed\n");
        mod = GetModuleHandleA(WINMM_DLLNAME);
        ok(mod == NULL, "Expected mod to be NULL, was %p\n", mod);
#else
        trace("Binary compiled without support for unloading\n");
#endif
    }
    else
    {
        skip("No IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT found, some advanced features might not work!\n");
    }

    /* Test the normal flow without a dll loaded */
    SetExpectedDli(g_winmm_get_cur_task);
    g_ExpectedDll = WINMM_DLLNAME;
    g_ExpectedName = "mmGetCurrentTask";
    DWORD task = mmGetCurrentTask();
    ok(task == GetCurrentThreadId(), "Expected ret to be current thread id (0x%lx), was 0x%lx\n", GetCurrentThreadId(), task);
    CheckDliDone();

    /* Test the normal flow with a dll loaded */
    SetExpectedDli(g_winmm_midi_out_close);
    g_ExpectedDll = WINMM_DLLNAME;
    g_ExpectedName = "midiOutClose";
    DWORD err = midiOutClose((HMIDIOUT)(ULONG_PTR)0xdeadbeef);
    ok(err == MMSYSERR_INVALHANDLE, "Expected err to be MMSYSERR_INVALHANDLE, was 0x%lx\n", err);
    CheckDliDone();

    /* Make sure GetProcAddress fails, also ignore the Failure hook, use the exception to set the address */
    SetExpectedDli(g_winmm_mide_in_close);
    g_ExpectedDll = WINMM_DLLNAME;
    g_ExpectedName = "MixerClose";
    g_BreakFunctionName = true;
    _SEH2_TRY
    {
        err = mixerClose((HMIXER)(ULONG_PTR)0xdeadbeef);
    }
    _SEH2_EXCEPT(ExceptionFilter(_SEH2_GetExceptionInformation(), _SEH2_GetExceptionCode()))
    {
        err = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    ok(err == MMSYSERR_INVALHANDLE, "Expected err to be MMSYSERR_INVALHANDLE, was 0x%lx\n", err);
    CheckDliDone();
    ok(g_BreakFunctionName == false, "Expected the functionname to be changed\n");

    /* Make the LoadLib fail, manually load the library in the Failure Hook,
    Respond to the dliNotePreGetProcAddress with an alternate function address */
    SetExpectedDli(g_sfc_key);
    BOOL ret = SfcIsKeyProtected(NULL, NULL, NULL);
    ok(ret == 12345, "Expected ret to be 12345, was %u\n", ret);    /* The original function returns FALSE! */
    CheckDliDone();

    /* Show that it works with the manually returned dll */
    SetExpectedDli(g_sfc_file);
    ret = SfcGetNextProtectedFile(NULL, NULL);
    ok(ret == FALSE, "Expected ret to be FALSE, was %u\n", ret);
    CheckDliDone();

    /* Return a fake dll handle, so that we can see when it is being used, and manually return a function in the Failure Hook */
    SetExpectedDli(g_version_a);
    ret = GetFileVersionInfoA(NULL, NULL, NULL, NULL);
    ok(ret == FALSE, "Expected ret to be FALSE, was %u\n", ret);
    CheckDliDone();

    /* Manually return a function in the failure hook, when the module is the previously set bad one */
    SetExpectedDli(g_version_w);
    ret = GetFileVersionInfoW(NULL, NULL, NULL, NULL);
    ok(ret == FALSE, "Expected ret to be FALSE, was %u\n", ret);
    CheckDliDone();

    if (HIWORD(SymGetOptions) == NULL)
    {
        skip("SymGetOptions until CORE-6504 is fixed\n");
    }
    else
    {
        /* Completely bypass most hooks, by directly replying with a function address */
        SetExpectedDli(g_scard);
        g_BypassMost = true;
        DWORD opt = SymGetOptions();
        g_BypassMost = false;
        ok(opt == 123, "Expected opt to be 123, was %lu\n", opt);    /* The original function returns ERROR_INVALID_HANDLE */
        CheckDliDone();
    }

    /* Import by ordinal */
    g_ImportByName = false;
    SetExpectedDli(g_shlwapi);
    PARSEDURLA pua = { sizeof(pua), 0 };
    HRESULT hr = ParseURLA("", &pua);
    ok(hr == URL_E_INVALID_SYNTAX, "Expected tmp to be URL_E_INVALID_SYNTAX, was %lx\n", hr);
    CheckDliDone();
    g_ImportByName = true;

    /* Handle LoadLib failure with an exception handler */
    if (HIWORD(MapAndLoad) == NULL)
    {
        skip("MapAndLoad until CORE-6504 is fixed\n");
    }
    else
    {
        SetExpectedDli(g_imagehlp);
        LOADED_IMAGE img = {0};
        ret = 123;
        g_ExceptionIsModule = true;
        g_ExpectedDll = "xxxxxhlp.dll";
        g_ExpectedName = "MapAndLoad";
        _SEH2_TRY
        {
            ret = MapAndLoad("some_not_existing_file.aabbcc", NULL, &img, FALSE, TRUE);
        }
        _SEH2_EXCEPT(ExceptionFilter(_SEH2_GetExceptionInformation(), _SEH2_GetExceptionCode()))
        {
            ;
        }
        _SEH2_END;
        g_ExceptionIsModule = false;
        ok(ret == FALSE, "Expected ret to be FALSE, was %u\n", ret);
        CheckDliDone();
    }
}
