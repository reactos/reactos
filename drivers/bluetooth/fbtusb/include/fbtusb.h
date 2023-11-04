// Copyright (c) 2004, Antony C. Roberts

// Use of this file is subject to the terms
// described in the LICENSE.TXT file that
// accompanies this file.
//
// Your use of this file indicates your
// acceptance of the terms described in
// LICENSE.TXT.
//
// http://www.freebt.net

#ifndef _FREEBT_H
#define _FREEBT_H

#include <initguid.h>
#include <wdm.h>
#include <wmilib.h>
#include <wmistr.h>
#include <windef.h>
#include "usbdi.h"
#include "usbdlib.h"

// Pull in all the command, event and structure definitions
#include "fbtHciDefs.h"

// Standard USB Wireless/Bluetooth class, etc
#define FREEBT_USB_STDCLASS		0xE0		// Wireless Controller
#define FREEBT_USB_STDSUBCLASS	0x01		// RF Controller
#define FREEBT_USB_STDPROTOCOL	0x01		// Bluetooth Programming

// Recommended Bluetooth Endpoints
#define FREEBT_STDENDPOINT_HCICMD	0x00	// HCI Command
#define FREEBT_STDENDPOINT_HCIEVENT	0x81	// HCI Event
#define FREEBT_STDENDPOINT_ACLIN	0x82	// HCI Data In
#define FREEBT_STDENDPOINT_ACLOUT	0x02	// HCI Data Out
#define FREEBT_STDENDPOINT_AUDIOIN	0x83	// SCO In
#define FREEBT_STDENDPOINT_AUDIOOUT	0x03	// SCO Out


#define OBTTAG (ULONG) 'OBTU'

#undef ExAllocatePool
#define ExAllocatePool(type, size) ExAllocatePoolWithTag(type, size, OBTTAG);

#if DBG

#define FreeBT_DbgPrint(level, _x_) \
            if((level) <= DebugLevel) { \
                DbgPrint _x_; \
            }

#else

#define FreeBT_DbgPrint(level, _x_)

#endif

typedef struct _GLOBALS
{
    UNICODE_STRING	FreeBT_RegistryPath;

} GLOBALS;

#define IDLE_INTERVAL 5000

typedef enum _PIPETYPE
{
	HciCommandPipe,
	HciEventPipe,
	AclDataIn,
	AclDataOut,
	SCODataIn,
	SCODataOut

} FREEBT_PIPETYPE;

typedef enum _DEVSTATE
{
    NotStarted,         // not started
    Stopped,            // device stopped
    Working,            // started and working
    PendingStop,        // stop pending
    PendingRemove,      // remove pending
    SurpriseRemoved,    // removed by surprise
    Removed             // removed

} DEVSTATE;

typedef enum _QUEUE_STATE
{
    HoldRequests,       // device is not started yet
    AllowRequests,      // device is ready to process
    FailRequests        // fail both existing and queued up requests

} QUEUE_STATE;

typedef enum _WDM_VERSION
{
    WinXpOrBetter,
    Win2kOrBetter,
    WinMeOrBetter,
    Win98OrBetter

} WDM_VERSION;

#define INITIALIZE_PNP_STATE(_Data_)    \
        (_Data_)->DeviceState =  NotStarted;\
        (_Data_)->PrevDevState = NotStarted;

#define SET_NEW_PNP_STATE(_Data_, _state_) \
        (_Data_)->PrevDevState =  (_Data_)->DeviceState;\
        (_Data_)->DeviceState = (_state_);

#define RESTORE_PREVIOUS_PNP_STATE(_Data_)   \
        (_Data_)->DeviceState =   (_Data_)->PrevDevState;


// registry path used for parameters
// global to all instances of the driver
#define FREEBT_REGISTRY_PARAMETERS_PATH  L"\\REGISTRY\\Machine\\System\\CurrentControlSet\\SERVICES\\BULKUSB\\Parameters"

typedef struct _FREEBT_PIPE_CONTEXT
{
    BOOLEAN			PipeOpen;
	FREEBT_PIPETYPE	PipeType;

} FREEBT_PIPE_CONTEXT, *PFREEBT_PIPE_CONTEXT;

