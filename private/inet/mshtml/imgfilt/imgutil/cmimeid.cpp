#include "stdafx.h"
#include "imgutil.h"
#include "cmimeid.h"
#include "identify.h"

static CRITICAL_SECTION g_csMIMEIdentifier;
static CMIMEIdentifier* g_pMIMEIdentifier = NULL;

void InitMIMEIdentifier()
{
   InitializeCriticalSection( &g_csMIMEIdentifier );
}

STDAPI GetMaxMIMEIDBytes( ULONG* pnMaxBytes )
{
   HRESULT hResult;

   if( pnMaxBytes != NULL )
   {
      *pnMaxBytes = 0;
   }
   if( pnMaxBytes == NULL )
   {
      return( E_POINTER );
   }

   EnterCriticalSection( &g_csMIMEIdentifier );
   if( g_pMIMEIdentifier == NULL )
   {
      g_pMIMEIdentifier = new CMIMEIdentifier;
      hResult = g_pMIMEIdentifier->InitFromRegistry();
      if( FAILED( hResult ) )
      {
         delete g_pMIMEIdentifier;
         g_pMIMEIdentifier = NULL;
         LeaveCriticalSection( &g_csMIMEIdentifier );
         return( hResult );
      }
   }

   *pnMaxBytes = g_pMIMEIdentifier->GetMaxBytes();

   LeaveCriticalSection( &g_csMIMEIdentifier );

   return( S_OK );
}

STDAPI IdentifyMIMEType( const BYTE* pbBytes, ULONG nBytes, 
   UINT* pnFormat )
{
   HRESULT hResult;
  
   if( pnFormat != NULL )
   {
      *pnFormat = 0;
   }
   if( pbBytes == NULL )
   {
      return( E_INVALIDARG );
   }
   if( pnFormat == NULL )
   {
      return( E_POINTER );
   }

   EnterCriticalSection( &g_csMIMEIdentifier );
   if( g_pMIMEIdentifier == NULL )
   {
      g_pMIMEIdentifier = new CMIMEIdentifier;
      hResult = g_pMIMEIdentifier->InitFromRegistry();
      if( FAILED( hResult ) )
      {
         delete g_pMIMEIdentifier;
         g_pMIMEIdentifier = NULL;
         LeaveCriticalSection( &g_csMIMEIdentifier );
         return( hResult );
      }
   }

   hResult = g_pMIMEIdentifier->Identify( pbBytes, nBytes, pnFormat );
   LeaveCriticalSection( &g_csMIMEIdentifier );

   return( hResult );
}

STDAPI SniffStream( IStream* pInStream, UINT* pnFormat, 
   IStream** ppOutStream )
{
   HRESULT hResult;
   HRESULT hIDResult;
   CComPtr< ISniffStream > pSniffStream;

   if( pnFormat != NULL )
   {
      *pnFormat = 0;
   }
   if( ppOutStream != NULL )
   {
      *ppOutStream = NULL;
   }

   if( pInStream == NULL )
   {
      return( E_INVALIDARG );
   }
   if( pnFormat == NULL )
   {
      return( E_POINTER );
   }
   if( ppOutStream == NULL )
   {
      return( E_POINTER );
   }

   hResult = pInStream->QueryInterface( IID_ISniffStream, 
      (void**)&pSniffStream );
   if( FAILED( hResult ) && (hResult != E_NOINTERFACE) )
   {
      return( hResult );
   }

   if( hResult == E_NOINTERFACE )
   {
      hResult = CoCreateInstance( CLSID_CoSniffStream, NULL, 
         CLSCTX_INPROC_SERVER, IID_ISniffStream, (void**)&pSniffStream );
      if( FAILED( hResult ) )
      {
         return( hResult );
      }

      hResult = pSniffStream->Init( pInStream );
      if( FAILED( hResult ) )
      {
         return( hResult );
      }
   }


   EnterCriticalSection( &g_csMIMEIdentifier );
   if( g_pMIMEIdentifier == NULL )
   {
      g_pMIMEIdentifier = new CMIMEIdentifier;
      hResult = g_pMIMEIdentifier->InitFromRegistry();
      if( FAILED( hResult ) )
      {
         delete g_pMIMEIdentifier;
         g_pMIMEIdentifier = NULL;
         LeaveCriticalSection( &g_csMIMEIdentifier );
         return( hResult );
      }
   }

   hIDResult = g_pMIMEIdentifier->IdentifyStream( pSniffStream, pnFormat );
   LeaveCriticalSection( &g_csMIMEIdentifier );
   if( FAILED( hIDResult ) )
   {
      return( hIDResult );
   }

   hResult = pSniffStream->QueryInterface( IID_IStream, (void**)ppOutStream );
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   return( hIDResult );
}

