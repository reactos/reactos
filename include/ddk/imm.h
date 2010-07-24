/*
 * Copyright (C) 2007 CodeWeavers, Aric Stewart
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef _DDKIMM_H_
#define _DDKIMM_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _tagINPUTCONTEXT {
    HWND                hWnd;
    BOOL                fOpen;
    POINT               ptStatusWndPos;
    POINT               ptSoftKbdPos;
    DWORD               fdwConversion;
    DWORD               fdwSentence;
    union   {
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
} INPUTCONTEXT, *LPINPUTCONTEXT;

typedef struct _tagIMEINFO {
    DWORD       dwPrivateDataSize;
    DWORD       fdwProperty;
    DWORD       fdwConversionCaps;
    DWORD       fdwSentenceCaps;
    DWORD       fdwUICaps;
    DWORD       fdwSCSCaps;
    DWORD       fdwSelectCaps;
} IMEINFO, *LPIMEINFO;

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
} COMPOSITIONSTRING, *LPCOMPOSITIONSTRING;

typedef struct tagGUIDELINE {
    DWORD dwSize;
    DWORD dwLevel;
    DWORD dwIndex;
    DWORD dwStrLen;
    DWORD dwStrOffset;
    DWORD dwPrivateSize;
    DWORD dwPrivateOffset;
} GUIDELINE, *LPGUIDELINE;

typedef struct tagCANDIDATEINFO {
    DWORD               dwSize;
    DWORD               dwCount;
    DWORD               dwOffset[32];
    DWORD               dwPrivateSize;
    DWORD               dwPrivateOffset;
} CANDIDATEINFO, *LPCANDIDATEINFO;

LPINPUTCONTEXT WINAPI ImmLockIMC(HIMC);
BOOL  WINAPI ImmUnlockIMC(HIMC);
DWORD WINAPI ImmGetIMCLockCount(HIMC);
HIMCC  WINAPI ImmCreateIMCC(DWORD);
HIMCC  WINAPI ImmDestroyIMCC(HIMCC);
LPVOID WINAPI ImmLockIMCC(HIMCC);
BOOL   WINAPI ImmUnlockIMCC(HIMCC);
DWORD  WINAPI ImmGetIMCCLockCount(HIMCC);
HIMCC  WINAPI ImmReSizeIMCC(HIMCC, DWORD);
DWORD  WINAPI ImmGetIMCCSize(HIMCC);

#define IMMGWL_IMC                      0
#define IMMGWL_PRIVATE                  (sizeof(LONG))

/* IME Property bits */
#define IME_PROP_END_UNLOAD             0x0001
#define IME_PROP_KBD_CHAR_FIRST         0x0002
#define IME_PROP_IGNORE_UPKEYS          0x0004
#define IME_PROP_NEED_ALTKEY            0x0008
#define IME_PROP_NO_KEYS_ON_CLOSE       0x0010

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

BOOL WINAPI ImmGenerateMessage(HIMC);
LRESULT WINAPI ImmRequestMessageA(HIMC, WPARAM, LPARAM);
LRESULT WINAPI ImmRequestMessageW(HIMC, WPARAM, LPARAM);
#define ImmRequestMessage WINELIB_NAME_AW(ImmRequestMessage);
BOOL WINAPI ImmTranslateMessage(HWND, UINT, WPARAM, LPARAM);
HWND WINAPI ImmCreateSoftKeyboard(UINT, UINT, int, int);
BOOL WINAPI ImmDestroySoftKeyboard(HWND);
BOOL WINAPI ImmShowSoftKeyboard(HWND, int);

BOOL WINAPI ImeInquire(LPIMEINFO, LPWSTR, LPCWSTR lpszOptions);
BOOL WINAPI ImeConfigure (HKL, HWND, DWORD, LPVOID);
DWORD WINAPI ImeConversionList(HIMC, LPCWSTR, LPCANDIDATELIST,DWORD,UINT);
BOOL WINAPI ImeDestroy(UINT);
LRESULT WINAPI ImeEscape(HIMC, UINT, LPVOID);
BOOL WINAPI ImeProcessKey(HIMC, UINT, LPARAM, CONST LPBYTE);
BOOL WINAPI ImeSelect(HIMC, BOOL);
BOOL WINAPI ImeSetActiveContext(HIMC, BOOL);
UINT WINAPI ImeToAsciiEx(UINT, UINT, CONST LPBYTE, LPDWORD, UINT, HIMC);
BOOL WINAPI NotifyIME(HIMC, DWORD, DWORD, DWORD);
BOOL WINAPI ImeRegisterWord(LPCWSTR, DWORD, LPCWSTR);
BOOL WINAPI ImeUnregisterWord(LPCWSTR, DWORD, LPCWSTR);
UINT WINAPI ImeGetRegisterWordStyle(UINT, LPSTYLEBUFW);
UINT WINAPI ImeEnumRegisterWord(REGISTERWORDENUMPROCW, LPCWSTR, DWORD, LPCWSTR, LPVOID);
BOOL WINAPI ImeSetCompositionString(HIMC, DWORD, LPCVOID, DWORD, LPCVOID, DWORD);
DWORD WINAPI ImeGetImeMenuItems(HIMC, DWORD, DWORD, LPIMEMENUITEMINFOW, LPIMEMENUITEMINFOW, DWORD);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* _DDKIMM_H_ */
