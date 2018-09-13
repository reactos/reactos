#include "stdafx.h"
#include "pngfilt.h"
#include "resource.h"
#include "cpngfilt.h"
#include "scanline.h"

void DuplicateScanLineARGB32( void* pScanLine, ULONG nDeltaX, ULONG nFullPixels, 
   ULONG nFullPixelWidth, ULONG nPartialPixelWidth )
{
   BYTE* pbSrcPixel;
   BYTE* pbDestPixel;
   ULONG iSrcPixel;
   ULONG iDestPixel;
   BYTE bAlpha;
   BYTE bRed;
   BYTE bGreen;
   BYTE bBlue;

   pbSrcPixel = LPBYTE( pScanLine );
   
   for( iSrcPixel = 0; iSrcPixel < nFullPixels; iSrcPixel++ )
   {
      bAlpha = pbSrcPixel[4];
      bRed = pbSrcPixel[2];
      bGreen = pbSrcPixel[1];
      bBlue = pbSrcPixel[0];
      pbDestPixel = pbSrcPixel+4;

      for( iDestPixel = 1; iDestPixel < nFullPixelWidth; iDestPixel++ )
      {
         pbDestPixel[0] = bBlue;
         pbDestPixel[1] = bGreen;
         pbDestPixel[2] = bRed;
         pbDestPixel[3] = bAlpha;
         
         pbDestPixel += 4;
      }

      pbSrcPixel += 4*nDeltaX;
   }

   if( nPartialPixelWidth > 0 )
   {
      bAlpha = pbSrcPixel[3];
      bRed = pbSrcPixel[2];
      bGreen = pbSrcPixel[1];
      bBlue = pbSrcPixel[0];
      pbDestPixel = pbSrcPixel+4;

      for( iDestPixel = 1; iDestPixel < nPartialPixelWidth; iDestPixel++ )
      {
         pbDestPixel[0] = bBlue;
         pbDestPixel[1] = bGreen;
         pbDestPixel[2] = bRed;
         pbDestPixel[3] = bAlpha;
      
         pbDestPixel += 4;
      }
   }
}

void DuplicateScanLineBGR24( void* pScanLine, ULONG nDeltaX, ULONG nFullPixels, 
   ULONG nFullPixelWidth, ULONG nPartialPixelWidth )
{
   BYTE* pbSrcPixel;
   BYTE* pbDestPixel;
   ULONG iSrcPixel;
   ULONG iDestPixel;
   BYTE bRed;
   BYTE bGreen;
   BYTE bBlue;

   pbSrcPixel = LPBYTE( pScanLine );
   
   for( iSrcPixel = 0; iSrcPixel < nFullPixels; iSrcPixel++ )
   {
      bRed = pbSrcPixel[2];
      bGreen = pbSrcPixel[1];
      bBlue = pbSrcPixel[0];
      pbDestPixel = pbSrcPixel+3;

      for( iDestPixel = 1; iDestPixel < nFullPixelWidth; iDestPixel++ )
      {
         pbDestPixel[0] = bBlue;
         pbDestPixel[1] = bGreen;
         pbDestPixel[2] = bRed;
         
         pbDestPixel += 3;
      }

      pbSrcPixel += 3*nDeltaX;
   }

   if( nPartialPixelWidth > 0 )
   {
      bRed = pbSrcPixel[2];
      bGreen = pbSrcPixel[1];
      bBlue = pbSrcPixel[0];
      pbDestPixel = pbSrcPixel+3;

      for( iDestPixel = 1; iDestPixel < nPartialPixelWidth; iDestPixel++ )
      {
         pbDestPixel[0] = bBlue;
         pbDestPixel[1] = bGreen;
         pbDestPixel[2] = bRed;
      
         pbDestPixel += 3;
      }
   }
}

