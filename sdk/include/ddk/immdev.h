/*
 * PROJECT:     ReactOS headers
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Providing DDK-compatible <immdev.h> and IME/IMM development helper
 * COPYRIGHT:   Copyright 2021-2022 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#ifndef _IMMDEV_
#define _IMMDEV_

#pragma once

#include <wingdi.h>
#include <imm.h>

#ifdef __cplusplus
extern "C" {
#endif

BOOL WINAPI ImmDisableTextFrameService(_In_ DWORD dwThreadId);

typedef struct tagSOFTKBDDATA
{
    UINT uCount;
    WORD wCode[1][256];
} SOFTKBDDATA, *PSOFTKBDDATA, *LPSOFTKBDDATA;

/* wParam for WM_IME_CONTROL */
#define IMC_GETCONVERSIONMODE           0x0001
#define IMC_GETSENTENCEMODE             0x0003
#define IMC_GETOPENSTATUS               0x0005
#define IMC_GETSOFTKBDFONT              0x0011
#define IMC_SETSOFTKBDFONT              0x0012
#define IMC_GETSOFTKBDPOS               0x0013
#define IMC_SETSOFTKBDPOS               0x0014
#define IMC_GETSOFTKBDSUBTYPE           0x0015
#define IMC_SETSOFTKBDSUBTYPE           0x0016
#define IMC_SETSOFTKBDDATA              0x0018

/* wParam for WM_IME_SYSTEM */
#define IMS_NOTIFYIMESHOW       0x05
#define IMS_UPDATEIMEUI         0x06
#define IMS_SETCANDFORM         0x09
#define IMS_SETCOMPFONT         0x0A
#define IMS_SETCOMPFORM         0x0B
#define IMS_CONFIGURE           0x0D
#define IMS_SETOPENSTATUS       0x0F
#define IMS_FREELAYOUT          0x11
#define IMS_GETCONVSTATUS       0x14
#define IMS_IMEHELP             0x15
#define IMS_IMEACTIVATE         0x17
#define IMS_IMEDEACTIVATE       0x18
#define IMS_ACTIVATELAYOUT      0x19
#define IMS_GETIMEMENU          0x1C
#define IMS_GETCONTEXT          0x1E
#define IMS_SENDNOTIFICATION    0x1F
#define IMS_COMPLETECOMPSTR     0x20
#define IMS_LOADTHREADLAYOUT    0x21
#define IMS_SETLANGBAND         0x23
#define IMS_UNSETLANGBAND       0x24

/* wParam for WM_IME_NOTIFY */
#define IMN_SOFTKBDDESTROYED    0x0011

#define IMMGWL_IMC       0
#define IMMGWL_PRIVATE   (sizeof(LONG))

#define IMMGWLP_IMC      0
#define IMMGWLP_PRIVATE  (sizeof(LONG_PTR))

typedef struct _tagINPUTCONTEXT {
    HWND                hWnd;
    BOOL                fOpen;
    POINT               ptStatusWndPos;
    POINT               ptSoftKbdPos;
    DWORD               fdwConversion;
    DWORD               fdwSentence;
    union {
        LOGFONTA        A;
        LOGFONTW        W;
    } lfFont;
    COMPOSITIONFORM     cfCompForm;
    CANDIDATEFORM       cfCandForm[4];
    HIMCC               hCompStr;
    HIMCC               hCandInfo;
    HIMCC               hGuideLine;
    HIMCC               hPrivate;
    DWORD               dwNumMsgBuf;
    HIMCC               hMsgBuf;
    DWORD               fdwInit;
    DWORD               dwReserve[3];
} INPUTCONTEXT, *PINPUTCONTEXT, *LPINPUTCONTEXT;

