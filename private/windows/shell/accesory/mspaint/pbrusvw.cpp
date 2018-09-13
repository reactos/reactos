// pbrusvw.cpp : implementation of the CPBView class
//

#include "stdafx.h"

#include "global.h"
#include "pbrush.h"
#include "pbrusdoc.h"
#include "pbrusfrm.h"
#include "pbrusvw.h"
#include "ipframe.h"
#include "minifwnd.h"
#include "bmobject.h"
#include "imgsuprt.h"
#include "imgwnd.h"
#include "imgcolor.h"
#include "imgbrush.h"
#include "imageatt.h"
#include "undo.h"
#include "props.h"
#include "imgwell.h"
#include "imgtools.h"
#include "imgdlgs.h"
#include "toolbox.h"
#include "thumnail.h"
#include "t_text.h"
#include "cmpmsg.h"
#include "printres.h"
#include "settings.h"
#include "colorsrc.h"
#include "cderr.h"
#include "srvritem.h"
#include <afxprntx.h>
#include <dlgprnt2.cpp>

#if 0 // THIS_FILE is already declared in dlgprnt2.cpp

#ifdef _DEBUG
#undef THIS_FILE
static CHAR BASED_CODE THIS_FILE[] = __FILE__;
#endif

#endif 

IMPLEMENT_DYNCREATE(CPBView, CView)

#include "memtrace.h"


/***************************************************************************/
// CPBView

BEGIN_MESSAGE_MAP(CPBView, CView)
    //{{AFX_MSG_MAP(CPBView)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
    ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
    ON_COMMAND(ID_EDIT_CUT, OnEditCut)
    ON_COMMAND(ID_EDIT_CLEAR, OnEditClear)
    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
    ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
    ON_COMMAND(ID_VIEW_GRID, OnViewGrid)
    ON_COMMAND(ID_VIEW_ZOOM_100, OnViewZoom100)
    ON_COMMAND(ID_VIEW_ZOOM_400, OnViewZoom400)
    ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOM_100, OnUpdateViewZoom100)
    ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOM_400, OnUpdateViewZoom400)
    ON_UPDATE_COMMAND_UI(ID_VIEW_GRID, OnUpdateViewGrid)
    ON_COMMAND(ID_IMAGE_INVERT_COLORS, OnImageInvertColors)
    ON_UPDATE_COMMAND_UI(ID_IMAGE_INVERT_COLORS, OnUpdateImageInvertColors)
    ON_COMMAND(IDM_TGLOPAQUE, OnTglopaque)
    ON_UPDATE_COMMAND_UI(IDM_TGLOPAQUE, OnUpdateTglopaque)
    ON_COMMAND(ID_IMAGE_ATTRIBUTES, OnImageAttributes)
    ON_COMMAND(IDMX_SEL2BSH, OnSel2bsh)
    ON_COMMAND(IDMX_LARGERBRUSH, OnLargerbrush)
    ON_COMMAND(IDMX_SMALLERBRUSH, OnSmallerbrush)
    ON_COMMAND(ID_VIEW_ZOOM, OnViewZoom)
    ON_COMMAND(ID_IMAGE_FLIP_ROTATE, OnImageFlipRotate)
    ON_UPDATE_COMMAND_UI(ID_IMAGE_FLIP_ROTATE, OnUpdateImageFlipRotate)
    ON_COMMAND(IDM_EDITCOLORS, OnEditcolors)
    ON_UPDATE_COMMAND_UI(IDM_EDITCOLORS, OnUpdateEditcolors)
#if 0
    ON_COMMAND(IDM_LOADCOLORS, OnLoadcolors)
    ON_UPDATE_COMMAND_UI(IDM_LOADCOLORS, OnUpdateLoadcolors)
    ON_COMMAND(IDM_SAVECOLORS, OnSavecolors)
    ON_UPDATE_COMMAND_UI(IDM_SAVECOLORS, OnUpdateSavecolors)
#endif
    ON_COMMAND(ID_EDIT_SELECT_ALL, OnEditSelectAll)
    ON_COMMAND(ID_EDIT_PASTE_FROM, OnEditPasteFrom)
    ON_COMMAND(ID_EDIT_COPY_TO, OnEditCopyTo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY_TO, OnUpdateEditCopyTo)
    ON_COMMAND(ID_IMAGE_STRETCH_SKEW, OnImageStretchSkew)
    ON_UPDATE_COMMAND_UI(ID_IMAGE_STRETCH_SKEW, OnUpdateImageStretchSkew)
    ON_COMMAND(ID_VIEW_VIEW_PICTURE, OnViewViewPicture)
    ON_UPDATE_COMMAND_UI(ID_VIEW_VIEW_PICTURE, OnUpdateViewViewPicture)
    ON_COMMAND(ID_VIEW_TEXT_TOOLBAR, OnViewTextToolbar)
    ON_UPDATE_COMMAND_UI(ID_VIEW_TEXT_TOOLBAR, OnUpdateViewTextToolbar)
    ON_COMMAND(ID_FILE_SETASWALLPAPER_T, OnFileSetaswallpaperT)
    ON_UPDATE_COMMAND_UI(ID_FILE_SETASWALLPAPER_T, OnUpdateFileSetaswallpaperT)
    ON_COMMAND(ID_FILE_SETASWALLPAPER_C, OnFileSetaswallpaperC)
    ON_UPDATE_COMMAND_UI(ID_FILE_SETASWALLPAPER_C, OnUpdateFileSetaswallpaperC)
    ON_COMMAND(ID_VIEW_THUMBNAIL, OnViewThumbnail)
    ON_UPDATE_COMMAND_UI(ID_VIEW_THUMBNAIL, OnUpdateViewThumbnail)
    ON_UPDATE_COMMAND_UI(ID_IMAGE_ATTRIBUTES, OnUpdateImageAttributes)
    ON_COMMAND(ID_ESCAPE, OnEscape)
    ON_COMMAND(ID_ESCAPE_SERVER, OnEscapeServer)
    ON_WM_SHOWWINDOW()
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditSelection)
    ON_UPDATE_COMMAND_UI(ID_EDIT_CLEAR, OnUpdateEditClearSel)
        ON_COMMAND(ID_FILE_PAGE_SETUP, OnFilePageSetup)
        ON_COMMAND(ID_IMAGE_CLEAR_IMAGE, OnImageClearImage)
    ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditClearSel)
        ON_UPDATE_COMMAND_UI(ID_IMAGE_CLEAR_IMAGE, OnUpdateImageClearImage)
        //}}AFX_MSG_MAP

        ON_WM_DESTROY()



    // Standard printing commands
    ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_PREVIEW, OnFilePrintPreview)

END_MESSAGE_MAP()

/***************************************************************************/
// CPBView construction/destruction

CPBView::CPBView()
    {
    m_pImgWnd             = NULL;

    m_pwndThumbNailFloat  = NULL;
    m_pwndThumbNailView   = NULL;
    }

/***************************************************************************/

CPBView::~CPBView()
    {
    // reset the toolbar
    if (g_pImgToolWnd && g_pImgToolWnd->m_hWnd &&
        IsWindow(g_pImgToolWnd->m_hWnd) )
    {
        g_pImgToolWnd->SelectTool( IDMB_ARROW );
        g_pImgToolWnd->InvalidateOptions();
    }

    DestroyThumbNailView();

    if (m_pImgWnd)
        {
        delete m_pImgWnd;
        m_pImgWnd = NULL;
        }
    }

/***************************************************************************/

