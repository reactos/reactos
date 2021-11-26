/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Win32 Graphical Subsystem (WIN32K)
 * FILE:            win32ss/include/ntgdihdl.h
 * PURPOSE:         Win32 Shared GDI Handle/Object Types
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#ifndef _NTGDIHDL_
#define _NTGDIHDL_

#include <winddi.h>

/* DEFINES *******************************************************************/

/* GDI handle table can hold 0x10000 handles */
#define GDI_HANDLE_COUNT 0x10000
#define GDI_CFONT_MAX 16

/* Handle Masks and shifts */
#define GDI_HANDLE_INDEX_MASK (GDI_HANDLE_COUNT - 1)
#define GDI_HANDLE_TYPE_MASK  0x007f0000
#define GDI_HANDLE_BASETYPE_MASK 0x001f0000
#define GDI_HANDLE_EXTYPE_MASK 0x00600000
#define GDI_HANDLE_STOCK_MASK 0x00800000
#define GDI_HANDLE_REUSE_MASK 0xff000000
#define GDI_HANDLE_UPPER_MASK (GDI_HANDLE_TYPE_MASK|GDI_HANDLE_STOCK_MASK|GDI_HANDLE_REUSE_MASK)
#define GDI_HANDLE_REUSECNT_SHIFT 24
#define GDI_HANDLE_BASETYPE_SHIFT 16

#define GDI_ENTRY_STOCK_MASK 0x00000080
#define GDI_ENTRY_BASETYPE_MASK 0x001f0000
#define GDI_ENTRY_UPPER_SHIFT 16

/* GDI Entry Flags */
#define GDI_ENTRY_UNDELETABLE  1    /* Mark Object as nonremovable */
#define GDI_ENTRY_DELETING     2    /* Used when deleting Font Objects */
#define GDI_ENTRY_VALIDATE_VIS 4    /* Validating Visible region data */
#define GDI_ENTRY_ALLOCATE_LAL 0x80 /* Object Allocated with Look aside List */

/*! \defgroup GDI object types
 *
 *  GDI object types
 *
 */
/*@{*/
#define GDI_OBJECT_TYPE_DC            0x00010000
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
#define GDI_OBJECT_TYPE_DRIVEROBJ     0x001C0000

/* Confrim on XP value is taken from NtGdiCreateDirectDrawObject */
#define GDI_OBJECT_TYPE_DIRECTDRAW  0x00200000

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

#define GDI_OBJECT_GET_TYPE_INDEX(t) \
    ((t & GDI_HANDLE_BASETYPE_MASK) >> GDI_HANDLE_BASETYPE_SHIFT)

/* Gdi Object Handle Managment Pid lock masking sets. */
/* Ref: used with DxEngSetDCOwner */
#define GDI_OBJ_HMGR_PUBLIC     0          /* Public owner, Open access? */
#define GDI_OBJ_HMGR_POWNED     0x80000002 /* Set to current owner. */
#define GDI_OBJ_HMGR_NONE       0x80000012 /* No owner, Open access? */
#define GDI_OBJ_HMGR_RESTRICTED 0x80000022 /* Restricted? */


/* DC OBJ Types */
#define DC_TYPE_DIRECT 0  /* normal device context */
#define DC_TYPE_MEMORY 1  /* memory device context */
#define DC_TYPE_INFO   2  /* information context */

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
#define DC_ICM_NOT_CALIBRATED               0x00020000
#define DC_ICM_BRUSH_DIRTY                  0x00040000
#define DC_ICM_PEN_DIRTY                    0x00080000
#define DC_ICM_NOT_SET                      0x00100000
#define DC_MODE_DIRTY                       0x00200000
#define DC_FONTTEXT_DIRTY                   0x00400000

/* DC_ATTR LCD Flags */
#define LDC_LDC           0x00000001 /* (init) local DC other than a normal DC */
#define LDC_EMFLDC        0x00000002 /* Enhance Meta File local DC */
#define LDC_SAPCALLBACK   0x00000020
#define LDC_INIT_DOCUMENT 0x00000040
#define LDC_INIT_PAGE     0x00000080
#define LDC_STARTPAGE     0x00000100
#define LDC_NEXTBAND      0x00000200
#define LDC_FONTHASH      0x00001000
#define LDC_CLOCKWISE     0x00002000
#define LDC_NEWFONT       0x00008000
#define LDC_KILL_DOCUMENT 0x00010000
#define LDC_META_PRINT    0x00020000
#define LDC_DIRECT        0x00040000
#define LDC_RESET_BANDING 0x00080000
#define LDC_DOWNLOADFONTS 0x00100000
#define LDC_RESETDC       0x00200000
#define LDC_UFIMAP        0x00400000
#define LDC_INFODC        0x01000000 /* If CreateIC was passed. */
#define LDC_DEVCAPS       0x02000000
#define LDC_XPS_PASS      0x08000000 // Guessing, not sure.
#define LDC_ATENDPAGE     0x10000000
#define LDC_COLORPAGE     0x20000000

