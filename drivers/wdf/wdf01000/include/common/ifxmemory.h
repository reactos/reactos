/*
Abstract:

    Abstract base class for a memory object.  It is necessary to split the
    memory interface away from an FxObject derived base class so that we can
    hand out WDFMEMORY handles that are embedded within other FxObject derived
    classes without having to embed another FxObject derived class within the
    parent.

Author:



Environment:

    kernel mode only

Revision History:

--*/

#ifndef __IFX_MEMORY_H__
#define __IFX_MEMORY_H__

#include "wdf.h"

// begin_wpp enum
enum IFxMemoryFlags {
    IFxMemoryFlagReadOnly = 0x0001,
};
// end_wpp


class IFxMemory {

public:
    virtual
    WDFMEMORY
    GetHandle(
        VOID
        ) =0;
};

#endif //__IFX_MEMORY_H__