BOOL CPBView::PreCreateWindow( CREATESTRUCT& cs )
    {
    cs.style     |= WS_CLIPCHILDREN;
    cs.dwExStyle |= WS_EX_CLIENTEDGE;

    return CView::PreCreateWindow( cs );
    }


/***************************************************************************/

BOOL CPBView::PreTranslateMessage(MSG *pMsg)
        {
        // Handle a bug in MFC regarding enabling of accelerators on Popup menus.
        if ( pMsg->message == WM_KEYDOWN )
                {
                // Find the app menu for this window.
                CWnd    *pWnd = this;
                CMenu   *pMenu = NULL;
                while( pWnd )
                        {
                        if ( (pMenu = pWnd->GetMenu()) && IsMenu(pMenu->m_hMenu) )
                                break;
                        else
                                pMenu = NULL;
                        pWnd = pWnd->GetParent();
                        }

                if ( pMenu )
                        {
                        pMenu->EnableMenuItem( ID_VIEW_ZOOM_100, MF_BYCOMMAND |
                                (m_pImgWnd && m_pImgWnd->GetZoom() != 1 ? MF_ENABLED : MF_DISABLED) );
                        pMenu->EnableMenuItem( ID_VIEW_ZOOM_400, MF_BYCOMMAND |
                                (m_pImgWnd && m_pImgWnd->GetZoom() != 4 ? MF_ENABLED : MF_DISABLED) );
                        pMenu->EnableMenuItem( ID_VIEW_GRID, MF_BYCOMMAND |
                                (m_pImgWnd && m_pImgWnd->GetZoom() > 2 ? MF_ENABLED : MF_DISABLED) );
                        }
                }

        return CView::PreTranslateMessage(pMsg);
        }

/***************************************************************************/
// CPBView drawing

void CPBView::OnDraw( CDC* pDC )
    {
    if (m_pImgWnd)
        {
        CRect rectPaint;
        CPalette* ppal = m_pImgWnd->SetImgPalette( pDC, FALSE );

        // if the dc passed in is a CPaint DC use the rcPaint rect to optimize
        // painting to only paint the invalid area.  ELSE use the whole image
        // size.
        if (pDC->IsKindOf( RUNTIME_CLASS( CPaintDC ) )  )
            {
            rectPaint = ((CPaintDC *)pDC)->m_ps.rcPaint;
            if ( theApp.m_bEmbedded )
                                m_pImgWnd->Invalidate();
            }
        else
            {
            m_pImgWnd->GetImageRect( rectPaint );
            }

        m_pImgWnd->DrawImage( pDC, &rectPaint );

        if (ppal)
            pDC->SelectPalette( ppal, FALSE );
        }
    }

/***************************************************************************/
// CPBView printing

BOOL CPBView::GetPrintToInfo(CPrintInfo* pInfo)
{

        ASSERT(pInfo != NULL);
        ASSERT(pInfo->m_pPD != NULL);

        if (theApp.m_strPrinterName.IsEmpty())
                return FALSE;

        ASSERT(pInfo->m_pPD->m_pd.hDC == NULL);
        pInfo->m_pPD->m_pd.hDC = ::CreateDC(NULL,
                                            theApp.m_strPrinterName,
                                            NULL, NULL);

        // set up From and To page range from Min and Max
        pInfo->m_pPD->m_pd.nFromPage = (WORD)pInfo->GetMinPage();
        pInfo->m_pPD->m_pd.nToPage = (WORD)pInfo->GetMaxPage();

        ASSERT(pInfo->m_pPD != NULL);
        ASSERT(pInfo->m_pPD->m_pd.hDC != NULL);

        pInfo->m_nNumPreviewPages = theApp.m_nNumPreviewPages;
        VERIFY(pInfo->m_strPageDesc.LoadString(AFX_IDS_PREVIEWPAGEDESC));
        return TRUE;
}

BOOL CPBView::OnPreparePrinting( CPrintInfo* pInfo )
    {


    //
    // Create a C_PrintDialogEx structure to replace the PrintDialog in the
    // CPrintInfo
    //


    m_pdRestore= pInfo->m_pPD;
    m_pdexSub = new C_PrintDialogEx (FALSE, PD_RETURNDC | PD_ALLPAGES | PD_NOSELECTION|PD_USEDEVMODECOPIESANDCOLLATE);
    pInfo->m_pPD = m_pdexSub;

    // These next 2 lines copied from mfc42 source to initialize the printdialog
    //
    pInfo->SetMinPage (1);
    pInfo->SetMaxPage (0xffff);

    pInfo->m_pPD->m_pd.nFromPage = 1;
    pInfo->m_pPD->m_pd.nToPage = 1;

    new CPrintResObj( this, pInfo );

    if (pInfo->m_lpUserData == NULL)
        return FALSE;

    if (theApp.m_bPrintOnly)
        {
        if (GetPrintToInfo(pInfo))
        {
            return(TRUE);
        }

        if (! theApp.GetPrinterDeviceDefaults( &pInfo->m_pPD->m_pd ))
            {
            // bring up dialog to alert the user they need to install a printer.
            if (theApp.DoPrintDialog( pInfo->m_pPD ) != IDOK)
                return FALSE;
            }

        if (! pInfo->m_pPD->m_pd.hDC)
            {
            // call CreatePrinterDC if DC was not created by above
            if (! pInfo->m_pPD->CreatePrinterDC())
                return FALSE;
            }

        // set up From and To page range from Min and Max
        pInfo->m_pPD->m_pd.nFromPage = (WORD)pInfo->GetMinPage();
        pInfo->m_pPD->m_pd.nToPage   = (WORD)pInfo->GetMaxPage();
            pInfo->m_nNumPreviewPages    = theApp.m_nNumPreviewPages;
        return TRUE;
        }

    // default preparation
    if (! DoPreparePrinting( pInfo ))
        {
        ((CPrintResObj*)pInfo->m_lpUserData)->EndPrinting( NULL, pInfo );
        pInfo->m_lpUserData = NULL;
        return FALSE;
        }
    return TRUE;
    }

/***************************************************************************/

void CPBView::OnBeginPrinting( CDC* pDC, CPrintInfo* pInfo )
    {

    if (pInfo               != NULL
    &&  pInfo->m_lpUserData != NULL)
        ((CPrintResObj*)pInfo->m_lpUserData)->BeginPrinting( pDC, pInfo );
    else
        CView::OnBeginPrinting( pDC, pInfo );
    }

/***************************************************************************/

void CPBView::OnPrepareDC( CDC* pDC, CPrintInfo* pInfo )
    {

#ifdef USE_MIRRORING
    //
    // Disable RTL mirroring
    //
    if (PBGetLayout(pDC->GetSafeHdc()) & LAYOUT_RTL)
    {
        PBSetLayout(pDC->GetSafeHdc(), 0);
    }
#endif

    CView::OnPrepareDC( pDC, pInfo );

    if (pInfo               != NULL
    &&  pInfo->m_lpUserData != NULL)
        ((CPrintResObj*)pInfo->m_lpUserData)->PrepareDC( pDC, pInfo );
    }

/***************************************************************************/

void CPBView::OnPrint( CDC* pDC, CPrintInfo* pInfo )
    {

    BOOL bProcessed = FALSE;

    if (pInfo               != NULL
    &&  pInfo->m_lpUserData != NULL)
        bProcessed = ((CPrintResObj*)pInfo->m_lpUserData)->PrintPage( pDC, pInfo );

    if (! bProcessed)
        CView::OnPrint( pDC, pInfo );
    }

