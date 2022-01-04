/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxInterruptUm.hpp

Abstract:

    This module implements a frameworks managed interrupt object

Author:



Environment:

    User mode only

Revision History:



--*/
#ifndef _FXINTERRUPTUM_H_
#define _FXINTERRUPTUM_H_

#include "FxInterrupt.hpp"

__inline
struct _KINTERRUPT*
FxInterrupt::GetInterruptPtr(
    VOID
    )
{
    //
    // m_Interrupt is always NULL in UMDF.
    //
    return NULL;
}

#endif // _FXINTERRUPTUM_H_

