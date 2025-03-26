/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/frontends/gui/conwnd.c
 * PURPOSE:         GUI Console Window Class
 * PROGRAMMERS:     Gé van Geldorp
 *                  Johannes Anderwald
 *                  Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 *                  Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

/* INCLUDES *******************************************************************/

#include <consrv.h>
#include <intrin.h>
#include <windowsx.h>
#include <shellapi.h>

#define NDEBUG
#include <debug.h>

#include "concfg/font.h"
#include "guiterm.h"
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

    ProcessData = ConSrvGetConsoleLeaderProcess(GuiData->Console);

    ASSERT(ProcessData != NULL);
    DPRINT("ProcessData: %p, ProcessData->Process %p.\n", ProcessData, ProcessData->Process);

    if (ProcessData->Process)
    {
        CLIENT_ID ConsoleLeaderCID = ProcessData->Process->ClientId;
        SetWindowLongPtrW(GuiData->hWindow, GWLP_CONSOLE_LEADER_PID,
                          (LONG_PTR)(ConsoleLeaderCID.UniqueProcess));
        SetWindowLongPtrW(GuiData->hWindow, GWLP_CONSOLE_LEADER_TID,
                          (LONG_PTR)(ConsoleLeaderCID.UniqueThread));
    }
    else
    {
        SetWindowLongPtrW(GuiData->hWindow, GWLP_CONSOLE_LEADER_PID, 0);
        SetWindowLongPtrW(GuiData->hWindow, GWLP_CONSOLE_LEADER_TID, 0);
    }
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
    WndClass.hbrBackground = NULL;
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
                            ARRAYSIZE(szMenuString)) > 0)
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
    HMENU hMenu;
    PWCHAR ptrTab;
    WCHAR szMenuStringBack[255];

    hMenu = GetSystemMenu(hWnd, FALSE);
    if (hMenu == NULL)
        return;

    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_STRING;
    mii.dwTypeData = szMenuStringBack;
    mii.cch = ARRAYSIZE(szMenuStringBack);

    GetMenuItemInfoW(hMenu, SC_CLOSE, FALSE, &mii);

    ptrTab = wcschr(szMenuStringBack, L'\t');
    if (ptrTab)
    {
        *ptrTab = L'\0';
        mii.cch = (UINT)wcslen(szMenuStringBack);

        SetMenuItemInfoW(hMenu, SC_CLOSE, FALSE, &mii);
    }

    AppendMenuItems(hMenu, GuiConsoleMainMenuItems);
    DrawMenuBar(hWnd);
}

static VOID
SendMenuEvent(PCONSRV_CONSOLE Console, UINT CmdId)
{
    INPUT_RECORD er;

    DPRINT("Menu item ID: %d\n", CmdId);

    if (!ConDrvValidateConsoleUnsafe((PCONSOLE)Console, CONSOLE_RUNNING, TRUE)) return;

    /* Send a menu event */
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
    sInfo.cbSize = sizeof(sInfo);
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

    /* Resize the window */
    SetWindowPos(GuiData->hWindow, NULL, 0, 0, Width, Height,
                 SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOCOPYBITS);
    // NOTE: The SWP_NOCOPYBITS flag can be replaced by a subsequent call
    // to: InvalidateRect(GuiData->hWindow, NULL, TRUE);
}


VOID
DeleteFonts(PGUI_CONSOLE_DATA GuiData)
{
    ULONG i;
    for (i = 0; i < ARRAYSIZE(GuiData->Font); ++i)
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
    LOGFONTW lf;

    /* Initialize the LOGFONT structure */
    RtlZeroMemory(&lf, sizeof(lf));

    /* Retrieve the details of the current font */
    if (GetObjectW(OrgFont, sizeof(lf), &lf) == 0)
        return NULL;

    /* Change the font attributes */
    // lf.lfHeight = FontSize.Y;
    // lf.lfWidth  = FontSize.X;
    lf.lfWeight = FontWeight;
    // lf.lfItalic = bItalic;
    lf.lfUnderline = bUnderline;
    lf.lfStrikeOut = bStrikeOut;

    /* Build a new font */
    return CreateFontIndirectW(&lf);
}