void DuplicateScanLineIndex8( void* pScanLine, ULONG nDeltaX, 
   ULONG nFullPixels, ULONG nFullPixelWidth, ULONG nPartialPixelWidth )
{
   BYTE* pbSrcPixel;
   BYTE* pbDestPixel;
   ULONG iSrcPixel;
   ULONG iDestPixel;
   BYTE bIndex;

   pbSrcPixel = LPBYTE( pScanLine );
   
   for( iSrcPixel = 0; iSrcPixel < nFullPixels; iSrcPixel++ )
   {
      bIndex = pbSrcPixel[0];
      pbDestPixel = pbSrcPixel+1;

      for( iDestPixel = 1; iDestPixel < nFullPixelWidth; iDestPixel++ )
      {
         pbDestPixel[0] = bIndex;
         
         pbDestPixel++;
      }

      pbSrcPixel += nDeltaX;
   }

   if( nPartialPixelWidth > 0 )
   {
      bIndex = pbSrcPixel[0];
      pbDestPixel = pbSrcPixel+1;

      for( iDestPixel = 1; iDestPixel < nPartialPixelWidth; iDestPixel++ )
      {
         pbDestPixel[0] = bIndex;
      
         pbDestPixel++;
      }
   }
}

const float RECIP65535 = 1.0f/65535.0f;
const float RECIP255 = 1.0f/255.0f;

void CopyScanLineRGBA64ToBGR24( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE *pXlate )
{
   const BYTE* pbSrc;
   BYTE* pbDest;
   ULONG iPixel;
   float fAlpha;
   float fInverseAlpha;
   float fSrcRed;
   float fSrcGreen;
   float fSrcBlue;
   float fDestRed;
   float fDestGreen;
   float fDestBlue;

   pbSrc = (const BYTE*)pSrc;
   pbDest = LPBYTE( pDest );

   for( iPixel = 0; iPixel < nPixels; iPixel++ )
   {
      fAlpha = ((pbSrc[6]<<8)+pbSrc[7])*RECIP65535;
      fInverseAlpha = 1.0f-fAlpha;

      fSrcRed = ((pbSrc[0]<<8)+pbSrc[1])*RECIP65535;
      fSrcGreen = ((pbSrc[2]<<8)+pbSrc[3])*RECIP65535;
      fSrcBlue = ((pbSrc[4]<<8)+pbSrc[5])*RECIP65535;

      fDestRed = (fAlpha*fSrcRed)+(fInverseAlpha*pfrgbBackground->fRed);
      fDestGreen = (fAlpha*fSrcGreen)+(fInverseAlpha*pfrgbBackground->fGreen);
      fDestBlue = (fAlpha*fSrcBlue)+(fInverseAlpha*pfrgbBackground->fBlue);

      pbDest[0] = pXlate[BYTE(fDestBlue*255.0f)];
      pbDest[1] = pXlate[BYTE(fDestGreen*255.0f)];
      pbDest[2] = pXlate[BYTE(fDestRed*255.0f)];

      pbSrc += 8;
      pbDest += 3*nDeltaXDest;
   }
}

void CopyScanLineRGBA32ToBGR24( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE *pXlate )
{
   const BYTE* pbSrc;
   BYTE* pbDest;
   ULONG iPixel;
   float fAlpha;
   float fInverseAlpha;
   float fSrcRed;
   float fSrcGreen;
   float fSrcBlue;
   float fDestRed;
   float fDestGreen;
   float fDestBlue;

   pbSrc = (const BYTE*)pSrc;
   pbDest = LPBYTE( pDest );

   for( iPixel = 0; iPixel < nPixels; iPixel++ )
   {
      fAlpha = pbSrc[3]*RECIP255;
      fInverseAlpha = 1.0f-fAlpha;

      fSrcRed = pbSrc[0]*RECIP255;
      fSrcGreen = pbSrc[1]*RECIP255;
      fSrcBlue = pbSrc[2]*RECIP255;

      fDestRed = (fAlpha*fSrcRed)+(fInverseAlpha*pfrgbBackground->fRed);
      fDestGreen = (fAlpha*fSrcGreen)+(fInverseAlpha*pfrgbBackground->fGreen);
      fDestBlue = (fAlpha*fSrcBlue)+(fInverseAlpha*pfrgbBackground->fBlue);

      pbDest[0] = pXlate[BYTE(fDestBlue*255.0f)];
      pbDest[1] = pXlate[BYTE(fDestGreen*255.0f)];
      pbDest[2] = pXlate[BYTE(fDestRed*255.0f)];

      pbSrc += 4;
      pbDest += 3*nDeltaXDest;
   }
}

