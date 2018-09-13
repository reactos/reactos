/*
 * clsiface.h - Class interface cache ADT description.
 */


/* Types
 ********/

/* handles */

DECLARE_HANDLE(HCLSIFACECACHE);
DECLARE_STANDARD_TYPES(HCLSIFACECACHE);


/* Prototypes
 *************/

/* rhcache.c */

extern BOOL CreateClassInterfaceCache(PHCLSIFACECACHE);
extern void DestroyClassInterfaceCache(HCLSIFACECACHE);
extern HRESULT GetClassInterface(HCLSIFACECACHE, PCCLSID, PCIID, PVOID *);

#ifdef DEBUG

extern BOOL IsValidHCLSIFACECACHE(HCLSIFACECACHE);

#endif

