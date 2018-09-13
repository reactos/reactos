/*
 * memmgr.h - Memory manager module description.
 */


/* Macros
 *********/

#ifdef DEBUG
#define AllocateMemory(size, ppv)   (GpcszElemHdrSize = TEXT(#size), GpcszElemHdrFile = TEXT(__FILE__), GulElemHdrLine = __LINE__, MyAllocateMemory(size, ppv))
#else
#define AllocateMemory(size, ppv)   MyAllocateMemory(size, ppv)
#endif   /* DEBUG */


/* Types
 ********/

#ifdef DEBUG

/* SpewHeapSummary() flags */

typedef enum _spewheapsummaryflags
{
   /* Spew description of each remaining used element. */

   SHS_FL_SPEW_USED_INFO            = 0x0001,

   /* flag combinations */

   ALL_SHS_FLAGS                    = SHS_FL_SPEW_USED_INFO
}
SPEWHEAPSUMMARYFLAGS;

#endif


/* Prototypes
 *************/

/* memmgr.c */

extern BOOL InitMemoryManagerModule(void);
extern void ExitMemoryManagerModule(void);

extern COMPARISONRESULT MyMemComp(PCVOID, PCVOID, DWORD);
extern BOOL MyAllocateMemory(DWORD, PVOID *);
extern void FreeMemory(PVOID);
extern BOOL ReallocateMemory(PVOID, DWORD, PVOID *);
extern DWORD GetMemorySize(PVOID);

#ifdef DEBUG

extern BOOL SetMemoryManagerModuleIniSwitches(void);
extern void SpewHeapSummary(DWORD);

#endif


/* Global Variables
 *******************/

#ifdef DEBUG

/* parameters used by debug version of AllocateMemory() */

extern LPCTSTR GpcszElemHdrSize;
extern LPCTSTR GpcszElemHdrFile;
extern ULONG GulElemHdrLine;

#endif
