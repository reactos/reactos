/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            consrv/frontends/tui/tuiterm.c
 * PURPOSE:         TUI Terminal Front-End - Virtual Consoles...
 * PROGRAMMERS:     David Welch
 *                  Gé van Geldorp
 *                  Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifdef TUITERM_COMPILE

#include <consrv.h>

// #include "include/conio.h"
#include "include/console.h"
#include "include/settings.h"
#include "tuiterm.h"

#include <ndk/iofuncs.h>
#include <ndk/setypes.h>
#include <drivers/blue/ntddblue.h>

#define NDEBUG
#include <debug.h>


/* GLOBALS ********************************************************************/

#define ConsoleOutputUnicodeToAnsiChar(Console, dChar, sWChar) \
    ASSERT((ULONG_PTR)dChar != (ULONG_PTR)sWChar); \
    WideCharToMultiByte((Console)->OutputCodePage, 0, (sWChar), 1, (dChar), 1, NULL, NULL)

/* TUI Console Window Class name */
#define TUI_CONSOLE_WINDOW_CLASS L"TuiConsoleWindowClass"

typedef struct _TUI_CONSOLE_DATA
{
    CRITICAL_SECTION Lock;
    LIST_ENTRY Entry;           /* Entry in the list of virtual consoles */
    // HANDLE hTuiInitEvent;
    // HANDLE hTuiTermEvent;

    HWND hWindow;               /* Handle to the console's window (used for the window's procedure) */

    PCONSRV_CONSOLE Console;           /* Pointer to the owned console */
    PCONSOLE_SCREEN_BUFFER ActiveBuffer;    /* Pointer to the active screen buffer (then maybe the previous Console member is redundant?? Or not...) */
    // TUI_CONSOLE_INFO TuiInfo;   /* TUI terminal settings */
} TUI_CONSOLE_DATA, *PTUI_CONSOLE_DATA;

#define GetNextConsole(Console) \
    CONTAINING_RECORD(Console->Entry.Flink, TUI_CONSOLE_DATA, Entry)

#define GetPrevConsole(Console) \
    CONTAINING_RECORD(Console->Entry.Blink, TUI_CONSOLE_DATA, Entry)


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
    pszDriverPath = ConsoleAllocHeap(HEAP_ZERO_MEMORY,
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
    ConsoleFreeHeap(pszDriverPath);
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
    pszDriverPath = ConsoleAllocHeap(HEAP_ZERO_MEMORY,
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
    ConsoleFreeHeap(pszDriverPath);
    return RtlNtStatusToDosError(Status);
}
#endif
/**\
\******************************************************************************/

#if 0
static BOOL
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
        Buffer = ConsoleAllocHeap(0, sizeof(COORD) + Title.MaximumLength);
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
        ConsoleFreeHeap(Buffer);
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
#endif

static VOID
TuiCopyRect(PCHAR Dest, PTEXTMODE_SCREEN_BUFFER Buff, SMALL_RECT* Region)
{
    UINT SrcDelta, DestDelta;
    LONG i;
    PCHAR_INFO Src, SrcEnd;

    Src = ConioCoordToPointer(Buff, Region->Left, Region->Top);
    SrcDelta = Buff->ScreenBufferSize.X * sizeof(CHAR_INFO);
    SrcEnd = Buff->Buffer + Buff->ScreenBufferSize.Y * Buff->ScreenBufferSize.X * sizeof(CHAR_INFO);
    DestDelta = ConioRectWidth(Region) * 2 /* 2 == sizeof(CHAR) + sizeof(BYTE) */;
    for (i = Region->Top; i <= Region->Bottom; i++)
    {
        ConsoleOutputUnicodeToAnsiChar(Buff->Header.Console, (PCHAR)Dest, &Src->Char.UnicodeChar);
        *(PBYTE)(Dest + 1) = (BYTE)Src->Attributes;

        Src += SrcDelta;
        if (SrcEnd <= Src)
        {
            Src -= Buff->ScreenBufferSize.Y * Buff->ScreenBufferSize.X * sizeof(CHAR_INFO);
        }
        Dest += DestDelta;
    }
}

static LRESULT CALLBACK
TuiConsoleWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
/*
    PTUI_CONSOLE_DATA TuiData = NULL;
    PCONSRV_CONSOLE Console = NULL;

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
#if 0
            if ((HIWORD(lParam) & KF_ALTDOWN) && wParam == VK_TAB)
            {
                // if ((HIWORD(lParam) & (KF_UP | KF_REPEAT)) != KF_REPEAT)
                    TuiSwapConsole(ShiftState & SHIFT_PRESSED ? -1 : 1);

                break;
            }
            else if (wParam == VK_MENU /* && !Down */)
            {
                TuiSwapConsole(0);
                break;
            }
