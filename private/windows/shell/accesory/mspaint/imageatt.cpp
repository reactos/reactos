// imageatt.cpp : implementation file
//

#include "stdafx.h"
#include "pbrush.h"
#include "imageatt.h"
#include "hlpcntxt.h"
#include "pbrusdoc.h"
#include "bmobject.h"
#include "imgsuprt.h" // for InvalColorWnd()
#include "image.h"
#ifndef UNICODE
#include <sys\stat.h>
#endif
#include <wchar.h>
#include <tchar.h>
#include <winnls.h>
#ifdef _DEBUG
#undef THIS_FILE
static CHAR BASED_CODE THIS_FILE[] = __FILE__;
#endif

#include "memtrace.h"

#define FIXED_FLOATPT_MULTDIV 1000
#define DECIMAL_POSITIONS 2

/************************* CImageAttr dialog *******************************/
/*

There are a few things to note about the way this object/dialog functions.
It  tries not to convert the currently displayed value unless it notices the
user has modified it.  In all other cases, it works with PIXELS, the value
passed in. If the user modified the width or height,  it does 1 conversion and
then works with pixels.

For the conversion to display the different unit values, it uses the saved
pixel value.

The reason for all of this is due to only n decimal place of accuracy in the
display

The member Vars m_lWidth  and m_lHeight are in the current units (store in
the member variable m_eUnitsCurrent).

The member Vars m_lWidthPixels and m_lHeightPixels are always in Pixels and
these are what are used to convert for the display when changing the units.
*/

CImageAttr::CImageAttr(CWnd* pParent /*=NULL*/)
           : CDialog(CImageAttr::IDD, pParent)
    {
    //{{AFX_DATA_INIT(CImageAttr)
    m_cStringWidth  = TEXT("");
    m_cStringHeight = TEXT("");
        //}}AFX_DATA_INIT

    m_eUnitsCurrent = (eUNITS)theApp.m_iCurrentUnits;

    bEditFieldModified = FALSE;

    m_bMonochrome   = FALSE;

    m_lHeightPixels = 0l;
    m_lWidthPixels  = 0l;
    m_lHeight       = 0l;
    m_lWidth        = 0l;
    }

/***************************************************************************/

void CImageAttr::DoDataExchange(CDataExchange* pDX)
    {
    // saving must be done before the generic dodataexchange below.

    if (! pDX->m_bSaveAndValidate)  // saving to dialog
        {
        FixedFloatPtToString( m_cStringWidth,  m_lWidth  );
        FixedFloatPtToString( m_cStringHeight, m_lHeight );
        }

    CDialog::DoDataExchange( pDX );

    //{{AFX_DATA_MAP(CImageAttr)
    DDX_Text(pDX, IDC_WIDTH, m_cStringWidth);
    DDV_MaxChars(pDX, m_cStringWidth, 5);
    DDX_Text(pDX, IDC_HEIGHT, m_cStringHeight);
    DDV_MaxChars(pDX, m_cStringHeight, 5);
        //}}AFX_DATA_MAP

    if (pDX->m_bSaveAndValidate) // retrieving from dialog
        {
        m_lWidth  = StringToFixedFloatPt( m_cStringWidth  );
        m_lHeight = StringToFixedFloatPt( m_cStringHeight );
        }
    }

/***************************************************************************/

LONG CImageAttr::StringToFixedFloatPt( CString& sString )
    {
    int iInteger = 0;
    int iDecimal = 0;

    TCHAR chDecimal[2] = TEXT("."); // default to period in case GetLocaleInfo
                           // messes up somehow
    GetLocaleInfo (LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, chDecimal, 2);
    if (! sString.IsEmpty())
        {
        int     iPos = sString.Find( chDecimal[0] );
        LPTSTR szTmp = sString.GetBuffer( 1 );

        iInteger = FIXED_FLOATPT_MULTDIV * Atoi( szTmp );

        if (iPos++ >= 0)
            {
            LPTSTR szDecimal = szTmp + iPos;

            if (lstrlen( szDecimal ) > DECIMAL_POSITIONS)
                szDecimal[DECIMAL_POSITIONS] = 0;

            iDecimal = Atoi( szDecimal ) * 10;

            for (int i = lstrlen( szDecimal ); i < DECIMAL_POSITIONS; ++i)
                iDecimal *= 10;
            }
        }

    return ( iInteger + iDecimal );
    }

/***************************************************************************/

void CImageAttr::FixedFloatPtToString( CString& sString, LONG lFixedFloatPt )
    {
    int iInteger =   lFixedFloatPt / FIXED_FLOATPT_MULTDIV;
    int iDecimal = ((lFixedFloatPt % FIXED_FLOATPT_MULTDIV) + 5) / 10;

    TCHAR chDecimal[2] = TEXT("."); // default to period in case GetLocaleInfo
                           // messes up somehow
    GetLocaleInfo (LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, chDecimal, 2);
    LPTSTR psz = sString.GetBufferSetLength( 24 );

    if (iDecimal)
        wsprintf( psz, TEXT("%d%s%d"), iInteger, chDecimal,iDecimal );
    else
        wsprintf( psz,    TEXT("%d"), iInteger );

    sString.ReleaseBuffer();
    }