void CopyScanLineGrayA32ToBGR24( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE *pXlate )
{
   const BYTE* pbSrc;
   BYTE* pbDest;
   ULONG iPixel;
   float fAlpha;
   float fInverseAlpha;
   float fSrc;
   float fDest;
   BYTE bDest;

   pbSrc = (const BYTE*)pSrc;
   pbDest = LPBYTE( pDest );

   for( iPixel = 0; iPixel < nPixels; iPixel++ )
   {
      fAlpha = ((pbSrc[2]<<8)+pbSrc[3])*RECIP65535;
      fInverseAlpha = 1.0f-fAlpha;

      fSrc = ((pbSrc[0]<<8)+pbSrc[1])*RECIP65535;

      fDest = (fAlpha*fSrc)+(fInverseAlpha*pfrgbBackground->fRed);
      bDest = pXlate[BYTE(fDest*255.0f)];

      pbDest[0] = bDest;
      pbDest[1] = bDest;
      pbDest[2] = bDest;

      pbSrc += 4;
      pbDest += 3*nDeltaXDest;
   }
}

void CopyScanLineGrayA16ToBGR24( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE *pXlate )
{
   const BYTE* pbSrc;
   BYTE* pbDest;
   ULONG iPixel;
   float fAlpha;
   float fInverseAlpha;
   float fSrc;
   float fDest;
   BYTE bDest;

   pbSrc = (const BYTE*)pSrc;
   pbDest = LPBYTE( pDest );

   for( iPixel = 0; iPixel < nPixels; iPixel++ )
   {
      fAlpha = pbSrc[1]*RECIP255;
      fInverseAlpha = 1.0f-fAlpha;

      fSrc = pbSrc[0]*RECIP255;

      fDest = (fAlpha*fSrc)+(fInverseAlpha*pfrgbBackground->fRed);
      bDest = pXlate[BYTE(fDest*255.0f)];

      pbDest[0] = bDest;
      pbDest[1] = bDest;
      pbDest[2] = bDest;

      pbSrc += 2;
      pbDest += 3*nDeltaXDest;
   }
}

void CopyScanLineRGBA64ToBGRA32( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE *pXlate )
{
   const BYTE* pbSrc;
   BYTE* pbDest;
   ULONG iPixel;

   (void)pfrgbBackground;
    
   pbSrc = (const BYTE*)pSrc;
   pbDest = LPBYTE( pDest );

   for( iPixel = 0; iPixel < nPixels; iPixel++ )
   {
      pbDest[0] = pXlate[pbSrc[4]];
      pbDest[1] = pXlate[pbSrc[2]];
      pbDest[2] = pXlate[pbSrc[0]];
      pbDest[3] = pbSrc[6]; // alpha not gamma corrected

      pbSrc += 8;
      pbDest += 4*nDeltaXDest;
   }
}

void CopyScanLineRGB48ToBGR24( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE *pXlate )
{
   const BYTE* pbSrc;
   BYTE* pbDest;
   ULONG iPixel;

   (void)pfrgbBackground;
    
   pbSrc = (const BYTE*)pSrc;
   pbDest = LPBYTE( pDest );

   for( iPixel = 0; iPixel < nPixels; iPixel++ )
   {
      pbDest[0] = pXlate[pbSrc[4]];
      pbDest[1] = pXlate[pbSrc[2]];
      pbDest[2] = pXlate[pbSrc[0]];

      pbSrc += 6;
      pbDest += 3*nDeltaXDest;
   }
}

void CopyScanLineRGBA32ToBGRA32( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE *pXlate )
{
   const BYTE* pbSrc;
   BYTE* pbDest;
   ULONG iPixel;

   (void)pfrgbBackground;

   pbSrc = (const BYTE*)pSrc;
   pbDest = LPBYTE( pDest );

   for( iPixel = 0; iPixel < nPixels; iPixel++ )
   {
      pbDest[0] = pXlate[pbSrc[2]];
      pbDest[1] = pXlate[pbSrc[1]];
      pbDest[2] = pXlate[pbSrc[0]];
      pbDest[3] = pbSrc[3]; // alpha not gamma corrected

      pbSrc += 4;
      pbDest += 4*nDeltaXDest;
   }
}

