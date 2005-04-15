/*
 * services.h
 */



/* services.c */

VOID PrintString(LPCSTR fmt, ...);


/* database.c */

NTSTATUS ScmCreateServiceDataBase(VOID);
VOID ScmGetBootAndSystemDriverState(VOID);
VOID ScmAutoStartServices(VOID);


/* rpcserver.c */

VOID ScmStartRpcServer(VOID);


/* EOF */

