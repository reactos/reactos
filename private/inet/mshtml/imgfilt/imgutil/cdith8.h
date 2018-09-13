const ULONG MAX_DITHERTABLE_CACHE_SIZE = 4;

class CDitherToRGB8 :
   public IImageDecodeEventSink,
   public IDithererImpl,
   public CComObjectRoot,
   public CComCoClass< CDitherToRGB8, &CLSID_CoDitherToRGB8 >
{
public:
   CDitherToRGB8();
   ~CDitherToRGB8();

   BEGIN_COM_MAP( CDitherToRGB8 )
      COM_INTERFACE_ENTRY( IImageDecodeEventSink )
      COM_INTERFACE_ENTRY( IDithererImpl )
   END_COM_MAP()


   DECLARE_REGISTRY( CDitherToRGB8, _T( "ImgUtil.CoDitherToRGB8.1" ),
      _T( "ImgUtil.CoDitherToRGB8" ), IDS_CODITHERTORGB8_DESC, 
      THREADFLAGS_BOTH );


//   DECLARE_NO_REGISTRY()

// IImageDecodeEventSink
public:
   STDMETHOD( GetSurface )( LONG nWidth, LONG nHeight, REFGUID bfid, 
      ULONG nPasses, DWORD dwHints, IUnknown** ppSurface );
   STDMETHOD( OnBeginDecode )( DWORD* pdwEvents, ULONG* pnFormats, 
      GUID** ppFormats );
   STDMETHOD( OnBitsComplete )();
   STDMETHOD( OnDecodeComplete )( HRESULT hrStatus );
   STDMETHOD( OnPalette )();
   STDMETHOD( OnProgress )( RECT* pBounds, BOOL bComplete );

// IDithererImpl
public: 
   STDMETHOD( SetDestColorTable )( ULONG nColors, const RGBQUAD* prgbColors );
   STDMETHOD( SetEventSink )( IImageDecodeEventSink* pEventSink );
public:
   static void InitTableCache();
   static void CleanupTableCache();

protected:
   HRESULT ConvertBlock( RECT* pBounds );
   HRESULT DitherBand( RECT* pBounds );
   HRESULT DitherFull();

protected:
   static CDitherTable* s_apTableCache[MAX_DITHERTABLE_CACHE_SIZE];
   static ULONG s_nCacheSize;
   static CRITICAL_SECTION s_csCache;

protected:
   typedef enum _ESrcFormat
   {
      RGB24,
      RGB8
   } ESrcFormat;

   CComPtr< IImageDecodeEventSink > m_pEventSink;
   CComPtr< IDirectDrawSurface > m_pDestSurface;
   CComPtr< IDirectDrawSurface > m_pSurface;
   BYTE *m_pbBits;
   DWORD m_dwEvents;
   ULONG m_nWidth;
   ULONG m_nHeight;
   ULONG m_nPitch;
   ULONG m_nBitsPerPixel;
   BOOL m_bProgressiveDither;
   ESrcFormat m_eSrcFormat;
   ULONG m_iScanLine;
   HBITMAP m_hbmDestDib;
   ERRBUF* m_pErrBuf;
   ERRBUF* m_pErrBuf1;
   ERRBUF* m_pErrBuf2;
   CDitherTable* m_pTable;
   RGBQUAD m_argbSrcColors[256];
};

