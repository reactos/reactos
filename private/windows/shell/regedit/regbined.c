/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGBINED.C
*
*  VERSION:     4.01
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        05 Mar 1994
*
*  Binary edit dialog for use by the Registry Editor.
*
*  Hexadecimal editor control for use by the Registry Editor.  Little attempt
*  is made to make this a generic control-- only one instance is assumed to
*  ever exist.
*
*******************************************************************************/

#include "pch.h"
#include "regresid.h"
#include "reghelp.h"

//
//  Following structure and data are used to move the controls of the
//  EditBinaryValue dialog so that the HexEdit control fills up the appropriate
//  amount of space based on the system metrics.
//

typedef struct _MOVEWND {
    int ControlID;
    UINT SetWindowPosFlags;
}   MOVEWND;

const TCHAR s_HexEditClassName[] = HEXEDIT_CLASSNAME;

const TCHAR s_HexEditClipboardFormatName[] = TEXT("RegEdit_HexData");

const TCHAR s_HexWordFormatSpec[] = TEXT("%04X");
const TCHAR s_HexByteFormatSpec[] = TEXT("%02X");

const MOVEWND s_EditBinaryValueMoveWnd[] = {
    IDOK,           SWP_NOSIZE | SWP_NOZORDER,
    IDCANCEL,       SWP_NOSIZE | SWP_NOZORDER,
    IDC_VALUENAME,  SWP_NOMOVE | SWP_NOZORDER,
    IDC_VALUEDATA,  SWP_NOMOVE | SWP_NOZORDER
};

const DWORD s_EditBinaryValueHelpIDs[] = {
    IDC_VALUEDATA, IDH_REGEDIT_VALUEDATA,
    IDC_VALUENAME, IDH_REGEDIT_VALUENAME,
    0, 0
};


#define HEM_SETBUFFER                   (WM_USER + 1)

//  Number of bytes that are displayed per line.  NOTE:  Assumptions have been
//  made that this is power of two.
#define BYTES_PER_HEXEDIT_LINE          8
#define BYTES_PER_HEXEDIT_LINE_MASK     0x0007

//
//  This font is used by the HexEdit window for all output.  The lfHeight
//  member is calculated later based on the system configuration.
//

LOGFONT s_HexEditFont = {
    0,                                  //  lfHeight
    0,                                  //  lfWidth
    0,                                  //  lfEscapement
    0,                                  //  lfOrientation
    FW_NORMAL,                          //  lfWeight
    FALSE,                              //  lfItalic
    FALSE,                              //  lfUnderline
    FALSE,                              //  lfStrikeout
    ANSI_CHARSET,                       //  lfCharSet
    OUT_DEFAULT_PRECIS,                 //  lfOutPrecision
    CLIP_DEFAULT_PRECIS,                //  lfClipPrecision
    DEFAULT_QUALITY,                    //  lfQuality
    FIXED_PITCH | FF_DONTCARE,          //  lfPitchAndFamily
    TEXT("Courier")                     //  lfFaceName
};

//
//  Reference data for the HexEdit window.  Because we only ever expect one
//  instance of this class to exist, we can safely create one instance of this
//  structure now to avoid allocating and managing the structure later.
//

typedef struct _HEXEDITDATA {
    UINT Flags;
    PBYTE pBuffer;
    int cbBuffer;
    int cxWindow;                       //  Width of the window
    int cyWindow;                       //  Height of the window
    HFONT hFont;                        //  Font being used for output
    LONG FontHeight;                    //  Height of the above font
    LONG FontMaxWidth;                  //  Maximum width of the above font
    int LinesVisible;                   //  Number of lines can be displayed
    int MaximumLines;                   //  Total number of lines
    int FirstVisibleLine;               //  Line number of top of display
    int xHexDumpStart;
    int xHexDumpByteWidth;
    int xAsciiDumpStart;
    int CaretIndex;
    int MinimumSelectedIndex;
    int MaximumSelectedIndex;
    int xPrevMessagePos;                //  Cursor point on last mouse message
    int yPrevMessagePos;                //  Cursor point on last mouse message
}   HEXEDITDATA;

//  Set if window has input focus, clear if not.
#define HEF_FOCUS                       0x00000001
#define HEF_NOFOCUS                     0x00000000
//  Set if dragging a range with mouse, clear if not.
#define HEF_DRAGGING                    0x00000002
#define HEF_NOTDRAGGING                 0x00000000
//  Set if editing ASCII column, clear if editing hexadecimal column.
#define HEF_CARETINASCIIDUMP            0x00000004
#define HEF_CARETINHEXDUMP              0x00000000
//
#define HEF_INSERTATLOWNIBBLE           0x00000008
#define HEF_INSERTATHIGHNIBBLE          0x00000000
//  Set if caret should be shown at the end of the previous line instead of at
//  the beginning of it's real caret line, clear if not.
#define HEF_CARETATENDOFLINE            0x00000010

HEXEDITDATA s_HexEditData;

typedef struct _HEXEDITCLIPBOARDDATA {
    DWORD cbSize;
    BYTE Data[1];
}   HEXEDITCLIPBOARDDATA, *LPHEXEDITCLIPBOARDDATA;

UINT s_HexEditClipboardFormat;

BOOL
PASCAL
EditBinaryValue_OnInitDialog(
    HWND hWnd,
    HWND hFocusWnd,
    LPARAM lParam
    );

LRESULT
PASCAL
HexEditWndProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
PASCAL
HexEdit_OnNcCreate(
    HWND hWnd,
    LPCREATESTRUCT lpCreateStruct
    );

VOID
PASCAL
HexEdit_OnSize(
    HWND hWnd,
    UINT State,
    int cx,
    int cy
    );

VOID
PASCAL
HexEdit_SetScrollInfo(
    HWND hWnd
    );

VOID
PASCAL
HexEdit_OnVScroll(
    HWND hWnd,
    HWND hCtlWnd,
    UINT Code,
    int Position
    );

VOID
PASCAL
HexEdit_OnPaint(
    HWND hWnd
    );

VOID
PASCAL
HexEdit_PaintRect(
    HDC hDC,
    LPRECT lpUpdateRect
    );

VOID
PASCAL
HexEdit_OnSetFocus(
    HWND hWnd
    );

VOID
PASCAL
HexEdit_OnKillFocus(
    HWND hWnd
    );

VOID
PASCAL
HexEdit_OnLButtonDown(
    HWND hWnd,
    BOOL fDoubleClick,
    int x,
    int y,
    UINT KeyFlags
    );

VOID
PASCAL
HexEdit_OnMouseMove(
    HWND hWnd,
    int x,
    int y,
    UINT KeyFlags
    );

VOID
PASCAL
HexEdit_OnLButtonUp(
    HWND hWnd,
    int x,
    int y,
    UINT KeyFlags
    );

int
PASCAL
HexEdit_HitTest(
    int x,
    int y
    );

VOID
PASCAL
HexEdit_OnKey(
    HWND hWnd,
    UINT VirtualKey,
    BOOL fDown,
    int cRepeat,
    UINT Flags
    );

VOID
PASCAL
HexEdit_OnChar(
    HWND hWnd,
    TCHAR Char,
    int cRepeat
    );

VOID
PASCAL
HexEdit_SetCaretPosition(
    HWND hWnd
    );

VOID
PASCAL
HexEdit_EnsureCaretVisible(
    HWND hWnd
    );

VOID
PASCAL
HexEdit_ChangeCaretIndex(
    HWND hWnd,
    int NewCaretIndex,
    BOOL fExtendSelection
    );

VOID
PASCAL
HexEdit_DeleteRange(
    HWND hWnd,
    UINT SourceKey
    );

