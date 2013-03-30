/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/guiconsole.c
 * PURPOSE:         GUI front-end
 * PROGRAMMERS:
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include "conio.h"
#include "console.h"
#include "settings.h"
#include "guiconsole.h"
#include "guisettings.h"

#define NDEBUG
#include <debug.h>

/*
// Define wmemset(...)
#include <wchar.h>
#define HAVE_WMEMSET
*/

/* GUI Console Window Class name */
#define GUI_CONSOLE_WINDOW_CLASS L"ConsoleWindowClass"

#ifndef WM_APP
    #define WM_APP 0x8000
#endif
#define PM_CREATE_CONSOLE       (WM_APP + 1)
#define PM_DESTROY_CONSOLE      (WM_APP + 2)
#define PM_CONSOLE_BEEP         (WM_APP + 3)
#define PM_CONSOLE_SET_TITLE    (WM_APP + 4)


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

/**************************************************************\
\** Define the Console Leader Process for the console window **/
#define GWLP_CONSOLEWND_ALLOC  (2 * sizeof(LONG_PTR))
#define GWLP_CONSOLE_LEADER_PID 0
#define GWLP_CONSOLE_LEADER_TID 4

#define SetConsoleWndConsoleLeaderCID(GuiData)  \
do {                                            \
    PCONSOLE_PROCESS_DATA ProcessData;          \
    CLIENT_ID ConsoleLeaderCID;                 \
    ProcessData = CONTAINING_RECORD((GuiData)->Console->ProcessList.Blink,  \
                                    CONSOLE_PROCESS_DATA,                   \
                                    ConsoleLink);                           \
    ConsoleLeaderCID = ProcessData->Process->ClientId;                      \
    SetWindowLongPtrW((GuiData)->hWindow, GWLP_CONSOLE_LEADER_PID, (LONG_PTR)(ConsoleLeaderCID.UniqueProcess));  \
    SetWindowLongPtrW((GuiData)->hWindow, GWLP_CONSOLE_LEADER_TID, (LONG_PTR)(ConsoleLeaderCID.UniqueThread ));  \
} while(0)
/**************************************************************/

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
GuiConsoleCreateSysMenu(HWND hWnd)
{
    HMENU hMenu;
    hMenu = GetSystemMenu(hWnd, FALSE);
    if (hMenu != NULL)
    {
        GuiConsoleAppendMenuItems(hMenu, GuiConsoleMainMenuItems);
        DrawMenuBar(hWnd);
    }
}


static VOID
GuiConsoleCopy(PGUI_CONSOLE_DATA GuiData);
static VOID
GuiConsolePaste(PGUI_CONSOLE_DATA GuiData);
static VOID
GuiConsoleUpdateSelection(PCONSOLE Console, PCOORD coord);
static VOID WINAPI
GuiDrawRegion(PCONSOLE Console, SMALL_RECT* Region);
static NTSTATUS WINAPI
GuiResizeBuffer(PCONSOLE Console, PCONSOLE_SCREEN_BUFFER ScreenBuffer, COORD Size);
static VOID
GuiConsoleResizeWindow(PGUI_CONSOLE_DATA GuiData);


static LRESULT
GuiConsoleHandleSysMenuCommand(PGUI_CONSOLE_DATA GuiData, WPARAM wParam, LPARAM lParam)
{
    LRESULT Ret = TRUE;
    PCONSOLE Console = GuiData->Console;

    if (!ConSrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE))
    {
        Ret = FALSE;
        goto Quit;
    }

    switch (wParam)
    {
        case ID_SYSTEM_EDIT_MARK:
            DPRINT1("Marking not handled yet\n");
            break;

        case ID_SYSTEM_EDIT_COPY:
            GuiConsoleCopy(GuiData);
            break;

        case ID_SYSTEM_EDIT_PASTE:
            GuiConsolePaste(GuiData);
            break;

        case ID_SYSTEM_EDIT_SELECTALL:
        {
            COORD bottomRight = { 0, 0 };

            bottomRight.X = Console->ConsoleSize.X - 1;
            bottomRight.Y = Console->ConsoleSize.Y - 1;
            GuiConsoleUpdateSelection(Console, &bottomRight);
            break;
        }

        case ID_SYSTEM_EDIT_SCROLL:
            DPRINT1("Scrolling is not handled yet\n");
            break;

        case ID_SYSTEM_EDIT_FIND:
            DPRINT1("Finding is not handled yet\n");
            break;

        case ID_SYSTEM_DEFAULTS:
            GuiConsoleShowConsoleProperties(GuiData, TRUE);
            break;

        case ID_SYSTEM_PROPERTIES:
            GuiConsoleShowConsoleProperties(GuiData, FALSE);
            break;

        default:
            Ret = FALSE;
            break;
    }

    LeaveCriticalSection(&Console->Lock);

Quit:
    if (!Ret)
        Ret = DefWindowProcW(GuiData->hWindow, WM_SYSCOMMAND, wParam, lParam);

    return Ret;
}

static PGUI_CONSOLE_DATA
GuiGetGuiData(HWND hWnd)
{
    /* This function ensures that the console pointer is not NULL */
    PGUI_CONSOLE_DATA GuiData = (PGUI_CONSOLE_DATA)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
    return ( ((GuiData == NULL) || (GuiData->hWindow == hWnd && GuiData->Console != NULL)) ? GuiData : NULL );
}

