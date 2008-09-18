/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2.2 Library
 * FILE:        lib/ws2_32.h
 * PURPOSE:     WinSock 2.2 Main Header
 */

#ifndef __WS2_32_H
#define __WS2_32_H

/* Definitions for NDK Usage */
#define WIN32_NO_STATUS
//#define _WIN32_WINNT 0x502
#define NTOS_MODE_USER
#define _CRT_SECURE_NO_DEPRECATE
#define WINSOCK_API_LINKAGE
#define DBG 1

/* C Header */
#include <stdio.h>

/* PSDK and NDK Headers */
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <Ws2spi.h>
#include <ndk/umtypes.h>
#include <ndk/rtlfuncs.h>

/* Shared NSP Headers */
#include <nsp_dns.h>

/* Winsock Helper Header */
#include <ws2help.h>

/* Missing definitions */
#define SO_OPENTYPE                 0x7008
#define SO_SYNCHRONOUS_NONALERT     0x20

/* Internal headers */
#include "ws2_32p.h"

#endif

