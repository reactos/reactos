#include "stdafx.h"
#include "imgutil.h"
#include "resource.h"
#include "cbmsurf.h"
#include "cbsfctry.h"

CBitmapSurfaceFactory::CBitmapSurfaceFactory() :
   m_nFormats( 7 )
{
   m_aFormats[0] = BFID_RGB_32;
   m_aFormats[1] = BFID_RGB_24;
   m_aFormats[2] = BFID_RGB_555;
   m_aFormats[3] = BFID_INDEXED_RGB_8;
   m_aFormats[4] = BFID_INDEXED_RGB_4;
   m_aFormats[5] = BFID_INDEXED_RGB_1;
   m_aFormats[6] = BFID_RGBA_32;
}

CBitmapSurfaceFactory::~CBitmapSurfaceFactory()
{
}

STDMETHODIMP CBitmapSurfaceFactory::CreateBitmapSurface( LONG nWidth, 
   LONG nHeight, GUID* pBFID, DWORD dwHintFlags, IBitmapSurface** ppSurface )
{
   CComPtr< IBitmapSurface > pSurface;
   CComPtr< IBitmapSurfaceImpl > pSurfaceImpl;
   HRESULT hResult;
   CLSID clsid;

   (void)dwHintFlags;

   if( ppSurface != NULL )
   {
      *ppSurface = NULL;
   }
   if( pBFID == NULL )
   {
      return( E_INVALIDARG );
   }
   if( ppSurface == NULL )
   {
      return( E_POINTER );
   }

   if( (nWidth <= 0) || (nHeight <= 0) )
   {
      return( E_INVALIDARG );
   }

   clsid = CLSID_CoBitmapSurface;
   hResult = CoCreateInstance( clsid, NULL, CLSCTX_INPROC_SERVER, 
      IID_IBitmapSurface, (void**)&pSurface );
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   hResult = pSurface->QueryInterface( IID_IBitmapSurfaceImpl,
      (void**)&pSurfaceImpl );
   _ASSERT( SUCCEEDED( hResult ) );

   hResult = pSurfaceImpl->Init( nWidth, nHeight, *pBFID, FALSE, 
      (IBitmapSurfaceFactory*)this );
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   *ppSurface = pSurface;
   (*ppSurface)->AddRef();

   return( S_OK );
}

STDMETHODIMP CBitmapSurfaceFactory::GetSupportedFormatsCount(
   LONG* pnFormats )
{
   if( pnFormats == NULL )
   {
      return( E_POINTER );
   }

   *pnFormats = m_nFormats;

   return( S_OK );
}

STDMETHODIMP CBitmapSurfaceFactory::GetSupportedFormats( LONG nFormats,
   GUID* pFormats )
{
   LONG iFormat;

   if( nFormats < 0 )
   {
      return( E_INVALIDARG );
   }
   if( nFormats > m_nFormats )
   {
      return( E_INVALIDARG );
   }
   if( nFormats == 0 )
   {
      return( S_OK );
   }

   if( pFormats == NULL )
   {
      return( E_POINTER );
   }

   for( iFormat = 0; iFormat < nFormats; iFormat++ )
   {
      pFormats[iFormat] = m_aFormats[iFormat];
   }

   return( S_OK );
}

/*
STDMETHODIMP CBitmapSurfaceFactory::CreateAnimation( IBounds* pBounds, 
   REFGUID bfid, IAnimation** ppAnimation )
{
   CComPtr< IBitmapSurface > pSurface;
   CComPtr< IBitmapSurfaceImpl > pSurfaceImpl;
   HRESULT hResult;
   RECT rect;
   CLSID clsid;

   if( pBounds == NULL )
   {
      return( E_INVALIDARG );
   }
   if( ppAnimation == NULL )
   {
      return( E_POINTER );
   }

   *ppAnimation = NULL;

   hResult = pBounds->GetRect( &rect.left, &rect.top, &rect.right, 
      &rect.bottom );
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   if( (rect.left != 0) || (rect.top != 0) || (rect.right <= 0) ||
      (rect.bottom <= 0) )
   {
      return( E_INVALIDARG );
   }

   clsid = CLSID_CoBitmapSurface;
   hResult = CoCreateInstance( clsid, NULL, CLSCTX_INPROC_SERVER, 
      IID_IBitmapSurface, (void**)&pSurface );
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   hResult = pSurface->QueryInterface( IID_IBitmapSurfaceImpl,
      (void**)&pSurfaceImpl );
   _ASSERT( SUCCEEDED( hResult ) );

   hResult = pSurfaceImpl->Init( rect.right-rect.left, rect.bottom-rect.top,
      bfid, TRUE, (IBitmapSurfaceFactory*)this );
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   hResult = pSurface->QueryInterface( IID_IAnimation, (void**)ppAnimation );
   _ASSERT( SUCCEEDED( hResult ) );

   return( S_OK );
}
*/
