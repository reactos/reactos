/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/frontends/tui/tuiterm.c
 * PURPOSE:         TUI Terminal Front-End - Virtual Consoles...
 * PROGRAMMERS:     David Welch
 *                  Gé van Geldorp
 *                  Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#include "consrv.h"
#include "include/conio.h"
#include "include/console.h"
#include "include/settings.h"
#include "tuiterm.h"
#include <drivers/blue/ntddblue.h>

#define NDEBUG
#include <debug.h>


/* GLOBALS ********************************************************************/

#define GetNextConsole(Console) \
    CONTAINING_RECORD(Console->Entry.Flink, TUI_CONSOLE_DATA, Entry)

#define GetPrevConsole(Console) \
    CONTAINING_RECORD(Console->Entry.Blink, TUI_CONSOLE_DATA, Entry)


/* TUI Console Window Class name */
#define TUI_CONSOLE_WINDOW_CLASS L"TuiConsoleWindowClass"

typedef struct _TUI_CONSOLE_DATA
{
    CRITICAL_SECTION Lock;
    LIST_ENTRY Entry;           /* Entry in the list of virtual consoles */
    // HANDLE hTuiInitEvent;

    HWND hWindow;               /* Handle to the console's window (used for the window's procedure */

    PCONSOLE Console;           /* Pointer to the owned console */
    // TUI_CONSOLE_INFO TuiInfo;   /* TUI terminal settings */
} TUI_CONSOLE_DATA, *PTUI_CONSOLE_DATA;

/* List of the maintained virtual consoles and its lock */
static LIST_ENTRY VirtConsList;
static PTUI_CONSOLE_DATA ActiveConsole; /* The active console on screen */
static CRITICAL_SECTION ActiveVirtConsLock;

static COORD PhysicalConsoleSize;
static HANDLE ConsoleDeviceHandle;

static BOOL ConsInitialized = FALSE;

/******************************************************************************\
|** BlueScreen Driver management                                             **|
\**/
/* Code taken and adapted from base/system/services/driver.c */
static DWORD
ScmLoadDriver(LPCWSTR lpServiceName)
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN WasPrivilegeEnabled = FALSE;
    PWSTR pszDriverPath;
    UNICODE_STRING DriverPath;

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
    Status = RtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE,
                                TRUE,
                                FALSE,
                                &WasPrivilegeEnabled);
    if (!NT_SUCCESS(Status))
    {
        /* We encountered a failure, exit properly */
        DPRINT1("CONSRV: Cannot acquire driver-loading privilege, Status = 0x%08lx\n", Status);
        goto done;
    }

    Status = NtLoadDriver(&DriverPath);

    /* Release driver-loading privilege */
    RtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE,
                       WasPrivilegeEnabled,
                       FALSE,
                       &WasPrivilegeEnabled);

done:
    RtlFreeHeap(ConSrvHeap, 0, pszDriverPath);
    return RtlNtStatusToDosError(Status);
}

#ifdef BLUESCREEN_DRIVER_UNLOADING
static DWORD
ScmUnloadDriver(LPCWSTR lpServiceName)
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN WasPrivilegeEnabled = FALSE;
    PWSTR pszDriverPath;
    UNICODE_STRING DriverPath;

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

    /* Acquire driver-unloading privilege */
    Status = RtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE,
                                TRUE,
                                FALSE,
                                &WasPrivilegeEnabled);
    if (!NT_SUCCESS(Status))
    {
        /* We encountered a failure, exit properly */
        DPRINT1("CONSRV: Cannot acquire driver-unloading privilege, Status = 0x%08lx\n", Status);
        goto done;
    }

    Status = NtUnloadDriver(&DriverPath);

    /* Release driver-unloading privilege */
    RtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE,
                       WasPrivilegeEnabled,
                       FALSE,
                       &WasPrivilegeEnabled);

done:
    RtlFreeHeap(ConSrvHeap, 0, pszDriverPath);
    return RtlNtStatusToDosError(Status);
}
#endif
/**\
\******************************************************************************/

