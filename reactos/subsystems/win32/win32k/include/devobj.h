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
    PFILE_OBJECT VideoFileObject;
    BOOLEAN       PreparedDriver;
} PDEVOBJ, *PPDEVOBJ;

extern PPDEVOBJ pPrimarySurface;

#endif
