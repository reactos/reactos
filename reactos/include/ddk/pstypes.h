#ifndef __INCLUDE_DDK_PSTYPES_H
#define __INCLUDE_DDK_PSTYPES_H

#include <kernel32/atom.h>
#include <internal/hal.h>

#ifndef TLS_MINIMUM_AVAILABLE
#define TLS_MINIMUM_AVAILABLE 	(64)
#endif
#ifndef MAX_PATH
#define MAX_PATH 	(260)
#endif

typedef NTSTATUS (*PKSTART_ROUTINE)(PVOID StartContext);

typedef struct _STACK_INFORMATION 
{
	PVOID 	BaseAddress;
	PVOID	UpperAddress;
} STACK_INFORMATION, *PSTACK_INFORMATION;

typedef struct linux_sigcontext {
        int     sc_gs;
        int     sc_fs;
        int     sc_es;
        int     sc_ds;
        int     sc_edi;
        int     sc_esi;
        int     sc_ebp;
        int     sc_esp;
        int     sc_ebx;
        int     sc_edx;
        int     sc_ecx;
        int     sc_eax;
        int     sc_trapno;
        int     sc_err;
        int     sc_eip;
        int     sc_cs;
        int     sc_eflags;
        int     sc_esp_at_signal;
        int     sc_ss;
        int     sc_387;
        int     sc_mask;
        int     sc_cr2;
} TRAP_FRAME, *PTRAP_FRAME;

typedef ULONG THREADINFOCLASS;

typedef struct _STARTUPINFOW { 
  DWORD   cb; 
  WCHAR	  WindowTitle[MAX_PATH];
  WCHAR	  ImageFile[MAX_PATH];	
  WCHAR	  CommandLine[MAX_PATH];
  WCHAR	  DllPath[MAX_PATH];
  LPWSTR  Reserved[MAX_PATH]; 
  LPWSTR  Desktop[MAX_PATH]; 
  LPWSTR  Title[MAX_PATH]; 
  DWORD   dwX; 
  DWORD   dwY; 
  DWORD   dwXSize; 
  DWORD   dwYSize; 
  DWORD   dwXCountChars; 
  DWORD   dwYCountChars; 
  DWORD   dwFillAttribute; 
  DWORD   dwFlags; 
  WORD    wShowWindow; 
  WORD    cbReserved2; 
  unsigned char * lpReserved2; 
  HANDLE  hStdInput; 
  HANDLE  hStdOutput; 
  HANDLE  hStdError; 
} PROCESSINFOW, *PPROCESSINFOW; 



typedef struct _LDR {
	UCHAR	Initialized;
	UCHAR	InInitializationOrderModuleList;
	PVOID	InLoadOrderModuleList;
	PVOID	InMemoryOrderModuleList;
} LDR, *PLDR;


typedef struct _NT_PEB
{
	UCHAR			InheritedAddressSpace;
	UCHAR			ReadImageFileExecOptions;
	UCHAR			BeingDebugged;
	LONG			ImageBaseAddress; 
	LDR			Ldr;

	WORD			NumberOfProcessors;
	WORD			NtGlobalFlag;

	PPROCESSINFOW		StartupInfo;
	HANDLE			ProcessHeap; 
	ATOMTABLE		LocalAtomTable;
	LPCRITICAL_SECTION	CriticalSection;
	DWORD			CriticalSectionTimeout; 
	WORD			MajorVersion; 
	WORD			MinorVersion; 
	WORD			BuildNumber;  
	WORD			PlatformId;	
} NT_PEB, *PNT_PEB;	

