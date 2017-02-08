#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <windef.h>
#include <winbase.h>

#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>
#include <ntsam.h>

#include <sam_c.h>

#include <wine/debug.h>
