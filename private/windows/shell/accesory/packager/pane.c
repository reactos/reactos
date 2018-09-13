/* pane.c - This file contains the multi-pane handling routines.
 *
 * Copyright (c) 1991-, Microsoft Corporation.
 * All rights reserved.
 */


#include "packager.h"
#include <shellapi.h>
#include "dialogs.h"


//#define  OLESVR_SUPPORT           /* enable support for OLE server files */


#define DRAG_EMBED  2                   // Ctrl + Drag
#define DRAG_LINK   6                   // Ctrl + Shift + Drag


static HBRUSH hbrBlack;                 // Black brush
static HCURSOR hcurSplit;
static HWND hwndDesc;
static HWND hwndInsertIcon = NULL;
static HWND hwndView = NULL;
static INT cxBorder;                    // WS_BORDER border width
static INT cyBorder;
static INT cxFudge = 0;                 // Fudge factors for good appearance
static INT cyFudge = 0;
static INT cxMinWidth;
static INT cxView;
static INT cxSplit;                     // Splitter bar width
static INT cxPict;
static INT cxDesc;
static INT cxInsertIcon;
static INT cxMin[CCHILDREN];
static INT cyHeight;
static INT xSplit = 0;
static CHAR szButton[] = "button";
static CHAR szStatic[] = "static";
static CHAR szPaneClass[] = "PaneClass";
static CHAR szSubtitleClass[] = "SubTitleClass";
static CHAR szDescription[CBMESSAGEMAX];
static CHAR szView[CBMESSAGEMAX];
static CHAR szPicture[CBMESSAGEMAX];
static CHAR szInsertIcon[CBMESSAGEMAX];
static CHAR szDropFile[CBPATHMAX];
static BOOL fHScrollEnable = FALSE;
static BOOL fVScrollEnable = FALSE;


static BOOL MakeWindows(VOID);
static INT GetTextLen(HDC hdc, LPSTR lpstr);
static VOID RecalibrateScroll(INT iPane, DWORD lParam);
static VOID Undo(INT iPane);
static VOID CalcWindows(BOOL fFirst);
static INT Constrain(INT x, INT right);
static VOID CopyOther(VOID);


/* InitPaneClasses() - Do application "global" initialization.
 *
 * This function registers the window classes used by the application.
 * Returns:  TRUE if successful.
 */
BOOL
InitPaneClasses(
    VOID
    )
{
    WNDCLASS  wc;

    wc.style            = 0;
    wc.lpfnWndProc      = SubtitleWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghInst;
    wc.hIcon            = LoadIcon(ghInst, MAKEINTRESOURCE(ID_APPLICATION));
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_3DFACE + 1);
    wc.lpszMenuName     = MAKEINTRESOURCE(ID_APPLICATION);
    wc.lpszClassName    = szSubtitleClass;

    if (!RegisterClass(&wc))
        return FALSE;

    wc.style            = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW;
    wc.lpfnWndProc      = PaneWndProc;
    wc.cbClsExtra       = 0;
    // Reserve space for the item specific data handle
    wc.cbWndExtra       = sizeof(LPVOID);
    wc.hInstance        = ghInst;
    wc.hIcon            = NULL;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_3DFACE + 1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = szPaneClass;

    if (!RegisterClass(&wc))
        return FALSE;

    return TRUE;
}



/* InitPanes() - Handles the instance-specific initialization.
 *
 * This function creates the main application window.
 * Returns:  TRUE if successful.
 */
BOOL
InitPanes(
    VOID
    )
{
    LOGFONT lf;
    LCID lcid = GetThreadLocale();

    hbrBlack = GetStockObject(BLACK_BRUSH);
    hcurSplit = LoadCursor(ghInst, MAKEINTRESOURCE(SPLIT));
    gcxIcon = GetSystemMetrics(SM_CXICON);
    gcyIcon = GetSystemMetrics(SM_CYICON);

    SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, FALSE);
    SystemParametersInfo(SPI_ICONHORIZONTALSPACING, 0, &gcxArrange, FALSE);
    SystemParametersInfo(SPI_ICONVERTICALSPACING, 0, &gcyArrange, FALSE);

        ghfontTitle = CreateFontIndirect(&lf);
        if (PRIMARYLANGID(LANGIDFROMLCID(lcid)) == LANG_CHINESE ||
            PRIMARYLANGID(LANGIDFROMLCID(lcid)) == LANG_JAPANESE)
            lf.lfWeight = FW_NORMAL;
        else
            lf.lfWeight = FW_BOLD;
        ghfontChild = CreateFontIndirect(&lf);


    if (!(ghfontTitle || ghfontChild))
    {
        if (ghfontTitle)
            DeleteObject(ghfontTitle);

        return FALSE;
    }

    LoadString(ghInst, IDS_CONTENT, gszCaption[CONTENT], CBMESSAGEMAX);
    LoadString(ghInst, IDS_VIEW, szView, CBMESSAGEMAX);
    LoadString(ghInst, IDS_DESCRIPTION, szDescription, CBMESSAGEMAX);
    LoadString(ghInst, IDS_PICTURE, szPicture, CBMESSAGEMAX);
    LoadString(ghInst, IDS_APPEARANCE, gszCaption[APPEARANCE], CBMESSAGEMAX);
    LoadString(ghInst, IDS_INSERTICON, szInsertIcon, CBMESSAGEMAX);

    // Create the window panes
    if (!MakeWindows())
        return FALSE;

    CalcWindows(TRUE);

    // Give the focus to the content pane
    PostMessage(ghwndPane[CONTENT], WM_LBUTTONDOWN, 0, 0L);

    return TRUE;
}



/* EndPaneInstance() - Instance-specific termination code.
 */
