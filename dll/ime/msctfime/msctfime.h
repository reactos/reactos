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
#include <ctffunc.h>
#include <shlwapi.h>
#include <strsafe.h>

#include <cicbase.h>
#include <cicarray.h>
#include <cicimc.h>
#include <cictf.h>
#include <ciccaret.h>
#include <cicuif.h>
#include <cicutb.h>

#include <wine/debug.h>

EXTERN_C BOOLEAN WINAPI DllShutdownInProgress(VOID);

HRESULT InitDisplayAttrbuteLib(PCIC_LIBTHREAD pLibThread);
HRESULT UninitDisplayAttrbuteLib(PCIC_LIBTHREAD pLibThread);

DEFINE_GUID(GUID_COMPARTMENT_CTFIME_DIMFLAGS,        0xA94C5FD2, 0xC471, 0x4031, 0x95, 0x46, 0x70, 0x9C, 0x17, 0x30, 0x0C, 0xB9);
DEFINE_GUID(GUID_COMPARTMENT_CTFIME_CICINPUTCONTEXT, 0x85A688F7, 0x6DC8, 0x4F17, 0xA8, 0x3A, 0xB1, 0x1C, 0x09, 0xCD, 0xD7, 0xBF);

#include "resource.h"

#include "bridge.h"
#include "compartment.h"
#include "functions.h"
#include "inputcontext.h"
#include "profile.h"
#include "sinks.h"
#include "tls.h"
#include "ui.h"

extern HINSTANCE g_hInst;
