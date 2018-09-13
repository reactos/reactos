extern "C" {
//
// Debugging macros
//
#if DBG
#   define  DBG_CODE    1

void DbgPrintf( LPTSTR szFmt, ... );
void DbgStopX(LPSTR mszFile, int iLine, LPTSTR szText );
HLOCAL MemAllocWorker(LPSTR szFile, int iLine, UINT uFlags, UINT cBytes);
HLOCAL MemFreeWorker(LPSTR szFile, int iLine, HLOCAL hMem);
void MemExitCheckWorker(void);


//#   define  MemAlloc( f, s )    MemAllocWorker( __FILE__, __LINE__, f, s )
//#   define  MemFree( h )        MemFreeWorker( __FILE__, __LINE__, h )
#   define  MEM_EXIT_CHECK()    MemExitCheckWorker()
#   define  DBGSTOP( t )        DbgStopX( __FILE__, __LINE__, TEXT(t) )
#   define  DBGSTOPX( f, l, t ) DbgStopX( f, l, TEXT(t) )
#   define  DBGPRINTF(p)        DbgPrintf p
#   define  DBGOUT(t)           DbgPrintf( TEXT("SYSCPL.CPL: %s\n"), TEXT(t) )
#else
//#   define  MemAlloc( f, s )    LocalAlloc( f, s )
//#   define  MemFree( h )        LocalFree( h )
#   define  MEM_EXIT_CHECK()
#   define  DBGSTOP( t )
#   define  DBGSTOPX( f, l, t )
#   define  DBGPRINTF(p)
#   define  DBGOUT(t)
#endif
}
