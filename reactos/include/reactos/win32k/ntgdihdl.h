/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Win32 Graphical Subsystem (WIN32K)
 * FILE:            include/win32k/ntgdihal.h
 * PURPOSE:         Win32 Shared GDI Handle/Object Types
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#ifndef _NTGDIHDL_
#define _NTGDIHDL_

/* DEFINES *******************************************************************/

/* Base address where the handle table is mapped to */
#define GDI_HANDLE_TABLE_BASE_ADDRESS (0x400000)

/* GDI handle table can hold 0x4000 handles */
#define GDI_HANDLE_COUNT 0x4000
#define GDI_GLOBAL_PROCESS (0x0)

/* Handle Masks and shifts */
#define GDI_HANDLE_INDEX_MASK (GDI_HANDLE_COUNT - 1)
#define GDI_HANDLE_TYPE_MASK  0x007f0000
#define GDI_HANDLE_BASETYPE_MASK 0x001f0000
#define GDI_HANDLE_STOCK_MASK 0x00800000
#define GDI_HANDLE_REUSE_MASK 0xff000000
#define GDI_HANDLE_UPPER_MASK (GDI_HANDLE_TYPE_MASK|GDI_HANDLE_STOCK_MASK|GDI_HANDLE_REUSE_MASK)
#define GDI_HANDLE_REUSECNT_SHIFT 24
#define GDI_HANDLE_BASETYPE_SHIFT 16

#define GDI_ENTRY_STOCK_MASK 0x00000080
#define GDI_ENTRY_REUSE_MASK 0x0000ff00
#define GDI_ENTRY_REUSE_INC 0x00000100
#define GDI_ENTRY_BASETYPE_MASK 0x001f0000
#define GDI_ENTRY_REUSECNT_SHIFT 8
#define GDI_ENTRY_UPPER_SHIFT 16

#define GDI_OBJECT_TAG_DC       TAG('G', 'l', 'a', '1')
#define GDI_OBJECT_TAG_REGION   TAG('G', 'l', 'a', '4')
#define GDI_OBJECT_TAG_BITMAP   TAG('G', 'l', 'a', '5')
#define GDI_OBJECT_TAG_CLIOBJ   TAG('G', 'h', '0', '6')
#define GDI_OBJECT_TAG_PATH     TAG('G', 'h', '0', '7')
#define GDI_OBJECT_TAG_PALETTE  TAG('G', 'l', 'a', '8')
#define GDI_OBJECT_TAG_COLSPC   TAG('G', 'h', '0', '9')
#define GDI_OBJECT_TAG_FONT     TAG('G', 'l', 'a', ':')
#define GDI_OBJECT_TAG_PFE      TAG('G', 'h', '0', '<')
#define GDI_OBJECT_TAG_BRUSH    TAG('G', 'l', 'a', '@')
#define GDI_OBJECT_TAG_ENUMFONT TAG('G', 'h', '0', 'F')

#define GDI_OBJECT_TAG_DDRAW    TAG('D', 'h', ' ', '1')
#define GDI_OBJECT_TAG_DDSURF   TAG('D', 'h', ' ', '2')

/*! \defgroup GDI object types
 *
 *  GDI object types
 *
 */
/*@{*/
#define GDI_OBJECT_TYPE_DC            0x00010000
#define GDI_OBJECT_TYPE_DIRECTDRAW    0x00020000 /* Should be moved away from gdi objects */
#define GDI_OBJECT_TYPE_DD_SURFACE    0x00030000 /* Should be moved away from gdi objects */
#define GDI_OBJECT_TYPE_REGION        0x00040000
#define GDI_OBJECT_TYPE_BITMAP        0x00050000
#define GDI_OBJECT_TYPE_CLIOBJ        0x00060000
#define GDI_OBJECT_TYPE_PATH          0x00070000
#define GDI_OBJECT_TYPE_PALETTE       0x00080000
#define GDI_OBJECT_TYPE_COLORSPACE    0x00090000
#define GDI_OBJECT_TYPE_FONT          0x000a0000

#define GDI_OBJECT_TYPE_BRUSH         0x00100000
#define GDI_OBJECT_TYPE_DD_VIDEOPORT  0x00120000 /* Should be moved away from gdi objects */
#define GDI_OBJECT_TYPE_DD_MOTIONCOMP 0x00140000 /* Should be moved away from gdi objects */
#define GDI_OBJECT_TYPE_ENUMFONT      0x00160000

