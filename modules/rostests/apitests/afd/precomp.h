/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Precompiled header for afd_apitest
 * COPYRIGHT:   Copyright 2018 Thomas Faber (thomas.faber@reactos.org)
 */

#if !defined(_AFD_APITEST_PRECOMP_H_)
#define _AFD_APITEST_PRECOMP_H_

#include <apitest.h>

#define WIN32_NO_STATUS
#include <ndk/exfuncs.h>
#include <ndk/iofuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/rtlfuncs.h>

#include <winsock2.h>
#include <tcpioctl.h>
#include <tdi.h>
#include <afd/shared.h>

#include "AfdHelpers.h"

#endif /* _AFD_APITEST_PRECOMP_H_ */
