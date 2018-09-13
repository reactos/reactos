#include "stdafx.h"
#include "pngfilt.h"
#include "resource.h"
#include "cpngfilt.h"
#include "scanline.h"
#include <math.h>

#include "pngcrc.cpp"

#undef  DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

DEFINE_GUID( IID_IDirectDrawSurface,		0x6C14DB81,0xA733,0x11CE,0xA5,0x21,0x00,
0x20,0xAF,0x0B,0xE5,0x60 );

#ifdef _DEBUG
static ULONG g_nPNGTraceLevel = 1;
static void _cdecl FakeTrace( LPCTSTR pszFormat, ... )
{
   (void)pszFormat;
}
#define PNGTRACE1 ((g_nPNGTraceLevel >= 1) ? AtlTrace : FakeTrace)
#define PNGTRACE ATLTRACE
#else
#define PNGTRACE1 ATLTRACE
#define PNGTRACE ATLTRACE
#endif

// Gamma correction table for sRGB, assuming a file gamma of 1.0

#define	VIEWING_GAMMA	1.125
#define	DISPLAY_GAMMA	2.2

#define	MAXFBVAL	255

BYTE gamma10[256] = {
      0, 15, 21, 26, 30, 34, 37, 41, 43, 46, 49, 51, 53, 56, 58, 60,
     62, 64, 66, 68, 69, 71, 73, 75, 76, 78, 79, 81, 82, 84, 85, 87,
     88, 90, 91, 92, 94, 95, 96, 98, 99,100,101,103,104,105,106,107,
    109,110,111,112,113,114,115,116,117,119,120,121,122,123,124,125,
    126,127,128,129,130,131,132,133,134,135,135,136,137,138,139,140,
    141,142,143,144,145,145,146,147,148,149,150,151,151,152,153,154,
    155,156,156,157,158,159,160,160,161,162,163,164,164,165,166,167,
    167,168,169,170,170,171,172,173,173,174,175,176,176,177,178,179,
    179,180,181,181,182,183,184,184,185,186,186,187,188,188,189,190,
    190,191,192,192,193,194,194,195,196,196,197,198,198,199,200,200,
    201,202,202,203,203,204,205,205,206,207,207,208,208,209,210,210,
    211,212,212,213,213,214,215,215,216,216,217,218,218,219,219,220,
    221,221,222,222,223,223,224,225,225,226,226,227,228,228,229,229,
    230,230,231,231,232,233,233,234,234,235,235,236,236,237,238,238,
    239,239,240,240,241,241,242,242,243,244,244,245,245,246,246,247,
    247,248,248,249,249,250,250,251,251,252,252,253,253,254,254,255,
};

#ifdef BIG_ENDIAN
#define my_ntohl(x) (x)
inline DWORD endianConverter( DWORD dwSrc )
{
   return( ((dwSrc&0xff)<<24)+((dwSrc&0xff00)<<8)+((dwSrc&0xff0000)>>8)+
      ((dwSrc&0xff000000)>>24) );
}
#else
inline DWORD my_ntohl( DWORD dwSrc )
{
   return( ((dwSrc&0xff)<<24)+((dwSrc&0xff00)<<8)+((dwSrc&0xff0000)>>8)+
      ((dwSrc&0xff000000)>>24) );
}
#endif

void FixByteOrder( PNGCHUNKHEADER* pChunkHeader )
{
   pChunkHeader->nDataLength = my_ntohl( pChunkHeader->nDataLength );
}

void FixByteOrder( PNGIHDRDATA* pIHDR )
{
   pIHDR->nWidth = my_ntohl( pIHDR->nWidth );
   pIHDR->nHeight = my_ntohl( pIHDR->nHeight );
}

void CopyPaletteEntriesFromColors(PALETTEENTRY *ppe, const RGBQUAD *prgb,
    UINT uCount)
{
    while (uCount--)
    {
        ppe->peRed   = prgb->rgbRed;
        ppe->peGreen = prgb->rgbGreen;
        ppe->peBlue  = prgb->rgbBlue;
        ppe->peFlags = 0;

        prgb++;
        ppe++;
    }
}

/////////////////////////////////////////////////////////////////////////////
//

CPNGFilter::CPNGFilter() :
   m_eInternalState( ISTATE_READFILEHEADER ),
   m_nBytesLeftInCurrentTask( 0 ),
   m_nDataBytesRead( 0 ),
   m_iAppend( 0 ),
   m_pStream( NULL ),
   m_bFinishedIDAT( FALSE ),
   m_nBytesInScanLine( 0 ),
   m_iScanLine( 0 ),
   m_iFirstStaleScanLine( 0 ),
   m_bExpandPixels( FALSE ),
   m_pbScanLine( NULL ),
   m_pbPrevScanLine( NULL ),
   m_pfnCopyScanLine( NULL ),
   m_dwChunksEncountered( 0 ),
   m_iBackgroundIndex( 0 ),
   m_bSurfaceUsesAlpha( FALSE ),
   m_bConvertAlpha( FALSE ),
   m_nFormats( 0 ),
   m_pFormats( NULL ),
   m_nTransparentColors( 0 )
{
   m_rgbBackground.rgbRed = 0;
   m_rgbBackground.rgbGreen = 0;
   m_rgbBackground.rgbBlue = 0;
   m_rgbBackground.rgbReserved = 0;

    for (int i = 0; i < 256; ++i)
        m_abTrans[i] = (BYTE)i;

    memcpy(m_abGamma, m_abTrans, sizeof(m_abTrans));
}

CPNGFilter::~CPNGFilter()
{
   if( m_pFormats != NULL )
   {
      CoTaskMemFree( m_pFormats );
   }
   delete m_pbPrevScanLine;
   delete m_pbScanLine;
}

const PNGCOPYSCANLINEPROC g_apfnCopyScanLineGray1[1] =
{
   CopyScanLineGray1ToBGR24
};

const PNGCOPYSCANLINEPROC g_apfnCopyScanLineGray2[1] =
{
   CopyScanLineGray2ToBGR24
};

const PNGCOPYSCANLINEPROC g_apfnCopyScanLineGray4[1] =
{
   CopyScanLineGray4ToBGR24
};

const PNGCOPYSCANLINEPROC g_apfnCopyScanLineGray8[1] =
{
   CopyScanLineGray8ToBGR24
};

const PNGCOPYSCANLINEPROC g_apfnCopyScanLineGray16[1] =
{
   CopyScanLineGray16ToBGR24
};

const PNGCOPYSCANLINEPROC g_apfnCopyScanLineRGB24[1] =
{
   CopyScanLineRGB24ToBGR24
};

const PNGCOPYSCANLINEPROC g_apfnCopyScanLineRGB48[1] =
{
   CopyScanLineRGB48ToBGR24
};

const PNGCOPYSCANLINEPROC g_apfnCopyScanLineIndex1[1] =
{
   CopyScanLineIndex1ToIndex8
};

const PNGCOPYSCANLINEPROC g_apfnCopyScanLineIndex2[1] =
{
   CopyScanLineIndex2ToIndex8
};

const PNGCOPYSCANLINEPROC g_apfnCopyScanLineIndex4[1] =
{
   CopyScanLineIndex4ToIndex8
};

const PNGCOPYSCANLINEPROC g_apfnCopyScanLineIndex8[1] =
{
   CopyScanLineIndex8ToIndex8
};

const PNGCOPYSCANLINEPROC g_apfnCopyScanLineGrayA16[2] =
{
   CopyScanLineGrayA16ToBGRA32,
   CopyScanLineGrayA16ToBGR24
};

const PNGCOPYSCANLINEPROC g_apfnCopyScanLineGrayA32[2] =
{
   CopyScanLineGrayA32ToBGRA32,
   CopyScanLineGrayA32ToBGR24
};

const PNGCOPYSCANLINEPROC g_apfnCopyScanLineRGBA32[2] =
{
   CopyScanLineRGBA32ToBGRA32,
   CopyScanLineRGBA32ToBGR24
};

const PNGCOPYSCANLINEPROC g_apfnCopyScanLineRGBA64[2] =
{
   CopyScanLineRGBA64ToBGRA32,
   CopyScanLineRGBA64ToBGR24
};

