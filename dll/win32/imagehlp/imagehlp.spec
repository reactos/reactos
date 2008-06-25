@ stdcall BindImage(str str str)
@ stdcall BindImageEx(long str str str ptr)
@ stdcall CheckSumMappedFile(ptr long ptr ptr)
@ stdcall EnumerateLoadedModules(long ptr ptr) dbghelp.EnumerateLoadedModules
@ stdcall EnumerateLoadedModules64(long ptr ptr) dbghelp.EnumerateLoadedModules64
@ stdcall FindDebugInfoFile(str str str) dbghelp.FindDebugInfoFile
@ stdcall FindDebugInfoFileEx(str str ptr ptr ptr) dbghelp.FindDebugInfoFileEx
@ stdcall FindExecutableImage(str str str) dbghelp.FindExecutableImage
@ stdcall FindExecutableImageEx(str str ptr ptr ptr) dbghelp.FindExecutableImageEx
@ stub FindFileInPath
@ stub FindFileInSearchPath
@ stdcall GetImageConfigInformation(ptr ptr)
@ stdcall GetImageUnusedHeaderBytes(ptr ptr)
@ stdcall GetTimestampForLoadedLibrary(long) dbghelp.GetTimestampForLoadedLibrary
@ stdcall ImageAddCertificate(long ptr ptr)
@ stdcall ImageDirectoryEntryToData(ptr long long ptr)
@ stdcall ImageDirectoryEntryToDataEx(ptr long long ptr ptr)
@ stdcall ImageEnumerateCertificates(long long ptr ptr long)
@ stdcall ImageGetCertificateData(long long ptr ptr)
@ stdcall ImageGetCertificateHeader(long long ptr)
@ stdcall ImageGetDigestStream(long long ptr long)
@ stdcall ImagehlpApiVersion() dbghelp.ImagehlpApiVersion
@ stdcall ImagehlpApiVersionEx(ptr) dbghelp.ImagehlpApiVersionEx
@ stdcall ImageLoad(str str)
@ stdcall ImageNtHeader(ptr) ntdll.RtlImageNtHeader
@ stdcall ImageRemoveCertificate(long long)
@ stdcall ImageRvaToSection(ptr ptr long) ntdll.RtlImageRvaToSection
@ stdcall ImageRvaToVa(ptr ptr long ptr) ntdll.RtlImageRvaToVa
@ stdcall ImageUnload(ptr)
@ stdcall MakeSureDirectoryPathExists(str) dbghelp.MakeSureDirectoryPathExists
@ stdcall MapAndLoad(str str ptr long long)
@ stdcall MapDebugInformation(long str str long) dbghelp.MapDebugInformation
@ stdcall MapFileAndCheckSumA(str ptr ptr)
@ stdcall MapFileAndCheckSumW(wstr ptr ptr)
@ stdcall ReBaseImage(str str long long long long ptr ptr ptr ptr long)
@ stub ReBaseImage64
@ stdcall RemovePrivateCvSymbolic(ptr ptr ptr)
@ stub RemovePrivateCvSymbolicEx
@ stdcall RemoveRelocations(ptr)
@ stdcall SearchTreeForFile(str str str) dbghelp.SearchTreeForFile
@ stdcall SetImageConfigInformation(ptr ptr)
@ stdcall SplitSymbols(str str str long)
@ stdcall StackWalk(long long long ptr ptr ptr ptr ptr ptr) dbghelp.StackWalk
@ stdcall StackWalk64(long long long ptr ptr ptr ptr ptr ptr) dbghelp.StackWalk64
@ stdcall SymCleanup(long) dbghelp.SymCleanup
@ stdcall SymEnumerateModules(long ptr ptr) dbghelp.SymEnumerateModules
@ stdcall SymEnumerateModules64(long ptr ptr) dbghelp.SymEnumerateModules64
@ stdcall SymEnumerateSymbols(long long ptr ptr) dbghelp.SymEnumerateSymbols
@ stub SymEnumerateSymbols64
@ stdcall SymEnumerateSymbolsW(long long ptr ptr) dbghelp.SymEnumerateSymbolsW
@ stub SymEnumerateSymbolsW64
@ stdcall SymEnumSourceFiles(long long str ptr ptr) dbghelp.SymEnumSourceFiles
@ stub SymEnumSym
@ stdcall SymEnumSymbols(long double str ptr ptr) dbghelp.SymEnumSymbols
@ stdcall SymEnumTypes(long long ptr ptr) dbghelp.SymEnumTypes
@ stdcall SymFindFileInPath(long str str ptr long long long ptr ptr ptr) dbghelp.SymFindFileInPath
@ stdcall SymFromAddr(long long ptr ptr) dbghelp.SymFromAddr
@ stdcall SymFromName(long str ptr) dbghelp.SymFromName
@ stdcall SymFunctionTableAccess(long long) dbghelp.SymFunctionTableAccess
@ stdcall SymFunctionTableAccess64(long double) dbghelp.SymFunctionTableAccess64
@ stdcall SymGetLineFromAddr(long long ptr ptr) dbghelp.SymGetLineFromAddr
@ stdcall SymGetLineFromAddr64(long double ptr ptr) dbghelp.SymGetLineFromAddr64
@ stdcall SymGetLineFromName(long str str long ptr ptr) dbghelp.SymGetLineFromName
@ stdcall SymGetLineFromName64(long str str long ptr ptr) dbghelp.SymGetLineFromName
@ stdcall SymGetLineNext(long ptr) dbghelp.SymGetLineNext
@ stdcall SymGetLineNext64(long ptr) dbghelp.SymGetLineNext64
@ stdcall SymGetLinePrev(long ptr) dbghelp.SymGetLinePrev
@ stdcall SymGetLinePrev64(long ptr) dbghelp.SymGetLinePrev64
@ stdcall SymGetModuleBase(long long) dbghelp.SymGetModuleBase
@ stdcall SymGetModuleBase64(long double) dbghelp.SymGetModuleBase64
@ stdcall SymGetModuleInfo(long long ptr) dbghelp.SymGetModuleInfo
@ stdcall SymGetModuleInfo64(long double ptr) dbghelp.SymGetModuleInfo64
@ stdcall SymGetModuleInfoW(long long ptr) dbghelp.SymGetModuleInfoW
@ stdcall SymGetModuleInfoW64(long double ptr) dbghelp.SymGetModuleInfoW64
@ stdcall SymGetOptions() dbghelp.SymGetOptions
@ stdcall SymGetSearchPath(long str long) dbghelp.SymGetSearchPath
@ stdcall SymGetSymFromAddr(long long ptr ptr) dbghelp.SymGetSymFromAddr
@ stdcall SymGetSymFromAddr64(long double ptr ptr) dbghelp.SymGetSymFromAddr64
@ stdcall SymGetSymFromName(long str ptr) dbghelp.SymGetSymFromName
@ stub SymGetSymFromName64
@ stdcall SymGetSymNext(long ptr) dbghelp.SymGetSymNext
@ stub SymGetSymNext64
@ stdcall SymGetSymPrev(long ptr) dbghelp.SymGetSymPrev
@ stub SymGetSymPrev64
@ stdcall SymGetTypeFromName(long long str ptr) dbghelp.SymGetTypeFromName
@ stdcall SymGetTypeInfo(long long long long ptr) dbghelp.SymGetTypeInfo
@ stdcall SymInitialize(long str long) dbghelp.SymInitialize
@ stdcall SymLoadModule(long long str str long long) dbghelp.SymLoadModule
@ stdcall SymLoadModule64(long long str str double long) dbghelp.SymLoadModule64
@ stdcall SymMatchFileName(str str ptr ptr) dbghelp.SymMatchFileName
@ stdcall SymMatchString(str str long) dbghelp.SymMatchString
@ stdcall SymRegisterCallback(long ptr ptr) dbghelp.SymRegisterCallback
@ stdcall SymRegisterCallback64(long ptr double) dbghelp.SymRegisterCallback64
@ stdcall SymRegisterFunctionEntryCallback(ptr ptr ptr) dbghelp.SymRegisterFunctionEntryCallback
@ stdcall SymRegisterFunctionEntryCallback64(ptr ptr double) dbghelp.SymRegisterFunctionEntryCallback64
@ stdcall SymSetContext(long ptr ptr) dbghelp.SymSetContext
@ stdcall SymSetOptions(long) dbghelp.SymSetOptions
@ stdcall SymSetSearchPath(long str) dbghelp.SymSetSearchPath
@ stdcall SymUnDName(ptr str long) dbghelp.SymUnDName
@ stub SymUnDName64
@ stdcall SymUnloadModule(long long) dbghelp.SymUnloadModule
@ stdcall SymUnloadModule64(long double) dbghelp.SymUnloadModule64
@ stdcall TouchFileTimes(long ptr)
@ stdcall UnDecorateSymbolName(str str long long) dbghelp.UnDecorateSymbolName
@ stdcall UnMapAndLoad(ptr)
@ stdcall UnmapDebugInformation(ptr) dbghelp.UnmapDebugInformation
@ stdcall UpdateDebugInfoFile(str str str ptr)
@ stdcall UpdateDebugInfoFileEx(str str str ptr long)