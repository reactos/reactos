class CBounds :
   public IBounds,
   public IFrameBounds,
   public CComObjectRoot,
   public CComCoClass< CBounds, &CLSID_CoBounds >
{
public:
   CBounds();  // Default constructor
   ~CBounds();  // Destructor

   HRESULT FinalConstruct();

   BEGIN_COM_MAP( CBounds )
      COM_INTERFACE_ENTRY( IBounds )
      COM_INTERFACE_ENTRY( IFrameBounds )
      COM_INTERFACE_ENTRY_AGGREGATE( IID_IMarshal, m_pUnkMarshal )
   END_COM_MAP()

/*
   DECLARE_REGISTRY( CBounds, _T( "BitmapSurface.CoBounds.1" ),
      _T( "BitmapSurface.CoBounds" ), IDS_COBOUNDS_DESC, THREADFLAGS_BOTH )
*/

   DECLARE_NO_REGISTRY()

// IBounds
public:
   STDMETHOD( Clone )( IBounds** ppResult );
   STDMETHOD( CopyFromBounds )( IBounds* pBounds );
   STDMETHOD( IsEmpty )();
   STDMETHOD( CompareBounds )( IBounds* pBounds );
   STDMETHOD( IntersectBounds )( IBounds* pSrc, IBounds** ppResult );
   STDMETHOD( UnionBounds )( IBounds* pSrc, IBounds** ppResult );
   STDMETHOD( GetRect )( LONG* pnLeft, LONG* pnTop, LONG* pnRight, 
      LONG* pnBottom );
   STDMETHOD( SetRect )( LONG nLeft, LONG nTop, LONG nRight, LONG nBottom );

// IFrameBounds
public:
   STDMETHOD( GetRange )( ULONG* piFirstFrame, ULONG* piLastFrame );
   STDMETHOD( SetRange )( ULONG iFirstFrame, ULONG iLastFrame );

protected:
   IUnknown* m_pUnkMarshal;
   LONG m_nLeft;
   LONG m_nTop;
   LONG m_nRight;
   LONG m_nBottom;
   ULONG m_iFirstFrame;
   ULONG m_iLastFrame;
};

