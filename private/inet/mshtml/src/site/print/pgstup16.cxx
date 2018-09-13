/* dlg_page.c -- Deal with PAGE Margins dialog. */

#include "headers.hxx"

#ifndef X_STDIO_H_
#define X_STDIO_H_
#include <stdio.h>
#endif

#ifndef X_PAGE_H_
#define X_PAGE_H_
#include <page.h>
#endif

#ifndef X_PRINT_H_
#define X_PRINT_H_
#include <print.h>
#endif

#ifndef X_PGSTUP16_HXX_
#define X_PGSTUP16_HXX_
#include <pgstup16.hxx>
#endif

#ifndef X_PUTIL_HXX_
#define X_PUTIL_HXX_
#include "putil.hxx"
#endif

#ifndef X_FPRINT_HXX_
#define X_FPRINT_HXX_
#include "fprint.hxx"
#endif

typedef struct _dev_info
{
    struct page_setup info;
    LPDEVMODE lpdm;
    GDIPOINT ptPaperSize;
    GDIRECT RtSampleXYWH;
    BOOL bEnvelope;
    BOOL bInitSample;
    int iConvertBy;
    BOOL bConvertedToMetric;
    char szMeasure[16];
}DEV_INFO, FAR * LPDEV_INFO;

static DEV_INFO di;

typedef struct tagBININFO
  {
    short  BinNumber;
    short  NbrofBins;
    short  Reserved1;
    short  Reserved2;
    short  Reserved3;
    short  Reserved4;
  } BININFO;

typedef char STR24[24];
typedef STR24 FAR * LPSTR24;
#define HPDRVNUM      0x3850

static HWND hwndRunning = NULL;
static BOOL bResult;
static char szDecimal[32];

const char szIntl[] = "intl";


//bugwin16: Fix this.
char szDeviceName[256];
char szPort[CCHDEVICENAME];
char szDrvName[CCHDEVICENAME];
HGLOBAL hDevMode = NULL;

#define MulDiv MulDivQuick
#define CONVERTTOMM(inches) ((inches) * 2540L * di.iConvertBy)
#define CONVERTTOINCHES(mm) (MulDiv((mm), di.iConvertBy, 2540L))

#define CONVERTTOMMFORSAMPLE(inches) ((inches) * 2540L)

#define D_MARGIN 10

void ReadHeaderOrFooter(HKEY hfKey,const TCHAR* pValueName,TCHAR* pHeaderFooter);
void WriteHeaderFooter(HKEY keyPS, TCHAR * pszHeader, TCHAR * pszFooter);

BOOL PASCAL Header_OnInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    TCHAR   szHeader[1024];
    TCHAR   szFooter[1024];
    HKEY    keyPageSetup = NULL;

    _tcscpy(szHeader,_T(""));
    _tcscpy(szFooter,_T(""));
    if (GetRegPrintOptionsKey(PRINTOPTSUBKEY_PAGESETUP,&keyPageSetup) == ERROR_SUCCESS)
    {
        // if successfull, it always can read the values
        ReadHeaderOrFooter(keyPageSetup, _T("header"), szHeader);
        ReadHeaderOrFooter(keyPageSetup, _T("footer"), szFooter);
        RegCloseKey(keyPageSetup);
    }

    SetDlgItemText(hDlg, RES_DLG_PAGE_LH, szHeader);
    SetDlgItemText(hDlg, RES_DLG_PAGE_LF, szFooter);
    return TRUE;
}


VOID PASCAL Header_OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    register WORD wID = LOWORD(wParam);
    register WORD wNotificationCode = HIWORD(wParam);
    register HWND hWndCtrl = (HWND) lParam;

    switch (wID)
    {
        case IDOK:
            {
                TCHAR   szHeader[1024];
                TCHAR   szFooter[1024];
                HKEY    keyPageSetup = NULL;

                GetDlgItemText(hDlg, RES_DLG_PAGE_LH, szHeader, 1024);
                GetDlgItemText(hDlg, RES_DLG_PAGE_LF, szFooter, 1024);
                if (GetRegPrintOptionsKey(PRINTOPTSUBKEY_PAGESETUP,&keyPageSetup) == ERROR_SUCCESS)
                {
                    WriteHeaderFooter(keyPageSetup,szHeader,szFooter);
                    RegCloseKey(keyPageSetup);
                }
                EndDialog(hDlg, TRUE);
            }
            return;

        case IDCANCEL:
            EndDialog(hDlg, FALSE);
            return;

        default:
            return;
    }
    /* NOT REACHED */
}


UINT CALLBACK Header_DialogProc(HWND hDlg, UINT uMsg,
            WPARAM wParam, LPARAM lParam)
{
    /* WARNING: the cracker/handlers don't appear to have been written
       with dialog boxes in mind, so we spell it out ourselves. */

    switch (uMsg)
    {
        case WM_INITDIALOG:
            return (Header_OnInitDialog(hDlg, wParam, lParam));
        case WM_COMMAND:
            Header_OnCommand(hDlg, wParam, lParam);
            return (TRUE);
        default:
            return (FALSE);
    }
    /* NOT REACHED */
}