typedef struct _CLIENT_ID 
{
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;
 
typedef struct _NT_TIB {
    struct _EXCEPTION_REGISTRATION_RECORD *ExceptionList;
    PVOID StackBase;
    PVOID StackLimit;
    PVOID SubSystemTib;
    union {
        PVOID FiberData;
        ULONG Version;
    } Fib;
    PVOID ArbitraryUserPointer;
    struct _NT_TIB *Self;
} NT_TIB, *PNT_TIB;

typedef struct _NT_TEB
{
	NT_TIB			Tib; 
	CLIENT_ID		Cid;
	HANDLE			RPCHandle;
	PVOID	 		TlsData[TLS_MINIMUM_AVAILABLE];
	DWORD 			dwTlsIndex;
	NT_PEB			*Peb;   
	DWORD			LastErrorCode;
	NTSTATUS		LastStatusValue; 
	DWORD			LockCount;
	UCHAR			HardErrorMode;
} NT_TEB;

typedef struct _KTHREAD 
{
	DISPATCHER_HEADER 	DispatcherHeader;
	TIME			ElapsedTime;
	TIME			KernelTime;
	TIME			UserTime;
	STACK_INFORMATION	StackInformation;
	PVOID			ServiceDescriptorTable;  // points to KeServiceDescriptorTable
	KAFFINITY		Affinity;
	KPRIORITY		CurrentPriority;
	KPRIORITY		BasePriority;
	ULONG			Quantum;
	UCHAR			ThreadState; //Thread state is a typeless enum, otherwise it should be const integer
	ULONG			FreezeCount;
	LONG			SuspendCount;
	PTRAP_FRAME		TrapFrame; 
	PVOID			*Tls;
	KWAIT_BLOCK		WaitBlock[4];	
	struct _KMUTANT*	MutantList;
	PLIST_ENTRY		ApcList;
	UCHAR			KernelApcDisable;
	KTIMER			TimerBlock;
	KDEVICE_QUEUE		DeviceQueue;
	NT_TEB*			Teb;
   
        /*
	 * PURPOSE: CPU state
	 * NOTE: I have temporarily added this to give somewhere to store
	 * cpu state when the thread isn't running
	 */
        hal_thread_state                   Context;
        LIST_ENTRY Entry;
        ULONG LastTick;
} KTHREAD, *PKTHREAD;


// According to documentation the stack should have a commited [ 1 page ] and
// a reserved part [ 1 M ] but can be specified otherwise in the image file.

typedef struct _INITIAL_TEB {
	PVOID StackBase;
    	PVOID StackLimit;
	PVOID StackCommit;
	PVOID StackCommitMax;
	PVOID StackReserved;
} INITIAL_TEB, *PINITIAL_TEB;






// TopLevelIrp can be one of the following values:
// FIXME I belong somewhere else

#define 	FSRTL_FSP_TOP_LEVEL_IRP			(0x01)
#define 	FSRTL_CACHE_TOP_LEVEL_IRP		(0x02)
#define 	FSRTL_MOD_WRITE_TOP_LEVEL_IRP		(0x03)
#define		FSRTL_FAST_IO_TOP_LEVEL_IRP		(0x04)
#define		FSRTL_MAX_TOP_LEVEL_IRP_FLAG		(0x04)

typedef struct _TOP_LEVEL_IRP
{
	PIRP TopLevelIrp;
	ULONG TopLevelIrpConst;
} TOP_LEVEL_IRP;

typedef struct _ETHREAD {
	KTHREAD			Tcb;
	TIME			CreateTime;
	TIME			ExitTime;
	NTSTATUS		ExitStatus;
	LIST_ENTRY		PostBlockList;
	LIST_ENTRY		TerminationPortList;  
	ULONG			ActiveTimerListLock;
	PVOID			ActiveTimerListHead;
	CLIENT_ID		Cid;
	PLARGE_INTEGER		LpcReplySemaphore;
	PVOID			LpcReplyMessage;
	PLARGE_INTEGER		LpcReplyMessageId;
	PVOID			ImpersonationInfo;
	LIST_ENTRY		IrpList; //
	TOP_LEVEL_IRP		TopLevelIrp;
	ULONG			ReadClusterSize;
	UCHAR			ForwardClusterOnly;
	UCHAR			DisablePageFaultClustering;
	UCHAR			DeadThread;
	UCHAR			HasTerminated;
	ACCESS_MASK		GrantedAccess;
	struct _EPROCESS*	ThreadsProcess;
	PKSTART_ROUTINE		StartAddress;
	LPTHREAD_START_ROUTINE  Win32StartAddress; // Should Specify a win32 start func
	UCHAR 			LpcExitThreadCalled;
	UCHAR 			HardErrorsAreDisabled;

   
   /*
    * Added by David Welch (welch@cwcom.net)
    */
   struct _EPROCESS* OldProcess;

} ETHREAD, *PETHREAD;


typedef struct _KPROCESS 
{
   DISPATCHER_HEADER 	DispatcherHeader;
   PVOID		PageTableDirectory; // FIXME: I shoud point to a PTD
   TIME			ElapsedTime;
   TIME			KernelTime;
   TIME			UserTime;
   LIST_ENTRY		InMemoryList;  
   LIST_ENTRY		SwappedOutList;   	
   KSPIN_LOCK		SpinLock;
   KAFFINITY		Affinity;
   ULONG		StackCount;
   KPRIORITY		BasePriority;
   ULONG		DefaultThreadQuantum;
   UCHAR		ProcessState;
   ULONG		ThreadSeed;
   UCHAR		DisableBoost;
   
   /*
    * Added by David Welch (welch@mcmail.com)
    */
   LIST_ENTRY           MemoryAreaList;
   HANDLE_TABLE         HandleTable;
} KPROCESS, *PKPROCESS;

typedef struct _EPROCESS
{
   KPROCESS Pcb;
} EPROCESS, *PEPROCESS;

#define PROCESS_STATE_TERMINATED (1)
#define PROCESS_STATE_ACTIVE     (2)

#endif /* __INCLUDE_DDK_PSTYPES_H */