/***************************************************************************/

BEGIN_MESSAGE_MAP(CImageAttr, CDialog)
    ON_MESSAGE(WM_HELP, OnHelp)
    ON_MESSAGE(WM_CONTEXTMENU, OnContextMenu)
    //{{AFX_MSG_MAP(CImageAttr)
    ON_BN_CLICKED(IDC_INCHES, OnInches)
    ON_BN_CLICKED(IDC_CENTIMETERS, OnCentimeters)
    ON_BN_CLICKED(IDC_PIXELS, OnPixels)
    ON_EN_CHANGE(IDC_HEIGHT, OnChangeHeight)
    ON_EN_CHANGE(IDC_WIDTH, OnChangeWidth)
    ON_BN_CLICKED(IDC_DEFAULT, OnDefault)
    ON_BN_CLICKED(IDC_USE_TRANS, OnUseTrans)
    ON_BN_CLICKED(IDC_SELECT_COLOR, OnSelectColor)
    ON_WM_PAINT()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/************************ CImageAttr message handlers **********************/

static DWORD ImageAttrHelpIds[] =
        {
        IDC_WIDTH_STATIC,       IDH_PAINT_IMAGE_ATTR_WIDTH,
        IDC_WIDTH,                      IDH_PAINT_IMAGE_ATTR_WIDTH,
        IDC_HEIGHT_STATIC,      IDH_PAINT_IMAGE_ATTR_HEIGHT,
        IDC_HEIGHT,                     IDH_PAINT_IMAGE_ATTR_HEIGHT,
        IDC_UNITS_GROUP,        IDH_COMM_GROUPBOX,
        IDC_INCHES,                     IDH_PAINT_IMAGE_ATTR_UNITS_INCHES,
        IDC_CENTIMETERS,        IDH_PAINT_IMAGE_ATTR_UNITS_CM,
        IDC_PIXELS,                     IDH_PAINT_IMAGE_ATTR_UNITS_PELS,
        IDC_COLORS_GROUP,       IDH_COMM_GROUPBOX,
        IDC_MONOCHROME,         IDH_PAINT_IMAGE_ATTR_COLORS_BW,
        IDC_COLORS,                     IDH_PAINT_IMAGE_ATTR_COLORS_COLORS,
        IDC_DEFAULT,            IDH_PAINT_IMAGE_ATTR_DEFAULT,
        IDC_FILEDATE_STATIC,    IDH_PAINT_IMAGE_ATTR_LASTSAVED,
        IDC_FILESIZE_STATIC,    IDH_PAINT_IMAGE_ATTR_SIZE,
        IDC_USE_TRANS,          IDH_PAINT_IMAGE_ATTR_USE_TRANSP,
        IDC_SELECT_COLOR,       IDH_PAINT_IMAGE_ATTR_SEL_COLOR,
        IDC_TRANS_PAINT,        IDH_PAINT_IMAGE_ATTR_PREVIEW,
        0, 0
        };

/***************************************************************************/

LONG
CImageAttr::OnHelp(WPARAM wParam, LPARAM lParam)
{
LONG lResult = 0;
::WinHelp((HWND)(((LPHELPINFO)lParam)->hItemHandle), TEXT("mspaint.hlp"),
                  HELP_WM_HELP, (ULONG_PTR)(LPTSTR)ImageAttrHelpIds);
return lResult;
}

/***************************************************************************/

LONG
CImageAttr::OnContextMenu(WPARAM wParam, LPARAM lParam)
{
LONG lResult = 0;
::WinHelp((HWND)wParam, TEXT("mspaint.hlp"),
                  HELP_CONTEXTMENU,(ULONG_PTR)(LPVOID)ImageAttrHelpIds);
return lResult;
}

/***************************************************************************/

void CImageAttr::PaintTransBox( COLORREF cr )
{
        CWnd * pWnd = GetDlgItem(IDC_TRANS_PAINT);
        CDC  * pDC  = pWnd->GetDC();

        RECT rect;
        pWnd->GetClientRect( &rect );

        CBrush newBrush( m_crTrans & 0xFFFFFF); // disregard palette-relative
        pDC->FillRect (&rect, &newBrush);
    //  CBrush * pOldBrush = pDC->SelectObject( &newBrush );
    //  pDC->Rectangle( &rect );
    //  DeleteObject( pDC->SelectObject( pOldBrush ) );

        pWnd->ReleaseDC( pDC );
}

