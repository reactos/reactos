#pragma once

BOOL FASTCALL co_UserRedrawWindow(PWND Wnd, const RECTL* UpdateRect, HRGN UpdateRgn, ULONG Flags);
VOID FASTCALL IntInvalidateWindows(PWND Window, HRGN hRgn, ULONG Flags);
BOOL FASTCALL IntGetPaintMessage(PWND Window, UINT MsgFilterMin, UINT MsgFilterMax, PTHREADINFO Thread, MSG *Message, BOOL Remove);
INT FASTCALL UserRealizePalette(HDC);
INT FASTCALL co_UserGetUpdateRgn(PWND, HRGN, BOOL);
VOID FASTCALL co_IntPaintWindows(PWND Window, ULONG Flags, BOOL Recurse);
BOOL FASTCALL IntValidateParent(PWND Child, HRGN hValidateRgn, BOOL Recurse);
BOOL FASTCALL IntIsWindowDirty(PWND);
BOOL FASTCALL IntEndPaint(PWND,PPAINTSTRUCT);
HDC FASTCALL IntBeginPaint(PWND,PPAINTSTRUCT);
HICON FASTCALL NC_IconForWindow( PWND );
