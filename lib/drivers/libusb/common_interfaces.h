
#ifndef COMMON_INTERFACES_HPP
#define COMMON_INTERFACES_HPP

typedef struct _USB_ENDPOINT
{
    USB_ENDPOINT_DESCRIPTOR EndPointDescriptor;
    UCHAR HubAddress;
    UCHAR HubPort;
    UCHAR DataToggle;
} USB_ENDPOINT, *PUSB_ENDPOINT;

typedef struct _USB_INTERFACE
{
    LIST_ENTRY ListEntry;
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    USB_ENDPOINT EndPoints[1];
} USB_INTERFACE, *PUSB_INTERFACE;

typedef struct
{
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor;
    LIST_ENTRY InterfaceList;
}USB_CONFIGURATION, *PUSB_CONFIGURATION;

//---------------------------------------------------------------------------
//
//          Object Hierarchy
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


//=========================================================================================
//
// class IRootHCDController
//
// Description: This class serves as the root host controller. The host controller mantains
// a list of registered controllers and provides support functions for the host controllers

struct IHCDController;

DECLARE_INTERFACE_(IRootHCDController, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

//-----------------------------------------------------------------------------------------
//
// Initialize
//
// Description: This function initializes the root host controller. It allocates the resources
// required to manage the registered controllers

    virtual NTSTATUS Initialize() = 0;

//-----------------------------------------------------------------------------------------
//
// RegisterHCD
//
// Description: this function registers a host controller with the root host controller

    virtual NTSTATUS RegisterHCD(struct IHCDController * Controller) = 0;

//-----------------------------------------------------------------------------------------
//
// UnregisterHCD
//
// Description: this function unregistes a host controller

    virtual NTSTATUS UnregisterHCD(struct IHCDController * Controller) = 0;

//-----------------------------------------------------------------------------------------
//
// GetControllerCount
//
// Description: returns the number of host controllers registered

    virtual ULONG GetControllerCount() = 0;

};

typedef IRootHCDController *PROOTHDCCONTROLLER;

//=========================================================================================
//
// class IHCDController
//
// Description: This class is used to manage a single USB host controller
//

DECLARE_INTERFACE_(IHCDController, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

//-----------------------------------------------------------------------------------------
//
// Initialize
//
// Description: This function initializes the IHCDController implementation. 
// It creates an IUSBHardwareDevice object and initializes it. It also registeres itself with
// the IRootHCDController
//
    virtual NTSTATUS Initialize(IN PROOTHDCCONTROLLER RootHCDController,
                                IN PDRIVER_OBJECT DriverObject,
                                IN PDEVICE_OBJECT PhysicalDeviceObject) = 0;

};

typedef IHCDController *PHCDCONTROLLER;


//=========================================================================================
//
// class IUSBHardwareDevice
//
// Description: This class provides access to the usb hardware controller
//

struct IDMAMemoryManager;
struct IUSBQueue;

#define DEFINE_ABSTRACT_USBHARDWAREDEVICE()                                 \
    STDMETHOD_(NTSTATUS, Initialize)( THIS_                                 \
        IN PDRIVER_OBJECT DriverObject,                                     \
        IN PDEVICE_OBJECT FunctionalDeviceObject,                           \
        IN PDEVICE_OBJECT PhysicalDeviceObject,                             \
        IN PDEVICE_OBJECT LowerDeviceObject) PURE;                          \
                                                                            \
     STDMETHOD_(NTSTATUS, PnpStart)( THIS_                                  \
        IN PCM_RESOURCE_LIST RawResources,                                  \
        IN PCM_RESOURCE_LIST TranslatedResources) PURE;                     \
                                                                            \
     STDMETHOD_(NTSTATUS, PnpStop)( THIS) PURE;                             \
                                                                            \
     STDMETHOD_(NTSTATUS, GetDeviceDetails)( THIS_                          \
        OUT OPTIONAL PUSHORT VendorId,                                      \
        OUT OPTIONAL PUSHORT DeviceId,                                      \
        OUT OPTIONAL PULONG NumberOfPorts,                                  \
        OUT OPTIONAL PULONG Speed) PURE;                                    \
                                                                            \
     STDMETHOD_(NTSTATUS, GetUSBQueue)( THIS_                               \
        OUT struct IUSBQueue **OutUsbQueue) PURE;                           \
                                                                            \
     STDMETHOD_(NTSTATUS, GetDMA)( THIS_                                    \
        OUT struct IDMAMemoryManager **OutDMA) PURE;                        \
                                                                            \
     STDMETHOD_(NTSTATUS, ResetPort)( THIS_                                 \
        IN ULONG PortNumber) PURE;                                          \
                                                                            \
     STDMETHOD_(NTSTATUS, GetPortStatus)( THIS_                             \
        IN ULONG PortId,                                                    \
        OUT USHORT *PortStatus,                                             \
        OUT USHORT *PortChange) PURE;                                       \
                                                                            \
     STDMETHOD_(NTSTATUS, ClearPortStatus)( THIS_                           \
        IN ULONG PortId,                                                    \
        IN ULONG Status) PURE;                                              \
                                                                            \
     STDMETHOD_(NTSTATUS, SetPortFeature)( THIS_                            \
        IN ULONG PortId,                                                    \
        IN ULONG Feature) PURE;                                             \
                                                                            \
     STDMETHOD_(VOID, SetStatusChangeEndpointCallBack)( THIS_               \
        IN PVOID CallBack,                                                  \
        IN PVOID Context) PURE;                                             \
                                                                            \
     STDMETHOD_(LPCSTR, GetUSBType)(THIS) PURE;