/***************************************************************************/

void CPBView::OnEndPrinting( CDC* pDC, CPrintInfo* pInfo )
    {

    if (pInfo == NULL)
        return;

    if (pInfo->m_lpUserData != NULL)
        {
        ((CPrintResObj*)pInfo->m_lpUserData)->EndPrinting( pDC, pInfo );
        pInfo->m_lpUserData = NULL;
        }
    //
    // Restore the original dialog pointer
    //
    pInfo->m_pPD = m_pdRestore;
    delete m_pdexSub;
    }

/******************************************************************************/

// CPBView diagnostics

#ifdef _DEBUG
void CPBView::AssertValid() const
    {
    CView::AssertValid();
    }

/***************************************************************************/

void CPBView::Dump( CDumpContext& dc ) const
    {
    CView::Dump( dc );
    }

/***************************************************************************/

CPBDoc* CPBView::GetDocument() // non-debug version is inline
    {
    ASSERT( m_pDocument->IsKindOf( RUNTIME_CLASS( CPBDoc ) ) );
    return (CPBDoc*)m_pDocument;
    }
#endif //_DEBUG

/***************************************************************************/
// CPBView message handlers
int CPBView::OnCreate( LPCREATESTRUCT lpCreateStruct )
    {
    if (CView::OnCreate( lpCreateStruct ) == -1)
        return -1;

#ifdef USE_MIRRORING
    //
    // Disable RTL mirroring on client drawing area. [samera]
    //
    if (lpCreateStruct->dwExStyle & WS_EX_LAYOUTRTL)
    {
        SetWindowLong( GetSafeHwnd(), GWL_EXSTYLE, lpCreateStruct->dwExStyle & ~WS_EX_LAYOUTRTL );
    }
#endif

    return 0;
    }

/***************************************************************************/

void CPBView::OnShowWindow( BOOL bShow, UINT nStatus )
    {
    if (theApp.m_bPrintOnly)
        return;

    CView::OnShowWindow( bShow, nStatus );
    }

/***************************************************************************/

void CPBView::OnDestroy()
    {
    // reset the toolbar
    if (g_pImgToolWnd && g_pImgToolWnd->m_hWnd &&
        IsWindow(g_pImgToolWnd->m_hWnd) )
    {
        g_pImgToolWnd->SelectTool( IDMB_ARROW );
        g_pImgToolWnd->InvalidateOptions();
    }

    DestroyThumbNailView();

    if (m_pImgWnd)
        {
        if ( ::IsWindow(m_pImgWnd->m_hWnd) )
            m_pImgWnd->DestroyWindow();

        delete m_pImgWnd;
        m_pImgWnd = NULL;
        }

    CView::OnDestroy();
    }

/***************************************************************************/

void CPBView::OnInitialUpdate( void )
    {
    CPBDoc* pDoc = GetDocument();

    if (SetObject())
        {
        if (theApp.m_bPrintOnly)
            {
            if (pDoc->m_bObjectLoaded)
                {
                OnFilePrint();

                GetParentFrame()->PostMessage( WM_CLOSE );
                return;
                }
            theApp.m_bPrintOnly = FALSE;
            }

        theUndo.SetMaxLevels( 3 );

        SetTools();
                }
    else
        {
        if (pDoc->m_pBitmapObjNew != NULL)
            {
            delete pDoc->m_pBitmapObjNew;
            pDoc->m_pBitmapObjNew = NULL;
            }

        TRACE( TEXT("OnInitialUpdate SetObject Failed!\n") );
        }
    }

/***************************************************************************/

void CPBView::OnActivateView( BOOL bActivate, CView* pActivateView,
                                              CView* pDeactiveView )
    {
    CView::OnActivateView( bActivate, pActivateView, pDeactiveView );
    }

/***************************************************************************/

BOOL CPBView::OnCmdMsg( UINT nID, int nCode, void* pExtra,
                       AFX_CMDHANDLERINFO* pHandlerInfo )
    {
    if (nCode == CN_COMMAND)
        {
        if (m_pImgWnd
        &&  m_pImgWnd->OnCmdMsg( nID, nCode, pExtra, pHandlerInfo ))
            return TRUE;
        }

    return CView::OnCmdMsg( nID, nCode, pExtra, pHandlerInfo );
    }

/***************************************************************************/


void CPBView::OnSize(UINT nType, int cx, int cy)
    {
    CView::OnSize( nType, cx, cy );

    // TODO: Add your message handler code here
    if (m_pImgWnd)
        m_pImgWnd->MoveWindow( 0, 0, cx, cy );
    }

/***************************************************************************/

BOOL CPBView::SetObject()
    {
    CPBDoc* pDoc = GetDocument();

    ASSERT( pDoc );
    ASSERT( pDoc->m_pBitmapObjNew );

    if (! pDoc || ! pDoc->m_pBitmapObjNew)
        return FALSE;

    CBitmapObj* pBitmapObj = pDoc->m_pBitmapObjNew;

    // see if a bad file was loaded, but not an empty file, which is OK
    if (! pDoc->m_bObjectLoaded
    &&  ! pBitmapObj->m_bTempName
    &&    pBitmapObj->GetData()
    &&    pBitmapObj->GetDataSize())
        {
        delete pBitmapObj;
                pBitmapObj = NULL;

        CString strDocName;
        CString strFilterExt;

        if (! pDoc->GetDocTemplate()->GetDocString( strDocName, CDocTemplate::docName )
        ||                                          strDocName.IsEmpty())
            // use generic 'untitled'
            VERIFY( strDocName.LoadString( AFX_IDS_UNTITLED ) );

        if (! pDoc->GetDocTemplate()->GetDocString( strFilterExt, CDocTemplate::filterExt )
        ||                                          strFilterExt.IsEmpty())
            pDoc->SetPathName( strDocName );
        else
            pDoc->SetPathName( strDocName + strFilterExt );

        // do settitle after setpathname.
        pDoc->SetTitle( strDocName );

        pDoc->m_sName.Empty();
        pDoc->m_bNewDoc = TRUE;

        if (! pDoc->CreateNewDocument())
            return FALSE;

        pBitmapObj = pDoc->m_pBitmapObjNew;
        }

    if (! pBitmapObj->m_pImg
    &&  ! pBitmapObj->CreateImg())
        return FALSE;

    if (pBitmapObj->m_pImg->cxWidth  < 1
    ||  pBitmapObj->m_pImg->cyHeight < 1)
        {
        CmpMessageBox( IDS_ERROR_BITMAPSIZE, AFX_IDS_APP_TITLE, MB_OK | MB_ICONEXCLAMATION );
        return FALSE;
        }

    CleanupImgUndo();
    CleanupImgRubber();

    if (! SetView( pBitmapObj ))
        return FALSE;

    if (pDoc->m_pBitmapObj)
                {
        delete pDoc->m_pBitmapObj;
                pDoc->m_pBitmapObj = NULL;
                }

    pDoc->m_pBitmapObj    = pBitmapObj;
    pDoc->m_pBitmapObjNew = NULL;

    return TRUE;
    }

/***************************************************************************/

