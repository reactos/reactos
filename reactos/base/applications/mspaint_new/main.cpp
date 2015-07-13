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
int *bmAddress;
BITMAPINFO bitmapinfo;
int imgXRes = 400;
int imgYRes = 300;

int widthSetInDlg;
int heightSetInDlg;

STRETCHSKEW stretchSkew;

ImageModel imageModel;
BOOL askBeforeEnlarging = FALSE;  // TODO: initialize from registry

POINT start;
POINT last;

ToolsModel toolsModel;

SelectionModel selectionModel;

LOGFONT lfTextFont;
HFONT hfontTextFont;
HWND hwndEditCtl;
LPTSTR textToolText = NULL;
int textToolTextMaxLen = 0;

PaletteModel paletteModel;

HWND hStatusBar;
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
CToolBox toolBoxContainer;
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
    HANDLE haccel;
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

    hProgInstance = hThisInstance;

    /* initialize common controls library */
    InitCommonControls();

    LoadString(hThisInstance, IDS_DEFAULTFILENAME, filename, SIZEOF(filename));
    LoadString(hThisInstance, IDS_WINDOWTITLE, resstr, SIZEOF(resstr));
    _stprintf(progtitle, resstr, filename);
    LoadString(hThisInstance, IDS_MINIATURETITLE, miniaturetitle, SIZEOF(miniaturetitle));

    /* create main window */
    RECT mainWindowPos = {0, 0, 544, 375};	// FIXME: use equivalent of CW_USEDEFAULT for position
    hwnd = mainWindow.Create(HWND_DESKTOP, mainWindowPos, progtitle, WS_OVERLAPPEDWINDOW);

    RECT miniaturePos = {180, 200, 180 + 120, 200 + 100};
    miniature.Create(hwnd, miniaturePos, miniaturetitle,
                     WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME, WS_EX_PALETTEWINDOW);

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

    CreateWindowEx(0, _T("STATIC"), NULL, WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ, 0, 0, 5000, 2, hwnd, NULL,
                   hThisInstance, NULL);

    RECT toolBoxContainerPos = {2, 2, 2 + 52, 2 + 350};
    toolBoxContainer.Create(hwnd, toolBoxContainerPos, NULL, WS_CHILD | WS_VISIBLE);
    /* creating the tool settings child window */
    RECT toolSettingsWindowPos = {5, 208, 5 + 42, 208 + 140};
    toolSettingsWindow.Create(toolBoxContainer.m_hWnd, toolSettingsWindowPos, NULL, WS_CHILD | WS_VISIBLE);

    /* creating the palette child window */
    RECT paletteWindowPos = {56, 9, 56 + 255, 9 + 32};
    paletteWindow.Create(hwnd, paletteWindowPos, NULL, WS_CHILD | WS_VISIBLE);

    /* creating the scroll box */
    RECT scrollboxWindowPos = {56, 49, 56 + 472, 49 + 248};
    scrollboxWindow.Create(hwnd, scrollboxWindowPos, NULL,
                           WS_CHILD | WS_GROUP | WS_HSCROLL | WS_VSCROLL | WS_VISIBLE, WS_EX_CLIENTEDGE);

    /* creating the status bar */
    hStatusBar =
        CreateWindowEx(0, STATUSCLASSNAME, NULL, SBARS_SIZEGRIP | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd,
                       NULL, hThisInstance, NULL);
    SendMessage(hStatusBar, SB_SETMINHEIGHT, 21, 0);

    RECT scrlClientWindowPos = {0, 0, 0 + 500, 0 + 500};
    scrlClientWindow.Create(scrollboxWindow.m_hWnd, scrlClientWindowPos, NULL, WS_CHILD | WS_VISIBLE);

    /* create selection window (initially hidden) */
    RECT selectionWindowPos = {350, 0, 350 + 100, 0 + 100};
    selectionWindow.Create(scrlClientWindow.m_hWnd, selectionWindowPos, NULL, WS_CHILD | BS_OWNERDRAW);

    /* creating the window inside the scroll box, on which the image in hDrawingDC's bitmap is drawn */
    RECT imageAreaPos = {3, 3, 3 + imgXRes, 3 + imgYRes};
    imageArea.Create(scrlClientWindow.m_hWnd, imageAreaPos, NULL, WS_CHILD | WS_VISIBLE);

    hDC = imageArea.GetDC();
    hDrawingDC = CreateCompatibleDC(hDC);
    selectionModel.SetDC(CreateCompatibleDC(hDC));
    imageArea.ReleaseDC(hDC);
    SelectObject(hDrawingDC, CreatePen(PS_SOLID, 0, paletteModel.GetFgColor()));
    SelectObject(hDrawingDC, CreateSolidBrush(paletteModel.GetBgColor()));

    //TODO: move to ImageModel
    imageModel.hBms[0] = CreateDIBWithProperties(imgXRes, imgYRes);
    SelectObject(hDrawingDC, imageModel.hBms[0]);
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
            imageModel.Insert(bmNew);
            GetFullPathName(lpszArgument, SIZEOF(filepathname), filepathname, &temp);
            _tcscpy(filename, temp);
            LoadString(hProgInstance, IDS_WINDOWTITLE, resstr, SIZEOF(resstr));
            _stprintf(tempstr, resstr, filename);
            mainWindow.SetWindowText(tempstr);
            imageModel.ClearHistory();
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
    sizeboxLeftTop.Create(scrlClientWindow.m_hWnd, sizeboxPos, NULL, WS_CHILD | WS_VISIBLE);
    sizeboxCenterTop.Create(scrlClientWindow.m_hWnd, sizeboxPos, NULL, WS_CHILD | WS_VISIBLE);
    sizeboxRightTop.Create(scrlClientWindow.m_hWnd, sizeboxPos, NULL, WS_CHILD | WS_VISIBLE);
    sizeboxLeftCenter.Create(scrlClientWindow.m_hWnd, sizeboxPos, NULL, WS_CHILD | WS_VISIBLE);
    sizeboxRightCenter.Create(scrlClientWindow.m_hWnd, sizeboxPos, NULL, WS_CHILD | WS_VISIBLE);
    sizeboxLeftBottom.Create(scrlClientWindow.m_hWnd, sizeboxPos, NULL, WS_CHILD | WS_VISIBLE);
    sizeboxCenterBottom.Create(scrlClientWindow.m_hWnd, sizeboxPos, NULL, WS_CHILD | WS_VISIBLE);
    sizeboxRightBottom.Create(scrlClientWindow.m_hWnd, sizeboxPos, NULL, WS_CHILD | WS_VISIBLE);
    /* placing the size boxes around the image */
    imageArea.SendMessage(WM_SIZE, 0, 0);

    /* by moving the window, the things in WM_SIZE are done */
    MoveWindow(hwnd, 100, 100, 600, 450, TRUE);

    /* creating the text editor window for the text tool */
    RECT textEditWindowPos = {300, 0, 300 + 300, 0 + 200};
    textEditWindow.Create(hwnd, textEditWindowPos, NULL, WS_OVERLAPPEDWINDOW);

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
