#ifndef INTERFACES_HPP
#define INTERFACES_HPP

//
// IUSBHardwareDevice
//
#define DEFINE_ABSTRACT_USBXHCIHARDWARE()                     \
    STDMETHOD_(VOID, SetRuntimeRegister)(THIS_                \
        IN ULONG Offset,                                      \
        IN ULONG Value) PURE;                                 \
                                                              \
    STDMETHOD_(ULONG, GetRuntimeRegister)(THIS_               \
        IN ULONG Offset) PURE;                                \
                                                              \
    STDMETHOD_(VOID, SetOperationalRegister)(THIS_            \
        IN ULONG Offset,                                      \
        IN ULONG Value) PURE;                                 \
                                                              \
    STDMETHOD_(NTSTATUS, GetDeviceInformationByAddress)(THIS_ \
        IN ULONG DeviceAddress,                               \
        OUT PDEVICE_INFORMATION *DeviceInformation) PURE;     \
                                                              \
    STDMETHOD_(NTSTATUS, GetDeviceInformationBySlotId)(THIS_  \
        IN ULONG SlotId,                                      \
        OUT PDEVICE_INFORMATION *DeviceInformation) PURE;     \
                                                              \
    STDMETHOD_(VOID, RingDoorbellRegister)(THIS_              \
        IN ULONG SlotId,                                      \
        IN ULONG Endpoint,                                    \
        IN ULONG StreamId) PURE;                              \

#define IMP_IUSBXHCIHARDWARE                                  \
    STDMETHODIMP_(VOID) SetRuntimeRegister(                   \
        IN ULONG Offset,                                      \
        IN ULONG Value);                                      \
                                                              \
    STDMETHODIMP_(ULONG) GetRuntimeRegister(                  \
        IN ULONG Offset);                                     \
                                                              \
    STDMETHODIMP_(VOID) SetOperationalRegister(               \
        IN ULONG Offset,                                      \
        IN ULONG Value);                                      \
                                                              \
    STDMETHODIMP_(NTSTATUS) GetDeviceInformationByAddress(    \
        IN ULONG DeviceAddress,                               \
        OUT PDEVICE_INFORMATION *DeviceInformation);          \
                                                              \
    STDMETHODIMP_(NTSTATUS) GetDeviceInformationBySlotId(     \
        IN ULONG SlotId,                                      \
        OUT PDEVICE_INFORMATION *DeviceInformation);          \
                                                              \
    STDMETHODIMP_(VOID) RingDoorbellRegister(                 \
        IN ULONG SlotId,                                      \
        IN ULONG Endpoint,                                    \
        IN ULONG StreamId);                                   \


DECLARE_INTERFACE_(IXHCIHardwareDevice, IUSBHardwareDevice)
{
    DEFINE_ABSTRACT_UNKNOWN();
    DEFINE_ABSTRACT_USBHARDWAREDEVICE();
    DEFINE_ABSTRACT_USBXHCIHARDWARE();
};

typedef IXHCIHardwareDevice *PXHCIHARDWAREDEVICE;

//
// IUSBRequest
//
struct _TRB;
struct _COMMAND_DESCRIPTOR;
#define DEFINE_ABSTRACT_USBXHCIREQUEST()                           \
    STDMETHOD_(VOID, CompletionCallback)(THIS_                     \
        IN NTSTATUS NtStatusCode,                                  \
        IN ULONG UrbStatusCode) PURE;                              \
                                                                   \
    STDMETHOD_(NTSTATUS, CreateCommandDescriptor)(THIS_            \
        IN struct _COMMAND_DESCRIPTOR **CommandDescriptor) PURE;   \
                                                                   \
    STDMETHOD_(NTSTATUS, GetCommandDescriptor)(THIS_               \
        IN struct _COMMAND_DESCRIPTOR **CommandDescriptor) PURE;   \
                                                                   \
    STDMETHOD_(NTSTATUS, FreeCommandDescriptor)(THIS_              \
        IN struct _COMMAND_DESCRIPTOR *CommandDescriptor) PURE;    \
                                                                   \
    STDMETHOD_(ULONG, GetRequestTarget)(THIS_                      \
        VOID) PURE;                                                \
                                                                   \
    STDMETHOD_(NTSTATUS, InitializeWithCommand)(THIS_              \
        IN PDMAMEMORYMANAGER DmaManager,                           \
        IN PCOMMAND_INFORMATION CommandInformation,                \
        IN PTRB OutTransferRequestBlock) PURE;                     \
                                                                   \
    STDMETHOD_(UCHAR, GetRequestDeviceAddress)(THIS_               \
        VOID) PURE;                                                \
                                                                   \
    STDMETHOD_(VOID, SetRequestDeviceInformation)(THIS_            \
        IN PDEVICE_INFORMATION DeviceInformation) PURE;

