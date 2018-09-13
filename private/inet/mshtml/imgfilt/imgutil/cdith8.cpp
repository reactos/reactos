#include "stdafx.h"
#include "resource.h"
#include "imgutil.h"
#include "cdithtbl.h"
#include "dithers.h"
#include "cdith8.h"
#include "align.h"
#include "cddsurf.h"
//#include <icapexp.h>

void CopyColorsFromPaletteEntries(RGBQUAD *prgb, const PALETTEENTRY *ppe, UINT uCount);

CDitherTable* CDitherToRGB8::s_apTableCache[MAX_DITHERTABLE_CACHE_SIZE];
ULONG CDitherToRGB8::s_nCacheSize;
CRITICAL_SECTION CDitherToRGB8::s_csCache;

void CDitherToRGB8::InitTableCache()
{
   ULONG iTable;

   InitializeCriticalSection( &s_csCache );

   s_nCacheSize = 0;
   for( iTable = 0; iTable < MAX_DITHERTABLE_CACHE_SIZE; iTable++ )
   {
      s_apTableCache[iTable] = NULL;
   }
}

void CDitherToRGB8::CleanupTableCache()
{
   ULONG iTable;

   EnterCriticalSection( &s_csCache );
   for( iTable = 0; iTable < s_nCacheSize; iTable++ )
   {
      _ASSERTE( s_apTableCache[iTable]->m_nRefCount == 0 );
      delete s_apTableCache[iTable];
      s_apTableCache[iTable] = NULL;
   }
   s_nCacheSize = 0;
   LeaveCriticalSection( &s_csCache );

   DeleteCriticalSection( &s_csCache );
}

CDitherToRGB8::CDitherToRGB8() :
   m_dwEvents( 0 ),
   m_iScanLine( 0 ),
   m_bProgressiveDither( FALSE ),
   m_pErrBuf( NULL ),
   m_pErrBuf1( NULL ),
   m_pErrBuf2( NULL ),
   m_pTable( NULL ),
   m_pbBits( NULL ),
   m_hbmDestDib( NULL )
{
}

CDitherToRGB8::~CDitherToRGB8()
{
    if (m_pTable)
        m_pTable->m_nRefCount--;

    if (m_hbmDestDib)
        DeleteObject(m_hbmDestDib);   
}

STDMETHODIMP CDitherToRGB8::GetSurface( LONG nWidth, LONG nHeight, 
   REFGUID bfid, ULONG nPasses, DWORD dwHints, IUnknown** ppSurface )
{
    HRESULT hResult;
    CComPtr< IUnknown > pDestSurface;

    if (ppSurface != NULL)
        *ppSurface = NULL;
      
    if ((nWidth <= 0) || (nHeight <= 0))
        return E_INVALIDARG;
        
    if (ppSurface == NULL)
        return E_POINTER;

    if (IsEqualGUID(bfid, BFID_RGB_24))
    {
        m_eSrcFormat = RGB24;
        m_nBitsPerPixel = 24;
    }
    else if (IsEqualGUID(bfid, BFID_INDEXED_RGB_8))
    {
        m_eSrcFormat = RGB8;
        m_nBitsPerPixel = 8;
    }
    else
        return E_NOINTERFACE;

    m_nWidth = nWidth;
    m_nHeight = nHeight;

    if ((dwHints & IMGDECODE_HINT_TOPDOWN) && 
        (dwHints & IMGDECODE_HINT_FULLWIDTH) &&
        (m_dwEvents & IMGDECODE_EVENT_PROGRESS))
        m_bProgressiveDither = TRUE;
    else
        m_bProgressiveDither = FALSE;

    m_pErrBuf = new ERRBUF[(m_nWidth+2)*2];
    if (m_pErrBuf == NULL)
      return E_OUTOFMEMORY;

    m_pErrBuf1 = &m_pErrBuf[1];
    m_pErrBuf2 = &m_pErrBuf[m_nWidth+3];

    memset(m_pErrBuf, 0, sizeof( ERRBUF )*(m_nWidth+2)*2);

    hResult = m_pEventSink->GetSurface(m_nWidth, m_nHeight, BFID_INDEXED_RGB_8,
        nPasses, dwHints, &pDestSurface);
    if (FAILED(hResult))
        return( hResult );

    hResult = pDestSurface->QueryInterface(IID_IDirectDrawSurface, 
                                            (void **)&m_pDestSurface);
    if (FAILED(hResult))
        return hResult;
    
    m_hbmDestDib = ImgCreateDib(m_nWidth, -LONG(m_nHeight), FALSE, m_nBitsPerPixel, 0, NULL, 
                        &m_pbBits, (int *)&m_nPitch);
    if (m_hbmDestDib == NULL)
        return E_OUTOFMEMORY;

    hResult = CreateDDrawSurfaceOnDIB(m_hbmDestDib, &m_pSurface);
    if (FAILED(hResult))
        return hResult;
        
    *ppSurface = (IUnknown *)m_pSurface;
    (*ppSurface)->AddRef();

    return S_OK;
}