#define IMP_IUSBHARDWAREDEVICE                                              \
    STDMETHODIMP_(NTSTATUS) Initialize(                                     \
        IN PDRIVER_OBJECT DriverObject,                                     \
        IN PDEVICE_OBJECT FunctionalDeviceObject,                           \
        IN PDEVICE_OBJECT PhysicalDeviceObject,                             \
        IN PDEVICE_OBJECT LowerDeviceObject);                               \
                                                                            \
     STDMETHODIMP_(NTSTATUS) PnpStart(                                      \
        IN PCM_RESOURCE_LIST RawResources,                                  \
        IN PCM_RESOURCE_LIST TranslatedResources);                          \
                                                                            \
     STDMETHODIMP_(NTSTATUS) PnpStop(VOID);                                 \
                                                                            \
     STDMETHODIMP_(NTSTATUS) GetDeviceDetails(                              \
        OUT OPTIONAL PUSHORT VendorId,                                      \
        OUT OPTIONAL PUSHORT DeviceId,                                      \
        OUT OPTIONAL PULONG NumberOfPorts,                                  \
        OUT OPTIONAL PULONG Speed);                                         \
                                                                            \
     STDMETHODIMP_(NTSTATUS) GetUSBQueue(                                   \
        OUT struct IUSBQueue **OutUsbQueue);                                \
                                                                            \
     STDMETHODIMP_(NTSTATUS) GetDMA(                                        \
        OUT struct IDMAMemoryManager **OutDMA);                             \
                                                                            \
     STDMETHODIMP_(NTSTATUS) ResetPort(                                     \
        IN ULONG PortNumber);                                               \
                                                                            \
     STDMETHODIMP_(NTSTATUS) GetPortStatus(                                 \
        IN ULONG PortId,                                                    \
        OUT USHORT *PortStatus,                                             \
        OUT USHORT *PortChange);                                            \
                                                                            \
     STDMETHODIMP_(NTSTATUS) ClearPortStatus(                               \
        IN ULONG PortId,                                                    \
        IN ULONG Status);                                                   \
                                                                            \
     STDMETHODIMP_(NTSTATUS) SetPortFeature(                                \
        IN ULONG PortId,                                                    \
        IN ULONG Feature);                                                  \
                                                                            \
     STDMETHODIMP_(VOID) SetStatusChangeEndpointCallBack(                   \
        IN PVOID CallBack,                                                  \
        IN PVOID Context);                                                  \
                                                                            \
     STDMETHODIMP_(LPCSTR) GetUSBType();

DECLARE_INTERFACE_(IUSBHardwareDevice, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()
    DEFINE_ABSTRACT_USBHARDWAREDEVICE()
};

typedef IUSBHardwareDevice *PUSBHARDWAREDEVICE;


//=========================================================================================
//
// class IDMAMemoryManager
//
// Description: This class provides access to the dma buffer. It provides methods to 
// allocate and free from the dma buffer
// 

