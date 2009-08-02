#ifndef __WIN32K_DEVOBJ_H
#define __WIN32K_DEVOBJ_H

typedef struct _PDEVOBJ
{
    DEVINFO                   DevInfo;
    GDIINFO                   GDIInfo;
    PFN_DrvMovePointer        pfnMovePointer;
    DHPDEV                    hPDev;
    PVOID                     pdmwDev;
    HSURF                     FillPatterns[HS_DDI_MAX];
    HSURF                     pSurface;
    union
    {
      DRIVER_FUNCTIONS        DriverFunctions;
      PVOID                   apfn[INDEX_LAST];
    };
    ULONG         DisplayNumber;
    DEVMODEW      DMW;
    PFILE_OBJECT  VideoFileObject;
    BOOLEAN       PreparedDriver;
    GDIPOINTER    Pointer;
    /* Stuff to keep track of software cursors; win32k gdi part */
    UINT SafetyRemoveLevel; /* at what level was the cursor removed?
                              0 for not removed */
    UINT SafetyRemoveCount;
} PDEVOBJ, *PPDEVOBJ;

extern PPDEVOBJ pPrimarySurface;

#endif