BOOL CPBView::SetView( CBitmapObj* pBitmapObj )
    {
    IMG* pImg = pBitmapObj->m_pImg;

    ASSERT( pImg );

    CImgWnd* pImgWnd = new CImgWnd( pImg );

    if (pImgWnd == NULL)
        {
        theApp.SetMemoryEmergency();

        TRACE( TEXT("Create CImgWnd faild\n") );

        return FALSE;
        }

    RECT rectPos;

    GetClientRect( &rectPos );

    if (! pImgWnd->Create( WS_CHILD | WS_VISIBLE, rectPos, this ))
        {
        TRACE( TEXT("Create img wnd failed\n") );
        return FALSE;
        }

    if (m_pImgWnd)
                {
                if ( ::IsWindow( m_pImgWnd->m_hWnd) )
                        m_pImgWnd->DestroyWindow();
        delete m_pImgWnd;
                m_pImgWnd = NULL;
                }

    m_pImgWnd = pImgWnd;

    if (m_pwndThumbNailView != NULL)
        {
        m_pImgWnd->SetThumbnailView( m_pwndThumbNailView );
        m_pwndThumbNailView->UpdateThumbNailView();
        }

    m_pImgWnd->SetZoom( 1 );
    // m_pImgWnd->SetFocus(); // Commented out to prevent focus problems w/OLE

    return TRUE;
    }

/***************************************************************************/

int CPBView::SetTools()
    {
    CFrameWnd* pOwnerWindow    = GetParentFrame();
    CFrameWnd* pParentWindow   = pOwnerWindow;
    BOOL       bRestoreState   = FALSE;

    ASSERT( pOwnerWindow != NULL );

    if (! theApp.m_bLinked && theApp.m_pwndInPlaceFrame != NULL)
        {
        pOwnerWindow = theApp.m_pwndInPlaceFrame;

        if (theApp.m_hwndInPlaceApp != NULL)
            pParentWindow = (CFrameWnd*)CFrameWnd::FromHandle( theApp.m_hwndInPlaceApp );
        }


        ASSERT(g_pStatBarWnd);
        ASSERT(g_pImgToolWnd);
        ASSERT(g_pImgColorsWnd);

        // Create the status bar
        if ( !g_pStatBarWnd->m_hWnd )
                {
                if ( g_pStatBarWnd->Create(pParentWindow) )
                        {
                        if (theApp.m_fntStatus.m_hObject != NULL)
                                g_pStatBarWnd->SetFont( &theApp.m_fntStatus, FALSE );
                        g_pStatBarWnd->SetOwner(pOwnerWindow);
                        ShowStatusBar(TRUE);

                        bRestoreState = TRUE;
                        }
                else
                        {
                        TRACE0("Failed to create status bar\n");
                        return -1;
                        }
                }

        pParentWindow->EnableDocking(CBRS_ALIGN_ANY);

        // Create and dock the tool bar
        if ( !g_pImgToolWnd->m_hWnd || !IsWindow(g_pImgToolWnd->m_hWnd) )
                {
        CString strToolWnd;
        strToolWnd.LoadString(IDS_PAINT_TOOL);
                if ( g_pImgToolWnd->Create( strToolWnd,
                                            WS_CHILD|WS_VISIBLE|CBRS_LEFT,
                                            CRect(0, 0, 0, 0),
                                            CPoint(25, 25),
                                            2,
                                            pParentWindow ) )
                        {
                        g_pImgToolWnd->SetOwner(pOwnerWindow);
                        g_pImgToolWnd->EnableDocking(CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT);
                        pParentWindow->DockControlBar(g_pImgToolWnd,
                                                      AFX_IDW_DOCKBAR_LEFT);

                        bRestoreState = TRUE;
                        }
                else
                {
                        TRACE0("Failed to create toolbar\n");
                        return -1;
                        }
                }

        // Create and dock the color bar
        if ( !g_pImgColorsWnd->m_hWnd || !IsWindow(g_pImgColorsWnd->m_hWnd) )
                {
        CString strColorsWnd;
        strColorsWnd.LoadString(IDS_COLORS);
                if ( g_pImgColorsWnd->Create(strColorsWnd,
                                            WS_CHILD|WS_VISIBLE|CBRS_BOTTOM,
                                            pParentWindow) )
                        {
                        g_pImgColorsWnd->SetOwner(pOwnerWindow);
                        g_pImgColorsWnd->EnableDocking(CBRS_ALIGN_BOTTOM|CBRS_ALIGN_TOP);
                        pParentWindow->DockControlBar(g_pImgColorsWnd,
                                                     AFX_IDW_DOCKBAR_BOTTOM);

                        bRestoreState = TRUE;
                        }
                else
                        {
                        TRACE0("Failed to create colorbar\n");
                        return -1;
                        }
                }

    if ( bRestoreState && !theApp.m_bLinked && !theApp.m_bEmbedded && !theApp.m_pwndInPlaceFrame )
        pOwnerWindow->LoadBarState(TEXT("General")); // Dangerous in-place!

    pOwnerWindow->DelayRecalcLayout( TRUE );
        return 0;
    }

/******************************************************************************/
BOOL CPBView::DestroyThumbNailView()
        {
        BOOL    bResult = FALSE;
        BOOL    bOriginalSetting = theApp.m_bShowThumbnail;

    theApp.m_bShowThumbnail = FALSE;

    if (m_pwndThumbNailFloat != NULL)
        {
        if ( ::IsWindow(m_pwndThumbNailFloat->m_hWnd) )
                m_pwndThumbNailFloat->DestroyWindow();
                delete m_pwndThumbNailFloat;
                m_pwndThumbNailFloat = NULL;
                bResult = TRUE;
                }


    theApp.m_bShowThumbnail = bOriginalSetting;
    m_pwndThumbNailView = NULL;

        if (m_pImgWnd)
        m_pImgWnd->SetThumbnailView( NULL );

        return bResult;
        }

BOOL CPBView::CreateThumbNailView()
    {
    if (m_pImgWnd == NULL)
        return FALSE;

        DestroyThumbNailView();


        m_pwndThumbNailFloat = new CFloatThumbNailView( m_pImgWnd );

        if (m_pwndThumbNailFloat != NULL)
            {
            if (m_pwndThumbNailFloat->Create( this ))
                m_pwndThumbNailView = m_pwndThumbNailFloat->GetThumbNailView();

            if (m_pwndThumbNailView)
                {
                m_pImgWnd->SetThumbnailView( m_pwndThumbNailView );
                m_pwndThumbNailFloat->ShowWindow( SW_SHOWNOACTIVATE );
                }
            else
                                {
                                delete m_pwndThumbNailFloat;
                m_pwndThumbNailFloat = NULL;
                                }
            }


    if (m_pwndThumbNailView == NULL)
        {
        theApp.m_bShowThumbnail = FALSE;
        theApp.SetMemoryEmergency();

        TRACE( TEXT("Create CThumbNailView failed\n") );
        return FALSE;
        }

    m_pwndThumbNailView->ShowWindow( SW_SHOWNOACTIVATE );
    m_pwndThumbNailView->UpdateWindow();
    UpdateWindow();

    return TRUE;
    }

/***************************************************************************/

void CPBView::ToggleThumbNailVisibility( void )
    {
        theApp.m_bShowThumbnail = !IsThumbNailVisible();

        if ( theApp.m_bShowThumbnail )
                ShowThumbNailView();
        else if (m_pwndThumbNailView)
                HideThumbNailView();
    }

/***************************************************************************/

void CPBView::HideThumbNailView(void)
    {
    if (IsThumbNailVisible())
                {
            if (m_pwndThumbNailFloat)
                m_pwndThumbNailFloat->ShowWindow( SW_HIDE );


            theApp.m_bShowThumbnail = FALSE;
                }
    }

/***************************************************************************/

