/*
 * PROJECT:     Dr. Watson crash reporter
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Entrypoint / main print function
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"
#include <winuser.h>
#include <algorithm>
#include <shlobj.h>
#include <shlwapi.h>
#include <tchar.h>
#include <strsafe.h>
#include <tlhelp32.h>
#include <dbghelp.h>
#include <conio.h>
#include <atlbase.h>
#include <atlstr.h>
#include "resource.h"


static const char szUsage[] = "Usage: DrWtsn32 [-i] [-g] [-p dddd] [-e dddd] [-?]\n"
                              "    -i: Install DrWtsn32 as the postmortem debugger\n"
                              "    -g: Ignored, Provided for compatibility with WinDbg and CDB.\n"
                              "    -p dddd: Attach to process dddd.\n"
                              "    -e dddd: Signal the event dddd.\n"
                              "    -?: This help.\n";

extern "C"
NTSYSAPI ULONG NTAPI vDbgPrintEx(_In_ ULONG ComponentId, _In_ ULONG Level, _In_z_ PCCH Format, _In_ va_list ap);
#define DPFLTR_ERROR_LEVEL 0

void xfprintf(FILE* stream, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stream, fmt, ap);
    vDbgPrintEx(-1, DPFLTR_ERROR_LEVEL, fmt, ap);
    va_end(ap);
}



static bool SortModules(const ModuleData& left, const ModuleData& right)
{
    return left.BaseAddress < right.BaseAddress;
}


void PrintBugreport(FILE* output, DumpData& data)
{
    PrintSystemInfo(output, data);
    xfprintf(output, NEWLINE "*----> Task List <----*" NEWLINE NEWLINE);
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(pe);
        if (Process32First(hSnap, &pe))
        {
            do
            {
                xfprintf(output, "%5d: %ls" NEWLINE, pe.th32ProcessID, pe.szExeFile);
            } while (Process32Next(hSnap, &pe));
        }
        CloseHandle(hSnap);
    }

    xfprintf(output, NEWLINE "*----> Module List <----*" NEWLINE NEWLINE);
    std::sort(data.Modules.begin(), data.Modules.end(), SortModules);

    ModuleData mainModule(NULL);
    mainModule.Update(data.ProcessHandle);
    xfprintf(output, "(%p - %p) %ls" NEWLINE,
             mainModule.BaseAddress,
             (PBYTE)mainModule.BaseAddress + mainModule.Size,
             data.ProcessPath.c_str());

    for (size_t n = 0; n < data.Modules.size(); ++n)
    {
        ModuleData& mod = data.Modules[n];
        if (!mod.Unloaded)
        {
            mod.Update(data.ProcessHandle);
            xfprintf(output, "(%p - %p) %s" NEWLINE,
                     mod.BaseAddress,
                     (PBYTE)mod.BaseAddress + mod.Size,
                     mod.ModuleName.c_str());
        }
    }

    BeginStackBacktrace(data);
    for (ThreadMap::iterator it = data.Threads.begin(); it != data.Threads.end(); ++it)
    {
        it->second.Update();

        xfprintf(output, NEWLINE "State Dump for Thread Id 0x%x" NEWLINE NEWLINE, it->first);
        const CONTEXT& ctx = it->second.Context;
        if ((ctx.ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER)
        {
#if defined(_M_IX86)
            xfprintf(output, "eax:%p ebx:%p ecx:%p edx:%p esi:%p edi:%p" NEWLINE,
                     ctx.Eax, ctx.Ebx, ctx.Ecx, ctx.Edx, ctx.Esi, ctx.Edi);
#elif defined(_M_AMD64)
            xfprintf(output, "rax:%p rbx:%p rcx:%p rdx:%p rsi:%p rdi:%p" NEWLINE,
                     ctx.Rax, ctx.Rbx, ctx.Rcx, ctx.Rdx, ctx.Rsi, ctx.Rdi);
            xfprintf(output, "r8:%p r9:%p r10:%p r11:%p r12:%p r13:%p r14:%p r15:%p" NEWLINE,
                     ctx.R8, ctx.R9, ctx.R10, ctx.R11, ctx.R12, ctx.R13, ctx.R14, ctx.R15);
#else
#error Unknown architecture
#endif
        }
        if ((ctx.ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL)
        {
#if defined(_M_IX86)
            xfprintf(output, "eip:%p esp:%p ebp:%p" NEWLINE,
                     ctx.Eip, ctx.Esp, ctx.Ebp);
#elif defined(_M_AMD64)
            xfprintf(output, "rip:%p rsp:%p rbp:%p" NEWLINE,
                     ctx.Rip, ctx.Rsp, ctx.Rbp);
#else
#error Unknown architecture
#endif
        }
        if ((ctx.ContextFlags & CONTEXT_DEBUG_REGISTERS) == CONTEXT_DEBUG_REGISTERS)
        {
#if defined(_M_IX86) || defined(_M_AMD64)
            xfprintf(output, "dr0:%p dr1:%p dr2:%p dr3:%p dr6:%p dr7:%p" NEWLINE,
                     ctx.Dr0, ctx.Dr1, ctx.Dr2, ctx.Dr3, ctx.Dr6, ctx.Dr7);
#else
#error Unknown architecture
#endif
        }

        PrintStackBacktrace(output, data, it->second);
    }
    EndStackBacktrace(data);
}


int abort(FILE* output, int err)
{
    if (output != stdout)
        fclose(output);
    else
        _getch();

    return err;
}

std::wstring Settings_GetOutputPath(void)
{
    WCHAR Buffer[MAX_PATH] = L"";
    ULONG BufferSize = _countof(Buffer);
    BOOL UseDefaultPath = FALSE;

    CRegKey key;
    if (key.Open(HKEY_CURRENT_USER, L"SOFTWARE\\ReactOS\\Crash Reporter", KEY_READ) != ERROR_SUCCESS)
    {
        UseDefaultPath = TRUE;
    }

    if (key.QueryStringValue(L"Dump Directory", Buffer, &BufferSize) != ERROR_SUCCESS)
    {
        UseDefaultPath = TRUE;
    }

    if (UseDefaultPath)
    {
        if (FAILED(SHGetFolderPathW(NULL, CSIDL_DESKTOP, NULL, SHGFP_TYPE_CURRENT, Buffer)))
        {
            return std::wstring();
        }
    }

    return std::wstring(Buffer);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR cmdLine, INT)
{
    int argc;
    WCHAR **argv = CommandLineToArgvW(cmdLine, &argc);

    DWORD pid = 0;
    WCHAR Filename[50];
    FILE* output = NULL;
    SYSTEMTIME st;
    DumpData data;


    for (int n = 0; n < argc; ++n)
    {
        WCHAR* arg = argv[n];

        if (!wcscmp(arg, L"-i"))
        {
            /* FIXME: Installs as the postmortem debugger. */
        }
        else if (!wcscmp(arg, L"-g"))
        {
        }
        else if (!wcscmp(arg, L"-p"))
        {
            if (n + 1 < argc)
            {
                pid = wcstoul(argv[n+1], NULL, 10);
                n++;
            }
        }
        else if (!wcscmp(arg, L"-e"))
        {
            if (n + 1 < argc)
            {
                data.Event = (HANDLE)wcstoul(argv[n+1], NULL, 10);
                n++;
            }
        }
        else if (!wcscmp(arg, L"-?"))
        {
            MessageBoxA(NULL, szUsage, "ReactOS Crash Reporter", MB_OK);
            return abort(output, 0);
        }
        else if (!wcscmp(arg, L"/?"))
        {
            xfprintf(stdout, "%s\n", szUsage);
            return abort(stdout, 0);
        }
    }

    if (!pid)
    {
        MessageBoxA(NULL, szUsage, "ReactOS Crash Reporter", MB_OK);
        return abort(stdout, 0);
    }

    GetLocalTime(&st);

    std::wstring OutputPath = Settings_GetOutputPath();
    BOOL HasPath = (OutputPath.size() != 0);

    if (!PathIsDirectoryW(OutputPath.c_str()))
    {
        int res = SHCreateDirectoryExW(NULL, OutputPath.c_str(), NULL);
        if (res != ERROR_SUCCESS && res != ERROR_ALREADY_EXISTS)
        {
            xfprintf(stdout, "Could not create output directory, not writing dump\n");
            MessageBoxA(NULL, "Could not create directory to write crash report.", "ReactOS Crash Reporter", MB_ICONERROR | MB_OK);
            return abort(stdout, 0);
        }
    }

    if (HasPath &&
        SUCCEEDED(StringCchPrintfW(Filename, _countof(Filename), L"Appcrash_%d-%02d-%02d_%02d-%02d-%02d.txt",
                                   st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond)))
    {
        OutputPath += L"\\";
        OutputPath += Filename;
        output = _wfopen(OutputPath.c_str(), L"wb");
    }
    if (!output)
        output = stdout;


    if (!DebugActiveProcess(pid))
        return abort(output, -2);

    /* We should not kill it? */
    DebugSetProcessKillOnExit(FALSE);

    DEBUG_EVENT evt;
    if (!WaitForDebugEvent(&evt, 30000))
        return abort(output, -3);

    assert(evt.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT);

    while (UpdateFromEvent(evt, data))
    {
        ContinueDebugEvent(evt.dwProcessId, evt.dwThreadId, DBG_CONTINUE);

        if (!WaitForDebugEvent(&evt, 30000))
            return abort(output, -4);
    }

    PrintBugreport(output, data);

    TerminateProcess(data.ProcessHandle, data.ExceptionInfo.ExceptionRecord.ExceptionCode);

    CStringW FormattedMessage;
    FormattedMessage.Format(IDS_USER_ALERT_MESSAGE, data.ProcessName.c_str(), OutputPath.c_str());
    CStringW DialogTitle;
    DialogTitle.LoadString(hInstance, IDS_APP_TITLE);

    MessageBoxW(NULL, FormattedMessage.GetString(), DialogTitle.GetString(), MB_OK);

    return abort(output, 0);
}
