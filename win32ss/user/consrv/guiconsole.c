/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/guiconsole.c
 * PURPOSE:         Implementation of GUI-mode consoles
 * PROGRAMMERS:
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include "settings.h"
#include "guiconsole.h"

#define NDEBUG
#include <debug.h>

/*
// Define wmemset(...)
#include <wchar.h>
#define HAVE_WMEMSET
*/

/* GUI Console Window Class name */
#define GUI_CONSOLE_WINDOW_CLASS L"ConsoleWindowClass"


/* Not defined in any header file */
// extern VOID WINAPI PrivateCsrssManualGuiCheck(LONG Check);
// From win32ss/user/win32csr/dllmain.c
VOID
WINAPI
PrivateCsrssManualGuiCheck(LONG Check)
{
    NtUserCallOneParam(Check, ONEPARAM_ROUTINE_CSRSS_GUICHECK);
}

/* GLOBALS ********************************************************************/

typedef struct _GUI_CONSOLE_DATA
{
    CRITICAL_SECTION Lock;
    HANDLE hGuiInitEvent;
    BOOL WindowSizeLock;
    POINT OldCursor;

    // HWND hWindow;
    // HICON hIcon;
    // HICON hIconSm;

    HFONT Font;
    UINT CharWidth;
    UINT CharHeight;

    GUI_CONSOLE_INFO GuiInfo;
} GUI_CONSOLE_DATA, *PGUI_CONSOLE_DATA;

static BOOL    ConsInitialized = FALSE;
static HICON   ghDefaultIcon = NULL;
static HICON   ghDefaultIconSm = NULL;
static HCURSOR ghDefaultCursor = NULL;
static HWND    NotifyWnd = NULL;

typedef struct _GUICONSOLE_MENUITEM
{
    UINT uID;
    const struct _GUICONSOLE_MENUITEM *SubMenu;
    WORD wCmdID;
} GUICONSOLE_MENUITEM, *PGUICONSOLE_MENUITEM;

static const GUICONSOLE_MENUITEM GuiConsoleEditMenuItems[] =
{
    { IDS_MARK, NULL, ID_SYSTEM_EDIT_MARK },
    { IDS_COPY, NULL, ID_SYSTEM_EDIT_COPY },
    { IDS_PASTE, NULL, ID_SYSTEM_EDIT_PASTE },
    { IDS_SELECTALL, NULL, ID_SYSTEM_EDIT_SELECTALL },
    { IDS_SCROLL, NULL, ID_SYSTEM_EDIT_SCROLL },
    { IDS_FIND, NULL, ID_SYSTEM_EDIT_FIND },

    { 0, NULL, 0 } /* End of list */
};

static const GUICONSOLE_MENUITEM GuiConsoleMainMenuItems[] =
{
    { IDS_EDIT, GuiConsoleEditMenuItems, 0 },
    { IDS_DEFAULTS, NULL, ID_SYSTEM_DEFAULTS },
    { IDS_PROPERTIES, NULL, ID_SYSTEM_PROPERTIES },

    { 0, NULL, 0 } /* End of list */
};

/*
 * Default 16-color palette for foreground and background
 * (corresponding flags in comments).
 */
const COLORREF s_Colors[16] =
{
    RGB(0, 0, 0),       // (Black)
    RGB(0, 0, 128),     // BLUE
    RGB(0, 128, 0),     // GREEN
    RGB(0, 128, 128),   // BLUE  | GREEN
    RGB(128, 0, 0),     // RED
    RGB(128, 0, 128),   // BLUE  | RED
    RGB(128, 128, 0),   // GREEN | RED
    RGB(192, 192, 192), // BLUE  | GREEN | RED

    RGB(128, 128, 128), // (Grey)  INTENSITY
    RGB(0, 0, 255),     // BLUE  | INTENSITY
    RGB(0, 255, 0),     // GREEN | INTENSITY
    RGB(0, 255, 255),   // BLUE  | GREEN | INTENSITY
    RGB(255, 0, 0),     // RED   | INTENSITY
    RGB(255, 0, 255),   // BLUE  | RED   | INTENSITY
    RGB(255, 255, 0),   // GREEN | RED   | INTENSITY
    RGB(255, 255, 255)  // BLUE  | GREEN | RED | INTENSITY
};

/* FUNCTIONS ******************************************************************/

static VOID
GuiConsoleAppendMenuItems(HMENU hMenu,
                          const GUICONSOLE_MENUITEM *Items)
{
    UINT i = 0;
    WCHAR szMenuString[255];
    HMENU hSubMenu;

    do
    {
        if (Items[i].uID != (UINT)-1)
        {
            if (LoadStringW(ConSrvDllInstance,
                            Items[i].uID,
                            szMenuString,
                            sizeof(szMenuString) / sizeof(szMenuString[0])) > 0)
            {
                if (Items[i].SubMenu != NULL)
                {
                    hSubMenu = CreatePopupMenu();
                    if (hSubMenu != NULL)
                    {
                        GuiConsoleAppendMenuItems(hSubMenu,
                                                  Items[i].SubMenu);

                        if (!AppendMenuW(hMenu,
                                         MF_STRING | MF_POPUP,
                                         (UINT_PTR)hSubMenu,
                                         szMenuString))
                        {
                            DestroyMenu(hSubMenu);
                        }
                    }
                }
                else
                {
                    AppendMenuW(hMenu,
                                MF_STRING,
                                Items[i].wCmdID,
                                szMenuString);
                }
            }
        }
        else
        {
            AppendMenuW(hMenu,
                        MF_SEPARATOR,
                        0,
                        NULL);
        }
        i++;
    } while(!(Items[i].uID == 0 && Items[i].SubMenu == NULL && Items[i].wCmdID == 0));
}

static VOID
GuiConsoleCreateSysMenu(PCONSOLE Console)
{
    HMENU hMenu;
    hMenu = GetSystemMenu(Console->hWindow,
                          FALSE);
    if (hMenu != NULL)
    {
        GuiConsoleAppendMenuItems(hMenu,
                                  GuiConsoleMainMenuItems);
        DrawMenuBar(Console->hWindow);
    }
}


static VOID
GuiConsoleCopy(HWND hWnd, PCONSOLE Console);
static VOID
GuiConsolePaste(HWND hWnd, PCONSOLE Console);
static VOID
GuiConsoleUpdateSelection(PCONSOLE Console, PCOORD coord);
static VOID
GuiConsoleShowConsoleProperties(PCONSOLE Console,
                                HWND hWnd,
                                BOOL Defaults);

static LRESULT
GuiConsoleHandleSysMenuCommand(PCONSOLE Console,
                               HWND hWnd,
                               WPARAM wParam, LPARAM lParam)
{
    LRESULT Ret = TRUE;
    COORD bottomRight = { 0, 0 };

    switch (wParam)
    {
        case ID_SYSTEM_EDIT_MARK:
            DPRINT1("Marking not handled yet\n");
            break;

        case ID_SYSTEM_EDIT_COPY:
            GuiConsoleCopy(hWnd, Console);
            break;

        case ID_SYSTEM_EDIT_PASTE:
            GuiConsolePaste(hWnd, Console);
            break;

        case ID_SYSTEM_EDIT_SELECTALL:
            bottomRight.X = Console->Size.X - 1;
            bottomRight.Y = Console->Size.Y - 1;
            GuiConsoleUpdateSelection(Console, &bottomRight);
            break;

        case ID_SYSTEM_EDIT_SCROLL:
            DPRINT1("Scrolling is not handled yet\n");
            break;

        case ID_SYSTEM_EDIT_FIND:
            DPRINT1("Finding is not handled yet\n");
            break;

        case ID_SYSTEM_DEFAULTS:
            GuiConsoleShowConsoleProperties(Console, hWnd, TRUE);
            break;

        case ID_SYSTEM_PROPERTIES:
            GuiConsoleShowConsoleProperties(Console, hWnd, FALSE);
            break;

        default:
            Ret = DefWindowProcW(hWnd, WM_SYSCOMMAND, wParam, lParam);
            break;
    }
    return Ret;
}

