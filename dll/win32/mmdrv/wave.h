
// FIXME: Should be moved somewhere else?
typedef struct _WAVE_DD_VOLUME {
    ULONG   Left;
    ULONG   Right;
} WAVE_DD_VOLUME, *PWAVE_DD_VOLUME;

// driver
#define WAVE_DD_STOP        0x0001
#define WAVE_DD_PLAY        0x0002      // output devices only
#define WAVE_DD_RECORD      0x0003      // input devices only
#define WAVE_DD_RESET       0x0004

// ioctl
#define WAVE_DD_IDLE        0x0000
#define WAVE_DD_STOPPED     0x0001      // stopped
#define WAVE_DD_PLAYING     0x0002      // output devices only
#define WAVE_DD_RECORDING   0x0003      // input devices only



typedef enum {
    WaveThreadInvalid,
    WaveThreadAddBuffer,
    WaveThreadSetState,
    WaveThreadSetData,
    WaveThreadGetData,
    WaveThreadBreakLoop,
    WaveThreadClose,
    WaveThreadTerminate
} WAVETHREADFUNCTION;

// WARNING: MS code below!!
typedef struct {
    OVERLAPPED Ovl;
    LPWAVEHDR WaveHdr;
} WAVEOVL, *PWAVEOVL;

// WARNING: MS code below!!
// per allocation structure for wave
typedef struct tag_WAVEALLOC {
    struct tag_WAVEALLOC *Next;         // Chaining
    UINT                DeviceNumber;   // Which device
    UINT                DeviceType;     // WaveInput or WaveOutput
    DWORD               dwCallback;     // client's callback
    DWORD               dwInstance;     // client's instance data
    DWORD               dwFlags;        // Open flags
    HWAVE               hWave;          // handle for stream

    HANDLE              hDev;           // Wave device handle
    LPWAVEHDR           DeviceQueue;    // Buffers queued by application
    LPWAVEHDR           NextBuffer;     // Next buffer to send to device
    DWORD               BufferPosition; // How far we're into a large buffer
    DWORD               BytesOutstanding;
                                        // Bytes being processed by device
    LPWAVEHDR           LoopHead;       // Start of loop if any
    DWORD               LoopCount;      // Number more loops to go

    WAVEOVL             DummyWaveOvl;   // For break loop
                                                                                //
    HANDLE              Event;          // Event for driver syncrhonization
                                        // and notification of auxiliary
                                        // task operation completion.
    WAVETHREADFUNCTION  AuxFunction;    // Function for thread to perform
    union {
        LPWAVEHDR       pHdr;           // Buffer to pass in aux task
        ULONG           State;          // State to set
        struct {
            ULONG       Function;       // IOCTL to use
            PBYTE       pData;          // Data to set or get
            ULONG       DataLen;        // Length of data
        } GetSetData;

    } AuxParam;
                                        // 0 means terminate task.
    HANDLE              AuxEvent1;      // Aux thread waits on this
    HANDLE              AuxEvent2;      // Caller of Aux thread waits on this
    HANDLE              ThreadHandle;   // Handle for thread termination ONLY
    MMRESULT            AuxReturnCode;  // Return code from Aux task
}WAVEALLOC, *PWAVEALLOC;

/* Misc should move to own header */
MMRESULT GetDeviceCapabilities(DWORD ID, UINT DeviceType,
                                      LPBYTE pCaps, DWORD Size);

DWORD AuxGetAudio(DWORD dwID, PBYTE pVolume, DWORD sizeVolume);
DWORD AuxSetAudio(DWORD dwID, PBYTE pVolume, DWORD sizeVolume);

typedef struct _AUX_DD_VOLUME {
        ULONG   Left;
        ULONG   Right;
} AUX_DD_VOLUME, *PAUX_DD_VOLUME;
