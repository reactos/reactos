/*
 * wdftypes.h
 *
 * Windows Driver Framework - C Driver Frameworks Basic Types
 *
 * This file is part of the ReactOS WDF package.
 *
 * Contributors:
 *   Created by Benjamin Aerni <admin@bennottelling.com>
 *
 * Intended Usecase:
 *   Kernel mode drivers
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
 #ifndef _WDFTYPES_H_
 #define _WDFTYPES_H_



#if (NTDDI_VERSION >= NTDDI_WIN2K)

#define WDFAPI

#ifndef __WDF_BYTE_DEFINED__
#define __WDF_BYTE_DEFINED__
typedef UCHAR BYTE;
#endif

/* Inline Definitions, from sdk/include/xdx/ntbasedef.h */
#ifndef FORCEINLINE
 #if defined(_MSC_VER)
  #define FORCEINLINE __forceinline
 #elif ( __MINGW_GNUC_PREREQ(4, 3)  &&  __STDC_VERSION__ >= 199901L)
  #define FORCEINLINE extern inline __attribute__((__always_inline__,__gnu_inline__))
 #else
  #define FORCEINLINE extern __inline__ __attribute__((__always_inline__))
 #endif
#endif /* FORCEINLINE */

/* This definiton is used by WPP trace template file */
#ifndef WDF_WPP_KMDF_DRIVER
#define WDF_WPP_KMDF_DRIVER
#endif

typedef enum _WDF_TRI_STATE{
    WdfFalse = FALSE,
    WdfTrue = TRUE,
    WdfUseDefault = 2,
} WDF_TRI_STATE, *PWDF_TRI_STATE;

typedef PVOID WDFCONTEXT;

/* Declare structures needed by other headers */
typedef struct WDFDEVICE_INIT *PWDFDEVICE_INIT;
typedef struct _WDF_OBJECT_ATTRIBUTES *PWDF_OBJECT_ATTRIBUTES;

#define WDF_NO_OBJECT_ATTRIBUTES (NULL)
#define WDF_NO_EVENT_CALLBACK (NULL)
#define WDF_NO_HANDLE (NULL)
#define WDF_NO_CONTEXT (NULL)
#define WDF_NO_SEND_OPTIONS (NULL) 

/* This is a generalized handle, should always be typeless */
typedef HANDLE WDFOBJECT, *PWDFOBJECT;

/* Core Handles */
DECLARE_HANDLE(WDFDRIVER);
DECLARE_HANDLE(WDFDEVICE);

DECLARE_HANDLE(WDFWMIPROVIDER);
DECLARE_HANDLE(WDFWMIINSTANCE);

DECLARE_HANDLE(WDFQUEUE);
DECLARE_HANDLE(WDFREQUEST);
DECLARE_HANDLE(WDFFILEOBJECT);
DECLARE_HANDLE(WDFDPC);
DECLARE_HANDLE(WDFTIMER);
DECLARE_HANDLE(WDFWORKITEM);
DECLARE_HANDLE(WDFINTERRUPT);

/* Synch and Lock Handles */
DECLARE_HANDLE(WDFWAITLOCK);
DECLARE_HANDLE(WDFSPINLOCK);

DECLARE_HANDLE(WDFMEMORY);
DECLARE_HANDLE(WDFLOOKASIDE);

/* I/O Targets for Different Busses */
DECLARE_HANDLE(WDFIOTARGET);
DECLARE_HANDLE(WDFUSBDEVICE);
DECLARE_HANDLE(WDFUSBINTERFACE);
DECLARE_HANDLE(WDFUSBPIPE);

/* DMA handles */
DECLARE_HANDLE(WDFDMAENABLER);
DECLARE_HANDLE(WDFDMATRANSACTION);
DECLARE_HANDLE(WDFCOMMONBUFFER);

/* Support Handles */
DECLARE_HANDLE(WDFKEY);
DECLARE_HANDLE(WDFSTRING);
DECLARE_HANDLE(WDFCOLLECTION);
DECLARE_HANDLE(WDFCHILDLIST);

DECLARE_HANDLE(WDFIORESREQLIST);
DECLARE_HANDLE(WDFIORESLIST);
DECLARE_HANDLE(WDFCMRESLIST);

#endif /* NTDDI_VERSION >= NTDDI_WIN2K */
#endif /* _WDFTYPES_H_ */