/***************************************************************************/
#define MAX_SEP_LEN 6
#define MAX_INT_LEN 16
// convert a number into a string with commas in the right place
CString CImageAttr::ReformatSizeString(DWORD dwNumber)
        {

        NUMBERFMT nmf;
        CString strRet;
        TCHAR szSep[MAX_SEP_LEN];
        TCHAR szDec[MAX_SEP_LEN];
        CString sNumber;
        TCHAR szInt[MAX_INT_LEN];
        ZeroMemory (&nmf, sizeof(nmf));
        //
        // Fill in the NUMBERFMT with defaults for the user locale,
        // except for "fractional digits" being 0
        //
        GetLocaleInfo (LOCALE_USER_DEFAULT, LOCALE_ILZERO,
                       szInt, MAX_INT_LEN);
        nmf.LeadingZero = _ttol (szInt);
        nmf.Grouping = 3;
        nmf.lpDecimalSep = (LPTSTR)szDec;
        nmf.lpThousandSep = (LPTSTR)szSep;
        GetLocaleInfo (LOCALE_USER_DEFAULT, LOCALE_SDECIMAL,  nmf.lpDecimalSep,
                       MAX_SEP_LEN);
        GetLocaleInfo (LOCALE_USER_DEFAULT, LOCALE_STHOUSAND,  nmf.lpThousandSep,
                       MAX_SEP_LEN);
        GetLocaleInfo (LOCALE_USER_DEFAULT, LOCALE_INEGNUMBER,
                       szInt,MAX_INT_LEN);
        nmf.NegativeOrder = _ttol (szInt);

        _ltot(dwNumber, sNumber.GetBuffer(20), 10);
        sNumber.ReleaseBuffer();
        int nChar = GetNumberFormat (LOCALE_USER_DEFAULT, 0, sNumber,
                                     &nmf, (LPTSTR)NULL, 0);
        if (nChar)
        {
           GetNumberFormat (LOCALE_USER_DEFAULT, 0, sNumber,
                            &nmf, strRet.GetBuffer(nChar), nChar);
           strRet.ReleaseBuffer();
           return strRet;
        }
        return CString(TEXT("0"));
}

BOOL CImageAttr::OnInitDialog()
{
   CDialog::OnInitDialog();
   CWnd * pFileDate = GetDlgItem(IDC_FILEDATE_STATIC);
   CWnd * pFileSize = GetDlgItem(IDC_FILESIZE_STATIC);
   CString cstrFileDate;
   CString cstrFileSize;

   if (((CPBApp *)AfxGetApp())->m_sCurFile.IsEmpty())
   {

      VERIFY(cstrFileDate.LoadString(IDS_FILEDATE_NA));
      VERIFY(cstrFileSize.LoadString(IDS_FILESIZE_NA));

      pFileDate->SetWindowText(cstrFileDate);
      pFileSize->SetWindowText(cstrFileSize);
   }
   else
   {

      DWORD dwSize = 0L;
      CString fn = ((CPBApp *)AfxGetApp())->m_sCurFile;
      HANDLE hFile;
      CString date;
      CString time;
      SYSTEMTIME sysTime;
      FILETIME   ftSaved;
      FILETIME   ftLocal;
      int dSize;
      //
      // Open a handle to the file, use GetFileTime to
      // get the FILETIME, convert to a SYSTEMTIME and
      // call GetDateFormat and GetTimeFormat
      //
      hFile = ::CreateFile (fn,GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,OPEN_EXISTING,
                            0,NULL);
      if (INVALID_HANDLE_VALUE != hFile)
      {
         // If your bitmap is bigger than 2GB, too bad.
         dwSize = ::GetFileSize (hFile, NULL);
         ::GetFileTime (hFile, NULL, NULL, &ftSaved);
         ::FileTimeToLocalFileTime (&ftSaved, &ftLocal);
         ::FileTimeToSystemTime (&ftLocal, &sysTime);
         dSize = ::GetDateFormat (LOCALE_USER_DEFAULT, DATE_SHORTDATE, &sysTime, NULL,
                                NULL, 0);
         ::GetDateFormat (LOCALE_USER_DEFAULT, DATE_SHORTDATE, &sysTime, NULL,
                        date.GetBuffer (dSize), dSize);
         dSize = ::GetTimeFormat (LOCALE_USER_DEFAULT, TIME_NOSECONDS, &sysTime, NULL,
                                  NULL, 0);
         ::GetTimeFormat (LOCALE_USER_DEFAULT, TIME_NOSECONDS, &sysTime, NULL,
                                  time.GetBuffer (dSize), dSize);
         date.ReleaseBuffer();
         time.ReleaseBuffer();

         VERIFY(cstrFileDate.LoadString(IDS_FILEDATE));
         VERIFY(cstrFileSize.LoadString(IDS_FILESIZE));
         TCHAR szFileDate[96];
         TCHAR szFileSize[64];

         // Display the date, followed by the time
         date+=TEXT(" ");
         date+=time;
         ::wsprintf( szFileDate, cstrFileDate, date );
         ::wsprintf( szFileSize, cstrFileSize, ReformatSizeString(dwSize) );
         ::CloseHandle (hFile);
         pFileDate->SetWindowText(szFileDate);
         pFileSize->SetWindowText(szFileSize);
      }
      else
      {

         VERIFY(cstrFileDate.LoadString(IDS_FILEDATE_NA));
         VERIFY(cstrFileSize.LoadString(IDS_FILESIZE_NA));
         pFileDate->SetWindowText(cstrFileDate);
         pFileSize->SetWindowText(cstrFileSize);
      }
    }

    int idButton = IDC_PIXELS;

    if (m_eUnitsCurrent != ePIXELS)
        idButton = (m_eUnitsCurrent == eINCHES)? IDC_INCHES: IDC_CENTIMETERS;

    CheckRadioButton( IDC_INCHES, IDC_PIXELS, idButton );
    CheckRadioButton( IDC_MONOCHROME, IDC_COLORS,
                      (m_bMonochrome? IDC_MONOCHROME: IDC_COLORS) );

    //
    // We enable the transparent color UI only if modifying a GIF
    //
    GetDlgItem (IDC_USE_TRANS )->EnableWindow (IFLT_GIF == theApp.m_nFltTypeUsed );

    CheckDlgButton( IDC_USE_TRANS, g_bUseTrans);

    CWnd* pSelectColorButton = GetDlgItem(IDC_SELECT_COLOR);
    pSelectColorButton->EnableWindow(g_bUseTrans);

    m_crTrans = crTrans;

    return TRUE;  // return TRUE  unless you set the focus to a control
    }

