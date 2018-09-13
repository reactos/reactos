/*
 * storage.h - Storage ADT description.
 */


/* Types
 ********/

/* handles */

DECLARE_HANDLE(HSTGIFACE);
DECLARE_STANDARD_TYPES(HSTGIFACE);


/* Prototypes
 *************/

/* storage.c */

extern BOOL ProcessInitStorageModule(void);
extern void ProcessExitStorageModule(void);
extern HRESULT GetStorageInterface(PIUnknown, PHSTGIFACE);
extern void ReleaseStorageInterface(HSTGIFACE);
extern HRESULT LoadFromStorage(HSTGIFACE, LPCTSTR);
extern HRESULT SaveToStorage(HSTGIFACE);
extern void HandsOffStorage(HSTGIFACE);
extern BOOL GetIMalloc(PIMalloc *);

#ifdef DEBUG

extern BOOL IsValidHSTGIFACE(HSTGIFACE);

#endif

