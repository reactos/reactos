#ifndef INTERFACES_HPP
#define INTERFACES_HPP

struct _OHCI_ENDPOINT_DESCRIPTOR;
struct IDMAMemoryManager;
struct IUSBQueue;

//=========================================================================================
//
// class IUSBHardwareDevice
//
// Description: This class provides access to the usb hardware controller
//


#define DEFINE_ABSTRACT_USBOHCIHARDWARE()                                   \
    STDMETHOD_(VOID, GetBulkHeadEndpointDescriptor)( THIS_                  \
        IN struct _OHCI_ENDPOINT_DESCRIPTOR ** OutDescriptor) PURE;         \
                                                                            \
    STDMETHOD_(VOID, GetControlHeadEndpointDescriptor)( THIS_               \
        IN struct _OHCI_ENDPOINT_DESCRIPTOR ** OutDescriptor) PURE;         \
                                                                            \
    STDMETHOD_(VOID, GetIsochronousHeadEndpointDescriptor)( THIS_           \
        IN struct _OHCI_ENDPOINT_DESCRIPTOR ** OutDescriptor) PURE;         \
                                                                            \
    STDMETHOD_(VOID, GetInterruptEndpointDescriptors)( THIS_                \
        IN struct _OHCI_ENDPOINT_DESCRIPTOR *** OutDescriptor) PURE;        \
                                                                            \
    STDMETHOD_(VOID, HeadEndpointDescriptorModified)( THIS_                 \
        IN ULONG Type) PURE;                                                \
                                                                            \
    STDMETHOD_(VOID, GetCurrentFrameNumber)( THIS_                          \
        IN PULONG FrameNumber) PURE;

#define IMP_IUSBOHCIHARDWAREDEVICE                                          \
    STDMETHODIMP_(VOID) GetBulkHeadEndpointDescriptor(                      \
        IN struct _OHCI_ENDPOINT_DESCRIPTOR ** OutDescriptor);              \
                                                                            \
    STDMETHODIMP_(VOID) GetControlHeadEndpointDescriptor(                   \
        IN struct _OHCI_ENDPOINT_DESCRIPTOR ** OutDescriptor);              \
                                                                            \
    STDMETHODIMP_(VOID) GetIsochronousHeadEndpointDescriptor(               \
        IN struct _OHCI_ENDPOINT_DESCRIPTOR ** OutDescriptor);              \
                                                                            \
    STDMETHODIMP_(VOID) GetInterruptEndpointDescriptors(                    \
        IN struct _OHCI_ENDPOINT_DESCRIPTOR *** OutDescriptor);             \
                                                                            \
    STDMETHODIMP_(VOID) HeadEndpointDescriptorModified(                     \
        IN ULONG Type);                                                     \
                                                                            \
    STDMETHODIMP_(VOID) GetCurrentFrameNumber(                              \
        OUT PULONG FrameNumber);

DECLARE_INTERFACE_(IOHCIHardwareDevice, IUSBHardwareDevice)
{
    DEFINE_ABSTRACT_UNKNOWN()
    DEFINE_ABSTRACT_USBHARDWAREDEVICE()
    DEFINE_ABSTRACT_USBOHCIHARDWARE()
};

typedef IOHCIHardwareDevice *POHCIHARDWAREDEVICE;


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


#define DEFINE_ABSTRACT_USBOHCIREQUEST()                                    \
    STDMETHOD_(NTSTATUS, GetEndpointDescriptor)( THIS_                      \
        IN struct _OHCI_ENDPOINT_DESCRIPTOR ** OutDescriptor) PURE;         \
                                                                            \
    STDMETHOD_(VOID, CompletionCallback)( THIS) PURE;                       \
                                                                            \
    STDMETHOD_(VOID, FreeEndpointDescriptor)( THIS_                         \
        IN struct _OHCI_ENDPOINT_DESCRIPTOR * OutDescriptor) PURE;          \
                                                                            \
    STDMETHOD_(UCHAR, GetInterval)( THIS) PURE;


#define IMP_IOHCIREQUEST                                                    \
    STDMETHODIMP_(NTSTATUS) GetEndpointDescriptor(                          \
        IN struct _OHCI_ENDPOINT_DESCRIPTOR ** OutDescriptor);              \
                                                                            \
    STDMETHODIMP_(VOID) CompletionCallback();                               \
                                                                            \
    STDMETHODIMP_(VOID) FreeEndpointDescriptor(                             \
        IN struct _OHCI_ENDPOINT_DESCRIPTOR * OutDescriptor);               \
                                                                            \
    STDMETHODIMP_(UCHAR) GetInterval();

DECLARE_INTERFACE_(IOHCIRequest, IUSBRequest)
{
    DEFINE_ABSTRACT_UNKNOWN()
    DEFINE_ABSTRACT_USBREQUEST()
    DEFINE_ABSTRACT_USBOHCIREQUEST()
};

typedef IOHCIRequest *POHCIREQUEST;

//=========================================================================================
//
// class IUSBQueue
//
// Description: This class manages pending requests
// 

#define DEFINE_ABSTRACT_USBOHCIQUEUE()                                      \
    STDMETHOD_(VOID, TransferDescriptorCompletionCallback)( THIS_           \
        IN ULONG TransferDescriptorLogicalAddress) PURE;

#define IMP_IUSBOHCIQUEUE                                                   \
    STDMETHODIMP_(VOID) TransferDescriptorCompletionCallback(               \
        IN ULONG TransferDescriptorLogicalAddress);

DECLARE_INTERFACE_(IOHCIQueue, IUSBQueue)
{
    DEFINE_ABSTRACT_UNKNOWN()
    DEFINE_ABSTRACT_USBQUEUE()
    DEFINE_ABSTRACT_USBOHCIQUEUE()
};

typedef IOHCIQueue *POHCIQUEUE;

#endif /* INTERFACES_HPP */
