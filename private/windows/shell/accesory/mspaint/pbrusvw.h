// pbrusvw.h : interface of the CPBView class
//



class CPBDoc;
class CImgWnd;

class CThumbNailView;
class CFloatThumbNailView;


class CBitmapObj;
class C_PrintDialogEx;
/***************************************************************************/

class CPBView : public CView
    {
    protected: // create from serialization only

    DECLARE_DYNCREATE( CPBView )

    CPBView();

    public: /* Attributes ***********************************************/

    enum DOCKERS
        {
        unknown,
        toolbox,
        colorbox,

        };

    CImgWnd*             m_pImgWnd;
    CThumbNailView*      m_pwndThumbNailView;
    CFloatThumbNailView* m_pwndThumbNailFloat;


    public: /* Operations ***********************************************/

    CPBDoc* GetDocument();

   void   OnPaletteChanged(CWnd* pFocusWnd);
    BOOL   OnQueryNewPalette();

    BOOL   SetObject();

    int     SetTools();
    CPoint GetDockedPos     ( DOCKERS tool, CSize& sizeTool );

    void   GetFloatPos      ( DOCKERS tool, CRect& rectPos );
    void   SetFloatPos      ( DOCKERS tool, CRect& rectPos );

    void   ShowThumbNailView( void );
    void   HideThumbNailView( void );



    private: /***************************************************************/
    C_PrintDialogEx *m_pdexSub; // substitute in for CPrintDialog
    CPrintDialog    *m_pdRestore; // dialog pointer to restore after printing

    BOOL    SetView( CBitmapObj* pBitmapObj );

    void    ToggleThumbNailVisibility( void );
    BOOL    IsThumbNailVisible       ( void );
    BOOL    CreateThumbNailView();
    BOOL    DestroyThumbNailView();

    BOOL    InitPageStruct( LPPAGESETUPDLGA );
    static  UINT APIENTRY PaintHookProc( HWND, UINT, WPARAM, LPARAM );
    BOOL    GetPrintToInfo(CPrintInfo* pInfo);

    public:  /* Implementation **********************************************/

    virtual     ~CPBView();

    virtual BOOL PreCreateWindow( CREATESTRUCT& cs );
    virtual void OnInitialUpdate( void );
    virtual void OnActivateView ( BOOL bActivate, CView* pActivateView, CView* pDeactiveView );
    virtual void OnDraw         ( CDC* pDC ); // overridden to draw this view
    virtual BOOL OnCmdMsg       ( UINT, int, void*, AFX_CMDHANDLERINFO*);
    virtual void OnPrepareDC    ( CDC* pDC, CPrintInfo* pInfo = NULL );


    // Printing support
    virtual BOOL OnPreparePrinting(           CPrintInfo* pInfo );
    virtual void OnBeginPrinting  ( CDC* pDC, CPrintInfo* pInfo );
    virtual void OnPrint          ( CDC* pDC, CPrintInfo* pInfo );
    virtual void OnEndPrinting    ( CDC* pDC, CPrintInfo* pInfo );

    BOOL CanSetWallpaper();
    void SetTheWallpaper( BOOL bTiled = FALSE );

    #ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
    #endif

    // Generated message map functions
    protected: /************************************************************/

    //{{AFX_MSG(CPBView)
    afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnFilePrint();
    afx_msg void OnFilePrintPreview();
    afx_msg void OnEditUndo();
    afx_msg void OnEditRedo();
    afx_msg void OnEditCut();
    afx_msg void OnEditClear();
    afx_msg void OnEditCopy();
    afx_msg void OnEditPaste();
    afx_msg void OnUpdateEditUndo(CCmdUI* pCmdUI);
    afx_msg void OnUpdateEditRedo(CCmdUI* pCmdUI);
    afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
    afx_msg void OnViewGrid();
    afx_msg void OnViewZoom100();
    afx_msg void OnViewZoom400();
    afx_msg void OnUpdateViewZoom100(CCmdUI* pCmdUI);
    afx_msg void OnUpdateViewZoom400(CCmdUI* pCmdUI);
    afx_msg void OnUpdateViewGrid(CCmdUI* pCmdUI);
    afx_msg void OnImageInvertColors();
    afx_msg void OnUpdateImageInvertColors(CCmdUI* pCmdUI);
    afx_msg void OnTglopaque();
    afx_msg void OnUpdateTglopaque(CCmdUI* pCmdUI);
    afx_msg void OnImageAttributes();
    afx_msg void OnSel2bsh();
    afx_msg void OnLargerbrush();
    afx_msg void OnSmallerbrush();
    afx_msg void OnViewZoom();
    afx_msg void OnImageFlipRotate();
    afx_msg void OnUpdateImageFlipRotate(CCmdUI* pCmdUI);
    afx_msg void OnEditcolors();
    afx_msg void OnUpdateEditcolors(CCmdUI* pCmdUI);
    #if 0 // unused, lame features
    afx_msg void OnLoadcolors();
    afx_msg void OnUpdateLoadcolors(CCmdUI* pCmdUI);
    afx_msg void OnSavecolors();
    afx_msg void OnUpdateSavecolors(CCmdUI* pCmdUI);
    #endif
    afx_msg void OnEditSelectAll();
    afx_msg void OnEditPasteFrom();
    afx_msg void OnEditCopyTo();
    afx_msg void OnUpdateEditCopyTo(CCmdUI* pCmdUI);
    afx_msg void OnImageStretchSkew();
    afx_msg void OnUpdateImageStretchSkew(CCmdUI* pCmdUI);
    afx_msg void OnViewViewPicture();
    afx_msg void OnUpdateViewViewPicture(CCmdUI* pCmdUI);
    afx_msg void OnViewTextToolbar();
    afx_msg void OnUpdateViewTextToolbar(CCmdUI* pCmdUI);
    afx_msg void OnFileSetaswallpaperT();
    afx_msg void OnUpdateFileSetaswallpaperT(CCmdUI* pCmdUI);
    afx_msg void OnFileSetaswallpaperC();
    afx_msg void OnUpdateFileSetaswallpaperC(CCmdUI* pCmdUI);
    afx_msg void OnViewThumbnail();
    afx_msg void OnUpdateViewThumbnail(CCmdUI* pCmdUI);
   afx_msg void OnUpdateImageAttributes(CCmdUI* pCmdUI);
    afx_msg void OnEscape();
    afx_msg void OnEscapeServer();
   afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
   afx_msg void OnUpdateEditSelection(CCmdUI* pCmdUI);
    afx_msg void OnUpdateEditClearSel(CCmdUI* pCmdUI);
   afx_msg void OnFilePageSetup();
   afx_msg void OnImageClearImage();
   afx_msg void OnUpdateImageClearImage(CCmdUI* pCmdUI);
   //}}AFX_MSG
   afx_msg void OnDestroy();
   afx_msg BOOL PreTranslateMessage(MSG *pMsg);

#ifdef CUSTOMFLOAT
    afx_msg void OnUpdateViewColorBox(CCmdUI* pCmdUI);
    afx_msg void OnUpdateViewToolBox(CCmdUI* pCmdUI);
#endif



    DECLARE_MESSAGE_MAP()

    friend class CPrintResObj;
    };

#ifndef _DEBUG  // debug version in pbrusvw.cpp
inline CPBDoc* CPBView::GetDocument() { return (CPBDoc*)m_pDocument; }
#endif

/***************************************************************************/
