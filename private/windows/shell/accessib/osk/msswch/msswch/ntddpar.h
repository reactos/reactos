/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1990-1993  Microsoft Corporation

Module Name:

    ntddpar.h

Abstract:

    This is the include file that defines all constants and types for
    accessing the Parallel device.

Author:

    Steve Wood (stevewo) 27-May-1990

Revision History:

--*/

//
// NtDeviceIoControlFile IoControlCode values for this device.
//
// Warning:  Remember that the low two bits of the code specify how the
//           buffers are passed to the driver!
//

#define IOCTL_PAR_BASE                  FILE_DEVICE_PARALLEL_PORT
#define IOCTL_PAR_QUERY_INFORMATION     CTL_CODE(FILE_DEVICE_PARALLEL_PORT,1,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_PAR_SET_INFORMATION       CTL_CODE(FILE_DEVICE_PARALLEL_PORT,2,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_PAR_QUERY_DEVICE_ID       CTL_CODE(FILE_DEVICE_PARALLEL_PORT,3,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_PAR_QUERY_DEVICE_ID_SIZE  CTL_CODE(FILE_DEVICE_PARALLEL_PORT,4,METHOD_BUFFERED,FILE_ANY_ACCESS)

//
// NtDeviceIoControlFile InputBuffer/OutputBuffer record structures for
// this device.
//

typedef struct _PAR_QUERY_INFORMATION{
       UCHAR Status;
} PAR_QUERY_INFORMATION, *PPAR_QUERY_INFORMATION;

typedef struct _PAR_SET_INFORMATION{
       UCHAR Init;
} PAR_SET_INFORMATION, *PPAR_SET_INFORMATION;

#define PARALLEL_INIT            0x1
#define PARALLEL_AUTOFEED        0x2
#define PARALLEL_PAPER_EMPTY     0x4
#define PARALLEL_OFF_LINE        0x8
#define PARALLEL_POWER_OFF       0x10
#define PARALLEL_NOT_CONNECTED   0x20
#define PARALLEL_BUSY            0x40
#define PARALLEL_SELECTED        0x80

//
// This is the structure returned by IOCTL_PAR_QUERY_DEVICE_ID_SIZE.
//

typedef struct _PAR_DEVICE_ID_SIZE_INFORMATION {
    ULONG   DeviceIdSize;
} PAR_DEVICE_ID_SIZE_INFORMATION, *PPAR_DEVICE_ID_SIZE_INFORMATION;
