#ifndef __WIN32K_XLATEOBJ_H
#define __WIN32K_XLATEOBJ_H

typedef struct _XLATEGDI {
    XLATEOBJ XlateObj;
    HPALETTE DestPal;
    HPALETTE SourcePal;
    BOOL UseShiftAndMask;

    //  union {
    //    struct {            /* For Shift Translations */
    ULONG RedMask;
    ULONG GreenMask;
    ULONG BlueMask;
    INT RedShift;
    INT GreenShift;
    INT BlueShift;
    //    };
    //    struct {            /* For Color -> Mono Translations */
    ULONG BackgroundColor;
    //    };
    //  };
} XLATEGDI;

XLATEOBJ* NTAPI
IntEngCreateXlate(USHORT DestPalType, USHORT SourcePalType,
                  HPALETTE PaletteDest, HPALETTE PaletteSource);

VOID FASTCALL
EngDeleteXlate(XLATEOBJ *XlateObj);

XLATEOBJ* FASTCALL
IntCreateXlateForBlt(PDC pDCDest, PDC pDCSrc, SURFACE* psurfDest, SURFACE* psurfSrc);

#endif
