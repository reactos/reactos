#ifndef INTERFACES_HPP
#define INTERFACES_HPP

//---------------------------------------------------------------------------
//
//          Object Hierachy
//       --------------------------------------------------------------------
//       |  IRootHCDController                                              |
//       |    IHCDController Intel USB Universal Host Controller - 3A37     |
//       |    IHCDController - Intel USB Universal HostController - 3A38    |
//       |    IHCDController - Intel USB Universal HostController - 3A38    |
//       |------------------------------------------------------------------|
//
//      
//       IHCDController Intel USB Universal Host Controller - 3A37
//         IHubController
//         IUSBHardwareDevice
//           IDMAMemoryManager
//           IUSBQueue  <-      interacts with -> IUSBRequest
//            
//
//  Each IHCDController creates an IUSBHardwareDevice class upon initialization. The 
//  IUSBHardwardeDevice class is used to abstract usb controller specifics. The IHubController
//  manages all attached devices and handles hub control ioctl requests.
//
//  Each IUSBHardwareDevice has one IDMAMemoryManager and one IUSBQueue. The IDMAMemoryManager
//  is used to handle dma memory allocations. The IUSBQueue manages requests which are send to the
//  usb hardware. See IUSBRequest class for details.
//


struct _UHCI_QUEUE_HEAD;
struct IDMAMemoryManager;
struct IUSBQueue;

//=========================================================================================
//
// class IUSBHardwareDevice
//
// Description: This class provides access to the usb hardware controller
//

#define DEFINE_ABSTRACT_USBUHCIHARDWAREDEVICE()                             \
    STDMETHOD_(VOID, GetQueueHead)( THIS_                                   \
        IN ULONG QueueHeadIndex,                                            \
        IN struct _UHCI_QUEUE_HEAD **OutQueueHead) PURE;

#define IMP_IUHCIHARDWAREDEVICE                                             \
    STDMETHODIMP_(VOID) GetQueueHead(                                       \
        IN ULONG QueueHeadIndex,                                            \
        IN struct _UHCI_QUEUE_HEAD **OutQueueHead);

DECLARE_INTERFACE_(IUHCIHardwareDevice, IUSBHardwareDevice)
{
    DEFINE_ABSTRACT_UNKNOWN()
    DEFINE_ABSTRACT_USBHARDWAREDEVICE()
    DEFINE_ABSTRACT_USBUHCIHARDWAREDEVICE()
};

typedef IUHCIHardwareDevice *PUHCIHARDWAREDEVICE;

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



#define DEFINE_ABSTRACT_USBUHCIREQUEST()                                    \
    STDMETHOD_(NTSTATUS, GetEndpointDescriptor)( THIS_                      \
        IN struct _UHCI_QUEUE_HEAD**OutDescriptor) PURE;                    \
                                                                            \
    STDMETHOD_(UCHAR, GetInterval)( THIS) PURE;                             \
                                                                            \
    STDMETHOD_(USB_DEVICE_SPEED, GetDeviceSpeed)( THIS) PURE;               \
                                                                            \
    STDMETHOD_(VOID, CompletionCallback)( THIS) PURE;                       \
                                                                            \
    STDMETHOD_(VOID, FreeEndpointDescriptor)( THIS_                         \
        IN struct _UHCI_QUEUE_HEAD *OutDescriptor) PURE;


#define IMP_IUHCIREQUEST                                                    \
    STDMETHODIMP_(NTSTATUS) GetEndpointDescriptor(THIS_                     \
        IN struct _UHCI_QUEUE_HEAD**OutDescriptor);                         \
                                                                            \
    STDMETHODIMP_(UCHAR) GetInterval(THIS);                                 \
                                                                            \
    STDMETHODIMP_(USB_DEVICE_SPEED) GetDeviceSpeed(THIS);                   \
                                                                            \
    STDMETHODIMP_(VOID) CompletionCallback(THIS);                           \
                                                                            \
    STDMETHODIMP_(VOID) FreeEndpointDescriptor(THIS_                        \
        IN struct _UHCI_QUEUE_HEAD * OutDescriptor);

DECLARE_INTERFACE_(IUHCIRequest, IUSBRequest)
{
    DEFINE_ABSTRACT_UNKNOWN()
    DEFINE_ABSTRACT_USBREQUEST()
    DEFINE_ABSTRACT_USBUHCIREQUEST()
};


typedef IUHCIRequest *PUHCIREQUEST;

//=========================================================================================
//
// class IUSBQueue
//
// Description: This class manages pending requests
// 

#define DEFINE_ABSTRACT_USBUHCIQUEUE()                                      \
    STDMETHOD_(VOID, TransferInterrupt)(                                    \
        IN UCHAR ErrorInterrupt) PURE;

#define IMP_IUHCIQUEUE                                                      \
    STDMETHODIMP_(VOID) TransferInterrupt(                                  \
        IN UCHAR ErrorInterrupt);

DECLARE_INTERFACE_(IUHCIQueue, IUSBQueue)
{
    DEFINE_ABSTRACT_UNKNOWN()
    DEFINE_ABSTRACT_USBQUEUE()
    DEFINE_ABSTRACT_USBUHCIQUEUE()
};

typedef IUHCIQueue *PUHCIQUEUE;

#endif /* INTERFACES_HPP */
