/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            frontends/gui/conwnd.c
 * PURPOSE:         GUI Console Window Class
 * PROGRAMMERS:     Gé van Geldorp
 *                  Johannes Anderwald
 *                  Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include <consrv.h>
#include <intrin.h>
#include <windowsx.h>

#define NDEBUG
#include <debug.h>

#include "guiterm.h"
#include "conwnd.h"
#include "resource.h"

/* GLOBALS ********************************************************************/

// #define PM_CREATE_CONSOLE       (WM_APP + 1)
// #define PM_DESTROY_CONSOLE      (WM_APP + 2)

// See guiterm.c
#define CONGUI_MIN_WIDTH      10
#define CONGUI_MIN_HEIGHT     10
#define CONGUI_UPDATE_TIME    0
#define CONGUI_UPDATE_TIMER   1

#define CURSOR_BLINK_TIME 500


/**************************************************************\
\** Define the Console Leader Process for the console window **/
#define GWLP_CONWND_ALLOC      (2 * sizeof(LONG_PTR))
#define GWLP_CONSOLE_LEADER_PID 0
#define GWLP_CONSOLE_LEADER_TID 4

VOID
SetConWndConsoleLeaderCID(IN PGUI_CONSOLE_DATA GuiData)
{
    PCONSOLE_PROCESS_DATA ProcessData;
    CLIENT_ID ConsoleLeaderCID;

    ProcessData = ConSrvGetConsoleLeaderProcess(GuiData->Console);
    ConsoleLeaderCID = ProcessData->Process->ClientId;
    SetWindowLongPtrW(GuiData->hWindow, GWLP_CONSOLE_LEADER_PID,
                      (LONG_PTR)(ConsoleLeaderCID.UniqueProcess));
    SetWindowLongPtrW(GuiData->hWindow, GWLP_CONSOLE_LEADER_TID,
                      (LONG_PTR)(ConsoleLeaderCID.UniqueThread));
}
/**************************************************************/

HICON   ghDefaultIcon = NULL;
HICON   ghDefaultIconSm = NULL;
HCURSOR ghDefaultCursor = NULL;

typedef struct _GUICONSOLE_MENUITEM
{
    UINT uID;
    const struct _GUICONSOLE_MENUITEM *SubMenu;
    WORD wCmdID;
} GUICONSOLE_MENUITEM, *PGUICONSOLE_MENUITEM;

static const GUICONSOLE_MENUITEM GuiConsoleEditMenuItems[] =
{
    { IDS_MARK,         NULL, ID_SYSTEM_EDIT_MARK       },
    { IDS_COPY,         NULL, ID_SYSTEM_EDIT_COPY       },
    { IDS_PASTE,        NULL, ID_SYSTEM_EDIT_PASTE      },
    { IDS_SELECTALL,    NULL, ID_SYSTEM_EDIT_SELECTALL  },
    { IDS_SCROLL,       NULL, ID_SYSTEM_EDIT_SCROLL     },
    { IDS_FIND,         NULL, ID_SYSTEM_EDIT_FIND       },

    { 0, NULL, 0 } /* End of list */
};

static const GUICONSOLE_MENUITEM GuiConsoleMainMenuItems[] =
{
    { IDS_EDIT,         GuiConsoleEditMenuItems, 0 },
    { IDS_DEFAULTS,     NULL, ID_SYSTEM_DEFAULTS   },
    { IDS_PROPERTIES,   NULL, ID_SYSTEM_PROPERTIES },

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

static LRESULT CALLBACK
ConWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

BOOLEAN
RegisterConWndClass(IN HINSTANCE hInstance)
{
    WNDCLASSEXW WndClass;
    ATOM WndClassAtom;

    ghDefaultIcon   = LoadImageW(hInstance,
                                 MAKEINTRESOURCEW(IDI_TERMINAL),
                                 IMAGE_ICON,
                                 GetSystemMetrics(SM_CXICON),
                                 GetSystemMetrics(SM_CYICON),
                                 LR_SHARED);
    ghDefaultIconSm = LoadImageW(hInstance,
                                 MAKEINTRESOURCEW(IDI_TERMINAL),
                                 IMAGE_ICON,
                                 GetSystemMetrics(SM_CXSMICON),
                                 GetSystemMetrics(SM_CYSMICON),
                                 LR_SHARED);
    ghDefaultCursor = LoadCursorW(NULL, IDC_ARROW);

    WndClass.cbSize = sizeof(WNDCLASSEXW);
    WndClass.lpszClassName = GUI_CONWND_CLASS;
    WndClass.lpfnWndProc = ConWndProc;
    WndClass.style = CS_DBLCLKS /* | CS_HREDRAW | CS_VREDRAW */;
    WndClass.hInstance = hInstance;
    WndClass.hIcon = ghDefaultIcon;
    WndClass.hIconSm = ghDefaultIconSm;
    WndClass.hCursor = ghDefaultCursor;
    WndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH); // The color of a terminal when it is switched off.
    WndClass.lpszMenuName = NULL;
    WndClass.cbClsExtra = 0;
    WndClass.cbWndExtra = GWLP_CONWND_ALLOC;

    WndClassAtom = RegisterClassExW(&WndClass);
    if (WndClassAtom == 0)
    {
        DPRINT1("Failed to register GUI console class\n");
    }
    else
    {
        NtUserConsoleControl(GuiConsoleWndClassAtom, &WndClassAtom, sizeof(ATOM));
    }

    return (WndClassAtom != 0);
}

BOOLEAN
UnRegisterConWndClass(HINSTANCE hInstance)
{
    return !!UnregisterClassW(GUI_CONWND_CLASS, hInstance);
}



static VOID
GetScreenBufferSizeUnits(IN PCONSOLE_SCREEN_BUFFER Buffer,
                         IN PGUI_CONSOLE_DATA GuiData,
                         OUT PUINT WidthUnit,
                         OUT PUINT HeightUnit)
{
    if (Buffer == NULL || GuiData == NULL ||
        WidthUnit == NULL || HeightUnit == NULL)
    {
        return;
    }

    if (GetType(Buffer) == TEXTMODE_BUFFER)
    {
        *WidthUnit  = GuiData->CharWidth ;
        *HeightUnit = GuiData->CharHeight;
    }
    else /* if (GetType(Buffer) == GRAPHICS_BUFFER) */
    {
        *WidthUnit  = 1;
        *HeightUnit = 1;
    }
}

static VOID
AppendMenuItems(HMENU hMenu,
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
                        AppendMenuItems(hSubMenu, Items[i].SubMenu);

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
    } while (!(Items[i].uID == 0 && Items[i].SubMenu == NULL && Items[i].wCmdID == 0));
}

//static
VOID
CreateSysMenu(HWND hWnd)
{
    MENUITEMINFOW mii;
    WCHAR szMenuStringBack[255];
    WCHAR *ptrTab;
    HMENU hMenu = GetSystemMenu(hWnd, FALSE);
    if (hMenu != NULL)
    {
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_STRING;
        mii.dwTypeData = szMenuStringBack;
        mii.cch = sizeof(szMenuStringBack)/sizeof(WCHAR);

        GetMenuItemInfoW(hMenu, SC_CLOSE, FALSE, &mii);

        ptrTab = wcschr(szMenuStringBack, '\t');
        if (ptrTab)
        {
           *ptrTab = '\0';
           mii.cch = wcslen(szMenuStringBack);

           SetMenuItemInfoW(hMenu, SC_CLOSE, FALSE, &mii);
        }

        AppendMenuItems(hMenu, GuiConsoleMainMenuItems);
        DrawMenuBar(hWnd);
    }
}

static VOID
SendMenuEvent(PCONSRV_CONSOLE Console, UINT CmdId)
{
    INPUT_RECORD er;

    DPRINT("Menu item ID: %d\n", CmdId);

    if (!ConDrvValidateConsoleUnsafe((PCONSOLE)Console, CONSOLE_RUNNING, TRUE)) return;

    er.EventType = MENU_EVENT;
    er.Event.MenuEvent.dwCommandId = CmdId;
    ConioProcessInputEvent(Console, &er);

    LeaveCriticalSection(&Console->Lock);
}

static VOID
Copy(PGUI_CONSOLE_DATA GuiData);
static VOID
Paste(PGUI_CONSOLE_DATA GuiData);
static VOID
UpdateSelection(PGUI_CONSOLE_DATA GuiData,
                PCOORD SelectionAnchor OPTIONAL,
                PCOORD coord);

static VOID
Mark(PGUI_CONSOLE_DATA GuiData)
{
    PCONSOLE_SCREEN_BUFFER ActiveBuffer = GuiData->ActiveBuffer;

    /* Clear the old selection */
    GuiData->Selection.dwFlags = CONSOLE_NO_SELECTION;

    /* Restart a new selection */
    GuiData->dwSelectionCursor = ActiveBuffer->ViewOrigin;
    UpdateSelection(GuiData,
                    &GuiData->dwSelectionCursor,
                    &GuiData->dwSelectionCursor);
}

static VOID
SelectAll(PGUI_CONSOLE_DATA GuiData)
{
    PCONSOLE_SCREEN_BUFFER ActiveBuffer = GuiData->ActiveBuffer;
    COORD SelectionAnchor;

    /* Clear the old selection */
    GuiData->Selection.dwFlags = CONSOLE_NO_SELECTION;

    /*
     * The selection area extends to the whole screen buffer's width.
     */
    SelectionAnchor.X = SelectionAnchor.Y = 0;
    GuiData->dwSelectionCursor.X = ActiveBuffer->ScreenBufferSize.X - 1;

    /*
     * Determine whether the selection must extend to just some part
     * (for text-mode screen buffers) or to all of the screen buffer's
     * height (for graphics ones).
     */
    if (GetType(ActiveBuffer) == TEXTMODE_BUFFER)
    {
        /*
         * We select all the characters from the first line
         * to the line where the cursor is positioned.
         */
        GuiData->dwSelectionCursor.Y = ActiveBuffer->CursorPosition.Y;
    }
    else /* if (GetType(ActiveBuffer) == GRAPHICS_BUFFER) */
    {
        /*
         * We select all the screen buffer area.
         */
        GuiData->dwSelectionCursor.Y = ActiveBuffer->ScreenBufferSize.Y - 1;
    }

    /* Restart a new selection */
    GuiData->Selection.dwFlags |= CONSOLE_MOUSE_SELECTION;
    UpdateSelection(GuiData, &SelectionAnchor, &GuiData->dwSelectionCursor);
}