STDMETHODIMP CDitherToRGB8::OnBeginDecode( DWORD* pdwEvents, ULONG* pnFormats,
   GUID** ppFormats )
{
   HRESULT hResult;
   GUID* pFormats;
   ULONG nFormats;
   ULONG iFormat;
   BOOL bFound;

   if( pdwEvents != NULL )
   {
      *pdwEvents = 0;
   }
   if( pnFormats != NULL )
   {
      *pnFormats = 0;
   }
   if( ppFormats != NULL )
   {
      *pnFormats = NULL;
   }
   if( pdwEvents == NULL )
   {
      return( E_POINTER );
   }
   if( pnFormats == NULL )
   {
      return( E_POINTER );
   }
   if( ppFormats == NULL )
   {
      return( E_POINTER );
   }

   hResult = m_pEventSink->OnBeginDecode( &m_dwEvents, &nFormats, &pFormats );
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   bFound = FALSE;
   for( iFormat = 0; (iFormat < nFormats) && !bFound; iFormat++ )
   {
      if( IsEqualGUID( pFormats[iFormat], BFID_INDEXED_RGB_8 ) )
      {
         bFound = TRUE;
      }
   }
   CoTaskMemFree( pFormats );
   if( !bFound )
   {
      return( E_FAIL );
   }

   *ppFormats = (GUID*)CoTaskMemAlloc( 3*sizeof( GUID ) );
   if( *ppFormats == NULL )
   {
      return( E_OUTOFMEMORY );
   }
   *pnFormats = 3;
   (*ppFormats)[0] = BFID_GRAY_8;
   (*ppFormats)[1] = BFID_RGB_24;
   (*ppFormats)[2] = BFID_INDEXED_RGB_8;

   *pdwEvents = m_dwEvents|IMGDECODE_EVENT_BITSCOMPLETE|
      IMGDECODE_EVENT_PALETTE;

   return( S_OK );
}

STDMETHODIMP CDitherToRGB8::OnBitsComplete()
{
   HRESULT hResult;

   hResult = DitherFull();
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   if( m_dwEvents & IMGDECODE_EVENT_BITSCOMPLETE )
   {
      hResult = m_pEventSink->OnBitsComplete();
      if( FAILED( hResult ) )
      {
         return( hResult );
      }
   }

   return( S_OK );
}

STDMETHODIMP CDitherToRGB8::OnDecodeComplete( HRESULT hrStatus )
{
   HRESULT hResult;

   delete m_pErrBuf;
   m_pErrBuf = NULL;
   m_pErrBuf1 = NULL;
   m_pErrBuf2 = NULL;

	// Propagate the transparency information if necessary
	
	if (m_pSurface && m_pDestSurface)
	{
	    DDCOLORKEY  ddKey;

	    if (SUCCEEDED(m_pSurface->GetColorKey(DDCKEY_SRCBLT, &ddKey)))
	        m_pDestSurface->SetColorKey(DDCKEY_SRCBLT, &ddKey);
	}
		
   if( m_pSurface != NULL )
   {
      m_pSurface.Release();
   }
   if( m_pDestSurface != NULL )
   {
      m_pDestSurface.Release();
   }

   hResult = m_pEventSink->OnDecodeComplete( hrStatus );
   m_pEventSink.Release();
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   return( S_OK );
}

