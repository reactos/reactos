/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WGDI.H
 *  WOW32 16-bit GDI API support
 *
 *  History:
 *  Created 07-Mar-1991 by Jeff Parsons (jeffpar)
--*/

typedef struct _LINEDDADATA {   /* LineDDAdata */
    VPPROC  vpfnLineDDAProc;    // 16-bit function
    DWORD   dwUserDDAParam;     // user param, if any
} LINEDDADATA, *PLINEDDADATA;

typedef struct _ENUMOBJDATA {   /* LineDDAdata */
    INT     ObjType;
    VPPROC  vpfnEnumObjProc;     // 16-bit function
    VPVOID  vpObjData;
    DWORD   dwUserParam;         // user param, if any
} ENUMOBJDATA, *PENUMOBJDATA;

// 16-bit version of DRAWPATRECT which is used in Windows 3.1
typedef struct _DRAWPATRECT16 {
        POINT16 ptPosition;
        POINT16 ptSize;
        WORD wStyle;
        WORD wPattern;
} DRAWPATRECT16, *PDPR16;

ULONG FASTCALL   WG32CreateBitmap(PVDMFRAME pFrame);
ULONG FASTCALL   WG32CreateBitmapIndirect(PVDMFRAME pFrame);
ULONG FASTCALL   WG32CreateBrushIndirect(PVDMFRAME pFrame);
ULONG FASTCALL   WG32CreateCompatibleDC(PVDMFRAME pFrame);
ULONG FASTCALL   WG32CreateDC(PVDMFRAME pFrame);
ULONG FASTCALL   WG32CreateDIBPatternBrush(PVDMFRAME pFrame);
ULONG FASTCALL   WG32CreateDIBitmap(PVDMFRAME pFrame);
ULONG FASTCALL   WG32CreateEllipticRgnIndirect(PVDMFRAME pFrame);
ULONG FASTCALL   WG32CreateIC(PVDMFRAME pFrame);
ULONG FASTCALL   WG32CreatePatternBrush(PVDMFRAME pFrame);
ULONG FASTCALL   WG32CreatePenIndirect(PVDMFRAME pFrame);
ULONG FASTCALL   WG32CreatePolyPolygonRgn(PVDMFRAME pFrame);
ULONG FASTCALL   WG32CreatePolygonRgn(PVDMFRAME pFrame);
ULONG FASTCALL   WG32CreateRectRgnIndirect(PVDMFRAME pFrame);
ULONG FASTCALL   WG32DPtoLP(PVDMFRAME pFrame);
ULONG FASTCALL   WG32DeleteDC(PVDMFRAME pFrame);
ULONG FASTCALL   WG32DeleteObject(PVDMFRAME pFrame);
ULONG FASTCALL   WG32DeviceMode(PVDMFRAME pFrame);
ULONG FASTCALL   WG32EnumObjects(PVDMFRAME pFrame);
ULONG FASTCALL   WG32Escape(PVDMFRAME pFrame);
ULONG FASTCALL   WG32ExtDeviceMode(PVDMFRAME pFrame);
ULONG FASTCALL   WG32DeviceCapabilities(PVDMFRAME pFrame);
ULONG FASTCALL   WG32GetBitmapDimension(PVDMFRAME pFrame);
ULONG FASTCALL   WG32GetBrushOrg(PVDMFRAME pFrame);
ULONG FASTCALL   WG32GetClipBox(PVDMFRAME pFrame);
ULONG FASTCALL   WG32GetCurLogFont(PVDMFRAME pFrame);
ULONG FASTCALL   WG32GetCurrentPosition(PVDMFRAME pFrame);
ULONG FASTCALL   WG32GetDCOrg(PVDMFRAME pFrame);
ULONG FASTCALL   WG32GetDIBits(PVDMFRAME pFrame);
ULONG FASTCALL   WG32GetDeviceCaps(PVDMFRAME pFrame);
ULONG FASTCALL   WG32GetEnvironment(PVDMFRAME pFrame);
ULONG FASTCALL   WG32GetObject(PVDMFRAME pFrame);
ULONG FASTCALL   WG32GetObjectType(PVDMFRAME pFrame);
ULONG FASTCALL   WG32GetRgnBox(PVDMFRAME pFrame);
ULONG FASTCALL   WG32GetViewportExt(PVDMFRAME pFrame);
ULONG FASTCALL   WG32GetViewportOrg(PVDMFRAME pFrame);
ULONG FASTCALL   WG32GetWindowExt(PVDMFRAME pFrame);
ULONG FASTCALL   WG32GetWindowOrg(PVDMFRAME pFrame);
ULONG FASTCALL   WG32LPtoDP(PVDMFRAME pFrame);
ULONG FASTCALL   WG32LineDDA(PVDMFRAME pFrame);
ULONG FASTCALL   WG32MoveTo(PVDMFRAME pFrame);
ULONG FASTCALL   WG32OffsetViewportOrg(PVDMFRAME pFrame);
ULONG FASTCALL   WG32OffsetWindowOrg(PVDMFRAME pFrame);
ULONG FASTCALL   WG32PolyPolygon(PVDMFRAME pFrame);
ULONG FASTCALL   WG32Polygon(PVDMFRAME pFrame);
ULONG FASTCALL   WG32Polyline(PVDMFRAME pFrame);
ULONG FASTCALL   WG32PolyPolylineWOW(PVDMFRAME pFrame);
ULONG FASTCALL   WG32RectInRegion(PVDMFRAME pFrame);
ULONG FASTCALL   WG32RectVisible(PVDMFRAME pFrame);
ULONG FASTCALL   WG32ScaleViewportExt(PVDMFRAME pFrame);
ULONG FASTCALL   WG32ScaleWindowExt(PVDMFRAME pFrame);
LONG             W32AbortProc(HDC hPr, int code);
ULONG FASTCALL   WG32SetBitmapBits(PVDMFRAME pFrame);
ULONG FASTCALL   WG32SetBitmapDimension(PVDMFRAME pFrame);
ULONG FASTCALL   WG32SetBrushOrg(PVDMFRAME pFrame);
ULONG FASTCALL   WG32SetDIBits(PVDMFRAME pFrame);
ULONG FASTCALL   WG32SetDIBitsToDevice(PVDMFRAME pFrame);
ULONG FASTCALL   WG32SetViewportExt(PVDMFRAME pFrame);
ULONG FASTCALL   WG32SetViewportOrg(PVDMFRAME pFrame);
ULONG FASTCALL   WG32SetWindowExt(PVDMFRAME pFrame);
ULONG FASTCALL   WG32SetWindowOrg(PVDMFRAME pFrame);
ULONG FASTCALL   WG32StretchDIBits(PVDMFRAME pFrame);
ULONG FASTCALL   WG32CreateDIBSection(PVDMFRAME pFrame);
ULONG FASTCALL   WG32GetDIBColorTable(PVDMFRAME pFrame);
ULONG FASTCALL   WG32SetDIBColorTable(PVDMFRAME pFrame);
ULONG FASTCALL   WG32DMBitBlt(PVDMFRAME pFrame);

BOOL  IsFaxPrinterSupportedDevice(PSZ pszDevice);
BOOL  IsFaxPrinterWriteProfileString(PSZ szSection, PSZ szKey, PSZ szString);
DWORD GetFaxPrinterProfileString(PSZ szSection, PSZ szKey, PSZ szDefault, PSZ szRetBuf, DWORD cbBufSize);

ULONG Get_RLE_Compression_Size(DWORD RLE_Type, PBYTE pStart, VPVOID vpBytes);

HANDLE hConvert16to32 (int h16);
HAND16 hConvert32to16 (DWORD h32);

#ifdef FE_SB //GetFontAssocStatus, pisuih, 10/5/94'
ULONG FASTCALL   WG32GetFontAssocStatus(PVDMFRAME pFrame);
#endif  //FE_SB