const PNGDUPLICATESCANLINEPROC g_apfnDuplicateScanLineBGR24[1] =
{
    DuplicateScanLineBGR24
};

const PNGDUPLICATESCANLINEPROC g_apfnDuplicateScanLineIndex8[1] =
{
    DuplicateScanLineIndex8
};

const PNGDUPLICATESCANLINEPROC g_apfnDuplicateScanLineAlphaSrc[2] =
{
    DuplicateScanLineARGB32,
    DuplicateScanLineBGR24
};

const GUID g_TargetGuidsForAlphaSrcs[2] =
{
    // BFID_RGBA_32
    { 0x773c9ac0, 0x3274, 0x11d0, { 0xb7, 0x24, 0x00, 0xaa, 0x00, 0x6c, 0x1a, 0x1 } },
    // BFID_RGB_24
    { 0xe436eb7d, 0x524f, 0x11ce, { 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } }
};

const PNG_FORMAT_INFO CPNGFilter::s_aFormatInfo[15] =
{
   { 1, &BFID_RGB_24, g_apfnCopyScanLineGray1, g_apfnDuplicateScanLineBGR24 },
   { 1, &BFID_RGB_24, g_apfnCopyScanLineGray2, g_apfnDuplicateScanLineBGR24 },
   { 1, &BFID_RGB_24, g_apfnCopyScanLineGray4, g_apfnDuplicateScanLineBGR24 },
   { 1, &BFID_RGB_24, g_apfnCopyScanLineGray8, g_apfnDuplicateScanLineBGR24 },
   { 1, &BFID_RGB_24, g_apfnCopyScanLineGray16, g_apfnDuplicateScanLineBGR24 },
   { 1, &BFID_RGB_24, g_apfnCopyScanLineRGB24, g_apfnDuplicateScanLineBGR24 },
   { 1, &BFID_RGB_24, g_apfnCopyScanLineRGB48, g_apfnDuplicateScanLineBGR24 },
   { 1, &BFID_INDEXED_RGB_8, g_apfnCopyScanLineIndex1, g_apfnDuplicateScanLineIndex8 },
   { 1, &BFID_INDEXED_RGB_8, g_apfnCopyScanLineIndex2, g_apfnDuplicateScanLineIndex8 },
   { 1, &BFID_INDEXED_RGB_8, g_apfnCopyScanLineIndex4, g_apfnDuplicateScanLineIndex8 },
   { 1, &BFID_INDEXED_RGB_8, g_apfnCopyScanLineIndex8, g_apfnDuplicateScanLineIndex8 },
   { 2, g_TargetGuidsForAlphaSrcs, g_apfnCopyScanLineGrayA16, g_apfnDuplicateScanLineAlphaSrc },
   { 2, g_TargetGuidsForAlphaSrcs, g_apfnCopyScanLineGrayA32, g_apfnDuplicateScanLineAlphaSrc },
   { 2, g_TargetGuidsForAlphaSrcs, g_apfnCopyScanLineRGBA32, g_apfnDuplicateScanLineAlphaSrc },
   { 2, g_TargetGuidsForAlphaSrcs, g_apfnCopyScanLineRGBA64, g_apfnDuplicateScanLineAlphaSrc }
};

HRESULT CPNGFilter::ChooseDestinationFormat( GUID* pBFID )
{
   ULONG iPossibleFormat;
   ULONG iAcceptableFormat;
   const PNG_FORMAT_INFO* pFormatInfo;
   BOOL bFound;
   ULONG iChosenFormat;

   _ASSERTE( pBFID != NULL );

   *pBFID = GUID_NULL;

   pFormatInfo = &s_aFormatInfo[m_eSrcFormat];

   bFound = FALSE;
   iChosenFormat = 0;
   for( iAcceptableFormat = 0; (iAcceptableFormat < m_nFormats) && !bFound;
      iAcceptableFormat++ )
   {
      for( iPossibleFormat = 0; iPossibleFormat <
         pFormatInfo->nPossibleFormats; iPossibleFormat++ )
      {
         if( IsEqualGUID(m_pFormats[iAcceptableFormat],
             pFormatInfo->pPossibleFormats[iPossibleFormat] ) )
         {
            iChosenFormat = iPossibleFormat;
            bFound = TRUE;
         }
      }
   }
   if( !bFound )
   {
      return( E_FAIL );
   }

   m_pfnCopyScanLine = pFormatInfo->ppfnCopyScanLineProcs[iChosenFormat];
   m_pfnDuplicateScanLine = pFormatInfo->ppfnDuplicateScanLineProcs[iChosenFormat];
   *pBFID = pFormatInfo->pPossibleFormats[iChosenFormat];

   return( S_OK );
}

HRESULT CPNGFilter::LockBits(RECT *prcBounds, DWORD dwLockFlags, void **ppBits, long *pPitch)
{
    HRESULT hResult;
    DDSURFACEDESC   ddsd;

    (dwLockFlags);

    ddsd.dwSize = sizeof(ddsd);
    hResult = m_pDDrawSurface->Lock(prcBounds, &ddsd, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);
    if (FAILED(hResult))
        return hResult;

    *ppBits = ddsd.lpSurface;
    *pPitch = ddsd.lPitch;

    return S_OK;
}

HRESULT CPNGFilter::UnlockBits(RECT *prcBounds, void *pBits)
{
    (prcBounds);

    return m_pDDrawSurface->Unlock(pBits);
}


HRESULT CPNGFilter::FireGetSurfaceEvent()
{
    HRESULT hResult;
    GUID bfid;
    CComPtr < IUnknown > pSurface;

    _ASSERTE( m_pEventSink != NULL );
    _ASSERTE( m_pDDrawSurface == NULL );

    m_bConvertAlpha = FALSE;
    m_bSurfaceUsesAlpha = FALSE;

    hResult = ChooseDestinationFormat(&bfid);
    if (FAILED(hResult))
    {
        return(hResult);
    }

    hResult = m_pEventSink->GetSurface(m_pngIHDR.nWidth, m_pngIHDR.nHeight,
        bfid, m_nPasses, IMGDECODE_HINT_TOPDOWN|IMGDECODE_HINT_FULLWIDTH,
        &pSurface);
    if (FAILED(hResult))
    {
        return( hResult);
    }

    hResult = pSurface->QueryInterface(IID_IDirectDrawSurface, (void **)&m_pDDrawSurface);

    if (FAILED(hResult))
        return(hResult);

    return (S_OK);
}

// Send an OnProgress event to the event sink (if it has requested progress
// notifications).
HRESULT CPNGFilter::FireOnProgressEvent()
{
   HRESULT hResult;
   RECT rect;

   if( !(m_dwEvents & IMGDECODE_EVENT_PROGRESS) )
   {
      return( S_OK );
   }

   PNGTRACE1(_T("Pass: %d\n"), m_iPass );

   rect.left = 0;
   rect.top = m_iFirstStaleScanLine;
   rect.right = m_pngIHDR.nWidth;
   rect.bottom = min( m_iScanLine, m_pngIHDR.nHeight );
   hResult = m_pEventSink->OnProgress( &rect, TRUE );
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   m_iFirstStaleScanLine = m_iScanLine;

   return( S_OK );
}


///////////////////////////////////////////////////////////////////////////////
// PNG scan line filtering routines

void CPNGFilter::NoneFilterScanLine()
{
}

void CPNGFilter::SubFilterScanLine()
{
   BYTE* pbByte;
   ULONG iByte;
   ULONG nSrcByte;

   pbByte = m_pbScanLine+1+m_nBPP;
   for( iByte = m_nBPP; iByte < m_nBytesInScanLine; iByte++ )
   {
      nSrcByte = *pbByte;
      nSrcByte += *(pbByte-m_nBPP);
      *pbByte = BYTE( nSrcByte );
      pbByte++;
   }
}

void CPNGFilter::UpFilterScanLine()
{
   ULONG iByte;

   if( m_iScanLineInPass == 0 )
   {
      // Unfiltering the top scan line is a NOP
      return;
   }

   for( iByte = 1; iByte <= m_nBytesInScanLine; iByte++ )
   {
      m_pbScanLine[iByte] = BYTE( m_pbScanLine[iByte]+
         m_pbPrevScanLine[iByte] );
   }
}

