#ifndef __INCLUDE_NAPI_WIN32_H
#define __INCLUDE_NAPI_WIN32_H

struct _DESKTOP_OBJECT;

typedef struct _WINSTATION_OBJECT
{   
  CSHORT Type;
  CSHORT Size;

  KSPIN_LOCK Lock;
  UNICODE_STRING Name;
  LIST_ENTRY DesktopListHead;
  PRTL_ATOM_TABLE AtomTable;
  PVOID HandleTable;
  struct _DESKTOP_OBJECT* ActiveDesktop;
  /* FIXME: Clipboard */
} WINSTATION_OBJECT, *PWINSTATION_OBJECT;

typedef struct _DESKTOP_OBJECT
{   
  CSHORT Type;
  CSHORT Size;
  LIST_ENTRY ListEntry;
  KSPIN_LOCK Lock;
  UNICODE_STRING Name;
  /* Pointer to the associated window station. */
  struct _WINSTATION_OBJECT *WindowStation;
  /* Head of the list of windows in this desktop. */
  LIST_ENTRY WindowListHead;
  /* Pointer to the active queue. */
  PVOID ActiveMessageQueue;
  /* Handle of the desktop window. */
  HANDLE DesktopWindow;
  HANDLE PrevActiveWindow;
} DESKTOP_OBJECT, *PDESKTOP_OBJECT;

typedef struct _W32THREAD
{
  PVOID MessageQueue;
  FAST_MUTEX WindowListLock;
  LIST_ENTRY WindowListHead;
  struct _DESKTOP_OBJECT *Desktop;
} __attribute__((packed)) W32THREAD, *PW32THREAD;

typedef struct _W32PROCESS
{
  FAST_MUTEX ClassListLock;
  LIST_ENTRY ClassListHead;
  struct _WINSTATION_OBJECT *WindowStation;
} W32PROCESS, *PW32PROCESS;

typedef struct _NCCALCSIZE_PARAMS { 
  RECT        rgrc[3]; 
  PWINDOWPOS  lppos; 
} NCCALCSIZE_PARAMS; 


PW32THREAD STDCALL
PsGetWin32Thread(VOID);
NTSTATUS STDCALL
PsCreateWin32Thread(PETHREAD Thread);
PW32PROCESS STDCALL
PsGetWin32Process(VOID);
NTSTATUS STDCALL
PsCreateWin32Process(PEPROCESS Process);
VOID STDCALL
PsEstablishWin32Callouts(PVOID Param1,
			 PVOID Param2,
			 PVOID Param3,
			 PVOID Param4,
			 ULONG W32ThreadSize,
			 ULONG W32ProcessSize);

#endif /* __INCLUDE_NAPI_WIN32_H */
