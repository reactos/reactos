#include "cmdlg.h"

/* Type definitions. */
#ifdef UNICODE
typedef unsigned short UTCHAR;
#else
typedef unsigned char UTCHAR;
#endif

#define BTOC(bytes)  ((bytes) / sizeof(TCHAR))
#define CTOB(cch)    ((cch) * sizeof(TCHAR))
#define UCHAR unsigned char

typedef struct tagSYCM
  {
    INT dxpBox;
    INT dypBox;
    INT dxpCM;
    INT dypCM;
    INT xpCh;
    INT ypCh;
    INT dxpMag;
    INT dypMag;
    INT xpMagCurr;
    INT ypMagCurr;
    INT ypDest;
    INT xpCM;
    INT ypCM;

    BOOL fHasFocus;
    BOOL fFocusState;
    BOOL fMouseDn;
    BOOL fCursorOff;
    BOOL fAnsiFont;
    UTCHAR chCurr;
    HFONT hFontMag;
    HFONT hFont;
    HDC hdcMag;
    HBITMAP hbmMag;
    INT rgdxp[255];
  } SYCM;
typedef SYCM *PSYCM;

typedef struct tagITEMDATA
  {
    SHORT FontType;
    BYTE CharSet;
    BYTE PitchAndFamily;
  } ITEMDATA;


#define LF_SUBSETSIZE 40

typedef struct tagUSUBSET
  {
    INT BeginRange;
    INT EndRange;
    INT StringResId;
    TCHAR Name[LF_SUBSETSIZE];
  } USUBSET;

/* Function declarations. */

BOOL InitApplication(HANDLE);
BOOL InitInstance(HANDLE, INT);
LONG  APIENTRY CharMapWndProc(HWND, UINT, WPARAM, LPARAM);
LONG  APIENTRY CharGridWndProc(HWND, UINT, WPARAM, LPARAM);
INT ChFromSymLParam(PSYCM, LONG);
VOID DrawSymChOutlineHwnd(PSYCM, HWND, UTCHAR, BOOL, BOOL);
VOID RecalcCharMap(HWND, PSYCM, INT, BOOL);
VOID DrawSymbolMap(PSYCM, HDC);
VOID DrawSymbolGrid(PSYCM, HDC);
VOID DrawSymbolChars(PSYCM, HDC);
VOID DrawSymChOutline(PSYCM, HDC, UTCHAR, BOOL, BOOL);
VOID MoveSymbolSel(PSYCM, UTCHAR);
VOID RestoreSymMag(PSYCM);
INT  APIENTRY FontLoadProc(LPLOGFONT, NEWTEXTMETRICEX*, DWORD, LPARAM);
HANDLE GetEditText(HWND);
VOID CopyString(HWND);
VOID SendRTFToClip(HWND, LPTSTR);
INT PointsToHeight(INT);
VOID UpdateKeystrokeText(HDC, UTCHAR, BOOL);
VOID PaintStatusLine(HDC, BOOL, BOOL);
BOOL UpdateHelpText(LPMSG, HWND);
INT KeyboardVKeyFromChar(UTCHAR);
BOOL DrawFamilyComboItem(LPDRAWITEMSTRUCT);
HBITMAP LoadBitmaps(INT);
VOID DoHelp(HWND, BOOL);
VOID SaveCurrentFont(HWND);
INT SelectInitialFont(HWND);
VOID ExitMagnify(HWND, PSYCM);
INT SelectInitialSubset(HWND);
VOID SaveCurrentSubset(HWND);
BOOL CALLBACK SubSetDlgProc(HWND, UINT, WPARAM, LPARAM);
VOID UpdateSymbolSelection(HWND, INT, INT);
