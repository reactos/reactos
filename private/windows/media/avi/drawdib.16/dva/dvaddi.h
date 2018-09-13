//
// DVADDDI.H
//
// Copyright (c) 1993 Microsoft Corporation
//
// DVA 1.0 Interface Definitions
//

#define DVAGETSURFACE       3074    // GDI Escape for DVA
#define DVA_VERSION         0x0100  // version number of DVA 1.0

//
// DVASURFACEINFO structure
//
typedef struct {                                               //
    BITMAPINFOHEADER BitmapInfo;                               // BITMAPINFO of surface
    DWORD            dwMask[3];                                // masks for BI_BITFIELDS
    DWORD            offSurface;                               // surface offset
    WORD             selSurface;                               // surface selector
    WORD             Version;                                  // DVA Version
    DWORD            Flags;                                    // Flags
    LPVOID           lpSurface;                                // driver use.
    BOOL (CALLBACK *OpenSurface) (LPVOID);                     // OpenSurface callback
    void (CALLBACK *CloseSurface)(LPVOID);                     // CloseSurface callback
    BOOL (CALLBACK *BeginAccess) (LPVOID,int,int,int,int);     // BeginAccess callback
    void (CALLBACK *EndAccess)   (LPVOID);                     // EndAccess callback
    UINT (CALLBACK *ShowSurface) (LPVOID,HWND,LPRECT,LPRECT);  // ShowSurface callback
} DVASURFACEINFO, FAR *LPDVASURFACEINFO;                       //

//
// Definitions for DVASURFACEINFO.dvaFlags
//
#define DVAF_1632_ACCESS    0x0001  // must access using 16:32 pointers