static LRESULT
OnCommand(PGUI_CONSOLE_DATA GuiData, WPARAM wParam, LPARAM lParam)
{
    LRESULT Ret = TRUE;
    PCONSRV_CONSOLE Console = GuiData->Console;

    /*
     * In case the selected menu item belongs to the user-reserved menu id range,
     * send to him a menu event and return directly. The user must handle those
     * reserved menu commands...
     */
    if (GuiData->CmdIdLow <= (UINT)wParam && (UINT)wParam <= GuiData->CmdIdHigh)
    {
        SendMenuEvent(Console, (UINT)wParam);
        goto Quit;
    }

    /* ... otherwise, perform actions. */
    switch (wParam)
    {
        case ID_SYSTEM_EDIT_MARK:
            Mark(GuiData);
            break;

        case ID_SYSTEM_EDIT_COPY:
            Copy(GuiData);
            break;

        case ID_SYSTEM_EDIT_PASTE:
            Paste(GuiData);
            break;

        case ID_SYSTEM_EDIT_SELECTALL:
            SelectAll(GuiData);
            break;

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

static VOID
ResizeConWnd(PGUI_CONSOLE_DATA GuiData, DWORD WidthUnit, DWORD HeightUnit)
{
    PCONSOLE_SCREEN_BUFFER Buff = GuiData->ActiveBuffer;
    SCROLLINFO sInfo;

    DWORD Width, Height;

    Width  = Buff->ViewSize.X * WidthUnit  +
             2 * (GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXEDGE));
    Height = Buff->ViewSize.Y * HeightUnit +
             2 * (GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CYEDGE)) + GetSystemMetrics(SM_CYCAPTION);

    /* Set scrollbar sizes */
    sInfo.cbSize = sizeof(SCROLLINFO);
    sInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    sInfo.nMin = 0;
    if (Buff->ScreenBufferSize.Y > Buff->ViewSize.Y)
    {
        sInfo.nMax  = Buff->ScreenBufferSize.Y - 1;
        sInfo.nPage = Buff->ViewSize.Y;
        sInfo.nPos  = Buff->ViewOrigin.Y;
        SetScrollInfo(GuiData->hWindow, SB_VERT, &sInfo, TRUE);
        Width += GetSystemMetrics(SM_CXVSCROLL);
        ShowScrollBar(GuiData->hWindow, SB_VERT, TRUE);
    }
    else
    {
        ShowScrollBar(GuiData->hWindow, SB_VERT, FALSE);
    }

    if (Buff->ScreenBufferSize.X > Buff->ViewSize.X)
    {
        sInfo.nMax  = Buff->ScreenBufferSize.X - 1;
        sInfo.nPage = Buff->ViewSize.X;
        sInfo.nPos  = Buff->ViewOrigin.X;
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
                 SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOCOPYBITS);
    // NOTE: The SWP_NOCOPYBITS flag can be replaced by a subsequent call
    // to: InvalidateRect(GuiData->hWindow, NULL, TRUE);
}


VOID
DeleteFonts(PGUI_CONSOLE_DATA GuiData)
{
    ULONG i;
    for (i = 0; i < sizeof(GuiData->Font) / sizeof(GuiData->Font[0]); ++i)
    {
        if (GuiData->Font[i] != NULL) DeleteObject(GuiData->Font[i]);
        GuiData->Font[i] = NULL;
    }
}

static HFONT
CreateDerivedFont(HFONT OrgFont,
                  // COORD   FontSize,
                  ULONG   FontWeight,
                  // BOOLEAN bItalic,
                  BOOLEAN bUnderline,
                  BOOLEAN bStrikeOut)
{
    LOGFONT lf;

    /* Initialize the LOGFONT structure */
    RtlZeroMemory(&lf, sizeof(lf));

    /* Retrieve the details of the current font */
    if (GetObject(OrgFont, sizeof(lf), &lf) == 0)
        return NULL;

    /* Change the font attributes */
    // lf.lfHeight = FontSize.Y;
    // lf.lfWidth  = FontSize.X;
    lf.lfWeight = FontWeight;
    // lf.lfItalic = bItalic;
    lf.lfUnderline = bUnderline;
    lf.lfStrikeOut = bStrikeOut;

    /* Build a new font */
    return CreateFontIndirect(&lf);
}

BOOL
InitFonts(PGUI_CONSOLE_DATA GuiData,
          LPWSTR FaceName, // Points to a WCHAR array of LF_FACESIZE elements.
          ULONG  FontFamily,
          COORD  FontSize,
          ULONG  FontWeight)
{
    HDC hDC;
    HFONT OldFont, NewFont;
    TEXTMETRICW Metrics;
    SIZE CharSize;

    hDC = GetDC(GuiData->hWindow);

    /*
     * Initialize a new NORMAL font and get its metrics.
     */

    FontSize.Y = FontSize.Y > 0 ? -MulDiv(FontSize.Y, GetDeviceCaps(hDC, LOGPIXELSY), 72)
                                : FontSize.Y;

    NewFont = CreateFontW(FontSize.Y,
                          FontSize.X,
                          0,
                          TA_BASELINE,
                          FontWeight,
                          FALSE,
                          FALSE,
                          FALSE,
                          OEM_CHARSET,
                          OUT_DEFAULT_PRECIS,
                          CLIP_DEFAULT_PRECIS,
                          DEFAULT_QUALITY,
                          FIXED_PITCH | FontFamily,
                          FaceName);
    if (NewFont == NULL)
    {
        DPRINT1("InitFonts: CreateFontW failed\n");
        ReleaseDC(GuiData->hWindow, hDC);
        return FALSE;
    }

    OldFont = SelectObject(hDC, NewFont);
    if (OldFont == NULL)
    {
        DPRINT1("InitFonts: SelectObject failed\n");
        ReleaseDC(GuiData->hWindow, hDC);
        DeleteObject(NewFont);
        return FALSE;
    }

    if (!GetTextMetricsW(hDC, &Metrics))
    {
        DPRINT1("InitFonts: GetTextMetrics failed\n");
        SelectObject(hDC, OldFont);
        ReleaseDC(GuiData->hWindow, hDC);
        DeleteObject(NewFont);
        return FALSE;
    }
    GuiData->CharWidth  = Metrics.tmMaxCharWidth;
    GuiData->CharHeight = Metrics.tmHeight + Metrics.tmExternalLeading;

    /* Measure real char width more precisely if possible */
    if (GetTextExtentPoint32W(hDC, L"R", 1, &CharSize))
        GuiData->CharWidth = CharSize.cx;

    SelectObject(hDC, OldFont);
    ReleaseDC(GuiData->hWindow, hDC);

    /*
     * Initialization succeeded.
     */
    // Delete all the old fonts first.
    DeleteFonts(GuiData);
    GuiData->Font[FONT_NORMAL] = NewFont;

    /*
     * Now build the other fonts (bold, underlined, mixed).
     */
    GuiData->Font[FONT_BOLD] =
        CreateDerivedFont(GuiData->Font[FONT_NORMAL],
                          FontWeight < FW_BOLD ? FW_BOLD : FontWeight,
                          FALSE,
                          FALSE);
    GuiData->Font[FONT_UNDERLINE] =
        CreateDerivedFont(GuiData->Font[FONT_NORMAL],
                          FontWeight,
                          TRUE,
                          FALSE);
    GuiData->Font[FONT_BOLD | FONT_UNDERLINE] =
        CreateDerivedFont(GuiData->Font[FONT_NORMAL],
                          FontWeight < FW_BOLD ? FW_BOLD : FontWeight,
                          TRUE,
                          FALSE);

    /*
     * Save the settings.
     */
    if (FaceName != GuiData->GuiInfo.FaceName)
    {
        wcsncpy(GuiData->GuiInfo.FaceName, FaceName, LF_FACESIZE);
        GuiData->GuiInfo.FaceName[LF_FACESIZE - 1] = UNICODE_NULL;
    }
    GuiData->GuiInfo.FontFamily = FontFamily;
    GuiData->GuiInfo.FontSize   = FontSize;
    GuiData->GuiInfo.FontWeight = FontWeight;

    return TRUE;
}


static BOOL
OnNcCreate(HWND hWnd, LPCREATESTRUCTW Create)
{
    PGUI_CONSOLE_DATA GuiData = (PGUI_CONSOLE_DATA)Create->lpCreateParams;
    PCONSRV_CONSOLE Console;

    if (NULL == GuiData)
    {
        DPRINT1("GuiConsoleNcCreate: No GUI data\n");
        return FALSE;
    }

    Console = GuiData->Console;

    GuiData->hWindow = hWnd;

    /* Initialize the fonts */
    if (!InitFonts(GuiData,
                   GuiData->GuiInfo.FaceName,
                   GuiData->GuiInfo.FontFamily,
                   GuiData->GuiInfo.FontSize,
                   GuiData->GuiInfo.FontWeight))
    {
        DPRINT1("GuiConsoleNcCreate: InitFonts failed\n");
        GuiData->hWindow = NULL;
        NtSetEvent(GuiData->hGuiInitEvent, NULL);
        return FALSE;
    }

    /* Initialize the terminal framebuffer */
    GuiData->hMemDC  = CreateCompatibleDC(NULL);
    GuiData->hBitmap = NULL;
    GuiData->hSysPalette = NULL; /* Original system palette */

    /* Update the icons of the window */
    if (GuiData->hIcon != ghDefaultIcon)
    {
        DefWindowProcW(GuiData->hWindow, WM_SETICON, ICON_BIG  , (LPARAM)GuiData->hIcon  );
        DefWindowProcW(GuiData->hWindow, WM_SETICON, ICON_SMALL, (LPARAM)GuiData->hIconSm);
    }

    // FIXME: Keep these instructions here ? ///////////////////////////////////
    Console->ActiveBuffer->CursorBlinkOn = TRUE;
    Console->ActiveBuffer->ForceCursorOff = FALSE;
    ////////////////////////////////////////////////////////////////////////////

    SetWindowLongPtrW(GuiData->hWindow, GWLP_USERDATA, (DWORD_PTR)GuiData);

    if (GuiData->IsWindowVisible)
    {
        SetTimer(GuiData->hWindow, CONGUI_UPDATE_TIMER, CONGUI_UPDATE_TIME, NULL);
    }

    // FIXME: HACK: Potential HACK for CORE-8129; see revision 63595.
    //CreateSysMenu(GuiData->hWindow);

    DPRINT("OnNcCreate - setting start event\n");
    NtSetEvent(GuiData->hGuiInitEvent, NULL);

    return (BOOL)DefWindowProcW(GuiData->hWindow, WM_NCCREATE, 0, (LPARAM)Create);
}


