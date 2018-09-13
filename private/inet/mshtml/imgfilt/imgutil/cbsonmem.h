class CBitmapSurfaceOnMemory : 
	public IBitmapSurface,
   public IRGBColorTable,
   public IBitmapSurfaceOnMemoryImpl,
	public CComObjectRoot,
   public CComCoClass< CBitmapSurfaceOnMemory, &CLSID_CoBitmapSurfaceOnMemory >
{
public:
	CBitmapSurfaceOnMemory();
   ~CBitmapSurfaceOnMemory();

   HRESULT FinalConstruct();

   BEGIN_COM_MAP( CBitmapSurfaceOnMemory )
	   COM_INTERFACE_ENTRY( IBitmapSurface )
      COM_INTERFACE_ENTRY_FUNC( IID_IRGBColorTable, 0, GetIRGBColorTable )
      COM_INTERFACE_ENTRY( IBitmapSurfaceOnMemoryImpl )
      COM_INTERFACE_ENTRY_AGGREGATE( IID_IMarshal, m_pUnkMarshal )
   END_COM_MAP()

   DECLARE_GET_CONTROLLING_UNKNOWN()

/*
   DECLARE_REGISTRY( CSmartBitmapSurface, 
      _T( "BitmapSurfaces.CoSmartBitmapSurface.1" ), 
      _T( "BitmapSurfaces.CoSmartBitmapSurface" ), 
      IDS_COSMARTBITMAPSURFACE_DESC, THREADFLAGS_BOTH )
*/
   
   DECLARE_NO_REGISTRY()

// DECLARE_NOT_AGGREGATABLE( CBitmapSurface ) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

// IBitmapSurface
public:
   STDMETHOD( Clone )( IBitmapSurface** ppSurface );
   STDMETHOD( GetSize )( LONG* pnWidth, LONG* pnHeight );
   STDMETHOD( GetFormat )( GUID* pBFID );
   STDMETHOD( GetFactory )( IBitmapSurfaceFactory** ppFactory );
   STDMETHOD( LockBits )( RECT* pBounds, DWORD dwLockFlags, void** ppBits, 
      LONG* pnPitch );
   STDMETHOD( UnlockBits )( RECT* pBounds, void* pBits );

// IRGBColorTable
public:
   STDMETHOD( GetColors )( LONG iFirst, LONG nCount, RGBQUAD* pColors );
   STDMETHOD( GetCount )( LONG* pnCount );
   STDMETHOD( SetCount )( LONG nCount );
   STDMETHOD( GetTransparentIndex )( LONG* piIndex );
   STDMETHOD( SetColors )( LONG iFirst, LONG nCount, RGBQUAD* pColors );
   STDMETHOD( SetTransparentIndex )( LONG iIndex );

// IBitmapSurfaceOnMemoryImpl
public:
   STDMETHOD( Init )( ULONG nWidth, ULONG nHeight, REFGUID bfid, void* pBits,
      LONG nPitch );

protected:
   static HRESULT WINAPI GetIRGBColorTable( void* pv, REFIID iid, void** ppv,
      DWORD dw );

protected:
   IUnknown* m_pUnkMarshal;
   BOOL m_bInitialized;
   GUID m_bfidFormat;
   BYTE* m_pBits;
   LONG m_nPitch;
   ULONG m_nBitsPerPixel;
   RECT* m_pLockedBounds;
   BYTE* m_pLockedBits;
   CComAutoCriticalSection m_cs;
   CComAutoCriticalSection m_csLock;
   LONG m_nLockCount;
   LONG m_nColors;
   LONG m_nWidth;
   LONG m_nHeight;
   LONG m_iTransparentIndex;
   RGBQUAD m_argbColors[256];
};

