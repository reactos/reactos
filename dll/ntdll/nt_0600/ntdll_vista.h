#pragma once

#define _NTSYSTEM_
#define _NTDLLBUILD_

#include <stdarg.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winnt.h>

#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>
#include <ndk/umfuncs.h>
#include <ndk/ldrfuncs.h>

#define NDEBUG
#include <debug.h>

#include "../include/ntdllp.h"