static VOID
GuiConsoleShowConsoleProperties(PCONSOLE Console,
                                HWND hWnd,
                                BOOL Defaults)
{
    NTSTATUS Status;
    PGUI_CONSOLE_DATA GuiData = Console->GuiData;
    PCONSOLE_PROCESS_DATA ProcessData;
    HANDLE hSection = NULL, hClientSection = NULL;
    LARGE_INTEGER SectionSize;
    ULONG ViewSize = 0;
    SIZE_T Length = 0;
    PCONSOLE_PROPS pSharedInfo = NULL;

    DPRINT("GuiConsoleShowConsoleProperties entered\n");

    if (GuiData == NULL) return;

    /* Create a memory section to share with the applet, and map it */
    SectionSize.QuadPart = sizeof(CONSOLE_PROPS);
    Status = NtCreateSection(&hSection,
                             SECTION_ALL_ACCESS,
                             NULL,
                             &SectionSize,
                             PAGE_READWRITE,
                             SEC_COMMIT,
                             NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Error: Impossible to create a shared section ; Status = %lu\n", Status);
        return;
    }

    Status = NtMapViewOfSection(hSection,
                                NtCurrentProcess(),
                                (PVOID*)&pSharedInfo,
                                0,
                                0,
                                NULL,
                                &ViewSize,
                                ViewUnmap,
                                0,
                                PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Error: Impossible to map the shared section ; Status = %lu\n", Status);
        NtClose(hSection);
        return;
    }

    /*
     * Setup the shared console properties structure.
     */
    /* Header */
    pSharedInfo->hConsoleWindow = hWnd; // Console->hWindow;
    pSharedInfo->ShowDefaultParams = Defaults;
    /* Console information */
    pSharedInfo->ci.HistoryBufferSize = Console->HistoryBufferSize;
    pSharedInfo->ci.NumberOfHistoryBuffers = Console->NumberOfHistoryBuffers;
    pSharedInfo->ci.HistoryNoDup = Console->HistoryNoDup;
    pSharedInfo->ci.FullScreen = Console->FullScreen;
    pSharedInfo->ci.QuickEdit = Console->QuickEdit;
    pSharedInfo->ci.InsertMode = Console->InsertMode;
    pSharedInfo->ci.InputBufferSize = 0;
    pSharedInfo->ci.ScreenBufferSize = Console->ActiveBuffer->ScreenBufferSize;
    pSharedInfo->ci.ConsoleSize = Console->Size;
    pSharedInfo->ci.CursorBlinkOn;
    pSharedInfo->ci.ForceCursorOff;
    pSharedInfo->ci.CursorSize = Console->ActiveBuffer->CursorInfo.dwSize;
    pSharedInfo->ci.ScreenAttrib = Console->ActiveBuffer->ScreenDefaultAttrib;
    pSharedInfo->ci.PopupAttrib  = Console->ActiveBuffer->PopupDefaultAttrib;
    pSharedInfo->ci.CodePage;
    /* GUI Information */
    wcsncpy(pSharedInfo->ci.u.GuiInfo.FaceName, GuiData->GuiInfo.FaceName, LF_FACESIZE);
    pSharedInfo->ci.u.GuiInfo.FontSize = (DWORD)GuiData->GuiInfo.FontSize;
    pSharedInfo->ci.u.GuiInfo.FontWeight = GuiData->GuiInfo.FontWeight;
    pSharedInfo->ci.u.GuiInfo.UseRasterFonts = GuiData->GuiInfo.UseRasterFonts;
    /// pSharedInfo->ci.u.GuiInfo.WindowPosition = GuiData->GuiInfo.WindowPosition;
    pSharedInfo->ci.u.GuiInfo.AutoPosition = GuiData->GuiInfo.AutoPosition;
    pSharedInfo->ci.u.GuiInfo.WindowOrigin = GuiData->GuiInfo.WindowOrigin;
    /* Palette */
    memcpy(pSharedInfo->ci.Colors, Console->Colors, sizeof(s_Colors)); // FIXME: Possible buffer overflow if s_colors is bigger than pSharedInfo->Colors.
    /* Title of the console, original one corresponding to the one set by the console leader */
    Length = min(sizeof(pSharedInfo->ci.ConsoleTitle) / sizeof(pSharedInfo->ci.ConsoleTitle[0]) - 1,
                 Console->OriginalTitle.Length / sizeof(WCHAR));
    wcsncpy(pSharedInfo->ci.ConsoleTitle, Console->OriginalTitle.Buffer, Length);
    pSharedInfo->ci.ConsoleTitle[Length] = L'\0';

    /* Unmap the view */
    NtUnmapViewOfSection(NtCurrentProcess(), pSharedInfo);

    /* Get the console leader process, our client */
    ProcessData = CONTAINING_RECORD(Console->ProcessList.Blink,
                                    CONSOLE_PROCESS_DATA,
                                    ConsoleLink);

    /* Duplicate the section handle for the client */
    Status = NtDuplicateObject(NtCurrentProcess(),
                               hSection,
                               ProcessData->Process->ProcessHandle,
                               &hClientSection,
                               0, 0, DUPLICATE_SAME_ACCESS);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Error: Impossible to duplicate section handle for client ; Status = %lu\n", Status);
        NtClose(hSection);
        return;
    }

    /* Start the properties dialog */
    if (ProcessData->PropDispatcher)
    {
        HANDLE Thread;

        Thread = CreateRemoteThread(ProcessData->Process->ProcessHandle, NULL, 0,
                                    ProcessData->PropDispatcher,
                                    (PVOID)hClientSection, 0, NULL);
        if (NULL == Thread)
        {
            DPRINT1("Failed thread creation (Error: 0x%x)\n", GetLastError());
            return;
        }

        DPRINT1("We succeeded at creating ProcessData->PropDispatcher remote thread, ProcessId = %x, Process = 0x%p\n", ProcessData->Process->ClientId.UniqueProcess, ProcessData->Process);
        /// WaitForSingleObject(Thread, INFINITE);
        CloseHandle(Thread);
    }

    /* We have finished, close the section handle */
    NtClose(hSection);
    return;
}


static NTSTATUS WINAPI
GuiResizeBuffer(PCONSOLE Console, PCONSOLE_SCREEN_BUFFER ScreenBuffer, COORD Size);
VOID FASTCALL
GuiConsoleInitScrollbar(PCONSOLE Console, HWND hwnd);

static VOID
GuiApplyUserSettings(PCONSOLE Console,
                     HANDLE hClientSection,
                     BOOL SaveSettings)
{
    NTSTATUS Status;
    PCONSOLE_PROCESS_DATA ProcessData;
    HANDLE hSection = NULL;
    ULONG ViewSize = 0;
    PCONSOLE_PROPS pConInfo = NULL;
    PCONSOLE_SCREEN_BUFFER ActiveBuffer = Console->ActiveBuffer;
    COORD BufSize;
    BOOL SizeChanged = FALSE;

    /// LOCK /// EnterCriticalSection(&Console->Lock);

    /* Get the console leader process, our client */
    ProcessData = CONTAINING_RECORD(Console->ProcessList.Blink,
                                    CONSOLE_PROCESS_DATA,
                                    ConsoleLink);

    /* Duplicate the section handle for ourselves */
    Status = NtDuplicateObject(ProcessData->Process->ProcessHandle,
                               hClientSection,
                               NtCurrentProcess(),
                               &hSection,
                               0, 0, DUPLICATE_SAME_ACCESS);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Error when mapping client handle, Status = %lu\n", Status);
        return;
    }

    /* Get a view of the shared section */
    Status = NtMapViewOfSection(hSection,
                                NtCurrentProcess(),
                                (PVOID*)&pConInfo,
                                0,
                                0,
                                NULL,
                                &ViewSize,
                                ViewUnmap,
                                0,
                                PAGE_READONLY);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Error when mapping view of file, Status = %lu\n", Status);
        NtClose(hSection);
        return;
    }

    /* Check that the section is well-sized */
    if (ViewSize < sizeof(CONSOLE_PROPS))
    {
        DPRINT1("Error: section bad-sized: sizeof(Section) < sizeof(CONSOLE_PROPS)\n");
        NtUnmapViewOfSection(NtCurrentProcess(), pConInfo);
        NtClose(hSection);
        return;
    }

    /*
     * Apply foreground and background colors for both screen and popup.
     * Copy the new palette.
     * TODO: Really update the screen attributes as FillConsoleOutputAttribute does.
     */
    ActiveBuffer->ScreenDefaultAttrib = pConInfo->ci.ScreenAttrib;
    ActiveBuffer->PopupDefaultAttrib  = pConInfo->ci.PopupAttrib;
    memcpy(Console->Colors, pConInfo->ci.Colors, sizeof(s_Colors)); // FIXME: Possible buffer overflow if s_colors is bigger than pConInfo->Colors.

    /* Apply cursor size */
    ActiveBuffer->CursorInfo.dwSize = min(max(pConInfo->ci.CursorSize, 1), 100);

    if (pConInfo->ci.ConsoleSize.X != Console->Size.X ||
        pConInfo->ci.ConsoleSize.Y != Console->Size.Y)
    {
        /* Resize window */
        Console->Size = pConInfo->ci.ConsoleSize;
        SizeChanged = TRUE;
    }

    BufSize = pConInfo->ci.ScreenBufferSize;
    if (BufSize.X != ActiveBuffer->ScreenBufferSize.X || BufSize.Y != ActiveBuffer->ScreenBufferSize.Y)
    {
        if (NT_SUCCESS(GuiResizeBuffer(Console, ActiveBuffer, BufSize)))
            SizeChanged = TRUE;
    }

    if (SizeChanged)
    {
        PGUI_CONSOLE_DATA GuiData = Console->GuiData;
        if (GuiData)
        {
            GuiData->WindowSizeLock = TRUE;
            GuiConsoleInitScrollbar(Console, pConInfo->hConsoleWindow);
            GuiData->WindowSizeLock = FALSE;
        }
    }

    /// LOCK /// LeaveCriticalSection(&Console->Lock);
    InvalidateRect(pConInfo->hConsoleWindow, NULL, TRUE);

    /* Save settings if needed */
    // FIXME: Do it in the console properties applet ??
    if (SaveSettings)
    {
        DWORD ProcessId = HandleToUlong(ProcessData->Process->ClientId.UniqueProcess);
        ConSrvWriteUserSettings(&pConInfo->ci, ProcessId);
    }

    /* Finally, close the section */
    NtUnmapViewOfSection(NtCurrentProcess(), pConInfo);
    NtClose(hSection);
}

static PCONSOLE
GuiGetWindowConsole(HWND hWnd)
{
    return (PCONSOLE)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
}

VOID
FASTCALL
GuiConsoleInitScrollbar(PCONSOLE Console, HWND hwnd)
{
    SCROLLINFO sInfo;
    PGUI_CONSOLE_DATA GuiData = Console->GuiData;

    DWORD Width = Console->Size.X * GuiData->CharWidth + 2 * (GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXEDGE));
    DWORD Height = Console->Size.Y * GuiData->CharHeight + 2 * (GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CYEDGE)) + GetSystemMetrics(SM_CYCAPTION);

    /* set scrollbar sizes */
    sInfo.cbSize = sizeof(SCROLLINFO);
    sInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    sInfo.nMin = 0;
    if (Console->ActiveBuffer->ScreenBufferSize.Y > Console->Size.Y)
    {
        sInfo.nMax = Console->ActiveBuffer->ScreenBufferSize.Y - 1;
        sInfo.nPage = Console->Size.Y;
        sInfo.nPos = Console->ActiveBuffer->ShowY;
        SetScrollInfo(hwnd, SB_VERT, &sInfo, TRUE);
        Width += GetSystemMetrics(SM_CXVSCROLL);
        ShowScrollBar(hwnd, SB_VERT, TRUE);
    }
    else
    {
        ShowScrollBar(hwnd, SB_VERT, FALSE);
    }

    if (Console->ActiveBuffer->ScreenBufferSize.X > Console->Size.X)
    {
        sInfo.nMax = Console->ActiveBuffer->ScreenBufferSize.X - 1;
        sInfo.nPage = Console->Size.X;
        sInfo.nPos = Console->ActiveBuffer->ShowX;
        SetScrollInfo(hwnd, SB_HORZ, &sInfo, TRUE);
        Height += GetSystemMetrics(SM_CYHSCROLL);
        ShowScrollBar(hwnd, SB_HORZ, TRUE);

    }
    else
    {
        ShowScrollBar(hwnd, SB_HORZ, FALSE);
    }

    SetWindowPos(hwnd, NULL, 0, 0, Width, Height,
                 SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
}

