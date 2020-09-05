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

//
// These are the differnet DMA engine states. The HW can only be in two states
// (on or off) but there are also 2 transition states.
// From DMA_ENGINE_RESET a failed start transitions to DMA_ENGINE_NEED_START, a
// successful start transitions to DMA_ENGINE_ON. From DMA_ENGINE_NEED_START a
// successful restart transitions to DMA_ENGINE_ON, a pause or stop transitions
// to DMA_ENGINE_RESET. From DMA_ENGINE_ON a pause or stop transitions to
// DMA_ENGINE_OFF. From DMA_ENGINE_OFF a start transitions to DMA_ENGINE_ON,
// a reset would transitions to DMA_ENGINE_RESET.
//
const int DMA_ENGINE_OFF        = 0;
const int DMA_ENGINE_ON         = 1;
const int DMA_ENGINE_RESET      = 2;
const int DMA_ENGINE_NEED_START = 3; // DMA_ENGINE_RESET | DMA_ENGINE_ON


 
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
// Structure to describe the AC97 Buffer Descriptor List (BDL).
// The AC97 can handle 32 entries, they are allocated at once in common
// memory (non-cached memory). To avoid slow-down of CPU, the additional
// information for handling this structure is stored in tBDList.
//
typedef struct tagBDEntry
{
    DWORD   dwPtrToPhyAddress;
    WORD    wLength;
    WORD    wPolicyBits;
} tBDEntry;

//
// Structure needed to keep track of the BDL tables.
// This structure is seperated from tBDEntry because we want to
// have it in cached memory (and not along with tBDEntry in non-
// cached memory).
//
typedef struct tagBDList
{
    PHYSICAL_ADDRESS        PhysAddr;       // Physical address of BDList
    volatile tBDEntry       *pBDEntry;      // Virtual Address of BDList
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
                               public IDrmAudioStream,
                               public CUnknown
{
private:
    //
    // CMiniportWaveICHStream private variables
    //
    CMiniportWaveICH *          Wave;           // Miniport Object
    ULONG                       Channel;        // channel this stream handles.
    BOOL                        Capture;        // TRUE=Capture,FALSE=Render
    ULONG                       CurrentRate;    // Current Sample Rate
    WORD                        NumberOfChannels; // Number of channels
    PSERVICEGROUP               ServiceGroup;   // service group helps with DPCs
    tBDList                     stBDList;       // needed for scatter gather org.
    PPORTWAVEPCISTREAM          PortStream;     // Port Stream Interface
    PKSDATAFORMAT_WAVEFORMATEX  DataFormat;     // Data Format
    KSPIN_LOCK                  MapLock;        // for processing mappings.
    ULONG               m_ulBDAddr;         // Offset of the stream's DMA registers.
    ULONG               DMAEngineState;     // DMA engine state (STOP, PAUSE, RUN)
    ULONGLONG           TotalBytesMapped;   // factor in position calculation
    ULONGLONG           TotalBytesReleased; // factor in position calculation
    DEVICE_POWER_STATE  m_PowerState;       // Current power state of the device.


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

    //
    // DMA start/stop/pause/reset routines.
    //
    NTSTATUS ResetDMA (void);
    NTSTATUS PauseDMA (void);
    NTSTATUS ResumeDMA (void);
    

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
     * Include IDrmAudioStream (public/exported) methods.
     *************************************************************************
     */
    IMP_IDrmAudioStream;
    
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

    //
    // Return the current sample rate.
    //
    ULONG GetCurrentSampleRate (void)
    {
        return CurrentRate;
    }

    //
    // Friends
    //
    friend
    NTSTATUS CMiniportWaveICH::InterruptServiceRoutine
    (
        IN  PINTERRUPTSYNC  InterruptSync,
        IN  PVOID           StaticContext
    );
};

#endif

