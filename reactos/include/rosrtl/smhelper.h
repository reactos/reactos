#ifndef _ROSRTL_SM_HELPER_H

/*** DATA TYPES ******************************************************/

#define SM_SB_NAME_MAX_LENGTH 120

#pragma pack(push,4)

/* SmConnectApiPort */
typedef struct _SM_CONNECT_DATA
{
  ULONG  Subsystem;
  WCHAR  SbName [SM_SB_NAME_MAX_LENGTH];

} SM_CONNECT_DATA, *PSM_CONNECT_DATA;

/* SmpConnectSbApiPort */
typedef struct _SB_CONNECT_DATA
{
  ULONG SmApiMax;
} SB_CONNECT_DATA, *PSB_CONNECT_DATA;

#pragma pack(pop)


/*** PROTOTYPES ******************************************************/


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

#endif /* ndef _ROSRTL_SM_HELPER_H */