LONG PASCAL PrintPageSetupPaintProc( HWND hWnd)
{
    LONG lResult;
    // PPRINTINFO pPI;
    HDC hDC;
    GDIRECT aRt, aRtPage, aRtUser;
    PAINTSTRUCT aPs;
    HGDIOBJ hPen, hBr, hFont, hFontGreek;
    HRGN hRgn;
    char szGreekText[] = "Dheevaeilnorpoefdi lfaocr, \nMoiccsriocsnoafrtf \tbnya\nSFlr acnn IF iynnnaepgmaonc\n F&i nyneelglaanm 'Ox' Mnaalgleenyn i&f QCnoamgpeannnyi FI nxca.r\nFSoaynb  Ftrfaonscoirscciom,  \rCoafl idfeopronlieav\ne\n";
    LPTSTR psGreekText;
    int i;

    hDC = BeginPaint(hWnd, &aPs);
    GetClientRect(hWnd, &aRt);
    FillRect(hDC, &aRt, (HBRUSH)GetStockObject(WHITE_BRUSH));
    EndPaint(hWnd, &aPs);
    lResult = 0;

    if (!(hDC = GetDC(hWnd)))
    {
        return (0);
    }

    // TransferPD2PSD(pPI);
    aRtPage = aRt;
    hPen = (HGDIOBJ)CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
    hPen = SelectObject(hDC, hPen);

    // Rectangle() does not work here
    MoveToEx( hDC, 0            , 0             , (GDIPOINT *)NULL );
    LineTo(   hDC, aRt.right - 1, 0              );
    MoveToEx( hDC, 0            , 1             , (GDIPOINT *)NULL );
    LineTo(   hDC, 0            , aRt.bottom - 1 );
    DeleteObject(SelectObject(hDC, hPen));

    // Rectangle() does not work here
    MoveToEx( hDC, aRt.right - 1, 0             , (GDIPOINT *)NULL );
    LineTo(   hDC, aRt.right - 1, aRt.bottom - 1 );
    MoveToEx( hDC, 0            , aRt.bottom - 1, (GDIPOINT *)NULL );
    LineTo(   hDC, aRt.right    , aRt.bottom - 1 );

    SetBkMode(hDC, TRANSPARENT);
    hPen = (HGDIOBJ)CreatePen(PS_DOT, 1, RGB(128, 128, 128));
    hPen = SelectObject(hDC, hPen);
    hBr  = (HGDIOBJ)GetStockObject(NULL_BRUSH);
    hBr  = SelectObject(hDC, hBr);

    hFont = hFontGreek = CreateFont( 5, 2,
                                     0,
                                     0,
                                     FW_DONTCARE,
                                     0,
                                     0,
                                     0,
                                     ANSI_CHARSET,
                                     OUT_DEFAULT_PRECIS,
                                     CLIP_DEFAULT_PRECIS,
                                     DEFAULT_QUALITY,
                                     VARIABLE_PITCH | FF_SWISS,
                                     NULL );
    hFont = SelectObject(hDC, hFont);

    InflateRect(&aRt, -1, -1);
    aRtUser = aRt;
    hRgn = CreateRectRgnIndirect(&aRtUser);
    SelectClipRgn(hDC, hRgn);
    DeleteObject(hRgn);

    // Avoid divide by zero
    if ((0 != di.ptPaperSize.x) && (0 != di.ptPaperSize.x))
    {
        if (di.bConvertedToMetric)
        {
            aRt.left   += (MulDiv(MulDiv(aRt.right, di.info.marginleft, di.iConvertBy), D_MARGIN, di.ptPaperSize.x));
            aRt.top    += (MulDiv(MulDiv(aRt.bottom, di.info.margintop, di.iConvertBy), D_MARGIN, di.ptPaperSize.y));
            aRt.right  -= (MulDiv(MulDiv(aRt.right, di.info.marginright, di.iConvertBy), D_MARGIN, di.ptPaperSize.x));
            aRt.bottom -= (MulDiv(MulDiv(aRt.bottom, di.info.marginbottom, di.iConvertBy), D_MARGIN, di.ptPaperSize.y));
        }
        else
        {
            aRt.left   += (MulDiv(aRt.right,  MulDiv(di.info.marginleft,   254, di.iConvertBy * 10) * D_MARGIN, di.ptPaperSize.x));
            aRt.top    += (MulDiv(aRt.bottom, MulDiv(di.info.margintop,    254, di.iConvertBy * 10) * D_MARGIN, di.ptPaperSize.y));
            aRt.right  -= (MulDiv(aRt.right,  MulDiv(di.info.marginright,  254, di.iConvertBy * 10) * D_MARGIN, di.ptPaperSize.x));
            aRt.bottom -= (MulDiv(aRt.bottom, MulDiv(di.info.marginbottom, 254, di.iConvertBy * 10) * D_MARGIN, di.ptPaperSize.y));
        }
    }

    if ( (aRt.left > aRtPage.left) && (aRt.left < aRtPage.right) &&
         (aRt.right < aRtPage.right) && (aRt.right > aRtPage.left) &&
         (aRt.top > aRtPage.top) && (aRt.top < aRtPage.bottom) &&
         (aRt.bottom < aRtPage.bottom) && (aRt.bottom > aRtPage.top) &&
         (aRt.left < aRt.right) &&
         (aRt.top < aRt.bottom) )
    {
        Rectangle(hDC, aRt.left, aRt.top, aRt.right, aRt.bottom);

//SkipMarginRectangle:

        InflateRect(&aRt, -1, -1);
        if (1)
        {
            psGreekText = (LPTSTR)MAKELP(GlobalAlloc(GPTR,10 * sizeof(szGreekText)),0);
            if (psGreekText != NULL)
            {
                for (i = 0; i < 10; i++)
                {
                    memcpy( &psGreekText[i * (sizeof(szGreekText) / sizeof(szGreekText[0]))],
                                szGreekText,
                                sizeof(szGreekText) );
    #ifdef DBCS
                    // On FE Win31, if the string has '\0'
                    // then the DrawText() was failed.
                    // It should be removed.
                    psGreekText[(i + 1) * (sizeof(szGreekText) / sizeof(szGreekText[0])) - 1] = ' ';  // change '\0' to ' '
    #endif //DBCS
                }
                aRt.left++;
                aRt.right--;
                // aRt.bottom -= (aRt.bottom - aRt.top) % pPI->PtMargins.y;
                aRt.bottom -= (aRt.bottom - aRt.top) % 10;
                hFontGreek = SelectObject(hDC, hFontGreek);
                DrawText( NULL,    // drawText is #defined to take 6 params, 1st is ignored.
                          hDC,
                          psGreekText,
                          10 * (sizeof(szGreekText) / sizeof(szGreekText[0])),
                          &aRt,
                          DT_NOPREFIX | DT_WORDBREAK );
                SelectObject(hDC, hFontGreek);
                GlobalFree((HGLOBAL)SELECTOROF(psGreekText));
            }
        }
    }

//SkipGreekText:

    InflateRect(&aRtPage, -1, -1);
    if (di.bEnvelope)
    {
        int iOrientation;

        aRt = aRtPage;
        if (aRt.right < aRt.bottom)     // portrait
        //  switch (pPI->dwRotation)
            {
        //      default :               // no landscape
        //      case ( ROTATE_LEFT ) :  // dot-matrix
        //      {
        //          aRt.left = aRt.right  - 16;
        //          aRt.top  = aRt.bottom - 32;
        //          iOrientation = 2;
        //          break;
        //      }
        //      case ( ROTATE_RIGHT ) : // HP PCL
        //      {
                    aRt.right  = aRt.left + 16;
                    aRt.bottom = aRt.top  + 32;
                    iOrientation = 1;
        //          break;
        //      }
            }
        else                            // landscape
        {
            aRt.left   = aRt.right - 32;
            aRt.bottom = aRt.top   + 16;
            iOrientation = 3;
        }
        hRgn = CreateRectRgnIndirect(&aRt);
        SelectClipRgn(hDC, hRgn);
        DeleteObject(hRgn);
#if 0
        if (1)
        {
            switch (iOrientation)
            {
                default :          // HP PCL
            //  case ( 1 ) :
                {
                    DrawIcon(hDC, aRt.left, aRt.top, hIconPSStampP);
                    break;
                }

            //  case ( 2 ) :       // dot-matrix
            //  {
            //      DrawIcon(hDC, aRt.left - 16, aRt.top, hIconPSStampP);
            //      break;
            //  }

                case ( 3 ) :       // landscape
                {
                    DrawIcon(hDC, aRt.left, aRt.top, hIconPSStampL);
                    break;
                }
            }
        }
#endif
    }

//SkipEnvelopeStamp:;

    aRtUser = aRtPage;
    hRgn = CreateRectRgnIndirect(&aRtUser);
    SelectClipRgn(hDC, hRgn);
    DeleteObject(hRgn);

    //
    //  Draw the envelope lines.
    //
    if (di.bEnvelope)
    {
        int iRotation;
        HGDIOBJ hPenBlack;

        aRt = aRtPage;
        if (aRt.right < aRt.bottom)                     // portrait
        {
        //  if (pPI->dwRotation == ROTATE_LEFT )        // dot-matrix
        //      iRotation = 3;
        //  else            // ROTATE_RIGHT             // HP PCL
                iRotation = 2;
        }
        else                                            // landscape
        {
            iRotation = 1;                              // normal
        }

        switch (iRotation)
        {
            default :
        //  case ( 1 ) :      // normal
            {
                aRt.right  = aRt.left + 32;
                aRt.bottom = aRt.top  + 13;
                break;
            }
            case ( 2 ) :      // left
            {
                aRt.right = aRt.left   + 13;
                aRt.top   = aRt.bottom - 32;
                break;
            }
        //  case ( 3 ) :      // right
        //  {
        //      aRt.left   = aRt.right - 13;
        //      aRt.bottom = aRt.top   + 32;
        //      break;
        //  }
        }

        InflateRect(&aRt, -3, -3);
        hPenBlack = SelectObject(hDC, GetStockObject(BLACK_PEN));
        switch (iRotation)
        {
            case ( 1 ) :       // normal
            {
                MoveToEx(hDC, aRt.left , aRt.top    , (GDIPOINT *)NULL);
                LineTo(  hDC, aRt.right, aRt.top);
                MoveToEx(hDC, aRt.left , aRt.top + 3, (GDIPOINT *)NULL);
                LineTo(  hDC, aRt.right, aRt.top + 3);
                MoveToEx(hDC, aRt.left , aRt.top + 6, (GDIPOINT *)NULL);
                LineTo(  hDC, aRt.right, aRt.top + 6);

                break;
            }

        //  case ( 2 ) :       // left
        //  case ( 3 ) :       // right
            default :
            {
                MoveToEx( hDC, aRt.left      , aRt.top       , (GDIPOINT *)NULL );
                LineTo(   hDC, aRt.left      , aRt.bottom     );
                MoveToEx( hDC, aRt.left   + 3, aRt.top       , (GDIPOINT *)NULL );
                LineTo(   hDC, aRt.left   + 3, aRt.bottom     );
                MoveToEx( hDC, aRt.left   + 6, aRt.top       , (GDIPOINT *)NULL );
                LineTo(   hDC, aRt.left   + 6, aRt.bottom     );

                break;
            }
        }
        SelectObject(hDC, hPenBlack);
    }

//NoMorePainting:

    DeleteObject(SelectObject(hDC, hPen));
    SelectObject(hDC, hBr);
    DeleteObject(SelectObject(hDC, hFont));
    // TransferPSD2PD(pPI);
    ReleaseDC(hWnd, hDC);

    return (lResult);
}

