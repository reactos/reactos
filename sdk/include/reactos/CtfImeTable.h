/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Defining the CTF IME file interface
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

/* The valid CTF IME file contains the following functions: */

/* DEFINE_CTF_IME_FN(func_name, ret_type, params) */
DEFINE_CTF_IME_FN(CtfImeCreateThreadMgr, HRESULT, (VOID))
DEFINE_CTF_IME_FN(CtfImeDestroyThreadMgr, HRESULT, (VOID))
DEFINE_CTF_IME_FN(CtfImeCreateInputContext, HRESULT, (HIMC hIMC))
DEFINE_CTF_IME_FN(CtfImeDestroyInputContext, HRESULT, (HIMC hIMC))
DEFINE_CTF_IME_FN(CtfImeSetActiveContextAlways, HRESULT, (HIMC hIMC, BOOL fActive, HWND hWnd, HKL hKL))
DEFINE_CTF_IME_FN(CtfImeProcessCicHotkey, HRESULT, (HIMC hIMC, UINT vKey, LPARAM lParam))
DEFINE_CTF_IME_FN(CtfImeDispatchDefImeMessage, LRESULT, (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam))
DEFINE_CTF_IME_FN(CtfImeIsIME, BOOL, (HKL hKL))