VOID
EndPanes(
    VOID
    )
{
    if (ghfontTitle)
        DeleteObject(ghfontTitle);

    if (ghfontChild)
        DeleteObject(ghfontChild);
}



/* MakeWindows() - Make the window panes.
 */
static BOOL
MakeWindows(
    VOID
    )
{
    if (ghwndBar[CONTENT] =
        CreateWindow(szSubtitleClass, gszCaption[CONTENT], WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0, ghwndFrame, NULL, ghInst, NULL))
    {

        hwndView = CreateWindow(szStatic, szView,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            0, 0, 0, 0, ghwndBar[CONTENT], NULL, ghInst, NULL);

        hwndDesc = CreateWindow(szButton, szDescription,
            WS_CHILD | BS_AUTORADIOBUTTON | WS_VISIBLE | WS_GROUP,
            0, 0, 0, 0, ghwndBar[CONTENT], (HMENU)IDM_DESC, ghInst, NULL);

        ghwndPict = CreateWindow(szButton, szPicture,
            WS_CHILD | BS_AUTORADIOBUTTON | WS_VISIBLE,
            0, 0, 0, 0, ghwndBar[CONTENT], (HMENU)IDM_PICT, ghInst, NULL);

        if (hwndView && hwndDesc && ghwndPict)
        {
            // Use the appropriate dialog font
            SendMessage(ghwndBar[CONTENT], WM_SETFONT, (WPARAM)ghfontChild, TRUE);
            SendMessage(hwndView, WM_SETFONT, (WPARAM)ghfontChild, TRUE);
            SendMessage(hwndDesc, WM_SETFONT, (WPARAM)ghfontChild, TRUE);
            SendMessage(ghwndPict, WM_SETFONT, (WPARAM)ghfontChild, TRUE);
            CheckRadioButton(ghwndBar[CONTENT], IDM_PICT, IDM_DESC, IDM_DESC);
            EnableWindow(ghwndPict, FALSE);
        }
        else
        {
            goto Error;
        }
    }
    else
    {
        goto Error;
    }

    if (ghwndBar[APPEARANCE] =
        CreateWindow(szSubtitleClass, gszCaption[APPEARANCE],
        WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, ghwndFrame, NULL, ghInst, NULL))
    {
        hwndInsertIcon =
            CreateWindow(szButton, szInsertIcon,
            WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE,
            0, 0, 0, 0, ghwndBar[APPEARANCE], (HMENU)IDM_INSERTICON,
            ghInst, NULL);

        if (hwndInsertIcon)
        {
            SendMessage(ghwndBar[APPEARANCE], WM_SETFONT, (WPARAM)ghfontChild,
                 TRUE);
            SendMessage(hwndInsertIcon, WM_SETFONT, (WPARAM)ghfontChild, TRUE);
        }
        else
        {
            goto Error;
        }
    }
    else
    {
        goto Error;
    }

    ghwndPane[APPEARANCE] =
        CreateWindowEx(WS_EX_CLIENTEDGE, szPaneClass, NULL,
        WS_BORDER | WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL,
        0, 0, 0, 0, ghwndFrame, NULL, ghInst, NULL);

    ghwndPane[CONTENT] =
        CreateWindowEx(WS_EX_CLIENTEDGE, szPaneClass, NULL,
        WS_BORDER | WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL,
        0, 0, 0, 0, ghwndFrame, NULL, ghInst, NULL);

    if (!ghwndPane[APPEARANCE] || !ghwndPane[CONTENT])
        goto Error;

    EnableScrollBar(ghwndPane[APPEARANCE], SB_HORZ, ESB_DISABLE_BOTH);
    EnableScrollBar(ghwndPane[APPEARANCE], SB_VERT, ESB_DISABLE_BOTH);
    EnableScrollBar(ghwndPane[CONTENT], SB_HORZ, ESB_DISABLE_BOTH);
    EnableScrollBar(ghwndPane[CONTENT], SB_VERT, ESB_DISABLE_BOTH);

    DragAcceptFiles(ghwndPane[CONTENT], TRUE);

    return TRUE;

Error:
    if (ghwndBar[CONTENT])
    {
        if (hwndView)
            DestroyWindow(hwndView);

        if (hwndDesc)
            DestroyWindow(hwndDesc);

        if (ghwndPict)
            DestroyWindow(ghwndPict);

        DestroyWindow(ghwndBar[CONTENT]);
    }

    if (ghwndBar[APPEARANCE])
    {
        if (hwndInsertIcon)
            DestroyWindow(hwndInsertIcon);

        DestroyWindow(ghwndBar[APPEARANCE]);
    }

    if (ghwndPane[APPEARANCE])
        DestroyWindow(ghwndPane[APPEARANCE]);

    if (ghwndPane[CONTENT])
        DestroyWindow(ghwndPane[CONTENT]);

    return FALSE;
}



/* SubtitleWndProc() - "Appearance" and "Content" bar window procedure.
 */