VOID
GuiConsoleMoveWindow(PGUI_CONSOLE_DATA GuiData)
{
    /* Move the window if needed (not positioned by the system) */
    if (!GuiData->GuiInfo.AutoPosition)
    {
        SetWindowPos(GuiData->hWindow,
                     NULL,
                     GuiData->GuiInfo.WindowOrigin.x,
                     GuiData->GuiInfo.WindowOrigin.y,
                     0,
                     0,
                     SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
    }
}

static VOID
GuiConsoleResizeWindow(PGUI_CONSOLE_DATA GuiData)
{
    PCONSOLE Console = GuiData->Console;
    SCROLLINFO sInfo;

    DWORD Width  = Console->ConsoleSize.X * GuiData->CharWidth  +
                       2 * (GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXEDGE));
    DWORD Height = Console->ConsoleSize.Y * GuiData->CharHeight +
                       2 * (GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CYEDGE)) + GetSystemMetrics(SM_CYCAPTION);

    /* Set scrollbar sizes */
    sInfo.cbSize = sizeof(SCROLLINFO);
    sInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    sInfo.nMin = 0;
    if (Console->ActiveBuffer->ScreenBufferSize.Y > Console->ConsoleSize.Y)
    {
        sInfo.nMax = Console->ActiveBuffer->ScreenBufferSize.Y - 1;
        sInfo.nPage = Console->ConsoleSize.Y;
        sInfo.nPos = Console->ActiveBuffer->ShowY;
        SetScrollInfo(GuiData->hWindow, SB_VERT, &sInfo, TRUE);
        Width += GetSystemMetrics(SM_CXVSCROLL);
        ShowScrollBar(GuiData->hWindow, SB_VERT, TRUE);
    }
    else
    {
        ShowScrollBar(GuiData->hWindow, SB_VERT, FALSE);
    }

    if (Console->ActiveBuffer->ScreenBufferSize.X > Console->ConsoleSize.X)
    {
        sInfo.nMax = Console->ActiveBuffer->ScreenBufferSize.X - 1;
        sInfo.nPage = Console->ConsoleSize.X;
        sInfo.nPos = Console->ActiveBuffer->ShowX;
        SetScrollInfo(GuiData->hWindow, SB_HORZ, &sInfo, TRUE);
        Height += GetSystemMetrics(SM_CYHSCROLL);
        ShowScrollBar(GuiData->hWindow, SB_HORZ, TRUE);
    }
    else
    {
        ShowScrollBar(GuiData->hWindow, SB_HORZ, FALSE);
    }

    /* Resize the window  */
    SetWindowPos(GuiData->hWindow, NULL, 0, 0, Width, Height,
                 SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
}

static BOOL
GuiConsoleHandleNcCreate(HWND hWnd, LPCREATESTRUCTW Create)
{
    PGUI_CONSOLE_DATA GuiData = (PGUI_CONSOLE_DATA)Create->lpCreateParams;
    PCONSOLE Console;
    HDC Dc;
    HFONT OldFont;
    TEXTMETRICW Metrics;
    SIZE CharSize;

    DPRINT1("GuiConsoleHandleNcCreate\n");

    if (NULL == GuiData)
    {
        DPRINT1("GuiConsoleNcCreate: No GUI data\n");
        return FALSE;
    }

    Console = GuiData->Console;

    GuiData->hWindow = hWnd;

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
        GuiData->hWindow = NULL;
        SetEvent(GuiData->hGuiInitEvent);
        return FALSE;
    }
    Dc = GetDC(GuiData->hWindow);
    if (NULL == Dc)
    {
        DPRINT1("GuiConsoleNcCreate: GetDC failed\n");
        DeleteObject(GuiData->Font);
        GuiData->hWindow = NULL;
        SetEvent(GuiData->hGuiInitEvent);
        return FALSE;
    }
    OldFont = SelectObject(Dc, GuiData->Font);
    if (NULL == OldFont)
    {
        DPRINT1("GuiConsoleNcCreate: SelectObject failed\n");
        ReleaseDC(GuiData->hWindow, Dc);
        DeleteObject(GuiData->Font);
        GuiData->hWindow = NULL;
        SetEvent(GuiData->hGuiInitEvent);
        return FALSE;
    }
    if (!GetTextMetricsW(Dc, &Metrics))
    {
        DPRINT1("GuiConsoleNcCreate: GetTextMetrics failed\n");
        SelectObject(Dc, OldFont);
        ReleaseDC(GuiData->hWindow, Dc);
        DeleteObject(GuiData->Font);
        GuiData->hWindow = NULL;
        SetEvent(GuiData->hGuiInitEvent);
        return FALSE;
    }
    GuiData->CharWidth  = Metrics.tmMaxCharWidth;
    GuiData->CharHeight = Metrics.tmHeight + Metrics.tmExternalLeading;

    /* Measure real char width more precisely if possible. */
    if (GetTextExtentPoint32W(Dc, L"R", 1, &CharSize))
        GuiData->CharWidth = CharSize.cx;

    SelectObject(Dc, OldFont);

    ReleaseDC(GuiData->hWindow, Dc);

    // FIXME: Keep these instructions here ? ///////////////////////////////////
    Console->ActiveBuffer->CursorBlinkOn = TRUE;
    Console->ActiveBuffer->ForceCursorOff = FALSE;
    ////////////////////////////////////////////////////////////////////////////

    SetWindowLongPtrW(GuiData->hWindow, GWLP_USERDATA, (DWORD_PTR)GuiData);

    SetTimer(GuiData->hWindow, CONGUI_UPDATE_TIMER, CONGUI_UPDATE_TIME, NULL);
    GuiConsoleCreateSysMenu(GuiData->hWindow);

    DPRINT1("GuiConsoleHandleNcCreate - setting start event\n");
    SetEvent(GuiData->hGuiInitEvent);

    return (BOOL)DefWindowProcW(GuiData->hWindow, WM_NCCREATE, 0, (LPARAM)Create);
}

static VOID
SmallRectToRect(PGUI_CONSOLE_DATA GuiData, PRECT Rect, PSMALL_RECT SmallRect)
{
    PCONSOLE Console = GuiData->Console;
    PCONSOLE_SCREEN_BUFFER Buffer = Console->ActiveBuffer;

    Rect->left   = (SmallRect->Left       - Buffer->ShowX) * GuiData->CharWidth;
    Rect->top    = (SmallRect->Top        - Buffer->ShowY) * GuiData->CharHeight;
    Rect->right  = (SmallRect->Right  + 1 - Buffer->ShowX) * GuiData->CharWidth;
    Rect->bottom = (SmallRect->Bottom + 1 - Buffer->ShowY) * GuiData->CharHeight;
}

