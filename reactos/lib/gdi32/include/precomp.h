#include <wine/windef.h>
#include <windows.h>
#include <ddraw.h>
#include <ddentry.h>
#include <string.h>
#include <win32k/kapi.h>
#include <ddk/prntfont.h>
#include <rosrtl/logfont.h>
#include <rosrtl/devmode.h>
#include <wine/unicode.h>
#define NTOS_MODE_USER
#include <ntos.h>

#define NtUserGetDCBrushColor(hbr) \
  (COLORREF)NtUserCallTwoParam((DWORD)(hbr), OBJ_BRUSH, TWOPARAM_ROUTINE_GETDCCOLOR)

#define NtUserGetDCPenColor(hbr) \
  (COLORREF)NtUserCallTwoParam((DWORD)(hbr), OBJ_PEN, TWOPARAM_ROUTINE_GETDCCOLOR)

#define NtUserSetDCBrushColor(hbr, crColor) \
  (COLORREF)NtUserCallTwoParam((DWORD)(hbr), (DWORD)crColor, TWOPARAM_ROUTINE_SETDCBRUSHCOLOR)

#define NtUserSetDCPenColor(hbr, crColor) \
  (COLORREF)NtUserCallTwoParam((DWORD)(hbr), (DWORD)crColor, TWOPARAM_ROUTINE_SETDCPENCOLOR)

#ifdef __USE_W32API
typedef int (CALLBACK* EMFPLAYPROC)( HDC, INT, HANDLE );
typedef DWORD FULLSCREENCONTROL;
typedef DWORD UNIVERSAL_FONT_ID;
typedef UNIVERSAL_FONT_ID *PUNIVERSAL_FONT_ID;
typedef DWORD REALIZATION_INFO;
typedef REALIZATION_INFO *PREALIZATION_INFO;
typedef DWORD CHWIDTHINFO;
typedef CHWIDTHINFO *PCHWIDTHINFO;
#endif

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

BOOL STDCALL CalculateColorTableSize(LPBITMAPINFOHEADER BitmapInfoHeader, UINT *ColorSpec, UINT *ColorTableSize);
LPBITMAPINFO STDCALL ConvertBitmapInfo(LPBITMAPINFO BitmapInfo, UINT ColorSpec, UINT *BitmapInfoSize, BOOL FollowedByData);

/* EOF */
