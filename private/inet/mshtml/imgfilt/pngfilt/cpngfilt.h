#ifndef UNIX
#pragma pack( push, PNG )
#pragma pack( 1 )
#endif

typedef struct _PNGCHUNKHEADER
{
   ULONG nDataLength;
   DWORD dwChunkType;
} PNGCHUNKHEADER;

typedef struct _PNGIHDRDATA
{
   ULONG nWidth;
   ULONG nHeight;
   BYTE nBitDepth;
   BYTE bColorType;
   BYTE bCompressionMethod;
   BYTE bFilterMethod;
   BYTE bInterlaceMethod;
} PNGIHDRDATA;

#ifndef UNIX
#pragma pack( pop, PNG )
#endif

typedef struct _PNG_INTERLACE_INFO
{
   ULONG nDeltaX;
   ULONG nDeltaY;
   ULONG nPixelWidth;
   ULONG nPixelHeight;
   ULONG iFirstX;
   ULONG iFirstY;
   BYTE anPixelsInPartialBlock[8];
   BYTE anScanLinesInPartialBlock[8];
} PNG_INTERLACE_INFO;

typedef struct _FLOATRGB
{
   float fRed;
   float fGreen;
   float fBlue;
} FLOATRGB;

typedef void (*PNGCOPYSCANLINEPROC)( void* pDest, const void* pSrc, 
   ULONG nPixels, ULONG nDeltaXDest, const FLOATRGB* pfrgbBackground, 
   BYTE* pXlate );
typedef void (*PNGDUPLICATESCANLINEPROC)( void* pScanLine, ULONG nDeltaX,
   ULONG nFullPixels, ULONG nFullPixelWidth, ULONG nPartialPixelWidth );

typedef struct _PNG_FORMAT_INFO
{
   ULONG nPossibleFormats;
   const GUID* pPossibleFormats;
   const PNGCOPYSCANLINEPROC* ppfnCopyScanLineProcs;
   const PNGDUPLICATESCANLINEPROC* ppfnDuplicateScanLineProcs;
} PNG_FORMAT_INFO;

#define PNGCHUNK( a, b, c, d ) \
   (MAKELONG( MAKEWORD( (a), (b) ), MAKEWORD( (c), (d) ) ))

const DWORD PNG_CHUNK_IHDR = PNGCHUNK( 'I', 'H', 'D', 'R' );
const DWORD PNG_CHUNK_IEND = PNGCHUNK( 'I', 'E', 'N', 'D' );
const DWORD PNG_CHUNK_IDAT = PNGCHUNK( 'I', 'D', 'A', 'T' );
const DWORD PNG_CHUNK_PLTE = PNGCHUNK( 'P', 'L', 'T', 'E' );
const DWORD PNG_CHUNK_BKGD = PNGCHUNK( 'b', 'K', 'G', 'D' );
const DWORD PNG_CHUNK_TRNS = PNGCHUNK( 't', 'R', 'N', 'S' );
const DWORD PNG_CHUNK_GAMA = PNGCHUNK( 'g', 'A', 'M', 'A' );

const DWORD PNG_CHUNK_ANCILLARY = 0x00000020;

const BYTE PNG_COMPRESSION_DEFLATE32K = 0;
const BYTE PNG_FILTER_ADAPTIVE = 0;
const BYTE PNG_INTERLACE_NONE = 0;
const BYTE PNG_INTERLACE_ADAM7 = 1;

const ULONG PNG_BUFFER_SIZE = 4096;

const DWORD CHUNK_IHDR = 0x01;
const DWORD CHUNK_PLTE = 0x02;
const DWORD CHUNK_POSTPLTE = 0x04;
const DWORD CHUNK_IDAT = 0x08;
const DWORD CHUNK_LASTIDAT = 0x10;
const DWORD CHUNK_IEND = 0x20;
const DWORD CHUNK_BKGD = 0x40;
const DWORD CHUNK_TRNS = 0x80;
const DWORD CHUNK_GAMA = 0x100;

const BYTE PNG_COLORTYPE_PALETTE_MASK = 0x01;
const BYTE PNG_COLORTYPE_COLOR_MASK = 0x02;
const BYTE PNG_COLORTYPE_ALPHA_MASK = 0x04;
const BYTE PNG_COLORTYPE_INDEXED = PNG_COLORTYPE_PALETTE_MASK|
   PNG_COLORTYPE_COLOR_MASK;
