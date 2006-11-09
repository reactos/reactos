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
#define GDI_HANDLE_STOCK_MASK 0x00800000
#define GDI_HANDLE_REUSE_MASK 0xff000000
#define GDI_HANDLE_REUSECNT_SHIFT 24

/*! \defgroup GDI object types
 *
 *  GDI object types
 *
 */
/*@{*/
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

#define LDC_LDC     0x01    // (init) local DC other than a normal DC
#define LDC_EMFLDC  0x02    // Enhance Meta File local DC

/* TYPES *********************************************************************/

typedef struct _GDI_TABLE_ENTRY
{
    PVOID KernelData; /* Points to the kernel mode structure */
    HANDLE ProcessId; /* process id that created the object, 0 for stock objects */
    LONG Type;        /* the first 16 bit is the object type including the stock obj flag, the last 16 bits is just the object type */
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
    ABORTPROC pAbortProc; /* AbortProc for Printing */
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
    INT IcmBrushColor;
    INT IcmPenColor;
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
    POINTL ptlBrushOrigin;
    RGNATTR VisRectRegion;
} DC_ATTR, *PDC_ATTR;

#endif