#endif

            if (ConDrvValidateConsoleUnsafe((PCONSOLE)ActiveConsole->Console, CONSOLE_RUNNING, TRUE))
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
            if (ConDrvValidateConsoleUnsafe((PCONSOLE)ActiveConsole->Console, CONSOLE_RUNNING, TRUE))
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

static DWORD NTAPI
TuiConsoleThread(PVOID Param)
{
    PTUI_CONSOLE_DATA TuiData = (PTUI_CONSOLE_DATA)Param;
    PCONSRV_CONSOLE Console = TuiData->Console;
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
    if (ConsoleDeviceHandle == INVALID_HANDLE_VALUE)
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

static VOID NTAPI
TuiDeinitFrontEnd(IN OUT PFRONTEND This /*,
                  IN PCONSRV_CONSOLE Console */);

static NTSTATUS NTAPI
TuiInitFrontEnd(IN OUT PFRONTEND This,
                IN PCONSRV_CONSOLE Console)
{
    PTUI_CONSOLE_DATA TuiData;
    HANDLE ThreadHandle;

    if (This == NULL || Console == NULL)
        return STATUS_INVALID_PARAMETER;

    if (GetType(Console->ActiveBuffer) != TEXTMODE_BUFFER)
        return STATUS_INVALID_PARAMETER;

    TuiData = ConsoleAllocHeap(HEAP_ZERO_MEMORY, sizeof(TUI_CONSOLE_DATA));
    if (!TuiData)
    {
        DPRINT1("CONSRV: Failed to create TUI_CONSOLE_DATA\n");
        return STATUS_UNSUCCESSFUL;
    }
    // Console->FrontEndIFace.Context = (PVOID)TuiData;
    TuiData->Console      = Console;
    TuiData->ActiveBuffer = Console->ActiveBuffer;
    TuiData->hWindow = NULL;

    InitializeCriticalSection(&TuiData->Lock);

    /*
     * HACK: Resize the console since we don't support for now changing
     * the console size when we display it with the hardware.
     */
    // Console->ConsoleSize = PhysicalConsoleSize;
    // ConioResizeBuffer(Console, (PTEXTMODE_SCREEN_BUFFER)(Console->ActiveBuffer), PhysicalConsoleSize);

    // /* The console cannot be resized anymore */
    // Console->FixedSize = TRUE; // MUST be placed AFTER the call to ConioResizeBuffer !!
    // // TermResizeTerminal(Console);

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
        // TuiDeinitFrontEnd(Console);
        TuiDeinitFrontEnd(This);
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

    /* Finally, initialize the frontend structure */
    This->Context  = TuiData;
    This->Context2 = NULL;

    return STATUS_SUCCESS;
}

static VOID NTAPI
TuiDeinitFrontEnd(IN OUT PFRONTEND This)
{
    // PCONSRV_CONSOLE Console = This->Console;
    PTUI_CONSOLE_DATA TuiData = This->Context;

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

    This->Context = NULL;
    DeleteCriticalSection(&TuiData->Lock);
    ConsoleFreeHeap(TuiData);
}

static VOID NTAPI
TuiDrawRegion(IN OUT PFRONTEND This,
              SMALL_RECT* Region)
{
    PTUI_CONSOLE_DATA TuiData = This->Context;
    PCONSOLE_SCREEN_BUFFER Buff = TuiData->Console->ActiveBuffer;
    PCONSOLE_DRAW ConsoleDraw;
    DWORD BytesReturned;
    UINT ConsoleDrawSize;

    if (TuiData != ActiveConsole) return;
    if (GetType(Buff) != TEXTMODE_BUFFER) return;

    ConsoleDrawSize = sizeof(CONSOLE_DRAW) +
                      (ConioRectWidth(Region) * ConioRectHeight(Region)) * 2;
    ConsoleDraw = ConsoleAllocHeap(0, ConsoleDrawSize);
    if (NULL == ConsoleDraw)
    {
        DPRINT1("ConsoleAllocHeap failed\n");
        return;
    }
    ConsoleDraw->X = Region->Left;
    ConsoleDraw->Y = Region->Top;
    ConsoleDraw->SizeX = ConioRectWidth(Region);
    ConsoleDraw->SizeY = ConioRectHeight(Region);
    ConsoleDraw->CursorX = Buff->CursorPosition.X;
    ConsoleDraw->CursorY = Buff->CursorPosition.Y;

    TuiCopyRect((PCHAR)(ConsoleDraw + 1), (PTEXTMODE_SCREEN_BUFFER)Buff, Region);

    if (!DeviceIoControl(ConsoleDeviceHandle, IOCTL_CONSOLE_DRAW,
                         NULL, 0, ConsoleDraw, ConsoleDrawSize, &BytesReturned, NULL))
    {
        DPRINT1("Failed to draw console\n");
        ConsoleFreeHeap(ConsoleDraw);
        return;
    }

    ConsoleFreeHeap(ConsoleDraw);
}

static VOID NTAPI
TuiWriteStream(IN OUT PFRONTEND This,
               SMALL_RECT* Region,
               SHORT CursorStartX,
               SHORT CursorStartY,
               UINT ScrolledLines,
               PWCHAR Buffer,
               UINT Length)
{
    PTUI_CONSOLE_DATA TuiData = This->Context;
    PCONSOLE_SCREEN_BUFFER Buff = TuiData->Console->ActiveBuffer;
    PCHAR NewBuffer;
    ULONG NewLength;
    DWORD BytesWritten;

    if (TuiData != ActiveConsole) return;
    if (GetType(Buff) != TEXTMODE_BUFFER) return;

    NewLength = WideCharToMultiByte(TuiData->Console->OutputCodePage, 0,
                                    Buffer, Length,
                                    NULL, 0, NULL, NULL);
    NewBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, NewLength * sizeof(CHAR));
    if (!NewBuffer) return;

    WideCharToMultiByte(TuiData->Console->OutputCodePage, 0,
                        Buffer, Length,
                        NewBuffer, NewLength, NULL, NULL);

    if (!WriteFile(ConsoleDeviceHandle, NewBuffer, NewLength * sizeof(CHAR), &BytesWritten, NULL))
    {
        DPRINT1("Error writing to BlueScreen\n");
    }

    RtlFreeHeap(RtlGetProcessHeap(), 0, NewBuffer);
}