#ifdef _WIN64
C_ASSERT(offsetof(INPUTCONTEXT, hWnd) == 0x0);
C_ASSERT(offsetof(INPUTCONTEXT, fOpen) == 0x8);
C_ASSERT(offsetof(INPUTCONTEXT, ptStatusWndPos) == 0xc);
C_ASSERT(offsetof(INPUTCONTEXT, ptSoftKbdPos) == 0x14);
C_ASSERT(offsetof(INPUTCONTEXT, fdwConversion) == 0x1c);
C_ASSERT(offsetof(INPUTCONTEXT, fdwSentence) == 0x20);
C_ASSERT(offsetof(INPUTCONTEXT, lfFont) == 0x24);
C_ASSERT(offsetof(INPUTCONTEXT, cfCompForm) == 0x80);
C_ASSERT(offsetof(INPUTCONTEXT, cfCandForm) == 0x9c);
C_ASSERT(offsetof(INPUTCONTEXT, hCompStr) == 0x120);
C_ASSERT(offsetof(INPUTCONTEXT, hCandInfo) == 0x128);
C_ASSERT(offsetof(INPUTCONTEXT, hGuideLine) == 0x130);
C_ASSERT(offsetof(INPUTCONTEXT, hPrivate) == 0x138);
C_ASSERT(offsetof(INPUTCONTEXT, dwNumMsgBuf) == 0x140);
C_ASSERT(offsetof(INPUTCONTEXT, hMsgBuf) == 0x148);
C_ASSERT(offsetof(INPUTCONTEXT, fdwInit) == 0x150);
C_ASSERT(offsetof(INPUTCONTEXT, dwReserve) == 0x154);
C_ASSERT(sizeof(INPUTCONTEXT) == 0x160);
#else
C_ASSERT(offsetof(INPUTCONTEXT, hWnd) == 0x0);
C_ASSERT(offsetof(INPUTCONTEXT, fOpen) == 0x4);
C_ASSERT(offsetof(INPUTCONTEXT, ptStatusWndPos) == 0x8);
C_ASSERT(offsetof(INPUTCONTEXT, ptSoftKbdPos) == 0x10);
C_ASSERT(offsetof(INPUTCONTEXT, fdwConversion) == 0x18);
C_ASSERT(offsetof(INPUTCONTEXT, fdwSentence) == 0x1c);
C_ASSERT(offsetof(INPUTCONTEXT, lfFont) == 0x20);
C_ASSERT(offsetof(INPUTCONTEXT, cfCompForm) == 0x7c);
C_ASSERT(offsetof(INPUTCONTEXT, cfCandForm) == 0x98);
C_ASSERT(offsetof(INPUTCONTEXT, hCompStr) == 0x118);
C_ASSERT(offsetof(INPUTCONTEXT, hCandInfo) == 0x11c);
C_ASSERT(offsetof(INPUTCONTEXT, hGuideLine) == 0x120);
C_ASSERT(offsetof(INPUTCONTEXT, hPrivate) == 0x124);
C_ASSERT(offsetof(INPUTCONTEXT, dwNumMsgBuf) == 0x128);
C_ASSERT(offsetof(INPUTCONTEXT, hMsgBuf) == 0x12c);
C_ASSERT(offsetof(INPUTCONTEXT, fdwInit) == 0x130);
C_ASSERT(offsetof(INPUTCONTEXT, dwReserve) == 0x134);
C_ASSERT(sizeof(INPUTCONTEXT) == 0x140);
#endif

struct IME_STATE;

