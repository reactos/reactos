/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/main.cpp
 * PURPOSE:     Initializing everything
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

/* FUNCTIONS ********************************************************/

HDC hDrawingDC;
HDC hSelDC;
int *bmAddress;
BITMAPINFO bitmapinfo;
int imgXRes = 400;
int imgYRes = 300;

int widthSetInDlg;
int heightSetInDlg;

STRETCHSKEW stretchSkew;

HBITMAP hBms[HISTORYSIZE];
int currInd = 0;
int undoSteps = 0;
int redoSteps = 0;
BOOL imageSaved = TRUE;

POINT start;
POINT last;
int lineWidth = 1;
int shapeStyle = 0;
int brushStyle = 0;
int activeTool = TOOL_PEN;
int airBrushWidth = 5;
int rubberRadius = 4;
int transpBg = 0;
int zoom = 1000;
RECT rectSel_src;
RECT rectSel_dest;
HBITMAP hSelBm;
HBITMAP hSelMask;
LOGFONT lfTextFont;
HFONT hfontTextFont;
HWND hwndEditCtl;
LPTSTR textToolText = NULL;
int textToolTextMaxLen = 0;

/* array holding palette colors; may be changed by the user during execution */
int palColors[28];

/* modern palette */
int modernPalColors[28] = { 0x000000, 0x464646, 0x787878, 0x300099, 0x241ced, 0x0078ff, 0x0ec2ff,
    0x00f2ff, 0x1de6a8, 0x4cb122, 0xefb700, 0xf36d4d, 0x99362f, 0x98316f,
    0xffffff, 0xdcdcdc, 0xb4b4b4, 0x3c5a9c, 0xb1a3ff, 0x7aaae5, 0x9ce4f5,
    0xbdf9ff, 0xbcf9d3, 0x61bb9d, 0xead999, 0xd19a70, 0x8e6d54, 0xd5a5b5
};

/* older palette containing VGA colors */
int oldPalColors[28] = { 0x000000, 0x808080, 0x000080, 0x008080, 0x008000, 0x808000, 0x800000,
    0x800080, 0x408080, 0x404000, 0xff8000, 0x804000, 0xff0040, 0x004080,
    0xffffff, 0xc0c0c0, 0x0000ff, 0x00ffff, 0x00ff00, 0xffff00, 0xff0000,
    0xff00ff, 0x80ffff, 0x80ff00, 0xffff80, 0xff8080, 0x8000ff, 0x4080ff
};

/* palette currently in use (1: modern, 2: old) */
int selectedPalette;

/* foreground and background colors with initial value */
int fgColor = 0x00000000;
int bgColor = 0x00ffffff;

HWND hStatusBar;
HWND hTrackbarZoom;
CHOOSECOLOR choosecolor;
OPENFILENAME ofn;
OPENFILENAME sfn;
HICON hNontranspIcon;
HICON hTranspIcon;

HCURSOR hCurFill;
HCURSOR hCurColor;
HCURSOR hCurZoom;
HCURSOR hCurPen;
HCURSOR hCurAirbrush;

HWND hToolBtn[16];

HINSTANCE hProgInstance;

TCHAR filename[256];
TCHAR filepathname[1000];
BOOL isAFile = FALSE;
int fileSize;
int fileHPPM = 2834;
int fileVPPM = 2834;
SYSTEMTIME fileTime;

BOOL showGrid = FALSE;
BOOL showMiniature = FALSE;

CMainWindow mainWindow;
CMiniatureWindow miniature;
CMainWindow toolBoxContainer;
CToolSettingsWindow toolSettingsWindow;
CPaletteWindow paletteWindow;
CScrollboxWindow scrollboxWindow;
CScrollboxWindow scrlClientWindow;
CSelectionWindow selectionWindow;
CImgAreaWindow imageArea;
CSizeboxWindow sizeboxLeftTop;
CSizeboxWindow sizeboxCenterTop;
CSizeboxWindow sizeboxRightTop;
CSizeboxWindow sizeboxLeftCenter;
CSizeboxWindow sizeboxRightCenter;
CSizeboxWindow sizeboxLeftBottom;
CSizeboxWindow sizeboxCenterBottom;
CSizeboxWindow sizeboxRightBottom;
CTextEditWindow textEditWindow;

