/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

   Event.h

Abstract:


Author:

   Jeff Midkiff    (jeffmi)    23-Jul-96

Enviroment:


Revision History:

--*/

#ifndef __EVENT__
#define __EVENT__


#include "devioctl.h"

typedef struct _SET_EVENT
{
    HANDLE  hEvent;
    LARGE_INTEGER DueTime; // requested DueTime in 100-nanosecond units

} SET_EVENT, *PSET_EVENT;

#define SIZEOF_SETEVENT sizeof(SET_EVENT)


#define IOCTL_SET_EVENT \
   CTL_CODE( FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS )


#endif // __EVENT__