/***************************************************************************/

void CImageAttr::OnOK()
    {
    ConvertWidthHeight();

    theApp.m_iCurrentUnits = m_eUnitsCurrent;
    m_bMonochrome = (GetCheckedRadioButton( IDC_MONOCHROME, IDC_COLORS )
                                         == IDC_MONOCHROME);
    if (g_bUseTrans = IsDlgButtonChecked( IDC_USE_TRANS ))
    {
       crTrans = m_crTrans;
    }
    InvalColorWnd();

    CDialog::OnOK();
    }

/***************************************************************************/

void CImageAttr::OnDefault()
    {
    int nWidth, nHeight;

    PBGetDefDims(nWidth, nHeight);

    SetWidthHeight( nWidth, nHeight );
    }

/***************************************************************************/

void CImageAttr::SetWidthHeight(UINT nWidthPixels, UINT nHeightPixels)
    {
    m_lWidthPixels  = nWidthPixels  * FIXED_FLOATPT_MULTDIV;
    m_lHeightPixels = nHeightPixels * FIXED_FLOATPT_MULTDIV;

    PelsToCurrentUnit();

    // only call updatedata if dialog exists...
    if (m_hWnd && ::IsWindow( m_hWnd ))
        UpdateData( FALSE );
    }

/***************************************************************************/

void  CImageAttr::ConvertWidthHeight(void)
    {
    // if user modified the edit field Width/Height then get new data and
    // convert to pixel format.  Else use stored pixel format.
    if (bEditFieldModified)
        {
        UpdateData( TRUE );

        switch (m_eUnitsCurrent)
            {
            case eINCHES:
                 m_lWidthPixels  = m_lWidth  * theApp.ScreenDeviceInfo.ixPelsPerINCH;
                 m_lHeightPixels = m_lHeight * theApp.ScreenDeviceInfo.iyPelsPerINCH;
                 break;

            case eCM:
                 m_lWidthPixels  = m_lWidth  * theApp.ScreenDeviceInfo.ixPelsPerDM / 10;
                 m_lHeightPixels = m_lHeight * theApp.ScreenDeviceInfo.iyPelsPerDM / 10;
                 break;

            case ePIXELS:
            default: // ePIXELS and all other assumed to be pixel
                 m_lWidthPixels  = m_lWidth;
                 m_lHeightPixels = m_lHeight;
                 break;
            }

        bEditFieldModified = FALSE;
        }
    }

/***************************************************************************/

void CImageAttr::PelsToCurrentUnit()
    {
    switch (m_eUnitsCurrent)
        {
        case eINCHES:
            m_lWidth  = m_lWidthPixels  / theApp.ScreenDeviceInfo.ixPelsPerINCH;
            m_lHeight = m_lHeightPixels / theApp.ScreenDeviceInfo.iyPelsPerINCH;
            break;

        case eCM:
            m_lWidth  = m_lWidthPixels  * 10 / theApp.ScreenDeviceInfo.ixPelsPerDM;
            m_lHeight = m_lHeightPixels * 10 / theApp.ScreenDeviceInfo.iyPelsPerDM;
            break;

        case ePIXELS:
        default:
            //Pixels cannot be partial
            //make sure whole number when converted to string (truncate! now).
            m_lWidth  = (m_lWidthPixels  / FIXED_FLOATPT_MULTDIV) * FIXED_FLOATPT_MULTDIV;
            m_lHeight = (m_lHeightPixels / FIXED_FLOATPT_MULTDIV) * FIXED_FLOATPT_MULTDIV;
            break;
        }
    }

/***************************************************************************/

CSize CImageAttr::GetWidthHeight(void)
    {
    return CSize( (int)(m_lWidthPixels  / FIXED_FLOATPT_MULTDIV),
                  (int)(m_lHeightPixels / FIXED_FLOATPT_MULTDIV) );
    }

/***************************************************************************/

void CImageAttr::OnInches()
    {
    SetNewUnits( eINCHES );
    }