void CopyScanLineRGB24ToBGR24( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE *pXlate )
{
   const BYTE* pbSrc;
   BYTE* pbDest;
   ULONG iPixel;

   (void)pfrgbBackground;
    
   pbSrc = (const BYTE*)pSrc;
   pbDest = LPBYTE( pDest );

   for( iPixel = 0; iPixel < nPixels; iPixel++ )
   {
      pbDest[0] = pXlate[pbSrc[2]];
      pbDest[1] = pXlate[pbSrc[1]];
      pbDest[2] = pXlate[pbSrc[0]];

      pbSrc += 3;
      pbDest += 3*nDeltaXDest;
   }
}

void CopyScanLineGrayA32ToBGRA32( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE *pXlate )
{
   const BYTE* pbSrc;
   BYTE* pbDest;
   ULONG iPixel;

   (void)pfrgbBackground;
    
   pbSrc = (const BYTE*)pSrc;
   pbDest = LPBYTE( pDest );

   for( iPixel = 0; iPixel < nPixels; iPixel++ )
   {
      pbDest[0] = pXlate[pbSrc[0]];
      pbDest[1] = pbDest[0];
      pbDest[2] = pbDest[0];
      pbDest[3] = pbSrc[2]; // alpha not gamma corrected

      pbSrc += 4;
      pbDest += 4*nDeltaXDest;
   }
}

void CopyScanLineGray16ToBGR24( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE *pXlate )
{
   const BYTE* pbSrc;
   BYTE* pbDest;
   ULONG iPixel;

   (void)pfrgbBackground;

   pbSrc = (const BYTE*)pSrc;
   pbDest = LPBYTE( pDest );

   for( iPixel = 0; iPixel < nPixels; iPixel++ )
   {
      pbDest[0] = pXlate[pbSrc[0]];
      pbDest[1] = pbDest[0];
      pbDest[2] = pbDest[0];
      
      pbSrc += 2;
      pbDest += 3*nDeltaXDest;
   }
}

void CopyScanLineGrayA16ToBGRA32( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE *pXlate )
{
   const BYTE* pbSrc;
   BYTE* pbDest;
   ULONG iPixel;

   (void)pfrgbBackground;
    
   pbSrc = (const BYTE*)pSrc;
   pbDest = LPBYTE( pDest );

   for( iPixel = 0; iPixel < nPixels; iPixel++ )
   {
      pbDest[0] = pXlate[pbSrc[0]];
      pbDest[1] = pbDest[0];
      pbDest[2] = pbDest[0];
      pbDest[3] = pbSrc[1];

      pbSrc += 2;
      pbDest += 4*nDeltaXDest;
   }
}

void CopyScanLineGray8ToBGR24( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE *pXlate )
{
   const BYTE* pbSrc;
   BYTE* pbDest;
   ULONG iPixel;

   (void)pfrgbBackground;

   pbSrc = (const BYTE*)pSrc;
   pbDest = LPBYTE( pDest );

   for( iPixel = 0; iPixel < nPixels; iPixel++ )
   {
      pbDest[0] = pXlate[pbSrc[0]];
      pbDest[1] = pbDest[0];
      pbDest[2] = pbDest[0];
      
      pbSrc++;
      pbDest += 3*nDeltaXDest;
   }
}

void CopyScanLineGray8ToGray8( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE *pXlate )
{
   const BYTE* pbSrc;
   BYTE* pbDest;
   ULONG iPixel;

   (void)pfrgbBackground;

   pbSrc = (const BYTE*)pSrc;
   pbDest = LPBYTE( pDest );

   for( iPixel = 0; iPixel < nPixels; iPixel++ )
   {
      pbDest[0] = pXlate[pbSrc[0]];
      
      pbSrc++;
      pbDest += nDeltaXDest;
   }
}

static inline BYTE Expand4To8( ULONG nIntensity )
{
   return( BYTE( nIntensity+(nIntensity<<4) ) );
}

