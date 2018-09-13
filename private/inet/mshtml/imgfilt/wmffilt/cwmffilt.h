// CWMFFilter.H : Declaration of the CWMFFilter

#pragma pack( push, WMF )
#pragma pack( 1 )

typedef struct tagSRECT 
{
   short	left;
   short	top;
   short	right;
   short	bottom;
} SRECT;

typedef struct 
{
   DWORD key;
   WORD hmf;
   SRECT bbox;
   WORD inch;
   DWORD reserved;
   WORD checksum;
} ALDUSMFHEADER;

#pragma pack( pop, WMF )

const DWORD ALDUSKEY = 0x9AC6CDD7;

/////////////////////////////////////////////////////////////////////////////
// WMFFilter

class CWMFFilter : 
	public IImageDecodeFilter,
	public CComObjectRoot,
	public CComCoClass< CWMFFilter,&CLSID_CoWMFFilter >
{
public:
	CWMFFilter();
   ~CWMFFilter();

   BEGIN_COM_MAP( CWMFFilter )
	   COM_INTERFACE_ENTRY( IImageDecodeFilter )
   END_COM_MAP()

   DECLARE_NOT_AGGREGATABLE( CWMFFilter ) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

   DECLARE_REGISTRY( CWMFFilter, _T( "WMFFilter.CoWMFFilter.1" ), 
      _T( "WMFFilter.CoWMFFilter" ), IDS_COWMFFILTER_DESC, THREADFLAGS_BOTH )

//   DECLARE_NO_REGISTRY()

// IImageDecodeFilter
public:
   STDMETHOD( Initialize )( IImageDecodeEventSink* pEventSink );
   STDMETHOD( Process )( IStream* pStream );
   STDMETHOD( Terminate )( HRESULT hrStatus );

protected:
   HRESULT FireGetSurfaceEvent();
   HRESULT FireOnBeginDecodeEvent();
   HRESULT FireOnBitsCompleteEvent();
   HRESULT FireOnProgressEvent();
   HRESULT GetColors();
   HRESULT ReadImage();

    HRESULT LockBits(RECT *prcBounds, DWORD dwLockFlags, void **ppBits, long *pPitch);
    HRESULT UnlockBits(RECT *prcBounds, void *pBits);
   
protected:
   IStream* m_pStream;
   CComPtr< IImageDecodeEventSink > m_pEventSink;
   CComPtr< IDirectDrawSurface > m_pDDrawSurface;
   RGBQUAD m_argbPalette[256];
   ULONG m_nColors;
   DWORD m_dwEvents;
   ULONG m_nWidth;
   ULONG m_nHeight;
   LONG  m_lTrans;
};
