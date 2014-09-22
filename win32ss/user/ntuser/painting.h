#pragma once

BOOL FASTCALL co_UserRedrawWindow(PWND Wnd, const RECTL* UpdateRect, PREGION UpdateRgn, ULONG Flags);
VOID FASTCALL IntInvalidateWindows(PWND Window, PREGION Rgn, ULONG Flags);
BOOL FASTCALL IntGetPaintMessage(PWND Window, UINT MsgFilterMin, UINT MsgFilterMax, PTHREADINFO Thread, MSG *Message, BOOL Remove);
INT FASTCALL UserRealizePalette(HDC);
INT FASTCALL co_UserGetUpdateRgn(PWND, PREGION, BOOL);
VOID FASTCALL co_IntPaintWindows(PWND Window, ULONG Flags, BOOL Recurse);
BOOL FASTCALL IntValidateParent(PWND Child, PREGION ValidateRgn, BOOL Recurse);
BOOL FASTCALL IntIsWindowDirty(PWND);
BOOL FASTCALL IntEndPaint(PWND,PPAINTSTRUCT);
HDC FASTCALL IntBeginPaint(PWND,PPAINTSTRUCT);
HICON FASTCALL NC_IconForWindow( PWND );
