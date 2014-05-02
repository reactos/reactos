#pragma once

INT FASTCALL GdiGetClipBox(HDC hDC, RECTL *rc);
INT FASTCALL GdiSelectVisRgn(HDC hdc, HRGN hrgn);
INT FASTCALL GdiExtSelectClipRgn (PDC dc, HRGN hrgn, int fnMode);
int FASTCALL CLIPPING_UpdateGCRegion(DC* Dc);
