/*
 * PROJECT:     ReactOS headers
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Providing DDK-compatible <immdev.h> and IME/IMM development helper
 * COPYRIGHT:   Copyright 2021-2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#ifndef _IMMDEV_
#define _IMMDEV_

#pragma once

#include <wingdi.h>
#include <imm.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _tagIMEINFO {
    DWORD dwPrivateDataSize;
    DWORD fdwProperty;
    DWORD fdwConversionCaps;
    DWORD fdwSentenceCaps;
    DWORD fdwUICaps;
    DWORD fdwSCSCaps;
    DWORD fdwSelectCaps;
} IMEINFO, *PIMEINFO, NEAR *NPIMEINFO, FAR *LPIMEINFO;

typedef struct tagCANDIDATEINFO {
    DWORD dwSize;
    DWORD dwCount;
    DWORD dwOffset[32];
    DWORD dwPrivateSize;
    DWORD dwPrivateOffset;
} CANDIDATEINFO, *PCANDIDATEINFO, NEAR *NPCANDIDATEINFO, FAR *LPCANDIDATEINFO;

#if (WINVER >= 0x040A)
BOOL WINAPI ImmDisableTextFrameService(_In_ DWORD dwThreadId);
#endif

typedef struct tagSOFTKBDDATA {
    UINT uCount;
    WORD wCode[ANYSIZE_ARRAY][256];
} SOFTKBDDATA, *PSOFTKBDDATA, NEAR *NPSOFTKBDDATA, FAR *LPSOFTKBDDATA;

typedef struct tagCOMPOSITIONSTRING {
    DWORD dwSize;
    DWORD dwCompReadAttrLen;
    DWORD dwCompReadAttrOffset;
    DWORD dwCompReadClauseLen;
    DWORD dwCompReadClauseOffset;
    DWORD dwCompReadStrLen;
    DWORD dwCompReadStrOffset;
    DWORD dwCompAttrLen;
    DWORD dwCompAttrOffset;
    DWORD dwCompClauseLen;
    DWORD dwCompClauseOffset;
    DWORD dwCompStrLen;
    DWORD dwCompStrOffset;
    DWORD dwCursorPos;
    DWORD dwDeltaStart;
    DWORD dwResultReadClauseLen;
    DWORD dwResultReadClauseOffset;
    DWORD dwResultReadStrLen;
    DWORD dwResultReadStrOffset;
    DWORD dwResultClauseLen;
    DWORD dwResultClauseOffset;
    DWORD dwResultStrLen;
    DWORD dwResultStrOffset;
    DWORD dwPrivateSize;
    DWORD dwPrivateOffset;
} COMPOSITIONSTRING, *PCOMPOSITIONSTRING, NEAR *NPCOMPOSITIONSTRING, FAR *LPCOMPOSITIONSTRING;

typedef struct tagGUIDELINE {
    DWORD dwSize;
    DWORD dwLevel;
    DWORD dwIndex;
    DWORD dwStrLen;
    DWORD dwStrOffset;
    DWORD dwPrivateSize;
    DWORD dwPrivateOffset;
} GUIDELINE, *PGUIDELINE, NEAR *NPGUIDELINE, FAR *LPGUIDELINE;

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
} INPUTCONTEXT, *PINPUTCONTEXT, NEAR *NPINPUTCONTEXT, FAR *LPINPUTCONTEXT;

#ifndef _WIN64
C_ASSERT(offsetof(INPUTCONTEXT, hWnd) == 0x0);
C_ASSERT(offsetof(INPUTCONTEXT, fOpen) == 0x4);
C_ASSERT(offsetof(INPUTCONTEXT, ptStatusWndPos) == 0x8);
C_ASSERT(offsetof(INPUTCONTEXT, ptSoftKbdPos) == 0x10);
C_ASSERT(offsetof(INPUTCONTEXT, fdwConversion) == 0x18);
C_ASSERT(offsetof(INPUTCONTEXT, fdwSentence) == 0x1C);
C_ASSERT(offsetof(INPUTCONTEXT, lfFont) == 0x20);
C_ASSERT(offsetof(INPUTCONTEXT, cfCompForm) == 0x7C);
C_ASSERT(offsetof(INPUTCONTEXT, cfCandForm) == 0x98);
C_ASSERT(offsetof(INPUTCONTEXT, hCompStr) == 0x118);
C_ASSERT(offsetof(INPUTCONTEXT, hCandInfo) == 0x11C);
C_ASSERT(offsetof(INPUTCONTEXT, hGuideLine) == 0x120);
C_ASSERT(offsetof(INPUTCONTEXT, hPrivate) == 0x124);
C_ASSERT(offsetof(INPUTCONTEXT, dwNumMsgBuf) == 0x128);
C_ASSERT(offsetof(INPUTCONTEXT, hMsgBuf) == 0x12C);
C_ASSERT(offsetof(INPUTCONTEXT, fdwInit) == 0x130);
C_ASSERT(offsetof(INPUTCONTEXT, dwReserve) == 0x134);
C_ASSERT(sizeof(INPUTCONTEXT) == 0x140);
#endif

/* bits of fdwInit of INPUTCONTEXT */
#define INIT_STATUSWNDPOS               0x00000001
#define INIT_CONVERSION                 0x00000002
#define INIT_SENTENCE                   0x00000004
#define INIT_LOGFONT                    0x00000008
#define INIT_COMPFORM                   0x00000010
#define INIT_SOFTKBDPOS                 0x00000020
#define INIT_GUIDMAP                    0x00000040

