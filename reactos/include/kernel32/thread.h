/*
This file contains a proposal for Thread Environment Block.
*/
#ifndef __TEB_DEFINED
#define __TEB_DEFINED


typedef struct _NT_TIB { 

    struct _EXCEPTION_REGISTRATION_RECORD *ExceptionList; 

    void *StackBase; 
    void *StackLimit; 
    void *SubSystemTib; 
    union { 
        void *FiberData; 
        DWORD Version; 
    };
    void *ArbitraryUserPointer; 
    struct _NT_TIB *Self; 

} NT_TIB; 





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


// should be an inline asm macro

NT_TEB *GetTeb()
{
	return NULL;
}

#endif