/* entry point */

int WINAPI
_tWinMain (HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPTSTR lpszArgument, int nFunsterStil)
{
    HWND hwnd;               /* This is the handle for our window */
    MSG messages;            /* Here messages to the application are saved */

    TCHAR progtitle[1000];
    TCHAR resstr[100];
    HMENU menu;
    HWND hToolbar;
    HIMAGELIST hImageList;
    HANDLE haccel;
    HBITMAP tempBm;
    int i;
    TCHAR tooltips[16][30];
    HDC hDC;

    TCHAR *c;
    TCHAR sfnFilename[1000];
    TCHAR sfnFiletitle[256];
    TCHAR sfnFilter[1000];
    TCHAR ofnFilename[1000];
    TCHAR ofnFiletitle[256];
    TCHAR ofnFilter[1000];
    TCHAR miniaturetitle[100];
    static int custColors[16] = { 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff,
        0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff
    };

    /* init font for text tool */
    lfTextFont.lfHeight = 0;
    lfTextFont.lfWidth = 0;
    lfTextFont.lfEscapement = 0;
    lfTextFont.lfOrientation = 0;
    lfTextFont.lfWeight = FW_NORMAL;
    lfTextFont.lfItalic = FALSE;
    lfTextFont.lfUnderline = FALSE;
    lfTextFont.lfStrikeOut = FALSE;
    lfTextFont.lfCharSet = DEFAULT_CHARSET;
    lfTextFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lfTextFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lfTextFont.lfQuality = DEFAULT_QUALITY;
    lfTextFont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    lstrcpy(lfTextFont.lfFaceName, _T(""));
    hfontTextFont = CreateFontIndirect(&lfTextFont);

    /* init palette */
    selectedPalette = 1;
    CopyMemory(palColors, modernPalColors, sizeof(palColors));

    hProgInstance = hThisInstance;

    /* initialize common controls library */
    InitCommonControls();

    LoadString(hThisInstance, IDS_DEFAULTFILENAME, filename, SIZEOF(filename));
    LoadString(hThisInstance, IDS_WINDOWTITLE, resstr, SIZEOF(resstr));
    _stprintf(progtitle, resstr, filename);
    LoadString(hThisInstance, IDS_MINIATURETITLE, miniaturetitle, SIZEOF(miniaturetitle));

    /* create main window */
    RECT mainWindowPos = {0, 0, 544, 375};	// FIXME: use equivalent of CW_USEDEFAULT for position
    hwnd = mainWindow.Create(HWND_DESKTOP, mainWindowPos, progtitle, WS_OVERLAPPEDWINDOW, 0, 0U, NULL);

    RECT miniaturePos = {180, 200, 180 + 120, 200 + 100};
    miniature.Create(hwnd, miniaturePos, miniaturetitle,
                     WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME, WS_EX_PALETTEWINDOW, 0U, NULL);

    /* loading and setting the window menu from resource */
    menu = LoadMenu(hThisInstance, MAKEINTRESOURCE(ID_MENU));
    SetMenu(hwnd, menu);
    haccel = LoadAccelerators(hThisInstance, MAKEINTRESOURCE(800));

    /* preloading the draw transparent/nontransparent icons for later use */
    hNontranspIcon =
        (HICON) LoadImage(hThisInstance, MAKEINTRESOURCE(IDI_NONTRANSPARENT), IMAGE_ICON, 40, 30, LR_DEFAULTCOLOR);
    hTranspIcon =
        (HICON) LoadImage(hThisInstance, MAKEINTRESOURCE(IDI_TRANSPARENT), IMAGE_ICON, 40, 30, LR_DEFAULTCOLOR);

    hCurFill     = LoadIcon(hThisInstance, MAKEINTRESOURCE(IDC_FILL));
    hCurColor    = LoadIcon(hThisInstance, MAKEINTRESOURCE(IDC_COLOR));
    hCurZoom     = LoadIcon(hThisInstance, MAKEINTRESOURCE(IDC_ZOOM));
    hCurPen      = LoadIcon(hThisInstance, MAKEINTRESOURCE(IDC_PEN));
    hCurAirbrush = LoadIcon(hThisInstance, MAKEINTRESOURCE(IDC_AIRBRUSH));

    CreateWindowEx(0, _T("STATIC"), _T(""), WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ, 0, 0, 5000, 2, hwnd, NULL,
                   hThisInstance, NULL);

    RECT toolBoxContainerPos = {2, 2, 2 + 52, 2 + 350};
    toolBoxContainer.Create(hwnd, toolBoxContainerPos, _T(""), WS_CHILD | WS_VISIBLE, 0, 0U, NULL);
    /* creating the 16 bitmap radio buttons and setting the bitmap */


    /*
     * FIXME: Unintentionally there is a line above the tool bar (hidden by y-offset).
     * To prevent cropping of the buttons height has been increased from 200 to 205
     */
    hToolbar =
        CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
                       WS_CHILD | WS_VISIBLE | CCS_NOPARENTALIGN | CCS_VERT | CCS_NORESIZE | TBSTYLE_TOOLTIPS,
                       1, -2, 50, 205, toolBoxContainer.m_hWnd, NULL, hThisInstance, NULL);
    hImageList = ImageList_Create(16, 16, ILC_COLOR24 | ILC_MASK, 16, 0);
    SendMessage(hToolbar, TB_SETIMAGELIST, 0, (LPARAM) hImageList);
    tempBm = (HBITMAP) LoadImage(hThisInstance, MAKEINTRESOURCE(IDB_TOOLBARICONS), IMAGE_BITMAP, 256, 16, 0);
    ImageList_AddMasked(hImageList, tempBm, 0xff00ff);
    DeleteObject(tempBm);
    SendMessage(hToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    for(i = 0; i < 16; i++)
    {
        TBBUTTON tbbutton;
        int wrapnow = 0;

        if (i % 2 == 1)
            wrapnow = TBSTATE_WRAP;

        LoadString(hThisInstance, IDS_TOOLTIP1 + i, tooltips[i], 30);
        ZeroMemory(&tbbutton, sizeof(TBBUTTON));
        tbbutton.iString   = (INT_PTR) tooltips[i];
        tbbutton.fsStyle   = TBSTYLE_CHECKGROUP;
        tbbutton.fsState   = TBSTATE_ENABLED | wrapnow;
        tbbutton.idCommand = ID_FREESEL + i;
        tbbutton.iBitmap   = i;
        SendMessage(hToolbar, TB_ADDBUTTONS, 1, (LPARAM) &tbbutton);
    }

    SendMessage(hToolbar, TB_CHECKBUTTON, ID_PEN, MAKELONG(TRUE, 0));
    SendMessage(hToolbar, TB_SETMAXTEXTROWS, 0, 0);
    SendMessage(hToolbar, TB_SETBUTTONSIZE, 0, MAKELONG(25, 25));

    /* creating the tool settings child window */
    RECT toolSettingsWindowPos = {5, 208, 5 + 42, 208 + 140};
    toolSettingsWindow.Create(toolBoxContainer.m_hWnd, toolSettingsWindowPos, _T(""), WS_CHILD | WS_VISIBLE, 0, 0U, NULL);
    hTrackbarZoom =
        CreateWindowEx(0, TRACKBAR_CLASS, _T(""), WS_CHILD | TBS_VERT | TBS_AUTOTICKS, 1, 1, 40, 64,
                       toolSettingsWindow.m_hWnd, NULL, hThisInstance, NULL);
    SendMessage(hTrackbarZoom, TBM_SETRANGE, (WPARAM) TRUE, (LPARAM) MAKELONG(0, 6));
    SendMessage(hTrackbarZoom, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) 3);

    /* creating the palette child window */
    RECT paletteWindowPos = {56, 9, 56 + 255, 9 + 32};
    paletteWindow.Create(hwnd, paletteWindowPos, _T(""), WS_CHILD | WS_VISIBLE, 0, 0U, NULL);

    /* creating the scroll box */
    RECT scrollboxWindowPos = {56, 49, 56 + 472, 49 + 248};
    scrollboxWindow.Create(hwnd, scrollboxWindowPos, _T(""),
                           WS_CHILD | WS_GROUP | WS_HSCROLL | WS_VSCROLL | WS_VISIBLE, WS_EX_CLIENTEDGE, 0U, NULL);

    /* creating the status bar */
    hStatusBar =
        CreateWindowEx(0, STATUSCLASSNAME, _T(""), SBARS_SIZEGRIP | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd,
                       NULL, hThisInstance, NULL);
    SendMessage(hStatusBar, SB_SETMINHEIGHT, 21, 0);

    RECT scrlClientWindowPos = {0, 0, 0 + 500, 0 + 500};
    scrlClientWindow.Create(scrollboxWindow.m_hWnd, scrlClientWindowPos, _T(""), WS_CHILD | WS_VISIBLE, 0, 0U, NULL);

    /* create selection window (initially hidden) */
    RECT selectionWindowPos = {350, 0, 350 + 100, 0 + 100};
    selectionWindow.Create(scrlClientWindow.m_hWnd, selectionWindowPos, _T(""), WS_CHILD | BS_OWNERDRAW, 0, 0U, NULL);

    /* creating the window inside the scroll box, on which the image in hDrawingDC's bitmap is drawn */
    RECT imageAreaPos = {3, 3, 3 + imgXRes, 3 + imgYRes};
    imageArea.Create(scrlClientWindow.m_hWnd, imageAreaPos, _T(""), WS_CHILD | WS_VISIBLE, 0, 0U, NULL);

    hDC = imageArea.GetDC();
    hDrawingDC = CreateCompatibleDC(hDC);
    hSelDC     = CreateCompatibleDC(hDC);
    imageArea.ReleaseDC(hDC);
    SelectObject(hDrawingDC, CreatePen(PS_SOLID, 0, fgColor));
    SelectObject(hDrawingDC, CreateSolidBrush(bgColor));

    hBms[0] = CreateDIBWithProperties(imgXRes, imgYRes);
    SelectObject(hDrawingDC, hBms[0]);
    Rectangle(hDrawingDC, 0 - 1, 0 - 1, imgXRes + 1, imgYRes + 1);

    if (lpszArgument[0] != 0)
    {
        HBITMAP bmNew = NULL;
        LoadDIBFromFile(&bmNew, lpszArgument, &fileTime, &fileSize, &fileHPPM, &fileVPPM);
        if (bmNew != NULL)
        {
            TCHAR tempstr[1000];
            TCHAR resstr[100];
            TCHAR *temp;
            insertReversible(bmNew);
            GetFullPathName(lpszArgument, SIZEOF(filepathname), filepathname, &temp);
            _tcscpy(filename, temp);
            LoadString(hProgInstance, IDS_WINDOWTITLE, resstr, SIZEOF(resstr));
            _stprintf(tempstr, resstr, filename);
            mainWindow.SetWindowText(tempstr);
            clearHistory();
            isAFile = TRUE;
        }
        else
        {
            exit(0);
        }
    }

    /* initializing the CHOOSECOLOR structure for use with ChooseColor */
    choosecolor.lStructSize    = sizeof(CHOOSECOLOR);
    choosecolor.hwndOwner      = hwnd;
    choosecolor.hInstance      = NULL;
    choosecolor.rgbResult      = 0x00ffffff;
    choosecolor.lpCustColors   = (COLORREF*) &custColors;
    choosecolor.Flags          = 0;
    choosecolor.lCustData      = 0;
    choosecolor.lpfnHook       = NULL;
    choosecolor.lpTemplateName = NULL;

    /* initializing the OPENFILENAME structure for use with GetOpenFileName and GetSaveFileName */
    CopyMemory(ofnFilename, filename, sizeof(filename));
    LoadString(hThisInstance, IDS_OPENFILTER, ofnFilter, SIZEOF(ofnFilter));
    for(c = ofnFilter; *c; c++)
        if (*c == '\1')
            *c = '\0';
    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize    = sizeof(OPENFILENAME);
    ofn.hwndOwner      = hwnd;
    ofn.hInstance      = hThisInstance;
    ofn.lpstrFilter    = ofnFilter;
    ofn.lpstrFile      = ofnFilename;
    ofn.nMaxFile       = SIZEOF(ofnFilename);
    ofn.lpstrFileTitle = ofnFiletitle;
    ofn.nMaxFileTitle  = SIZEOF(ofnFiletitle);
    ofn.Flags          = OFN_HIDEREADONLY;

    CopyMemory(sfnFilename, filename, sizeof(filename));
    LoadString(hThisInstance, IDS_SAVEFILTER, sfnFilter, SIZEOF(sfnFilter));
    for(c = sfnFilter; *c; c++)
        if (*c == '\1')
            *c = '\0';
    ZeroMemory(&sfn, sizeof(OPENFILENAME));
    sfn.lStructSize    = sizeof(OPENFILENAME);
    sfn.hwndOwner      = hwnd;
    sfn.hInstance      = hThisInstance;
    sfn.lpstrFilter    = sfnFilter;
    sfn.lpstrFile      = sfnFilename;
    sfn.nMaxFile       = SIZEOF(sfnFilename);
    sfn.lpstrFileTitle = sfnFiletitle;
    sfn.nMaxFileTitle  = SIZEOF(sfnFiletitle);
    sfn.Flags          = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;

    /* creating the size boxes */
    RECT sizeboxPos = {0, 0, 0 + 3, 0 + 3};
    sizeboxLeftTop.Create(scrlClientWindow.m_hWnd, sizeboxPos, _T(""), WS_CHILD | WS_VISIBLE, 0, 0U, NULL);
    sizeboxCenterTop.Create(scrlClientWindow.m_hWnd, sizeboxPos, _T(""), WS_CHILD | WS_VISIBLE, 0, 0U, NULL);
    sizeboxRightTop.Create(scrlClientWindow.m_hWnd, sizeboxPos, _T(""), WS_CHILD | WS_VISIBLE, 0, 0U, NULL);
    sizeboxLeftCenter.Create(scrlClientWindow.m_hWnd, sizeboxPos, _T(""), WS_CHILD | WS_VISIBLE, 0, 0U, NULL);
    sizeboxRightCenter.Create(scrlClientWindow.m_hWnd, sizeboxPos, _T(""), WS_CHILD | WS_VISIBLE, 0, 0U, NULL);
    sizeboxLeftBottom.Create(scrlClientWindow.m_hWnd, sizeboxPos, _T(""), WS_CHILD | WS_VISIBLE, 0, 0U, NULL);
    sizeboxCenterBottom.Create(scrlClientWindow.m_hWnd, sizeboxPos, _T(""), WS_CHILD | WS_VISIBLE, 0, 0U, NULL);
    sizeboxRightBottom.Create(scrlClientWindow.m_hWnd, sizeboxPos, _T(""), WS_CHILD | WS_VISIBLE, 0, 0U, NULL);
    /* placing the size boxes around the image */
    imageArea.SendMessage(WM_SIZE, 0, 0);

    /* by moving the window, the things in WM_SIZE are done */
    MoveWindow(hwnd, 100, 100, 600, 450, TRUE);

    /* creating the text editor window for the text tool */
    RECT textEditWindowPos = {300, 0, 300 + 300, 0 + 200};
    textEditWindow.Create(hwnd, textEditWindowPos, _T(""), WS_OVERLAPPEDWINDOW, 0, 0U, NULL);
    /* creating the edit control within the editor window */
    hwndEditCtl =
        CreateWindowEx(WS_EX_CLIENTEDGE, _T("EDIT"), _T(""),
                       WS_CHILD | WS_VISIBLE | WS_BORDER | WS_HSCROLL | WS_VSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_AUTOHSCROLL | ES_AUTOVSCROLL,
                       0, 0, 100, 100, textEditWindow.m_hWnd, NULL, hThisInstance, NULL);

    /* Make the window visible on the screen */
    ShowWindow (hwnd, nFunsterStil);

    /* inform the system, that the main window accepts dropped files */
    DragAcceptFiles(hwnd, TRUE);

    /* Run the message loop. It will run until GetMessage() returns 0 */
    while (GetMessage(&messages, NULL, 0, 0))
    {
        TranslateAccelerator(hwnd, (HACCEL) haccel, &messages);

        /* Translate virtual-key messages into character messages */
        TranslateMessage(&messages);
        /* Send message to WindowProcedure */
        DispatchMessage(&messages);
    }

    /* The program return-value is 0 - The value that PostQuitMessage() gave */
    return messages.wParam;
}