void CopyScanLineGray4ToBGR24( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE *pXlate )
{
   const BYTE* pbSrc;
   BYTE* pbDest;
   ULONG iPair;
   ULONG nPairs;
   BYTE bSrc;
   BYTE bDest;

   (void)pfrgbBackground;
    
   pbSrc = (const BYTE*)pSrc;
   pbDest = LPBYTE( pDest );
   nPairs = nPixels/2;
   for( iPair = 0; iPair < nPairs; iPair++ )
   {
      bSrc = pbSrc[0];
      bDest = pXlate[BYTE((bSrc & 0xf0) + ((bSrc & 0xf0) >> 4))];
      pbDest[0] = bDest;
      pbDest[1] = bDest;
      pbDest[2] = bDest;

      pbDest += 3*nDeltaXDest;

      bDest = pXlate[BYTE((bSrc & 0x0f) + ((bSrc & 0x0f) << 4))];
      pbDest[0] = bDest;
      pbDest[1] = bDest;
      pbDest[2] = bDest;

      pbDest += 3*nDeltaXDest;
      pbSrc++;
   }

   if( (nPixels%2) > 0 )
   {
      bSrc = pbSrc[0];
      bDest = pXlate[BYTE((bSrc & 0xf0) + ((bSrc & 0xf0) >> 4))];
      pbDest[0] = bDest;
      pbDest[1] = bDest;
      pbDest[2] = bDest;
   }
}

static BYTE g_abExpand2To8[4] = { 0, 85, 170, 255 };

static inline BYTE Expand2To8( ULONG nIntensity )
{
   return( g_abExpand2To8[nIntensity] );
}

void CopyScanLineGray2ToBGR24( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE *pXlate )
{
   const BYTE* pbSrc;
   BYTE* pbDest;
   ULONG iQuad;
   ULONG nQuads;
   ULONG iPixel;
   ULONG nShift;
   ULONG nExtraPixels;
   BYTE bSrc;
   BYTE bDest;

   (void)pfrgbBackground;

   pbSrc = (const BYTE*)pSrc;
   pbDest = LPBYTE( pDest );
   nQuads = nPixels/4;
   for( iQuad = 0; iQuad < nQuads; iQuad++ )
   {
      bSrc = pbSrc[0];
      nShift = 6;
      for( iPixel = 0; iPixel < 4; iPixel++ )
      {
         bDest = pXlate[Expand2To8((bSrc >> nShift) & 0x03)];
         pbDest[0] = bDest;
         pbDest[1] = bDest;
         pbDest[2] = bDest;

         nShift -= 2;
         pbDest += 3*nDeltaXDest;
      }

      pbSrc++;
   }

   nExtraPixels = nPixels%4;
   if( nExtraPixels > 0 )
   {
      nShift = 6;
      bSrc = pbSrc[0];
      for( iPixel = 0; iPixel < nExtraPixels; iPixel++ )
      {
         bDest = pXlate[Expand2To8((bSrc >> nShift) & 0x03)];
         pbDest[0] = bDest;
         pbDest[1] = bDest;
         pbDest[2] = bDest;

         nShift -= 2;
         pbDest += 3*nDeltaXDest;
      }
   }
}

static BYTE g_abExpand1To8[2] = { 0, 255 };

static inline BYTE Expand1To8( ULONG nIntensity )
{
   return( g_abExpand1To8[nIntensity] );
}

void CopyScanLineGray1ToBGR24( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE *pXlate )
{
   const BYTE* pbSrc;
   BYTE* pbDest;
   ULONG iOctet;
   ULONG nOctets;
   ULONG nShift;
   ULONG nExtraPixels;
   ULONG iPixel;
   BYTE bSrc;
   BYTE bDest;

   (void)pfrgbBackground;
    (void)pXlate;

   pbSrc = (const BYTE*)pSrc;
   pbDest = LPBYTE( pDest );
   nOctets = nPixels/8;
   for( iOctet = 0; iOctet < nOctets; iOctet++ )
   {
      bSrc = pbSrc[0];
      nShift = 7;
      for( iPixel = 0; iPixel < 8; iPixel++ )
      {
         bDest = Expand1To8( (bSrc>>nShift)&0x01 );
         pbDest[0] = bDest;
         pbDest[1] = bDest;
         pbDest[2] = bDest;

         nShift--;
         pbDest += 3*nDeltaXDest;
      }
      
      pbSrc++;
   }

   nExtraPixels = nPixels%8;
   if( nExtraPixels > 0 )
   {
      nShift = 7;
      bSrc = pbSrc[0];
      for( iPixel = 0; iPixel < nExtraPixels; iPixel++ )
      {
         bDest = Expand1To8( (bSrc>>nShift)&0x01 );
         pbDest[0] = bDest;
         pbDest[1] = bDest;
         pbDest[2] = bDest;

         nShift--;
         pbDest += 3*nDeltaXDest;
      }
   }
}

