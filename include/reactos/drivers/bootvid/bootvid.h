#ifndef _BOOTVID_
#define _BOOTVID_

BOOLEAN
NTAPI
VidInitialize(IN BOOLEAN SetMode);

VOID
NTAPI
VidResetDisplay(IN BOOLEAN HalReset);

ULONG
NTAPI
VidSetTextColor(ULONG Color);

VOID
NTAPI
VidDisplayStringXY(PUCHAR String,
                   ULONG Left,
                   ULONG Top,
                   BOOLEAN Transparent);

VOID
NTAPI
VidSetScrollRegion(ULONG x1,
                   ULONG y1,
                   ULONG x2,
                   ULONG y2);


VOID
NTAPI
VidCleanUp(VOID);

VOID
NTAPI
VidBufferToScreenBlt(IN PUCHAR Buffer,
                     IN ULONG Left,
                     IN ULONG Top,
                     IN ULONG Width,
                     IN ULONG Height,
                     IN ULONG Delta);

VOID
NTAPI
VidDisplayString(PUCHAR String);

VOID
NTAPI
VidBitBlt(PUCHAR Buffer,
          ULONG Left,
          ULONG Top);

VOID
NTAPI
VidScreenToBufferBlt(PUCHAR Buffer,
                     ULONG Left,
                     ULONG Top,
                     ULONG Width,
                     ULONG Height,
                     ULONG Delta);

VOID
NTAPI
VidSolidColorFill(IN ULONG Left,
                  IN ULONG Top,
                  IN ULONG Right,
                  IN ULONG Bottom,
                  IN UCHAR Color);

#endif
