#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#define NTOS_MODE_USER
#include <ndk/exfuncs.h>
#include <ndk/mmfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>

#include <psapi.h>
#include <epsapi/epsapi.h>

#include "internal.h"

#include <pseh/pseh2.h>
