void DuplicateScanLineARGB32( void* pScanLine, ULONG nDeltaX, ULONG nFullPixels, 
   ULONG nFullPixelWidth, ULONG nPartialPixelWidth );
void DuplicateScanLineBGR24( void* pScanLine, ULONG nDeltaX, ULONG nFullPixels, 
   ULONG nFullPixelWidth, ULONG nPartialPixelWidth );
void DuplicateScanLineIndex8( void* pScanLine, ULONG nDeltaX, 
   ULONG nFullPixels, ULONG nFullPixelWidth, ULONG nPartialPixelWidth );

void CopyScanLineRGBA64ToBGR24( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE* pXlate );
void CopyScanLineRGBA32ToBGR24( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE* pXlate );
void CopyScanLineGrayA32ToBGR24( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE* pXlate );
void CopyScanLineGrayA16ToBGR24( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE* pXlate );

void CopyScanLineRGBA64ToBGRA32( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE* pXlate );
void CopyScanLineRGB48ToBGR24( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE* pXlate );
void CopyScanLineRGBA32ToBGRA32( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE* pXlate );
void CopyScanLineRGB24ToBGR24( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE* pXlate );

void CopyScanLineGrayA32ToBGRA32( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE* pXlate );
void CopyScanLineGray16ToBGR24( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE* pXlate );
void CopyScanLineGrayA16ToBGRA32( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE* pXlate );
void CopyScanLineGray8ToBGR24( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE* pXlate );
void CopyScanLineGray8ToGray8( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE* pXlate );
void CopyScanLineGray4ToBGR24( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE* pXlate );
void CopyScanLineGray2ToBGR24( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE* pXlate );
void CopyScanLineGray1ToBGR24( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE* pXlate );

void CopyScanLineIndex8ToIndex8( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE* pXlate );
void CopyScanLineIndex4ToIndex8( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE* pXlate );
void CopyScanLineIndex2ToIndex8( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE* pXlate );
void CopyScanLineIndex1ToIndex8( void* pDest, const void* pSrc, ULONG nPixels,
   ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, BYTE* pXlate );