void CPNGFilter::AverageFilterScanLine()
{
   ULONG iByte;
   ULONG nSum;

   if( m_iScanLineInPass == 0 )
   {
      // No prior scan line.  Skip the first m_nBPP bytes, since they are not
      // affected by the filter
      for( iByte = m_nBPP+1; iByte <= m_nBytesInScanLine; iByte++ )
      {
         m_pbScanLine[iByte] = BYTE( m_pbScanLine[iByte]+
            (m_pbScanLine[iByte-m_nBPP]/2) );
      }
   }
   else
   {
      for( iByte = 1; iByte <= m_nBPP; iByte++ )
      {
         m_pbScanLine[iByte] = BYTE( m_pbScanLine[iByte]+
            (m_pbPrevScanLine[iByte]/2) );
      }
      for( ; iByte <= m_nBytesInScanLine; iByte++ )
      {
         nSum = m_pbScanLine[iByte-m_nBPP]+m_pbPrevScanLine[iByte];
         m_pbScanLine[iByte] = BYTE( m_pbScanLine[iByte]+(nSum/2) );
      }
   }
}

static inline int Abs( int n )
{
   if( n > 0 )
   {
      return( n );
   }
   else
   {
      return( -n );
   }
}

BYTE PaethPredictor( BYTE a, BYTE b, BYTE c )
{
   int p;
   int pa;
   int pb;
   int pc;

   p = int( a )+int( b )-int( c );
   pa = Abs( p-a );
   pb = Abs( p-b );
   pc = Abs( p-c );

   if( (pa <= pb) && (pa <= pc) )
   {
      return( a );
   }
   if( pb <= pc )
   {
      return( b );
   }
   return( c );
}

void CPNGFilter::PaethFilterScanLine()
{
   ULONG iByte;

   if( m_iScanLineInPass == 0 )
   {
      for( iByte = 1; iByte <= m_nBPP; iByte++ )
      {
         m_pbScanLine[iByte] = BYTE( m_pbScanLine[iByte]+PaethPredictor( 0, 0,
            0 ) );
      }
      for( ; iByte <= m_nBytesInScanLine; iByte++ )
      {
         m_pbScanLine[iByte] = BYTE( m_pbScanLine[iByte]+PaethPredictor(
            m_pbScanLine[iByte-m_nBPP], 0, 0 ) );
      }
   }
   else
   {
      for( iByte = 1; iByte <= m_nBPP; iByte++ )
      {
         m_pbScanLine[iByte] = BYTE( m_pbScanLine[iByte]+PaethPredictor( 0,
            m_pbPrevScanLine[iByte], 0 ) );
      }
      for( ; iByte <= m_nBytesInScanLine; iByte++ )
      {
         m_pbScanLine[iByte] = BYTE( m_pbScanLine[iByte]+PaethPredictor(
            m_pbScanLine[iByte-m_nBPP], m_pbPrevScanLine[iByte],
            m_pbPrevScanLine[iByte-m_nBPP] ) );
      }
   }
}

// Update a CRC accumulator with new data bytes
DWORD UpdateCRC( DWORD dwInitialCRC, const BYTE* pbData, ULONG nCount )
{
   DWORD dwCRC;
   ULONG iByte;

   dwCRC = dwInitialCRC;
   for( iByte = 0; iByte < nCount; iByte++ )
   {
      dwCRC = g_adwCRCTable[(dwCRC^pbData[iByte])&0xff]^(dwCRC>>8);
   }

   return( dwCRC );
}

HRESULT CPNGFilter::NextState()
{
   switch( m_eInternalState )
   {
   case ISTATE_READFILEHEADER:
      m_eInternalState = ISTATE_READCHUNKHEADER;
      break;

   case ISTATE_READCHUNKHEADER:
      if( m_pngChunkHeader.dwChunkType == PNG_CHUNK_IDAT )
      {
        if (!(m_dwChunksEncountered & CHUNK_BKGD))
            m_eInternalState = ISTATE_CHOOSEBKGD;
        else
            m_eInternalState = ISTATE_READIDATDATA;
      }
      else
      {
         m_eInternalState = ISTATE_READCHUNKDATA;
      }
      break;

    case ISTATE_CHOOSEBKGD:
        m_eInternalState = ISTATE_READIDATDATA;
        break;

   case ISTATE_READCHUNKDATA:
   
      if (m_bSkipData)
      {
         m_eInternalState = ISTATE_EATDATA;
      }
      else switch( m_pngChunkHeader.dwChunkType )
      {
      case PNG_CHUNK_BKGD:
         m_eInternalState = ISTATE_PROCESSBKGD;
         break;

      case PNG_CHUNK_IHDR:
         m_eInternalState = ISTATE_PROCESSIHDR;
         break;

      case PNG_CHUNK_TRNS:
         m_eInternalState = ISTATE_PROCESSTRNS;
         break;

      case PNG_CHUNK_GAMA:
         m_eInternalState = ISTATE_PROCESSGAMA;
         break;

      case PNG_CHUNK_PLTE:
         m_eInternalState = ISTATE_PROCESSPLTE;
         break;

      case PNG_CHUNK_IEND:
         m_eInternalState = ISTATE_PROCESSIEND;
         break;

      case PNG_CHUNK_IDAT:
         _ASSERT( FALSE );
         // fallthrough
         
      default:
         m_eInternalState = ISTATE_EATDATA;
         break;
      }

      break;

   case ISTATE_PROCESSBKGD:
      m_eInternalState = ISTATE_READCHUNKCRC;
      break;

   case ISTATE_PROCESSIHDR:
      m_eInternalState = ISTATE_READCHUNKCRC;
      break;

   case ISTATE_PROCESSIEND:
      m_eInternalState = ISTATE_READCHUNKCRC;
      break;

   case ISTATE_EATDATA:
      if( m_nDataBytesRead != m_pngChunkHeader.nDataLength )
      {
         m_eInternalState = ISTATE_READCHUNKDATA;
      }
      else
      {
         m_eInternalState = ISTATE_READCHUNKCRC;
      }
      break;

   case ISTATE_READCHUNKCRC:
      if( m_dwChunksEncountered & CHUNK_IEND )
      {
         m_eInternalState = ISTATE_DONE;
      }
      else
      {
         m_eInternalState = ISTATE_READCHUNKHEADER;
      }
      break;

   case ISTATE_READIDATDATA:
      if( m_nDataBytesRead < m_pngChunkHeader.nDataLength )
      {
         m_eInternalState = ISTATE_READIDATDATA;
      }
      else
      {
         _ASSERTE( m_nDataBytesRead == m_pngChunkHeader.nDataLength );
         m_eInternalState = ISTATE_READCHUNKCRC;
      }
      break;

   case ISTATE_PROCESSPLTE:
      m_eInternalState = ISTATE_READCHUNKCRC;
      break;

    case ISTATE_PROCESSTRNS:
        m_eInternalState = ISTATE_READCHUNKCRC;
        break;

    case ISTATE_PROCESSGAMA:
        m_eInternalState = ISTATE_READCHUNKCRC;
        break;

   case ISTATE_DONE:
      m_eInternalState = ISTATE_DONE;
      break;

   default:
      PNGTRACE(_T("Unknown state\n"));
      _ASSERT( FALSE );
      break;
   }

   m_nBytesLeftInCurrentTask = 0;

   return( S_OK );
}

