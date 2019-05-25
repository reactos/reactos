#ifndef _CMIDRIVER_PCH_
#define _CMIDRIVER_PCH_

#include <wdm.h>
#include <portcls.h>

#include "debug.hpp"

PVOID
__cdecl
operator new(
    size_t size,
    POOL_TYPE pool_type,
    ULONG tag);

#endif /* _CMIDRIVER_PCH_ */