BOOL
PASCAL
HexEdit_OnCopy(
    HWND hWnd
    );

BOOL
PASCAL
HexEdit_OnPaste(
    HWND hWnd
    );

VOID
PASCAL
HexEdit_OnContextMenu(
    HWND hWnd,
    int x,
    int y
    );

/*******************************************************************************
*
*  EditBinaryValueDlgProc
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

INT_PTR
CALLBACK
EditBinaryValueDlgProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )
{

    LPEDITVALUEPARAM lpEditValueParam;

    switch (Message) {

        HANDLE_MSG(hWnd, WM_INITDIALOG, EditBinaryValue_OnInitDialog);

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {

                case IDOK:
                    lpEditValueParam = (LPEDITVALUEPARAM) GetWindowLongPtr(hWnd,
                        DWLP_USER);

                    lpEditValueParam-> cbValueData = s_HexEditData.cbBuffer;

                    //  FALL THROUGH

                case IDCANCEL:
                    EndDialog(hWnd, GET_WM_COMMAND_ID(wParam, lParam));
                    break;

                default:
                    return FALSE;

            }
            break;

        case WM_HELP:
            WinHelp(((LPHELPINFO) lParam)-> hItemHandle, g_pHelpFileName,
                HELP_WM_HELP, (ULONG_PTR) s_EditBinaryValueHelpIDs);
            break;

        case WM_CONTEXTMENU:
            WinHelp((HWND) wParam, g_pHelpFileName, HELP_CONTEXTMENU,
                (DWORD) (ULONG_PTR) s_EditBinaryValueHelpIDs);
            break;

        default:
            return FALSE;

    }

    return TRUE;

}

/*******************************************************************************
*
*  EditBinaryValue_OnInitDialog
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd,
*     hFocusWnd,
*     lParam,
*
*******************************************************************************/

BOOL
PASCAL
EditBinaryValue_OnInitDialog(
    HWND hWnd,
    HWND hFocusWnd,
    LPARAM lParam
    )
{

    LPEDITVALUEPARAM lpEditValueParam;
    RECT Rect;
    int HexEditIdealWidth;
    int dxChange;
    HWND hControlWnd;
    UINT Counter;

    SetWindowLongPtr(hWnd, DWLP_USER, lParam);
    lpEditValueParam = (LPEDITVALUEPARAM) lParam;

    SetDlgItemText(hWnd, IDC_VALUENAME, lpEditValueParam-> pValueName);

    SendDlgItemMessage(hWnd, IDC_VALUEDATA, HEM_SETBUFFER, (WPARAM)
        lpEditValueParam-> cbValueData, (LPARAM) lpEditValueParam-> pValueData);

    //
    //  Figure out how big the "ideally" size HexEdit should be-- this means
    //  displaying the address, hex dump, ASCII dump, and potentially a scroll
    //  bar.
    //

    GetWindowRect(GetDlgItem(hWnd, IDC_VALUEDATA), &Rect);

    HexEditIdealWidth = s_HexEditData.xAsciiDumpStart +
        s_HexEditData.FontMaxWidth * (BYTES_PER_HEXEDIT_LINE + 1) +
        GetSystemMetrics(SM_CXVSCROLL) + GetSystemMetrics(SM_CXEDGE) * 2;

    dxChange = HexEditIdealWidth - (Rect.right - Rect.left);

    //
    //  Resize the dialog box.
    //

    GetWindowRect(hWnd, &Rect);

    MoveWindow(hWnd, Rect.left, Rect.top, Rect.right - Rect.left + dxChange,
        Rect.bottom - Rect.top, FALSE);

    //
    //  Resize or move the controls as necessary.
    //

    for (Counter = 0; Counter < (sizeof(s_EditBinaryValueMoveWnd) /
        sizeof(MOVEWND)); Counter++) {

        hControlWnd = GetDlgItem(hWnd,
            s_EditBinaryValueMoveWnd[Counter].ControlID);

        GetWindowRect(hControlWnd, &Rect);

        if (s_EditBinaryValueMoveWnd[Counter].SetWindowPosFlags & SWP_NOSIZE) {

            MapWindowPoints(NULL, hWnd, (LPPOINT) &Rect, 2);
            Rect.left += dxChange;

        }

        else
            Rect.right += dxChange;

        SetWindowPos(hControlWnd, NULL, Rect.left, Rect.top, Rect.right -
            Rect.left, Rect.bottom - Rect.top,
            s_EditBinaryValueMoveWnd[Counter].SetWindowPosFlags);

    }

    return TRUE;

    UNREFERENCED_PARAMETER(hFocusWnd);

}

/*******************************************************************************
*
*  RegisterHexEditClass
*
*  DESCRIPTION:
*     Register the HexEdit window class with the system.
*
*  PARAMETERS:
*     (none).
*
*******************************************************************************/

BOOL
PASCAL
RegisterHexEditClass(
    VOID
    )
{

    WNDCLASS WndClass;

    s_HexEditClipboardFormat =
        RegisterClipboardFormat(s_HexEditClipboardFormatName);

    WndClass.style = CS_DBLCLKS;
    WndClass.lpfnWndProc = HexEditWndProc;
    WndClass.cbClsExtra = 0;
    WndClass.cbWndExtra = 0;
    WndClass.hInstance = g_hInstance;
    WndClass.hIcon = NULL;
    WndClass.hCursor = LoadCursor(NULL, IDC_IBEAM);
    WndClass.hbrBackground = NULL;
    WndClass.lpszMenuName = NULL;
    WndClass.lpszClassName = s_HexEditClassName;

    return (RegisterClass(&WndClass) != 0);

}

/*******************************************************************************
*
*  HexEditWndProc
*
*  DESCRIPTION:
*     Callback procedure for the HexEdit window.
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*     Message,
*     wParam,
*     lParam,
*     (returns),
*
*******************************************************************************/

LRESULT
PASCAL
HexEditWndProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )
{

    switch (Message) {

        HANDLE_MSG(hWnd, WM_NCCREATE, HexEdit_OnNcCreate);
        HANDLE_MSG(hWnd, WM_SIZE, HexEdit_OnSize);
        HANDLE_MSG(hWnd, WM_VSCROLL, HexEdit_OnVScroll);
        HANDLE_MSG(hWnd, WM_PAINT, HexEdit_OnPaint);
        HANDLE_MSG(hWnd, WM_LBUTTONDOWN, HexEdit_OnLButtonDown);
        HANDLE_MSG(hWnd, WM_LBUTTONDBLCLK, HexEdit_OnLButtonDown);
        HANDLE_MSG(hWnd, WM_MOUSEMOVE, HexEdit_OnMouseMove);
        HANDLE_MSG(hWnd, WM_LBUTTONUP, HexEdit_OnLButtonUp);
        HANDLE_MSG(hWnd, WM_KEYDOWN, HexEdit_OnKey);
        HANDLE_MSG(hWnd, WM_CHAR, HexEdit_OnChar);

        case WM_SETFOCUS:
            HexEdit_OnSetFocus(hWnd);
            break;

        case WM_KILLFOCUS:
            HexEdit_OnKillFocus(hWnd);
            break;

        case WM_TIMER:
            HexEdit_OnMouseMove(hWnd, s_HexEditData.xPrevMessagePos,
                s_HexEditData.yPrevMessagePos, 0);
            break;

        case WM_GETDLGCODE:
            return (LPARAM) (DLGC_WANTCHARS | DLGC_WANTARROWS);

        case WM_ERASEBKGND:
            return TRUE;

        case WM_NCDESTROY:
            DeleteObject(s_HexEditData.hFont);
            break;

        case WM_CONTEXTMENU:
            HexEdit_OnContextMenu(hWnd, LOWORD(lParam), HIWORD(lParam));
            break;

        //  Message: HEM_SETBUFFER
        //  wParam:  Number of bytes in the buffer.
        //  lParam:  Pointer to the buffer.
        case HEM_SETBUFFER:
            s_HexEditData.pBuffer = (PBYTE) lParam;
            s_HexEditData.cbBuffer = (int) wParam;

            s_HexEditData.CaretIndex = 0;
            s_HexEditData.MinimumSelectedIndex = 0;
            s_HexEditData.MaximumSelectedIndex = 0;

            s_HexEditData.FirstVisibleLine = 0;

            HexEdit_SetScrollInfo(hWnd);

            break;

        default:
            return DefWindowProc(hWnd, Message, wParam, lParam);

    }

    return 0;

}