void CPBView::ShowThumbNailView(void)
    {
        if ( theApp.m_bShowThumbnail
          && !IsThumbNailVisible() )
                {
                if ( m_pwndThumbNailView )
                        {
                    if (m_pwndThumbNailFloat)
                        m_pwndThumbNailFloat->ShowWindow( SW_SHOWNOACTIVATE );

                        }
                else
                        CreateThumbNailView();
                }
    }

/***************************************************************************/

BOOL CPBView::IsThumbNailVisible(void)
    {
    BOOL bVisible = FALSE;


    if (m_pwndThumbNailFloat != NULL)
        bVisible = m_pwndThumbNailFloat->IsWindowVisible();

    return bVisible;
    }


/***************************************************************************/

CPoint CPBView::GetDockedPos( DOCKERS tool, CSize& sizeTool )
    {
    CPoint      pt;
    CRect       rectClient;
    CRect       rectView;
    CFrameWnd*  pFrame = GetParentFrame();

    pFrame->GetClientRect( &rectClient );
    pFrame->NegotiateBorderSpace( CFrameWnd::borderGet, &rectView );

    switch (tool)
        {
        case toolbox:
                        ASSERT(0);
            break;

        case colorbox:
                        ASSERT(0);
            break;

        }
    pFrame->ClientToScreen( &pt );
    return pt;
    }

/***************************************************************************/

void CPBView::GetFloatPos( DOCKERS tool, CRect& rectPos )
    {
       // removed docked thumbnail code
    }

/***************************************************************************/

void CPBView::SetFloatPos( DOCKERS tool, CRect& rectPos )
    {
       //removed docked thumbnail code
     }

/***************************************************************************/

void CPBView::OnViewThumbnail()
    {
    ToggleThumbNailVisibility();
    }

/***************************************************************************/

void CPBView::OnUpdateViewThumbnail(CCmdUI* pCmdUI)
    {
    BOOL bEnable = FALSE;

    if (m_pImgWnd != NULL
    &&  m_pImgWnd->GetZoom() > 1)
        {
        bEnable = TRUE;
        }

    pCmdUI->Enable( bEnable );
    pCmdUI->SetCheck( theApp.m_bShowThumbnail );
    }

/***************************************************************************/

void CPBView::OnEditUndo()
    {
    if (!TextToolProcessed( ID_EDIT_UNDO ))
        {
        CancelToolMode(FALSE);

        CommitSelection(TRUE);

        theUndo.DoUndo();
        DirtyImg (m_pImgWnd->m_pImg);
        }
    }

/***************************************************************************/

void CPBView::OnEditRedo()
    {
    CancelToolMode(FALSE);

    theUndo.DoRedo();
    DirtyImg (m_pImgWnd->m_pImg);
    }

/***************************************************************************/

void CPBView::OnEditCut()
    {
    m_pImgWnd->CmdCut();
    }

/***************************************************************************/

void CPBView::OnEditClear()
    {
    m_pImgWnd->CmdClear();
    }


/***************************************************************************/

void CPBView::OnEditCopy()
{
        m_pImgWnd->CmdCopy();
}

/***************************************************************************/

void CPBView::OnEditPaste()
    {
    m_pImgWnd->CmdPaste();
    }

/***************************************************************************/


void CPBView::OnUpdateEditUndo(CCmdUI* pCmdUI)
    {
    // the text tool has no idea if it can undo and neither do we
    pCmdUI->Enable(IsUserEditingText() || theUndo.CanUndo());
    }

/***************************************************************************/

void CPBView::OnUpdateEditRedo(CCmdUI* pCmdUI)
    {
    // the text tool does not have a redo stack
    pCmdUI->Enable(!IsUserEditingText() && theUndo.CanRedo());
    }

/***************************************************************************/

void CPBView::OnUpdateEditSelection(CCmdUI* pCmdUI)
    {
    BOOL bEnable = FALSE;

    if (m_pImgWnd != NULL)
        {
        bEnable = m_pImgWnd->IsSelectionAvailable();
        }

    pCmdUI->Enable( bEnable );
    }

/***************************************************************************/

void CPBView::OnUpdateEditClearSel(CCmdUI* pCmdUI)
    {
    BOOL bEnable = FALSE;

    if (m_pImgWnd != NULL)
        bEnable = m_pImgWnd->IsSelectionAvailable();

    pCmdUI->Enable( bEnable );
    }

/***************************************************************************/

void CPBView::OnUpdateEditPaste(CCmdUI* pCmdUI)
    {
    BOOL bEnable = FALSE;

    if (m_pImgWnd != NULL)
        {
        bEnable = m_pImgWnd->IsPasteAvailable();
        }

    pCmdUI->Enable( bEnable );
    }

/***************************************************************************/

void CPBView::OnViewGrid()
    {
    m_pImgWnd->CmdShowGrid();
    }

/***************************************************************************/

void CPBView::OnViewZoom100()
    {
    if (m_pImgWnd->GetZoom() != 1)
                {
                m_pImgWnd->SetZoom        ( 1 );
            m_pImgWnd->CheckScrollBars();
                }
    }

/***************************************************************************/

void CPBView::OnViewZoom400()
    {
    if (m_pImgWnd->GetZoom() != 4)
                {
            m_pImgWnd->SetZoom        ( 4 );
            m_pImgWnd->CheckScrollBars();
                }
    }

/***************************************************************************/

void CPBView::OnViewZoom()
    {
    CZoomViewDlg dlg;

    dlg.m_nCurrent = m_pImgWnd->GetZoom();

    if (dlg.DoModal() != IDOK)
        return;

    m_pImgWnd->SetZoom( dlg.m_nCurrent );
    m_pImgWnd->CheckScrollBars();
    }

/***************************************************************************/

void CPBView::OnUpdateViewZoom100(CCmdUI* pCmdUI)
    {
    BOOL bEnable = FALSE;

    if (m_pImgWnd)
        bEnable = (m_pImgWnd->GetZoom() != 1);

    pCmdUI->Enable( bEnable );
    }

/***************************************************************************/

void CPBView::OnUpdateViewZoom400(CCmdUI* pCmdUI)
    {
    BOOL bEnable = FALSE;

    if (m_pImgWnd)
        bEnable = (m_pImgWnd->GetZoom() != 4);

    pCmdUI->Enable( bEnable );
    }

/***************************************************************************/

void CPBView::OnUpdateViewGrid(CCmdUI* pCmdUI)
    {
    BOOL bCheck  = FALSE;
    BOOL bEnable = FALSE;

    if (m_pImgWnd)
        {
        bEnable = (m_pImgWnd->GetZoom() > 2);
        bCheck  =  m_pImgWnd->IsGridVisible();
        }
    pCmdUI->Enable  ( bEnable );
    pCmdUI->SetCheck( bCheck  );
    }

/***************************************************************************/

void CPBView::OnImageInvertColors()
    {
    CancelToolMode(TRUE);

    m_pImgWnd->CmdInvertColors();
    }

/***************************************************************************/
// Don't show the Invert Colors menu item if we're using a palette
void CPBView::OnUpdateImageInvertColors(CCmdUI* pCmdUI)
    {

    BOOL bEnable = FALSE;

    if (m_pImgWnd)
       {
       bEnable = (!theApp.m_bPaletted);
       }
   pCmdUI->Enable  ( bEnable );
   }

/***************************************************************************/

void CPBView::OnTglopaque()
    {
    m_pImgWnd->CmdTglOpaque();
    }

/***************************************************************************/

