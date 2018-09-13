// CGIFFilter.H : Declaration of the CGIFFilter

#define XX_DMsg(x, y)

#define PROG_INTERVAL   0x3

#define NUM_IMPORTANT_COLORS 256

#define MAXCOLORMAPSIZE     256

#define TRUE    1
#define FALSE   0

#define CM_RED      0
#define CM_GREEN    1
#define CM_BLUE     2

#define MAX_LWZ_BITS        12

#define INTERLACE       0x40
#define LOCALCOLORMAP   0x80
#define BitSet(byte, bit)   (((byte) & (bit)) == (bit))

#define LM_to_uint(a,b)         ((((unsigned int) b)<<8)|((unsigned int)a))

#define dwIndefiniteGIFThreshold 300    // 300 seconds == 5 minutes
                                        // If the GIF runs longer than
                                        // this, we will assume the author
                                        // intended an indefinite run.
#define dwMaxGIFBits 13107200           // keep corrupted GIFs from causing
                                        // us to allocate _too_ big a buffer.
                                        // This one is 1280 X 1024 X 10.
typedef struct _GIFSCREEN
{
    unsigned long Width;
    unsigned long Height;
    unsigned char ColorMap[3][MAXCOLORMAPSIZE];
    unsigned long BitPixel;
    unsigned long ColorResolution;
    unsigned long Background;
    unsigned long AspectRatio;
}
GIFSCREEN;

typedef struct _GIF89
{
    long transparent;
    long delayTime;
    long inputFlag;
    long disposal;
}
GIF89;

#define MAX_STACK_SIZE  ((1 << (MAX_LWZ_BITS)) * 2)
#define MAX_TABLE_SIZE  (1 << MAX_LWZ_BITS)
typedef struct _GIFINFO
{
    unsigned char *src;
    GIF89 Gif89;
    long lGifLoc;
    long ZeroDataBlock;

/*
 **  Pulled out of nextCode
 */
    long curbit, lastbit, get_done;
    long last_byte;
    long return_clear;
/*
 **  Out of nextLWZ
 */
    unsigned short *pstack, *sp;
    long stacksize;
    long code_size, set_code_size;
    long max_code, max_code_size;
    long clear_code, end_code;

/*
 *   Were statics in procedures
 */
    unsigned char buf[280];
    unsigned short *table[2];
    long tablesize;
    long firstcode, oldcode;

} GIFINFO;

enum
{
    gifNoneSpecified =  0, // no disposal method specified
    gifNoDispose =      1, // do not dispose, leave the bits there
    gifRestoreBkgnd =   2, // replace the image with the background color
    gifRestorePrev =    3  // replace the image with the previous pixels
};

struct GIFFRAME
{
    GIFFRAME *      pgfNext;
    HBITMAP         hbmDib;             // Dib section of the image
    HBITMAP         hbmMask;            // Dib section of the transparent mask
    HRGN            hrgnVis;            // region describing currently visible portion of the frame
    BYTE            bDisposalMethod;    // see enum above
    BYTE            bTransFlags;        // see TRANSF_ flags below
    BYTE            bTransIndex;        // transparent index
    BYTE            bRgnKind;           // region type for hrgnVis
    UINT            uiDelayTime;        // frame duration, in ms
    int             left;               // bounds relative to the GIF logical screen 
    int             top; 
    int             width;
    int             height;
};

#define TRANSF_TRANSPARENT  0x01        // Image is marked transparent
#define TRANSF_TRANSMASK    0x02        // Attempted to create an hbmMask

#define dwGIFVerUnknown     ((DWORD)0)   // unknown version of GIF file
#define dwGIFVer87a         ((DWORD)87)  // GIF87a file format
#define dwGIFVer89a        ((DWORD)89)  // GIF89a file format.

struct GIFANIMDATA
{
    BYTE            fAnimated;          // TRUE if cFrames and pgf define a GIF animation
    BYTE            fLooped;            // TRUE if we've seen a Netscape loop block
    BYTE            fHasTransparency;   // TRUE if a frame is transparent, or if a frame does
                                        // not cover the entire logical screen.
    BYTE            fNoBwMap;           // TRUE if we saw more than two colors in anywhere in the file.
    DWORD           dwLastProgTimeMS;   // time at which pgfLastProg was displayed.
    UINT            cLoops;             // A la Netscape, we will treat this as 
                                        // "loop forever" if it is zero.
    DWORD           dwGIFVer;           // GIF Version <see defines above> we need to special case 87a backgrounds
    GIFFRAME *      pgf;                // animation frame entries
    GIFFRAME *      pgfLastProg;        // remember the last frame to be drawn during decoding
};

