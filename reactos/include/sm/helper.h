#if !defined(INCLUDE_SM_HELPER_H)
#define INCLUDE_SM_HELPER_H

/* $Id$ */

/* smdll/connect.c */
NTSTATUS STDCALL
SmConnectApiPort (IN      PUNICODE_STRING  pSbApiPortName  OPTIONAL,
		  IN      HANDLE           hSbApiPort      OPTIONAL,
		  IN      DWORD            dwSubsystem     OPTIONAL, /* pe.h */
		  IN OUT  PHANDLE          phSmApiPort);
/* smdll/compses.c */
NTSTATUS STDCALL
SmCompleteSession (IN     HANDLE  hSmApiPort,
		   IN     HANDLE  hSbApiPort,
		   IN     HANDLE  hApiPort);
/* smdll/execpgm.c */
NTSTATUS STDCALL
SmExecuteProgram (IN     HANDLE           hSmApiPort,
		  IN     PUNICODE_STRING  Pgm
		  );

#endif /* ndef INCLUDE_SM_HELPER_H */