/*******************************************************************************
*
*  HexEdit_OnNcCreate
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*
*******************************************************************************/

BOOL
PASCAL
HexEdit_OnNcCreate(
    HWND hWnd,
    LPCREATESTRUCT lpCreateStruct
    )
{

    HDC hDC;
    HFONT hPrevFont;
    TEXTMETRIC TextMetric;

    s_HexEditData.cbBuffer = 0;

    //  BUGBUG:  Do this symbolically...
    s_HexEditData.Flags = 0;

    s_HexEditData.cxWindow = 0;
    s_HexEditData.cyWindow = 0;

    hDC = GetDC(hWnd);

    s_HexEditFont.lfHeight = -(10 * GetDeviceCaps(hDC, LOGPIXELSY) / 72);

    if ((s_HexEditData.hFont = CreateFontIndirect(&s_HexEditFont)) != NULL) {

        hPrevFont = SelectObject(hDC, s_HexEditData.hFont);
        GetTextMetrics(hDC, &TextMetric);
        SelectObject(hDC, hPrevFont);

        s_HexEditData.FontHeight = TextMetric.tmHeight;

        s_HexEditData.LinesVisible = s_HexEditData.cyWindow /
            s_HexEditData.FontHeight;

        s_HexEditData.FontMaxWidth = TextMetric.tmMaxCharWidth;

        s_HexEditData.xHexDumpByteWidth = s_HexEditData.FontMaxWidth * 3;
        s_HexEditData.xHexDumpStart = s_HexEditData.FontMaxWidth * 11 / 2;
        s_HexEditData.xAsciiDumpStart = s_HexEditData.xHexDumpStart +
            BYTES_PER_HEXEDIT_LINE * s_HexEditData.xHexDumpByteWidth +
            s_HexEditData.FontMaxWidth * 3 / 2;

    }

    ReleaseDC(hWnd, hDC);

    if (s_HexEditData.hFont == NULL)
        return FALSE;

    return (BOOL) DefWindowProc(hWnd, WM_NCCREATE, 0, (LPARAM) lpCreateStruct);

}

/*******************************************************************************
*
*  HexEdit_OnSize
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
HexEdit_OnSize(
    HWND hWnd,
    UINT State,
    int cx,
    int cy
    )
{

    s_HexEditData.cxWindow = cx;
    s_HexEditData.cyWindow = cy;

    s_HexEditData.LinesVisible = cy / s_HexEditData.FontHeight;

    HexEdit_SetScrollInfo(hWnd);

    UNREFERENCED_PARAMETER(State);
    UNREFERENCED_PARAMETER(cx);

}

/*******************************************************************************
*
*  HexEdit_SetScrollInfo
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
HexEdit_SetScrollInfo(
    HWND hWnd
    )
{

    SCROLLINFO ScrollInfo;

    s_HexEditData.MaximumLines = (s_HexEditData.cbBuffer +
        BYTES_PER_HEXEDIT_LINE) / BYTES_PER_HEXEDIT_LINE - 1;

    ScrollInfo.cbSize = sizeof(SCROLLINFO);
    ScrollInfo.fMask = (SIF_RANGE | SIF_PAGE | SIF_POS);
    ScrollInfo.nMin = 0;
    ScrollInfo.nMax = s_HexEditData.MaximumLines;
    ScrollInfo.nPage = s_HexEditData.LinesVisible;
    ScrollInfo.nPos = s_HexEditData.FirstVisibleLine;

    SetScrollInfo(hWnd, SB_VERT, &ScrollInfo, TRUE);

}

/*******************************************************************************
*
*  HexEdit_OnVScroll
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*
*******************************************************************************/

VOID
PASCAL
HexEdit_OnVScroll(
    HWND hWnd,
    HWND hCtlWnd,
    UINT Code,
    int Position
    )
{

    int NewFirstVisibleLine;
    SCROLLINFO ScrollInfo;

    NewFirstVisibleLine = s_HexEditData.FirstVisibleLine;

    switch (Code) {

        case SB_LINEUP:
            NewFirstVisibleLine--;
            break;

        case SB_LINEDOWN:
            NewFirstVisibleLine++;
            break;

        case SB_PAGEUP:
            NewFirstVisibleLine -= s_HexEditData.LinesVisible;
            break;

        case SB_PAGEDOWN:
            NewFirstVisibleLine += s_HexEditData.LinesVisible;
            break;

        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            NewFirstVisibleLine = Position;
            break;

    }

    //
    //  Change the scroll bar position.  Note that SetScrollInfo will take into
    //  account the clipping between zero and the maximum value.  It will also
    //  return the final scroll bar position.
    //

    ScrollInfo.cbSize = sizeof(SCROLLINFO);
    ScrollInfo.fMask = SIF_POS;
    ScrollInfo.nPos = NewFirstVisibleLine;

    NewFirstVisibleLine = SetScrollInfo(hWnd, SB_VERT, &ScrollInfo, TRUE);

    if (s_HexEditData.FirstVisibleLine != NewFirstVisibleLine) {

        ScrollWindowEx(hWnd, 0, (s_HexEditData.FirstVisibleLine -
            NewFirstVisibleLine) * s_HexEditData.FontHeight, NULL, NULL, NULL,
            NULL, SW_INVALIDATE);

        s_HexEditData.FirstVisibleLine = NewFirstVisibleLine;

        HexEdit_SetCaretPosition(hWnd);

    }

    UNREFERENCED_PARAMETER(hCtlWnd);

}

/*******************************************************************************
*
*  HexEdit_OnPaint
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*
*******************************************************************************/

VOID
PASCAL
HexEdit_OnPaint(
    HWND hWnd
    )
{

    PAINTSTRUCT PaintStruct;

    BeginPaint(hWnd, &PaintStruct);

    HexEdit_PaintRect(PaintStruct.hdc, &PaintStruct.rcPaint);

    EndPaint(hWnd, &PaintStruct);

}