static BOOL
GuiConsoleHandleNcCreate(HWND hWnd, LPCREATESTRUCTW Create)
{
    PCONSOLE Console = (PCONSOLE)Create->lpCreateParams;
    PGUI_CONSOLE_DATA GuiData = Console->GuiData;
    HDC Dc;
    HFONT OldFont;
    TEXTMETRICW Metrics;
    SIZE CharSize;

    Console->hWindow = hWnd;

    if (NULL == GuiData)
    {
        DPRINT1("GuiConsoleNcCreate: RtlAllocateHeap failed\n");
        return FALSE;
    }

    InitializeCriticalSection(&GuiData->Lock);

    GuiData->Font = CreateFontW(LOWORD(GuiData->GuiInfo.FontSize),
                                0, // HIWORD(GuiData->GuiInfo.FontSize),
                                0,
                                TA_BASELINE,
                                GuiData->GuiInfo.FontWeight,
                                FALSE,
                                FALSE,
                                FALSE,
                                OEM_CHARSET,
                                OUT_DEFAULT_PRECIS,
                                CLIP_DEFAULT_PRECIS,
                                NONANTIALIASED_QUALITY,
                                FIXED_PITCH | GuiData->GuiInfo.FontFamily /* FF_DONTCARE */,
                                GuiData->GuiInfo.FaceName);
    if (NULL == GuiData->Font)
    {
        DPRINT1("GuiConsoleNcCreate: CreateFont failed\n");
        DeleteCriticalSection(&GuiData->Lock);
        RtlFreeHeap(ConSrvHeap, 0, GuiData);
        return FALSE;
    }
    Dc = GetDC(hWnd);
    if (NULL == Dc)
    {
        DPRINT1("GuiConsoleNcCreate: GetDC failed\n");
        DeleteObject(GuiData->Font);
        DeleteCriticalSection(&GuiData->Lock);
        RtlFreeHeap(ConSrvHeap, 0, GuiData);
        return FALSE;
    }
    OldFont = SelectObject(Dc, GuiData->Font);
    if (NULL == OldFont)
    {
        DPRINT1("GuiConsoleNcCreate: SelectObject failed\n");
        ReleaseDC(hWnd, Dc);
        DeleteObject(GuiData->Font);
        DeleteCriticalSection(&GuiData->Lock);
        RtlFreeHeap(ConSrvHeap, 0, GuiData);
        return FALSE;
    }
    if (!GetTextMetricsW(Dc, &Metrics))
    {
        DPRINT1("GuiConsoleNcCreate: GetTextMetrics failed\n");
        SelectObject(Dc, OldFont);
        ReleaseDC(hWnd, Dc);
        DeleteObject(GuiData->Font);
        DeleteCriticalSection(&GuiData->Lock);
        RtlFreeHeap(ConSrvHeap, 0, GuiData);
        return FALSE;
    }
    GuiData->CharWidth = Metrics.tmMaxCharWidth;
    GuiData->CharHeight = Metrics.tmHeight + Metrics.tmExternalLeading;

    /* Measure real char width more precisely if possible. */
    if (GetTextExtentPoint32W(Dc, L"R", 1, &CharSize))
        GuiData->CharWidth = CharSize.cx;

    SelectObject(Dc, OldFont);

    ReleaseDC(hWnd, Dc);

    // FIXME: Keep these instructions here ? ///////////////////////////////////
    Console->ActiveBuffer->CursorBlinkOn = TRUE;
    Console->ActiveBuffer->ForceCursorOff = FALSE;
    ////////////////////////////////////////////////////////////////////////////

    DPRINT("Console %p GuiData %p\n", Console, GuiData);
    SetWindowLongPtrW(hWnd, GWLP_USERDATA, (DWORD_PTR)Console);

    SetTimer(hWnd, CONGUI_UPDATE_TIMER, CONGUI_UPDATE_TIME, NULL);
    GuiConsoleCreateSysMenu(Console);

    GuiData->WindowSizeLock = TRUE;
    GuiConsoleInitScrollbar(Console, hWnd);
    GuiData->WindowSizeLock = FALSE;

    SetEvent(GuiData->hGuiInitEvent);

    return (BOOL)DefWindowProcW(hWnd, WM_NCCREATE, 0, (LPARAM)Create);
}

static VOID
SmallRectToRect(PCONSOLE Console, PRECT Rect, PSMALL_RECT SmallRect)
{
    PCONSOLE_SCREEN_BUFFER Buffer = Console->ActiveBuffer;
    PGUI_CONSOLE_DATA GuiData = Console->GuiData;
    Rect->left   = (SmallRect->Left       - Buffer->ShowX) * GuiData->CharWidth;
    Rect->top    = (SmallRect->Top        - Buffer->ShowY) * GuiData->CharHeight;
    Rect->right  = (SmallRect->Right  + 1 - Buffer->ShowX) * GuiData->CharWidth;
    Rect->bottom = (SmallRect->Bottom + 1 - Buffer->ShowY) * GuiData->CharHeight;
}

static VOID
GuiConsoleUpdateSelection(PCONSOLE Console, PCOORD coord)
{
    RECT oldRect, newRect;
    HWND hWnd = Console->hWindow;

    SmallRectToRect(Console, &oldRect, &Console->Selection.srSelection);

    if(coord != NULL)
    {
        SMALL_RECT rc;
        /* exchange left/top with right/bottom if required */
        rc.Left   = min(Console->Selection.dwSelectionAnchor.X, coord->X);
        rc.Top    = min(Console->Selection.dwSelectionAnchor.Y, coord->Y);
        rc.Right  = max(Console->Selection.dwSelectionAnchor.X, coord->X);
        rc.Bottom = max(Console->Selection.dwSelectionAnchor.Y, coord->Y);

        SmallRectToRect(Console, &newRect, &rc);

        if (Console->Selection.dwFlags & CONSOLE_SELECTION_NOT_EMPTY)
        {
            if (memcmp(&rc, &Console->Selection.srSelection, sizeof(SMALL_RECT)) != 0)
            {
                HRGN rgn1, rgn2;

                /* calculate the region that needs to be updated */
                if((rgn1 = CreateRectRgnIndirect(&oldRect)))
                {
                    if((rgn2 = CreateRectRgnIndirect(&newRect)))
                    {
                        if(CombineRgn(rgn1, rgn2, rgn1, RGN_XOR) != ERROR)
                        {
                            InvalidateRgn(hWnd, rgn1, FALSE);
                        }

                        DeleteObject(rgn2);
                    }
                    DeleteObject(rgn1);
                }
            }
        }
        else
        {
            InvalidateRect(hWnd, &newRect, FALSE);
        }
        Console->Selection.dwFlags |= CONSOLE_SELECTION_NOT_EMPTY;
        Console->Selection.srSelection = rc;
        ConioPause(Console, PAUSED_FROM_SELECTION);
    }
    else
    {
        /* clear the selection */
        if (Console->Selection.dwFlags & CONSOLE_SELECTION_NOT_EMPTY)
        {
            InvalidateRect(hWnd, &oldRect, FALSE);
        }
        Console->Selection.dwFlags = CONSOLE_NO_SELECTION;
        ConioUnpause(Console, PAUSED_FROM_SELECTION);
    }
}

