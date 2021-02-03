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

static PCWSTR opList[] = {L"?", L"v"};

#define OP_PARAM_INVALID    -1
#define OP_PARAM_HELP       0
#define OP_PARAM_VERBOSE    1