/*******************************************************************************
*
*  HexEdit_PaintRect
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
HexEdit_PaintRect(
    HDC hDC,
    LPRECT lpUpdateRect
    )
{

    HFONT hPrevFont;
    int CurrentByteIndex;
    BYTE Byte;
    int CurrentLine;
    int LastLine;
    int BytesOnLastLine;
    int BytesOnLine;
    BOOL fUsingHighlight;
    int Counter;
    TCHAR Buffer[5];                     //  Room for four hex digits plus null
    RECT TextRect;
    RECT AsciiTextRect;
    int x;

    if (s_HexEditData.hFont)
        hPrevFont = SelectFont(hDC, s_HexEditData.hFont);

    SetBkColor(hDC, g_clrWindow);
    SetTextColor(hDC, g_clrWindowText);

    //
    //  Figure out the range of lines of the control that must be painted.
    //  Using this information we can compute the offset into the buffer to
    //  start reading from.
    //

    CurrentLine = lpUpdateRect-> top / s_HexEditData.FontHeight;

    TextRect.bottom = CurrentLine * s_HexEditData.FontHeight;
    AsciiTextRect.bottom = TextRect.bottom;

    CurrentByteIndex = (s_HexEditData.FirstVisibleLine + CurrentLine) *
        BYTES_PER_HEXEDIT_LINE;

    LastLine = lpUpdateRect-> bottom / s_HexEditData.FontHeight;

    //
    //  Figure out if there's enough in the buffer to fill up the entire window
    //  and the last line that we paint.
    //

    if (LastLine >= s_HexEditData.MaximumLines -
        s_HexEditData.FirstVisibleLine) {

        LastLine = s_HexEditData.MaximumLines - s_HexEditData.FirstVisibleLine;

        BytesOnLastLine = s_HexEditData.cbBuffer % BYTES_PER_HEXEDIT_LINE;

    }

    else
        BytesOnLastLine = BYTES_PER_HEXEDIT_LINE;

    BytesOnLine = BYTES_PER_HEXEDIT_LINE;
    fUsingHighlight = FALSE;

    //
    //  Loop through each of the lines to be displayed.
    //

    while (CurrentLine <= LastLine) {

        //
        //  If we're on the last line of the display and this is at the end
        //  of the buffer, we may not have a complete line to paint.
        //

        if (CurrentLine == LastLine)
            BytesOnLine = BytesOnLastLine;

        TextRect.top = TextRect.bottom;
        TextRect.bottom += s_HexEditData.FontHeight;

        TextRect.left = 0;
        TextRect.right = s_HexEditData.xHexDumpStart;

        x = TextRect.right + s_HexEditData.FontMaxWidth / 2;

        wsprintf(Buffer, s_HexWordFormatSpec, CurrentByteIndex);
        ExtTextOut(hDC, 0, TextRect.top, ETO_OPAQUE, &TextRect, Buffer, 4,
            NULL);

        AsciiTextRect.top = AsciiTextRect.bottom;
        AsciiTextRect.bottom += s_HexEditData.FontHeight;
        AsciiTextRect.right = s_HexEditData.xAsciiDumpStart;

        for (Counter = 0; Counter < BytesOnLine; Counter++,
            CurrentByteIndex++) {

            //
            //  Determine what colors to use to paint the current byte.
            //

            if (CurrentByteIndex >= s_HexEditData.MinimumSelectedIndex) {

                if (CurrentByteIndex >= s_HexEditData.MaximumSelectedIndex) {

                    if (fUsingHighlight) {

                        fUsingHighlight = FALSE;

                        SetBkColor(hDC, g_clrWindow);
                        SetTextColor(hDC, g_clrWindowText);

                    }

                }

                else {

                    if (!fUsingHighlight) {

                        fUsingHighlight = TRUE;

                        SetBkColor(hDC, g_clrHighlight);
                        SetTextColor(hDC, g_clrHighlightText);

                    }

                }

            }

            Byte = s_HexEditData.pBuffer[CurrentByteIndex];

            //
            //  Paint the hexadecimal representation.
            //

            TextRect.left = TextRect.right;
            TextRect.right += s_HexEditData.xHexDumpByteWidth;

            wsprintf(Buffer, s_HexByteFormatSpec, Byte);

            ExtTextOut(hDC, x, TextRect.top, ETO_OPAQUE, &TextRect,
                Buffer, 2, NULL);

            x += s_HexEditData.xHexDumpByteWidth;

            //
            //  Paint the ASCII representation.
            //

            AsciiTextRect.left = AsciiTextRect.right;
            AsciiTextRect.right += s_HexEditData.FontMaxWidth;

            Buffer[0] = (TCHAR) (((Byte & 0x7F) >= TEXT(' ')) ? Byte : TEXT('.'));

            ExtTextOut(hDC, AsciiTextRect.left, AsciiTextRect.top, ETO_OPAQUE,
                &AsciiTextRect, Buffer, 1, NULL);

        }

        //
        //  Paint any leftover strips between the hexadecimal and ASCII columns
        //  and the ASCII column and the right edge of the window.
        //

        if (fUsingHighlight) {

            fUsingHighlight = FALSE;

            SetBkColor(hDC, g_clrWindow);
            SetTextColor(hDC, g_clrWindowText);

        }

        TextRect.left = TextRect.right;
        TextRect.right = s_HexEditData.xAsciiDumpStart;

        ExtTextOut(hDC, TextRect.left, TextRect.top, ETO_OPAQUE, &TextRect,
            NULL, 0, NULL);

        AsciiTextRect.left = AsciiTextRect.right;
        AsciiTextRect.right = s_HexEditData.cxWindow;

        ExtTextOut(hDC, AsciiTextRect.left, AsciiTextRect.top, ETO_OPAQUE,
            &AsciiTextRect, NULL, 0, NULL);

        CurrentLine++;

    }

    //
    //  Paint any remaining space in the control by filling it with the
    //  background color.
    //

    if (TextRect.bottom < lpUpdateRect-> bottom) {

        TextRect.left = 0;
        TextRect.right = s_HexEditData.cxWindow;
        TextRect.top = TextRect.bottom;
        TextRect.bottom = lpUpdateRect-> bottom;

        ExtTextOut(hDC, 0, TextRect.top, ETO_OPAQUE, &TextRect, NULL, 0, NULL);

    }

    if (s_HexEditData.hFont)
        SelectFont(hDC, hPrevFont);

}

/*******************************************************************************
*
*  HexEdit_OnSetFocus
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*
*******************************************************************************/

VOID
PASCAL
HexEdit_OnSetFocus(
    HWND hWnd
    )
{

    s_HexEditData.Flags |= HEF_FOCUS;

    CreateCaret(hWnd, NULL, 0, s_HexEditData.FontHeight);
    HexEdit_SetCaretPosition(hWnd);
    ShowCaret(hWnd);

}

/*******************************************************************************
*
*  HexEdit_OnKillFocus
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*
*******************************************************************************/

VOID
PASCAL
HexEdit_OnKillFocus(
    HWND hWnd
    )
{

    if (s_HexEditData.Flags & HEF_FOCUS) {

        s_HexEditData.Flags &= ~HEF_FOCUS;

        DestroyCaret();

    }

}

/*******************************************************************************
*
*  HexEdit_OnLButtonDown
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*     fDoubleClick, TRUE if this is a double-click message, else FALSE.
*     x, x-coordinate of the cursor relative to the client area.
*     y, y-coordinate of the cursor relative to the client area.
*     KeyFlags, state of various virtual keys.
*
*******************************************************************************/

VOID
PASCAL
HexEdit_OnLButtonDown(
    HWND hWnd,
    BOOL fDoubleClick,
    int x,
    int y,
    UINT KeyFlags
    )
{

    int NewCaretIndex;

    if (fDoubleClick) {

        HexEdit_ChangeCaretIndex(hWnd, s_HexEditData.CaretIndex + 1, TRUE);
        return;

    }

    NewCaretIndex = HexEdit_HitTest(x, y);

    HexEdit_ChangeCaretIndex(hWnd, NewCaretIndex, (KeyFlags & MK_SHIFT));

    //
    //  If we don't already have the focus, try to get it.
    //

    if (!(s_HexEditData.Flags & HEF_FOCUS))
        SetFocus(hWnd);

    SetCapture(hWnd);
    s_HexEditData.Flags |= HEF_DRAGGING;

    s_HexEditData.xPrevMessagePos = x;
    s_HexEditData.yPrevMessagePos = y;

    SetTimer(hWnd, 1, 400, NULL);

    UNREFERENCED_PARAMETER(fDoubleClick);

}