static VOID
GuiConsoleUpdateSelection(PCONSOLE Console, PCOORD coord)
{
    PGUI_CONSOLE_DATA GuiData = Console->TermIFace.Data;
    RECT oldRect, newRect;

    SmallRectToRect(GuiData, &oldRect, &Console->Selection.srSelection);

    if(coord != NULL)
    {
        SMALL_RECT rc;
        /* exchange left/top with right/bottom if required */
        rc.Left   = min(Console->Selection.dwSelectionAnchor.X, coord->X);
        rc.Top    = min(Console->Selection.dwSelectionAnchor.Y, coord->Y);
        rc.Right  = max(Console->Selection.dwSelectionAnchor.X, coord->X);
        rc.Bottom = max(Console->Selection.dwSelectionAnchor.Y, coord->Y);

        SmallRectToRect(GuiData, &newRect, &rc);

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
                            InvalidateRgn(GuiData->hWindow, rgn1, FALSE);
                        }

                        DeleteObject(rgn2);
                    }
                    DeleteObject(rgn1);
                }
            }
        }
        else
        {
            InvalidateRect(GuiData->hWindow, &newRect, FALSE);
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
            InvalidateRect(GuiData->hWindow, &oldRect, FALSE);
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

    TopLine = rc->top / GuiData->CharHeight + Buff->ShowY;
    BottomLine = (rc->bottom + (GuiData->CharHeight - 1)) / GuiData->CharHeight - 1 + Buff->ShowY;
    LeftChar = rc->left / GuiData->CharWidth + Buff->ShowX;
    RightChar = (rc->right + (GuiData->CharWidth - 1)) / GuiData->CharWidth - 1 + Buff->ShowX;
    LastAttribute = ConioCoordToPointer(Buff, LeftChar, TopLine)[1];

    SetTextColor(hDC, RGBFromAttrib(Console, TextAttribFromAttrib(LastAttribute)));
    SetBkColor(hDC, RGBFromAttrib(Console, BkgdAttribFromAttrib(LastAttribute)));

    if (BottomLine >= Buff->ScreenBufferSize.Y) BottomLine = Buff->ScreenBufferSize.Y - 1;
    if (RightChar >= Buff->ScreenBufferSize.X) RightChar = Buff->ScreenBufferSize.X - 1;

    OldFont = SelectObject(hDC, GuiData->Font);

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

            OldBrush = SelectObject(hDC, CursorBrush);
            PatBlt(hDC,
                   (CursorX - Buff->ShowX) * GuiData->CharWidth,
                   (CursorY - Buff->ShowY) * GuiData->CharHeight + (GuiData->CharHeight - CursorHeight),
                   GuiData->CharWidth,
                   CursorHeight,
                   PATCOPY);
            SelectObject(hDC, OldBrush);
            DeleteObject(CursorBrush);
        }
    }

    SelectObject(hDC, OldFont);
}

static VOID
GuiConsoleHandlePaint(PGUI_CONSOLE_DATA GuiData)
{
    BOOL Success = TRUE;
    PCONSOLE Console = GuiData->Console;
    HDC hDC;
    PAINTSTRUCT ps;

    if (!ConSrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE))
    {
        Success = FALSE;
        goto Quit;
    }

    if (Console->ActiveBuffer == NULL ||
        Console->ActiveBuffer->Buffer == NULL)
    {
        goto Quit;
    }

    hDC = BeginPaint(GuiData->hWindow, &ps);
    if (hDC != NULL &&
            ps.rcPaint.left < ps.rcPaint.right &&
            ps.rcPaint.top < ps.rcPaint.bottom)
    {
        EnterCriticalSection(&GuiData->Lock);

        GuiConsolePaint(Console,
                        GuiData,
                        hDC,
                        &ps.rcPaint);

        if (Console->Selection.dwFlags & CONSOLE_SELECTION_NOT_EMPTY)
        {
            RECT rc;
            SmallRectToRect(GuiData, &rc, &Console->Selection.srSelection);

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
    EndPaint(GuiData->hWindow, &ps);

Quit:
    if (Success)
        LeaveCriticalSection(&Console->Lock);
    else
        DefWindowProcW(GuiData->hWindow, WM_PAINT, 0, 0);

    return;
}

static VOID
GuiConsoleHandleKey(PGUI_CONSOLE_DATA GuiData, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PCONSOLE Console = GuiData->Console;
    MSG Message;

    if (!ConSrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE)) return;

    Message.hwnd = GuiData->hWindow;
    Message.message = msg;
    Message.wParam = wParam;
    Message.lParam = lParam;

    if(msg == WM_CHAR || msg == WM_SYSKEYDOWN)
    {
        /* clear the selection */
        GuiConsoleUpdateSelection(Console, NULL);
    }

    ConioProcessKey(Console, &Message);

    LeaveCriticalSection(&Console->Lock);
}

static VOID
GuiInvalidateCell(PCONSOLE Console, UINT x, UINT y)
{
    SMALL_RECT CellRect = { x, y, x, y };
    GuiDrawRegion(Console, &CellRect);
}