/***************************************************************************/

void CImageAttr::OnCentimeters()
    {
    SetNewUnits( eCM );
    }

/***************************************************************************/

void CImageAttr::OnPixels()
    {
    SetNewUnits( ePIXELS );
    }

/***************************************************************************/

void CImageAttr::SetNewUnits( eUNITS NewUnit )
    {
    if (NewUnit == m_eUnitsCurrent)
        return;

    // must call getwidthheight before  setting to new mode
    ConvertWidthHeight(); // get in a common form of pixels.

    m_eUnitsCurrent = NewUnit;

    PelsToCurrentUnit();

    UpdateData( FALSE );
    }

/***************************************************************************/

void CImageAttr::OnChangeHeight()
    {
    bEditFieldModified = TRUE;
    }

/***************************************************************************/

void CImageAttr::OnChangeWidth()
    {
    bEditFieldModified = TRUE;
    }

/************************ CZoomViewDlg dialog ******************************/

CZoomViewDlg::CZoomViewDlg(CWnd* pParent /*=NULL*/)
             : CDialog(CZoomViewDlg::IDD, pParent)
    {
    //{{AFX_DATA_INIT(CZoomViewDlg)
    //}}AFX_DATA_INIT

    m_nCurrent = 0;
    }

/***************************************************************************/

void CZoomViewDlg::DoDataExchange(CDataExchange* pDX)
    {
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CZoomViewDlg)
    //}}AFX_DATA_MAP
    }

/***************************************************************************/

BEGIN_MESSAGE_MAP(CZoomViewDlg, CDialog)
        ON_MESSAGE(WM_HELP, OnHelp)
        ON_MESSAGE(WM_CONTEXTMENU, OnContextMenu)
    //{{AFX_MSG_MAP(CZoomViewDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/************************ CZoomViewDlg message handlers **********************/

static DWORD ZoomViewHelpIds[] =
        {
        IDC_CURRENT_ZOOM_STATIC,        IDH_PAINT_ZOOM_CURRENT,
        IDC_CURRENT_ZOOM,                       IDH_PAINT_ZOOM_CURRENT,
        IDC_ZOOM_GROUP,                         IDH_PAINT_ZOOM_SET_GROUP,
        IDC_ZOOM_100,                           IDH_PAINT_ZOOM_SET_GROUP,
        IDC_ZOOM_200,                           IDH_PAINT_ZOOM_SET_GROUP,
        IDC_ZOOM_400,                           IDH_PAINT_ZOOM_SET_GROUP,
        IDC_ZOOM_600,                           IDH_PAINT_ZOOM_SET_GROUP,
        IDC_ZOOM_800,                           IDH_PAINT_ZOOM_SET_GROUP,
        0, 0
        };

/***************************************************************************/

LONG
CZoomViewDlg::OnHelp(WPARAM wParam, LPARAM lParam)
{
LONG lResult = 0;
::WinHelp((HWND)(((LPHELPINFO)lParam)->hItemHandle), TEXT("mspaint.hlp"),
                  HELP_WM_HELP, (ULONG_PTR)(LPTSTR)ZoomViewHelpIds);
return lResult;
}

/***************************************************************************/

LONG
CZoomViewDlg::OnContextMenu(WPARAM wParam, LPARAM lParam)
{
LONG lResult = 0;
::WinHelp((HWND)wParam, TEXT("mspaint.hlp"),
                  HELP_CONTEXTMENU,(ULONG_PTR)(LPVOID)ZoomViewHelpIds);
return lResult;
}

/***************************************************************************/

BOOL CZoomViewDlg::OnInitDialog()
    {
    CDialog::OnInitDialog();

    TCHAR* pZoom = TEXT("100%");
    UINT nButton = IDC_ZOOM_100;

    if (m_nCurrent < 8)
        if (m_nCurrent < 6)
            if (m_nCurrent < 4)
                if (m_nCurrent < 2)
                    ;
                else
                    {
                    pZoom = TEXT("200%");
                    nButton = IDC_ZOOM_200;
                    }
            else
                {
                pZoom = TEXT("400%");
                nButton = IDC_ZOOM_400;
                }
        else
            {
            pZoom = TEXT("600%");
            nButton = IDC_ZOOM_600;
            }
    else
        {
        pZoom = TEXT("800%");
        nButton = IDC_ZOOM_800;
        }

    SetDlgItemText( IDC_CURRENT_ZOOM, pZoom );
    CheckRadioButton( IDC_ZOOM_100, IDC_ZOOM_800, nButton );

    return TRUE;  // return TRUE  unless you set the focus to a control
    }

/***************************************************************************/

void CZoomViewDlg::OnOK()
    {
    m_nCurrent = GetCheckedRadioButton( IDC_ZOOM_100, IDC_ZOOM_800 ) - IDC_ZOOM_100;

    if (m_nCurrent < 1)
        m_nCurrent  = 1;
    else
        m_nCurrent *= 2;

    CDialog::OnOK();
    }

/************************ CFlipRotateDlg dialog ****************************/