BOOL
EnterFullScreen(PGUI_CONSOLE_DATA GuiData);
VOID
LeaveFullScreen(PGUI_CONSOLE_DATA GuiData);
VOID
SwitchFullScreen(PGUI_CONSOLE_DATA GuiData, BOOL FullScreen);
VOID
GuiConsoleSwitchFullScreen(PGUI_CONSOLE_DATA GuiData);

static VOID
OnActivate(PGUI_CONSOLE_DATA GuiData, WPARAM wParam)
{
    WORD ActivationState = LOWORD(wParam);

    DPRINT("WM_ACTIVATE - ActivationState = %d\n", ActivationState);

    if ( ActivationState == WA_ACTIVE ||
         ActivationState == WA_CLICKACTIVE )
    {
        if (GuiData->GuiInfo.FullScreen)
        {
            EnterFullScreen(GuiData);
            // // PostMessageW(GuiData->hWindow, WM_SYSCOMMAND, SC_RESTORE, 0);
            // SendMessageW(GuiData->hWindow, WM_SYSCOMMAND, SC_RESTORE, 0);
        }
    }
    else // if (ActivationState == WA_INACTIVE)
    {
        if (GuiData->GuiInfo.FullScreen)
        {
            SendMessageW(GuiData->hWindow, WM_SYSCOMMAND, SC_MINIMIZE, 0);
            LeaveFullScreen(GuiData);
            // // PostMessageW(GuiData->hWindow, WM_SYSCOMMAND, SC_MINIMIZE, 0);
            // SendMessageW(GuiData->hWindow, WM_SYSCOMMAND, SC_MINIMIZE, 0);
        }
    }

    /*
     * Ignore the next mouse signal when we are going to be enabled again via
     * the mouse, in order to prevent, e.g. when we are in Edit mode, erroneous
     * mouse actions from the user that could spoil text selection or copy/pastes.
     */
    if (ActivationState == WA_CLICKACTIVE)
        GuiData->IgnoreNextMouseSignal = TRUE;
}

static VOID
OnFocus(PGUI_CONSOLE_DATA GuiData, BOOL SetFocus)
{
    PCONSRV_CONSOLE Console = GuiData->Console;
    INPUT_RECORD er;

    if (!ConDrvValidateConsoleUnsafe((PCONSOLE)Console, CONSOLE_RUNNING, TRUE)) return;

    er.EventType = FOCUS_EVENT;
    er.Event.FocusEvent.bSetFocus = SetFocus;
    ConioProcessInputEvent(Console, &er);

    LeaveCriticalSection(&Console->Lock);

    if (SetFocus)
        DPRINT1("TODO: Create console caret\n");
    else
        DPRINT1("TODO: Destroy console caret\n");
}

static VOID
SmallRectToRect(PGUI_CONSOLE_DATA GuiData, PRECT Rect, PSMALL_RECT SmallRect)
{
    PCONSOLE_SCREEN_BUFFER Buffer = GuiData->ActiveBuffer;
    UINT WidthUnit, HeightUnit;

    GetScreenBufferSizeUnits(Buffer, GuiData, &WidthUnit, &HeightUnit);

    Rect->left   = (SmallRect->Left       - Buffer->ViewOrigin.X) * WidthUnit ;
    Rect->top    = (SmallRect->Top        - Buffer->ViewOrigin.Y) * HeightUnit;
    Rect->right  = (SmallRect->Right  + 1 - Buffer->ViewOrigin.X) * WidthUnit ;
    Rect->bottom = (SmallRect->Bottom + 1 - Buffer->ViewOrigin.Y) * HeightUnit;
}

VOID
GetSelectionBeginEnd(PCOORD Begin, PCOORD End,
                     PCOORD SelectionAnchor,
                     PSMALL_RECT SmallRect)
{
    if (Begin == NULL || End == NULL) return;

    *Begin = *SelectionAnchor;
    End->X = (SelectionAnchor->X == SmallRect->Left) ? SmallRect->Right
              /* Case X != Left, must be == Right */ : SmallRect->Left;
    End->Y = (SelectionAnchor->Y == SmallRect->Top ) ? SmallRect->Bottom
              /* Case Y != Top, must be == Bottom */ : SmallRect->Top;

    /* Exchange Begin / End if Begin > End lexicographically */
    if (Begin->Y > End->Y || (Begin->Y == End->Y && Begin->X > End->X))
    {
        End->X = _InterlockedExchange16(&Begin->X, End->X);
        End->Y = _InterlockedExchange16(&Begin->Y, End->Y);
    }
}

static HRGN
CreateSelectionRgn(PGUI_CONSOLE_DATA GuiData,
                   BOOL LineSelection,
                   PCOORD SelectionAnchor,
                   PSMALL_RECT SmallRect)
{
    if (!LineSelection)
    {
        RECT rect;
        SmallRectToRect(GuiData, &rect, SmallRect);
        return CreateRectRgnIndirect(&rect);
    }
    else
    {
        HRGN SelRgn;
        COORD Begin, End;

        GetSelectionBeginEnd(&Begin, &End, SelectionAnchor, SmallRect);

        if (Begin.Y == End.Y)
        {
            SMALL_RECT sr;
            RECT       r ;

            sr.Left   = Begin.X;
            sr.Top    = Begin.Y;
            sr.Right  = End.X;
            sr.Bottom = End.Y;

            // Debug thingie to see whether I can put this corner case
            // together with the previous one.
            if (SmallRect->Left   != sr.Left  ||
                SmallRect->Top    != sr.Top   ||
                SmallRect->Right  != sr.Right ||
                SmallRect->Bottom != sr.Bottom)
            {
                DPRINT1("\n"
                        "SmallRect = (%d, %d, %d, %d)\n"
                        "sr = (%d, %d, %d, %d)\n"
                        "\n",
                        SmallRect->Left, SmallRect->Top, SmallRect->Right, SmallRect->Bottom,
                        sr.Left, sr.Top, sr.Right, sr.Bottom);
            }

            SmallRectToRect(GuiData, &r, &sr);
            SelRgn = CreateRectRgnIndirect(&r);
        }
        else
        {
            PCONSOLE_SCREEN_BUFFER ActiveBuffer = GuiData->ActiveBuffer;

            HRGN       rg1, rg2, rg3;
            SMALL_RECT sr1, sr2, sr3;
            RECT       r1 , r2 , r3 ;

            sr1.Left   = Begin.X;
            sr1.Top    = Begin.Y;
            sr1.Right  = ActiveBuffer->ScreenBufferSize.X - 1;
            sr1.Bottom = Begin.Y;

            sr2.Left   = 0;
            sr2.Top    = Begin.Y + 1;
            sr2.Right  = ActiveBuffer->ScreenBufferSize.X - 1;
            sr2.Bottom = End.Y - 1;

            sr3.Left   = 0;
            sr3.Top    = End.Y;
            sr3.Right  = End.X;
            sr3.Bottom = End.Y;

            SmallRectToRect(GuiData, &r1, &sr1);
            SmallRectToRect(GuiData, &r2, &sr2);
            SmallRectToRect(GuiData, &r3, &sr3);

            rg1 = CreateRectRgnIndirect(&r1);
            rg2 = CreateRectRgnIndirect(&r2);
            rg3 = CreateRectRgnIndirect(&r3);

            CombineRgn(rg1, rg1, rg2, RGN_XOR);
            CombineRgn(rg1, rg1, rg3, RGN_XOR);
            DeleteObject(rg3);
            DeleteObject(rg2);

            SelRgn = rg1;
        }

        return SelRgn;
    }
}

static VOID
PaintSelectionRect(PGUI_CONSOLE_DATA GuiData, PPAINTSTRUCT pps)
{
    HRGN rgnPaint = CreateRectRgnIndirect(&pps->rcPaint);
    HRGN rgnSel   = CreateSelectionRgn(GuiData, GuiData->LineSelection,
                                       &GuiData->Selection.dwSelectionAnchor,
                                       &GuiData->Selection.srSelection);

    /* Invert the selection */

    int ErrorCode = CombineRgn(rgnPaint, rgnPaint, rgnSel, RGN_AND);
    if (ErrorCode != ERROR && ErrorCode != NULLREGION)
    {
        InvertRgn(pps->hdc, rgnPaint);
    }

    DeleteObject(rgnSel);
    DeleteObject(rgnPaint);
}

