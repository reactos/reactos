/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/globalvar.h
 * PURPOSE:     Declaring global variables for later initialization
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

//#include <windows.h>
//#include "definitions.h"

/* TYPES ************************************************************/

typedef struct tagSTRETCHSKEW {
    POINT percentage;
    POINT angle;
} STRETCHSKEW;

/* VARIABLES declared in main.c *************************************/

extern HDC hDrawingDC;
extern HDC hSelDC;
extern int *bmAddress;
extern BITMAPINFO bitmapinfo;
extern int imgXRes;
extern int imgYRes;

extern int widthSetInDlg;
extern int heightSetInDlg;

extern STRETCHSKEW stretchSkew;

extern HBITMAP hBms[HISTORYSIZE];
extern int currInd;
extern int undoSteps;
extern int redoSteps;
extern BOOL imageSaved;

extern POINT start;
extern POINT last;

class ToolsModel;
extern ToolsModel toolsModel;

extern RECT rectSel_src;
extern RECT rectSel_dest;
extern HBITMAP hSelBm;
extern HBITMAP hSelMask;
extern HWND hwndEditCtl;
extern LOGFONT lfTextFont;
extern HFONT hfontTextFont;
extern LPTSTR textToolText;
extern int textToolTextMaxLen;

class PaletteModel;
extern PaletteModel paletteModel;

extern HWND hStatusBar;
extern CHOOSECOLOR choosecolor;
extern OPENFILENAME ofn;
extern OPENFILENAME sfn;
extern HICON hNontranspIcon;
extern HICON hTranspIcon;

extern HCURSOR hCurFill;
extern HCURSOR hCurColor;
extern HCURSOR hCurZoom;
extern HCURSOR hCurPen;
extern HCURSOR hCurAirbrush;

extern HWND hToolBtn[16];

extern HINSTANCE hProgInstance;

extern TCHAR filename[256];
extern TCHAR filepathname[1000];
extern BOOL isAFile;
extern int fileSize;
extern int fileHPPM;
extern int fileVPPM;
extern SYSTEMTIME fileTime;

extern BOOL showGrid;
extern BOOL showMiniature;

class CMainWindow;
class CMiniatureWindow;
class CToolBox;
class CToolSettingsWindow;
class CPaletteWindow;
class CScrollboxWindow;
class CSelectionWindow;
class CImgAreaWindow;
class CSizeboxWindow;
class CTextEditWindow;

extern CMainWindow mainWindow;
extern CMiniatureWindow miniature;
extern CToolBox toolBoxContainer;
extern CToolSettingsWindow toolSettingsWindow;
extern CPaletteWindow paletteWindow;
extern CScrollboxWindow scrollboxWindow;
extern CScrollboxWindow scrlClientWindow;
extern CSelectionWindow selectionWindow;
extern CImgAreaWindow imageArea;
extern CSizeboxWindow sizeboxLeftTop;
extern CSizeboxWindow sizeboxCenterTop;
extern CSizeboxWindow sizeboxRightTop;
extern CSizeboxWindow sizeboxLeftCenter;
extern CSizeboxWindow sizeboxRightCenter;
extern CSizeboxWindow sizeboxLeftBottom;
extern CSizeboxWindow sizeboxCenterBottom;
extern CSizeboxWindow sizeboxRightBottom;
extern CTextEditWindow textEditWindow;

/* VARIABLES declared in mouse.c ************************************/

extern POINT pointStack[256];
extern short pointSP;
extern POINT *ptStack;
extern int ptSP;