static VOID
GuiConsolePaint(PCONSOLE Console,
                PGUI_CONSOLE_DATA GuiData,
                HDC hDC,
                PRECT rc)
{
    PCONSOLE_SCREEN_BUFFER Buff;
    ULONG TopLine, BottomLine, LeftChar, RightChar;
    ULONG Line, Char, Start;
    PBYTE From;
    PWCHAR To;
    BYTE LastAttribute, Attribute;
    ULONG CursorX, CursorY, CursorHeight;
    HBRUSH CursorBrush, OldBrush;
    HFONT OldFont;

    Buff = Console->ActiveBuffer;

    /// LOCK /// EnterCriticalSection(&Buff->Header.Console->Lock);

    TopLine = rc->top / GuiData->CharHeight + Buff->ShowY;
    BottomLine = (rc->bottom + (GuiData->CharHeight - 1)) / GuiData->CharHeight - 1 + Buff->ShowY;
    LeftChar = rc->left / GuiData->CharWidth + Buff->ShowX;
    RightChar = (rc->right + (GuiData->CharWidth - 1)) / GuiData->CharWidth - 1 + Buff->ShowX;
    LastAttribute = ConioCoordToPointer(Buff, LeftChar, TopLine)[1];

    SetTextColor(hDC, RGBFromAttrib(Console, TextAttribFromAttrib(LastAttribute)));
    SetBkColor(hDC, RGBFromAttrib(Console, BkgdAttribFromAttrib(LastAttribute)));

    if (BottomLine >= Buff->ScreenBufferSize.Y) BottomLine = Buff->ScreenBufferSize.Y - 1;
    if (RightChar >= Buff->ScreenBufferSize.X) RightChar = Buff->ScreenBufferSize.X - 1;

    OldFont = SelectObject(hDC,
                           GuiData->Font);

    for (Line = TopLine; Line <= BottomLine; Line++)
    {
        WCHAR LineBuffer[80];
        From = ConioCoordToPointer(Buff, LeftChar, Line);
        Start = LeftChar;
        To = LineBuffer;

        for (Char = LeftChar; Char <= RightChar; Char++)
        {
            if (*(From + 1) != LastAttribute || (Char - Start == sizeof(LineBuffer) / sizeof(WCHAR)))
            {
                TextOutW(hDC,
                         (Start - Buff->ShowX) * GuiData->CharWidth,
                         (Line - Buff->ShowY) * GuiData->CharHeight,
                         LineBuffer,
                         Char - Start);
                Start = Char;
                To = LineBuffer;
                Attribute = *(From + 1);
                if (Attribute != LastAttribute)
                {
                    SetTextColor(hDC, RGBFromAttrib(Console, TextAttribFromAttrib(Attribute)));
                    SetBkColor(hDC, RGBFromAttrib(Console, BkgdAttribFromAttrib(Attribute)));
                    LastAttribute = Attribute;
                }
            }

            MultiByteToWideChar(Console->OutputCodePage,
                                0,
                                (PCHAR)From,
                                1,
                                To,
                                1);
            To++;
            From += 2;
        }

        TextOutW(hDC,
                 (Start - Buff->ShowX) * GuiData->CharWidth,
                 (Line - Buff->ShowY) * GuiData->CharHeight,
                 LineBuffer,
                 RightChar - Start + 1);
    }

    if (Buff->CursorInfo.bVisible && Buff->CursorBlinkOn &&
            !Buff->ForceCursorOff)
    {
        CursorX = Buff->CursorPosition.X;
        CursorY = Buff->CursorPosition.Y;
        if (LeftChar <= CursorX && CursorX <= RightChar &&
                TopLine <= CursorY && CursorY <= BottomLine)
        {
            CursorHeight = ConioEffectiveCursorSize(Console, GuiData->CharHeight);
            From = ConioCoordToPointer(Buff, Buff->CursorPosition.X, Buff->CursorPosition.Y) + 1;

            if (*From != DEFAULT_SCREEN_ATTRIB)
            {
                CursorBrush = CreateSolidBrush(RGBFromAttrib(Console, *From));
            }
            else
            {
                CursorBrush = CreateSolidBrush(RGBFromAttrib(Console, Buff->ScreenDefaultAttrib));
            }

            OldBrush = SelectObject(hDC,
                                    CursorBrush);
            PatBlt(hDC,
                   (CursorX - Buff->ShowX) * GuiData->CharWidth,
                   (CursorY - Buff->ShowY) * GuiData->CharHeight + (GuiData->CharHeight - CursorHeight),
                   GuiData->CharWidth,
                   CursorHeight,
                   PATCOPY);
            SelectObject(hDC,
                         OldBrush);
            DeleteObject(CursorBrush);
        }
    }

    /// LOCK /// LeaveCriticalSection(&Buff->Header.Console->Lock);

    SelectObject(hDC,
                 OldFont);
}

static VOID
GuiConsoleHandlePaint(PCONSOLE Console,
                      HWND hWnd,
                      HDC hDCPaint)
{
    PGUI_CONSOLE_DATA GuiData = Console->GuiData;
    HDC hDC;
    PAINTSTRUCT ps;

    if (GuiData == NULL) return;
    if (Console->ActiveBuffer == NULL) return;

    hDC = BeginPaint(hWnd, &ps);
    if (hDC != NULL &&
            ps.rcPaint.left < ps.rcPaint.right &&
            ps.rcPaint.top < ps.rcPaint.bottom)
    {
        if (Console->ActiveBuffer->Buffer != NULL)
        {
            EnterCriticalSection(&GuiData->Lock);

            GuiConsolePaint(Console,
                            GuiData,
                            hDC,
                            &ps.rcPaint);

            if (Console->Selection.dwFlags & CONSOLE_SELECTION_NOT_EMPTY)
            {
                RECT rc;
                SmallRectToRect(Console, &rc, &Console->Selection.srSelection);

                /* invert the selection */
                if (IntersectRect(&rc,
                                  &ps.rcPaint,
                                  &rc))
                {
                    PatBlt(hDC,
                           rc.left,
                           rc.top,
                           rc.right - rc.left,
                           rc.bottom - rc.top,
                           DSTINVERT);
                }
            }

            LeaveCriticalSection(&GuiData->Lock);
        }
    }
    EndPaint(hWnd, &ps);
}

static VOID
GuiConsoleHandleKey(PCONSOLE Console, HWND hWnd,
                    UINT msg, WPARAM wParam, LPARAM lParam)
{
    MSG Message;

    Message.hwnd = hWnd;
    Message.message = msg;
    Message.wParam = wParam;
    Message.lParam = lParam;

    if(msg == WM_CHAR || msg == WM_SYSKEYDOWN)
    {
        /* clear the selection */
        GuiConsoleUpdateSelection(Console, NULL);
    }

    ConioProcessKey(Console, &Message);
}

static BOOL WINAPI
GuiProcessKeyCallback(PCONSOLE Console, MSG* msg, BYTE KeyStateMenu, DWORD ShiftState, UINT VirtualKeyCode, BOOL Down)
{
    if ((ShiftState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED) || KeyStateMenu & 0x80) &&
        (VirtualKeyCode == VK_ESCAPE || VirtualKeyCode == VK_TAB || VirtualKeyCode == VK_SPACE))
    {
        DefWindowProcW(msg->hwnd, msg->message, msg->wParam, msg->lParam);
        return TRUE;
    }

    return FALSE;
}

static VOID WINAPI
GuiDrawRegion(PCONSOLE Console, SMALL_RECT* Region)
{
    RECT RegionRect;
    SmallRectToRect(Console, &RegionRect, Region);
    InvalidateRect(Console->hWindow, &RegionRect, FALSE);
}

static VOID
GuiInvalidateCell(PCONSOLE Console, UINT x, UINT y)
{
    SMALL_RECT CellRect = { x, y, x, y };
    GuiDrawRegion(Console, &CellRect);
}

static VOID WINAPI
GuiWriteStream(PCONSOLE Console, SMALL_RECT* Region, LONG CursorStartX, LONG CursorStartY,
               UINT ScrolledLines, CHAR *Buffer, UINT Length)
{
    PGUI_CONSOLE_DATA GuiData = Console->GuiData;
    PCONSOLE_SCREEN_BUFFER Buff = Console->ActiveBuffer;
    LONG CursorEndX, CursorEndY;
    RECT ScrollRect;

    if (NULL == Console->hWindow || NULL == GuiData)
    {
        return;
    }

    if (0 != ScrolledLines)
    {
        ScrollRect.left = 0;
        ScrollRect.top = 0;
        ScrollRect.right = Console->Size.X * GuiData->CharWidth;
        ScrollRect.bottom = Region->Top * GuiData->CharHeight;

        ScrollWindowEx(Console->hWindow,
                       0,
                       -(ScrolledLines * GuiData->CharHeight),
                       &ScrollRect,
                       NULL,
                       NULL,
                       NULL,
                       SW_INVALIDATE);
    }

    GuiDrawRegion(Console, Region);

    if (CursorStartX < Region->Left || Region->Right < CursorStartX
            || CursorStartY < Region->Top || Region->Bottom < CursorStartY)
    {
        GuiInvalidateCell(Console, CursorStartX, CursorStartY);
    }

    CursorEndX = Buff->CursorPosition.X;
    CursorEndY = Buff->CursorPosition.Y;
    if ((CursorEndX < Region->Left || Region->Right < CursorEndX
            || CursorEndY < Region->Top || Region->Bottom < CursorEndY)
            && (CursorEndX != CursorStartX || CursorEndY != CursorStartY))
    {
        GuiInvalidateCell(Console, CursorEndX, CursorEndY);
    }

    // Set up the update timer (very short interval) - this is a "hack" for getting the OS to
    // repaint the window without having it just freeze up and stay on the screen permanently.
    Buff->CursorBlinkOn = TRUE;
    SetTimer(Console->hWindow, CONGUI_UPDATE_TIMER, CONGUI_UPDATE_TIME, NULL);
}

static BOOL WINAPI
GuiSetCursorInfo(PCONSOLE Console, PCONSOLE_SCREEN_BUFFER Buff)
{
    if (Console->ActiveBuffer == Buff)
    {
        GuiInvalidateCell(Console, Buff->CursorPosition.X, Buff->CursorPosition.Y);
    }

    return TRUE;
}

static BOOL WINAPI
GuiSetScreenInfo(PCONSOLE Console, PCONSOLE_SCREEN_BUFFER Buff, UINT OldCursorX, UINT OldCursorY)
{
    if (Console->ActiveBuffer == Buff)
    {
        /* Redraw char at old position (removes cursor) */
        GuiInvalidateCell(Console, OldCursorX, OldCursorY);
        /* Redraw char at new position (shows cursor) */
        GuiInvalidateCell(Console, Buff->CursorPosition.X, Buff->CursorPosition.Y);
    }

    return TRUE;
}

static BOOL WINAPI
GuiUpdateScreenInfo(PCONSOLE Console, PCONSOLE_SCREEN_BUFFER Buff)
{
    return TRUE;
}