CFlipRotateDlg::CFlipRotateDlg(CWnd* pParent /*=NULL*/)
               : CDialog(CFlipRotateDlg::IDD, pParent)
    {
    //{{AFX_DATA_INIT(CFlipRotateDlg)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    m_bHorz  = TRUE;
    m_bAngle = FALSE;
    m_nAngle = 90;
    }

/***************************************************************************/

void CFlipRotateDlg::DoDataExchange(CDataExchange* pDX)
    {
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CFlipRotateDlg)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
    }

/***************************************************************************/

BEGIN_MESSAGE_MAP(CFlipRotateDlg, CDialog)
        ON_MESSAGE(WM_HELP, OnHelp)
        ON_MESSAGE(WM_CONTEXTMENU, OnContextMenu)
    //{{AFX_MSG_MAP(CFlipRotateDlg)
    ON_BN_CLICKED(IDC_BY_ANGLE, OnByAngle)
    ON_BN_CLICKED(IDC_HORIZONTAL, OnNotByAngle)
    ON_BN_CLICKED(IDC_VERTICAL, OnNotByAngle)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/************************ CFlipRotateDlg message handlers **********************/

static DWORD FlipRotateHelpIds[] =
        {
        IDC_PAINT_FLIP_GROUP,   IDH_COMM_GROUPBOX,
        IDC_HORIZONTAL,                 IDH_PAINT_IMAGE_FLIP_HORIZ,
        IDC_VERTICAL,                   IDH_PAINT_IMAGE_FLIP_VERT,
        IDC_BY_ANGLE,                   IDH_PAINT_IMAGE_FLIP_ROTATE,
        IDC_90_DEG,                             IDH_PAINT_IMAGE_FLIP_ROTATE,
        IDC_180_DEG,                    IDH_PAINT_IMAGE_FLIP_ROTATE,
        IDC_270_DEG,                    IDH_PAINT_IMAGE_FLIP_ROTATE,
        0, 0
        };

/***************************************************************************/

LONG
CFlipRotateDlg::OnHelp(WPARAM wParam, LPARAM lParam)
{
LONG lResult = 0;
::WinHelp((HWND)(((LPHELPINFO)lParam)->hItemHandle), TEXT("mspaint.hlp"),
                  HELP_WM_HELP, (ULONG_PTR)(LPTSTR)FlipRotateHelpIds);
return lResult;
}

/***************************************************************************/

LONG
CFlipRotateDlg::OnContextMenu(WPARAM wParam, LPARAM lParam)
{
LONG lResult = 0;
::WinHelp((HWND)wParam, TEXT("mspaint.hlp"),
                  HELP_CONTEXTMENU,(ULONG_PTR)(LPVOID)FlipRotateHelpIds);
return lResult;
}

/***************************************************************************/

BOOL CFlipRotateDlg::OnInitDialog()
    {
    CDialog::OnInitDialog();

    CheckRadioButton( IDC_90_DEG, IDC_270_DEG, IDC_90_DEG );

    UINT uButton = (m_bAngle? IDC_BY_ANGLE: (m_bHorz? IDC_HORIZONTAL: IDC_VERTICAL));

    CheckRadioButton( IDC_HORIZONTAL, IDC_BY_ANGLE, uButton );

    if (uButton != IDC_BY_ANGLE)
        OnNotByAngle();

    return TRUE;  // return TRUE  unless you set the focus to a control
    }

/***************************************************************************/

void CFlipRotateDlg::OnByAngle()
    {
    GetDlgItem( IDC_90_DEG  )->EnableWindow( TRUE );
    GetDlgItem( IDC_180_DEG )->EnableWindow( TRUE );
    GetDlgItem( IDC_270_DEG )->EnableWindow( TRUE );
    }

/***************************************************************************/

void CFlipRotateDlg::OnNotByAngle()
    {
    GetDlgItem( IDC_90_DEG  )->EnableWindow( FALSE );
    GetDlgItem( IDC_180_DEG )->EnableWindow( FALSE );
    GetDlgItem( IDC_270_DEG )->EnableWindow( FALSE );
    }

/***************************************************************************/

void CFlipRotateDlg::OnOK()
    {
    UINT uButton = GetCheckedRadioButton( IDC_HORIZONTAL, IDC_BY_ANGLE );

    m_bHorz  = (uButton == IDC_HORIZONTAL);
    m_bAngle = (uButton == IDC_BY_ANGLE);

    switch (GetCheckedRadioButton( IDC_90_DEG, IDC_270_DEG ))
        {
        case IDC_90_DEG:
            m_nAngle = 90;
            break;

        case IDC_180_DEG:
            m_nAngle = 180;
            break;

        case IDC_270_DEG:
            m_nAngle = 270;
            break;
        }

    CDialog::OnOK();
    }

/************************* CStretchSkewDlg dialog **************************/

CStretchSkewDlg::CStretchSkewDlg(CWnd* pParent /*=NULL*/)
                : CDialog(CStretchSkewDlg::IDD, pParent)
    {
    //{{AFX_DATA_INIT(CStretchSkewDlg)
    m_wSkewHorz = 0;
    m_wSkewVert = 0;
    m_iStretchVert = 100;
    m_iStretchHorz = 100;
    //}}AFX_DATA_INIT

    //m_bStretchHorz = TRUE;
    //m_bSkewHorz    = TRUE;
    }

