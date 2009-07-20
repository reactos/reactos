#ifndef __WIN32K_DC_H
#define __WIN32K_DC_H

typedef struct _DC
{
    GDIOBJHDR    BaseObject;

    PPDEVOBJ     pPDevice;

    /* Device palette */
    HPALETTE     hPalette;

    PSURFACE     pBitmap;
    PBRUSHGDI    pFillBrush;
    PBRUSHGDI    pLineBrush;
    COLORREF     crForegroundClr;
    COLORREF     crBackgroundClr;

    /* Origins and extents */
    RECT rcDcRect; /* Relative to Vport */
    RECT rcVport;

    /* Combined clipping region */
    CLIPOBJ *CombinedClip;

    /* Transformations */
    MATRIX       mxWorldToDevice;
    MATRIX       mxDeviceToWorld;
    MATRIX       mxWorldToPage;
} DC, *PDC;

INT APIENTRY GreGetDeviceCaps(PDC pDC, INT cap);

/* NOTE! Following structures are direct copies from gdi_private.h */
typedef enum tagGdiPathState
{
   PATH_Null,
   PATH_Open,
   PATH_Closed
} GdiPathState;

typedef struct tagGdiPath
{
   GdiPathState state;
   POINT      *pPoints;
   BYTE         *pFlags;
   int          numEntriesUsed, numEntriesAllocated;
   BOOL       newStroke;
} GdiPath;

typedef struct tagGdiFont GdiFont;

struct saved_visrgn
{
    struct saved_visrgn *next;
    HRGN                 hrgn;
};

typedef struct tagUSERDC
{
    GDIOBJHDR    header;
    HDC          hSelf;            /* Handle to this DC */
    const struct tagDC_FUNCS *funcs; /* DC function table */
    PVOID        physDev;         /* Physical device (driver-specific) */
    DWORD        thread;          /* thread owning the DC */
    LONG         refcount;        /* thread refcount */
    LONG         dirty;           /* dirty flag */
    INT          saveLevel;
    HDC          saved_dc;
    DWORD_PTR    dwHookData;
    PVOID        hookProc;         /* the original SEGPTR ... */
    PVOID        hookThunk;        /* ... and the thunk to call it */

    INT          wndOrgX;          /* Window origin */
    INT          wndOrgY;
    INT          wndExtX;          /* Window extent */
    INT          wndExtY;
    INT          vportOrgX;        /* Viewport origin */
    INT          vportOrgY;
    INT          vportExtX;        /* Viewport extent */
    INT          vportExtY;
    FLOAT        miterLimit;

    int           flags;
    DWORD         layout;
    HRGN          hClipRgn;      /* Clip region (may be 0) */
    HRGN          hMetaRgn;      /* Meta region (may be 0) */
    HRGN          hMetaClipRgn;  /* Intersection of meta and clip regions (may be 0) */
    HRGN          hVisRgn;       /* Visible region (must never be 0) */
    HPEN          hPen;
    HBRUSH        hBrush;
    HFONT         hFont;
    HBITMAP       hBitmap;
    HANDLE        hDevice;
    HPALETTE      hPalette;

    GdiFont      *gdiFont;
    GdiPath       path;

    UINT          font_code_page;
    WORD          ROPmode;
    WORD          polyFillMode;
    WORD          stretchBltMode;
    WORD          relAbsMode;
    WORD          backgroundMode;
    COLORREF      backgroundColor;
    COLORREF      textColor;
    COLORREF      dcBrushColor;
    COLORREF      dcPenColor;
    short         brushOrgX;
    short         brushOrgY;

    WORD          textAlign;         /* Text alignment from SetTextAlign() */
    INT           charExtra;         /* Spacing from SetTextCharacterExtra() */
    INT           breakExtra;        /* breakTotalExtra / breakCount */
    INT           breakRem;          /* breakTotalExtra % breakCount */
    INT           MapMode;
    INT           GraphicsMode;      /* Graphics mode */
    ABORTPROC     pAbortProc;        /* AbortProc for Printing */
#ifndef __REACTOS__
    ABORTPROC16   pAbortProc16;
#endif
    INT           CursPosX;          /* Current position */
    INT           CursPosY;
    INT           ArcDirection;
    XFORM         xformWorld2Wnd;    /* World-to-window transformation */
    XFORM         xformWorld2Vport;  /* World-to-viewport transformation */
    XFORM         xformVport2World;  /* Inverse of the above transformation */
    BOOL          vport2WorldValid;  /* Is xformVport2World valid? */
    RECT          BoundsRect;        /* Current bounding rect */

    struct saved_visrgn *saved_visrgn;
} USERDC;

#endif