static VOID
GuiConsoleHandleTimer(PCONSOLE Console, HWND hWnd)
{
    PGUI_CONSOLE_DATA GuiData = Console->GuiData;
    PCONSOLE_SCREEN_BUFFER Buff;

    if (GuiData == NULL) return;

    SetTimer(hWnd, CONGUI_UPDATE_TIMER, CURSOR_BLINK_TIME, NULL);

    Buff = Console->ActiveBuffer;
    GuiInvalidateCell(Console, Buff->CursorPosition.X, Buff->CursorPosition.Y);
    Buff->CursorBlinkOn = !Buff->CursorBlinkOn;

    if((GuiData->OldCursor.x != Buff->CursorPosition.X) || (GuiData->OldCursor.y != Buff->CursorPosition.Y))
    {
        SCROLLINFO xScroll;
        int OldScrollX = -1, OldScrollY = -1;
        int NewScrollX = -1, NewScrollY = -1;

        xScroll.cbSize = sizeof(SCROLLINFO);
        xScroll.fMask = SIF_POS;
        // Capture the original position of the scroll bars and save them.
        if(GetScrollInfo(hWnd, SB_HORZ, &xScroll))OldScrollX = xScroll.nPos;
        if(GetScrollInfo(hWnd, SB_VERT, &xScroll))OldScrollY = xScroll.nPos;

        // If we successfully got the info for the horizontal scrollbar
        if(OldScrollX >= 0)
        {
            if((Buff->CursorPosition.X < Buff->ShowX)||(Buff->CursorPosition.X >= (Buff->ShowX + Console->Size.X)))
            {
                // Handle the horizontal scroll bar
                if(Buff->CursorPosition.X >= Console->Size.X) NewScrollX = Buff->CursorPosition.X - Console->Size.X + 1;
                else NewScrollX = 0;
            }
            else
            {
                NewScrollX = OldScrollX;
            }
        }
        // If we successfully got the info for the vertical scrollbar
        if(OldScrollY >= 0)
        {
            if((Buff->CursorPosition.Y < Buff->ShowY) || (Buff->CursorPosition.Y >= (Buff->ShowY + Console->Size.Y)))
            {
                // Handle the vertical scroll bar
                if(Buff->CursorPosition.Y >= Console->Size.Y) NewScrollY = Buff->CursorPosition.Y - Console->Size.Y + 1;
                else NewScrollY = 0;
            }
            else
            {
                NewScrollY = OldScrollY;
            }
        }

        // Adjust scroll bars and refresh the window if the cursor has moved outside the visible area
        // NOTE: OldScroll# and NewScroll# will both be -1 (initial value) if the info for the respective scrollbar
        //       was not obtained successfully in the previous steps. This means their difference is 0 (no scrolling)
        //       and their associated scrollbar is left alone.
        if((OldScrollX != NewScrollX) || (OldScrollY != NewScrollY))
        {
            Buff->ShowX = NewScrollX;
            Buff->ShowY = NewScrollY;
            ScrollWindowEx(hWnd,
                           (OldScrollX - NewScrollX) * GuiData->CharWidth,
                           (OldScrollY - NewScrollY) * GuiData->CharHeight,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           SW_INVALIDATE);
            if(NewScrollX >= 0)
            {
                xScroll.nPos = NewScrollX;
                SetScrollInfo(hWnd, SB_HORZ, &xScroll, TRUE);
            }
            if(NewScrollY >= 0)
            {
                xScroll.nPos = NewScrollY;
                SetScrollInfo(hWnd, SB_VERT, &xScroll, TRUE);
            }
            UpdateWindow(hWnd);
            GuiData->OldCursor.x = Buff->CursorPosition.X;
            GuiData->OldCursor.y = Buff->CursorPosition.Y;
        }
    }
}

static VOID
GuiConsoleHandleClose(PCONSOLE Console, HWND hWnd)
{
    PLIST_ENTRY current_entry;
    PCONSOLE_PROCESS_DATA current;

    /// LOCK /// EnterCriticalSection(&Console->Lock);

    /*
     * Loop through the process list, from the most recent process
     * (the active one) to the oldest one (the first created,
     * i.e. the console leader process), and for each, send a
     * CTRL-CLOSE event (new processes are inserted at the head
     * of the console process list).
     */
    current_entry = Console->ProcessList.Flink;
    while (current_entry != &Console->ProcessList)
    {
        current = CONTAINING_RECORD(current_entry, CONSOLE_PROCESS_DATA, ConsoleLink);
        current_entry = current_entry->Flink;

        /* FIXME: Windows will wait up to 5 seconds for the thread to exit.
         * We shouldn't wait here, though, since the console lock is entered.
         * A copy of the thread list probably needs to be made. */
        ConSrvConsoleCtrlEvent(CTRL_CLOSE_EVENT, current);
    }

    /// LOCK /// LeaveCriticalSection(&Console->Lock);
}

static VOID
GuiConsoleHandleNcDestroy(PCONSOLE Console, HWND hWnd)
{
    PGUI_CONSOLE_DATA GuiData = Console->GuiData;

    KillTimer(hWnd, 1);
    Console->GuiData = NULL;
    DeleteCriticalSection(&GuiData->Lock);
    GetSystemMenu(hWnd, TRUE);

    RtlFreeHeap(ConSrvHeap, 0, GuiData);
}

static COORD
PointToCoord(PCONSOLE Console, LPARAM lParam)
{
    PGUI_CONSOLE_DATA GuiData = Console->GuiData;
    PCONSOLE_SCREEN_BUFFER Buffer = Console->ActiveBuffer;
    COORD Coord;

    Coord.X = Buffer->ShowX + ((short)LOWORD(lParam) / (int)GuiData->CharWidth);
    Coord.Y = Buffer->ShowY + ((short)HIWORD(lParam) / (int)GuiData->CharHeight);

    /* Clip coordinate to ensure it's inside buffer */
    if (Coord.X < 0)
        Coord.X = 0;
    else if (Coord.X >= Buffer->ScreenBufferSize.X)
        Coord.X = Buffer->ScreenBufferSize.X - 1;

    if (Coord.Y < 0)
        Coord.Y = 0;
    else if (Coord.Y >= Buffer->ScreenBufferSize.Y)
        Coord.Y = Buffer->ScreenBufferSize.Y - 1;

    return Coord;
}

static VOID
GuiConsoleLeftMouseDown(PCONSOLE Console, HWND hWnd,
                        LPARAM lParam)
{
    Console->Selection.dwSelectionAnchor = PointToCoord(Console, lParam);

    SetCapture(hWnd);

    Console->Selection.dwFlags |= CONSOLE_SELECTION_IN_PROGRESS | CONSOLE_MOUSE_SELECTION | CONSOLE_MOUSE_DOWN;

    GuiConsoleUpdateSelection(Console, &Console->Selection.dwSelectionAnchor);
}

static VOID
GuiConsoleLeftMouseUp(PCONSOLE Console, HWND hWnd,
                      LPARAM lParam)
{
    COORD c;

    if (!(Console->Selection.dwFlags & CONSOLE_MOUSE_DOWN)) return;

    c = PointToCoord(Console, lParam);

    Console->Selection.dwFlags &= ~CONSOLE_MOUSE_DOWN;

    GuiConsoleUpdateSelection(Console, &c);

    ReleaseCapture();
}

static VOID
GuiConsoleMouseMove(PCONSOLE Console, HWND hWnd,
                    WPARAM wParam, LPARAM lParam)
{
    COORD c;

    if (!(wParam & MK_LBUTTON)) return;

    if (!(Console->Selection.dwFlags & CONSOLE_MOUSE_DOWN)) return;

    c = PointToCoord(Console, lParam); /* TODO: Scroll buffer to bring c into view */

    GuiConsoleUpdateSelection(Console, &c);
}

static VOID
GuiConsoleCopy(HWND hWnd, PCONSOLE Console)
{
    if (OpenClipboard(hWnd) == TRUE)
    {
        HANDLE hData;
        PBYTE ptr;
        LPSTR data, dstPos;
        ULONG selWidth, selHeight;
        ULONG xPos, yPos, size;

        selWidth = Console->Selection.srSelection.Right - Console->Selection.srSelection.Left + 1;
        selHeight = Console->Selection.srSelection.Bottom - Console->Selection.srSelection.Top + 1;
        DPRINT("Selection is (%d|%d) to (%d|%d)\n",
               Console->Selection.srSelection.Left,
               Console->Selection.srSelection.Top,
               Console->Selection.srSelection.Right,
               Console->Selection.srSelection.Bottom);

        /* Basic size for one line and termination */
        size = selWidth + 1;
        if (selHeight > 0)
        {
            /* Multiple line selections have to get \r\n appended */
            size += ((selWidth + 2) * (selHeight - 1));
        }

        /* Allocate memory, it will be passed to the system and may not be freed here */
        hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, size);
        if (hData == NULL)
        {
            CloseClipboard();
            return;
        }
        data = GlobalLock(hData);
        if (data == NULL)
        {
            CloseClipboard();
            return;
        }

        DPRINT("Copying %dx%d selection\n", selWidth, selHeight);
        dstPos = data;

        for (yPos = 0; yPos < selHeight; yPos++)
        {
            ptr = ConioCoordToPointer(Console->ActiveBuffer, 
                                      Console->Selection.srSelection.Left,
                                      yPos + Console->Selection.srSelection.Top);
            /* Copy only the characters, leave attributes alone */
            for (xPos = 0; xPos < selWidth; xPos++)
            {
                dstPos[xPos] = ptr[xPos * 2];
            }
            dstPos += selWidth;
            if (yPos != (selHeight - 1))
            {
                strcat(data, "\r\n");
                dstPos += 2;
            }
        }

        DPRINT("Setting data <%s> to clipboard\n", data);
        GlobalUnlock(hData);

        EmptyClipboard();
        SetClipboardData(CF_TEXT, hData);
        CloseClipboard();
    }
}

static VOID
GuiConsolePaste(HWND hWnd, PCONSOLE Console)
{
    if (OpenClipboard(hWnd) == TRUE)
    {
        HANDLE hData;
        LPSTR str;
        size_t len;

        hData = GetClipboardData(CF_TEXT);
        if (hData == NULL)
        {
            CloseClipboard();
            return;
        }

        str = GlobalLock(hData);
        if (str == NULL)
        {
            CloseClipboard();
            return;
        }
        DPRINT("Got data <%s> from clipboard\n", str);
        len = strlen(str);

        // TODO: Push the text into the input buffer.
        ConioWriteConsole(Console, Console->ActiveBuffer, str, len, TRUE);

        GlobalUnlock(hData);
        CloseClipboard();
    }
}

static VOID
GuiConsoleRightMouseDown(PCONSOLE Console, HWND hWnd)
{
    if (!(Console->Selection.dwFlags & CONSOLE_SELECTION_NOT_EMPTY))
    {
        GuiConsolePaste(hWnd, Console);
    }
    else
    {
        GuiConsoleCopy(hWnd, Console);

        /* Clear the selection */
        GuiConsoleUpdateSelection(Console, NULL);
    }
}

