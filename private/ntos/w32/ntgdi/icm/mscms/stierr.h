/*++

Copyright (c) 1986-1997  Microsoft Corporation

Module Name:

    stierr.h

Abstract:

    This module contains the user mode still image APIs error and status codes

Author:


Revision History:


--*/

#ifndef _STIERR_
#define _STIERR_


//
// Generic test for success on any status value (non-negative numbers
// indicate success).
//

#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)

//
// Generic test for information on any status value.
//

#define NT_INFORMATION(Status) ((ULONG)(Status) >> 30 == 1)

//
// Generic test for warning on any status value.
//

#define NT_WARNING(Status) ((ULONG)(Status) >> 30 == 2)

//
// Generic test for error on any status value.
//

#define NT_ERROR(Status) ((ULONG)(Status) >> 30 == 3)

//
// Error codes are constructed as compound COM status codes
//

/*
 * The operation completed successfully
 */
#define STI_OK  S_OK
#define STI_ERROR_NO_ERROR          STI_OK

/*
 * The device exists but not currently attached to the system
 */
#define STI_NOTCONNECTED            S_FALSE

/*
 * The requested change in device mode settings had no effect
 */
#define STI_CHANGENOEFFECT          S_FALSE

/*
 * The application requires newer version
 */
#define STIERR_OLD_VERSION      \
        MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,ERROR_OLD_WIN_VERSION)

/*
 * The application was written for pre-release version of provider DLL
 */
#define STIERR_BETA_VERSION     \
        MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,ERROR_RMODE_APP)

/*
 * The requested object could not be created due to incompatible or mismatched driver
 */
#define STIERR_BADDRIVER        \
        MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,ERROR_BAD_DRIVER_LEVEL)

/*
 * The device is not registered
 */
#define STIERR_DEVICENOTREG     REGDB_E_CLASSNOTREG

/*
 * The requested container does not exist
 */
#define STIERR_OBJECTNOTFOUND \
        MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,ERROR_FILE_NOT_FOUND)

/*
 * An invalid or not state matching parameter was passed to the API
 */
#define STIERR_INVALID_PARAM    E_INVALIDARG

/*
 * The specified interface is not supported
 */
#define STIERR_NOINTERFACE      E_NOINTERFACE

/*
 * The undetermined error occured
 */
#define STIERR_GENERIC          E_FAIL

/*
 * There is not enough memory to perform requested operation
 */
#define STIERR_OUTOFMEMORY      E_OUTOFMEMORY

/*
 * The application called unsupported (at this time)function
 */
#define STIERR_UNSUPPORTED      E_NOTIMPL

/*
 * The application requires newer version
 */
#define STIERR_NOT_INITIALIZED     \
        MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,ERROR_NOT_READY)

/*
 * The application requires newer version
 */
#define STIERR_ALREADY_INITIALIZED     \
        MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,ERROR_ALREADY_INITIALIZED)

/*
 * The operation can not performed while device is locked
 */
#define STIERR_DEVICE_LOCKED    \
        MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,ERROR_LOCK_VIOLATION)

/*
 * The specified propery can not be changed for this device
 */
#define STIERR_READONLY         E_ACCESSDENIED

/*
 * The device already has notification handle associated with it
 */
#define STIERR_NOTINITIALIZED   E_ACCESSDENIED


/*
 * The device needs to be locked before attempting this operation
 */
#define STIERR_NEEDS_LOCK    \
        MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,ERROR_NOT_LOCKED)

/*
 * The device is opened by another application in data mode
 */
#define STIERR_SHARING_VIOLATION    \
        MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,ERROR_SHARING_VIOLATION)


/*
 * Handle already set for this context
 */
#define STIERR_HANDLEEXISTS     \
        MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,ERROR_ALREADY_EXISTS)

 /*
  * Device name is not recognized
  */
#define STIERR_INVALID_DEVICE_NAME     \
        MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,ERROR_INVALID_NAME)

 /*
  * Device hardware type is not valid
  */
#define STIERR_INVALID_HW_TYPE     \
        MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,ERROR_INVALID_DATA)
                            

 /*
  * Device hardware type is not valid
  */
#define STIERR_INVALID_HW_TYPE     \
        MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,ERROR_INVALID_DATA)
                            
 /*
  * No events available
  */
#define STIERR_NOEVENTS     \
        MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,ERROR_NO_MORE_ITEMS)


//#define STIERR_


#endif // _STIERR_



