/*
 * clsfact.h - IClassFactory implementation.
 */


#ifdef __cplusplus
extern "C" {                        /* Assume C declarations for C++. */
#endif   /* __cplusplus */


/* Prototypes
 *************/

/* clsfact.cpp */

extern ULONG DLLAddRef(void);
extern ULONG DLLRelease(void);
extern PULONG GetDLLRefCountPtr(void);


#ifdef __cplusplus
}                                   /* End of extern "C" {. */
#endif   /* __cplusplus */