static VOID
GuiConsoleGetMinMaxInfo(PCONSOLE Console,
                        HWND hWnd,
                        PMINMAXINFO minMaxInfo)
{
    PGUI_CONSOLE_DATA GuiData = Console->GuiData;
    DWORD windx, windy;

    if (GuiData == NULL) return;

    windx = CONGUI_MIN_WIDTH * GuiData->CharWidth + 2 * (GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXEDGE));
    windy = CONGUI_MIN_HEIGHT * GuiData->CharHeight + 2 * (GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CYEDGE)) + GetSystemMetrics(SM_CYCAPTION);

    minMaxInfo->ptMinTrackSize.x = windx;
    minMaxInfo->ptMinTrackSize.y = windy;

    windx = (Console->ActiveBuffer->ScreenBufferSize.X) * GuiData->CharWidth  + 2 * (GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXEDGE));
    windy = (Console->ActiveBuffer->ScreenBufferSize.Y) * GuiData->CharHeight + 2 * (GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CYEDGE)) + GetSystemMetrics(SM_CYCAPTION);

    if(Console->Size.X < Console->ActiveBuffer->ScreenBufferSize.X) windy += GetSystemMetrics(SM_CYHSCROLL);    // window currently has a horizontal scrollbar
    if(Console->Size.Y < Console->ActiveBuffer->ScreenBufferSize.Y) windx += GetSystemMetrics(SM_CXVSCROLL);    // window currently has a vertical scrollbar

    minMaxInfo->ptMaxTrackSize.x = windx;
    minMaxInfo->ptMaxTrackSize.y = windy;
}

static VOID
GuiConsoleResize(PCONSOLE Console,
                 HWND hWnd,
                 WPARAM wParam, LPARAM lParam)
{
    PGUI_CONSOLE_DATA GuiData = Console->GuiData;

    if (GuiData == NULL) return;

    if ((GuiData->WindowSizeLock == FALSE) && (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED || wParam == SIZE_MINIMIZED))
    {
        PCONSOLE_SCREEN_BUFFER Buff = Console->ActiveBuffer;
        DWORD windx, windy, charx, chary;

        GuiData->WindowSizeLock = TRUE;

        windx = LOWORD(lParam);
        windy = HIWORD(lParam);

        // Compensate for existing scroll bars (because lParam values do not accommodate scroll bar)
        if(Console->Size.X < Buff->ScreenBufferSize.X) windy += GetSystemMetrics(SM_CYHSCROLL);    // window currently has a horizontal scrollbar
        if(Console->Size.Y < Buff->ScreenBufferSize.Y) windx += GetSystemMetrics(SM_CXVSCROLL);    // window currently has a vertical scrollbar

        charx = windx / GuiData->CharWidth;
        chary = windy / GuiData->CharHeight;

        // Character alignment (round size up or down)
        if((windx % GuiData->CharWidth) >= (GuiData->CharWidth / 2)) ++charx;
        if((windy % GuiData->CharHeight) >= (GuiData->CharHeight / 2)) ++chary;

        // Compensate for added scroll bars in new window
        if(charx < Buff->ScreenBufferSize.X)windy -= GetSystemMetrics(SM_CYHSCROLL);    // new window will have a horizontal scroll bar
        if(chary < Buff->ScreenBufferSize.Y)windx -= GetSystemMetrics(SM_CXVSCROLL);    // new window will have a vertical scroll bar

        charx = windx / GuiData->CharWidth;
        chary = windy / GuiData->CharHeight;

        // Character alignment (round size up or down)
        if((windx % GuiData->CharWidth) >= (GuiData->CharWidth / 2)) ++charx;
        if((windy % GuiData->CharHeight) >= (GuiData->CharHeight / 2)) ++chary;

        // Resize window
        if((charx != Console->Size.X) || (chary != Console->Size.Y))
        {
            Console->Size.X = (charx <= Buff->ScreenBufferSize.X) ? charx : Buff->ScreenBufferSize.X;
            Console->Size.Y = (chary <= Buff->ScreenBufferSize.Y) ? chary : Buff->ScreenBufferSize.Y;
        }

        GuiConsoleInitScrollbar(Console, hWnd);

        // Adjust the start of the visible area if we are attempting to show nonexistent areas
        if((Buff->ScreenBufferSize.X - Buff->ShowX) < Console->Size.X) Buff->ShowX = Buff->ScreenBufferSize.X - Console->Size.X;
        if((Buff->ScreenBufferSize.Y - Buff->ShowY) < Console->Size.Y) Buff->ShowY = Buff->ScreenBufferSize.Y - Console->Size.Y;
        InvalidateRect(hWnd, NULL, TRUE);

        GuiData->WindowSizeLock = FALSE;
    }
}

VOID
FASTCALL
GuiConsoleHandleScrollbarMenu(VOID)
{
    HMENU hMenu;

    hMenu = CreatePopupMenu();
    if (hMenu == NULL)
    {
        DPRINT("CreatePopupMenu failed\n");
        return;
    }
    //InsertItem(hMenu, MIIM_STRING, MIIM_ID | MIIM_FTYPE | MIIM_STRING, 0, NULL, IDS_SCROLLHERE);
    //InsertItem(hMenu, MFT_SEPARATOR, MIIM_FTYPE, 0, NULL, -1);
    //InsertItem(hMenu, MIIM_STRING, MIIM_ID | MIIM_FTYPE | MIIM_STRING, 0, NULL, IDS_SCROLLTOP);
    //InsertItem(hMenu, MIIM_STRING, MIIM_ID | MIIM_FTYPE | MIIM_STRING, 0, NULL, IDS_SCROLLBOTTOM);
    //InsertItem(hMenu, MFT_SEPARATOR, MIIM_FTYPE, 0, NULL, -1);
    //InsertItem(hMenu, MIIM_STRING, MIIM_ID | MIIM_FTYPE | MIIM_STRING, 0, NULL, IDS_SCROLLPAGE_UP);
    //InsertItem(hMenu, MIIM_STRING, MIIM_ID | MIIM_FTYPE | MIIM_STRING, 0, NULL, IDS_SCROLLPAGE_DOWN);
    //InsertItem(hMenu, MFT_SEPARATOR, MIIM_FTYPE, 0, NULL, -1);
    //InsertItem(hMenu, MIIM_STRING, MIIM_ID | MIIM_FTYPE | MIIM_STRING, 0, NULL, IDS_SCROLLUP);
    //InsertItem(hMenu, MIIM_STRING, MIIM_ID | MIIM_FTYPE | MIIM_STRING, 0, NULL, IDS_SCROLLDOWN);

}

static NTSTATUS WINAPI
GuiResizeBuffer(PCONSOLE Console, PCONSOLE_SCREEN_BUFFER ScreenBuffer, COORD Size)
{
    BYTE * Buffer;
    DWORD Offset = 0;
    BYTE * OldPtr;
    USHORT CurrentY;
    BYTE * OldBuffer;
#ifdef HAVE_WMEMSET
    USHORT value = MAKEWORD(' ', ScreenBuffer->ScreenDefaultAttrib);
#else
    DWORD i;
#endif
    DWORD diff;

    /* Buffer size is not allowed to be smaller than window size */
    if (Size.X < Console->Size.X || Size.Y < Console->Size.Y)
        return STATUS_INVALID_PARAMETER;

    if (Size.X == ScreenBuffer->ScreenBufferSize.X && Size.Y == ScreenBuffer->ScreenBufferSize.Y)
        return STATUS_SUCCESS;

    Buffer = RtlAllocateHeap(ConSrvHeap, 0, Size.X * Size.Y * 2);
    if (!Buffer)
        return STATUS_NO_MEMORY;

    DPRINT1("Resizing (%d,%d) to (%d,%d)\n", ScreenBuffer->ScreenBufferSize.X, ScreenBuffer->ScreenBufferSize.Y, Size.X, Size.Y);
    OldBuffer = ScreenBuffer->Buffer;

    for (CurrentY = 0; CurrentY < ScreenBuffer->ScreenBufferSize.Y && CurrentY < Size.Y; CurrentY++)
    {
        OldPtr = ConioCoordToPointer(ScreenBuffer, 0, CurrentY);
        if (Size.X <= ScreenBuffer->ScreenBufferSize.X)
        {
            /* reduce size */
            RtlCopyMemory(&Buffer[Offset], OldPtr, Size.X * 2);
            Offset += (Size.X * 2);
        }
        else
        {
            /* enlarge size */
            RtlCopyMemory(&Buffer[Offset], OldPtr, ScreenBuffer->ScreenBufferSize.X * 2);
            Offset += (ScreenBuffer->ScreenBufferSize.X * 2);

            diff = Size.X - ScreenBuffer->ScreenBufferSize.X;
            /* zero new part of it */
#ifdef HAVE_WMEMSET
            wmemset((PWCHAR)&Buffer[Offset], value, diff);
#else
            for (i = 0; i < diff; i++)
            {
                Buffer[Offset++] = ' ';
                Buffer[Offset++] = ScreenBuffer->ScreenDefaultAttrib;
            }
#endif
        }
    }

    if (Size.Y > ScreenBuffer->ScreenBufferSize.Y)
    {
        diff = Size.X * (Size.Y - ScreenBuffer->ScreenBufferSize.Y);
#ifdef HAVE_WMEMSET
        wmemset((PWCHAR)&Buffer[Offset], value, diff);
#else
        for (i = 0; i < diff; i++)
        {
            Buffer[Offset++] = ' ';
            Buffer[Offset++] = ScreenBuffer->ScreenDefaultAttrib;
        }
#endif
    }

    (void)InterlockedExchangePointer((PVOID volatile*)&ScreenBuffer->Buffer, Buffer);
    RtlFreeHeap(ConSrvHeap, 0, OldBuffer);
    ScreenBuffer->ScreenBufferSize = Size;
    ScreenBuffer->VirtualY = 0;

    /* Ensure cursor and window are within buffer */
    if (ScreenBuffer->CursorPosition.X >= Size.X)
        ScreenBuffer->CursorPosition.X = Size.X - 1;
    if (ScreenBuffer->CursorPosition.Y >= Size.Y)
        ScreenBuffer->CursorPosition.Y = Size.Y - 1;
    if (ScreenBuffer->ShowX > Size.X - Console->Size.X)
        ScreenBuffer->ShowX = Size.X - Console->Size.X;
    if (ScreenBuffer->ShowY > Size.Y - Console->Size.Y)
        ScreenBuffer->ShowY = Size.Y - Console->Size.Y;

    /* TODO: Should update scrollbar, but can't use anything that
     * calls SendMessage or it could cause deadlock --> Use PostMessage */

    return STATUS_SUCCESS;
}

