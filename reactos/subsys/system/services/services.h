/*
 * services.h
 */

typedef struct _SERVICE_GROUP
{
  LIST_ENTRY GroupListEntry;
  UNICODE_STRING GroupName;

  BOOLEAN ServicesRunning;

} SERVICE_GROUP, *PSERVICE_GROUP;

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

  BOOLEAN ServiceRunning;
  BOOLEAN ServiceVisited;

  HANDLE ControlPipeHandle;
  ULONG ProcessId;
  ULONG ThreadId;
} SERVICE, *PSERVICE;

/* services.c */

void PrintString(char* fmt,...);


/* database.c */

NTSTATUS ScmCreateServiceDataBase(VOID);
VOID ScmGetBootAndSystemDriverState(VOID);
VOID ScmAutoStartServices(VOID);

PSERVICE FASTCALL
ScmCreateServiceListEntry(PUNICODE_STRING ServiceName);
PSERVICE FASTCALL
ScmFindService(PUNICODE_STRING ServiceName);
NTSTATUS FASTCALL
ScmStartService(PSERVICE Service, PSERVICE_GROUP Group);

/* EOF */