static VOID
GuiConsoleHandleTimer(PGUI_CONSOLE_DATA GuiData)
{
    PCONSOLE Console = GuiData->Console;
    PCONSOLE_SCREEN_BUFFER Buff;

    SetTimer(GuiData->hWindow, CONGUI_UPDATE_TIMER, CURSOR_BLINK_TIME, NULL);

    if (!ConSrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE)) return;

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
        if(GetScrollInfo(GuiData->hWindow, SB_HORZ, &xScroll))OldScrollX = xScroll.nPos;
        if(GetScrollInfo(GuiData->hWindow, SB_VERT, &xScroll))OldScrollY = xScroll.nPos;

        // If we successfully got the info for the horizontal scrollbar
        if(OldScrollX >= 0)
        {
            if((Buff->CursorPosition.X < Buff->ShowX)||(Buff->CursorPosition.X >= (Buff->ShowX + Console->ConsoleSize.X)))
            {
                // Handle the horizontal scroll bar
                if(Buff->CursorPosition.X >= Console->ConsoleSize.X) NewScrollX = Buff->CursorPosition.X - Console->ConsoleSize.X + 1;
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
            if((Buff->CursorPosition.Y < Buff->ShowY) || (Buff->CursorPosition.Y >= (Buff->ShowY + Console->ConsoleSize.Y)))
            {
                // Handle the vertical scroll bar
                if(Buff->CursorPosition.Y >= Console->ConsoleSize.Y) NewScrollY = Buff->CursorPosition.Y - Console->ConsoleSize.Y + 1;
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
            ScrollWindowEx(GuiData->hWindow,
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
                SetScrollInfo(GuiData->hWindow, SB_HORZ, &xScroll, TRUE);
            }
            if(NewScrollY >= 0)
            {
                xScroll.nPos = NewScrollY;
                SetScrollInfo(GuiData->hWindow, SB_VERT, &xScroll, TRUE);
            }
            UpdateWindow(GuiData->hWindow);
            GuiData->OldCursor.x = Buff->CursorPosition.X;
            GuiData->OldCursor.y = Buff->CursorPosition.Y;
        }
    }

    LeaveCriticalSection(&Console->Lock);
}

static VOID
GuiConsoleHandleClose(PGUI_CONSOLE_DATA GuiData)
{
    PCONSOLE Console = GuiData->Console;
    if (!ConSrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE)) return;

    /*
     * FIXME: Windows will wait up to 5 seconds for the thread to exit.
     * We shouldn't wait here, though, since the console lock is entered.
     * A copy of the thread list probably needs to be made.
     */
    ConSrvConsoleProcessCtrlEvent(Console, 0, CTRL_CLOSE_EVENT);

    LeaveCriticalSection(&Console->Lock);
}

static LRESULT
GuiConsoleHandleNcDestroy(HWND hWnd)
{
    // PGUI_CONSOLE_DATA GuiData;

    KillTimer(hWnd, CONGUI_UPDATE_TIMER);
    GetSystemMenu(hWnd, TRUE);

    /* Free the GuiData registration */
    SetWindowLongPtrW(hWnd, GWLP_USERDATA, (DWORD_PTR)NULL);
    // GuiData->hWindow = NULL;

    // return 0;
    return DefWindowProcW(hWnd, WM_NCDESTROY, 0, 0);
}

static COORD
PointToCoord(PGUI_CONSOLE_DATA GuiData, LPARAM lParam)
{
    PCONSOLE Console = GuiData->Console;
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

static LRESULT
GuiConsoleHandleMouse(PGUI_CONSOLE_DATA GuiData, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT Ret = TRUE;
    PCONSOLE Console = GuiData->Console;

    if (!ConSrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE))
    {
        Ret = FALSE;
        goto Quit;
    }

    switch (msg)
    {
        case WM_LBUTTONDOWN:
        {
            Console->Selection.dwSelectionAnchor = PointToCoord(GuiData, lParam);
            SetCapture(GuiData->hWindow);
            Console->Selection.dwFlags |= CONSOLE_SELECTION_IN_PROGRESS | CONSOLE_MOUSE_SELECTION | CONSOLE_MOUSE_DOWN;
            GuiConsoleUpdateSelection(Console, &Console->Selection.dwSelectionAnchor);
            break;
        }

        case WM_LBUTTONUP:
        {
            COORD c;

            if (!(Console->Selection.dwFlags & CONSOLE_MOUSE_DOWN)) break;

            c = PointToCoord(GuiData, lParam);
            Console->Selection.dwFlags &= ~CONSOLE_MOUSE_DOWN;
            GuiConsoleUpdateSelection(Console, &c);
            ReleaseCapture();

            break;
        }

        case WM_RBUTTONDOWN:
        {
            if (!(Console->Selection.dwFlags & CONSOLE_SELECTION_NOT_EMPTY))
            {
                GuiConsolePaste(GuiData);
            }
            else
            {
                GuiConsoleCopy(GuiData);

                /* Clear the selection */
                GuiConsoleUpdateSelection(Console, NULL);
            }

            break;
        }

        case WM_MOUSEMOVE:
        {
            COORD c;

            if (!(wParam & MK_LBUTTON)) break;
            if (!(Console->Selection.dwFlags & CONSOLE_MOUSE_DOWN)) break;

            c = PointToCoord(GuiData, lParam); /* TODO: Scroll buffer to bring c into view */
            GuiConsoleUpdateSelection(Console, &c);

            break;
        }

        default:
            Ret = FALSE;
            break;
    }

    LeaveCriticalSection(&Console->Lock);

Quit:
    if (!Ret)
        Ret = DefWindowProcW(GuiData->hWindow, msg, wParam, lParam);

    return Ret;
}

static VOID
GuiConsoleCopy(PGUI_CONSOLE_DATA GuiData)
{
    PCONSOLE Console = GuiData->Console;

    if (OpenClipboard(GuiData->hWindow) == TRUE)
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
GuiConsolePaste(PGUI_CONSOLE_DATA GuiData)
{
    PCONSOLE Console = GuiData->Console;

    if (OpenClipboard(GuiData->hWindow) == TRUE)
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
GuiConsoleGetMinMaxInfo(PGUI_CONSOLE_DATA GuiData, PMINMAXINFO minMaxInfo)
{
    PCONSOLE Console = GuiData->Console;
    DWORD windx, windy;

    if (!ConSrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE)) return;

    windx = CONGUI_MIN_WIDTH * GuiData->CharWidth + 2 * (GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXEDGE));
    windy = CONGUI_MIN_HEIGHT * GuiData->CharHeight + 2 * (GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CYEDGE)) + GetSystemMetrics(SM_CYCAPTION);

    minMaxInfo->ptMinTrackSize.x = windx;
    minMaxInfo->ptMinTrackSize.y = windy;

    windx = (Console->ActiveBuffer->ScreenBufferSize.X) * GuiData->CharWidth  + 2 * (GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXEDGE));
    windy = (Console->ActiveBuffer->ScreenBufferSize.Y) * GuiData->CharHeight + 2 * (GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CYEDGE)) + GetSystemMetrics(SM_CYCAPTION);

    if(Console->ConsoleSize.X < Console->ActiveBuffer->ScreenBufferSize.X) windy += GetSystemMetrics(SM_CYHSCROLL);    // window currently has a horizontal scrollbar
    if(Console->ConsoleSize.Y < Console->ActiveBuffer->ScreenBufferSize.Y) windx += GetSystemMetrics(SM_CXVSCROLL);    // window currently has a vertical scrollbar

    minMaxInfo->ptMaxTrackSize.x = windx;
    minMaxInfo->ptMaxTrackSize.y = windy;

    LeaveCriticalSection(&Console->Lock);
}

