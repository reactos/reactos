class CJPEGFilter;

class CJPEGSource :
   public jpeg_source_mgr
{
public:
   CJPEGSource( CJPEGFilter* pFilter );
   ~CJPEGSource();

   HRESULT Init();
   
   static void InitSource( j_decompress_ptr pInfo );
   static boolean FillInputBuffer( j_decompress_ptr pInfo );
   static void SkipInputData( j_decompress_ptr pInfo, long nBytes );
   static void TermSource( j_decompress_ptr pInfo );

protected:
   CJPEGFilter* m_pFilter;
   BYTE* m_pbBuffer;
};

const DWORD JPEG_EXCEPTION = 0x01;

class CJPEGFilter : 
   public IImageDecodeFilter,
	public CComObjectRoot,
	public CComCoClass< CJPEGFilter, &CLSID_CoJPEGFilter >
{
   friend CJPEGSource;

public:
	CJPEGFilter();
   ~CJPEGFilter();

   HRESULT FinalConstruct();

   BEGIN_COM_MAP( CJPEGFilter )
	   COM_INTERFACE_ENTRY( IImageDecodeFilter )
   END_COM_MAP()

   DECLARE_NOT_AGGREGATABLE( CJPEGFilter )  
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

   DECLARE_REGISTRY( CJPEGFilter, _T( "JPEGFilter.CoJPEGFilter.1" ), 
      _T( "JPEGFilter.CoJPEGFilter" ), IDS_COJPEGFILTER_DESC, 
      THREADFLAGS_BOTH )
   
//   DECLARE_NO_REGISTRY()

// IImageDecodeFilter
public:
   STDMETHOD( Initialize )( IImageDecodeEventSink* pEventSink );
   STDMETHOD( Process )( IStream* pStream );
   STDMETHOD( Terminate )( HRESULT hrStatus );

protected:
   HRESULT FireOnBitsCompleteEvent();
   HRESULT FireGetSurfaceEvent();
   HRESULT FireOnProgressEvent();

    HRESULT LockBits(RECT *prcBounds, DWORD dwLockFlags, void **ppBits, long *pPitch);
    HRESULT UnlockBits(RECT *prcBounds, void *pBits);

   static void ErrorExit( j_common_ptr pInfo );

protected:
   DWORD m_dwEvents;
   ULONG m_iScanLine;
   ULONG m_iFirstStaleScanLine;
   IStream* m_pStream;
   CJPEGSource* m_pSource;
   CComPtr< IImageDecodeEventSink > m_pEventSink;
   CComPtr< IBitmapSurface > m_pBitmapSurface;
   CComPtr< IDirectDrawSurface > m_pDDrawSurface;
   struct jpeg_decompress_struct m_jpegInfo;
   struct jpeg_error_mgr m_jpegError;
   ULONG m_nWidth;
   ULONG m_nHeight;
   ULONG m_nBytesPerPixel;
   BYTE* m_pbScanLines;
   BOOL m_bRGB8Allowed;
};