const LPCTSTR MIME_DATABASE_ROOT = _T( "MIME\\Database\\Content Type" );

CMIMEBitMatcher::CMIMEBitMatcher() :
   m_pNext( NULL ),
   m_nOffset( 0 ),
   m_nBytes( 0 ),
   m_pMask( NULL ),
   m_pData( NULL )
{
   memcpy( m_achSignature, "NoLK", 4 );
}

CMIMEBitMatcher::~CMIMEBitMatcher()
{
   delete m_pMask;
   delete m_pData;
}

HRESULT CMIMEBitMatcher::InitFromBinary( const BYTE* pData, ULONG nBytes,
   ULONG* pnBytesToMatch )
{
   const BYTE* pTrav;
#ifdef BIG_ENDIAN
   BYTE pTravBig[4];
#endif

   _ASSERTE( pData != NULL );
   _ASSERTE( pnBytesToMatch != NULL );

   if( nBytes <= sizeof( ULONG ) )
   {
      return( E_FAIL );
   }

   pTrav = pData;
#ifdef BIG_ENDIAN
   pTravBig[0] = pTrav[3];
   pTravBig[1] = pTrav[2];
   pTravBig[2] = pTrav[1];
   pTravBig[3] = pTrav[0];
   m_nBytes = *(ULONG*)pTravBig;
#else
   m_nBytes = *(const ULONG*)(pTrav);
#endif   

   pTrav += sizeof( ULONG );

   if( nBytes != (2*m_nBytes)+sizeof( ULONG ) )
   {
      return( E_FAIL );
   }

   m_pMask = new BYTE[m_nBytes];
   if( m_pMask == NULL )
   {
      return( E_OUTOFMEMORY );
   }

   m_pData = new BYTE[m_nBytes];
   if( m_pData == NULL )
   {
      return( E_OUTOFMEMORY );
   }

   memcpy( m_pMask, pTrav, m_nBytes );
   pTrav += m_nBytes;
   memcpy( m_pData, pTrav, m_nBytes );

   *pnBytesToMatch = m_nBytes;

   return( S_OK );
}

HRESULT CMIMEBitMatcher::Match( const BYTE* pBytes, ULONG nBytes ) const
{
   ULONG iByte;
   ULONG nBytesToMatch;

   _ASSERTE( m_nBytes > 0 );

   nBytesToMatch = min( nBytes, m_nBytes );
   for( iByte = 0; iByte < nBytesToMatch; iByte++ )
   {
      if( (pBytes[iByte]&m_pMask[iByte]) != m_pData[iByte] )
      {
         // The bits definitely don't match
         return( S_FALSE );
      }
   }

   if( nBytes < m_nBytes )
   {
      // We could have a match, but we need more data to be sure.
      return( E_PENDING );
   }

   // We have a match
   return( S_OK );
}


CMIMEType::CMIMEType() :
   m_pNext( NULL ),
   m_nClipboardFormat( 0 ),
   m_lpBitMatchers( NULL ),
   m_nMaxBytes( 0 )
{
   memcpy( m_achSignature, "NoLK", 4 );
}

CMIMEType::~CMIMEType()
{
   CMIMEBitMatcher* pBitMatcher;

   while( m_lpBitMatchers != NULL )
   {
      pBitMatcher = m_lpBitMatchers;
      m_lpBitMatchers = m_lpBitMatchers->m_pNext;
      delete pBitMatcher;
   }
}