STDMETHODIMP CDitherToRGB8::OnPalette()
{
    HRESULT hResult;
    CComPtr< IDirectDrawPalette > pPalette;
    PALETTEENTRY ape[256];

    if (m_eSrcFormat == RGB8)
    {
        hResult = m_pSurface->GetPalette(&pPalette);
        if (FAILED(hResult))
            return hResult;

        hResult = pPalette->GetEntries(0, 0, 256, ape);
        if (FAILED(hResult))
            return hResult;

        CopyColorsFromPaletteEntries(m_argbSrcColors, ape, 256);
    }

    if (m_dwEvents & IMGDECODE_EVENT_PALETTE)
    {
        hResult = m_pEventSink->OnPalette();
        if (FAILED(hResult))
        {
            return hResult;
        }
    }

    return S_OK;
}

STDMETHODIMP CDitherToRGB8::OnProgress( RECT* pBounds, BOOL bComplete )
{
   HRESULT hResult;

   if( pBounds == NULL )
   {
      return( E_INVALIDARG );
   }

   if( m_bProgressiveDither && bComplete )
   {
      hResult = DitherBand( pBounds );
      if( FAILED( hResult ) )
      {
         return( hResult );
      }
   }
   else
   {
      hResult = ConvertBlock( pBounds );
      if( FAILED( hResult ) )
      {
         return( hResult );
      }
   }

   if( m_dwEvents & IMGDECODE_EVENT_PROGRESS )
   {
      hResult = m_pEventSink->OnProgress( pBounds, bComplete );
      if( FAILED( hResult ) )
      {
         return( hResult );
      }
   }

   return( S_OK );
}

STDMETHODIMP CDitherToRGB8::SetDestColorTable( ULONG nColors, 
   const RGBQUAD* prgbColors )
{
   ULONG iTable;
   HRESULT hResult;

   if( (nColors == 0) || (nColors > 256) )
   {
      return( E_INVALIDARG );
   }
   if( prgbColors == NULL )
   {
      return( E_INVALIDARG );
   }

   EnterCriticalSection( &s_csCache );

   if( m_pTable != NULL )
   {
      // Release whatever table we've got already
      m_pTable->m_nRefCount--;
      m_pTable = NULL;
   }

   // See if we can find the requested table in the cache
   for( iTable = 0; (iTable < s_nCacheSize) && (m_pTable == NULL); iTable++ )
   {
      if( s_apTableCache[iTable]->Match( nColors, prgbColors ) )
      {
         m_pTable = s_apTableCache[iTable];
         m_pTable->m_nRefCount++;
      }
   }

   if( m_pTable == NULL )
   {
      if( s_nCacheSize < MAX_DITHERTABLE_CACHE_SIZE )
      {
         m_pTable = new CDitherTable;
         if( m_pTable == NULL )
         {
            LeaveCriticalSection( &s_csCache );
            return( E_OUTOFMEMORY );
         }
         hResult = m_pTable->SetColors( nColors, prgbColors );
         if( FAILED( hResult ) )
         {
            LeaveCriticalSection( &s_csCache );
            m_pTable = NULL;
            return( hResult );
         }

         // Add a new cache entry
         m_pTable->m_nRefCount++;
         s_apTableCache[s_nCacheSize] = m_pTable;
         s_nCacheSize++;
      }
      else
      {
         // Find a cache entry to replace.
         for( iTable = 0; (iTable < s_nCacheSize) && (m_pTable == NULL); 
            iTable++ )
         {
            if( s_apTableCache[iTable]->m_nRefCount == 0 )
            {
               m_pTable = s_apTableCache[iTable];
               hResult = m_pTable->SetColors( nColors, prgbColors );
               if( FAILED( hResult ) )
               {
                  LeaveCriticalSection( &s_csCache );
                  m_pTable = NULL;
                  return( hResult );
               }
               m_pTable->m_nRefCount++;
            }
         }
      }
   }

   _ASSERTE( m_pTable != NULL );

   LeaveCriticalSection( &s_csCache );

   return( S_OK );
}

