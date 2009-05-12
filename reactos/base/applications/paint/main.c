/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        main.c
 * PURPOSE:     Initializing everything
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include "definitions.h"

#include "drawing.h"
#include "dib.h"

#include "globalvar.h"
#include "history.h"
#include "mouse.h"

#include "winproc.h"
#include "palette.h"
#include "toolsettings.h"
#include "selection.h"

/* FUNCTIONS ********************************************************/

HDC hDrawingDC;
HDC hSelDC;
int *bmAddress;
BITMAPINFO bitmapinfo;
int imgXRes = 400;
int imgYRes = 300;

HBITMAP hBms[4];
int currInd = 0;
int undoSteps = 0;
int redoSteps = 0;

// global status variables

short startX;
short startY;
short lastX;
short lastY;
int lineWidth = 1;
int shapeStyle = 0;
int brushStyle = 0;
int activeTool = 7;
int airBrushWidth = 5;
int rubberRadius = 4;
int transpBg = 0;
int zoom = 1000;
int rectSel_src[4];
int rectSel_dest[4];
HWND hSelection;
HWND hImageArea;
HBITMAP hSelBm;


// global declarations and WinMain


// initial palette colors; may be changed by the user during execution
int palColors[28] =
    {0x000000, 0x464646, 0x787878, 0x300099, 0x241ced, 0x0078ff, 0x0ec2ff,
    0x00f2ff, 0x1de6a8, 0x4cb122, 0xefb700, 0xf36d4d, 0x99362f, 0x98316f,
    0xffffff, 0xdcdcdc, 0xb4b4b4, 0x3c5a9c, 0xb1a3ff, 0x7aaae5, 0x9ce4f5,
    0xbdf9ff, 0xbcf9d3, 0x61bb9d, 0xead999, 0xd19a70, 0x8e6d54, 0xd5a5b5};
// foreground and background colors with initial value
int fgColor = 0x00000000;
int bgColor = 0x00ffffff;
// the current zoom in percent*10
HWND hStatusBar;
HWND hScrollbox;
HWND hMainWnd;
HWND hPalWin;
HWND hToolSettings;
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

HWND hScrlClient;

HWND hToolBtn[16];

HINSTANCE hProgInstance;

char filename[256];
char filepathname[1000];
BOOL isAFile = FALSE;

