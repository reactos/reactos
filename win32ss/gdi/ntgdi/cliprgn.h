#pragma once

_Success_(return!=ERROR)
INT
FASTCALL
GdiGetClipBox(
    _In_ HDC hdc,
    _Out_ LPRECT prc);

VOID FASTCALL GdiSelectVisRgn(HDC hdc, PREGION prgn);
INT FASTCALL IntGdiExtSelectClipRgn (PDC dc, PREGION prgn, int fnMode);
VOID FASTCALL CLIPPING_UpdateGCRegion(DC* Dc);
VOID FASTCALL IntGdiReleaseRaoRgn(PDC);
VOID FASTCALL IntGdiReleaseVisRgn(PDC);