LRESULT CALLBACK
SubtitleWndProc(
    HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PAINTSTRUCT ps;
    RECT rcCaption;
    INT iPane;

    iPane = (hWnd == ghwndBar[CONTENT]);

    switch (msg)
    {
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDM_INSERTICON:
                    Raise(APPEARANCE);
                    DeletePane(APPEARANCE, FALSE);

                    if (gptyUndo[APPEARANCE] != ICON)
                        glpobj[APPEARANCE] = IconCreateFromFile("");
                    else
                        glpobj[APPEARANCE] = IconClone(glpobjUndo[APPEARANCE]);

                    if (glpobj[APPEARANCE])
                        gpty[APPEARANCE] = ICON;

                    if (glpobj[APPEARANCE] && IconDialog(glpobj[APPEARANCE]))
                    {
                        InvalidateRect(ghwndPane[APPEARANCE], NULL, TRUE);
                        Dirty();
                    }
                    else
                    {
                        IconDelete(glpobj[APPEARANCE]);
                        gpty[APPEARANCE] = NOTHING;
                        glpobj[APPEARANCE] = NULL;
                        SendMessage(ghwndPane[APPEARANCE], WM_COMMAND,
                            IDM_UNDO, 0);
                    }

                    break;

                case IDM_DESC:
                    if (!IsDlgButtonChecked(ghwndBar[CONTENT], IDM_DESC))
                        CheckRadioButton(ghwndBar[CONTENT], IDM_PICT,
                            IDM_DESC, IDM_DESC);

                    if (fHScrollEnable)
                        EnableScrollBar(ghwndPane[iPane], SB_HORZ,
                            ESB_DISABLE_BOTH);

                    if (fVScrollEnable)
                        EnableScrollBar(ghwndPane[iPane], SB_VERT,
                            ESB_DISABLE_BOTH);

                    InvalidateRect(ghwndPane[CONTENT], NULL, TRUE);

                    goto defProcess;

                case IDM_PICT:
                    if (!IsDlgButtonChecked(ghwndBar[CONTENT], IDM_PICT)
                        && IsWindowEnabled(GetDlgItem(ghwndBar[CONTENT],
                        IDM_PICT)))
                        CheckRadioButton(ghwndBar[CONTENT], IDM_PICT,
                            IDM_DESC, IDM_PICT);

                    if (fHScrollEnable)
                        EnableScrollBar(ghwndPane[iPane], SB_HORZ,
                            ESB_ENABLE_BOTH);

                    if (fVScrollEnable)
                        EnableScrollBar(ghwndPane[iPane], SB_VERT,
                            ESB_ENABLE_BOTH);

                    InvalidateRect(ghwndPane[CONTENT], NULL, TRUE);
                    // Fall through

                default:
defProcess:
                    if (GetTopWindow(ghwndFrame) != ghwndPane[iPane])
                    {
                        if (gbDBCS)
                        {
                            /* 4-Oct-93 #2701 v-katsuy */
                             //win31#1203: 12/26/92:fixing Focus Line Scroll
                             //delete Focus Rect on another pane
                            InvalidateRect(ghwndPane[APPEARANCE], NULL, TRUE);
                        }
                        BringWindowToTop(ghwndPane[iPane]);
                        InvalidateRect(ghwndBar[APPEARANCE], NULL, TRUE);
                        InvalidateRect(ghwndBar[CONTENT], NULL, TRUE);
                        if (LOWORD(wParam) == IDM_PICT
                            || LOWORD(wParam) == IDM_DESC)
                            UpdateWindow(ghwndPane[CONTENT]);
                    }
            }

            break;

        case WM_LBUTTONDOWN:
            if (GetTopWindow(ghwndFrame) != ghwndPane[iPane])
                SendMessage(ghwndPane[iPane], WM_LBUTTONDOWN, 0, 0);

            break;

        case WM_PAINT:
            {
                HFONT hfontOld;

                GetClientRect(hWnd, &rcCaption);
                BeginPaint(hWnd, &ps);

                if (GetTopWindow(ghwndFrame) == ghwndPane[iPane])
                {
                    SetTextColor(ps.hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                    SetBkColor(ps.hdc, GetSysColor(COLOR_HIGHLIGHT));
                }
                else
                {
                    SetTextColor(ps.hdc, GetSysColor(COLOR_WINDOWTEXT));
                    SetBkColor(ps.hdc, GetSysColor(COLOR_WINDOW));
                }

                hfontOld = SelectObject(ps.hdc, ghfontChild);
                rcCaption.left += cxFudge;
                DrawText(ps.hdc, gszCaption[iPane], -1, &rcCaption,
                    DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
                SelectObject(ps.hdc, hfontOld);

                EndPaint(hWnd, &ps);
            }

            break;

        case WM_SIZE:
            if (iPane == APPEARANCE)
            {
                if (hwndInsertIcon)
                {
                    GetClientRect(hWnd, &rcCaption);

                    SetWindowPos(hwndInsertIcon, 0,
                        rcCaption.right - cxInsertIcon, cyFudge, 0, 0,
                        SWP_NOSIZE | SWP_NOZORDER);

                    InvalidateRect(ghwndBar[APPEARANCE], NULL, TRUE);
                }
            }
            else
            {
                if (hwndView)
                {
                    BOOL bChinese = PRIMARYLANGID(LANGIDFROMLCID(GetThreadLocale())) == LANG_CHINESE;
                    GetClientRect(hWnd, &rcCaption);
                    SetWindowPos(hwndView, 0,
                                 bChinese ?
                                   rcCaption.right - cxDesc - cxPict - cxView - 15 :
                                   rcCaption.right - cxDesc - cxPict - cxView,
                                 cyFudge * 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

                    SetWindowPos(hwndDesc, 0,
                                 rcCaption.right - cxDesc - cxPict,
                                 cyFudge, 0, 0,
                                 SWP_NOSIZE | SWP_NOZORDER);

                    SetWindowPos(ghwndPict, 0,
                                 rcCaption.right - cxPict,
                                 cyFudge, 0, 0,
                                 SWP_NOSIZE | SWP_NOZORDER);

                    InvalidateRect(ghwndBar[CONTENT], NULL, TRUE);
                }
            }

            break;

        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    return 0L;
}



static INT
GetTextLen(
    HDC hdc,
    LPSTR lpstr
    )
{
    SIZE Size;

    GetTextExtentPoint(hdc, lpstr, lstrlen(lpstr), &Size);

    return Size.cx + (cxFudge * 2);
}



LRESULT CALLBACK
PaneWndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    BOOL fFocus;
    LPVOID lpobjTemp;
    PAINTSTRUCT ps;
    RECT rc;
    CHAR szFile[CBPATHMAX];
    INT iOld;
    INT iPane;
    INT iPos;
    INT Max;
    INT Min;
    INT nBar;

    iPane = (hwnd == ghwndPane[CONTENT]);

    switch (msg)
    {
        case WM_HSCROLL:
        case WM_VSCROLL:
            // If not the content pane in picture mode, break
            if (gpty[iPane] == PICTURE
                && iPane == CONTENT
                && !IsDlgButtonChecked(ghwndBar[CONTENT], IDM_PICT))
                break;

            // Can't scroll anything but cmd line and picture
            if (gpty[iPane] != PICTURE && gpty[iPane] != CMDLINK)
                break;

            nBar = (msg == WM_HSCROLL ? SB_HORZ : SB_VERT);
            iOld = iPos = GetScrollPos(hwnd, nBar);

            switch (LOWORD(wParam))
            {
                case SB_LINEUP:
                    iPos--;
                    break;

                case SB_LINEDOWN:
                    iPos++;
                    break;

                case SB_PAGEUP:
                case SB_PAGEDOWN:
                    GetClientRect(hwnd, &rc);
                    if (LOWORD(wParam) == SB_PAGEUP)
                        iPos -= (rc.bottom - rc.top + 1);
                    else
                        iPos += (rc.bottom - rc.top + 1);

                    break;

                case SB_THUMBPOSITION:
                    iPos = (INT)HIWORD(wParam);
                    break;
            }

            // Make sure that iPos is in the range
            GetScrollRange(hwnd, nBar, &Min, &Max);

            if (iPos < Min)
                iPos = Min;
            if (iPos > Max)
                iPos = Max;

            SetScrollPos(hwnd, nBar, iPos, TRUE);

            if (msg == WM_HSCROLL)
                ScrollWindow(hwnd, iOld - iPos, 0, NULL, NULL);
            else
                ScrollWindow(hwnd, 0, iOld - iPos, NULL, NULL);

            UpdateWindow(hwnd);
            break;

        case WM_LBUTTONDOWN:
            if (GetTopWindow(ghwndFrame) != hwnd)
            {
                BringWindowToTop(hwnd);
                InvalidateRect(ghwndBar[APPEARANCE], NULL, TRUE);
                InvalidateRect(ghwndBar[CONTENT], NULL, TRUE);
                InvalidateRect(ghwndPane[APPEARANCE], NULL, TRUE);
                InvalidateRect(ghwndPane[CONTENT], NULL, TRUE);
            }

            break;

        case WM_PAINT:
            GetClientRect(hwnd, &rc);
            BeginPaint(hwnd, &ps);
            if (fFocus = (ghwndPane[iPane] == GetTopWindow(ghwndFrame)))
            {
                SetTextColor(ps.hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                SetBkColor(ps.hdc, GetSysColor(COLOR_HIGHLIGHT));
            }
            else
            {
                SetTextColor(ps.hdc, GetSysColor(COLOR_WINDOWTEXT));
                SetBkColor(ps.hdc, GetSysColor(COLOR_WINDOW));
            }

            switch (gpty[iPane])
            {
                case CMDLINK:
                    CmlDraw(glpobj[iPane], ps.hdc, &rc,
                        GetScrollPos(hwnd, SB_HORZ), fFocus);
                    break;

                case PEMBED:
                    EmbDraw(glpobj[iPane], ps.hdc, &rc, fFocus);
                    break;

                case ICON:
                    IconDraw(glpobj[iPane], ps.hdc, &rc, fFocus, 0, 0);
                    break;

                case PICTURE:
                    PicDraw(glpobj[iPane], ps.hdc, &rc,
                        GetScrollPos(hwnd, SB_HORZ),
                        GetScrollPos(hwnd, SB_VERT),
                        hwnd == ghwndPane[APPEARANCE] ||
                        IsDlgButtonChecked(ghwndBar[CONTENT], IDM_PICT), fFocus);
                    break;

                default:
                    FillRect(ps.hdc, &rc, ghbrBackground);
                    break;
            }

            EndPaint(hwnd, &ps);
            break;

        case WM_FIXSCROLL:
            GetClientRect(hwnd, &rc);
            lParam = ((DWORD)rc.bottom << 16) | (DWORD)rc.right;

            // Fall through

        case WM_SIZE:
            if (gpty[iPane] == PICTURE || gpty[iPane] == CMDLINK)
                RecalibrateScroll(iPane, (DWORD)lParam);

            break;

        case WM_DESTROY:
            DeletePane(iPane, TRUE);
            break;

        case WM_DROPFILES:
            {
                BYTE bKeyState = 0;

                // Retrieve the file name
                DragQueryFile((HANDLE)wParam, 0, szDropFile, CBPATHMAX);

                DragFinish((HANDLE)wParam);

                // We got dropped on, so bring ourselves to the top
                BringWindowToTop(ghwndFrame);
                BringWindowToTop(hwnd);

                // See what the user wants us to do
                bKeyState = ((GetKeyState(VK_SHIFT) < 0) << 2)
                    | ((GetKeyState(VK_CONTROL) < 0) << 1)
                    | ((GetKeyState(VK_MENU) < 0));

                switch (bKeyState)
                {
                    case DRAG_LINK:
                        PostMessage(hwnd, WM_COMMAND, IDM_LINKFILE, 0L);
                        break;

                    case DRAG_EMBED:
                    default:
                        PostMessage(hwnd, WM_COMMAND, IDM_EMBEDFILE, 0L);
                        break;
                }

                break;
            }

        case WM_LBUTTONDBLCLK:
            // Alt + Double Click = Properties
            if (gpty[iPane] == PICTURE && GetKeyState(VK_MENU) < 0)
            {
                wParam = IDM_LINKS;
            }
            else
            {
                if (gpty[iPane] == PEMBED)
                {
                    //
                    // If the server is a OLE server, we want to activate in
                    // OLE fashion. But from users perspective it should not
                    // look like an object. So for non-objects double-click
                    // implies show the server. We should try to do the same
                    // thing while editing ole server files.
                    //
                    wParam = IDD_EDIT;
                }
                else
                {
                    wParam  = IDD_PLAY;
                }
            }

            msg = WM_COMMAND;
            lParam = 0;

            // Fall through

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDM_COPY:
                case IDM_CUT:
                    switch (gpty[iPane])
                    {
                        case PICTURE:
                            PicCopy(glpobj[iPane]);

                        default:
                            if (iPane == APPEARANCE)
                                CopyOther();
                            break;
                    }

                    if (LOWORD(wParam) == IDM_COPY)
                        break;

                    // Fall through to delete the selection

                case IDM_CLEAR:
                    DeletePane(iPane, FALSE);
                    break;

                case IDM_LINKS:
                    {
                        LONG objtype;

                        OleQueryType(((LPPICT)glpobj[iPane])->lpObject, &objtype);
                        if (objtype == OT_LINK)
                            DialogBoxAfterBlock(MAKEINTRESOURCE(DTPROP),
                                ghwndPane[iPane], fnProperties);

                        break;
                    }
                case IDM_LINKFILE:
                    lstrcpy(szFile, szDropFile);
                    wParam = IDM_PASTELINK;
                    goto CreateFromFile;

                case IDM_EMBEDFILE:
                    lstrcpy(szFile, szDropFile);
                    wParam = IDM_PASTE;
                    goto CreateFromFile;

                case IDM_PASTE:
                case IDM_PASTELINK:
                    // Try to paste a file name from the File Manager
                    if (iPane == CONTENT)
                    {
                        HANDLE hdata;
                        LPSTR lpstrFile;

                        if (IsClipboardFormatAvailable(gcfFileName))
                        {
                            if (!OpenClipboard(ghwndFrame))
                                break;

                            if (!(hdata = GetClipboardData(gcfFileName)) || !(lpstrFile =
                                GlobalLock(hdata)))
                            {
                                CloseClipboard();
                                break;
                            }

                            lstrcpy(szFile, lpstrFile);
                            GlobalUnlock(hdata);
                            CloseClipboard();

CreateFromFile:

#ifdef OLESVR_SUPPORT
                            if (IsOleServerDoc (szFile))
                            {
                                lpobjTemp = PicFromFile((wParam == IDM_PASTE),
                                    szFile);
                                if (!lpobjTemp)
                                {
                                    ErrorMessage(E_GET_FROM_CLIPBOARD_FAILED);
                                    break;
                                }

                                goto StuffNewObject;
                            }
                            else
                            {
#endif
                                DeletePane(CONTENT, FALSE);
                                if (wParam == IDM_PASTE)
                                {
                                    if (glpobj[CONTENT] = EmbCreate(szFile))
                                        gpty[CONTENT] = PEMBED;
                                }
                                else
                                {
                                    if (glpobj[CONTENT] =
                                           CmlCreateFromFilename(szFile, TRUE))
                                        gpty[CONTENT] = CMDLINK;
                                }
#ifdef OLESVR_SUPPORT
                            }
#endif
                            InvalidateRect(ghwndPane[CONTENT], NULL, TRUE);
                            Dirty();

                            if (!gpty[APPEARANCE])
                            {
                                if (glpobj[APPEARANCE] =
                                    IconCreateFromFile(szFile))
                                {
                                    gpty[APPEARANCE] = ICON;
                                    InvalidateRect(ghwndPane[APPEARANCE],
                                        NULL, TRUE);
                                }
                            }

                            break;
                        }
                    }

                    // Not a file name, try to paste an OLE object
                    if (!(lpobjTemp = PicPaste(LOWORD(wParam) == IDM_PASTE,
                                                gszCaption[iPane])))
                    {
                        ErrorMessage(E_GET_FROM_CLIPBOARD_FAILED);
                        break;
                    }
#ifdef OLESVR_SUPPORT

StuffNewObject:

#endif
                    DeletePane(iPane, FALSE);
                    glpobj[iPane] = lpobjTemp;
                    gpty[iPane] = PICTURE;
                    SendMessage(ghwndPane[iPane], WM_FIXSCROLL, 0, 0L);
                    InvalidateRect(ghwndPane[iPane], NULL, TRUE);
                    Dirty();

                    if (iPane == CONTENT)
                    {
                        EnableWindow(ghwndPict, TRUE);

                        if (!gpty[APPEARANCE])
                        {
                            if (glpobj[APPEARANCE] = IconCreateFromObject(
                                ((LPPICT)glpobj[iPane])->lpObject))
                            {
                                gpty[APPEARANCE] = ICON;
                                InvalidateRect(ghwndPane[APPEARANCE], NULL, TRUE);
                            }
                        }
                    }

                    Dirty();
                    break;

                case IDD_EDIT:          /* Edit the icon form */
                case IDD_PLAY:
                    switch (gpty[iPane])
                    {
                        case CMDLINK:
                            CmlActivate(glpobj[iPane]);
                            break;

                        case PEMBED:
                            EmbActivate(glpobj[iPane], LOWORD(wParam));
                            break;

                        case PICTURE:
                            PicActivate(glpobj[iPane], LOWORD(wParam));
                            break;

                        default:
                            break;
                    }

                    break;

                case IDD_UPDATE:        /* Update the (link) object */
                    if (gpty[iPane] == PICTURE)
                        PicUpdate(glpobj[iPane]);

                    break;

                case IDD_FREEZE:        /* Make the object static */
                    if (gpty[iPane] == PICTURE)
                        PicFreeze(glpobj[iPane]);

                    break;

                case IDD_CHANGE:
                    if (gpty[iPane] == PICTURE)
                        PicChangeLink(glpobj[iPane]);

                    break;

                case IDM_UNDO:
                    Undo(iPane);
                    break;

                case IDD_AUTO:          /* Change the (link) update options */
                case IDD_MANUAL:
                    if (gpty[iPane] == PICTURE
                        && !PicSetUpdateOptions(glpobj[iPane], LOWORD(wParam)))
                        break;

                case IDM_LINKDONE:      /* The link update has completed */
                    PostMessage(ghwndError, WM_REDRAW, 0, 0L);
                    break;

                default:
                    break;
            }

            break;

        default:
            return (DefWindowProc(hwnd, msg, wParam, lParam));
    }

    return 0;
}



LRESULT CALLBACK
SplitterFrame(
    HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    RECT rc;

    switch (msg)
    {
        case WM_SIZE:
            if (wParam != SIZEICONIC && ghwndBar[APPEARANCE])
            {
                GetClientRect(hWnd, &rc);

                // Make sure the splitter bar is still valid
                xSplit = Constrain(xSplit, rc.right);
                CalcWindows(FALSE);

                // Invalidate the splitter bar, forcing a repaint
                rc.left = xSplit - cxSplit / 2 - cxBorder;
                rc.right = xSplit + cxSplit / 2 + cxBorder;
                InvalidateRect(hWnd, &rc, TRUE);
            }

            break;

        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                RECT rcBlack;

                BeginPaint(hWnd, &ps);
                GetClientRect(hWnd, &rc);

                SetRect(&rcBlack, xSplit - cxSplit / 2 - cxBorder,
                    rc.top, xSplit + cxSplit / 2 + cxBorder,
                    rc.top + cyHeight + cyBorder);
                FillRect(ps.hdc, &rcBlack, hbrBlack);

                SetRect(&rcBlack, xSplit - cxSplit / 2 - cxBorder,
                    rc.bottom - GetSystemMetrics(SM_CYHSCROLL) + 1,
                    xSplit + cxSplit / 2 + cxBorder,
                    rc.bottom);
                FillRect(ps.hdc, &rcBlack, hbrBlack);

                EndPaint(hWnd, &ps);
                break;
            }

        case WM_GETMINMAXINFO:
            {
                LPPOINT rgpt = (LPPOINT)lParam;

                rgpt[3].x = cxMinWidth;
                rgpt[3].y = cyHeight * 6;
                break;
            }

        case WM_LBUTTONDOWN:
            {
                MSG msg1;
                INT x;
                INT y;
                INT dy;
                HDC hdc;
                HCURSOR hcurOld;

                if (IsIconic(hWnd))
                    break;

                x  = LOWORD(lParam);
                GetClientRect(hWnd, &rc);
                y  = 0;
                dy = rc.bottom;

                // Constrain the splitter bar...
                x = Constrain(x, rc.right);
                hdc = GetDC(hWnd);

                // split bar loop
                PatBlt(hdc, x - cxSplit / 2, y, cxSplit, dy, PATINVERT);
                SetCapture(hWnd);
                hcurOld = SetCursor(hcurSplit);

                while (GetMessage(&msg1, NULL, 0, 0))
                {
                    if (msg1.message >= WM_MOUSEFIRST
                        && msg1.message <= WM_MOUSELAST)
                    {
                        if (msg1.message == WM_LBUTTONUP
                            || msg1.message == WM_LBUTTONDOWN)
                            break;

                        if (msg1.message == WM_MOUSEMOVE)
                        {
                            ScreenToClient(hWnd, &msg1.pt);
                            x = Constrain(x, rc.right);

                            // erase old
                            PatBlt(hdc, x - cxSplit / 2, y, cxSplit, dy,
                                PATINVERT);

                            // put down new
                            x = Constrain(msg1.pt.x, rc.right);
                            PatBlt(hdc, x - cxSplit / 2, y, cxSplit, dy,
                                PATINVERT);
                        }
                    }
                    else
                    {
                        DispatchMessage(&msg1);
                    }
                }

                SetCursor(hcurOld);
                ReleaseCapture();

                // Constrain the splitter bar...
                x = Constrain(x, rc.right);

                // erase old
                PatBlt(hdc, x - cxSplit / 2, y, cxSplit, dy, PATINVERT);

                ReleaseDC(hWnd, hdc);

                if (msg1.wParam != VK_ESCAPE)
                {
                    xSplit = x;
                    CalcWindows(FALSE);
                    InvalidateRect(ghwndBar[APPEARANCE], NULL, TRUE);
                    InvalidateRect(ghwndBar[CONTENT], NULL, TRUE);
                }

                break;
            }

        default:
            return FALSE;
    }

    return TRUE;
}



VOID
DeletePane(
    INT iPane,
    BOOL fDeleteUndo
    )
{
    // Delete the last Undo object
    if (glpobjUndo[iPane])
        DeletePaneObject(glpobjUndo[iPane], gptyUndo[iPane]);

    // If we don't wish to keep an undo, delete the object too!
    if (fDeleteUndo)
    {
        DeletePaneObject(glpobj[iPane], gpty[iPane]);
        gptyUndo[iPane] = NOTHING;
        glpobjUndo[iPane] = NULL;
    }
    else
    {
        gptyUndo[iPane] = gpty[iPane];
        glpobjUndo[iPane] = glpobj[iPane];
    }

    // Handle the buttons and such
    if (gpty[iPane] == PICTURE || gpty[iPane] == CMDLINK)
    {
        CHAR szUndoName[CBMESSAGEMAX];

        EnableScrollBar(ghwndPane[iPane], SB_HORZ, ESB_DISABLE_BOTH);
        EnableScrollBar(ghwndPane[iPane], SB_VERT, ESB_DISABLE_BOTH);

        if (gpty[iPane] == PICTURE)
        {
            if (iPane == CONTENT)
            {
                CheckRadioButton(ghwndBar[CONTENT], IDM_PICT, IDM_DESC, IDM_DESC);
                EnableWindow(ghwndPict, FALSE);
            }

            // If the Undo object isn't deleted already, rename it
            if (!fDeleteUndo)
            {
                wsprintf(szUndoName, szUndo, gszCaption[iPane]);
                OleRename(((LPPICT)glpobjUndo[iPane])->lpObject, szUndoName);
            }
        }
    }

    glpobj[iPane] = NULL;
    gpty[iPane]   = NOTHING;

    if (IsWindow(ghwndPane[iPane]))
        InvalidateRect(ghwndPane[iPane], NULL, TRUE);
}



static VOID
RecalibrateScroll(
    INT iPane,
    DWORD lParam
    )
{
    INT cxDel;
    INT cyDel;
    BOOL bDesc = FALSE;
    LPPICT lppict = (LPPICT)glpobj[iPane];

    // Compute the amount of scrolling possible
    cxDel = lppict->rc.right - lppict->rc.left - (INT)(lParam & 0xffff);
    cyDel = lppict->rc.bottom - lppict->rc.top - (INT)(lParam >> 16);

    // Normalize the scroll bar lengths
    if (cxDel < 0)
        cxDel = 0;

    if (cyDel < 0)
        cyDel = 0;

    if (iPane == CONTENT)
    {
        bDesc = IsDlgButtonChecked(ghwndBar[iPane], IDM_DESC);
        fHScrollEnable = cxDel;
        fVScrollEnable = cyDel;
    }

    EnableScrollBar(ghwndPane[iPane], SB_HORZ,
        (cxDel && !bDesc) ? ESB_ENABLE_BOTH : ESB_DISABLE_BOTH);

    EnableScrollBar(ghwndPane[iPane], SB_VERT,
        (cyDel && !bDesc) ? ESB_ENABLE_BOTH : ESB_DISABLE_BOTH);

    // Ensure that the thumb is at a meaningful position
    if (GetScrollPos(ghwndPane[iPane], SB_HORZ) > cxDel)
        SetScrollPos(ghwndPane[iPane], SB_HORZ, cxDel, TRUE);

    if (GetScrollPos(ghwndPane[iPane], SB_VERT) > cyDel)
        SetScrollPos(ghwndPane[iPane], SB_VERT, cyDel, TRUE);
}



static VOID
Undo(
    INT iPane
    )
{
    DWORD ot;
    LPPICT lppict;
    LPVOID lpobjTemp;
    INT ptyTemp;

    if (gpty[iPane] == PICTURE)
    {
        lppict = glpobj[iPane];

        // Close the old object
        if (lppict->lpObject)
        {
            OleQueryType(lppict->lpObject, &ot);
            if (ot != OT_STATIC)
                Error(OleClose(lppict->lpObject));
        }

        OleRename(lppict->lpObject, gszTemp);
    }

    if (gptyUndo[iPane] == PICTURE)
    {
        lppict = glpobjUndo[iPane];

        // Try to reconnect the new object if it's a link
        if (lppict->lpObject)
        {
            OleQueryType(lppict->lpObject, &ot);
            if (ot == OT_LINK && Error(OleReconnect(lppict->lpObject)))
                ErrorMessage(E_FAILED_TO_RECONNECT_OBJECT);
        }

        OleRename(lppict->lpObject, gszCaption[iPane]);
    }

    if (gpty[iPane] == PICTURE)
    {
        CHAR szUndoName[CBMESSAGEMAX];

        lppict = glpobj[iPane];
        wsprintf(szUndoName, szUndo, gszCaption[iPane]);
        OleRename(lppict->lpObject, szUndoName);
    }

    // Handle the buttons and enable/disable scroll bars

    // Going from picture to non-picture, disable all special things
    if (gptyUndo[iPane] != PICTURE && gpty[iPane] == PICTURE)
    {
        if (iPane == CONTENT)
        {
            CheckRadioButton(ghwndBar[CONTENT], IDM_PICT, IDM_DESC, IDM_DESC);
            EnableWindow(ghwndPict, FALSE);
        }

        EnableScrollBar(ghwndPane[iPane], SB_HORZ, ESB_DISABLE_BOTH);
        EnableScrollBar(ghwndPane[iPane], SB_VERT, ESB_DISABLE_BOTH);
    }

    if (gptyUndo[iPane] == PICTURE || gptyUndo[iPane] == CMDLINK)
    {
        SendMessage(ghwndPane[iPane], WM_FIXSCROLL, 0, 0L);

        if (gptyUndo[iPane] == PICTURE)
        {
            // Going from non-picture to picture, enable button
            if (gpty[iPane] != PICTURE && iPane == CONTENT)
                EnableWindow(ghwndPict, TRUE);
        }
    }

    lpobjTemp = glpobj[iPane];
    glpobj[iPane] = glpobjUndo[iPane];
    glpobjUndo[iPane] = lpobjTemp;

    ptyTemp = gpty[iPane];
    gpty[iPane] = gptyUndo[iPane];
    gptyUndo[iPane] = ptyTemp;

    InvalidateRect(ghwndPane[iPane], NULL, TRUE);
    Dirty();
}



static VOID
CalcWindows(
    BOOL fFirst
    )
{
    if (fFirst)
    {
        HDC hdc;

        // Figure out the length of the text strings, and all dimensions
        hdc = GetWindowDC(ghwndFrame);
        cxBorder = GetSystemMetrics(SM_CXBORDER);
        cyBorder = GetSystemMetrics(SM_CYBORDER);
        cxFudge = cxBorder * 2;
        cyFudge = cyBorder * 2;
        cxSplit = cxBorder * 4;

        if (gbDBCS)
        {
            /* #3963 13-Dec-93 v-katsuy */
            /* ORIGINALBUG! Window width calculated for just "&Picture".
             *  This width should be calculate
             *  [Radiobutton] + [Text](not include '&').
             */
            CHAR  szTemp[CBMESSAGEMAX];
            LPSTR lpText, lpTemp;

            for (lpText = szPicture, lpTemp = szTemp; *lpText; ) {
                if (*lpText == '&')
                    lpText++;
                else
                    *lpTemp++ = *lpText++;
            }
            *lpTemp = 0;
            cxPict = GetTextLen(hdc, szTemp) + cxFudge * 2
                   + GetSystemMetrics(SM_CXSIZE); // for radiobutton

            for (lpText = szDescription, lpTemp = szTemp; *lpText; ) {
                if (*lpText == '&')
                    lpText++;
                else
                    *lpTemp++ = *lpText++;
            }
            *lpTemp = 0;
            cxDesc = GetTextLen(hdc, szTemp)
                   + GetSystemMetrics(SM_CXSIZE); // for radiobutton
        }
        else
        {
            cxPict = GetTextLen(hdc, szPicture) + cxFudge * 2;
            cxDesc = GetTextLen(hdc, szDescription);
        }

        cxView = GetTextLen(hdc, szView);
        cxInsertIcon = GetTextLen(hdc, szInsertIcon) + cxFudge;

        cxMin[CONTENT] = cxPict + cxDesc + cxView +
            GetTextLen(hdc, gszCaption[CONTENT]) + cxFudge;
        cxMin[APPEARANCE] = cxInsertIcon +
            GetTextLen(hdc, gszCaption[APPEARANCE]) + cxFudge;
        cyHeight = GetSystemMetrics(SM_CYMENU) + cyFudge * 2;
        ReleaseDC(ghwndFrame, hdc);

        cxMinWidth = cxMin[APPEARANCE] + cxMin[CONTENT] + cxSplit +
            GetSystemMetrics(SM_CXFRAME) + cxFudge;

        // Compute all the window sizes that we can
        SetWindowPos(ghwndFrame, 0, 0, 0,
            cxMinWidth + cxFudge * 20,
            cxMinWidth * 7 / 18,
            SWP_NOMOVE | SWP_NOZORDER);
        SetWindowPos(hwndInsertIcon, 0, 0, 0,
            cxInsertIcon - cxFudge,
            GetSystemMetrics(SM_CYMENU),
            SWP_NOMOVE | SWP_NOZORDER);
        SetWindowPos(hwndView, 0, 0, 0,
            cxView, cyHeight - cyFudge,
            SWP_NOMOVE | SWP_NOZORDER);
        SetWindowPos(ghwndPict, 0, 0, 0,
            (cxPict - cxFudge) << 1,
            GetSystemMetrics(SM_CYMENU),
            SWP_NOMOVE | SWP_NOZORDER);
        SetWindowPos(hwndDesc, 0, 0, 0,
            cxDesc, GetSystemMetrics(SM_CYMENU),
            SWP_NOMOVE | SWP_NOZORDER);
    }
    else
    {
        RECT rc;

        GetClientRect(ghwndFrame, &rc);

        // Move the windows to the appropriate locations
        SetWindowPos(ghwndBar[APPEARANCE], 0, 0, 0,
            xSplit - cxSplit / 2 - cxBorder, cyHeight, SWP_NOZORDER);

        SetWindowPos(ghwndBar[CONTENT], 0,
            xSplit + cxSplit / 2 + cxBorder,
            0,
            rc.right - (xSplit + cxSplit / 2) + 1 - cxBorder,
            cyHeight,
            SWP_NOZORDER);

        SetWindowPos(ghwndPane[APPEARANCE], 0,
            -cxBorder,
            cyHeight,
            cxBorder + xSplit - cxSplit / 2,
            rc.bottom + cyBorder - cyHeight,
            SWP_NOZORDER);

        SetWindowPos(ghwndPane[CONTENT], 0,
            xSplit + cxSplit / 2,
            cyHeight,
            cxBorder + rc.right - (xSplit + cxSplit / 2) + 1,
            rc.bottom + cyBorder - cyHeight,
            SWP_NOZORDER);
    }
}



static INT
Constrain(
    INT x,
    INT right
    )
{
    // Constrain the splitter bar...
    if (x < cxMin[APPEARANCE] + cxSplit / 2 - 1)
        return cxMin[APPEARANCE] + cxSplit / 2 - 1;
    else if (x > (right - cxMin[CONTENT] - cxSplit / 2 + 1))
        return right - cxMin[CONTENT] - cxSplit / 2 + 1;

    return x;
}



/* CopyOther() - Copies the picture in appearance pane
 *
 * Returns:  none
 */
static VOID
CopyOther(
    VOID
    )
{
    HANDLE hdata;

    if (OpenClipboard(ghwndFrame))
    {
        Hourglass(TRUE);
        EmptyClipboard();

        if (hdata = GetMF())
            SetClipboardData(CF_METAFILEPICT, hdata);

        CloseClipboard();
        Hourglass(FALSE);
    }
}



VOID
DeletePaneObject(
    LPVOID lpobj,
    INT objType
    )
{
    switch (objType)
    {
        case CMDLINK:
            CmlDelete(lpobj);
            break;

        case PEMBED:
            EmbDelete(lpobj);
            break;

        case ICON:
            IconDelete(lpobj);
            break;

        case PICTURE:
            PicDelete(lpobj);
            break;
    }
}
