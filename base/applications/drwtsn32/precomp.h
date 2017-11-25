/*
 * PROJECT:     Dr. Watson crash reporter
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Precompiled Header
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */

#ifndef _DRWTSN32_PRECOMP_H_
#define _DRWTSN32_PRECOMP_H_

#include <ntstatus.h>
#define WIN32_NO_STATUS

#include <windef.h>
#include <winbase.h>
#include <winver.h>

#include <string>
#include <vector>
#include <map>
#include <stdio.h>
#include <assert.h>

#include "drwtsn32.h"

typedef LONG NTSTATUS;

#endif // _DRWTSN32_PRECOMP_H_
