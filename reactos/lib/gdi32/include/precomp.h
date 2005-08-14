/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/gdi32/include/precomp.h
 * PURPOSE:         User-Mode Win32 GDI Library Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

/* SDK/DDK/NDK Headers. */
#include <windows.h>
#include <ddraw.h>
#include <ddk/winddi.h>
#include <ddk/prntfont.h>
#include <ddrawgdi.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

/* Win32K External Headers */
#include <win32k/kapi.h>

#define NtUserGetDCBrushColor(hbr) \
  (COLORREF)NtUserCallTwoParam((DWORD)(hbr), OBJ_BRUSH, TWOPARAM_ROUTINE_GETDCCOLOR)

#define NtUserGetDCPenColor(hbr) \
  (COLORREF)NtUserCallTwoParam((DWORD)(hbr), OBJ_PEN, TWOPARAM_ROUTINE_GETDCCOLOR)

#define NtUserSetDCBrushColor(hbr, crColor) \
  (COLORREF)NtUserCallTwoParam((DWORD)(hbr), (DWORD)crColor, TWOPARAM_ROUTINE_SETDCBRUSHCOLOR)

#define NtUserSetDCPenColor(hbr, crColor) \
  (COLORREF)NtUserCallTwoParam((DWORD)(hbr), (DWORD)crColor, TWOPARAM_ROUTINE_SETDCPENCOLOR)

typedef int (CALLBACK* EMFPLAYPROC)( HDC, INT, HANDLE );
typedef DWORD FULLSCREENCONTROL;
typedef DWORD UNIVERSAL_FONT_ID;
typedef UNIVERSAL_FONT_ID *PUNIVERSAL_FONT_ID;
typedef DWORD REALIZATION_INFO;
typedef REALIZATION_INFO *PREALIZATION_INFO;
typedef DWORD CHWIDTHINFO;
typedef CHWIDTHINFO *PCHWIDTHINFO;

/* == GLOBAL VARIABLES ====================================================== */

extern PGDI_TABLE_ENTRY GdiHandleTable;
extern HANDLE hProcessHeap;
extern HANDLE CurrentProcessId;

/* == HEAP ================================================================== */

PVOID    HEAP_alloc ( DWORD len );
NTSTATUS HEAP_strdupA2W ( LPWSTR* ppszW, LPCSTR lpszA );
VOID     HEAP_free ( LPVOID memory );

/* == FONT ================================================================== */

BOOL FASTCALL TextMetricW2A(TEXTMETRICA *tma, TEXTMETRICW *tmw);
BOOL FASTCALL NewTextMetricW2A(NEWTEXTMETRICA *tma, NEWTEXTMETRICW *tmw);
BOOL FASTCALL NewTextMetricExW2A(NEWTEXTMETRICEXA *tma, NEWTEXTMETRICEXW *tmw);

/* == GDI HANDLES =========================================================== */

BOOL GdiIsHandleValid(HGDIOBJ hGdiObj);
BOOL GdiGetHandleUserData(HGDIOBJ hGdiObj, PVOID *UserData);

/* == BITMAP UTILITY FUNCTIONS ============================================== */

BOOL STDCALL CalculateColorTableSize(CONST BITMAPINFOHEADER *BitmapInfoHeader, UINT *ColorSpec, UINT *ColorTableSize);
LPBITMAPINFO STDCALL ConvertBitmapInfo(CONST BITMAPINFO *BitmapInfo, UINT ColorSpec, UINT *BitmapInfoSize, BOOL FollowedByData);

/* == CONVERSION FUNCTIONS ================================================== */
DEVMODEW *
STDCALL
GdiConvertToDevmodeW(DEVMODEA *dm);

VOID
STDCALL
LogFontA2W(LPLOGFONTW pW, CONST LOGFONTA *pA);

VOID
STDCALL
LogFontW2A(LPLOGFONTA pA, CONST LOGFONTW *pW);
/* EOF */
