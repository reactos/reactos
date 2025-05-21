/*
 * PROJECT:     ReactOS msutb.dll
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Language Bar (Tipbar)
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include <stdlib.h>

#define COBJMACROS
#define INITGUID

#include <windows.h>
#include <oleacc.h>
#include <imm.h>
#include <undocuser.h>
#include <cguid.h>
#include <msctf.h>
#include <msctf_undoc.h>
#include <ctffunc.h>
#include <ctfutb.h>
#include <shlwapi.h>
#include <atlbase.h>
#include <atlcom.h>
#include <strsafe.h>

#include "resource.h"
#include <cicreg.h>
#include <cicutb.h>
#include <cicuif.h>

#include <wine/debug.h>