static VOID NTAPI
TuiRingBell(IN OUT PFRONTEND This)
{
    Beep(800, 200);
}

static BOOL NTAPI
TuiSetCursorInfo(IN OUT PFRONTEND This,
                 PCONSOLE_SCREEN_BUFFER Buff)
{
    PTUI_CONSOLE_DATA TuiData = This->Context;
    CONSOLE_CURSOR_INFO Info;
    DWORD BytesReturned;

    if (TuiData != ActiveConsole) return TRUE;
    if (TuiData->Console->ActiveBuffer != Buff) return TRUE;
    if (GetType(Buff) != TEXTMODE_BUFFER) return FALSE;

    Info.dwSize = ConioEffectiveCursorSize(TuiData->Console, 100);
    Info.bVisible = Buff->CursorInfo.bVisible;

    if (!DeviceIoControl(ConsoleDeviceHandle, IOCTL_CONSOLE_SET_CURSOR_INFO,
                         &Info, sizeof(Info), NULL, 0, &BytesReturned, NULL))
    {
        DPRINT1( "Failed to set cursor info\n" );
        return FALSE;
    }

    return TRUE;
}

static BOOL NTAPI
TuiSetScreenInfo(IN OUT PFRONTEND This,
                 PCONSOLE_SCREEN_BUFFER Buff,
                 SHORT OldCursorX,
                 SHORT OldCursorY)
{
    PTUI_CONSOLE_DATA TuiData = This->Context;
    CONSOLE_SCREEN_BUFFER_INFO Info;
    DWORD BytesReturned;

    if (TuiData != ActiveConsole) return TRUE;
    if (TuiData->Console->ActiveBuffer != Buff) return TRUE;
    if (GetType(Buff) != TEXTMODE_BUFFER) return FALSE;

    Info.dwCursorPosition = Buff->CursorPosition;
    Info.wAttributes = ((PTEXTMODE_SCREEN_BUFFER)Buff)->ScreenDefaultAttrib;

    if (!DeviceIoControl(ConsoleDeviceHandle, IOCTL_CONSOLE_SET_SCREEN_BUFFER_INFO,
                         &Info, sizeof(CONSOLE_SCREEN_BUFFER_INFO), NULL, 0,
                         &BytesReturned, NULL))
    {
        DPRINT1( "Failed to set cursor position\n" );
        return FALSE;
    }

    return TRUE;
}

