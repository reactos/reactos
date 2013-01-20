/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/tuiconsole.c
 * PURPOSE:         Interface to text-mode consoles
 * PROGRAMMERS:
 */

#include "consrv.h"
#include "tuiconsole.h"
#include <drivers/blue/ntddblue.h>

#define NDEBUG
#include <debug.h>


/* TUI Console Window Class name */
#define TUI_CONSOLE_WINDOW_CLASS L"TuiConsoleWindowClass"


CRITICAL_SECTION ActiveConsoleLock;
static COORD PhysicalConsoleSize;
static HANDLE ConsoleDeviceHandle;
static PCONSOLE ActiveConsole;

static BOOL ConsInitialized = FALSE;

/******************************************************************************\
|** BlueScreen Driver management                                             **|
\**/
/* Code taken and adapted from base/system/services/driver.c */
static DWORD EnablePrivilege(LPCWSTR lpszPrivilegeName, BOOL bEnablePrivilege)
{
    DWORD  dwRet  = ERROR_SUCCESS;
    HANDLE hToken = NULL;

    if (OpenProcessToken(GetCurrentProcess(),
                         TOKEN_ADJUST_PRIVILEGES,
                         &hToken))
    {
        TOKEN_PRIVILEGES tp;

        tp.PrivilegeCount = 1;
        tp.Privileges[0].Attributes = (bEnablePrivilege ? SE_PRIVILEGE_ENABLED : 0);

        if (LookupPrivilegeValueW(NULL,
                                  lpszPrivilegeName,
                                  &tp.Privileges[0].Luid))
        {
            if (AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL))
            {
                if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
                    dwRet = ERROR_NOT_ALL_ASSIGNED;
            }
            else
            {
                dwRet = GetLastError();
            }
        }
        else
        {
            dwRet = GetLastError();
        }

        CloseHandle(hToken);
    }
    else
    {
        dwRet = GetLastError();
    }

    return dwRet;
}

