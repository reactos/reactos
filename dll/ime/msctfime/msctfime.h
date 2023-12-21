/*
 * PROJECT:     ReactOS msctfime.ime
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Supporting IME interface of Text Input Processors (TIPs)
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include <stdlib.h>

#define WIN32_NO_STATUS
#define COBJMACROS
#define INITGUID

#include <windows.h>
#include <imm.h>
#include <ddk/immdev.h>
#include <cguid.h>
#include <msctf.h>
#include <ctffunc.h>
#include <shlwapi.h>
#include <strsafe.h>

#include <cicero/cicbase.h>
#include <cicero/cicarray.h>
#include <cicero/cicpath.h>
#include <cicero/cicimc.h>

class CicInputContext;

typedef struct tagCTFIMECONTEXT
{
    CicInputContext *m_pCicIC;
    DWORD m_dwCicFlags;
} CTFIMECONTEXT, *PCTFIMECONTEXT;

#include <wine/debug.h>

#include "resource.h"

#define IS_IME_HKL(hKL) ((((ULONG_PTR)(hKL)) & 0xF0000000) == 0xE0000000)

extern HINSTANCE g_hInst;