// Process a PNG background color chunk.
HRESULT CPNGFilter::ProcessBKGD()
{
   if( !(m_dwChunksEncountered & CHUNK_IHDR) )
   {
      PNGTRACE(_T("Missing IHDR\n"));
      return( E_FAIL );
   }
   if( m_dwChunksEncountered & CHUNK_BKGD )
   {
      PNGTRACE(_T("Multiple bKGD chunks\n"));
      return( E_FAIL );
   }
   if( m_dwChunksEncountered & (CHUNK_IDAT|CHUNK_IEND) )
   {
      PNGTRACE(_T("Invalid bKGD placement\n"));
      return( E_FAIL );
   }
   if( m_bPalette && !(m_dwChunksEncountered & CHUNK_PLTE) )
   {
      PNGTRACE(_T("bKGD before PLTE in indexed-color image\n"));
      return( E_FAIL );
   }

   m_dwChunksEncountered |= (CHUNK_BKGD|CHUNK_POSTPLTE);

   switch( m_pngIHDR.bColorType )
   {
   case PNG_COLORTYPE_INDEXED:
      if( m_pngChunkHeader.nDataLength != 1 )
      {
         PNGTRACE(_T("Invalid bKGD size\n"));
         return( E_FAIL );
      }
      m_iBackgroundIndex = m_abData[0];
      if( m_iBackgroundIndex >= m_nColors )
      {
         PNGTRACE(_T("Invalid palette index in bKGD\n"));
         return( E_FAIL );
      }
      break;

   case PNG_COLORTYPE_RGB:
   case PNG_COLORTYPE_RGBA:
      if( m_pngChunkHeader.nDataLength != 6 )
      {
         PNGTRACE(_T("Invalid bKGD size\n"));
         return( E_FAIL );
      }
      if( m_pngIHDR.nBitDepth == 8 )
      {
         m_frgbBackground.fRed = float( m_abData[1]/255.0 );
         m_frgbBackground.fGreen = float( m_abData[3]/255.0 );
         m_frgbBackground.fBlue = float( m_abData[5]/255.0 );
      }
      else
      {
         m_frgbBackground.fRed = float( ((m_abData[0]<<8)+m_abData[1] )/
            65535.0 );
         m_frgbBackground.fGreen = float( ((m_abData[2]<<8)+m_abData[3] )/
            65535.0 );
         m_frgbBackground.fBlue = float( ((m_abData[4]<<8)+m_abData[5] )/
            65535.0 );
      }
      break;

   case PNG_COLORTYPE_GRAY:
   case PNG_COLORTYPE_GRAYA:
      if( m_pngChunkHeader.nDataLength != 2 )
      {
         PNGTRACE(_T("Invalid bKGD size\n"));
         return( E_FAIL );
      }
      m_frgbBackground.fRed = float( ((m_abData[0]<<8)+m_abData[1])&
         ((0x01<<m_pngIHDR.nBitDepth)-1) );
      m_frgbBackground.fRed /= float( (0x01<<m_pngIHDR.nBitDepth)-1 );
      m_frgbBackground.fGreen = m_frgbBackground.fRed;
      m_frgbBackground.fBlue = m_frgbBackground.fRed;
      break;

   default:
      _ASSERT( FALSE );
      break;
   }

   m_rgbBackground.rgbRed = BYTE( m_frgbBackground.fRed*255.0 );
   m_rgbBackground.rgbGreen = BYTE( m_frgbBackground.fGreen*255.0 );
   m_rgbBackground.rgbBlue = BYTE( m_frgbBackground.fBlue*255.0 );
   m_rgbBackground.rgbReserved = 0;

   return( S_OK );
}

HRESULT CPNGFilter::ChooseBKGD()
{
   if( !(m_dwChunksEncountered & CHUNK_IHDR) )
   {
      PNGTRACE(_T("Missing IHDR\n"));
      return( E_FAIL );
   }

   if( m_dwChunksEncountered & CHUNK_BKGD )
   {
      PNGTRACE(_T("Multiple bKGD chunks\n"));
      return( E_FAIL );
   }

   m_dwChunksEncountered |= (CHUNK_BKGD|CHUNK_POSTPLTE);

    // Since the image doesn't specify a background color we have to
    // choose one.  Since the image target isn't known we'll use the
    // button face color for lack of anything better...

    *((DWORD *)&m_rgbBackground) = (GetSysColor(COLOR_BTNFACE) & 0x00FFFFFF);
    m_frgbBackground.fRed = float( m_rgbBackground.rgbRed/255.0 );
    m_frgbBackground.fGreen = float( m_rgbBackground.rgbGreen/255.0 );
    m_frgbBackground.fBlue = float( m_rgbBackground.rgbBlue/255.0 );

   return S_OK;
}

// Get ready to read the image data
HRESULT CPNGFilter::BeginImage()
{
    LPDIRECTDRAWPALETTE pDDPalette;
    PALETTEENTRY        ape[256];
    HRESULT hResult;
    BYTE *pby;
    int i;

    // Nothing to do if there's no palette
    if (!m_bPalette)
        return S_OK;

    if (!(m_dwChunksEncountered & CHUNK_PLTE))
    {
        PNGTRACE(_T("No PLTE chunk found for indexed color image\n"));
        return (E_FAIL);
    }


    // TRICK: This applies gamma to the rgbReserved field as well
    //        but this field is always 0, and gamma correction of
    //        0 is always 0, so this safe.
    pby = (BYTE *)m_argbColors;
    for (i = m_nColors * 4; i ; --i, ++pby)
        *pby = m_abGamma[*pby];

	hResult = m_pDDrawSurface->GetPalette(&pDDPalette);
	if (SUCCEEDED(hResult))
    {
        CopyPaletteEntriesFromColors(ape, m_argbColors, m_nColors);
		pDDPalette->SetEntries(0, 0, m_nColors, ape);
		pDDPalette->Release();
    }
		
    if (m_dwEvents & IMGDECODE_EVENT_PALETTE)
    {
        hResult = m_pEventSink->OnPalette();
        if (FAILED(hResult))
        {
            return (hResult);
        }
    }

   return (S_OK);
}

// Process the PNG end-of-stream chunk
HRESULT CPNGFilter::ProcessIEND()
{
   if( !(m_dwChunksEncountered & CHUNK_LASTIDAT) )
   {
      PNGTRACE(_T("Invalid IEND placement\n"));
      return( E_FAIL );
   }

   m_dwChunksEncountered |= CHUNK_IEND;

   if( m_pngChunkHeader.nDataLength > 0 )
   {
      PNGTRACE(_T("Invalid IEND chunk length\n"));
      return( E_FAIL );
   }

   return( S_OK );
}

HRESULT CPNGFilter::DetermineSourceFormat()
{
   switch( m_pngIHDR.bColorType )
   {
   case PNG_COLORTYPE_RGB:
      switch( m_pngIHDR.nBitDepth )
      {
      case 8:
         m_eSrcFormat = SRC_RGB_24;
         break;

      case 16:
         m_eSrcFormat = SRC_RGB_48;
         break;

      default:
         PNGTRACE(_T("Invalid bit depth %d for RGB image\n"), 
            m_pngIHDR.nBitDepth );
         return( E_FAIL );
         break;
      }
      m_nBitsPerPixel = m_pngIHDR.nBitDepth*3;
      break;

   case PNG_COLORTYPE_RGBA:
      switch( m_pngIHDR.nBitDepth )
      {
      case 8:
         m_eSrcFormat = SRC_RGBA_32;
         break;

      case 16:
         m_eSrcFormat = SRC_RGBA_64;
         break;

      default:
         PNGTRACE(_T("Invalid bit depth %d for RGBA image\n"), 
            m_pngIHDR.nBitDepth );
         return( E_FAIL );
         break;
      }
      m_nBitsPerPixel = m_pngIHDR.nBitDepth*4;
      break;

   case PNG_COLORTYPE_INDEXED:
      switch( m_pngIHDR.nBitDepth )
      {
      case 1:
         m_eSrcFormat = SRC_INDEXED_RGB_1;
         break;

      case 2:
         m_eSrcFormat = SRC_INDEXED_RGB_2;
         break;

      case 4:
         m_eSrcFormat = SRC_INDEXED_RGB_4;
         break;

      case 8:
         m_eSrcFormat = SRC_INDEXED_RGB_8;
         break;

      default:
         PNGTRACE(_T("Invalid bit depth %d for indexed-color image\n"),
            m_pngIHDR.nBitDepth );
         return( E_FAIL );
         break;
      }
      m_nBitsPerPixel = m_pngIHDR.nBitDepth;
      break;

   case PNG_COLORTYPE_GRAY:
      switch( m_pngIHDR.nBitDepth )
      {
      case 1:
         m_eSrcFormat = SRC_GRAY_1;
         break;

      case 2:
         m_eSrcFormat = SRC_GRAY_2;
         break;

      case 4:
         m_eSrcFormat = SRC_GRAY_4;
         break;

      case 8:
         m_eSrcFormat = SRC_GRAY_8;
         break;

      case 16:
         m_eSrcFormat = SRC_GRAY_16;
         break;

      default:
         PNGTRACE(_T("Invalid bit depth %d for grayscale image\n"),
            m_pngIHDR.nBitDepth );
         return( E_FAIL );
         break;
      }
      m_nBitsPerPixel = m_pngIHDR.nBitDepth;
      break;

   case PNG_COLORTYPE_GRAYA:
      switch( m_pngIHDR.nBitDepth )
      {
      case 8:
         m_eSrcFormat = SRC_GRAYA_16;
         break;

      case 16:
         m_eSrcFormat = SRC_GRAYA_32;
         break;

      default:
         PNGTRACE(_T("Invalid bit depth %d for grayscale/alpha image\n"),
            m_pngIHDR.nBitDepth );
         return( E_FAIL );
         break;
      }
      m_nBitsPerPixel = m_pngIHDR.nBitDepth*2;
      break;

   default:
      PNGTRACE(_T("Invalid color type %d\n"), m_pngIHDR.bColorType );
      return( E_FAIL );
      break;
   }

   return( S_OK );
}

