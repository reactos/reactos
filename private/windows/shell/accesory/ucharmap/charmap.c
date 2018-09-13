/****************************************************************************

    PROGRAM: CharMap

    PURPOSE: Utility providing users an easy interface for selecting special
             characters.

    COMMENTS:
        Created by MikeSch (7-16-91)
        Partially derived from WinWord 2.0 Insert.Symbol dialog.

****************************************************************************/

#define WIN31
#include "windows.h"
#include <port1632.h>
#include "charmap.h"
#include "stdlib.h"
#include "tchar.h"
#ifdef UNICODE
#include "wchar.h"
#else
#include "stdio.h"
#endif
#include "commctrl.h"

/*
 * Macros
 */
#define FMagData(psycm) ((psycm)->xpMagCurr != 0)
#define abs(x) (((x) >= 0) ? (x) : (-(x)))

/*
 * Useful constants.
 */
#define STATUSPOINTSIZE          8      // Point size of status bar font.
#define DX_BITMAP               20      // Width of TT bitmap.
#define DY_BITMAP               12      // Height of TT bitmap.
#define BACKGROUND      0x000000FF      // bright blue
#define BACKGROUNDSEL   0x00FF00FF      // bright purple
#define BUTTONFACE      0x00C0C0C0      // bright grey
#define BUTTONSHADOW    0x00808080      // dark grey

// Font types
#define PS_OPENTYPE_FONT    0x0001
#define TT_OPENTYPE_FONT    0x0002
#define TRUETYPE_FONT       0x0004
#define TYPE1_FONT          0x0008

/*
 * Globals.
 */
HANDLE hInst;
INT cchSymRow = 32;          // Number of characters across the character grid.
INT cchSymCol = 8;           // Number of rows in the character grid.
UTCHAR chSymFirst = 32;
UTCHAR chSymLast  = 255;
SYCM sycm;                   // Tons of data need to do char grid painting.
WORD wCFRichText = 0;        // Private clipboard format, rich text format.
HFONT hFontClipboard = NULL; // Tells us which font is in the clipboard.
HANDLE hstrClipboard = NULL; // Contains the string which is in the clipboard.
BOOL fDelClipboardFont = FALSE; // The clipboard font needs to be deleted.
INT iControl = ID_CHARGRID;  // Index indicating which control has focus.
HBITMAP hbmFont = NULL;      // TT bitmap drawn before font facenames in combo.
LONG lEditSel = 0;           // Contains the selection range of the EC.
HBRUSH hStaticBrush;         // Used for static controls during WM_CTLCOLOR

//
// BUGBUG  04 Dec 92 - GregoryW
//    Currently there is no defined
//    interface for querying what character ranges a Unicode font
//    supports.  For now this table only has the subsets that contain
//    characters supported by the Lucida Sans Unicode font uncommented.
//    When we get an API that allows querying the font driver for
//    ranges of Unicode characters supported (and whether or not a font
//    is a Unicode font!) then all entries can be uncommented.
//
USUBSET aSubsetData[] = {{ 0x0020, 0x00ff, IDS_LATIN1},
                       { 0x0100, 0x017f, IDS_LATINEXA},
                       { 0x0180, 0x024f, IDS_LATINEXB},
                       { 0x0250, 0x02af, IDS_IPAEX},
                       { 0x02b0, 0x02ff, IDS_SPACINGMODIFIERS},
                       { 0x0300, 0x036f, IDS_COMBININGDIACRITICS},
                       { 0x0370, 0x03cf, IDS_BASICGREEK},
                       { 0x03d0, 0x03ff, IDS_GREEKSYMBOLS},
                       { 0x0400, 0x04ff, IDS_CYRILLIC},
// not supported       { 0x0530, 0x058f, IDS_ARMENIAN},
                       { 0x0590, 0x05ff, IDS_HEBREW},
// not supported       { 0x0600, 0x0652, IDS_BASICARABIC},
// not supported       { 0x0653, 0x06ff, IDS_ARABICEX},
// not supported       { 0x0900, 0x097f, IDS_DEVANAGARI},
// not supported       { 0x0980, 0x09ff, IDS_BENGALI},
// not supported       { 0x0a00, 0x0a7f, IDS_GURMUKHI},
// not supported       { 0x0a80, 0x0aff, IDS_GUJARATI},
// not supported       { 0x0b00, 0x0b7f, IDS_ORIYA},
// not supported       { 0x0b80, 0x0bff, IDS_TAMIL},
// not supported       { 0x0c00, 0x0c7f, IDS_TELUGU},
// not supported       { 0x0c80, 0x0cff, IDS_KANNADA},
// not supported       { 0x0d00, 0x0d7f, IDS_MALAYALAM},
// not supported       { 0x0e00, 0x0e7f, IDS_THAI},
// not supported       { 0x0e80, 0x0eff, IDS_LAO},
// not supported       { 0x10d0, 0x10ff, IDS_BASICGEORGIAN},
// not supported       { 0x10a0, 0x10cf, IDS_GEORGIANEX},
// not supported       { 0x1100, 0x11ff, IDS_HANGULJAMO},
// not supported       { 0x1e00, 0x1eff, IDS_LATINEXADDITIONAL},
// not supported       { 0x1f00, 0x1fff, IDS_GREEKEX},
// not supported       { 0x2000, 0x206f, IDS_GENERALPUNCTUATION},
// not supported       { 0x2070, 0x209f, IDS_SUPERANDSUBSCRIPTS},
                       { 0x20a0, 0x20cf, IDS_CURRENCYSYMBOLS},
// not supported       { 0x20d0, 0x20ff, IDS_COMBININGDIACRITICSFORSYMBOLS},
                       { 0x2100, 0x214f, IDS_LETTERLIKESYMBOLS},
// not supported       { 0x2150, 0x218f, IDS_NUMBERFORMS},
                       { 0x2190, 0x21ff, IDS_ARROWS},
                       { 0x2200, 0x22ff, IDS_MATHEMATICALOPS},
// not supported       { 0x2300, 0x23ff, IDS_MISCTECHNICAL},
// not supported       { 0x2400, 0x243f, IDS_CONTROLPICTURES},
// not supported       { 0x2440, 0x245f, IDS_OPTICALCHAR},
// not supported       { 0x2460, 0x24ff, IDS_ENCLOSEDALPHANUM},
// not supported       { 0x2500, 0x257f, IDS_BOXDRAWING},
// not supported       { 0x2580, 0x259f, IDS_BLOCKELEMENTS},
// not supported       { 0x25a0, 0x25ff, IDS_GEOMETRICSHAPES},
// not supported       { 0x2600, 0x26ff, IDS_MISCDINGBATS},
// not supported       { 0x2700, 0x27bf, IDS_DINGBATS},
// not supported       { 0x3000, 0x303f, IDS_CJKSYMBOLSANDPUNC},
// not supported       { 0x3040, 0x309f, IDS_HIRAGANA},
// not supported       { 0x30a0, 0x30ff, IDS_KATAKANA},
// not supported       { 0x3100, 0x312f, IDS_BOPOMOFO},
// not supported       { 0x3130, 0x318f, IDS_HANGULCOMPATIBILITYJAMO},
// not supported       { 0x3190, 0x319f, IDS_CJKMISC},
// not supported       { 0x3200, 0x32ff, IDS_ENCLOSEDCJKLETTERSANDMONTHS},
// not supported       { 0x3300, 0x33ff, IDS_CJKCOMPATIBILITY},
// not supported       { 0x3400, 0x3d2d, IDS_HANGUL},
// not supported       { 0x3d2e, 0x44b7, IDS_HANGULSUPPA},
// not supported       { 0x44b8, 0x4dff, IDS_HANGULSUPPB},
// not supported       { 0x4e00, 0x9fff, IDS_CJKUNIFIEDIDEOGRAPHS},
// not supported       { 0xe000, 0xf8ff, IDS_PRIVATEUSEAREA},
// not supported       { 0xf900, 0xfaff, IDS_CJKCOMPATIBILITYIDEOGRAPHS},
// not supported       { 0xfb00, 0xfb4f, IDS_ALPAHPRESENTATIONFORMS},
// not supported       { 0xfb50, 0xfdff, IDS_ARABICPRESENTATIONFORMSA},
// not supported       { 0xfe30, 0xfe4f, IDS_CJKCOMPFORMS},
// not supported       { 0xfe50, 0xfe6f, IDS_SMALLFORMVARIANTS},
// not supported       { 0xfe70, 0xfefe, IDS_ARABICPRESENTATIONFORMSB},
// not supported       { 0xff00, 0xffef, IDS_HALFANDFULLWIDTHFORMS},
// not supported       { 0xfff0, 0xfffd, IDS_SPECIALS}
                       };
INT cSubsets = sizeof(aSubsetData) / sizeof(USUBSET);
INT iCurSubset = 0;    // index of current Unicode subset - default to Latin-1

// Useful window handles.
HWND hwndDialog;
HWND hwndCharGrid;

// Data used to draw the status bar.
RECT rcStatusLine;                        // Bounding rect for status bar.
RECT rcToolbar[2];                        // Bounding rects for toolbars.
INT dyStatus;                             // Height of status bar.
INT dyToolbar[2];                         // Height of tool bars.
INT dxHelpField;                          // Width of help window.
INT dxKeystrokeField;                     // Width of keystroke window.
TCHAR szKeystrokeText[30];                // Buffer for keystroke text.
TCHAR szKeystrokeLabel[30];               // Buffer for keystroke label.
TCHAR szSpace[15];                        // Strings for keystroke description.
TCHAR szCtrl[15];
TCHAR szCtrlAlt[25];
TCHAR szShiftCtrlAlt[25];
TCHAR szAlt[15];
TCHAR szUnicodeLabel[23];                 // Buffer for Unicode label.
INT iKeystrokeTextStart;                  // Place to start appending text to above.
INT iUnicodeLabelStart;                   // Place to start appending text to above.
HFONT hfontStatus;                        // Font used for text of status bar.

/****************************************************************************

    FUNCTION: WinMain(HANDLE, HANDLE, LPSTR, int)

    PURPOSE: calls initialization function, processes message loop, cleanup.

    COMMENTS:

****************************************************************************/

INT PASCAL WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    MSG msg;

    if (!InitApplication(hInstance))
            return (FALSE);

    // Perform initialization for this instance.
    if (!InitInstance(hInstance, nCmdShow)) {
        return (FALSE);
    }

    while (GetMessage(&msg,
            NULL,
            0,
            0))
    {
        // Filter for possible tabs now to implement context sensitive help.
        if (msg.message == WM_KEYDOWN)
            if (!UpdateHelpText(&msg, NULL))
                continue;

        // Main message loop.
        if (!IsDialogMessage(hwndDialog, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // Free up some stuff.
    if (hfontStatus)
        DeleteObject(hfontStatus);
    if (hbmFont)
        DeleteObject(hbmFont);

    return (msg.wParam);
}


/****************************************************************************

    FUNCTION: InitApplication(HANDLE)

    PURPOSE: Initializes window data and registers window class

    COMMENTS:

****************************************************************************/

BOOL InitApplication(HANDLE hInstance)
{
    WNDCLASS  wc;

    /*
     * Register a window class that we will use to draw the character grid
     * into.
     */
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = CharGridWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName =  NULL;
    wc.lpszClassName = TEXT("CharGridWClass");

    if (!RegisterClass(&wc))
        return (FALSE);

    wc.style = 0;
    wc.lpfnWndProc = DefDlgProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = DLGWINDOWEXTRA;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDIC_CHARMAP));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName =  NULL;
    wc.lpszClassName = TEXT("MyDlgClass");

    if (!RegisterClass(&wc))
        return (FALSE);
}

