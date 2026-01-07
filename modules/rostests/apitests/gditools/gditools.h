
#pragma once

extern HBITMAP ghbmp1, ghbmp1_InvCol, ghbmp1_RB, ghbmp4, ghbmp8, ghbmp16, ghbmp24, ghbmp32;
extern HBITMAP ghbmpDIB1, ghbmpDIB1_InvCol, ghbmpDIB1_RB, ghbmpDIB4, ghbmpDIB8, ghbmpDIB16, ghbmpDIB24, ghbmpDIB32;
extern HDC ghdcDIB1, ghdcDIB1_InvCol, ghdcDIB1_RB, ghdcDIB4, ghdcDIB8, ghdcDIB16, ghdcDIB24, ghdcDIB32;
extern PVOID gpvDIB1, gpvDIB1_InvCol, gpvDIB1_RB, gpvDIB4, gpvDIB8, gpvDIB16, gpvDIB24, gpvDIB32;
extern HDC ghdcInfo;

extern HBITMAP ghbmpDIB32;
//extern PULONG pulDIB32Bits;
extern PULONG pulDIB4Bits;
extern HPALETTE ghpal;
typedef struct
{
    WORD palVersion;
    WORD palNumEntries;
    PALETTEENTRY logpalettedata[8];
} MYPAL;

extern ULONG (*gpDIB32)[8][8];

extern MYPAL gpal;

BOOL GdiToolsInit(void);

PENTRY
GdiQueryTable(
    VOID);

BOOL
GdiIsHandleValid(
    _In_ HGDIOBJ hobj);

BOOL
GdiIsHandleValidEx(
    _In_ HGDIOBJ hobj,
    _In_ GDILOOBJTYPE ObjectType);

PVOID
GdiGetHandleUserData(
    _In_ HGDIOBJ hobj);

BOOL
ChangeScreenBpp(
    _In_ ULONG cBitsPixel,
    _Out_ PULONG pcOldBitsPixel);
