/*
 * services.h
 */

typedef struct _SERVICE
{
  LIST_ENTRY ServiceListEntry;
  UNICODE_STRING ServiceName;
  UNICODE_STRING RegistryPath;
  UNICODE_STRING ServiceGroup;

  ULONG Start;
  ULONG Type;
  ULONG ErrorControl;
  ULONG Tag;

  ULONG CurrentState;
  ULONG ControlsAccepted;
  ULONG Win32ExitCode;
  ULONG ServiceSpecificExitCode;
  ULONG CheckPoint;
  ULONG WaitHint;

  BOOLEAN ServiceVisited;

  HANDLE ControlPipeHandle;
  ULONG ProcessId;
  ULONG ThreadId;
} SERVICE, *PSERVICE;


/* services.c */

VOID PrintString(LPCSTR fmt, ...);


/* database.c */

NTSTATUS ScmCreateServiceDataBase(VOID);
VOID ScmGetBootAndSystemDriverState(VOID);
VOID ScmAutoStartServices(VOID);

PSERVICE
ScmGetServiceEntryByName(PUNICODE_STRING ServiceName);


/* rpcserver.c */

VOID ScmStartRpcServer(VOID);


/* EOF */

