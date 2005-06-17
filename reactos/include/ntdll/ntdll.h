
#include <ntos/ntdef.h>

#define UNIMPLEMENTED DbgPrint("%s in %s:%d is unimplemented\n",__FUNCTION__,__FILE__,__LINE__);

#ifndef __USE_W32API

#ifndef NASSERT
#define assert(x) if (!(x)) {DbgPrint("Assertion "#x" failed at %s:%d\n", __FILE__,__LINE__); for(;;);}
#define ASSERT(x) if (!(x)) {DbgPrint("Assertion "#x" failed at %s:%d\n", __FILE__,__LINE__); for(;;);}
#else
#define assert(x)
#define ASSERT(x)
#endif

#endif

#ifdef NDEBUG
#if defined(__GNUC__)
#define TRACE_LDR(args...) if (RtlGetNtGlobalFlags() & FLG_SHOW_LDR_SNAPS) { DbgPrint("(LDR:%s:%d) ",__FILE__,__LINE__); DbgPrint(args); }
#define DPRINT(args...)
#else
#define DPRINT
#endif	/* __GNUC__ */
#define CHECKPOINT
#else
#define TRACE_LDR(args...) do { DbgPrint("(LDR:%s:%d) ",__FILE__,__LINE__); DbgPrint(args); } while(0)
#define DPRINT(args...) do { DbgPrint("(NTDLL:%s:%d) ",__FILE__,__LINE__); DbgPrint(args); } while(0)
#define CHECKPOINT do { DbgPrint("(NTDLL:%s:%d) Checkpoint\n",__FILE__,__LINE__); } while(0)
#endif

#ifdef DBG
#if defined(__GNUC__)
#define DPRINT1(args...) do { DbgPrint("(NTDLL:%s:%d) ",__FILE__,__LINE__); DbgPrint(args); } while(0)
#else
#define DPRINT1              DbgPrint("(NTDLL:%s:%d) ",__FILE__,__LINE__); DbgPrint
#endif
#define CHECKPOINT1 do { DbgPrint("(NTDLL:%s:%d) Checkpoint\n",__FILE__,__LINE__); } while(0)
#else
#define DPRINT1(args...)
#define CHECKPOINT1(args...)
#endif

/* Macros expanding to the appropriate inline assembly to raise a breakpoint */
#if defined(_M_IX86)
#define ASM_BREAKPOINT "\nint $3\n"
#elif defined(_M_ALPHA)
#define ASM_BREAKPOINT "\ncall_pal bpt\n"
#elif defined(_M_MIPS)
#define ASM_BREAKPOINT "\nbreak\n"
#else
#error Unsupported architecture.
#endif

#define ROUNDUP(a,b)	((((a)+(b)-1)/(b))*(b))
#define ROUNDDOWN(a,b)	(((a)/(b))*(b))

#define  MAGIC(c1,c2,c3,c4)  ((c1) + ((c2)<<8) + ((c3)<<16) + ((c4)<<24))

#define  MAGIC_HEAP        MAGIC( 'H','E','A','P' )

#ifdef DBG
extern VOID FASTCALL CHECK_PAGED_CODE_RTL(char *file, int line);
#define PAGED_CODE_RTL() CHECK_PAGED_CODE_RTL(__FILE__, __LINE__)
#else
#define PAGED_CODE_RTL()
#endif
