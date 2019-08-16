/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/main.cpp
 * PURPOSE:     Initializing everything
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

/* FUNCTIONS ********************************************************/

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

RegistrySettings registrySettings;

ImageModel imageModel;
BOOL askBeforeEnlarging = FALSE;  // TODO: initialize from registry

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

TCHAR filepathname[1000];
BOOL isAFile = FALSE;
int fileSize;
int fileHPPM = 2834;
int fileVPPM = 2834;
SYSTEMTIME fileTime;

BOOL showGrid = FALSE;
BOOL showMiniature = FALSE;

CMainWindow mainWindow;
CFullscreenWindow fullscreenWindow;
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

// get file name extension from filter string
static BOOL
FileExtFromFilter(LPTSTR pExt, LPCTSTR pTitle, OPENFILENAME *pOFN)
{
    LPTSTR pchExt = pExt;
    *pchExt = 0;

    DWORD nIndex = 1;
    for (LPCTSTR pch = pOFN->lpstrFilter; *pch; ++nIndex)
    {
        pch += lstrlen(pch) + 1;
        if (pOFN->nFilterIndex == nIndex)
        {
            for (++pch; *pch && *pch != _T(';'); ++pch)
            {
                *pchExt++ = *pch;
            }
            *pchExt = 0;
            CharLower(pExt);
            return TRUE;
        }
        pch += lstrlen(pch) + 1;
    }
    return FALSE;
}

// Hook procedure for OPENFILENAME to change the file name extension
static UINT_PTR APIENTRY
OFNHookProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hParent;
    OFNOTIFY *pon;
    switch (uMsg)
    {
    case WM_NOTIFY:
        pon = (OFNOTIFY *)lParam;
        if (pon->hdr.code == CDN_TYPECHANGE)
        {
            hParent = GetParent(hwnd);
            TCHAR Path[MAX_PATH];
            SendMessage(hParent, CDM_GETFILEPATH, SIZEOF(Path), (LPARAM)Path);
            LPTSTR pchTitle = _tcsrchr(Path, _T('\\'));
            if (pchTitle == NULL)
                pchTitle = _tcsrchr(Path, _T('/'));

            LPTSTR pch = _tcsrchr((pchTitle ? pchTitle : Path), _T('.'));
            if (pch && pchTitle)
            {
                pchTitle++;
                *pch = 0;
                FileExtFromFilter(pch, pchTitle, pon->lpOFN);
                SendMessage(hParent, CDM_SETCONTROLTEXT, 0x047c, (LPARAM)pchTitle);
                lstrcpyn(pon->lpOFN->lpstrFile, Path, pon->lpOFN->nMaxFile);
            }
        }
        break;
    }
    return 0;
}

/* entry point */