void CPBView::OnUpdateTglopaque(CCmdUI* pCmdUI)
    {
    pCmdUI->Enable( m_pImgWnd != NULL );
    pCmdUI->SetCheck( theImgBrush.m_bOpaque );
    }

/***************************************************************************/

void CPBView::OnImageAttributes()
    {
    CancelToolMode(FALSE);

    CPBDoc* pDoc = GetDocument();

    ASSERT( pDoc );
    ASSERT( m_pImgWnd );

    CBitmapObj* pBitmapRes = pDoc->m_pBitmapObj;

    CImageAttr dlg;

    BOOL bMono = (pBitmapRes->m_pImg->cPlanes   == 1
               && pBitmapRes->m_pImg->cBitCount == 1);

    dlg.m_bMonochrome = bMono;
    dlg.SetWidthHeight( pBitmapRes->m_pImg->cxWidth,
                        pBitmapRes->m_pImg->cyHeight );

    if (dlg.DoModal() != IDOK)
        return;

    CSize size = dlg.GetWidthHeight();

    if (size.cx != pBitmapRes->m_pImg->cxWidth
    ||  size.cy != pBitmapRes->m_pImg->cyHeight)
        {
        theUndo.BeginUndo( TEXT("Property Edit") );

        pBitmapRes->SetSizeProp( P_Size, size );

        theUndo.EndUndo();

        theApp.m_sizeBitmap = size;
        }

    if (dlg.m_bMonochrome != bMono
        && (!dlg.m_bMonochrome
            || AfxMessageBox(IDS_WARNING_MONO, MB_YESNO|MB_ICONEXCLAMATION)==IDYES))
        {
        theUndo.BeginUndo( TEXT("Property Edit") );

        pBitmapRes->SetIntProp( P_Colors, dlg.m_bMonochrome );

        theUndo.EndUndo();
        }
    }

/***************************************************************************/

void CPBView::OnImageClearImage()
    {
    CancelToolMode(FALSE);

    m_pImgWnd->CmdClear();
    }

/***************************************************************************/

void CPBView::OnFilePrint()
    {
    CancelToolMode(FALSE);

    CView::OnFilePrint();

    }

/***************************************************************************/

void CPBView::OnFilePrintPreview()
    {
    CancelToolMode(FALSE);

    CView::OnFilePrintPreview();
    }

/***************************************************************************/

void CPBView::OnUpdateImageClearImage( CCmdUI* pCmdUI )
    {
    BOOL bEnable = FALSE;

    if (m_pImgWnd)
        bEnable = (CImgTool::GetCurrentID() != IDMX_TEXTTOOL
                      && ! m_pImgWnd->IsSelectionAvailable() );
    pCmdUI->Enable( bEnable );
    }

/***************************************************************************/

void CPBView::OnSel2bsh()
    {
    m_pImgWnd->CmdSel2Bsh();
    }

/***************************************************************************/

void CPBView::OnLargerbrush()
    {
    m_pImgWnd->CmdLargerBrush();
    }

/***************************************************************************/

void CPBView::OnSmallerbrush()
    {
    m_pImgWnd->CmdSmallerBrush();
    }

/***************************************************************************/

void CPBView::OnImageFlipRotate()
    {
    CancelToolMode(TRUE);

    CFlipRotateDlg dlg;

    if (dlg.DoModal() != IDOK)
        return;

    if (dlg.m_bAngle)
        {
        switch (dlg.m_nAngle)
            {
            case 90:
                m_pImgWnd->CmdRot90();
                break;

            case 180:
                theUndo.BeginUndo( TEXT("Rotate 180") );

                m_pImgWnd->CmdFlipBshH();
                m_pImgWnd->CmdFlipBshV();

                theUndo.EndUndo();
                break;

            case 270:
                theUndo.BeginUndo( TEXT("Rotate 270") );

                m_pImgWnd->CmdRot90();
                m_pImgWnd->CmdFlipBshH();
                m_pImgWnd->CmdFlipBshV();

                theUndo.EndUndo();
                break;
            }
        }
    else
        if (dlg.m_bHorz)
            m_pImgWnd->CmdFlipBshH();
        else
            m_pImgWnd->CmdFlipBshV();
    }

/***************************************************************************/

void CPBView::OnUpdateImageFlipRotate(CCmdUI* pCmdUI)
    {
    pCmdUI->Enable( m_pImgWnd != NULL );
    }

/***************************************************************************/

void CPBView::OnEditcolors()
    {
    g_pColors->CmdEditColor();
    }

/***************************************************************************/

void CPBView::OnUpdateEditcolors(CCmdUI* pCmdUI)
    {
    pCmdUI->Enable( g_pColors != NULL );
    }

/***************************************************************************/
#if 0
void CPBView::OnLoadcolors()
    {
    CancelToolMode(FALSE);

    g_pColors->CmdLoadColors();
    }

/***************************************************************************/

void CPBView::OnUpdateLoadcolors(CCmdUI* pCmdUI)
    {
    BOOL bEnable = FALSE;

    if (g_pColors && m_pImgWnd && m_pImgWnd->m_pImg &&
        m_pImgWnd->m_pImg->m_pBitmapObj)
        {
        // not allowed except on 24 bit images
        bEnable = ( m_pImgWnd->m_pImg->m_pBitmapObj->m_nColors == 3 );
        }

    pCmdUI->Enable( bEnable );
    }

/***************************************************************************/

void CPBView::OnSavecolors()
    {
    g_pColors->CmdSaveColors();
    }

/***************************************************************************/

void CPBView::OnUpdateSavecolors(CCmdUI* pCmdUI)
    {
    pCmdUI->Enable( g_pColors != NULL );
    }

/***************************************************************************/
#endif
void CPBView::OnEditSelectAll()
    {
    if (m_pImgWnd)
        {
        if (!TextToolProcessed( ID_EDIT_SELECT_ALL ))
            {
            IMG *img = m_pImgWnd? m_pImgWnd->GetImg() : NULL;

            if (img)
                {
                CImgTool::Select(IDMB_PICKTOOL);
                m_pImgWnd->MakeBrush( img->hDC,
                    CRect( 0, 0, img->cxWidth, img->cyHeight ) );
                }
            }
        }
    }

/***************************************************************************/

void CPBView::OnEditPasteFrom()
    {
    CBitmapObj *pResObject = new CBitmapObj();

    ASSERT(pResObject != NULL);

    if (pResObject != NULL)
        {
        ASSERT( m_pImgWnd != NULL );

        pResObject->MakeEmpty();

        CString newName;
        int iColor = 0;

        if (theApp.DoPromptFileName( newName, IDS_EDIT_PASTE_FROM,
                                     OFN_PATHMUSTEXIST, TRUE, iColor, FALSE ))
            {
            if (pResObject->Import( newName ))
                m_pImgWnd->PasteImageFile( (LPSTR)(pResObject->GetData()) );
            }

        pResObject->m_pImg = NULL;
        delete pResObject;
                pResObject = NULL;
        }
    }


/***************************************************************************/

