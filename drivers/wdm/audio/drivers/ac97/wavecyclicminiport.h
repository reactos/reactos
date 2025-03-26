/********************************************************************************
**    Copyright (c) 1998-2000 Microsoft Corporation. All Rights Reserved.
**
**       Portions Copyright (c) 1998-1999 Intel Corporation
**
********************************************************************************/

#ifndef _MINWAVE_H_
#define _MINWAVE_H_

#include "shared.h"
#define PPORT_ PPORTWAVECYCLIC
#define PPORTSTREAM_ PUNKNOWN
#include "miniport.h"

/*****************************************************************************
 * Forward References
 *****************************************************************************
 */
class CMiniportWaveCyclicStream;

extern NTSTATUS CreateMiniportWaveCyclicStream
(
    OUT     CMiniportWaveCyclicStream **   pWaveIchStream,
    IN      PUNKNOWN                    pUnknown,
    _When_((PoolType & NonPagedPoolMustSucceed) != 0,
       __drv_reportError("Must succeed pool allocations are forbidden. "
			 "Allocation failures cause a system crash"))
    IN      POOL_TYPE                   PoolType
);

/*****************************************************************************
 * Classes
 *****************************************************************************
 */

/*****************************************************************************
 * CMiniportWaveCyclic
 *****************************************************************************
 * AC97 wave PCI miniport.  This object is associated with the device and is
 * created when the device is started.  The class inherits IMiniportWaveCyclic
 * so it can expose this interface, CUnknown so it automatically gets
 * reference counting and aggregation support, and IPowerNotify for ACPI
 * power management notification.
 */
class CMiniportWaveCyclic : public IMiniportWaveCyclic,
                                public CUnknown,
                                public CMiniport
{
private:
    // The stream class accesses a lot of private member variables.
    // A better way would be to abstract the access through member
    // functions which on the other hand would produce more overhead
    // both in CPU time and programming time.
    friend class CMiniportWaveCyclicStream;

public:
    /*************************************************************************
     * The following two macros are from STDUNK.H.  DECLARE_STD_UNKNOWN()
     * defines inline IUnknown implementations that use CUnknown's aggregation
     * support.  NonDelegatingQueryInterface() is declared, but it cannot be
     * implemented generically.  Its definition appears in MINWAVE.CPP.
     * DEFINE_STD_CONSTRUCTOR() defines inline a constructor which accepts
     * only the outer unknown, which is used for aggregation.  The standard
     * create macro (in MINWAVE.CPP) uses this constructor.
     */
    DECLARE_STD_UNKNOWN ();
    DEFINE_STD_CONSTRUCTOR (CMiniportWaveCyclic);

    //
    // Include IMiniportWaveCyclic (public/exported) methods
    //
    IMP_IMiniportWaveCyclic;
};

#endif          // _MINWAVE_H_