/***************************************************************************/

void CStretchSkewDlg::DoDataExchange(CDataExchange* pDX)
    {
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CStretchSkewDlg)
    DDX_Text(pDX, IDC_STRETCH_VERT_PERCENT, m_iStretchVert);
    DDV_MinMaxInt(pDX, m_iStretchVert, 1, 500);
    DDX_Text(pDX, IDC_STRETCH_HORZ_PERCENT, m_iStretchHorz);
    DDV_MinMaxInt(pDX, m_iStretchHorz, 1, 500);
    DDX_Text(pDX, IDC_SKEW_HORZ_DEGREES, m_wSkewHorz);
    DDV_MinMaxInt(pDX, m_wSkewHorz, -89, 89);
    DDX_Text(pDX, IDC_SKEW_VERT_DEGREES, m_wSkewVert);
    DDV_MinMaxInt(pDX, m_wSkewVert, -89, 89);

    //}}AFX_DATA_MAP
    }

/***************************************************************************/

BEGIN_MESSAGE_MAP(CStretchSkewDlg, CDialog)
        ON_MESSAGE(WM_HELP, OnHelp)
        ON_MESSAGE(WM_CONTEXTMENU, OnContextMenu)
    //{{AFX_MSG_MAP(CStretchSkewDlg)
    /*
    ON_BN_CLICKED(IDC_SKEW_HORZ, OnSkewHorz)
    ON_BN_CLICKED(IDC_SKEW_VERT, OnSkewVert)
    ON_BN_CLICKED(IDC_STRETCH_HORZ, OnStretchHorz)
    ON_BN_CLICKED(IDC_STRETCH_VERT, OnStretchVert)
    */
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/************************ CStretchSkewDlg message handlers **********************/

static DWORD StretchSkewHelpIds[] =
        {
        IDC_STRETCH_GROUP,                      IDH_COMM_GROUPBOX,
        IDC_STRETCH_HORZ_ICON,          IDH_PAINT_IMAGE_STRETCH_HORIZ,
        IDC_STRETCH_HORZ,                       IDH_PAINT_IMAGE_STRETCH_HORIZ,
        IDC_STRETCH_HORZ_PERCENT,       IDH_PAINT_IMAGE_STRETCH_HORIZ,
        IDC_STRETCH_HORZ_SUFFIX,        IDH_PAINT_IMAGE_STRETCH_HORIZ,
        IDC_STRETCH_VERT_ICON,          IDH_PAINT_IMAGE_STRETCH_VERT,
        IDC_STRETCH_VERT,                       IDH_PAINT_IMAGE_STRETCH_VERT,
        IDC_STRETCH_VERT_PERCENT,       IDH_PAINT_IMAGE_STRETCH_VERT,
        IDC_STRETCH_VERT_SUFFIX,        IDH_PAINT_IMAGE_STRETCH_VERT,
        IDC_SKEW_GROUP,                         IDH_COMM_GROUPBOX,
        IDC_SKEW_HORZ_ICON,                     IDH_PAINT_IMAGE_SKEW_HOR,
        IDC_SKEW_HORZ,                          IDH_PAINT_IMAGE_SKEW_HOR,
        IDC_SKEW_HORZ_DEGREES,          IDH_PAINT_IMAGE_SKEW_HOR,
        IDC_SKEW_HORZ_SUFFIX,           IDH_PAINT_IMAGE_SKEW_HOR,
        IDC_SKEW_VERT_ICON,                     IDH_PAINT_IMAGE_SKEW_VERT,
        IDC_SKEW_VERT,                          IDH_PAINT_IMAGE_SKEW_VERT,
        IDC_SKEW_VERT_DEGREES,          IDH_PAINT_IMAGE_SKEW_VERT,
        IDC_SKEW_VERT_SUFFIX,           IDH_PAINT_IMAGE_SKEW_VERT,
        0, 0
        };

/***************************************************************************/

LONG
CStretchSkewDlg::OnHelp(WPARAM wParam, LPARAM lParam)
{
LONG lResult = 0;
::WinHelp((HWND)(((LPHELPINFO)lParam)->hItemHandle), TEXT("mspaint.hlp"),
                  HELP_WM_HELP, (ULONG_PTR)(LPTSTR)StretchSkewHelpIds);
return lResult;
}

/***************************************************************************/

LONG
CStretchSkewDlg::OnContextMenu(WPARAM wParam, LPARAM lParam)
{
LONG lResult = 0;
::WinHelp((HWND)wParam, TEXT("mspaint.hlp"),
                  HELP_CONTEXTMENU,(ULONG_PTR)(LPVOID)StretchSkewHelpIds);
return lResult;
}

/***************************************************************************/


