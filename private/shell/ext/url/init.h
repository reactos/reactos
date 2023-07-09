/*
 * init.h - DLL startup routines module description.
 */


/* Prototypes
 *************/

/* functions to be provided by client */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef UNIX
extern BOOL AttachProcess(HANDLE);
extern BOOL DetachProcess(HANDLE);
extern BOOL AttachThread(HANDLE);
extern BOOL DetachThread(HANDLE);
#else
extern BOOL AttachProcess(HMODULE);
extern BOOL DetachProcess(HMODULE);
extern BOOL AttachThread(HMODULE);
extern BOOL DetachThread(HMODULE);
#endif /* UNIX */

#define PLATFORM_UNKNOWN     0
#define PLATFORM_IE3         1
#define PLATFORM_INTEGRATED  2

UINT _WhichPlatform(void);
#define WhichPlatform _WhichPlatform

#ifdef __cplusplus
}
#endif
