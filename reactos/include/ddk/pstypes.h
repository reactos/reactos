#ifndef __INCLUDE_DDK_PSTYPES_H
#define __INCLUDE_DDK_PSTYPES_H

#include <ntos/ps.h>

#include <ntos/tss.h>
#include <napi/teb.h>

#ifndef TLS_MINIMUM_AVAILABLE
#define TLS_MINIMUM_AVAILABLE 	(64)
#endif
#ifndef TLS_OUT_OF_INDEXES
#define TLS_OUT_OF_INDEXES		0xFFFFFFFF
#endif
#ifndef MAX_PATH
#define MAX_PATH 	(260)
#endif

struct _EPROCESS;
struct _KPROCESS;
struct _ETHREAD;
struct _KTHREAD;

typedef struct _KTHREAD *PKTHREAD, *PRKTHREAD;

typedef NTSTATUS STDCALL_FUNC
(*PKSTART_ROUTINE)(PVOID StartContext);

typedef VOID STDCALL_FUNC
(*PCREATE_PROCESS_NOTIFY_ROUTINE)(HANDLE ParentId,
				  HANDLE ProcessId,
				  BOOLEAN Create);

typedef VOID STDCALL_FUNC
(*PCREATE_THREAD_NOTIFY_ROUTINE)(HANDLE ProcessId,
				 HANDLE ThreadId,
				 BOOLEAN Create);

typedef NTSTATUS STDCALL_FUNC
(*PW32_PROCESS_CALLBACK)(struct _EPROCESS *Process,
			 BOOLEAN Create);

typedef NTSTATUS STDCALL_FUNC
(*PW32_THREAD_CALLBACK)(struct _ETHREAD *Thread,
			BOOLEAN Create);

typedef struct _STACK_INFORMATION
{
  PVOID BaseAddress;
  PVOID UpperAddress;
} STACK_INFORMATION, *PSTACK_INFORMATION;

typedef ULONG THREADINFOCLASS;

struct _KPROCESS;

#define LOW_PRIORITY (0)
#define LOW_REALTIME_PRIORITY (16)
#define HIGH_PRIORITY (31)
#define MAXIMUM_PRIORITY (32)

#endif /* __INCLUDE_DDK_PSTYPES_H */
