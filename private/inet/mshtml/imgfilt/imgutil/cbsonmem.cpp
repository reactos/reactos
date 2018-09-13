#include "stdafx.h"
#include "imgutil.h"
#include "resource.h"
#include "cbsonmem.h"
#include "align.h"


CBitmapSurfaceOnMemory::CBitmapSurfaceOnMemory() :
   m_pUnkMarshal( NULL ),
   m_bInitialized( FALSE ),
   m_nLockCount( -1 ),
   m_pBits( NULL ),
   m_pLockedBits( NULL ),
   m_bfidFormat( GUID_NULL ),
   m_nColors( 0 ),
   m_iTransparentIndex( COLOR_NO_TRANSPARENT )
{
}

CBitmapSurfaceOnMemory::~CBitmapSurfaceOnMemory()
{
   // On final release, the lock count should be -1 if the surface isn't
   // locked, or 0 if it is.  If it's higher, it means that someone is using
   // the surface after it's been released.
   _ASSERTE( m_nLockCount <= 0 );

   if( m_nLockCount >= 0 )
   {
      // The surface was locked when it was released.
      m_csLock.Unlock();
   }

   if( m_pUnkMarshal != NULL )
   {
      m_pUnkMarshal->Release();
   }
}

HRESULT CBitmapSurfaceOnMemory::FinalConstruct()
{
   HRESULT hResult;

   hResult = CoCreateFreeThreadedMarshaler( GetControllingUnknown(), 
      &m_pUnkMarshal );
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   return( S_OK );
}

STDMETHODIMP CBitmapSurfaceOnMemory::Init( ULONG nWidth, ULONG nHeight, 
   REFGUID bfid, void* pBits, LONG nPitch )
{
   m_nWidth = nWidth;
   m_nHeight = nHeight;
   m_nPitch = nPitch;
   m_pBits = LPBYTE( pBits );

   m_bfidFormat = bfid;
   if( InlineIsEqualGUID( m_bfidFormat, BFID_INDEXED_RGB_8 ) )
   {
      m_nBitsPerPixel = 8;
      m_nColors = 256;
   }
   else if( InlineIsEqualGUID( m_bfidFormat, BFID_RGB_24 ) )
   {
      m_nBitsPerPixel = 24;
      m_nColors = 0;
   }

   m_bInitialized = TRUE;

   return( S_OK );
}


///////////////////////////////////////////////////////////////////////////////
// IBitmapSurface methods
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBitmapSurfaceOnMemory::Clone( IBitmapSurface** ppSurface )
{
   if( ppSurface == NULL )
   {
      return( E_POINTER );
   }

   *ppSurface = NULL;

   return( E_NOTIMPL );
}

STDMETHODIMP CBitmapSurfaceOnMemory::GetSize( LONG* pnWidth, LONG* pnHeight )
{
   if( pnWidth != NULL )
   {
      *pnWidth = 0;
   }
   if( pnHeight != NULL )
   {
      *pnHeight = 0;
   }
   if( (pnWidth == NULL) || (pnHeight == NULL) )
   {
      return( E_POINTER );
   }

   // No critical section lock, since this data is read-only
   *pnWidth = m_nWidth;
   *pnHeight = m_nHeight;

   return( S_OK );
}

STDMETHODIMP CBitmapSurfaceOnMemory::GetFormat( GUID* pBFID )
{
   if( pBFID == NULL )
   {
      return( E_POINTER );
   }

   // No critical section lock, since this data is read-only
   *pBFID = m_bfidFormat;

   return( S_OK );
}

STDMETHODIMP CBitmapSurfaceOnMemory::GetFactory(
   IBitmapSurfaceFactory** ppFactory )
{
   if( ppFactory == NULL )
   {
      return( E_POINTER );
   }

   *ppFactory = NULL;

   return( E_NOTIMPL );
}

STDMETHODIMP CBitmapSurfaceOnMemory::LockBits( RECT* pBounds, 
   DWORD dwLockFlags, void** ppBits, LONG* pnPitch )
{
   LONG nLockResult;

   (void)dwLockFlags;

   if( (ppBits == NULL) || (pnPitch == NULL) )
   {
      return( E_POINTER );
   }

   *ppBits = NULL;
   *pnPitch = NULL;

   if( pBounds != NULL )
   {
      if( (pBounds->left < 0) || (pBounds->top < 0) || 
         (pBounds->right > m_nWidth) || (pBounds->bottom > m_nHeight) )
      {
         return( E_INVALIDARG );
      }
   }

   nLockResult = InterlockedIncrement( &m_nLockCount );
   if( (nLockResult > 0) && !(dwLockFlags & SURFACE_LOCK_WAIT) )
   {
      // We can't lock right now.  Return an error.
      InterlockedDecrement( &m_nLockCount );
      return( E_FAIL );
   }

   m_csLock.Lock();

   if( m_pLockedBits != NULL )
   {
      // Multiple locks not allowed
      m_csLock.Unlock();
      InterlockedDecrement( &m_nLockCount );
      return( E_FAIL );
   }

   _ASSERTE( m_pBits != NULL );

   if( pBounds == NULL )
   {
      m_pLockedBits = m_pBits;
   }
   else
   {
      m_pLockedBits = m_pBits+(pBounds->top*m_nPitch)+
         ((pBounds->left*m_nBitsPerPixel)/8);
   }
   m_pLockedBounds = pBounds;
   *ppBits = m_pLockedBits;
   *pnPitch = m_nPitch;

   return( S_OK );
}

