// srvritem.cpp : implementation of the CPBSrvrItem class
//

#include "stdafx.h"
#include "pbrush.h"
#include "pbrusdoc.h"
#include "pbrusfrm.h"
#include "pbrusvw.h"
#include "srvritem.h"
#include "bmobject.h"
#include "docking.h"
#include "minifwnd.h"
#include "imgwnd.h"
#include "imgsuprt.h"
#include "imgcolor.h"
#include "tracker.h"

#ifdef _DEBUG
#undef THIS_FILE
static CHAR BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CPBSrvrItem, COleServerItem)

#include "memtrace.h"

/***************************************************************************/
// CPBSrvrItem implementation

CPBSrvrItem::CPBSrvrItem( CPBDoc* pContainerDoc, CBitmapObj* pBM )
                : COleServerItem( pContainerDoc, TRUE )
{
        if (pBM)
        {
                m_pBitmapObj = pBM;
        }
        else
        {
                pBM = pContainerDoc->m_pBitmapObj;

                m_pBitmapObj = NULL;
        }

        if (pBM && pBM->m_pImg)
        {
                COleDataSource* oleDataSrc = GetDataSource();

                // support CF_DIB format
                oleDataSrc->DelayRenderFileData( CF_DIB );
                oleDataSrc->DelayRenderData( CF_BITMAP );

                if (pBM->m_pImg->m_pPalette != NULL)
                {
                        oleDataSrc->DelayRenderData( CF_PALETTE );
                }
        }
}

/***************************************************************************/

CPBSrvrItem::~CPBSrvrItem()
{
        // TODO: add cleanup code here
        if (m_pBitmapObj)
        {
                delete m_pBitmapObj;
        }
}

/***************************************************************************/

void CPBSrvrItem::Serialize(CArchive& ar)
    {
    // CPBSrvrItem::Serialize will be called by the framework if
    //  the item is copied to the clipboard.  This can happen automatically
    //  through the OLE callback OnGetClipboardData.  A good default for
    //  the embedded item is simply to delegate to the document's Serialize
    //  function.  If you support links, then you will want to serialize
    //  just a portion of the document.

    // IsLinkedItem always returns TRUE even though we don't support links,
    // so I am just removing the check
    // if (! IsLinkedItem())
        {
        CPBDoc* pDoc = GetDocument();
        ASSERT_VALID(pDoc);

        CBitmapObj* pBMCur = m_pBitmapObj ? m_pBitmapObj : pDoc->m_pBitmapObj;
        CBitmapObj* pBMNew = m_pBitmapObj ? m_pBitmapObj : pDoc->m_pBitmapObjNew;

        pDoc->SerializeBitmap( ar, pBMCur, pBMNew, TRUE );
        }
    }

/***************************************************************************/

BOOL CPBSrvrItem::OnGetExtent(DVASPECT dwDrawAspect, CSize& rSize)
    {
    // Most applications, like this one, only handle drawing the content
    //  aspect of the item.  If you wish to support other aspects, such
    //  as DVASPECT_THUMBNAIL (by overriding OnDrawEx), then this
    //  implementation of OnGetExtent should be modified to handle the
    //  additional aspect(s).

    if (dwDrawAspect != DVASPECT_CONTENT)
        return COleServerItem::OnGetExtent(dwDrawAspect, rSize);

    // CPBSrvrItem::OnGetExtent is called to get the extent in
    //  HIMETRIC units of the entire item.  The default implementation
    //  here simply returns a hard-coded number of units.

    CPBDoc* pDoc = GetDocument();

    ASSERT_VALID( pDoc );

    CBitmapObj* pBM = m_pBitmapObj ? m_pBitmapObj : pDoc->m_pBitmapObj;

    if (pBM         != NULL
    &&  pBM->m_pImg != NULL)
        {
       //
       // What was the padding for?
       //
        rSize.cx = pBM->m_pImg->cxWidth ;// + GetSystemMetrics( SM_CXBORDER ) + CTracker::HANDLE_SIZE * 2;
        rSize.cy = pBM->m_pImg->cyHeight;// + GetSystemMetrics( SM_CYBORDER ) + CTracker::HANDLE_SIZE * 2;

        CDC* pDC = CDC::FromHandle( pBM->m_pImg->hDC );

        if (pDC != NULL)
            {
            pDC->DPtoHIMETRIC( &rSize );
            }
        else /* punt */
            {
            rSize.cx = (int)(((long)rSize.cx * 10000L) / (long)theApp.ScreenDeviceInfo.ixPelsPerDM);
            rSize.cy = (int)(((long)rSize.cy * 10000L) / (long)theApp.ScreenDeviceInfo.iyPelsPerDM);
            }
        }
    else
        rSize = CSize( 3000, 3000 );

    return TRUE;
    }

/***************************************************************************/

BOOL CPBSrvrItem::OnSetExtent( DVASPECT nDrawAspect, const CSize& size )
    {
    TRACE( TEXT("MSPaint OnSetExtent %d %d\n"), size.cx, size.cy );

    return COleServerItem::OnSetExtent( nDrawAspect, size );
    }

/***************************************************************************/

