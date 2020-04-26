/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    wdf.h

Abstract:

    Main header file for Windows Driver Frameworks

Environment:

    kernel mode only

Revision History:

--*/

#ifndef _WDF_H_
#define _WDF_H_



#ifdef __cplusplus
  #define WDF_EXTERN_C       extern "C"
  #define WDF_EXTERN_C_START extern "C" {
  #define WDF_EXTERN_C_END   }
#else
  #define WDF_EXTERN_C
  #define WDF_EXTERN_C_START
  #define WDF_EXTERN_C_END
#endif

WDF_EXTERN_C_START




typedef VOID (*WDFFUNC) (VOID);
extern WDFFUNC WdfFunctions [];

#ifndef __drv_dispatchType
#include <driverspecs.h>
#endif

__drv_Mode_impl(KMDF_INCLUDED)

// Basic definitions
#include "wdftypes.h"
#include "wdfglobals.h"
#include "wdffuncenum.h"
#include "wdfstatus.h"
#include "wdfassert.h"
#include "wdfverifier.h"
#include "wdfpool.h"

// generic object
#include "wdfobject.h"

// Synchronization
#include "wdfsync.h"

#include "wdfcore.h"

#include "wdfdriver.h"

// Objects
#include "WdfQueryInterface.h"
#include "wdfmemory.h"
#include "wdfchildlist.h"
#include "wdffileobject.h"
#include "wdfdevice.h"
#include "wdfcollection.h"
#include "wdfdpc.h"
#include "wdftimer.h"
#include "wdfworkitem.h"
#include "wdfinterrupt.h"
#include "wdfresource.h"

// I/O
#include "wdfrequest.h"
#include "wdfiotarget.h"
#include "wdfio.h"

// particular device types
#include "wdffdo.h"
#include "wdfpdo.h"
#include "wdfcontrol.h"

#include "WdfWMI.h"

#include "wdfstring.h"
#include "wdfregistry.h"

// Dma
#include "wdfDmaEnabler.h"
#include "wdfDmaTransaction.h"
#include "wdfCommonBuffer.h"

#include "wdfbugcodes.h"
#include "wdfroletypes.h"

WDF_EXTERN_C_END

#endif // _WDF_H_