/****************************************************************************

    FUNCTION:  InitInstance(HANDLE, int)

    PURPOSE:  Does some initialization and creates main window which is a
              dialog.

    COMMENTS:

****************************************************************************/

BOOL InitInstance(HANDLE hInstance, INT nCmdShow)
{
    INT i;

    // Save the instance handle in a global variable.
    hInst = hInstance;

    // This font will be used to paint the status line.
    hfontStatus = CreateFont(-PointsToHeight(STATUSPOINTSIZE), 0, 0, 0, 400, 0, 0, 0,
                     ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                     DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, TEXT("Helv"));
    dyStatus = 2 * PointsToHeight(STATUSPOINTSIZE);
    dyToolbar[0] = 28;  /* defined by UI gods */
    dyToolbar[1] = 28;  /* defined by UI gods */

    // Load the Unicode subset names before initializing the main window.
    for (i = 0; i < cSubsets; i++) {
        if (!LoadString(
             hInst,
             aSubsetData[i].StringResId,
             (LPTSTR)aSubsetData[i].Name,
             50)
             )
            return (FALSE);
    }

    // Create a main window for this application instance.
    if (!(hwndDialog = CreateDialog(hInstance, TEXT("CharMap"), NULL,
                              (DLGPROC)CharMapWndProc)))
        return (FALSE);

    /*
     * Initialize some strings used for the Keystroke status bar field.
     */
    // For international purposes, this string could be length 0.
    LoadString(
        hInst,
        IDS_KEYSTROKE,
        (LPTSTR)szKeystrokeLabel,
        BTOC(sizeof(szKeystrokeLabel))
        );
    if (!LoadString(
             hInst,
             IDS_UNICODELABEL,
             (LPTSTR)szUnicodeLabel,
             BTOC(sizeof(szUnicodeLabel))
             ))
    if (!LoadString(
             hInst,
             IDS_SPACE,
             (LPTSTR)szSpace,
             BTOC(sizeof(szSpace))
             ))
        return (FALSE);
    if (!LoadString(
             hInst,
             IDS_CTRL,
             (LPTSTR)szCtrl,
             BTOC(sizeof(szCtrl))
             ))
        return (FALSE);
    if (!LoadString(
             hInst,
             IDS_CTRLALT,
             (LPTSTR)szCtrlAlt,
             BTOC(sizeof(szCtrlAlt))
             ))
        return (FALSE);
    if (!LoadString(
             hInst,
             IDS_SHIFTCTRLALT,
             (LPTSTR)szShiftCtrlAlt,
             BTOC(sizeof(szShiftCtrlAlt))
             ))
        return (FALSE);
    if (!LoadString(
             hInst,
             IDS_ALT,
             (LPTSTR)szAlt,
             BTOC(sizeof(szAlt))
             ))
        return (FALSE);

    // Store the index to where we start adding status line text changes.
    iKeystrokeTextStart = lstrlen(szKeystrokeLabel);
    iUnicodeLabelStart = lstrlen(szUnicodeLabel);

    /*
     * Initialize keystroke text, make the window visible,
     * update its client area, and return "success".
     */
    UpdateKeystrokeText(NULL, sycm.chCurr, FALSE);
    ShowWindow(hwndDialog, nCmdShow);
    UpdateWindow(hwndDialog);
    return (TRUE);

}

/****************************************************************************

    FUNCTION: CharMapWndProc(HWND, UINT, WPARAM, LPARAM)

    PURPOSE:  Processes messages for the main window.

    COMMENTS: This window is a dialog box.

****************************************************************************/

