/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/secur32/precomp.h
 * PURPOSE:         Security Library Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

#ifndef _SECUR32_PCH_
#define _SECUR32_PCH_

#include <stdarg.h>

/* SDK/DDK/NDK Headers. */
#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <winreg.h>
#define NTOS_MODE_USER
#include <ndk/exfuncs.h>
#include <ndk/rtlfuncs.h>

#include <secext.h>
#include <security.h>

#include "secur32_priv.h"
#include "thunks.h"

#endif /* _SECUR32_PCH_ */
