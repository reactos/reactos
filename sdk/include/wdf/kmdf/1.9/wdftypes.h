/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

_WdfVersionBuild_

Module Name:

    wdfypes.h

Abstract:

    This module contains contains the C driver frameworks basic types.

Environment:

    kernel mode only

Revision History:


--*/

#ifndef _WDFTYPES_H_
#define _WDFTYPES_H_



#if (NTDDI_VERSION >= NTDDI_WIN2K)



#define WDFAPI

#ifndef __WDF_BYTE_DEFINED__
#define __WDF_BYTE_DEFINED__
typedef UCHAR BYTE;
#endif // __WDF_BYTE_DEFINED__

//
// Windows 2000 does not define FORCEINLINE, so define it if needed
//
#ifndef FORCEINLINE
#if (_MSC_VER >= 1200)
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE __inline
#endif
#endif


//
// WDF_WPP_KMDF_DRIVER  define is used by the WPP trace template file
// (km-init.tpl) to call the framework to register the KMDF provider
// if the WppInit and WppCleanup functions are executed in the 
// Windows 2000 system.
//
#ifndef WDF_WPP_KMDF_DRIVER 
#define WDF_WPP_KMDF_DRIVER 
#endif // WDF_WPP_KMDF_DRIVER 


// 
// Do not create an invalid value for this enum in case driver writers mix up
// the usage of WdfFalse/FALSE and WdfTrue/TRUE.
// 
typedef enum _WDF_TRI_STATE {
    WdfFalse = FALSE,
    WdfTrue = TRUE,
    WdfUseDefault = 2,
} WDF_TRI_STATE, *PWDF_TRI_STATE;



typedef PVOID WDFCONTEXT;

//
// Forward declare structures needed later header files
//
typedef struct WDFDEVICE_INIT *PWDFDEVICE_INIT;

typedef struct _WDF_OBJECT_ATTRIBUTES *PWDF_OBJECT_ATTRIBUTES;


#define WDF_NO_OBJECT_ATTRIBUTES (NULL)
#define WDF_NO_EVENT_CALLBACK (NULL)
#define WDF_NO_HANDLE (NULL)
#define WDF_NO_CONTEXT (NULL)
#define WDF_NO_SEND_OPTIONS (NULL)

//
// General Handle Type, should always be typeless
//
typedef HANDLE WDFOBJECT, *PWDFOBJECT;

//
// core handles
//
DECLARE_HANDLE( WDFDRIVER );
DECLARE_HANDLE( WDFDEVICE );

DECLARE_HANDLE( WDFWMIPROVIDER );
DECLARE_HANDLE( WDFWMIINSTANCE );

DECLARE_HANDLE( WDFQUEUE );
DECLARE_HANDLE( WDFREQUEST );
DECLARE_HANDLE( WDFFILEOBJECT );
DECLARE_HANDLE( WDFDPC );
DECLARE_HANDLE( WDFTIMER );
DECLARE_HANDLE( WDFWORKITEM );
DECLARE_HANDLE( WDFINTERRUPT );

//
// synch and lock handles
//
DECLARE_HANDLE( WDFWAITLOCK );
DECLARE_HANDLE( WDFSPINLOCK );

DECLARE_HANDLE( WDFMEMORY );
DECLARE_HANDLE( WDFLOOKASIDE );

//
// i/o targets for different busses
//
DECLARE_HANDLE( WDFIOTARGET );
DECLARE_HANDLE( WDFUSBDEVICE );
DECLARE_HANDLE( WDFUSBINTERFACE );
DECLARE_HANDLE( WDFUSBPIPE );

// dma handles
DECLARE_HANDLE( WDFDMAENABLER );
DECLARE_HANDLE( WDFDMATRANSACTION );
DECLARE_HANDLE( WDFCOMMONBUFFER );

//
// support handles
//
DECLARE_HANDLE( WDFKEY );
DECLARE_HANDLE( WDFSTRING );
DECLARE_HANDLE( WDFCOLLECTION );
DECLARE_HANDLE( WDFCHILDLIST );

DECLARE_HANDLE( WDFIORESREQLIST );
DECLARE_HANDLE( WDFIORESLIST );
DECLARE_HANDLE( WDFCMRESLIST );

typedef enum _WDF_REQUEST_TYPE {
    WdfRequestTypeCreate = 0x0,
    WdfRequestTypeCreateNamedPipe = 0x1,
    WdfRequestTypeClose = 0x2,
    WdfRequestTypeRead = 0x3,
    WdfRequestTypeWrite = 0x4,
    WdfRequestTypeQueryInformation = 0x5,
    WdfRequestTypeSetInformation = 0x6,
    WdfRequestTypeQueryEA = 0x7,
    WdfRequestTypeSetEA = 0x8,
    WdfRequestTypeFlushBuffers = 0x9,
    WdfRequestTypeQueryVolumeInformation = 0xa,
    WdfRequestTypeSetVolumeInformation = 0xb,
    WdfRequestTypeDirectoryControl = 0xc,
    WdfRequestTypeFileSystemControl = 0xd,
    WdfRequestTypeDeviceControl = 0xe,
    WdfRequestTypeDeviceControlInternal = 0xf,
    WdfRequestTypeShutdown = 0x10,
    WdfRequestTypeLockControl = 0x11,
    WdfRequestTypeCleanup = 0x12,
    WdfRequestTypeCreateMailSlot = 0x13,
    WdfRequestTypeQuerySecurity = 0x14,
    WdfRequestTypeSetSecurity = 0x15,
    WdfRequestTypePower = 0x16,
    WdfRequestTypeSystemControl = 0x17,
    WdfRequestTypeDeviceChange = 0x18,
    WdfRequestTypeQueryQuota = 0x19,
    WdfRequestTypeSetQuota = 0x1A,
    WdfRequestTypePnp = 0x1B,
    WdfRequestTypeOther =0x1C,
    WdfRequestTypeUsb = 0x40,
    WdfRequestTypeNoFormat = 0xFF,
    WdfRequestTypeMax,
} WDF_REQUEST_TYPE;

#endif // (NTDDI_VERSION >= NTDDI_WIN2K)


#endif // _WDFTYPES_H_

