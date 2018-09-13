class CBitmapSurface : 
	public IBitmapSurface,
   public IGdiSurface,
   public IRGBColorTable,
//   public IAnimation,
   public IBitmapSurfaceImpl,
	public CComObjectRoot,
   public CComCoClass< CBitmapSurface, &CLSID_CoBitmapSurface >
{
public:
	CBitmapSurface();
   ~CBitmapSurface();

   HRESULT FinalConstruct();

   BEGIN_COM_MAP( CBitmapSurface )
	   COM_INTERFACE_ENTRY( IBitmapSurface )
      COM_INTERFACE_ENTRY( IGdiSurface )
      COM_INTERFACE_ENTRY_FUNC( IID_IRGBColorTable, 0, GetIRGBColorTable )
//      COM_INTERFACE_ENTRY_FUNC( IID_IAnimation, 0, GetIAnimation )
      COM_INTERFACE_ENTRY( IBitmapSurfaceImpl )
      COM_INTERFACE_ENTRY_AGGREGATE( IID_IMarshal, m_pUnkMarshal )
   END_COM_MAP()

   DECLARE_GET_CONTROLLING_UNKNOWN()

   DECLARE_REGISTRY( CBitmapSurface, _T( "ImgUtil.CoBitmapSurfaceOnMemory.1" ), 
      _T( "ImgUtil.CoBitmapSurfaceOnMemory" ), IDS_COBITMAPSURFACE_DESC, 
      THREADFLAGS_BOTH )

//   DECLARE_NO_REGISTRY()

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

// IGdiSurface
public:
   STDMETHOD( GetDC )( HDC* phDC, DWORD dwLockFlags );
   STDMETHOD( ReleaseDC )( HDC hDC );

// IRGBColorTable
public:
   STDMETHOD( GetColors )( LONG iFirst, LONG nCount, RGBQUAD* pColors );
   STDMETHOD( GetCount )( LONG* pnCount );
   STDMETHOD( SetCount )( LONG nCount );
   STDMETHOD( GetTransparentIndex )( LONG* piIndex );
   STDMETHOD( SetColors )( LONG iFirst, LONG nCount, RGBQUAD* pColors );
   STDMETHOD( SetTransparentIndex )( LONG iIndex );

// IAnimation
public:
//   STDMETHOD( AddFrames )( ULONG nFrames );
//   STDMETHOD( GetFrame )( ULONG iFrame, IBitmapSurface** ppSurface );
//   STDMETHOD( GetNumFrames )( ULONG* pnFrames );

// IBitmapSurfaceImpl
public:
   STDMETHOD( Init )( ULONG nWidth, ULONG nHeight, REFGUID bfid,
      BOOL bAnimation, IBitmapSurfaceFactory* pFactory );

protected:
   static HRESULT WINAPI GetIAnimation( void* pv, REFIID iid, void** ppv,
      DWORD dw );
   static HRESULT WINAPI GetIRGBColorTable( void* pv, REFIID iid, void** ppv,
      DWORD dw );

protected:
   char m_achSignature[4];
   IUnknown* m_pUnkMarshal;
   BOOL m_bAnimation;
   BOOL m_bInitialized;
   GUID m_bfidFormat;
   CComPtr< IBitmapSurfaceFactory > m_pFactory;
   BYTE* m_pBits;
   LONG m_nPitch;
   ULONG m_nBitsPerPixel;
   RECT* m_pLockedBounds;
   BYTE* m_pLockedBits;
   HBITMAP m_hBitmap;
   CComAutoCriticalSection m_cs;
   BITMAPINFO* m_pBMI;
   LONG m_nColors;
   LONG m_nWidth;
   LONG m_nHeight;
   LONG m_iTransparentIndex;
   HDC m_hDC;
   HGDIOBJ m_hOldObject;
   IBitmapSurface** m_ppFrames;
   ULONG m_nFrames;
};