/* Following object types are derived types from the above base types
   use 0x001f0000 as mask to get the base type */
#define GDI_OBJECT_TYPE_EMF         0x00210000
#define GDI_OBJECT_TYPE_METAFILE    0x00260000
#define GDI_OBJECT_TYPE_ENHMETAFILE 0x00460000
#define GDI_OBJECT_TYPE_PEN         0x00300000
#define GDI_OBJECT_TYPE_EXTPEN      0x00500000
#define GDI_OBJECT_TYPE_METADC      0x00660000
/*#define GDI_OBJECT_TYPE_DD_PALETTE    0x00630000 unused at the moment, other value required */
/*#define GDI_OBJECT_TYPE_DD_CLIPPER    0x00640000 unused at the moment, other value required  */

/* Following object types made up for ROS */
#define GDI_OBJECT_TYPE_DONTCARE    0x007f0000
/** Not really an object type. Forces GDI_FreeObj to be silent. */
#define GDI_OBJECT_TYPE_SILENT      0x80000000
/*@}*/

/* Handle macros */
#define GDI_HANDLE_CREATE(i, t)    \
    ((HANDLE)(((i) & GDI_HANDLE_INDEX_MASK) | ((t) & GDI_HANDLE_TYPE_MASK)))

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

#define GDI_HANDLE_GET_REUSECNT(h)     \
    (((ULONG_PTR)(h)) >> GDI_HANDLE_REUSECNT_SHIFT)

#define GDI_ENTRY_GET_REUSECNT(e)     \
    ((((ULONG_PTR)(e)) & GDI_ENTRY_REUSE_MASK) >> GDI_ENTRY_REUSECNT_SHIFT)

#define GDI_OBJECT_GET_TYPE_INDEX(t) \
    ((t & GDI_HANDLE_BASETYPE_MASK) >> GDI_HANDLE_BASETYPE_SHIFT)

/* DC OBJ Types */
#define DC_TYPE_DIRECT 0  // normal device context
#define DC_TYPE_MEMORY 1  // memory device context
#define DC_TYPE_INFO   2  // information context

/* DC OBJ Flags */
#define DC_FLAG_DISPLAY            0x0001
#define DC_FLAG_DIRECT             0x0002
#define DC_FLAG_CANCELLED          0x0004
#define DC_FLAG_PERMANENT          0x0008
#define DC_FLAG_DIRTY_RAO          0x0010
#define DC_FLAG_ACCUM_WMGR         0x0020
#define DC_FLAG_ACCUM_APP          0x0040
#define DC_FLAG_RESET              0x0080
#define DC_FLAG_SYNCHRONIZEACCESS  0x0100
#define DC_FLAG_EPSPRINTINGESCAPE  0x0200
#define DC_FLAG_TEMPINFODC         0x0400
#define DC_FLAG_FULLSCREEN         0x0800
#define DC_FLAG_IN_CLONEPDEV       0x1000
#define DC_FLAG_REDIRECTION        0x2000
#define DC_FLAG_SHAREACCESS        0x4000

/* DC_ATTR Dirty Flags */
#define DIRTY_FILL                          0x00000001
#define DIRTY_LINE                          0x00000002
#define DIRTY_TEXT                          0x00000004
#define DIRTY_BACKGROUND                    0x00000008
#define DIRTY_CHARSET                       0x00000010
#define SLOW_WIDTHS                         0x00000020
#define DC_CACHED_TM_VALID                  0x00000040
#define DISPLAY_DC                          0x00000080
#define DIRTY_PTLCURRENT                    0x00000100
#define DIRTY_PTFXCURRENT                   0x00000200
#define DIRTY_STYLESTATE                    0x00000400
#define DC_PLAYMETAFILE                     0x00000800
#define DC_BRUSH_DIRTY                      0x00001000
#define DC_PEN_DIRTY                        0x00002000
#define DC_DIBSECTION                       0x00004000
#define DC_LAST_CLIPRGN_VALID               0x00008000
#define DC_PRIMARY_DISPLAY                  0x00010000
#define DC_MODE_DIRTY                       0x00200000
#define DC_FONTTEXT_DIRTY                   0x00400000

/* DC_ATTR LCD Flags */
#define LDC_LDC           0x00000001 // (init) local DC other than a normal DC
#define LDC_EMFLDC        0x00000002 // Enhance Meta File local DC
#define LDC_INIT_DOCUMENT 0x00000040
#define LDC_INIT_PAGE     0x00000080
#define LDC_KILL_DOCUMENT 0x00010000