LRESULT FAR PASCAL PageSampleWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
            break;

        case WM_PAINT:
            return PrintPageSetupPaintProc(hWnd);
            break;

        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
            break;
    }

    return 1;
}

LRESULT FAR PASCAL PageSampleGrayWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
            break;

        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
            break;
    }

    return 1;
}


BOOL PASCAL InitPageSampleWnd()
{
    static BOOL bRegistered = FALSE;
    WNDCLASS wc;

    if (bRegistered)
        return TRUE;

    wc.style = 0;
    wc.lpfnWndProc = PageSampleWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = g_hInstResource;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW + 1;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "PageSampleWnd";

    if (!RegisterClass(&wc))
        return FALSE;

    wc.style = 0;
    wc.lpfnWndProc = PageSampleGrayWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = g_hInstResource;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(DKGRAY_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "PageSampleGrayWnd";

    if (!RegisterClass(&wc))
        return FALSE;

    bRegistered = TRUE;
    return TRUE;
}


/*
 * EvilHPDrivers -- Is this an HP driver
 *
 * The problem is that HPPCL and HPPCL5A for version 3.0 did not appropriately
 * incorporate the DC_BINNAMES option of DeviceCapabilities.  The HPPCL5A
 * driver also messed up with the DC_PAPERS option of DeviceCapabilities.
 * Worse, they give back inappropriate indexes, which causes trouble.  It
 * was decided not to offer the options, rather than forcing apps to special
 * case the two drivers.  Note that this routine should only be called when
 * it is known that the driver is pre-3.1.
 * Return:  2 is HPPCL5A
 *          1 is HPPCL
 *          0 is no match
 */
#define EVIL_HPPCL   1
#define EVIL_HPPCL5A 2
#define EVIL_SD_LJET 3

short PASCAL EvilHPDrivers(LPSTR lpDriverName)
{
    short i;
    static char *pszHPDrivers[] = {"HPPCL", "HPPCL5A", "SD_LJET", ""};

    for (i = 0; *pszHPDrivers[i]; i++)
    {
      if (!lstrcmpi(pszHPDrivers[i], lpDriverName))
        {
          return(i + 1);
        }
    }
    return(0);
}


BOOL PASCAL PaperBins(HWND hDlg, HANDLE hDrv, LPDEVMODE lpDevMode)
{
  BOOL bEnable = FALSE;
  HANDLE hMem=NULL;
  LPINT lpMem;
  STR24 FAR *lpBins;
  WORD  i, nBins;
  LPFNDEVCAPS lpfnDevCap;
  DWORD dwDMSize;
  LPSTR lpszDevice = szDeviceName;
  LPSTR lpszPort = szPort;
  LPSTR lpDrvName = szDrvName;

  SendDlgItemMessage(hDlg, RES_DLG_PAGE_COMBO2, CB_RESETCONTENT, 0, 0L);

/* Both the HPPCL5A and HPPCL drivers screw up on Paper Bins */
/* WinWord has shipped a special version of HPPCL5A which doesn't screw
 * up.  Versions HPDRVNUM and beyond should work correctly.
 *                                                7 Oct 1991  Clark Cyr
 */
  if ((lpDevMode->dmSpecVersion < 0x030A) &&
        (i = EvilHPDrivers(lpDrvName)) &&
        ((i != EVIL_HPPCL5A) || (lpDevMode->dmDriverVersion < HPDRVNUM)))
      goto DontAsk;

/* NotBadDriver only needs to be called on 3.00 drivers */
  if ((lpDevMode->dmSpecVersion >= 0x030A) && /*NotBadDriver(lpDN) &&*/
      (lpfnDevCap = (LPFNDEVCAPS)GetProcAddress((HINSTANCE)hDrv, "DeviceCapabilities")))
    {
      if ((dwDMSize = (*lpfnDevCap)(lpszDevice, lpszPort, DC_BINNAMES, 0L,
                                      lpDevMode)) && (dwDMSize != (DWORD)(-1)))
        {
          hMem = GlobalAlloc(GMEM_MOVEABLE,(WORD)dwDMSize * (sizeof(short)+24));
          if (hMem == NULL)
              goto DontAsk;

          lpMem = (LPINT) GlobalLock(hMem);
          (*lpfnDevCap)(lpszDevice, lpszPort, DC_BINS, (LPSTR)lpMem, lpDevMode);
          nBins = (WORD)(*lpfnDevCap)(lpszDevice, lpszPort, DC_BINNAMES,
                             ((LPSTR)lpMem) + ((WORD)dwDMSize * sizeof(short)),
                             lpDevMode);
          if (nBins)
            {
              bEnable = TRUE;
            }
          else
            {
              GlobalUnlock(hMem);
              GlobalFree(hMem);
            }
        }
    }

  if (!bEnable)
    {
      HDC hIC;

/* Must use ENUMPAPERBINS because DeviceCapabilities() either was not found
   or has refused to support the DC_BINNAMES index though it still has bins */
      hIC = CreateIC((LPSTR)szDrvName,
                                      lpszDevice, lpszPort, (LPSTR) lpDevMode);
      nBins = ENUMPAPERBINS;
      if (hIC)
        {
          if (Escape(hIC, QUERYESCSUPPORT, sizeof(int), (LPSTR)&nBins, NULL))
            {
              nBins = GETSETPAPERBINS;
              if (Escape(hIC, QUERYESCSUPPORT,sizeof(int),(LPSTR)&nBins,NULL))
                {
                  BININFO BinInfo;

                  Escape(hIC, GETSETPAPERBINS, 0, NULL,(LPSTR)&BinInfo);
                  if (nBins = BinInfo.NbrofBins)
                    {
                      hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                                 nBins * (sizeof(short) + 24));
                      if (hMem == NULL)
                          goto DontAsk;
                      lpMem = (LPINT) GlobalLock(hMem);
                      Escape(hIC, ENUMPAPERBINS, sizeof(int), (LPSTR)&nBins,
                                                                (LPSTR)lpMem);
                      bEnable = TRUE;
                    }
                }
            }
          DeleteDC(hIC);
        }
    }

DontAsk:
  EnableWindow(GetDlgItem(hDlg, RES_DLG_PAGE_COMBO2), bEnable);

  if (bEnable)
    {
      lpBins = (LPSTR24)(lpMem + nBins);
      for (i = 0; (WORD) i < nBins; i++)
        {
          if (*((LPSTR)lpBins))
            {
              SendDlgItemMessage(hDlg, RES_DLG_PAGE_COMBO2, CB_INSERTSTRING, (WPARAM) -1,
                                                              (LPARAM)lpBins++);
              SendDlgItemMessage(hDlg, RES_DLG_PAGE_COMBO2, CB_SETITEMDATA, i, (LONG)(*lpMem));
              if (lpDevMode->dmDefaultSource == *lpMem++)
                  SendDlgItemMessage(hDlg, RES_DLG_PAGE_COMBO2, CB_SETCURSEL, i, 0L);
            }
        }
      if (SendDlgItemMessage(hDlg, RES_DLG_PAGE_COMBO2, CB_GETCURSEL, 0, 0L) == CB_ERR)
          SendDlgItemMessage(hDlg, RES_DLG_PAGE_COMBO2, CB_SETCURSEL, 0, 0L);
      if (hMem)
      {
          GlobalUnlock(hMem);
          GlobalFree(hMem);
      }
    }
  return(bEnable);
}

