/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver DLL
 * FILE:        include/reactos/winsock/msafd.h
 * PURPOSE:     Ancillary Function Driver DLL header
 */

#define NTOS_MODE_USER
#define WIN32_NO_STATUS
#define _CRT_SECURE_NO_DEPRECATE
#define _WIN32_WINNT 0x502

/* Winsock Headers */
#include <winsock2.h>
#include <mswsock.h>
#include <ws2spi.h>
#include <ws2tcpip.h>
#include <windns.h>
#include <nspapi.h>
#include <ws2atm.h>

/* NDK */
#include <rtlfuncs.h>
#include <obfuncs.h>
#include <exfuncs.h>
#include <iofuncs.h>
#include <kefuncs.h>

/* Shared NSP Header */
#include <nsp_dns.h>

/* Winsock 2 API Helper Header */
#include <ws2help.h>

/* Winsock Helper Header */
#include <wsahelp.h>

/* AFD/TDI Headers */
#include <tdi.h>
#include <afd/shared.h>

/* DNSLIB/API Header */
#include <dnslib.h>
#include <windnsp.h>

/* Library Headers */
#include "msafdlib.h"
#include "rnr20lib.h"
#include "wsmobile.h"
#include "mswinsock.h"

/* EOF */
