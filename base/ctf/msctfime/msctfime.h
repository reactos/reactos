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
#include <immdev.h>
#include <imm32_undoc.h>
#include <cguid.h>
#include <tchar.h>
#include <msctf.h>
#include <msctf_undoc.h>
#include <ctffunc.h>
#include <shlwapi.h>
#include <strsafe.h>

#include <cicbase.h>
#include <cicarray.h>
#include <cicimc.h>
#include <cictf.h>
#include <cicreg.h>
#include <ciccaret.h>
#include <cicuif.h>
#include <cicutb.h>

#include <wine/debug.h>

extern HINSTANCE g_hInst;
extern CRITICAL_SECTION g_csLock;

typedef CicArray<GUID> CDispAttrPropCache;
extern CDispAttrPropCache *g_pPropCache;

HRESULT
Inquire(
    _Out_ LPIMEINFO lpIMEInfo,
    _Out_ LPWSTR lpszWndClass,
    _In_ DWORD dwSystemInfoFlags,
    _In_ HKL hKL);

DEFINE_GUID(GUID_COMPARTMENT_CTFIME_DIMFLAGS,        0xA94C5FD2, 0xC471, 0x4031, 0x95, 0x46, 0x70, 0x9C, 0x17, 0x30, 0x0C, 0xB9);
DEFINE_GUID(GUID_COMPARTMENT_CTFIME_CICINPUTCONTEXT, 0x85A688F7, 0x6DC8, 0x4F17, 0xA8, 0x3A, 0xB1, 0x1C, 0x09, 0xCD, 0xD7, 0xBF);
DEFINE_GUID(GUID_MODEBIAS_FILENAME,                  0xD7F707FE, 0x44C6, 0x4FCA, 0x8E, 0x76, 0x86, 0xAB, 0x50, 0xC7, 0x93, 0x1B);
DEFINE_GUID(GUID_MODEBIAS_NUMERIC,                   0x4021766C, 0xE872, 0x48FD, 0x9C, 0xEE, 0x4E, 0xC5, 0xC7, 0x5E, 0x16, 0xC3);
DEFINE_GUID(GUID_MODEBIAS_URLHISTORY,                0x8B0E54D9, 0x63F2, 0x4C68, 0x84, 0xD4, 0x79, 0xAE, 0xE7, 0xA5, 0x9F, 0x09);
DEFINE_GUID(GUID_MODEBIAS_DEFAULT,                   0xF3DA8BD4, 0x0786, 0x49C2, 0x8C, 0x09, 0x68, 0x39, 0xD8, 0xB8, 0x4F, 0x58);
DEFINE_GUID(GUID_PROP_MODEBIAS,                      0x372E0716, 0x974F, 0x40AC, 0xA0, 0x88, 0x08, 0xCD, 0xC9, 0x2E, 0xBF, 0xBC);
#define GUID_MODEBIAS_NONE GUID_NULL

#include "resource.h"

#include "bridge.h"
#include "inputcontext.h"
#include "misc.h"
#include "profile.h"
#include "sinks.h"
#include "tls.h"
#include "ui.h"