#define IMP_IXHCIREQUEST                                           \
    STDMETHODIMP_(VOID) CompletionCallback(                        \
        IN NTSTATUS NtStatusCode,                                  \
        IN ULONG UrbStatusCode);                                   \
                                                                   \
    STDMETHODIMP_(NTSTATUS) CreateCommandDescriptor(               \
        IN struct _COMMAND_DESCRIPTOR **CommandDescriptor);        \
                                                                   \
    STDMETHODIMP_(NTSTATUS) GetCommandDescriptor(                  \
        IN struct _COMMAND_DESCRIPTOR **CommandDescriptor);        \
                                                                   \
    STDMETHODIMP_(NTSTATUS) FreeCommandDescriptor(                 \
        IN struct _COMMAND_DESCRIPTOR *CommandDescriptor);         \
                                                                   \
    STDMETHODIMP_(ULONG) GetRequestTarget(                         \
        VOID);                                                     \
                                                                   \
    STDMETHODIMP_(NTSTATUS) InitializeWithCommand(                 \
        IN PDMAMEMORYMANAGER DmaManager,                           \
        IN PCOMMAND_INFORMATION CommandInformation,                \
        IN PTRB OutTransferRequestBlock);                          \
                                                                   \
    STDMETHODIMP_(UCHAR) GetRequestDeviceAddress(                  \
        VOID);                                                     \
                                                                   \
    STDMETHODIMP_(VOID) SetRequestDeviceInformation(               \
        IN PDEVICE_INFORMATION DeviceInformation);                 \


DECLARE_INTERFACE_(IXHCIRequest, IUSBRequest)
{
    DEFINE_ABSTRACT_UNKNOWN();
    DEFINE_ABSTRACT_USBREQUEST();
    DEFINE_ABSTRACT_USBXHCIREQUEST();
};

typedef IXHCIRequest *PXHCIREQUEST;

//
// IUSBQueue
//
#define DEFINE_ABSTRACT_USBXHCIQUEUE()                       \
    STDMETHOD_(VOID, CompleteCommandRequest)(THIS_           \
        IN PTRB TransferRequestBlock) PURE;                  \
                                                             \
    STDMETHOD_(VOID, CompleteTransferRequest)(THIS_          \
        IN PTRB TransferRequestBlock) PURE;                  \
                                                             \
    STDMETHOD_(BOOLEAN, IsEventRingEmpty)(THIS_) PURE;       \
                                                             \
    STDMETHOD_(PTRB, GetEventRingDequeuePointer)(THIS_) PURE;\
                                                             \
    STDMETHOD_(NTSTATUS, SubmitInternalCommand)(THIS_        \
        IN PDMAMEMORYMANAGER MemoryManager,                  \
        IN PCOMMAND_INFORMATION CommandInformation,          \
        IN PTRB OutTransferRequestBlock) PURE;

#define IMP_IXHCIQUEUE                                      \
    STDMETHODIMP_(VOID) CompleteCommandRequest(             \
        IN PTRB TransferRequestBlock);                      \
                                                            \
    STDMETHODIMP_(VOID) CompleteTransferRequest(            \
        IN PTRB TransferRequestBlock);                      \
                                                            \
    STDMETHODIMP_(BOOLEAN) IsEventRingEmpty(VOID);          \
                                                            \
    STDMETHODIMP_(PTRB) GetEventRingDequeuePointer(VOID);   \
                                                            \
    STDMETHODIMP_(NTSTATUS) SubmitInternalCommand(          \
        IN PDMAMEMORYMANAGER MemoryManager,                 \
        IN PCOMMAND_INFORMATION CommandInformation,         \
        IN PTRB OutTransferRequestBlock);                   \

DECLARE_INTERFACE_(IXHCIQueue, IUSBQueue)
{
    DEFINE_ABSTRACT_UNKNOWN();
    DEFINE_ABSTRACT_USBQUEUE();
    DEFINE_ABSTRACT_USBXHCIQUEUE();
};

typedef IXHCIQueue *PXHCIQUEUE;
#endif // INTERFACES_HPP