#if !defined(INCLUDE_SM_HELPER_H)
#define INCLUDE_SM_HELPER_H

/* $Id$ */

/* smlib/connect.c */
NTSTATUS STDCALL
SmConnectApiPort (IN      PUNICODE_STRING  pSbApiPortName  OPTIONAL,
		  IN      HANDLE           hSbApiPort      OPTIONAL,
		  IN      DWORD            dwSubsystem     OPTIONAL, /* pe.h */
		  IN OUT  PHANDLE          phSmApiPort);
/* smlib/compses.c */
NTSTATUS STDCALL
SmCompleteSession (IN     HANDLE  hSmApiPort,
		   IN     HANDLE  hSbApiPort,
		   IN     HANDLE  hApiPort);
/* smlib/execpgm.c */
NTSTATUS STDCALL
SmExecuteProgram (IN     HANDLE           hSmApiPort,
		  IN     PUNICODE_STRING  Pgm
		  );
/* smdll/query.c */
typedef enum {
	SM_BASE_INFORMATION
} SM_INFORMATION_CLASS, *PSM_INFORMATION_CLASS;

NTSTATUS STDCALL
SmQuery (IN      HANDLE                SmApiPort,
	 IN      SM_INFORMATION_CLASS  SmInformationClass,
	 IN OUT  PVOID                 Data,
	 IN OUT  PULONG                DataLength);

#endif /* ndef INCLUDE_SM_HELPER_H */
