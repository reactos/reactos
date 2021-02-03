/*
 * PROJECT:     ReactOS Tasklist Command
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Displays a list of currently running processes on the computer.
 * COPYRIGHT:   Copyright 2020 He Yang (1160386205@qq.com)
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>

#define WIN32_NO_STATUS
#include <windows.h>
#include <ntndk.h>

#include <conutils.h>

#include "resource.h"

#define NT_SYSTEM_QUERY_MAX_RETRY 5

#define COLUMNWIDTH_IMAGENAME   25
#define COLUMNWIDTH_PID         8
#define COLUMNWIDTH_SESSION     11
#define COLUMNWIDTH_MEMUSAGE    12

#define RES_STR_MAXLEN 64

// the strings in opList are the command-line options.
// should always correspond with the defines below, in sequence (except OP_PARAM_INVALID)
static PCWSTR opList[] = {L"?", L"nh"};

#define OP_PARAM_INVALID    -1
#define OP_PARAM_HELP       0
#define OP_PARAM_NOHEADER    1
