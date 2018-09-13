

// SQL debugging is not available in kernel mode
// for test purposes, it is also disabled on non-x86 boxes (temporarily)

#if !defined(KERNEL) && defined(TARGET_i386) && 0

extern void DMSqlStartup( HPRCX );
extern void DMSqlLoadDll( HPRCX, LOAD_DLL_DEBUG_INFO64 *, int );
extern void DMSqlTerminate( HPRCX pprc );
extern XOSD DMSqlSystemService( HPRCX pprc, LPBYTE command );
extern void DMSqlPreLoad( DWORD );
#define SQLDBG 1

#else

// macros to stub out functions on unsupported platforms

#define DMSqlStartup(x)
#define DMSqlLoadDll(x,y,z)
#define DMSqlTerminate(x)
#define DMSqlPreLoad(x)
#define SQLDBG 0

#endif
