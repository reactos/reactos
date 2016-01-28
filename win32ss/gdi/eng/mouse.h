#pragma once

_Requires_lock_held_(*ppdev->hsemDevLock)
BOOL
NTAPI
MouseSafetyOnDrawStart(
    _Inout_ PPDEVOBJ ppdev,
    _In_ LONG HazardX1,
    _In_ LONG HazardY1,
    _In_ LONG HazardX2,
    _In_ LONG HazardY2);

_Requires_lock_held_(*ppdev->hsemDevLock)
BOOL
NTAPI
MouseSafetyOnDrawEnd(
    _Inout_ PPDEVOBJ ppdev);

ULONG
NTAPI
IntEngSetPointerShape(
    _In_ SURFOBJ *pso,
    _In_opt_ SURFOBJ *psoMask,
    _In_opt_ SURFOBJ *psoColor,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ LONG xHot,
    _In_ LONG yHot,
    _In_ LONG x,
    _In_ LONG y,
    _In_ RECTL *prcl,
    _In_ FLONG fl);

ULONG
NTAPI
GreSetPointerShape(
    _In_ HDC hdc,
    _In_opt_ HBITMAP hbmMask,
    _In_opt_ HBITMAP hbmColor,
    _In_ LONG xHot,
    _In_ LONG yHot,
    _In_ LONG x,
    _In_ LONG y,
    _In_ FLONG fl);

VOID
NTAPI
GreMovePointer(
    _In_ HDC hdc,
    _In_ LONG x,
    _In_ LONG y);

