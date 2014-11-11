
#ifndef __TCPIP_PRECOMP_H__
#define __TCPIP_PRECOMP_H__

#include <ntifs.h>
#include <windef.h>
#include <tdi.h>
#include <tdiinfo.h>
#include <tdikrnl.h>
#include <tcpioctl.h>
#include <ndis.h>
#include <ipifcons.h>

typedef unsigned short u_short;
#include <ws2def.h>

#include <rtlfuncs.h>

#include <pseh/pseh2.h>

#include <lwip/icmp.h>
#include <lwip/ip.h>
#include <lwip/raw.h>
#include <lwip/snmp.h>
#include <lwip/tcpip.h>
#include <lwip/udp.h>
#include <netif/etharp.h>

#include "entities.h"
#include "address.h"
#include "information.h"
#include "interface.h"
#include "ndis_lwip.h"
#include "tcp.h"

#endif /* __TCPIP_PRECOMP_H__ */
