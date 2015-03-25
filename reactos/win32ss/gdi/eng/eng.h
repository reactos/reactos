#pragma once

extern const BYTE gajRop2ToRop3[16];

#define MIX_TO_ROP4(mix) \
    (((ULONG)gajRop2ToRop3[((mix) - 1) & 0xF]) | \
     ((ULONG)gajRop2ToRop3[(((mix) >> 8) - 1) & 0xF] << 8))

/* Copied from winddi.h, where it is only for vista+ */
_Acquires_lock_(_Global_critical_region_)
_Requires_lock_not_held_(*hsem)
_Acquires_shared_lock_(*hsem)
ENGAPI
VOID
NTAPI
EngAcquireSemaphoreShared(
    _Inout_ HSEMAPHORE hsem);

BOOL
APIENTRY
IntEngMaskBlt(
    _Inout_ SURFOBJ *psoDest,
    _In_ SURFOBJ *psoMask,
    _In_ CLIPOBJ *pco,
    _In_ XLATEOBJ *pxloDest,
    _In_ XLATEOBJ *pxloSource,
    _In_ RECTL *prclDest,
    _In_ POINTL *pptlMask,
    _In_ BRUSHOBJ *pbo,
    _In_ POINTL *pptlBrushOrg);

VOID
FASTCALL
IntEngWindowChanged(
    _In_ struct _WND *Window,
    _In_ FLONG flChanged);

ULONGLONG
APIENTRY
EngGetTickCount(
    VOID);

HANDLE
APIENTRY
EngSecureMemForRead(
    PVOID Address,
    ULONG Length);

VOID
DecompressBitmap(
    SIZEL Size,
    BYTE *CompressedBits,
    BYTE *UncompressedBits,
    LONG Delta,
    ULONG iFormat);