static VOID
GuiConsoleResize(PGUI_CONSOLE_DATA GuiData, WPARAM wParam, LPARAM lParam)
{
    PCONSOLE Console = GuiData->Console;

    if (!ConSrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE)) return;

    if ((GuiData->WindowSizeLock == FALSE) &&
        (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED || wParam == SIZE_MINIMIZED))
    {
        PCONSOLE_SCREEN_BUFFER Buff = Console->ActiveBuffer;
        DWORD windx, windy, charx, chary;

        GuiData->WindowSizeLock = TRUE;

        windx = LOWORD(lParam);
        windy = HIWORD(lParam);

        // Compensate for existing scroll bars (because lParam values do not accommodate scroll bar)
        if(Console->ConsoleSize.X < Buff->ScreenBufferSize.X) windy += GetSystemMetrics(SM_CYHSCROLL);    // window currently has a horizontal scrollbar
        if(Console->ConsoleSize.Y < Buff->ScreenBufferSize.Y) windx += GetSystemMetrics(SM_CXVSCROLL);    // window currently has a vertical scrollbar

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
        if((charx != Console->ConsoleSize.X) || (chary != Console->ConsoleSize.Y))
        {
            Console->ConsoleSize.X = (charx <= Buff->ScreenBufferSize.X) ? charx : Buff->ScreenBufferSize.X;
            Console->ConsoleSize.Y = (chary <= Buff->ScreenBufferSize.Y) ? chary : Buff->ScreenBufferSize.Y;
        }

        GuiConsoleResizeWindow(GuiData);

        // Adjust the start of the visible area if we are attempting to show nonexistent areas
        if((Buff->ScreenBufferSize.X - Buff->ShowX) < Console->ConsoleSize.X) Buff->ShowX = Buff->ScreenBufferSize.X - Console->ConsoleSize.X;
        if((Buff->ScreenBufferSize.Y - Buff->ShowY) < Console->ConsoleSize.Y) Buff->ShowY = Buff->ScreenBufferSize.Y - Console->ConsoleSize.Y;
        InvalidateRect(GuiData->hWindow, NULL, TRUE);

        GuiData->WindowSizeLock = FALSE;
    }

    LeaveCriticalSection(&Console->Lock);
}

/*
// HACK: This functionality is standard for general scrollbars. Don't add it by hand.

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
*/

static LRESULT
GuiConsoleHandleScroll(PGUI_CONSOLE_DATA GuiData, UINT uMsg, WPARAM wParam)
{
    PCONSOLE Console = GuiData->Console;
    PCONSOLE_SCREEN_BUFFER Buff;
    SCROLLINFO sInfo;
    int fnBar;
    int old_pos, Maximum;
    PUSHORT pShowXY;

    if (!ConSrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE)) return 0;

    Buff = Console->ActiveBuffer;

    if (uMsg == WM_HSCROLL)
    {
        fnBar = SB_HORZ;
        Maximum = Buff->ScreenBufferSize.X - Console->ConsoleSize.X;
        pShowXY = &Buff->ShowX;
    }
    else
    {
        fnBar = SB_VERT;
        Maximum = Buff->ScreenBufferSize.Y - Console->ConsoleSize.Y;
        pShowXY = &Buff->ShowY;
    }

    /* set scrollbar sizes */
    sInfo.cbSize = sizeof(SCROLLINFO);
    sInfo.fMask = SIF_RANGE | SIF_POS | SIF_PAGE | SIF_TRACKPOS;

    if (!GetScrollInfo(GuiData->hWindow, fnBar, &sInfo)) goto Quit;

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

        ScrollWindowEx(GuiData->hWindow,
                       (OldX - Buff->ShowX) * GuiData->CharWidth,
                       (OldY - Buff->ShowY) * GuiData->CharHeight,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       SW_INVALIDATE);

        sInfo.fMask = SIF_POS;
        SetScrollInfo(GuiData->hWindow, fnBar, &sInfo, TRUE);

        UpdateWindow(GuiData->hWindow);
    }

Quit:
    LeaveCriticalSection(&Console->Lock);
    return 0;
}