DECLARE_INTERFACE_(IDMAMemoryManager, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

//-----------------------------------------------------------------------------------------
//
// Initialize
//
// Description: initializes the memory manager

    virtual NTSTATUS Initialize(IN PUSBHARDWAREDEVICE Device,
                                IN PKSPIN_LOCK Lock,
                                IN ULONG DmaBufferSize,
                                IN PVOID VirtualBase,
                                IN PHYSICAL_ADDRESS PhysicalAddress,
                                IN ULONG DefaultBlockSize) = 0;

//-----------------------------------------------------------------------------------------
//
// Allocate
//
// Description: allocates block of memory from allocator

    virtual NTSTATUS Allocate(IN ULONG Size,
                              OUT PVOID *OutVirtualBase,
                              OUT PPHYSICAL_ADDRESS OutPhysicalAddress) = 0;


//-----------------------------------------------------------------------------------------
//
// Free
//
// Description: releases memory block

    virtual NTSTATUS Release(IN PVOID VirtualBase,
                             IN ULONG Size) = 0;

};

typedef IDMAMemoryManager *PDMAMEMORYMANAGER;


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

struct IUSBDevice;

#define DEFINE_ABSTRACT_USBREQUEST()                                        \
    STDMETHOD_(NTSTATUS, InitializeWithSetupPacket)( THIS_                  \
        IN PDMAMEMORYMANAGER DmaManager,                                    \
        IN PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket,                      \
        IN struct IUSBDevice *Device,                                       \
        IN OPTIONAL struct _USB_ENDPOINT *EndpointDescriptor,               \
        IN OUT ULONG TransferBufferLength,                                  \
        IN OUT PMDL TransferBuffer) PURE;                                   \
                                                                            \
    STDMETHOD_(NTSTATUS, InitializeWithIrp)( THIS_                          \
        IN PDMAMEMORYMANAGER DmaManager,                                    \
        IN struct IUSBDevice *Device,                                       \
        IN OUT PIRP Irp) PURE;                                              \
                                                                            \
    STDMETHOD_(BOOLEAN, IsRequestComplete)( THIS) PURE;                     \
                                                                            \
    STDMETHOD_(ULONG, GetTransferType)( THIS) PURE;                         \
                                                                            \
    STDMETHOD_(VOID, GetResultStatus)( THIS_                                \
        OUT OPTIONAL NTSTATUS * NtStatusCode,                               \
        OUT OPTIONAL PULONG UrbStatusCode) PURE;

#define IMP_IUSBREQUEST                                                     \
    STDMETHODIMP_(NTSTATUS) InitializeWithSetupPacket(                      \
        IN PDMAMEMORYMANAGER DmaManager,                                    \
        IN PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket,                      \
        IN struct IUSBDevice *Device,                                       \
        IN OPTIONAL struct _USB_ENDPOINT *EndpointDescriptor,               \
        IN OUT ULONG TransferBufferLength,                                  \
        IN OUT PMDL TransferBuffer);                                        \
                                                                            \
    STDMETHODIMP_(NTSTATUS) InitializeWithIrp(                              \
        IN PDMAMEMORYMANAGER DmaManager,                                    \
        IN struct IUSBDevice *Device,                                       \
        IN OUT PIRP Irp);                                                   \
                                                                            \
    STDMETHODIMP_(BOOLEAN) IsRequestComplete(VOID);                         \
                                                                            \
    STDMETHODIMP_(ULONG) GetTransferType(VOID);                             \
                                                                            \
    STDMETHODIMP_(VOID) GetResultStatus(                                    \
        OUT OPTIONAL NTSTATUS * NtStatusCode,                               \
        OUT OPTIONAL PULONG UrbStatusCode);

DECLARE_INTERFACE_(IUSBRequest, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()
    DEFINE_ABSTRACT_USBREQUEST()
};


typedef IUSBRequest *PUSBREQUEST;

//=========================================================================================
//
// class IUSBQueue
//
// Description: This class manages pending requests
// 

#define DEFINE_ABSTRACT_USBQUEUE()                                          \
    STDMETHOD_(NTSTATUS, Initialize)( THIS_                                 \
        IN PUSBHARDWAREDEVICE Hardware,                                     \
        IN PDMA_ADAPTER AdapterObject,                                      \
        IN PDMAMEMORYMANAGER MemManager,                                    \
        IN OPTIONAL PKSPIN_LOCK Lock) PURE;                                 \
                                                                            \
    STDMETHOD_(NTSTATUS, AddUSBRequest)( THIS_                              \
        IN IUSBRequest * Request) PURE;                                     \
                                                                            \
    STDMETHOD_(NTSTATUS, CreateUSBRequest)( THIS_                           \
        IN IUSBRequest **OutRequest) PURE;                                  \
                                                                            \
    STDMETHOD_(NTSTATUS, AbortDevicePipe)( THIS_                            \
        IN UCHAR DeviceAddress,                                             \
        IN PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor) PURE;