STDMETHODIMP CDitherToRGB8::SetEventSink( IImageDecodeEventSink* pEventSink )
{
   if( pEventSink == NULL )
   {
      return( E_INVALIDARG );
   }

   m_pEventSink = pEventSink;

   return( S_OK );
}

HRESULT CDitherToRGB8::ConvertBlock( RECT* pBounds )
{
    HRESULT hResult;
    void* pSrcBits;
    void* pDestBits;
    LONG nSrcPitch;
    LONG nDestPitch;
    DDSURFACEDESC ddsd;

    _ASSERTE( pBounds->left == 0 );
    _ASSERTE( pBounds->right == LONG( m_nWidth ) );

    ddsd.dwSize = sizeof(ddsd);
    hResult = m_pSurface->Lock(pBounds, &ddsd, 0, 0);
    if (FAILED(hResult))
        return hResult;

    pSrcBits = ddsd.lpSurface;
    nSrcPitch = ddsd.lPitch;

    hResult = m_pDestSurface->Lock(pBounds, &ddsd, 0, 0);
    if (FAILED(hResult))
    {
        m_pSurface->Unlock(pSrcBits);
        return hResult;
    }

    pDestBits = ddsd.lpSurface;
    nDestPitch = ddsd.lPitch;
    
    switch( m_eSrcFormat )
    {
        case RGB24:
            Convert24to8(LPBYTE(pDestBits), LPBYTE(pSrcBits), nDestPitch, 
                    nSrcPitch, m_pTable->m_abInverseMap, pBounds->left, 
                    pBounds->right-pBounds->left, pBounds->top, 
                    pBounds->bottom-pBounds->top );
            break;

        case RGB8:
            Convert8to8( LPBYTE( pDestBits ), LPBYTE( pSrcBits ), nDestPitch, 
                    nSrcPitch, m_argbSrcColors, m_pTable->m_abInverseMap, pBounds->left, 
                    pBounds->right-pBounds->left, pBounds->top, 
                    pBounds->bottom-pBounds->top );
            break;

        default:
            return E_FAIL;
            break;
    }

    m_pDestSurface->Unlock(pDestBits);
    m_pSurface->Unlock(pSrcBits);

    return S_OK;
}

HRESULT CDitherToRGB8::DitherBand( RECT* pBounds )
{
    HRESULT hResult;
    void* pSrcBits;
    void* pDestBits;
    LONG nSrcPitch;
    LONG nDestPitch;
    LONG lDestTrans = -1;
    LONG lSrcTrans = -1;
    DDSURFACEDESC ddsd;
    DDCOLORKEY ddColorKey;

    _ASSERTE( pBounds->left == 0 );
    _ASSERTE( pBounds->right == LONG( m_nWidth ) );

    ddsd.dwSize = sizeof(ddsd);
    hResult = m_pSurface->Lock(pBounds, &ddsd, 0, 0);
    if (FAILED(hResult))
        return hResult;

    pSrcBits = ddsd.lpSurface;
    nSrcPitch = ddsd.lPitch;

    hResult = m_pDestSurface->Lock(pBounds, &ddsd, 0, 0);
    if (FAILED(hResult))
    {
        m_pSurface->Unlock(pSrcBits);
        return hResult;
    }

    pDestBits = ddsd.lpSurface;
    nDestPitch = ddsd.lPitch;

    switch (m_eSrcFormat)
    {    
        case RGB24:
            Dith24to8(LPBYTE(pDestBits), LPBYTE(pSrcBits), nDestPitch, 
                nSrcPitch, m_pTable->m_argbColors, m_pTable->m_abInverseMap, 
                m_pErrBuf1, m_pErrBuf2, pBounds->left, pBounds->right-pBounds->left, 
                pBounds->top, pBounds->bottom-pBounds->top);
            break;

        case RGB8:
            if (SUCCEEDED(m_pSurface->GetColorKey(DDCKEY_SRCBLT, &ddColorKey)))
                lSrcTrans = ddColorKey.dwColorSpaceLowValue;

            if (SUCCEEDED(m_pDestSurface->GetColorKey(DDCKEY_SRCBLT, &ddColorKey)))
                lDestTrans = ddColorKey.dwColorSpaceLowValue;

            // preserve the transparent index if necessary
            if (lSrcTrans >= 0 && lDestTrans == -1)
            {
                lDestTrans = lSrcTrans;
            }

            if (lSrcTrans == -1 || lDestTrans == -1)
            {
                Dith8to8(LPBYTE(pDestBits), LPBYTE(pSrcBits), nDestPitch, nSrcPitch,
                    m_argbSrcColors, m_pTable->m_argbColors, m_pTable->m_abInverseMap, 
                    m_pErrBuf1, m_pErrBuf2, pBounds->left, pBounds->right-pBounds->left, 
                    pBounds->top, pBounds->bottom-pBounds->top);
            }
            else
            {
                Dith8to8t(LPBYTE(pDestBits), LPBYTE(pSrcBits), nDestPitch, nSrcPitch,
                    m_argbSrcColors, m_pTable->m_argbColors, m_pTable->m_abInverseMap, 
                    m_pErrBuf1, m_pErrBuf2, pBounds->left, pBounds->right-pBounds->left, 
                    pBounds->top, pBounds->bottom-pBounds->top, (BYTE)lDestTrans, (BYTE)lSrcTrans);
            }
            break;

        default:
            return E_FAIL;
    }

    m_pDestSurface->Unlock(pDestBits);
    m_pSurface->Unlock(pSrcBits);

    return S_OK;
}

