/*
 * init.h - DLL startup routines module description.
 */


/* Prototypes
 *************/

/* functions to be provided by client */

extern BOOL AttachProcess(HANDLE);
extern BOOL DetachProcess(HANDLE);
extern BOOL AttachThread(HANDLE);
extern BOOL DetachThread(HANDLE);

