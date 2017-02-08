#pragma once

INT  NTAPI MouseSafetyOnDrawStart(PPDEVOBJ ppdev, LONG HazardX1, LONG HazardY1, LONG HazardX2, LONG HazardY2);
INT  NTAPI MouseSafetyOnDrawEnd(PPDEVOBJ ppdev);

ULONG
NTAPI
GreSetPointerShape(
    HDC hdc,
    HBITMAP hbmMask,
    HBITMAP hbmColor,
    LONG xHot,
    LONG yHot,
    LONG x,
    LONG y,
    FLONG fl);

VOID
NTAPI
GreMovePointer(
    HDC hdc,
    LONG x,
    LONG y);