BOOL FillBitmapObj(CImgWnd* pImgWnd, CBitmapObj* pResObject, IMG* pImgStruct,
        int iColor)
{
        ASSERT(pImgWnd != NULL);

        pResObject->MakeEmpty();

        if (pImgWnd->m_pImg                           == NULL
                ||  pImgWnd->m_pImg->m_pBitmapObj         == NULL
                ||  pImgWnd->m_pImg->m_pBitmapObj->m_pImg == NULL)
        {
                return(FALSE);
        }

        if (iColor < 0)
        {
                iColor = pImgWnd->m_pImg->m_pBitmapObj->m_nColors;
        }

        if (theImgBrush.m_bFirstDrag)
        {
                PickupSelection();
        }

        *pImgStruct = *theImgBrush.m_pImg;

        pImgStruct->hDC        = theImgBrush.m_dc.GetSafeHdc();
        pImgStruct->hBitmap    = (HBITMAP)theImgBrush.m_bitmap.GetSafeHandle();
        pImgStruct->hBitmapOld = theImgBrush.m_hbmOld;
        pImgStruct->bDirty     = TRUE;
        pImgStruct->m_pPalette = theApp.m_pPalette;
        pImgStruct->cxWidth    = theImgBrush.m_size.cx;
        pImgStruct->cyHeight   = theImgBrush.m_size.cy;

        if (iColor < 4 && iColor >= 0)
        {
                pResObject->m_nSaveColors = iColor;
        }

#ifdef PCX_SUPPORT
        pResObject->m_bPCX        = (iColor == 4);
#endif
#ifdef ICO_SUPPORT
        pResObject->m_bSaveIcon   = (iColor == 5);
#endif
        pResObject->m_nWidth      = pImgStruct->cxWidth;
        pResObject->m_nHeight     = pImgStruct->cyHeight;
        pResObject->m_nColors     = pImgWnd->m_pImg->m_pBitmapObj->m_nColors;
        pResObject->m_bCompressed = pImgWnd->m_pImg->m_pBitmapObj->m_bCompressed;

        pResObject->m_pImg = pImgStruct;

        return(TRUE);
}

void CPBView::OnEditCopyTo()
{
        CString newName;
        int iColor = m_pImgWnd->m_pImg->m_pBitmapObj->m_nColors;

        if (theApp.DoPromptFileName( newName, IDS_EDIT_COPY_TO,
                OFN_HIDEREADONLY | OFN_PATHMUSTEXIST, FALSE, iColor, TRUE ))
        {
                BeginWaitCursor();

                CBitmapObj cResObject;

                IMG imgStruct;

                if (!FillBitmapObj(m_pImgWnd, &cResObject, &imgStruct, iColor))
                {
                        // BUGBUG: Need an error message
                        // Actually, can this ever happen?
                        return;
                }

                cResObject.SaveResource( FALSE );
                cResObject.Export( newName );

                EndWaitCursor();

                // Don't delete this on destructor
                cResObject.m_pImg = NULL;
        }
}

/***************************************************************************/

void CPBView::OnUpdateEditCopyTo(CCmdUI* pCmdUI)
    {
    BOOL bEnable = FALSE;

    if (CImgTool::GetCurrentID() == IDMB_PICKTOOL
    ||  CImgTool::GetCurrentID() == IDMB_PICKRGNTOOL)
        {
        ASSERT( m_pImgWnd != NULL );

        if (m_pImgWnd != NULL)
            {
            if (m_pImgWnd->m_pImg != NULL)
                {
                if (m_pImgWnd->m_pImg == theImgBrush.m_pImg)
                    {
                    bEnable = TRUE;
                    }
                }
            }
        }

    pCmdUI->Enable( bEnable );
    }

/***************************************************************************/

void CPBView::OnImageStretchSkew()
    {
    CancelToolMode(TRUE);

    CStretchSkewDlg dlg;

    if (dlg.DoModal() != IDOK)
        return;

    int iStretchHorz = dlg.GetStretchHorz();
    int iStretchVert = dlg.GetStretchVert();

    if (iStretchVert
    ||  iStretchHorz)
        {
        CPBDoc* pDoc = GetDocument();

        ASSERT( pDoc );

        int iWidthImg    = pDoc->m_pBitmapObj->m_pImg->cxWidth;
        int iHeightImg   = pDoc->m_pBitmapObj->m_pImg->cyHeight;

        if (theImgBrush.m_pImg == NULL)
            {
            int lX = iWidthImg  + (iWidthImg  * iStretchHorz) / 100;
            int lY = iHeightImg + (iHeightImg * iStretchVert) / 100;

            CBitmap bmWork;
            CDC     dcWork;
            CDC*    pdcImg = CDC::FromHandle( pDoc->m_pBitmapObj->m_pImg->hDC );
            CSize   sizeBMP( (UINT)lX, (UINT)lY );
            CRect   rect( 0, 0, lX, lY );

            if (! dcWork.CreateCompatibleDC    ( pdcImg )
            ||  ! bmWork.CreateCompatibleBitmap( pdcImg, iWidthImg, iHeightImg ))
                {
                theApp.SetGdiEmergency( TRUE );
                return;
                }

            CBitmap*   pbmOld = dcWork.SelectObject( &bmWork );
            CPalette* ppalOld = m_pImgWnd->SetImgPalette( &dcWork, FALSE );

            BeginWaitCursor();

            dcWork.BitBlt( 0, 0, iWidthImg, iHeightImg, pdcImg, 0, 0, SRCCOPY );

            theUndo.BeginUndo( TEXT("Property Edit") );

            pDoc->m_pBitmapObj->SetSizeProp( P_Size, sizeBMP );

            StretchCopy( pdcImg->m_hDC, 0, 0, lX, lY,
                          dcWork.m_hDC, 0, 0, iWidthImg, iHeightImg );

            InvalImgRect ( m_pImgWnd->m_pImg, NULL );
            CommitImgRect( m_pImgWnd->m_pImg, NULL );
            theUndo.EndUndo();
            DirtyImg(m_pImgWnd->m_pImg);

            dcWork.SelectObject( pbmOld );
            bmWork.DeleteObject();

            if (ppalOld)
                dcWork.SelectPalette( ppalOld, FALSE );

            dcWork.DeleteDC();

            theApp.m_sizeBitmap = sizeBMP;

            EndWaitCursor();
            }
        else
            {
            CRect rect = theImgBrush.m_rcSelection;
            long  lX   = theImgBrush.m_size.cx;
            long  lY   = theImgBrush.m_size.cy;

            lX += (lX * iStretchHorz) / 100;
            lY += (lY * iStretchVert) / 100;

            rect.right  = rect.left + (UINT)lX;
            rect.bottom = rect.top  + (UINT)lY;

            // If the image is a bitmap and the bitmap in the clipboard is larger,
            // then give the suer the option2 of growing the image...
//          if (lX > iWidthImg || lY > iHeightImg)
//              {
//              switch (AfxMessageBox( IDS_ENLAGEBITMAPFORSTRETCH,
//                                     MB_YESNOCANCEL | MB_ICONQUESTION ))
//                  {
//                  default:
//                      return;
//                      break;

//                  case IDYES:
//                      {
//                      CSize size( max( lX, iWidthImg  ),
//                                  max( lY, iHeightImg ) );

//                      theUndo.BeginUndo( "Resize Bitmap" );
//                      VERIFY( pDoc->m_pBitmapObj->SetSizeProp( P_Size, size ) );

//                      theUndo.EndUndo();
//                      }
//                      break;

//                  case IDNO:
//                      break;
//                  }
//              }

            m_pImgWnd->PrepareForBrushChange();

            HideBrush();

            theImgBrush.SetSize( CSize( (UINT)lX, (UINT)lY ) );

            SetCombineMode( combineColor );

            InvalImgRect( theImgBrush.m_pImg, NULL ); // draw selection tracker
            m_pImgWnd->MoveBrush( rect );
            }
        }

    int wSkewHorz = (int)dlg.GetSkewHorz();
    int wSkewVert = (int)dlg.GetSkewVert();

    if (wSkewHorz)
        m_pImgWnd->CmdSkewBrush( wSkewHorz, TRUE );

    if (wSkewVert)
        m_pImgWnd->CmdSkewBrush( wSkewVert, FALSE );
    }