static VOID NTAPI
TuiResizeTerminal(IN OUT PFRONTEND This)
{
}

static VOID NTAPI
TuiSetActiveScreenBuffer(IN OUT PFRONTEND This)
{
    // PGUI_CONSOLE_DATA GuiData = This->Context;
    // PCONSOLE_SCREEN_BUFFER ActiveBuffer;
    // HPALETTE hPalette;

    // EnterCriticalSection(&GuiData->Lock);
    // GuiData->WindowSizeLock = TRUE;

    // InterlockedExchangePointer(&GuiData->ActiveBuffer,
                               // ConDrvGetActiveScreenBuffer(GuiData->Console));

    // GuiData->WindowSizeLock = FALSE;
    // LeaveCriticalSection(&GuiData->Lock);

    // ActiveBuffer = GuiData->ActiveBuffer;

    // /* Change the current palette */
    // if (ActiveBuffer->PaletteHandle == NULL)
    // {
        // hPalette = GuiData->hSysPalette;
    // }
    // else
    // {
        // hPalette = ActiveBuffer->PaletteHandle;
    // }

    // DPRINT("GuiSetActiveScreenBuffer using palette 0x%p\n", hPalette);

    // /* Set the new palette for the framebuffer */
    // SelectPalette(GuiData->hMemDC, hPalette, FALSE);

    // /* Specify the use of the system palette for the framebuffer */
    // SetSystemPaletteUse(GuiData->hMemDC, ActiveBuffer->PaletteUsage);

    // /* Realize the (logical) palette */
    // RealizePalette(GuiData->hMemDC);

    // GuiResizeTerminal(This);
    // // ConioDrawConsole(Console);
}

static VOID NTAPI
TuiReleaseScreenBuffer(IN OUT PFRONTEND This,
                       IN PCONSOLE_SCREEN_BUFFER ScreenBuffer)
{
    // PGUI_CONSOLE_DATA GuiData = This->Context;

    // /*
     // * If we were notified to release a screen buffer that is not actually
     // * ours, then just ignore the notification...
     // */
    // if (ScreenBuffer != GuiData->ActiveBuffer) return;

    // /*
     // * ... else, we must release our active buffer. Two cases are present:
     // * - If ScreenBuffer (== GuiData->ActiveBuffer) IS NOT the console
     // *   active screen buffer, then we can safely switch to it.
     // * - If ScreenBuffer IS the console active screen buffer, we must release
     // *   it ONLY.
     // */

    // /* Release the old active palette and set the default one */
    // if (GetCurrentObject(GuiData->hMemDC, OBJ_PAL) == ScreenBuffer->PaletteHandle)
    // {
        // /* Set the new palette */
        // SelectPalette(GuiData->hMemDC, GuiData->hSysPalette, FALSE);
    // }

    // /* Set the adequate active screen buffer */
    // if (ScreenBuffer != GuiData->Console->ActiveBuffer)
    // {
        // GuiSetActiveScreenBuffer(This);
    // }
    // else
    // {
        // EnterCriticalSection(&GuiData->Lock);
        // GuiData->WindowSizeLock = TRUE;

        // InterlockedExchangePointer(&GuiData->ActiveBuffer, NULL);

        // GuiData->WindowSizeLock = FALSE;
        // LeaveCriticalSection(&GuiData->Lock);
    // }
}

static VOID NTAPI
TuiRefreshInternalInfo(IN OUT PFRONTEND This)
{
}

static VOID NTAPI
TuiChangeTitle(IN OUT PFRONTEND This)
{
}

static BOOL NTAPI
TuiChangeIcon(IN OUT PFRONTEND This,
              HICON IconHandle)
{
    return TRUE;
}

