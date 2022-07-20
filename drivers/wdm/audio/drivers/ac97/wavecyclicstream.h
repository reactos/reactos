/********************************************************************************
**    Copyright (c) 1998-2000 Microsoft Corporation. All Rights Reserved.
**
**       Portions Copyright (c) 1998-1999 Intel Corporation
**
********************************************************************************/

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
 * CMiniportWaveCyclicStream
 *****************************************************************************
 * AC97 wave miniport stream.
 */
class CMiniportWaveCyclicStream : public IMiniportWaveCyclicStream,
                               public CUnknown,
                               public CMiniportStream
{
private:
    DWORD m_bufferSize;

    /*************************************************************************
     * CMiniportWaveCyclicStream methods
     *************************************************************************
     *
     * These are private member functions used internally by the object.  See
     * ICHWAVE.CPP for specific descriptions.
     */

    CMiniportWaveCyclic* Wave() {
      return (CMiniportWaveCyclic*)Miniport; }

    NTSTATUS ResizeBuffer(void);

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
    DEFINE_STD_CONSTRUCTOR (CMiniportWaveCyclicStream);

    ~CMiniportWaveCyclicStream ();

    /*************************************************************************
     * Include IMiniportWaveCyclicStream (public/exported) methods.
     *************************************************************************
     */
    IMP_IMiniportWaveCyclicStream;

    /*************************************************************************
     * CMiniportWaveCyclicStream methods
     *************************************************************************
     */

    //
    // Initializes the Stream object.
    //
    NTSTATUS Init_();

    void InterruptServiceRoutine();
};

#endif

