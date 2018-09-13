#include "stdafx.h"
#include "imgutil.h"
#include "cdithtbl.h"
#include "cddsurf.h"

// Get rid of "unused formal parameters warning"
#pragma warning(disable : 4100)

STDAPI DecodeImage( IStream* pStream, IMapMIMEToCLSID* pMap, 
   IUnknown* pUnknownEventSink )
{
   USES_CONVERSION;
   HRESULT hResult;
   CComPtr< IStream > pSniffedStream;
   UINT nFormat;
   TCHAR szMIMEType[64];
   LPCOLESTR pszMIMETypeO;
   CComPtr< IMapMIMEToCLSID > pActualMap;
   CComPtr< IImageDecodeFilter > pFilter;
   CComPtr< IImageDecodeEventSink > pEventSink;
   CLSID clsid;
   int nChars;
    
   if( pStream == NULL )
   {
      return( E_INVALIDARG );
   }
   if( pUnknownEventSink == NULL )
   {
      return( E_INVALIDARG );
   }

    pUnknownEventSink->QueryInterface(IID_IImageDecodeEventSink,
        (void **)&pEventSink);

    if (pEventSink == NULL)
        return E_INVALIDARG;
        
   if( pMap == NULL )
   {
      hResult = CoCreateInstance( CLSID_CoMapMIMEToCLSID, NULL, 
         CLSCTX_INPROC_SERVER, IID_IMapMIMEToCLSID, (void**)&pActualMap );
      if( FAILED( hResult ) )
      {
         return( hResult );
      }
   }
   else
   {
      pActualMap = pMap;
   }

   hResult = SniffStream( pStream, &nFormat, &pSniffedStream );
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   nChars = GetClipboardFormatName( nFormat, szMIMEType, 63 );
   if( nChars == 0 )
   {
      return( E_FAIL );
   }
   if( nChars > 60 )
   {
      return( E_FAIL );
   }

   pszMIMETypeO = T2COLE( szMIMEType );
   hResult = pActualMap->MapMIMEToCLSID( pszMIMETypeO, &clsid );
   if( FAILED( hResult ) )
   {
      return( hResult );
   }
   if( hResult == S_FALSE )
   {
      return( E_FAIL );
   }

   hResult = CoCreateInstance( clsid, NULL, CLSCTX_INPROC_SERVER, 
      IID_IImageDecodeFilter, (void**)&pFilter );
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   hResult = pFilter->Initialize( pEventSink );
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   hResult = pFilter->Process( pSniffedStream );

   pFilter->Terminate( hResult );

   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   return( S_OK );
}

STDAPI CreateMIMEMap( IMapMIMEToCLSID** ppMap )
{
   if( ppMap == NULL )
   {
      return( E_POINTER );
   }

   return( CoCreateInstance( CLSID_CoMapMIMEToCLSID, NULL, 
      CLSCTX_INPROC_SERVER, IID_IMapMIMEToCLSID, (void**)ppMap ) );
}

STDAPI ComputeInvCMAP(const RGBQUAD *pRGBColors, ULONG nColors, BYTE *pInvTable, ULONG cbTable)
{
#ifndef MINSUPPORT
    CDitherTable *pDitherTable;
    HRESULT hr;
    
    if (pRGBColors == NULL)
        return E_POINTER;

    if (pInvTable == NULL)
        return E_POINTER;

    if (nColors < 0 || nColors > 256)
        return E_INVALIDARG;

    if (cbTable != 32768)
        return E_INVALIDARG;


    pDitherTable = new CDitherTable;
    if (pDitherTable == NULL)
        return E_OUTOFMEMORY;

    hr = pDitherTable->SetColors(nColors, pRGBColors);
    if (SUCCEEDED(hr))
        memcpy(pInvTable, pDitherTable->m_abInverseMap, 32768);

    delete pDitherTable;
    
    return hr;
#else
    return E_NOTIMPL;
#endif    
}

#ifdef MINSUPPORT

HRESULT DitherTo8( BYTE * pDestBits, LONG nDestPitch, 
                   BYTE * pSrcBits, LONG nSrcPitch, REFGUID bfidSrc, 
                   RGBQUAD * prgbDestColors, RGBQUAD * prgbSrcColors,
                   BYTE * pbDestInvMap,
                   LONG x, LONG y, LONG cx, LONG cy,
                   LONG lDestTrans, LONG lSrcTrans)
{
    return E_NOTIMPL;
}

#endif

STDAPI CreateDDrawSurfaceOnDIB(HBITMAP hbmDib, IDirectDrawSurface **ppSurface)
{
#ifndef MINSUPPORT
    if (hbmDib == NULL)
        return E_INVALIDARG;

    if (ppSurface == NULL)
        return E_POINTER;

    *ppSurface = (IDirectDrawSurface *)(new CDDrawWrapper(hbmDib));

    return *ppSurface ? S_OK : E_OUTOFMEMORY;
#else
    return E_NOTIMPL;
#endif    
}


