#pragma once

#define MAXCURICONHANDLES 4096

typedef struct tagCURICON_PROCESS
{
  LIST_ENTRY ListEntry;
  PPROCESSINFO Process;
} CURICON_PROCESS, *PCURICON_PROCESS;

#ifdef NEW_CURSORICON
typedef struct _CURICON_FRAME
{
    HBITMAP hbmMask;
    HBITMAP hbmColor;
    HBITMAP hbmAlpha;
} CURICON_FRAME, *PCURICON_FRAME;

typedef struct _CURICON_OBJECT
{
  PROCMARKHEAD head;
  LIST_ENTRY ListEntry;
  HANDLE Self;
  LIST_ENTRY ProcessList;
  HMODULE hModule;
  HRSRC hRsrc;
  SIZE Size;
  BYTE Shadow;
  BOOL bIcon;
  POINTL ptlHotspot;
  CURICON_FRAME aFrame[1];
} CURICON_OBJECT, *PCURICON_OBJECT;

#else

typedef struct _CURICON_OBJECT
{
  PROCMARKHEAD head;
  LIST_ENTRY ListEntry;
  HANDLE Self;
  LIST_ENTRY ProcessList;
  HMODULE hModule;
  HRSRC hRsrc;
  HRSRC hGroupRsrc;
  SIZE Size;
  BYTE Shadow;
  ICONINFO IconInfo;
} CURICON_OBJECT, *PCURICON_OBJECT;
#endif

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

BOOL InitCursorImpl(VOID);
PCURICON_OBJECT IntCreateCurIconHandle(VOID);
VOID FASTCALL IntCleanupCurIcons(struct _EPROCESS *Process, PPROCESSINFO Win32Process);

BOOL UserDrawIconEx(HDC hDc, INT xLeft, INT yTop, PCURICON_OBJECT pIcon, INT cxWidth,
   INT cyHeight, UINT istepIfAniCur, HBRUSH hbrFlickerFreeDraw, UINT diFlags);
PCURICON_OBJECT FASTCALL UserGetCurIconObject(HCURSOR hCurIcon);
BOOL UserSetCursorPos( INT x, INT y, DWORD flags, ULONG_PTR dwExtraInfo, BOOL Hook);
BOOL APIENTRY UserClipCursor(RECTL *prcl);
PSYSTEM_CURSORINFO IntGetSysCursorInfo(VOID);

#define IntReleaseCurIconObject(CurIconObj) \
  UserDereferenceObject(CurIconObj)

/* EOF */
