
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

DECLARE_INTERFACE_(IUSBHardwareDevice, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

//-----------------------------------------------------------------------------------------
//
// Initialize
//
// Description: Initializes the usb device controller

    virtual NTSTATUS Initialize(PDRIVER_OBJECT DriverObject,
                                PDEVICE_OBJECT FunctionalDeviceObject,
                                PDEVICE_OBJECT PhysicalDeviceObject,
                                PDEVICE_OBJECT LowerDeviceObject) = 0;

//-----------------------------------------------------------------------------------------
//
// PnpStart
//
// Description: handles pnp start request from device. It registeres the interrupt, 
// sets up the ports and prepares the device. It then starts the controller

   virtual NTSTATUS PnpStart(PCM_RESOURCE_LIST RawResources,
                             PCM_RESOURCE_LIST TranslatedResources) = 0;

//-----------------------------------------------------------------------------------------
//
// PnpStop
//
// Description: handles pnp stop request from device. It unregisteres the interrupt, releases ports and dma object.

    virtual NTSTATUS PnpStop(void) = 0;

//-----------------------------------------------------------------------------------------
//
// GetDeviceDetails
//
// Description: returns the device details such as vendor id, device id, number of ports and speed

    virtual NTSTATUS GetDeviceDetails(OUT OPTIONAL PUSHORT VendorId,
                                      OUT OPTIONAL PUSHORT DeviceId,
                                      OUT OPTIONAL PULONG NumberOfPorts,
                                      OUT OPTIONAL PULONG Speed) = 0;

//-----------------------------------------------------------------------------------------
//
// GetUSBQueue
//
// Description: returns interface to internal IUSBQueue
// Interface is reference counted, you need to call release method when you are done with it
// Do not call Initialize on IUSBQueue, the object is already initialized

    virtual NTSTATUS GetUSBQueue(OUT struct IUSBQueue **OutUsbQueue) = 0;

//-----------------------------------------------------------------------------------------
//
// GetDMA
//
// Description: returns the DMA object which can be used to allocate memory from the common buffer

    virtual NTSTATUS GetDMA(OUT struct IDMAMemoryManager **OutDMAMemoryManager) = 0;


//-----------------------------------------------------------------------------------------
//
// ResetController()
//
// Description: this function resets the controller
// Returns STATUS_SUCCESS when the controller was successfully reset

   virtual NTSTATUS ResetController() = 0;

//-----------------------------------------------------------------------------------------
//
// StartController
//
// Description: this functions starts controller allowing interrupts for device connects/removal, and execution of
// Periodic and Asynchronous Schedules. 
//

    virtual NTSTATUS StartController() = 0;

//-----------------------------------------------------------------------------------------
//
// StopController
//
// Description: this functions stops controller disabling interrupts for device connects/removal, and execution of
// Periodic and Asynchronous Schedules. 
//

    virtual NTSTATUS StopController() = 0;

//-----------------------------------------------------------------------------------------
//
// ResetPort
//
// Description: this functions resets the port on the controller
//

    virtual NTSTATUS ResetPort(ULONG PortNumber) = 0;

//-----------------------------------------------------------------------------------------
//
// GetPortStatus
//
// Description: this functions return status and change state of port
//
    virtual NTSTATUS GetPortStatus(ULONG PortId, OUT USHORT *PortStatus, OUT USHORT *PortChange) = 0;

//-----------------------------------------------------------------------------------------
//
// ClearPortStatus
//
// Description: Clears Status of Port, for example Connection, Enable and Reset
//
    virtual NTSTATUS ClearPortStatus(ULONG PortId, ULONG Status) = 0;

//-----------------------------------------------------------------------------------------
//
// SetPortFeature
//
// Description: this functions Sets Feature on Port, for example Enable, Power and Reset
//
    virtual NTSTATUS SetPortFeature(ULONG PortId, ULONG Feature) = 0;

//-----------------------------------------------------------------------------------------
//
// SetAsyncListRegister
//
// Description: this functions sets the register to a address that is the physical address of a QueueHead.
// This is the location at which the controller will start executing the Asynchronous Schedule.
//
// FIXME: This is only available for USB 2.0
    virtual VOID SetAsyncListRegister(ULONG PhysicalAddress) = 0;

//-----------------------------------------------------------------------------------------
//
// SetPeriodicListRegister
//
// Description: this functions sets the register to a address that is the physical address of a ???.
// This is the location at which the controller will start executing the Periodic Schedule.
//
    virtual VOID SetPeriodicListRegister(ULONG PhysicalAddress) = 0;

//-----------------------------------------------------------------------------------------
//
// GetAsyncListRegister
//
// Description: Returns the memory address used in the Asynchronous Register
//
    virtual struct _QUEUE_HEAD * GetAsyncListQueueHead() = 0;

//-----------------------------------------------------------------------------------------
//
// GetPeriodicListRegister
//
// Description: Returns the the memory address used in the Periodic Register
//
    virtual ULONG GetPeriodicListRegister() = 0;

//-----------------------------------------------------------------------------------------
//
// SetStatusChangeEndpointCallBack
//
// Description: Used to callback to the hub controller when SCE detected
//
    virtual VOID SetStatusChangeEndpointCallBack(PVOID CallBack,PVOID Context) = 0;

//-----------------------------------------------------------------------------------------
//
// AcquireDeviceLock
//
// Description: acquires the device lock

    virtual KIRQL AcquireDeviceLock(void) = 0;

//-----------------------------------------------------------------------------------------
//
// ReleaseLock
//
// Description: releases the device lock

    virtual void ReleaseDeviceLock(KIRQL OldLevel) = 0;

	    // set command
    virtual void SetCommandRegister(struct _EHCI_USBCMD_CONTENT *UsbCmd) = 0;

    // get command
    virtual void GetCommandRegister(struct _EHCI_USBCMD_CONTENT *UsbCmd) = 0;



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

struct _QUEUE_HEAD;

DECLARE_INTERFACE_(IUSBRequest, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

//-----------------------------------------------------------------------------------------
//
// InitializeWithSetupPacket
//
// Description: initializes the request packet with an setup packet
// If there is a TransferBuffer, the TransferBufferLength contains the length of the buffer


    virtual NTSTATUS InitializeWithSetupPacket(IN PDMAMEMORYMANAGER DmaManager,
                                               IN PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket,
                                               IN UCHAR DeviceAddress,
                                               IN OPTIONAL PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor,
                                               IN OUT ULONG TransferBufferLength,
                                               IN OUT PMDL TransferBuffer) = 0;

//-----------------------------------------------------------------------------------------
//
// InitializeWithIrp
//
// Description: initializes the request with an IRP
// The irp contains an URB block which contains all necessary information

    virtual NTSTATUS InitializeWithIrp(IN PDMAMEMORYMANAGER DmaManager, 
                                       IN OUT PIRP Irp) = 0;

//-----------------------------------------------------------------------------------------
//
// CompletionCallback
//
// Description: called when request has been completed. It is called when
// IUSBQueue completes a queue head

    virtual VOID CompletionCallback(IN NTSTATUS NtStatusCode,
                                    IN ULONG UrbStatusCode,
                                    IN struct _QUEUE_HEAD *QueueHead) = 0;

//-----------------------------------------------------------------------------------------
//
// CancelCallback
//
// Description: called when the queue head is cancelled

    virtual VOID CancelCallback(IN NTSTATUS NtStatusCode,
                                IN struct _QUEUE_HEAD *QueueHead) = 0;

//-----------------------------------------------------------------------------------------
//
//  GetQueueHead
//
// Description: returns an initialized queue head which contains all transfer descriptors

    virtual NTSTATUS GetQueueHead(struct _QUEUE_HEAD ** OutHead) = 0;

//-----------------------------------------------------------------------------------------
//
//  IsRequestComplete
//
// Description: returns true when the request has been completed
// Should be called after the CompletionCallback has been invoked
// This function is called by IUSBQueue after queue head has been completed
// If the function returns true, IUSBQueue will then call ShouldReleaseRequestAfterCompletion
// If that function returns also true, it calls Release() to delete the IUSBRequest

    virtual BOOLEAN IsRequestComplete() = 0;

//-----------------------------------------------------------------------------------------
//
// GetTransferType
//
// Description: returns the type of the request: control, bulk, iso, interrupt

    virtual ULONG GetTransferType() = 0;

//-----------------------------------------------------------------------------------------
//
// GetResultStatus
//
// Description: returns the status code of the result
// Note: this function will block the caller untill the request has been completed

    virtual VOID GetResultStatus(OUT OPTIONAL NTSTATUS * NtStatusCode,
                                 OUT OPTIONAL PULONG UrbStatusCode) = 0;

//-----------------------------------------------------------------------------------------
//
// IsRequestInitialized
//
// Description: returns true when the request has been successfully initialized using InitializeXXX methods

    virtual BOOLEAN IsRequestInitialized() = 0;

//-----------------------------------------------------------------------------------------
//
// ShouldReleaseRequestAfterCompletion
//
// Description: this function gets called when the request returns
// IUSBQueue will then call Release() on the object to release all associated memory
// This function will typically return true when the request has been initialized with an irp
// If the request was initialized with an setup packet, it will return false

    virtual BOOLEAN ShouldReleaseRequestAfterCompletion() = 0;

//----------------------------------------------------------------------------------------
//
// FreeQueueHead
//
// Description: frees the queue head with the associated transfer descriptors

    virtual VOID FreeQueueHead(struct _QUEUE_HEAD * QueueHead) = 0;

//---------------------------------------------------------------------------------------
//
// GetTransferBuffer
//
// Description: this function returns the transfer buffer mdl and length
// Used by IUSBQueue for mapping buffer contents with DMA

    virtual VOID GetTransferBuffer(OUT PMDL * OutMDL,
                                   OUT PULONG TransferLength) = 0;

//--------------------------------------------------------------------------------------
//
// IsQueueHeadComplete
//
// Description: returns true when the queue head which was passed as a parameter has been completed

    virtual BOOLEAN IsQueueHeadComplete(struct _QUEUE_HEAD * QueueHead) = 0;
};


typedef IUSBRequest *PUSBREQUEST;

//=========================================================================================
//
// class IUSBQueue
//
// Description: This class manages pending requests
// 

DECLARE_INTERFACE_(IUSBQueue, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

//-----------------------------------------------------------------------------------------
//
// Initialize
//
// Description: initializes the object

    virtual NTSTATUS Initialize(IN PUSBHARDWAREDEVICE Hardware,
                                IN PDMA_ADAPTER AdapterObject,
                                IN PDMAMEMORYMANAGER MemManager,
                                IN OPTIONAL PKSPIN_LOCK Lock) = 0;

//-----------------------------------------------------------------------------------------
//
// GetPendingRequestCount
//
// Description: returns the number of pending requests true from IsRequestComplete

    virtual ULONG GetPendingRequestCount() = 0;

//-----------------------------------------------------------------------------------------
//
// AddUSBRequest
//
// Description: adds an usb request to the queue. 
// Returns status success when successful

    virtual NTSTATUS AddUSBRequest(IUSBRequest * Request) = 0;
    virtual NTSTATUS AddUSBRequest(PURB Urb) = 0;
//-----------------------------------------------------------------------------------------
//
// CancelRequests()
//
// Description: cancels all requests

    virtual NTSTATUS CancelRequests() = 0;

//-----------------------------------------------------------------------------------------
//
// CreateUSBRequest
//
// Description: creates an usb request

    virtual NTSTATUS CreateUSBRequest(IUSBRequest **OutRequest) = 0;

//--------------------------------------------------------------------------------------
//
// InterruptCallback
//
// Description: callback when the periodic / asynchronous queue has been completed / queue head been completed

    virtual VOID InterruptCallback(IN NTSTATUS Status, OUT PULONG ShouldRingDoorBell) = 0;

//--------------------------------------------------------------------------------------
//
// CompleteAsyncRequests
//
// Description: once a request has been completed it is moved to pending queue. Since a queue head should only be freed
// after a door bell ring, this needs some synchronization.
// This function gets called by IUSBHardware after it the Interrupt on Async Advance bit has been set

    virtual VOID CompleteAsyncRequests() = 0;
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
};

typedef IUSBDevice *PUSBDEVICE;

#endif
