#pragma once

INT FASTCALL GdiGetClipBox(HDC hDC, RECTL *rc);
VOID FASTCALL GdiSelectVisRgn(HDC hdc, PREGION prgn);
INT FASTCALL IntGdiExtSelectClipRgn (PDC dc, PREGION prgn, int fnMode);
VOID FASTCALL CLIPPING_UpdateGCRegion(DC* Dc);