void CopyScanLineIndex8ToIndex8( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE *pXlate )
{
   const BYTE* pbSrc;
   BYTE* pbDest;
   ULONG iPixel;

   (void)pfrgbBackground;

   pbSrc = (const BYTE*)pSrc;
   pbDest = LPBYTE( pDest );

   for( iPixel = 0; iPixel < nPixels; iPixel++ )
   {
      pbDest[0] = pXlate[pbSrc[0]];
      
      pbSrc++;
      pbDest += nDeltaXDest;
   }
}

void CopyScanLineIndex4ToIndex8( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE *pXlate )
{
   const BYTE* pbSrc;
   BYTE* pbDest;
   ULONG iPair;
   ULONG nPairs;
   BYTE bSrc;

   (void)pfrgbBackground;

   pbSrc = (const BYTE*)pSrc;
   pbDest = LPBYTE( pDest );
   nPairs = nPixels/2;
   for( iPair = 0; iPair < nPairs; iPair++ )
   {
      bSrc = pbSrc[0];
      pbDest[0] = pXlate[BYTE((bSrc >> 4) & 0x0f)];
      pbDest[nDeltaXDest] = pXlate[BYTE(bSrc & 0x0f)];
      
      pbSrc++;
      pbDest += 2*nDeltaXDest;
   }

   if( (nPixels%2) > 0 )
   {
      pbDest[0] = pXlate[BYTE((pbSrc[0] >> 4) & 0x0f)];
   }
}

void CopyScanLineIndex2ToIndex8( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE *pXlate )
{
   const BYTE* pbSrc;
   BYTE* pbDest;
   ULONG iQuad;
   ULONG nQuads;
   ULONG nShift;
   ULONG nExtraPixels;
   ULONG iPixel;
   BYTE bSrc;

   (void)pfrgbBackground;

   pbSrc = (const BYTE*)pSrc;
   pbDest = LPBYTE( pDest );
   nQuads = nPixels/4;
   for( iQuad = 0; iQuad < nQuads; iQuad++ )
   {
      bSrc = pbSrc[0];
      pbDest[0] = pXlate[BYTE((bSrc>>6) & 0x03)];
      pbDest[nDeltaXDest] = pXlate[BYTE((bSrc >> 4) & 0x03)];
      pbDest[2*nDeltaXDest] = pXlate[BYTE((bSrc >> 2) & 0x03)];
      pbDest[3*nDeltaXDest] = pXlate[BYTE(bSrc & 0x03)];
      
      pbSrc++;
      pbDest += 4*nDeltaXDest;
   }

   nExtraPixels = nPixels%4;
   if( nExtraPixels > 0 )
   {
      nShift = 6;
      bSrc = pbSrc[0];
      for( iPixel = 0; iPixel < nExtraPixels; iPixel++ )
      {
         pbDest[0] = pXlate[BYTE((bSrc >> nShift) & 0x03)];
         nShift -= 2;
         pbDest += nDeltaXDest;
      }
   }
}

void CopyScanLineIndex1ToIndex8( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE *pXlate)
{
   const BYTE* pbSrc;
   BYTE* pbDest;
   ULONG iOctet;
   ULONG nOctets;
   ULONG nShift;
   ULONG nExtraPixels;
   ULONG iPixel;
   BYTE bSrc;

   (void)pfrgbBackground;

   pbSrc = (const BYTE*)pSrc;
   pbDest = LPBYTE( pDest );
   nOctets = nPixels/8;
   for( iOctet = 0; iOctet < nOctets; iOctet++ )
   {
      bSrc = pbSrc[0];
      nShift = 7;
      for( iPixel = 0; iPixel < 8; iPixel++ )
      {
         pbDest[0] = pXlate[BYTE((bSrc>>nShift) &0x01)]; 
         nShift--;
         pbDest += nDeltaXDest;
      }
      
      pbSrc++;
   }

   nExtraPixels = nPixels%8;
   if( nExtraPixels > 0 )
   {
      nShift = 7;
      bSrc = pbSrc[0];
      for( iPixel = 0; iPixel < nExtraPixels; iPixel++ )
      {
         pbDest[0] = pXlate[BYTE((bSrc>>nShift) &0x01)]; 
         nShift--;
         pbDest += nDeltaXDest;
      }
   }
}