static BOOL FASTCALL
TuiSwapConsole(INT Next)
{
    static PTUI_CONSOLE_DATA SwapConsole = NULL; /* Console we are thinking about swapping with */
    DWORD BytesReturned;
    ANSI_STRING Title;
    PVOID Buffer;
    PCOORD pos;

    if (0 != Next)
    {
        /*
         * Alt-Tab, swap consoles.
         * move SwapConsole to next console, and print its title.
         */
        EnterCriticalSection(&ActiveVirtConsLock);
        if (!SwapConsole) SwapConsole = ActiveConsole;

        SwapConsole = (0 < Next ? GetNextConsole(SwapConsole) : GetPrevConsole(SwapConsole));
        Title.MaximumLength = RtlUnicodeStringToAnsiSize(&SwapConsole->Console->Title);
        Title.Length = 0;
        Buffer = RtlAllocateHeap(ConSrvHeap, 0,
                                 sizeof(COORD) + Title.MaximumLength);
        pos = (PCOORD)Buffer;
        Title.Buffer = (PVOID)((ULONG_PTR)Buffer + sizeof(COORD));

        RtlUnicodeStringToAnsiString(&Title, &SwapConsole->Console->Title, FALSE);
        pos->X = (PhysicalConsoleSize.X - Title.Length) / 2;
        pos->Y = PhysicalConsoleSize.Y / 2;
        /* Redraw the console to clear off old title */
        ConioDrawConsole(ActiveConsole->Console);
        if (!DeviceIoControl(ConsoleDeviceHandle, IOCTL_CONSOLE_WRITE_OUTPUT_CHARACTER,
                             NULL, 0, Buffer, sizeof(COORD) + Title.Length,
                             &BytesReturned, NULL))
        {
            DPRINT1( "Error writing to console\n" );
        }
        RtlFreeHeap(ConSrvHeap, 0, Buffer);
        LeaveCriticalSection(&ActiveVirtConsLock);

        return TRUE;
    }
    else if (NULL != SwapConsole)
    {
        EnterCriticalSection(&ActiveVirtConsLock);
        if (SwapConsole != ActiveConsole)
        {
            /* First remove swapconsole from the list */
            SwapConsole->Entry.Blink->Flink = SwapConsole->Entry.Flink;
            SwapConsole->Entry.Flink->Blink = SwapConsole->Entry.Blink;
            /* Now insert before activeconsole */
            SwapConsole->Entry.Flink = &ActiveConsole->Entry;
            SwapConsole->Entry.Blink = ActiveConsole->Entry.Blink;
            ActiveConsole->Entry.Blink->Flink = &SwapConsole->Entry;
            ActiveConsole->Entry.Blink = &SwapConsole->Entry;
        }
        ActiveConsole = SwapConsole;
        SwapConsole = NULL;
        ConioDrawConsole(ActiveConsole->Console);
        LeaveCriticalSection(&ActiveVirtConsLock);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static VOID FASTCALL
TuiCopyRect(char *Dest, PCONSOLE_SCREEN_BUFFER Buff, SMALL_RECT* Region)
{
    UINT SrcDelta, DestDelta;
    LONG i;
    PBYTE Src, SrcEnd;

    Src = ConioCoordToPointer(Buff, Region->Left, Region->Top);
    SrcDelta = Buff->ScreenBufferSize.X * 2;
    SrcEnd = Buff->Buffer + Buff->ScreenBufferSize.Y * Buff->ScreenBufferSize.X * 2;
    DestDelta = ConioRectWidth(Region) * 2;
    for (i = Region->Top; i <= Region->Bottom; i++)
    {
        memcpy(Dest, Src, DestDelta);
        Src += SrcDelta;
        if (SrcEnd <= Src)
        {
            Src -= Buff->ScreenBufferSize.Y * Buff->ScreenBufferSize.X * 2;
        }
        Dest += DestDelta;
    }
}

static LRESULT CALLBACK
TuiConsoleWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
/*
    PTUI_CONSOLE_DATA TuiData = NULL;
    PCONSOLE Console = NULL;

    TuiData = TuiGetGuiData(hWnd);
    if (TuiData == NULL) return 0;
*/

    switch (msg)
    {
        case WM_CHAR:
        case WM_SYSCHAR:
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            if (ConSrvValidateConsoleUnsafe(ActiveConsole->Console, CONSOLE_RUNNING, TRUE))
            {
                MSG Message;
                Message.hwnd = hWnd;
                Message.message = msg;
                Message.wParam = wParam;
                Message.lParam = lParam;

                ConioProcessKey(ActiveConsole->Console, &Message);
                LeaveCriticalSection(&ActiveConsole->Console->Lock);
            }
            break;
        }

        case WM_ACTIVATE:
        {
            if (ConSrvValidateConsoleUnsafe(ActiveConsole->Console, CONSOLE_RUNNING, TRUE))
            {
                if (LOWORD(wParam) != WA_INACTIVE)
                {
                    SetFocus(hWnd);
                    ConioDrawConsole(ActiveConsole->Console);
                }
                LeaveCriticalSection(&ActiveConsole->Console->Lock);
            }
            break;
        }

        default:
            break;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static DWORD WINAPI
TuiConsoleThread(PVOID Data)
{
    PTUI_CONSOLE_DATA TuiData = (PTUI_CONSOLE_DATA)Data;
    PCONSOLE Console = TuiData->Console;
    HWND NewWindow;
    MSG msg;

    NewWindow = CreateWindowW(TUI_CONSOLE_WINDOW_CLASS,
                              Console->Title.Buffer,
                              0,
                              -32000, -32000, 0, 0,
                              NULL, NULL,
                              ConSrvDllInstance,
                              (PVOID)Console);
    if (NULL == NewWindow)
    {
        DPRINT1("CONSRV: Unable to create console window\n");
        return 1;
    }
    TuiData->hWindow = NewWindow;

    SetForegroundWindow(TuiData->hWindow);
    NtUserConsoleControl(ConsoleAcquireDisplayOwnership, NULL, 0);

    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}

static BOOL
TuiInit(DWORD OemCP)
{
    BOOL Ret = FALSE;
    CONSOLE_SCREEN_BUFFER_INFO ScrInfo;
    DWORD BytesReturned;
    WNDCLASSEXW wc;
    ATOM ConsoleClassAtom;
    USHORT TextAttribute = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;

    /* Exit if we were already initialized */
    if (ConsInitialized) return TRUE;

    /*
     * Initialize the TUI front-end:
     * - load the console driver,
     * - set default screen attributes,
     * - grab the console size.
     */
    ScmLoadDriver(L"Blue");

    ConsoleDeviceHandle = CreateFileW(L"\\\\.\\BlueScreen",
                                      FILE_ALL_ACCESS,
                                      0, NULL,
                                      OPEN_EXISTING,
                                      0, NULL);
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
    InitializeListHead(&VirtConsList);
    InitializeCriticalSection(&ActiveVirtConsLock);

    if (!DeviceIoControl(ConsoleDeviceHandle, IOCTL_CONSOLE_GET_SCREEN_BUFFER_INFO,
                         NULL, 0, &ScrInfo, sizeof(ScrInfo), &BytesReturned, NULL))
    {
        DPRINT1("Failed to get console info\n");
        Ret = FALSE;
        goto Quit;
    }
    PhysicalConsoleSize = ScrInfo.dwSize;

    /* Register the TUI notification window class */
    RtlZeroMemory(&wc, sizeof(WNDCLASSEXW));
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpszClassName = TUI_CONSOLE_WINDOW_CLASS;
    wc.lpfnWndProc = TuiConsoleWndProc;
    wc.cbWndExtra = 0;
    wc.hInstance = ConSrvDllInstance;

    ConsoleClassAtom = RegisterClassExW(&wc);
    if (ConsoleClassAtom == 0)
    {
        DPRINT1("Failed to register TUI console wndproc\n");
        Ret = FALSE;
    }
    else
    {
        Ret = TRUE;
    }

Quit:
    if (Ret == FALSE)
    {
        DeleteCriticalSection(&ActiveVirtConsLock);
        CloseHandle(ConsoleDeviceHandle);
    }

    ConsInitialized = Ret;
    return Ret;
}



/******************************************************************************
 *                             TUI Console Driver                             *
 ******************************************************************************/

static VOID WINAPI
TuiCleanupConsole(PCONSOLE Console)
{
    PTUI_CONSOLE_DATA TuiData = Console->TermIFace.Data;

    /* Close the notification window */
    DestroyWindow(TuiData->hWindow);

    /*
     * Set the active console to the next one
     * and remove the console from the list.
     */
    EnterCriticalSection(&ActiveVirtConsLock);
    ActiveConsole = GetNextConsole(TuiData);
    RemoveEntryList(&TuiData->Entry);

    // /* Switch to next console */
    // if (ActiveConsole == TuiData)
    // if (ActiveConsole->Console == Console)
    // {
        // ActiveConsole = (TuiData->Entry.Flink != TuiData->Entry ? GetNextConsole(TuiData) : NULL);
    // }

    // if (GetNextConsole(TuiData) != TuiData)
    // {
        // TuiData->Entry.Blink->Flink = TuiData->Entry.Flink;
        // TuiData->Entry.Flink->Blink = TuiData->Entry.Blink;
    // }

    LeaveCriticalSection(&ActiveVirtConsLock);

    /* Switch to the next console */
    if (NULL != ActiveConsole) ConioDrawConsole(ActiveConsole->Console);

    Console->TermIFace.Data = NULL;
    DeleteCriticalSection(&TuiData->Lock);
    RtlFreeHeap(ConSrvHeap, 0, TuiData);
}

static VOID WINAPI
TuiWriteStream(PCONSOLE Console, SMALL_RECT* Region, SHORT CursorStartX, SHORT CursorStartY,
               UINT ScrolledLines, CHAR *Buffer, UINT Length)
{
    DWORD BytesWritten;
    PCONSOLE_SCREEN_BUFFER Buff = Console->ActiveBuffer;

    if (ActiveConsole->Console->ActiveBuffer != Buff) return;

    if (!WriteFile(ConsoleDeviceHandle, Buffer, Length, &BytesWritten, NULL))
    {
        DPRINT1("Error writing to BlueScreen\n");
    }
}

static VOID WINAPI
TuiDrawRegion(PCONSOLE Console, SMALL_RECT* Region)
{
    DWORD BytesReturned;
    PCONSOLE_SCREEN_BUFFER Buff = Console->ActiveBuffer;
    PCONSOLE_DRAW ConsoleDraw;
    UINT ConsoleDrawSize;

    if (ActiveConsole->Console != Console) return;

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
    ConsoleDraw->CursorX = Buff->CursorPosition.X;
    ConsoleDraw->CursorY = Buff->CursorPosition.Y;

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

static BOOL WINAPI
TuiSetCursorInfo(PCONSOLE Console, PCONSOLE_SCREEN_BUFFER Buff)
{
    CONSOLE_CURSOR_INFO Info;
    DWORD BytesReturned;

    if (ActiveConsole->Console->ActiveBuffer != Buff) return TRUE;

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
TuiSetScreenInfo(PCONSOLE Console, PCONSOLE_SCREEN_BUFFER Buff, SHORT OldCursorX, SHORT OldCursorY)
{
    CONSOLE_SCREEN_BUFFER_INFO Info;
    DWORD BytesReturned;

    if (ActiveConsole->Console->ActiveBuffer != Buff) return TRUE;

    Info.dwCursorPosition = Buff->CursorPosition;
    Info.wAttributes = Buff->ScreenDefaultAttrib;

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
TuiIsBufferResizeSupported(PCONSOLE Console)
{
    return (Console && Console->State == CONSOLE_INITIALIZING ? TRUE : FALSE);
}

static VOID WINAPI
TuiResizeTerminal(PCONSOLE Console)
{
}

static BOOL WINAPI
TuiProcessKeyCallback(PCONSOLE Console, MSG* msg, BYTE KeyStateMenu, DWORD ShiftState, UINT VirtualKeyCode, BOOL Down)
{
    if (0 != (ShiftState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED)) &&
        VK_TAB == VirtualKeyCode)
    {
        if (Down)
        {
            TuiSwapConsole(ShiftState & SHIFT_PRESSED ? -1 : 1);
        }

        return TRUE;
    }
    else if (VK_MENU == VirtualKeyCode && !Down)
    {
        return TuiSwapConsole(0);
    }

    return FALSE;
}

static VOID WINAPI
TuiRefreshInternalInfo(PCONSOLE Console)
{
}

static VOID WINAPI
TuiChangeTitle(PCONSOLE Console)
{
}

static BOOL WINAPI
TuiChangeIcon(PCONSOLE Console, HICON hWindowIcon)
{
    return TRUE;
}

static HWND WINAPI
TuiGetConsoleWindowHandle(PCONSOLE Console)
{
    PTUI_CONSOLE_DATA TuiData = Console->TermIFace.Data;
    return TuiData->hWindow;
}

static VOID WINAPI
TuiGetLargestConsoleWindowSize(PCONSOLE Console, PCOORD pSize)
{
    if (!pSize) return;
    *pSize = PhysicalConsoleSize;
}

static FRONTEND_VTBL TuiVtbl =
{
    TuiCleanupConsole,
    TuiWriteStream,
    TuiDrawRegion,
    TuiSetCursorInfo,
    TuiSetScreenInfo,
    TuiUpdateScreenInfo,
    TuiIsBufferResizeSupported,
    TuiResizeTerminal,
    TuiProcessKeyCallback,
    TuiRefreshInternalInfo,
    TuiChangeTitle,
    TuiChangeIcon,
    TuiGetConsoleWindowHandle,
    TuiGetLargestConsoleWindowSize
};

NTSTATUS FASTCALL
TuiInitConsole(PCONSOLE Console,
               /*IN*/ PCONSOLE_START_INFO ConsoleStartInfo,
               PCONSOLE_INFO ConsoleInfo,
               DWORD ProcessId)
{
    PTUI_CONSOLE_DATA TuiData;
    HANDLE ThreadHandle;

    if (Console == NULL || ConsoleInfo == NULL)
        return STATUS_INVALID_PARAMETER;

    /* Initialize the TUI terminal emulator */
    if (!TuiInit(Console->CodePage)) return STATUS_UNSUCCESSFUL;

    /* Initialize the console */
    Console->TermIFace.Vtbl = &TuiVtbl;

    TuiData = RtlAllocateHeap(ConSrvHeap, HEAP_ZERO_MEMORY,
                              sizeof(TUI_CONSOLE_DATA));
    if (!TuiData)
    {
        DPRINT1("CONSRV: Failed to create TUI_CONSOLE_DATA\n");
        return STATUS_UNSUCCESSFUL;
    }
    Console->TermIFace.Data = (PVOID)TuiData;
    TuiData->Console = Console;
    TuiData->hWindow = NULL;

    InitializeCriticalSection(&TuiData->Lock);

    /*
     * HACK: Resize the console since we don't support for now changing
     * the console size when we display it with the hardware.
     */
    Console->ConsoleSize = PhysicalConsoleSize;
    ConioResizeBuffer(Console, Console->ActiveBuffer, PhysicalConsoleSize);
    Console->ActiveBuffer->DisplayMode |= CONSOLE_FULLSCREEN_MODE;
    // ConioResizeTerminal(Console);

    /*
     * Contrary to what we do in the GUI front-end, here we create
     * an input thread for each console. It will dispatch all the
     * input messages to the proper console (on the GUI it is done
     * via the default GUI dispatch thread).
     */
    ThreadHandle = CreateThread(NULL,
                                0,
                                TuiConsoleThread,
                                (PVOID)TuiData,
                                0,
                                NULL);
    if (NULL == ThreadHandle)
    {
        DPRINT1("CONSRV: Unable to create console thread\n");
        TuiCleanupConsole(Console);
        return STATUS_UNSUCCESSFUL;
    }
    CloseHandle(ThreadHandle);

    /*
     * Insert the newly created console in the list of virtual consoles
     * and activate it (give it the focus).
     */
    EnterCriticalSection(&ActiveVirtConsLock);
    InsertTailList(&VirtConsList, &TuiData->Entry);
    ActiveConsole = TuiData;
    LeaveCriticalSection(&ActiveVirtConsLock);

    return STATUS_SUCCESS;
}

/* EOF */
