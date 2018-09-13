
#include "stdafx.h"
#include "global.h"
#include "pbrush.h"
#include "pbrusdoc.h"
#include "pbrusfrm.h"
#include "pbrusvw.h"
#include "docking.h"
#include "minifwnd.h"
#include "bmobject.h"
#include "colorsrc.h"
#include "imgsuprt.h"
#include "imgwnd.h"
#include "cmpmsg.h"
#include "imgdlgs.h"
#include "ferr.h"

#include <colordlg.h>
#include <direct.h>

#ifdef _DEBUG
#undef THIS_FILE
static CHAR BASED_CODE THIS_FILE[] = __FILE__;
#endif

#include "memtrace.h"

CSize NEAR g_defaultTileGridSize(16, 15);
BOOL  NEAR g_bDefaultTileGrid = FALSE;


BEGIN_MESSAGE_MAP(C3dDialog, CDialog)
    ON_COMMAND(IDOK, OnRobustOK)
    ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

C3dDialog::C3dDialog(LPCTSTR lpszTemplateName, CWnd* pParentWnd)
          : CDialog(lpszTemplateName, pParentWnd)
    {
    }

C3dDialog::C3dDialog(UINT nTemplateID, CWnd* pParentWnd)
          : CDialog(nTemplateID, pParentWnd)
    {
    }


BOOL C3dDialog::OnInitDialog()
    {
    // automatically center the dialog relative to it's parent
    CenterWindow(CmpCenterParent());

    return CDialog::OnInitDialog();
    }

void C3dDialog::OnRobustOK()
    {
    OnOK(); // ok to call "real" OnOK
    }

HBRUSH C3dDialog::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
    {
    HBRUSH hbrush = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

    return hbrush;
    }

BEGIN_MESSAGE_MAP(CImgGridDlg, C3dDialog)
    ON_BN_CLICKED(IDC_PIXELGRID, OnClickPixelGrid)
    ON_BN_CLICKED(IDC_TILEGRID, OnClickTileGrid)
END_MESSAGE_MAP()


CImgGridDlg::CImgGridDlg() : C3dDialog(IDD_IMGGRIDOPT)
    {
    m_bPixelGrid = theApp.m_bShowGrid;
    m_bTileGrid  = g_bDefaultTileGrid;
    m_nWidth     = g_defaultTileGridSize.cx;
    m_nHeight    = g_defaultTileGridSize.cy;
    }


BOOL CImgGridDlg::OnInitDialog()
    {
    C3dDialog::OnInitDialog();

    CheckDlgButton(IDC_PIXELGRID, m_bPixelGrid);
    CheckDlgButton(IDC_TILEGRID, m_bTileGrid);
    SetDlgItemInt(IDC_WIDTH, m_nWidth, FALSE);
    SetDlgItemInt(IDC_HEIGHT, m_nHeight, FALSE);

    GetDlgItem(IDC_TILEGRID)->EnableWindow(m_bPixelGrid);
    GetDlgItem(IDC_WIDTH)->EnableWindow(m_bPixelGrid && m_bTileGrid);
    GetDlgItem(IDC_HEIGHT)->EnableWindow(m_bPixelGrid && m_bTileGrid);

    return TRUE;
    }


void CImgGridDlg::OnClickPixelGrid()
    {
    m_bPixelGrid = !m_bPixelGrid;
    CheckDlgButton(IDC_PIXELGRID, m_bPixelGrid);

    GetDlgItem(IDC_TILEGRID)->EnableWindow(m_bPixelGrid);
    GetDlgItem(IDC_WIDTH)->EnableWindow(m_bPixelGrid);
    GetDlgItem(IDC_HEIGHT)->EnableWindow(m_bPixelGrid);
    }


void CImgGridDlg::OnClickTileGrid()
    {
    m_bTileGrid = !m_bTileGrid;
    CheckDlgButton(IDC_TILEGRID, m_bTileGrid);

    GetDlgItem(IDC_WIDTH)->EnableWindow(m_bTileGrid);
    GetDlgItem(IDC_HEIGHT)->EnableWindow(m_bTileGrid);
    }