/*******************************************************************************
*
*  HexEdit_OnMouseMove
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*     x, x-coordinate of the cursor relative to the client area.
*     y, y-coordinate of the cursor relative to the client area.
*     KeyFlags, state of various virtual keys.
*
*******************************************************************************/

VOID
PASCAL
HexEdit_OnMouseMove(
    HWND hWnd,
    int x,
    int y,
    UINT KeyFlags
    )
{

    int NewCaretIndex;

    if (!(s_HexEditData.Flags & HEF_DRAGGING))
        return;

    NewCaretIndex = HexEdit_HitTest(x, y);

    HexEdit_ChangeCaretIndex(hWnd, NewCaretIndex, TRUE);

    s_HexEditData.xPrevMessagePos = x;
    s_HexEditData.yPrevMessagePos = y;

    {

    int i, j;

    i = y < 0 ? -y : y - s_HexEditData.cyWindow;
    j = 400 - ((UINT)i << 4);
    if (j < 100)
        j = 100;
    SetTimer(hWnd, 1, j, NULL);

    }

    UNREFERENCED_PARAMETER(KeyFlags);

}

/*******************************************************************************
*
*  HexEdit_OnLButtonUp
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*     x, x-coordinate of the cursor relative to the client area.
*     y, y-coordinate of the cursor relative to the client area.
*     KeyFlags, state of various virtual keys.
*
*******************************************************************************/

VOID
PASCAL
HexEdit_OnLButtonUp(
    HWND hWnd,
    int x,
    int y,
    UINT KeyFlags
    )
{

    if (!(s_HexEditData.Flags & HEF_DRAGGING))
        return;

    KillTimer(hWnd, 1);

    ReleaseCapture();
    s_HexEditData.Flags &= ~HEF_DRAGGING;

    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
    UNREFERENCED_PARAMETER(KeyFlags);

}

/*******************************************************************************
*
*  HexEdit_HitTest
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     x, x-coordinate of the cursor relative to the client area.
*     y, y-coordinate of the cursor relative to the client area.
*     (returns), index of "hit" byte.
*
*******************************************************************************/

int
PASCAL
HexEdit_HitTest(
    int x,
    int y
    )
{

    int HitLine;
    int BytesOnHitLine;
    int HitByte;

    //
    //  Figure out which line the user clicked on and how many bytes are on that
    //  line.
    //

    if (y < 0)
        HitLine = -1;

    else if (y >= s_HexEditData.cyWindow)
        HitLine = s_HexEditData.LinesVisible + 1;

    else
        HitLine = y / s_HexEditData.FontHeight;

    HitLine += s_HexEditData.FirstVisibleLine;

    if (HitLine >= s_HexEditData.MaximumLines) {

        HitLine = s_HexEditData.MaximumLines;

        BytesOnHitLine = (s_HexEditData.cbBuffer + 1) %
            BYTES_PER_HEXEDIT_LINE;

        if (BytesOnHitLine == 0)
            BytesOnHitLine = BYTES_PER_HEXEDIT_LINE;

    }

    else {

        if (HitLine < 0)
            HitLine = 0;

        BytesOnHitLine = BYTES_PER_HEXEDIT_LINE;

    }

    //
    //
    //

    if (x < s_HexEditData.xHexDumpStart)
        x = s_HexEditData.xHexDumpStart;

    if (x >= s_HexEditData.xHexDumpStart && x <
        s_HexEditData.xHexDumpStart + s_HexEditData.xHexDumpByteWidth *
        BYTES_PER_HEXEDIT_LINE + s_HexEditData.FontMaxWidth) {

        x -= s_HexEditData.xHexDumpStart;

        HitByte = x / s_HexEditData.xHexDumpByteWidth;

        s_HexEditData.Flags &= ~HEF_CARETINASCIIDUMP;

    }

    else {

        HitByte = (x - (s_HexEditData.xAsciiDumpStart -
            s_HexEditData.FontMaxWidth / 2)) / s_HexEditData.FontMaxWidth;

        s_HexEditData.Flags |= HEF_CARETINASCIIDUMP;

    }

    //
    //  We allow the user to "hit" the first byte of any line via two ways:
    //      *  clicking before the first byte on that line.
    //      *  clicking beyond the last byte/character of either display of the
    //         previous line.
    //
    //  We would like to see the latter case so that dragging in the control
    //  works naturally-- it's possible to drag to the end of the line to select
    //  the entire range.
    //

    s_HexEditData.Flags &= ~HEF_CARETATENDOFLINE;

    if (HitByte >= BytesOnHitLine) {

        if (BytesOnHitLine == BYTES_PER_HEXEDIT_LINE) {

            HitByte = BYTES_PER_HEXEDIT_LINE;
            s_HexEditData.Flags |= HEF_CARETATENDOFLINE;

        }

        else
            HitByte = BytesOnHitLine - 1;

    }

    return HitLine * BYTES_PER_HEXEDIT_LINE + HitByte;

}

/*******************************************************************************
*
*  HexEdit_OnKey
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*     Char,
*     cRepeat,
*
*******************************************************************************/

VOID
PASCAL
HexEdit_OnKey(
    HWND hWnd,
    UINT VirtualKey,
    BOOL fDown,
    int cRepeat,
    UINT Flags
    )
{

    BOOL fControlDown;
    BOOL fShiftDown;
    int NewCaretIndex;
    UINT ScrollCode;

    fControlDown = (GetKeyState(VK_CONTROL) < 0);
    fShiftDown = (GetKeyState(VK_SHIFT) < 0);

    NewCaretIndex = s_HexEditData.CaretIndex;

    switch (VirtualKey) {

        case VK_UP:
            if (fControlDown)
                break;

            NewCaretIndex -= BYTES_PER_HEXEDIT_LINE;
            goto onkey_CheckLowerBound;

        case VK_DOWN:
            if (fControlDown)
                break;

            NewCaretIndex += BYTES_PER_HEXEDIT_LINE;

            if (NewCaretIndex / BYTES_PER_HEXEDIT_LINE >
                s_HexEditData.MaximumLines) {

                if (s_HexEditData.Flags & HEF_CARETATENDOFLINE)
                    goto onkey_MoveToEndOfBuffer;

                break;

            }

            goto onkey_CheckUpperBound;

        case VK_HOME:
            if (fControlDown)
                NewCaretIndex = 0;

            else {

                if (s_HexEditData.Flags & HEF_CARETATENDOFLINE)
                    NewCaretIndex -= BYTES_PER_HEXEDIT_LINE;

                else
                    NewCaretIndex &= (~BYTES_PER_HEXEDIT_LINE_MASK);

            }

            s_HexEditData.Flags &= ~HEF_CARETATENDOFLINE;

            goto onkey_ChangeCaretIndex;

        case VK_END:
            if (fControlDown) {

onkey_MoveToEndOfBuffer:
                s_HexEditData.Flags &= ~HEF_CARETATENDOFLINE;
                NewCaretIndex = s_HexEditData.cbBuffer;

            }

            else {

                if (s_HexEditData.Flags & HEF_CARETATENDOFLINE)
                    break;

                NewCaretIndex = (NewCaretIndex &
                    (~BYTES_PER_HEXEDIT_LINE_MASK)) + BYTES_PER_HEXEDIT_LINE;

                if (NewCaretIndex > s_HexEditData.cbBuffer)
                    NewCaretIndex = s_HexEditData.cbBuffer;

                else
                    s_HexEditData.Flags |= HEF_CARETATENDOFLINE;

            }

            goto onkey_ChangeCaretIndex;

        case VK_PRIOR:
        case VK_NEXT:
            NewCaretIndex -= s_HexEditData.FirstVisibleLine *
                BYTES_PER_HEXEDIT_LINE;

            ScrollCode = ((VirtualKey == VK_PRIOR) ? SB_PAGEUP : SB_PAGEDOWN);

            HexEdit_OnVScroll(hWnd, NULL, ScrollCode, 0);

            NewCaretIndex += s_HexEditData.FirstVisibleLine *
                BYTES_PER_HEXEDIT_LINE;

            if (VirtualKey == VK_PRIOR)
                goto onkey_CheckLowerBound;

            else
                goto onkey_CheckUpperBound;

        case VK_LEFT:
            s_HexEditData.Flags &= ~HEF_CARETATENDOFLINE;
            NewCaretIndex--;

onkey_CheckLowerBound:
            if (NewCaretIndex < 0)
                break;

            goto onkey_ChangeCaretIndex;

        case VK_RIGHT:
            s_HexEditData.Flags &= ~HEF_CARETATENDOFLINE;
            NewCaretIndex++;

onkey_CheckUpperBound:
            if (NewCaretIndex > s_HexEditData.cbBuffer)
                NewCaretIndex = s_HexEditData.cbBuffer;

onkey_ChangeCaretIndex:
            HexEdit_ChangeCaretIndex(hWnd, NewCaretIndex, fShiftDown);
            break;

        case VK_DELETE:
            if (!fControlDown) {

                if (fShiftDown)
                    HexEdit_OnChar(hWnd, IDKEY_CUT, 0);
                else
                    HexEdit_DeleteRange(hWnd, VK_DELETE);

            }
            break;

        case VK_INSERT:
            if (fShiftDown) {

                if (!fControlDown)
                    HexEdit_OnChar(hWnd, IDKEY_PASTE, 0);

            }

            else if (fControlDown)
                HexEdit_OnChar(hWnd, IDKEY_COPY, 0);
            break;

    }

}