FARPROC PASCAL GetExtDevModeAddr(HANDLE hDriver)
{
    FARPROC lpfnDevMode;

    /* First see if ExtDeviceMode is supported (Win 3.0 drivers) */
    if (lpfnDevMode = GetProcAddress((HINSTANCE)hDriver, (LPSTR)"ExtDeviceMode"))
        return(lpfnDevMode);
    else
        return (NULL);

}

HGLOBAL Init_DevMode()
{
    LPSTR lp1, lp2;
    HINSTANCE hModule;
    LPFNDEVMODE lpfnDevMode;
    char szLib[13];
    char szBuf[80];
    HGLOBAL hDevModeRet = NULL;

    GetProfileString("windows", "device", "", szBuf, sizeof(szBuf));

    if (!szBuf[0])
        return NULL;

    lp1 = szBuf;
    lp2 = szDeviceName;

    // Get Device Name
    while(*lp1 && (*lp1 != ','))
        *lp2++ = *lp1++;
    if (!*lp1)
        return NULL;
    *lp2 = '\0';

    // Get Driver name
    lp1++;
    lp2 = szDrvName;
    while(*lp1 && (*lp1 != ','))
        *lp2++ = *lp1++;
    if (!*lp1)
        return NULL;
    *lp2 = '\0';


    // Get Port Name
    lp1++;
    lp2 = szPort;
    while(*lp1 && (*lp1 != ','))
        *lp2++ = *lp1++;
    *lp2 = '\0';


    lstrcpy(szLib, szDrvName);
    lstrcat(szLib, ".drv");
    hModule = LoadLibrary(szLib);
    if (HINSTANCE_ERROR < hModule)
    {
        if (lpfnDevMode = (LPFNDEVMODE)GetExtDevModeAddr((HANDLE)hModule))
        {
            LPDEVMODE  lpDM;
            DWORD dwDMSize = (*lpfnDevMode)(NULL, hModule, NULL,
                                            szDeviceName, szPort,
                                            NULL, NULL, NULL);

            hDevModeRet = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, dwDMSize);

            if (lpDM = (LPDEVMODE)GlobalLock(hDevModeRet))
            {
                (*lpfnDevMode)(NULL, hModule, lpDM, szDeviceName,
                                            szPort, NULL, NULL, DM_COPY);
                GlobalUnlock(hDevModeRet);
            }
        }
        FreeLibrary(hModule);
    }

    return(hDevModeRet);
}

