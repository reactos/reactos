/*
 * memmgr.hpp - Memory manager module description.
 */


/* Inline Functions
 *******************/

//
// N.b., you must use parentheses around the argument to new under the debug
// memory manager, i.e., call "new(int)" not "new int".  This allows the debug
// memory manager to identify orphaned heap elements by file and line number
// where the heap element was allocated.  If you use new without parentheses
// around the argument, the debug memory manager will record incorrect source
// allocation information for the allocated heap element, and you may get
// runtime RIPs in memmgr.c::DebugAllocateMemory() complaining that pcszSize or
// pcszFile is an invalid string pointer.
//
// There is not currently a way to differentiate between "new int" and
// "new(int)" at compile time to warn the developer to use parentheses around
// the argument to new.
//
INLINE PVOID __cdecl operator new(size_t cbSize)
{
   PVOID pv;

   /* Ignore return value. */

#ifdef DEBUG

   DebugAllocateMemory(cbSize, &pv, g_pcszElemHdrSize, g_pcszElemHdrFile, g_ulElemHdrLine);

   // Invalidate debug heap element allocation parameters to try to catch calls
   // to operator new not invoked via the new() macro.

   g_pcszElemHdrSize = NULL;
   g_pcszElemHdrFile = NULL;
   g_ulElemHdrLine = 0;

#else

   IAllocateMemory(cbSize, &pv);

#endif

   return(pv);
}

INLINE void __cdecl operator delete(PVOID pv)
{
   FreeMemory(pv);
}

INLINE int __cdecl _purecall(void)
{
   return(0);
}


/* Macros
 *********/

#ifdef DEBUG
#define new(type)                         (g_pcszElemHdrSize = #type, \
                                           g_pcszElemHdrFile = __FILE__, \
                                           g_ulElemHdrLine = __LINE__, \
                                           new type)
#else
#define new(type)                         (new type)
#endif   /* DEBUG */