HRESULT CDitherToRGB8::DitherFull()
{
    HRESULT hResult;
    RECT rect;

    rect.left = 0;
    rect.top = 0;
    rect.right = m_nWidth;
    rect.bottom = m_nHeight;

    hResult = DitherBand(&rect);
    if (FAILED(hResult))
        return hResult;

    return S_OK;
}

HRESULT DitherTo8( BYTE * pDestBits, LONG nDestPitch, 
                   BYTE * pSrcBits, LONG nSrcPitch, REFGUID bfidSrc, 
                   RGBQUAD * prgbDestColors, RGBQUAD * prgbSrcColors,
                   BYTE * pbDestInvMap,
                   LONG x, LONG y, LONG cx, LONG cy,
                   LONG lDestTrans, LONG lSrcTrans)
{
   ERRBUF* m_pErrBuf;
   ERRBUF* m_pErrBuf1;
   ERRBUF* m_pErrBuf2;

//    StartCAPAll();
    
    HRESULT hr = S_OK;

    m_pErrBuf = new ERRBUF[(cx+2)*2];
    if (m_pErrBuf == NULL)
    {
        return( E_OUTOFMEMORY );
    }

    m_pErrBuf1 = &m_pErrBuf[1];
    m_pErrBuf2 = &m_pErrBuf[cx+3];

    memset(m_pErrBuf, 0, sizeof( ERRBUF )*(cx+2)*2);

    if (bfidSrc == BFID_RGB_24)
    {
        Dith24to8( pDestBits, pSrcBits, nDestPitch, nSrcPitch, 
            prgbDestColors, pbDestInvMap, 
            m_pErrBuf1, m_pErrBuf2, x, cx, y, cy );
    }
    else if (bfidSrc == BFID_RGB_8)
    {
        if (lDestTrans == -1 || lSrcTrans == -1)
        {
            Dith8to8( pDestBits, pSrcBits, nDestPitch, nSrcPitch,
                prgbSrcColors, prgbDestColors, pbDestInvMap, 
                m_pErrBuf1, m_pErrBuf2, x, cx, y, cy );
        }
        else
        {
            Dith8to8t( pDestBits, pSrcBits, nDestPitch, nSrcPitch,
                prgbSrcColors, prgbDestColors, pbDestInvMap, 
                m_pErrBuf1, m_pErrBuf2, x, cx, y, cy, (BYTE)lDestTrans, (BYTE)lSrcTrans );
        }
    }
    else
    {
        hr = E_FAIL;
    }

   delete m_pErrBuf;

//    StopCAPAll();
    
    return hr;
}


