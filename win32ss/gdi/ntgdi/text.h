#pragma once

/* GDI logical font object */
typedef struct _LFONT LFONT, *PLFONT;

/*  Internal interface  */

/* dwFlags for IntGdiAddFontResourceEx */
#define AFRX_WRITE_REGISTRY 0x1
#define AFRX_ALTERNATIVE_PATH 0x2
#define AFRX_DOS_DEVICE_PATH 0x4

NTSTATUS FASTCALL TextIntCreateFontIndirect(CONST LPLOGFONTW lf, HFONT *NewFont);
BYTE FASTCALL IntCharSetFromCodePage(UINT uCodePage);
BOOL FASTCALL InitFontSupport(VOID);
#define FontGetObject LFONT_GetObject
BOOL FASTCALL IntLoadFontsInRegistry(VOID);
INT FASTCALL IntGdiAddFontResourceEx(PUNICODE_STRING FileName, DWORD Characteristics,
                                     DWORD dwFlags);

ULONG
FASTCALL
LFONT_GetObject(
    PLFONT plfont,
    ULONG Count,
    PVOID Buffer);

HFONT
NTAPI
GreCreateFontIndirectW(
    _In_ const LOGFONTW *plfw);

BOOL
NTAPI
GreTextOutW(
    _In_ HDC hdc,
    _In_ int xStart,
    _In_ int yStart,
    _In_ LPCWSTR pwcString,
    _In_ int cwc);

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

BOOL
WINAPI
GreGetTextMetricsW(
    _In_ HDC hdc,
    _Out_ LPTEXTMETRICW lptm);

BOOL
NTAPI
GreGetTextExtentW(
    HDC hdc,
    LPWSTR lpwsz,
    INT cwc,
    LPSIZE psize,
    UINT flOpts);

BOOL
NTAPI
GreGetTextExtentExW(
    IN HDC hdc,
    IN OPTIONAL LPWSTR lpwsz,
    IN ULONG cwc,
    IN ULONG dxMax,
    OUT OPTIONAL ULONG *pcch,
    OUT OPTIONAL PULONG pdxOut,
    OUT LPSIZE psize,
    IN FLONG fl);
