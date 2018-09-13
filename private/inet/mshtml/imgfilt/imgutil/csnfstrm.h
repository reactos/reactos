class CSniffStream :
   public IStream,
   public ISniffStream,
   public CComObjectRoot,
   public CComCoClass< CSniffStream, &CLSID_CoSniffStream >
{
public:
   CSniffStream();  // Default constructor
   ~CSniffStream();  // Destructor

   BEGIN_COM_MAP( CSniffStream )
      COM_INTERFACE_ENTRY( IStream )
      COM_INTERFACE_ENTRY( ISniffStream )
   END_COM_MAP()


   DECLARE_REGISTRY( CSniffStream, _T( "ImgUtil.CoSniffStream.1" ),
      _T( "ImgUtil.CoSniffStream" ), IDS_COSNIFFSTREAM_DESC, 
      THREADFLAGS_BOTH );


//   DECLARE_NO_REGISTRY()

// IStream
public:
   STDMETHOD( Clone )( IStream** ppStream );
   STDMETHOD( Commit )( DWORD dwFlags );
   STDMETHOD( CopyTo )( IStream* pStream, ULARGE_INTEGER nBytes, 
      ULARGE_INTEGER* pnBytesRead, ULARGE_INTEGER* pnBytesWritten );
   STDMETHOD( LockRegion )( ULARGE_INTEGER iOffset, ULARGE_INTEGER nBytes,
      DWORD dwLockType );
   STDMETHOD( Read )( void* pBuffer, ULONG nBytes, ULONG* pnBytesRead );
   STDMETHOD( Revert )();
   STDMETHOD( Seek )( LARGE_INTEGER nDisplacement, DWORD dwOrigin, 
      ULARGE_INTEGER* piNewPosition );
   STDMETHOD( SetSize )( ULARGE_INTEGER nNewSize );
   STDMETHOD( Stat )( STATSTG* pStatStg, DWORD dwFlags );
   STDMETHOD( UnlockRegion )( ULARGE_INTEGER iOffset, ULARGE_INTEGER nBytes,
      DWORD dwLockType );
   STDMETHOD( Write )( const void* pBuffer, ULONG nBytes, 
      ULONG* pnBytesWritten );

// ISniffStream
public:
   STDMETHOD( Init )( IStream* pStream );
   STDMETHOD( Peek )( void* pBuffer, ULONG nBytes, ULONG* pnBytesRead );

protected:
   CComPtr< IStream > m_pStream;
   BYTE* m_pbBuffer;
   ULONG m_nBufferSize;
   ULONG m_nValidBytes;
   ULONG m_iNextFreeByte;
   ULONG m_iOffset;
};
