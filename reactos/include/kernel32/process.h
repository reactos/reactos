
#ifndef __PEB_DEFINED
#define __PEB_DEFINED

#include "heap.h"

typedef void* HANDLE_TABLE;

typedef struct _pPebInfo {
	LPWSTR		lpCommandLine;
	DWORD		cb;
	HANDLE		hStdInput; //18
    	HANDLE		hStdOutput;  
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
	void			*HeapIndex;
	DWORD			dwTlsBits[2]; // tls in use bits 
	WORD			NumberOfProcessors;
	WORD			NtGlobalFlag;
	DWORD			dwCriticalSectionTimeout;
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
	
} NT_PEB;	

NT_PEB *GetPeb()
{
	return NULL;
}

#endif
