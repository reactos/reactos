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

#include <windows.h>
#include <imm.h>
#include <ddk/immdev.h>
#include <strsafe.h>

#include <wine/debug.h>

#include "resource.h"

extern HINSTANCE g_hInst;