// Fill in a list box with Paper Name.  If the printer returns the actual names
// then use the names array, and add in the paper size number from the paper
// size array.  If the printer returns just the paper size array, then
// look up the names in a standard list.
void PASCAL Init_PageListBox(HWND hDlg)
{
    LPFNDEVCAPS lpfnDevCaps;
    HINSTANCE hDrv;
    char szLib[13];

    SendDlgItemMessage(hDlg, RES_DLG_PAGE_COMBO1, CB_RESETCONTENT, 0, 0L);

    lstrcpy(szLib, szDrvName);
    lstrcat(szLib, ".drv");
    hDrv = LoadLibrary(szLib);
    if ((hDrv = LoadLibrary(szLib)) > HINSTANCE_ERROR)
    {
        if (lpfnDevCaps = (LPFNDEVCAPS)GetProcAddress(hDrv, "DeviceCapabilities"))
        {
            DWORD dwPN = 0L;
            LPSTR lpPN = NULL;
            DWORD dwDMSize = 0;
            LPWORD lpMem = NULL;
            int i, j;
            BOOL bFound = FALSE;

            // Get the Number of paper sizes supported
            dwDMSize = (*lpfnDevCaps)(szDeviceName,szPort,
                                        DC_PAPERS, 0L, di.lpdm);

            if (dwDMSize && (dwDMSize != (DWORD)(-1)))
            {
                if (di.lpdm->dmSpecVersion >= 0x030A)
                {
                    // Get the number of Paper names
                    dwPN = (*lpfnDevCaps)(szDeviceName,szPort,
                                        DC_PAPERNAMES, 0L, di.lpdm);

                    if ((dwPN == -1) || (dwPN == 0xFFFF))
                        dwPN = 0L;
                }

                // get the actual Paper names
                if (dwPN &&
                    (lpPN = (LPSTR)MAKELP(GlobalAlloc(GPTR,
                                LOWORD(dwPN) * CCHPAPERNAME), 0)))
                {

                    (*lpfnDevCaps)(szDeviceName,szPort,
                                        DC_PAPERNAMES, lpPN, di.lpdm);
                }

                // Get the Paper Sizes array
                lpMem = (LPWORD)MAKELP(GlobalAlloc(GPTR, dwDMSize * sizeof(WORD)), 0);
                if (lpMem != NULL)
                {
                    dwDMSize = (*lpfnDevCaps)(szDeviceName,szPort,
                                            DC_PAPERS, (LPSTR)lpMem, di.lpdm);

                    // If the Paper Names are supplied then use them
                    if (dwPN)
                    {
                        for (i = j = 0; i < (short) dwDMSize; i++, lpMem++)
                        {
                            SendDlgItemMessage(hDlg, RES_DLG_PAGE_COMBO1,
                                CB_INSERTSTRING, (WPARAM)-1, (LPARAM)(lpPN + i * CCHPAPERNAME));

                            if ((WORD)di.lpdm->dmPaperSize == *lpMem)
                            {
                                SendDlgItemMessage(hDlg, RES_DLG_PAGE_COMBO1, CB_SETCURSEL, j, 0L);
                                bFound = TRUE;
                            }
                            SendDlgItemMessage(hDlg, RES_DLG_PAGE_COMBO1, CB_SETITEMDATA, j++,
                                                            (LONG)*lpMem);
                        }
                    }
                    else
                    {
                        char    szPaperName[CCHPAPERNAME];

                        for (i = j = 0; i < (short) dwDMSize; i++, lpMem++)
                        {
                            if (*lpMem <= DMPAPER_USER)
                            {
                                // Load the paper name from the standard name list.  The name
                                // list starts with letter, which is paper # 1, which is why
                                // we need the -1.
                                if (LoadString(g_hInstResource, IDS_DMPAPER_LETTER + ((WORD)*lpMem - (WORD)1),
                                    (LPSTR) szPaperName, sizeof(szPaperName)))
                                {
                                    SendDlgItemMessage(hDlg, RES_DLG_PAGE_COMBO1, CB_INSERTSTRING, (WPARAM)-1,
                                                       (LPARAM)szPaperName);

                                    if ((WORD)di.lpdm->dmPaperSize == *lpMem)
                                    {
                                        SendDlgItemMessage(hDlg, RES_DLG_PAGE_COMBO1, CB_SETCURSEL, j, 0L);
                                        bFound = TRUE;
                                    }
                                    SendDlgItemMessage(hDlg, RES_DLG_PAGE_COMBO1, CB_SETITEMDATA, j++,
                                                                    (LONG)*lpMem);
                                }
                            }
                        }
                    }

                    if (!bFound)
                        SendDlgItemMessage(hDlg, RES_DLG_PAGE_COMBO1, CB_SETCURSEL, 0, 0L);

                    GlobalFree((HGLOBAL)(SELECTOROF(lpMem)));
                }
                if (lpPN)
                    GlobalFree((HGLOBAL)(SELECTOROF(lpPN)));
            }
        }

        PaperBins(hDlg, hDrv, di.lpdm);

        FreeLibrary(hDrv);
    }
}


void PASCAL IntlHiresToA(INT_TYPE hires, LPSTR lp)
{
    char szBuf[32];
    LPSTR lpTmp;

    hires = MulDiv(hires, 1, di.iConvertBy);

    if (di.bConvertedToMetric)
        wsprintf(szBuf, "%ld", hires/100);
    else
        wsprintf(szBuf, "%ld.%02d", hires/100, hires%100);

    lpTmp = szBuf;

    while(*lpTmp && (*lpTmp != '.'))
        *lp++ = *lpTmp++;

    *lp='\0';
    if (!di.bConvertedToMetric)
    {
        lstrcpy(lp, szDecimal);
        lstrcat(lp, ++lpTmp);
    }
    lstrcat(lp, di.szMeasure);
}

INT_TYPE PASCAL IntlAToHires(LPSTR lp)
{
    char szBuf[32];
    LPSTR lpTmp;
    INT_TYPE integral = 0;

    lpTmp = szBuf;

    // Convert integral part.
    while(*lp && (*lp >='0') && (*lp <= '9'))
        *lpTmp++ = *lp++;

    *lpTmp = '\0';
    integral = atol(szBuf);
    if (integral > 999)
        return 999 * di.iConvertBy;

    INT_TYPE decimal = 0;

    // Convert decimal part.
    lpTmp = szBuf;
    if (lstrcmpi(lp, szDecimal) > 0)
    {
        // Skip to decimal point
        while(*lp && ((*lp <'0') || (*lp > '9')))
            lp++;

        // Get 2 or 3 digits after the decimal
        // depending on whether it's inches or mm
        //
        int iConvertBy = di.iConvertBy;
        while(*lp && (iConvertBy > 1) && (*lp >='0') && (*lp <= '9'))
        {
            decimal = decimal*10 + *lp - '0';
            iConvertBy /= 10;
            ++lp;
        }

        // Scale by the number of unused digits after the decimal
        if (iConvertBy >= 10)
        {
            decimal *= iConvertBy;
        }

    }
    *lpTmp = '\0';

    return (integral * (INT_TYPE)di.iConvertBy) + decimal;
}