// Process the PNG image header chunk
HRESULT CPNGFilter::ProcessIHDR()
{
   PNGIHDRDATA* pIHDR;
   HRESULT hResult;
   int nError;

   if( m_dwChunksEncountered != 0 )
   {
      PNGTRACE(_T("Multiple IHDR chunks\n"));
      return( E_FAIL );
   }

   m_dwChunksEncountered |= CHUNK_IHDR;

   pIHDR = (PNGIHDRDATA*)m_abData;
   FixByteOrder( pIHDR );
   memcpy( &m_pngIHDR, pIHDR, sizeof( m_pngIHDR ) );

   PNGTRACE1(_T("%dx%dx%d\n"), m_pngIHDR.nWidth, m_pngIHDR.nHeight, 
      m_pngIHDR.nBitDepth );

   if( (m_pngIHDR.nWidth == 0) || (m_pngIHDR.nHeight == 0) )
   {
      PNGTRACE(_T("Invalid image size\n"));
      return( E_FAIL );
   }

   m_bPalette = m_pngIHDR.bColorType & PNG_COLORTYPE_PALETTE_MASK;
   m_bColor = m_pngIHDR.bColorType & PNG_COLORTYPE_COLOR_MASK;
   m_bAlpha = m_pngIHDR.bColorType & PNG_COLORTYPE_ALPHA_MASK;

   hResult = DetermineSourceFormat();
   if( FAILED( hResult ) )
   {
      return( hResult );
   }
   m_nBytesInScanLine = ((m_pngIHDR.nWidth*m_nBitsPerPixel)+7)/8;
   m_nBPP = max( 1, m_nBytesInScanLine/m_pngIHDR.nWidth );

   m_pbPrevScanLine = new BYTE[m_nBytesInScanLine+1];
   if( m_pbPrevScanLine == NULL )
   {
      return( E_OUTOFMEMORY );
   }
   m_pbScanLine = new BYTE[m_nBytesInScanLine+1];
   if( m_pbScanLine == NULL )
   {
      return( E_OUTOFMEMORY );
   }

   switch( m_pngIHDR.bCompressionMethod )
   {
   case PNG_COMPRESSION_DEFLATE32K:
      m_zlibStream.zalloc = NULL;
      m_zlibStream.zfree = NULL;
      m_zlibStream.opaque = NULL;
      nError = inflateInit( &m_zlibStream );
      if( nError != Z_OK )
      {
         return( E_OUTOFMEMORY );
      }
      break;

   default:
      PNGTRACE(_T("Unknown compression method %x\n"),
         m_pngIHDR.bCompressionMethod );
      return( E_FAIL );
      break;
   }
   if( m_pngIHDR.bFilterMethod != PNG_FILTER_ADAPTIVE )
   {
      PNGTRACE(_T("Unknown filter method %x\n"), m_pngIHDR.bFilterMethod );
      return( E_FAIL );
   }

   switch( m_pngIHDR.bInterlaceMethod )
   {
   case PNG_INTERLACE_NONE:
      PNGTRACE1(_T("Image is not interlaced\n"));
      m_nPasses = 1;
      m_pInterlaceInfo = s_aInterlaceInfoNone;
      m_bExpandPixels = FALSE;
      break;

   case PNG_INTERLACE_ADAM7:
      PNGTRACE1(_T("Image is Adam7 interlaced\n"));
      m_nPasses = 7;
      m_pInterlaceInfo = s_aInterlaceInfoAdam7;
      if( m_dwEvents & IMGDECODE_EVENT_PROGRESS )
      {
         m_bExpandPixels = TRUE;
      }
      else
      {
         // Don't bother expanding the pixels if the event sink doesn't care
         // about progress messages.
         m_bExpandPixels = FALSE;
      }
      break;

   default:
      PNGTRACE(_T("Unknown interlace method %d\n"), m_pngIHDR.bInterlaceMethod );
      return( E_FAIL );
      break;
   }
   m_iPass = 0;
   BeginPass( m_iPass );

   if( m_bPalette )
   {
      PNGTRACE1(_T("Palette used\n"));
   }
   if( m_bColor )
   {
      PNGTRACE1(_T("Color used\n"));
   }
   if( m_bAlpha )
   {
      PNGTRACE1(_T("Alpha channel used\n"));
   }

   hResult = FireGetSurfaceEvent();
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   m_iAppend = 0;

   return( S_OK );
}

HRESULT CPNGFilter::ProcessPLTE()
{
   ULONG iColor;
   ULONG iByte;

   if( !(m_dwChunksEncountered & CHUNK_IHDR) )
   {
      PNGTRACE(_T("Missing IHDR\n"));
      return( E_FAIL );
   }

   if( m_dwChunksEncountered & CHUNK_PLTE )
   {
      PNGTRACE(_T("Multiple PLTE chunks\n"));
      return( E_FAIL );
   }

   if( m_dwChunksEncountered & (CHUNK_POSTPLTE|CHUNK_IDAT|CHUNK_IEND) )
   {
      PNGTRACE(_T("Invalid PLTE placement\n"));
      return( E_FAIL );
   }

   if( !m_bColor )
   {
      PNGTRACE( _T("Palettes not allowed for grayscale images - ignoring\n" ));
      return( S_OK );
   }

   m_dwChunksEncountered |= CHUNK_PLTE;

   if( m_pngChunkHeader.nDataLength == 0 )
   {
      return( E_FAIL );
   }
   if( m_bPalette )
   {
      // Image requires a palette
      if( m_pngChunkHeader.nDataLength > (1U<<m_pngIHDR.nBitDepth)*3 )
      {
         return( E_FAIL );
      }
   }
   else
   {
      if( m_pngChunkHeader.nDataLength > 256*3 )
      {
         return( E_FAIL );
      }
   }
   if( m_pngChunkHeader.nDataLength%3 != 0 )
   {
      return( E_FAIL );
   }

   m_nColors = m_pngChunkHeader.nDataLength/3;

   iByte = 0;
   for( iColor = 0; iColor < m_nColors; iColor++ )
   {
      m_argbColors[iColor].rgbRed = m_abData[iByte];
      m_argbColors[iColor].rgbGreen = m_abData[iByte+1];
      m_argbColors[iColor].rgbBlue = m_abData[iByte+2];
//      ATLTRACE( "Palette[%x] = (%x, %x, %x)\n", iColor, m_abData[iByte],
//         m_abData[iByte+1], m_abData[iByte+2] );
      iByte += 3;
   }

   m_iAppend = 0;

   return( S_OK );
}

