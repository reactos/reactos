#pragma once

/* GDI logical font object */
typedef struct _LFONT TEXTOBJ, *PTEXTOBJ;

/*  Internal interface  */

#define  TEXTOBJ_UnlockText(pBMObj) GDIOBJ_vUnlockObject ((POBJ)pBMObj)
/* dwFlags for IntGdiAddFontResourceEx */
#define AFRX_WRITE_REGISTRY 0x1
#define AFRX_ALTERNATIVE_PATH 0x2
#define AFRX_DOS_DEVICE_PATH 0x4

NTSTATUS FASTCALL TextIntCreateFontIndirect(CONST LPLOGFONTW lf, HFONT *NewFont);
BYTE FASTCALL IntCharSetFromCodePage(UINT uCodePage);
BOOL FASTCALL InitFontSupport(VOID);
ULONG FASTCALL FontGetObject(PTEXTOBJ TextObj, ULONG Count, PVOID Buffer);
BOOL FASTCALL IntLoadFontsInRegistry(VOID);
INT FASTCALL IntGdiAddFontResourceEx(PUNICODE_STRING FileName, DWORD Characteristics,
                                     DWORD dwFlags);
BOOL FASTCALL GreTextOutW(HDC,int,int,LPCWSTR,int);
HFONT FASTCALL GreCreateFontIndirectW( LOGFONTW * );
BOOL WINAPI GreGetTextMetricsW( _In_  HDC hdc, _Out_ LPTEXTMETRICW lptm);

BOOL
NTAPI
GreExtTextOutW(
    IN HDC,
    IN INT,
    IN INT,
    IN UINT,
    IN OPTIONAL RECTL*,
    IN LPWSTR,
    IN INT,
    IN OPTIONAL LPINT,
    IN DWORD);