BOOL
InitFonts(
    _Inout_ PGUI_CONSOLE_DATA GuiData,
    _In_reads_or_z_(LF_FACESIZE)
         PCWSTR FaceName,
    _In_ ULONG FontWeight,
    _In_ ULONG FontFamily,
    _In_ COORD FontSize,
    _In_opt_ UINT CodePage,
    _In_ BOOL UseDefaultFallback)
{
    HDC hDC;
    HFONT hFont;
    FONT_DATA FontData;
    UINT OldCharWidth  = GuiData->CharWidth;
    UINT OldCharHeight = GuiData->CharHeight;
    COORD OldFontSize  = GuiData->GuiInfo.FontSize;
    WCHAR NewFaceName[LF_FACESIZE];

    /* Default to current code page if none has been provided */
    if (!CodePage)
        CodePage = GuiData->Console->OutputCodePage;

    /*
     * Initialize a new NORMAL font.
     */

    /* Copy the requested face name into the local buffer.
     * It will be modified in output by CreateConsoleFontEx()
     * to hold a possible fallback font face name. */
    StringCchCopyNW(NewFaceName, ARRAYSIZE(NewFaceName),
                    FaceName, LF_FACESIZE);

    /* NOTE: FontSize is always in cell height/width units (pixels) */
    hFont = CreateConsoleFontEx((LONG)(ULONG)FontSize.Y,
                                (LONG)(ULONG)FontSize.X,
                                NewFaceName,
                                FontWeight,
                                FontFamily,
                                CodePage,
                                UseDefaultFallback,
                                &FontData);
    if (!hFont)
    {
        DPRINT1("InitFonts: CreateConsoleFontEx('%S') failed\n", NewFaceName);
        return FALSE;
    }

    /* Retrieve its character cell size */
    hDC = GetDC(GuiData->hWindow);
    if (!GetFontCellSize(hDC, hFont, &GuiData->CharHeight, &GuiData->CharWidth))
    {
        DPRINT1("InitFonts: GetFontCellSize failed\n");
        ReleaseDC(GuiData->hWindow, hDC);
        DeleteObject(hFont);
        return FALSE;
    }
    ReleaseDC(GuiData->hWindow, hDC);

    /*
     * Initialization succeeded.
     */
    // Delete all the old fonts first.
    DeleteFonts(GuiData);
    GuiData->Font[FONT_NORMAL] = hFont;

    /*
     * Now build the optional fonts (bold, underlined, mixed).
     * Do not error in case they fail to be created.
     */
    GuiData->Font[FONT_BOLD] =
        CreateDerivedFont(GuiData->Font[FONT_NORMAL],
                          max(FW_BOLD, FontData.Weight),
                          FALSE,
                          FALSE);
    GuiData->Font[FONT_UNDERLINE] =
        CreateDerivedFont(GuiData->Font[FONT_NORMAL],
                          FontData.Weight,
                          TRUE,
                          FALSE);
    GuiData->Font[FONT_BOLD | FONT_UNDERLINE] =
        CreateDerivedFont(GuiData->Font[FONT_NORMAL],
                          max(FW_BOLD, FontData.Weight),
                          TRUE,
                          FALSE);

    /*
     * Save the new font characteristics.
     */
    StringCchCopyNW(GuiData->GuiInfo.FaceName,
                    ARRAYSIZE(GuiData->GuiInfo.FaceName),
                    NewFaceName, ARRAYSIZE(NewFaceName));
    GuiData->GuiInfo.FontWeight = FontData.Weight;
    GuiData->GuiInfo.FontFamily = FontData.Family;
    GuiData->GuiInfo.FontSize   = FontData.Size;

    /* Resize the terminal, in case the new font has a different size */
    if ((OldCharWidth  != GuiData->CharWidth)  ||
        (OldCharHeight != GuiData->CharHeight) ||
        (OldFontSize.X != FontData.Size.X ||
         OldFontSize.Y != FontData.Size.Y))
    {
        TermResizeTerminal(GuiData->Console);
    }

    return TRUE;
}


