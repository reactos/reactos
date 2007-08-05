#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

typedef struct _GDI_TABLE_ENTRY
{
	PVOID KernelData; /* Points to the kernel mode structure */
	HANDLE ProcessId; /* process id that created the object, 0 for stock objects */
	LONG Type;        /* the first 16 bit is the object type including the stock obj flag, the last 16 bits is just the object type */
	PVOID UserData;   /* Points to the user mode structure, usually NULL though */
} GDI_TABLE_ENTRY, *PGDI_TABLE_ENTRY;

typedef PGDI_TABLE_ENTRY (CALLBACK * GDIQUERYPROC) (void);

/* GDI handle table can hold 0x4000 handles */
#define GDI_HANDLE_COUNT 0x10000
#define GDI_GLOBAL_PROCESS (0x0)

/* Handle Masks and shifts */
#define GDI_HANDLE_INDEX_MASK (GDI_HANDLE_COUNT - 1)
#define GDI_HANDLE_TYPE_MASK  0x007f0000
#define GDI_HANDLE_STOCK_MASK 0x00800000
#define GDI_HANDLE_REUSE_MASK 0xff000000
#define GDI_HANDLE_REUSECNT_SHIFT 24
#define GDI_HANDLE_UPPER_MASK 0xffff0000

/* Handle macros */
#define GDI_HANDLE_CREATE(i, t)    \
    ((HANDLE)(((i) & GDI_HANDLE_INDEX_MASK) | ((t) << 16)))

#define GDI_HANDLE_GET_INDEX(h)    \
    (((ULONG_PTR)(h)) & GDI_HANDLE_INDEX_MASK)

#define GDI_HANDLE_GET_TYPE(h)     \
    (((ULONG_PTR)(h)) & GDI_HANDLE_TYPE_MASK)

#define GDI_HANDLE_IS_TYPE(h, t)   \
    ((t) == (((ULONG_PTR)(h)) & GDI_HANDLE_TYPE_MASK))

#define GDI_HANDLE_IS_STOCKOBJ(h)  \
    (0 != (((ULONG_PTR)(h)) & GDI_HANDLE_STOCK_MASK))

#define GDI_HANDLE_SET_STOCKOBJ(h) \
    ((h) = (HANDLE)(((ULONG_PTR)(h)) | GDI_HANDLE_STOCK_MASK))

#define GDI_HANDLE_GET_UPPER(h)     \
    (((ULONG_PTR)(h)) & GDI_HANDLE_UPPER_MASK)

#define GDI_OBJECT_TYPE_DC          0x00010000
#define GDI_OBJECT_TYPE_REGION      0x00040000
#define GDI_OBJECT_TYPE_BITMAP      0x00050000
#define GDI_OBJECT_TYPE_PALETTE     0x00080000
#define GDI_OBJECT_TYPE_FONT        0x000a0000
#define GDI_OBJECT_TYPE_BRUSH       0x00100000
#define GDI_OBJECT_TYPE_EMF         0x00210000
#define GDI_OBJECT_TYPE_PEN         0x00300000
#define GDI_OBJECT_TYPE_EXTPEN      0x00500000
#define GDI_OBJECT_TYPE_COLORSPACE  0x00090000
#define GDI_OBJECT_TYPE_METADC      0x00660000
#define GDI_OBJECT_TYPE_METAFILE    0x00260000
#define GDI_OBJECT_TYPE_ENHMETAFILE 0x00460000
/* Following object types made up for ROS */
#define GDI_OBJECT_TYPE_ENHMETADC   0x00740000
#define GDI_OBJECT_TYPE_MEMDC       0x00750000
#define GDI_OBJECT_TYPE_DCE         0x00770000
#define GDI_OBJECT_TYPE_DONTCARE    0x007f0000
/** Not really an object type. Forces GDI_FreeObj to be silent. */
#define GDI_OBJECT_TYPE_SILENT      0x80000000



/* Number Representation */

typedef LONG FIX;

typedef struct _EFLOAT_S
{
    LONG lMant;
    LONG lExp;
} EFLOAT_S;

/* XFORM Structures */
typedef struct _MATRIX_S
{
    EFLOAT_S efM11;
    EFLOAT_S efM12;
    EFLOAT_S efM21;
    EFLOAT_S efM22;
    EFLOAT_S efDx;
    EFLOAT_S efDy;
    FIX fxDx;
    FIX fxDy;
    FLONG flAccel;
} MATRIX;

/* GDI object structures */

typedef struct _RGNATTR
{
    ULONG AttrFlags;
    ULONG Flags;
    RECTL Rect;
} RGNATTR,*PRGNATTR;

// Local DC structure (_DC_ATTR) PVOID pvLDC;
typedef struct _LDC
{
    HDC hDC;
    ULONG Flags;
    INT iType;
    PVOID pvEmfDC;        /* Pointer to ENHMETAFILE structure */
    ABORTPROC pAbortProc; /* AbortProc for Printing */
    HANDLE hPrinter;      /* Local or Remote Printer driver */
    INT iInitPage;        /* Start/Stop */
    INT iInitDocument;
} LDC, *PLDC;

typedef struct
{
  void *      pvLDC;                 // 000
  ULONG       ulDirty;
  HBRUSH      hbrush;
  HPEN        hpen;

  COLORREF    crBackgroundClr;       // 010
  ULONG       ulBackgroundClr;
  COLORREF    crForegroundClr;
  ULONG       ulForegroundClr;

#if (_WIN32_WINNT >= 0x0500)
  unsigned    unk020_00000000[4];    // 020
#endif
  int         iCS_CP;                // 030
  int         iGraphicsMode;
  BYTE        jROP2;                 // 038
  BYTE        jBkMode;
  BYTE        jFillMode;
  BYTE        jStretchBltMode;

  POINT       ptlCurrent;            // 03C
  POINTFX     ptfxCurrent;           // 044
  long        lBkMode;               // 04C

  long        lFillMode;             // 050
  long        lStretchBltMode;

#if (_WIN32_WINNT >= 0x0500)
  long        flFontMapper;          // 058
  long        lIcmMode;
  unsigned    hcmXform;              // 060
  HCOLORSPACE hColorSpace;
  unsigned    unk068_00000000;
  unsigned    IcmBrushColor;
  unsigned    IcmPenColor;           // 070
  unsigned    unk074_00000000;
#endif

  long        flTextAlign;           // 078
  long        lTextAlign;
  long        lTextExtra;            // 080
  long        lRelAbs;
  long        lBreakExtra;
  long        cBreak;

  HFONT       hlfntNew;              // 090
  MATRIX      mxWorldToDevice;       // 094
  MATRIX      mxDeviceToWorld;       // 0D0
  MATRIX      mxWorldToPage;         // 10C

  unsigned    unk048_00000000[8];    // 148

  int         iMapMode;              // 168

#if (_WIN32_WINNT >= 0x0500)
  DWORD       dwLayout;              // 16c
  long        lWindowOrgx;           // 170
#endif
  POINT       ptlWindowOrg;          // 174
  SIZE        szlWindowExt;          // 17c
  POINT       ptlViewportOrg;        // 184
  SIZE        szlViewportExt;        // 18c

  long        flXform;               // 194
  SIZE        szlVirtualDevicePixel; // 198
  SIZE        szlVirtualDeviceMm;    // 1a0
  POINT       ptlBrushOrigin;        // 1a8

  unsigned    unk1b0_00000000[2];    // 1b0
  unsigned    RectRegionFlag;        // 1b4
  RECT        VisRectRegion;         // 1b8
} DC_ATTR, *PDC_ATTR;