static VOID
UpdateSelection(PGUI_CONSOLE_DATA GuiData,
                PCOORD SelectionAnchor OPTIONAL,
                PCOORD coord)
{
    PCONSRV_CONSOLE Console = GuiData->Console;
    HRGN oldRgn = CreateSelectionRgn(GuiData, GuiData->LineSelection,
                                     &GuiData->Selection.dwSelectionAnchor,
                                     &GuiData->Selection.srSelection);

    /* Update the anchor if needed (use the old one if NULL) */
    if (SelectionAnchor)
        GuiData->Selection.dwSelectionAnchor = *SelectionAnchor;

    if (coord != NULL)
    {
        SMALL_RECT rc;
        HRGN newRgn;

        /*
         * Pressing the Control key while selecting text, allows us to enter
         * into line-selection mode, the selection mode of *nix terminals.
         */
        BOOL OldLineSel = GuiData->LineSelection;
        GuiData->LineSelection = !!(GetKeyState(VK_CONTROL) & 0x8000);

        /* Exchange left/top with right/bottom if required */
        rc.Left   = min(GuiData->Selection.dwSelectionAnchor.X, coord->X);
        rc.Top    = min(GuiData->Selection.dwSelectionAnchor.Y, coord->Y);
        rc.Right  = max(GuiData->Selection.dwSelectionAnchor.X, coord->X);
        rc.Bottom = max(GuiData->Selection.dwSelectionAnchor.Y, coord->Y);

        newRgn = CreateSelectionRgn(GuiData, GuiData->LineSelection,
                                    &GuiData->Selection.dwSelectionAnchor,
                                    &rc);

        if (GuiData->Selection.dwFlags & CONSOLE_SELECTION_NOT_EMPTY)
        {
            if (OldLineSel != GuiData->LineSelection ||
                memcmp(&rc, &GuiData->Selection.srSelection, sizeof(SMALL_RECT)) != 0)
            {
                /* Calculate the region that needs to be updated */
                if (oldRgn && newRgn && CombineRgn(newRgn, newRgn, oldRgn, RGN_XOR) != ERROR)
                {
                    InvalidateRgn(GuiData->hWindow, newRgn, FALSE);
                }
            }
        }
        else
        {
            InvalidateRgn(GuiData->hWindow, newRgn, FALSE);
        }

        DeleteObject(newRgn);

        GuiData->Selection.dwFlags |= CONSOLE_SELECTION_NOT_EMPTY;
        GuiData->Selection.srSelection = rc;
        GuiData->dwSelectionCursor = *coord;

        if ((GuiData->Selection.dwFlags & CONSOLE_SELECTION_IN_PROGRESS) == 0)
        {
            LPWSTR SelTypeStr = NULL   , WindowTitle = NULL;
            SIZE_T SelTypeStrLength = 0, Length = 0;

            /* Clear the old selection */
            if (GuiData->Selection.dwFlags & CONSOLE_SELECTION_NOT_EMPTY)
            {
                InvalidateRgn(GuiData->hWindow, oldRgn, FALSE);
            }

            /*
             * When passing a zero-length buffer size, LoadString(...) returns
             * a read-only pointer buffer to the program's resource string.
             */
            SelTypeStrLength =
                LoadStringW(ConSrvDllInstance,
                            (GuiData->Selection.dwFlags & CONSOLE_MOUSE_SELECTION)
                                ? IDS_SELECT_TITLE : IDS_MARK_TITLE,
                            (LPWSTR)&SelTypeStr, 0);

            /*
             * Prepend the selection type string to the current console title
             * if we succeeded in retrieving a valid localized string.
             */
            if (SelTypeStr)
            {
                // 3 for " - " and 1 for NULL
                Length = Console->Title.Length + (SelTypeStrLength + 3 + 1) * sizeof(WCHAR);
                WindowTitle = ConsoleAllocHeap(0, Length);

                wcsncpy(WindowTitle, SelTypeStr, SelTypeStrLength);
                WindowTitle[SelTypeStrLength] = L'\0';
                wcscat(WindowTitle, L" - ");
                wcscat(WindowTitle, Console->Title.Buffer);

                SetWindowTextW(GuiData->hWindow, WindowTitle);
                ConsoleFreeHeap(WindowTitle);
            }

            GuiData->Selection.dwFlags |= CONSOLE_SELECTION_IN_PROGRESS;
            ConioPause(Console, PAUSED_FROM_SELECTION);
        }
    }
    else
    {
        /* Clear the selection */
        if (GuiData->Selection.dwFlags & CONSOLE_SELECTION_NOT_EMPTY)
        {
            InvalidateRgn(GuiData->hWindow, oldRgn, FALSE);
        }

        GuiData->Selection.dwFlags = CONSOLE_NO_SELECTION;
        ConioUnpause(Console, PAUSED_FROM_SELECTION);

        /* Restore the console title */
        SetWindowTextW(GuiData->hWindow, Console->Title.Buffer);
    }

    DeleteObject(oldRgn);
}


VOID
GuiPaintTextModeBuffer(PTEXTMODE_SCREEN_BUFFER Buffer,
                       PGUI_CONSOLE_DATA GuiData,
                       PRECT rcView,
                       PRECT rcFramebuffer);
VOID
GuiPaintGraphicsBuffer(PGRAPHICS_SCREEN_BUFFER Buffer,
                       PGUI_CONSOLE_DATA GuiData,
                       PRECT rcView,
                       PRECT rcFramebuffer);

static VOID
OnPaint(PGUI_CONSOLE_DATA GuiData)
{
    PCONSOLE_SCREEN_BUFFER ActiveBuffer = GuiData->ActiveBuffer;
    PAINTSTRUCT ps;
    RECT rcPaint;

    /* Do nothing if the window is hidden */
    if (!GuiData->IsWindowVisible) return;

    BeginPaint(GuiData->hWindow, &ps);
    if (ps.hdc != NULL &&
        ps.rcPaint.left < ps.rcPaint.right &&
        ps.rcPaint.top < ps.rcPaint.bottom)
    {
        EnterCriticalSection(&GuiData->Lock);

        /* Compose the current screen-buffer on-memory */
        if (GetType(ActiveBuffer) == TEXTMODE_BUFFER)
        {
            GuiPaintTextModeBuffer((PTEXTMODE_SCREEN_BUFFER)ActiveBuffer,
                                   GuiData, &ps.rcPaint, &rcPaint);
        }
        else /* if (GetType(ActiveBuffer) == GRAPHICS_BUFFER) */
        {
            GuiPaintGraphicsBuffer((PGRAPHICS_SCREEN_BUFFER)ActiveBuffer,
                                   GuiData, &ps.rcPaint, &rcPaint);
        }

        /* Send it to screen */
        BitBlt(ps.hdc,
               ps.rcPaint.left,
               ps.rcPaint.top,
               rcPaint.right  - rcPaint.left,
               rcPaint.bottom - rcPaint.top,
               GuiData->hMemDC,
               rcPaint.left,
               rcPaint.top,
               SRCCOPY);

        /* Draw the selection region if needed */
        if (GuiData->Selection.dwFlags & CONSOLE_SELECTION_NOT_EMPTY)
        {
            PaintSelectionRect(GuiData, &ps);
        }

        LeaveCriticalSection(&GuiData->Lock);
    }
    EndPaint(GuiData->hWindow, &ps);

    return;
}

static VOID
OnPaletteChanged(PGUI_CONSOLE_DATA GuiData)
{
    PCONSOLE_SCREEN_BUFFER ActiveBuffer = GuiData->ActiveBuffer;

    /* Do nothing if the window is hidden */
    if (!GuiData->IsWindowVisible) return;

    // See WM_PALETTECHANGED message
    // if ((HWND)wParam == hWnd) break;

    // if (GetType(ActiveBuffer) == GRAPHICS_BUFFER)
    if (ActiveBuffer->PaletteHandle)
    {
        DPRINT("WM_PALETTECHANGED changing palette\n");

        /* Specify the use of the system palette for the framebuffer */
        SetSystemPaletteUse(GuiData->hMemDC, ActiveBuffer->PaletteUsage);

        /* Realize the (logical) palette */
        RealizePalette(GuiData->hMemDC);
    }
}

static BOOL
IsSystemKey(WORD VirtualKeyCode)
{
    switch (VirtualKeyCode)
    {
        /* From MSDN, "Virtual-Key Codes" */
        case VK_RETURN:
        case VK_SHIFT:
        case VK_CONTROL:
        case VK_MENU:
        case VK_PAUSE:
        case VK_CAPITAL:
        case VK_ESCAPE:
        case VK_LWIN:
        case VK_RWIN:
        case VK_NUMLOCK:
        case VK_SCROLL:
            return TRUE;
        default:
            return FALSE;
    }
}

static VOID
OnKey(PGUI_CONSOLE_DATA GuiData, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PCONSRV_CONSOLE Console = GuiData->Console;
    PCONSOLE_SCREEN_BUFFER ActiveBuffer;

    if (!ConDrvValidateConsoleUnsafe((PCONSOLE)Console, CONSOLE_RUNNING, TRUE)) return;

    ActiveBuffer = GuiData->ActiveBuffer;

    if (GuiData->Selection.dwFlags & CONSOLE_SELECTION_IN_PROGRESS)
    {
        WORD VirtualKeyCode = LOWORD(wParam);

        if (msg != WM_KEYDOWN) goto Quit;

        if (VirtualKeyCode == VK_RETURN)
        {
            /* Copy (and clear) selection if ENTER is pressed */
            Copy(GuiData);
            goto Quit;
        }
        else if ( VirtualKeyCode == VK_ESCAPE ||
                 (VirtualKeyCode == 'C' && (GetKeyState(VK_CONTROL) & 0x8000)) )
        {
            /* Cancel selection if ESC or Ctrl-C are pressed */
            UpdateSelection(GuiData, NULL, NULL);
            goto Quit;
        }

        if ((GuiData->Selection.dwFlags & CONSOLE_MOUSE_SELECTION) == 0)
        {
            /* Keyboard selection mode */
            BOOL Interpreted = FALSE;
            BOOL MajPressed  = !!(GetKeyState(VK_SHIFT) & 0x8000);

            switch (VirtualKeyCode)
            {
                case VK_LEFT:
                {
                    Interpreted = TRUE;
                    if (GuiData->dwSelectionCursor.X > 0)
                        GuiData->dwSelectionCursor.X--;

                    break;
                }

                case VK_RIGHT:
                {
                    Interpreted = TRUE;
                    if (GuiData->dwSelectionCursor.X < ActiveBuffer->ScreenBufferSize.X - 1)
                        GuiData->dwSelectionCursor.X++;

                    break;
                }

                case VK_UP:
                {
                    Interpreted = TRUE;
                    if (GuiData->dwSelectionCursor.Y > 0)
                        GuiData->dwSelectionCursor.Y--;

                    break;
                }

                case VK_DOWN:
                {
                    Interpreted = TRUE;
                    if (GuiData->dwSelectionCursor.Y < ActiveBuffer->ScreenBufferSize.Y - 1)
                        GuiData->dwSelectionCursor.Y++;

                    break;
                }

                case VK_HOME:
                {
                    Interpreted = TRUE;
                    GuiData->dwSelectionCursor.X = 0;
                    GuiData->dwSelectionCursor.Y = 0;
                    break;
                }

                case VK_END:
                {
                    Interpreted = TRUE;
                    GuiData->dwSelectionCursor.Y = ActiveBuffer->ScreenBufferSize.Y - 1;
                    break;
                }

                case VK_PRIOR:
                {
                    Interpreted = TRUE;
                    GuiData->dwSelectionCursor.Y -= ActiveBuffer->ViewSize.Y;
                    if (GuiData->dwSelectionCursor.Y < 0)
                        GuiData->dwSelectionCursor.Y = 0;

                    break;
                }

                case VK_NEXT:
                {
                    Interpreted = TRUE;
                    GuiData->dwSelectionCursor.Y += ActiveBuffer->ViewSize.Y;
                    if (GuiData->dwSelectionCursor.Y >= ActiveBuffer->ScreenBufferSize.Y)
                        GuiData->dwSelectionCursor.Y  = ActiveBuffer->ScreenBufferSize.Y - 1;

                    break;
                }

                default:
                    break;
            }

            if (Interpreted)
            {
                UpdateSelection(GuiData,
                                !MajPressed ? &GuiData->dwSelectionCursor : NULL,
                                &GuiData->dwSelectionCursor);
            }
            else if (!IsSystemKey(VirtualKeyCode))
            {
                /* Emit an error beep sound */
                SendNotifyMessage(GuiData->hWindow, PM_CONSOLE_BEEP, 0, 0);
            }

            goto Quit;
        }
        else
        {
            /* Mouse selection mode */

            if (!IsSystemKey(VirtualKeyCode))
            {
                /* Clear the selection and send the key into the input buffer */
                UpdateSelection(GuiData, NULL, NULL);
            }
            else
            {
                goto Quit;
            }
        }
    }

    if ((GuiData->Selection.dwFlags & CONSOLE_SELECTION_IN_PROGRESS) == 0)
    {
        MSG Message;

        Message.hwnd = GuiData->hWindow;
        Message.message = msg;
        Message.wParam = wParam;
        Message.lParam = lParam;

        ConioProcessKey(Console, &Message);
    }

Quit:
    LeaveCriticalSection(&Console->Lock);
}