static DWORD
ScmLoadDriver(LPCWSTR lpServiceName)
{
    PWSTR pszDriverPath;
    UNICODE_STRING DriverPath;
    NTSTATUS Status;
    DWORD dwError = ERROR_SUCCESS;

    /* Build the driver path */
    /* 52 = wcslen(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\") */
    pszDriverPath = RtlAllocateHeap(ConSrvHeap,
                              HEAP_ZERO_MEMORY,
                              (52 + wcslen(lpServiceName) + 1) * sizeof(WCHAR));
    if (pszDriverPath == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    wcscpy(pszDriverPath,
           L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
    wcscat(pszDriverPath,
           lpServiceName);

    RtlInitUnicodeString(&DriverPath,
                         pszDriverPath);

    DPRINT("  Path: %wZ\n", &DriverPath);

    /* Acquire driver-loading privilege */
    dwError = EnablePrivilege(SE_LOAD_DRIVER_NAME, TRUE);
    if (dwError != ERROR_SUCCESS)
    {
        /* We encountered a failure, exit properly */
        DPRINT1("CONSRV: Cannot acquire driver-loading privilege, error = %lu\n", dwError);
        goto done;
    }

    Status = NtLoadDriver(&DriverPath);

    /* Release driver-loading privilege */
    EnablePrivilege(SE_LOAD_DRIVER_NAME, FALSE);

    if (!NT_SUCCESS(Status))
    {
        dwError = RtlNtStatusToDosError(Status);
    }

done:
    RtlFreeHeap(ConSrvHeap, 0, pszDriverPath);

    return dwError;
}

#ifdef BLUESCREEN_DRIVER_UNLOADING
static DWORD
ScmUnloadDriver(LPCWSTR lpServiceName)
{
    PWSTR pszDriverPath;
    UNICODE_STRING DriverPath;
    NTSTATUS Status;
    DWORD dwError = ERROR_SUCCESS;

    /* Build the driver path */
    /* 52 = wcslen(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\") */
    pszDriverPath = RtlAllocateHeap(ConSrvHeap,
                              HEAP_ZERO_MEMORY,
                              (52 + wcslen(lpServiceName) + 1) * sizeof(WCHAR));
    if (pszDriverPath == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    wcscpy(pszDriverPath,
           L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
    wcscat(pszDriverPath,
           lpServiceName);

    RtlInitUnicodeString(&DriverPath,
                         pszDriverPath);

    /* Acquire driver-unloading privilege */
    dwError = EnablePrivilege(SE_LOAD_DRIVER_NAME, TRUE);
    if (dwError != ERROR_SUCCESS)
    {
        /* We encountered a failure, exit properly */
        DPRINT1("CONSRV: Cannot acquire driver-unloading privilege, error = %lu\n", dwError);
        goto done;
    }

    Status = NtUnloadDriver(&DriverPath);

    /* Release driver-unloading privilege */
    EnablePrivilege(SE_LOAD_DRIVER_NAME, FALSE);

    if (!NT_SUCCESS(Status))
    {
        dwError = RtlNtStatusToDosError(Status);
    }

done:
    RtlFreeHeap(ConSrvHeap, 0, pszDriverPath);

    return dwError;
}
#endif
/**\
\******************************************************************************/

static LRESULT CALLBACK
TuiConsoleWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_ACTIVATE)
    {
        if (LOWORD(wParam) != WA_INACTIVE)
        {
            SetFocus(hWnd);
            ConioDrawConsole(ActiveConsole);
        }
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static BOOL FASTCALL
TuiInit(DWORD OemCP)
{
    CONSOLE_SCREEN_BUFFER_INFO ScrInfo;
    DWORD BytesReturned;
    WNDCLASSEXW wc;
    ATOM ConsoleClassAtom;
    USHORT TextAttribute = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;

    ScmLoadDriver(L"Blue");

    ConsoleDeviceHandle = CreateFileW(L"\\\\.\\BlueScreen", FILE_ALL_ACCESS, 0, NULL,
                                      OPEN_EXISTING, 0, NULL);
    if (INVALID_HANDLE_VALUE == ConsoleDeviceHandle)
    {
        DPRINT1("Failed to open BlueScreen.\n");
        return FALSE;
    }

    if (!DeviceIoControl(ConsoleDeviceHandle, IOCTL_CONSOLE_LOADFONT,
                         &OemCP, sizeof(OemCP), NULL, 0,
                         &BytesReturned, NULL))
    {
        DPRINT1("Failed to load the font for codepage %d\n", OemCP);
        /* Let's suppose the font is good enough to continue */
    }

    if (!DeviceIoControl(ConsoleDeviceHandle, IOCTL_CONSOLE_SET_TEXT_ATTRIBUTE,
                         &TextAttribute, sizeof(TextAttribute), NULL, 0,
                         &BytesReturned, NULL))
    {
        DPRINT1("Failed to set text attribute\n");
    }

    ActiveConsole = NULL;
    InitializeCriticalSection(&ActiveConsoleLock);
    if (!DeviceIoControl(ConsoleDeviceHandle, IOCTL_CONSOLE_GET_SCREEN_BUFFER_INFO,
                         NULL, 0, &ScrInfo, sizeof(ScrInfo), &BytesReturned, NULL))
    {
        DPRINT1("Failed to get console info\n");
        return FALSE;
    }
    PhysicalConsoleSize = ScrInfo.dwSize;

    RtlZeroMemory(&wc, sizeof(WNDCLASSEXW));
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpszClassName = TUI_CONSOLE_WINDOW_CLASS;
    wc.lpfnWndProc = TuiConsoleWndProc;
    wc.cbWndExtra = GWLP_CONSOLEWND_ALLOC;
    wc.hInstance = (HINSTANCE)GetModuleHandleW(NULL);

    ConsoleClassAtom = RegisterClassExW(&wc);
    if (ConsoleClassAtom == 0)
    {
        DPRINT1("Failed to register TUI console wndproc\n");
        return FALSE;
    }
    else
    {
        NtUserConsoleControl(TuiConsoleWndClassAtom, &ConsoleClassAtom, sizeof(ATOM));
    }

    return TRUE;
}

static VOID WINAPI
TuiInitScreenBuffer(PCONSOLE Console, PCONSOLE_SCREEN_BUFFER Buffer)
{
    Buffer->DefaultAttrib = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
}

static void FASTCALL
TuiCopyRect(char *Dest, PCONSOLE_SCREEN_BUFFER Buff, SMALL_RECT* Region)
{
    UINT SrcDelta, DestDelta;
    LONG i;
    PBYTE Src, SrcEnd;

    Src = ConioCoordToPointer(Buff, Region->Left, Region->Top);
    SrcDelta = Buff->MaxX * 2;
    SrcEnd = Buff->Buffer + Buff->MaxY * Buff->MaxX * 2;
    DestDelta = ConioRectWidth(Region) * 2;
    for (i = Region->Top; i <= Region->Bottom; i++)
    {
        memcpy(Dest, Src, DestDelta);
        Src += SrcDelta;
        if (SrcEnd <= Src)
        {
            Src -= Buff->MaxY * Buff->MaxX * 2;
        }
        Dest += DestDelta;
    }
}

static VOID WINAPI
TuiDrawRegion(PCONSOLE Console, SMALL_RECT* Region)
{
    DWORD BytesReturned;
    PCONSOLE_SCREEN_BUFFER Buff = Console->ActiveBuffer;
    PCONSOLE_DRAW ConsoleDraw;
    UINT ConsoleDrawSize;

    if (ActiveConsole != Console)
    {
        return;
    }

    ConsoleDrawSize = sizeof(CONSOLE_DRAW) +
                      (ConioRectWidth(Region) * ConioRectHeight(Region)) * 2;
    ConsoleDraw = RtlAllocateHeap(ConSrvHeap, 0, ConsoleDrawSize);
    if (NULL == ConsoleDraw)
    {
        DPRINT1("RtlAllocateHeap failed\n");
        return;
    }
    ConsoleDraw->X = Region->Left;
    ConsoleDraw->Y = Region->Top;
    ConsoleDraw->SizeX = ConioRectWidth(Region);
    ConsoleDraw->SizeY = ConioRectHeight(Region);
    ConsoleDraw->CursorX = Buff->CurrentX;
    ConsoleDraw->CursorY = Buff->CurrentY;

    TuiCopyRect((char *) (ConsoleDraw + 1), Buff, Region);

    if (!DeviceIoControl(ConsoleDeviceHandle, IOCTL_CONSOLE_DRAW,
                         NULL, 0, ConsoleDraw, ConsoleDrawSize, &BytesReturned, NULL))
    {
        DPRINT1("Failed to draw console\n");
        RtlFreeHeap(ConSrvHeap, 0, ConsoleDraw);
        return;
    }

    RtlFreeHeap(ConSrvHeap, 0, ConsoleDraw);
}

static VOID WINAPI
TuiWriteStream(PCONSOLE Console, SMALL_RECT* Region, LONG CursorStartX, LONG CursorStartY,
               UINT ScrolledLines, CHAR *Buffer, UINT Length)
{
    DWORD BytesWritten;
    PCONSOLE_SCREEN_BUFFER Buff = Console->ActiveBuffer;

    if (ActiveConsole->ActiveBuffer != Buff)
    {
        return;
    }

    if (! WriteFile(ConsoleDeviceHandle, Buffer, Length, &BytesWritten, NULL))
    {
        DPRINT1("Error writing to BlueScreen\n");
    }
}

static BOOL WINAPI
TuiSetCursorInfo(PCONSOLE Console, PCONSOLE_SCREEN_BUFFER Buff)
{
    CONSOLE_CURSOR_INFO Info;
    DWORD BytesReturned;

    if (ActiveConsole->ActiveBuffer != Buff)
    {
        return TRUE;
    }

    Info.dwSize = ConioEffectiveCursorSize(Console, 100);
    Info.bVisible = Buff->CursorInfo.bVisible;

    if (!DeviceIoControl(ConsoleDeviceHandle, IOCTL_CONSOLE_SET_CURSOR_INFO,
                         &Info, sizeof(Info), NULL, 0, &BytesReturned, NULL))
    {
        DPRINT1( "Failed to set cursor info\n" );
        return FALSE;
    }

    return TRUE;
}

static BOOL WINAPI
TuiSetScreenInfo(PCONSOLE Console, PCONSOLE_SCREEN_BUFFER Buff, UINT OldCursorX, UINT OldCursorY)
{
    CONSOLE_SCREEN_BUFFER_INFO Info;
    DWORD BytesReturned;

    if (ActiveConsole->ActiveBuffer != Buff)
    {
        return TRUE;
    }

    Info.dwCursorPosition.X = Buff->CurrentX;
    Info.dwCursorPosition.Y = Buff->CurrentY;
    Info.wAttributes = Buff->DefaultAttrib;

    if (!DeviceIoControl(ConsoleDeviceHandle, IOCTL_CONSOLE_SET_SCREEN_BUFFER_INFO,
                         &Info, sizeof(CONSOLE_SCREEN_BUFFER_INFO), NULL, 0,
                         &BytesReturned, NULL))
    {
        DPRINT1( "Failed to set cursor position\n" );
        return FALSE;
    }

    return TRUE;
}

static BOOL WINAPI
TuiUpdateScreenInfo(PCONSOLE Console, PCONSOLE_SCREEN_BUFFER Buff)
{
    return TRUE;
}

static BOOL WINAPI
TuiChangeTitle(PCONSOLE Console)
{
    return TRUE;
}

static VOID WINAPI
TuiCleanupConsole(PCONSOLE Console)
{
    DestroyWindow(Console->hWindow);

    EnterCriticalSection(&ActiveConsoleLock);

    /* Switch to next console */
    if (ActiveConsole == Console)
    {
        ActiveConsole = Console->Next != Console ? Console->Next : NULL;
    }

    if (Console->Next != Console)
    {
        Console->Prev->Next = Console->Next;
        Console->Next->Prev = Console->Prev;
    }
    LeaveCriticalSection(&ActiveConsoleLock);

    if (NULL != ActiveConsole)
    {
        ConioDrawConsole(ActiveConsole);
    }
}

static BOOL WINAPI
TuiChangeIcon(PCONSOLE Console, HICON hWindowIcon)
{
    return TRUE;
}

static NTSTATUS WINAPI
TuiResizeBuffer(PCONSOLE Console, PCONSOLE_SCREEN_BUFFER ScreenBuffer, COORD Size)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

DWORD WINAPI
TuiConsoleThread(PVOID Data)
{
    PCONSOLE Console = (PCONSOLE) Data;
    HWND NewWindow;
    MSG msg;

    NewWindow = CreateWindowW(TUI_CONSOLE_WINDOW_CLASS,
                              Console->Title.Buffer,
                              0,
                              -32000, -32000, 0, 0,
                              NULL, NULL,
                              (HINSTANCE)GetModuleHandleW(NULL),
                              (PVOID)Console);
    if (NULL == NewWindow)
    {
        DPRINT1("CSR: Unable to create console window\n");
        return 1;
    }
    Console->hWindow = NewWindow;
    SetConsoleWndConsoleLeaderCID(Console);

    SetForegroundWindow(Console->hWindow);

    while (TRUE)
    {
        GetMessageW(&msg, 0, 0, 0);
        DispatchMessage(&msg);
        TranslateMessage(&msg);

        if (msg.message == WM_CHAR || msg.message == WM_SYSCHAR ||
                msg.message == WM_KEYDOWN || msg.message == WM_KEYUP ||
                msg.message == WM_SYSKEYDOWN || msg.message == WM_SYSKEYUP)
        {
            ConioProcessKey(&msg, Console, TRUE);
        }
    }

    return 0;
}

static CONSOLE_VTBL TuiVtbl =
{
    TuiInitScreenBuffer,
    TuiWriteStream,
    TuiDrawRegion,
    TuiSetCursorInfo,
    TuiSetScreenInfo,
    TuiUpdateScreenInfo,
    TuiChangeTitle,
    TuiCleanupConsole,
    TuiChangeIcon,
    TuiResizeBuffer,
};

NTSTATUS FASTCALL
TuiInitConsole(PCONSOLE Console)
{
    HANDLE ThreadHandle;

    if (!ConsInitialized)
    {
        ConsInitialized = TRUE;
        if (!TuiInit(Console->CodePage))
        {
            ConsInitialized = FALSE;
            return STATUS_UNSUCCESSFUL;
        }
    }

    Console->Vtbl = &TuiVtbl;
    Console->hWindow = NULL;
    Console->Size = PhysicalConsoleSize;
    Console->ActiveBuffer->MaxX = PhysicalConsoleSize.X;
    Console->ActiveBuffer->MaxY = PhysicalConsoleSize.Y;

    ThreadHandle = CreateThread(NULL,
                                0,
                                TuiConsoleThread,
                                (PVOID)Console,
                                0,
                                NULL);
    if (NULL == ThreadHandle)
    {
        DPRINT1("CSR: Unable to create console thread\n");
        return STATUS_UNSUCCESSFUL;
    }
    CloseHandle(ThreadHandle);

    EnterCriticalSection(&ActiveConsoleLock);
    if (NULL != ActiveConsole)
    {
        Console->Prev = ActiveConsole;
        Console->Next = ActiveConsole->Next;
        ActiveConsole->Next->Prev = Console;
        ActiveConsole->Next = Console;
    }
    else
    {
        Console->Prev = Console;
        Console->Next = Console;
    }
    ActiveConsole = Console;
    LeaveCriticalSection(&ActiveConsoleLock);

    return STATUS_SUCCESS;
}

PCONSOLE FASTCALL
TuiGetFocusConsole(VOID)
{
    return ActiveConsole;
}

BOOL FASTCALL
TuiSwapConsole(INT Next)
{
    static PCONSOLE SwapConsole = NULL; /* console we are thinking about swapping with */
    DWORD BytesReturned;
    ANSI_STRING Title;
    PVOID Buffer;
    PCOORD pos;

    if (0 != Next)
    {
        /* alt-tab, swap consoles */
        /* move SwapConsole to next console, and print its title */
        EnterCriticalSection(&ActiveConsoleLock);
        if (! SwapConsole)
        {
            SwapConsole = ActiveConsole;
        }

        SwapConsole = (0 < Next ? SwapConsole->Next : SwapConsole->Prev);
        Title.MaximumLength = RtlUnicodeStringToAnsiSize(&SwapConsole->Title);
        Title.Length = 0;
        Buffer = RtlAllocateHeap(ConSrvHeap,
                           0,
                           sizeof(COORD) + Title.MaximumLength);
        pos = (PCOORD )Buffer;
        Title.Buffer = (PVOID)((ULONG_PTR)Buffer + sizeof( COORD ));

        RtlUnicodeStringToAnsiString(&Title, &SwapConsole->Title, FALSE);
        pos->Y = PhysicalConsoleSize.Y / 2;
        pos->X = (PhysicalConsoleSize.X - Title.Length) / 2;
        /* redraw the console to clear off old title */
        ConioDrawConsole(ActiveConsole);
        if (!DeviceIoControl(ConsoleDeviceHandle, IOCTL_CONSOLE_WRITE_OUTPUT_CHARACTER,
                             NULL, 0, Buffer, sizeof(COORD) + Title.Length,
                             &BytesReturned, NULL))
        {
            DPRINT1( "Error writing to console\n" );
        }
        RtlFreeHeap(ConSrvHeap, 0, Buffer);
        LeaveCriticalSection(&ActiveConsoleLock);

        return TRUE;
    }
    else if (NULL != SwapConsole)
    {
        EnterCriticalSection(&ActiveConsoleLock);
        if (SwapConsole != ActiveConsole)
        {
            /* first remove swapconsole from the list */
            SwapConsole->Prev->Next = SwapConsole->Next;
            SwapConsole->Next->Prev = SwapConsole->Prev;
            /* now insert before activeconsole */
            SwapConsole->Next = ActiveConsole;
            SwapConsole->Prev = ActiveConsole->Prev;
            ActiveConsole->Prev->Next = SwapConsole;
            ActiveConsole->Prev = SwapConsole;
        }
        ActiveConsole = SwapConsole;
        SwapConsole = NULL;
        ConioDrawConsole(ActiveConsole);
        LeaveCriticalSection(&ActiveConsoleLock);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/* EOF */