HRESULT CPNGFilter::ProcessTRNS()
{
    WORD    *pw = (WORD *)m_abData;
    RGBQUAD trans;
    int     byShiftCnt;
    ULONG   i;
    HRESULT hResult;
    DDCOLORKEY  ddKey;

    // TRNS chunk must precede first IDAT chunk and must follow the
    // PLTE chunk (if any).
    if ((m_dwChunksEncountered & CHUNK_IDAT)
        || (m_bPalette && (~m_dwChunksEncountered & CHUNK_PLTE)))
    {
        PNGTRACE(_T("Invalid TRNS placement\n"));
        return (E_FAIL);
    }

    m_dwChunksEncountered |= CHUNK_TRNS;


    switch (m_pngIHDR.bColorType)
    {
        case PNG_COLORTYPE_RGB:
        case PNG_COLORTYPE_GRAY:
            // BUGBUG we really should preserve the full 16-bit values
            // for proper transparent calculation but our main client,
            // MSHTML, doesn't preserve the RGB values at 16-bit resolution
            // either so it doesn't matter.

            byShiftCnt = (m_eSrcFormat == SRC_RGB_48) ? 8 : 0;
            trans.rgbRed   = (BYTE)(my_ntohl(pw[0]) >> byShiftCnt);
            trans.rgbReserved = 0;

            if (m_pngIHDR.bColorType == PNG_COLORTYPE_GRAY)
            {
                trans.rgbGreen = trans.rgbBlue = trans.rgbRed;
            }
            else
            {
                trans.rgbGreen = (BYTE)(my_ntohl(pw[1]) >> byShiftCnt);
                trans.rgbBlue  = (BYTE)(my_ntohl(pw[2]) >> byShiftCnt);
            }

            m_nTransparentColors = 1;
            m_dwTransKey = *((DWORD *)&trans);
            break;

        case PNG_COLORTYPE_INDEXED:
            // Fill in m_abTrans.  Remember this is filled with
            // the identity map in the constructor...
            for (i = 0; i < m_pngChunkHeader.nDataLength; ++i)
            {
                if (m_abData[i] != 0xff)
                {
                    if (m_nTransparentColors++)
                    {
                        // collapse transparent index to first level seen
                        m_abTrans[i] = (BYTE)m_dwTransKey;
                    }
                    else
                    {
                        // first transparent index seen
                        m_dwTransKey = i;
                        m_abTrans[i] = (BYTE)i;
                    }
                }
            }
            break;

        default:
            PNGTRACE( _T("Color type %d doesn't allow tRNS chunk\n"), m_pngIHDR.bColorType );
            return E_FAIL;
    }

    // Tell the surface what the transparent index is


    ddKey.dwColorSpaceLowValue = m_dwTransKey;
    ddKey.dwColorSpaceHighValue = m_dwTransKey;
    hResult = m_pDDrawSurface->SetColorKey(DDCKEY_SRCBLT, &ddKey);

    return (S_OK);
}

HRESULT CPNGFilter::ProcessGAMA()
{
	double	gbright, gcvideo, file_gamma, max_sample, final_gamma;
	ULONG   ulGamma;
	int     i, iGamma;
	
    // GAMA chunk must precede first IDAT chunk
    if (m_dwChunksEncountered & CHUNK_IDAT)
    {
        PNGTRACE(_T("Invalid GAMA placement\n"));
        return (E_FAIL);
    }

    m_dwChunksEncountered |= CHUNK_GAMA;

    // Get the file gamma and compute table if it's not 1.0

    ulGamma = my_ntohl(*((ULONG *)m_abData));
    max_sample = 255;

    // use our precomputed table if possible

    if (ulGamma == 100000)
    {
        memcpy(m_abGamma, gamma10, sizeof(gamma10));
    }
    else
    {
        file_gamma = ulGamma / 100000.0;

        final_gamma = (VIEWING_GAMMA / (file_gamma * DISPLAY_GAMMA));

	    for (i = 0; i < 256; ++i)
	    {
		    gbright = (double)i / max_sample;
		    gcvideo = pow(gbright, final_gamma);
		    iGamma = (int)(gcvideo * MAXFBVAL + 0.5);
		    m_abGamma[i] = (iGamma > 255) ? (BYTE)255 : (BYTE)iGamma;
	    }
	}

   return (S_OK);
}

HRESULT CPNGFilter::ReadChunkCRC()
{
   HRESULT hResult;
   ULONG nBytesRead;
   BYTE* pBuffer;

   if( m_nBytesLeftInCurrentTask == 0 )
   {
      m_nBytesLeftInCurrentTask = 4;
   }
   pBuffer = LPBYTE( &m_dwChunkCRC )+4-m_nBytesLeftInCurrentTask;
   hResult = m_pStream->Read( pBuffer, m_nBytesLeftInCurrentTask,
      &nBytesRead );
   m_nBytesLeftInCurrentTask -= nBytesRead;
   switch( hResult )
   {
   case S_OK:
      break;

   case S_FALSE:
      return( E_FAIL );
      break;

   default:
      return( hResult );
      break;
   }

   m_dwChunkCRC = my_ntohl( m_dwChunkCRC );

   if( m_dwChunkCRC != ~m_dwCRC )
   {
      PNGTRACE(_T("Bad CRC\n"));
      return( E_FAIL );
   }

   if( m_pngChunkHeader.dwChunkType == PNG_CHUNK_IEND )
   {
      PNGTRACE1(_T("Finished IEND chunk\n"));
   }

   return( S_OK );
}

HRESULT CPNGFilter::ReadChunkData()
{
   HRESULT hResult = S_OK;
   ULONG nBytesToRead;
   ULONG nBytesRead;
   BYTE* pBuffer;

   if( m_nBytesLeftInCurrentTask == 0 )
   {
      if( m_pngChunkHeader.nDataLength == 0 )
      {
         return( S_OK );
      }

      m_iAppend = 0;
      m_nDataBytesRead = 0;
      m_nBytesLeftInCurrentTask = m_pngChunkHeader.nDataLength;
   }

   if (m_nBytesLeftInCurrentTask > PNG_BUFFER_SIZE - m_iAppend)
   {
      // We should have already previously decided to skip too-long data
      _ASSERTE(m_bSkipData);
      m_bSkipData = TRUE;
   }

   while (m_nBytesLeftInCurrentTask && hResult == S_OK)
   {
      pBuffer = &m_abData[m_iAppend];
      
      _ASSERTE(!m_nBytesLeftInCurrentTask || m_iAppend < PNG_BUFFER_SIZE);
      
      nBytesToRead = min(PNG_BUFFER_SIZE - m_iAppend, m_nBytesLeftInCurrentTask);
      
      hResult = m_pStream->Read( pBuffer, nBytesToRead,
         &nBytesRead );
      m_nBytesLeftInCurrentTask -= nBytesRead;
      m_nDataBytesRead += nBytesRead;
      m_iAppend += nBytesRead;
      m_dwCRC = UpdateCRC( m_dwCRC, pBuffer, nBytesRead );
      
     // If we're just skipping data, reset starting point
     if (m_bSkipData)
        m_iAppend = 0;
   }
      
   switch( hResult )
   {
   case S_OK:
      break;

   case S_FALSE:
      return( E_FAIL );
      break;

   default:
      return( hResult );
      break;
   }

   return( S_OK );
}

const PNG_INTERLACE_INFO CPNGFilter::s_aInterlaceInfoNone[1] =
{
   {
      1, 1, 1, 1, 0, 0,
      { 0, 1, 2, 3, 4, 5, 6, 7 },
      { 0, 1, 2, 3, 4, 5, 6, 7 }
   }
};

const PNG_INTERLACE_INFO CPNGFilter::s_aInterlaceInfoAdam7[7] =
{
   {
      8, 8, 8, 8, 0, 0,
      { 0, 1, 1, 1, 1, 1, 1, 1 },
      { 0, 1, 1, 1, 1, 1, 1, 1 }
   },
   {
      8, 8, 4, 8, 4, 0,
      { 0, 0, 0, 0, 0, 1, 1, 1 },
      { 0, 1, 1, 1, 1, 1, 1, 1 }
   },
   {
      4, 8, 4, 4, 0, 4,
      { 0, 1, 1, 1, 1, 2, 2, 2 },
      { 0, 0, 0, 0, 0, 1, 1, 1 }
   },
   {
      4, 4, 2, 4, 2, 0,
      { 0, 0, 0, 1, 1, 1, 1, 2 },
      { 0, 1, 1, 1, 1, 2, 2, 2 }
   },
   {
      2, 4, 2, 2, 0, 2,
      { 0, 1, 1, 2, 2, 3, 3, 4 },
      { 0, 0, 0, 1, 1, 1, 1, 2 }
   },
   {
      2, 2, 1, 2, 1, 0,
      { 0, 0, 1, 1, 2, 2, 3, 3 },
      { 0, 1, 1, 2, 2, 3, 3, 4 }
   },
   {
      1, 2, 1, 1, 0, 1,
      { 0, 1, 2, 3, 4, 5, 6, 7 },
      { 0, 0, 1, 1, 2, 2, 3, 3 }
   }
};

