/*++

Module Name:

    ktracep.h

Abstract:

	Private include for ktrace.c and ktrace.mac to use.

Author:

    Roy D'Souza (rdsouza@gomez.sc.intel.com) 22-April-1996

Environment:

    User or Kernel mode.

Revision History:

--*/

/* The number of slots in the trace */
#define KTRACE_LOG_SIZE 0x400

/* The record size in bytes
IF YOU MAKE A CHANGE TO THE KTRACE TYPEDEF IN KTRACE.C
YOU NEED TO RE-CALCULATE THE SIZE IN BYTES AND UPDATE THE
FOLLOWING: */
#define RECORD_SIZE_IN_BYTES 0x70

/* The maximum value a module ID can take */
#define MAX_MODULE_ID 0x80000000

/* The maximum value a message type can take */
#define MAX_MESSAGE_TYPE 0x10000

/* The maximum value a message index can take */
#define MAX_MESSAGE_INDEX 0x10000

/***********************************************************************
Message Types:
***********************************************************************/

#define MESSAGE_INFORMATION 0x1
#define MESSAGE_WARNING     0x2
#define MESSAGE_ERROR       0x4

/***********************************************************************
Module IDs:
***********************************************************************/

#define MODULE_INIT  0x1
#define MODULE_KE    0x2
#define MODULE_EX    0x4
#define MODULE_MM    0x8
#define MODULE_LPC   0x10
#define MODULE_SE    0x20
#define MODULE_TDI   0x40
#define MODULE_RTL   0x80
#define MODULE_PO    0x100
#define MODULE_PNP   0x200

#define DRIVER_1    0x10000000
#define DRIVER_2    0x20000000
#define DRIVER_3    0x40000000
#define DRIVER_4    0x80000000


