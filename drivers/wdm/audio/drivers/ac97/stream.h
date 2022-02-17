#ifndef _STREAM_H_
#define _STREAM_H_

const int DMA_ENGINE_OFF        = 0;
const int DMA_ENGINE_PAUSE      = 1;
const int DMA_ENGINE_PEND       = 2;
const int DMA_ENGINE_ON         = 3;

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
    WavePins                    Pin;        // channel this stream handles.
    BOOL                        Capture;        // TRUE=Capture,FALSE=Render
    WORD                        NumberOfChannels; // Number of channels
    DEVICE_POWER_STATE  m_PowerState;       // Current power state of the device.

    PPORTSTREAM_                PortStream;



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


    //
    // Get playback position
    //
    int GetBuffPos
    (
        DWORD* buffPos
    );
    
    // 
    // Stream regiter read/write
    // 
    void WriteReg8(ULONG addr, UCHAR data);
    void WriteReg16(ULONG addr, USHORT data);
    void WriteReg32(ULONG addr, ULONG data);
    UCHAR ReadReg8(ULONG addr);
    USHORT ReadReg16(ULONG addr);
    ULONG ReadReg32(ULONG addr);

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
    void PowerChangeNotify
    (
        IN  POWER_STATE NewState
    );

    virtual void PowerChangeNotify_
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

    NTSTATUS Init
    (
        IN  CMiniport*              Miniport_,
        IN  PUNKNOWN                PortStream,
        IN  WavePins                Pin_,
        IN  BOOLEAN                 Capture_,
        IN  PKSDATAFORMAT           DataFormat_,
        OUT PSERVICEGROUP           *ServiceGroup_
    );


    virtual void InterruptServiceRoutine() PURE;
    virtual NTSTATUS Init_() PURE;

    NTSTATUS NonDelegatingQueryInterface
    (
        _In_         REFIID  Interface,
        _COM_Outptr_ PVOID * Object,
        _In_         REFIID iStream,
        _In_         PUNKNOWN stream
    );


    NTSTATUS SetState
    (
        _In_  KSSTATE State
    );

    NTSTATUS NormalizePhysicalPosition
    (
        _Inout_ PLONGLONG PhysicalPosition
    );


    PVOID BDList_Alloc();
    void BDList_Free();

    void UpdateLviCyclic() 
    {
        UCHAR CIV = ReadReg8(X_CIV);
        WriteReg8(X_LVI, (CIV-1) & BDL_MASK);
    }
};



#define IMP_CMiniportStream_SetFormat(cType) \
    STDMETHODIMP_(NTSTATUS) cType::SetFormat (_In_ PKSDATAFORMAT Format) \
      { return CMiniportStream::SetFormat(Format); }

#define IMP_CMiniportStream_QueryInterface(cType, sType) \
    STDMETHODIMP_(NTSTATUS) cType::NonDelegatingQueryInterface(     \
        _In_         REFIID  Interface,                             \
        _COM_Outptr_ PVOID  *Object)                                \
    {   return CMiniportStream::NonDelegatingQueryInterface(        \
            Interface, Object, IID_##sType, (sType*)this); }

#define IMP_CMiniport_SetState(cType) \
    STDMETHODIMP_(NTSTATUS) cType::SetState (_In_ KSSTATE State) \
    { return CMiniportStream::SetState(State); }

#define IMP_CMiniport_NormalizePhysicalPosition(cType) \
    STDMETHODIMP_(NTSTATUS) cType::NormalizePhysicalPosition (_Inout_ PLONGLONG PhysicalPosition) \
    { return CMiniportStream::NormalizePhysicalPosition(PhysicalPosition); }

#endif
