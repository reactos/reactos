/*
 * memmgr.h - Memory manager module description.
 */


/* Macros
 *********/

#ifdef DEBUG
#define AllocateMemory(size, ppv)         DebugAllocateMemory(size, ppv, #size, __FILE__, __LINE__)
#define ReallocateMemory(pv, size, ppv)   DebugReallocateMemory(pv, size, ppv)
#define FreeMemory(pv)                    DebugFreeMemory(pv)
#define MemorySize(pv)                    DebugMemorySize(pv)
#else
#define AllocateMemory(size, ppv)         IAllocateMemory(size, ppv)
#define ReallocateMemory(pv, size, ppv)   IReallocateMemory(pv, size, ppv)
#define FreeMemory(pv)                    IFreeMemory(pv)
#define MemorySize(pv)                    IMemorySize(pv)
#endif   /* DEBUG */


/* Types
 ********/

#ifdef DEBUG

/* SpewHeapSummary() flags */

typedef enum _spewheapsummaryflags
{
   /* Spew description of each remaining used element. */

   SHS_FL_SPEW_USED_INFO   = 0x0001,

   /* flag combinations */

   ALL_SHS_FLAGS           = SHS_FL_SPEW_USED_INFO
}
SPEWHEAPSUMMARYFLAGS;

#endif   /* DEBUG */


/* Prototypes
 *************/

/* memmgr.c */

extern BOOL InitMemoryManagerModule(void);
extern void ExitMemoryManagerModule(void);
extern COMPARISONRESULT MyMemComp(PCVOID, PCVOID, DWORD);

#ifdef DEBUG

extern BOOL DebugAllocateMemory(DWORD, PVOID *, PCSTR, PCSTR, ULONG);
extern void DebugFreeMemory(PVOID);
extern BOOL DebugReallocateMemory(PVOID, DWORD, PVOID *);
extern DWORD DebugMemorySize(PVOID);
extern BOOL SetMemoryManagerModuleIniSwitches(void);
extern void SpewHeapSummary(DWORD);

#endif


/* Inline Functions
 *******************/

INLINE BOOL IAllocateMemory(DWORD dwcbSize, PVOID *ppvNew)
{
   *ppvNew = GlobalAlloc(GPTR, dwcbSize);
   return(*ppvNew != NULL);
}

INLINE BOOL IReallocateMemory(PVOID pvOld, DWORD dwcbNewSize, PVOID *ppvNew)
{
   *ppvNew = GlobalReAlloc(pvOld, dwcbNewSize, GMEM_MOVEABLE);
   return(*ppvNew != NULL);
}


INLINE void IFreeMemory(PVOID pv)
{
   EVAL(! GlobalFree(pv));

   return;
}

INLINE DWORD IMemorySize(PVOID pv)
{
   return(DWORD)(GlobalSize(pv));
}


/* Global Variables
 *******************/

#ifdef DEBUG

/* parameters used by debug version of AllocateMemory() wrapper macros */

extern PCSTR g_pcszElemHdrSize;
extern PCSTR g_pcszElemHdrFile;
extern ULONG g_ulElemHdrLine;

#endif   /* DEBUG */
