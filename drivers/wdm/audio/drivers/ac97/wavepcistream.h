/********************************************************************************
**    Copyright (c) 1998-2000 Microsoft Corporation. All Rights Reserved.
**
**       Portions Copyright (c) 1998-1999 Intel Corporation
**
********************************************************************************/

/* The file wavepcistream.h was reviewed by LCA in June 2011 and is acceptable for use by Microsoft. */

#ifndef _ICHWAVE_H_
#define _ICHWAVE_H_

#include "shared.h"
#include "miniport.h"


//*****************************************************************************
// Data Structures and Typedefs
//*****************************************************************************

//
// This is a description of one mapping entry. It contains information
// about the buffer and a tag which is used for "cancel mappings".
// For one mapping that is described here, there is an entry in the
// scatter gather engine.
//
typedef struct tagMapData
{
    ULONG               ulBufferLength;         //buffer length
    ULONG               ulState;                //buffer state
} tMapData;

//
// Structure needed to keep track of the BDL tables.
// This structure is seperated from tBDEntry because we want to
// have it in cached memory (and not along with tBDEntry in non-
// cached memory).
//
typedef struct tagBDList
{
    int                     nHead;          // index for the BDL Head
    int                     nTail;          // index for the BDL Tail
    int                     nBDEntries;     // number of entries.
    tMapData                pMapData[32];      // mapping list
} tBDList;


//*****************************************************************************
// Classes
//*****************************************************************************

/*****************************************************************************
 * CMiniportWaveICHStream
 *****************************************************************************
 * AC97 wave miniport stream.
 */
class CMiniportWaveICHStream : public IMiniportWavePciStream,
                               public CUnknown,
                               public CMiniportStream
{
private:
    //
    // CMiniportWaveICHStream private variables
    //
    KSPIN_LOCK                  MapLock;        // for processing mappings.
    ULONGLONG           TotalBytesMapped;   // factor in position calculation
    ULONGLONG           TotalBytesReleased; // factor in position calculation
    BOOL                m_inGetMapping;
    
    tBDList                     stBDList;       // needed for scatter gather org.


    /*************************************************************************
     * CMiniportWaveICHStream methods
     *************************************************************************
     *
     * These are private member functions used internally by the object.  See
     * ICHWAVE.CPP for specific descriptions.
     */

    //
    // Called when new mappings have to be processed.
    //
    NTSTATUS GetNewMappings (void);

    //
    // Called when we want to release some mappings.
    //
    NTSTATUS ReleaseUsedMappings (void);

    CMiniportWaveICH* Wave() {
      return (CMiniportWaveICH*)Miniport; }


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
    DEFINE_STD_CONSTRUCTOR (CMiniportWaveICHStream);

    ~CMiniportWaveICHStream ();

    /*************************************************************************
     * Include IMiniportWavePciStream (public/exported) methods.
     *************************************************************************
     */
    IMP_IMiniportWavePciStream;

    /*************************************************************************
     * CMiniportWaveICHStream methods
     *************************************************************************
     */

    //
    // Initializes the Stream object.
    //
    NTSTATUS Init_();

    //
    // This method is called when the device changes power states.
    //
    void PowerChangeNotify_
    (
        IN  POWER_STATE NewState
    );

    void InterruptServiceRoutine();
};

#endif

