#ifndef _NTOS_H
#define _NTOS_H
/* $Id: ntos.h,v 1.4 2002/09/07 15:12:12 chorns Exp $ */

#include <stdarg.h>
#include <windef.h>
#include <ntdef.h>

#if defined(NTOS_USER_MODE)
// include windows.h before ntddk.h to get user mode prototype for InterlockedXxx functions
#include <windows.h>
#include <ddk/ntddk.h>
#include <winnt.h>
#include <ddk/ntapi.h>
#include <ddk/ntifs.h>
#include "ntos/i386/segment.h"
#include "ntos/types.h"
#include "ntos/disk.h"
#include "ntos/lpc.h"
#include "ntos/dbg.h"
#include "ntos/npipe.h"
#include "ntos/shared_data.h"
#include "ntos/teb.h"
#include "ntos/minmax.h"
#include "ntos/security.h"
#include "ntos/except.h"
#include "ntos/rtl.h"
#include "ntos/zwtypes.h"
#include "ntos/zw.h"
#include "ntos/win32.h"
#include "ntos/ps.h"
#include "ntos/fs.h"
#include "ntos/ntddblue.h"
#include "ntos/console.h"
#include "ntdll/csr.h"
#include "ntdll/dbg.h"
#include "ntdll/ldr.h"
#include "ntdll/registry.h"
#include "ntdll/rtl.h"
#include "ntdll/trace.h"
#include "kernel32/error.h"
#elif defined(NTOS_KERNEL_MODE)
#include <ddk/ntddk.h>
#include <winbase.h>
#include <winnt.h>
#include <ddk/ntapi.h>
#include <ddk/ntifs.h>
#include "ntos/i386/segment.h"
#include "ntos/types.h"
#include "ntos/disk.h"
#include "ntos/lpc.h"
#include "ntos/npipe.h"
#include "ntos/shared_data.h"
#include "ntos/teb.h"
#include "ntos/minmax.h"
#include "ntos/security.h"
#include "ntos/except.h"
#include "ntos/rtl.h"
#include "ntos/zwtypes.h"
#include "ntos/zw.h"
#include "ntos/win32.h"
#include "ntos/ps.h"
#include "ntos/fs.h"
#include "ntos/ntddblue.h"
#include "ntos/hal.h"
#include "ntos/core.h"
#else
  #error Specify NTOS_USER_MODE or NTOS_KERNEL_MODE
#endif

#endif /* ndef _NTOS_H */