static HWND NTAPI
TuiGetConsoleWindowHandle(IN OUT PFRONTEND This)
{
    PTUI_CONSOLE_DATA TuiData = This->Context;
    return TuiData->hWindow;
}

static VOID NTAPI
TuiGetLargestConsoleWindowSize(IN OUT PFRONTEND This,
                               PCOORD pSize)
{
    if (!pSize) return;
    *pSize = PhysicalConsoleSize;
}

static BOOL NTAPI
TuiGetSelectionInfo(IN OUT PFRONTEND This,
                    PCONSOLE_SELECTION_INFO pSelectionInfo)
{
    return TRUE;
}

static BOOL NTAPI
TuiSetPalette(IN OUT PFRONTEND This,
              HPALETTE PaletteHandle,
              UINT PaletteUsage)
{
    return TRUE;
}

static ULONG NTAPI
TuiGetDisplayMode(IN OUT PFRONTEND This)
{
    return CONSOLE_FULLSCREEN_HARDWARE; // CONSOLE_FULLSCREEN;
}

static BOOL NTAPI
TuiSetDisplayMode(IN OUT PFRONTEND This,
                  ULONG NewMode)
{
    // if (NewMode & ~(CONSOLE_FULLSCREEN_MODE | CONSOLE_WINDOWED_MODE))
    //     return FALSE;
    return TRUE;
}

static INT NTAPI
TuiShowMouseCursor(IN OUT PFRONTEND This,
                   BOOL Show)
{
    return 0;
}

static BOOL NTAPI
TuiSetMouseCursor(IN OUT PFRONTEND This,
                  HCURSOR CursorHandle)
{
    return TRUE;
}

static HMENU NTAPI
TuiMenuControl(IN OUT PFRONTEND This,
               UINT CmdIdLow,
               UINT CmdIdHigh)
{
    return NULL;
}

static BOOL NTAPI
TuiSetMenuClose(IN OUT PFRONTEND This,
                BOOL Enable)
{
    return TRUE;
}

static FRONTEND_VTBL TuiVtbl =
{
    TuiInitFrontEnd,
    TuiDeinitFrontEnd,
    TuiDrawRegion,
    TuiWriteStream,
    TuiRingBell,
    TuiSetCursorInfo,
    TuiSetScreenInfo,
    TuiResizeTerminal,
    TuiSetActiveScreenBuffer,
    TuiReleaseScreenBuffer,
    TuiRefreshInternalInfo,
    TuiChangeTitle,
    TuiChangeIcon,
    TuiGetConsoleWindowHandle,
    TuiGetLargestConsoleWindowSize,
    TuiGetSelectionInfo,
    TuiSetPalette,
    TuiGetDisplayMode,
    TuiSetDisplayMode,
    TuiShowMouseCursor,
    TuiSetMouseCursor,
    TuiMenuControl,
    TuiSetMenuClose,
};

static BOOLEAN
IsConsoleMode(VOID)
{
    return (BOOLEAN)NtUserCallNoParam(NOPARAM_ROUTINE_ISCONSOLEMODE);
}

NTSTATUS NTAPI
TuiLoadFrontEnd(IN OUT PFRONTEND FrontEnd,
                IN OUT PCONSOLE_STATE_INFO ConsoleInfo,
                IN OUT PCONSOLE_INIT_INFO ConsoleInitInfo,
                IN HANDLE ConsoleLeaderProcessHandle)
{
    if (FrontEnd == NULL || ConsoleInfo == NULL)
        return STATUS_INVALID_PARAMETER;

    /* We must be in console mode already */
    if (!IsConsoleMode()) return STATUS_UNSUCCESSFUL;

    /* Initialize the TUI terminal emulator */
    if (!TuiInit(ConsoleInfo->CodePage)) return STATUS_UNSUCCESSFUL;

    /* Finally, initialize the frontend structure */
    FrontEnd->Vtbl     = &TuiVtbl;
    FrontEnd->Context  = NULL;
    FrontEnd->Context2 = NULL;

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
TuiUnloadFrontEnd(IN OUT PFRONTEND FrontEnd)
{
    if (FrontEnd == NULL) return STATUS_INVALID_PARAMETER;
    if (FrontEnd->Context) TuiDeinitFrontEnd(FrontEnd);

    return STATUS_SUCCESS;
}

#endif

/* EOF */
