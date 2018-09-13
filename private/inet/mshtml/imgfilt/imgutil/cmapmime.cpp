#include "stdafx.h"
#include "imgutil.h"
#include "resource.h"
#include "cmapmime.h"

CMapStringToCLSID* g_pDefaultMappings = NULL;
BOOL g_bDefaultMappingsInitialized = FALSE;
CRITICAL_SECTION g_csDefaultMappings;
const LPCTSTR MIME_DATABASE_ROOT = _T( "MIME\\Database\\Content Type" );

int UNICODEstrlen(LPCTSTR psz)
{
    const TCHAR *p;

    for (p = psz; *p; ++p)
        ;

    return (int)(p - psz);
}

HRESULT InitDefaultMappings()
{
   HRESULT hResult;
   LONG nResult;
   HKEY hKey;
   ULONG iSubkey;
   HKEY hSubkey;
   BOOL bDone;
   ULONG nNameLength;
   TCHAR szKeyName[MAX_PATH+1];
   CMapStringToCLSID* pMapping;
   FILETIME time;

   InitializeCriticalSection( &g_csDefaultMappings );
   
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

         pMapping = new CMapStringToCLSID;
         if( pMapping == NULL )
         {
            return( E_OUTOFMEMORY );
         }
         hResult = pMapping->InitFromKey( hSubkey, szKeyName );
         if( SUCCEEDED( hResult ) )
         {
            pMapping->m_pNext = g_pDefaultMappings;
            g_pDefaultMappings = pMapping;
         }
         else
         {
            delete pMapping;
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

void CleanupDefaultMappings()
{
   CMapStringToCLSID* pKill;

   EnterCriticalSection( &g_csDefaultMappings );

   while( g_pDefaultMappings != NULL )
   {
      pKill = g_pDefaultMappings;
      g_pDefaultMappings = g_pDefaultMappings->m_pNext;
      delete pKill;
   }

   LeaveCriticalSection( &g_csDefaultMappings );

   DeleteCriticalSection( &g_csDefaultMappings );
}

static HRESULT DefaultMapMIMEToCLSID( LPCTSTR pszMIMEType, CLSID* pCLSID )
{
   CMapStringToCLSID* pTrav;
   BOOL bFound;

   _ASSERTE( pszMIMEType != NULL );
   _ASSERTE( pCLSID != NULL );

   *pCLSID = CLSID_NULL;

   EnterCriticalSection( &g_csDefaultMappings );

   bFound = FALSE;
   pTrav = g_pDefaultMappings;
   while( (pTrav != NULL) && !bFound )
   {
      if( lstrcmp( pszMIMEType, pTrav->GetString() ) == 0 )
      {
         *pCLSID = pTrav->GetCLSID();
         bFound = TRUE;
      }
      pTrav = pTrav->m_pNext;
   }

   LeaveCriticalSection( &g_csDefaultMappings );

   if( IsEqualCLSID( *pCLSID, CLSID_NULL ) )
   {
      return( S_FALSE );
   }
   else
   {
      return( S_OK );
   }
}

CMapStringToCLSID::CMapStringToCLSID() :
   m_pNext( NULL ),
   m_pszString( NULL ),
   m_clsid( CLSID_NULL ),
   m_dwMapMode( MAPMIME_DEFAULT )
{
   memcpy( m_achSignature, "NoLK", 4 );
}

CMapStringToCLSID::~CMapStringToCLSID()
{
   delete m_pszString;
}

const CLSID& CMapStringToCLSID::GetCLSID() const
{
   return( m_clsid );
}

DWORD CMapStringToCLSID::GetMapMode() const
{
   return( m_dwMapMode );
}

LPCTSTR CMapStringToCLSID::GetString() const
{
   _ASSERTE( m_pszString != NULL );

   return( m_pszString );
}

HRESULT CMapStringToCLSID::InitFromKey( HKEY hKey, LPCTSTR pszKeyName )
{
   LONG nResult;
   HRESULT hResult;
   DWORD dwValueType;
   BYTE* pData;
   ULONG nBytes;
   CLSID clsid;
   USES_CONVERSION;

   nResult = RegQueryValueEx( hKey, _T( "Image Filter CLSID" ), NULL, 
      &dwValueType, NULL, &nBytes );
   if( (nResult != ERROR_SUCCESS) || (dwValueType != REG_SZ) || nBytes > 8192 )
   {
      return( E_FAIL );
   }
   pData = LPBYTE( _alloca( nBytes ) );

   nResult = RegQueryValueEx( hKey, _T( "Image Filter CLSID" ), NULL, 
      &dwValueType, pData, &nBytes );
   if( nResult != ERROR_SUCCESS )
   {
      return( E_FAIL );
   }

   hResult = SetString( pszKeyName );
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   CLSIDFromString( T2OLE( LPTSTR( pData ) ), &clsid );
   SetCLSID( clsid );

   return( S_OK );
}

void CMapStringToCLSID::SetCLSID( REFGUID clsid )
{
   m_clsid = clsid;
}

void CMapStringToCLSID::SetMapMode( DWORD dwMapMode )
{
   m_dwMapMode = dwMapMode;
}

HRESULT CMapStringToCLSID::SetString( LPCTSTR pszString )
{
   _ASSERTE( m_pszString == NULL );

   m_pszString = new TCHAR[UNICODEstrlen( pszString )+1];
   if( m_pszString == NULL )
   {
      return( E_OUTOFMEMORY );
   }

   lstrcpy( m_pszString, pszString );

   return( S_OK );
}

CMapMIMEToCLSID::CMapMIMEToCLSID() :
   m_nMappings( 0 ),
   m_pMappings( NULL ),
   m_bEnableDefaultMappings( TRUE )
{
}

CMapMIMEToCLSID::~CMapMIMEToCLSID()
{
   CMapStringToCLSID* pKill;

   while( m_pMappings != NULL )
   {
      pKill = m_pMappings;
      m_pMappings = m_pMappings->m_pNext;
      delete pKill;
   }
}

CMapStringToCLSID* CMapMIMEToCLSID::FindMapping( LPCTSTR pszMIMEType )
{
   CMapStringToCLSID* pMapping;

   pMapping = m_pMappings;
   while( pMapping != NULL )
   {
      if( lstrcmp( pMapping->GetString(), pszMIMEType ) == 0 )
      {
         return( pMapping );
      }
      pMapping = pMapping->m_pNext;
   }

   return( NULL );
}

CMapStringToCLSID* CMapMIMEToCLSID::AddMapping( LPCTSTR pszMIMEType )
{
   CMapStringToCLSID* pMapping;
   HRESULT hResult;

   pMapping = new CMapStringToCLSID;
   if( pMapping == NULL )
   {
      return( NULL );
   }
   
   hResult = pMapping->SetString( pszMIMEType );
   if( FAILED( hResult ) )
   {
      delete pMapping;
      return( NULL );
   }

   pMapping->m_pNext = m_pMappings;
   m_pMappings = pMapping;
   m_nMappings++;

   return( pMapping );
}

void CMapMIMEToCLSID::DeleteMapping( LPCTSTR pszMIMEType )
{
   CMapStringToCLSID* pMapping;
   CMapStringToCLSID* pFollow;

   pFollow = NULL;
   pMapping = m_pMappings;
   while( pMapping != NULL )
   {
      if( lstrcmp( pMapping->GetString(), pszMIMEType ) == 0 )
      {
         if( pFollow == NULL )
         {
            m_pMappings = pMapping->m_pNext;
         }
         else
         {
            pFollow->m_pNext = pMapping->m_pNext;
         }
         delete pMapping;
         m_nMappings--;
         return;
      }
      pFollow = pMapping;
      pMapping = pMapping->m_pNext;
   }
}

STDMETHODIMP CMapMIMEToCLSID::SetMapping( LPCOLESTR pszMIMEType, 
   DWORD dwMapMode, REFGUID clsid )
{
   USES_CONVERSION;
   CMapStringToCLSID* pMapping;
   LPCTSTR pszMIMETypeT;

   if( pszMIMEType == NULL )
   {
      return( E_INVALIDARG );
   }
   if( dwMapMode > MAPMIME_DEFAULT_ALWAYS )
   {
      return( E_INVALIDARG );
   }

   pszMIMETypeT = OLE2CT( pszMIMEType );

   if( dwMapMode == MAPMIME_DEFAULT )
   {
      DeleteMapping( pszMIMETypeT );
   }
   else
   {
      pMapping = FindMapping( pszMIMETypeT );
      if( pMapping == NULL )
      {
         pMapping = AddMapping( pszMIMETypeT );
         if( pMapping == NULL )
         {
            return( E_OUTOFMEMORY );
         }
      }
      pMapping->SetMapMode( dwMapMode );
      if( dwMapMode == MAPMIME_CLSID )
      {
         pMapping->SetCLSID( clsid );
      }
   }

   return( S_OK );
}

STDMETHODIMP CMapMIMEToCLSID::EnableDefaultMappings( BOOL bEnable )
{
   m_bEnableDefaultMappings = bEnable;

   return( S_OK );
}

STDMETHODIMP CMapMIMEToCLSID::MapMIMEToCLSID( LPCOLESTR pszMIMEType, 
   GUID* pCLSID )
{
   USES_CONVERSION;
   LPCTSTR pszMIMETypeT;
   DWORD dwMapMode;
   GUID clsid = CLSID_NULL;
   CMapStringToCLSID* pMapping;
   HRESULT hResult;

   if( pCLSID != NULL )
   {
      *pCLSID = CLSID_NULL;
   }
   if( pszMIMEType == NULL )
   {
      return( E_INVALIDARG );
   }
   if( pCLSID == NULL )
   {
      return( E_POINTER );
   }

   pszMIMETypeT = OLE2CT( pszMIMEType );

   pMapping = FindMapping( pszMIMETypeT );
   if( pMapping != NULL )
   {
      dwMapMode = pMapping->GetMapMode();
      if( dwMapMode == MAPMIME_CLSID )
      {
         clsid = pMapping->GetCLSID();
      }
   }
   else
   {
      dwMapMode = MAPMIME_DEFAULT;
   }

   switch( dwMapMode )
   {
   case MAPMIME_DEFAULT:
      if( m_bEnableDefaultMappings )
      {
         hResult = DefaultMapMIMEToCLSID( pszMIMETypeT, &clsid );
      }
      else
      {
         hResult = S_FALSE;
      }
      break;

   case MAPMIME_DEFAULT_ALWAYS:
      hResult = DefaultMapMIMEToCLSID( pszMIMETypeT, &clsid );
      break;

   case MAPMIME_CLSID:
      hResult = S_OK;
      break;

   case MAPMIME_DISABLE:
      hResult = S_FALSE;
      break;

   default:
      _ASSERT( FALSE );
      hResult = E_FAIL;
      break;
   }

   if( hResult == S_OK )
   {
      *pCLSID = clsid;
   }

   return( hResult );
}