STDMETHODIMP CBitmapSurfaceOnMemory::UnlockBits( RECT* pBounds, void* pBits )
{
   if( pBounds != m_pLockedBounds )
   {
      return( E_INVALIDARG );
   }
   if( pBits != m_pLockedBits )
   {
      return( E_INVALIDARG );
   }

   m_pLockedBounds = NULL;
   m_pLockedBits = NULL;

   m_csLock.Unlock();
   InterlockedDecrement( &m_nLockCount );

   return( S_OK );
}


///////////////////////////////////////////////////////////////////////////////
// IRGBColorTable methods
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBitmapSurfaceOnMemory::GetColors( LONG iFirst, LONG nCount,
   RGBQUAD* pColors )
{
   if( (iFirst >= m_nColors) || (iFirst < 0) )
   {
      return( E_INVALIDARG );
   }
   if( nCount < 0 )
   {
      return( E_INVALIDARG );
   }
   if( (iFirst+nCount) > m_nColors )
   {
      return( E_INVALIDARG );
   }
   if( nCount == 0 )
   {
      return( S_OK );
   }
   if( pColors == NULL )
   {
      return( E_POINTER );
   }

   m_cs.Lock();

   memcpy( pColors, &m_argbColors[iFirst], nCount*sizeof( RGBQUAD ) );

   m_cs.Unlock();

   return( S_OK );
}

STDMETHODIMP CBitmapSurfaceOnMemory::GetCount( LONG* pnCount )
{
   if( pnCount == NULL )
   {
      return( E_POINTER );
   }

   m_cs.Lock();

   *pnCount = m_nColors;

   m_cs.Unlock();

   return( S_OK );
}

STDMETHODIMP CBitmapSurfaceOnMemory::SetCount( LONG nCount )
{
    nCount = nCount;
    return E_NOTIMPL;
}

STDMETHODIMP CBitmapSurfaceOnMemory::GetTransparentIndex( LONG* piIndex )
{
   if( piIndex == NULL )
   {
      return( E_POINTER );
   }

   m_cs.Lock();

   *piIndex = m_iTransparentIndex;

   m_cs.Unlock();

   return( S_OK );
}

STDMETHODIMP CBitmapSurfaceOnMemory::SetColors( LONG iFirst, LONG nCount,
   RGBQUAD* pColors )
{
   if( (iFirst >= m_nColors) || (iFirst < 0) )
   {
      return( E_INVALIDARG );
   }
   if( nCount < 0 )
   {
      return( E_INVALIDARG );
   }
   if( (iFirst+nCount) > m_nColors )
   {
      return( E_INVALIDARG );
   }
   if( nCount == 0 )
   {
      return( S_OK );
   }
   if( pColors == NULL )
   {
      return( E_POINTER );
   }

   m_cs.Lock();

   memcpy( &m_argbColors[iFirst], pColors, nCount*sizeof( RGBQUAD ) );

   m_cs.Unlock();

   return( S_OK );
}

STDMETHODIMP CBitmapSurfaceOnMemory::SetTransparentIndex( LONG iIndex )
{
   if( (iIndex >= m_nColors) && (iIndex != COLOR_NO_TRANSPARENT) )
   {
      return( E_INVALIDARG );
   }

   m_cs.Lock();

   m_iTransparentIndex = iIndex;

   m_cs.Unlock();

   return( S_OK );
}

HRESULT WINAPI CBitmapSurfaceOnMemory::GetIRGBColorTable( void* pv, REFIID iid,
   void** ppv, DWORD dw )
{
   IUnknown* pUnk;

   (void)iid;
   (void)dw;

   _ASSERTE( pv != NULL );
   _ASSERTE( ppv != NULL );
   _ASSERTE( IsEqualIID( iid, IID_IRGBColorTable ) );

   if( ((CBitmapSurfaceOnMemory*)pv)->m_nColors == 0 )
   {
      *ppv = NULL;
      return( E_NOINTERFACE );
   }

   *ppv = (IRGBColorTable*)(CBitmapSurfaceOnMemory*)pv;
   pUnk = (IUnknown*)(*ppv);
   pUnk->AddRef();

   return( S_OK );
}