LONG  APIENTRY CharMapWndProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message) {
        case WM_CTLCOLORSTATIC:
            {
            POINT point;

            SetBkColor((HDC)wParam, GetSysColor(COLOR_BTNFACE));
            UnrealizeObject(hStaticBrush);
            point.x = point.y = 0;
            ClientToScreen(hWnd, &point);

            return((DWORD)hStaticBrush);
            break;
            }

        case WM_INITDIALOG:
            {
            RECT rectParent, rectTopRightControl;
            INT iSubset;

            /*
             * Create the character grid with dimensions which just fit inside
             * the space allowed in the dialog.  When it processes the
             * WM_CREATE message it will be sized and centered more accurately.
             */
            GetClientRect(hWnd, &rectParent);
            GetWindowRect(GetDlgItem(hWnd, ID_CLOSE), &rectTopRightControl);
            ScreenToClient(hWnd, (LPPOINT)&(rectTopRightControl.left));
            ScreenToClient(hWnd, (LPPOINT)&(rectTopRightControl.right));

            if (!(hwndCharGrid =
                CreateWindow(
                    TEXT("CharGridWClass"),
                    NULL,
                    WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                    1,
                    rectParent.top + dyToolbar[0] + dyToolbar[1],
                    rectParent.right - 1,
                    rectParent.bottom - rectParent.top - dyStatus - dyToolbar[0] - dyToolbar[1] - 1,
                    hWnd,
                    (HMENU) ID_CHARGRID,
                    hInst,
                    NULL
                    ))) {
                DestroyWindow(hWnd);
                break;
            }

            hStaticBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));

            // Initialize the status line data.
            dxHelpField = 22 * rectParent.right / 32;
            dxKeystrokeField = 8 * rectParent.right / 32;
            rcStatusLine = rectParent;
            rcStatusLine.top = rcStatusLine.bottom - dyStatus;

            // Initialize the toolbars
            rcToolbar[0] = rectParent;
            rcToolbar[0].bottom = rcToolbar[0].top + dyToolbar[0];

            rcToolbar[1] = rcToolbar[0];
            rcToolbar[1].top = rcToolbar[0].bottom + GetSystemMetrics(SM_CYBORDER);
            rcToolbar[1].bottom = rcToolbar[1].top + dyToolbar[1];

            // Disable Copy button.
            EnableWindow(GetDlgItem(hWnd, ID_COPY), FALSE);

            /* fill "Subset" list box */
            for (iSubset = 0; iSubset < cSubsets; iSubset++) {
                SendDlgItemMessage(
                    hWnd,
                    ID_UNICODESUBSET,
                    CB_ADDSTRING,
                    0,
                    (DWORD)aSubsetData[iSubset].Name
                    );

            }
            iCurSubset = SelectInitialSubset(hWnd);

            }

            /* fall through to WM_FONTCHANGE */
        case WM_FONTCHANGE:
            {
            HDC hdc = GetDC(hWnd);

            /*
             * Get the fonts from the system and put them in the font selection
             * combo box.
             */
            if (message == WM_FONTCHANGE) {
                SaveCurrentFont(hWnd);
                SendDlgItemMessage(hWnd, ID_FONT, CB_RESETCONTENT, 0, 0L);
            }

            EnumFontFamilies(hdc, NULL, (FONTENUMPROC)FontLoadProc, (LPARAM)hWnd);

            ReleaseDC(hWnd, hdc);

            // Setup character dimensions and select this font.
            RecalcCharMap(hWnd, &sycm, SelectInitialFont(hWnd),
                          (message == WM_FONTCHANGE));
            SendDlgItemMessage(hWnd, ID_STRING, WM_SETFONT,
                               (WPARAM)sycm.hFont, (DWORD)TRUE);

            if (message == WM_INITDIALOG)
                SetFocus(hwndCharGrid);
                /* fall through to WM_SYSCOLORCHANGE */
            else
                break;
            }

        case WM_SYSCOLORCHANGE:
            if (hbmFont)
                DeleteObject(hbmFont);
            hbmFont = LoadBitmaps(IDBM_TT);
            DeleteObject(hStaticBrush);
            hStaticBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
            break;

        case WM_PARENTNOTIFY:
            {
            POINTS points;
            DWORD dwMsgPos;
            POINT point;

            /*
             * We process this message to implement the context sensitive
             * help.  Downclicks to controls are found here, the help
             * message is updated in the status bar.
             */
            // BUG - The parameters with this message are unreliable!
            if (wParam == WM_LBUTTONDOWN) {
                dwMsgPos = GetMessagePos();
                points = MAKEPOINTS(dwMsgPos);
                point.x = points.x;
                point.y = points.y;
                UpdateHelpText(NULL, WindowFromPoint(point));
            }

            }
            break;


    case WM_PAINT:

            {
            HBRUSH hBrush;
            RECT rcTemp, rectNextButton;
            INT dyBorder, dxBorder;
            PAINTSTRUCT ps;
            HDC hdc;

            /*
             * This code implements painting of the status bar.
             */
            hdc = BeginPaint(hWnd, &ps);

            rcTemp = rcStatusLine;

            dyBorder = GetSystemMetrics(SM_CYBORDER);
            dxBorder = GetSystemMetrics(SM_CXBORDER);

            // Make the whole thing grey.
              if (hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE))) {
                FillRect(hdc, &rcTemp, hBrush);
                rcTemp.left = rcToolbar[0].left;
                rcTemp.top = rcToolbar[0].top;
                rcTemp.right = rcToolbar[1].right;
                rcTemp.bottom = rcToolbar[1].bottom;
                FillRect(hdc, &rcTemp, hBrush);
                DeleteObject(hBrush);
              }

              GetWindowRect(GetDlgItem(hWnd, ID_TOPLEFT), &rectNextButton);
              ScreenToClient(hWnd, (LPPOINT)&(rectNextButton.left));
              ScreenToClient(hWnd, (LPPOINT)&(rectNextButton.right));
              // solid black line across bottom of toolbar
              if (hBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOWFRAME))) {
                rcTemp = rcToolbar[0];
                rcTemp.top = rcTemp.bottom;
                rcTemp.bottom += dyBorder;
                rcTemp.left = rectNextButton.left - 2 - dxBorder;
                FillRect(hdc, &rcTemp, hBrush);
                rcTemp = rcToolbar[1];
                rcTemp.top = rcTemp.bottom;
                rcTemp.bottom += dyBorder;
                FillRect(hdc, &rcTemp, hBrush);

                // vertical line
                rcTemp.top = rcToolbar[0].top;
                rcTemp.bottom = rcToolbar[1].bottom;
                rcTemp.left = rectNextButton.left - 2 - dxBorder;
                rcTemp.right = rectNextButton.left - 2;
                FillRect(hdc, &rcTemp, hBrush);
                DeleteObject(hBrush);
              }

            if (hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNSHADOW))) {

                // Status line top.
                rcTemp.left   = 8 * dyBorder;
                rcTemp.right  = rcTemp.left + dxHelpField;
                rcTemp.top    = rcStatusLine.top + dyBorder * 2;
                rcTemp.bottom = rcTemp.top + dyBorder;
                FillRect(hdc, &rcTemp, hBrush);

                // Keystroke line top.
                rcTemp.right = rcStatusLine.right - 8 * dyBorder;
                rcTemp.left = rcTemp.right - dxKeystrokeField;
                FillRect(hdc, &rcTemp, hBrush);

                // Status line left side.
                rcTemp = rcStatusLine;
                rcTemp.left = 8 * dyBorder;
                rcTemp.right = rcTemp.left + dyBorder;
                rcTemp.top += dyBorder * 2;
                rcTemp.bottom -= dyBorder * 2;
                FillRect(hdc, &rcTemp, hBrush);

                // Keystroke line left side.
                rcTemp.left = rcStatusLine.right - 9 * dyBorder - dxKeystrokeField;
                rcTemp.right = rcTemp.left + dyBorder;
                FillRect(hdc, &rcTemp, hBrush);

                DeleteObject(hBrush);
            }

            if (hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNHIGHLIGHT))) {

                // Status line bottom.
                rcTemp.left   = 8 * dyBorder;
                rcTemp.right  = rcTemp.left + dxHelpField;
                rcTemp.top    = rcStatusLine.bottom - 3 * dyBorder;
                rcTemp.bottom = rcTemp.top + dyBorder;
                FillRect(hdc, &rcTemp, hBrush);

                // Keystroke line bottom.
                rcTemp.right = rcStatusLine.right - 8 * dyBorder;
                rcTemp.left = rcTemp.right - dxKeystrokeField;
                FillRect(hdc, &rcTemp, hBrush);

                // Status line right side.
                rcTemp = rcStatusLine;
                rcTemp.left = 8 * dyBorder + dxHelpField;
                rcTemp.right = rcTemp.left + dyBorder;
                rcTemp.top += dyBorder * 2;
                rcTemp.bottom -= dyBorder * 2;
                FillRect(hdc, &rcTemp, hBrush);

                // Keystroke line right side.
                rcTemp.left = rcStatusLine.right - 8 * dyBorder;
                rcTemp.right = rcTemp.left + dyBorder;
                FillRect(hdc, &rcTemp, hBrush);

                DeleteObject(hBrush);
            }

            // solid black line across top

            if (hBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOWFRAME))) {
            rcTemp = rcStatusLine;
            rcTemp.bottom = rcTemp.top;
            rcTemp.top -= dyBorder;
            FillRect(hdc, &rcTemp, hBrush);
            DeleteObject(hBrush);
            }

            PaintStatusLine(hdc, TRUE, TRUE);

            EndPaint(hWnd, &ps);

            return (TRUE);
            }

        case WM_MEASUREITEM:
            {
            HDC hDC;
            HFONT hFont;
            TEXTMETRIC tm;

            hDC = GetDC(NULL);
            hFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0L);
            if (hFont)
                hFont = SelectObject(hDC, hFont);
            GetTextMetrics(hDC, &tm);
            if (hFont)
                SelectObject(hDC, hFont);
            ReleaseDC(NULL, hDC);

            ((LPMEASUREITEMSTRUCT)lParam)->itemHeight = max(tm.tmHeight, DY_BITMAP);
            break;
            }

        case WM_DRAWITEM:
            if (((LPDRAWITEMSTRUCT)lParam)->itemID != -1)
                DrawFamilyComboItem((LPDRAWITEMSTRUCT)lParam);

            break;

        case WM_ASKCBFORMATNAME:
            LoadString(hInst, IDS_RTF, (LPTSTR)lParam, wParam);

            return (TRUE);

        case WM_PAINTCLIPBOARD:
            {
            LPPAINTSTRUCT lpPS;
            HANDLE hFont;
            LPTSTR lpstrText;

            if (hstrClipboard) {
                // Setup.
                lpPS = (LPPAINTSTRUCT)GlobalLock((HANDLE)lParam);
                lpstrText = (LPTSTR)GlobalLock(hstrClipboard);

                // Lets paint.
                hFont = SelectObject(lpPS->hdc, hFontClipboard);
                TextOut(lpPS->hdc, 0, 0, lpstrText,
                        lstrlen(lpstrText));
                SelectObject(lpPS->hdc, hFont);

                // Cleanup.
                GlobalUnlock(hstrClipboard);
                GlobalUnlock((HANDLE)lParam);
            }

            return (TRUE);
            }

        case WM_CLOSE:
            DestroyWindow(hWnd);
            return (TRUE);

        case WM_COMMAND:
            switch(GET_WM_COMMAND_ID(wParam, lParam)) {

                case IDCANCEL:
                case ID_CLOSE:
                    DestroyWindow(hWnd);
                    return (TRUE);
                    break;

                case ID_SELECT:
                    SendDlgItemMessage(hWnd, ID_STRING, WM_CHAR,
                                       (WPARAM)sycm.chCurr, 0L);
                    break;

                case ID_COPY:
                    CopyString(hWnd);
                    return (TRUE);
                    break;

                case ID_FONT:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        RecalcCharMap(hWnd, &sycm,
                                      (INT)SendDlgItemMessage(hWnd, ID_FONT,
                                                              CB_GETCURSEL, 0, 0L),
                                      TRUE);
                        SendDlgItemMessage(hWnd, ID_STRING, WM_SETFONT,
                                           (WPARAM)sycm.hFont, (DWORD)TRUE);
                    } else if (HIWORD(wParam) == CBN_SETFOCUS) {
                        // Necessary if hotkey is used to get to the CB.
                        UpdateHelpText(NULL, (HWND)lParam);
                    }

                    return (TRUE);
                    break;

                case ID_UNICODESUBSET:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        INT iSubset;

                        iSubset = (INT)SendDlgItemMessage(
                                          hWnd,
                                          ID_UNICODESUBSET,
                                          CB_GETCURSEL,
                                          0, 0);
                        UpdateSymbolSelection(
                            hWnd,
                            aSubsetData[iSubset].BeginRange,
                            aSubsetData[iSubset].EndRange
                            );
                        InvalidateRect(hwndCharGrid, NULL, TRUE);
                    } else if (HIWORD(wParam) == CBN_SETFOCUS) {
                        // Necessary if hotkey is used to get to the CB.
                        UpdateHelpText(NULL, (HWND)lParam);
                    }
                    return 0;
                    break;

                case ID_NEXTSUBSET:
                    {
                        INT iCurSelection, iNumEntries;

                        iCurSelection = (INT)SendDlgItemMessage(
                                                 hWnd,
                                                 ID_UNICODESUBSET,
                                                 CB_GETCURSEL,
                                                 0, 0);
                        if (iCurSelection == CB_ERR) {
                             return 0;
                        }
                        iNumEntries = (INT)SendDlgItemMessage(
                                               hWnd,
                                               ID_UNICODESUBSET,
                                               CB_GETCOUNT,
                                               0, 0);
                        if (iNumEntries == CB_ERR) {
                             return 0;
                        }
                        if (iCurSelection++ < (iNumEntries - 1)) {
                            if (iCurSelection == 1) {
                                // Enable Previous button
                                EnableWindow(GetDlgItem(hWnd, ID_PREVSUBSET), TRUE);
                            }

                            SendDlgItemMessage(
                                hWnd,
                                ID_UNICODESUBSET,
                                CB_SETCURSEL,
                                iCurSelection, 0);
                            UpdateSymbolSelection(
                                hWnd,
                                aSubsetData[iCurSelection].BeginRange,
                                aSubsetData[iCurSelection].EndRange
                                );
                            InvalidateRect(hwndCharGrid, NULL, TRUE);
                            if (iCurSelection == (iNumEntries - 1)) {
                                HWND hwndButton;

                                EnableWindow(GetDlgItem(hWnd, ID_NEXTSUBSET), FALSE);
                                //
                                // Only reset the button style and focus if
                                // the "Next" button currently has it.
                                //
                                if (iControl == ID_NEXTSUBSET) {
                                    SendDlgItemMessage(hwndDialog, ID_PREVSUBSET,
                                         BM_SETSTYLE, BS_DEFPUSHBUTTON, 1);
                                    SendDlgItemMessage(hwndDialog, ID_NEXTSUBSET,
                                        BM_SETSTYLE, BS_PUSHBUTTON, 1);
                                    hwndButton = GetDlgItem(hWnd, ID_PREVSUBSET);
                                    SetFocus(hwndButton);
                                    UpdateHelpText(NULL, hwndButton);
                                }
                            }
                        }

                    }
                    return 0;
                    break;

                case ID_PREVSUBSET:
                    {
                        INT iCurSelection;

                        iCurSelection = (INT)SendDlgItemMessage(
                                          hWnd,
                                          ID_UNICODESUBSET,
                                          CB_GETCURSEL,
                                          0, 0);
                        if (iCurSelection == CB_ERR) {
                             return 0;
                        }
                        if (iCurSelection > 0) {
                            iCurSelection--;

                            if (iCurSelection == (cSubsets - 2)) {
                                // Enable Next button
                                EnableWindow(GetDlgItem(hWnd, ID_NEXTSUBSET), TRUE);
                            }

                            SendDlgItemMessage(
                                hWnd,
                                ID_UNICODESUBSET,
                                CB_SETCURSEL,
                                iCurSelection, 0);
                            UpdateSymbolSelection(
                                hWnd,
                                aSubsetData[iCurSelection].BeginRange,
                                aSubsetData[iCurSelection].EndRange
                                );
                            InvalidateRect(hwndCharGrid, NULL, TRUE);
                            if (iCurSelection == 0) {
                                HWND hwndButton;

                                EnableWindow(GetDlgItem(hWnd, ID_PREVSUBSET), FALSE);
                                //
                                // Only reset the button style and focus if
                                // the "Previous" button currently has it.
                                //
                                if (iControl == ID_PREVSUBSET) {
                                    SendDlgItemMessage(hwndDialog, ID_NEXTSUBSET,
                                         BM_SETSTYLE, BS_DEFPUSHBUTTON, 1);
                                    SendDlgItemMessage(hwndDialog, ID_PREVSUBSET,
                                         BM_SETSTYLE, BS_PUSHBUTTON, 1);
                                    hwndButton = GetDlgItem(hWnd, ID_NEXTSUBSET);
                                    SetFocus(hwndButton);
                                    UpdateHelpText(NULL, hwndButton);
                                }
                            }
                        }
                    }
                    return 0;
                    break;

                case ID_STRING:
                    if (HIWORD(wParam) == EN_SETFOCUS) {
                        // Necessary if hotkey is used to get to the EC.
                        UpdateHelpText(NULL, (HWND)lParam);
                    } else if (HIWORD(wParam) == EN_CHANGE) {
                        // Disable Copy button if there are no chars in EC.
                        INT iLength;

                        iLength = GetWindowTextLength((HWND)lParam);
                        EnableWindow(GetDlgItem(hWnd, ID_COPY), (BOOL)iLength);
                    }

                    break;

                case ID_HELP:
                    DoHelp(hWnd, TRUE);
                    break;
            }
            break;

        case WM_DESTROY:
            SaveCurrentFont(hWnd);
            SaveCurrentSubset(hWnd);
            DoHelp(hWnd, FALSE);
            DeleteObject(hStaticBrush);
            PostQuitMessage(0);
            break;

        case WM_ACTIVATEAPP:
            if (wParam) {
                SendDlgItemMessage(hWnd, ID_STRING, EM_SETSEL, LOWORD(lEditSel), HIWORD(lEditSel));
            } else {
                lEditSel = SendDlgItemMessage(hWnd, ID_STRING, EM_GETSEL, 0, 0L);
                SendDlgItemMessage(hWnd, ID_STRING, EM_SETSEL, 0, 0L);
            }
            break;


    }
    return (FALSE);
}



/****************************************************************************

    FUNCTION: CharGridWndProc(HWND, UINT, WPARAM, LPARAM)

    PURPOSE:  Processes messages for the character grid window.

    COMMENTS:

****************************************************************************/