BOOL CPNGFilter::BeginPass( ULONG iPass )
{
   const PNG_INTERLACE_INFO* pInfo;
   ULONG iRightEdgeOfLastPixel;

   _ASSERTE( iPass < m_nPasses );

   pInfo = &m_pInterlaceInfo[iPass];

   m_nDeltaX = pInfo->nDeltaX;
   m_nDeltaY = pInfo->nDeltaY;
   m_iFirstX = pInfo->iFirstX;
   m_iScanLine = pInfo->iFirstY;
   m_nPixelsInScanLine = ((m_pngIHDR.nWidth/8)*(8/m_nDeltaX))+
      pInfo->anPixelsInPartialBlock[m_pngIHDR.nWidth%8];
   m_nBytesInScanLine = (m_nBitsPerPixel*m_nPixelsInScanLine+7)/8;
   m_nScanLinesInPass = ((m_pngIHDR.nHeight/8)*(8/m_nDeltaY))+
      pInfo->anScanLinesInPartialBlock[m_pngIHDR.nHeight%8];
   m_iScanLineInPass = 0;
   m_iFirstStaleScanLine = 0;
   if( m_bExpandPixels )
   {
      m_nPixelWidth = pInfo->nPixelWidth;
      m_nPixelHeight = pInfo->nPixelHeight;
      iRightEdgeOfLastPixel = m_iFirstX+((m_nPixelsInScanLine-1)*m_nDeltaX)+
         m_nPixelWidth;
      if( iRightEdgeOfLastPixel > m_pngIHDR.nWidth )
      {
         // The last pixel in the scan line is a partial pixel
         m_nPartialPixelWidth = m_nPixelWidth-(iRightEdgeOfLastPixel-
            m_pngIHDR.nWidth);
         m_nFullPixelsInScanLine = m_nPixelsInScanLine-1;
      }
      else
      {
         m_nPartialPixelWidth = 0;
         m_nFullPixelsInScanLine = m_nPixelsInScanLine;
      }
   }
   else
   {
      m_nPixelWidth = 1;
      m_nPixelHeight = 1;
      m_nPartialPixelWidth = 0;
   }

   PNGTRACE1(_T("Pass %d.  %d pixels in scan line\n"), iPass, 
      m_nPixelsInScanLine );

   m_zlibStream.next_out = m_pbScanLine;
   m_zlibStream.avail_out = m_nBytesInScanLine+1;
   if( (m_nPixelsInScanLine == 0) || (m_nScanLinesInPass == 0) )
   {
      return( TRUE );
   }

   return( FALSE );
}

HRESULT CPNGFilter::NextPass()
{
   BOOL bEmpty;

   bEmpty = FALSE;
   do
   {
      m_iPass++;
      if( m_iPass < m_nPasses )
      {
         bEmpty = BeginPass( m_iPass );
      }
   } while( (m_iPass < m_nPasses) && bEmpty );

   if( m_iPass >= m_nPasses )
   {
      return( S_FALSE );
   }

   return( S_OK );
}

HRESULT CPNGFilter::NextScanLine()
{
   HRESULT hResult;
   BYTE* pbTemp;

   _ASSERTE( m_zlibStream.avail_out == 0 );

   m_iScanLine += m_nDeltaY;
   m_iScanLineInPass++;
   if( m_iScanLineInPass >= m_nScanLinesInPass )
   {
      // We're done with this pass
      hResult = FireOnProgressEvent();
      if( FAILED( hResult ) )
      {
         return( hResult );
      }

      hResult = NextPass();

      return( hResult );
   }
   else if( ((m_iScanLine-m_iFirstStaleScanLine)/m_nDeltaY) >= 16 )
   {
      hResult = FireOnProgressEvent();
      if( FAILED( hResult ) )
      {
         return( hResult );
      }
   }

   pbTemp = m_pbScanLine;
   m_pbScanLine = m_pbPrevScanLine;
   m_pbPrevScanLine = pbTemp;

   m_zlibStream.avail_out = m_nBytesInScanLine+1;
   m_zlibStream.next_out = m_pbScanLine;

   return( S_OK );
}

HRESULT CPNGFilter::ReadIDATData()
{
   HRESULT hResult;
   ULONG nBytesToRead;
   ULONG nBytesRead;
   int nError;

   if( !(m_dwChunksEncountered & CHUNK_IHDR) )
   {
      PNGTRACE(_T("Missing IHDR\n"));
      return( E_FAIL );
   }

   if( m_dwChunksEncountered & CHUNK_LASTIDAT )
   {
      PNGTRACE(_T("Extra IDAT chunk\n"));
      return( E_FAIL );
   }

   if( !(m_dwChunksEncountered & CHUNK_IDAT) )
   {
      // This is the first IDAT chunk.  Initialize the surface.
      hResult = BeginImage();
      if( FAILED( hResult ) )
      {
         return( hResult );
      }
   }

   m_dwChunksEncountered |= CHUNK_IDAT;

   nBytesToRead = min( m_pngChunkHeader.nDataLength-m_nDataBytesRead,
      PNG_BUFFER_SIZE );

   hResult = m_pStream->Read( m_abData, nBytesToRead, &nBytesRead );
   m_nDataBytesRead += nBytesRead;
   m_dwCRC = UpdateCRC( m_dwCRC, m_abData, nBytesRead );
   switch( hResult )
   {
   case S_OK:
      break;

   case S_FALSE:
      return( E_FAIL );
      break;

   case E_PENDING:
      if( nBytesRead == 0 )
      {
         return( E_PENDING );
      }
      break;

   default:
      return( hResult );
      break;
   }

   m_zlibStream.next_in = m_abData;
   m_zlibStream.avail_in = nBytesRead;

   do
   {
      nError = inflate( &m_zlibStream, Z_PARTIAL_FLUSH );
      if( (nError == Z_OK) || (nError == Z_STREAM_END) )
      {
         if( m_zlibStream.avail_out == 0 )
         {
            switch( m_pbScanLine[0] )
            {
            case 0:
               NoneFilterScanLine();
               break;

            case 1:
               SubFilterScanLine();
               break;

            case 2:
               UpFilterScanLine();
               break;

            case 3:
               AverageFilterScanLine();
               break;

            case 4:
               PaethFilterScanLine();
               break;

            default:
               _ASSERT( FALSE );
               break;
            }
            hResult = WriteScanLine();
            if( FAILED( hResult ) )
            {
               return( hResult );
            }

            hResult = NextScanLine();
            if( FAILED( hResult ) )
            {
               return( hResult );
            }
         }
         else
         {
            _ASSERTE( m_zlibStream.avail_in == 0 );
         }
      }
      else
      {
         return( E_FAIL );
      }

      if( nError == Z_STREAM_END )
      {
         if( m_nDataBytesRead < m_pngChunkHeader.nDataLength )
         {
            PNGTRACE(_T("Extra IDAT data\n"));
            return( E_FAIL );
         }
         m_dwChunksEncountered |= CHUNK_LASTIDAT;
         m_bFinishedIDAT = TRUE;
         inflateEnd( &m_zlibStream );

         if( m_dwEvents & IMGDECODE_EVENT_BITSCOMPLETE )
         {
            hResult = m_pEventSink->OnBitsComplete();
            if( FAILED( hResult ) )
            {
               return( hResult );
            }
         }
      }
   } while( (nError == Z_OK) && (m_zlibStream.avail_in > 0) );

   return( S_OK );
}

