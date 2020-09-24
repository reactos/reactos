/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxResourceCollection.cpp

Abstract:

    This module implements a base object for derived collection classes and
    the derived collection classes.

Author:



Environment:

    Kernel mode only

Revision History:

--*/

#include "FxSupportPch.hpp"

#if defined(EVENT_TRACING)
// Tracing support
extern "C" {
#include "FxResourceCollectionKm.tmh"
}
#endif

FxCmResList::~FxCmResList()
{
}