/***************************************************************************/

void CPBView::OnUpdateImageStretchSkew(CCmdUI* pCmdUI)
    {
    pCmdUI->Enable( m_pImgWnd != NULL );
    }

/***************************************************************************/

void CPBView::OnViewViewPicture()
    {
    CPBDoc *pDoc;
    CString strCaption;

    ASSERT(! theApp.m_bEmbedded);
    pDoc = GetDocument();

    CFullScreenThumbNailView *pcThumbNailView = new CFullScreenThumbNailView( m_pImgWnd );

    if (  pcThumbNailView == NULL
    ||  ! pcThumbNailView->Create((LPCTSTR)pDoc->GetPathName()))
        {
        theApp.SetMemoryEmergency();

        TRACE( TEXT("Create CThumbNailView faild\n") );
        }
    }

/***************************************************************************/

void CPBView::OnUpdateViewViewPicture(CCmdUI* pCmdUI)
    {
    pCmdUI->Enable( m_pImgWnd != NULL );
    }

/***************************************************************************/

void CPBView::OnViewTextToolbar()
    {
    ASSERT( CImgTool::GetCurrentID() == IDMX_TEXTTOOL );

    CTextTool* pTextTool = (CTextTool*)CImgTool::GetCurrent();

    ASSERT( pTextTool->IsKindOf( RUNTIME_CLASS( CTextTool ) ) );

    pTextTool->ToggleFontPalette();
    }

/***************************************************************************/

void CPBView::OnUpdateViewTextToolbar(CCmdUI* pCmdUI)
    {
    BOOL bEnable = FALSE;

    if (CImgTool::GetCurrentID() == IDMX_TEXTTOOL)
        {
        CTextTool* pTextTool = (CTextTool*)CImgTool::GetCurrent();

        ASSERT( pTextTool );
        ASSERT( pTextTool->IsKindOf( RUNTIME_CLASS( CTextTool ) ) );

        if (pTextTool
        &&  pTextTool->IsSlectionVisible())
            bEnable = TRUE;
        }

    pCmdUI->SetCheck( theApp.m_bShowTextToolbar );
    pCmdUI->Enable  ( bEnable );
    }

/***************************************************************************/

void CPBView::OnFileSetaswallpaperT()
    {
    SetTheWallpaper( TRUE );
    }

/***************************************************************************/

void CPBView::OnUpdateFileSetaswallpaperT( CCmdUI* pCmdUI )
    {
    pCmdUI->Enable( CanSetWallpaper() );
    }

/***************************************************************************/

void CPBView::OnFileSetaswallpaperC()
    {
    SetTheWallpaper( FALSE );
    }

/***************************************************************************/

void CPBView::OnUpdateFileSetaswallpaperC(CCmdUI* pCmdUI)
    {
    pCmdUI->Enable( CanSetWallpaper() );
    }

/***************************************************************************/

BOOL CPBView::CanSetWallpaper()
    {
    BOOL bEnable = FALSE;

    if (m_pImgWnd != NULL)
        {
        CPBDoc* pDoc = GetDocument();

        ASSERT( pDoc );

        bEnable = (! pDoc->GetPathName().IsEmpty() || pDoc->IsModified());
        }
    return bEnable;
    }

/***************************************************************************/

void CPBView::SetTheWallpaper( BOOL bTiled /* = FALSE */ )
    {
    CPBDoc* pDoc = GetDocument();
    ASSERT( pDoc != NULL );

    CString cStrFileName = pDoc->GetPathName();

    BOOL bSetWallpaper = ! (cStrFileName.IsEmpty() || pDoc->IsModified() || pDoc->m_bNonBitmapFile);

    if (! bSetWallpaper)
        switch (AfxMessageBox( IDS_MUST_SAVE_WALLPAPER, MB_OKCANCEL | MB_ICONEXCLAMATION ))
            {
            case IDOK:
                // If so, either Save or Update, as appropriate
                bSetWallpaper = pDoc->SaveTheDocument();
                cStrFileName = pDoc->GetPathName();
                break;

            case IDCANCEL:
                break;

            default:
                theApp.SetMemoryEmergency();
                break;
            }

    if (bSetWallpaper)
        {
        DWORD dwDisp;
        HKEY  hKey = 0;

        if (RegCreateKeyEx( HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"),
                            REG_OPTION_RESERVED, TEXT(""),
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS, NULL, &hKey, &dwDisp ) == ERROR_SUCCESS)
            {
            RegSetValueEx( hKey, TEXT("TileWallpaper"), 0, REG_SZ,
                            (BYTE *)(bTiled? TEXT("1"): TEXT("0")), 2*sizeof(TCHAR) );
            RegCloseKey( hKey );
            }

        SystemParametersInfo( SPI_SETDESKWALLPAPER, bTiled? 1: 0,
                         (LPVOID)(cStrFileName.GetBuffer( cStrFileName.GetLength() )),
                         SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE );

        cStrFileName.ReleaseBuffer();
        }
    }

/***************************************************************************/

void CPBView::OnPaletteChanged(CWnd* pFocusWnd)
    {
    // If this application did not change the palette, select
    // and realize this application's palette
    if ((pFocusWnd         != m_pImgWnd)
    &&  (m_pImgWnd         != NULL)
    &&  (m_pImgWnd->m_pImg != NULL))
        {
        if (theApp.m_pPalette)
            {
            // Redraw the entire client area
            m_pImgWnd->InvalidateRect(NULL);
            m_pImgWnd->UpdateWindow();

                if (g_pImgColorsWnd && g_pImgColorsWnd->m_hWnd && IsWindow(g_pImgColorsWnd->m_hWnd))
                {
                InvalColorWnd();
                // g_pImgColorsWnd->UpdateWindow();
                }
            }
        }
    }

/***************************************************************************/

BOOL CPBView::OnQueryNewPalette()
    {
    HPALETTE hOldPal = NULL;

    if (m_pImgWnd && ::IsWindow(m_pImgWnd->m_hWnd)
    &&  m_pImgWnd->m_pImg)
        {
        if (theApp.m_pPalette)
            {
            // Redraw the entire client area
            m_pImgWnd->InvalidateRect(NULL);
            m_pImgWnd->UpdateWindow();

                if (g_pImgColorsWnd && g_pImgColorsWnd->m_hWnd && IsWindow(g_pImgColorsWnd->m_hWnd))
                {
                InvalColorWnd();
                g_pImgColorsWnd->UpdateWindow();
                }
            }
        }
    return TRUE;
    }

/***************************************************************************/

void CPBView::OnUpdateImageAttributes( CCmdUI* pCmdUI )
    {
    BOOL bEnable = (m_pImgWnd != NULL);

    pCmdUI->Enable( bEnable );
    }

/***************************************************************************/

void CPBView::OnEscape()
{
        if (m_pImgWnd != NULL)
        {
                m_pImgWnd->CmdCancel();
        }

        OnCancelMode();
}

/***************************************************************************/

void CPBView::OnEscapeServer()
{
        CImgTool* pImgTool = CImgTool::GetCurrent();

        if (pImgTool->IsToolModal())
        {
                OnEscape();
                return;
        }
        else
        {
                // Tell the OLE client (if there is one) we are all done
                GetDocument()->OnDeactivateUI(FALSE);
        }
}

/***************************************************************************/
