/*
 * PROJECT:     ReactOS Console IME
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Console IME header
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

typedef struct tagIME_STATUS
{
    DWORD cch;
    DWORD unknown0;
    CHAR_INFO CharInfo[160];
} IME_STATUS, *PIME_STATUS;

#ifndef _WIN64
C_ASSERT(sizeof(IME_STATUS) == 0x288);
#endif

/* Console IME messages */
#define CONIMEM_INIT             (WM_USER + 0) /* == 0x400 */
#define CONIMEM_UNINIT           (WM_USER + 1)
#define CONIMEM_SET_FOCUS        (WM_USER + 2)
#define CONIMEM_KILL_FOCUS       (WM_USER + 3)
#define CONIMEM_SIMULATE_KEY     (WM_USER + 4)
#define CONIMEM_GET_IME_STATE    (WM_USER + 5)
#define CONIMEM_SET_IME_STATE    (WM_USER + 6)
#define CONIMEM_SET_SCREEN_SIZE  (WM_USER + 7)
#define CONIMEM_SEND_IME_STATUS  (WM_USER + 8)
#define CONIMEM_LANGUAGE_CHANGE  (WM_USER + 9)
#define CONIMEM_SET_CODEPAGE     (WM_USER + 10)
#define CONIMEM_GO               (WM_USER + 11)
#define CONIMEM_GO_NEXT          (WM_USER + 12)
#define CONIMEM_GO_PREV          (WM_USER + 13)

/* WM_COPYDATA COPYDATASTRUCT.dwData */
#define CONIME_COPYDATA_SEND_COMPSTR 0x4B425930
#define CONIME_COPYDATA_SEND_IME_STATUS 0x4B425931
#define CONIME_COPYDATA_SEND_GUIDELINE 0x4B425932
#define CONIME_COPYDATA_SEND_CLOSE_CAND 0x4B425935
#define CONIME_COPYDATA_SEND_IME_SYSTEM 0x4B425936

/* Flags for IntGetImeState/IntSetImeState */
#define IME_STATE_OPENED 0x20000000
#define IME_STATE_DISABLED 0x40000000

typedef struct tagCONIME_COMPSTR
{
    DWORD cbSize;
    DWORD Unknown1;
} CONIME_COMPSTR, *PCONIME_COMPSTR;