typedef struct {
    BOOL        fAnimating;                 // TRUE if animation is (still) running
    DWORD       dwLoopIter;                 // current iteration of looped animation, not actually used for Netscape compliance reasons
    GIFFRAME *  pgfDraw;                    // last frame we need to draw
    DWORD       dwNextTimeMS;               // Time to display pgfDraw->pgfNext, or next iteration
} GIFANIMATIONSTATE, *PGIFANIMATIONSTATE;

/////////////////////////////////////////////////////////////////////////////
// GIFFilter

class CGIFFilter : 
	public IImageDecodeFilter,
	public CComObjectRoot,
	public CComCoClass< CGIFFilter,&CLSID_CoGIFFilter >
{
public:
	CGIFFilter();
   ~CGIFFilter();

   BEGIN_COM_MAP( CGIFFilter )
	   COM_INTERFACE_ENTRY( IImageDecodeFilter )
   END_COM_MAP()

   DECLARE_NOT_AGGREGATABLE( CGIFFilter ) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

//   DECLARE_REGISTRY( CGIFFilter, _T( "GIFFilter.CoGIFFilter.1" ), 
//      _T( "GIFFilter.CoGIFFilter" ), IDS_COGIFFILTER_DESC, THREADFLAGS_BOTH )

//   DECLARE_REGISTRY_RESOURCEID( IDR_COGIFFILTER_REG );

   DECLARE_NO_REGISTRY()

// IImageDecodeFilter
public:
   STDMETHOD( Initialize )( IImageDecodeEventSink* pEventSink );
   STDMETHOD( Process )( IStream* pStream );
   STDMETHOD( Terminate )( HRESULT hrStatus );

protected:
   HRESULT FireGetSurfaceEvent();
   HRESULT FireOnBeginDecodeEvent();
   HRESULT FireOnBitsCompleteEvent();
   HRESULT FireOnProgressEvent(BOOL fLast);

    HRESULT LockBits(RECT *prcBounds, DWORD dwLockFlags, void **ppBits, long *pPitch);
    HRESULT UnlockBits(RECT *prcBounds, void *pBits);

   BOOL Read(void * pv, ULONG cb, ULONG * pcbRead = NULL);

    void OnProg(BOOL fLast);
    HRESULT ReadGIFMaster();
    long ReadColorMap(long number, unsigned char buffer[3][MAXCOLORMAPSIZE]);
    long DoExtension(long label);
    long GetDataBlock(unsigned char *buf);
    BOOL initLWZ(long input_code_size);
    long nextCode(long code_size);
    unsigned short * growStack();
    BOOL growTables();
    long readLWZ();
    long nextLWZ();
    void CalculateUpdateRect(int logicalRow0, int logicalRowN, BOOL *pfInvalidateAll, LONG *pyBottom);
    void ProgressiveDithering(int logicalFill, int logicalRow);
    HRESULT ReadImage(long len, long height, BOOL fInterlace, BOOL fGIFFrame,
        int crgbColors, RGBQUAD * prgbColors);

protected:
   IStream* m_pStream;
   CComPtr< IImageDecodeEventSink > m_pEventSink;
   CComPtr< IBitmapSurface > m_pBitmapSurface;
   CComPtr< IDirectDrawSurface > m_pDDrawSurface;
   RGBQUAD m_argbPalette[MAXCOLORMAPSIZE];
   ULONG m_nColors;
   DWORD m_dwEvents;
   ULONG m_nWidth;
   ULONG m_nHeight;

    BOOL                _fMustBeTransparent;
    BOOL				_fInvalidateAll;
    BOOL                _fDither;
    DWORD               _cbImage;
    BOOL                _fInterleaved;
    BOOL                _fProgAni;
    LONG                _yLogRow;
    LONG                _yLogRowDraw;
    LONG				_yBottom;
    LONG                _ySrcBot;
    LONG                 _yDithRow;
    void *              _pvDithData;
    LONG				_lTrans;
    GIFINFO             _gifinfo;
    GIFANIMATIONSTATE   _gas;
    GIFANIMDATA         _gad;
    RGBQUAD        		_argbFirst[256];
    int                 _mapConstrained[256];
    BYTE *              _pbSrcAlloc;
    BYTE *              _pbSrc;
    BYTE *              _pbDst;
};
