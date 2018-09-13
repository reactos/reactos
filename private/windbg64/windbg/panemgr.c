/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

    panemgr.c

Abstract:

    This module contains the panel manager for the debug windows.

Author:

    William J. Heaton (v-willhe) 20-Jul-1992
    Griffith Wm. Kadnier (v-griffk) 10-Mar-1993

Environment:

    Win32, User Mode

--*/
#include "precomp.h"
#pragma hdrstop

#ifndef GET_WM_COMMAND_HWND
#define GET_WM_COMMAND_HWND(wp, lp)             (HWND)(lp)
#endif

#ifndef GET_WM_COMMAND_CMD
#define GET_WM_COMMAND_CMD(wp, lp)              HIWORD(wp)
#endif

#include <ime.h>

#define PAGE (p->PaneLines-1)

#define TOP_OFFSET    4
#define BOTTOM_OFFSET 1
#define LEFT_OFFSET   4
#define BUTTON_SIZE  12
#define SIZER_HANDLE  4
#define RIGHT_OFFSET  4
#define TOTAL_WIDTH   (LEFT_OFFSET + BUTTON_SIZE + LEFT_OFFSET + SIZER_HANDLE + RIGHT_OFFSET)

#define WS_STYLE  (WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS)
#define LBS_STYLE (LBS_NODATA | LBS_OWNERDRAWFIXED | LBS_NOTIFY | LBS_DISABLENOSCROLL | LBS_NOINTEGRALHEIGHT | LBS_WANTKEYBOARDINPUT)

#define BUTTON_STYLE (LBS_STYLE | WS_STYLE)
#define PANE_STYLE   (LBS_STYLE | WS_STYLE | WS_HSCROLL)
#define SCROLL_STYLE (WS_CHILD  | SBS_VERT)

#define PTEXT (DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER)

extern HMENU hMainMenuSave;
#define CheckMenu() if ((DWORD_PTR)hMainMenu != (DWORD_PTR)hMainMenuSave) DebugBreak()



/*
*  Global Storage (FILE)
*/

WNDPROC lpfnButtonEditProc = NULL;      // Original Button WndProc     (Plus/Minus)
WNDPROC lpfnEditEditProc = NULL;        // Original Edit Window Proc   (Panes)
WNDPROC lpfnSizerEditProc = NULL;       // Original Sizer Window Proc  (bar)

HDC hdcSpace = NULL;
HDC hdcPlus  = NULL;
HDC hdcMinus = NULL;


int nLineHt = 13;



/*
 *  Function Prototypes
 */


extern int PaneCaretNum( PPANE p);


LRESULT CreatePaneWindow( HWND hWnd, WPARAM wParam, LPARAM lParam);
void DrawPaneButton(HWND hWnd, PPANE p, LPDRAWITEMSTRUCT lpDis );
void DrawPaneFormat(HWND hWnd, PPANE p, LPDRAWITEMSTRUCT lpDis, UCHAR * pBuff, UCHAR * pFmt);
void DrawPaneSelection(HWND hWnd, PPANE p, LPDRAWITEMSTRUCT lpDis, UCHAR * pBuff);
void PaintPane(HWND hWnd);
void PaintSizer(HWND hWnd);
void ResetSplitter(HWND hWndCntl, LPARAM lParam);
void SetPaneFont( HWND hWnd, PPANE p, LPLOGFONT LogFont );
void SizePanels( PPANE p, int cx, int cy);
void CheckHorizontalScroll (PPANE p);

LRESULT FAR PASCAL LOADDS PaneButtonWndProc(HWND,UINT,WPARAM,LPARAM);
LRESULT FAR PASCAL LOADDS PaneLeftWndProc(HWND,UINT,WPARAM,LPARAM);
LRESULT FAR PASCAL LOADDS PaneRightWndProc(HWND,UINT,WPARAM,LPARAM);
LRESULT FAR PASCAL LOADDS PaneSizerWndProc(HWND,UINT,WPARAM,LPARAM);





/***  MDIPaneWndProc
**
**  Entry:
**      Standard WNDPROC (MDI flavor)
**
**  Returns:
**      Standard WNDPROC
**
**  Description:
**      Base WNDPROC for the Pane Manager controlled windows.
*/

