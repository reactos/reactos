#include <stdio.h>
#include <stdarg.h>

#define WIN32_NO_STATUS
#include <windows.h>
#include <winerror.h>
#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>
#include <ntsam.h>

#include "sam_c.h"

#include <wine/debug.h>
