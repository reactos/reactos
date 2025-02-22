/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS DNS Shared Library
 * FILE:        lib/dnslib/precomp.h
 * PURPOSE:     DNSLIB Precompiled Header
 */

#define _CRT_SECURE_NO_DEPRECATE
#define _WIN32_WINNT 0x502
#define WIN32_NO_STATUS

/* PSDK Headers */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windns.h>

/* DNSLIB and DNSAPI Headers */
#include <dnslib.h>
#include <windnsp.h>

/* NDK */
#include <rtlfuncs.h>

/* EOF */
