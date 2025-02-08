#if !defined(_SKLHDAUDBUS_FDO_H_)
#define _SKLHDAUDBUS_FDO_H_

union baseaddr {
    PVOID Base;
    UINT8* baseptr;
};

typedef struct _PCI_BAR {
    union baseaddr Base;
    ULONG Len;
} PCI_BAR, * PPCI_BAR;

#include "adsp.h"

#define HDA_UNSOL_QUEUE_SIZE	64
#define MAX_NOTIF_EVENTS 16
#define HDA_MAX_CODECS		16	/* limit by controller side */

struct _FDO_CONTEXT;
struct _PDO_DEVICE_DATA;

typedef struct _HDAC_STREAM_CALLBACK {
    BOOLEAN InUse;
    PDEVICE_OBJECT Fdo;
    PHDAUDIO_DMA_NOTIFICATION_CALLBACK NotificationCallback;
    PVOID CallbackContext;
} HDAC_STREAM_CALLBACK, *PHDAC_STREAM_CALLBACK;

typedef struct _HDAC_BDLENTRY {
    UINT32 lowAddr;
    UINT32 highAddr;
    UINT32 len;
    UINT32 ioc;
} HDAC_BDLENTRY, *PHDAC_BDLENTRY;

typedef struct _HDAC_STREAM {
    struct _FDO_CONTEXT* FdoContext;
    struct _PDO_DEVICE_DATA* PdoContext;

    PMDL mdlBuf;
    UINT32* posbuf;

    HDAC_BDLENTRY* bdl;

    BOOLEAN stripe;

    UINT32 bufSz;
    UINT32 periodBytes;
    UINT32 fifoSize;
    UINT16 numBlocks;

    UINT8* sdAddr;
    UINT32 int_sta_mask;

    UINT8* spib_addr; //spbcap

    UINT8 streamTag;
    UINT8 idx;

    HDAUDIO_STREAM_FORMAT streamFormat;

    PKEVENT registeredEvents[MAX_NOTIF_EVENTS];
    HDAC_STREAM_CALLBACK registeredCallbacks[MAX_NOTIF_EVENTS];

    BOOLEAN running;
    BOOLEAN irqReceived;
} HDAC_STREAM, *PHDAC_STREAM;

typedef struct _HDAC_RIRB {
    UINT32 response;
    UINT32 response_ex;
} HDAC_RIRB, *PHDAC_RIRB;

typedef struct _HDAC_CODEC_XFER {
    PHDAUDIO_CODEC_TRANSFER xfer[HDA_MAX_CORB_ENTRIES];
} HDAC_CODEC_XFER, *PHDAC_CODEC_XFER;

typedef struct _HDAC_RB {
    union {
        UINT32* buf;
        PHDAC_RIRB rirbbuf;
    };
    PHYSICAL_ADDRESS addr;
    UINT16 rp, wp;
    LONG cmds[HDA_MAX_CODECS];
    KEVENT xferEvent[HDA_MAX_CODECS];
    HDAC_CODEC_XFER xfer[HDA_MAX_CODECS];
} HDAC_RB, *PHDAC_RB;

typedef struct _GRAPHICSWORKITEM_CONTEXT {
    struct _FDO_CONTEXT* FdoContext;
    UNICODE_STRING GPUDeviceSymlink;
} GRAPHICSWORKITEM_CONTEXT, *PGRAPHICSWORKITEM_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(GRAPHICSWORKITEM_CONTEXT, GraphicsWorkitem_GetContext)

typedef struct _GRAPHICSIOTARGET_CONTEXT {
    struct _FDO_CONTEXT* FdoContext;
    DXGK_GRAPHICSPOWER_REGISTER_OUTPUT graphicsPowerRegisterOutput;
} GRAPHICSIOTARGET_CONTEXT, *PGRAPHICSIOTARGET_CONTEXT;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(GRAPHICSIOTARGET_CONTEXT, GraphicsIoTarget_GetContext)

typedef struct _FDO_CONTEXT
{
    WDFDEVICE WdfDevice;

    UINT16 venId;
    UINT16 devId;
    UINT8 revId;

    PCI_BAR m_BAR0; //required
    PCI_BAR m_BAR4; //Intel AudioDSP
    BUS_INTERFACE_STANDARD BusInterface; //PCI Bus Interface
    WDFINTERRUPT Interrupt;

    //Graphics Notifications
    PVOID GraphicsNotificationHandle;
    WDFWAITLOCK GraphicsDevicesCollectionWaitLock;
    WDFCOLLECTION GraphicsDevicesCollection;
    ULONG GraphicsCodecAddress;
    BOOLEAN UseSGPCCodec;

    UINT8 *mlcap;
    UINT8 *ppcap;
    UINT8 *spbcap;

    BOOLEAN ControllerEnabled;

    BOOLEAN is64BitOK;
    UINT16 hwVersion;

    UINT32 captureIndexOff;
    UINT32 playbackIndexOff;
    UINT32 captureStreams;
    UINT32 playbackStreams;
    UINT32 numStreams;

    PHDAC_STREAM streams;
    struct _PDO_DEVICE_DATA* codecs[HDA_MAX_CODECS];

    PADSP_INTERRUPT_CALLBACK dspInterruptCallback;
    PVOID dspInterruptContext;
    PVOID nhlt;
    UINT64 nhltSz;
    PVOID sofTplg;
    UINT64 sofTplgSz;

    //unsolicited events
    HDAC_RIRB unsol_queue[HDA_UNSOL_QUEUE_SIZE * 2];
    UINT unsol_rp, unsol_wp;
    BOOL processUnsol;

    //bit flags of detected codecs
    UINT16 codecMask;
    USHORT numCodecs;

    HDAC_RB corb;
    HDAC_RB rirb;

    UINT8 *rb; //CORB and RIRB buffers
    PVOID posbuf;
} FDO_CONTEXT, * PFDO_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FDO_CONTEXT, Fdo_GetContext)

NTSTATUS
Fdo_Create(
	_Inout_ PWDFDEVICE_INIT DeviceInit
);

#endif