static LRESULT CALLBACK
GuiConsoleWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT Result = 0;
    PGUI_CONSOLE_DATA GuiData = NULL;
    PCONSOLE Console = NULL;

    /*
     * - If it's the first time we create a window for the terminal,
     *   just initialize it and return.
     *
     * - If we are destroying the window, just do it and return.
     */
    if (msg == WM_NCCREATE)
    {
        return (LRESULT)GuiConsoleHandleNcCreate(hWnd, (LPCREATESTRUCTW)lParam);
    }
    else if (msg == WM_NCDESTROY)
    {
        return GuiConsoleHandleNcDestroy(hWnd);
    }

    /*
     * Now the terminal window is initialized.
     * Get the terminal data via the window's data.
     * If there is no data, just go away.
     */
    GuiData = GuiGetGuiData(hWnd);
    if (GuiData == NULL) return 0;

    /*
     * Each helper function which needs the console
     * has to validate and lock it.
     */

    /* We have a console, start message dispatching */
    switch (msg)
    {
        case WM_CLOSE:
            GuiConsoleHandleClose(GuiData);
            break;

        case WM_PAINT:
            GuiConsoleHandlePaint(GuiData);
            break;

        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_CHAR:
        {
            GuiConsoleHandleKey(GuiData, msg, wParam, lParam);
            break;
        }

        case WM_TIMER:
            GuiConsoleHandleTimer(GuiData);
            break;

        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MBUTTONDBLCLK:
        case WM_MOUSEWHEEL:
        {
            Result = GuiConsoleHandleMouse(GuiData, msg, wParam, lParam);
            break;
        }

        case WM_SYSCOMMAND:
        {
            Result = GuiConsoleHandleSysMenuCommand(GuiData, wParam, lParam);
            break;
        }

        case WM_HSCROLL:
        case WM_VSCROLL:
        {
            Result = GuiConsoleHandleScroll(GuiData, msg, wParam);
            break;
        }

        case WM_GETMINMAXINFO:
            GuiConsoleGetMinMaxInfo(GuiData, (PMINMAXINFO)lParam);
            break;

        case WM_SIZE:
            GuiConsoleResize(GuiData, wParam, lParam);
            break;

        case PM_APPLY_CONSOLE_INFO:
        {
            Console = GuiData->Console; // Not NULL because checked in GuiGetGuiData.
            if (ConSrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE))
            {
                GuiApplyUserSettings(GuiData, (HANDLE)wParam, (BOOL)lParam);
                LeaveCriticalSection(&Console->Lock);
            }
            break;
        }

        case PM_CONSOLE_BEEP:
            DPRINT1("Beep !!\n");
            Beep(800, 200);
            break;

        // case PM_CONSOLE_SET_TITLE:
            // SetWindowText(GuiData->hWindow, GuiData->Console->Title.Buffer);
            // break;

        default:
            Result = DefWindowProcW(hWnd, msg, wParam, lParam);
            break;
    }

    return Result;
}



/******************************************************************************
 *                        GUI Terminal Initialization                         *
 ******************************************************************************/

static LRESULT CALLBACK
GuiConsoleNotifyWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND NewWindow;
    LONG WindowCount;
    MSG Msg;

    switch (msg)
    {
        case WM_CREATE:
        {
            SetWindowLongW(hWnd, GWL_USERDATA, 0);
            return 0;
        }
    
        case PM_CREATE_CONSOLE:
        {
            PGUI_CONSOLE_DATA GuiData = (PGUI_CONSOLE_DATA)lParam;
            PCONSOLE Console = GuiData->Console;

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
                                        (PVOID)GuiData);
            if (NULL != NewWindow)
            {
                WindowCount = GetWindowLongW(hWnd, GWL_USERDATA);
                WindowCount++;
                SetWindowLongW(hWnd, GWL_USERDATA, WindowCount);

                DPRINT1("Set icons via PM_CREATE_CONSOLE\n");
                if (GuiData->hIcon == NULL)
                {
                    DPRINT1("Not really /o\\...\n");
                    GuiData->hIcon   = ghDefaultIcon;
                    GuiData->hIconSm = ghDefaultIconSm;
                }
                else if (GuiData->hIcon != ghDefaultIcon)
                {
                    DPRINT1("Yes \\o/\n");
                    SendMessageW(GuiData->hWindow, WM_SETICON, ICON_BIG, (LPARAM)GuiData->hIcon);
                    SendMessageW(GuiData->hWindow, WM_SETICON, ICON_SMALL, (LPARAM)GuiData->hIconSm);
                }

                /* Move and resize the window to the user's values */
                /* CAN WE DEADLOCK ?? */
                GuiConsoleMoveWindow(GuiData);
                GuiData->WindowSizeLock = TRUE;
                GuiConsoleResizeWindow(GuiData);
                GuiData->WindowSizeLock = FALSE;

                // ShowWindow(NewWindow, (int)wParam);
                ShowWindowAsync(NewWindow, (int)wParam);
                DPRINT1("Window showed\n");
            }

            return (LRESULT)NewWindow;
        }

        case PM_DESTROY_CONSOLE:
        {
            PGUI_CONSOLE_DATA GuiData = (PGUI_CONSOLE_DATA)lParam;

            /*
             * Window creation is done using a PostMessage(), so it's possible
             * that the window that we want to destroy doesn't exist yet.
             * So first empty the message queue.
             */
            /*
            while(PeekMessageW(&Msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&Msg);
                DispatchMessageW(&Msg);
            }*/
            while (PeekMessageW(&Msg, NULL, 0, 0, PM_REMOVE)) ;

            if (GuiData->hWindow != NULL) /* && DestroyWindow(GuiData->hWindow) */
            {
                DestroyWindow(GuiData->hWindow);

                WindowCount = GetWindowLongW(hWnd, GWL_USERDATA);
                WindowCount--;
                SetWindowLongW(hWnd, GWL_USERDATA, WindowCount);
                if (0 == WindowCount)
                {
                    NotifyWnd = NULL;
                    DestroyWindow(hWnd);
                    DPRINT1("CONSRV: Going to quit the Gui Thread!!\n");
                    PostQuitMessage(0);
                }
            }

            return 0;
        }

        default:
            return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
}

static DWORD WINAPI
GuiConsoleGuiThread(PVOID Data)
{
    MSG msg;
    PHANDLE GraphicsStartupEvent = (PHANDLE)Data;

    /*
     * This thread dispatches all the console notifications to the notify window.
     * It is common for all the console windows.
     */

    PrivateCsrssManualGuiCheck(+1);

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

    DPRINT1("CONSRV: Quit the Gui Thread!!\n");
    PrivateCsrssManualGuiCheck(-1);

    return 1;
}

static BOOL
GuiInit(VOID)
{
    WNDCLASSEXW wc;
    ATOM ConsoleClassAtom;

    /* Exit if we were already initialized */
    // if (ConsInitialized) return TRUE;

    /*
     * Initialize and register the different window classes, if needed.
     */
    if (!ConsInitialized)
    {
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

        ConsInitialized = TRUE;
    }

    /*
     * Set-up the notification window
     */
    if (NULL == NotifyWnd)
    {
        HANDLE ThreadHandle;
        HANDLE GraphicsStartupEvent;

        GraphicsStartupEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
        if (NULL == GraphicsStartupEvent) return FALSE;

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
            return FALSE;
        }
        SetThreadPriority(ThreadHandle, THREAD_PRIORITY_HIGHEST);
        CloseHandle(ThreadHandle);

        WaitForSingleObject(GraphicsStartupEvent, INFINITE);
        CloseHandle(GraphicsStartupEvent);

        if (NULL == NotifyWnd)
        {
            DPRINT1("CONSRV: Failed to create notification window.\n");
            return FALSE;
        }
    }

    // ConsInitialized = TRUE;

    return TRUE;
}