LONG  APIENTRY CharGridWndProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{

    switch (message) {
        case WM_CREATE:
                {
                RECT rect;
                HDC hdcScrn;
                POINT point1, point2;

                // Setup global.
                hwndCharGrid = hWnd;

                GetClientRect(hWnd, &rect);

                /*
                 * Calculate metrics for the character grid and the
                 * magnify window.
                 */
                sycm.dxpBox = (rect.right-1)  / (cchSymRow + 1);
                sycm.dypBox = (rect.bottom-2) / (cchSymCol + 1);
                sycm.dxpCM  = sycm.dxpBox * cchSymRow+1;
                sycm.dypCM  = sycm.dypBox * cchSymCol+1;        // space inside for border

                sycm.dxpMag = sycm.dxpBox * 2 + 4;      // twice the size + 2 bit border
                sycm.dypMag = sycm.dypBox * 2 + 4;

                sycm.chCurr   = chSymFirst;
                sycm.hFontMag = NULL;
                sycm.hFont    = NULL;
                sycm.hdcMag   = NULL;
                sycm.hbmMag   = NULL;
                sycm.ypDest   = 0;

                sycm.fFocusState = sycm.fMouseDn = sycm.fCursorOff = FALSE;

                // Size the window precisely so the grid fits and is centered.
                MoveWindow(hWnd, (rect.right - sycm.dxpCM + 1) / 2,
                                 (rect.bottom - sycm.dypCM + 1) / 2 + ((LPCREATESTRUCT)lParam)->y - 2,
                                 sycm.dxpCM + 2,
                                 sycm.dypCM + 2,
                                 FALSE);

                /*
                 * Figure out what the offsets are between the dialog
                 * and the character grid window.
                 */
                point1.x = point1.y = point2.x = point2.y = 0;
                ClientToScreen(hWnd, &point1);
                ClientToScreen(((LPCREATESTRUCT)lParam)->hwndParent, &point2);
                sycm.xpCM = (point1.x - point2.x) - (sycm.dxpMag - sycm.dxpBox) / 2;
                sycm.ypCM = (point1.y - point2.y) - (sycm.dypMag - sycm.dypBox) / 2;


                // Create dc and bitmap for the magnify window.
                if ((hdcScrn = GetWindowDC(hWnd)) != NULL)
                        {
                        if ((sycm.hdcMag = CreateCompatibleDC(hdcScrn)) != NULL)
                                {
                                SetTextColor(sycm.hdcMag,
                                             GetSysColor(COLOR_WINDOWTEXT));
                                SetBkColor(sycm.hdcMag,
                                           GetSysColor(COLOR_WINDOW));
                                SetBkMode(sycm.hdcMag, OPAQUE);
                                if ((sycm.hbmMag = CreateCompatibleBitmap(hdcScrn,
                                         sycm.dxpMag, sycm.dypMag*2)) == NULL)
                                        {
                                        DeleteObject(sycm.hdcMag);
                                        }
                                else
                                        {
                                        SelectObject(sycm.hdcMag, sycm.hbmMag);
                                        }
                                }
                        ReleaseDC(hWnd, hdcScrn);
                        }
                }
            break;

        case WM_DESTROY:
            if (sycm.fMouseDn)
                ExitMagnify(hWnd, &sycm);
            if (fDelClipboardFont)
                DeleteObject(hFontClipboard);
            if (sycm.hFont != NULL)
                DeleteObject(sycm.hFont);
            if (sycm.hFontMag != NULL)
                DeleteObject(sycm.hFontMag);
            if (sycm.hdcMag != NULL)
                DeleteDC(sycm.hdcMag);
            if (sycm.hbmMag != NULL)
                DeleteObject(sycm.hbmMag);
            break;

        case WM_SETFOCUS:
        case WM_KILLFOCUS:
            RestoreSymMag(&sycm);
            DrawSymChOutlineHwnd(&sycm, hWnd, sycm.chCurr, TRUE, message == WM_SETFOCUS);
            break;

        case WM_LBUTTONDOWN:
            {
            RECT rect;

            // Don't draw anything if there's an update region pending.
            if (GetUpdateRect(hWnd, (LPRECT)&rect, FALSE) != 0)
                break;

            SetFocus(hWnd);
            SetCapture(hWnd);

            sycm.fMouseDn = TRUE;

            if (!FMagData(&sycm))
                DrawSymChOutlineHwnd(&sycm, hWnd, sycm.chCurr, FALSE, FALSE);

            }

            // Fall through to WM_MOUSEMOVE

        case WM_MOUSEMOVE:
            if (sycm.fMouseDn) {
               POINT pt;
               UTCHAR chMouseSymbol;

               pt.x = LOWORD(lParam);
               pt.y = HIWORD(lParam);
                ClientToScreen(hWnd, (LPPOINT)&pt);
                if (WindowFromPoint(pt) == hWnd) {
                    ScreenToClient(hWnd, (LPPOINT)&pt);
                    // convert back to a 'points'-like thing
                    lParam = MAKELONG((WORD)pt.x, (WORD)pt.y);
                    chMouseSymbol = (UTCHAR)ChFromSymLParam(&sycm, lParam);
                    if (chMouseSymbol > chSymLast) {
                        //
                        // We're outside of current character range (but still
                        // within the grid).  Restore cursor and leave
                        // magnified character.
                        //
                        if (sycm.fCursorOff) {
                            sycm.fCursorOff = FALSE;
                            ShowCursor(TRUE);
                        }
                    } else {
                        //
                        // We're in the grid and within the range of currently
                        // displayed characters, display magnified character.
                        //
                        if (!sycm.fCursorOff) {
                            sycm.fCursorOff = TRUE;
                            ShowCursor(FALSE);
                        }
                        MoveSymbolSel(&sycm, chMouseSymbol);
                    }
                } else {
                    // Left grid, leave magnified character and restore cursor.
                    if (sycm.fCursorOff) {
                        sycm.fCursorOff = FALSE;
                        ShowCursor(TRUE);
                    }
                }
            }
            break;

        case WM_CANCELMODE:
        case WM_LBUTTONUP:
            if (sycm.fMouseDn)
                ExitMagnify(hWnd, &sycm);

            break;

        case WM_LBUTTONDBLCLK:
            // Send this character to the entry field.
            SendDlgItemMessage(hwndDialog, ID_STRING, WM_CHAR,
                               (WPARAM)sycm.chCurr, 0L);
            break;

        case WM_GETDLGCODE:
            // Necessary to obtain arrow and tab messages.
            return (DLGC_WANTARROWS | DLGC_WANTCHARS);
            break;

        case WM_KEYDOWN:
                {
                UTCHAR chNew = sycm.chCurr;

                if (sycm.fMouseDn)
                    break;

                switch (wParam)
                        {
                case VK_LEFT:
                        if (--chNew < chSymFirst)
                                return 0L;
                        break;

                case VK_UP:
                        if ((chNew -= cchSymRow) < chSymFirst)
                                return 0L;
                        break;

                case VK_RIGHT:
                        if (++chNew > chSymLast)
                                return 0L;
                        break;

                case VK_DOWN:
                        if ((chNew += cchSymRow) > chSymLast)
                                return 0L;
                        break;

                default:
                                return 0L;
                        }       /* switch (wParam) */

                if (!FMagData(&sycm))
                        DrawSymChOutlineHwnd(&sycm, hWnd, sycm.chCurr, FALSE, FALSE);
                MoveSymbolSel(&sycm, (UTCHAR)chNew);
                }
                break;

        case WM_CHAR:
                if (sycm.fMouseDn)
                    break;

                if (wParam >= chSymFirst  &&  wParam <= chSymLast) {
                    if (!FMagData(&sycm))
                        DrawSymChOutlineHwnd(&sycm, hWnd, sycm.chCurr, FALSE, FALSE);
                    MoveSymbolSel(&sycm, (UTCHAR) wParam);
                    SendDlgItemMessage(hwndDialog, ID_STRING, WM_CHAR,
                                       (WPARAM)sycm.chCurr, 0L);
                }
                break;

        case WM_PAINT:
            {
            HDC hdc;
            PAINTSTRUCT ps;

            hdc = BeginPaint(hWnd, &ps);
            DrawSymbolMap(&sycm, hdc);
            EndPaint(hWnd, &ps);
                return (TRUE);
            }

        default:                          /* Passes it on if unproccessed    */
            return (DefWindowProc(hWnd, message, wParam, lParam));
    }
    return 0L;
}

/****************************************************************************

    FUNCTION: ChFromSymLParam(PSYCM, long)

    PURPOSE:  Determine the character to select from the mouse
              position (lParam)

    COMMENTS:

****************************************************************************/

INT ChFromSymLParam(
    PSYCM psycm,
    LPARAM lParam)
{
        return min(cchSymRow-1, max(0, ((INT) LOWORD(lParam)-1) / psycm->dxpBox))
                 + min(cchSymCol-1, max(0, ((INT) HIWORD(lParam)-1) / psycm->dypBox))
                        * cchSymRow + chSymFirst;
}


/****************************************************************************

    FUNCTION: DrawSymChOutlineHwnd(PSYCM, HWND, UTCHAR, BOOL, BOOL);

    PURPOSE:  Gets a DC for hwnd, calls DrawSymChOutline.

    COMMENTS:

****************************************************************************/

VOID DrawSymChOutlineHwnd(
    PSYCM   psycm,
    HWND    hwnd,
    UTCHAR   ch,
    BOOL    fVisible,
    BOOL    fFocus)
{
        HDC hdc = GetDC(hwnd);
        DrawSymChOutline(psycm, hdc, ch, fVisible, fFocus);
        ReleaseDC(hwnd, hdc);
}

/****************************************************************************

    FUNCTION: RecalcCharMap(HWND, PSYCM, int, BOOL);

    PURPOSE:  Recalculate fixed character map data (font info, sizes, etc.)

    COMMENTS:

****************************************************************************/

VOID RecalcCharMap(
    HWND hwndDlg,
    PSYCM psycm,
    INT iCombo,
    BOOL fRedraw)
{
        HDC          hdc;
        TEXTMETRIC   tm;
        UINT         ch;
        LPINT        lpdxp;
        HFONT        hFont;
        LOGFONT      LogFont;

        // Get rid of the old font handles.
        if (hFontClipboard && hFontClipboard == psycm->hFont)
            fDelClipboardFont = TRUE;
        if (psycm->hFont && hFontClipboard != psycm->hFont)
            DeleteObject(psycm->hFont);
        if (psycm->hFontMag)
            DeleteObject(psycm->hFontMag);

        hdc = GetDC(hwndCharGrid);

        /*
         * Set up the LogFont structure.
         */
        // Make sure it fits in the grid.
        LogFont.lfHeight = psycm->dypBox - 3; // Allow for whitespace.
        // Set these guys to zero.
        LogFont.lfWidth = LogFont.lfEscapement = LogFont.lfOrientation =
                          LogFont.lfWeight = 0;
        // Set these at zero too.
        LogFont.lfItalic = LogFont.lfUnderline = LogFont.lfStrikeOut =
            LogFont.lfOutPrecision = LogFont.lfClipPrecision =
            LogFont.lfQuality = LogFont.lfPitchAndFamily = 0;
        // Let the facename and size define the font.
        LogFont.lfCharSet = ANSI_CHARSET;
        // Get the facename from the combo box.
        SendDlgItemMessage(hwndDlg, ID_FONT, CB_GETLBTEXT, iCombo,
                           (LONG)(LPTSTR)LogFont.lfFaceName);

        /*
         * BUGBUG  27 Oct 92   GregoryW
         *   For now we don't have a way to determine if this is a
         *   Unicode font or an ANSI font.  The best we can do is
         *   look at the face name to see if it is the one Unicode
         *   font we recognize.
         */
        if (!lstrcmpi(LogFont.lfFaceName, TEXT("Lucida Sans Unicode"))) {
            LONG iCurSel;

            psycm->fAnsiFont = FALSE;
            // Enable Block listbox and set defaults appropriately.
            EnableWindow(GetDlgItem(hwndDlg, ID_UNICODESUBSET), TRUE);
            iCurSel = SendDlgItemMessage(
                          hwndDlg,
                          ID_UNICODESUBSET,
                          CB_GETCURSEL,
                          0,
                          0L
                          );
            UpdateSymbolSelection(
                hwndDlg,
                aSubsetData[iCurSel].BeginRange,
                aSubsetData[iCurSel].EndRange
                );
            // Enable Previous button if not on first subset.
            if (iCurSel > 0) {
                EnableWindow(GetDlgItem(hwndDlg, ID_PREVSUBSET), TRUE);
            } else {
                EnableWindow(GetDlgItem(hwndDlg, ID_PREVSUBSET), FALSE);
            }
            // Enable Next button if not on last subset.
            if (iCurSel < (cSubsets - 1)) {
                EnableWindow(GetDlgItem(hwndDlg, ID_NEXTSUBSET), TRUE);
            } else {
                EnableWindow(GetDlgItem(hwndDlg, ID_NEXTSUBSET), FALSE);
            }
        } else {
            // put back the ANSI defaults and disable Unicode Block listbox
            psycm->fAnsiFont = TRUE;
            UpdateSymbolSelection(hwndDlg, 32, 255);
            EnableWindow(GetDlgItem(hwndDlg, ID_UNICODESUBSET), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, ID_NEXTSUBSET), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, ID_PREVSUBSET), FALSE);
        }

        // Create the font.
        psycm->hFont = CreateFontIndirect(&LogFont);
        hFont = SelectObject(hdc, psycm->hFont);

        // Create the magnify font.
        LogFont.lfHeight = psycm->dypMag - 5;  // Allow for whitespace.
        psycm->hFontMag = CreateFontIndirect(&LogFont);

        /*
         * Calculate new values and place in window data structure.
         */
        GetTextMetrics(hdc, &tm);
        psycm->xpCh = 2;
        psycm->ypCh = (4 + psycm->dypBox - tm.tmHeight) / 2;

        lpdxp = (LPINT) psycm->rgdxp;

        GetCharWidth(hdc, chSymFirst, chSymLast, lpdxp);

        SelectObject(hdc, hFont);

        for (ch = (UINT) chSymFirst; ch <= (UINT) chSymLast; ch++, lpdxp++)
                {
                    *lpdxp = (psycm->dxpBox - *lpdxp) / 2 - 1;
                }
        ReleaseDC(hwndCharGrid, hdc);

        psycm->xpMagCurr = 0;   // No magnification data

        if (fRedraw)
            InvalidateRect(hwndCharGrid, NULL, TRUE);
}