#define IMP_IUSBQUEUE                                                       \
    STDMETHODIMP_(NTSTATUS) Initialize(                                     \
        IN PUSBHARDWAREDEVICE Hardware,                                     \
        IN PDMA_ADAPTER AdapterObject,                                      \
        IN PDMAMEMORYMANAGER MemManager,                                    \
        IN OPTIONAL PKSPIN_LOCK Lock);                                      \
                                                                            \
    STDMETHODIMP_(NTSTATUS) AddUSBRequest(                                  \
        IN IUSBRequest * Request);                                          \
                                                                            \
    STDMETHODIMP_(NTSTATUS) CreateUSBRequest(                               \
        OUT IUSBRequest **OutRequest);                                      \
                                                                            \
    STDMETHODIMP_(NTSTATUS) AbortDevicePipe(                                \
        IN UCHAR DeviceAddress,                                             \
        IN PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor);

DECLARE_INTERFACE_(IUSBQueue, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()
    DEFINE_ABSTRACT_USBQUEUE()
};

typedef IUSBQueue *PUSBQUEUE;

//=========================================================================================
//
// class IHubController
//
// Description: This class implements a hub controller
// 

DECLARE_INTERFACE_(IHubController, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

//----------------------------------------------------------------------------------------
//
// Initialize
//
// Description: Initializes the hub controller

    virtual NTSTATUS Initialize(IN PDRIVER_OBJECT DriverObject,
                                IN PHCDCONTROLLER Controller,
                                IN PUSBHARDWAREDEVICE Device,
                                IN BOOLEAN IsRootHubDevice,
                                IN ULONG DeviceAddress) = 0;

//----------------------------------------------------------------------------------------
//
// GetHubControllerDeviceObject
//
// Description: Returns the hub controller device object

    virtual NTSTATUS GetHubControllerDeviceObject(PDEVICE_OBJECT * HubDeviceObject) = 0;

//----------------------------------------------------------------------------------------
//
// GetHubControllerSymbolicLink
//
// Description: Returns the symbolic link of the root hub

    virtual NTSTATUS GetHubControllerSymbolicLink(ULONG BufferLength, PVOID Buffer, PULONG RequiredLength) = 0;


};

typedef IHubController *PHUBCONTROLLER;

//=========================================================================================
//
// class IDispatchIrp
//
// Description: This class is used to handle irp dispatch requests
// 

DECLARE_INTERFACE_(IDispatchIrp, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

//-----------------------------------------------------------------------------------------
//
// HandlePnp
//
// Description: This function handles all pnp requests

    virtual NTSTATUS HandlePnp(IN PDEVICE_OBJECT DeviceObject,
                               IN OUT PIRP Irp) = 0;

//-----------------------------------------------------------------------------------------
//
// HandlePower
//
// Description: This function handles all power pnp requests
//
    virtual NTSTATUS HandlePower(IN PDEVICE_OBJECT DeviceObject,
                                 IN OUT PIRP Irp) = 0;

//-----------------------------------------------------------------------------------------
//
// HandleDeviceControl
//
// Description: handles device io control requests

    virtual NTSTATUS HandleDeviceControl(IN PDEVICE_OBJECT DeviceObject,
                                         IN OUT PIRP Irp) = 0;

//-----------------------------------------------------------------------------------------
//
// HandleSystemControl
//
// Description: handles WMI system control requests

    virtual NTSTATUS HandleSystemControl(IN PDEVICE_OBJECT DeviceObject,
                                         IN OUT PIRP Irp) = 0;
};

typedef IDispatchIrp *PDISPATCHIRP;

//=========================================================================================
//
// class IUSBDevice
//
// Description: This class is used to abstract details of a usb device
// 

