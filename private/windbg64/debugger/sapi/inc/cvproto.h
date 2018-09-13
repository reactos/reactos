extern KNF knf;

#define MHAlloc   (*knf.lpfnMHAlloc)
#define MHRealloc (*knf.lpfnMHRealloc)
#define MHFree    (*knf.lpfnMHFree)
#define MHAllocHuge (*knf.lpfnMHAllocHuge)
#define MHFreeHuge  (*knf.lpfnMHFreeHuge)

#define MMAlloc   (*knf.lpfnMMAllocHmem)
#define MMFree    (*knf.lpfnMMFreeHmem)
#define MMLock    (*knf.lpfnMMLock)
#define MMUnlock  (*knf.lpfnMMUnlock)

#define LLInit    (*knf.lpfnLLInit)
#define LLCreate  (*knf.lpfnLLCreate)
#define LLAdd     (*knf.lpfnLLAdd)
#define LLInsert  (*knf.lpfnLLInsert)
#define LLDelete  (*knf.lpfnLLDelete)
#define LLNext    (*knf.lpfnLLNext)
#define LLDestroy (*knf.lpfnLLDestroy)
#define LLFind    (*knf.lpfnLLFind)
#define LLSize    (*knf.lpfnLLSize)
#define LLLock    (*knf.lpfnLLLock)
#define LLUnlock  (*knf.lpfnLLUnlock)
#define LLLast    (*knf.lpfnLLLast)
#define LLAddHead (*knf.lpfnLLAddHead)
#define LLRemove  (*knf.lpfnLLRemove)

#define SYFixupAddr   (*knf.lpfnSYFixupAddr)
#define SYUnFixupAddr (*knf.lpfnSYUnFixupAddr)
#define SYOpen        (*knf.lpfnSYOpen)
#define SYClose       (*knf.lpfnSYClose)
#define SYReadFar     (*knf.lpfnSYReadFar)
#define SYSeek        (*knf.lpfnSYSeek)
#define SYTell        (*knf.lpfnSYTell)
#define SYProcessor   (*knf.lpfnSYProcessor)
#define LBPrintf  (*knf.lpfnLBPrintf)
#define LBQuit    (*knf.lpfnLBQuit)

#define GetRegistryRoot (knf.lpfnGetRegistryRoot)

#if 0
#define _searchenv  (*knf.lpfn_searchenv)
#define sprintf     (*knf.lpfnsprintf)
#define _splitpath  (*knf.lpfn_splitpath)
#define _fullpath   (*knf.lpfn_fullpath)
#define _makepath   (*knf.lpfn_makepath)
#endif

#define SYGetDefaultShe  (*knf.lpfnSYGetDefaultShe)
#define DLoadedSymbols  (*knf.lpfnLoadedSymbols)
#define SYFindExeFile (*knf.lpfnSYFindExeFile)
#define GetSymbolFileFromServer (*knf.lpfnGetSymbolFileFromServer)
#define SYIgnoreAllSymbolErrors (*knf.lpfnSYIgnoreAllSymbolErrors)
#define pSYFindDebugInfoFile (knf.pfnSYFindDebugInfoFile)


#pragma message ("Read cvproto.hxx for Shell interface changes")
#if 0
#pragma message("SYGetDefaultShe always TRUE")
#define SYGetDefaultShe(a, b) TRUE
#pragma message("DLoadedSymbols NULL'd out")
#define DLoadedSymbols
#pragma message("SYFindExeFile not Implemented")
#define SYFindExeFile -1
#endif

#define SYGetProfileString (knf.lpfnGetProfileString)
#define SYSetProfileString (knf.lpfnSetProfileString)