BOOL PASCAL UpdatePageOrientation(HWND hDlg)
{
    int id;
    int i;
    WORD wPaperSize;
    DWORD dwDMSize;
    DWORD dwPSSize;
    GDIPOINT* lpPt;
    LPWORD lpMem;
    HWND hWndSample;
    HWND hWndShadowRight;
    HWND hWndShadowBottom;
    LPFNDEVCAPS lpfnDevCaps;
    HINSTANCE hDrv;
    char szLib[13];
    char buf[80];
    GDIPOINT pt = {2159,2794};
    GDIPOINT ptEnvelope = {1800,4500};

    if (!di.bInitSample)
        return FALSE;

    lstrcpy(szLib, szDrvName);
    lstrcat(szLib, ".drv");

    GetDlgItemText(hDlg, RES_DLG_PAGE_LEFT, buf, ARRAYSIZE(buf));
    di.info.marginleft = IntlAToHires(buf);

    GetDlgItemText(hDlg, RES_DLG_PAGE_TOP, buf, ARRAYSIZE(buf));
    di.info.margintop = IntlAToHires(buf);

    GetDlgItemText(hDlg, RES_DLG_PAGE_RIGHT, buf, ARRAYSIZE(buf));
    di.info.marginright = IntlAToHires(buf);

    GetDlgItemText(hDlg, RES_DLG_PAGE_BOTTOM, buf, ARRAYSIZE(buf));
    di.info.marginbottom = IntlAToHires(buf);

    if (HINSTANCE_ERROR >= (hDrv = LoadLibrary(szLib)))
        return FALSE;

    if (!(lpfnDevCaps = (LPFNDEVCAPS)GetProcAddress(hDrv, "DeviceCapabilities")))
    {
        FreeLibrary(hDrv);
        return FALSE;
    }

    id = (int) SendDlgItemMessage(hDlg, RES_DLG_PAGE_COMBO1, CB_GETCURSEL, 0, 0L);
    wPaperSize = (int) SendDlgItemMessage(hDlg, RES_DLG_PAGE_COMBO1, CB_GETITEMDATA, id, 0L);

    dwDMSize = (*lpfnDevCaps)(szDeviceName,szPort,
                                        DC_PAPERS, 0L, di.lpdm);
    dwPSSize = (*lpfnDevCaps)(szDeviceName,szPort,
                                        DC_PAPERSIZE, 0L, di.lpdm);

    // Special case weird HP5SI behavior: The HP5SI driver can return a non-zero value for
    // DC_PAPERS, and a zero value for DC_PAPERSIZE.  Since we GlobalAlloc based on these return
    // values, assume that the driver meant to return the same numerical value for DC_PAPERSIZE
    // and allocate a segment based on that.
    if (dwPSSize == 0 && dwDMSize > 0)
    {
        dwPSSize = dwDMSize;
    }

    lpMem = (LPWORD)MAKELP(GlobalAlloc(GPTR, dwDMSize * sizeof(WORD)), 0);
    lpPt = (GDIPOINT*)MAKELP(GlobalAlloc(GPTR, dwPSSize * sizeof(GDIPOINT)), 0);

    if(lpMem && lpPt)
    {
        dwDMSize = (*lpfnDevCaps)(szDeviceName,szPort,
                                  DC_PAPERS, (LPSTR)lpMem, di.lpdm);

        (*lpfnDevCaps)(szDeviceName,szPort, DC_PAPERSIZE, (LPSTR)lpPt, di.lpdm);


        di.bEnvelope = FALSE;
        switch (wPaperSize)
        {
            case ( DMPAPER_ENV_9 ) :
            case ( DMPAPER_ENV_10 ) :
            case ( DMPAPER_ENV_11 ) :
            case ( DMPAPER_ENV_12 ) :
            case ( DMPAPER_ENV_14 ) :
            case ( DMPAPER_ENV_DL ) :
            case ( DMPAPER_ENV_C5 ) :
            case ( DMPAPER_ENV_C3 ) :
            case ( DMPAPER_ENV_C4 ) :
            case ( DMPAPER_ENV_C6 ) :
            case ( DMPAPER_ENV_C65 ) :
            case ( DMPAPER_ENV_B4 ) :
            case ( DMPAPER_ENV_B5 ) :
            case ( DMPAPER_ENV_B6 ) :
            case ( DMPAPER_ENV_ITALY ) :
            case ( DMPAPER_ENV_MONARCH ) :
            case ( DMPAPER_ENV_PERSONAL ) :
            {
                di.bEnvelope = TRUE;
                break;
            }
        }

        for (i = 0; i < (int) dwDMSize; i ++)
        {
            if (lpMem[i] == wPaperSize)
                di.ptPaperSize = lpPt[i];
        }

        if (IsDlgButtonChecked(hDlg, RES_DLG_PAGE_PORTRAIT) != 1)
        {
            int tmp;
            tmp = di.ptPaperSize.x;
            di.ptPaperSize.x = di.ptPaperSize.y;
            di.ptPaperSize.y = tmp;
        }

    }

    if (lpPt)
        GlobalFree((HGLOBAL)(SELECTOROF(lpPt)));
    if (lpMem)
        GlobalFree((HGLOBAL)(SELECTOROF(lpMem)));


    //
    //  Update the sample window size & shadow.
    //
    if ( (hWndSample = GetDlgItem(hDlg, RES_DLG_RCT1)) &&
         (hWndShadowRight = GetDlgItem(hDlg, RES_DLG_RCT2)) &&
         (hWndShadowBottom = GetDlgItem(hDlg, RES_DLG_RCT3)) )
    {
        int iWidth = di.ptPaperSize.x;
        int iLength = di.ptPaperSize.y;
        int iExtent;
        GDIRECT aRtSampleXYWH = di.RtSampleXYWH;
        int iX = aRtSampleXYWH.right  / 16;
        int iY = aRtSampleXYWH.bottom / 16;

        if ((iWidth <= 0 || iLength <= 0))
        {
            GDIPOINT ptDefault = di.bEnvelope ? ptEnvelope : pt;
            if (IsDlgButtonChecked(hDlg, RES_DLG_PAGE_PORTRAIT) == 1)
            {
                iWidth = di.ptPaperSize.x = ptDefault.x;
                iLength = di.ptPaperSize.y = ptDefault.y;
            }
            else
            {
                iWidth = di.ptPaperSize.x = ptDefault.y;
                iLength = di.ptPaperSize.y = ptDefault.x;
            }
        }

        if (iWidth > iLength)
        {
            iExtent = MulDiv(aRtSampleXYWH.bottom , iLength , iWidth);
            aRtSampleXYWH.top += (aRtSampleXYWH.bottom - iExtent) / 2;
            aRtSampleXYWH.bottom = iExtent;
        }
        else
        {
            iExtent = MulDiv(aRtSampleXYWH.right , iWidth , iLength);
            aRtSampleXYWH.left += (aRtSampleXYWH.right - iExtent) / 2;
            aRtSampleXYWH.right = iExtent;
        }

        SetWindowPos( hWndSample,
                      0,
                      aRtSampleXYWH.left,
                      aRtSampleXYWH.top,
                      aRtSampleXYWH.right,
                      aRtSampleXYWH.bottom,
                      SWP_NOZORDER );

        SetWindowPos( hWndShadowRight,
                      0,
                      aRtSampleXYWH.left + aRtSampleXYWH.right,
                      aRtSampleXYWH.top + iY,
                      iX,
                      aRtSampleXYWH.bottom,
                      SWP_NOZORDER );

        SetWindowPos( hWndShadowBottom,
                      0,
                      aRtSampleXYWH.left + iX,
                      aRtSampleXYWH.top + aRtSampleXYWH.bottom,
                      aRtSampleXYWH.right,
                      iY,
                      SWP_NOZORDER );

        InvalidateRect(hWndSample, (GDIRECT *)NULL, TRUE);
        UpdateWindow(hDlg);
        UpdateWindow(hWndSample);
        UpdateWindow(hWndShadowRight);
        UpdateWindow(hWndShadowBottom);
    }

    FreeLibrary(hDrv);
    return TRUE;
}