#define UFIHASHTABLESIZE  64

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

/* BRUSH/RGN_ATTR Flags */
#define ATTR_CACHED                         0x00000001
#define ATTR_TO_BE_DELETED                  0x00000002
#define ATTR_NEW_COLOR                      0x00000004
#define ATTR_CANT_SELECT                    0x00000008
#define ATTR_RGN_VALID                      0x00000010
#define ATTR_RGN_DIRTY                      0x00000020

/* Set/Clear Bitmap/Brush Stock Attribute */
#define SC_BB_STOCKOBJ 1

/* TYPES *********************************************************************/

typedef struct _GDI_TABLE_ENTRY
{
    PVOID KernelData; /* Points to the kernel mode structure */
    DWORD ProcessId;  /* process id that created the object, 0 for stock objects */
    union{            /* temp union structure. */
    LONG  Type;       /* the first 16 bit is the object type including the stock obj flag, the last 16 bits is just the object type */
    struct{
    USHORT FullUnique; /* unique */
    UCHAR  ObjectType; /* objt */
    UCHAR  Flags;      /* Flags */
    };};
    PVOID UserData;   /* pUser Points to the user mode structure, usually NULL though */
} GDI_TABLE_ENTRY, *PGDI_TABLE_ENTRY;

typedef struct _ENTRY
{
    union _EINFO
    {
        struct _BASEOBJECT *pobj;
        HGDIOBJ hFree;
    } einfo;

    union _OBJECTOWNER
    {
        struct _OBJECTOWNER_S
        {
            unsigned Lock:1;
            unsigned Pid_Shifted:31;
        } Share;
        ULONG ulObj;
    } ObjectOwner;

    USHORT FullUnique;
    UCHAR Objt;
    UCHAR Flags;
    PVOID pUser;
} ENTRY, *PENTRY;

/*
 * User space only structure!
 */
typedef struct __GDI_SHARED_HANDLE_TABLE /* Must match win32k/include/gdiobj.h */
{
    GDI_TABLE_ENTRY Entries[GDI_HANDLE_COUNT]; /* Handle table. */
    DEVCAPS         DevCaps;                   /* Shared device capabilities. */
    FLONG           flDeviceUniq;              /* Device settings uniqueness. */
    PVOID           pvLangPack;                /* Lanuage Pack. */
    CFONT           cfPublic[GDI_CFONT_MAX];   /* Public Fonts. */
    DWORD           dwCFCount;
} GDI_SHARED_HANDLE_TABLE, *PGDI_SHARED_HANDLE_TABLE;

typedef struct _RGN_ATTR
{
    ULONG AttrFlags;
    ULONG iComplexity;     /* Clipping region's complexity. NULL, SIMPLE & COMPLEXREGION */
    RECTL Rect;
} RGN_ATTR,*PRGN_ATTR;

/* Local DC structure (_DC_ATTR) PVOID pvLDC; */
typedef struct _LDC
{
    HDC hDC;
    ULONG Flags;
    INT iType;
    PVOID pvEmfDC;        /* Pointer to ENHMETAFILE structure */
    LPWSTR pwszPort;
    ABORTPROC pAbortProc; /* AbortProc for Printing */
    DWORD CallBackTick;
    HANDLE hPrinter;      /* Local or Remote Printer driver */
    PDEVMODEW pdm;
    PVOID pUMPDev;        /* Ptr to User Mode Printer Device structure */
    PUMDHPDEV pUMdhpdev;  /* Ptr to Combined UMPD and DHPDEV structure */
    PVOID UFIHashTable[3];
    UNIVERSAL_FONT_ID ufi;
    PVOID pvEMFSpoolData;
    ULONG cjSize;
    LIST_ENTRY leRecords;
    DEVCAPS DevCaps;
    HBRUSH BrushColor;
    HPEN PenColor;
    // wine data
    DWORD dwData[5];
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
    MATRIX mxWorldToDevice;
    MATRIX mxDeviceToWorld;
    MATRIX mxWorldToPage;
    FLOATOBJ efM11PtoD;
    FLOATOBJ efM22PtoD;
    FLOATOBJ efDxPtoD;
    FLOATOBJ efDyPtoD;
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
    RGN_ATTR VisRectRegion;
} DC_ATTR, *PDC_ATTR;

typedef struct _BRUSH_ATTR /* Used with pen too. */
{
    FLONG    AttrFlags;
    COLORREF lbColor;
} BRUSH_ATTR, *PBRUSH_ATTR;

typedef struct _FONT_ATTR
{
    BOOL    bSlowWidths;
    PCFONT  pCharWidthData;
} FONT_ATTR, *PFONT_ATTR;


#endif
