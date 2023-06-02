/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Private header for imm32.dll
 * COPYRIGHT:   Copyright 2021-2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#define IS_IME_HKL(hkl) ((((ULONG_PTR)(hkl)) & 0xF0000000) == 0xE0000000)
#define IS_IMM_MODE() (gpsi && (gpsi->dwSRVIFlags & SRVINFO_IMM32))
#define IS_CICERO_MODE() (gpsi && (gpsi->dwSRVIFlags & SRVINFO_CICERO_ENABLED))

typedef struct tagTRANSMSG
{
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
} TRANSMSG, *PTRANSMSG, *LPTRANSMSG;

typedef struct tagTRANSMSGLIST
{
    UINT     uMsgCount;
    TRANSMSG TransMsg[ANYSIZE_ARRAY];
} TRANSMSGLIST, *PTRANSMSGLIST, *LPTRANSMSGLIST;

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
} IMEINFOEX, *PIMEINFOEX;

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
    BOOL bUnknown4;
} CLIENTIMC, *PCLIENTIMC;

#define DEFINE_IME_ENTRY(type, name, params, extended) typedef type (WINAPI *FN_##name) params;
#include <imetable.h>
#undef DEFINE_IME_ENTRY

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

typedef enum IMEINFOEXCLASS
{
    ImeInfoExKeyboardLayout,
    ImeInfoExKeyboardLayoutTFS,
    ImeInfoExImeWindow,
    ImeInfoExImeFileName
} IMEINFOEXCLASS;

#ifdef __cplusplus
extern "C" {
#endif

BOOL WINAPI
ImmGetImeInfoEx(PIMEINFOEX pImeInfoEx, IMEINFOEXCLASS SearchType, PVOID pvSearchKey);

PCLIENTIMC WINAPI ImmLockClientImc(HIMC hImc);
VOID WINAPI ImmUnlockClientImc(PCLIENTIMC pClientImc);
PIMEDPI WINAPI ImmLockImeDpi(HKL hKL);
VOID WINAPI ImmUnlockImeDpi(PIMEDPI pImeDpi);
HRESULT APIENTRY CtfImmTIMCreateInputContext(HIMC hIMC);
HRESULT APIENTRY CtfImmTIMDestroyInputContext(HIMC hIMC);
HRESULT WINAPI CtfImmTIMActivate(HKL hKL);
BOOL WINAPI ImmSetActiveContextConsoleIME(HWND hwnd, BOOL fFlag);
BOOL WINAPI ImmDisableTextFrameService(DWORD dwThreadId);

DWORD WINAPI
ImmCallImeConsoleIME(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _Out_ LPUINT puVK);

#ifdef __cplusplus
} // extern "C"
#endif