const BYTE PNG_COLORTYPE_RGB = PNG_COLORTYPE_COLOR_MASK;
const BYTE PNG_COLORTYPE_GRAY = 0x00;
const BYTE PNG_COLORTYPE_RGBA = PNG_COLORTYPE_COLOR_MASK|
   PNG_COLORTYPE_ALPHA_MASK;
const BYTE PNG_COLORTYPE_GRAYA = PNG_COLORTYPE_ALPHA_MASK;

class CPNGFilter : 
   public IImageDecodeFilter,
	public CComObjectRoot,
	public CComCoClass< CPNGFilter, &CLSID_CoPNGFilter >
{
public:
	CPNGFilter();
   ~CPNGFilter();

   BEGIN_COM_MAP( CPNGFilter )
	   COM_INTERFACE_ENTRY( IImageDecodeFilter )
   END_COM_MAP()

   DECLARE_NOT_AGGREGATABLE( CPNGFilter )  
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

   DECLARE_REGISTRY( CPNGFilter, _T( "PNGFilter.CoPNGFilter.1" ), 
      _T( "PNGFilter.CoPNGFilter" ), IDS_COPNGFILTER_DESC, THREADFLAGS_BOTH )

//   DECLARE_NO_REGISTRY()

// IImageDecodeFilter
public:
   STDMETHOD( Initialize )( IImageDecodeEventSink* pEventSink );
   STDMETHOD( Process )( IStream* pStream );
   STDMETHOD( Terminate )( HRESULT hrStatus );

protected:
   HRESULT BeginImage();
   HRESULT ChooseDestinationFormat( GUID* pBFID );
   HRESULT DetermineSourceFormat();
   HRESULT EatData();
   HRESULT FireGetSurfaceEvent();
   HRESULT FireOnProgressEvent();
   HRESULT NextState();
   HRESULT OutputBytes( const BYTE* pData, ULONG nBytes );
   HRESULT ChooseBKGD();
   HRESULT ProcessBKGD();
   HRESULT ProcessIDAT();
   HRESULT ProcessIEND();
   HRESULT ProcessIHDR();
   HRESULT ProcessPLTE();
   HRESULT ProcessTRNS();
   HRESULT ProcessGAMA();
   HRESULT ReadChunkHeader();
   HRESULT ReadChunkData();
   HRESULT ReadChunkCRC();
   HRESULT ReadFileHeader();
   HRESULT ReadIDATData();
   HRESULT NextPass();
   HRESULT NextScanLine();
   BOOL BeginPass( ULONG iPass );
   HRESULT WriteScanLine();

    HRESULT LockBits(RECT *prcBounds, DWORD dwLockFlags, void **ppBits, long *pPitch);
    HRESULT UnlockBits(RECT *prcBounds, void *pBits);

   void NoneFilterScanLine();
   void SubFilterScanLine();
   void UpFilterScanLine();
   void AverageFilterScanLine();
   void PaethFilterScanLine();

protected:
   static const PNG_INTERLACE_INFO s_aInterlaceInfoNone[1];
   static const PNG_INTERLACE_INFO s_aInterlaceInfoAdam7[7];
   static const PNG_FORMAT_INFO s_aFormatInfo[15];

protected:
   typedef enum _EInternalState
   {
      ISTATE_READFILEHEADER,
      ISTATE_READCHUNKHEADER,
      ISTATE_READCHUNKDATA,
      ISTATE_READIDATDATA,
      ISTATE_READCHUNKCRC,
      ISTATE_PROCESSIHDR,
      ISTATE_PROCESSIEND,
      ISTATE_PROCESSPLTE,
      ISTATE_PROCESSBKGD,
      ISTATE_PROCESSTRNS,
      ISTATE_PROCESSGAMA,
      ISTATE_CHOOSEBKGD,
      ISTATE_EATDATA,
      ISTATE_DONE
   } EInternalState;
   typedef enum _ESrcFormat
   {
      SRC_GRAY_1,
      SRC_GRAY_2,
      SRC_GRAY_4,
      SRC_GRAY_8,
      SRC_GRAY_16,
      SRC_RGB_24,
      SRC_RGB_48,
      SRC_INDEXED_RGB_1,
      SRC_INDEXED_RGB_2,
      SRC_INDEXED_RGB_4,
      SRC_INDEXED_RGB_8,
      SRC_GRAYA_16,
      SRC_GRAYA_32,
      SRC_RGBA_32,
      SRC_RGBA_64
   } ESrcFormat;

   EInternalState m_eInternalState;  // State of decode state machine
   DWORD m_dwEvents;  // Events the event sink wants to receive
   PNGCOPYSCANLINEPROC m_pfnCopyScanLine;
   PNGDUPLICATESCANLINEPROC m_pfnDuplicateScanLine;
   const PNG_INTERLACE_INFO* m_pInterlaceInfo;
   ULONG m_nFormats;  // Number of formats the event sink supports
   GUID* m_pFormats;  // Formats supported by the event sink
   BOOL m_bPalette;  // Does image use a palette?
   BOOL m_bColor;  // Does image use color?
   BOOL m_bAlpha;  // Does image have an alpha channel
   BOOL m_bSurfaceUsesAlpha;
   BOOL m_bConvertAlpha;
   BOOL m_bSkipData;
   ESrcFormat m_eSrcFormat;  // Source pixel format
   DWORD m_dwCRC;  // CRC accumulator
   DWORD m_dwChunkCRC;  // Stored CRC of current chunk
   ULONG m_nColors;  // Number of colors in palette
   ULONG m_iBackgroundIndex;  // Index of background color
   RGBQUAD m_rgbBackground;  // Background color
   FLOATRGB m_frgbBackground;  // Floating-point background color
   DWORD  m_dwTransKey;      // Transparent color key (RGB or indexed
   ULONG    m_nTransparentColors;   // # transparent indices
   IStream* m_pStream;  // Source stream
   CComPtr< IImageDecodeEventSink > m_pEventSink;  // Event sink
   PNGCHUNKHEADER m_pngChunkHeader;  // Header of current chunk
   PNGIHDRDATA m_pngIHDR;  // IHDR chunk
   DWORD m_dwChunksEncountered;  // CHUNK_* flags for what chunks have been
      // encountered in the image stream so far
   CComPtr< IDirectDrawSurface > m_pDDrawSurface;
   BOOL m_bFinishedIDAT;  // Have we finished the IDAT section?
   ULONG m_nBytesLeftInCurrentTask;  // Bytes remaining before we switch to a
      // new state
   ULONG m_nDataBytesRead;  // Bytes of chunk data read
   ULONG m_iAppend;  // Where to append data in buffer
   BYTE* m_pbScanLine;  // Current decoded scan line (including filter byte)
   BYTE* m_pbPrevScanLine;  // Previous decoded scan line
   ULONG m_iPass;  // Current pass
   ULONG m_nPasses;  // Number of passes
   ULONG m_nBytesInScanLine;  // Number of bytes in one scan line
   ULONG m_nPixelsInScanLine;  // Number of pixels in one scan line
   ULONG m_nBitsPerPixel;  // Bits per pixel in source image
   BOOL m_bExpandPixels;  // Expand interlaced pixels?
   ULONG m_iScanLine;  // Current scan line
   ULONG m_nScanLinesInPass;  // Number of scan lines in current pass
   ULONG m_iScanLineInPass;  // Current scan line in pass
   ULONG m_iFirstStaleScanLine;  // First scan line whose progress has not been
      // reported
   ULONG m_nBPP;  // Bytes per pixel
   ULONG m_nDeltaX;  // Horizontal distance between pixels
   ULONG m_nDeltaY;  // Vertical distance between pixels
   ULONG m_nPixelWidth;  // Width of a pixel
   ULONG m_nPixelHeight;  // Height of a pixel
   ULONG m_iFirstX;  // Horizontal position of first pixel in scan line
   ULONG m_iFirstY;  // Vertical position of first scan line in pass
   ULONG m_nFullPixelsInScanLine;
   ULONG m_nPartialPixelWidth;
   z_stream m_zlibStream;  // ZLib data
   BYTE m_abData[PNG_BUFFER_SIZE];  // Data buffer
   BYTE m_abTrans[256];     // table to collapse multiple transparent indices
   BYTE m_abGamma[256];     // gamma correction table
   RGBQUAD m_argbColors[256];

};


