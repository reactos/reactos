/*
 * wdfstatus.h
 *
 * Windows Driver Framework - WDFSATUS Value Definitions
 *
 * This file is part of the ReactOS wdf package.
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
#ifndef _WDFSTATUS_H_
#define _WDFSTATUS_H_

#define FACILITY_DRIVER_FRAMEWORK    0x20

/* Values are 32 bit laid out as follows:
    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
    1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
	______________________________________________________________
    | Sev | C | R |         Facility         |         Code      |
    ______________________________________________________________
	
    Sev is the severity level
	    00 - Success / Good
	    01 - Informational Code
	    10 - Warnign
	    11 - Error
		
    C - Customer code flag

    R - Reserved bit

    Facility - Facility code
	
    Code - Is the facility's code
	
*/


/* Severity Codes */
#define STATUS_SEVERITY_WARNING        0x2
#define STATUS_SEVERITY_SUCCESS        0x0
#define STATUS_SEVERITY_INFORMATIONAL  0x1
#define STATUS_SEVERITY_ERROR          0x3


/* 
Message ID: STATUS_WDF_QUEUED
Message Text: 
Request queued internally, but not submitted to target yet
*/
#define STATUS_WDF_QUEUED        ((NTSTATUS)0x40200000L)

/* 
Message ID: STATUS_WDF_POSTED
Message Text: 
Posted to worker thread
*/
#define STATUS_WDF_POSTED        ((NTSTATUS)0x40200002L)

/* 
Message ID: STATUS_WDF_NO_PACKAGE
Message Text: 
No package of selected type defined
*/
#define STATUS_WDF_NO_PACKAGE        ((NTSTATUS)0xC0200200L)

/* 
Message ID: STATUS_WDF_INTERNAL_ERROR
Message Text: 
Internal error has occured
*/
#define STATUS_WDF_INTERNAL_ERROR        ((NTSTATUS)0xC0200201L)

/* 
Message ID: STATUS_WDF_PAUSED
Message Text: 
Object in paused state
*/
#define STATUS_WDF_PAUSED        ((NTSTATUS)0xC0200203L)

/* 
Message ID: STATUS_WDF_BUSY
Message Text: 
Object busy with previous requests
*/
#define STATUS_WDF_BUSY        ((NTSTATUS)0xC0200204L)

/* 
Message ID: STATUS_WDF_IO_TIMEOUT_NOT_SENT
Message Text: 
Device was removed and request was never sent
*/
#define STATUS_WDF_IO_TIMEOUT_NOT_SENT        ((NTSTATUS)0xC0200205L)

/*
Message ID: STATUS_WDF_REQUEST_ALREADY_PENDING
Message Text: 
Request has already been submitted
*/
#define STATUS_WDF_REQUEST_ALREADY_PENDING        ((NTSTATUS)0xC0200207L)

/* 
Message ID: STATUS_WDF_REQUEST_INVALID_STATE
Message Text: 
WDF_OBJECT_ATTRIBUTES was invalid
*/
#define STATUS_WDF_REQUEST_INVALID_STATE        ((NTSTATUS)0xC0200209L)

/* 
Message ID: STATUS_WDF_TOO_FRAGMENTED
Message Text: 
Request has mode SCATTER_GATHER_ELEMENTS than object's MaximumFragments
*/
#define STATUS_WDF_TOO_FRAGMENTED        ((NTSTATUS)0xC020020AL)

/* 
Message ID: STATUS_WDF_NO_CALLBACK
Message Text: 
Callback not registered
*/
#define STATUS_WDF_NO_CALLBACK        ((NTSTATUS)0xC020020BL)

/* 
Message ID: STATUS_WDF_INCOMPATIBLE_EXECUTION_LEVEL
Message Text: 
Does not support this WDF_EXECUTION_LEVEL
*/
#define STATUS_WDF_INCOMPATIBLE_EXECUTION_LEVEL        ((NTSTATUS)0xC020020CL)

/* 
Message ID: STATUS_WDF_PARENT_ALREADY_ASSIGNED
Message Text: 
Parent already assigned
*/
#define STATUS_WDF_PARENT_ALREADY_ASSIGNED        ((NTSTATUS)0xC020020DL)

/* 
Message ID: STATUS_WDF_PARENT_IS_SELF
Message Text: 
Parent can't be self
*/
#define STATUS_WDF_PARENT_IS_SELF        ((NTSTATUS)0xC020020EL)

/* 
Message ID: STATUS_WDF_PARENT_ASSIGNMENT_NOT_ALLOWED
Message Text: 
Can't have a driver specified parrent
*/
#define STATUS_WDF_PARENT_ASSIGNMENT_NOT_ALLOWED        ((NTSTATUS)0xC020020FL)

/* 
Message ID: STATUS_WDF_SYNCHRONIZATION_SCOPE_INVALID
Message Text: 
Can't have a driver specified parrent
*/
#define STATUS_WDF_SYNCHRONIZATION_SCOPE_INVALID        ((NTSTATUS)0xC0200210L)

/* 
Message ID: STATUS_WDF_EXECUTION_LEVEL_INVALID
Message Text: 
Does not support this WDF_EXECUTION_LEVEL
*/
#define STATUS_WDF_EXECUTION_LEVEL_INVALID        ((NTSTATUS)0xC0200211L)

/* 
Message ID: STATUS_WDF_PARENT_NOT_SPECIFIED
Message Text: 
Parent object not specified in WDF_OBJECT_ATTRIBUTES
*/
#define STATUS_WDF_PARENT_NOT_SPECIFIED        ((NTSTATUS)0xC0200212L)


#endif /* _WDFSTATUS_H_ */
