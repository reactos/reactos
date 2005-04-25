#ifndef __INCLUDE_INTERNAL_DBG_H
#define __INCLUDE_INTERNAL_DBG_H

#include <napi/dbg.h>
#include <internal/port.h>

#define KdPrintEx(_x_) DbgPrintEx _x_

#if defined(KDBG) || defined(DBG)
# define KDB_LOADUSERMODULE_HOOK(LDRMOD)	KdbSymLoadUserModuleSymbols(LDRMOD)
# define KDB_LOADDRIVER_HOOK(FILENAME, MODULE)	KdbSymLoadDriverSymbols(FILENAME, MODULE)
# define KDB_UNLOADDRIVER_HOOK(MODULE)		KdbSymUnloadDriverSymbols(MODULE)
# define KDB_LOADERINIT_HOOK(NTOS, HAL)		KdbSymInit(NTOS, HAL)
# define KDB_SYMBOLFILE_HOOK(FILENAME)		KdbSymProcessBootSymbols(FILENAME)
# define KDB_DELETEPROCESS_HOOK(PROCESS)	KdbDeleteProcessHook(PROCESS)
#else
# define KDB_LOADUSERMODULE_HOOK(LDRMOD)	do { } while (0)
# define KDB_LOADDRIVER_HOOK(FILENAME, MODULE)	do { } while (0)
# define KDB_UNLOADDRIVER_HOOK(MODULE)		do { } while (0)
# define KDB_LOADERINIT_HOOK(NTOS, HAL)		do { } while (0)
# define KDB_SYMBOLFILE_HOOK(FILENAME)		do { } while (0)
# define KDB_CREATE_THREAD_HOOK(CONTEXT)	do { } while (0)
# define KDB_DELETEPROCESS_HOOK(PROCESS)	do { } while (0)
#endif

#ifdef KDBG
# define KeRosPrintAddress(ADDRESS)         KdbSymPrintAddress(ADDRESS)
# define KdbInit()                          KdbpCliInit()
# define KdbModuleLoaded(FILENAME)          KdbpCliModuleLoaded(FILENAME)
//# define KdbBreak()                         KdbBreak()
#else
# define KeRosPrintAddress(ADDRESS)         KiRosPrintAddress(ADDRESS)
# define KdbEnterDebuggerException(ER, PM, C, TF, F)  kdHandleException
# define KdbInit()                          do { } while (0)
# define KdbEnter()                         do { } while (0)
# define KdbModuleLoaded(X)                 do { } while (0)
//# define KdbBreak()                         do { } while (0)
#endif

#endif /* __INCLUDE_INTERNAL_DBG_H */
