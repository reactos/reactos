/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2.2 Library
 * FILE:        dll/win32/ws2_32_new/inc/ws2_32.h
 * PURPOSE:     WinSock 2.2 Main Header
 */

#ifndef _WS2_32_NEW_PCH_
#define _WS2_32_NEW_PCH_

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
//#define _WIN32_WINNT 0x502
#define NTOS_MODE_USER
#define _CRT_SECURE_NO_DEPRECATE
#define WINSOCK_API_LINKAGE

/* C Header */
#include <stdio.h>
#include <stdlib.h>

/* PSDK and NDK Headers */
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winnls.h>
#include <winuser.h>
#include <ws2spi.h>
#include <ndk/rtlfuncs.h>

/* Winsock Helper Header */
#include <ws2help.h>

#include <nsp_dns.h>

/* Missing definitions */
#define SO_OPENTYPE                 0x7008
#define SO_SYNCHRONOUS_NONALERT     0x20

/* Internal headers */
#include "ws2_32p.h"

#endif /* _WS2_32_NEW_PCH_ */