void CPBSrvrItem::OnOpen()
    {
    CPBDoc* pDoc = (CPBDoc*)GetDocument();

        if (theApp.m_bLinked)
            {
        theApp.m_bHidden = FALSE;

        if (g_pDragBrushWnd != NULL)
            HideBrush();

        g_pDragBrushWnd = NULL;
        g_pMouseImgWnd  = NULL;
        fDraggingBrush  = FALSE;

        POSITION pos = pDoc->GetFirstViewPosition();
        CPBView* pView = (CPBView*)(pDoc->GetNextView( pos ));

        if (pView != NULL)
            {
            pView->SetTools();
            }
        }
    COleServerItem::OnOpen();
    }

/***************************************************************************/

void CPBSrvrItem::OnShow()
    {
    theApp.m_bHidden = FALSE;

    COleServerItem::OnShow();
    }

/***************************************************************************/

void CPBSrvrItem::OnHide()
    {
    theApp.m_bHidden = TRUE;

    g_pMouseImgWnd = NULL;

    if (g_pDragBrushWnd != NULL)
        HideBrush();

    COleServerItem::OnHide();
    }

/***************************************************************************/

BOOL CPBSrvrItem::OnDraw(CDC* pDC, CSize& rSize)
    {
    CPBDoc* pDoc = GetDocument();

    ASSERT_VALID(pDoc);

    CBitmapObj* pBM = m_pBitmapObj ? m_pBitmapObj : pDoc->m_pBitmapObj;

    if (pBM != NULL)
        {
        CDC* pDCBitmap = CDC::FromHandle( pBM->m_pImg->hDC );

        if (pDCBitmap != NULL)
            {
            CSize size( pBM->m_pImg->cxWidth,
                        pBM->m_pImg->cyHeight );

                size.cy = -size.cy;

            pDC->SetMapMode  ( MM_ANISOTROPIC );
            pDC->SetWindowExt( size );
            pDC->SetWindowOrg( 0, 0 );

            CPalette* ppalOld = NULL;

            if (pBM->m_pImg->m_pPalette != NULL)
                {
                ppalOld = pDC->SelectPalette( pBM->m_pImg->m_pPalette, FALSE ); // Background ??

                pDC->RealizePalette();
                }

            pDC->StretchBlt( 0, 0, size.cx, size.cy, pDCBitmap,
                             0, 0, pBM->m_pImg->cxWidth,
                                   pBM->m_pImg->cyHeight, SRCCOPY );

            if (pBM->m_pImg->m_pPalette != NULL)
                pDC->SelectPalette( ppalOld, FALSE ); // Background ??
            }
        }
    return TRUE;
    }

/***************************************************************************/

COleDataSource* CPBSrvrItem::OnGetClipboardData( BOOL bIncludeLink,
                                                 CPoint* pptOffset,
                                                 CSize *pSize )
    {
    ASSERT_VALID( this );

    COleDataSource* pDataSource = new COleDataSource;

    TRY
        {
        GetClipboardData( pDataSource, bIncludeLink, pptOffset, pSize );
        }
    CATCH_ALL( e )
        {
        delete pDataSource;

        THROW_LAST();
        }
    END_CATCH_ALL

    ASSERT_VALID( pDataSource );

    return pDataSource;
    }

/***************************************************************************/

BOOL CPBSrvrItem::OnRenderGlobalData( LPFORMATETC lpFormatEtc, HGLOBAL* phGlobal )
    {
        ASSERT( lpFormatEtc != NULL );

        BOOL bResult = FALSE;

    CPBDoc* pDoc = GetDocument();

    ASSERT_VALID( pDoc );

    CBitmapObj* pBM = m_pBitmapObj ? m_pBitmapObj : pDoc->m_pBitmapObj;

    if ((lpFormatEtc->cfFormat == CF_BITMAP
      || lpFormatEtc->cfFormat == CF_PALETTE)
    && pBM != NULL)
        {
        if (lpFormatEtc->cfFormat == CF_BITMAP)
            {
            }
        else // CF_PALETTE
            {
            }
        }
    else
        bResult = COleServerItem::OnRenderGlobalData( lpFormatEtc, phGlobal );

    return bResult;
    }

/***************************************************************************/

BOOL CPBSrvrItem::OnRenderFileData( LPFORMATETC lpFormatEtc, CFile* pFile )
    {
        ASSERT( lpFormatEtc != NULL );

        BOOL bResult = FALSE;

    CPBDoc* pDoc = GetDocument();

    ASSERT_VALID( pDoc );

    CBitmapObj* pBM = m_pBitmapObj ? m_pBitmapObj : pDoc->m_pBitmapObj;

    if (lpFormatEtc->cfFormat == CF_DIB && pBM != NULL)
        {
        TRY
            {
            // save as dib
            pBM->SaveResource( FALSE );
            pBM->WriteResource( pFile, CBitmapObj::rtDIB );
            bResult = TRUE;
            }
        CATCH( CFileException, ex )
            {
            theApp.SetFileError( IDS_ERROR_SAVE, ex->m_cause );
            return FALSE;
            }
        END_CATCH
        }
    else
        bResult = COleServerItem::OnRenderFileData( lpFormatEtc, pFile );

        return bResult;
    }

/***************************************************************************/
// CPBSrvrItem diagnostics

#ifdef _DEBUG
void CPBSrvrItem::AssertValid() const
    {
    COleServerItem::AssertValid();
    }

/***************************************************************************/

void CPBSrvrItem::Dump(CDumpContext& dc) const
    {
    COleServerItem::Dump(dc);
    }
#endif

/***************************************************************************/