// FIXME: Remove after fixing OnTimer
VOID
InvalidateCell(PGUI_CONSOLE_DATA GuiData,
               SHORT x, SHORT y);

static VOID
OnTimer(PGUI_CONSOLE_DATA GuiData)
{
    PCONSRV_CONSOLE Console = GuiData->Console;
    PCONSOLE_SCREEN_BUFFER Buff;

    /* Do nothing if the window is hidden */
    if (!GuiData->IsWindowVisible) return;

    SetTimer(GuiData->hWindow, CONGUI_UPDATE_TIMER, CURSOR_BLINK_TIME, NULL);

    if (!ConDrvValidateConsoleUnsafe((PCONSOLE)Console, CONSOLE_RUNNING, TRUE)) return;

    Buff = GuiData->ActiveBuffer;

    if (GetType(Buff) == TEXTMODE_BUFFER)
    {
        InvalidateCell(GuiData, Buff->CursorPosition.X, Buff->CursorPosition.Y);
        Buff->CursorBlinkOn = !Buff->CursorBlinkOn;

        if ((GuiData->OldCursor.x != Buff->CursorPosition.X) ||
            (GuiData->OldCursor.y != Buff->CursorPosition.Y))
        {
            SCROLLINFO xScroll;
            int OldScrollX = -1, OldScrollY = -1;
            int NewScrollX = -1, NewScrollY = -1;

            xScroll.cbSize = sizeof(SCROLLINFO);
            xScroll.fMask = SIF_POS;
            // Capture the original position of the scroll bars and save them.
            if (GetScrollInfo(GuiData->hWindow, SB_HORZ, &xScroll)) OldScrollX = xScroll.nPos;
            if (GetScrollInfo(GuiData->hWindow, SB_VERT, &xScroll)) OldScrollY = xScroll.nPos;

            // If we successfully got the info for the horizontal scrollbar
            if (OldScrollX >= 0)
            {
                if ((Buff->CursorPosition.X < Buff->ViewOrigin.X) ||
                    (Buff->CursorPosition.X >= (Buff->ViewOrigin.X + Buff->ViewSize.X)))
                {
                    // Handle the horizontal scroll bar
                    if (Buff->CursorPosition.X >= Buff->ViewSize.X)
                        NewScrollX = Buff->CursorPosition.X - Buff->ViewSize.X + 1;
                    else
                        NewScrollX = 0;
                }
                else
                {
                    NewScrollX = OldScrollX;
                }
            }
            // If we successfully got the info for the vertical scrollbar
            if (OldScrollY >= 0)
            {
                if ((Buff->CursorPosition.Y < Buff->ViewOrigin.Y) ||
                    (Buff->CursorPosition.Y >= (Buff->ViewOrigin.Y + Buff->ViewSize.Y)))
                {
                    // Handle the vertical scroll bar
                    if (Buff->CursorPosition.Y >= Buff->ViewSize.Y)
                        NewScrollY = Buff->CursorPosition.Y - Buff->ViewSize.Y + 1;
                    else
                        NewScrollY = 0;
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
            if ((OldScrollX != NewScrollX) || (OldScrollY != NewScrollY))
            {
                Buff->ViewOrigin.X = NewScrollX;
                Buff->ViewOrigin.Y = NewScrollY;
                ScrollWindowEx(GuiData->hWindow,
                               (OldScrollX - NewScrollX) * GuiData->CharWidth,
                               (OldScrollY - NewScrollY) * GuiData->CharHeight,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               SW_INVALIDATE);
                if (NewScrollX >= 0)
                {
                    xScroll.nPos = NewScrollX;
                    SetScrollInfo(GuiData->hWindow, SB_HORZ, &xScroll, TRUE);
                }
                if (NewScrollY >= 0)
                {
                    xScroll.nPos = NewScrollY;
                    SetScrollInfo(GuiData->hWindow, SB_VERT, &xScroll, TRUE);
                }
                UpdateWindow(GuiData->hWindow);
                // InvalidateRect(GuiData->hWindow, NULL, FALSE);
                GuiData->OldCursor.x = Buff->CursorPosition.X;
                GuiData->OldCursor.y = Buff->CursorPosition.Y;
            }
        }
    }
    else /* if (GetType(Buff) == GRAPHICS_BUFFER) */
    {
    }

    LeaveCriticalSection(&Console->Lock);
}

static BOOL
OnClose(PGUI_CONSOLE_DATA GuiData)
{
    PCONSRV_CONSOLE Console = GuiData->Console;

    if (!ConDrvValidateConsoleUnsafe((PCONSOLE)Console, CONSOLE_RUNNING, TRUE))
        return TRUE;

    // TODO: Prompt for termination ? (Warn the user about possible apps running in this console)

    /*
     * FIXME: Windows will wait up to 5 seconds for the thread to exit.
     * We shouldn't wait here, though, since the console lock is entered.
     * A copy of the thread list probably needs to be made.
     */
    ConSrvConsoleProcessCtrlEvent(Console, 0, CTRL_CLOSE_EVENT);

    LeaveCriticalSection(&Console->Lock);
    return FALSE;
}

static LRESULT
OnNcDestroy(HWND hWnd)
{
    PGUI_CONSOLE_DATA GuiData = GuiGetGuiData(hWnd);

    if (GuiData->IsWindowVisible)
    {
        KillTimer(hWnd, CONGUI_UPDATE_TIMER);
    }

    GetSystemMenu(hWnd, TRUE);

    if (GuiData)
    {
        /* Free the terminal framebuffer */
        if (GuiData->hMemDC ) DeleteDC(GuiData->hMemDC);
        if (GuiData->hBitmap) DeleteObject(GuiData->hBitmap);
        // if (GuiData->hSysPalette) DeleteObject(GuiData->hSysPalette);
        DeleteFonts(GuiData);
    }

    /* Free the GuiData registration */
    SetWindowLongPtrW(hWnd, GWLP_USERDATA, (DWORD_PTR)NULL);

    return DefWindowProcW(hWnd, WM_NCDESTROY, 0, 0);
}

static COORD
PointToCoord(PGUI_CONSOLE_DATA GuiData, LPARAM lParam)
{
    PCONSOLE_SCREEN_BUFFER Buffer = GuiData->ActiveBuffer;
    COORD Coord;
    UINT  WidthUnit, HeightUnit;

    GetScreenBufferSizeUnits(Buffer, GuiData, &WidthUnit, &HeightUnit);

    Coord.X = Buffer->ViewOrigin.X + ((SHORT)LOWORD(lParam) / (int)WidthUnit );
    Coord.Y = Buffer->ViewOrigin.Y + ((SHORT)HIWORD(lParam) / (int)HeightUnit);

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
OnMouse(PGUI_CONSOLE_DATA GuiData, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL Err = FALSE;
    PCONSRV_CONSOLE Console = GuiData->Console;

    // FIXME: It's here that we need to check whether we has focus or not
    // and whether we are in edit mode or not, to know if we need to deal
    // with the mouse, or not.

    if (GuiData->IgnoreNextMouseSignal)
    {
        if (msg != WM_LBUTTONDOWN &&
            msg != WM_MBUTTONDOWN &&
            msg != WM_RBUTTONDOWN)
        {
            /*
             * If this mouse signal is not a button-down action
             * then it is the last signal being ignored.
             */
            GuiData->IgnoreNextMouseSignal = FALSE;
        }
        else
        {
            /*
             * This mouse signal is a button-down action.
             * Ignore it and perform default action.
             */
            Err = TRUE;
        }
        goto Quit;
    }

    if (!ConDrvValidateConsoleUnsafe((PCONSOLE)Console, CONSOLE_RUNNING, TRUE))
    {
        Err = TRUE;
        goto Quit;
    }

    if ( (GuiData->Selection.dwFlags & CONSOLE_SELECTION_IN_PROGRESS) ||
         (Console->QuickEdit) )
    {
        switch (msg)
        {
            case WM_LBUTTONDOWN:
            {
                /* Clear the old selection */
                GuiData->Selection.dwFlags = CONSOLE_NO_SELECTION;

                /* Restart a new selection */
                GuiData->dwSelectionCursor = PointToCoord(GuiData, lParam);
                SetCapture(GuiData->hWindow);
                GuiData->Selection.dwFlags |= CONSOLE_MOUSE_SELECTION | CONSOLE_MOUSE_DOWN;
                UpdateSelection(GuiData,
                                &GuiData->dwSelectionCursor,
                                &GuiData->dwSelectionCursor);

                break;
            }

            case WM_LBUTTONUP:
            {
                if (!(GuiData->Selection.dwFlags & CONSOLE_MOUSE_DOWN)) break;

                // GuiData->dwSelectionCursor = PointToCoord(GuiData, lParam);
                GuiData->Selection.dwFlags &= ~CONSOLE_MOUSE_DOWN;
                // UpdateSelection(GuiData, NULL, &GuiData->dwSelectionCursor);
                ReleaseCapture();

                break;
            }

            case WM_LBUTTONDBLCLK:
            {
                PCONSOLE_SCREEN_BUFFER Buffer = GuiData->ActiveBuffer;

                if (GetType(Buffer) == TEXTMODE_BUFFER)
                {
#define IS_WORD_SEP(c)  \
    ((c) == L'\0' || (c) == L' ' || (c) == L'\t' || (c) == L'\r' || (c) == L'\n')

                    PTEXTMODE_SCREEN_BUFFER TextBuffer = (PTEXTMODE_SCREEN_BUFFER)Buffer;
                    COORD cL, cR;
                    PCHAR_INFO ptrL, ptrR;

                    /* Starting point */
                    cL = cR = PointToCoord(GuiData, lParam);
                    ptrL = ptrR = ConioCoordToPointer(TextBuffer, cL.X, cL.Y);

                    /* Enlarge the selection by checking for whitespace */
                    while ((0 < cL.X) && !IS_WORD_SEP(ptrL->Char.UnicodeChar)
                                      && !IS_WORD_SEP((ptrL-1)->Char.UnicodeChar))
                    {
                        --cL.X;
                        --ptrL;
                    }
                    while ((cR.X < TextBuffer->ScreenBufferSize.X - 1) &&
                           !IS_WORD_SEP(ptrR->Char.UnicodeChar)        &&
                           !IS_WORD_SEP((ptrR+1)->Char.UnicodeChar))
                    {
                        ++cR.X;
                        ++ptrR;
                    }

                    /*
                     * Update the selection started with the single
                     * left-click that preceded this double-click.
                     */
                    GuiData->Selection.dwFlags |= CONSOLE_MOUSE_SELECTION | CONSOLE_MOUSE_DOWN;
                    UpdateSelection(GuiData, &cL, &cR);

                    /* Ignore the next mouse move signal */
                    GuiData->IgnoreNextMouseSignal = TRUE;
                }

                break;
            }

            case WM_RBUTTONDOWN:
            case WM_RBUTTONDBLCLK:
            {
                if (!(GuiData->Selection.dwFlags & CONSOLE_SELECTION_NOT_EMPTY))
                {
                    Paste(GuiData);
                }
                else
                {
                    Copy(GuiData);
                }

                /* Ignore the next mouse move signal */
                GuiData->IgnoreNextMouseSignal = TRUE;
                break;
            }

            case WM_MOUSEMOVE:
            {
                if (!(wParam & MK_LBUTTON)) break;
                if (!(GuiData->Selection.dwFlags & CONSOLE_MOUSE_DOWN)) break;

                // TODO: Scroll buffer to bring SelectionCursor into view
                GuiData->dwSelectionCursor = PointToCoord(GuiData, lParam);
                UpdateSelection(GuiData, NULL, &GuiData->dwSelectionCursor);

                break;
            }

            default:
                Err = FALSE; // TRUE;
                break;
        }
    }
    else if (Console->InputBuffer.Mode & ENABLE_MOUSE_INPUT)
    {
        INPUT_RECORD er;
        WORD  wKeyState         = GET_KEYSTATE_WPARAM(wParam);
        DWORD dwButtonState     = 0;
        DWORD dwControlKeyState = 0;
        DWORD dwEventFlags      = 0;

        switch (msg)
        {
            case WM_LBUTTONDOWN:
                SetCapture(GuiData->hWindow);
                dwButtonState = FROM_LEFT_1ST_BUTTON_PRESSED;
                dwEventFlags  = 0;
                break;

            case WM_MBUTTONDOWN:
                SetCapture(GuiData->hWindow);
                dwButtonState = FROM_LEFT_2ND_BUTTON_PRESSED;
                dwEventFlags  = 0;
                break;

            case WM_RBUTTONDOWN:
                SetCapture(GuiData->hWindow);
                dwButtonState = RIGHTMOST_BUTTON_PRESSED;
                dwEventFlags  = 0;
                break;

            case WM_LBUTTONUP:
                ReleaseCapture();
                dwButtonState = 0;
                dwEventFlags  = 0;
                break;

            case WM_MBUTTONUP:
                ReleaseCapture();
                dwButtonState = 0;
                dwEventFlags  = 0;
                break;

            case WM_RBUTTONUP:
                ReleaseCapture();
                dwButtonState = 0;
                dwEventFlags  = 0;
                break;

            case WM_LBUTTONDBLCLK:
                dwButtonState = FROM_LEFT_1ST_BUTTON_PRESSED;
                dwEventFlags  = DOUBLE_CLICK;
                break;

            case WM_MBUTTONDBLCLK:
                dwButtonState = FROM_LEFT_2ND_BUTTON_PRESSED;
                dwEventFlags  = DOUBLE_CLICK;
                break;

            case WM_RBUTTONDBLCLK:
                dwButtonState = RIGHTMOST_BUTTON_PRESSED;
                dwEventFlags  = DOUBLE_CLICK;
                break;

            case WM_MOUSEMOVE:
                dwButtonState = 0;
                dwEventFlags  = MOUSE_MOVED;
                break;

            case WM_MOUSEWHEEL:
                dwButtonState = GET_WHEEL_DELTA_WPARAM(wParam) << 16;
                dwEventFlags  = MOUSE_WHEELED;
                break;

            case WM_MOUSEHWHEEL:
                dwButtonState = GET_WHEEL_DELTA_WPARAM(wParam) << 16;
                dwEventFlags  = MOUSE_HWHEELED;
                break;

            default:
                Err = TRUE;
                break;
        }

        /*
         * HACK FOR CORE-8394: Ignore the next mouse move signal
         * just after mouse down click actions.
         */
        switch (msg)
        {
            case WM_LBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_RBUTTONDOWN:
                GuiData->IgnoreNextMouseSignal = TRUE;
            default:
                break;
        }

        if (!Err)
        {
            if (wKeyState & MK_LBUTTON)
                dwButtonState |= FROM_LEFT_1ST_BUTTON_PRESSED;
            if (wKeyState & MK_MBUTTON)
                dwButtonState |= FROM_LEFT_2ND_BUTTON_PRESSED;
            if (wKeyState & MK_RBUTTON)
                dwButtonState |= RIGHTMOST_BUTTON_PRESSED;

            if (GetKeyState(VK_RMENU) & 0x8000)
                dwControlKeyState |= RIGHT_ALT_PRESSED;
            if (GetKeyState(VK_LMENU) & 0x8000)
                dwControlKeyState |= LEFT_ALT_PRESSED;
            if (GetKeyState(VK_RCONTROL) & 0x8000)
                dwControlKeyState |= RIGHT_CTRL_PRESSED;
            if (GetKeyState(VK_LCONTROL) & 0x8000)
                dwControlKeyState |= LEFT_CTRL_PRESSED;
            if (GetKeyState(VK_SHIFT) & 0x8000)
                dwControlKeyState |= SHIFT_PRESSED;
            if (GetKeyState(VK_NUMLOCK) & 0x0001)
                dwControlKeyState |= NUMLOCK_ON;
            if (GetKeyState(VK_SCROLL) & 0x0001)
                dwControlKeyState |= SCROLLLOCK_ON;
            if (GetKeyState(VK_CAPITAL) & 0x0001)
                dwControlKeyState |= CAPSLOCK_ON;
            /* See WM_CHAR MSDN documentation for instance */
            if (lParam & 0x01000000)
                dwControlKeyState |= ENHANCED_KEY;

            er.EventType = MOUSE_EVENT;
            er.Event.MouseEvent.dwMousePosition   = PointToCoord(GuiData, lParam);
            er.Event.MouseEvent.dwButtonState     = dwButtonState;
            er.Event.MouseEvent.dwControlKeyState = dwControlKeyState;
            er.Event.MouseEvent.dwEventFlags      = dwEventFlags;

            ConioProcessInputEvent(Console, &er);
        }
    }
    else
    {
        Err = TRUE;
    }

    LeaveCriticalSection(&Console->Lock);

Quit:
    if (Err)
        return DefWindowProcW(GuiData->hWindow, msg, wParam, lParam);
    else
        return 0;
}

VOID
GuiCopyFromTextModeBuffer(PTEXTMODE_SCREEN_BUFFER Buffer,
                          PGUI_CONSOLE_DATA GuiData);
VOID
GuiCopyFromGraphicsBuffer(PGRAPHICS_SCREEN_BUFFER Buffer,
                          PGUI_CONSOLE_DATA GuiData);

static VOID
Copy(PGUI_CONSOLE_DATA GuiData)
{
    if (OpenClipboard(GuiData->hWindow))
    {
        PCONSOLE_SCREEN_BUFFER Buffer = GuiData->ActiveBuffer;

        if (GetType(Buffer) == TEXTMODE_BUFFER)
        {
            GuiCopyFromTextModeBuffer((PTEXTMODE_SCREEN_BUFFER)Buffer, GuiData);
        }
        else /* if (GetType(Buffer) == GRAPHICS_BUFFER) */
        {
            GuiCopyFromGraphicsBuffer((PGRAPHICS_SCREEN_BUFFER)Buffer, GuiData);
        }

        CloseClipboard();
    }

    /* Clear the selection */
    UpdateSelection(GuiData, NULL, NULL);
}

VOID
GuiPasteToTextModeBuffer(PTEXTMODE_SCREEN_BUFFER Buffer,
                         PGUI_CONSOLE_DATA GuiData);
VOID
GuiPasteToGraphicsBuffer(PGRAPHICS_SCREEN_BUFFER Buffer,
                         PGUI_CONSOLE_DATA GuiData);

static VOID
Paste(PGUI_CONSOLE_DATA GuiData)
{
    if (OpenClipboard(GuiData->hWindow))
    {
        PCONSOLE_SCREEN_BUFFER Buffer = GuiData->ActiveBuffer;

        if (GetType(Buffer) == TEXTMODE_BUFFER)
        {
            GuiPasteToTextModeBuffer((PTEXTMODE_SCREEN_BUFFER)Buffer, GuiData);
        }
        else /* if (GetType(Buffer) == GRAPHICS_BUFFER) */
        {
            GuiPasteToGraphicsBuffer((PGRAPHICS_SCREEN_BUFFER)Buffer, GuiData);
        }

        CloseClipboard();
    }
}

static VOID
OnGetMinMaxInfo(PGUI_CONSOLE_DATA GuiData, PMINMAXINFO minMaxInfo)
{
    PCONSRV_CONSOLE Console = GuiData->Console;
    PCONSOLE_SCREEN_BUFFER ActiveBuffer;
    DWORD windx, windy;
    UINT  WidthUnit, HeightUnit;

    if (!ConDrvValidateConsoleUnsafe((PCONSOLE)Console, CONSOLE_RUNNING, TRUE)) return;

    ActiveBuffer = GuiData->ActiveBuffer;

    GetScreenBufferSizeUnits(ActiveBuffer, GuiData, &WidthUnit, &HeightUnit);

    windx = CONGUI_MIN_WIDTH  * WidthUnit  + 2 * (GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXEDGE));
    windy = CONGUI_MIN_HEIGHT * HeightUnit + 2 * (GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CYEDGE)) + GetSystemMetrics(SM_CYCAPTION);

    minMaxInfo->ptMinTrackSize.x = windx;
    minMaxInfo->ptMinTrackSize.y = windy;

    windx = (ActiveBuffer->ScreenBufferSize.X) * WidthUnit  + 2 * (GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXEDGE));
    windy = (ActiveBuffer->ScreenBufferSize.Y) * HeightUnit + 2 * (GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CYEDGE)) + GetSystemMetrics(SM_CYCAPTION);

    if (ActiveBuffer->ViewSize.X < ActiveBuffer->ScreenBufferSize.X) windy += GetSystemMetrics(SM_CYHSCROLL);    // window currently has a horizontal scrollbar
    if (ActiveBuffer->ViewSize.Y < ActiveBuffer->ScreenBufferSize.Y) windx += GetSystemMetrics(SM_CXVSCROLL);    // window currently has a vertical scrollbar

    minMaxInfo->ptMaxTrackSize.x = windx;
    minMaxInfo->ptMaxTrackSize.y = windy;

    LeaveCriticalSection(&Console->Lock);
}