static
LRESULT
GuiConsoleHandleScroll(PCONSOLE Console,
                       HWND hWnd,
                       UINT uMsg,
                       WPARAM wParam)
{
    PGUI_CONSOLE_DATA GuiData = Console->GuiData;
    PCONSOLE_SCREEN_BUFFER Buff;
    SCROLLINFO sInfo;
    int fnBar;
    int old_pos, Maximum;
    PUSHORT pShowXY;

    if (GuiData == NULL) return FALSE;

    Buff = Console->ActiveBuffer;

    if (uMsg == WM_HSCROLL)
    {
        fnBar = SB_HORZ;
        Maximum = Buff->ScreenBufferSize.X - Console->Size.X;
        pShowXY = &Buff->ShowX;
    }
    else
    {
        fnBar = SB_VERT;
        Maximum = Buff->ScreenBufferSize.Y - Console->Size.Y;
        pShowXY = &Buff->ShowY;
    }

    /* set scrollbar sizes */
    sInfo.cbSize = sizeof(SCROLLINFO);
    sInfo.fMask = SIF_RANGE | SIF_POS | SIF_PAGE | SIF_TRACKPOS;

    if (!GetScrollInfo(hWnd, fnBar, &sInfo))
    {
        return FALSE;
    }

    old_pos = sInfo.nPos;

    switch (LOWORD(wParam))
    {
        case SB_LINELEFT:
            sInfo.nPos -= 1;
            break;

        case SB_LINERIGHT:
            sInfo.nPos += 1;
            break;

        case SB_PAGELEFT:
            sInfo.nPos -= sInfo.nPage;
            break;

        case SB_PAGERIGHT:
            sInfo.nPos += sInfo.nPage;
            break;

        case SB_THUMBTRACK:
            sInfo.nPos = sInfo.nTrackPos;
            ConioPause(Console, PAUSED_FROM_SCROLLBAR);
            break;

        case SB_THUMBPOSITION:
            ConioUnpause(Console, PAUSED_FROM_SCROLLBAR);
            break;

        case SB_TOP:
            sInfo.nPos = sInfo.nMin;
            break;

        case SB_BOTTOM:
            sInfo.nPos = sInfo.nMax;
            break;

        default:
            break;
    }

    sInfo.nPos = max(sInfo.nPos, 0);
    sInfo.nPos = min(sInfo.nPos, Maximum);

    if (old_pos != sInfo.nPos)
    {
        USHORT OldX = Buff->ShowX;
        USHORT OldY = Buff->ShowY;
        *pShowXY = sInfo.nPos;

        ScrollWindowEx(hWnd,
                       (OldX - Buff->ShowX) * GuiData->CharWidth,
                       (OldY - Buff->ShowY) * GuiData->CharHeight,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       SW_INVALIDATE);

        sInfo.fMask = SIF_POS;
        SetScrollInfo(hWnd, fnBar, &sInfo, TRUE);

        UpdateWindow(hWnd);
    }

    return 0;
}

static LRESULT CALLBACK
GuiConsoleWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT Result = 0;
    PCONSOLE Console = NULL;

    /*
     * If it's the first time we create a window
     * for the console, just initialize it.
     */
    if (msg == WM_NCCREATE)
    {
        return (LRESULT)GuiConsoleHandleNcCreate(hWnd, (LPCREATESTRUCTW)lParam);
    }

    /*
     * Now the console window is initialized.
     * Get the console owned by the window.
     * If there is no console, just go away.
     */
    Console = GuiGetWindowConsole(hWnd);
    if (Console == NULL) return 0;

    // TODO: If the console is about to be destroyed, leave the loop.

    /* Lock the console */
    EnterCriticalSection(&Console->Lock);

    /* We have a console, start message dispatching. */
    switch (msg)
    {
        case WM_CLOSE:
            GuiConsoleHandleClose(Console, hWnd);
            break;

        case WM_NCDESTROY:
            GuiConsoleHandleNcDestroy(Console, hWnd);
            break;

        case WM_PAINT:
            GuiConsoleHandlePaint(Console, hWnd, (HDC)wParam);
            break;

        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_CHAR:
            GuiConsoleHandleKey(Console, hWnd, msg, wParam, lParam);
            break;

        case WM_TIMER:
            GuiConsoleHandleTimer(Console, hWnd);
            break;

        case WM_LBUTTONDOWN:
            GuiConsoleLeftMouseDown(Console, hWnd, lParam);
            break;

        case WM_LBUTTONUP:
            GuiConsoleLeftMouseUp(Console, hWnd, lParam);
            break;

        case WM_RBUTTONDOWN:
            GuiConsoleRightMouseDown(Console, hWnd);
            break;

        case WM_MOUSEMOVE:
            GuiConsoleMouseMove(Console, hWnd, wParam, lParam);
            break;

        case WM_SYSCOMMAND:
            Result = GuiConsoleHandleSysMenuCommand(Console, hWnd, wParam, lParam);
            break;

        case WM_HSCROLL:
        case WM_VSCROLL:
            Result = GuiConsoleHandleScroll(Console, hWnd, msg, wParam);
            break;

        case WM_GETMINMAXINFO:
            GuiConsoleGetMinMaxInfo(Console, hWnd, (PMINMAXINFO)lParam);
            break;

        case WM_SIZE:
            GuiConsoleResize(Console, hWnd, wParam, lParam);
            break;

        case PM_APPLY_CONSOLE_INFO:
            GuiApplyUserSettings(Console, (HANDLE)wParam, (BOOL)lParam);
            break;

        case PM_CONSOLE_BEEP:
            DPRINT1("Beep !!\n");
            Beep(800, 200);
            break;

        case PM_CONSOLE_SET_TITLE:
            SetWindowText(hWnd, Console->Title.Buffer);
            break;

        default:
            Result = DefWindowProcW(hWnd, msg, wParam, lParam);
            break;
    }

    /* Unlock the console */
    LeaveCriticalSection(&Console->Lock);

    return Result;
}

static LRESULT CALLBACK
GuiConsoleNotifyWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND NewWindow;
    LONG WindowCount;
    MSG Msg;
    PCONSOLE Console = (PCONSOLE)lParam;

    switch (msg)
    {
        case WM_CREATE:
            SetWindowLongW(hWnd, GWL_USERDATA, 0);
            return 0;
        case PM_CREATE_CONSOLE:
            NewWindow = CreateWindowExW(WS_EX_CLIENTEDGE,
                                        GUI_CONSOLE_WINDOW_CLASS,
                                        Console->Title.Buffer,
                                        WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
                                        CW_USEDEFAULT,
                                        CW_USEDEFAULT,
                                        CW_USEDEFAULT,
                                        CW_USEDEFAULT,
                                        NULL,
                                        NULL,
                                        ConSrvDllInstance,
                                        (PVOID)Console);
            if (NULL != NewWindow)
            {
                SetConsoleWndConsoleLeaderCID(Console);
                SetWindowLongW(hWnd, GWL_USERDATA, GetWindowLongW(hWnd, GWL_USERDATA) + 1);

                DPRINT1("Set icons via PM_CREATE_CONSOLE\n");
                if (Console->hIcon == NULL)
                {
                    DPRINT1("Not really...\n");
                    Console->hIcon   = ghDefaultIcon;
                    Console->hIconSm = ghDefaultIconSm;
                }
                else if (Console->hIcon != ghDefaultIcon)
                {
                    DPRINT1("Yes !\n");
                    SendMessageW(Console->hWindow, WM_SETICON, ICON_BIG, (LPARAM)Console->hIcon);
                    SendMessageW(Console->hWindow, WM_SETICON, ICON_SMALL, (LPARAM)Console->hIconSm);
                }

                ShowWindow(NewWindow, (int)wParam);
            }
            return (LRESULT)NewWindow;
        case PM_DESTROY_CONSOLE:
            /* Window creation is done using a PostMessage(), so it's possible that the
             * window that we want to destroy doesn't exist yet. So first empty the message
             * queue */
            while(PeekMessageW(&Msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&Msg);
                DispatchMessageW(&Msg);
            }
            DestroyWindow(Console->hWindow);
            Console->hWindow = NULL;
            WindowCount = GetWindowLongW(hWnd, GWL_USERDATA);
            WindowCount--;
            SetWindowLongW(hWnd, GWL_USERDATA, WindowCount);
            if (0 == WindowCount)
            {
                NotifyWnd = NULL;
                DestroyWindow(hWnd);
                PrivateCsrssManualGuiCheck(-1);
                PostQuitMessage(0);
            }
            return 0;
        default:
            return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
}