/* bits for INPUTCONTEXTDX.dwChange */
#define INPUTCONTEXTDX_CHANGE_OPEN          0x1
#define INPUTCONTEXTDX_CHANGE_CONVERSION    0x2
#define INPUTCONTEXTDX_CHANGE_SENTENCE      0x4
#define INPUTCONTEXTDX_CHANGE_FORCE_OPEN    0x100

#ifndef WM_IME_REPORT
    #define WM_IME_REPORT 0x280
#endif

/* WM_IME_REPORT wParam */
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

/* IMC */

LPINPUTCONTEXT WINAPI ImmLockIMC(_In_ HIMC hIMC);
BOOL  WINAPI ImmUnlockIMC(_In_ HIMC hIMC);
DWORD WINAPI ImmGetIMCLockCount(_In_ HIMC hIMC);

/* IMCC */

HIMCC  WINAPI ImmCreateIMCC(_In_ DWORD size);
HIMCC  WINAPI ImmDestroyIMCC(_In_ HIMCC block);
LPVOID WINAPI ImmLockIMCC(_In_ HIMCC imcc);
BOOL   WINAPI ImmUnlockIMCC(_In_ HIMCC imcc);
DWORD  WINAPI ImmGetIMCCLockCount(_In_ HIMCC imcc);
HIMCC  WINAPI ImmReSizeIMCC(_In_ HIMCC imcc, _In_ DWORD size);
DWORD  WINAPI ImmGetIMCCSize(_In_ HIMCC imcc);

/* Messaging */

BOOL WINAPI ImmGenerateMessage(_In_ HIMC hIMC);

BOOL WINAPI
ImmTranslateMessage(
    _In_ HWND hwnd,
    _In_ UINT msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lKeyData);

LRESULT WINAPI ImmRequestMessageA(_In_ HIMC hIMC, _In_ WPARAM wParam, _In_ LPARAM lParam);
LRESULT WINAPI ImmRequestMessageW(_In_ HIMC hIMC, _In_ WPARAM wParam, _In_ LPARAM lParam);

#ifdef UNICODE
    #define ImmRequestMessage ImmRequestMessageW
#else
    #define ImmRequestMessage ImmRequestMessageA
#endif

typedef struct _tagTRANSMSG {
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
} TRANSMSG, *PTRANSMSG, NEAR *NPTRANSMSG, FAR *LPTRANSMSG;