static BOOL
OnNcCreate(HWND hWnd, LPCREATESTRUCTW Create)
{
    PGUI_CONSOLE_DATA GuiData = (PGUI_CONSOLE_DATA)Create->lpCreateParams;
    PCONSRV_CONSOLE Console;

    if (GuiData == NULL)
    {
        DPRINT1("GuiConsoleNcCreate: No GUI data\n");
        return FALSE;
    }

    Console = GuiData->Console;

    GuiData->hWindow = hWnd;
    GuiData->hSysMenu = GetSystemMenu(hWnd, FALSE);
    GuiData->IsWindowActive = FALSE;

    /* Initialize the fonts */
    if (!InitFonts(GuiData,
                   GuiData->GuiInfo.FaceName,
                   GuiData->GuiInfo.FontWeight,
                   GuiData->GuiInfo.FontFamily,
                   GuiData->GuiInfo.FontSize,
                   0, FALSE))
    {
        /* Reset only the output code page if we don't have a suitable
         * font for it, possibly falling back to "United States (OEM)". */
        UINT AltCodePage = GetOEMCP();

        if (AltCodePage == Console->OutputCodePage)
            AltCodePage = CP_USA;

        DPRINT1("Could not initialize font '%S' for code page %d - Resetting CP to %d\n",
                GuiData->GuiInfo.FaceName, Console->OutputCodePage, AltCodePage);

        CON_SET_OUTPUT_CP(Console, AltCodePage);

        /* We will use a fallback font if we cannot find
         * anything for this replacement code page. */
        if (!InitFonts(GuiData,
                       GuiData->GuiInfo.FaceName,
                       GuiData->GuiInfo.FontWeight,
                       GuiData->GuiInfo.FontFamily,
                       GuiData->GuiInfo.FontSize,
                       0, TRUE))
        {
            DPRINT1("Failed to initialize font '%S' for code page %d\n",
                    GuiData->GuiInfo.FaceName, Console->OutputCodePage);

            DPRINT1("GuiConsoleNcCreate: InitFonts failed\n");
            GuiData->hWindow = NULL;
            NtSetEvent(GuiData->hGuiInitEvent, NULL);
            return FALSE;
        }
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

    /* We accept dropped files */
    DragAcceptFiles(GuiData->hWindow, TRUE);

    return (BOOL)DefWindowProcW(GuiData->hWindow, WM_NCCREATE, 0, (LPARAM)Create);
}

static VOID
OnActivate(PGUI_CONSOLE_DATA GuiData, WPARAM wParam)
{
    WORD ActivationState = LOWORD(wParam);

    DPRINT("WM_ACTIVATE - ActivationState = %d\n", ActivationState);

    GuiData->IsWindowActive = (ActivationState != WA_INACTIVE);

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
     * Ignore the next mouse event when we are going to be enabled again via
     * the mouse, in order to prevent, e.g. when we are in Edit mode, erroneous
     * mouse actions from the user that could spoil text selection or copy/pastes.
     */
    if (ActivationState == WA_CLICKACTIVE)
        GuiData->IgnoreNextMouseEvent = TRUE;
}

static VOID
OnFocus(PGUI_CONSOLE_DATA GuiData, BOOL SetFocus)
{
    PCONSRV_CONSOLE Console = GuiData->Console;
    INPUT_RECORD er;

    if (!ConDrvValidateConsoleUnsafe((PCONSOLE)Console, CONSOLE_RUNNING, TRUE)) return;

    /* Set console focus state */
    Console->HasFocus = SetFocus;

    /*
     * Set the priority of the processes of this console
     * in accordance with the console focus state.
     */
    ConSrvSetConsoleProcessFocus(Console, SetFocus);

    /* Send a focus event */
    er.EventType = FOCUS_EVENT;
    er.Event.FocusEvent.bSetFocus = SetFocus;
    ConioProcessInputEvent(Console, &er);

    LeaveCriticalSection(&Console->Lock);

    if (SetFocus)
        DPRINT("TODO: Create console caret\n");
    else
        DPRINT("TODO: Destroy console caret\n");
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

    // TODO: Scroll buffer to bring 'coord' into view

    if (coord != NULL)
    {
        SMALL_RECT rc;
        HRGN newRgn;

        /*
         * Pressing the Control key while selecting text, allows us to enter
         * into line-selection mode, the selection mode of *nix terminals.
         */
        BOOL OldLineSel = GuiData->LineSelection;
        GuiData->LineSelection = !!(GetKeyState(VK_CONTROL) & KEY_PRESSED);

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
                WindowTitle[SelTypeStrLength] = UNICODE_NULL;
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

        // TODO: Move cursor display here!

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
                 (VirtualKeyCode == 'C' && (GetKeyState(VK_CONTROL) & KEY_PRESSED)) )
        {
            /* Cancel selection if ESC or Ctrl-C are pressed */
            UpdateSelection(GuiData, NULL, NULL);
            goto Quit;
        }

        if ((GuiData->Selection.dwFlags & CONSOLE_MOUSE_SELECTION) == 0)
        {
            /* Keyboard selection mode */
            BOOL Interpreted = FALSE;
            BOOL MajPressed  = !!(GetKeyState(VK_SHIFT) & KEY_PRESSED);

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
        /* Repaint the caret */
        if (GuiData->IsWindowActive || Buff->CursorBlinkOn)
        {
            InvalidateCell(GuiData, Buff->CursorPosition.X, Buff->CursorPosition.Y);
            Buff->CursorBlinkOn = !Buff->CursorBlinkOn;
        }

        if ((GuiData->OldCursor.x != Buff->CursorPosition.X) ||
            (GuiData->OldCursor.y != Buff->CursorPosition.Y))
        {
            SCROLLINFO sInfo;
            int OldScrollX = -1, OldScrollY = -1;
            int NewScrollX = -1, NewScrollY = -1;

            sInfo.cbSize = sizeof(sInfo);
            sInfo.fMask = SIF_POS;
            // Capture the original position of the scroll bars and save them.
            if (GetScrollInfo(GuiData->hWindow, SB_HORZ, &sInfo)) OldScrollX = sInfo.nPos;
            if (GetScrollInfo(GuiData->hWindow, SB_VERT, &sInfo)) OldScrollY = sInfo.nPos;

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
                    sInfo.nPos = NewScrollX;
                    SetScrollInfo(GuiData->hWindow, SB_HORZ, &sInfo, TRUE);
                }
                if (NewScrollY >= 0)
                {
                    sInfo.nPos = NewScrollY;
                    SetScrollInfo(GuiData->hWindow, SB_VERT, &sInfo, TRUE);
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

    /* Free the GuiData registration */
    SetWindowLongPtrW(hWnd, GWLP_USERDATA, (DWORD_PTR)NULL);

    /* Reset the system menu back to default and destroy the previous menu */
    GetSystemMenu(hWnd, TRUE);

    if (GuiData)
    {
        if (GuiData->IsWindowVisible)
            KillTimer(hWnd, CONGUI_UPDATE_TIMER);

        /* Free the terminal framebuffer */
        if (GuiData->hMemDC ) DeleteDC(GuiData->hMemDC);
        if (GuiData->hBitmap) DeleteObject(GuiData->hBitmap);
        // if (GuiData->hSysPalette) DeleteObject(GuiData->hSysPalette);
        DeleteFonts(GuiData);
    }

    return DefWindowProcW(hWnd, WM_NCDESTROY, 0, 0);
}

static VOID
OnScroll(PGUI_CONSOLE_DATA GuiData, INT nBar, WORD sbCode)
{
    PCONSRV_CONSOLE Console = GuiData->Console;
    PCONSOLE_SCREEN_BUFFER Buff;
    SCROLLINFO sInfo;
    INT oldPos, Maximum;
    PSHORT pOriginXY;

    ASSERT(nBar == SB_HORZ || nBar == SB_VERT);

    if (!ConDrvValidateConsoleUnsafe((PCONSOLE)Console, CONSOLE_RUNNING, TRUE)) return;

    Buff = GuiData->ActiveBuffer;

    if (nBar == SB_HORZ)
    {
        Maximum = Buff->ScreenBufferSize.X - Buff->ViewSize.X;
        pOriginXY = &Buff->ViewOrigin.X;
    }
    else // if (nBar == SB_VERT)
    {
        Maximum = Buff->ScreenBufferSize.Y - Buff->ViewSize.Y;
        pOriginXY = &Buff->ViewOrigin.Y;
    }

    /* Set scrollbar sizes */
    sInfo.cbSize = sizeof(sInfo);
    sInfo.fMask = SIF_RANGE | SIF_POS | SIF_PAGE | SIF_TRACKPOS;

    if (!GetScrollInfo(GuiData->hWindow, nBar, &sInfo)) goto Quit;

    oldPos = sInfo.nPos;

    switch (sbCode)
    {
        case SB_LINEUP:   // SB_LINELEFT:
            sInfo.nPos--;
            break;

        case SB_LINEDOWN: // SB_LINERIGHT:
            sInfo.nPos++;
            break;

        case SB_PAGEUP:   // SB_PAGELEFT:
            sInfo.nPos -= sInfo.nPage;
            break;

        case SB_PAGEDOWN: // SB_PAGERIGHT:
            sInfo.nPos += sInfo.nPage;
            break;

        case SB_THUMBTRACK:
            sInfo.nPos = sInfo.nTrackPos;
            ConioPause(Console, PAUSED_FROM_SCROLLBAR);
            break;

        case SB_THUMBPOSITION:
            sInfo.nPos = sInfo.nTrackPos;
            ConioUnpause(Console, PAUSED_FROM_SCROLLBAR);
            break;

        case SB_TOP:    // SB_LEFT:
            sInfo.nPos = sInfo.nMin;
            break;

        case SB_BOTTOM: // SB_RIGHT:
            sInfo.nPos = sInfo.nMax;
            break;

        default:
            break;
    }

    sInfo.nPos = min(max(sInfo.nPos, 0), Maximum);

    if (oldPos != sInfo.nPos)
    {
        USHORT OldX = Buff->ViewOrigin.X;
        USHORT OldY = Buff->ViewOrigin.Y;
        UINT   WidthUnit, HeightUnit;

        /* We now modify Buff->ViewOrigin */
        *pOriginXY = sInfo.nPos;

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
        SetScrollInfo(GuiData->hWindow, nBar, &sInfo, TRUE);

        UpdateWindow(GuiData->hWindow);
        // InvalidateRect(GuiData->hWindow, NULL, FALSE);
    }

Quit:
    LeaveCriticalSection(&Console->Lock);
    return;
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
    BOOL DoDefault = FALSE;
    PCONSRV_CONSOLE Console = GuiData->Console;

    /*
     * HACK FOR CORE-8394 (Part 2):
     *
     * Check whether we should ignore the next mouse move event.
     * In either case we reset the HACK flag.
     *
     * See Part 1 of this hack below.
     */
    if (GuiData->HackCORE8394IgnoreNextMove && msg == WM_MOUSEMOVE)
    {
        GuiData->HackCORE8394IgnoreNextMove = FALSE;
        goto Quit;
    }
    GuiData->HackCORE8394IgnoreNextMove = FALSE;

    // FIXME: It's here that we need to check whether we have focus or not
    // and whether we are or not in edit mode, in order to know if we need
    // to deal with the mouse.

    if (GuiData->IgnoreNextMouseEvent)
    {
        if (msg != WM_LBUTTONDOWN &&
            msg != WM_MBUTTONDOWN &&
            msg != WM_RBUTTONDOWN &&
            msg != WM_XBUTTONDOWN)
        {
            /*
             * If this mouse event is not a button-down action
             * then this is the last one being ignored.
             */
            GuiData->IgnoreNextMouseEvent = FALSE;
        }
        else
        {
            /*
             * This mouse event is a button-down action.
             * Ignore it and perform default action.
             */
            DoDefault = TRUE;
        }
        goto Quit;
    }

    if (!ConDrvValidateConsoleUnsafe((PCONSOLE)Console, CONSOLE_RUNNING, TRUE))
    {
        DoDefault = TRUE;
        goto Quit;
    }

    if ( (GuiData->Selection.dwFlags & CONSOLE_SELECTION_IN_PROGRESS) ||
         (Console->QuickEdit) )
    {
        switch (msg)
        {
            case WM_LBUTTONDOWN:
            {
                /* Check for selection state */
                if ( (GuiData->Selection.dwFlags & CONSOLE_SELECTION_IN_PROGRESS) &&
                     (GuiData->Selection.dwFlags & CONSOLE_MOUSE_SELECTION)       &&
                     (GetKeyState(VK_SHIFT) & KEY_PRESSED) )
                {
                    /*
                     * A mouse selection is currently in progress and the user
                     * has pressed the SHIFT key and clicked somewhere, update
                     * the selection.
                     */
                    GuiData->dwSelectionCursor = PointToCoord(GuiData, lParam);
                    UpdateSelection(GuiData, NULL, &GuiData->dwSelectionCursor);
                }
                else
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
                }

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

                    /* Ignore the next mouse move event */
                    GuiData->IgnoreNextMouseEvent = TRUE;
#undef IS_WORD_SEP
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

                /* Ignore the next mouse move event */
                GuiData->IgnoreNextMouseEvent = TRUE;
                break;
            }

            case WM_MOUSEMOVE:
            {
                if (!(GET_KEYSTATE_WPARAM(wParam) & MK_LBUTTON)) break;
                if (!(GuiData->Selection.dwFlags & CONSOLE_MOUSE_DOWN)) break;

                GuiData->dwSelectionCursor = PointToCoord(GuiData, lParam);
                UpdateSelection(GuiData, NULL, &GuiData->dwSelectionCursor);
                break;
            }

            default:
                DoDefault = TRUE; // FALSE;
                break;
        }

        /*
         * HACK FOR CORE-8394 (Part 1):
         *
         * It appears that when running ReactOS on VBox with Mouse Integration
         * enabled, the next mouse event coming after a button-down action is
         * a mouse-move. However it is NOT always a rule, so that we cannot use
         * the IgnoreNextMouseEvent flag to just "ignore" the next mouse event,
         * thinking it would always be a mouse-move event.
         *
         * To work around this problem (that should really be fixed in Win32k),
         * we use a second flag to ignore this possible next mouse move event.
         */
        switch (msg)
        {
            case WM_LBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_XBUTTONDOWN:
                GuiData->HackCORE8394IgnoreNextMove = TRUE;
            default:
                break;
        }
    }
    else if (GetConsoleInputBufferMode(Console) & ENABLE_MOUSE_INPUT)
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

            case WM_XBUTTONDOWN:
            {
                /* Get which X-button was pressed */
                WORD wButton = GET_XBUTTON_WPARAM(wParam);

                /* Check for X-button validity */
                if (wButton & ~(XBUTTON1 | XBUTTON2))
                {
                    DPRINT1("X-button 0x%04x invalid\n", wButton);
                    DoDefault = TRUE;
                    break;
                }

                SetCapture(GuiData->hWindow);
                dwButtonState = (wButton == XBUTTON1 ? FROM_LEFT_3RD_BUTTON_PRESSED
                                                     : FROM_LEFT_4TH_BUTTON_PRESSED);
                dwEventFlags  = 0;
                break;
            }

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

            case WM_XBUTTONUP:
            {
                /* Get which X-button was released */
                WORD wButton = GET_XBUTTON_WPARAM(wParam);

                /* Check for X-button validity */
                if (wButton & ~(XBUTTON1 | XBUTTON2))
                {
                    DPRINT1("X-button 0x%04x invalid\n", wButton);
                    /* Ok, just release the button anyway... */
                }

                ReleaseCapture();
                dwButtonState = 0;
                dwEventFlags  = 0;
                break;
            }

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

            case WM_XBUTTONDBLCLK:
            {
                /* Get which X-button was double-clicked */
                WORD wButton = GET_XBUTTON_WPARAM(wParam);

                /* Check for X-button validity */
                if (wButton & ~(XBUTTON1 | XBUTTON2))
                {
                    DPRINT1("X-button 0x%04x invalid\n", wButton);
                    DoDefault = TRUE;
                    break;
                }

                dwButtonState = (wButton == XBUTTON1 ? FROM_LEFT_3RD_BUTTON_PRESSED
                                                     : FROM_LEFT_4TH_BUTTON_PRESSED);
                dwEventFlags  = DOUBLE_CLICK;
                break;
            }

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
                DoDefault = TRUE;
                break;
        }

        /*
         * HACK FOR CORE-8394 (Part 1):
         *
         * It appears that when running ReactOS on VBox with Mouse Integration
         * enabled, the next mouse event coming after a button-down action is
         * a mouse-move. However it is NOT always a rule, so that we cannot use
         * the IgnoreNextMouseEvent flag to just "ignore" the next mouse event,
         * thinking it would always be a mouse-move event.
         *
         * To work around this problem (that should really be fixed in Win32k),
         * we use a second flag to ignore this possible next mouse move event.
         */
        switch (msg)
        {
            case WM_LBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_XBUTTONDOWN:
                GuiData->HackCORE8394IgnoreNextMove = TRUE;
            default:
                break;
        }

        if (!DoDefault)
        {
            if (wKeyState & MK_LBUTTON)
                dwButtonState |= FROM_LEFT_1ST_BUTTON_PRESSED;
            if (wKeyState & MK_MBUTTON)
                dwButtonState |= FROM_LEFT_2ND_BUTTON_PRESSED;
            if (wKeyState & MK_RBUTTON)
                dwButtonState |= RIGHTMOST_BUTTON_PRESSED;
            if (wKeyState & MK_XBUTTON1)
                dwButtonState |= FROM_LEFT_3RD_BUTTON_PRESSED;
            if (wKeyState & MK_XBUTTON2)
                dwButtonState |= FROM_LEFT_4TH_BUTTON_PRESSED;

            if (GetKeyState(VK_RMENU) & KEY_PRESSED)
                dwControlKeyState |= RIGHT_ALT_PRESSED;
            if (GetKeyState(VK_LMENU) & KEY_PRESSED)
                dwControlKeyState |= LEFT_ALT_PRESSED;
            if (GetKeyState(VK_RCONTROL) & KEY_PRESSED)
                dwControlKeyState |= RIGHT_CTRL_PRESSED;
            if (GetKeyState(VK_LCONTROL) & KEY_PRESSED)
                dwControlKeyState |= LEFT_CTRL_PRESSED;
            if (GetKeyState(VK_SHIFT) & KEY_PRESSED)
                dwControlKeyState |= SHIFT_PRESSED;
            if (GetKeyState(VK_NUMLOCK) & KEY_TOGGLED)
                dwControlKeyState |= NUMLOCK_ON;
            if (GetKeyState(VK_SCROLL) & KEY_TOGGLED)
                dwControlKeyState |= SCROLLLOCK_ON;
            if (GetKeyState(VK_CAPITAL) & KEY_TOGGLED)
                dwControlKeyState |= CAPSLOCK_ON;
            /* See WM_CHAR MSDN documentation for instance */
            if (HIWORD(lParam) & KF_EXTENDED)
                dwControlKeyState |= ENHANCED_KEY;

            /* Send a mouse event */
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
        DoDefault = TRUE;
    }

    LeaveCriticalSection(&Console->Lock);

Quit:
    if (!DoDefault)
        return 0;

    if (msg == WM_MOUSEWHEEL || msg == WM_MOUSEHWHEEL)
    {
        INT   nBar;
        WORD  sbCode;
        // WORD  wKeyState = GET_KEYSTATE_WPARAM(wParam);
        SHORT wDelta    = GET_WHEEL_DELTA_WPARAM(wParam);

        if (msg == WM_MOUSEWHEEL)
            nBar = SB_VERT;
        else // if (msg == WM_MOUSEHWHEEL)
            nBar = SB_HORZ;

        // NOTE: We currently do not support zooming...
        // if (wKeyState & MK_CONTROL)

        // FIXME: For some reason our win32k does not set the key states
        // when sending WM_MOUSEWHEEL or WM_MOUSEHWHEEL ...
        // if (wKeyState & MK_SHIFT)
        if (GetKeyState(VK_SHIFT) & KEY_PRESSED)
            sbCode = (wDelta >= 0 ? SB_PAGEUP : SB_PAGEDOWN);
        else
            sbCode = (wDelta >= 0 ? SB_LINEUP : SB_LINEDOWN);

        OnScroll(GuiData, nBar, sbCode);
    }

    return DefWindowProcW(GuiData->hWindow, msg, wParam, lParam);
}


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

    if (ActiveBuffer->ViewSize.X < ActiveBuffer->ScreenBufferSize.X) windy += GetSystemMetrics(SM_CYHSCROLL); // Window currently has a horizontal scrollbar
    if (ActiveBuffer->ViewSize.Y < ActiveBuffer->ScreenBufferSize.Y) windx += GetSystemMetrics(SM_CXVSCROLL); // Window currently has a vertical scrollbar

    minMaxInfo->ptMaxTrackSize.x = windx;
    minMaxInfo->ptMaxTrackSize.y = windy;

    LeaveCriticalSection(&Console->Lock);
}