int WINAPI
_tWinMain (HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPTSTR lpszArgument, int nFunsterStil)
{
    HWND hwnd;               /* This is the handle for our window */
    MSG messages;            /* Here messages to the application are saved */

    HMENU menu;
    HACCEL haccel;

    TCHAR sfnFilename[1000];
    TCHAR sfnFiletitle[256];
    TCHAR ofnFilename[1000];
    TCHAR ofnFiletitle[256];
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

    LoadString(hThisInstance, IDS_DEFAULTFILENAME, filepathname, SIZEOF(filepathname));
    CPath pathFileName(filepathname);
    pathFileName.StripPath();
    CString strTitle;
    strTitle.Format(IDS_WINDOWTITLE, (LPCTSTR)pathFileName);
    LoadString(hThisInstance, IDS_MINIATURETITLE, miniaturetitle, SIZEOF(miniaturetitle));

    /* load settings from registry */
    registrySettings.Load();
    showMiniature = registrySettings.ShowThumbnail;
    imageModel.Crop(registrySettings.BMPWidth, registrySettings.BMPHeight);

    /* create main window */
    RECT mainWindowPos = {0, 0, 544, 375};	// FIXME: use equivalent of CW_USEDEFAULT for position
    hwnd = mainWindow.Create(HWND_DESKTOP, mainWindowPos, strTitle, WS_OVERLAPPEDWINDOW);

    RECT fullscreenWindowPos = {0, 0, 100, 100};
    fullscreenWindow.Create(HWND_DESKTOP, fullscreenWindowPos, NULL, WS_POPUPWINDOW | WS_MAXIMIZE);

    RECT miniaturePos = {(LONG) registrySettings.ThumbXPos, (LONG) registrySettings.ThumbYPos,
                         (LONG) registrySettings.ThumbXPos + (LONG) registrySettings.ThumbWidth,
                         (LONG) registrySettings.ThumbYPos + (LONG) registrySettings.ThumbHeight};
    miniature.Create(hwnd, miniaturePos, miniaturetitle,
                     WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME, WS_EX_PALETTEWINDOW);
    miniature.ShowWindow(showMiniature ? SW_SHOW : SW_HIDE);

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
    RECT imageAreaPos = {3, 3, 3 + imageModel.GetWidth(), 3 + imageModel.GetHeight()};
    imageArea.Create(scrlClientWindow.m_hWnd, imageAreaPos, NULL, WS_CHILD | WS_VISIBLE);

    if (__argc >= 2)
    {
        WIN32_FIND_DATAW find;
        HANDLE hFind = FindFirstFileW(__targv[1], &find);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            FindClose(hFind);

            // check the file size
            if (find.nFileSizeHigh || find.nFileSizeLow)
            {
                // load it now
                HBITMAP bmNew = NULL;
                LoadDIBFromFile(&bmNew, __targv[1], &fileTime, &fileSize, &fileHPPM, &fileVPPM);
                if (bmNew)
                {
                    // valid bitmap file
                    GetFullPathName(__targv[1], SIZEOF(filepathname), filepathname, NULL);
                    imageModel.Insert(bmNew);
                    CPath pathFileName(filepathname);
                    pathFileName.StripPath();

                    CString strTitle;
                    strTitle.Format(IDS_WINDOWTITLE, (LPCTSTR)pathFileName);
                    mainWindow.SetWindowText(strTitle);

                    imageModel.ClearHistory();

                    isAFile = TRUE;
                    registrySettings.SetMostRecentFile(filepathname);
                }
                else
                {
                    // cannot open and not empty
                    CStringW strText;
                    strText.Format(IDS_LOADERRORTEXT, __targv[1]);
                    MessageBoxW(NULL, strText, NULL, MB_ICONERROR);
                }
            }
            else
            {
                // open the empty file
                GetFullPathName(__targv[1], SIZEOF(filepathname), filepathname, NULL);
                CPath pathFileName(filepathname);
                pathFileName.StripPath();

                CString strTitle;
                strTitle.Format(IDS_WINDOWTITLE, (LPCTSTR)pathFileName);
                mainWindow.SetWindowText(strTitle);

                imageModel.ClearHistory();

                isAFile = TRUE;
                registrySettings.SetMostRecentFile(filepathname);
            }
        }
        else
        {
            // does not exist
            CStringW strText;
            strText.Format(IDS_LOADERRORTEXT, __targv[1]);
            MessageBoxW(NULL, strText, NULL, MB_ICONERROR);
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
    CopyMemory(ofnFilename, filepathname, sizeof(filepathname));
    CString strImporters;
    CSimpleArray<GUID> aguidFileTypesI;
    CString strAllPictureFiles;
    strAllPictureFiles.LoadString(hThisInstance, IDS_ALLPICTUREFILES);
    CImage::GetImporterFilterString(strImporters, aguidFileTypesI, strAllPictureFiles, CImage::excludeDefaultLoad, _T('\0'));
//     CAtlStringW strAllFiles;
//     strAllFiles.LoadString(hThisInstance, IDS_ALLFILES);
//     strImporters = strAllFiles + CAtlStringW(_T("|*.*|")).Replace('|', '\0') + strImporters;
    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize    = sizeof(OPENFILENAME);
    ofn.hwndOwner      = hwnd;
    ofn.hInstance      = hThisInstance;
    ofn.lpstrFilter    = strImporters;
    ofn.lpstrFile      = ofnFilename;
    ofn.nMaxFile       = SIZEOF(ofnFilename);
    ofn.lpstrFileTitle = ofnFiletitle;
    ofn.nMaxFileTitle  = SIZEOF(ofnFiletitle);
    ofn.Flags          = OFN_HIDEREADONLY;
    ofn.lpstrDefExt    = L"bmp";

    CopyMemory(sfnFilename, filepathname, sizeof(filepathname));
    CString strExporters;
    CSimpleArray<GUID> aguidFileTypesE;
    CImage::GetExporterFilterString(strExporters, aguidFileTypesE, NULL, CImage::excludeDefaultSave, _T('\0'));
    ZeroMemory(&sfn, sizeof(OPENFILENAME));
    sfn.lStructSize    = sizeof(OPENFILENAME);
    sfn.hwndOwner      = hwnd;
    sfn.hInstance      = hThisInstance;
    sfn.lpstrFilter    = strExporters;
    sfn.lpstrFile      = sfnFilename;
    sfn.nMaxFile       = SIZEOF(sfnFilename);
    sfn.lpstrFileTitle = sfnFiletitle;
    sfn.nMaxFileTitle  = SIZEOF(sfnFiletitle);
    sfn.Flags          = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_EXPLORER | OFN_ENABLEHOOK;
    sfn.lpfnHook       = OFNHookProc;
    sfn.lpstrDefExt    = L"bmp";

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
    mainWindow.SetWindowPlacement(&(registrySettings.WindowPlacement));

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
        TranslateAccelerator(hwnd, haccel, &messages);

        /* Translate virtual-key messages into character messages */
        TranslateMessage(&messages);
        /* Send message to WindowProcedure */
        DispatchMessage(&messages);
    }

    /* write back settings to registry */
    registrySettings.ShowThumbnail = showMiniature;
    registrySettings.BMPWidth = imageModel.GetWidth();
    registrySettings.BMPHeight = imageModel.GetHeight();
    registrySettings.Store();

    /* The program return-value is 0 - The value that PostQuitMessage() gave */
    return messages.wParam;
}
