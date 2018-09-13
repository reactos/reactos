// srvritem.h : interface of the CPBSrvrItem class
//

class CPBSrvrItem : public COleServerItem
    {
    DECLARE_DYNAMIC(CPBSrvrItem)

    // Constructors
    public:

    CPBSrvrItem(CPBDoc* pContainerDoc, CBitmapObj* pBM = NULL);

    // Attributes
    CPBDoc* GetDocument() const { return (CPBDoc*)COleServerItem::GetDocument(); }

    // Implementation
    public:

    CBitmapObj* m_pBitmapObj;

    ~CPBSrvrItem();

    #ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
    #endif

    virtual BOOL OnDraw(CDC* pDC, CSize& rSize);
    virtual BOOL OnGetExtent( DVASPECT dwDrawAspect, CSize& rSize );
    virtual BOOL OnSetExtent( DVASPECT nDrawAspect, const CSize& size );
    virtual void OnOpen( void );
    virtual void OnShow( void );
    virtual void OnHide( void );
    virtual BOOL OnRenderGlobalData( LPFORMATETC lpFormatEtc, HGLOBAL* phGlobal );
	virtual BOOL OnRenderFileData( LPFORMATETC lpFormatEtc, CFile* pFile );
	virtual COleDataSource* OnGetClipboardData( BOOL bIncludeLink,
                                                CPoint* pptOffset, CSize *pSize );
   
    protected:

    virtual void Serialize(CArchive& ar);   // overridden for document i/o
    };

/***************************************************************************/
