/*-----------------------------------------------------------------------*/
/* DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE */
/* DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE */
/* DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE */
/* DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE */
/* DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE */
/* DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE */
/* DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE */
/* DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE */
/* DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE */
/* DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE */
/* DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE */
/* DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE */
/* DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE */
/* DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE */
/* DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE */
/* DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE */
/* DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE */
/* DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE */
/* DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE */
/* DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE */
/* DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE */
/* DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE */
/* DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE */
/* DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE */
/* DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE */
/* DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE DEAD FILE */
/*-----------------------------------------------------------------------*/

#include "stdafx.h"
#include "imgutil.h"
#include "resource.h"
#include "csmartbs.h"
#include "align.h"


CSmartBitmapSurface::CSmartBitmapSurface() :
   m_pUnkMarshal( NULL ),
   m_bInitialized( FALSE ),
   m_nLockCount( -1 ),
   m_pBits( NULL ),
   m_pLockedBits( NULL ),
   m_bfidFormat( GUID_NULL ),
   m_nColors( 0 ),
   m_iTransparentIndex( COLOR_NO_TRANSPARENT ),
   m_pFirstCommittedByte( NULL ),
   m_pLastCommittedByte( NULL )
{
}

CSmartBitmapSurface::~CSmartBitmapSurface()
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

   if( m_pBits != NULL )
   {
      VirtualFree( m_pBits, m_nBufferSize, MEM_DECOMMIT );
      VirtualFree( m_pBits, 0, MEM_RELEASE );
   }

   if( m_pUnkMarshal != NULL )
   {
      m_pUnkMarshal->Release();
   }
}

HRESULT CSmartBitmapSurface::FinalConstruct()
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

STDMETHODIMP CSmartBitmapSurface::Init( ULONG nWidth, ULONG nHeight, 
   REFGUID bfid, BOOL bAnimation, IBitmapSurfaceFactory* pFactory )
{
   (void)bAnimation;
   (void)pFactory;

   _ASSERTE( !bAnimation );
   _ASSERTE( pFactory == NULL );

   m_nWidth = nWidth;
   m_nHeight = nHeight;

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

   m_nPitch = AlignLong( ((m_nWidth*m_nBitsPerPixel)+7)/8 );
   m_nBufferSize = m_nPitch*m_nHeight;

   m_pBits = LPBYTE( VirtualAlloc( NULL, m_nBufferSize, MEM_RESERVE,
      PAGE_READWRITE ) );
   if( m_pBits == NULL )
   {
      return( E_OUTOFMEMORY );
   }

   m_pFirstCommittedByte = m_pBits+m_nBufferSize;
   m_pLastCommittedByte = m_pBits;

   m_bInitialized = TRUE;

   return( S_OK );
}


///////////////////////////////////////////////////////////////////////////////
// IBitmapSurface methods
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CSmartBitmapSurface::Clone( IBitmapSurface** ppSurface )
{
   if( ppSurface == NULL )
   {
      return( E_POINTER );
   }

   *ppSurface = NULL;

   return( E_NOTIMPL );
}

STDMETHODIMP CSmartBitmapSurface::GetSize( LONG* pnWidth, LONG* pnHeight )
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

   *pnWidth = m_nWidth;
   *pnHeight = m_nHeight;

   return( S_OK );
}

STDMETHODIMP CSmartBitmapSurface::GetFormat( GUID* pBFID )
{
   if( pBFID == NULL )
   {
      return( E_POINTER );
   }

   *pBFID = m_bfidFormat;

   return( S_OK );
}

STDMETHODIMP CSmartBitmapSurface::GetFactory( 
   IBitmapSurfaceFactory** ppFactory )
{
   if( ppFactory == NULL )
   {
      return( E_POINTER );
   }

   *ppFactory = NULL;

   return( E_NOTIMPL );
}

