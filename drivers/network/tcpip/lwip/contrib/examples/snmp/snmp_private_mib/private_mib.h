/**
 * @file
 * Exports Private lwIP MIB 
 */

#ifndef LWIP_HDR_PRIVATE_MIB_H
#define LWIP_HDR_PRIVATE_MIB_H

#include "lwip/apps/snmp_opts.h"

#include "lwip/apps/snmp_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* export MIB */
extern const struct snmp_mib mib_private;

void lwip_privmib_init(void);

#ifdef __cplusplus
}
#endif

#endif