// A structure representing the instance information associated with
// this particular device.
typedef struct _DEVICE_EXTENSION
{
    // Functional Device Object
    PDEVICE_OBJECT FunctionalDeviceObject;

    // Device object we call when submitting Urbs
    PDEVICE_OBJECT TopOfStackDeviceObject;

    // The bus driver object
    PDEVICE_OBJECT PhysicalDeviceObject;

    // Name buffer for our named Functional device object link
    // The name is generated based on the driver's class GUID
    UNICODE_STRING InterfaceName;

    // Bus drivers set the appropriate values in this structure in response
    // to an IRP_MN_QUERY_CAPABILITIES IRP. Function and filter drivers might
    // alter the capabilities set by the bus driver.
    DEVICE_CAPABILITIES DeviceCapabilities;

    // Configuration Descriptor
    PUSB_CONFIGURATION_DESCRIPTOR UsbConfigurationDescriptor;

    // Interface Information structure
    PUSBD_INTERFACE_INFORMATION UsbInterface;

    // Pipe context for the driver
    PFREEBT_PIPE_CONTEXT PipeContext;

    // current state of device
    DEVSTATE DeviceState;

    // state prior to removal query
    DEVSTATE PrevDevState;

    // obtain and hold this lock while changing the device state,
    // the queue state and while processing the queue.
    KSPIN_LOCK DevStateLock;

    // current system power state
    SYSTEM_POWER_STATE SysPower;

    // current device power state
    DEVICE_POWER_STATE DevPower;

    // Pending I/O queue state
    QUEUE_STATE QueueState;

    // Pending I/O queue
    LIST_ENTRY NewRequestsQueue;

    // I/O Queue Lock
    KSPIN_LOCK QueueLock;

    KEVENT RemoveEvent;

    KEVENT StopEvent;

    ULONG OutStandingIO;

    KSPIN_LOCK IOCountLock;

    // Selective Suspend variables
    LONG SSEnable;
    LONG SSRegistryEnable;
    PUSB_IDLE_CALLBACK_INFO IdleCallbackInfo;
    PIRP PendingIdleIrp;
    LONG IdleReqPend;
    LONG FreeIdleIrpCount;
    KSPIN_LOCK IdleReqStateLock;
    KEVENT NoIdleReqPendEvent;

    // Default power state to power down to on self-susped
    ULONG PowerDownLevel;

    // remote wakeup variables
    PIRP WaitWakeIrp;
    LONG FlagWWCancel;
    LONG FlagWWOutstanding;
    LONG WaitWakeEnable;

    // Open handle count
    LONG OpenHandleCount;

    // Selective suspend model uses timers, dpcs and work item.
    KTIMER Timer;

    KDPC DeferredProcCall;

    // This event is cleared when a DPC/Work Item is queued.
    // and signaled when the work-item completes.
    // This is essential to prevent the driver from unloading
    // while we have DPC or work-item queued up.
    KEVENT NoDpcWorkItemPendingEvent;

    // WMI information
    WMILIB_CONTEXT WmiLibInfo;

    // WDM version
    WDM_VERSION WdmVersion;

    // Pipe type
    FREEBT_PIPETYPE	PipeType;

    // User accessible object name
	WCHAR wszDosDeviceName[50];

	// A never triggered event used for delaying execution
	KEVENT DelayEvent;

	// Significant pipes
	USBD_PIPE_INFORMATION EventPipe;
	USBD_PIPE_INFORMATION DataInPipe;
	USBD_PIPE_INFORMATION DataOutPipe;
	USBD_PIPE_INFORMATION AudioInPipe;
	USBD_PIPE_INFORMATION AudioOutPipe;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


typedef struct _IRP_COMPLETION_CONTEXT
{
    PDEVICE_EXTENSION DeviceExtension;
    PKEVENT Event;

} IRP_COMPLETION_CONTEXT, *PIRP_COMPLETION_CONTEXT;

extern GLOBALS Globals;
extern ULONG DebugLevel;

#endif
