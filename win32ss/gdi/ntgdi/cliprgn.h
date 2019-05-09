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
VOID FASTCALL UpdateVisRgn(PDC);
BOOL FASTCALL REGION_bCopy(PREGION,PREGION);
BOOL FASTCALL REGION_bIntersectRegion(PREGION,PREGION,PREGION);
int FASTCALL IntGdiExtSelectClipRect(PDC,PRECTL,int);
