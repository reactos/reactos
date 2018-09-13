#include "stdafx.h"
#include "imgutil.h"
#include "resource.h"
#include "cbmsurf.h"
#include "align.h"


CBitmapSurface::CBitmapSurface() :
   m_pUnkMarshal( NULL ),
   m_bInitialized( FALSE ),
   m_pFactory( NULL ),
   m_hBitmap( NULL ),
   m_pBits( NULL ),
   m_pLockedBits( NULL ),
   m_bfidFormat( GUID_NULL ),
   m_nColors( 0 ),
   m_iTransparentIndex( COLOR_NO_TRANSPARENT ),
   m_pBMI( NULL ),
   m_hDC( NULL ),
   m_hOldObject( NULL ),
   m_bAnimation( FALSE ),
   m_ppFrames( NULL ),
   m_nFrames( 1 )
{
   memcpy( m_achSignature, "BmpS", 4 );
}

CBitmapSurface::~CBitmapSurface()
{
   ULONG iFrame;

   m_cs.Lock();

   for( iFrame = 1; iFrame < m_nFrames; iFrame++ )
   {
      m_ppFrames[iFrame]->Release();
   }
   free( m_ppFrames );

   free( m_pBMI );

   if( m_hBitmap != NULL )
   {
      DeleteObject( m_hBitmap );
   }

   m_cs.Unlock();

   if( m_pUnkMarshal != NULL )
   {
      m_pUnkMarshal->Release();
   }
}

HRESULT CBitmapSurface::FinalConstruct()
{
   HRESULT hResult;

   hResult = CoCreateFreeThreadedMarshaler( GetControllingUnknown(), 
      &m_pUnkMarshal );
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   m_ppFrames = (IBitmapSurface**)malloc( sizeof( IBitmapSurface* ) );
   if( m_ppFrames == NULL )
   {
      return( E_OUTOFMEMORY );
   }
   m_ppFrames[0] = NULL;

   return( S_OK );
}

STDMETHODIMP CBitmapSurface::Init( ULONG nWidth, ULONG nHeight, REFGUID bfid, 
   BOOL bAnimation, IBitmapSurfaceFactory* pFactory )
{
   m_bAnimation = bAnimation;

   m_nWidth = nWidth;
   m_nHeight = nHeight;

   m_bfidFormat = bfid;
   if( InlineIsEqualGUID( m_bfidFormat, BFID_INDEXED_RGB_8 ) )
   {
      m_nBitsPerPixel = 8;
      m_nColors = 256;
   }
   else if( InlineIsEqualGUID( m_bfidFormat, BFID_INDEXED_RGB_4 ) )
   {
      m_nBitsPerPixel = 4;
      m_nColors = 16;
   }
   else if( InlineIsEqualGUID( m_bfidFormat, BFID_INDEXED_RGB_1 ) )
   {
      m_nBitsPerPixel = 1;
      m_nColors = 2;
   }
   else if( InlineIsEqualGUID( m_bfidFormat, BFID_RGB_32 ) )
   {
      m_nBitsPerPixel = 32;
      m_nColors = 0;
   }
   else if( InlineIsEqualGUID( m_bfidFormat, BFID_RGB_24 ) )
   {
      m_nBitsPerPixel = 24;
      m_nColors = 0;
   }
   else if( InlineIsEqualGUID( m_bfidFormat, BFID_RGB_555 ) )
   {
      m_nBitsPerPixel = 16;
      m_nColors = 0;
   }
   else if( InlineIsEqualGUID( m_bfidFormat, BFID_RGBA_32 ) )
   {
      m_nBitsPerPixel = 32;
      m_nColors = 0;
   }

   m_nPitch = AlignLong( ((m_nWidth*m_nBitsPerPixel)+7)/8 );

   m_pFactory = pFactory;

   m_pBMI = (BITMAPINFO*)malloc( sizeof( BITMAPINFOHEADER )+
      m_nColors*sizeof( RGBQUAD ) );
   if( m_pBMI == NULL )
   {
      return( E_OUTOFMEMORY );
   }

   memset( &m_pBMI->bmiHeader, 0, sizeof( m_pBMI->bmiHeader ) );
   m_pBMI->bmiHeader.biSize = sizeof( m_pBMI->bmiHeader );
   m_pBMI->bmiHeader.biWidth = m_nWidth;
   m_pBMI->bmiHeader.biHeight = -m_nHeight;
   m_pBMI->bmiHeader.biPlanes = 1;
   m_pBMI->bmiHeader.biBitCount = WORD( m_nBitsPerPixel );
   m_pBMI->bmiHeader.biCompression = BI_RGB;

   m_hBitmap = CreateDIBSection( NULL, m_pBMI, DIB_RGB_COLORS, 
      (void**)&m_pBits, NULL, 0 );
   if( m_hBitmap == NULL )
   {
      return( E_OUTOFMEMORY );
   }

   m_bInitialized = TRUE;

   return( S_OK );
}