static VOID
OnSize(PGUI_CONSOLE_DATA GuiData, WPARAM wParam, LPARAM lParam)
{
    PCONSRV_CONSOLE Console = GuiData->Console;

    /* Do nothing if the window is hidden */
    if (!GuiData->IsWindowVisible) return;

    if (!ConDrvValidateConsoleUnsafe((PCONSOLE)Console, CONSOLE_RUNNING, TRUE)) return;

    if ((GuiData->WindowSizeLock == FALSE) &&
        (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED || wParam == SIZE_MINIMIZED))
    {
        PCONSOLE_SCREEN_BUFFER Buff = GuiData->ActiveBuffer;
        DWORD windx, windy, charx, chary;
        UINT  WidthUnit, HeightUnit;

        GetScreenBufferSizeUnits(Buff, GuiData, &WidthUnit, &HeightUnit);

        GuiData->WindowSizeLock = TRUE;

        windx = LOWORD(lParam);
        windy = HIWORD(lParam);

        // Compensate for existing scroll bars (because lParam values do not accommodate scroll bar)
        if (Buff->ViewSize.X < Buff->ScreenBufferSize.X) windy += GetSystemMetrics(SM_CYHSCROLL);    // window currently has a horizontal scrollbar
        if (Buff->ViewSize.Y < Buff->ScreenBufferSize.Y) windx += GetSystemMetrics(SM_CXVSCROLL);    // window currently has a vertical scrollbar

        charx = windx / (int)WidthUnit ;
        chary = windy / (int)HeightUnit;

        // Character alignment (round size up or down)
        if ((windx % WidthUnit ) >= (WidthUnit  / 2)) ++charx;
        if ((windy % HeightUnit) >= (HeightUnit / 2)) ++chary;

        // Compensate for added scroll bars in new window
        if (charx < Buff->ScreenBufferSize.X) windy -= GetSystemMetrics(SM_CYHSCROLL);    // new window will have a horizontal scroll bar
        if (chary < Buff->ScreenBufferSize.Y) windx -= GetSystemMetrics(SM_CXVSCROLL);    // new window will have a vertical scroll bar

        charx = windx / (int)WidthUnit ;
        chary = windy / (int)HeightUnit;

        // Character alignment (round size up or down)
        if ((windx % WidthUnit ) >= (WidthUnit  / 2)) ++charx;
        if ((windy % HeightUnit) >= (HeightUnit / 2)) ++chary;

        // Resize window
        if ((charx != Buff->ViewSize.X) || (chary != Buff->ViewSize.Y))
        {
            Buff->ViewSize.X = (charx <= Buff->ScreenBufferSize.X) ? charx : Buff->ScreenBufferSize.X;
            Buff->ViewSize.Y = (chary <= Buff->ScreenBufferSize.Y) ? chary : Buff->ScreenBufferSize.Y;
        }

        ResizeConWnd(GuiData, WidthUnit, HeightUnit);

        // Adjust the start of the visible area if we are attempting to show nonexistent areas
        if ((Buff->ScreenBufferSize.X - Buff->ViewOrigin.X) < Buff->ViewSize.X) Buff->ViewOrigin.X = Buff->ScreenBufferSize.X - Buff->ViewSize.X;
        if ((Buff->ScreenBufferSize.Y - Buff->ViewOrigin.Y) < Buff->ViewSize.Y) Buff->ViewOrigin.Y = Buff->ScreenBufferSize.Y - Buff->ViewSize.Y;
        InvalidateRect(GuiData->hWindow, NULL, TRUE);

        GuiData->WindowSizeLock = FALSE;
    }

    LeaveCriticalSection(&Console->Lock);
}