LRESULT
CALLBACK
MDIPaneWndProc(
               HWND hWnd,
               UINT message,
               WPARAM wParam,
               LPARAM lParam
               )
{

    PPANE p = (PPANE)GetWindowLongPtr(hWnd, GWW_EDIT );
    LPCHOOSEFONT Cf;
    LRESULT lRet;



    CheckMenu();

    __try {

        switch (message) {

        case WM_CREATE:
            lRet = CreatePaneWindow( hWnd, wParam, lParam);
            return(lRet);

        case WM_MDIACTIVATE:

            //
            // Activating a Window
            //

            if (hWnd == (HWND) lParam) {

                hwndActive     = hWnd;
                hwndActiveEdit = hWnd;
                curView = p->iView;
                EnableToolbarControls();
            }

            //
            //  Deactivating A Window
            //

            else {
                hwndActive = NULL;
                hwndActiveEdit = NULL;
                curView = -1;
            }
            break;

        case WM_DESTROY:
            (*p->fnEditProc)(hWnd, message, 0, (LPARAM)p);
            DeleteWindowMenuItem (p->iView);
            Views[p->iView].Doc = -1; /* Clear the view out */
            goto CallClient;

        case WM_SIZE:

            SizePanels( p, LOWORD(lParam), HIWORD(lParam) );

            // Button pane may have changed its top index, so
            // make sure that we resync

            SyncPanes(p,(WORD)-1);

            WindowTitle( p->iView, 0 );

            // No Break Intended


        case WM_PAINT:

            if ( !IsIconic(hWnd) ) {
                PaintSizer(p->hWndSizer);
                PaintPane(hWnd);
            }
            goto CallClient;


        case WM_MEASUREITEM:
            ((LPMEASUREITEMSTRUCT)lParam)->itemHeight = p->LineHeight;
            return(TRUE);

        case WM_DRAWITEM:
            DrawPaneItem( hWnd, p, (LPDRAWITEMSTRUCT)lParam );
            return(TRUE);


        case WM_SETCURSOR:

            if ( (HWND)wParam == p->hWndSizer) {
                SetCursor(LoadCursor(NULL,IDC_SIZEWE));
                return(TRUE);
            }
            goto CallClient;

        case WM_SETFONT:
            Cf = (LPCHOOSEFONT)lParam;
            SetPaneFont( hWnd, p, Cf->lpLogFont);
            InvalidateRect(hWnd, NULL, TRUE);
            return(TRUE);

        case WU_CLR_BACKCHANGE:
            DeleteObject((HGDIOBJ)p->hbrBackground);
            p->hbrBackground = CreateSolidBrush(StringColors[p->Type].BkgndColor);
            return (TRUE);

        case WM_CTLCOLORLISTBOX:
            return ((LRESULT)(p->hbrBackground));

        case WM_CLOSE:

            (*p->fnEditProc)(hWnd, WM_CLOSE, 0, (LPARAM)p);
            goto CallClient;


        case WM_SETFOCUS:
            curView = p->iView;         // Make sure the doc's know we have it

            if ( p->hWndFocus == NULL) {
                p->hWndFocus = p->hWndLeft;
                p->nCtrlId   = ID_PANE_LEFT;
            } else if ((p->hWndFocus == p->hWndLeft)) {
                p->nCtrlId   = ID_PANE_LEFT;
            } else if ((p->hWndFocus == p->hWndLeft)) {
                p->nCtrlId   = ID_PANE_RIGHT;
            } else if (p->hWndFocus == p->hWndButton) {
                p->nCtrlId   = ID_PANE_BUTTON;
            }

            SetFocus(p->hWndFocus);

            return(FALSE);

        case WM_KILLFOCUS:

            HideCaret(p->hWndFocus);
            DestroyCaret();
            return FALSE;

        case WM_MOUSEWHEEL:
            {
                SHORT snWheel = ((short) HIWORD(wParam)) / WHEEL_DELTA;

                if (snWheel) {
                    // Scroll up
                    if (p->CurIdx != 0xFFFF) {
                        PaneSetIdx(p,(SHORT)(p->CurIdx-snWheel));
                        PaneCaretNum(p);
                    }
                } else {
                    // Scroll down
                    if (p->CurIdx < p->MaxIdx) {
                        PaneSetIdx(p,(SHORT)(p->CurIdx-snWheel));
                        PaneCaretNum(p);
                    }
                }
            }
            return FALSE;

        case WM_VSCROLL:

            switch (LOWORD(wParam)) {
            case SB_LINEDOWN:
                if (p->CurIdx < p->MaxIdx) {
                    PaneSetIdx(p,(SHORT)(p->CurIdx+1));
                    PaneCaretNum(p);
                }
                break;

            case SB_LINEUP:
                if (p->CurIdx != 0xFFFF) {
                    PaneSetIdx(p,(SHORT)(p->CurIdx-1));
                    PaneCaretNum(p);
                }
                break;

            case SB_PAGEDOWN:
                PaneSetIdx(p,(SHORT)(p->CurIdx+PAGE));
                PaneCaretNum(p);
                break;

            case SB_PAGEUP:
                PaneSetIdx(p,(SHORT)(p->CurIdx-PAGE));
                PaneCaretNum(p);
                break;

            case SB_THUMBPOSITION:
            case SB_THUMBTRACK:
                PaneSetIdx(p,(SHORT)((int)HIWORD(wParam)));
                PaneCaretNum(p);
                break;

            }

            return(FALSE);


            case WM_COMMAND:
                {
                    static  BOOL    bOldImeStatus;
                    TCHAR   szClass[64];
                    WORD    wNotice1 = 0;
                    WORD    wNotice2 = 0;

                    if (0 < GetClassName(
                        GET_WM_COMMAND_HWND(wParam, lParam),
                        szClass, sizeof(szClass)-1)) {
                        if (lstrcmpi(szClass, TEXT("ListBox")) == 0) {
                            wNotice1 = LBN_SETFOCUS;
                            wNotice2 = LBN_KILLFOCUS;
                        } else if (lstrcmpi(szClass, TEXT("Edit")) == 0) {
                            wNotice1 = EN_SETFOCUS;
                            wNotice2 = EN_KILLFOCUS;
                        } else if (lstrcmpi(szClass, TEXT("Combobox")) == 0) {
                            wNotice1 = CBN_SETFOCUS;
                            wNotice2 = CBN_KILLFOCUS;
                        } else {
                            szClass[0] = TEXT('\0');
                        }
                        if (szClass[0]) {
                            if (GET_WM_COMMAND_CMD(wParam, lParam) == wNotice1) {
                                ImeSendVkey(
                                    GET_WM_COMMAND_HWND(wParam, lParam),
                                    VK_DBE_FLUSHSTRING);
                                bOldImeStatus = ImeWINNLSEnableIME(NULL, FALSE);
                            } else
                                if (GET_WM_COMMAND_CMD(wParam, lParam) == wNotice2) {
                                    ImeWINNLSEnableIME(NULL, bOldImeStatus);
                                }
                        }
                    }
                }
                goto CallClient;

            case WM_COPY:
            case WM_PASTE:
                PaneKeyboardHandler(p->hWndFocus, message, wParam, lParam);
                return(TRUE);

            case WU_INVALIDATE:

                (*p->fnEditProc)(hWnd, message, wParam, (LPARAM)p);

                CheckHorizontalScroll (p);
                return(FALSE);

            case WU_UPDATE:

                (*p->fnEditProc)(hWnd, message, wParam, (LPARAM)p);

                if ( p->ScrollBarUp ) {
                    SetScrollRange(p->hWndScroll, SB_CTL, 0, p->MaxIdx - 1, FALSE);
                    SetScrollPos( p->hWndScroll, SB_CTL, (INT)p->CurIdx, FALSE);
                }
                ShowScrollBar( p->hWndScroll, SB_CTL, p->ScrollBarUp);

                if (!p->ScrollBarUp) {
                    p->TopIdx = 0;   //reset top if no scrolling
                }

                SyncPanes(p,(WORD)-1);

                CheckHorizontalScroll (p);
                return(FALSE);

            case WU_DBG_LOADEM:
            case WU_DBG_LOADEE:
            case WU_DBG_UNLOADEM:
            case WU_DBG_UNLOADEE:
                (*p->fnEditProc)(hWnd, message, wParam, (LPARAM)p);
                return(FALSE);

            default:
CallClient:

                // Call MDI client for default actions.
                return DefMDIChildProc(hWnd, message, wParam, lParam);
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {

        return FALSE;

    }

    return FALSE;
}                   /* MDIPaneWndProc() */

/***  DlgPaneWndProc
**
**  Entry:
**      Standard WNDPROC (DLG flavor)
**
**  Returns:
**      Standard WNDPROC
**
**  Description:
**      Base WNDPROC for the Pane Manager controlled windows when called from a
**      dialog box
*/

LRESULT
CALLBACK
DLGPaneWndProc(
               HWND hWnd,
               UINT message,
               WPARAM wParam,
               LPARAM lParam
               )
{
    PPANE p = (PPANE)GetWindowLongPtr(hWnd, GWW_EDIT );

    CheckMenu();

    __try {

        switch (message) {

        case WM_CREATE:
            return( CreatePane( hWnd, 0, QUICKW_WIN) );
            break;


        case WM_DESTROY:

            (*p->fnEditProc)(hWnd, message, 0, (LPARAM)p);
            goto CallClient;

        case WM_SIZE:

            SizePanels( p, LOWORD(lParam), HIWORD(lParam) );
            SyncPanes(p,(WORD)-1);

            // No Break Intended


        case WM_PAINT:

            if ( !IsIconic(hWnd) ) {
                PaintSizer(p->hWndSizer);
                PaintPane(hWnd);
            }
            goto CallClient;


        case WM_MEASUREITEM:
            ((LPMEASUREITEMSTRUCT)lParam)->itemHeight = p->LineHeight;
            return(TRUE);

        case WM_DRAWITEM:
            DrawPaneItem( hWnd, p, (LPDRAWITEMSTRUCT)lParam );
            return(TRUE);


        case WM_SETCURSOR:

            if ( (HWND)wParam == p->hWndSizer) {
                SetCursor(LoadCursor(NULL,IDC_SIZEWE));
                return(TRUE);
            }
            goto CallClient;

        case WM_SETFONT:
            SetPaneFont(hWnd, p, &g_logfontDefault);
            return(TRUE);

        case WM_CLOSE:
            (*p->fnEditProc)(hWnd, WM_CLOSE, 0, (LPARAM)p);
            goto CallClient;


        case WM_SETFOCUS:

            if ((p->hWndFocus != p->hWndLeft) &&
                (p->hWndFocus != p->hWndRight) &&
                (p->hWndFocus != p->hWndButton))
            {
                p->hWndFocus = p->hWndButton;
                p->nCtrlId   = ID_PANE_BUTTON;
            }
            CreateCaret( p->hWndFocus, 0, 3, p->LineHeight);
            PaneSwitchFocus(p, NULL, FALSE);
            ShowCaret (p->hWndFocus);
            break;


        case WM_KILLFOCUS:
            HideCaret(hWnd);
            DestroyCaret();
            break;

        case WM_MOUSEWHEEL:
            {
                SHORT snWheel = ((short) HIWORD(wParam)) / WHEEL_DELTA;

                if (snWheel) {
                    // Scroll up
                    if (p->CurIdx >= 0) {
                        PaneSetIdx(p,(SHORT)(p->CurIdx - snWheel));
                        PaneCaretNum(p);
                    }
                } else {
                    // Scroll down
                    if (p->CurIdx < p->MaxIdx) {
                        PaneSetIdx(p,(SHORT)(p->CurIdx - snWheel));
                        PaneCaretNum(p);
                    }
                }
            }
            return FALSE;

        case WM_VSCROLL:
            switch (LOWORD(wParam)) {

            case SB_LINEDOWN:
                if (p->CurIdx < p->MaxIdx) {
                    PaneSetIdx(p,(SHORT)(p->CurIdx+1));
                    PaneCaretNum(p);
                }
                break;

            case SB_LINEUP:
                if (p->CurIdx >= 0) {
                    PaneSetIdx(p,(SHORT)(p->CurIdx-1));
                    PaneCaretNum(p);
                }
                break;

            case SB_PAGEDOWN:
                PaneSetIdx(p,(SHORT)(p->CurIdx+PAGE));
                PaneCaretNum(p);
                break;

            case SB_PAGEUP:
                PaneSetIdx(p,(SHORT)(p->CurIdx-PAGE));
                PaneCaretNum(p);
                break;

            case SB_THUMBPOSITION:
            case SB_THUMBTRACK:
                PaneSetIdx(p,(SHORT)((int)HIWORD(wParam)));
                PaneCaretNum(p);
                break;


            }
            return(FALSE);


            case WM_COMMAND:
                {
                    static  BOOL    bOldImeStatus;
                    TCHAR   szClass[64];
                    WORD    wNotice1 = 0;
                    WORD    wNotice2 = 0;

                    if (0 < GetClassName(
                        GET_WM_COMMAND_HWND(wParam, lParam),
                        szClass, sizeof(szClass)-1)) {
                        if (lstrcmpi(szClass, TEXT("ListBox")) == 0) {
                            wNotice1 = LBN_SETFOCUS;
                            wNotice2 = LBN_KILLFOCUS;
                        } else if (lstrcmpi(szClass, TEXT("Edit")) == 0) {
                            wNotice1 = EN_SETFOCUS;
                            wNotice2 = EN_KILLFOCUS;
                        } else if (lstrcmpi(szClass, TEXT("Combobox")) == 0) {
                            wNotice1 = CBN_SETFOCUS;
                            wNotice2 = CBN_KILLFOCUS;
                        } else {
                            szClass[0] = '\0';
                        }
                        if (szClass[0]) {
                            if (GET_WM_COMMAND_CMD(wParam, lParam) == wNotice1) {
                                ImeSendVkey(
                                    GET_WM_COMMAND_HWND(wParam, lParam),
                                    VK_DBE_FLUSHSTRING);
                                bOldImeStatus = ImeWINNLSEnableIME(NULL, FALSE);
                            } else
                                if (GET_WM_COMMAND_CMD(wParam, lParam) == wNotice2) {
                                    ImeWINNLSEnableIME(NULL, bOldImeStatus);
                                }
                        }
                    }
                }
                goto CallClient;


            case WU_INVALIDATE:

                (*p->fnEditProc)(hWnd, message, wParam, (LPARAM)p);

                CheckHorizontalScroll (p);
                return(FALSE);

            case WU_UPDATE:
                (*p->fnEditProc)(hWnd, message, wParam, (LPARAM)p);

                if ( p->ScrollBarUp ) {
                    SetScrollRange(p->hWndScroll, SB_CTL, 0, p->MaxIdx - 1, FALSE);
                    SetScrollPos( p->hWndScroll, SB_CTL, (INT)p->CurIdx, FALSE);
                }
                ShowScrollBar( p->hWndScroll, SB_CTL, p->ScrollBarUp);

                if (!p->ScrollBarUp) {
                    p->TopIdx = 0;   //reset top if no scrolling
                }

                SyncPanes(p,(WORD)-1);

                return(FALSE);

            case WU_DBG_LOADEM:
            case WU_DBG_LOADEE:
            case WU_DBG_UNLOADEM:
            case WU_DBG_UNLOADEE:

                (*p->fnEditProc)(hWnd, message, wParam, (LPARAM)p);
                return(FALSE);

            default:
CallClient:

                // Call the default dialog proc
                return ( DefWindowProc(hWnd, message, wParam, lParam) );
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {

        return FALSE;

    }

    return FALSE;
}                   /* DLGPaneWndProc() */


/***  PaneSizerWndProc
**
**  Entry:
**      Standard Window Proc
**
**  Returns:
**      Standard Window Proc
**
**  Description:
**      Windows Proc for the Sizer Bar, Allows us to intercept a button
**      down message and handle moving the bar.
**
*/


LRESULT
CALLBACK
PaneSizerWndProc(
                 HWND hWnd,
                 UINT message,
                 WPARAM wParam,
                 LPARAM lParam
                 )
{

    CheckMenu();

    __try {

        switch (message) {

        case WM_LBUTTONDOWN:
            ResetSplitter( hWnd, lParam );
            return(FALSE);

        default:

            DAssert(lpfnSizerEditProc);
            return(CallWindowProc(lpfnSizerEditProc,hWnd,message,wParam,lParam));
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {

        return 0;

    }
}



/***  PaneButtonWndProc
**
**  Synopsis:
**      LRESULT PaneButtonWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
**
**  Entry:
**      Standard Window Proc
**
**  Returns:
**      Standard Window Prod
**
**  Description:
**      The subclassed window proc for the button pane.  Allow us to interscept
**      the keyboard.
**
*/


LRESULT
CALLBACK
PASCAL
EXPORT
PaneButtonWndProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{

    PPANE p = (PPANE)GetWindowLongPtr(GetParent(hWnd), GWW_EDIT );



    CheckMenu();

    __try {

        switch (message) {


        case WM_SETFOCUS:
            CreateCaret(hWnd, 0, 3, p->LineHeight);
            HideCaret(hWnd);
            return FALSE;

        case WM_KILLFOCUS:
            DestroyCaret();
            return FALSE;

        case WM_KEYDOWN:
        case WM_CHAR:
            if (wParam == VK_SHIFT) {
                return FALSE;
            }

            PaneKeyboardHandler(hWnd, message, wParam, lParam);
            return FALSE;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
            PaneKeyboardHandler(hWnd, message, wParam, lParam);
            return FALSE;

        case WM_IME_REPORT:
            if (IR_STRING == wParam) {
                return TRUE;
            }
            DAssert(lpfnButtonEditProc);
            return(lpfnButtonEditProc(hWnd,message,wParam,lParam));

        default:
            DAssert(lpfnButtonEditProc);
            return(lpfnButtonEditProc(hWnd,message,wParam,lParam));
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {

        return FALSE;

    }
}



/***  PaneLeftWndProc
**
**  Synopsis:
**      LRESULT PASCAL PaneLeftWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
**
**  Entry:
**      Standard WNDPROC
**
**  Returns:
**      Standard WNDPROC
**
**  Description:
**      The WNDPROC for the Left Pane control.  We intercept deletes and
**      backspaces and don't allow them to cross line bountries.  We use
**      tab to switch between panes.  We Also monitor what the top line
**      that is visible so that we can keep all the panes in sync.
**
*/


LRESULT
CALLBACK
PASCAL
EXPORT
PaneLeftWndProc(
                HWND hWnd,
                UINT message,
                WPARAM wParam,
                LPARAM lParam
                )
{

    PPANE p = (PPANE)GetWindowLongPtr(GetParent(hWnd), GWW_EDIT );
    POINT pnt;



    CheckMenu();

    __try {

        switch (message) {


        case WM_SETFOCUS:
            CreateCaret(hWnd, 0, 3, p->LineHeight);
            SetCaretPos (p->X, p->Y);
            ShowCaret (hWnd);
            return FALSE;
            break;


        case WM_KILLFOCUS:
            GetCaretPos(&pnt);
            p->X = pnt.x;
            p->Y = pnt.y;
            HideCaret(hWnd);
            DestroyCaret();
            return FALSE;
            break;



        case WM_KEYDOWN:
        case WM_CHAR:
            if (wParam == VK_SHIFT) {
                return FALSE;
            }
            PaneKeyboardHandler(hWnd, message, wParam, lParam);
            return FALSE;


        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
            PaneKeyboardHandler(hWnd, message, wParam, lParam);
            CheckMenu();
            return FALSE;

        case WM_IME_REPORT:
            if (IR_STRING == wParam) {
                return TRUE;
            }
            DAssert(lpfnEditEditProc);
            return(lpfnEditEditProc(hWnd,message,wParam,lParam));
            break;

        default:
            DAssert(lpfnEditEditProc);
            return(lpfnEditEditProc(hWnd,message,wParam,lParam));
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        ;
    }

    CheckMenu();
    return(FALSE);
}

/***  PaneRightWndProc
**
**  Synopsis:
**      LRESULT PASCAL PaneRightWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
**
**  Entry:
**      Standard WNDPROC
**
**  Returns:
**      Standard WNDPROC
**
**  Description:
**      The WNDPROC for the Right Pane.  We intercept deletes and
**      backspaces and don't allow them to cross line bountries.  We use
**      tab to switch between panes.  We Also monitor what the top line
**      that is visible so that we can keep all the panes in sync.
**
**
*/


LRESULT
CALLBACK
PASCAL
EXPORT
PaneRightWndProc(
                 HWND hWnd,
                 UINT message,
                 WPARAM wParam,
                 LPARAM lParam
                 )
{

    PPANE p = (PPANE)GetWindowLongPtr(GetParent(hWnd), GWW_EDIT );
    POINT pnt;

    CheckMenu();

    __try {

        switch (message) {

        case WM_SETFOCUS:
            CreateCaret(hWnd, 0, 3, p->LineHeight);
            SetCaretPos (p->X, p->Y);
            ShowCaret (hWnd);
            return FALSE;


        case WM_KILLFOCUS:
            GetCaretPos(&pnt);
            p->X = pnt.x;
            p->Y = pnt.y;
            HideCaret(hWnd);
            DestroyCaret();
            return FALSE;


        case WM_KEYDOWN:
        case WM_CHAR:
            if (wParam == VK_SHIFT) {
                return FALSE;
            }
            PaneKeyboardHandler(hWnd, message, wParam, lParam);
            return FALSE;


        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
            PaneKeyboardHandler(hWnd, message, wParam, lParam);
            CheckMenu();
            return FALSE;

        case WM_IME_REPORT:
            if (IR_STRING == wParam) {
                return TRUE;
            }
            DAssert(lpfnEditEditProc);
            return(lpfnEditEditProc(hWnd,message,wParam,lParam));
            break;

        default:
            DAssert(lpfnEditEditProc);
            return(lpfnEditEditProc(hWnd,message,wParam,lParam));
        }

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        ;
    }

    CheckMenu();
    return(FALSE);
}
/***    OpenPanedWindow
**
**  Entry:
**      type    -  The Type of Window to Open (WATCH_WIN, LOCALS_WIN...)
**      bUserActivated - Indicates whether this action was initiated by the
**                user or by windbg. The value is to determine the Z order of
**                any windows that are opened.
**
**  Returns:
**      None
**
**  Description:
**      Creates (or makes visible the type of window needed.
*/


void
OpenPanedWindow(
    int type,
    LPWININFO lpWinInfo,
    int Preference,
    BOOL bUserActivated
    )
{
    WORD  classId;
    WORD  winTitle;
    HWND  hWnd;
    int   view;

    MDICREATESTRUCT mcs;
    char  szClass[MAX_MSG_TXT];
    char  title[MAX_MSG_TXT];
    char  final[MAX_MSG_TXT+4];


    ZeroMemory(&mcs, sizeof(mcs));

    //  Figure out the details of what we're to do

    switch ( type ) {
    case WATCH_WIN:
#if defined( NEW_WINDOWING_CODE )
        classId  = SYS_NewWatch_wClass;
#else
        classId  = SYS_Watch_wClass;
#endif
        winTitle = SYS_WatchWin_Title;
        hWnd     = GetWatchHWND();
        break;

    case LOCALS_WIN:
#if defined( NEW_WINDOWING_CODE )
        classId  = SYS_NewLocals_wClass;
#else
        classId  = SYS_Locals_wClass;
#endif
        winTitle = SYS_LocalsWin_Title;
        hWnd     = GetLocalHWND();
        break;

    case CPU_WIN:
#if defined( NEW_WINDOWING_CODE )
        classId  = SYS_NewCpu_wClass;
#else
        classId  = SYS_Cpu_wClass;
#endif
        winTitle = SYS_CpuWin_Title;
        hWnd     = GetCpuHWND();
        break;

    case FLOAT_WIN:
#if defined( NEW_WINDOWING_CODE )
        classId  = SYS_NewFloat_wClass;
#else
        classId  = SYS_Float_wClass;
#endif
        winTitle = SYS_FloatWin_Title;
        hWnd     = GetFloatHWND();
        break;

    default:
        DAssert(FALSE);
        return;
    }

    //  If we don't have one yet, create it

    if ( hWnd == NULL) {

    /*
    *  Determine which view index we are going to use
    */

        if ( (Preference != -1) && (Views[Preference].Doc == -1) ) {
            view = Preference;
        }

        else {
            for (view=0; (view < MAX_VIEWS) && (Views[view].Doc != -1); view++);
        }

        if (view == MAX_VIEWS) {
            ErrorBox(ERR_Too_Many_Opened_Views);
            return;
        }

        // Get the Window Title and Window Class

        Dbg(LoadString(g_hInst, classId,  szClass, MAX_MSG_TXT));
        Dbg(LoadString(g_hInst, winTitle, title, MAX_MSG_TXT));
        RemoveMnemonic(title,title);
        sprintf(final,"%s", title);

        // Make sure the Menu gets setup
        AddWindowMenuItem(-type, view);

        // Have MDI Client create the Child

        mcs.szTitle = final;
        mcs.szClass = szClass;
        mcs.hOwner  = g_hInst;
        if (lpWinInfo) {
            mcs.x = lpWinInfo->coord.left;
            mcs.y = lpWinInfo->coord.top;
            mcs.cx = lpWinInfo->coord.right - lpWinInfo->coord.left;
            mcs.cy = lpWinInfo->coord.bottom - lpWinInfo->coord.top;
            mcs.style = lpWinInfo->style;
        } else {
            mcs.x       = mcs.cx = CW_USEDEFAULT;
            mcs.y       = mcs.cy = CW_USEDEFAULT;
            mcs.style   = 0L;
        }

        mcs.style |= WS_VISIBLE;

        if (hwndActive && ( IsZoomed(hwndActive) || IsZoomed(GetParent(hwndActive)) ) ) {
            mcs.style |= WS_MAXIMIZE;
        }

        mcs.lParam  = (ULONG) (type | (view << 16));

        Views[view].hwndClient = g_hwndMDIClient;
        Views[view].NextView = -1;  /* No next view */
        Views[view].Doc = -type;

        hWnd = (HWND)SendMessage(g_hwndMDIClient, WM_MDICREATE, 0, (LPARAM) &mcs);

        SetWindowWord(hWnd, GWW_VIEW, (WORD)view);
        Views[view].hwndFrame = hWnd;
    } else {

        //  Child already exists, reactivate the previous one.

        if (IsIconic(hWnd)) {
            OpenIcon(hWnd);
        }

        //SendMessage(g_hwndMDIClient, WM_MDIACTIVATE, (WPARAM)hWnd, (ULONG)type);
        ActivateMDIChild(hWnd, bUserActivated);
    }
}


/***  PaintPane
**
**  Synopsis:
**      PaintPane( HWND hWnd);
**
**  Entry:
**      hWnd - Handle to the base window of the pane
**
**  Returns:
**      None
**
**  Description:
**      Paints the accents for the base window to give it a 3D look
**
*/


void
PaintPane(
          HWND hWnd
          )
{
    PAINTSTRUCT ps;
    HPEN blackPen, whitePen;

    BeginPaint (hWnd, &ps);

    //Prepare the pens
    Dbg(whitePen = (HPEN) GetStockObject(WHITE_PEN));
    Dbg(blackPen = (HPEN) GetStockObject(BLACK_PEN));

    //Draw a top white line
    Dbg(SelectObject(ps.hdc, whitePen));
    MoveToX(ps.hdc, ps.rcPaint.left, 0, NULL);
    LineTo(ps.hdc, ps.rcPaint.right, 0);

    EndPaint (hWnd, &ps);
}

/***  PaintSizer
**
**  Synopsis:
**      PaintSizer( HWND hWnd );
**
**  Entry:
**      hWnd    - Handle to the sizer window.
**
**  Returns:
**      None
**
**  Description:
**      Paints the accents on the sizer control to give it a 3D look.
*/


void
PaintSizer(
           HWND hWnd
           )
{
    PAINTSTRUCT ps;
    HPEN blackPen, grayPen;
    int cx;

    BeginPaint (hWnd, &ps);
    cx = ps.rcPaint.right - ps.rcPaint.left - 1;

    //Prepare the pens
    Dbg(blackPen = (HPEN) GetStockObject(BLACK_PEN));
    Dbg(grayPen  = CreatePen(PS_SOLID, cx, GRAYDARK));

    //  Background to gray
    Dbg(SelectObject(ps.hdc, grayPen));
    MoveToX(ps.hdc, ps.rcPaint.left + (cx/2),ps.rcPaint.top, NULL);
    LineTo(ps.hdc, ps.rcPaint.left + (cx/2), ps.rcPaint.bottom);

    //  Left side to white
    Dbg(SelectObject(ps.hdc, blackPen));
    MoveToX(ps.hdc, ps.rcPaint.left, ps.rcPaint.top, NULL);
    LineTo(ps.hdc, ps.rcPaint.left, ps.rcPaint.bottom);

    //  Right side to black
    Dbg(SelectObject(ps.hdc, blackPen));
    MoveToX(ps.hdc, ps.rcPaint.right, ps.rcPaint.top, NULL);
    LineTo(ps.hdc, ps.rcPaint.right, ps.rcPaint.bottom);

    // Lose the Pen and release the paint
    Dbg(DeleteObject (grayPen));
    EndPaint (hWnd, &ps);

}


/***  SyncPanes
**
**  Synopsis:
**      SyncPanes(PPANE p, WORD Index);
**
**  Entry:
**      p      - Pointer to the Pane structure.
**      Index  - Index to Sync to
**
**  Returns:
**      None
**
**  Description:
**      Checks each of the pane controls (Button Pane, Left Pane, Right Pane)
**      makes sure that the index of the top line is the same.
*/

void
SyncPanes(
          PPANE p,
          WORD NewTop
          )
{
    if ( NewTop != (WORD)-1) {
        p->TopIdx = NewTop;
    }

    if ( p->ScrollBarUp) {
        SetScrollRange(p->hWndScroll, SB_CTL, 0, (p->MaxIdx - 1), FALSE);
        SetScrollPos(p->hWndScroll, SB_CTL, p->CurIdx, TRUE);
    }


    SendMessage(p->hWndButton, LB_SETTOPINDEX, (WPARAM)p->TopIdx,0);
    SendMessage(p->hWndLeft  , LB_SETTOPINDEX, (WPARAM)p->TopIdx,0);
    SendMessage(p->hWndRight , LB_SETTOPINDEX, (WPARAM)p->TopIdx,0);

}


/***  SizePanels
**
**  Synopsis:
**      SizePanels(PPANE p, int cx, int cy);
**
**  Entry:
**      p  - Pointer to the Pane structure.
**      cx - The New width  of the base window
**      cy - The New height of the base window
**
**  Returns:
**      None
**
**  Description:
**      Sizes the panes in the window to fit the new size of the base.
*/

void
SizePanels(
           PPANE p,
           int cx,
           int cy
           )
{
    int bar_width;
    int bar_height;
    int height, left_pane, right_pane, left_start, right_start;


    //
    //  Calculate where things should go
    //

    bar_width   = GetSystemMetrics(SM_CXVSCROLL);
    bar_height  = GetSystemMetrics(SM_CXHSCROLL);
    height      = cy - TOP_OFFSET - BOTTOM_OFFSET + 2;
    left_start  = LEFT_OFFSET + BUTTON_SIZE + LEFT_OFFSET + 1;
    left_pane   = ((cx - TOTAL_WIDTH ) * p->PanePerCent ) / 1000;
    right_start = left_start + left_pane + SIZER_HANDLE + 1;
    right_pane  = cx - TOTAL_WIDTH - left_pane;

    p->PaneLines   = (height - bar_height)/p->LineHeight;
    p->ScrollBarUp = p->MaxIdx > p->PaneLines;


    //
    //  Move the Pane components into their new positions
    //

    MoveWindow(p->hWndButton,LEFT_OFFSET,TOP_OFFSET,BUTTON_SIZE,height - bar_height ,TRUE);
    MoveWindow(p->hWndLeft,left_start,TOP_OFFSET,left_pane,height,TRUE);
    MoveWindow(p->hWndSizer,left_start+left_pane,TOP_OFFSET,SIZER_HANDLE,height,TRUE);

    //  Do Scroll bar first, it may adjust the right pane size


    if ( p->ScrollBarUp ) {
        right_pane -= bar_width;

        SetScrollRange(p->hWndScroll, SB_CTL, 0, p->MaxIdx - 1, FALSE);

        SetScrollPos( p->hWndScroll, SB_CTL, (INT)p->CurIdx, TRUE);

        MoveWindow(p->hWndScroll, cx - bar_width - RIGHT_OFFSET + 2, TOP_OFFSET,
            bar_width, height - bar_height, TRUE);
    }
    ShowScrollBar( p->hWndScroll, SB_CTL, p->ScrollBarUp);

    if (!p->ScrollBarUp) {
        p->TopIdx = 0;   //reset top if no scrolling
    }
    //  Now do the right pane

    MoveWindow(p->hWndRight,right_start,TOP_OFFSET,right_pane,height,TRUE);

    //InvalidateRect (p->hWndButton,NULL,TRUE);
    //InvalidateRect (p->hWndLeft,NULL,TRUE);
    //InvalidateRect (p->hWndRight,NULL,TRUE);


    UpdateWindow (p->hWndButton);
    UpdateWindow (p->hWndLeft);
    UpdateWindow (p->hWndRight);


    CheckHorizontalScroll (p);
}


void
CheckHorizontalScroll(
                      PPANE p
                      )
{
    int         Max, i, MaxLen, nItems;
    PANEINFO    Info = {0,0,0,0,NULL};
    SIZE        Size = {0, 0};
    HDC         hDC;


    if (p->hWndLeft) {
        Max = 0;
        Info.CtrlId = ID_PANE_LEFT;
        nItems = min(p->PaneLines,p->MaxIdx);
        if (nItems > 0) {
            for (i = 0;i < nItems ;i++) {
                Info.ItemId = i;
                (*p->fnEditProc)(p->hWndLeft,WU_INFO,(WPARAM)&Info,(LPARAM)p);
                if (Info.pBuffer) {
                    MaxLen = strlen (Info.pBuffer);
                    hDC = GetDC(p->hWndLeft);
                    SelectObject(hDC, p->hFont);
                    GetTextExtentPoint(hDC, Info.pBuffer, MaxLen, &Size);
                    ReleaseDC(p->hWndLeft,hDC);
                }
                if (Size.cx > Max) {
                    Max = Size.cx;
                }
            }
            SendMessage(p->hWndLeft, LB_SETHORIZONTALEXTENT, (WPARAM)(Max + p->CharWidth), 0);
        }
    }

    if (p->hWndRight) {
        Max = 0;
        Info.CtrlId = ID_PANE_RIGHT;
        nItems = min(p->PaneLines,p->MaxIdx);
        if (nItems > 0) {
            for (i = 0;i < nItems ;i++) {
                Info.ItemId = i;
                (*p->fnEditProc)(p->hWndRight,WU_INFO,(WPARAM)&Info,(LPARAM)p);
                if (Info.pBuffer != NULL) {
                    MaxLen = strlen (Info.pBuffer);
                    hDC = GetDC(p->hWndRight);
                    SelectObject(hDC, p->hFont);
                    GetTextExtentPoint(hDC, Info.pBuffer, MaxLen, &Size);
                    ReleaseDC(p->hWndRight,hDC);
                }
                if (Size.cx > Max) {
                    Max = Size.cx;
                }
            }
            SendMessage(p->hWndRight, LB_SETHORIZONTALEXTENT, (WPARAM)(Max + p->CharWidth), 0);
        }
    }
}


/***  CreatePaneWindow
**
**  Synopsis:
**      CreatePaneWindow( WPARAM wParam, LPARAM lParam)
**
**  Entry:
**      hWnd    - hWnd from the WM_CREATE message
**      wParam  - wParam from the WM_CREATE message
**      lParam  - lParam from the WM_CREATE message
**
**  Returns:
**      None
**
**  Description:
**      Create the Pane manager information, and windows and
**      in general gets everything setup for operations.
*/

LRESULT
CreatePaneWindow(
    HWND hWnd,
    WPARAM wParam,
    LPARAM lParam
    )
{
    MDICREATESTRUCT FAR *mdi;
    int iView;
    int iType;

    //  Get the MDICREATE struct

    mdi = (MDICREATESTRUCT FAR *)
        (((CREATESTRUCT FAR *)lParam)->lpCreateParams);

    iView = HIWORD(mdi->lParam);
    iType = LOWORD(mdi->lParam);
    return( CreatePane( hWnd, iView, iType) );
}

/***  CreatePane
**
**  Synopsis:
**      CreatePane( HWND hWnd, int iView, int iType)
**
**  Entry:
**      hWnd    - hWnd from the WM_CREATE message
**      iView   - View number for this window (zero if dialog)
**      lType   - Window Type
**
**  Returns:
**      TRUE/FALSE
**
**  Description:
**      Create the Pane manager information, and windows and
**      in general gets everything setup for operations.
*/

LONG_PTR
CreatePane(
    HWND hWnd,
    int iView,
    int iType
    )
{
    PPANE   p;
    HDC     hdc;
    HBITMAP hBitmap;

    // Allocate the Pane data and store it in the class data

    if ( (p = (PPANE) calloc(1,sizeof(PANE))) == NULL) {
        return(FALSE);
    }


    SetWindowLongPtr(hWnd, GWW_EDIT , (LONG_PTR)p);

    p->iView = iView;
    p->Type  = (WORD) iType;

    // Set the Window Specific Details

    switch ( p->Type ) {

    case WATCH_WIN:
        p->ColorItem = WatchWindow;
        p->fnEditProc = WatchEditProc;
        if (Views[iView].Doc < 0)
            Views[iView].hwndFrame = hWnd;
        break;


    case LOCALS_WIN:
        p->ColorItem = LocalsWindow;
        p->fnEditProc = LocalEditProc;
        if (Views[iView].Doc < 0)
            Views[iView].hwndFrame = hWnd;
        break;

    case CPU_WIN:
        p->ColorItem = RegistersWindow;
        p->fnEditProc = CPUEditProc;
        if (Views[iView].Doc < 0)
            Views[iView].hwndFrame = hWnd;
        break;

    case FLOAT_WIN:
        p->ColorItem = FloatingPointWindow;
        p->fnEditProc = CPUEditProc;
        if (Views[iView].Doc < 0)
            Views[iView].hwndFrame = hWnd;
        break;

    case QUICKW_WIN:
        p->ColorItem = WatchWindow;
        p->fnEditProc = QuickEditProc;
        break;

    default:
        DAssert(FALSE);
        return(FALSE);
    }


    // Setup the default info

    p->PanePerCent = 300;                    // Default to 30.0%
    p->ScrollBarUp = FALSE;                  // Default to no-scroll bar
    p->LeftOk      = FALSE;                  // Left Pane needs painting
    p->RightOk     = FALSE;                  // Right Pane needs painting
    p->nXoffLeft = 0;
    p->nXoffRight = 0;
    p->X = 0;
    p->Y = 0;

    //Initialize font information for Pane Windows
    SetPaneFont(hWnd, p, &g_logfontDefault);

    // Initialize background brush for Pane Windows
    p->hbrBackground = CreateSolidBrush(StringColors[iType].BkgndColor);

    // Create Buttons, subclass the editproc
    p->hWndButton = CreateWindow( "ListBox" , "", BUTTON_STYLE,
        0, 0, 0, 0, hWnd,(HMENU)ID_PANE_BUTTON,g_hInst,NULL);
    lpfnButtonEditProc = (WNDPROC)GetWindowLongPtr(p->hWndButton, GWLP_WNDPROC);
    SetWindowLongPtr(p->hWndButton, GWLP_WNDPROC, (LONG_PTR) PaneButtonWndProc);


    // Create Expression Pane and Subclass the edit proc
    p->hWndLeft = CreateWindow("ListBox", "", PANE_STYLE,
        0, 0, 0, 0, hWnd, (HMENU)ID_PANE_LEFT, g_hInst,NULL);
    lpfnEditEditProc = (WNDPROC)GetWindowLongPtr(p->hWndLeft, GWLP_WNDPROC);
    SetWindowLongPtr(p->hWndLeft, GWLP_WNDPROC, (LONG_PTR) PaneLeftWndProc);


    // Create Sizer Handle
    p->hWndSizer = CreateWindow("Edit", "", WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0, hWnd, (HMENU)ID_PANE_SIZER, g_hInst,NULL);
    lpfnSizerEditProc = (WNDPROC)GetWindowLongPtr(p->hWndSizer, GWLP_WNDPROC);
    SetWindowLongPtr(p->hWndSizer, GWLP_WNDPROC, (LONG_PTR) PaneSizerWndProc);


    // Create Results Pane, reference our editproc
    p->hWndRight = CreateWindow("ListBox", "", PANE_STYLE,
        0, 0, 0, 0, hWnd, (HMENU)ID_PANE_RIGHT, g_hInst,NULL);
    SetWindowLongPtr(p->hWndRight, GWLP_WNDPROC, (LONG_PTR) PaneRightWndProc);


    // Create Scroll Bar
    p->hWndScroll = CreateWindow("ScrollBar",NULL, SCROLL_STYLE,
        0, 0, 0, 0, hWnd, (HMENU)ID_PANE_SCROLL, g_hInst, NULL);

    // Once again now that we have the children windows
    SetPaneFont(hWnd, p, &g_logfontDefault);

    // If we don't have the BITMAP hdc's get them
    if ( hdcSpace == NULL) {
        Dbg(hdc      = GetDC(p->hWndButton));

        Dbg(hdcSpace = CreateCompatibleDC(hdc));
        Dbg(hBitmap = LoadBitmap(g_hInst, MAKEINTRESOURCE(VGA_PANE_BLANK)));
        Dbg(SelectObject(hdcSpace, hBitmap));

        Dbg(hdcPlus = CreateCompatibleDC(hdc));
        Dbg(hBitmap = LoadBitmap(g_hInst, MAKEINTRESOURCE(VGA_PANE_PLUS)));
        Dbg(SelectObject(hdcPlus, hBitmap));

        Dbg(hdcMinus = CreateCompatibleDC(hdc));
        Dbg(hBitmap = LoadBitmap(g_hInst, MAKEINTRESOURCE(VGA_PANE_MINUS)));
        Dbg(SelectObject(hdcMinus, hBitmap));

        ReleaseDC(p->hWndButton, hdc);
    }

    // Initilize the Window
    (*p->fnEditProc)(hWnd, WU_INITDEBUGWIN, 0, (LONG_PTR)p);

    PaneSwitchFocus(p, NULL, FALSE);
    return((LONG_PTR)hWnd);
}

/***  SetPaneFont
**
**  Synopsis:
**      void SetPaneFont( HWND hWnd, PPANE p, HFONT hFont);
**
**  Entry:
**      hWnd    - Handle of the Pane Managed Window
**      p       - Pointer to the Pane Manager Info
**      hFont   - Handle to the New Font
**
**  Returns:
**      None
**
**  Description:
**      Sets a new font to be used in the pane manager window.
*/


void
SetPaneFont(
            HWND hWnd,
            PPANE p,
            LPLOGFONT LogFont
            )
{
    HDC  hDC;
    TEXTMETRIC tm;
    int  Max;

    p->hFont = CreateFontIndirect(LogFont);
    Views[p->iView].font = p->hFont;


    hDC   = GetDC(hWnd);
    SelectObject(hDC, p->hFont);
    GetTextMetrics(hDC, &tm);
    GetCharWidth(hDC, 0, MAX_CHARS_IN_FONT - 1, (LPINT)(Views[p->iView].charWidth));
    ReleaseDC(hWnd, hDC);

    p->LineHeight  = (WORD)tm.tmHeight;
    nLineHt        = tm.tmHeight;
    p->CharWidth   = (WORD)tm.tmMaxCharWidth;
    Max            = tm.tmMaxCharWidth * 256;


    if ( p->hWndButton) {
        SendMessage(p->hWndButton,WM_SETFONT,(WPARAM)p->hFont,(LPARAM)FALSE);
        SendMessage(p->hWndButton,LB_SETITEMHEIGHT, 0, MAKELPARAM(p->LineHeight,0));
        SendMessage(p->hWndButton,WM_SETFOCUS,0,0L);

    }

    if ( p->hWndLeft ) {
        SendMessage(p->hWndLeft, WM_SETFONT,(WPARAM)p->hFont,(LPARAM)FALSE);
        SendMessage(p->hWndLeft, LB_SETITEMHEIGHT, 0, MAKELPARAM(p->LineHeight,0));
        SendMessage(p->hWndLeft, LB_SETHORIZONTALEXTENT, (WPARAM)Max, 0);
        SendMessage(p->hWndLeft, WM_SETFOCUS,0,0L);
    }

    if ( p->hWndRight) {
        SendMessage(p->hWndRight, WM_SETFONT,(WPARAM)p->hFont,(LPARAM)FALSE);
        SendMessage(p->hWndRight, LB_SETITEMHEIGHT, 0, MAKELPARAM(p->LineHeight,0));
        SendMessage(p->hWndRight, LB_SETHORIZONTALEXTENT, (WPARAM)Max, 0);
        SendMessage(p->hWndRight, WM_SETFOCUS,0,0L);

    }

    if ( p->hWndLeft)
        SendMessage (p->hWndLeft,WM_SETFOCUS,0,0L);

    return;
}


/***  ResetSplitter
**
**  Synopsis:
**      ResetSplitter( hWndCntrl )
**
**  Entry:
**      hWnd    - hWnd from the WM_LBUTTONDOWN message
**      lParam  - lParam from WM_BUTTONDOWN message
**
**  Returns:
**      None
**
**  Description:
**      Allows the user to move the splitter control.
*/

void
ResetSplitter(
              HWND hWndCntl,
              LPARAM lParam
              )
{
    HWND hWnd = GetParent(hWndCntl);
    PPANE p = (PPANE)GetWindowLongPtr(hWnd, GWW_EDIT );
    RECT rc;
    HDC  hdc;
    MSG msg;
    INT x, y, dx, dy;
    HCURSOR hOldCursor;
    int PerCent;
    BOOL Hit = FALSE;

    hOldCursor = SetCursor(LoadCursor(NULL, IDC_SIZEWE));

    //  Get the Size of splitter bar
    GetClientRect(hWndCntl, &rc);
    x = LOWORD(lParam);
    y = rc.top;
    dx = rc.right - rc.left;
    dy = rc.bottom - rc.top;

    //  Grap the device context and the mouse, then invert a vertical bar
    hdc = GetDC(hWnd);
    SetCapture(hWnd);
    PatBlt(hdc, x - dx / 2, y, dx, dy, PATINVERT);

    //  Track the mouse for a while
    while (GetMessage(&msg, NULL, 0, 0)) {

        if ( msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST ) {

            if (msg.message == WM_LBUTTONUP ||
                msg.message == WM_LBUTTONDOWN)
                break;


            if (msg.message == WM_MOUSEMOVE) {

                // erase old
                PatBlt(hdc, x - dx / 2, y, dx, dy, PATINVERT);
                ScreenToClient(hWnd, &msg.pt);
                x = msg.pt.x;

                // put down new
                PatBlt(hdc, x - dx / 2, y, dx, dy, PATINVERT);
                Hit = TRUE;
            }
        }

        else {
            DispatchMessage(&msg);
        }
    }

    //  Clean-up the track clutter

    ReleaseCapture();
    PatBlt(hdc, x - dx / 2, y, dx, dy, PATINVERT);
    ReleaseDC(hWnd, hdc);
    SetCursor(hOldCursor);

    //  Calculate the new percentage

    GetClientRect(hWnd, &rc);

    //PerCent =  ((x-TOTAL_WIDTH) * 1000)/(WORD)(rc.right-rc.left - TOTAL_WIDTH);
    if ( Hit ) {
        PerCent =  ((x-TOTAL_WIDTH+SIZER_HANDLE+RIGHT_OFFSET) * 1000)/(WORD)(rc.right-rc.left - TOTAL_WIDTH);

        if ( PerCent < 10) {
            p->PanePerCent = 10;
        } else if ( PerCent > 990) {
            p->PanePerCent = 990;
        } else {
            p->PanePerCent = (WORD)PerCent;
        }

        PostMessage(hWnd, WM_SIZE, 0, MAKELONG(rc.right,rc.bottom));
    }
}

/***  CheckPaneScrollBar
**
**  Synopsis:
**      void CheckPaneScrollBar( pInfo, NewCount)
**
**  Entry:
**      PPANE pInfo   - Pointer to the Pane Information
**      WORD NewCount - New Count of Lines in the controls
**
**  Returns:
**      None
**
**  Description:
**      Called when the number of items in the controls changed (or that
**      size of the panes changed).  Determines if we need a scroll bar
**      or not, and then makes it visible or not as needed.
*/

void
CheckPaneScrollBar(
                   PPANE p,
                   WORD Count
                   )
{
    int   nHeight;
    int   nWidth;
    RECT  Rect;

    int   nBar     = GetSystemMetrics(SM_CXHSCROLL);
    BOOL  fNeedBar = ((WORD)Count > p->PaneLines);



    //  Exit if what we need is what we have
    if ( fNeedBar == p->ScrollBarUp) {
        if ( p->PaneLines ) {
            SetScrollRange(p->hWndScroll, SB_CTL, 0, p->MaxIdx - 1, FALSE);
            CheckHorizontalScroll (p);
        }
        return;
    }

    //  Get size of Right Pane
    GetClientRect( GetParent(p->hWndRight), &Rect);
    nHeight  = Rect.bottom - Rect.top;
    nWidth   = Rect.right - Rect.left;


    //  Get Current Size Pane Windows Client Area and resize the panels.

    GetClientRect( GetParent(p->hWndRight), &Rect);
    nHeight  = Rect.bottom - Rect.top;
    nWidth   = Rect.right - Rect.left;
    SizePanels( p, nWidth, nHeight);
    CheckHorizontalScroll (p);
    return;
}


/***  ScrollPanes
**
**  Synopsis:
**      void ScrollPanes( PPANE pPane, WPARAM wParam, LPARAM lParam);
**
**  Entry:
**      PPANE  pPane  - Pointer to the pane information
**      WPARAM wParam - The wParam from the WM_VSCROLL
**      LPARAM lPAram - The lParam from the WM_VSCROLL
**
**  Returns:
**      None
**
**  Description:
**      Services the WM_VSCROLL message.  Determines the type of scroll
**      and then does it.
*/

void
ScrollPanes(
            PPANE p,
            WPARAM wParam,
            LPARAM lParam
            )
{
    int  nScroll = (int) LOWORD(wParam);
    int  nPos    = (int) HIWORD(wParam);
    int  nNewPos = (int)p->TopIdx;

    Unreferenced(lParam);

    switch ( nScroll ) {
    case SB_LINEDOWN:
        nNewPos++;
        break;

    case SB_LINEUP:
        nNewPos--;
        break;

    case SB_PAGEDOWN:
        nNewPos += (p->PaneLines-1);
        break;

    case SB_PAGEUP:
        nNewPos -= (p->PaneLines-1);
        break;

    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
        nNewPos = (WORD)nPos;
        break;

    case SB_TOP:
        nNewPos = 0;
        break;

    case SB_BOTTOM:
        nNewPos =  p->MaxIdx-1;
        break;
    }

    //  Range check the result

    if ( nNewPos < 0 ) {
        nNewPos = 0;
    }

    else if ( nNewPos > p->MaxIdx-1) {
        nNewPos = p->MaxIdx-1;
    }



    //  And Sync the Pane

    SetLineColumn_StatusBar(p->CurIdx+1, 1);
    SyncPanes(p, (WORD)nNewPos);
    return;
}



/***  GetPaneStatus
**
**  Synopsis:
**      PLONG GetPaneStatus( ViewNo );
**
**  Entry:
**      int ViewNo  - The View number we need status for
**
**  Returns:
**      Returns a pointer to status area.
**
**  Description:
**      Returns a buffer with status information for a given view number.
**      The first long in the buffer is the length of the buffer (including
**      this field).  When you are done with the status area use
**      FreePaneStatus() to clean up.
*/

PLONG
GetPaneStatus(
    int nView
    )
{
    int   length;
    HWND  hWnd     = Views[nView].hwndFrame;
    PPANE p        = (PPANE)GetWindowLongPtr(hWnd, GWW_EDIT );
    PINFO pStatus  = NULL;
    char  *pWatchs = NULL;

    DAssert( p );
    DAssert( p->Type == (WORD)(-Views[nView].Doc) );

    pStatus = (PINFO) calloc(1, sizeof(INFO));

    if ( pStatus ) {

        pStatus->Length  = sizeof(INFO);
        pStatus->PerCent = p->PanePerCent;
        pStatus->Flags   = p->bFlags;
        pStatus->Text[0] = 0;

        if ( p->Type == WATCH_WIN) {
            pWatchs = FTGetWatchList( GetWatchVit() );

            if (pWatchs != NULL) {

                length = strlen(pWatchs);
                pStatus  = (PINFO) realloc(pStatus, sizeof(INFO) + length);
                pStatus->Length = sizeof(INFO) + length;
                strcpy( pStatus->Text, pWatchs);
            }
        }
    }

    return((PLONG)pStatus);
}

/***  FreePaneStatus
**
**  Synopsis:
**      void FreePaneStatus( ViewNo, Status );
**
**  Entry:
**      int   ViewNo  - View number
**      PLONG Status  - Status area returned by GetPaneStatus()
**
**  Returns:
**      None
**
**  Description:
**      Release any buffers needed to creat the status area.
*/

void
FreePaneStatus(
               int nView,
               PLONG pStatus
               )
{
    Unreferenced(nView);
    free(pStatus);
    return;
}

/***  SetPaneStatus
**
**  Synopsis:
**      void SetPaneStatus( ViewNo, Status );
**
**  Entry:
**      int ViewNo  - The View number we need status for
**      PLONG Status  - Status area returned by GetPaneStatus()
**
**  Returns:
**      None
**
**  Description:
**      Restores the pane status information to the indicated view.
*/

void
SetPaneStatus(
              int nView,
              PLONG Status
              )
{
    HWND  hWnd     = Views[nView].hwndFrame;
    PPANE p       = (PPANE)GetWindowLongPtr(hWnd, GWW_EDIT );
    PINFO pStatus = (PINFO)Status;
    RECT  rc;

    DAssert( p );
    DAssert( p->Type == (WORD)(-Views[nView].Doc) );
    DAssert( pStatus );

    p->PanePerCent = (WORD) pStatus->PerCent;
    if ( p->PanePerCent < 10 ) p->PanePerCent = 10;
    if ( p->PanePerCent > 990) p->PanePerCent = 990;

    p->bFlags      = pStatus->Flags;

    if ( p->Type == WATCH_WIN) {
        FTSetWatchList( GetWatchVit(), pStatus->Text );
        p->LeftOk = FALSE;
    }

    SendMessage( hWnd, WU_UPDATE, 0, 0L);
    GetClientRect(hWnd, &rc);
    PostMessage(hWnd, WM_SIZE, 0, MAKELONG(rc.right,rc.bottom));

    return;
}


/***  DrawPaneItem
**
**  Synopsis:
**      void DrawPaneItem( hWnd, p, lpDis);
**
**  Entry:
**      HWND  hWnd              - Handle to the Window
**      PPANE p                 - The pane information for the draw request
**      int   xOrigin           - The X origin of the horz. Scroll bar
**      LPDRAWITEMSTRUCT lpDis  - Draw Info from WM_DRAWITEM message
**
**  Returns:
**      None
**
**  Description:
**      Paint an item in the pane manager.
*/

VOID
DrawPaneItem(
             HWND hWnd,
             PPANE p,
             LPDRAWITEMSTRUCT lpDis
             )
{
    PANEINFO Info = {0,0,0,0,NULL,NULL};
    PSTR     pBuff= NULL;
    COLORREF cfBack;
    COLORREF cfFore;

    /*
    *  Draw Logic
    */


    if ( lpDis->itemAction & ODA_DRAWENTIRE) {

    /*
    *  Button pane gets a bitmap
        */

        if ( lpDis->CtlID == ID_PANE_BUTTON) {
            DrawPaneButton(hWnd, p, lpDis);
            return;
        }

        /*
        *  Is the pane the one we editing?
        */

        else if ( lpDis->CtlID == p->nCtrlId &&
            lpDis->itemID == (UINT)p->CurIdx && p->Edit) {
            pBuff = p->EditBuf;
            cfFore = StringColors[ActiveEdit].FgndColor;
            cfBack = StringColors[ActiveEdit].BkgndColor;

        }


        /*
        *  Otherwise get the buffer from the low level and set the color
        *  either to the ChangeHistory color if its changed, or the default
        *  color it its hasn't.
        */

        else {
            Info.CtrlId = lpDis->CtlID;
            Info.ItemId = lpDis->itemID;
            (PSTR)(*p->fnEditProc)(hWnd,WU_INFO,(WPARAM)&Info,(LPARAM)p);
            pBuff = Info.pBuffer;

            if ( Info.NewText ) {
                cfFore = StringColors[ChangeHistory].FgndColor;
                cfBack = StringColors[ChangeHistory].BkgndColor;
            }

            else {
                cfFore = StringColors[p->ColorItem].FgndColor;
                cfBack = StringColors[p->ColorItem].BkgndColor;
            }
        }

        /*
        *  Make sure we ended up with a buffer and if so paint it
        */

        if (pBuff != NULL) {

            int iLen;

            iLen = strlen (pBuff);

            SetTextColor(lpDis->hDC, cfFore);
            SetBkColor  (lpDis->hDC, cfBack);

            DrawText(lpDis->hDC, pBuff, iLen, &lpDis->rcItem, PTEXT);

            if ( lpDis->CtlID == p->nCtrlId
                && lpDis->itemID == (UINT)p->CurIdx
                && p->SelLen != 0 ) {
                DrawPaneSelection( hWnd, p, lpDis, (UCHAR *)pBuff);
                return;

            }

            if ( lpDis->CtlID == ID_PANE_LEFT && Info.pFormat && !p->Edit) {
                DrawPaneFormat( hWnd, p, lpDis, (UCHAR *)pBuff, (UCHAR *)Info.pFormat);
            }

        }
    }

}   /* DrawPaneItem() */


/***  DrawPaneFormat
**
**  Synopsis:
**      void DrawPaneFormat( hWnd, p, lpDis, pBuff, pFmt);
**
**  Entry:
**      HWND  hWnd              - Handle to the Window
**      PPANE p                 - The pane information for the draw request
**      LPDRAWITEMSTRUCT lpDis  - Draw Info from WM_DRAWITEM message
**      UCHAR * pBuff           - The original buffer
**      UCHAR * pFmt            - The format string to add
**
**  Returns:
**      None
**
**  Description:
**      Paint a format string after the current string
*/

void
DrawPaneFormat(
               HWND hWnd,
               PPANE p,
               LPDRAWITEMSTRUCT lpDis,
               UCHAR * pBuff,
               UCHAR * pFmt
               )
{
    SIZE     Size = { 0, 0 };
    RECT     Rect = lpDis->rcItem;
    int      iLen;

    if (pFmt == NULL) {
        return;
    }

    iLen = strlen ( (PSTR) pFmt);

    SelectObject(lpDis->hDC, p->hFont);
    GetTextExtentPoint(lpDis->hDC, (PSTR) pBuff, strlen( (PSTR) pBuff), &Size);
    Rect.left = Size.cx;

    SetTextColor( lpDis->hDC, StringColors[ChangeHistory].FgndColor);
    SetBkColor  ( lpDis->hDC, StringColors[ChangeHistory].BkgndColor);
    DrawText( lpDis->hDC, (PSTR) pFmt, iLen, &Rect, PTEXT);


}   /* DrawPaneFormat() */


/***  DrawPaneSelection
**
**  Synopsis:
**      void DrawPaneSelection( hWnd, p, lpDis, pBuff);
**
**  Entry:
**      HWND  hWnd              - Handle to the Window
**      PPANE p                 - The pane information for the draw request
**      LPDRAWITEMSTRUCT lpDis  - Draw Info from WM_DRAWITEM message
**      UCHAR *  pBuff          - The original buffer
**
**  Returns:
**      None
**
**  Description:
**      Paints the portion of the string that is selected
*/

void
DrawPaneSelection(
                  HWND hWnd,
                  PPANE p,
                  LPDRAWITEMSTRUCT lpDis,
                  UCHAR * pBuff
                  )
{
    WORD     x;
    WORD     i;
    RECT     Rect;
    WORD     Begin;
    int      Len;
    int      Available;
    int      Size;
    int      iLen;
    TEXTMETRIC tm;

    if (pBuff == NULL)
    {
        return;
    }

    if ( p->SelLen > 0 ) {
        Begin = p->SelPos;
        Len   = p->SelLen;
    } else {
        Begin = (WORD)(p->SelPos + p->SelLen);
        Len   = -(p->SelLen);
    }

    SelectObject(lpDis->hDC, p->hFont);
    GetTextMetrics(lpDis->hDC, &tm);
    GetCharWidth(lpDis->hDC, 0, MAX_CHARS_IN_FONT - 1, (LPINT)(Views[p->iView].charWidth));

    x           = 0;
    Rect.left   = lpDis->rcItem.left;

    while ( x < Begin ) {
        Rect.left += Views[p->iView].charWidth[ pBuff[x] ];
        x++;
    }

    Rect.right = Rect.left;
    Available  = (WORD)(lpDis->rcItem.right - Rect.left);

    for ( i = 0; i < Len; i++ ) {

        Size = Views[p->iView].charWidth[ pBuff[x] ];

        if ( Available < Size ) {
            Rect.right += Available;
            break;
        }

        Rect.right += Size;
        Available  -= Size;
        x++;
    }

    Rect.top    =   lpDis->rcItem.top;
    Rect.bottom =   lpDis->rcItem.bottom;

    iLen = strlen ( (PSTR) (pBuff+Begin));

    SetTextColor( lpDis->hDC, StringColors[TextSelection].FgndColor);
    SetBkColor  ( lpDis->hDC, StringColors[TextSelection].BkgndColor);
    DrawText( lpDis->hDC, (PSTR) (pBuff+Begin), iLen, &Rect, PTEXT);
} /* DrawPaneSelection */


/***  DrawPaneButton
**
**  Synopsis:
**      void DrawPaneButton( hWnd, p, lpDis);
**
**  Entry:
**      HWND  hWnd              - Handle to the Window
**      PPANE p                 - The pane information for the draw request
**      LPDRAWITEMSTRUCT lpDis  - Draw Info from WM_DRAWITEM message
**
**  Returns:
**      None
**
**  Description:
**      Draw a button in the button pane.
*/

void
DrawPaneButton(
               HWND hWnd,
               PPANE p,
               LPDRAWITEMSTRUCT lpDis
               )
{
    LPRECT   r;
    LONG     dx, dy;
    LONG     d;
    PANEINFO Info = {0,0,0,0,NULL};

    Info.CtrlId = lpDis->CtlID;
    Info.ItemId = lpDis->itemID;

    (PSTR)(*p->fnEditProc)(hWnd,WU_INFO,(WPARAM)&Info,(LPARAM)p);
    r  = &lpDis->rcItem;
    dx = ( r->right - r->left) + 1;
    dy = ( r->bottom - r->top) +1;

    d = (p->LineHeight - BUTTON_SIZE)/2;
    r->top += d;
    dy     -= d;

    // Paint the pretty picture
    switch( Info.pBuffer ? *Info.pBuffer : ' ') {

    case '~':      // Don't want a button
        break;

    case '+':
        BitBlt (lpDis->hDC, r->left, r->top, dx, dy,
            hdcPlus, 0, 0, SRCCOPY);
        break;

    case '-':
        BitBlt (lpDis->hDC, r->left, r->top, dx, dy,
            hdcMinus, 0, 0, SRCCOPY);
        break;

    default:
        break;
    }

    // If its the current item, invert it.
    if ( lpDis->CtlID == p->nCtrlId && lpDis->itemID == (UINT)p->CurIdx ) {
        InvertButton( p );
    }

    return;         // Return Early (No Text to Paint)
}

/***  InvertButton
**
**  Synopsis:
**      void InvertButton( PPANE p)
**
**  Entry:
**      PPANE p                 - The pane information for the draw request
**
**  Returns:
**      None
**
**  Description:
**
*/

void
InvertButton(
             PPANE p
             )
{
    RECT        Rect;
    HDC         hDC;
    HBRUSH      hBrush;
    RECT        rc;

    hDC     = GetDC(p->hWndFocus);
    hBrush  = (HBRUSH) GetStockObject(BLACK_BRUSH);

    Rect.left   = 0;
    Rect.top    = (p->CurIdx - p->TopIdx) * p->LineHeight + (p->LineHeight - BUTTON_SIZE)/2 ;
    Rect.right  = BUTTON_SIZE;
    Rect.bottom = Rect.top + BUTTON_SIZE;


    GetClientRect(p->hWndFocus, &rc);

    if ( Rect.bottom < rc.bottom ) {
        FrameRect( hDC, &Rect, hBrush );

        Rect.left++;
        Rect.top++;
        Rect.right--;
        Rect.bottom--;
        FrameRect( hDC, &Rect, hBrush );
    }

    DeleteObject(hBrush);
    ReleaseDC(p->hWndFocus,hDC);
}