///////////////////////////////////////////////////////////////////////////////
// IBitmapSurface methods
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBitmapSurface::Clone( IBitmapSurface** ppSurface )
{
   if( ppSurface == NULL )
   {
      return( E_POINTER );
   }

   *ppSurface = NULL;

   return( E_NOTIMPL );
}

STDMETHODIMP CBitmapSurface::GetSize( LONG* pnWidth, LONG* pnHeight )
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

STDMETHODIMP CBitmapSurface::GetFormat( GUID* pBFID )
{
   if( pBFID == NULL )
   {
      return( E_POINTER );
   }

   *pBFID = m_bfidFormat;

   return( S_OK );
}

STDMETHODIMP CBitmapSurface::GetFactory( IBitmapSurfaceFactory** ppFactory )
{
   if( ppFactory == NULL )
   {
      return( E_POINTER );
   }

   *ppFactory = m_pFactory;
   (*ppFactory)->AddRef();

   return( S_OK );
}

STDMETHODIMP CBitmapSurface::LockBits( RECT* pBounds, DWORD dwLockFlags,
   void** ppBits, LONG* pnPitch )
{
   (void)dwLockFlags;

   if( (ppBits == NULL) || (pnPitch == NULL) )
   {
      return( E_POINTER );
   }

   *ppBits = NULL;
   *pnPitch = NULL;

   m_cs.Lock();

   if( m_pBits == NULL )
   {
      m_cs.Unlock();
      return( E_FAIL );
   }

   if( m_pLockedBits != NULL )
   {
      m_cs.Unlock();
      return( E_FAIL );
   }

   if( pBounds == NULL )
   {
      m_pLockedBits = m_pBits;
   }
   else
   {
      if( (pBounds->left < 0) || (pBounds->top < 0) || 
         (pBounds->right > m_nWidth) || (pBounds->bottom > m_nHeight) )
      {
         m_cs.Unlock();
         return( E_INVALIDARG );
      }
      m_pLockedBits = m_pBits+(pBounds->top*m_nPitch)+
         ((pBounds->left*m_nBitsPerPixel)/8);
   }
   m_pLockedBounds = pBounds;
   *ppBits = m_pLockedBits;
   *pnPitch = m_nPitch;

   return( S_OK );
}

STDMETHODIMP CBitmapSurface::UnlockBits( RECT* pBounds, void* pBits )
{
   if( pBounds != m_pLockedBounds )
   {
      return( E_INVALIDARG );
   }
   if( pBits == NULL )
   {
      return( E_INVALIDARG );
   }
   if( pBits != m_pLockedBits )
   {
      return( E_INVALIDARG );
   }

   m_pLockedBounds = NULL;
   m_pLockedBits = NULL;

   m_cs.Unlock();

   return( S_OK );
}


///////////////////////////////////////////////////////////////////////////////
// IGdiSurface methods
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBitmapSurface::GetDC( HDC* phDC, DWORD dwLockFlags )
{
   (void)dwLockFlags;

   if( phDC == NULL )
   {
      return( E_POINTER );
   }

   *phDC = NULL;

   m_cs.Lock();

   if( m_hDC != NULL )
   {
      m_cs.Unlock();
      return( E_FAIL );
   }

   m_hDC = CreateCompatibleDC( NULL );
   if( m_hDC == NULL )
   {
      m_cs.Unlock();
      return( E_OUTOFMEMORY );
   }

   m_hOldObject = SelectObject( m_hDC, m_hBitmap );

   *phDC = m_hDC;

   return( S_OK );
}

STDMETHODIMP CBitmapSurface::ReleaseDC( HDC hDC )
{
   if( hDC == NULL )
   {
      return( E_INVALIDARG );
   }
   if( hDC != m_hDC )
   {
      return( E_INVALIDARG );
   }

   SelectObject( m_hDC, m_hOldObject );
   m_hOldObject = NULL;

   DeleteDC( m_hDC );
   m_hDC = NULL;

   m_cs.Unlock();

   return( S_OK );
}


///////////////////////////////////////////////////////////////////////////////
// IRGBColorTable methods
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBitmapSurface::GetColors( LONG iFirst, LONG nCount,
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

   memcpy( pColors, &m_pBMI->bmiColors[iFirst], nCount*sizeof( RGBQUAD ) );

   m_cs.Unlock();

   return( S_OK );
}

STDMETHODIMP CBitmapSurface::GetCount( LONG* pnCount )
{
   if( pnCount == NULL )
   {
      return( E_POINTER );
   }

   *pnCount = m_nColors;

   return( S_OK );
}

STDMETHODIMP CBitmapSurface::SetCount( LONG nCount )
{
    nCount = nCount;
    return E_NOTIMPL;
}

STDMETHODIMP CBitmapSurface::GetTransparentIndex( LONG* piIndex )
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