/****************************************************************************

    FUNCTION: DrawSymbolMap(PSYCM, HDC);

    PURPOSE:  Draw all of the pieces of the symbol character map

    COMMENTS:

****************************************************************************/

VOID DrawSymbolMap(
    PSYCM psycm,
    HDC     hdc)
{
        BOOL fFocus;

        DrawSymbolGrid(psycm, hdc);
        DrawSymbolChars(psycm, hdc);
        /*
         * We need to force the focus rect to paint if we have the focus
         * since the old focus rect has been drawn over already.
         */
        if (fFocus = psycm->fFocusState)
            psycm->fFocusState = FALSE;
        DrawSymChOutline(psycm, hdc, psycm->chCurr, TRUE, fFocus);
}


void MoveTo(HDC hdc, int x, int y){

   MoveToEx(hdc, x, y, NULL);
}

/****************************************************************************

    FUNCTION: DrawSymbolGrid(PSYCM, HDC);

    PURPOSE:  Draw the symbol character map grid.

    COMMENTS:

****************************************************************************/

VOID DrawSymbolGrid(
    PSYCM psycm,
    HDC hdc)
{
        INT    cli;             /* Count of lines */
        INT    xp, yp;
        INT    dxpBox  = psycm->dxpBox;
        INT    dypBox  = psycm->dypBox;
        HPEN   hpenOld;

        hpenOld = SelectObject(hdc, CreatePen(PS_SOLID, 1,
                                              GetSysColor(COLOR_WINDOWFRAME)));

        // Draw horizontal lines.
        xp = psycm->dxpCM + 1;
        yp = 1;
        cli = cchSymCol+1;
        while (cli--)
                {
                MoveTo(hdc, 1, yp);
                LineTo(hdc, xp, yp);
                yp += dypBox;
                }

        // Draw vertical lines.
        yp = psycm->dypCM;
        xp = 1;
        cli = cchSymRow+1;
        while (cli--)
                {
                MoveTo(hdc, xp, 1);
                LineTo(hdc, xp, yp);
                xp += dxpBox;
                }

        DeleteObject(SelectObject(hdc, hpenOld));
}

/****************************************************************************

    FUNCTION: DrawSymbolChars(PSYCM, HDC);

    PURPOSE:  Draw the symbol character map.

    COMMENTS:

****************************************************************************/

VOID DrawSymbolChars(
    PSYCM psycm,
    HDC     hdc)
{
        INT    dxpBox  = psycm->dxpBox;
        INT    dypBox  = psycm->dypBox;

        INT    cch;
        INT    x, y;
        INT    yp;
        TCHAR   ch;

        HFONT  hFontOld;

        RECT   rect;
        LPRECT lprect = (LPRECT) &rect;
        LPINT lpdxp;

        // Setup the font and colors.
        hFontOld = (HFONT) SelectObject(hdc, psycm->hFont);
        SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
        SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
        SetBkMode(hdc, OPAQUE);

        // Draw characters.
        cch = 1;
        ch = chSymFirst;

        lpdxp = (LPINT) psycm->rgdxp;

        rect.top = 2;
        yp = psycm->ypCh;
        rect.bottom = rect.top + dypBox - 1;

        for (y = 0; y++ < cchSymCol;)
                {
                rect.left  = psycm->xpCh;
                rect.right = rect.left + dxpBox - 1;
                for (x = 0; x++ < cchSymRow && ch <= chSymLast;)
                        {
                        if (psycm->fAnsiFont) {
                            ExtTextOutA(hdc, rect.left + (*lpdxp++), yp,
                                ETO_OPAQUE | ETO_CLIPPED, lprect, &(CHAR)ch, 1, NULL);
                        } else
                            ExtTextOutW(hdc, rect.left + (*lpdxp++), yp,
                                ETO_OPAQUE | ETO_CLIPPED, lprect, &ch, 1, NULL);
                        ch++;
                        rect.left  += dxpBox;
                        rect.right += dxpBox;
                        }
                yp += dypBox;
                rect.top += dypBox;
                rect.bottom += dypBox;
                }

        SelectObject(hdc, hFontOld);
}

/****************************************************************************

    FUNCTION: DrawSymChOutline(PSYCM, HDC, UTCHAR, BOOL, BOOL);

    PURPOSE:  Draw an outline around the symbol in the character map

    COMMENTS: If fVisible, draw outline else erase it.

****************************************************************************/

VOID DrawSymChOutline(
    PSYCM   psycm,
    HDC     hdc,
    UTCHAR   ch,
    BOOL    fVisible,
    BOOL    fFocus)
{
        HBRUSH hbrOld;
        RECT rc;
        INT dxpBox = psycm->dxpBox;
        INT dypBox = psycm->dypBox;

        hbrOld = SelectObject(hdc,
                              CreateSolidBrush(GetSysColor(fVisible ?
                                                           COLOR_WINDOWFRAME :
                                                           COLOR_WINDOW)));
        ch -= chSymFirst;

        rc.left   = (ch % cchSymRow) * dxpBox +2;
        rc.right  = rc.left + dxpBox -1;
        rc.top    = (ch / cchSymRow) * dypBox +2;
        rc.bottom = rc.top  + dypBox -1;

        // Draw selection rectangle.
        PatBlt( hdc, rc.left,    rc.top-2,    dxpBox-1, 1, PATCOPY);
        PatBlt( hdc, rc.left,    rc.bottom+1, dxpBox-1, 1, PATCOPY);
        PatBlt( hdc, rc.left-2,  rc.top,      1, dypBox-1, PATCOPY);
        PatBlt( hdc, rc.right+1, rc.top,      1, dypBox-1, PATCOPY);

        DeleteObject(SelectObject(hdc, GetStockObject(NULL_BRUSH)));

        // Deal with the focus rectangle.
        if (fFocus != psycm->fFocusState) {
            DrawFocusRect(hdc, &rc);
            psycm->fFocusState = fFocus;
        }

        SelectObject(hdc, hbrOld);
}


/****************************************************************************

    FUNCTION: MoveSymbolSel(PSYCM, UTCHAR);

    PURPOSE:  Change the current symbol selection.  Handles drawing of
              magnified characters.

    COMMENTS:

****************************************************************************/

VOID MoveSymbolSel(
    PSYCM psycm,
    UTCHAR chNew)
{
        HDC    hdc;
        HDC    hdcMag  = psycm->hdcMag;
        RECT   rc;
        HFONT  hFontOld;
        HFONT  hFontMag;        // old font in memory dc
        HPEN   hpenOld;

        UTCHAR chNorm = chNew - chSymFirst + 32;
        INT dxpMag = psycm->dxpMag;     // for quick reference
        INT dypMag = psycm->dypMag;
        INT ypMemSrc  = psycm->ypDest;
        INT ypMemDest = ypMemSrc ^ dypMag;
        INT xpCurr  = psycm->xpMagCurr;
        INT ypCurr  = psycm->ypMagCurr;
        INT xpNew   = psycm->xpCM + (psycm->dxpBox *  (chNorm % cchSymRow));
        INT ypNew   = psycm->ypCM + (psycm->dypBox * ((chNorm / cchSymRow) - 1));
        INT dxpCh;      // width of extra character space (used to center char in box)
        INT dypCh;

        if (((chNew == (UTCHAR)psycm->chCurr) && FMagData(psycm)))
                return;

        /*
         * Don't draw a magnified character if the char grid has an update
         * region or is not visible.
         */
        if (!IsWindowVisible(hwndCharGrid) || GetUpdateRect(hwndCharGrid, &rc, FALSE))
            return;

        hdc = GetDC(hwndDialog);

        // Setup the magnified font character.
        hFontMag = SelectObject(hdcMag, psycm->hFontMag);
        { SIZE sz;
          GetTextExtentPoint(hdcMag, &chNew, 1, &sz);

          dxpCh = (dxpMag - (INT)sz.cx) / 2 - 1;
          dypCh = (dypMag - (INT)sz.cy) / 2 - 1;
        }
        hpenOld = SelectObject(hdc, CreatePen(PS_SOLID, 1,
                                              GetSysColor(COLOR_WINDOWFRAME)));
        hFontOld = SelectObject(hdc, psycm->hFontMag);

        // Copy screen data to offscreen bitmap.
        BitBlt(hdcMag, 0, ypMemDest, dxpMag, dypMag, hdc, xpNew, ypNew, SRCCOPY);

        // Setup DC.
        SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
        SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
        SetBkMode(hdc, OPAQUE);

        if (FMagData(psycm))
                {
                INT xpT  = xpNew - xpCurr;              // point of overlap in offscreen data
                INT ypT  = ypNew - ypCurr;
                INT dxpT = dxpMag - abs(xpT);   // size of overlap
                INT dypT = dypMag - abs(ypT);

                if ((dxpT > 0) && (dypT > 0))
                        {
                        INT xpTmax,  ypTmax;   // max(0, xpT);
                        INT xpTmin,  ypTmin;   // min(0, xpT);
                        INT xpTnmin, ypTnmin;  // min(0, -xpT);

                        if (xpT < 0)
                                {
                                xpTnmin = - (xpTmin = xpT);
                                xpTmax  = 0;
                                }
                        else
                                {
                                xpTmax  = xpT;
                                xpTnmin = xpTmin = 0;
                                }
                        if (ypT < 0)
                                {
                                ypTnmin = - (ypTmin = ypT);
                                ypTmax  = 0;
                                }
                        else
                                {
                                ypTmax  = ypT;
                                ypTnmin = ypTmin = 0;
                                }

                        rc.left  = xpTmax;
                        rc.right = xpTmin + dxpMag;
                        rc.top   = ypTmax + ypMemSrc;
                        rc.bottom= ypTmin + dypMag + ypMemSrc;

                        // Copy overlapping offscreen data.
                        BitBlt(hdcMag, xpTnmin, ypTnmin + ypMemDest, dxpT, dypT,
                                   hdcMag, xpTmax,  ypTmax  + ypMemSrc, SRCCOPY);

                        // Print part of char over old screen data.
                        if (psycm->fAnsiFont) {
                            ExtTextOutA(hdcMag, xpT + dxpCh, ypT + dypCh + ypMemSrc,
                                ETO_OPAQUE | ETO_CLIPPED, (LPRECT) &rc, &(CHAR)chNew, 1, NULL);
                        } else
                            ExtTextOutW(hdcMag, xpT + dxpCh, ypT + dypCh + ypMemSrc,
                                ETO_OPAQUE | ETO_CLIPPED, (LPRECT) &rc, &chNew, 1, NULL);

                        }

                // Restore old screen data.
                BitBlt(hdc, xpCurr, ypCurr, dxpMag, dypMag, hdcMag, 0, ypMemSrc, SRCCOPY);

                }

        rc.right  = (psycm->xpMagCurr = rc.left = xpNew) + dxpMag - 2;
        rc.bottom = (psycm->ypMagCurr = rc.top  = ypNew) + dypMag - 2;


        // The rectangle.
        MoveTo(hdc, rc.left, rc.top);
        LineTo(hdc, rc.left, rc.bottom - 1);
        LineTo(hdc, rc.right - 1, rc.bottom - 1);
        LineTo(hdc, rc.right - 1, rc.top);
        LineTo(hdc, rc.left, rc.top);

        // The shadow.
        MoveTo(hdc, rc.right, rc.top + 1);
        LineTo(hdc, rc.right, rc.bottom);
        LineTo(hdc, rc.left, rc.bottom);
        MoveTo(hdc, rc.right + 1, rc.top + 2);
        LineTo(hdc, rc.right + 1, rc.bottom + 1);
        LineTo(hdc, rc.left + 1, rc.bottom + 1);

        rc.left++;
        rc.top++;
        rc.right--;
        rc.bottom--;

        // Draw magnified character on screen.
        if (psycm->fAnsiFont) {
            ExtTextOutA(hdc, xpNew + dxpCh, ypNew + dypCh,
                   ETO_OPAQUE | ETO_CLIPPED, (LPRECT) &rc, &(CHAR)chNew, 1, NULL);
        } else
            ExtTextOutW(hdc, xpNew + dxpCh, ypNew + dypCh,
                   ETO_OPAQUE | ETO_CLIPPED, (LPRECT) &rc, &chNew, 1, NULL);

        psycm->ypDest = ypMemDest;

        DeleteObject(SelectObject(hdc, hpenOld));
        SelectObject(hdc, hFontOld);
        SelectObject(hdcMag, hFontMag);

        UpdateKeystrokeText(hdc, chNew, TRUE);

        ReleaseDC(hwndDialog, hdc);

        psycm->chCurr = chNew;
}

