#ifndef INTERFACES_HPP
#define INTERFACES_HPP

//=========================================================================================
//
// class IUSBHardwareDevice
//
// Description: This class provides access to the usb hardware controller
//

#define DEFINE_ABSTRACT_USBEHCIHARDWARE()                                   \
    STDMETHOD_(VOID, SetAsyncListRegister)( THIS_                           \
        IN ULONG PhysicalAddress) PURE;                                     \
                                                                            \
    STDMETHOD_(VOID, SetPeriodicListRegister)( THIS_                        \
        IN ULONG PhysicalAddress) PURE;                                     \
                                                                            \
    STDMETHOD_(struct _QUEUE_HEAD *, GetAsyncListQueueHead)( THIS) PURE;    \
                                                                            \
    STDMETHOD_(ULONG, GetPeriodicListRegister)( THIS) PURE;                 \
                                                                            \
    STDMETHOD_(VOID, SetCommandRegister)( THIS_                             \
        IN struct _EHCI_USBCMD_CONTENT *UsbCmd) PURE;                       \
                                                                            \
    STDMETHOD_(VOID, GetCommandRegister)( THIS_                             \
        OUT struct _EHCI_USBCMD_CONTENT *UsbCmd) PURE;

#define IMP_IUSBEHCIHARDWARE                                                \
    STDMETHODIMP_(VOID) SetAsyncListRegister(                               \
        IN ULONG PhysicalAddress);                                          \
                                                                            \
    STDMETHODIMP_(VOID) SetPeriodicListRegister(                            \
        IN ULONG PhysicalAddress);                                          \
                                                                            \
    STDMETHODIMP_(struct _QUEUE_HEAD *) GetAsyncListQueueHead();            \
                                                                            \
    STDMETHODIMP_(ULONG) GetPeriodicListRegister();                         \
                                                                            \
    STDMETHODIMP_(VOID) SetCommandRegister(                                 \
        IN struct _EHCI_USBCMD_CONTENT *UsbCmd);                            \
    STDMETHODIMP_(VOID) GetCommandRegister(                                 \
        OUT struct _EHCI_USBCMD_CONTENT *UsbCmd);

DECLARE_INTERFACE_(IEHCIHardwareDevice, IUSBHardwareDevice)
{
    DEFINE_ABSTRACT_UNKNOWN()
    DEFINE_ABSTRACT_USBHARDWAREDEVICE()
    DEFINE_ABSTRACT_USBEHCIHARDWARE()
};

typedef IEHCIHardwareDevice *PEHCIHARDWAREDEVICE;

//=========================================================================================
//
// class IUSBRequest
//
// Description: This class is used to issue request to usb controller. The class is 
// initialized using InitializeXXX methods. You also need to call SetEndpoint to define the endpoint
// In addition you can call SetCompletionDetails if you need to wait for the end of
// the request or want to complete an irp. You call AddUSBRequest to add the request to the queue. 
// Once the request is completed the CompletionCallback is invoked. The CompletionCallback
// will take care of any completion details which have been set. If the request is cancelled, the 
// CancelCallback routine is invoked.
// 

struct _QUEUE_HEAD;
struct _USB_ENDPOINT;

#define DEFINE_ABSTRACT_USBEHCIREQUEST()                                    \
    STDMETHOD_(VOID, CompletionCallback)( THIS_                             \
        IN NTSTATUS NtStatusCode,                                           \
        IN ULONG UrbStatusCode,                                             \
        IN struct _QUEUE_HEAD *QueueHead) PURE;                             \
                                                                            \
    STDMETHOD_(NTSTATUS, GetQueueHead)( THIS_                               \
        IN struct _QUEUE_HEAD ** OutHead) PURE;                             \
                                                                            \
    STDMETHOD_(BOOLEAN, ShouldReleaseRequestAfterCompletion)( THIS) PURE;   \
                                                                            \
                                                                            \
    STDMETHOD_(VOID, FreeQueueHead)( THIS_                                  \
        IN struct _QUEUE_HEAD * QueueHead) PURE;                            \
                                                                            \
    STDMETHOD_(BOOLEAN, IsQueueHeadComplete)( THIS_                         \
        IN struct _QUEUE_HEAD * QueueHead) PURE;                            \
                                                                            \
    STDMETHOD_(USB_DEVICE_SPEED, GetSpeed)( THIS) PURE;                     \
                                                                            \
    STDMETHOD_(UCHAR, GetInterval)( THIS) PURE;

#define IMP_IEHCIREQUEST                                                    \
    STDMETHODIMP_(VOID) CompletionCallback(                                 \
        IN NTSTATUS NtStatusCode,                                           \
        IN ULONG UrbStatusCode,                                             \
        IN struct _QUEUE_HEAD *QueueHead);                                  \
                                                                            \
    STDMETHODIMP_(NTSTATUS) GetQueueHead(                                   \
        IN struct _QUEUE_HEAD ** OutHead);                                  \
                                                                            \
    STDMETHODIMP_(BOOLEAN) ShouldReleaseRequestAfterCompletion();           \
                                                                            \
    STDMETHODIMP_(VOID) FreeQueueHead(struct _QUEUE_HEAD * QueueHead);      \
                                                                            \
    STDMETHODIMP_(BOOLEAN) IsQueueHeadComplete(                             \
        IN struct _QUEUE_HEAD * QueueHead);                                 \
                                                                            \
    STDMETHODIMP_(USB_DEVICE_SPEED) GetSpeed( THIS);                        \
                                                                            \
    STDMETHODIMP_(UCHAR) GetInterval( THIS);

DECLARE_INTERFACE_(IEHCIRequest, IUSBRequest)
{
    DEFINE_ABSTRACT_UNKNOWN()
    DEFINE_ABSTRACT_USBREQUEST()
    DEFINE_ABSTRACT_USBEHCIREQUEST()
};


typedef IEHCIRequest *PEHCIREQUEST;

//=========================================================================================
//
// class IUSBQueue
//
// Description: This class manages pending requests
// 

#define DEFINE_ABSTRACT_USBEHCIQUEUE()                                      \
    STDMETHOD_(VOID, InterruptCallback)( THIS_                              \
        IN NTSTATUS Status,                                                 \
        OUT PULONG ShouldRingDoorBell) PURE;                                \
                                                                            \
    STDMETHOD_(VOID, CompleteAsyncRequests)( THIS) PURE;

#define IMP_IEHCIQUEUE                                                      \
    STDMETHODIMP_(VOID) InterruptCallback(                                  \
        IN NTSTATUS Status,                                                 \
        OUT PULONG ShouldRingDoorBell);                                     \
                                                                            \
    STDMETHODIMP_(VOID) CompleteAsyncRequests();

DECLARE_INTERFACE_(IEHCIQueue, IUSBQueue)
{
    DEFINE_ABSTRACT_UNKNOWN()
    DEFINE_ABSTRACT_USBQUEUE()
    DEFINE_ABSTRACT_USBEHCIQUEUE()
};

typedef IEHCIQueue *PEHCIQUEUE;

#endif /* INTERFACES_HPP */
