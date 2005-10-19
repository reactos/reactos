/* $Id:$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Service
 * FILE:            subsys/system/dhcp
 * PURPOSE:         DHCP client service entry point
 * PROGRAMMER:      Art Yerkes (arty@users.sf.net)
 * UPDATE HISTORY:
 *                  Created 03/08/2005
 */

#include <windows.h>
#include "dhcpd.h"
#include "version.h"

typedef struct _DHCP_API_REQUEST {
    int type;
    UINT flags;
    LPDHCPAPI_CLASSID class_id;
    DHCP_API_PARAMS_ARRAY vendor_params;
    DHCP_API_PARAMS_ARRAY general_params;
    LPWSTR request_id, adapter_name;
} DHCP_API_REQUEST;

typedef struct _DHCP_MANAGED_ADAPTER {
    LPWSTR adapter_name, hostname, dns_server;
    UINT   adapter_index;
    struct sockaddr_in address, netmask;
    struct interface_info *dhcp_info;
} DHCP_MANAGED_ADAPTER;

#define DHCP_REQUESTPARAM    WM_USER + 0
#define DHCP_PARAMCHANGE     WM_USER + 1
#define DHCP_CANCELREQUEST   WM_USER + 2
#define DHCP_NOPARAMCHANGE   WM_USER + 3
#define DHCP_MANAGEADAPTER   WM_USER + 4
#define DHCP_UNMANAGEADAPTER WM_USER + 5

UINT DhcpEventTimer;
HANDLE DhcpServiceThread;
DWORD  DhcpServiceThreadId;
LIST_ENTRY ManagedAdapters;

LRESULT WINAPI ServiceThread( PVOID Data ) {
    MSG msg;

    while( GetMessage( &msg, 0, 0, 0 ) ) {
        switch( msg.message ) {
        case DHCP_MANAGEADAPTER:

            break;

        case DHCP_UNMANAGEADAPTER:
            break;

        case DHCP_REQUESTPARAM:
            break;

        case DHCP_CANCELREQUEST:
            break;

        case DHCP_PARAMCHANGE:
            break;

        case DHCP_NOPARAMCHANGE:
            break;
        }
    }
}

int main( int argc, char **argv ) {
}
