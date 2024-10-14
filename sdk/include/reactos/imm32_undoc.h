/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Private header for imm32.dll
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <immdev.h>

#define IME_MASK        (0xE0000000UL)
#define SUBST_MASK      (0xD0000000UL)
#define SPECIAL_MASK    (0xF0000000UL)

#define IS_IME_HKL(hKL)         ((((ULONG_PTR)(hKL)) & 0xF0000000) == IME_MASK)
#define IS_SPECIAL_HKL(hKL)     ((((ULONG_PTR)(hKL)) & 0xF0000000) == SPECIAL_MASK)
#define SPECIALIDFROMHKL(hKL)   ((WORD)(HIWORD(hKL) & 0x0FFF))

#define IS_IME_KLID(dwKLID)     ((((ULONG)(dwKLID)) & 0xF0000000) == IME_MASK)
#define IS_SUBST_KLID(dwKLID)   ((((ULONG)(dwKLID)) & 0xF0000000) == SUBST_MASK)

/* The special values for ImmFreeLayout hKL */
#define HKL_SWITCH_TO_NON_IME   ((HKL)UlongToHandle(1))
#define HKL_RELEASE_IME         ((HKL)UlongToHandle(2))

typedef struct tagIMEINFOEX
{
    HKL hkl;
    IMEINFO ImeInfo;
    WCHAR wszUIClass[16];
    ULONG fdwInitConvMode;
    INT fInitOpen;
    INT fLoadFlag;
    DWORD dwProdVersion;
    DWORD dwImeWinVersion;
    WCHAR wszImeDescription[50];
    WCHAR wszImeFile[80];
    struct
    {
        INT fSysWow64Only:1;
        INT fCUASLayer:1;
    };
} IMEINFOEX, *PIMEINFOEX, NEAR *NPIMEINFOEX, FAR *LPIMEINFOEX;

typedef enum IMEINFOEXCLASS
{
    ImeInfoExKeyboardLayout,
    ImeInfoExKeyboardLayoutTFS,
    ImeInfoExImeWindow,
    ImeInfoExImeFileName
} IMEINFOEXCLASS;

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
} IMEDPI, *PIMEDPI, NEAR *NPIMEDPI, FAR *LPIMEDPI;

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

PIMEDPI WINAPI ImmLockImeDpi(_In_ HKL hKL);
VOID WINAPI ImmUnlockImeDpi(_Inout_ PIMEDPI pImeDpi);

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
} CLIENTIMC, *PCLIENTIMC, NEAR *NPCLIENTIMC, FAR *LPCLIENTIMC;

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

PCLIENTIMC WINAPI ImmLockClientImc(_In_ HIMC hImc);
VOID WINAPI ImmUnlockClientImc(_Inout_ PCLIENTIMC pClientImc);

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
    UINT nVKey;                 // +0x140
    BOOL bNeedsTrans;           // +0x144
    DWORD dwUnknown1;
    DWORD dwUIFlags;            // +0x14c
    DWORD dwUnknown2;
    struct IME_STATE *pState;   // +0x154
    DWORD dwChange;             // +0x158
    HIMCC hCtfImeContext;
} INPUTCONTEXTDX, *PINPUTCONTEXTDX, NEAR *NPINPUTCONTEXTDX, FAR *LPINPUTCONTEXTDX;

typedef struct IME_SUBSTATE
{
    struct IME_SUBSTATE *pNext;
    HKL hKL;
    DWORD dwValue;
} IME_SUBSTATE, *PIME_SUBSTATE, NEAR *NPIME_SUBSTATE, FAR *PIME_SUBSTATE;

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
} IME_STATE, *PIME_STATE, NEAR *NPIME_STATE, FAR *LPIME_STATE;

#ifndef _WIN64
C_ASSERT(sizeof(IME_STATE) == 0x18);
#endif

/* for WM_IME_REPORT IR_UNDETERMINE */
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

UINT WINAPI GetKeyboardLayoutCP(_In_ LANGID wLangId);

BOOL WINAPI
ImmGetImeInfoEx(
    _Out_ PIMEINFOEX pImeInfoEx,
    _In_ IMEINFOEXCLASS SearchType,
    _In_ PVOID pvSearchKey);

BOOL WINAPI ImmLoadLayout(_In_ HKL hKL, _Inout_ PIMEINFOEX pImeInfoEx);
DWORD WINAPI ImmGetAppCompatFlags(_In_ HIMC hIMC);
BOOL WINAPI ImmSetActiveContext(_In_ HWND hwnd, _In_ HIMC hIMC, _In_ BOOL fFlag);
BOOL WINAPI ImmLoadIME(_In_ HKL hKL);
DWORD WINAPI ImmProcessKey(_In_ HWND, _In_ HKL, _In_ UINT, _In_ LPARAM, _In_ DWORD);

HRESULT WINAPI CtfAImmActivate(_Out_opt_ HINSTANCE *phinstCtfIme);
HRESULT WINAPI CtfAImmDeactivate(_In_ BOOL bDestroy);
BOOL WINAPI CtfAImmIsIME(_In_ HKL hKL);
BOOL WINAPI CtfImmIsCiceroStartedInThread(VOID);
VOID WINAPI CtfImmSetCiceroStartInThread(_In_ BOOL bStarted);
VOID WINAPI CtfImmSetAppCompatFlags(_In_ DWORD dwFlags);
DWORD WINAPI CtfImmHideToolbarWnd(VOID);
VOID WINAPI CtfImmRestoreToolbarWnd(_In_ LPVOID pUnused, _In_ DWORD dwShowFlags);
BOOL WINAPI CtfImmGenerateMessage(_In_ HIMC hIMC, _In_ BOOL bSend);
VOID WINAPI CtfImmCoUninitialize(VOID);
VOID WINAPI CtfImmEnterCoInitCountSkipMode(VOID);
BOOL WINAPI CtfImmLeaveCoInitCountSkipMode(VOID);
HRESULT WINAPI CtfImmLastEnabledWndDestroy(_In_ BOOL bCreate);
BOOL WINAPI CtfImmIsCiceroStartedInThread(VOID);
HRESULT WINAPI CtfImmTIMActivate(_In_ HKL hKL);
BOOL WINAPI CtfImmIsTextFrameServiceDisabled(VOID);

LRESULT WINAPI
CtfImmDispatchDefImeMessage(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam);

#ifdef __cplusplus
} // extern "C"
#endif
