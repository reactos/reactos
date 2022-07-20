/********************************************************************************
**    Copyright (c) 1998-2000 Microsoft Corporation. All Rights Reserved.
**
**       Portions Copyright (c) 1998-1999 Intel Corporation
**
********************************************************************************/

/* The file rtstream.h was reviewed by LCA in June 2011 and is acceptable for use by Microsoft. */

#ifndef _RTSTREAM_H_
#define _RTSTREAM_H_

#include "shared.h"

#if (NTDDI_VERSION >= NTDDI_VISTA)

//*****************************************************************************
// Defines
//*****************************************************************************

//
// The scatter gather can (only) handle 32 entries
//
const int MAX_BDL_ENTRIES = 32;

//
// Mask for accessing the scatter gather entries with a counter.
//
const int BDL_MASK = 31;


//*****************************************************************************
// Classes
//*****************************************************************************

/*****************************************************************************
 * CAC97MiniportWaveRTStream
 *****************************************************************************
 * AC97 wave miniport stream.
 */
class CAC97MiniportWaveRTStream : public IMiniportWaveRTStream,
                                             public CUnknown,
                                             public CMiniportStream
{
private:
    //
    // CAC97MiniportWaveRTStream private variables
    //
    DEVICE_POWER_STATE  m_PowerState;       // Current power state of the device.


    int                 mapEntries;
    tBDEntry            *BDList;
    PMDL                BDListMdl;


    /*************************************************************************
     * CAC97MiniportWaveRTStream methods
     *************************************************************************
     *
     * These are private member functions used internally by the object.  See
     * ICHWAVE.CPP for specific descriptions.
     */

public:
    /*************************************************************************
     * The following two macros are from STDUNK.H.  DECLARE_STD_UNKNOWN()
     * defines inline IUnknown implementations that use CUnknown's aggregation
     * support.  NonDelegatingQueryInterface() is declared, but it cannot be
     * implemented generically.  Its definition appears in ICHWAVE.CPP.
     * DEFINE_STD_CONSTRUCTOR() defines inline a constructor which accepts
     * only the outer unknown, which is used for aggregation.  The standard
     * create macro (in ICHWAVE.CPP) uses this constructor.
     */
    DECLARE_STD_UNKNOWN ();
    DEFINE_STD_CONSTRUCTOR (CAC97MiniportWaveRTStream);

    ~CAC97MiniportWaveRTStream ();

    /*************************************************************************
     * Include IMiniportWaveRTStream (public/exported) methods.
     *************************************************************************
     */
    IMP_IMiniportWaveRTStream;

    /*************************************************************************
     * CAC97MiniportWaveRTStream methods
     *************************************************************************
     */

    //
    // Initializes the Stream object.
    //
    NTSTATUS Init
    (
        IN  CAC97MiniportWaveRT    *Miniport_,
        IN  PPORTWAVERTSTREAM       PortStream,
        IN  ULONG               Channel,
        IN  BOOLEAN             Capture,
        IN  PKSDATAFORMAT       DataFormat
    );

    //
    // Friends
    //
    friend
    NTSTATUS CAC97MiniportWaveRT::InterruptServiceRoutine
    (
        IN  PINTERRUPTSYNC  InterruptSync,
        IN  PVOID           StaticContext
    );
};

#endif          // (NTDDI_VERSION >= NTDDI_VISTA)

#endif          // _RTSTREAM_H_


