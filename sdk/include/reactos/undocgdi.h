
#ifndef _UNDOCGDI_H
#define _UNDOCGDI_H

#ifdef __cplusplus
extern "C" {
#endif

#define DS_TILE 0x2
#define DS_TRANSPARENTALPHA 0x4
#define DS_TRANSPARENTCLR 0x8
#define DS_TRUESIZE 0x20

typedef struct GDI_DRAW_STREAM_TAG
{
    DWORD   signature;     // must be 0x44727753;//"Swrd"
    DWORD   reserved;      // must be 0
    HDC     hDC;           // handle to the device object of windiw to draw.
    RECT    rcDest;        // desination rect of dc to draw.
    DWORD   unknown1;      // must be 1.
    HBITMAP hImage;
    DWORD   unknown2;      // must be 9.
    RECT    rcClip;
    RECT    rcSrc;         // source rect of bitmap to draw.
    DWORD   drawOption;    // DS_ flags
    DWORD   leftSizingMargin;
    DWORD   rightSizingMargin;
    DWORD   topSizingMargin;
    DWORD   bottomSizingMargin;
    DWORD   crTransparent; // transparent color.
} GDI_DRAW_STREAM, *PGDI_DRAW_STREAM;

BOOL WINAPI GdiDrawStream(HDC dc, ULONG l, PGDI_DRAW_STREAM pDS);

BOOL WINAPI
GetTextExtentExPointWPri(
    HDC hdc,
    LPCWSTR lpwsz,
    INT cwc,
    INT dxMax,
    LPINT pcCh,
    LPINT pdxOut,
    LPSIZE psize);

BOOL WINAPI
GetFontResourceInfoW(
    _In_z_ LPCWSTR lpFileName,
    _Inout_ DWORD *pdwBufSize,
    _Out_writes_to_opt_(*pdwBufSize, 1) PVOID lpBuffer,
    _In_ DWORD dwType);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