/* Page_OnInitDialog() -- process WM_INITDIALOG.
   return FALSE iff we called SetFocus(). */

BOOL PASCAL Page_OnInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    HWND hCtl;
    HDC hDC;
    int iHeight;
    char buf[PAGE_SETUP_STRINGLIMIT + 1];

    PAGESETUPDLG *ppsd;

    SetWindowLong(hDlg, DWL_USER, (LONG)lParam);

    bResult = FALSE;

    GetProfileString(szIntl, "sDecimal", ".",szDecimal, sizeof(szDecimal));

    IntlHiresToA(di.info.marginleft, buf);
    SetDlgItemText(hDlg, RES_DLG_PAGE_LEFT, buf);

    IntlHiresToA(di.info.margintop, buf);
    SetDlgItemText(hDlg, RES_DLG_PAGE_TOP, buf);

    IntlHiresToA(di.info.marginright, buf);
    SetDlgItemText(hDlg, RES_DLG_PAGE_RIGHT, buf);

    IntlHiresToA(di.info.marginbottom, buf);
    SetDlgItemText(hDlg, RES_DLG_PAGE_BOTTOM, buf);

    if (di.lpdm->dmOrientation ==  DMORIENT_PORTRAIT)
    {
        CheckDlgButton(hDlg, RES_DLG_PAGE_PORTRAIT, 1);
        CheckDlgButton(hDlg, RES_DLG_PAGE_LANDSCAPE, 0);
    }
    else
    {
        CheckDlgButton(hDlg, RES_DLG_PAGE_PORTRAIT, 0);
        CheckDlgButton(hDlg, RES_DLG_PAGE_LANDSCAPE, 1);
    }

    Init_PageListBox(hDlg);

    if (hCtl = GetDlgItem(hDlg, RES_DLG_RCT1))
    {
        GetWindowRect(hCtl, &di.RtSampleXYWH);
        ScreenToClient(hDlg, (GDIPOINT*)&di.RtSampleXYWH.left);
        ScreenToClient(hDlg, (GDIPOINT*)&di.RtSampleXYWH.right);

        iHeight = di.RtSampleXYWH.bottom - di.RtSampleXYWH.top;
        di.RtSampleXYWH.bottom = iHeight;

        if (hDC = GetDC(0))
        {
            iHeight = iHeight * GetDeviceCaps(hDC, LOGPIXELSX) /
                                GetDeviceCaps(hDC, LOGPIXELSY);
            ReleaseDC(0, hDC);
        }

        di.RtSampleXYWH.left =
            (di.RtSampleXYWH.left + di.RtSampleXYWH.right - iHeight) / 2;
        di.RtSampleXYWH.right = iHeight;
    }

    di.bInitSample = TRUE;
    UpdatePageOrientation(hDlg);

    return (TRUE);
}

/* Page_OnCommand() -- process commands from the dialog box. */

#pragma optimize( "gle", off)
VOID PASCAL Page_OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    WORD wID = LOWORD(wParam);
    WORD wNotificationCode = HIWORD(wParam);
    HWND hWndCtrl = (HWND) lParam;

    switch (wID)
    {
        case IDOK:
            {
                char buf[PAGE_SETUP_STRINGLIMIT];
                int id;

                GetDlgItemText(hDlg, RES_DLG_PAGE_LEFT, buf, ARRAYSIZE(buf));
                di.info.marginleft = IntlAToHires(buf);

                GetDlgItemText(hDlg, RES_DLG_PAGE_TOP, buf, ARRAYSIZE(buf));
                di.info.margintop = IntlAToHires(buf);

                GetDlgItemText(hDlg, RES_DLG_PAGE_RIGHT, buf, ARRAYSIZE(buf));
                di.info.marginright = IntlAToHires(buf);

                GetDlgItemText(hDlg, RES_DLG_PAGE_BOTTOM, buf, ARRAYSIZE(buf));
                di.info.marginbottom = IntlAToHires(buf);

                if (di.bConvertedToMetric)
                {
                    di.info.marginleft   = CONVERTTOINCHES(di.info.marginleft);
                    di.info.marginright  = CONVERTTOINCHES(di.info.marginright);
                    di.info.margintop    = CONVERTTOINCHES(di.info.margintop);
                    di.info.marginbottom = CONVERTTOINCHES(di.info.marginbottom);
                }


                if (IsDlgButtonChecked(hDlg, RES_DLG_PAGE_PORTRAIT) == 1)
                    di.lpdm->dmOrientation =  DMORIENT_PORTRAIT;
                else
                    di.lpdm->dmOrientation =  DMORIENT_LANDSCAPE;

                id = (int) SendDlgItemMessage(hDlg, RES_DLG_PAGE_COMBO1,
                                                CB_GETCURSEL, 0, 0L);
                di.lpdm->dmPaperSize = (int) SendDlgItemMessage(hDlg,
                                RES_DLG_PAGE_COMBO1, CB_GETITEMDATA, id, 0L);
                id = (int) SendDlgItemMessage(hDlg, RES_DLG_PAGE_COMBO2,
                                                CB_GETCURSEL, 0, 0L);
                di.lpdm->dmDefaultSource = (int) SendDlgItemMessage(hDlg,
                                RES_DLG_PAGE_COMBO2, CB_GETITEMDATA, id, 0L);

                bResult = TRUE;
                EndDialog(hDlg, TRUE);
            }
            break;

        case IDCANCEL:
            bResult = FALSE;
            EndDialog(hDlg, FALSE);
            break;

        case RES_DLG_PAGE_PRINTER:
            {
            HRESULT hr;
            PRINTINFOBAG *ppib = (PRINTINFOBAG *)GetWindowLong(hDlg, DWL_USER);
            Assert(ppib);

            // Call the print dialog with the current dev mode
            GlobalUnlock(hDevMode);
            ppib->hDevMode = hDevMode;
            hr = FormsPrint(ppib, hDlg, FALSE);
            hDevMode = ppib->hDevMode ;
            di.lpdm = (DEVMODE *)GlobalLock(hDevMode);

            // If something changed, update our UI
            if (SUCCEEDED(hr))
            {
                if (di.lpdm->dmOrientation ==  DMORIENT_PORTRAIT)
                {
                    CheckDlgButton(hDlg, RES_DLG_PAGE_PORTRAIT, 1);
                    CheckDlgButton(hDlg, RES_DLG_PAGE_LANDSCAPE, 0);
                }
                else
                {
                    CheckDlgButton(hDlg, RES_DLG_PAGE_PORTRAIT, 0);
                    CheckDlgButton(hDlg, RES_DLG_PAGE_LANDSCAPE, 1);
                }
                Init_PageListBox(hDlg);
                UpdatePageOrientation(hDlg);
            }
            break;
        }

        case RES_DLG_PAGE_HEADER:
            DialogBox(g_hInstResource, MAKEINTRESOURCE(RES_DLG_PAGE_TITLE),
                       hDlg, (DLGPROC) Header_DialogProc);
            break;

        case RES_DLG_PAGE_COMBO1:
            if ( GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE )
                UpdatePageOrientation(hDlg);
            break;

        case RES_DLG_PAGE_PORTRAIT:
        case RES_DLG_PAGE_LANDSCAPE:
            if ( GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED )
                UpdatePageOrientation(hDlg);
            break;

        case RES_DLG_PAGE_LEFT:
        case RES_DLG_PAGE_RIGHT:
        case RES_DLG_PAGE_TOP:
        case RES_DLG_PAGE_BOTTOM:
            if ( GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE )
                UpdatePageOrientation(hDlg);
            break;

        default:
            break;
    }
}
#pragma optimize( "gle", on)

