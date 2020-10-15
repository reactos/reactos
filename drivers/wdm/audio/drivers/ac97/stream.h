#ifndef _STREAM_H_
#define _STREAM_H_

const int DMA_ENGINE_OFF        = 0;
const int DMA_ENGINE_PAUSE      = 1;
const int DMA_ENGINE_PEND       = 2;
const int DMA_ENGINE_ON         = 3;

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


class CMiniportStream : public IDrmAudioStream
{
    UCHAR UpdateDMA(void);

public:
    CMiniport*                  Miniport;
    ULONG                       Channel;        // channel this stream handles.
    BOOL                        Capture;        // TRUE=Capture,FALSE=Render
    WORD                        NumberOfChannels; // Number of channels
    DEVICE_POWER_STATE  m_PowerState;       // Current power state of the device.



    PKSDATAFORMAT_WAVEFORMATEX  DataFormat;     // Data Format


    PSERVICEGROUP               ServiceGroup;   // service group helps with DPCs
    ULONG                       CurrentRate;    // Current Sample Rate
    ULONG                       DMAEngineState; // DMA engine state
    ULONG                       m_ulBDAddr;     // Offset of the stream's DMA registers.

    PHYSICAL_ADDRESS            BDList_PhysAddr;     // Physical address of BDList
    tBDEntry                    *BDList;             // Virtual Address of BDList

public:
    ~CMiniportStream();



    //
    // DMA start/stop/pause/reset routines.
    //
    void ResetDMA (void);
    void PauseDMA (void);
    void ResumeDMA (ULONG state = DMA_ENGINE_ON);

    /*************************************************************************
     * Include IDrmAudioStream (public/exported) methods.
     *************************************************************************
     */
    IMP_IDrmAudioStream;


    STDMETHODIMP_(NTSTATUS) SetFormat
    (
        _In_  PKSDATAFORMAT   Format
    );




    //
    // This method is called when the device changes power states.
    //
    virtual NTSTATUS PowerChangeNotify
    (
        IN  POWER_STATE NewState
    ) = 0;

    //
    // Return the current sample rate.
    //
    ULONG GetCurrentSampleRate (void)
    {
        return CurrentRate;
    }

    NTSTATUS Init
    (
        IN  CMiniport*              Miniport_,
        IN  ULONG                   Channel_,
        IN  BOOLEAN                 Capture_,
        IN  PKSDATAFORMAT           DataFormat_,
        OUT PSERVICEGROUP           *ServiceGroup_
    );


};



#define IMP_CMiniportStream(cType) \
    STDMETHODIMP_(NTSTATUS) cType::SetFormat (_In_ PKSDATAFORMAT Format) \
      { return CMiniportStream::SetFormat(Format); }

#endif