STDMETHODIMP CBitmapSurface::SetColors( LONG iFirst, LONG nCount,
   RGBQUAD* pColors )
{
   HDC hDC;
   HGDIOBJ hOldObject;

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

   memcpy( &m_pBMI->bmiColors[iFirst], pColors, nCount*sizeof( RGBQUAD ) );

   hDC = m_hDC;
   hOldObject = NULL;

   if( hDC == NULL )
   {
      hDC = CreateCompatibleDC( NULL );
      if( hDC == NULL )
      {
         return( E_OUTOFMEMORY );
      }
      hOldObject = SelectObject( hDC, m_hBitmap );
   }

   SetDIBColorTable( hDC, iFirst, nCount, pColors );

   if( m_hDC == NULL )
   {
      SelectObject( hDC, hOldObject );
      DeleteDC( hDC );
   }

   m_cs.Unlock();

   return( S_OK );
}

STDMETHODIMP CBitmapSurface::SetTransparentIndex( LONG iIndex )
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


///////////////////////////////////////////////////////////////////////////////
// IAnimation methods
///////////////////////////////////////////////////////////////////////////////
/*
STDMETHODIMP CBitmapSurface::AddFrames( ULONG nFrames )
{
   HRESULT hResult;
   ULONG iFrame;
   IBitmapSurface** ppNewFrames;
   ULONG nNewFrames;
   CComPtr< IBounds > pBounds;

   if( nFrames == 0 )
   {
      return( S_OK );
   }

   hResult = CComCreator< CComObject< CBounds > >::CreateInstance( NULL,
      IID_IBounds, (void**)&pBounds );
   if( FAILED( hResult ) )
   {
      return( hResult );
   }
   pBounds->SetRect( 0, 0, m_nWidth, m_nHeight );

   m_cs.Lock();

   nNewFrames = m_nFrames+nFrames;
   ppNewFrames = (IBitmapSurface**)realloc( m_ppFrames, nNewFrames*
      sizeof( IBitmapSurface* ) );
   if( ppNewFrames == NULL )
   {
      m_cs.Unlock();
      return( E_OUTOFMEMORY );
   }
   m_ppFrames = ppNewFrames;

   for( iFrame = m_nFrames; iFrame < nNewFrames; iFrame++ )
   {
      hResult = m_pFactory->CreateBitmapSurface( pBounds, m_bfidFormat, 0, 
         &m_ppFrames[iFrame] );
      if( FAILED( hResult ) )
      {
         m_nFrames = iFrame;
         m_cs.Unlock();
         return( hResult );
      }
   }

   m_nFrames = nNewFrames;

   m_cs.Unlock();

   return( S_OK );
}

STDMETHODIMP CBitmapSurface::GetFrame( ULONG iFrame, 
   IBitmapSurface** ppSurface )
{
   if( ppSurface == NULL )
   {
      return( E_POINTER );
   }
   *ppSurface = NULL;

   m_cs.Lock();

   if( iFrame >= m_nFrames )
   {
      m_cs.Unlock();
      return( E_INVALIDARG );
   }

   if( iFrame == 0 )
   {
      GetControllingUnknown()->QueryInterface( IID_IBitmapSurface, 
         (void**)ppSurface );
   }
   else
   {
      *ppSurface = m_ppFrames[iFrame];
      _ASSERTE( *ppSurface != NULL );
      (*ppSurface)->AddRef();
   }

   m_cs.Unlock();

   return( S_OK );
}

STDMETHODIMP CBitmapSurface::GetNumFrames( ULONG* pnFrames )
{
   if( pnFrames == NULL )
   {
      return( E_POINTER );
   }

   m_cs.Lock();

   *pnFrames = m_nFrames;

   m_cs.Unlock();

   return( S_OK );
}

HRESULT WINAPI CBitmapSurface::GetIAnimation( void* pv, REFIID iid, void** ppv,
   DWORD dw )
{
   IUnknown* pUnk;

   (void)iid;
   (void)dw;

   _ASSERTE( pv != NULL );
   _ASSERTE( ppv != NULL );
   _ASSERTE( IsEqualIID( iid, IID_IAnimation ) );

   if( !((CBitmapSurface*)pv)->m_bAnimation )
   {
      *ppv = NULL;
      return( E_NOINTERFACE );
   }

   *ppv = (IAnimation*)(CBitmapSurface*)pv;
   pUnk = (IUnknown*)(*ppv);
   pUnk->AddRef();

   return( S_OK );
}
*/

HRESULT WINAPI CBitmapSurface::GetIRGBColorTable( void* pv, REFIID iid,
   void** ppv, DWORD dw )
{
   IUnknown* pUnk;

   (void)iid;
   (void)dw;

   _ASSERTE( pv != NULL );
   _ASSERTE( ppv != NULL );
   _ASSERTE( IsEqualIID( iid, IID_IRGBColorTable ) );

   if( ((CBitmapSurface*)pv)->m_nColors == 0 )
   {
      *ppv = NULL;
      return( E_NOINTERFACE );
   }

   *ppv = (IRGBColorTable*)(CBitmapSurface*)pv;
   pUnk = (IUnknown*)(*ppv);
   pUnk->AddRef();

   return( S_OK );
}

