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
    ULONG               ulTag;                  //tag, a simple counter.
    PHYSICAL_ADDRESS    PhysAddr;               //phys. addr. of buffer
    PVOID               pVirtAddr;              //virt. addr. of buffer
    ULONG               ulBufferLength;         //buffer length
} tMapData;

//
// Structure needed to keep track of the BDL tables.
// This structure is seperated from tBDEntry because we want to
// have it in cached memory (and not along with tBDEntry in non-
// cached memory).
//
typedef struct tagBDList
{
    tBDEntry                *pBDEntryBackup;// needed for rearranging the BDList
    tMapData                *pMapData;      // mapping list
    tMapData                *pMapDataBackup;// needed for rearranging the BDList
    int                     nHead;          // index for the BDL Head
    int                     nTail;          // index for the BDL Tail
    ULONG                   ulTagCounter;   // the tag is a simple counter.
    int                     nBDEntries;     // number of entries.
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

    tBDList                     stBDList;       // needed for scatter gather org.
    PPORTWAVEPCISTREAM          PortStream;     // Port Stream Interface

    KSPIN_LOCK                  MapLock;        // for processing mappings.
    ULONGLONG           TotalBytesMapped;   // factor in position calculation
    ULONGLONG           TotalBytesReleased; // factor in position calculation



    /*************************************************************************
     * CMiniportWaveICHStream methods
     *************************************************************************
     *
     * These are private member functions used internally by the object.  See
     * ICHWAVE.CPP for specific descriptions.
     */

    //
    // Moves the BDL and associated mappings list around.
    //
    void MoveBDList
    (
        IN  int nFirst,
        IN  int nLast,
        IN  int nNewPos
    );

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
    NTSTATUS Init
    (
        IN  CMiniportWaveICH    *Miniport_,
        IN  PPORTWAVEPCISTREAM  PortStream,
        IN  ULONG               Channel,
        IN  BOOLEAN             Capture,
        IN  PKSDATAFORMAT       DataFormat,
        OUT PSERVICEGROUP *     ServiceGroup
    );

    //
    // This method is called when the device changes power states.
    //
    NTSTATUS PowerChangeNotify
    (
        IN  POWER_STATE NewState
    );

    void InterruptServiceRoutine();
};

#endif