/******************************************************************************
 *                             GUI Console Driver                             *
 ******************************************************************************/

static VOID WINAPI
GuiCleanupConsole(PCONSOLE Console)
{
    PGUI_CONSOLE_DATA GuiData = Console->TermIFace.Data;

    SendMessageW(NotifyWnd, PM_DESTROY_CONSOLE, 0, (LPARAM)GuiData);

    DPRINT1("Destroying icons !! - GuiData->hIcon = 0x%p ; ghDefaultIcon = 0x%p ; GuiData->hIconSm = 0x%p ; ghDefaultIconSm = 0x%p\n",
            GuiData->hIcon, ghDefaultIcon, GuiData->hIconSm, ghDefaultIconSm);
    if (GuiData->hIcon != NULL && GuiData->hIcon != ghDefaultIcon)
    {
        DPRINT1("Destroy hIcon\n");
        DestroyIcon(GuiData->hIcon);
    }
    if (GuiData->hIconSm != NULL && GuiData->hIconSm != ghDefaultIconSm)
    {
        DPRINT1("Destroy hIconSm\n");
        DestroyIcon(GuiData->hIconSm);
    }

    Console->TermIFace.Data = NULL;
    DeleteCriticalSection(&GuiData->Lock);
    RtlFreeHeap(ConSrvHeap, 0, GuiData);

    DPRINT1("Quit GuiCleanupConsole\n");
}