/****************************************************************************

    FUNCTION: RestoreSymMag(PSYCM);

    PURPOSE:  Restore the screen data under the magnifier.

    COMMENTS:

****************************************************************************/

VOID RestoreSymMag(
    PSYCM psycm)
{

        if (FMagData(psycm))
                {
                HDC hdc = GetDC(hwndDialog);

                BitBlt(hdc, psycm->xpMagCurr, psycm->ypMagCurr,
                        psycm->dxpMag, psycm->dypMag,
                        psycm->hdcMag, 0, psycm->ypDest, SRCCOPY);

                ReleaseDC(hwndDialog, hdc);

                psycm->xpMagCurr = 0;   // flag - no data offscreen (see FMagData)
                }
}


/****************************************************************************

    FUNCTION: FontLoadProc(LPLOGFONT, NEWTEXTMETRICEX*, short, LPTSTR);

    PURPOSE:  Used by EnumFonts to load our combo box with all the fonts
              installed in the system.

    COMMENTS:

****************************************************************************/

INT  APIENTRY FontLoadProc(LPLOGFONT lpLogFont,
                           NEWTEXTMETRICEX* lpTextMetric,
                           DWORD nFontType,
                           LPARAM lpData)
{
    INT iPos;
    TCHAR szFace[LF_FACESIZE];

    // Check for duplicates.
    iPos = (INT)SendDlgItemMessage((HWND)lpData, ID_FONT, CB_FINDSTRING, (WPARAM)-1,
                                  (DWORD)&lpLogFont->lfFaceName);

    if (iPos == CB_ERR) {
NotInListYet:
        // Doesn't exist, insert the facename into the combo box.
        iPos = (INT)SendDlgItemMessage((HWND)lpData, ID_FONT,
                                       CB_ADDSTRING, 0,
                                       (DWORD)&lpLogFont->lfFaceName);
    } else {

        // make sure it is not just a substring (want a full match)
        SendDlgItemMessage((HWND)lpData, ID_FONT, CB_GETLBTEXT, iPos, (LONG)(LPTSTR)szFace);
        if (lstrcmpi(szFace, lpLogFont->lfFaceName))
            goto NotInListYet;

        // Already exists, blow out now if this is not a true type font.
        if (!(nFontType & TRUETYPE_FONTTYPE))
            return (1);
    }

    /*
     * Store the pertinant font information in the combo item data.
     */
    if ((iPos != CB_ERR) && (iPos != CB_ERRSPACE)) {
        ITEMDATA ItemData;
        DWORD   ntmFlags = lpTextMetric->ntmTm.ntmFlags;
        SHORT   sFontType = 0;

        if (ntmFlags & NTM_PS_OPENTYPE)
        {
            sFontType = PS_OPENTYPE_FONT;
        }
        else if (ntmFlags & NTM_TYPE1)
        {
            sFontType = TYPE1_FONT;
        }
        else if (nFontType & TRUETYPE_FONTTYPE)
        {
            if (ntmFlags & NTM_TT_OPENTYPE)
            {
                sFontType = TT_OPENTYPE_FONT;
            }
            else
                sFontType = TRUETYPE_FONT;
        }

        ItemData.FontType = sFontType;
        ItemData.CharSet = lpLogFont->lfCharSet;
        ItemData.PitchAndFamily = lpLogFont->lfPitchAndFamily;

        SendDlgItemMessage((HWND)lpData, ID_FONT, CB_SETITEMDATA, iPos,
                           *(DWORD *)&ItemData);
    }

    // Continue enumeration.
    return (1);
}

/****************************************************************************

    FUNCTION: GetEditText(HWND);

    PURPOSE:  Returns HANDLE containing the text in the edit control.

    COMMENTS: Caller is responsible for freeing this handle!

****************************************************************************/

HANDLE GetEditText(
    HWND hwndDlg)
{
    INT cchText;
    HWND hwndEditCtl;
    HANDLE hmem;
    LPTSTR lpstrText;
    DWORD dwSel;

    hwndEditCtl = GetDlgItem(hwndDlg, ID_STRING);

    cchText = GetWindowTextLength(hwndEditCtl);

    hmem = GlobalAlloc(0, CTOB((cchText + 1)));

    lpstrText = (LPTSTR)GlobalLock(hmem);

    cchText = GetWindowText(hwndEditCtl, lpstrText, cchText+1);

    dwSel = SendMessage(hwndEditCtl, EM_GETSEL, 0, 0L);

    if (LOWORD(dwSel) != HIWORD(dwSel)) {
        // If there is a selection, then only get the selected text.
        *(lpstrText + HIWORD(dwSel)) = TEXT('\0');
        lstrcpy(lpstrText, lpstrText + LOWORD(dwSel));
    }

    GlobalUnlock(hmem);

    if (cchText == 0)
        hmem = GlobalFree(hmem);

    return (hmem);
}

/****************************************************************************

    FUNCTION: CopyString(HWND);

    PURPOSE:  Implement the Copy function.

    COMMENTS:

****************************************************************************/

VOID CopyString(
    HWND hwndDlg)
{
    HANDLE hmem;
    LPTSTR lpstrText;

    if (hmem = GetEditText(hwndDlg)) {
        lpstrText = (LPTSTR)GlobalLock(hmem);

        // Copying string to clipboard.
        if (OpenClipboard(hwndDlg)) {
            EmptyClipboard();
            SendRTFToClip(hwndDlg, lpstrText);
#ifdef UNICODE
            SetClipboardData(CF_UNICODETEXT, hmem);
#else
            SetClipboardData(CF_TEXT, hmem);
#endif
            CloseClipboard();
        } else {
            // If we couldn't open the clipboard, then we need to free memory.
            GlobalUnlock(hmem);
            GlobalFree(hmem);
        }
    }
}

/****************************************************************************

    FUNCTION: SendRTFToClip(HWND, LPTSTR);

    PURPOSE:  Put the string in the clipboard using Rich Text Format.

    COMMENTS: Assumes that the clipboard has already been opened.

****************************************************************************/