BOOL CStretchSkewDlg::OnInitDialog()
    {
    BOOL bEnableTrans;
    CDialog::OnInitDialog();

    CheckRadioButton( IDC_STRETCH_HORZ, IDC_STRETCH_VERT, IDC_STRETCH_HORZ );
    CheckRadioButton( IDC_SKEW_HORZ   , IDC_SKEW_VERT   , IDC_SKEW_HORZ    );

   /* GetDlgItem( IDC_STRETCH_HORZ_PERCENT )->EnableWindow(   m_iStretchHorz );
    GetDlgItem( IDC_STRETCH_VERT_PERCENT )->EnableWindow( ! m_iStretchHorz );
    GetDlgItem( IDC_SKEW_HORZ_DEGREES )->EnableWindow(   m_bSkewHorz );
    GetDlgItem( IDC_SKEW_VERT_DEGREES )->EnableWindow( ! m_bSkewHorz );
*/
    return TRUE;  // return TRUE  unless you set the focus to a control
    }

/***************************************************************************/

void CStretchSkewDlg::OnStretchHorz()
    {
    m_bStretchHorz = TRUE;

    GetDlgItem( IDC_STRETCH_HORZ_PERCENT )->EnableWindow( TRUE  );
    GetDlgItem( IDC_STRETCH_VERT_PERCENT )->EnableWindow( FALSE );
    CheckRadioButton( IDC_STRETCH_HORZ, IDC_STRETCH_VERT, IDC_STRETCH_HORZ );
    }

/***************************************************************************/

void CStretchSkewDlg::OnStretchVert()
    {
    m_bStretchHorz = FALSE;

    GetDlgItem( IDC_STRETCH_HORZ_PERCENT )->EnableWindow( FALSE );
    GetDlgItem( IDC_STRETCH_VERT_PERCENT )->EnableWindow( TRUE  );
    CheckRadioButton( IDC_STRETCH_HORZ, IDC_STRETCH_VERT, IDC_STRETCH_VERT );
    }

/***************************************************************************/

void CStretchSkewDlg::OnSkewHorz()
    {
    m_bSkewHorz = TRUE;

    GetDlgItem( IDC_SKEW_HORZ_DEGREES )->EnableWindow( TRUE  );
    GetDlgItem( IDC_SKEW_VERT_DEGREES )->EnableWindow( FALSE );
    CheckRadioButton( IDC_SKEW_HORZ, IDC_SKEW_VERT, IDC_SKEW_HORZ );
    }

/***************************************************************************/

void CStretchSkewDlg::OnSkewVert()
    {
    m_bSkewHorz = FALSE;

    GetDlgItem( IDC_SKEW_HORZ_DEGREES )->EnableWindow( FALSE );
    GetDlgItem( IDC_SKEW_VERT_DEGREES )->EnableWindow( TRUE  );
    CheckRadioButton( IDC_SKEW_HORZ, IDC_SKEW_VERT, IDC_SKEW_VERT );
    }

/***************************************************************************/

void CStretchSkewDlg::OnOK()
    {
    if (GetCheckedRadioButton( IDC_STRETCH_HORZ, IDC_STRETCH_VERT )
                            == IDC_STRETCH_HORZ)
        m_iStretchVert = 0;
    else
        m_iStretchHorz = 0;

    if (GetCheckedRadioButton( IDC_SKEW_HORZ, IDC_SKEW_VERT )
                            == IDC_SKEW_HORZ)
        m_wSkewVert = 0;
    else
        m_wSkewHorz = 0;

    CDialog::OnOK();
    }

/***************************************************************************/

void CImageAttr::OnUseTrans()
{
   CWnd* pSelectColorButton = GetDlgItem(IDC_SELECT_COLOR);
   pSelectColorButton->EnableWindow(IsDlgButtonChecked(IDC_USE_TRANS));
}

extern INT_PTR CALLBACK AfxDlgProc(HWND, UINT, WPARAM, LPARAM);

static UINT_PTR CALLBACK /*LPCCHOOKPROC*/
SelectColorHook(HWND hColorDlg, UINT nMessage, WPARAM wParam, LPARAM lParam)
{
// Are we initializing the dialog window?
if ( nMessage == WM_INITDIALOG )
        {
        // Reset the common dialog title
        CString strDialogTitle;
        VERIFY(strDialogTitle.LoadString(IDS_SELECT_COLOR));
        SetWindowText( hColorDlg, strDialogTitle );
        }
// Pass All Messages Along to Common Dialog
return (UINT)AfxDlgProc(hColorDlg, nMessage, wParam, lParam );
}

void CImageAttr::OnSelectColor()
{
   // for default color selection, disregard palette-relative
    CColorDialog dlg( m_crTrans & 0xFFFFFF, CC_FULLOPEN );
        dlg.m_cc.lpfnHook = SelectColorHook;

    if (dlg.DoModal() != IDOK)
        return;

        PaintTransBox( m_crTrans = dlg.GetColor() );
}

void CImageAttr::OnPaint()
{
        CPaintDC dc(this); // device context for painting

        if (m_crTrans != TRANS_COLOR_NONE)    // not default
                PaintTransBox( m_crTrans );
}
