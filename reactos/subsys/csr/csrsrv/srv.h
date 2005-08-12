#if !defined(_INCLUDE_CSR_CSRSRV_SRV_H)
#define _INCLUDE_CSR_CSRSRV_SRV_H

/* PSDK/NDK Headers */
#include <stdio.h>
#include <windows.h>

#define NTOS_MODE_USER
#include <ndk/ntndk.h>

/* CSR Headers */
#include <csr/server.h>

/* Maximum number of hosted servers, included the one in csrsrv.dll */
#define CSR_SERVER_DLL_MAX 4

typedef enum {
	CSRSST_NONE=0,
	CSRSST_TEXT,
	CSRSST_WINDOWS,
	CSRSST_MAX
	
} CSR_SUBSYSTEM_TYPE, * PCSR_SUBSYSTEM_TYPE;

typedef struct _CSR_SERVER_DLL
{
	USHORT ServerIndex;
	USHORT Unused;
	UNICODE_STRING DllName;
	UNICODE_STRING DllEntryPoint;
} CSR_SERVER_DLL, * PCSR_SERVER_DLL;

/* dllmain.c */
extern HANDLE CsrSrvDllHandle;

/* process.c */

/* server.c */
typedef struct
{
	struct {
		UNICODE_STRING      Root;
		HANDLE              RootHandle;
	} NameSpace;
	CSR_SUBSYSTEM_TYPE  SubSystemType;
	struct {
		USHORT              RequestCount;
		USHORT              MaxRequestCount;
	} Threads;
	struct {
	 	BOOL                ProfileControl;
		BOOL                Windows;
		BOOL                Sessions;
	} Flag;
	USHORT PortSharedSectionSize;
	struct {
		USHORT InteractiveDesktopHeapSize;
		USHORT NonInteractiveDesktopHeapSize;
	} Heap;
} CSRSRV_OPTION, * PCSRSRV_OPTION;

extern CSRSRV_OPTION CsrSrvOption;
extern HANDLE CsrSrvApiPortHandle;

NTSTATUS STDCALL CsrSrvRegisterServerDll (PCSR_SERVER_DLL);
NTSTATUS STDCALL CsrSrvBootstrap (VOID);

/* session.c */
NTSTATUS STDCALL CsrSrvInitializeSession (VOID);


#endif /* !def _INCLUDE_CSR_CSRSRV_SRV_H */
