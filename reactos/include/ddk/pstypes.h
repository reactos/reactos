#ifndef __INCLUDE_DDK_PSTYPES_H
#define __INCLUDE_DDK_PSTYPES_H

#include <kernel32/heap.h>

typedef ULONG THREADINFOCLASS;

typedef struct _CLIENT_ID 
{
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

//typedef void* HEAP;
typedef void* HANDLE_TABLE;
typedef void* ATOMTABLE;

typedef struct _pPebInfo {
	LPWSTR		lpCommandLine;
	DWORD		cb;
	HANDLE		hStdInput; //18
    	HANDLE		hStdput;  
    	HANDLE 		hStdError; 
	LPWSTR		lpEnvironment;
	DWORD		dwX;
	DWORD		dwY;
    	DWORD		dwXSize;  
    	DWORD		dwYSize;  
    	DWORD		dwXCountChars;  
    	DWORD		dwYCountChars; 
    	DWORD		dwFillAttribute; 
    	DWORD		dwFlags; 
    	DWORD		wShowWindow; 
	LPTSTR		lpTitle;  
	LPTSTR		lpDesktop; 
	LPTSTR		reserved; 
	DWORD		cbReserved2; 
	LPTSTR		lpReserved1; 
} PEBINFO;

typedef struct _NT_PEB
{
	
	LONG			ImageBaseAddress; 
	DWORD			nActiveStdHandle;
	void			*HeapIndex;
	DWORD			dwTlsBits[2]; // tls in use bits 
	WORD			NumberOfProcessors;
	WORD			NtGlobalFlag;
	DWORD			dwCriticalSectionTime;
	DWORD			dwHeapReserve;
	DWORD			dwHeapCommit;
	DWORD			dwHeapDecommitFreeBlockThreshold;
	DWORD			dwNumberOfHeaps;
	DWORD			dwMaxiumNumberOfHeaps;
	PEBINFO			*pPebInfo; 
	HEAP			*pProcessHeap; 
	HANDLE_TABLE		htGDISharedHandleTable;
	ATOMTABLE		LocalAtomTable;
	CRITICAL_SECTION	*pCriticalSection; 
	WORD			wMajorVersion; 
	WORD			wMinorVersion; 
	WORD			wBuildNumber;  
	WORD			wPlatformId;	
} NT_PEB, *PPEB;


typedef struct _NT_TIB {
    struct _EXCEPTION_REGISTRATION_RECORD *ExceptionList;
    PVOID StackBase;
    PVOID StackLimit;
    PVOID SubSystemTib;
    union {
        PVOID FiberData;
        ULONG Version;
    } s;
    PVOID ArbitraryUserPointer;
    struct _NT_TIB *Self;
} NT_TIB, *PNT_TIB;

typedef struct _NT_TEB
{

	NT_TIB			Tib; 
	DWORD			dwProcessId;
	DWORD			dwThreadId;	
	HANDLE			hRPC;
	NT_PEB			*pPeb;   
	DWORD			dwErrCode; 
	WORD			nMutexCount;
	LCID  			Locale;
	//HQUEUE		MessageQueue
	DWORD	 		dwTlsIndex ;
	LPVOID	 		TlsData[512];
	
	
} NT_TEB;


typedef NT_TEB *PINITIAL_TEB;

typedef struct _EPROCESS
{
} EPROCESS, *PEPROCESS;

//typedef KTHREAD ETHREAD, *PETHREAD;

#if ETHREAD_NOT_THE_SAME_AS_KTHREAD
typedef struct _ETHREAD
{
   EPROCESS* Process;
} ETHREAD, *PETHREAD;

/*
 * PURPOSE: Thread object
 */
typedef struct 
{
   CSHORT Type;
   CSHORT Size;
   
   /*
    * PURPOSE: Entry in the linked list of threads
    */
   LIST_ENTRY Entry;
   
   /*
    * PURPOSE: Current state of the thread
    */
   ULONG State;
   
   /*
    * PURPOSE: Priority modifier of the thread
    */
   ULONG Priority;
   
   /*
    * PURPOSE: Pointer to our parent process
    */
//   PEPROCESS Parent;
   
   /*
    * PURPOSE: Handle of our parent process
    */
   HANDLE ParentHandle;
   
   /*
    * PURPOSE: Not currently used 
    */
   ULONG AffinityMask;
   
   /*
    * PURPOSE: Saved thread context
    */
   hal_thread_state context;
   
} THREAD_OBJECT, *PTHREAD_OBJECT;
#endif

#endif /* __INCLUDE_DDK_PSTYPES_H */
