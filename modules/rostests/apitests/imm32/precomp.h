#pragma once

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <imm.h>
#include <ddk/imm.h>
#include <pseh/pseh2.h>
#include <ndk/umtypes.h>
#include <ndk/pstypes.h>
#include "../../../win32ss/include/ntuser.h"
#include <undocuser.h>
#include <imm32_undoc.h>
#include <ndk/rtlfuncs.h>
#include <wine/test.h>
#include <stdio.h>