static VOID
OnMove(PGUI_CONSOLE_DATA GuiData)
{
    RECT rcWnd;

    // TODO: Simplify the code.
    // See: GuiConsoleNotifyWndProc() PM_CREATE_CONSOLE.

    /* Retrieve our real position */
    GetWindowRect(GuiData->hWindow, &rcWnd);
    GuiData->GuiInfo.WindowOrigin.x = rcWnd.left;
    GuiData->GuiInfo.WindowOrigin.y = rcWnd.top;
}

/*
// HACK: This functionality is standard for general scrollbars. Don't add it by hand.

VOID
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
OnScroll(PGUI_CONSOLE_DATA GuiData, UINT uMsg, WPARAM wParam)
{
    PCONSRV_CONSOLE Console = GuiData->Console;
    PCONSOLE_SCREEN_BUFFER Buff;
    SCROLLINFO sInfo;
    int fnBar;
    int old_pos, Maximum;
    PSHORT pShowXY;

    if (!ConDrvValidateConsoleUnsafe((PCONSOLE)Console, CONSOLE_RUNNING, TRUE)) return 0;

    Buff = GuiData->ActiveBuffer;

    if (uMsg == WM_HSCROLL)
    {
        fnBar = SB_HORZ;
        Maximum = Buff->ScreenBufferSize.X - Buff->ViewSize.X;
        pShowXY = &Buff->ViewOrigin.X;
    }
    else
    {
        fnBar = SB_VERT;
        Maximum = Buff->ScreenBufferSize.Y - Buff->ViewSize.Y;
        pShowXY = &Buff->ViewOrigin.Y;
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
        USHORT OldX = Buff->ViewOrigin.X;
        USHORT OldY = Buff->ViewOrigin.Y;
        UINT   WidthUnit, HeightUnit;

        *pShowXY = sInfo.nPos;

        GetScreenBufferSizeUnits(Buff, GuiData, &WidthUnit, &HeightUnit);

        ScrollWindowEx(GuiData->hWindow,
                       (OldX - Buff->ViewOrigin.X) * WidthUnit ,
                       (OldY - Buff->ViewOrigin.Y) * HeightUnit,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       SW_INVALIDATE);

        sInfo.fMask = SIF_POS;
        SetScrollInfo(GuiData->hWindow, fnBar, &sInfo, TRUE);

        UpdateWindow(GuiData->hWindow);
        // InvalidateRect(GuiData->hWindow, NULL, FALSE);
    }

Quit:
    LeaveCriticalSection(&Console->Lock);
    return 0;
}


static LRESULT CALLBACK
ConWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT Result = 0;
    PGUI_CONSOLE_DATA GuiData = NULL;
    PCONSRV_CONSOLE Console = NULL;

    /*
     * - If it's the first time we create a window for the terminal,
     *   just initialize it and return.
     *
     * - If we are destroying the window, just do it and return.
     */
    if (msg == WM_NCCREATE)
    {
        return (LRESULT)OnNcCreate(hWnd, (LPCREATESTRUCTW)lParam);
    }
    else if (msg == WM_NCDESTROY)
    {
        return OnNcDestroy(hWnd);
    }

    /*
     * Now the terminal window is initialized.
     * Get the terminal data via the window's data.
     * If there is no data, just go away.
     */
    GuiData = GuiGetGuiData(hWnd);
    if (GuiData == NULL) return DefWindowProcW(hWnd, msg, wParam, lParam);

    // TEMPORARY HACK until all of the functions can deal with a NULL GuiData->ActiveBuffer ...
    if (GuiData->ActiveBuffer == NULL) return DefWindowProcW(hWnd, msg, wParam, lParam);

    /*
     * Just retrieve a pointer to the console in case somebody needs it.
     * It is not NULL because it was checked in GuiGetGuiData.
     * Each helper function which needs the console has to validate and lock it.
     */
    Console = GuiData->Console;

    /* We have a console, start message dispatching */
    switch (msg)
    {
        case WM_ACTIVATE:
            OnActivate(GuiData, wParam);
            break;

        case WM_CLOSE:
            if (OnClose(GuiData)) goto Default;
            break;

        case WM_PAINT:
            OnPaint(GuiData);
            break;

        case WM_TIMER:
            OnTimer(GuiData);
            break;

        case WM_PALETTECHANGED:
        {
            DPRINT("WM_PALETTECHANGED called\n");

            /*
             * Protects against infinite loops:
             * "... A window that receives this message must not realize
             * its palette, unless it determines that wParam does not contain
             * its own window handle." (WM_PALETTECHANGED description - MSDN)
             *
             * This message is sent to all windows, including the one that
             * changed the system palette and caused this message to be sent.
             * The wParam of this message contains the handle of the window
             * that caused the system palette to change. To avoid an infinite
             * loop, care must be taken to check that the wParam of this message
             * does not match the window's handle.
             */
            if ((HWND)wParam == hWnd) break;

            DPRINT("WM_PALETTECHANGED ok\n");
            OnPaletteChanged(GuiData);
            DPRINT("WM_PALETTECHANGED quit\n");
            break;
        }

        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_CHAR:
        case WM_DEADCHAR:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_SYSCHAR:
        case WM_SYSDEADCHAR:
        {
            /* Detect Alt-Enter presses and switch back and forth to fullscreen mode */
            if (msg == WM_SYSKEYDOWN && (HIWORD(lParam) & KF_ALTDOWN) && wParam == VK_RETURN)
            {
                /* Switch only at first Alt-Enter press, and ignore subsequent key repetitions */
                if ((HIWORD(lParam) & (KF_UP | KF_REPEAT)) != KF_REPEAT)
                    GuiConsoleSwitchFullScreen(GuiData);

                break;
            }
            /* Detect Alt-Esc/Space/Tab presses defer to DefWindowProc */
            if ( (HIWORD(lParam) & KF_ALTDOWN) && (wParam == VK_ESCAPE || wParam == VK_SPACE || wParam == VK_TAB))
            {
                return DefWindowProcW(hWnd, msg, wParam, lParam);
            }

            OnKey(GuiData, msg, wParam, lParam);
            break;
        }

        case WM_SETCURSOR:
        {
            /* Do nothing if the window is hidden */
            if (!GuiData->IsWindowVisible) goto Default;

            /*
             * The message was sent because we are manually triggering a change.
             * Check whether the mouse is indeed present on this console window
             * and take appropriate decisions.
             */
            if (wParam == -1 && lParam == -1)
            {
                POINT mouseCoords;
                HWND  hWndHit;

                /* Get the placement of the mouse */
                GetCursorPos(&mouseCoords);

                /* On which window is placed the mouse ? */
                hWndHit = WindowFromPoint(mouseCoords);

                /* It's our window. Perform the hit-test to be used later on. */
                if (hWndHit == hWnd)
                {
                    wParam = (WPARAM)hWnd;
                    lParam = DefWindowProcW(hWndHit, WM_NCHITTEST, 0,
                                            MAKELPARAM(mouseCoords.x, mouseCoords.y));
                }
            }

            /* Set the mouse cursor only when we are in the client area */
            if ((HWND)wParam == hWnd && LOWORD(lParam) == HTCLIENT)
            {
                if (GuiData->MouseCursorRefCount >= 0)
                {
                    /* Show the cursor */
                    SetCursor(GuiData->hCursor);
                }
                else
                {
                    /* Hide the cursor if the reference count is negative */
                    SetCursor(NULL);
                }
                return TRUE;
            }
            else
            {
                goto Default;
            }
        }

        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
        case WM_MOUSEMOVE:
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
        {
            Result = OnMouse(GuiData, msg, wParam, lParam);
            break;
        }

        case WM_HSCROLL:
        case WM_VSCROLL:
        {
            Result = OnScroll(GuiData, msg, wParam);
            break;
        }

        case WM_CONTEXTMENU:
        {
            /* Do nothing if the window is hidden */
            if (!GuiData->IsWindowVisible) break;

            if (DefWindowProcW(hWnd /*GuiData->hWindow*/, WM_NCHITTEST, 0, lParam) == HTCLIENT)
            {
                HMENU hMenu = CreatePopupMenu();
                if (hMenu != NULL)
                {
                    AppendMenuItems(hMenu, GuiConsoleEditMenuItems);
                    TrackPopupMenuEx(hMenu,
                                     TPM_RIGHTBUTTON,
                                     GET_X_LPARAM(lParam),
                                     GET_Y_LPARAM(lParam),
                                     hWnd,
                                     NULL);
                    DestroyMenu(hMenu);
                }
                break;
            }
            else
            {
                goto Default;
            }
        }

        case WM_INITMENU:
        {
            HMENU hMenu = (HMENU)wParam;
            if (hMenu != NULL)
            {
                /* Enable or disable the Close menu item */
                EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND |
                               (GuiData->IsCloseButtonEnabled ? MF_ENABLED : MF_GRAYED));

                /* Enable or disable the Copy and Paste items */
                EnableMenuItem(hMenu, ID_SYSTEM_EDIT_COPY , MF_BYCOMMAND |
                               ((GuiData->Selection.dwFlags & CONSOLE_SELECTION_IN_PROGRESS) &&
                                (GuiData->Selection.dwFlags & CONSOLE_SELECTION_NOT_EMPTY) ? MF_ENABLED : MF_GRAYED));
                // FIXME: Following whether the active screen buffer is text-mode
                // or graphics-mode, search for CF_UNICODETEXT or CF_BITMAP formats.
                EnableMenuItem(hMenu, ID_SYSTEM_EDIT_PASTE, MF_BYCOMMAND |
                               (!(GuiData->Selection.dwFlags & CONSOLE_SELECTION_IN_PROGRESS) &&
                                IsClipboardFormatAvailable(CF_UNICODETEXT) ? MF_ENABLED : MF_GRAYED));
            }

            SendMenuEvent(Console, WM_INITMENU);
            break;
        }

        case WM_MENUSELECT:
        {
            if (HIWORD(wParam) == 0xFFFF) // Allow all the menu flags
            {
                SendMenuEvent(Console, WM_MENUSELECT);
            }
            break;
        }

        case WM_COMMAND:
        case WM_SYSCOMMAND:
        {
            Result = OnCommand(GuiData, wParam, lParam);
            break;
        }

        case WM_SETFOCUS:
        case WM_KILLFOCUS:
            OnFocus(GuiData, (msg == WM_SETFOCUS));
            break;

        case WM_GETMINMAXINFO:
            OnGetMinMaxInfo(GuiData, (PMINMAXINFO)lParam);
            break;

        case WM_MOVE:
            OnMove(GuiData);
            break;