static DWORD WINAPI
GuiConsoleGuiThread(PVOID Data)
{
    MSG msg;
    PHANDLE GraphicsStartupEvent = (PHANDLE) Data;

    NotifyWnd = CreateWindowW(L"ConSrvCreateNotify",
                              L"",
                              WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              NULL,
                              NULL,
                              ConSrvDllInstance,
                              NULL);
    if (NULL == NotifyWnd)
    {
        PrivateCsrssManualGuiCheck(-1);
        SetEvent(*GraphicsStartupEvent);
        return 1;
    }

    SetEvent(*GraphicsStartupEvent);

    while(GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 1;
}

static BOOL
GuiInit(VOID)
{
    WNDCLASSEXW wc;
    ATOM ConsoleClassAtom;

    if (NULL == NotifyWnd)
    {
        PrivateCsrssManualGuiCheck(+1);
    }

    /* Initialize the notification window class */
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpszClassName = L"ConSrvCreateNotify";
    wc.lpfnWndProc = GuiConsoleNotifyWndProc;
    wc.style = 0;
    wc.hInstance = ConSrvDllInstance;
    wc.hIcon = NULL;
    wc.hIconSm = NULL;
    wc.hCursor = NULL;
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    if (RegisterClassExW(&wc) == 0)
    {
        DPRINT1("Failed to register GUI notify wndproc\n");
        return FALSE;
    }

    /* Initialize the console window class */
    ghDefaultIcon = LoadImageW(ConSrvDllInstance, MAKEINTRESOURCEW(IDI_CONSOLE), IMAGE_ICON,
                               GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON),
                               LR_SHARED);
    ghDefaultIconSm = LoadImageW(ConSrvDllInstance, MAKEINTRESOURCEW(IDI_CONSOLE), IMAGE_ICON,
                                 GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
                                 LR_SHARED);
    ghDefaultCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpszClassName = GUI_CONSOLE_WINDOW_CLASS;
    wc.lpfnWndProc = GuiConsoleWndProc;
    wc.style = 0;
    wc.hInstance = ConSrvDllInstance;
    wc.hIcon = ghDefaultIcon;
    wc.hIconSm = ghDefaultIconSm;
    wc.hCursor = ghDefaultCursor;
    wc.hbrBackground = CreateSolidBrush(RGB(0,0,0)); // FIXME: Use defaults from registry.
    wc.lpszMenuName = NULL;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = GWLP_CONSOLEWND_ALLOC;

    ConsoleClassAtom = RegisterClassExW(&wc);
    if (ConsoleClassAtom == 0)
    {
        DPRINT1("Failed to register GUI console wndproc\n");
        return FALSE;
    }
    else
    {
        NtUserConsoleControl(GuiConsoleWndClassAtom, &ConsoleClassAtom, sizeof(ATOM));
    }

    return TRUE;
}

static VOID WINAPI
GuiChangeTitle(PCONSOLE Console)
{
    PostMessageW(Console->hWindow, PM_CONSOLE_SET_TITLE, 0, 0);
}

static BOOL WINAPI
GuiChangeIcon(PCONSOLE Console, HICON hWindowIcon)
{
    HICON hIcon, hIconSm;

    if (hWindowIcon == NULL)
    {
        hIcon   = ghDefaultIcon;
        hIconSm = ghDefaultIconSm;
    }
    else
    {
        hIcon   = CopyIcon(hWindowIcon);
        hIconSm = CopyIcon(hWindowIcon);
    }

    if (hIcon == NULL)
    {
        return FALSE;
    }

    if (hIcon != Console->hIcon)
    {
        if (Console->hIcon != NULL && Console->hIcon != ghDefaultIcon)
        {
            DestroyIcon(Console->hIcon);
        }
        if (Console->hIconSm != NULL && Console->hIconSm != ghDefaultIconSm)
        {
            DestroyIcon(Console->hIconSm);
        }

        Console->hIcon   = hIcon;
        Console->hIconSm = hIconSm;

        DPRINT1("Set icons in GuiChangeIcon\n");
        PostMessageW(Console->hWindow, WM_SETICON, ICON_BIG, (LPARAM)Console->hIcon);
        PostMessageW(Console->hWindow, WM_SETICON, ICON_SMALL, (LPARAM)Console->hIconSm);
    }

    return TRUE;
}

static VOID WINAPI
GuiCleanupConsole(PCONSOLE Console)
{
    SendMessageW(NotifyWnd, PM_DESTROY_CONSOLE, 0, (LPARAM)Console);

    DPRINT1("Destroying icons !! - Console->hIcon = 0x%p ; ghDefaultIcon = 0x%p ; Console->hIconSm = 0x%p ; ghDefaultIconSm = 0x%p\n",
            Console->hIcon, ghDefaultIcon, Console->hIconSm, ghDefaultIconSm);
    if (Console->hIcon != NULL && Console->hIcon != ghDefaultIcon)
    {
        DPRINT1("Destroy hIcon\n");
        DestroyIcon(Console->hIcon);
    }
    if (Console->hIconSm != NULL && Console->hIconSm != ghDefaultIconSm)
    {
        DPRINT1("Destroy hIconSm\n");
        DestroyIcon(Console->hIconSm);
    }
}

static CONSOLE_VTBL GuiVtbl =
{
    GuiWriteStream,
    GuiDrawRegion,
    GuiSetCursorInfo,
    GuiSetScreenInfo,
    GuiUpdateScreenInfo,
    GuiChangeTitle,
    GuiCleanupConsole,
    GuiChangeIcon,
    GuiResizeBuffer,
    GuiProcessKeyCallback
};

NTSTATUS FASTCALL
GuiInitConsole(PCONSOLE Console,
               LPCWSTR AppPath,
               PCONSOLE_INFO ConsoleInfo,
               LPCWSTR IconPath,
               INT IconIndex)
{
    HANDLE GraphicsStartupEvent;
    HANDLE ThreadHandle;
    PGUI_CONSOLE_DATA GuiData;

    /* Initialize the GUI */
    if (!ConsInitialized)
    {
        ConsInitialized = TRUE;
        if (!GuiInit())
        {
            ConsInitialized = FALSE;
            return STATUS_UNSUCCESSFUL;
        }
    }

    /* Finish to initialize the console */
    Console->Vtbl = &GuiVtbl;
    Console->hWindow = NULL;

    if (NULL == NotifyWnd)
    {
        GraphicsStartupEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
        if (NULL == GraphicsStartupEvent)
        {
            return STATUS_UNSUCCESSFUL;
        }

        ThreadHandle = CreateThread(NULL,
                                    0,
                                    GuiConsoleGuiThread,
                                    (PVOID)&GraphicsStartupEvent,
                                    0,
                                    NULL);
        if (NULL == ThreadHandle)
        {
            CloseHandle(GraphicsStartupEvent);
            DPRINT1("CONSRV: Failed to create graphics console thread. Expect problems\n");
            return STATUS_UNSUCCESSFUL;
        }
        SetThreadPriority(ThreadHandle, THREAD_PRIORITY_HIGHEST);
        CloseHandle(ThreadHandle);

        WaitForSingleObject(GraphicsStartupEvent, INFINITE);
        CloseHandle(GraphicsStartupEvent);

        if (NULL == NotifyWnd)
        {
            DPRINT1("CONSRV: Failed to create notification window.\n");
            return STATUS_UNSUCCESSFUL;
        }
    }

    GuiData = RtlAllocateHeap(ConSrvHeap, HEAP_ZERO_MEMORY,
                              sizeof(GUI_CONSOLE_DATA));
    if (!GuiData)
    {
        DPRINT1("CONSRV: Failed to create GUI_CONSOLE_DATA\n");
        return STATUS_UNSUCCESSFUL;
    }
    Console->GuiData = (PVOID)GuiData;

    if (ConsoleInfo)
    {
        wcsncpy(GuiData->GuiInfo.FaceName, ConsoleInfo->u.GuiInfo.FaceName, LF_FACESIZE);
        GuiData->GuiInfo.FontFamily     = ConsoleInfo->u.GuiInfo.FontFamily;
        GuiData->GuiInfo.FontSize       = ConsoleInfo->u.GuiInfo.FontSize;
        GuiData->GuiInfo.FontWeight     = ConsoleInfo->u.GuiInfo.FontWeight;
        GuiData->GuiInfo.UseRasterFonts = ConsoleInfo->u.GuiInfo.UseRasterFonts;

        GuiData->GuiInfo.ShowWindow   = ConsoleInfo->u.GuiInfo.ShowWindow;
        GuiData->GuiInfo.AutoPosition = ConsoleInfo->u.GuiInfo.AutoPosition;
        GuiData->GuiInfo.WindowOrigin = ConsoleInfo->u.GuiInfo.WindowOrigin;
    }
    /*
    else
    {
        // TODO: What could be the defaults ?
    }
    */

    /* Initialize the icon handles to their default values */
    Console->hIcon   = ghDefaultIcon;
    Console->hIconSm = ghDefaultIconSm;

    /* Get the associated icon, if any */
    if (IconPath == NULL || *IconPath == L'\0')
    {
        IconPath  = AppPath;
        IconIndex = 0;
    }
    DPRINT1("IconPath = %S ; IconIndex = %lu\n", (IconPath ? IconPath : L"n/a"), IconIndex);
    if (IconPath)
    {
        HICON hIcon = NULL, hIconSm = NULL;
        PrivateExtractIconExW(IconPath,
                              IconIndex,
                              &hIcon,
                              &hIconSm,
                              1);
        DPRINT1("hIcon = 0x%p ; hIconSm = 0x%p\n", hIcon, hIconSm);
        if (hIcon != NULL)
        {
            DPRINT1("Effectively set the icons\n");
            Console->hIcon   = hIcon;
            Console->hIconSm = hIconSm;
        }
    }

    /*
     * We need to wait until the GUI has been fully initialized
     * to retrieve custom settings i.e. WindowSize etc...
     * Ideally we could use SendNotifyMessage for this but its not
     * yet implemented.
     */
    GuiData->hGuiInitEvent = CreateEventW(NULL, FALSE, FALSE, NULL);

    /* Create the GUI console */
    PostMessageW(NotifyWnd, PM_CREATE_CONSOLE, GuiData->GuiInfo.ShowWindow, (LPARAM)Console);

    /* Wait until initialization has finished */
    WaitForSingleObject(GuiData->hGuiInitEvent, INFINITE);
    DPRINT("Received event Console %p GuiData %p X %d Y %d\n", Console, Console->GuiData, Console->Size.X, Console->Size.Y);
    CloseHandle(GuiData->hGuiInitEvent);
    GuiData->hGuiInitEvent = NULL;

    return STATUS_SUCCESS;
}

/* EOF */
