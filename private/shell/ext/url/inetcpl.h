/*
 * inetcpl.h - Indirect calls to inetcpl.cpl description.
 */


#ifdef __cplusplus
extern "C" {                        /* Assume C declarations for C++. */
#endif   /* __cplusplus */


/* Prototypes
 *************/

/* inetcpl.c */
#define ExitInternetCPLModule() UnloadInternetCPL()
	
extern PULONG GetInternetCPLRefCountPtr(void);
extern HRESULT InternetCPLCanUnloadNow(void);
extern void UnloadInternetCPL(void);
extern BOOL InitInternetCPLModule(void);
extern HRESULT WINAPI AddInternetPropertySheets(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lparam, PUINT pucRefCount, LPFNPSPCALLBACK pfnCallback);


#ifdef __cplusplus
}                                   /* End of extern "C" {. */
#endif   /* __cplusplus */

