#ifndef _MRSW_H
#define _MRSW_H

//
// structs
//
typedef struct _MRSW {
    CHAR aszObjectName[MAX_PATH];
    HANDLE hSemaphoreNumReaders;
    HANDLE hMutexWrite;
    HANDLE hEventTryToWrite;
} MRSW, *PMRSW;


//
// prototypes
//
STDAPI_(PMRSW) MRSW_Create(LPCTSTR pszObjectName);
STDAPI_(BOOL) MRSW_EnterRead(PMRSW pmwsr);
STDAPI_(BOOL) MRSW_LeaveRead(PMRSW pmwsr);
STDAPI_(BOOL) MRSW_EnterWrite(PMRSW pmwsr);
STDAPI_(BOOL) MRSW_LeaveWrite(PMRSW pmwsr);
STDAPI_(BOOL) MRSW_Destroy(PMRSW pmwsr);


#endif // _MRSW_H