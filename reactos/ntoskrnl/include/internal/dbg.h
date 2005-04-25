#ifndef __INCLUDE_INTERNAL_DBG_H
#define __INCLUDE_INTERNAL_DBG_H

#include <napi/dbg.h>
#include <internal/port.h>

#define KdPrintEx(_x_) DbgPrintEx _x_

#if defined(KDBG) || defined(DBG)

VOID
KdbSymLoadUserModuleSymbols(IN PLDR_MODULE LdrModule);

VOID
KdbSymFreeProcessSymbols(IN PEPROCESS Process);

VOID
KdbSymLoadDriverSymbols(IN PUNICODE_STRING Filename,
                        IN PMODULE_OBJECT Module);

VOID
KdbSymUnloadDriverSymbols(IN PMODULE_OBJECT ModuleObject);

VOID
KdbSymProcessBootSymbols(IN PCHAR FileName);

VOID
KdbSymInit(IN PMODULE_TEXT_SECTION NtoskrnlTextSection,
           IN PMODULE_TEXT_SECTION LdrHalTextSection);

BOOLEAN 
KdbSymPrintAddress(IN PVOID Address);

VOID
KdbDeleteProcessHook(IN PEPROCESS Process);

NTSTATUS
KdbSymGetAddressInformation(IN PROSSYM_INFO  RosSymInfo,
                            IN ULONG_PTR  RelativeAddress,
                            OUT PULONG LineNumber  OPTIONAL,
                            OUT PCH FileName  OPTIONAL,
                            OUT PCH FunctionName  OPTIONAL);

typedef struct _KDB_MODULE_INFO
{
    WCHAR        Name[256];
    ULONG_PTR    Base;
    ULONG        Size;
    PROSSYM_INFO RosSymInfo;
} KDB_MODULE_INFO, *PKDB_MODULE_INFO;

# define KDB_LOADUSERMODULE_HOOK(LDRMOD)	KdbSymLoadUserModuleSymbols(LDRMOD)
# define KDB_LOADDRIVER_HOOK(FILENAME, MODULE)	KdbSymLoadDriverSymbols(FILENAME, MODULE)
# define KDB_UNLOADDRIVER_HOOK(MODULE)		KdbSymUnloadDriverSymbols(MODULE)
# define KDB_LOADERINIT_HOOK(NTOS, HAL)		KdbSymInit(NTOS, HAL)
# define KDB_SYMBOLFILE_HOOK(FILENAME)		KdbSymProcessBootSymbols(FILENAME)
#else
# define KDB_LOADUSERMODULE_HOOK(LDRMOD)	do { } while (0)
# define KDB_LOADDRIVER_HOOK(FILENAME, MODULE)	do { } while (0)
# define KDB_UNLOADDRIVER_HOOK(MODULE)		do { } while (0)
# define KDB_LOADERINIT_HOOK(NTOS, HAL)		do { } while (0)
# define KDB_SYMBOLFILE_HOOK(FILENAME)		do { } while (0)
# define KDB_CREATE_THREAD_HOOK(CONTEXT)	do { } while (0)
#endif

#ifdef KDBG
# define KeRosPrintAddress(ADDRESS)         KdbSymPrintAddress(ADDRESS)
# define KdbInit()                          KdbpCliInit()
# define KdbModuleLoaded(FILENAME)          KdbpCliModuleLoaded(FILENAME)
# define KDB_DELETEPROCESS_HOOK(PROCESS)	KdbDeleteProcessHook(PROCESS)
#else
# define KeRosPrintAddress(ADDRESS)         KiRosPrintAddress(ADDRESS)
# define KdbEnterDebuggerException(ER, PM, C, TF, F)  kdHandleException
# define KdbInit()                          do { } while (0)
# define KdbEnter()                         do { } while (0)
# define KdbModuleLoaded(X)                 do { } while (0)
# define KDB_DELETEPROCESS_HOOK(PROCESS)	do { } while (0)
#endif

#endif /* __INCLUDE_INTERNAL_DBG_H */
