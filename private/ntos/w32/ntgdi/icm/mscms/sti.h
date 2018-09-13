/*++

Copyright (c) 1986-1997  Microsoft Corporation

Module Name:

    sti.h

Abstract:

    This module contains the user mode still image APIs in COM format

Revision History:


--*/

#ifndef _STICOM_
#define _STICOM_

//
// Set packing
//
#include <pshpack8.h>

//
// Only use UNICODE STI interfaces
//
#define STI_UNICODE 1

//
// Include COM definitions
//
#ifndef _NO_COM
#include <objbase.h>
#endif

#include <stireg.h>
#include <stierr.h>

//
// Compiler pragmas
//
#pragma warning(disable:4200)       // warning about zero-sized arrays being non-stadard C extension

#define DLLEXP __declspec( dllexport )

#ifdef __cplusplus
extern "C" {
#endif

#if defined( _WIN32 ) && !defined( _NO_COM)

/*
 * Class IID's
 */

// B323F8E0-2E68-11D0-90EA-00AA0060F86C
DEFINE_GUID(CLSID_Sti, 0xB323F8E0L, 0x2E68, 0x11D0, 0x90, 0xEA, 0x00, 0xAA, 0x00, 0x60, 0xF8, 0x6C);

/*
 * Interface IID's
 */

// {641BD880-2DC8-11D0-90EA-00AA0060F86C}
DEFINE_GUID(IID_IStillImageW, 0x641BD880L, 0x2DC8, 0x11D0, 0x90, 0xEA, 0x00, 0xAA, 0x00, 0x60, 0xF8, 0x6C);

// {A7B1F740-1D7F-11D1-ACA9-00A02438AD48}
DEFINE_GUID(IID_IStillImageA, 0xA7B1F740L, 0x1D7F, 0x11D1, 0xAC, 0xA9, 0x00, 0xA0, 0x24, 0x38, 0xAD, 0x48);


// {6CFA5A80-2DC8-11D0-90EA-00AA0060F86C}
DEFINE_GUID(IID_IStiDevice, 0x6CFA5A80L, 0x2DC8, 0x11D0, 0x90, 0xEA, 0x00, 0xAA, 0x00, 0x60, 0xF8, 0x6C);

#endif

//
// Generic constants and definitions
//
#define STI_VERSION                 0x00000002
#define STI_VERSION_MIN_ALLOWED     0x00000002

#define GET_STIVER_MAJOR(dwVersion)   HIWORD(dwVersion)
#define GET_STIVER_MINOR(dwVersion)   LOWORD(dwVersion)

//
// Maximum length of internal device name
//
#define STI_MAX_INTERNAL_NAME_LENGTH    128

// begin sti_device_information

//
//  Device information definitions and prototypes
// ----------------------------------------------
//

//
//  Following information is used for enumerating still image devices , currently configured
//  in the system. Presence of the device in the enumerated list does not mean availability
// of the device, it only means that device was installed at least once and had not been removed since.
//

//
// Type of device ( scanner, camera) is represented by DWORD value with
// hi word containing generic device type , and lo word containing sub-type
//
typedef enum _STI_DEVICE_MJ_TYPE {
    StiDeviceTypeDefault          = 0,
    StiDeviceTypeScanner          = 1,
    StiDeviceTypeDigitalCamera    = 2
} STI_DEVICE_MJ_TYPE;

typedef DWORD STI_DEVICE_TYPE;

//
// Macros to extract device type/subtype from single type field
//
#define GET_STIDEVICE_TYPE(dwDevType)   HIWORD(dwDevType)
#define GET_STIDEVICE_SUBTYPE(dwDevType)   LOWORD(dwDevType)

//
// Device capabilities bits.
// Various capabilities are grouped into separate bitmasks
//

typedef struct _STI_DEV_CAPS {
    DWORD   dwGeneric;
} STI_DEV_CAPS, *PSTI_DEV_CAPS;


//
// Notifications are supported.
// If this capability set , device can be subscribed to .
//
#define STI_GENCAP_NOTIFICATIONS    0x00000001

//
// Polling required .
// This capability is used when previous is set to TRUE. Presence of it means
// that device is not capable of issuing "truly" asyncronous notifications, but can
// be polled to determine the moment when event happened
#define STI_GENCAP_POLLING_NEEDED   0x00000002


//
// Type of bus connection for those in need to know
//
#define STI_HW_CONFIG_UNKNOWN   0x0001
#define STI_HW_CONFIG_SCSI      0x0002
#define STI_HW_CONFIG_USB       0x0004
#define STI_HW_CONFIG_SERIAL    0x0008
#define STI_HW_CONFIG_PARALLEL  0x0010

//
// Device information structure, this is not configurable. This data is returned from
// device enumeration API and is used for populating UI or selecting which device
// should be used in current session
//
typedef struct _STI_DEVICE_INFORMATIONW {
    DWORD   dwSize;

    // Type of the hardware imaging device
    STI_DEVICE_TYPE   DeviceType;

    // Device identifier for reference when creating device object
    WCHAR   szDeviceInternalName[STI_MAX_INTERNAL_NAME_LENGTH];

    // Set of capabilities flags
    STI_DEV_CAPS   DeviceCapabilities;

    // This includes bus type
    DWORD   dwHardwareConfiguration;

    // Vendor description string
    LPWSTR    pszVendorDescription;

    // Device description , provided by vendor
    LPWSTR    pszDeviceDescription;

    // String , representing port on which device is accessible.
    LPWSTR    pszPortName;

    // Control panel propery provider
    LPWSTR    pszPropProvider;

    // Local specific ("friendly") name of the device, mainly used for showing in the UI
    LPWSTR    pszLocalName;

} STI_DEVICE_INFORMATIONW, *PSTI_DEVICE_INFORMATIONW;

typedef struct _STI_DEVICE_INFORMATIONA {
    DWORD   dwSize;

    // Type of the hardware imaging device
    STI_DEVICE_TYPE   DeviceType;

    // Device identifier for reference when creating device object
    CHAR    szDeviceInternalName[STI_MAX_INTERNAL_NAME_LENGTH];

    // Set of capabilities flags
    STI_DEV_CAPS   DeviceCapabilities;

    // This includes bus type
    DWORD   dwHardwareConfiguration;

    // Vendor description string
    LPCSTR    pszVendorDescription;

    // Device description , provided by vendor
    LPCSTR    pszDeviceDescription;

    // String , representing port on which device is accessible.
    LPCSTR    pszPortName;

    // Control panel propery provider
    LPCSTR    pszPropProvider;

    // Local specific ("friendly") name of the device, mainly used for showing in the UI
    LPCSTR    pszLocalName;

} STI_DEVICE_INFORMATIONA, *PSTI_DEVICE_INFORMATIONA;

#if defined(UNICODE) || defined(STI_UNICODE)
typedef STI_DEVICE_INFORMATIONW STI_DEVICE_INFORMATION;
typedef PSTI_DEVICE_INFORMATIONW PSTI_DEVICE_INFORMATION;
#else
typedef STI_DEVICE_INFORMATIONA STI_DEVICE_INFORMATION;
typedef PSTI_DEVICE_INFORMATIONA PSTI_DEVICE_INFORMATION;
#endif

// end sti_device_information

//
// Device state information.
// ------------------------
//
// Following types  are used to inquire state characteristics of the device after
// it had been opened.
//
// Device configuration structure contains configurable parameters reflecting
// current state of the device
//
//
// Device hardware status.
//

//
// Individual bits for state acquiring  through StatusMask
//

// State of hardware as known to USD
#define STI_DEVSTATUS_ONLINE_STATE      0x0001

// State of pending events ( as known to USD)
#define STI_DEVSTATUS_EVENTS_STATE      0x0002

//
// Online state values
//
#define STI_ONLINESTATE_OPERATIONAL         0x00000001
#define STI_ONLINESTATE_PENDING             0x00000002
#define STI_ONLINESTATE_ERROR               0x00000004
#define STI_ONLINESTATE_PAUSED              0x00000008
#define STI_ONLINESTATE_PAPER_JAM           0x00000010
#define STI_ONLINESTATE_PAPER_PROBLEM       0x00000020
#define STI_ONLINESTATE_OFFLINE             0x00000040
#define STI_ONLINESTATE_IO_ACTIVE           0x00000080
#define STI_ONLINESTATE_BUSY                0x00000100
#define STI_ONLINESTATE_TRANSFERRING        0x00000200
#define STI_ONLINESTATE_INITIALIZING        0x00000400
#define STI_ONLINESTATE_WARMING_UP          0x00000800
#define STI_ONLINESTATE_USER_INTERVENTION   0x00001000
#define STI_ONLINESTATE_POWER_SAVE          0x00002000

//
// Event processing parameters
//
#define STI_EVENTHANDLING_ENABLED           0x00000001
#define STI_EVENTHANDLING_POLLING           0x00000002
#define STI_EVENTHANDLING_PENDING           0x00000004

typedef struct _STI_DEVICE_STATUS {

    DWORD   dwSize;

    // Request field - bits of status to verify
    DWORD   StatusMask;

    //
    // Fields are set when status mask contains STI_DEVSTATUS_ONLINE_STATE bit set
    //
    // Bitmask describing  device state
    DWORD   dwOnlineState;

    // Device status code as defined by vendor
    DWORD   dwHardwareStatusCode;

    //
    // Fields are set when status mask contains STI_DEVSTATUS_EVENTS_STATE bit set
    //

    // State of device notification processing (enabled, pending)
    DWORD   dwEventHandlingState;

    // If device is polled, polling interval in ms
    DWORD   dwPollingInterval;

} STI_DEVICE_STATUS,*PSTI_DEVICE_STATUS;

//
// Structure to describe diagnostic ( test ) request to be processed by USD
//

// Basic test for presence of associated hardware
#define STI_DIAGCODE_HWPRESENCE         0x00000001

//
// Status bits for diagnostic
//

//
// generic diagnostic errors
//

typedef struct _ERROR_INFOW {

    DWORD   dwSize;

    // Generic error , describing results of last operation
    DWORD   dwGenericError;

    // vendor specific error code
    DWORD   dwVendorError;

    // String, describing in more details results of last operation if it failed
    WCHAR   szExtendedErrorText[255];

} STI_ERROR_INFOW,*PSTI_ERROR_INFOW;

typedef struct _ERROR_INFOA {

    DWORD   dwSize;

    DWORD   dwGenericError;
    DWORD   dwVendorError;

    CHAR   szExtendedErrorText[255];

} STI_ERROR_INFOA,*PSTI_ERROR_INFOA;

#if defined(UNICODE) || defined(STI_UNICODE)
typedef STI_ERROR_INFOW STI_ERROR_INFO;
#else
typedef STI_ERROR_INFOA STI_ERROR_INFO;
#endif

typedef STI_ERROR_INFO* PSTI_ERROR_INFO;

typedef struct _STI_DIAG {

    DWORD   dwSize;

    // Diagnostic request fields. Are set on request by caller

    // One of the
    DWORD   dwBasicDiagCode;
    DWORD   dwVendorDiagCode;

    // Response fields
    DWORD   dwStatusMask;

    STI_ERROR_INFO  sErrorInfo;

} STI_DIAG,*LPSTI_DIAG;

//
typedef STI_DIAG    DIAG;
typedef LPSTI_DIAG  LPDIAG;


// end device state information.

//
// Flags passed to WriteToErrorLog call in a first parameter, indicating type of the message
// which needs to be logged
//
#define STI_TRACE_INFORMATION       0x00000001
#define STI_TRACE_WARNING           0x00000002
#define STI_TRACE_ERROR             0x00000004

//
// Event notification mechansims.
// ------------------------------
//
// Those are used to inform last subscribed caller of the changes in device state, initiated by
// device.
//
// The only supported discipline of notification is stack. The last caller to subscribe will be notified
// and will receive notification data. After caller unsubscribes , the previously subscribed caller will
// become active.
//

// Notifications are sent to subscriber via window message. Window handle is passed as
// parameter
#define STI_SUBSCRIBE_FLAG_WINDOW   0x0001

// Device notification is signalling Win32 event ( auto-set event). Event handle
// is passed as a parameter
#define STI_SUBSCRIBE_FLAG_EVENT    0x0002

typedef struct _STISUBSCRIBE {

    DWORD   dwSize;

    DWORD   dwFlags;

    // Not used . Will be used for subscriber to set bit mask filtering different events
    DWORD   dwFilter;

    // When STI_SUBSCRIBE_FLAG_WINDOW bit is set, following fields should be set
    // Handle of the window which will receive notification message
    HWND    hWndNotify;

    // Handle of Win32 auto-reset event , which will be signalled whenever device has
    // notification pending
    HANDLE  hEvent;

    // Code of notification message, sent to window
    UINT    uiNotificationMessage;

} STISUBSCRIBE,*LPSTISUBSCRIBE;

#define MAX_NOTIFICATION_DATA   64


//
// Structure to describe notification information
//
typedef struct _STINOTIFY {

    DWORD   dwSize;                 // Total size of the notification structure

    // GUID of the notification being retrieved
    GUID    guidNotificationCode;

    // Vendor specific notification description
    BYTE    abNotificationData[MAX_NOTIFICATION_DATA];     // USD specific

} STINOTIFY,*LPSTINOTIFY;


// end event_mechanisms

//
// STI device broadcasting
//

//
// When STI Device is being added or removed, PnP broadacst is being sent , but it is not obvious
// for application code to recognize if it is STI device and if so, what is the name of the
// device. STI subsystem will analyze PnP broadcasts and rebroadcast another message via
// BroadcastSystemMessage / WM_DEVICECHANGE / DBT_USERDEFINED .

// String passed as user defined message contains STI prefix, action and device name

#define STI_ADD_DEVICE_BROADCAST_ACTION     "Arrival"
#define STI_REMOVE_DEVICE_BROADCAST_ACTION  "Removal"

#define STI_ADD_DEVICE_BROADCAST_STRING     "STI\\" STI_ADD_DEVICE_BROADCAST_ACTION "\\%s"
#define STI_REMOVE_DEVICE_BROADCAST_STRING  "STI\\" STI_REMOVE_DEVICE_BROADCAST_ACTION "\\%s"


// end STI broadcasting


//
// Device create modes
//

// Device is being opened only for status querying and notifications receiving
#define STI_DEVICE_CREATE_STATUS         0x00000001

// Device is being opened for data transfer ( supersedes status mode)
#define STI_DEVICE_CREATE_DATA           0x00000002

#define STI_DEVICE_CREATE_BOTH           0x00000003

//
// Bit mask for legitimate mode bits, which can be used when calling CreateDevice
//
#define STI_DEVICE_CREATE_MASK           0x0000FFFF

//
// Flags controlling device enumeration
//
#define STIEDFL_ALLDEVICES             0x00000000
#define STIEDFL_ATTACHEDONLY           0x00000001

//
// Control code , sent to the device through raw control interface
//
typedef  DWORD STI_RAW_CONTROL_CODE;

//
// All raw codes below this one are reserved for future use.
//
#define STI_RAW_RESERVED    0x1000

 /*
  * COM Interfaces to STI
  */

#ifdef __cplusplus

/* 'struct' not 'class' per the way DECLARE_INTERFACE_ is defined */
interface IStillImageW;
interface IStillImageA;

interface IStiDevice;

#endif

#ifndef MIDL_PASS

DLLEXP STDMETHODIMP StiCreateInstanceW(HINSTANCE hinst, DWORD dwVer, interface IStillImageW **ppSti, LPUNKNOWN punkOuter);
DLLEXP STDMETHODIMP StiCreateInstanceA(HINSTANCE hinst, DWORD dwVer, interface IStillImageA **ppSti, LPUNKNOWN punkOuter);

#if defined(UNICODE) || defined(STI_UNICODE)
#define IID_IStillImage     IID_IStillImageW
#define IStillImage         IStillImageW
#define StiCreateInstance   StiCreateInstanceW
#else
#define IID_IStillImage     IID_IStillImageA
#define IStillImage         IStillImageA
#define StiCreateInstance   StiCreateInstanceA
#endif

typedef interface IStiDevice              *LPSTILLIMAGEDEVICE;

typedef interface IStillImage             *PSTI;
typedef interface IStiDevice              *PSTIDEVICE;

typedef interface IStillImageA            *PSTIA;
typedef interface IStiDeviceA             *PSTIDEVICEA;

typedef interface IStillImageW            *PSTIW;
typedef interface IStiDeviceW             *PSTIDEVICEW;

//DLLEXP STDMETHODIMP StiCreateInstance(HINSTANCE hinst, DWORD dwVer, PSTI *ppSti, LPUNKNOWN punkOuter);

/*
 * IStillImage interface
 *
 * Top level STI access interface.
 *
 */

#undef INTERFACE
#define INTERFACE IStillImageW
DECLARE_INTERFACE_(IStillImageW, IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;

    /*** IStillImage methods ***/
    STDMETHOD(Initialize) (THIS_ HINSTANCE hinst,DWORD dwVersion) PURE;

    STDMETHOD(GetDeviceList)(THIS_ DWORD dwType,DWORD dwFlags,DWORD *pdwItemsReturned,LPVOID *ppBuffer) PURE;
    STDMETHOD(GetDeviceInfo)(THIS_ LPWSTR  pwszDeviceName, LPVOID *ppBuffer) PURE;

    STDMETHOD(CreateDevice) (THIS_ LPWSTR  pwszDeviceName, DWORD   dwMode, PSTIDEVICE *pDevice,LPUNKNOWN punkOuter) PURE;

    //
    // Device instance values. Used to associate various data with device.
    //
    STDMETHOD(GetDeviceValue)(THIS_ LPWSTR  pwszDeviceName,LPWSTR    pValueName,LPDWORD  pType,LPBYTE   pData,LPDWORD    cbData);
    STDMETHOD(SetDeviceValue)(THIS_ LPWSTR  pwszDeviceName,LPWSTR   pValueName,DWORD   Type,LPBYTE  pData,DWORD   cbData);

    //
    // For appllication started through push model launch, returns associated information
    //
    STDMETHOD(GetSTILaunchInformation)(THIS_ LPWSTR  pwszDeviceName, DWORD *pdwEventCode,LPWSTR  pwszEventName) PURE;
    STDMETHOD(RegisterLaunchApplication)(THIS_ LPWSTR  pwszAppName,LPWSTR  pwszCommandLine) PURE;
    STDMETHOD(UnregisterLaunchApplication)(THIS_ LPWSTR  pwszAppName) PURE;

    //
    // To control state of notification handling. For polled devices this means state of monitor
    // polling, for true notification devices means enabling/disabling notification flow
    // from monitor to registered applications
    //
    STDMETHOD(EnableHwNotifications)(THIS_ LPCWSTR  pwszDeviceName,BOOL bNewState) PURE;
    STDMETHOD(GetHwNotificationState)(THIS_ LPCWSTR  pwszDeviceName,BOOL *pbCurrentState) PURE;

    //
    // When device is installed but not accessible, application may request bus refresh
    // which in some cases will make device known. This is mainly used for nonPnP buses
    // like SCSI, when device was powered on after PnP enumeration
    //
    //
    STDMETHOD(RefreshDeviceBus)(THIS_ LPCWSTR  pwszDeviceName) PURE;

    //
    // Launch application to emulate event on a device. Used by "control center" style components,
    // which intercept device event , analyze and later force launch based on certain criteria.
    //
    STDMETHOD(LaunchApplicationForDevice)(THIS_ LPWSTR  pwszDeviceName,LPWSTR    pwszAppName,LPSTINOTIFY    pStiNotify);

    //
    // For non-PnP devices with non-known bus type connection, setup extension, associated with the
    // device can set it's parameters
    //
    STDMETHOD(SetupDeviceParameters)(THIS_ PSTI_DEVICE_INFORMATIONW);

    //
    // Write message to STI error log
    //
    STDMETHOD(WriteToErrorLog)(THIS_ DWORD dwMessageType,LPCWSTR pszMessage) PURE;

    #ifdef NOT_IMPLEMENTED

        //
        // TO register application for receiving various STI notifications
        //
        STIMETHOD(RegisterDeviceNotification(THIS_ LPWSTR  pwszAppName,LPSUBSCRIBE lpSubscribe) PURE;
        STIMETHOD(UnregisterDeviceNotification(THIS_ ) PURE;

    #endif //NOT_IMPLEMENTED

};

typedef struct IStillImageW *LPSTILLIMAGEW;

#undef INTERFACE
#define INTERFACE IStillImageA
DECLARE_INTERFACE_(IStillImageA, IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;

    /*** IStillImage methods ***/
    STDMETHOD(Initialize) (THIS_ HINSTANCE hinst,DWORD dwVersion) PURE;

    STDMETHOD(GetDeviceList)(THIS_ DWORD dwType,DWORD dwFlags,DWORD *pdwItemsReturned,LPVOID *ppBuffer) PURE;
    STDMETHOD(GetDeviceInfo)(THIS_ LPCSTR  pwszDeviceName, LPVOID *ppBuffer) PURE;

    STDMETHOD(CreateDevice) (THIS_ LPCSTR  pwszDeviceName, DWORD   dwMode, PSTIDEVICE *pDevice,LPUNKNOWN punkOuter) PURE;

    //
    // Device instance values. Used to associate various data with device.
    //
    STDMETHOD(GetDeviceValue)(THIS_ LPCSTR  pwszDeviceName,LPCSTR   pValueName,LPDWORD  pType,LPBYTE   pData,LPDWORD    cbData);
    STDMETHOD(SetDeviceValue)(THIS_ LPCSTR  pwszDeviceName,LPCSTR   pValueName,DWORD   Type,LPBYTE  pData,DWORD   cbData);

    //
    // For appllication started through push model launch, returns associated information
    //
    STDMETHOD(GetSTILaunchInformation)(THIS_ LPCSTR  pwszDeviceName, DWORD *pdwEventCode,LPSTR  pwszEventName) PURE;
    STDMETHOD(RegisterLaunchApplication)(THIS_ LPCSTR  pwszAppName,LPCSTR  pwszCommandLine) PURE;
    STDMETHOD(UnregisterLaunchApplication)(THIS_ LPCSTR  pwszAppName) PURE;

    //
    // To control state of notification handling. For polled devices this means state of monitor
    // polling, for true notification devices means enabling/disabling notification flow
    // from monitor to registered applications
    //
    STDMETHOD(EnableHwNotifications)(THIS_ LPCSTR  pwszDeviceName,BOOL bNewState) PURE;
    STDMETHOD(GetHwNotificationState)(THIS_ LPCSTR  pwszDeviceName,BOOL *pbCurrentState) PURE;

    //
    // When device is installed but not accessible, application may request bus refresh
    // which in some cases will make device known. This is mainly used for nonPnP buses
    // like SCSI, when device was powered on after PnP enumeration
    //
    //
    STDMETHOD(RefreshDeviceBus)(THIS_ LPCSTR  pwszDeviceName) PURE;

    //
    // Launch application to emulate event on a device. Used by "control center" style components,
    // which intercept device event , analyze and later force launch based on certain criteria.
    //
    STDMETHOD(LaunchApplicationForDevice)(THIS_ LPCSTR    pwszDeviceName,LPCSTR    pwszAppName,LPSTINOTIFY    pStiNotify);


    //
    // For non-PnP devices with non-known bus type connection, setup extension, associated with the
    // device can set it's parameters
    //
    STDMETHOD(SetupDeviceParameters)(THIS_ PSTI_DEVICE_INFORMATIONA);

    //
    // Write message to STI error log
    //
    STDMETHOD(WriteToErrorLog)(THIS_ DWORD dwMessageType,LPCSTR pszMessage) PURE;

    #ifdef NOT_IMPLEMENTED

        //
        // TO register application for receiving various STI notifications
        //
        STIMETHOD(RegisterDeviceNotification(THIS_ LPWSTR  pwszAppName,LPSUBSCRIBE lpSubscribe) PURE;
        STIMETHOD(UnregisterDeviceNotification(THIS_ ) PURE;

    #endif //NOT_IMPLEMENTED

};

typedef struct IStillImageA *LPSTILLIMAGEA;

#if defined(UNICODE) || defined(STI_UNICODE)
#define IStillImageVtbl     IStillImageWVtbl
#else
#define IStillImageVtbl     IStillImageAVtbl
#endif

typedef struct IStillImage  *LPSTILLIMAGE;

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IStillImage_QueryInterface(p,a,b)       (p)->lpVtbl->QueryInterface(p,a,b)
#define IStillImage_AddRef(p)                   (p)->lpVtbl->AddRef(p)
#define IStillImage_Release(p)                  (p)->lpVtbl->Release(p)
#define IStillImage_Initialize(p,a,b)           (p)->lpVtbl->Initialize(p,a,b)

#define IStillImage_GetDeviceList(p,a,b,c,d)    (p)->lpVtbl->GetDeviceList(p,a,b,c,d)
#define IStillImage_GetDeviceInfo(p,a,b)        (p)->lpVtbl->GetDeviceInfo(p,a,b)
#define IStillImage_CreateDevice(p,a,b,c,d)     (p)->lpVtbl->CreateDevice(p,a,b,c,d)
#define IStillImage_GetDeviceValue(p,a,b,c,d,e)           (p)->lpVtbl->GetDeviceValue(p,a,b,c,d,e)
#define IStillImage_SetDeviceValue(p,a,b,c,d,e)           (p)->lpVtbl->SetDeviceValue(p,a,b,c,d,e)
#define IStillImage_GetSTILaunchInformation(p,a,b,c)      (p)->lpVtbl->GetSTILaunchInformation(p,a,b,c)
#define IStillImage_RegisterLaunchApplication(p,a,b)      (p)->lpVtbl->RegisterLaunchApplication(p,a,b)
#define IStillImage_UnregisterLaunchApplication(p,a)      (p)->lpVtbl->UnregisterLaunchApplication(p,a)
#define IStillImage_EnableHwNotifications(p,a,b)          (p)->lpVtbl->EnableHwNotifications(p,a,b)
#define IStillImage_GetHwNotificationState(p,a,b)         (p)->lpVtbl->GetHwNotificationState(p,a,b)
#define IStillImage_RefreshDeviceBus(p,a)                 (p)->lpVtbl->RefreshDeviceBus(p,a)

#endif

/*
 * IStillImage_Device interface
 *
 * This is generic per device interface. Specialized interfaces are also
 * available
 */
#undef INTERFACE
#define INTERFACE IStiDevice
DECLARE_INTERFACE_(IStiDevice, IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;

    /*** IStiDevice methods ***/
    STDMETHOD(Initialize) (THIS_ HINSTANCE hinst,LPCWSTR pwszDeviceName,DWORD dwVersion,DWORD  dwMode) PURE;

    STDMETHOD(GetCapabilities) (THIS_ PSTI_DEV_CAPS pDevCaps) PURE;

    STDMETHOD(GetStatus) (THIS_ PSTI_DEVICE_STATUS pDevStatus) PURE;

    STDMETHOD(DeviceReset)(THIS ) PURE;
    STDMETHOD(Diagnostic)(THIS_ LPSTI_DIAG pBuffer) PURE;

    STDMETHOD(Escape)(THIS_ STI_RAW_CONTROL_CODE    EscapeFunction,LPVOID  lpInData,DWORD   cbInDataSize,LPVOID pOutData,DWORD dwOutDataSize,LPDWORD pdwActualData) PURE ;

    STDMETHOD(GetLastError) (THIS_ LPDWORD pdwLastDeviceError) PURE;

    STDMETHOD(LockDevice) (THIS_ DWORD dwTimeOut) PURE;
    STDMETHOD(UnLockDevice) (THIS ) PURE;

    STDMETHOD(RawReadData)(THIS_ LPVOID lpBuffer,LPDWORD lpdwNumberOfBytes,LPOVERLAPPED lpOverlapped) PURE;
    STDMETHOD(RawWriteData)(THIS_ LPVOID lpBuffer,DWORD nNumberOfBytes,LPOVERLAPPED lpOverlapped) PURE;

    STDMETHOD(RawReadCommand)(THIS_ LPVOID lpBuffer,LPDWORD lpdwNumberOfBytes,LPOVERLAPPED lpOverlapped) PURE;
    STDMETHOD(RawWriteCommand)(THIS_ LPVOID lpBuffer,DWORD nNumberOfBytes,LPOVERLAPPED lpOverlapped) PURE;

    //
    // Subscription is used to enable "control center" style applications , where flow of
    // notifications should be redirected from monitor itself to another "launcher"
    //
    STDMETHOD(Subscribe)(THIS_ LPSTISUBSCRIBE lpSubsribe) PURE;
    STDMETHOD(GetLastNotificationData)(THIS_ LPSTINOTIFY   lpNotify) PURE;
    STDMETHOD(UnSubscribe)(THIS ) PURE;

    STDMETHOD(GetLastErrorInfo) (THIS_ STI_ERROR_INFO *pLastErrorInfo) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IStiDevice_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IStiDevice_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IStiDevice_Release(p)                   (p)->lpVtbl->Release(p)
#define IStiDevice_Initialize(p,a,b,c,d)        (p)->lpVtbl->Initialize(p,a,b,c,d)

#define IStiDevice_GetCapabilities(p,a)         (p)->lpVtbl->GetCapabilities(p,a)
#define IStiDevice_GetStatus(p,a)               (p)->lpVtbl->GetStatus(p,a)
#define IStiDevice_DeviceReset(p)               (p)->lpVtbl->DeviceReset(p)
#define IStiDevice_LockDevice(p,a)              (p)->lpVtbl->LockDevice(p,a)
#define IStiDevice_UnLockDevice(p)              (p)->lpVtbl->LockDevice(p)

#define IStiDevice_Diagnostic(p,a)              (p)->lpVtbl->Diagnostic(p,a)
#define IStiDevice_Escape(p,a,b,c,d,e,f)        (p)->lpVtbl->Escape(p,a,b,c,d,e,f)
#define IStiDevice_GetLastError(p,a)            (p)->lpVtbl->GetLastError(p,a)
#define IStiDevice_RawReadData(p,a,b,c)         (p)->lpVtbl->RawReadData(p,a,b,c)
#define IStiDevice_RawWriteData(p,a,b,c)        (p)->lpVtbl->RawWriteData(p,a,b,c)
#define IStiDevice_RawReadCommand(p,a,b,c)      (p)->lpVtbl->RawReadCommand(p,a,b,c)
#define IStiDevice_RawWriteCommand(p,a,b,c)     (p)->lpVtbl->RawWriteCommand(p,a,b,c)

#define IStiDevice_Subscribe(p,a)               (p)->lpVtbl->Subscribe(p,a)
#define IStiDevice_GetNotificationData(p,a)     (p)->lpVtbl->GetNotificationData(p,a)
#define IStiDevice_UnSubscribe(p)               (p)->lpVtbl->UnSubscribe(p)

#define IStiDevice_GetLastErrorInfo(p,a)        (p)->lpVtbl->GetLastErrorInfo(p,a)

#endif

#endif  // MIDL_PASS

#ifdef __cplusplus
};
#endif

//
// Reset packing
//
#include <poppack.h>

#endif // _STICOM_