typedef struct _tagTRANSMSGLIST {
    UINT     uMsgCount;
    TRANSMSG TransMsg[ANYSIZE_ARRAY];
} TRANSMSGLIST, *PTRANSMSGLIST, NEAR *NPTRANSMSGLIST, FAR *LPTRANSMSGLIST;

/* Soft keyboard */

HWND WINAPI
ImmCreateSoftKeyboard(
    _In_ UINT uType,
    _In_ HWND hwndParent,
    _In_ INT x,
    _In_ INT y);

BOOL WINAPI
ImmShowSoftKeyboard(
    _In_ HWND hwndSoftKBD,
    _In_ INT nCmdShow);

BOOL WINAPI
ImmDestroySoftKeyboard(
    _In_ HWND hwndSoftKBD);

/* IME file interface */

BOOL WINAPI
ImeInquire(
    _Out_ LPIMEINFO lpIMEInfo,
    _Out_ LPTSTR lpszWndClass,
    _In_ DWORD dwSystemInfoFlags);

DWORD WINAPI
ImeConversionList(
    _In_ HIMC hIMC,
    _In_ LPCTSTR lpSrc,
    _Out_ LPCANDIDATELIST lpDst,
    _In_ DWORD dwBufLen,
    _In_ UINT uFlag);

BOOL WINAPI
ImeRegisterWord(
    _In_ LPCTSTR lpszReading,
    _In_ DWORD dwStyle,
    _In_ LPCTSTR lpszString);

BOOL WINAPI
ImeUnregisterWord(
    _In_ LPCTSTR lpszReading,
    _In_ DWORD dwStyle,
    _In_ LPCTSTR lpszString);

UINT WINAPI
ImeGetRegisterWordStyle(
    _In_ UINT nItem,
    _Out_ LPSTYLEBUF lpStyleBuf);

UINT WINAPI
ImeEnumRegisterWord(
    _In_ REGISTERWORDENUMPROC lpfnEnumProc,
    _In_opt_ LPCTSTR lpszReading,
    _In_ DWORD dwStyle,
    _In_opt_ LPCTSTR lpszString,
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
    _Inout_opt_ LPIMEMENUITEMINFO lpImeParentMenu,
    _Inout_opt_ LPIMEMENUITEMINFO lpImeMenu,
    _In_ DWORD dwSize);

#ifdef __cplusplus
} /* extern "C" */
#endif

/* IME Property bits */
#define IME_PROP_END_UNLOAD             0x0001
#define IME_PROP_KBD_CHAR_FIRST         0x0002
#define IME_PROP_IGNORE_UPKEYS          0x0004
#define IME_PROP_NEED_ALTKEY            0x0008
#define IME_PROP_NO_KEYS_ON_CLOSE       0x0010
#define IME_PROP_ACCEPT_WIDE_VKEY       0x0020

/* for NI_CONTEXTUPDATED */
#define IMC_SETCONVERSIONMODE           0x0002
#define IMC_SETSENTENCEMODE             0x0004
#define IMC_SETOPENSTATUS               0x0006

/* dwAction for ImmNotifyIME */
#define NI_CONTEXTUPDATED               0x0003
#define NI_OPENCANDIDATE                0x0010
#define NI_CLOSECANDIDATE               0x0011
#define NI_SELECTCANDIDATESTR           0x0012
#define NI_CHANGECANDIDATELIST          0x0013
#define NI_FINALIZECONVERSIONRESULT     0x0014
#define NI_COMPOSITIONSTR               0x0015
#define NI_SETCANDIDATE_PAGESTART       0x0016
#define NI_SETCANDIDATE_PAGESIZE        0x0017
#define NI_IMEMENUSELECTED              0x0018

/* dwSystemInfoFlags bits */
#define IME_SYSINFO_WINLOGON            0x0001
#define IME_SYSINFO_WOW16               0x0002

#endif  /* ndef _IMMDEV_ */
