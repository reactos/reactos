/*
 * server.h - Server vtable functions module description.
 */


/* Types
 ********/

#include <msshrui.h>

typedef struct _servervtable
{
   PFNGETNETRESOURCEFROMLOCALPATH GetNetResourceFromLocalPath;
   PFNGETLOCALPATHFROMNETRESOURCE GetLocalPathFromNetResource;
}
SERVERVTABLE;
DECLARE_STANDARD_TYPES(SERVERVTABLE);


/* Prototypes
 *************/

/* server.c */

extern BOOL ProcessInitServerModule(void);
extern void ProcessExitServerModule(void);
extern BOOL GetServerVTable(PCSERVERVTABLE *);