HRESULT CPNGFilter::ReadChunkHeader()
{
   HRESULT hResult;
   ULONG nBytesRead;
   BYTE* pBuffer;

   if( m_nBytesLeftInCurrentTask == 0 )
   {
      m_nBytesLeftInCurrentTask = sizeof( m_pngChunkHeader );
   }

   pBuffer = LPBYTE( &m_pngChunkHeader )+sizeof( m_pngChunkHeader )-
      m_nBytesLeftInCurrentTask;
   hResult = m_pStream->Read( pBuffer, m_nBytesLeftInCurrentTask,
      &nBytesRead );
   m_nBytesLeftInCurrentTask -= nBytesRead;
   switch( hResult )
   {
   case S_OK:
      break;

   case S_FALSE:
      return( E_FAIL );
      break;

   default:
      return( hResult );
      break;
   }

   FixByteOrder( &m_pngChunkHeader );

   m_dwCRC = UpdateCRC( 0xffffffff, LPBYTE( &m_pngChunkHeader.dwChunkType ),
      sizeof( m_pngChunkHeader.dwChunkType ) );

   #ifdef BIG_ENDIAN
      m_pngChunkHeader.dwChunkType = endianConverter(m_pngChunkHeader.dwChunkType);
   #endif

   m_nDataBytesRead = 0;
   m_bSkipData = FALSE;

   PNGTRACE1(_T("Chunk type: %c%c%c%c\n"), m_pngChunkHeader.dwChunkType&0xff,
      (m_pngChunkHeader.dwChunkType>>8)&0xff,
      (m_pngChunkHeader.dwChunkType>>16)&0xff,
      (m_pngChunkHeader.dwChunkType>>24)&0xff );
   PNGTRACE1(_T("Data length: %d\n"), m_pngChunkHeader.nDataLength );

   if( !(m_pngChunkHeader.dwChunkType & PNG_CHUNK_ANCILLARY) )
   {
      switch( m_pngChunkHeader.dwChunkType )
      {
      case PNG_CHUNK_IHDR:
      case PNG_CHUNK_PLTE:
      case PNG_CHUNK_IEND:
      
         // If m_pngChunkHeader.nDataLength > 4096 on an critical non-IDAT chunk,
         // we can't decode it, so fail.
          
         if (m_pngChunkHeader.nDataLength > PNG_BUFFER_SIZE)
         {
            PNGTRACE(_T("Critical chunk too long\n"));
            return( E_FAIL );
         }
            
         break;
         
      case PNG_CHUNK_IDAT:
      
         break;

      default:
         PNGTRACE(_T("Unknown critical chunk\n"));
         return( E_FAIL );
         break;
      }
   }
   else
   {
      // If m_pngChunkHeader.nDataLength > 4096 on an ancillary chunk,
      // set a flag so we discard the data
      
      if (m_pngChunkHeader.nDataLength > PNG_BUFFER_SIZE)
      {
         PNGTRACE(_T("Discarding ancillary chunk that is too long\n"));
         m_bSkipData = TRUE;
      }
   }

   return( S_OK );
}

static const BYTE g_abPNGHeader[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };

HRESULT CPNGFilter::ReadFileHeader()
{
   HRESULT hResult;
   ULONG nBytesRead;
   BYTE* pBuffer;

   if( m_nBytesLeftInCurrentTask == 0 )
   {
      m_nBytesLeftInCurrentTask = 8;
   }

   pBuffer = &m_abData[m_iAppend];
   hResult = m_pStream->Read( pBuffer, m_nBytesLeftInCurrentTask,
      &nBytesRead );
   m_nBytesLeftInCurrentTask -= nBytesRead;
   switch( hResult )
   {
   case S_OK:
      break;

   case S_FALSE:
      return( E_FAIL );
      break;

   default:
      return( hResult );
      break;
   }

   if( memcmp( m_abData, g_abPNGHeader, 8 ) == 0 )
   {
      PNGTRACE1(_T("File is a PNG image\n"));
   }
   else
   {
      PNGTRACE(_T("File is not a PNG image\n"));
      return( E_FAIL );
   }

   m_iAppend = 0;

   return( S_OK );
}

HRESULT CPNGFilter::EatData()
{
   m_iAppend = 0;

   return( S_OK );
}


///////////////////////////////////////////////////////////////////////////////
// IImageDecodeFilter methods
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPNGFilter::Initialize( IImageDecodeEventSink* pEventSink )
{
   HRESULT hResult;

   if( pEventSink == NULL )
   {
      return( E_INVALIDARG );
   }

   m_pEventSink = pEventSink;

   hResult = m_pEventSink->OnBeginDecode( &m_dwEvents, &m_nFormats,
      &m_pFormats );
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   return( S_OK );
}

STDMETHODIMP CPNGFilter::Process( IStream* pStream )
{
   HRESULT hResult;
   BYTE bData;
   ULONG nBytesRead;

   // We have to do this every time.  We don't AddRef, since we don't hold onto
   // the stream.
   m_pStream = pStream;

   do
   {
      switch( m_eInternalState )
      {
      case ISTATE_READFILEHEADER:
         hResult = ReadFileHeader();
         break;

      case ISTATE_READCHUNKHEADER:
         hResult = ReadChunkHeader();
         break;

      case ISTATE_READCHUNKDATA:
         hResult = ReadChunkData();
         break;

      case ISTATE_READIDATDATA:
         hResult = ReadIDATData();
         break;

      case ISTATE_READCHUNKCRC:
         hResult = ReadChunkCRC();
         break;

      case ISTATE_EATDATA:
         hResult = EatData();
         break;

      case ISTATE_PROCESSBKGD:
         hResult = ProcessBKGD();
         break;

        case ISTATE_CHOOSEBKGD:
            hResult = ChooseBKGD();
            break;

        case ISTATE_PROCESSTRNS:
            hResult = ProcessTRNS();
            break;

        case ISTATE_PROCESSGAMA:
            hResult = ProcessGAMA();
            break;

      case ISTATE_PROCESSIEND:
         hResult = ProcessIEND();
         break;

      case ISTATE_PROCESSIHDR:
         hResult = ProcessIHDR();
         break;

      case ISTATE_PROCESSPLTE:
         hResult = ProcessPLTE();
         break;

      case ISTATE_DONE:
         hResult = m_pStream->Read( &bData, 1, &nBytesRead );
         if (hResult == S_OK && nBytesRead == 0)
            hResult = S_FALSE;
         break;

      default:
         PNGTRACE(_T("Unknown state\n"));
         _ASSERT( FALSE );
         hResult = E_UNEXPECTED;
         break;
      }
      if( hResult == S_OK )
      {
         NextState();
      }
   } while( hResult == S_OK );

   m_pStream = NULL;

   return( hResult );
}

STDMETHODIMP CPNGFilter::Terminate( HRESULT hrStatus )
{
   PNGTRACE1(_T("Image decode terminated.  Status: %x\n"), hrStatus );

    if (m_pDDrawSurface != NULL)
    {
        m_pDDrawSurface.Release();
    }

   if( m_pEventSink != NULL )
   {
      m_pEventSink->OnDecodeComplete( hrStatus );
      m_pEventSink.Release();
   }

   return( S_OK );
}

HRESULT CPNGFilter::WriteScanLine()
{
   ULONG nPixelHeight;
   ULONG iScanLine;
   RECT rect;
   HRESULT hResult;
   void* pBits;
   LONG nPitch;

   nPixelHeight = min( m_nPixelHeight, m_pngIHDR.nHeight-m_iScanLine );
   if (nPixelHeight < 1)
       return S_OK;
   rect.left = m_iFirstX;
   rect.top = m_iScanLine;
   rect.right = m_pngIHDR.nWidth;
   rect.bottom = m_iScanLine+nPixelHeight;
   hResult = LockBits( &rect, SURFACE_LOCK_EXCLUSIVE, &pBits,
      &nPitch );
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   m_pfnCopyScanLine( pBits, &m_pbScanLine[1], m_nPixelsInScanLine, m_nDeltaX,
      &m_frgbBackground, m_bPalette ? m_abTrans : m_abGamma);
   if( m_bExpandPixels )
   {
      for( iScanLine = 0; iScanLine < nPixelHeight; iScanLine++ )
      {
         m_pfnDuplicateScanLine( pBits, m_nDeltaX, m_nFullPixelsInScanLine,
            m_nPixelWidth, m_nPartialPixelWidth );
      }
   }

   UnlockBits( &rect, pBits );

   return( S_OK );
}
