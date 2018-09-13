class CBitmapSurfaceFactory :
   public IBitmapSurfaceFactory,
//   public IAnimationFactory,
   public CComObjectRoot,
   public CComCoClass< CBitmapSurfaceFactory, &CLSID_CoBitmapSurfaceFactory >
{
public:
   CBitmapSurfaceFactory();  // Default constructor
   ~CBitmapSurfaceFactory();  // Destructor

   BEGIN_COM_MAP( CBitmapSurfaceFactory )
      COM_INTERFACE_ENTRY( IBitmapSurfaceFactory )
//      COM_INTERFACE_ENTRY( IAnimationFactory )
      COM_INTERFACE_ENTRY( IBitmapSurfaceFactoryImpl )
   END_COM_MAP()

/*
   DECLARE_REGISTRY( CBitmapSurfaceFactory, 
      _T( "BitmapSurface.CoBitmapSurfaceFactory.1" ), 
      _T( "BitmapSurface.CoBitmapSurfaceFactory" ), 
      IDS_COBITMAPSURFACEFACTORY_DESC, 
      THREADFLAGS_BOTH )
*/

   DECLARE_NO_REGISTRY()

// IBitmapSurfaceFactory
public:
   STDMETHOD( CreateBitmapSurface )( LONG nWidth, LONG nHeight, GUID* pBFID,
      DWORD dwHintFlags, IBitmapSurface** ppSurface );
   STDMETHOD( GetSupportedFormatsCount )( LONG* pnFormats );
   STDMETHOD( GetSupportedFormats )( LONG nFormats, GUID* pBFIDs );

// IAnimationFactory
public:
//   STDMETHOD( CreateAnimation )( IBounds* pBounds, REFGUID bfid, 
//      IAnimation** ppAnimation );

protected:
   GUID m_aFormats[7];
   LONG m_nFormats;
};


