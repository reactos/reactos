/*
 * services.h
 */


/* services.c */

void PrintString(char* fmt,...);


/* database.c */

NTSTATUS ScmCreateServiceDataBase(VOID);
VOID ScmGetBootAndSystemDriverState(VOID);
VOID ScmAutoStartServices(VOID);


/* EOF */