DECLARE_INTERFACE_(IUSBDevice, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

//----------------------------------------------------------------------------------------
//
// Initialize
//
// Description: Initializes the usb device

    virtual NTSTATUS Initialize(IN PHUBCONTROLLER HubController,
                                IN PUSBHARDWAREDEVICE Device,
                                IN PVOID Parent,
                                IN ULONG Port,
                                IN ULONG PortStatus) = 0;

//-----------------------------------------------------------------------------------------
//
// IsHub
//
// Description: returns true when device is a hub

    virtual BOOLEAN IsHub() = 0;

//-----------------------------------------------------------------------------------------
//
// GetParent
//
// Description: gets the parent device of the this device

    virtual NTSTATUS GetParent(PVOID * Parent) = 0;

//-----------------------------------------------------------------------------------------
//
// GetDeviceAddress
//
// Description: gets the device address of the this device

    virtual UCHAR GetDeviceAddress() = 0;


//-----------------------------------------------------------------------------------------
//
// GetPort
//
// Description: gets the port to which this device is connected

    virtual ULONG GetPort() = 0;

//-----------------------------------------------------------------------------------------
//
// GetSpeed
//
// Description: gets the speed of the device

    virtual USB_DEVICE_SPEED GetSpeed() = 0;

//-----------------------------------------------------------------------------------------
//
// GetType
//
// Description: gets the type of the device, either 1.1 or 2.0 device

    virtual USB_DEVICE_TYPE GetType() = 0;

//-----------------------------------------------------------------------------------------
//
// GetState
//
// Description: gets the device state

    virtual ULONG GetState() = 0;

//-----------------------------------------------------------------------------------------
//
// SetDeviceHandleData
//
// Description: sets device handle data

    virtual void SetDeviceHandleData(PVOID Data) = 0;

//-----------------------------------------------------------------------------------------
//
// SetDeviceAddress
//
// Description: sets device handle data

    virtual NTSTATUS SetDeviceAddress(UCHAR DeviceAddress) = 0;

//-----------------------------------------------------------------------------------------
//
// GetDeviceDescriptor
//
// Description: sets device handle data

    virtual void GetDeviceDescriptor(PUSB_DEVICE_DESCRIPTOR DeviceDescriptor) = 0;

//-----------------------------------------------------------------------------------------
//
// GetConfigurationValue
//
// Description: gets current selected configuration index

   virtual UCHAR GetConfigurationValue() = 0;

//-----------------------------------------------------------------------------------------
//
// SubmitIrp
//
// Description: submits an irp containing an urb

    virtual NTSTATUS SubmitIrp(PIRP Irp) = 0;

//-----------------------------------------------------------------------------------------
//
// GetConfigurationDescriptors
//
// Description: returns one or more configuration descriptors

    virtual VOID GetConfigurationDescriptors(IN PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptorBuffer,
                                             IN ULONG BufferLength,
                                             OUT PULONG OutBufferLength) = 0;

//-----------------------------------------------------------------------------------------
//
// Description: returns length of configuration descriptors
//
     virtual ULONG GetConfigurationDescriptorsLength() = 0;

//-----------------------------------------------------------------------------------------
//
// SubmitSetupPacket
//
// Description: submits an setup packet. The usb device will then create an usb request from it and submit it to the queue

     virtual NTSTATUS SubmitSetupPacket(IN PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket,
                                        IN OUT ULONG BufferLength,
                                        OUT PVOID Buffer) = 0;

//-----------------------------------------------------------------------------------------
//
// SelectConfiguration
//
// Description: selects a configuration

    virtual NTSTATUS SelectConfiguration(IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
                                         IN PUSBD_INTERFACE_INFORMATION Interface,
                                         OUT USBD_CONFIGURATION_HANDLE *ConfigurationHandle) = 0;

//-----------------------------------------------------------------------------------------
//
// SelectConfiguration
//
// Description: selects a interface of an configuration

    virtual NTSTATUS SelectInterface(IN USBD_CONFIGURATION_HANDLE ConfigurationHandle,
                                     IN OUT PUSBD_INTERFACE_INFORMATION Interface) = 0;

//-----------------------------------------------------------------------------------------
//
// AbortPipe
//
// Description: aborts all pending requests

    virtual NTSTATUS AbortPipe(IN PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor) = 0;

//-----------------------------------------------------------------------------------------
//
// GetMaxPacketSize
//
// Description: aborts all pending requests

    virtual UCHAR GetMaxPacketSize() = 0;
};

typedef IUSBDevice *PUSBDEVICE;

#endif