UINT CMIMEType::GetClipboardFormat() const
{
   return( m_nClipboardFormat );
}

HRESULT CMIMEType::InitFromKey( HKEY hKey, LPCTSTR pszName, ULONG* pnMaxBytes )
{
   LONG nResult;
   HRESULT hResult;
   HKEY hBitsKey;
   DWORD dwValueType;
   BYTE* pData;
   ULONG nBytes;
   ULONG nBytesToMatch;
   CMIMEBitMatcher* pBitMatcher;

   _ASSERTE( hKey != NULL );
   _ASSERTE( pszName != NULL );
   _ASSERTE( pnMaxBytes != NULL );

   nResult = RegOpenKeyEx( hKey, _T( "Bits" ), 0, KEY_READ, &hBitsKey );
   if( nResult != ERROR_SUCCESS )
   {
      return( E_FAIL );
   }

   nResult = RegQueryValueEx( hBitsKey, _T( "0" ), NULL, &dwValueType, NULL,
      &nBytes );
   if( (nResult != ERROR_SUCCESS) || (dwValueType != REG_BINARY) || nBytes > 8192 )
   {
      RegCloseKey( hBitsKey );
      return( E_FAIL );
   }
   pData = LPBYTE( _alloca( nBytes ) );

   nResult = RegQueryValueEx( hBitsKey, _T( "0" ), NULL, &dwValueType, pData,
      &nBytes );
   if( nResult != ERROR_SUCCESS )
   {
      return( E_FAIL );
   }

   RegCloseKey( hBitsKey );

   pBitMatcher = new CMIMEBitMatcher;
   if( pBitMatcher == NULL )
   {
      return( E_OUTOFMEMORY );
   }

   hResult = pBitMatcher->InitFromBinary( pData, nBytes, &nBytesToMatch );
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   m_nMaxBytes = max( m_nMaxBytes, nBytesToMatch );

   m_lpBitMatchers = pBitMatcher;

   m_nClipboardFormat = RegisterClipboardFormat( pszName );
   if( m_nClipboardFormat == 0 )
   {
      return( E_FAIL );
   }

   *pnMaxBytes = m_nMaxBytes;

   return( S_OK );
}

HRESULT CMIMEType::Match( const BYTE* pBytes, ULONG nBytes ) const
{
   HRESULT hResult;
   HRESULT hResultSoFar;
   CMIMEBitMatcher* pBitMatcher;

   _ASSERTE( pBytes != NULL );
   _ASSERTE( m_nClipboardFormat != 0 );
   _ASSERTE( m_lpBitMatchers != NULL );

   hResultSoFar = S_FALSE;
   for( pBitMatcher = m_lpBitMatchers; pBitMatcher != NULL; pBitMatcher =
      pBitMatcher->m_pNext )
   {
      hResult = pBitMatcher->Match( pBytes, nBytes );
      switch( hResult )
      {
      case S_OK:
         return( S_OK );
         break;

      case E_PENDING:
         hResultSoFar = E_PENDING;
         break;

      case S_FALSE:
         break;

      default:
         return( hResult );
         break;
      }
   }

   return( hResultSoFar );
}


CMIMEIdentifier::CMIMEIdentifier() :
   m_lpTypes( NULL ),
   m_nMaxBytes( 0 )
{
   memcpy( m_achSignature, "NoLK", 4 );
}

CMIMEIdentifier::~CMIMEIdentifier()
{
   CMIMEType* pType;

   while( m_lpTypes != NULL )
   {
      pType = m_lpTypes;
      m_lpTypes = m_lpTypes->m_pNext;
      delete pType;
   }
}

ULONG CMIMEIdentifier::GetMaxBytes() const
{
   return( m_nMaxBytes );
}