/*******************************************************************************
*
*  HexEdit_OnChar
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*     Char,
*     cRepeat,
*
*******************************************************************************/

VOID
PASCAL
HexEdit_OnChar(
    HWND hWnd,
    TCHAR Char,
    int cRepeat
    )
{

    PBYTE pCaretByte;
    BYTE NewCaretByte;
    int PrevCaretIndex;
    RECT UpdateRect;

    //
    //  Check for any special control characters.
    //

    switch (Char) {

        case IDKEY_COPY:
            HexEdit_OnCopy(hWnd);
            return;

        case IDKEY_PASTE:
            PrevCaretIndex = s_HexEditData.CaretIndex;

            if (HexEdit_OnPaste(hWnd))
                goto UpdateDisplay;

            return;

        case IDKEY_CUT:
            if (!HexEdit_OnCopy(hWnd))
                return;
            //  FALL THROUGH

        case VK_BACK:
            HexEdit_DeleteRange(hWnd, VK_BACK);
            return;

    }

    //
    //  Validate and convert the typed character depending on the "column" the
    //  user is typing in.
    //

    if (s_HexEditData.Flags & HEF_CARETINASCIIDUMP) {

        if (Char < TEXT(' ')) {

            MessageBeep(MB_OK);
            return;

        }

        NewCaretByte = (BYTE) Char;

    }

    else {

        Char = (CHAR) CharLower((LPTSTR) Char);

        if (Char >= TEXT('0') && Char <= TEXT('9'))
            NewCaretByte = (BYTE) (Char - TEXT('0'));

        else if (Char >= TEXT('a') && Char <= TEXT('f'))
            NewCaretByte = (BYTE) (Char - TEXT('a') + 10);

        else {

            MessageBeep(MB_OK);
            return;

        }

    }

    if (!(s_HexEditData.Flags & HEF_INSERTATLOWNIBBLE)) {

        //
        //  Check to see if we're inserting while a range is selected.  If so,
        //  delete the range and insert at the start of the range.
        //

        if (s_HexEditData.MinimumSelectedIndex !=
            s_HexEditData.MaximumSelectedIndex)
            HexEdit_DeleteRange(hWnd, 0);

        //
        //  Verify that we aren't overruning the value data buffer.
        //

        if (s_HexEditData.cbBuffer >= MAXDATA_LENGTH) {

            MessageBeep(MB_OK);
            return;

        }

        //
        //  Make room for the new byte by shifting all bytes after the insertion
        //  point down one byte.
        //

        pCaretByte = s_HexEditData.pBuffer + s_HexEditData.CaretIndex;

        MoveMemory(pCaretByte + 1, pCaretByte, s_HexEditData.cbBuffer -
            s_HexEditData.CaretIndex);

        s_HexEditData.cbBuffer++;

        HexEdit_SetScrollInfo(hWnd);

        if (s_HexEditData.Flags & HEF_CARETINASCIIDUMP)
            *pCaretByte = NewCaretByte;

        else {

            s_HexEditData.Flags |= HEF_INSERTATLOWNIBBLE;

            *pCaretByte = NewCaretByte << 4;

        }

    }

    else {

        s_HexEditData.Flags &= ~HEF_INSERTATLOWNIBBLE;

        *(s_HexEditData.pBuffer + s_HexEditData.CaretIndex) |= NewCaretByte;

    }

    PrevCaretIndex = s_HexEditData.CaretIndex;

    if (!(s_HexEditData.Flags & HEF_INSERTATLOWNIBBLE)) {

        s_HexEditData.CaretIndex++;

        s_HexEditData.MinimumSelectedIndex = s_HexEditData.CaretIndex;
        s_HexEditData.MaximumSelectedIndex = s_HexEditData.CaretIndex;

    }

UpdateDisplay:
    s_HexEditData.Flags &= ~HEF_CARETATENDOFLINE;
    HexEdit_EnsureCaretVisible(hWnd);

    UpdateRect.left = 0;
    UpdateRect.right = s_HexEditData.cxWindow;
    UpdateRect.top = (PrevCaretIndex / BYTES_PER_HEXEDIT_LINE -
        s_HexEditData.FirstVisibleLine) * s_HexEditData.FontHeight;
    UpdateRect.bottom = s_HexEditData.cyWindow;

    InvalidateRect(hWnd, &UpdateRect, FALSE);

}

/*******************************************************************************
*
*  HexEdit_SetCaretPosition
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*
*******************************************************************************/

VOID
PASCAL
HexEdit_SetCaretPosition(
    HWND hWnd
    )
{

    int CaretByte;
    int xCaret;
    int yCaret;

    CaretByte = s_HexEditData.CaretIndex % BYTES_PER_HEXEDIT_LINE;

    yCaret = (s_HexEditData.CaretIndex / BYTES_PER_HEXEDIT_LINE -
        s_HexEditData.FirstVisibleLine) * s_HexEditData.FontHeight;

    //
    //  Check if caret should really be displayed at the end of the previous
    //  line.
    //

    if (s_HexEditData.Flags & HEF_CARETATENDOFLINE) {

        CaretByte = BYTES_PER_HEXEDIT_LINE;
        yCaret -= s_HexEditData.FontHeight;

    }

    //
    //  Figure out which "column" the user is editing in and thus should have
    //  the caret.
    //

    if (s_HexEditData.Flags & HEF_CARETINASCIIDUMP) {

        xCaret = s_HexEditData.xAsciiDumpStart + CaretByte *
            s_HexEditData.FontMaxWidth;

    }

    else {

        xCaret = s_HexEditData.xHexDumpStart + CaretByte *
            s_HexEditData.xHexDumpByteWidth;

        if (s_HexEditData.Flags & HEF_INSERTATLOWNIBBLE)
            xCaret += s_HexEditData.FontMaxWidth * 3 / 2;

    }

    SetCaretPos(xCaret, yCaret);

}