/* DC_ATTR Xform Flags */
#define METAFILE_TO_WORLD_IDENTITY          0x00000001
#define WORLD_TO_PAGE_IDENTITY              0x00000002
#define DEVICE_TO_PAGE_INVALID              0x00000008
#define DEVICE_TO_WORLD_INVALID             0x00000010
#define WORLD_TRANSFORM_SET                 0x00000020
#define POSITIVE_Y_IS_UP                    0x00000040
#define INVALIDATE_ATTRIBUTES               0x00000080
#define PTOD_EFM11_NEGATIVE                 0x00000100
#define PTOD_EFM22_NEGATIVE                 0x00000200
#define ISO_OR_ANISO_MAP_MODE               0x00000400
#define PAGE_TO_DEVICE_IDENTITY             0x00000800
#define PAGE_TO_DEVICE_SCALE_IDENTITY       0x00001000
#define PAGE_XLATE_CHANGED                  0x00002000
#define PAGE_EXTENTS_CHANGED                0x00004000
#define WORLD_XFORM_CHANGED                 0x00008000

/* TYPES *********************************************************************/

typedef struct _GDI_TABLE_ENTRY
{
    PVOID KernelData; /* Points to the kernel mode structure */
    SHORT ProcessId;  /* process id that created the object, 0 for stock objects */
    SHORT nCount;     /* usage count of object handles */
    LONG  Type;       /* the first 16 bit is the object type including the stock obj flag, the last 16 bits is just the object type */
    PVOID UserData;   /* Points to the user mode structure, usually NULL though */
} GDI_TABLE_ENTRY, *PGDI_TABLE_ENTRY;

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

typedef struct _DC_ATTR
{
    PVOID pvLDC;
    ULONG ulDirty_;
    HANDLE hbrush;
    HANDLE hpen;
    COLORREF crBackgroundClr;
    ULONG ulBackgroundClr;
    COLORREF crForegroundClr;
    ULONG ulForegroundClr;
    COLORREF crBrushClr;
    ULONG ulBrushClr;
    COLORREF crPenClr;
    ULONG ulPenClr;
    DWORD iCS_CP;
    INT iGraphicsMode;
    BYTE jROP2;
    BYTE jBkMode;
    BYTE jFillMode;
    BYTE jStretchBltMode;
    POINTL ptlCurrent;
    POINTL ptfxCurrent;
    LONG lBkMode;
    LONG lFillMode;
    LONG lStretchBltMode;
    FLONG flFontMapper;
    LONG lIcmMode;
    HANDLE hcmXform;
    HCOLORSPACE hColorSpace;
    FLONG flIcmFlags;
    INT IcmBrushColor;
    INT IcmPenColor;
    PVOID pvLIcm;
    FLONG flTextAlign;
    LONG lTextAlign;
    LONG lTextExtra;
    LONG lRelAbs;
    LONG lBreakExtra;
    LONG cBreak;
    HANDLE hlfntNew;
    MATRIX_S mxWorldToDevice;
    MATRIX_S mxDevicetoWorld;
    MATRIX_S mxWorldToPage;
    EFLOAT_S efM11PtoD;
    EFLOAT_S efM22PtoD;
    EFLOAT_S efDxPtoD;
    EFLOAT_S efDyPtoD;
    INT iMapMode;
    DWORD dwLayout;
    LONG lWindowOrgx;
    POINTL ptlWindowOrg;
    SIZEL szlWindowExt;
    POINTL ptlViewportOrg;
    SIZEL szlViewportExt;
    FLONG flXform;
    SIZEL szlVirtualDevicePixel;
    SIZEL szlVirtualDeviceMm;
    SIZEL szlVirtualDeviceSize;
    POINTL ptlBrushOrigin;
    RGNATTR VisRectRegion;
} DC_ATTR, *PDC_ATTR;

typedef struct _BRUSH_ATTR
{
    LOGBRUSH logbrush;
    DWORD    dwUnused[3];
} BRUSH_ATTR, *PBRUSH_ATTR;

typedef struct _REGION_ATTR
{
    DWORD dwUnknown1; 
    DWORD dwUnknown2; 
    LONG right; 
    LONG bottom;
    LONG left; 
    LONG top; 
} REGION_ATTR, *PREGION_ATTR;

typedef struct _FONT_ATTR
{
    DWORD dwUnknown;
    void *pCharWidthData;
} FONT_ATTR, *PFONT_ATTR;

#endif