/* unconfirmed */
#ifdef __cplusplus
typedef struct INPUTCONTEXTDX : INPUTCONTEXT
{
#else
typedef struct INPUTCONTEXTDX
{
    INPUTCONTEXT;
#endif
    UINT nVKey;
    BOOL bNeedsTrans;
    DWORD dwUnknown1;
    DWORD dwUIFlags;
    DWORD dwUnknown2;
    struct IME_STATE *pState;
    DWORD dwChange;
    DWORD dwUnknown5;
} INPUTCONTEXTDX, *PINPUTCONTEXTDX, *LPINPUTCONTEXTDX;

#ifndef _WIN64
C_ASSERT(offsetof(INPUTCONTEXTDX, nVKey) == 0x140);
C_ASSERT(offsetof(INPUTCONTEXTDX, bNeedsTrans) == 0x144);
C_ASSERT(offsetof(INPUTCONTEXTDX, dwUIFlags) == 0x14c);
C_ASSERT(offsetof(INPUTCONTEXTDX, pState) == 0x154);
C_ASSERT(offsetof(INPUTCONTEXTDX, dwChange) == 0x158);
C_ASSERT(sizeof(INPUTCONTEXTDX) == 0x160);
#endif

// bits of fdwInit of INPUTCONTEXT
#define INIT_STATUSWNDPOS               0x00000001
#define INIT_CONVERSION                 0x00000002
#define INIT_SENTENCE                   0x00000004
#define INIT_LOGFONT                    0x00000008
#define INIT_COMPFORM                   0x00000010
#define INIT_SOFTKBDPOS                 0x00000020

// bits for INPUTCONTEXTDX.dwChange
#define INPUTCONTEXTDX_CHANGE_OPEN          0x1
#define INPUTCONTEXTDX_CHANGE_CONVERSION    0x2
#define INPUTCONTEXTDX_CHANGE_SENTENCE      0x4
#define INPUTCONTEXTDX_CHANGE_FORCE_OPEN    0x100

#ifndef WM_IME_REPORT
    #define WM_IME_REPORT 0x280
#endif

// WM_IME_REPORT wParam
#define IR_STRINGSTART   0x100
#define IR_STRINGEND     0x101
#define IR_OPENCONVERT   0x120
#define IR_CHANGECONVERT 0x121
#define IR_CLOSECONVERT  0x122
#define IR_FULLCONVERT   0x123
#define IR_IMESELECT     0x130
#define IR_STRING        0x140
#define IR_DBCSCHAR      0x160
#define IR_UNDETERMINE   0x170
#define IR_STRINGEX      0x180
#define IR_MODEINFO      0x190

// for IR_UNDETERMINE
typedef struct tagUNDETERMINESTRUCT
{
    DWORD dwSize;
    UINT  uDefIMESize;
    UINT  uDefIMEPos;
    UINT  uUndetTextLen;
    UINT  uUndetTextPos;
    UINT  uUndetAttrPos;
    UINT  uCursorPos;
    UINT  uDeltaStart;
    UINT  uDetermineTextLen;
    UINT  uDetermineTextPos;
    UINT  uDetermineDelimPos;
    UINT  uYomiTextLen;
    UINT  uYomiTextPos;
    UINT  uYomiDelimPos;
} UNDETERMINESTRUCT, *PUNDETERMINESTRUCT, *LPUNDETERMINESTRUCT;

LPINPUTCONTEXT WINAPI ImmLockIMC(HIMC);

typedef struct IME_SUBSTATE
{
    struct IME_SUBSTATE *pNext;
    HKL hKL;
    DWORD dwValue;
} IME_SUBSTATE, *PIME_SUBSTATE;

#ifndef _WIN64
C_ASSERT(sizeof(IME_SUBSTATE) == 0xc);
#endif

typedef struct IME_STATE
{
    struct IME_STATE *pNext;
    WORD wLang;
    WORD fOpen;
    DWORD dwConversion;
    DWORD dwSentence;
    DWORD dwInit;
    PIME_SUBSTATE pSubState;
} IME_STATE, *PIME_STATE;

#ifndef _WIN64
C_ASSERT(sizeof(IME_STATE) == 0x18);
#endif

typedef struct _tagTRANSMSG
{
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
} TRANSMSG, *PTRANSMSG, *LPTRANSMSG;

typedef struct _tagTRANSMSGLIST
{
    UINT     uMsgCount;
    TRANSMSG TransMsg[ANYSIZE_ARRAY];
} TRANSMSGLIST, *PTRANSMSGLIST, *LPTRANSMSGLIST;

#define DEFINE_IME_ENTRY(type, name, params, extended) typedef type (WINAPI *FN_##name) params;
#include <imetable.h>
#undef DEFINE_IME_ENTRY

typedef struct IMEDPI
{
    struct IMEDPI *pNext;
    HINSTANCE      hInst;
    HKL            hKL;
    IMEINFO        ImeInfo;
    UINT           uCodePage;
    WCHAR          szUIClass[16];
    DWORD          cLockObj;
    DWORD          dwFlags;
#define DEFINE_IME_ENTRY(type, name, params, extended) FN_##name name;
#include <imetable.h>
#undef DEFINE_IME_ENTRY
} IMEDPI, *PIMEDPI;

#ifndef _WIN64
C_ASSERT(offsetof(IMEDPI, pNext) == 0x0);
C_ASSERT(offsetof(IMEDPI, hInst) == 0x4);
C_ASSERT(offsetof(IMEDPI, hKL) == 0x8);
C_ASSERT(offsetof(IMEDPI, ImeInfo) == 0xc);
C_ASSERT(offsetof(IMEDPI, uCodePage) == 0x28);
C_ASSERT(offsetof(IMEDPI, szUIClass) == 0x2c);
C_ASSERT(offsetof(IMEDPI, cLockObj) == 0x4c);
C_ASSERT(offsetof(IMEDPI, dwFlags) == 0x50);
C_ASSERT(offsetof(IMEDPI, ImeInquire) == 0x54);
C_ASSERT(offsetof(IMEDPI, ImeConversionList) == 0x58);
C_ASSERT(offsetof(IMEDPI, ImeRegisterWord) == 0x5c);
C_ASSERT(offsetof(IMEDPI, ImeUnregisterWord) == 0x60);
C_ASSERT(offsetof(IMEDPI, ImeGetRegisterWordStyle) == 0x64);
C_ASSERT(offsetof(IMEDPI, ImeEnumRegisterWord) == 0x68);
C_ASSERT(offsetof(IMEDPI, ImeConfigure) == 0x6c);
C_ASSERT(offsetof(IMEDPI, ImeDestroy) == 0x70);
C_ASSERT(offsetof(IMEDPI, ImeEscape) == 0x74);
C_ASSERT(offsetof(IMEDPI, ImeProcessKey) == 0x78);
C_ASSERT(offsetof(IMEDPI, ImeSelect) == 0x7c);
C_ASSERT(offsetof(IMEDPI, ImeSetActiveContext) == 0x80);
C_ASSERT(offsetof(IMEDPI, ImeToAsciiEx) == 0x84);
C_ASSERT(offsetof(IMEDPI, NotifyIME) == 0x88);
C_ASSERT(offsetof(IMEDPI, ImeSetCompositionString) == 0x8c);
C_ASSERT(offsetof(IMEDPI, ImeGetImeMenuItems) == 0x90);
C_ASSERT(offsetof(IMEDPI, CtfImeInquireExW) == 0x94);
C_ASSERT(offsetof(IMEDPI, CtfImeSelectEx) == 0x98);
C_ASSERT(offsetof(IMEDPI, CtfImeEscapeEx) == 0x9c);
C_ASSERT(offsetof(IMEDPI, CtfImeGetGuidAtom) == 0xa0);
C_ASSERT(offsetof(IMEDPI, CtfImeIsGuidMapEnable) == 0xa4);
C_ASSERT(sizeof(IMEDPI) == 0xa8);
#endif

/* flags for IMEDPI.dwFlags */
#define IMEDPI_FLAG_UNLOADED 0x1
#define IMEDPI_FLAG_LOCKED 0x2

/* unconfirmed */
typedef struct tagCLIENTIMC
{
    HANDLE hInputContext;   /* LocalAlloc'ed LHND */
    LONG cLockObj;
    DWORD dwFlags;
    DWORD dwCompatFlags;
    RTL_CRITICAL_SECTION cs;
    UINT uCodePage;
    HKL hKL;
    BOOL bCtfIme;
} CLIENTIMC, *PCLIENTIMC;

#ifndef _WIN64
C_ASSERT(offsetof(CLIENTIMC, hInputContext) == 0x0);
C_ASSERT(offsetof(CLIENTIMC, cLockObj) == 0x4);
C_ASSERT(offsetof(CLIENTIMC, dwFlags) == 0x8);
C_ASSERT(offsetof(CLIENTIMC, dwCompatFlags) == 0xc);
C_ASSERT(offsetof(CLIENTIMC, cs) == 0x10);
C_ASSERT(offsetof(CLIENTIMC, uCodePage) == 0x28);
C_ASSERT(offsetof(CLIENTIMC, hKL) == 0x2c);
C_ASSERT(sizeof(CLIENTIMC) == 0x34);
#endif

/* flags for CLIENTIMC */
#define CLIENTIMC_WIDE 0x1
#define CLIENTIMC_ACTIVE 0x2
#define CLIENTIMC_UNKNOWN4 0x20
#define CLIENTIMC_DESTROY 0x40
#define CLIENTIMC_DISABLEIME 0x80
#define CLIENTIMC_UNKNOWN2 0x100

/* IME file interface */

BOOL WINAPI
ImeInquire(
    _Out_ LPIMEINFO lpIMEInfo,
    _Out_ LPWSTR lpszWndClass,
    _In_ DWORD dwSystemInfoFlags);

DWORD WINAPI
ImeConversionList(
    _In_ HIMC hIMC,
    _In_ LPCWSTR lpSrc,
    _Out_ LPCANDIDATELIST lpDst,
    _In_ DWORD dwBufLen,
    _In_ UINT uFlag);

BOOL WINAPI
ImeRegisterWord(
    _In_ LPCWSTR lpszReading,
    _In_ DWORD dwStyle,
    _In_ LPCWSTR lpszString);

BOOL WINAPI
ImeUnregisterWord(
    _In_ LPCWSTR lpszReading,
    _In_ DWORD dwStyle,
    _In_ LPCWSTR lpszString);

UINT WINAPI
ImeGetRegisterWordStyle(
    _In_ UINT nItem,
    _Out_ LPSTYLEBUFW lpStyleBuf);

UINT WINAPI
ImeEnumRegisterWord(
    _In_ REGISTERWORDENUMPROCW lpfnEnumProc,
    _In_opt_ LPCWSTR lpszReading,
    _In_ DWORD dwStyle,
    _In_opt_ LPCWSTR lpszString,
    _In_opt_ LPVOID lpData);

BOOL WINAPI
ImeConfigure(
    _In_ HKL hKL,
    _In_ HWND hWnd,
    _In_ DWORD dwMode,
    _Inout_opt_ LPVOID lpData);

BOOL WINAPI
ImeDestroy(
    _In_ UINT uReserved);

LRESULT WINAPI
ImeEscape(
    _In_ HIMC hIMC,
    _In_ UINT uEscape,
    _Inout_opt_ LPVOID lpData);

BOOL WINAPI
ImeProcessKey(
    _In_ HIMC hIMC,
    _In_ UINT uVirKey,
    _In_ LPARAM lParam,
    _In_ CONST LPBYTE lpbKeyState);

BOOL WINAPI
ImeSelect(
    _In_ HIMC hIMC,
    _In_ BOOL fSelect);

BOOL WINAPI
ImeSetActiveContext(
    _In_ HIMC hIMC,
    _In_ BOOL fFlag);

UINT WINAPI
ImeToAsciiEx(
    _In_ UINT uVirKey,
    _In_ UINT uScanCode,
    _In_ CONST LPBYTE lpbKeyState,
    _Out_ LPTRANSMSGLIST lpTransMsgList,
    _In_ UINT fuState,
    _In_ HIMC hIMC);

BOOL WINAPI
NotifyIME(
    _In_ HIMC hIMC,
    _In_ DWORD dwAction,
    _In_ DWORD dwIndex,
    _In_ DWORD_PTR dwValue);

BOOL WINAPI
ImeSetCompositionString(
    _In_ HIMC hIMC,
    _In_ DWORD dwIndex,
    _In_opt_ LPCVOID lpComp,
    _In_ DWORD dwCompLen,
    _In_opt_ LPCVOID lpRead,
    _In_ DWORD dwReadLen);

DWORD WINAPI
ImeGetImeMenuItems(
    _In_ HIMC hIMC,
    _In_ DWORD dwFlags,
    _In_ DWORD dwType,
    _Inout_opt_ LPIMEMENUITEMINFOW lpImeParentMenu,
    _Inout_opt_ LPIMEMENUITEMINFOW lpImeMenu,
    _In_ DWORD dwSize);

#ifdef __cplusplus
} // extern "C"
#endif

#endif  /* ndef _IMMDEV_ */