static VOID WINAPI
GuiWriteStream(PCONSOLE Console, SMALL_RECT* Region, LONG CursorStartX, LONG CursorStartY,
               UINT ScrolledLines, CHAR *Buffer, UINT Length)
{
    PGUI_CONSOLE_DATA GuiData = Console->TermIFace.Data;
    PCONSOLE_SCREEN_BUFFER Buff = Console->ActiveBuffer;
    LONG CursorEndX, CursorEndY;
    RECT ScrollRect;

    if (NULL == GuiData || NULL == GuiData->hWindow)
    {
        return;
    }

    if (0 != ScrolledLines)
    {
        ScrollRect.left = 0;
        ScrollRect.top = 0;
        ScrollRect.right = Console->ConsoleSize.X * GuiData->CharWidth;
        ScrollRect.bottom = Region->Top * GuiData->CharHeight;

        ScrollWindowEx(GuiData->hWindow,
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
    SetTimer(GuiData->hWindow, CONGUI_UPDATE_TIMER, CONGUI_UPDATE_TIME, NULL);
}

static VOID WINAPI
GuiDrawRegion(PCONSOLE Console, SMALL_RECT* Region)
{
    PGUI_CONSOLE_DATA GuiData = Console->TermIFace.Data;
    RECT RegionRect;

    SmallRectToRect(GuiData, &RegionRect, Region);
    InvalidateRect(GuiData->hWindow, &RegionRect, FALSE);
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
    if (Size.X < Console->ConsoleSize.X || Size.Y < Console->ConsoleSize.Y)
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
    if (ScreenBuffer->ShowX > Size.X - Console->ConsoleSize.X)
        ScreenBuffer->ShowX = Size.X - Console->ConsoleSize.X;
    if (ScreenBuffer->ShowY > Size.Y - Console->ConsoleSize.Y)
        ScreenBuffer->ShowY = Size.Y - Console->ConsoleSize.Y;

    /* TODO: Should update scrollbar, but can't use anything that
     * calls SendMessage or it could cause deadlock --> Use PostMessage */
    // TODO: Tell the terminal to resize its scrollbars.

    return STATUS_SUCCESS;
}

static VOID WINAPI
GuiResizeTerminal(PCONSOLE Console)
{
    PGUI_CONSOLE_DATA GuiData = Console->TermIFace.Data;

    /* Resize the window to the user's values */
    GuiData->WindowSizeLock = TRUE;
    GuiConsoleResizeWindow(GuiData);
    GuiData->WindowSizeLock = FALSE;
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
GuiRefreshInternalInfo(PCONSOLE Console)
{
    PGUI_CONSOLE_DATA GuiData = Console->TermIFace.Data;

    /* Update the console leader information held by the window */
    SetConsoleWndConsoleLeaderCID(GuiData);
}

static VOID WINAPI
GuiChangeTitle(PCONSOLE Console)
{
    PGUI_CONSOLE_DATA GuiData = Console->TermIFace.Data;
    // PostMessageW(GuiData->hWindow, PM_CONSOLE_SET_TITLE, 0, 0);
    SetWindowText(GuiData->hWindow, Console->Title.Buffer);
}

static BOOL WINAPI
GuiChangeIcon(PCONSOLE Console, HICON hWindowIcon)
{
    PGUI_CONSOLE_DATA GuiData = Console->TermIFace.Data;
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

    if (hIcon != GuiData->hIcon)
    {
        if (GuiData->hIcon != NULL && GuiData->hIcon != ghDefaultIcon)
        {
            DestroyIcon(GuiData->hIcon);
        }
        if (GuiData->hIconSm != NULL && GuiData->hIconSm != ghDefaultIconSm)
        {
            DestroyIcon(GuiData->hIconSm);
        }

        GuiData->hIcon   = hIcon;
        GuiData->hIconSm = hIconSm;

        DPRINT1("Set icons in GuiChangeIcon\n");
        PostMessageW(GuiData->hWindow, WM_SETICON, ICON_BIG, (LPARAM)GuiData->hIcon);
        PostMessageW(GuiData->hWindow, WM_SETICON, ICON_SMALL, (LPARAM)GuiData->hIconSm);
    }

    return TRUE;
}

static HWND WINAPI
GuiGetConsoleWindowHandle(PCONSOLE Console)
{
    PGUI_CONSOLE_DATA GuiData = Console->TermIFace.Data;
    return GuiData->hWindow;
}

static FRONTEND_VTBL GuiVtbl =
{
    GuiCleanupConsole,
    GuiWriteStream,
    GuiDrawRegion,
    GuiSetCursorInfo,
    GuiSetScreenInfo,
    GuiUpdateScreenInfo,
    GuiResizeBuffer,
    GuiResizeTerminal,
    GuiProcessKeyCallback,
    GuiRefreshInternalInfo,
    GuiChangeTitle,
    GuiChangeIcon,
    GuiGetConsoleWindowHandle
};

NTSTATUS FASTCALL
GuiInitConsole(PCONSOLE Console,
               /*IN*/ PCONSOLE_START_INFO ConsoleStartInfo,
               PCONSOLE_INFO ConsoleInfo,
               DWORD ProcessId,
               LPCWSTR IconPath,
               INT IconIndex)
{
    PGUI_CONSOLE_DATA GuiData;
    GUI_CONSOLE_INFO TermInfo;
    SIZE_T Length = 0;

    if (Console == NULL || ConsoleInfo == NULL)
        return STATUS_INVALID_PARAMETER;

    /* Initialize the GUI terminal emulator */
    if (!GuiInit()) return STATUS_UNSUCCESSFUL;

    /* Initialize the console */
    Console->TermIFace.Vtbl = &GuiVtbl;

    GuiData = RtlAllocateHeap(ConSrvHeap, HEAP_ZERO_MEMORY,
                              sizeof(GUI_CONSOLE_DATA));
    if (!GuiData)
    {
        DPRINT1("CONSRV: Failed to create GUI_CONSOLE_DATA\n");
        return STATUS_UNSUCCESSFUL;
    }
    Console->TermIFace.Data = (PVOID)GuiData;
    GuiData->Console = Console;
    GuiData->hWindow = NULL;

    InitializeCriticalSection(&GuiData->Lock);


    /*
     * Load the terminal settings
     */

    /***********************************************
     * Adapted from ConSrvInitConsole in console.c *
     ***********************************************/

    /* 1. Load the default settings */
    GuiConsoleGetDefaultSettings(&TermInfo, ProcessId);

    /* 2. Load the remaining console settings via the registry. */
    if ((ConsoleStartInfo->dwStartupFlags & STARTF_TITLEISLINKNAME) == 0)
    {
        /* Load the terminal infos from the registry. */
        GuiConsoleReadUserSettings(&TermInfo, ConsoleInfo->ConsoleTitle, ProcessId);

        /*
         * Now, update them with the properties the user might gave to us
         * via the STARTUPINFO structure before calling CreateProcess
         * (and which was transmitted via the ConsoleStartInfo structure).
         * We therefore overwrite the values read in the registry.
         */
        if (ConsoleStartInfo->dwStartupFlags & STARTF_USESHOWWINDOW)
        {
            TermInfo.ShowWindow = ConsoleStartInfo->ShowWindow;
        }
        if (ConsoleStartInfo->dwStartupFlags & STARTF_USEPOSITION)
        {
            TermInfo.AutoPosition = FALSE;
            TermInfo.WindowOrigin = ConsoleStartInfo->ConsoleWindowOrigin;
        }
        /*
        if (ConsoleStartInfo->dwStartupFlags & STARTF_RUNFULLSCREEN)
        {
        }
        */
    }


    /*
     * Set up the GUI data
     */

    Length = min(wcslen(TermInfo.FaceName) + 1, LF_FACESIZE); // wcsnlen
    wcsncpy(GuiData->GuiInfo.FaceName, TermInfo.FaceName, LF_FACESIZE);
    GuiData->GuiInfo.FaceName[Length] = L'\0';
    GuiData->GuiInfo.FontFamily     = TermInfo.FontFamily;
    GuiData->GuiInfo.FontSize       = TermInfo.FontSize;
    GuiData->GuiInfo.FontWeight     = TermInfo.FontWeight;
    GuiData->GuiInfo.UseRasterFonts = TermInfo.UseRasterFonts;
    GuiData->GuiInfo.ShowWindow     = TermInfo.ShowWindow;
    GuiData->GuiInfo.AutoPosition   = TermInfo.AutoPosition;
    GuiData->GuiInfo.WindowOrigin   = TermInfo.WindowOrigin;

    /* Initialize the icon handles to their default values */
    GuiData->hIcon   = ghDefaultIcon;
    GuiData->hIconSm = ghDefaultIconSm;

    /* Get the associated icon, if any */
    if (IconPath == NULL || *IconPath == L'\0')
    {
        IconPath  = ConsoleStartInfo->AppPath;
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
            GuiData->hIcon   = hIcon;
            GuiData->hIconSm = hIconSm;
        }
    }

    /*
     * We need to wait until the GUI has been fully initialized
     * to retrieve custom settings i.e. WindowSize etc...
     * Ideally we could use SendNotifyMessage for this but its not
     * yet implemented.
     */
    GuiData->hGuiInitEvent = CreateEventW(NULL, FALSE, FALSE, NULL);

    /* Create the terminal window */
    PostMessageW(NotifyWnd, PM_CREATE_CONSOLE, GuiData->GuiInfo.ShowWindow, (LPARAM)GuiData);

    /* Wait until initialization has finished */
    WaitForSingleObject(GuiData->hGuiInitEvent, INFINITE);
    DPRINT1("OK we created the console window\n");
    CloseHandle(GuiData->hGuiInitEvent);
    GuiData->hGuiInitEvent = NULL;

    /* Check whether we really succeeded in initializing the terminal window */
    if (GuiData->hWindow == NULL)
    {
        DPRINT1("GuiInitConsole - We failed at creating a new terminal window\n");
        // ConioCleanupConsole(Console);
        GuiCleanupConsole(Console);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

/* EOF */
