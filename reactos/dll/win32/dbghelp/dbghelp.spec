@ stub DbgHelpCreateUserDump
@ stub DbgHelpCreateUserDumpW
@ stdcall EnumDirTree(long str str ptr ptr ptr)
@ stdcall EnumDirTreeW(long wstr wstr ptr ptr ptr)
@ stdcall EnumerateLoadedModules(long ptr ptr)
@ stdcall EnumerateLoadedModules64(long ptr ptr)
@ stdcall EnumerateLoadedModulesW64(long ptr ptr)
@ stdcall ExtensionApiVersion()
@ stdcall FindDebugInfoFile(str str ptr)
@ stdcall FindDebugInfoFileEx(str str ptr ptr ptr)
@ stub FindDebugInfoFileExW
@ stdcall FindExecutableImage(str str str)
@ stdcall FindExecutableImageEx(str str ptr ptr ptr)
@ stdcall FindExecutableImageExW(wstr wstr ptr ptr ptr)
@ stub FindFileInPath
@ stub FindFileInSearchPath
@ stdcall GetTimestampForLoadedLibrary(long)
@ stdcall ImageDirectoryEntryToData(ptr long long ptr) ntdll.RtlImageDirectoryEntryToData
@ stub ImageDirectoryEntryToDataEx
@ stdcall ImageNtHeader(ptr) ntdll.RtlImageNtHeader
@ stdcall ImageRvaToSection(ptr ptr long) ntdll.RtlImageRvaToSection
@ stdcall ImageRvaToVa(ptr ptr long ptr) ntdll.RtlImageRvaToVa
@ stdcall ImagehlpApiVersion()
@ stdcall ImagehlpApiVersionEx(ptr)
@ stdcall MakeSureDirectoryPathExists(str)
@ stdcall MapDebugInformation(long str str long)
@ stdcall MiniDumpReadDumpStream(ptr long ptr ptr ptr)
@ stdcall MiniDumpWriteDump(ptr long ptr long long long long)
@ stdcall SearchTreeForFile(str str ptr)
@ stdcall SearchTreeForFileW(wstr wstr ptr)
@ stdcall StackWalk(long long long ptr ptr ptr ptr ptr ptr)
@ stdcall StackWalk64(long long long ptr ptr ptr ptr ptr ptr)
@ stub SymAddSymbol
@ stub SymAddSymbolW
@ stdcall SymCleanup(long)
@ stdcall SymEnumLines(ptr double str str ptr ptr)
@ stub SymEnumLinesW
@ stdcall SymEnumSourceFiles(ptr double str ptr ptr)
@ stub SymEnumSourceFilesW
@ stub SymEnumSym
@ stdcall SymEnumSymbols(ptr double str ptr ptr)
@ stdcall SymEnumSymbolsW(ptr double wstr ptr ptr)
@ stub SymEnumSymbolsForAddr
@ stub SymEnumSymbolsForAddrW
@ stdcall SymEnumTypes(ptr double ptr ptr)
@ stdcall SymEnumTypesW(ptr double ptr ptr)
@ stdcall SymEnumerateModules(long ptr ptr)
@ stdcall SymEnumerateModules64(long ptr ptr)
@ stdcall SymEnumerateModulesW64(long ptr ptr)
@ stdcall SymEnumerateSymbols(long long ptr ptr)
@ stub SymEnumerateSymbols64
@ stub SymEnumerateSymbolsW
@ stub SymEnumerateSymbolsW64
@ stub SymFindDebugInfoFile
@ stub SymFindDebugInfoFileW
@ stdcall SymFindFileInPath(long str str ptr long long long ptr ptr ptr)
@ stdcall SymFindFileInPathW(long wstr wstr ptr long long long ptr ptr ptr)
@ stdcall SymFromAddr(ptr double ptr ptr)
@ stdcall SymFromAddrW(ptr double ptr ptr)
@ stub SymFromIndex
@ stub SymFromIndexW
@ stdcall SymFromName(long str ptr)
@ stub SymFromNameW
@ stub SymFromToken
@ stub SymFromTokenW
@ stdcall SymFunctionTableAccess(long long)
@ stdcall SymFunctionTableAccess64(long double)
@ stub SymGetFileLineOffsets64
@ stub SymGetHomeDirectory
@ stub SymGetHomeDirectoryW
@ stdcall SymGetLineFromAddr(long long ptr ptr)
@ stdcall SymGetLineFromAddr64(long double ptr ptr)
@ stdcall SymGetLineFromAddrW64(long double ptr ptr)
@ stub SymGetLineFromName
@ stub SymGetLineFromName64
@ stdcall SymGetLineNext(long ptr)
@ stdcall SymGetLineNext64(long ptr)
@ stub SymGetLineNextW64
@ stdcall SymGetLinePrev(long ptr)
@ stdcall SymGetLinePrev64(long ptr)
@ stub SymGetLinePrevW64
@ stdcall SymGetModuleBase(long long)
@ stdcall SymGetModuleBase64(long double)
@ stdcall SymGetModuleInfo(long long ptr)
@ stdcall SymGetModuleInfo64(long double ptr)
@ stdcall SymGetModuleInfoW(long long ptr)
@ stdcall SymGetModuleInfoW64(long double ptr)
@ stub SymGetOmapBlockBase
@ stdcall SymGetOptions()
@ stub SymGetScope
@ stub SymGetScopeW
@ stdcall SymGetSearchPath(long ptr long)
@ stdcall SymGetSearchPathW(long ptr long)
@ stub SymGetSourceFileFromToken
@ stub SymGetSourceFileFromTokenW
@ stdcall SymGetSourceFileToken(ptr double str ptr ptr)
@ stdcall SymGetSourceFileTokenW(ptr double wstr ptr ptr)
@ stub SymGetSourceFileW
@ stub SymGetSourceVarFromToken
@ stub SymGetSourceVarFromTokenW
@ stdcall SymGetSymFromAddr(long long ptr ptr)
@ stdcall SymGetSymFromAddr64(long double ptr ptr)
@ stdcall SymGetSymFromName(long str ptr)
@ stub SymGetSymFromName64
@ stdcall SymGetSymNext(long ptr)
@ stub SymGetSymNext64
@ stdcall SymGetSymPrev(long ptr)
@ stub SymGetSymPrev64
@ stub SymGetSymbolFile
@ stub SymGetSymbolFileW
@ stdcall SymGetTypeFromName(ptr double str ptr)
@ stub SymGetTypeFromNameW
@ stdcall SymGetTypeInfo(ptr double long long ptr)
@ stub SymGetTypeInfoEx
@ stdcall SymInitialize(long str long)
@ stdcall SymInitializeW(long wstr long)
@ stdcall SymLoadModule(long long str str long long)
@ stdcall SymLoadModule64(long long str str double long)
@ stdcall SymLoadModuleEx(long long str str double long ptr long)
@ stdcall SymLoadModuleExW(long long wstr wstr double long ptr long)
@ stdcall SymMatchFileName(str str ptr ptr)
@ stdcall SymMatchFileNameW(wstr wstr ptr ptr)
@ stdcall SymMatchString(str str long)
@ stub SymMatchStringA
@ stub SymMatchStringW
@ stub SymNext
@ stub SymNextW
@ stub SymPrev
@ stub SymPrevW
@ stub SymRefreshModuleList
@ stdcall SymRegisterCallback(long ptr ptr)
@ stdcall SymRegisterCallback64(long ptr double)
@ stdcall SymRegisterCallbackW64(long ptr double)
@ stdcall SymRegisterFunctionEntryCallback(ptr ptr ptr)
@ stdcall SymRegisterFunctionEntryCallback64(ptr ptr double)
@ stdcall SymSearch(long double long long str double ptr ptr long)
@ stdcall SymSearchW(long double long long wstr double ptr ptr long)
@ stdcall SymSetContext(long ptr ptr)
@ stub SymSetHomeDirectory
@ stub SymSetHomeDirectoryW
@ stdcall SymSetOptions(long)
@ stdcall SymSetParentWindow(long)
@ stdcall SymSetSearchPath(long str)
@ stdcall SymSetSearchPathW(long wstr)
@ stub SymSetSymWithAddr64
@ stub SymSrvDeltaName
@ stub SymSrvDeltaNameW
@ stub SymSrvGetFileIndexInfo
@ stub SymSrvGetFileIndexInfoW
@ stub SymSrvGetFileIndexString
@ stub SymSrvGetFileIndexStringW
@ stub SymSrvGetFileIndexes
@ stub SymSrvGetFileIndexesW
@ stub SymSrvGetSupplement
@ stub SymSrvGetSupplementW
@ stub SymSrvIsStore
@ stub SymSrvIsStoreW
@ stub SymSrvStoreFile
@ stub SymSrvStoreFileW
@ stub SymSrvStoreSupplement
@ stub SymSrvStoreSupplementW
# @ stub SymSetSymWithAddr64 no longer present ??
@ stdcall SymUnDName(ptr str long)
@ stub SymUnDName64
@ stdcall SymUnloadModule(long long)
@ stdcall SymUnloadModule64(long double)
@ stdcall UnDecorateSymbolName(str str long long)
@ stub UnDecorateSymbolNameW
@ stdcall UnmapDebugInformation(ptr)
@ stdcall WinDbgExtensionDllInit(ptr long long)
#@ stub block
#@ stub chksym
#@ stub dbghelp
#@ stub dh
#@ stub fptr
#@ stub homedir
#@ stub itoldyouso
#@ stub lmi
#@ stub lminfo
#@ stub omap
#@ stub srcfiles
#@ stub stack_force_ebp
#@ stub stackdbg
#@ stub sym
#@ stub symsrv
#@ stub vc7fpo
