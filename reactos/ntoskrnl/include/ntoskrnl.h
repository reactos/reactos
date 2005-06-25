#ifndef __INCLUDE_NTOSKRNL_H
#define __INCLUDE_NTOSKRNL_H

#define NTKERNELAPI

/* include the ntoskrnl config.h file */
#include "config.h"
#include <roscfg.h>
  
/* DDK/IFS/NDK Headers */
#include <ddk/ntddk.h>
#include <ddk/ntifs.h>
#include <ddk/wdmguid.h>
#include <ndk/ntndk.h>
#undef IO_TYPE_FILE
#define IO_TYPE_FILE                    0x0F5L /* Temp Hack */
  
/* FIXME: Add to ndk, or at least move somewhere else */
#include <ntos/ntpnp.h>
#include <napi/core.h>

/* ReactOS Headers */
#include <reactos/version.h>
#include <reactos/resource.h>
#include <reactos/bugcodes.h>
#include <reactos/rossym.h>

/* C Headers */
#include <malloc.h>
#include <wchar.h>

/* SEH support with PSEH */
#include <pseh.h>

/* Helper Header */
#include <reactos/helper.h>

/* Internal Headers */
#include "internal/ntoskrnl.h"

#endif /* INCLUDE_NTOSKRNL_H */
