#include <windows.h>
#include "clipbrd.h"


/*--------------------------------------------------------------------------*/
/*                                                                            */
/*  SetCharDimensions() -                                                    */
/*                                                                            */
/*--------------------------------------------------------------------------*/
void SetCharDimensions(
HWND hWnd,
HFONT hFont)
{
register HDC  hdc;
TEXTMETRIC    tm;

hdc = GetDC(hWnd);
SelectObject(hdc, hFont);
GetTextMetrics(hdc, (LPTEXTMETRIC)&tm);
ReleaseDC(hWnd, hdc);

cxChar = tm.tmAveCharWidth;
cxMaxCharWidth = tm.tmMaxCharWidth;
cyLine = tm.tmHeight + tm.tmExternalLeading;
cxMargin = cxChar / 2;
cyMargin = cyLine / 4;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ClipbrdInit() -                                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/

BOOL NEAR PASCAL ClipbrdInit()

{
  WNDCLASS  class;

  if (!(hAccel = LoadAccelerators(hInst, (LPTSTR)MAKEINTRESOURCE(CBACCEL))))
      return(FALSE);

  hbrBackground = CreateSolidBrush(GetSysColor(COLOR_WINDOW));

  class.hCursor       = LoadCursor(NULL, IDC_ARROW);
  class.hIcon              = LoadIcon(hInst, MAKEINTRESOURCE(CBICON));
  class.lpszClassName = szAppName;
  class.hbrBackground = (HBRUSH)NULL;
  class.style              = CS_HREDRAW | CS_VREDRAW | CS_BYTEALIGNCLIENT;
  class.lpszMenuName  = (LPTSTR)MAKEINTRESOURCE(CBMENU);
  class.hInstance     = hInst;
  class.lpfnWndProc   = ClipbrdWndProc;
  class.cbClsExtra    = 0;
  class.cbWndExtra    = 0;

  return(RegisterClass((LPWNDCLASS)&class));
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  WinMain() -                                                             */
/*                                                                          */
/*--------------------------------------------------------------------------*/

int WINAPI WinMain(
HINSTANCE hInstance,
HINSTANCE hPrevInstance,
LPSTR lpszCmdLine,
int cmdShow)
{
MSG         msg;
HWND   hwndPrev;
typedef VOID (FAR PASCAL *LPVFNWB)(WORD, BOOL);
LPVFNWB lpfnRegisterPenApp = NULL;
LOGFONT UniFont;

hInst = hInstance;

// If there's a previous instance, activate its window and blow.
if (hwndPrev = FindWindow(szAppName, NULL))
   {
   ShowWindow(hwndPrev, SW_RESTORE);
   SetForegroundWindow(hwndPrev);
   return(0);
   }
else
   {
   LoadString(hInst, IDS_NAME, szCaptionName, sizeof(szCaptionName));
   LoadString(hInst, IDS_HELPFILE, (LPTSTR)szHelpFileName, sizeof(szHelpFileName));

   /* load caption strings for new File/Open and File/saveAs dialogs */
   LoadString(hInst, IDS_OPENCAPTION, (LPTSTR)szOpenCaption, CAPTIONMAX);
   LoadString(hInst, IDS_SAVECAPTION, (LPTSTR)szSaveCaption, CAPTIONMAX);

   /* Load default extension */
   LoadString(hInst, IDS_DEFEXTENSION, (LPTSTR)szDefExt, CCH_szDefExt);

   /* Load custom resources for CommDlg filters                           */
   /* a-mgates 9/24/92                                                    */
   LoadString(hInst, IDS_FILTERTEXT, szFilterSpec, FILTERMAX);

   /*  To prevent LoadString failure on low memory situations. */
   LoadString(hInst, IDS_MEMERROR,(LPTSTR)szMemErr,MSGMAX);

   if (!ClipbrdInit())
      {
      return FALSE;
      }

   hwndMain = CreateWindow(szAppName,
                           szCaptionName,
                           WS_TILEDWINDOW | WS_VSCROLL | WS_HSCROLL,
                           CW_USEDEFAULT, 0,
                           GetSystemMetrics(SM_CXSCREEN) / 2, GetSystemMetrics(SM_CYSCREEN) / 2,
                           (HWND)NULL,
                           (HMENU)NULL,
                           hInstance, (LPTSTR)NULL);

   /* Obtain size of standard chars; compute white border size from this */
   hfontSys = GetStockObject(SYSTEM_FONT);
   hfontOem = GetStockObject(OEM_FIXED_FONT);
   GetObject(hfontSys, sizeof(LOGFONT), &UniFont);
   UniFont.lfCharSet = ANSI_CHARSET;
   lstrcpy(UniFont.lfFaceName, TEXT("Lucida Sans Unicode"));
   hfontUni = CreateFontIndirect(&UniFont);
   if (hfontUni == NULL)
       hfontUni = (HFONT)hfontSys;

   /* Get the character dimensions for the default font */
   SetCharDimensions(hwndMain, hfontSys);

   /* Attach us to the clipboard viewer chain */
   hwndNextViewer = SetClipboardViewer(hwndMain);

   /* init. some fields of the OPENFILENAME struct used by fileopen and
    * filesaveas
    */
   OFN.lStructSize       = sizeof(OPENFILENAME);
   OFN.hwndOwner         = hwndMain;
   OFN.lpstrFileTitle    = 0;
   OFN.nMaxCustFilter    = FILTERMAX;
   OFN.nFilterIndex      = 1;
   OFN.nMaxFile          = PATHMAX;
   OFN.lpfnHook          = NULL;
   OFN.Flags             = 0L;/* for now, since there's no readonly support */

   /* determine the message number to be used for communication with
    * help application
    */
   if (!(wHlpMsg = RegisterWindowMessage ((LPTSTR)HELPMSGSTRING)))
        return FALSE;

   ShowWindow(hwndMain, cmdShow);

   if (lpfnRegisterPenApp = (LPVFNWB)
       GetProcAddress((HANDLE)GetSystemMetrics(SM_PENWINDOWS),
       "RegisterPenApp"))                /*   Anas May 92 should I?   */
      {
      (*lpfnRegisterPenApp)(1, TRUE);
      }

   while (GetMessage(&msg, NULL, 0, 0))
     {
       if (TranslateAccelerator(hwndMain, hAccel, (LPMSG)&msg) == 0)
         {
           TranslateMessage(&msg);
           DispatchMessage(&msg);
         }
     }

   if (lpfnRegisterPenApp)
       (*lpfnRegisterPenApp)(1, FALSE);

   return(msg.wParam);
   }

UNREFERENCED_PARAMETER(hPrevInstance);
UNREFERENCED_PARAMETER(lpszCmdLine);
}
