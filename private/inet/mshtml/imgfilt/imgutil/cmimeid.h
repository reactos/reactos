class CMIMEBitMatcher
{
public:
   CMIMEBitMatcher();  // Default constructor
   ~CMIMEBitMatcher();  // Destructor

   HRESULT InitFromBinary( const BYTE* pData, ULONG nBytes, 
      ULONG* pnBytesToMatch );
   HRESULT Match( const BYTE* pBytes, ULONG nBytes ) const;

protected:
   char m_achSignature[4];

public:
   CMIMEBitMatcher* m_pNext;

protected:
   ULONG m_nOffset;
   ULONG m_nBytes;
   BYTE* m_pMask;
   BYTE* m_pData;
};

class CMIMEType
{
public:
   CMIMEType();  // Default constructor
   ~CMIMEType();  // Destructor

   UINT GetClipboardFormat() const;
   HRESULT InitFromKey( HKEY hKey, LPCTSTR pszName, ULONG* pnMaxBytes );
   HRESULT Match( const BYTE* pBytes, ULONG nBytes ) const;

protected:
   char m_achSignature[4];

public:
   CMIMEType* m_pNext;

protected:
   UINT m_nClipboardFormat;
   CMIMEBitMatcher* m_lpBitMatchers;
   ULONG m_nMaxBytes;
};

class CMIMEIdentifier
{
public:
   CMIMEIdentifier();  // Default constructor
   ~CMIMEIdentifier();  // Destructor

   ULONG GetMaxBytes() const;
   HRESULT Identify( const BYTE* pbBytes, ULONG nBytes, UINT* pnFormat );
   HRESULT IdentifyStream( ISniffStream* pSniffStream, 
      UINT* pnClipboardFormat );
   HRESULT InitFromRegistry();

protected:
   char m_achSignature[4];

protected:
   ULONG m_nMaxBytes;
   CMIMEType* m_lpTypes;
};

void InitMIMEIdentifier();
