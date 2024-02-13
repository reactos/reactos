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

#include "resource.h"

extern HINSTANCE g_hInst;