/*******************************************************************************
*
*  HexEdit_EnsureCaretVisible
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*
*******************************************************************************/

VOID
PASCAL
HexEdit_EnsureCaretVisible(
    HWND hWnd
    )
{

    int CaretLine;
    int LastVisibleLine;
    int Delta;

    if (!(s_HexEditData.Flags & HEF_FOCUS))
        return;

    CaretLine = s_HexEditData.CaretIndex / BYTES_PER_HEXEDIT_LINE;

    //
    //  Check if caret should really be displayed at the end of the previous
    //  line.
    //

    if (s_HexEditData.Flags & HEF_CARETATENDOFLINE)
        CaretLine--;

    LastVisibleLine = s_HexEditData.FirstVisibleLine +
        s_HexEditData.LinesVisible - 1;

    if (CaretLine > LastVisibleLine)
        Delta = LastVisibleLine;

    else if (CaretLine < s_HexEditData.FirstVisibleLine)
        Delta = s_HexEditData.FirstVisibleLine;

    else
        Delta = -1;

    if (Delta != -1) {

        ScrollWindowEx(hWnd, 0, (Delta - CaretLine) * s_HexEditData.FontHeight,
            NULL, NULL, NULL, NULL, SW_INVALIDATE);

        s_HexEditData.FirstVisibleLine += CaretLine - Delta;

        HexEdit_SetScrollInfo(hWnd);

    }

    HexEdit_SetCaretPosition(hWnd);

}

/*******************************************************************************
*
*  HexEdit_ChangeCaretIndex
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*     NewCaretIndex,
*     fExtendSelection,
*
*******************************************************************************/

VOID
PASCAL
HexEdit_ChangeCaretIndex(
    HWND hWnd,
    int NewCaretIndex,
    BOOL fExtendSelection
    )
{

    int PrevMinimumSelectedIndex;
    int PrevMaximumSelectedIndex;
    int Swap;
    int UpdateRectCount;
    RECT UpdateRect[2];
    BOOL fPrevRangeEmpty;
    HDC hDC;
    int Index;

    s_HexEditData.Flags &= ~HEF_INSERTATLOWNIBBLE;

    PrevMinimumSelectedIndex = s_HexEditData.MinimumSelectedIndex;
    PrevMaximumSelectedIndex = s_HexEditData.MaximumSelectedIndex;

    if (fExtendSelection) {

        if (s_HexEditData.CaretIndex == s_HexEditData.MaximumSelectedIndex)
            s_HexEditData.MaximumSelectedIndex = NewCaretIndex;

        else
            s_HexEditData.MinimumSelectedIndex = NewCaretIndex;

        if (s_HexEditData.MinimumSelectedIndex >
            s_HexEditData.MaximumSelectedIndex) {

            Swap = s_HexEditData.MinimumSelectedIndex;
            s_HexEditData.MinimumSelectedIndex =
                s_HexEditData.MaximumSelectedIndex;
            s_HexEditData.MaximumSelectedIndex = Swap;

        }

    }

    else {

        s_HexEditData.MinimumSelectedIndex = NewCaretIndex;
        s_HexEditData.MaximumSelectedIndex = NewCaretIndex;

    }

    s_HexEditData.CaretIndex = NewCaretIndex;

    UpdateRectCount = 0;

    if (s_HexEditData.MinimumSelectedIndex > PrevMinimumSelectedIndex) {

        UpdateRect[0].top = PrevMinimumSelectedIndex;
        UpdateRect[0].bottom = s_HexEditData.MinimumSelectedIndex;

        UpdateRectCount++;

    }

    else if (s_HexEditData.MinimumSelectedIndex < PrevMinimumSelectedIndex) {

        UpdateRect[0].top = s_HexEditData.MinimumSelectedIndex;
        UpdateRect[0].bottom = PrevMinimumSelectedIndex;

        UpdateRectCount++;

    }

    if (s_HexEditData.MaximumSelectedIndex > PrevMaximumSelectedIndex) {

        UpdateRect[UpdateRectCount].top = PrevMaximumSelectedIndex;
        UpdateRect[UpdateRectCount].bottom = s_HexEditData.MaximumSelectedIndex;

        UpdateRectCount++;

    }

    else if (s_HexEditData.MaximumSelectedIndex < PrevMaximumSelectedIndex) {

        UpdateRect[UpdateRectCount].top = s_HexEditData.MaximumSelectedIndex;
        UpdateRect[UpdateRectCount].bottom = PrevMaximumSelectedIndex;

        UpdateRectCount++;

    }

    if (fPrevRangeEmpty = (PrevMinimumSelectedIndex ==
        PrevMaximumSelectedIndex)) {

        UpdateRect[0].top = s_HexEditData.MinimumSelectedIndex;
        UpdateRect[0].bottom = s_HexEditData.MaximumSelectedIndex;

        UpdateRectCount = 1;

    }

    if (s_HexEditData.MinimumSelectedIndex ==
        s_HexEditData.MaximumSelectedIndex) {

        if (!fPrevRangeEmpty) {

            UpdateRect[0].top = PrevMinimumSelectedIndex;
            UpdateRect[0].bottom = PrevMaximumSelectedIndex;

            UpdateRectCount = 1;

        }

        else
            UpdateRectCount = 0;

    }

    if (UpdateRectCount) {

        HideCaret(hWnd);

        hDC = GetDC(hWnd);

        for (Index = 0; Index < UpdateRectCount; Index++) {

            UpdateRect[Index].top = (UpdateRect[Index].top /
                BYTES_PER_HEXEDIT_LINE - s_HexEditData.FirstVisibleLine) *
                s_HexEditData.FontHeight;
            UpdateRect[Index].bottom = (UpdateRect[Index].bottom /
                BYTES_PER_HEXEDIT_LINE - s_HexEditData.FirstVisibleLine + 1) *
                s_HexEditData.FontHeight;

            if (UpdateRect[Index].top >= s_HexEditData.cyWindow ||
                UpdateRect[Index].bottom < 0)
                continue;

            if (UpdateRect[Index].top < 0)
                UpdateRect[Index].top = 0;

            if (UpdateRect[Index].bottom > s_HexEditData.cyWindow)
                UpdateRect[Index].bottom = s_HexEditData.cyWindow;

            HexEdit_PaintRect(hDC, &UpdateRect[Index]);

        }

        ReleaseDC(hWnd, hDC);

        ShowCaret(hWnd);

    }


    HexEdit_EnsureCaretVisible(hWnd);

}

