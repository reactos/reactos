#ifndef _MSV1_0_PRECOMP_H_
#define _MSV1_0_PRECOMP_H_

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#define NTOS_MODE_USER
#include <ndk/cmfuncs.h>
#include <ndk/kefuncs.h>
#include <ndk/lpctypes.h>
#include <ndk/lpcfuncs.h>
#include <ndk/mmfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/setypes.h>
#include <ndk/sefuncs.h>

#include <sspi.h>
#include <ntsecapi.h>
#include <ntsecpkg.h>
#include <ntsam.h>
#include <ntlsa.h>

#include <samsrv/samsrv.h>
//#include <lsass/lsasrv.h>

#include "msv1_0.h"

#include <wine/debug.h>

#endif
