/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/smlib/precomp.h
 * PURPOSE:         SMLIB Library Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

#ifndef _SMLIB_PCH_
#define _SMLIB_PCH_

#define WIN32_NO_STATUS
#include <windef.h>
#include <winreg.h>
#define NTOS_MODE_USER
#include <ndk/cmfuncs.h>
#include <ndk/lpctypes.h>
#include <ndk/lpcfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/rtlfuncs.h>

#include <sm/helper.h>

#endif /* _SMLIB_PCH_ */
