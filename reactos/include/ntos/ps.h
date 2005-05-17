/*
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         include/ntos/ps.h
 * PURPOSE:      Process/thread declarations used by all the parts of the 
 *               system
 * PROGRAMMER:   David Welch <welch@cwcom.net>
 * UPDATE HISTORY: 
 *               27/06/00: Created
 */


#ifndef __INCLUDE_PS_H
#define __INCLUDE_PS_H

#define THREAD_READ			(0x020048L)
#define THREAD_WRITE			(0x020037L)
#define THREAD_EXECUTE			(0x120000L)

#define PROCESS_READ			(0x020410L)
#define PROCESS_WRITE			(0x020bebL)
#define PROCESS_EXECUTE			(0x120000L)

/* Thread priorities */
#define THREAD_PRIORITY_BELOW_NORMAL	(-1)
#define THREAD_PRIORITY_IDLE	(-15)
#define THREAD_PRIORITY_LOWEST	(-2)

/* Process priority classes */
#define PROCESS_PRIORITY_CLASS_HIGH	(4) /* FIXME */
#define PROCESS_PRIORITY_CLASS_IDLE	(0) /* FIXME */
#define PROCESS_PRIORITY_CLASS_NORMAL	(2) /* FIXME */
#define PROCESS_PRIORITY_CLASS_REALTIME	(5) /* FIXME */
#define PROCESS_PRIORITY_CLASS_BELOW_NORMAL (1) /* FIXME */
#define PROCESS_PRIORITY_CLASS_ABOVE_NORMAL (3) /* FIXME */

/* Job access rights */
#define JOB_OBJECT_ASSIGN_PROCESS	(1)
#define JOB_OBJECT_SET_ATTRIBUTES	(2)
#define JOB_OBJECT_QUERY	(4)
#define JOB_OBJECT_TERMINATE	(8)
#define JOB_OBJECT_SET_SECURITY_ATTRIBUTES	(16)
#define JOB_OBJECT_ALL_ACCESS	(STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|31)

#ifndef __USE_W32API

/* Thread access rights */
#define THREAD_TERMINATE		(0x0001L)
#define THREAD_SUSPEND_RESUME		(0x0002L)
#define THREAD_ALERT (0x0004L)
#define THREAD_GET_CONTEXT		(0x0008L)
#define THREAD_SET_CONTEXT		(0x0010L)
#define THREAD_SET_INFORMATION		(0x0020L)
#define THREAD_QUERY_INFORMATION	(0x0040L)
#define THREAD_SET_THREAD_TOKEN		(0x0080L)
#define THREAD_IMPERSONATE		(0x0100L)
#define THREAD_DIRECT_IMPERSONATION	(0x0200L)

#define THREAD_ALL_ACCESS		(0x1f03ffL)

/* Process access rights */
#define PROCESS_TERMINATE		(0x0001L)
#define PROCESS_CREATE_THREAD		(0x0002L)
#define PROCESS_SET_SESSIONID		(0x0004L)
#define PROCESS_VM_OPERATION		(0x0008L)
#define PROCESS_VM_READ			(0x0010L)
#define PROCESS_VM_WRITE		(0x0020L)
#define PROCESS_DUP_HANDLE		(0x0040L)
#define PROCESS_CREATE_PROCESS		(0x0080L)
#define PROCESS_SET_QUOTA		(0x0100L)
#define PROCESS_SET_INFORMATION		(0x0200L)
#define PROCESS_QUERY_INFORMATION	(0x0400L)
#define PROCESS_SUSPEND_RESUME		(0x0800L)

#define PROCESS_ALL_ACCESS		(0x1f0fffL)

/* Thread priorities */
#define THREAD_PRIORITY_ABOVE_NORMAL	(1)
#define THREAD_PRIORITY_HIGHEST	(2)
#define THREAD_PRIORITY_NORMAL	(0)
#define THREAD_PRIORITY_TIME_CRITICAL	(15)
#define THREAD_PRIORITY_ERROR_RETURN	(2147483647)

/* CreateProcess */
#define CREATE_DEFAULT_ERROR_MODE	(67108864)
#define CREATE_NEW_CONSOLE	(16)
#define CREATE_NEW_PROCESS_GROUP	(512)
#define CREATE_SEPARATE_WOW_VDM	(2048)
#define CREATE_SUSPENDED	(4)
#define CREATE_UNICODE_ENVIRONMENT	(1024)
#define DEBUG_PROCESS	(1)
#define DEBUG_ONLY_THIS_PROCESS	(2)
#define DETACHED_PROCESS	(8)
#define HIGH_PRIORITY_CLASS	(128)
#define IDLE_PRIORITY_CLASS	(64)
#define NORMAL_PRIORITY_CLASS	(32)
#define REALTIME_PRIORITY_CLASS	(256)
#define BELOW_NORMAL_PRIORITY_CLASS (16384)
#define ABOVE_NORMAL_PRIORITY_CLASS (32768)

/* ResumeThread / SuspendThread */
#define MAXIMUM_SUSPEND_COUNT	(0x7f)

#endif /* !__USE_W32API */

#ifdef __NTOSKRNL__
#ifdef __GNUC__
extern struct _EPROCESS* EXPORTED PsInitialSystemProcess;
extern POBJECT_TYPE EXPORTED      PsProcessType;
extern POBJECT_TYPE EXPORTED      PsThreadType;
#else /* __GNUC__ */
/* Microsft-style */
extern EXPORTED struct _EPROCESS* PsInitialSystemProcess;
extern EXPORTED POBJECT_TYPE      PsProcessType;
extern EXPORTED POBJECT_TYPE      PsThreadType;
#endif /* __GNUC__ */
#else /* __NTOSKRNL__ */
#ifdef __GNUC__ // robd
extern struct _EPROCESS* IMPORTED PsInitialSystemProcess;
extern POBJECT_TYPE IMPORTED PsProcessType;
extern POBJECT_TYPE IMPORTED PsThreadType;
#endif
#endif

#endif /* __INCLUDE_PS_H */