STDMETHODIMP CSmartBitmapSurface::LockBits( RECT* pBounds, DWORD dwLockFlags, 
   void** ppBits, LONG* pnPitch )
{
   BYTE* pBits;
   BYTE* pFirstCommittedByte;
   BYTE* pLastCommittedByte;
   LONG nLockResult;

   if( ppBits != NULL )
   {
      *ppBits = NULL;
   }
   if( pnPitch != NULL )
   {
      *pnPitch = 0;
   }
   if( (ppBits == NULL) || (pnPitch == NULL) )
   {
      return( E_POINTER );
   }

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
      if( (m_pFirstCommittedByte > m_pBits) || (m_pLastCommittedByte <
         (m_pBits+m_nBufferSize) ) )
      {
         pBits = LPBYTE( VirtualAlloc( m_pBits, m_nBufferSize, MEM_COMMIT, 
            PAGE_READWRITE ) );
         if( pBits == NULL )
         {
            m_csLock.Unlock();
            InterlockedDecrement( &m_nLockCount );
            return( E_OUTOFMEMORY );
         }
         m_pFirstCommittedByte = m_pBits;
         m_pLastCommittedByte = m_pBits+m_nBufferSize;
      }
      m_pLockedBits = m_pBits;
   }
   else
   {
      pFirstCommittedByte = m_pBits+(pBounds->top*m_nPitch)+
         ((pBounds->left*m_nBitsPerPixel)/8);
      pLastCommittedByte = m_pBits+((pBounds->bottom-1)*m_nPitch)+
         ((pBounds->right*m_nBitsPerPixel)/8);
      if( (pFirstCommittedByte < m_pFirstCommittedByte) || 
         (pLastCommittedByte > m_pLastCommittedByte) )
      {
         pBits = LPBYTE( VirtualAlloc( pFirstCommittedByte, pLastCommittedByte-
            pFirstCommittedByte, MEM_COMMIT, PAGE_READWRITE ) );
         if( pBits == NULL )
         {
            m_csLock.Unlock();
            InterlockedDecrement( &m_nLockCount );
            return( E_OUTOFMEMORY );
         }
         m_pFirstCommittedByte = min( pFirstCommittedByte, 
            m_pFirstCommittedByte );
         m_pLastCommittedByte = max( pLastCommittedByte, 
            m_pLastCommittedByte );
      }
      m_pLockedBits = pFirstCommittedByte;
   }
   m_pLockedBounds = pBounds;
   *ppBits = m_pLockedBits;
   *pnPitch = m_nPitch;

   return( S_OK );
}

STDMETHODIMP CSmartBitmapSurface::UnlockBits( RECT* pBounds, void* pBits )
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

STDMETHODIMP CSmartBitmapSurface::GetColors( LONG iFirst, LONG nCount,
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

STDMETHODIMP CSmartBitmapSurface::GetCount( LONG* pnCount )
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

STDMETHODIMP CSmartBitmapSurface::SetCount( LONG nCount )
{
    nCount = nCount;
    return E_NOTIMPL;
}

STDMETHODIMP CSmartBitmapSurface::GetTransparentIndex( LONG* piIndex )
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

STDMETHODIMP CSmartBitmapSurface::SetColors( LONG iFirst, LONG nCount,
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

STDMETHODIMP CSmartBitmapSurface::SetTransparentIndex( LONG iIndex )
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

HRESULT WINAPI CSmartBitmapSurface::GetIRGBColorTable( void* pv, REFIID iid,
   void** ppv, DWORD dw )
{
   IUnknown* pUnk;

   (void)iid;
   (void)dw;

   _ASSERTE( pv != NULL );
   _ASSERTE( ppv != NULL );
   _ASSERTE( IsEqualIID( iid, IID_IRGBColorTable ) );

   if( ((CSmartBitmapSurface*)pv)->m_nColors == 0 )
   {
      *ppv = NULL;
      return( E_NOINTERFACE );
   }

   *ppv = (IRGBColorTable*)(CSmartBitmapSurface*)pv;
   pUnk = (IUnknown*)(*ppv);
   pUnk->AddRef();

   return( S_OK );
}