#if 0 // This code is here to prepare & control dynamic console SB resizing.
        case WM_SIZING:
        {
            PRECT dragRect = (PRECT)lParam;
            switch (wParam)
            {
                case WMSZ_LEFT:
                    DPRINT1("WMSZ_LEFT\n");
                    break;
                case WMSZ_RIGHT:
                    DPRINT1("WMSZ_RIGHT\n");
                    break;
                case WMSZ_TOP:
                    DPRINT1("WMSZ_TOP\n");
                    break;
                case WMSZ_TOPLEFT:
                    DPRINT1("WMSZ_TOPLEFT\n");
                    break;
                case WMSZ_TOPRIGHT:
                    DPRINT1("WMSZ_TOPRIGHT\n");
                    break;
                case WMSZ_BOTTOM:
                    DPRINT1("WMSZ_BOTTOM\n");
                    break;
                case WMSZ_BOTTOMLEFT:
                    DPRINT1("WMSZ_BOTTOMLEFT\n");
                    break;
                case WMSZ_BOTTOMRIGHT:
                    DPRINT1("WMSZ_BOTTOMRIGHT\n");
                    break;
                default:
                    DPRINT1("wParam = %d\n", wParam);
                    break;
            }
            DPRINT1("dragRect = {.left = %d ; .top = %d ; .right = %d ; .bottom = %d}\n",
                    dragRect->left, dragRect->top, dragRect->right, dragRect->bottom);
            break;
        }
#endif

        case WM_SIZE:
            OnSize(GuiData, wParam, lParam);
            break;

        case PM_RESIZE_TERMINAL:
        {
            PCONSOLE_SCREEN_BUFFER Buff = GuiData->ActiveBuffer;
            HDC hDC;
            HBITMAP hnew, hold;

            DWORD Width, Height;
            UINT  WidthUnit, HeightUnit;

            /* Do nothing if the window is hidden */
            if (!GuiData->IsWindowVisible) break;

            GetScreenBufferSizeUnits(Buff, GuiData, &WidthUnit, &HeightUnit);

            Width  = Buff->ScreenBufferSize.X * WidthUnit ;
            Height = Buff->ScreenBufferSize.Y * HeightUnit;

            /* Recreate the framebuffer */
            hDC  = GetDC(GuiData->hWindow);
            hnew = CreateCompatibleBitmap(hDC, Width, Height);
            ReleaseDC(GuiData->hWindow, hDC);
            hold = SelectObject(GuiData->hMemDC, hnew);
            if (GuiData->hBitmap)
            {
                if (hold == GuiData->hBitmap) DeleteObject(GuiData->hBitmap);
            }
            GuiData->hBitmap = hnew;

            /* Resize the window to the user's values */
            GuiData->WindowSizeLock = TRUE;
            ResizeConWnd(GuiData, WidthUnit, HeightUnit);
            GuiData->WindowSizeLock = FALSE;
            break;
        }

        case PM_APPLY_CONSOLE_INFO:
        {
            GuiApplyUserSettings(GuiData, (HANDLE)wParam, (BOOL)lParam);
            break;
        }

        /*
         * Undocumented message sent by Windows' console.dll for applying console info.
         * See http://www.catch22.net/sites/default/source/files/setconsoleinfo.c
         * and http://www.scn.rain.com/~neighorn/PDF/MSBugPaper.pdf
         * for more information.
         */
        case WM_SETCONSOLEINFO:
        {
            DPRINT1("WM_SETCONSOLEINFO message\n");
            GuiApplyWindowsConsoleSettings(GuiData, (HANDLE)wParam);
            break;
        }

        case PM_CONSOLE_BEEP:
            DPRINT1("Beep\n");
            Beep(800, 200);
            break;

        // case PM_CONSOLE_SET_TITLE:
            // SetWindowTextW(GuiData->hWindow, GuiData->Console->Title.Buffer);
            // break;

        default: Default:
            Result = DefWindowProcW(hWnd, msg, wParam, lParam);
            break;
    }

    return Result;
}

/* EOF */