static VOID
OnSize(PGUI_CONSOLE_DATA GuiData, WPARAM wParam, LPARAM lParam)
{
    PCONSRV_CONSOLE Console = GuiData->Console;

    /* Do nothing if the window is hidden */
    if (!GuiData->IsWindowVisible || IsIconic(GuiData->hWindow)) return;

    if (!ConDrvValidateConsoleUnsafe((PCONSOLE)Console, CONSOLE_RUNNING, TRUE)) return;

    if (!GuiData->WindowSizeLock &&
        (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED || wParam == SIZE_MINIMIZED))
    {
        PCONSOLE_SCREEN_BUFFER Buff = GuiData->ActiveBuffer;
        DWORD windx, windy, charx, chary;
        UINT  WidthUnit, HeightUnit;

        GetScreenBufferSizeUnits(Buff, GuiData, &WidthUnit, &HeightUnit);

        GuiData->WindowSizeLock = TRUE;

        windx = LOWORD(lParam);
        windy = HIWORD(lParam);

        /* Compensate for existing scroll bars (because lParam values do not accommodate scroll bar) */
        if (Buff->ViewSize.X < Buff->ScreenBufferSize.X) windy += GetSystemMetrics(SM_CYHSCROLL); // Window currently has a horizontal scrollbar
        if (Buff->ViewSize.Y < Buff->ScreenBufferSize.Y) windx += GetSystemMetrics(SM_CXVSCROLL); // Window currently has a vertical scrollbar

        charx = windx / (int)WidthUnit ;
        chary = windy / (int)HeightUnit;

        /* Character alignment (round size up or down) */
        if ((windx % WidthUnit ) >= (WidthUnit  / 2)) ++charx;
        if ((windy % HeightUnit) >= (HeightUnit / 2)) ++chary;

        /* Compensate for added scroll bars in window */
        if (Buff->ViewSize.X < Buff->ScreenBufferSize.X) windy -= GetSystemMetrics(SM_CYHSCROLL); // Window will have a horizontal scroll bar
        if (Buff->ViewSize.Y < Buff->ScreenBufferSize.Y) windx -= GetSystemMetrics(SM_CXVSCROLL); // Window will have a vertical scroll bar

        charx = windx / (int)WidthUnit ;
        chary = windy / (int)HeightUnit;

        /* Character alignment (round size up or down) */
        if ((windx % WidthUnit ) >= (WidthUnit  / 2)) ++charx;
        if ((windy % HeightUnit) >= (HeightUnit / 2)) ++chary;

        /* Resize window */
        if ((charx != Buff->ViewSize.X) || (chary != Buff->ViewSize.Y))
        {
            Buff->ViewSize.X = (charx <= (DWORD)Buff->ScreenBufferSize.X) ? charx : Buff->ScreenBufferSize.X;
            Buff->ViewSize.Y = (chary <= (DWORD)Buff->ScreenBufferSize.Y) ? chary : Buff->ScreenBufferSize.Y;
        }

        ResizeConWnd(GuiData, WidthUnit, HeightUnit);

        /* Adjust the start of the visible area if we are attempting to show nonexistent areas */
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

static VOID
OnDropFiles(PCONSRV_CONSOLE Console, HDROP hDrop)
{
    LPWSTR pszPath;
    WCHAR szPath[MAX_PATH + 2];

    szPath[0] = L'"';

    DragQueryFileW(hDrop, 0, &szPath[1], ARRAYSIZE(szPath) - 1);
    DragFinish(hDrop);

    if (wcschr(&szPath[1], L' ') != NULL)
    {
        StringCchCatW(szPath, ARRAYSIZE(szPath), L"\"");
        pszPath = szPath;
    }
    else
    {
        pszPath = &szPath[1];
    }

    PasteText(Console, pszPath, wcslen(pszPath));
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

HBITMAP
CreateFrameBufferBitmap(HDC hDC, int width, int height)
{
    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = GetDeviceCaps(hDC, BITSPIXEL);
    bmi.bmiHeader.biCompression = BI_RGB;
    return CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, NULL, NULL, 0);
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

        case WM_ERASEBKGND:
            return TRUE;

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
            /* Detect Alt+Shift */
            if (wParam == VK_SHIFT && (msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP))
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
        case WM_XBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        case WM_XBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
        case WM_XBUTTONDBLCLK:
        case WM_MOUSEMOVE:
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
        {
            Result = OnMouse(GuiData, msg, wParam, lParam);
            break;
        }

        case WM_HSCROLL:
            OnScroll(GuiData, SB_HORZ, LOWORD(wParam));
            break;

        case WM_VSCROLL:
            OnScroll(GuiData, SB_VERT, LOWORD(wParam));
            break;

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

        case WM_DROPFILES:
            OnDropFiles(Console, (HDROP)wParam);
            break;

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
            hnew = CreateFrameBufferBitmap(hDC, Width, Height);
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

        /*
         * Undocumented message sent by Windows' console.dll for applying console info.
         * See https://web.archive.org/web/20160307053337/https://www.catch22.net/sites/default/source/files/setconsoleinfo.c
         * and https://dl.packetstormsecurity.net/papers/win/MSBugPaper.pdf
         * for more information.
         */
        case WM_SETCONSOLEINFO:
        {
            GuiApplyUserSettings(GuiData, (HANDLE)wParam);
            break;
        }

        case PM_CONSOLE_BEEP:
            DPRINT1("Beep\n");
            Beep(800, 200);
            break;

         case PM_CONSOLE_SET_TITLE:
            SetWindowTextW(GuiData->hWindow, GuiData->Console->Title.Buffer);
            break;

        default: Default:
            Result = DefWindowProcW(hWnd, msg, wParam, lParam);
            break;
    }

    return Result;
}

/* EOF */