VOID SendRTFToClip(
    HWND hwndDlg,
    LPTSTR lpstrText)
{
    INT iCurrFont;
    ITEMDATA ItemData;
    TCHAR szFaceName[LF_FACESIZE];
    HANDLE hmemRTF, hmemClip;
    LPTSTR lpstrClipString;
    TCHAR achHeader[] = TEXT("{\\rtf1\\ansi {\\fonttbl{\\f0\\");
    TCHAR achMiddle[] = TEXT(";}}\\sectd\\pard\\plain\\f0 ");

    #define MAXLENGTHFONTFAMILY 8
    #define ALITTLEEXTRA 10    // Covers extra characters + length of font size.

    iCurrFont = (INT)SendDlgItemMessage(hwndDlg, ID_FONT, CB_GETCURSEL, 0, 0L);

    // Get the item data - contains fonttype, charset, and pitchandfamily.
    *(DWORD *)&ItemData = SendDlgItemMessage(hwndDlg, ID_FONT, CB_GETITEMDATA,
                                             iCurrFont, 0L);

    // Get the facename from the combo box.
    SendDlgItemMessage(hwndDlg, ID_FONT, CB_GETLBTEXT, iCurrFont,
                       (LPARAM)(LPTSTR)szFaceName);

    hmemRTF = GlobalAlloc(0, CTOB(lstrlen((LPTSTR)achHeader) +
                          MAXLENGTHFONTFAMILY +
                          lstrlen(szFaceName) +
                          lstrlen((LPTSTR)achMiddle) +
                          4 * lstrlen(lpstrText) +  // 4 times in case they're all > 7 bits
                          ALITTLEEXTRA));
    if (hmemRTF == NULL)
        return;

    // Allocate memory for local storage of clipboard string for owner draw.
    if (hmemClip  = GlobalAlloc(0, CTOB(lstrlen(lpstrText) + 1))) {
        // Get rid of old guys.
        if (hstrClipboard)
            GlobalFree(hstrClipboard);
        if (fDelClipboardFont) {
            fDelClipboardFont = FALSE;
            DeleteObject(hFontClipboard);
        }

        // Save this stuff away for owner drawing in a clipboard viewer.
        hFontClipboard = sycm.hFont;
        hstrClipboard = hmemClip;
        lstrcpy(GlobalLock(hstrClipboard), lpstrText);
        GlobalUnlock(hstrClipboard);
    } else {
        GlobalFree(hmemRTF);
        return;
    }

    lpstrClipString = GlobalLock(hmemRTF);

    lstrcpy(lpstrClipString, achHeader);

    if (ItemData.CharSet == SYMBOL_CHARSET) {
        lstrcat(lpstrClipString, (LPTSTR)TEXT("ftech "));
    } else {
        // top four bits specify family
        switch (ItemData.PitchAndFamily & 0xf0) {
            case FF_DECORATIVE:
                lstrcat(lpstrClipString, (LPTSTR)TEXT("fdecor "));
                break;

            case FF_MODERN:
                lstrcat(lpstrClipString, (LPTSTR)TEXT("fmodern "));
                break;

            case FF_ROMAN:
                lstrcat(lpstrClipString, (LPTSTR)TEXT("froman "));
                break;

            case FF_SCRIPT:
                lstrcat(lpstrClipString, (LPTSTR)TEXT("fscript "));
                break;

            case FF_SWISS:
                lstrcat(lpstrClipString, (LPTSTR)TEXT("fswiss "));
                break;

            default:
                break;
        }
    }

    lstrcat(lpstrClipString, szFaceName);

    lstrcat(lpstrClipString, (LPTSTR)achMiddle);

    /*
     * We need to do the text character by character, making sure
     * that we output a special sequence \'hh for characters bigger
     * than 7 bits long!
     */
    lpstrClipString = (LPTSTR)(lpstrClipString + lstrlen(lpstrClipString));
    while (*lpstrText) {
        if ((UTCHAR)*lpstrText < 128) {
            if (*lpstrText == TEXT('\\') || *lpstrText == TEXT('{') || *lpstrText == TEXT('}'))
                /*
                 * Need to preface these symbols with a '\' since they are
                 * special control characters for RTF.
                 */
                *lpstrClipString++ = TEXT('\\');

            *lpstrClipString++ = *lpstrText++;
        } else {
            *lpstrClipString++ = TEXT('\\');
            *lpstrClipString++ = TEXT('\'');
#ifdef BUG
//
//BUGBUG - GregoryW 23/10/92  need a Unicode version of itoa...
//
            _itoa(*(UTCHAR FAR *)lpstrText++, achMiddle, 16);
#else
            wsprintf(achMiddle, TEXT("%x"), (INT)*lpstrText);
            lpstrText++;
#endif
            *lpstrClipString++ = achMiddle[0];
            *lpstrClipString++ = achMiddle[1];
        }
    }
    *lpstrClipString++ = TEXT('}');
    *lpstrClipString = TEXT('\0');

    if (!wCFRichText) {
         TCHAR szRTF[80];

         LoadString(hInst, IDS_RTF, szRTF, BTOC(sizeof(szRTF)) - 1);
         wCFRichText = RegisterClipboardFormat(szRTF);
    }

    // Put RTF and OwnerDisplay formats in the clipboard.
    SetClipboardData(wCFRichText, hmemRTF);
    SetClipboardData(CF_OWNERDISPLAY, NULL);
}

/****************************************************************************

    FUNCTION: PointsToHeight(int);

    PURPOSE:  Calculates the height in pixels of the specified point
              size for the current display.

    COMMENTS:

****************************************************************************/

INT PointsToHeight(INT iPoints)
{
    HDC hdc;
    INT iHeight;

    hdc = GetDC(HWND_DESKTOP);
    iHeight = MulDiv(iPoints, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    ReleaseDC(HWND_DESKTOP, hdc);
    return(iHeight);
}

/****************************************************************************

    FUNCTION: UpdateKeystrokeText(HDC, UTCHAR, BOOL);

    PURPOSE:  Calculates and updates the text string displayed in the
              Keystroke field of the status bar.

    COMMENTS:  Repaints status field if fRedraw == TRUE.

****************************************************************************/

VOID UpdateKeystrokeText(
    HDC hdc,
    UTCHAR chNew,
    BOOL fRedraw)
{
    TCHAR szUnshifted[2];
    SHORT vkRes;

#ifdef UNICODE
    if (chNew > 255) {
        lstrcpy(szKeystrokeText, szUnicodeLabel);
        wsprintf((LPTSTR)(szKeystrokeText + iUnicodeLabelStart), TEXT("U+%04x"), chNew);
    } else {
#endif
    lstrcpy(szKeystrokeText, szKeystrokeLabel);
    if (chNew == TEXT(' ')) {
        lstrcpy((LPTSTR)(szKeystrokeText + iKeystrokeTextStart), szSpace);
    } else {
        vkRes = VkKeyScan(chNew);
        /*
         * Map the virtual key code into an unshifted character value
         */
#ifdef UNICODE
        szUnshifted[0] = MapVirtualKey(LOBYTE(vkRes), 2);
#else
        szUnshifted[0] = LOBYTE(MapVirtualKey(LOBYTE(vkRes), 2));
#endif
        szUnshifted[1] = TEXT('\0');

        switch(HIBYTE(vkRes)) {
            case 0: // Unshifted char.
            case 1: // Character is shifted, just display the shifted char.
                szKeystrokeText[iKeystrokeTextStart] = chNew;
                szKeystrokeText[iKeystrokeTextStart + 1] = TEXT('\0');
                break;

            case 2: // Character is control character.
                lstrcpy((LPTSTR)(szKeystrokeText + iKeystrokeTextStart), szCtrl);
                lstrcat(szKeystrokeText, (LPTSTR)szUnshifted);
                break;

            case 6: // Character is CONTROL+ALT.
                lstrcpy((LPTSTR)(szKeystrokeText + iKeystrokeTextStart), szCtrlAlt);
                lstrcat(szKeystrokeText, (LPTSTR)szUnshifted);
                break;

            case 7: // Character is SHIFT+CONTROL+ALT.
                lstrcpy((LPTSTR)(szKeystrokeText + iKeystrokeTextStart), szShiftCtrlAlt);
                lstrcat(szKeystrokeText, (LPTSTR)szUnshifted);
                break;

            default: // Character created via Alt + Numpad
                lstrcpy((LPTSTR)(szKeystrokeText + iKeystrokeTextStart), szAlt);
                wsprintf((LPTSTR)(szKeystrokeText + lstrlen(szKeystrokeText)), TEXT("%d"), chNew);
                break;
        }
    }
#ifdef UNICODE
    }
#endif

    if (fRedraw)
        PaintStatusLine(hdc, FALSE, TRUE);
}


/****************************************************************************

    FUNCTION: UpdateHelpText(LPMSG, HWND);

    PURPOSE:  Calculates if the Help string needs to be updated, and does
              so if necessary.

    COMMENTS:  If hwndCtrl is not NULL, then it specifies the window handle
               of the control gaining focus, and lpmsg is ignored.

               If hwndCtrl is NULL, then lpmsg must point to a valid message
               structure.  If it is a tab character, then we calculate
               what the next control is that will receive the focus.
****************************************************************************/

BOOL UpdateHelpText(
    LPMSG lpmsg,
    HWND hwndCtrl)
{
    HDC hdc;
    BOOL fPaintStatus = FALSE;
    BOOL fRet = TRUE;

    if (hwndCtrl != NULL) {
        fPaintStatus = TRUE;
        iControl = GetDlgCtrlID(hwndCtrl);
    } else  if (lpmsg->message == WM_KEYDOWN) {
        if (lpmsg->wParam == VK_TAB) {
            fPaintStatus = TRUE;
            hwndCtrl = GetNextDlgTabItem(hwndDialog,
                                         GetDlgItem(hwndDialog, iControl),
                                         (BOOL)(GetKeyState(VK_SHIFT) & 0x8000));
            iControl = GetDlgCtrlID(hwndCtrl);
            if (iControl == ID_STRING) {
                /*
                 * Do this ourselves, otherwise default action will select
                 * the whole edit control.
                 */
                SetFocus(hwndCtrl);
                fRet = FALSE;
            }
            if (iControl == ID_CHARGRID) {
                /*
                 * Set the default button back to "Select".  The default
                 * might have changed to the "Next" or "Previous" button.
                 */
                SendMessage(hwndDialog, DM_SETDEFID, ID_SELECT, 0);
            }
        } else if (lpmsg->wParam == VK_F1) {
            PostMessage(hwndDialog, WM_COMMAND, ID_HELP, 0L);
        }
    }

    if (fPaintStatus) {
        hdc = GetDC(hwndDialog);
        PaintStatusLine(hdc, TRUE, FALSE);
        ReleaseDC(hwndDialog, hdc);
    }

    return (fRet);
}

/****************************************************************************

    FUNCTION: UpdateSymbolSelection(HWND, INT, INT);

    PURPOSE:  Updates the values of the following global values:
                  chSymFirst
                  chSymLast
                  sycm.chCurr
              Subsets in the Unicode character set have different numbers
              of characters.  We have to do some bounds checking in order
              to set an appropriate sycm.chCurr value.  The "Keystroke"
              status field is updated.

    COMMENTS:  Repaints Keystroke field if HWND != NULL.

****************************************************************************/
VOID UpdateSymbolSelection(
    HWND hwnd,
    INT FirstChar,
    INT LastChar)
{
    UTCHAR chSymOffset;

    chSymOffset = sycm.chCurr - chSymFirst;
    chSymFirst = FirstChar;
    chSymLast = LastChar;
    sycm.chCurr = chSymOffset + chSymFirst;
    if (sycm.chCurr > chSymLast) {
        sycm.chCurr = chSymFirst;
    }
    if (hwnd != NULL) {
        HDC hdc;

        hdc = GetDC(hwnd);
        UpdateKeystrokeText(hdc, sycm.chCurr, TRUE);
        ReleaseDC(hwnd, hdc);
    } else {
        UpdateKeystrokeText(NULL, sycm.chCurr, FALSE);
    }
}

/****************************************************************************

    FUNCTION: PaintStatusLine(HDC, BOOL, BOOL);

    PURPOSE:  Paints the Help and Keystroke fields in the status bar.

    COMMENTS:  Repaints Help field if fHelp == TRUE, repaints the Keystroke
               field if fKeystroke == TRUE.

****************************************************************************/

VOID PaintStatusLine(
    HDC hdc,
    BOOL fHelp,
    BOOL fKeystroke)
{
    HFONT hfontOld = NULL;
    RECT rect;
    INT dyBorder;
    TCHAR szHelpText[100];

    dyBorder = GetSystemMetrics(SM_CYBORDER);

    if (hfontStatus)
        hfontOld = SelectObject(hdc, hfontStatus);

    // set the text and background colors

    SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));
    SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));

    if (fHelp) {
        // now the help text, with a gray background

        rect.top    = rcStatusLine.top + 3 * dyBorder;
        rect.bottom = rcStatusLine.bottom - 3 * dyBorder;
        rect.left   = 9 * dyBorder;
        rect.right  = rect.left + dxHelpField - 2 * dyBorder;

        LoadString(hInst, iControl, szHelpText, BTOC(sizeof(szHelpText)) - 1);

        ExtTextOut(hdc, rect.left + dyBorder * 2, rect.top,
                   ETO_OPAQUE | ETO_CLIPPED, &rect, szHelpText,
                   lstrlen(szHelpText), NULL);
    }

    if (fKeystroke) {
        // now the keystroke text, with a gray background
        rect.top    = rcStatusLine.top + 3 * dyBorder;
        rect.bottom = rcStatusLine.bottom - 3 * dyBorder;
        rect.right = rcStatusLine.right - 9 * dyBorder;
        rect.left = rect.right - dxKeystrokeField + 2 * dyBorder;

        ExtTextOut(hdc, rect.left + dyBorder * 2, rect.top,
                   ETO_OPAQUE | ETO_CLIPPED, &rect, szKeystrokeText,
                   lstrlen(szKeystrokeText), NULL);
    }

    if (hfontOld)
        SelectObject(hdc, hfontOld);

}

