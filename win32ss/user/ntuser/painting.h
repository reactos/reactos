#pragma once

#define FLASHW_MASK         0x0000000f
#define FLASHW_SYSTIMER     0x00000400
#define FLASHW_FINISHED     0x00000800
#define FLASHW_STARTED      0x00001000
#define FLASHW_COUNT        0x00002000
#define FLASHW_KILLSYSTIMER 0x00004000
#define FLASHW_ACTIVE       0x00008000

#define PRGN_NULL    ((PREGION)0) /* NULL empty region */
#define PRGN_WINDOW  ((PREGION)1) /* region from window rcWindow */
#define PRGN_MONITOR ((PREGION)2) /* region from monitor region. */

#define GreCreateRectRgnIndirect(prc) \
  NtGdiCreateRectRgn((prc)->left, (prc)->top, (prc)->right, (prc)->bottom)

#define GreSetRectRgnIndirect(hRgn, prc) \
  NtGdiSetRectRgn(hRgn, (prc)->left, (prc)->top, (prc)->right, (prc)->bottom);
  
BOOL FASTCALL co_UserRedrawWindow(PWND Wnd, const RECTL* UpdateRect, PREGION UpdateRgn, ULONG Flags);
VOID FASTCALL IntInvalidateWindows(PWND Window, PREGION Rgn, ULONG Flags);
BOOL FASTCALL IntGetPaintMessage(PWND Window, UINT MsgFilterMin, UINT MsgFilterMax, PTHREADINFO Thread, MSG *Message, BOOL Remove);
INT FASTCALL UserRealizePalette(HDC);
INT FASTCALL co_UserGetUpdateRgn(PWND, HRGN, BOOL);
BOOL FASTCALL co_UserGetUpdateRect(PWND, PRECT, BOOL);
VOID FASTCALL co_IntPaintWindows(PWND Window, ULONG Flags, BOOL Recurse);
VOID FASTCALL IntSendSyncPaint(PWND, ULONG);
VOID FASTCALL co_IntUpdateWindows(PWND, ULONG, BOOL);
BOOL FASTCALL IntIsWindowDirty(PWND);
BOOL FASTCALL IntEndPaint(PWND,PPAINTSTRUCT);
HDC FASTCALL IntBeginPaint(PWND,PPAINTSTRUCT);
PCURICON_OBJECT FASTCALL NC_IconForWindow( PWND );
BOOL FASTCALL IntFlashWindowEx(PWND,PFLASHWINFO);
BOOL FASTCALL IntIntersectWithParents(PWND, RECTL *);
BOOL FASTCALL IntIsWindowDrawable(PWND);
BOOL UserDrawCaption(PWND,HDC,RECTL*,HFONT,HICON,const PUNICODE_STRING,UINT);
VOID FASTCALL UpdateThreadWindows(PWND,PTHREADINFO,HRGN);