/* Page_DialogProc() -- THE WINDOW PROCEDURE FOR THE DIALOG BOX. */

UINT CALLBACK Page_DialogProc(HWND hDlg, UINT uMsg,
            WPARAM wParam, LPARAM lParam)
{
    /* WARNING: the cracker/handlers don't appear to have been written
       with dialog boxes in mind, so we spell it out ourselves. */

    switch (uMsg)
    {
        case WM_INITDIALOG:
            hwndRunning = hDlg;
            return (Page_OnInitDialog(hDlg, wParam, lParam));

        case WM_COMMAND:
            Page_OnCommand(hDlg, wParam, lParam);
            return (TRUE);

        case WM_DESTROY:
            hwndRunning = NULL;
            return TRUE;

        default:
            break;
    }
    return (FALSE);
}

BOOL DlgPage_RunDialog(PAGESETUPDLG *pPsd)
{
    // We can only run one instance of this dialog at a time because of global data
    if (hwndRunning)
    {
        if (IsWindow(hwndRunning))
        {
            SetActiveWindow(hwndRunning);
            if (IsIconic(hwndRunning))
            {
                ShowWindow(hwndRunning, SW_RESTORE);
            }
        }
        return FALSE;
    }

    HWND hwnd;
    InitPageSampleWnd();


    // bugbug16: Should use pPsd directly instead of di.info
    di.info.marginleft   = pPsd->rtMargin.left;
    di.info.marginright  = pPsd->rtMargin.right;
    di.info.margintop    = pPsd->rtMargin.top;
    di.info.marginbottom = pPsd->rtMargin.bottom;

    di.bInitSample = FALSE;


    // If a dev mode is passed in, we use it
    hDevMode = pPsd->hDevMode;
    if (NULL == hDevMode)
    {
        // No dev mode, so create one
        hDevMode = Init_DevMode();
        if (!hDevMode)
        {
            char szTitle[100];
            char szMessage[256];
            LoadString(g_hInstResource, IDS_PAGE_SETUP, szTitle, sizeof(szTitle));
            LoadString(g_hInstResource, IDS_NODEFAULTPRINTER, szMessage, sizeof(szMessage));
            MessageBox(pPsd->hwndOwner, szMessage, szTitle, MB_OK | MB_ICONEXCLAMATION);
            return FALSE;
        }
    }
    di.lpdm = (LPDEVMODE)GlobalLock(hDevMode);


    if (!GetProfileInt(szIntl, "iMeasure", 1))
    {
        di.iConvertBy        = 100;
        di.info.marginleft   = CONVERTTOMM(di.info.marginleft);
        di.info.marginright  = CONVERTTOMM(di.info.marginright);
        di.info.margintop    = CONVERTTOMM(di.info.margintop);
        di.info.marginbottom = CONVERTTOMM(di.info.marginbottom);

        di.bConvertedToMetric = TRUE;
        LoadString(g_hInstResource, IDS_MM, di.szMeasure, sizeof(di.szMeasure));
    }
    else
    {
        di.iConvertBy         = 1000;
        di.info.marginleft   *= 100;
        di.info.marginright  *= 100;
        di.info.margintop    *= 100;
        di.info.marginbottom *= 100;

        di.bConvertedToMetric = FALSE;
        LoadString(g_hInstResource, IDS_INCH, di.szMeasure, sizeof(di.szMeasure));
    }

//    FARPROC lpfnDlg = MakeProcInstance((FARPROC)Page_DialogProc, g_hInstResource);
    int ret = DialogBoxParam(g_hInstResource, MAKEINTRESOURCE(RES_DLG_PAGE_SETUP), pPsd->hwndOwner, (DLGPROC)Page_DialogProc,
                pPsd->lCustData);
//    FreeProcInstance(lpfnDlg);

    GlobalUnlock(hDevMode);

    if (ret == 1)
    {
        // Copy hDevMode into the the pagesetup structure.
        // The caller is now responsible for freeing it.
        pPsd->hDevMode = hDevMode;

        pPsd->ptPaperSize.x       = di.ptPaperSize.x;
        pPsd->ptPaperSize.y       = di.ptPaperSize.y;
        pPsd->rtMargin.left       = di.info.marginleft;
        pPsd->rtMargin.right      = di.info.marginright;
        pPsd->rtMargin.top        = di.info.margintop;
        pPsd->rtMargin.bottom     = di.info.marginbottom;
    }
    else
    {
        // Cancelled, so free our dev mode if we created one
        if (NULL == pPsd->hDevMode)
        {
            GlobalFree(hDevMode);
            pPsd->hDevMode = NULL;
        }
    }

    hDevMode = NULL;
    di.lpdm = NULL;

    return ret;
}
        
        