/****************************************************************************

    FUNCTION: DrawFamilyComboItem(LPDRAWITEMSTRUCT)

    PURPOSE:  Paints the font facenames and TT bitmap in the font combo box.

    COMMENTS:

****************************************************************************/

BOOL DrawFamilyComboItem(
    LPDRAWITEMSTRUCT lpdis)
{
    HDC hDC, hdcMem;
    DWORD rgbBack, rgbText;
    TCHAR szFace[LF_FACESIZE];
    HBITMAP hOld;
    INT dy;
    SHORT  sFontType;

    hDC = lpdis->hDC;

    if (lpdis->itemState & ODS_SELECTED) {
        rgbBack = SetBkColor(hDC, GetSysColor(COLOR_HIGHLIGHT));
        rgbText = SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
    } else {
        rgbBack = SetBkColor(hDC, GetSysColor(COLOR_WINDOW));
        rgbText = SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
    }

    SendMessage(lpdis->hwndItem, CB_GETLBTEXT, lpdis->itemID, (LONG)(LPTSTR)szFace);
    ExtTextOut(hDC, lpdis->rcItem.left + DX_BITMAP, lpdis->rcItem.top, ETO_OPAQUE | ETO_CLIPPED, &lpdis->rcItem, szFace, lstrlen(szFace), NULL);

    hdcMem = CreateCompatibleDC(hDC);
    if (hdcMem) {
        if (hbmFont) {
            hOld = SelectObject(hdcMem, hbmFont);
            sFontType = ((ITEMDATA FAR *)&(lpdis->itemData))->FontType;

            if (sFontType)
            {
                int xSrc;

                dy = ((lpdis->rcItem.bottom - lpdis->rcItem.top) - DY_BITMAP) / 2;

                if (sFontType & TRUETYPE_FONT)
                    xSrc = 0;
                else if (sFontType & TT_OPENTYPE_FONT)
                    xSrc = 2;
                else if (sFontType & PS_OPENTYPE_FONT)
                    xSrc = 3;
                else if (sFontType & TYPE1_FONT)
                    xSrc = 4;

                BitBlt(hDC, lpdis->rcItem.left, lpdis->rcItem.top + dy, DX_BITMAP, DY_BITMAP, hdcMem,
                        xSrc*DX_BITMAP, lpdis->itemState & ODS_SELECTED ? DY_BITMAP : 0, SRCCOPY);
            }

            SelectObject(hdcMem, hOld);
        }
        DeleteDC(hdcMem);
    }

    SetTextColor(hDC, rgbText);
    SetBkColor(hDC, rgbBack);

    return TRUE;
}

/****************************************************************************

    FUNCTION: LoadBitmaps(int)

    PURPOSE:  This routine loads DIB bitmaps, and "fixes up" their color tables
              so that we get the desired result for the device we are on.

    COMMENTS: This routine requires:
              - The DIB is a 16 color DIB authored with the standard windows colors.
              - bright blue (00 00 FF) is converted to the background color.
              - light grey  (C0 C0 C0) is replaced with the button face color.
              - dark grey   (80 80 80) is replaced with the button shadow color.

              This means you can't have any of these colors in your bitmap.

****************************************************************************/

HBITMAP LoadBitmaps(INT id)
{
  HDC                   hdc;
  HANDLE                h;
  DWORD FAR             *p;
  LPBYTE                lpBits;
  HANDLE                hRes;
  LPBITMAPINFOHEADER    lpBitmapInfo;
  INT numcolors;
  DWORD rgbSelected;
  DWORD rgbUnselected;
  HBITMAP hbm;

  rgbSelected = GetSysColor(COLOR_HIGHLIGHT);
  // Flip the colors.
  rgbSelected = RGB(GetBValue(rgbSelected),
                    GetGValue(rgbSelected),
                    GetRValue(rgbSelected));
  rgbUnselected = GetSysColor(COLOR_WINDOW);
  // Flip the colors.
  rgbUnselected = RGB(GetBValue(rgbUnselected),
                      GetGValue(rgbUnselected),
                      GetRValue(rgbUnselected));


  h = FindResource(hInst, MAKEINTRESOURCE(id), RT_BITMAP);
  hRes = LoadResource(hInst, h);

  /* Lock the bitmap and get a pointer to the color table. */
  lpBitmapInfo = (LPBITMAPINFOHEADER)LockResource(hRes);

  if (!lpBitmapInfo)
        return FALSE;

  p = (DWORD FAR *)((LPSTR)(lpBitmapInfo) + lpBitmapInfo->biSize);

  /* Search for the Solid Blue entry and replace it with the current
   * background RGB.
   */
  numcolors = 16;

  while (numcolors-- > 0) {
      if (*p == BACKGROUND)
          *p = rgbUnselected;
      else if (*p == BACKGROUNDSEL)
          *p = rgbSelected;
      p++;
  }
  UnlockResource(hRes);

  /* Now create the DIB. */
  lpBitmapInfo = (LPBITMAPINFOHEADER)LockResource(hRes);

  /* First skip over the header structure */
  lpBits = (LPBYTE)(lpBitmapInfo + 1);

  /* Skip the color table entries, if any */
  lpBits += (1 << (lpBitmapInfo->biBitCount)) * sizeof(RGBQUAD);

  /* Create a color bitmap compatible with the display device */
  hdc = GetDC(NULL);
  hbm = CreateDIBitmap(hdc, lpBitmapInfo, (DWORD)CBM_INIT, lpBits, (LPBITMAPINFO)lpBitmapInfo, DIB_RGB_COLORS);
  ReleaseDC(NULL, hdc);

  GlobalUnlock(hRes);
  FreeResource(hRes);

  return hbm;
}


/****************************************************************************

    FUNCTION: DoHelp(HWND, BOOL)

    PURPOSE:  This routine invokes help if BOOL is true, or dismisses help
              if BOOL is false.

    COMMENTS:

****************************************************************************/

VOID DoHelp(HWND hWnd, BOOL fInvokeHelp)
{
    TCHAR szHelp[80];

    if (LoadString(hInst, IDS_HELP, szHelp, BTOC(sizeof(szHelp)) - 1)) {
        if (fInvokeHelp)
            WinHelp(hWnd, (LPTSTR)szHelp, HELP_INDEX, 0L);
        else
            WinHelp(hWnd, (LPTSTR)szHelp, HELP_QUIT, 0L);
    }

}

/****************************************************************************

    FUNCTION: SaveFont(HWND)

    PURPOSE:  Used to save the current font facename in win.ini, so that
              it can be selected the next time charmap comes up.

    COMMENTS:

****************************************************************************/

VOID SaveCurrentFont(HWND hWndDlg)
{
    TCHAR szFaceName[LF_FACESIZE] = TEXT("");

    SendDlgItemMessage(hWndDlg, ID_FONT, CB_GETLBTEXT,
                       (WORD)SendDlgItemMessage(hWndDlg, ID_FONT, CB_GETCURSEL,
                                                0, 0L),
                       (LONG)(LPTSTR)szFaceName);

    WriteProfileString(TEXT("MSCharMap"), TEXT("Font"), (LPTSTR)szFaceName);
}


/****************************************************************************

    FUNCTION: SelectInitialFont(HWND)

    PURPOSE:  Used to select the initial font by getting a saved facename
              from win.ini and selecting it in the combo box.

    COMMENTS: Returns index to font selected.

****************************************************************************/

INT SelectInitialFont(HWND hWndDlg)
{
    TCHAR szFaceName[LF_FACESIZE] = TEXT("");
    INT iIndex;

    if ((GetProfileString(TEXT("MSCharMap"), TEXT("Font"), NULL, (LPTSTR)szFaceName,
                          BTOC(sizeof(szFaceName))) == 0) ||
        ((iIndex = (INT)SendDlgItemMessage(hWndDlg, ID_FONT, CB_SELECTSTRING, (WPARAM)-1,
                               (LONG)(LPTSTR)szFaceName)) == CB_ERR)) {
        /*
         * If there was no profile or the selection failed then try selecting
         * the symbol font, if that fails then select the first one.
         */
        if ((iIndex = (INT)SendDlgItemMessage(hWndDlg, ID_FONT, CB_SELECTSTRING,
                                (WPARAM)-1, (LONG)(LPTSTR)TEXT("Symbol"))) == CB_ERR)
            SendDlgItemMessage(hWndDlg, ID_FONT, CB_SETCURSEL, iIndex = 0, 0L);
    }

    return(iIndex);
}


/****************************************************************************

    FUNCTION: SaveCurrentSubset(HWND)

    PURPOSE:  Used to save the current subset name in win.ini, so that
              it can be selected the next time charmap comes up.

    COMMENTS:

****************************************************************************/

VOID SaveCurrentSubset(HWND hWndDlg)
{
    TCHAR szSubsetName[LF_SUBSETSIZE] = TEXT("");

    SendDlgItemMessage(hWndDlg, ID_UNICODESUBSET, CB_GETLBTEXT,
                       (WORD)SendDlgItemMessage(hWndDlg, ID_UNICODESUBSET, CB_GETCURSEL,
                                                0, 0L),
                       (LONG)(LPTSTR)szSubsetName);

    WriteProfileString(TEXT("MSCharMap"), TEXT("Block"), (LPTSTR)szSubsetName);
}


/****************************************************************************

    FUNCTION: SelectInitialSubset(HWND)

    PURPOSE:  Used to select the initial Unicode subset by getting a saved
              block name from win.ini.

    COMMENTS: Returns index to subset selected.

****************************************************************************/

INT SelectInitialSubset(HWND hWndDlg)
{
    TCHAR szSubsetName[LF_SUBSETSIZE] = TEXT("");
    INT iIndex;

    if (
        (GetProfileString(
             TEXT("MSCharMap"),
             TEXT("Block"),
             NULL,
             (LPTSTR)szSubsetName,
             BTOC(sizeof(szSubsetName))
             ) == 0)
       || ((iIndex = (INT)SendDlgItemMessage(
                              hWndDlg,
                              ID_UNICODESUBSET,
                              CB_SELECTSTRING,
                              (WPARAM)-1,
                              (LONG)(LPTSTR)szSubsetName
                              )) == CB_ERR)) {
        /*
         * If there was no profile or the selection failed then try selecting
         * the Basic Latin block, if that fails then select the first one.
         */
        if ((iIndex = (INT)SendDlgItemMessage(
                               hWndDlg,
                               ID_UNICODESUBSET,
                               CB_SELECTSTRING,
                               (WPARAM)-1,
                               (LONG)(LPTSTR)TEXT("Basic Latin")
                               )) == CB_ERR)
            SendDlgItemMessage(hWndDlg, ID_UNICODESUBSET, CB_SETCURSEL, iIndex = 0, 0L);
    }

    chSymFirst = aSubsetData[iIndex].BeginRange;
    chSymLast = aSubsetData[iIndex].EndRange;
    sycm.chCurr = chSymFirst;

    return iIndex;
}


/****************************************************************************

    FUNCTION: ExitMagnify(HWND, SYCM)

    PURPOSE:  Used to release mouse capture, exit magnify mode,
              and restore the cursor.

    COMMENTS:

****************************************************************************/

VOID ExitMagnify(
    HWND hWnd,
    PSYCM psycm)
{
    // Release capture, remove magnified character, restore cursor.
    ReleaseCapture();
    RestoreSymMag(psycm);
    DrawSymChOutlineHwnd(psycm, hWnd, psycm->chCurr, TRUE, TRUE);
    if (psycm->fCursorOff)
        ShowCursor(TRUE);
    psycm->fMouseDn = psycm->fCursorOff = FALSE;
}
