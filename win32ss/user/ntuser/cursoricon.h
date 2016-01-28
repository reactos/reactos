#pragma once

#define MAXCURICONHANDLES 4096

/* Flags that are allowed to be set through NtUserSetCursorIconData() */
#define CURSORF_USER_MASK \
    (CURSORF_FROMRESOURCE | CURSORF_LRSHARED | CURSORF_ACON)

typedef struct _CURICON_OBJECT
{
    PROCMARKHEAD head;
    struct _CURICON_OBJECT* pcurNext;
    UNICODE_STRING strName;
    USHORT atomModName;
    USHORT rt;
    ULONG CURSORF_flags;
    SHORT xHotspot;
    SHORT yHotspot;
    HBITMAP hbmMask;
    HBITMAP hbmColor;
    HBITMAP hbmAlpha;
    RECT rcBounds;
    HBITMAP hbmUserAlpha;
    ULONG bpp;
    ULONG cx;
    ULONG cy;
} CURICON_OBJECT, *PCURICON_OBJECT;

typedef struct tagACON
{
    PROCMARKHEAD head;
    struct _CURICON_OBJECT* pcurNext;
    UNICODE_STRING strName;
    USHORT atomModName;
    USHORT rt;
    ULONG CURSORF_flags;
    UINT cpcur;
    UINT cicur;
    PCURICON_OBJECT * aspcur;
    DWORD * aicur;
    INT * ajifRate;
    UINT iicur;
} ACON, *PACON;

C_ASSERT(FIELD_OFFSET(ACON, cpcur) == FIELD_OFFSET(CURICON_OBJECT, xHotspot));

BOOLEAN
IntDestroyCurIconObject(
    _In_ PVOID Object);

VOID FASTCALL
IntCleanupCurIconCache(PPROCESSINFO Win32Process);

VOID
FreeCurIconObject(
    _In_ PVOID Object);

typedef struct _CURSORACCELERATION_INFO
{
    UINT FirstThreshold;
    UINT SecondThreshold;
    UINT Acceleration;
} CURSORACCELERATION_INFO, *PCURSORACCELERATION_INFO;

typedef struct _SYSTEM_CURSORINFO
{
  BOOL Enabled;
  BOOL ClickLockActive;
  DWORD ClickLockTime;
//  BOOL SwapButtons;
  UINT ButtonsDown;
  RECTL rcClip;
  BOOL bClipped;
  PCURICON_OBJECT CurrentCursorObject;
  INT ShowingCursor;
/*
  UINT WheelScroLines;
  UINT WheelScroChars;
  UINT DblClickSpeed;
  UINT DblClickWidth;
  UINT DblClickHeight;

  UINT MouseHoverTime;
  UINT MouseHoverWidth;
  UINT MouseHoverHeight;

  UINT MouseSpeed;
  CURSORACCELERATION_INFO CursorAccelerationInfo;
*/
  DWORD LastBtnDown;
  LONG LastBtnDownX;
  LONG LastBtnDownY;
  HANDLE LastClkWnd;
  BOOL ScreenSaverRunning;
} SYSTEM_CURSORINFO, *PSYSTEM_CURSORINFO;

typedef struct {
    DWORD type;
    PCURICON_OBJECT handle;
} SYSTEMCURICO;

extern SYSTEMCURICO gasysico[];
extern SYSTEMCURICO gasyscur[];

#define ROIC_SAMPLE 0
#define ROIC_HAND 1
#define ROIC_QUES 2
#define ROIC_BANG 3
#define ROIC_NOTE 4
#define ROIC_WINLOGO 5

#define ROCR_ARROW 0
#define ROCR_IBEAM 1
#define ROCR_WAIT 2
#define ROCR_CROSS 3
#define ROCR_UP 4
#define ROCR_SIZE 5
#define ROCR_ICON 6
#define ROCR_SIZENWSE 7
#define ROCR_SIZENESW 8
#define ROCR_SIZEWE 9
#define ROCR_SIZENS 10
#define ROCR_SIZEALL 11
#define ROCR_NO 12
#define ROCR_HAND 13
#define ROCR_APPSTARTING 14
#define ROCR_HELP 15

#define SYSTEMCUR(func) (gasyscur[ROCR_ ## func].handle)
#define SYSTEMICO(func) (gasysico[ROIC_ ## func].handle)

VOID IntLoadSystenIcons(HICON,DWORD);

BOOL InitCursorImpl(VOID);
HANDLE IntCreateCurIconHandle(BOOLEAN Anim);

BOOL UserDrawIconEx(HDC hDc, INT xLeft, INT yTop, PCURICON_OBJECT pIcon, INT cxWidth,
   INT cyHeight, UINT istepIfAniCur, HBRUSH hbrFlickerFreeDraw, UINT diFlags);
PCURICON_OBJECT FASTCALL UserGetCurIconObject(HCURSOR hCurIcon);
BOOL UserSetCursorPos( INT x, INT y, DWORD flags, ULONG_PTR dwExtraInfo, BOOL Hook);
BOOL APIENTRY UserClipCursor(RECTL *prcl);
PSYSTEM_CURSORINFO IntGetSysCursorInfo(VOID);

#define IntReleaseCurIconObject(CurIconObj) \
  UserDereferenceObject(CurIconObj)

/* EOF */