/*******************************************************************************
*
*  HexEdit_DeleteRange
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
HexEdit_DeleteRange(
    HWND hWnd,
    UINT SourceKey
    )
{

    int MinimumSelectedIndex;
    int MaximumSelectedIndex;
    PBYTE pMinimumSelectedByte;
    int Length;
    RECT UpdateRect;

    s_HexEditData.Flags &= ~HEF_CARETATENDOFLINE;

    MinimumSelectedIndex = s_HexEditData.MinimumSelectedIndex;
    MaximumSelectedIndex = s_HexEditData.MaximumSelectedIndex;

    //
    //  Check to see if a range is selected.  If not, then artificially create
    //  one based on the key that caused this routine to be called.
    //

    if (MinimumSelectedIndex == MaximumSelectedIndex) {

        if (SourceKey == VK_DELETE || s_HexEditData.Flags &
            HEF_INSERTATLOWNIBBLE) {

            s_HexEditData.Flags &= ~HEF_INSERTATLOWNIBBLE;

            MaximumSelectedIndex++;

            if (MaximumSelectedIndex > s_HexEditData.cbBuffer)
                return;

        }

        else if (SourceKey == VK_BACK) {

            MinimumSelectedIndex--;

            if (MinimumSelectedIndex < 0)
                return;

        }

        else
            return;

    }

    //
    //  Compute where to start deleting from and the number of bytes to delete.
    //

    pMinimumSelectedByte = s_HexEditData.pBuffer + MinimumSelectedIndex;

    Length = MaximumSelectedIndex - MinimumSelectedIndex;

    //
    //  Delete the bytes and update all appropriate window data.
    //

    MoveMemory(pMinimumSelectedByte, pMinimumSelectedByte + Length,
        s_HexEditData.cbBuffer - MaximumSelectedIndex);

    s_HexEditData.cbBuffer -= Length;

    s_HexEditData.CaretIndex = MinimumSelectedIndex;
    s_HexEditData.MinimumSelectedIndex = MinimumSelectedIndex;
    s_HexEditData.MaximumSelectedIndex = MinimumSelectedIndex;

    HexEdit_SetScrollInfo(hWnd);

    //  BUGBUG:  OnChar has the following same sequence!!!
    HexEdit_EnsureCaretVisible(hWnd);

    UpdateRect.left = 0;
    UpdateRect.right = s_HexEditData.cxWindow;
    UpdateRect.top = (MinimumSelectedIndex / BYTES_PER_HEXEDIT_LINE -
        s_HexEditData.FirstVisibleLine) * s_HexEditData.FontHeight;
    UpdateRect.bottom = s_HexEditData.cyWindow;

    InvalidateRect(hWnd, &UpdateRect, FALSE);

}

/*******************************************************************************
*
*  HexEdit_OnCopy
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*
*******************************************************************************/

BOOL
PASCAL
HexEdit_OnCopy(
    HWND hWnd
    )
{

    BOOL fSuccess;
    int cbClipboardData;
    LPBYTE lpStartByte;
    HANDLE hClipboardData;
    LPHEXEDITCLIPBOARDDATA lpClipboardData;

    fSuccess = FALSE;

    cbClipboardData = s_HexEditData.MaximumSelectedIndex -
        s_HexEditData.MinimumSelectedIndex;

    if (cbClipboardData != 0) {

        lpStartByte = s_HexEditData.pBuffer +
            s_HexEditData.MinimumSelectedIndex;

        if (OpenClipboard(hWnd)) {

            if ((hClipboardData = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE,
                cbClipboardData + sizeof(HEXEDITCLIPBOARDDATA) - 1)) != NULL) {

                lpClipboardData = (LPHEXEDITCLIPBOARDDATA)
                    GlobalLock(hClipboardData);
                CopyMemory(lpClipboardData-> Data, lpStartByte,
                    cbClipboardData);
                lpClipboardData-> cbSize = cbClipboardData;
                GlobalUnlock(hClipboardData);

                EmptyClipboard();
                SetClipboardData(s_HexEditClipboardFormat, hClipboardData);

                fSuccess = TRUE;

            }

            CloseClipboard();

        }

    }

    return fSuccess;

}

/*******************************************************************************
*
*  HexEdit_OnPaste
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*
*******************************************************************************/

BOOL
PASCAL
HexEdit_OnPaste(
    HWND hWnd
    )
{

    BOOL fSuccess;
    HANDLE hClipboardData;
    LPHEXEDITCLIPBOARDDATA lpClipboardData;
    PBYTE pCaretByte;
    DWORD cbSize;

    fSuccess = FALSE;

    if (s_HexEditData.Flags & HEF_INSERTATLOWNIBBLE) {

        s_HexEditData.Flags &= ~HEF_INSERTATLOWNIBBLE;
        s_HexEditData.CaretIndex++;

    }

    if (OpenClipboard(hWnd)) {

        if ((hClipboardData = GetClipboardData(s_HexEditClipboardFormat)) !=
            NULL) {

            lpClipboardData = (LPHEXEDITCLIPBOARDDATA)
                GlobalLock(hClipboardData);

            if (s_HexEditData.cbBuffer + lpClipboardData-> cbSize <=
                MAXDATA_LENGTH) {

                if (s_HexEditData.MinimumSelectedIndex !=
                    s_HexEditData.MaximumSelectedIndex)
                    HexEdit_DeleteRange(hWnd, VK_BACK);

                //
                //  Make room for the new bytes by shifting all bytes after the
                //  the insertion point down the necessary amount.
                //

                pCaretByte = s_HexEditData.pBuffer + s_HexEditData.CaretIndex;
                cbSize = lpClipboardData-> cbSize;

                MoveMemory(pCaretByte + cbSize, pCaretByte,
                    s_HexEditData.cbBuffer - s_HexEditData.CaretIndex);
                CopyMemory(pCaretByte, lpClipboardData-> Data, cbSize);

                s_HexEditData.cbBuffer += cbSize;
                s_HexEditData.CaretIndex += cbSize;

                HexEdit_SetScrollInfo(hWnd);

                fSuccess = TRUE;

            }

            GlobalUnlock(hClipboardData);

        }

        CloseClipboard();

    }

    return fSuccess;

}

/*******************************************************************************
*
*  HexEdit_OnContextMenu
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*     x, horizontal position of the cursor.
*     y, vertical position of the cursor.
*
*******************************************************************************/

VOID
PASCAL
HexEdit_OnContextMenu(
    HWND hWnd,
    int x,
    int y
    )
{

    HMENU hContextMenu;
    HMENU hContextPopupMenu;
    int MenuCommand;

    //
    //  Give us the focus if we don't already have it.
    //

    if (!(s_HexEditData.Flags & HEF_FOCUS))
        SetFocus(hWnd);

    //
    //  Load the HexEdit context menu from our resources.
    //

    if ((hContextMenu = LoadMenu(g_hInstance,
        MAKEINTRESOURCE(IDM_HEXEDIT_CONTEXT))) == NULL)
        return;

    hContextPopupMenu = GetSubMenu(hContextMenu, 0);

    //
    //  Disable editing menu options as appropriate.
    //

    if (s_HexEditData.MinimumSelectedIndex ==
        s_HexEditData.MaximumSelectedIndex) {

        EnableMenuItem(hContextPopupMenu, IDKEY_COPY, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(hContextPopupMenu, IDKEY_CUT, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(hContextPopupMenu, VK_DELETE, MF_BYCOMMAND | MF_GRAYED);

    }

    if (!IsClipboardFormatAvailable(s_HexEditClipboardFormat))
        EnableMenuItem(hContextPopupMenu, IDKEY_PASTE, MF_BYCOMMAND |
            MF_GRAYED);

    if (s_HexEditData.MinimumSelectedIndex == 0 &&
        s_HexEditData.MaximumSelectedIndex == s_HexEditData.cbBuffer)
        EnableMenuItem(hContextPopupMenu, ID_SELECTALL, MF_BYCOMMAND |
            MF_GRAYED);

    //
    //  Display and handle the selected command.
    //

    MenuCommand = TrackPopupMenuEx(hContextPopupMenu, TPM_RETURNCMD |
        TPM_RIGHTBUTTON | TPM_LEFTALIGN | TPM_TOPALIGN, x, y, hWnd, NULL);

    DestroyMenu(hContextMenu);

    switch (MenuCommand) {

        case IDKEY_COPY:
        case IDKEY_PASTE:
        case IDKEY_CUT:
        case VK_DELETE:
            HexEdit_OnChar(hWnd, (TCHAR) MenuCommand, 0);
            break;

        case ID_SELECTALL:
            s_HexEditData.MinimumSelectedIndex = 0;
            s_HexEditData.MaximumSelectedIndex = s_HexEditData.cbBuffer;
            s_HexEditData.CaretIndex = s_HexEditData.cbBuffer;
            HexEdit_SetCaretPosition(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);
            break;

    }

}
