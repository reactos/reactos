#pragma once

/* GDI surface object */
typedef struct _SURFACE
{
    BASEOBJECT  BaseObject;

    SURFOBJ     SurfObj;
    //XDCOBJ *   pdcoAA;
    FLONG       flags;
    struct _PALETTE  * const ppal; // Use SURFACE_vSetPalette to assign a palette
    //UINT       unk_050;

    union
    {
        HANDLE  hSecureUMPD;  // if UMPD_SURFACE set
        HANDLE  hMirrorParent;// if MIRROR_SURFACE set
        HANDLE  hDDSurface;   // if DIRECTDRAW_SURFACE set
    };

    SIZEL       sizlDim;      /* For SetBitmapDimension(), do NOT use
                               to get width/height of bitmap, use
                               bitmap.bmWidth/bitmap.bmHeight for
                               that */

    HDC         hdc;          // Doc in "Undocumented Windows", page 546, seems to be supported with XP.
    ULONG       cRef;
    HPALETTE    hpalHint;

    /* For device-independent bitmaps: */
    HANDLE      hDIBSection;
    HANDLE      hSecure;
    DWORD       dwOffset;
    //UINT       unk_078;

  /* reactos specific */
    DWORD biClrImportant;
} SURFACE, *PSURFACE;

/* flags field */
enum _SURFACEFLAGS
{
//#define HOOK_BITBLT               0x00000001
//#define HOOK_STRETCHBLT           0x00000002
//#define HOOK_PLGBLT               0x00000004
//#define HOOK_TEXTOUT              0x00000008
//#define HOOK_PAINT                0x00000010
//#define HOOK_STROKEPATH           0x00000020
//#define HOOK_FILLPATH             0x00000040
//#define HOOK_STROKEANDFILLPATH    0x00000080
//#define HOOK_LINETO               0x00000100
    SHAREACCESS_SURFACE       = 0x00000200,
//#define HOOK_COPYBITS             0x00000400
//#define REDIRECTION_SURFACE       0x00000800 // ?
//#define HOOK_MOVEPANNING          0x00000800
//#define HOOK_SYNCHRONIZE          0x00001000
//#define HOOK_STRETCHBLTROP        0x00002000
//#define HOOK_SYNCHRONIZEACCESS    0x00004000
//#define USE_DEVLOCK_SURFACE       0x00004000
//#define HOOK_TRANSPARENTBLT       0x00008000
//#define HOOK_ALPHABLEND           0x00010000
//#define HOOK_GRADIENTFILL         0x00020000
//#if (NTDDI_VERSION < 0x06000000)
// #define HOOK_FLAGS               0x0003B5FF
//#else
// #define HOOK_FLAGS               0x0003B5EF
//#endif
    UMPD_SURFACE              = 0x00040000,
    MIRROR_SURFACE            = 0x00080000,
    DIRECTDRAW_SURFACE        = 0x00100000,
    DRIVER_CREATED_SURFACE    = 0x00200000,
    ENG_CREATE_DEVICE_SURFACE = 0x00400000,
    DDB_SURFACE               = 0x00800000,
    LAZY_DELETE_SURFACE       = 0x01000000,
    BANDING_SURFACE           = 0x02000000,
    API_BITMAP                = 0x04000000,
    PALETTE_SELECT_SET        = 0x08000000,
    UNREADABLE_SURFACE        = 0x10000000,
    DYNAMIC_MODE_PALETTE      = 0x20000000,
    ABORT_SURFACE             = 0x40000000,
    PDEV_SURFACE              = 0x80000000
};

#define BMF_POOLALLOC 0x100

/*  Internal interface  */

#define SURFACE_AllocSurfaceWithHandle()    ((PSURFACE) GDIOBJ_AllocObjWithHandle(GDI_OBJECT_TYPE_BITMAP, sizeof(SURFACE)))

/* NOTE: Use shared locks! */
#define  SURFACE_ShareLockSurface(hBMObj) \
  ((PSURFACE) GDIOBJ_ShareLockObj ((HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_BITMAP))
#define  SURFACE_UnlockSurface(pBMObj)  \
  GDIOBJ_vUnlockObject ((POBJ)pBMObj)
#define  SURFACE_ShareUnlockSurface(pBMObj)  \
  GDIOBJ_vDereferenceObject ((POBJ)pBMObj)

#define GDIDEV(SurfObj) ((PDEVOBJ *)((SurfObj)->hdev))
#define GDIDEVFUNCS(SurfObj) ((PDEVOBJ *)((SurfObj)->hdev))->DriverFunctions

extern UCHAR gajBitsPerFormat[];
#define BitsPerFormat(Format) gajBitsPerFormat[Format]

#define WIDTH_BYTES_ALIGN32(cx, bpp) ((((cx) * (bpp) + 31) & ~31) >> 3)
#define WIDTH_BYTES_ALIGN16(cx, bpp) ((((cx) * (bpp) + 15) & ~15) >> 3)

ULONG
FASTCALL
BitmapFormat(ULONG cBits, ULONG iCompression);

VOID
NTAPI
SURFACE_vCleanup(PVOID ObjectBody);

PSURFACE
NTAPI
SURFACE_AllocSurface(
    _In_ USHORT iType,
    _In_ ULONG cx,
    _In_ ULONG cy,
    _In_ ULONG iFormat,
    _In_ ULONG fjBitmap,
    _In_opt_ ULONG cjWidth,
    _In_opt_ ULONG cjBits,
    _In_opt_ PVOID pvBits);

FORCEINLINE
VOID
SURFACE_vSetPalette(
    _Inout_ PSURFACE psurf,
    _In_ PPALETTE ppal)
{
    if (psurf->ppal)
        GDIOBJ_vDereferenceObject((POBJ)psurf->ppal);
    if (ppal)
        GDIOBJ_vReferenceObjectByPointer((POBJ)ppal);
    *(PVOID*)&psurf->ppal = ppal;
}