void CImgGridDlg::OnOK()
    {
    m_bPixelGrid = IsDlgButtonChecked(IDC_PIXELGRID);
    m_bTileGrid = IsDlgButtonChecked(IDC_TILEGRID);

    if (m_bTileGrid)
        {
        BOOL bTranslated;

        if (GetDlgItem(IDC_WIDTH)->GetWindowTextLength() == 0)
            {
            m_nWidth = 1;
            }
        else
            {
            m_nWidth = GetDlgItemInt(IDC_WIDTH, &bTranslated, FALSE);
            if (!bTranslated || m_nWidth < GRIDMIN || m_nWidth > GRIDMAX)
                {
                CmpMessageBoxPrintf(IDS_ERROR_GRIDRANGE, AFX_IDS_APP_TITLE,
                    MB_OK | MB_ICONEXCLAMATION, GRIDMIN, GRIDMAX);      // LOCALIZATION
                GetDlgItem(IDC_WIDTH)->SetFocus();
                return;
                }
            }

        if (GetDlgItem(IDC_HEIGHT)->GetWindowTextLength() == 0)
            {
            m_nHeight = 1;
            }
        else
            {
            m_nHeight = GetDlgItemInt(IDC_HEIGHT, &bTranslated, FALSE);
            if (!bTranslated || m_nHeight < GRIDMIN || m_nHeight > GRIDMAX)
                {
                CmpMessageBoxPrintf(IDS_ERROR_GRIDRANGE, AFX_IDS_APP_TITLE,
                    MB_OK | MB_ICONEXCLAMATION, GRIDMIN, GRIDMAX);      // LOCALIZATION
                GetDlgItem(IDC_HEIGHT)->SetFocus();
                return;
                }
            }

        g_defaultTileGridSize.cx = m_nWidth;
        g_defaultTileGridSize.cy = m_nHeight;
        }

    g_bDefaultTileGrid = m_bTileGrid;

    C3dDialog::OnOK();
    }


/***************************************************************************/
// CColorTable dialog

CColorTable::CColorTable( CWnd* pParent /*=NULL*/ )
            : CDialog( CColorTable::IDD, pParent )
    {
    m_bLeft  = TRUE;
    m_iColor = 0;
    }

/***************************************************************************/

void CColorTable::DoDataExchange( CDataExchange* pDX )
    {
        CDialog::DoDataExchange( pDX );
    }

/***************************************************************************/

BEGIN_MESSAGE_MAP(CColorTable, CDialog)
    //{{AFX_MSG_MAP(CColorTable)
    ON_WM_DRAWITEM()
    ON_WM_MEASUREITEM()
    ON_LBN_DBLCLK(IDC_COLORLIST, OnDblclkColorlist)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/***************************************************************************/
// CColorTable message handlers

void CColorTable::OnDrawItem( int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct )
    {
    if (lpDrawItemStruct->itemID >= 0)
        {
        CDC       dcCombo;
        CBrush    brColor;
        CPalette* pPalOld = NULL;
        CRect     rect    = lpDrawItemStruct->rcItem;

        dcCombo.Attach( lpDrawItemStruct->hDC );

        if (theApp.m_pPalette)
            {
            pPalOld = dcCombo.SelectPalette( theApp.m_pPalette, FALSE );
            dcCombo.RealizePalette();
            }

        brColor.CreateSolidBrush( (COLORREF)lpDrawItemStruct->itemData );

        if ((lpDrawItemStruct->itemState & ODS_SELECTED) == ODS_SELECTED)
            {
            dcCombo.FillRect( &(lpDrawItemStruct->rcItem), &brColor );
            rect.InflateRect( -theApp.m_cxFrame, -theApp.m_cyFrame );
            }

        dcCombo.FillRect( &(lpDrawItemStruct->rcItem), &brColor );

        brColor.DeleteObject();

        if ((lpDrawItemStruct->itemState & ODS_FOCUS) == ODS_FOCUS)
            dcCombo.DrawFocusRect( &(lpDrawItemStruct->rcItem) );

        if (pPalOld != NULL)
            dcCombo.SelectPalette( pPalOld, FALSE );
        }

    CDialog::OnDrawItem( nIDCtl, lpDrawItemStruct );
    }

/***************************************************************************/

BOOL CColorTable::OnInitDialog()
    {
    CDialog::OnInitDialog();

    int iColorCnt   = g_pColors->GetColorCount();
    CListBox* pList = (CListBox*)GetDlgItem( IDC_COLORLIST );

    for (int iLoop = 0; iLoop < iColorCnt; iLoop++)
        {
        pList->AddString( TEXT("") );
        pList->SetItemData( iLoop, g_pColors->GetColor( iLoop ) );
        }


    return TRUE;  // return TRUE  unless you set the focus to a control
    }

/***************************************************************************/

void CColorTable::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct)
    {
    lpMeasureItemStruct->itemHeight = theApp.m_cyCaption;

    CDialog::OnMeasureItem(nIDCtl, lpMeasureItemStruct);
    }

/***************************************************************************/

void CColorTable::OnDblclkColorlist()
    {


    }

/***************************************************************************/

void CColorTable::OnOK()
    {
//  m_iColor = ;

        CDialog::OnOK();
    }

/***************************************************************************/
