
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
// GetDmaMemoryManager
//
// Description: returns interface to DMAMemoryManager
// Interface is reference counted, you need to call Release method when you are done with it
// Do not call Initialize on IDMAMemoryManager, the object is already initialized

    virtual NTSTATUS GetDmaMemoryManager(OUT struct IDMAMemoryManager **OutMemoryManager) = 0;

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
// ResetController()
//
// Description: this function resets the controller
// Returns STATUS_SUCCESS when the controller was successfully reset

   virtual NTSTATUS ResetController() = 0;

//-----------------------------------------------------------------------------------------
//
// ResetPort
//
// Description: this functions resets the port on the controller
//

    virtual NTSTATUS ResetPort(ULONG PortNumber) = 0;

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
                              OUT PPHYSICAL_ADDRESS *OutPhysicalAddress) = 0;


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

DECLARE_INTERFACE_(IUSBRequest, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

//-----------------------------------------------------------------------------------------
//
// InitializeWithSetupPacket
//
// Description: initializes the request packet with an setup packet
// If there is a TransferBuffer, the TransferBufferLength contains the length of the buffer


    virtual NTSTATUS InitializeWithSetupPacket(IN PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket,
                                               IN ULONG TransferBufferLength,
                                               IN PVOID TransferBuffer) = 0;

//
//TODO: find required parameters for different packet types 
//


//-----------------------------------------------------------------------------------------
//
// SetEndPoint
//
// Description: sets the endpoint of the request. 

    virtual NTSTATUS SetEndPoint(PUSB_ENDPOINT_DESCRIPTOR EndPoint);

//-----------------------------------------------------------------------------------------
//
// SetCompletionDetails
//
// Description: sets up how the request should be completed
// If an irp is passed, then it is completed with status code of the 
// CompletionCallback or CancelCallback
// If an event is passed, then the event is signaled

    virtual NTSTATUS SetCompletionDetails(IN OPTIONAL PIRP Irp,
                                          IN OPTIONAL PKEVENT Event) = 0;

//-----------------------------------------------------------------------------------------
//
// CompletionCallback
//
// Description: called when request has been completed. It is called when 
// IUSBQueue completes the request

    virtual VOID CompletionCallback(IN NTSTATUS NtStatusCode,
                                    IN ULONG UrbStatusCode) = 0;

//-----------------------------------------------------------------------------------------
//
// CancelCallback
//
// Description: called when request is cancelled. Called by IUSBQueue

    virtual VOID CancelCallback(IN NTSTATUS NtStatusCode) = 0;

};

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
                                IN OPTIONAL PKSPIN_LOCK Lock,
                                IN PDMAMEMORYMANAGER MemoryManager) = 0;

//-----------------------------------------------------------------------------------------
//
// GetPendingRequestCount
//
// Description: returns the number of pending requests

    virtual ULONG GetPendingRequestCount() = 0;

//-----------------------------------------------------------------------------------------
//
// AddUSBRequest
//
// Description: adds an usb request to the queue. 
// Returns status success when successful

    virtual NTSTATUS AddUSBRequest(IUSBRequest * Request) = 0;

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
};

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

#endif
