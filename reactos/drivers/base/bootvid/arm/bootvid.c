#include "precomp.h"
#define NDEBUG
#include "debug.h"

/* PRIVATE FUNCTIONS *********************************************************/

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
BOOLEAN
NTAPI
VidInitialize(IN BOOLEAN SetMode)
{
    DPRINT1("bv-arm\n");
    UNIMPLEMENTED;
    while (TRUE);
    return FALSE;
}

/*
 * @implemented
 */
VOID
NTAPI
VidResetDisplay(IN BOOLEAN HalReset)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
ULONG
NTAPI
VidSetTextColor(ULONG Color)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @implemented
 */
VOID
NTAPI
VidDisplayStringXY(PUCHAR String,
                   ULONG Left,
                   ULONG Top,
                   BOOLEAN Transparent)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
VOID
NTAPI
VidSetScrollRegion(ULONG x1,
                   ULONG y1,
                   ULONG x2,
                   ULONG y2)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
VOID
NTAPI
VidCleanUp(VOID)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
VOID
NTAPI
VidBufferToScreenBlt(IN PUCHAR Buffer,
                     IN ULONG Left,
                     IN ULONG Top,
                     IN ULONG Width,
                     IN ULONG Height,
                     IN ULONG Delta)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
VOID
NTAPI
VidDisplayString(PUCHAR String)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
VOID
NTAPI
VidBitBlt(PUCHAR Buffer,
          ULONG Left,
          ULONG Top)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
VOID
NTAPI
VidScreenToBufferBlt(PUCHAR Buffer,
                     ULONG Left,
                     ULONG Top,
                     ULONG Width,
                     ULONG Height,
                     ULONG Delta)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
VOID
NTAPI
VidSolidColorFill(IN ULONG Left,
                  IN ULONG Top,
                  IN ULONG Right,
                  IN ULONG Bottom,
                  IN UCHAR Color)
{
    UNIMPLEMENTED;
}