HRESULT CMIMEIdentifier::Identify( const BYTE* pbBytes, ULONG nBytes, 
   UINT* pnFormat )
{
   HRESULT hResultSoFar;
   CMIMEType* pType;
   HRESULT hResult;

   if( pnFormat != NULL )
   {
      *pnFormat = 0;
   }
   if( pbBytes == NULL )
   {
      return( E_INVALIDARG );
   }
   if( pnFormat == NULL )
   {
      return( E_POINTER );
   }

   hResultSoFar = S_FALSE;
   for( pType = m_lpTypes; pType != NULL; pType = pType->m_pNext )
   {
      hResult = pType->Match( pbBytes, nBytes );
      switch( hResult )
      {
      case S_OK:
         *pnFormat = pType->GetClipboardFormat();
         return( S_OK );
         break;

      case E_PENDING:
         hResultSoFar = E_PENDING;
         break;

      case S_FALSE:
         break;

      default:
         return( hResult );
         break;
      }
   }

   return( hResultSoFar );
}

HRESULT CMIMEIdentifier::IdentifyStream( ISniffStream* pSniffStream, 
   UINT* pnFormat )
{
   HRESULT hResult;
   HRESULT hPeekResult;
   BYTE* pbBytes;
   ULONG nBytesRead;
   UINT nFormat;

   if( pnFormat != NULL )
   {
      *pnFormat = 0;
   }
   if( pSniffStream == NULL )
   {
      return( E_INVALIDARG );
   }
   if( pnFormat == NULL )
   {
      return( E_POINTER );
   }

   pbBytes = LPBYTE( _alloca( m_nMaxBytes ) );
   hPeekResult = pSniffStream->Peek( pbBytes, m_nMaxBytes, &nBytesRead );
   if( FAILED( hPeekResult ) && (hPeekResult != E_PENDING) )
   {
      return( hPeekResult );
   }

   hResult = Identify( pbBytes, nBytesRead, &nFormat );
   if( hResult == S_OK )
   {
      *pnFormat = nFormat;
   }
   if( (hResult == E_PENDING) && (hPeekResult == S_FALSE) )
   {
      return( S_FALSE );
   }

   return( hResult );
}

HRESULT CMIMEIdentifier::InitFromRegistry()
{
   LONG nResult;
   HKEY hKey;
   ULONG iSubkey;
   HKEY hSubkey;
   TCHAR szKeyName[MAX_PATH+1];
   ULONG nNameLength;
   FILETIME time;
   BOOL bDone;
   CMIMEType* pType;
   HRESULT hResult;
   ULONG nMaxBytes;

   if( m_lpTypes != NULL )
   {
      return( E_FAIL );
   }

   nResult = RegOpenKeyEx( HKEY_CLASSES_ROOT, MIME_DATABASE_ROOT, 0, KEY_READ,
      &hKey );
   if( nResult != ERROR_SUCCESS )
   {
      return( E_FAIL );
   }

   iSubkey = 0;
   bDone = FALSE;
   while( !bDone )
   {
      nNameLength = sizeof( szKeyName )/sizeof( *szKeyName );
      nResult = RegEnumKeyEx( hKey, iSubkey, szKeyName, &nNameLength, NULL, 
         NULL, NULL, &time );
      if( (nResult != ERROR_SUCCESS) && (nResult != ERROR_NO_MORE_ITEMS) )
      {
         RegCloseKey( hKey );
         return( E_FAIL );
      }
      if( nResult == ERROR_SUCCESS )
      {
         nResult = RegOpenKeyEx( hKey, szKeyName, 0, KEY_READ, &hSubkey );
         if( nResult != ERROR_SUCCESS )
         {
            RegCloseKey( hKey );
            return( E_FAIL );
         }

         pType = new CMIMEType;
         if( pType == NULL )
         {
            return( E_OUTOFMEMORY );
         }
         hResult = pType->InitFromKey( hSubkey, szKeyName, &nMaxBytes );
         if( SUCCEEDED( hResult ) )
         {
            m_nMaxBytes = max( m_nMaxBytes, nMaxBytes );
            pType->m_pNext = m_lpTypes;
            m_lpTypes = pType;
         }
         else
         {
            delete pType;
         }

         RegCloseKey( hSubkey );
      }
      else
      {
         bDone = TRUE;
      }

      iSubkey++;
   }

   return( S_OK );
}