int WINAPI WinMain (HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nFunsterStil)
{
    hProgInstance = hThisInstance;
    HWND hwnd;               /* This is the handle for our window */
    MSG messages;            /* Here messages to the application are saved */

    // Necessary
    InitCommonControls();

    //initializing and registering the window class used for the main window
    WNDCLASSEX wincl;
    wincl.hInstance         = hThisInstance;
    wincl.lpszClassName     = "WindowsApp";
    wincl.lpfnWndProc       = WindowProcedure;
    wincl.style             = CS_DBLCLKS;
    wincl.cbSize            = sizeof (WNDCLASSEX);
    wincl.hIcon             = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hIconSm           = LoadIcon (hThisInstance, MAKEINTRESOURCE(500));
    wincl.hCursor           = LoadCursor (NULL, IDC_ARROW);
    wincl.lpszMenuName      = NULL;
    wincl.cbClsExtra        = 0;
    wincl.cbWndExtra        = 0;
    wincl.hbrBackground     = GetSysColorBrush(COLOR_BTNFACE);
    RegisterClassEx (&wincl);

    // initializing and registering the window class used for the scroll box

    WNDCLASSEX wclScroll;
    wclScroll.hInstance     = hThisInstance;
    wclScroll.lpszClassName = "Scrollbox";
    wclScroll.lpfnWndProc   = WindowProcedure;
    wclScroll.style         = 0;
    wclScroll.cbSize        = sizeof (WNDCLASSEX);
    wclScroll.hIcon         = NULL;
    wclScroll.hIconSm       = NULL;
    wclScroll.hCursor       = LoadCursor (NULL, IDC_ARROW);
    wclScroll.lpszMenuName  = NULL;
    wclScroll.cbClsExtra    = 0;
    wclScroll.cbWndExtra    = 0;
    wclScroll.hbrBackground = GetSysColorBrush(COLOR_APPWORKSPACE);
    RegisterClassEx (&wclScroll);

    // initializing and registering the window class used for the palette window

    WNDCLASSEX wclPal;
    wclPal.hInstance        = hThisInstance;
    wclPal.lpszClassName    = "Palette";
    wclPal.lpfnWndProc      = PalWinProc;
    wclPal.style            = CS_DBLCLKS;
    wclPal.cbSize           = sizeof (WNDCLASSEX);
    wclPal.hIcon            = NULL;
    wclPal.hIconSm          = NULL;
    wclPal.hCursor          = LoadCursor (NULL, IDC_ARROW);
    wclPal.lpszMenuName     = NULL;
    wclPal.cbClsExtra       = 0;
    wclPal.cbWndExtra       = 0;
    wclPal.hbrBackground    = GetSysColorBrush(COLOR_BTNFACE);
    RegisterClassEx (&wclPal);

    // initializing and registering the window class for the settings window

    WNDCLASSEX wclSettings;
    wclSettings.hInstance       = hThisInstance;
    wclSettings.lpszClassName   = "ToolSettings";
    wclSettings.lpfnWndProc     = SettingsWinProc;
    wclSettings.style           = CS_DBLCLKS;
    wclSettings.cbSize          = sizeof (WNDCLASSEX);
    wclSettings.hIcon           = NULL;
    wclSettings.hIconSm         = NULL;
    wclSettings.hCursor         = LoadCursor (NULL, IDC_ARROW);
    wclSettings.lpszMenuName    = NULL;
    wclSettings.cbClsExtra      = 0;
    wclSettings.cbWndExtra      = 0;
    wclSettings.hbrBackground   = GetSysColorBrush(COLOR_BTNFACE);
    RegisterClassEx (&wclSettings);

    // initializing and registering the window class for the selection frame

    WNDCLASSEX wclSelection;
    wclSelection.hInstance      = hThisInstance;
    wclSelection.lpszClassName  = "Selection";
    wclSelection.lpfnWndProc    = SelectionWinProc;
    wclSelection.style          = CS_DBLCLKS;
    wclSelection.cbSize         = sizeof (WNDCLASSEX);
    wclSelection.hIcon          = NULL;
    wclSelection.hIconSm        = NULL;
    wclSelection.hCursor        = LoadCursor (NULL, IDC_SIZEALL);
    wclSelection.lpszMenuName   = NULL;
    wclSelection.cbClsExtra     = 0;
    wclSelection.cbWndExtra     = 0;
    wclSelection.hbrBackground  = NULL;//GetSysColorBrush(COLOR_BTNFACE);
    RegisterClassEx (&wclSelection);

    LoadString(hThisInstance, IDS_DEFAULTFILENAME, (LPTSTR)&filename, 256);
    char progtitle[1000];
    char resstr[100];
    LoadString(hThisInstance, IDS_WINDOWTITLE, (LPTSTR)&resstr, 100);
    sprintf(progtitle, resstr, &filename);
    
    
    // create main window
    hwnd = CreateWindowEx (0, "WindowsApp", (LPTSTR)progtitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 544, 375, HWND_DESKTOP, NULL, hThisInstance, NULL);

    hMainWnd = hwnd;

    // loading and setting the window menu from resource
    HMENU menu;
    menu = LoadMenu(hThisInstance, MAKEINTRESOURCE(ID_MENU));
    SetMenu(hwnd, menu);
    HANDLE haccel = LoadAccelerators(hThisInstance, MAKEINTRESOURCE(800));

    // preloading the draw transparent/nontransparent icons for later use
    hNontranspIcon  = LoadImage(hThisInstance, MAKEINTRESOURCE(IDI_NONTRANSPARENT), IMAGE_ICON, 40, 30, LR_DEFAULTCOLOR);
    hTranspIcon     = LoadImage(hThisInstance, MAKEINTRESOURCE(IDI_TRANSPARENT), IMAGE_ICON, 40, 30, LR_DEFAULTCOLOR);

    hCurFill        = LoadIcon(hThisInstance, MAKEINTRESOURCE(IDC_FILL));
    hCurColor       = LoadIcon(hThisInstance, MAKEINTRESOURCE(IDC_COLOR));
    hCurZoom        = LoadIcon(hThisInstance, MAKEINTRESOURCE(IDC_ZOOM));
    hCurPen         = LoadIcon(hThisInstance, MAKEINTRESOURCE(IDC_PEN));
    hCurAirbrush    = LoadIcon(hThisInstance, MAKEINTRESOURCE(IDC_AIRBRUSH));

    HWND hLine = CreateWindowEx (0, "STATIC", "", WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ, 0, 0, 5000, 2, hwnd, NULL, hThisInstance, NULL);

    // creating the 16 bitmap radio buttons and setting the bitmap


    // FIXME: Unintentionally there is a line above the tool bar. To prevent cropping of the buttons height has been increased from 200 to 205
    HWND hToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, WS_CHILD | WS_VISIBLE | CCS_NOPARENTALIGN | CCS_VERT | CCS_NORESIZE | TBSTYLE_TOOLTIPS, 3, 3, 50, 205, hwnd, NULL, hThisInstance, NULL);
    HIMAGELIST hImageList = ImageList_Create(16, 16, ILC_COLOR24 | ILC_MASK, 16, 0);
    SendMessage(hToolbar, TB_SETIMAGELIST, 0, (LPARAM)hImageList);
    HBITMAP tempBm = LoadImage(hThisInstance, MAKEINTRESOURCE(IDB_TOOLBARICONS), IMAGE_BITMAP, 256, 16, 0);
    ImageList_AddMasked(hImageList, tempBm, 0xff00ff);
    DeleteObject(tempBm);
    SendMessage(hToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    int i;
    char tooltips[16][30];
    for (i=0; i<16; i++)
    {
        int wrapnow = 0;
        if (i%2==1) wrapnow = TBSTATE_WRAP;
        LoadString(hThisInstance, IDS_TOOLTIP1+i, (LPTSTR)&tooltips[i], 30);
        TBBUTTON tbbutton = {i, (HMENU)(ID_FREESEL+i), TBSTATE_ENABLED | wrapnow, TBSTYLE_CHECKGROUP, {0}, 0, &tooltips[i]};
        SendMessage(hToolbar, TB_ADDBUTTONS, 1, (LPARAM)&tbbutton);
    }
   // SendMessage(hToolbar, TB_SETROWS, MAKEWPARAM(8, FALSE), (LPARAM)NULL);
    SendMessage(hToolbar, TB_CHECKBUTTON, ID_PEN, MAKELONG(TRUE, 0));
    SendMessage(hToolbar, TB_SETMAXTEXTROWS, 0, 0);

    SendMessage(hToolbar, TB_SETBUTTONSIZE, 0, MAKELONG(25, 25));
   // SendMessage(hToolbar, TB_AUTOSIZE, 0, 0);




    // creating the tool settings child window
    hToolSettings = CreateWindowEx(0, "ToolSettings", "", WS_CHILD | WS_VISIBLE, 7, 210, 42, 140, hwnd, NULL, hThisInstance, NULL);

    // creating the palette child window
    hPalWin = CreateWindowEx(0, "Palette", "", WS_CHILD | WS_VISIBLE, 56, 9, 255, 32, hwnd, NULL, hThisInstance, NULL);

    // creating the scroll box
    hScrollbox = CreateWindowEx (WS_EX_CLIENTEDGE, "Scrollbox", "", WS_CHILD | WS_GROUP | WS_HSCROLL | WS_VSCROLL | WS_VISIBLE, 56, 49, 472, 248, hwnd, NULL, hThisInstance, NULL);

    // creating the status bar
    hStatusBar = CreateWindowEx (0, STATUSCLASSNAME, "", SBARS_SIZEGRIP | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, NULL, hThisInstance, NULL);
    SendMessage(hStatusBar, SB_SETMINHEIGHT, 21, 0);

    hScrlClient = CreateWindowEx(0, "Scrollbox", "", WS_CHILD | WS_VISIBLE, 0, 0, 500, 500, hScrollbox, NULL, hThisInstance, NULL);

    // create selection window (initially hidden)
    hSelection = CreateWindowEx(WS_EX_TRANSPARENT, "Selection", "", WS_CHILD | BS_OWNERDRAW, 350, 0, 100, 100, hScrlClient, NULL, hThisInstance, NULL);

    // creating the window inside the scroll box, on which the image in hDrawingDC's bitmap is drawn
    hImageArea = CreateWindowEx (0, "Scrollbox", "", WS_CHILD | WS_VISIBLE, 3, 3, imgXRes, imgYRes, hScrlClient, NULL, hThisInstance, NULL);

    hDrawingDC = CreateCompatibleDC(GetDC(hImageArea));
    hSelDC = CreateCompatibleDC(GetDC(hImageArea));
    SelectObject(hDrawingDC, CreatePen(PS_SOLID, 0, fgColor));
    SelectObject(hDrawingDC, CreateSolidBrush(bgColor));

    hBms[0] = CreateDIBWithProperties(imgXRes, imgYRes);
    SelectObject(hDrawingDC, hBms[0]);
    Rectangle(hDrawingDC, 0-1, 0-1, imgXRes+1, imgYRes+1);

    // initializing the CHOOSECOLOR structure for use with ChooseColor
    int custColors[16] =
        {0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff,
        0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff};
    choosecolor.lStructSize     = sizeof(CHOOSECOLOR);
    choosecolor.hwndOwner       = hwnd;
    choosecolor.hInstance       = NULL;
    choosecolor.rgbResult       = 0x00ffffff;
    choosecolor.lpCustColors    = (COLORREF*)&custColors;
    choosecolor.Flags           = 0;
    choosecolor.lCustData       = 0;
    choosecolor.lpfnHook        = NULL;
    choosecolor.lpTemplateName  = NULL;

    int c;

    // initializing the OPENFILENAME structure for use with GetOpenFileName and GetSaveFileName
    char ofnFilename[1000];
    CopyMemory(&ofnFilename, &filename, 256);
    char ofnFiletitle[256];
    char ofnFilter[1000];
    LoadString(hThisInstance, IDS_OPENFILTER, (LPTSTR)&ofnFilter, 1000);
    for (c=0; c<1000; c++) if (ofnFilter[c]==(char)1) ofnFilter[c] = (char)0;
    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize     = sizeof (OPENFILENAME);
    ofn.hwndOwner       = hwnd;
    ofn.hInstance       = hThisInstance;
    ofn.lpstrFilter     = (LPCTSTR)&ofnFilter;
    ofn.lpstrFile       = (LPTSTR)&ofnFilename;
    ofn.nMaxFile        = 1000;
    ofn.lpstrFileTitle  = (LPTSTR)&ofnFiletitle;
    ofn.nMaxFileTitle   = 256;
    ofn.Flags           = OFN_HIDEREADONLY;

    char sfnFilename[1000];
    CopyMemory(&sfnFilename, &filename, 256);
    char sfnFiletitle[256];
    char sfnFilter[1000];
    LoadString(hThisInstance, IDS_SAVEFILTER, (LPTSTR)&sfnFilter, 1000);
    for (c=0; c<1000; c++) if (sfnFilter[c]==(char)1) sfnFilter[c] = (char)0;
    ZeroMemory(&sfn, sizeof(OPENFILENAME));
    sfn.lStructSize     = sizeof (OPENFILENAME);
    sfn.hwndOwner       = hwnd;
    sfn.hInstance       = hThisInstance;
    sfn.lpstrFilter     = (LPCTSTR)&sfnFilter;
    sfn.lpstrFile       = (LPTSTR)&sfnFilename;
    sfn.nMaxFile        = 1000;
    sfn.lpstrFileTitle  = (LPTSTR)&sfnFiletitle;
    sfn.nMaxFileTitle   = 256;
    sfn.Flags           = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;


    // by moving the window, the things in WM_SIZE are done
    MoveWindow(hwnd, 100, 100, 600, 450, TRUE);

    /* Make the window visible on the screen */
    ShowWindow (hwnd, nFunsterStil);

    /* Run the message loop. It will run until GetMessage() returns 0 */
    while (GetMessage (&messages, NULL, 0, 0))
    {
        TranslateAccelerator(hwnd, haccel, &messages);

        /* Translate virtual-key messages into character messages */
        TranslateMessage(&messages);
        /* Send message to WindowProcedure */
        DispatchMessage(&messages);
    }

    /* The program return-value is 0 - The value that PostQuitMessage() gave */
    return messages.wParam;
}
