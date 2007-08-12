@ stub DbgHelpCreateUserDump
@ stub DbgHelpCreateUserDumpW
@ stdcall EnumDirTree(long str str ptr ptr ptr)
@ stdcall EnumerateLoadedModules(long ptr ptr)
@ stub EnumerateLoadedModules64
@ stub ExtensionApiVersion
@ stdcall FindDebugInfoFile(str str ptr)
@ stdcall FindDebugInfoFileEx(str str ptr ptr ptr)
@ stdcall FindExecutableImage(str str str)
@ stub FindExecutableImageEx
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
@ stub MiniDumpReadDumpStream
@ stdcall MiniDumpWriteDump(ptr long ptr long long long long)
@ stdcall SearchTreeForFile(str str str)
@ stdcall StackWalk(long long long ptr ptr ptr ptr ptr ptr)
@ stub StackWalk64
@ stdcall SymCleanup(long)
@ stdcall SymEnumSourceFiles(ptr long long str ptr ptr)
@ stub SymEnumSym
@ stdcall SymEnumSymbols(ptr long long str ptr ptr)
@ stdcall SymEnumTypes(ptr long long ptr ptr)
@ stdcall SymEnumerateModules(long ptr ptr)
@ stub SymEnumerateModules64
@ stdcall SymEnumerateSymbols(long long ptr ptr)
@ stub SymEnumerateSymbols64
@ stub SymEnumerateSymbolsW
@ stub SymEnumerateSymbolsW64
@ stdcall SymFindFileInPath(long str str ptr long long long ptr ptr ptr)
@ stdcall SymFromAddr(ptr long long ptr ptr)
@ stdcall SymFromName(long str ptr)
@ stdcall SymFunctionTableAccess(long long)
@ stub SymFunctionTableAccess64
@ stub SymGetFileLineOffsets64
@ stdcall SymGetLineFromAddr(long long ptr ptr)
@ stub SymGetLineFromAddr64
@ stub SymGetLineFromName
@ stub SymGetLineFromName64
@ stdcall SymGetLineNext(long ptr)
@ stub SymGetLineNext64
@ stdcall SymGetLinePrev(long ptr)
@ stub SymGetLinePrev64
@ stdcall SymGetModuleBase(long long)
@ stub SymGetModuleBase64
@ stdcall SymGetModuleInfo(long long ptr)
@ stub SymGetModuleInfo64
@ stub SymGetModuleInfoW
@ stub SymGetModuleInfoW64
@ stdcall SymGetOptions()
@ stdcall SymGetSearchPath(long str long)
@ stdcall SymGetSymFromAddr(long long ptr ptr)
@ stub SymGetSymFromAddr64
@ stdcall SymGetSymFromName(long str ptr)
@ stub SymGetSymFromName64
@ stdcall SymGetSymNext(long ptr)
@ stub SymGetSymNext64
@ stdcall SymGetSymPrev(long ptr)
@ stub SymGetSymPrev64
@ stdcall SymGetTypeFromName(ptr long long str ptr)
@ stdcall SymGetTypeInfo(ptr long long long long ptr)
@ stdcall SymInitialize(long str long)
@ stdcall SymLoadModule(long long str str long long)
@ stub SymLoadModule64
@ stub SymLoadModuleEx
@ stdcall SymMatchFileName(str str ptr ptr)
@ stub SymMatchString
@ stdcall SymRegisterCallback(long ptr ptr)
@ stub SymRegisterCallback64
@ stub SymRegisterFunctionEntryCallback
@ stub SymRegisterFunctionEntryCallback64
@ stdcall SymSetContext(long ptr ptr)
@ stdcall SymSetOptions(long)
@ stdcall SymSetSearchPath(long str)
@ stub SymSetSymWithAddr64
@ stdcall SymUnDName(ptr str long)
@ stub SymUnDName64
@ stdcall SymUnloadModule(long long)
@ stub SymUnloadModule64
@ stdcall UnDecorateSymbolName(str str long long)
@ stdcall UnmapDebugInformation(ptr)
@ stub WinDbgExtensionDllInit
#@ stub dbghelp
#@ stub dh
#@ stub lm
#@ stub lmi
#@ stub omap
#@ stub srcfiles
#@ stub sym
#@ stub vc7fpo
