#ifndef ROSDHCP_H
#define ROSDHCP_H

#include <roscfg.h>
#include <windows.h>
#include <winnt.h>
#include <iprtrmib.h>
#include <iphlpapi.h>
#include <winsock2.h>
#include <stdio.h>
#include <setjmp.h>
#include "stdint.h"
#include "predec.h"
#include "debug.h"
#define IFNAMSIZ MAX_INTERFACE_NAME_LEN
#undef interface /* wine/objbase.h -- Grrr */

#undef IGNORE
#undef ACCEPT
#undef PREFER
#define DHCP_DISCOVER_INTERVAL 15
#define DHCP_REBOOT_TIMEOUT 300
#define DHCP_BACKOFF_MAX 300
#define _PATH_DHCLIENT_PID "\\systemroot\\system32\\drivers\\etc\\dhclient.pid"

typedef void *VOIDPTR;

#define NTOS_MODE_USER
#include <ntos.h>
typedef u_int32_t uintTIME;
#define TIME uintTIME
#include "dhcpd.h"

#define INLINE inline
#define PROTO(x) x

typedef void (*handler_t) PROTO ((struct packet *));

typedef struct _DHCP_ADAPTER {
    LIST_ENTRY     ListEntry;
    MIB_IFROW      IfMib;
    MIB_IPADDRROW  IfAddr;
    SOCKADDR       Address;
    struct interface_info DhclientInfo;
    struct client_state DhclientState;
    struct client_config DhclientConfig;
    struct sockaddr_in ListenAddr;
    unsigned int BindStatus;
    char recv_buf[1];
} DHCP_ADAPTER, *PDHCP_ADAPTER;

#define random rand
#define srandom srand

#endif/*ROSDHCP_H*/
