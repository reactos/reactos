@ stdcall BindImage(str str str)
@ stdcall BindImageEx(long str str str ptr)
@ stdcall CheckSumMappedFile(ptr long ptr ptr)
@ stdcall EnumerateLoadedModules64(long ptr ptr) dbghelp.EnumerateLoadedModules64
@ stdcall EnumerateLoadedModules(long ptr ptr) dbghelp.EnumerateLoadedModules
@ stdcall FindDebugInfoFile(str str ptr) dbghelp.FindDebugInfoFile
@ stdcall FindDebugInfoFileEx(str str ptr ptr ptr) dbghelp.FindDebugInfoFileEx
@ stdcall FindExecutableImage(str str str) dbghelp.FindExecutableImage
@ stdcall FindExecutableImageEx(str str ptr ptr ptr) dbghelp.FindExecutableImageEx
@ stub FindFileInPath
@ stub FindFileInSearchPath
@ stdcall GetImageConfigInformation(ptr ptr)
@ stdcall GetImageUnusedHeaderBytes(ptr ptr)
@ stdcall GetTimestampForLoadedLibrary(long) dbghelp.GetTimestampForLoadedLibrary
@ stdcall ImageAddCertificate(long ptr ptr)
@ stdcall ImageDirectoryEntryToData(ptr long long ptr) dbghelp.ImageDirectoryEntryToData
@ stdcall ImageDirectoryEntryToDataEx(ptr long long ptr ptr) dbghelp.ImageDirectoryEntryToDataEx
@ stdcall ImageEnumerateCertificates(long long ptr ptr long)
@ stdcall ImageGetCertificateData(long long ptr ptr)
@ stdcall ImageGetCertificateHeader(long long ptr)
@ stdcall ImageGetDigestStream(long long ptr long)
@ stdcall ImageLoad(str str)
@ stdcall ImageNtHeader(ptr) ntdll.RtlImageNtHeader
@ stdcall ImageRemoveCertificate(long long)
@ stdcall ImageRvaToSection(ptr ptr long) ntdll.RtlImageRvaToSection
@ stdcall ImageRvaToVa(ptr ptr long ptr) ntdll.RtlImageRvaToVa
@ stdcall ImageUnload(ptr)
@ stdcall ImagehlpApiVersion() dbghelp.ImagehlpApiVersion
@ stdcall ImagehlpApiVersionEx(ptr) dbghelp.ImagehlpApiVersionEx
@ stdcall MakeSureDirectoryPathExists(str) dbghelp.MakeSureDirectoryPathExists
@ stdcall MapAndLoad(str str ptr long long)
@ stdcall MapDebugInformation(long str str long) dbghelp.MapDebugInformation
@ stdcall MapFileAndCheckSumA(str ptr ptr)
@ stdcall MapFileAndCheckSumW(wstr ptr ptr)
@ stub  MarkImageAsRunFromSwap
@ stub ReBaseImage64
@ stdcall ReBaseImage(str str long long long long ptr ptr ptr ptr long)
@ stdcall RemovePrivateCvSymbolic(ptr ptr ptr)
@ stub RemovePrivateCvSymbolicEx
@ stdcall RemoveRelocations(ptr)
@ stdcall SearchTreeForFile(str str ptr) dbghelp.SearchTreeForFile
@ stdcall SetImageConfigInformation(ptr ptr)
@ stdcall SplitSymbols(str str str long)
@ stdcall StackWalk64(long long long ptr ptr ptr ptr ptr ptr) dbghelp.StackWalk64
@ stdcall StackWalk(long long long ptr ptr ptr ptr ptr ptr) dbghelp.StackWalk
@ stdcall SymCleanup(long) dbghelp.SymCleanup
@ stdcall SymEnumSourceFiles(ptr int64 str ptr ptr) dbghelp.SymEnumSourceFiles
@ stub SymEnumSym
@ stdcall SymEnumSymbols(ptr int64 str ptr ptr) dbghelp.SymEnumSymbols
@ stdcall SymEnumTypes(ptr int64 ptr ptr) dbghelp.SymEnumTypes
@ stdcall SymEnumerateModules64(long ptr ptr) dbghelp.SymEnumerateModules64
@ stdcall SymEnumerateModules(long ptr ptr) dbghelp.SymEnumerateModules
@ stdcall SymEnumerateSymbols64(long int64 ptr ptr) dbghelp.SymEnumerateSymbols64
@ stdcall SymEnumerateSymbols(long long ptr ptr) dbghelp.SymEnumerateSymbols
@ stub SymEnumerateSymbolsW64
@ stub SymEnumerateSymbolsW
@ stdcall SymFindFileInPath(long str str ptr long long long ptr ptr ptr) dbghelp.SymFindFileInPath
@ stdcall SymFromAddr(ptr int64 ptr ptr) dbghelp.SymFromAddr
@ stdcall SymFromName(long str ptr) dbghelp.SymFromName
@ stdcall SymFunctionTableAccess64(long int64) dbghelp.SymFunctionTableAccess64
@ stdcall SymFunctionTableAccess(long long) dbghelp.SymFunctionTableAccess
@ stdcall SymGetLineFromAddr64(long int64 ptr ptr) dbghelp.SymGetLineFromAddr64
@ stdcall SymGetLineFromAddr(long long ptr ptr) dbghelp.SymGetLineFromAddr
@ stub SymGetLineFromName64
@ stub SymGetLineFromName
@ stdcall SymGetLineNext64(long ptr) dbghelp.SymGetLineNext64
@ stdcall SymGetLineNext(long ptr) dbghelp.SymGetLineNext
@ stdcall SymGetLinePrev64(long ptr) dbghelp.SymGetLinePrev64
@ stdcall SymGetLinePrev(long ptr) dbghelp.SymGetLinePrev
@ stdcall SymGetModuleBase64(long int64) dbghelp.SymGetModuleBase64
@ stdcall SymGetModuleBase(long long) dbghelp.SymGetModuleBase
@ stdcall SymGetModuleInfo64(long int64 ptr) dbghelp.SymGetModuleInfo64
@ stdcall SymGetModuleInfo(long long ptr) dbghelp.SymGetModuleInfo
@ stdcall SymGetModuleInfoW64(long int64 ptr) dbghelp.SymGetModuleInfoW64
@ stdcall SymGetModuleInfoW(long long ptr) dbghelp.SymGetModuleInfoW
@ stdcall SymGetOptions() dbghelp.SymGetOptions
@ stdcall SymGetSearchPath(long ptr long) dbghelp.SymGetSearchPath
@ stdcall SymGetSymFromAddr64(long int64 ptr ptr) dbghelp.SymGetSymFromAddr64
@ stdcall SymGetSymFromAddr(long long ptr ptr) dbghelp.SymGetSymFromAddr
@ stdcall SymGetSymFromName64(long str ptr) dbghelp.SymGetSymFromName64
@ stdcall SymGetSymFromName(long str ptr) dbghelp.SymGetSymFromName
@ stdcall SymGetSymNext64(long ptr) dbghelp.SymGetSymNext64
@ stdcall SymGetSymNext(long ptr) dbghelp.SymGetSymNext
@ stdcall SymGetSymPrev64(long ptr) dbghelp.SymGetSymPrev64
@ stdcall SymGetSymPrev(long ptr) dbghelp.SymGetSymPrev
@ stdcall SymGetTypeFromName(ptr int64 str ptr) dbghelp.SymGetTypeFromName
@ stdcall SymGetTypeInfo(ptr int64 long long ptr) dbghelp.SymGetTypeInfo
@ stdcall SymInitialize(long str long) dbghelp.SymInitialize
@ stdcall SymLoadModule64(long long str str int64 long) dbghelp.SymLoadModule64
@ stdcall SymLoadModule(long long str str long long) dbghelp.SymLoadModule
@ stdcall SymMatchFileName(str str ptr ptr) dbghelp.SymMatchFileName
@ stdcall SymMatchString(str str long) dbghelp.SymMatchString
@ stdcall SymRegisterCallback64(long ptr int64) dbghelp.SymRegisterCallback64
@ stdcall SymRegisterCallback(long ptr ptr) dbghelp.SymRegisterCallback
@ stdcall SymRegisterFunctionEntryCallback64(ptr ptr int64) dbghelp.SymRegisterFunctionEntryCallback64
@ stdcall SymRegisterFunctionEntryCallback(ptr ptr ptr) dbghelp.SymRegisterFunctionEntryCallback
@ stdcall SymSetContext(long ptr ptr) dbghelp.SymSetContext
@ stdcall SymSetOptions(long) dbghelp.SymSetOptions
@ stdcall SymSetSearchPath(long str) dbghelp.SymSetSearchPath
@ stdcall SymUnDName64(ptr str long) dbghelp.SymUnDName64
@ stdcall SymUnDName(ptr str long) dbghelp.SymUnDName
@ stdcall SymUnloadModule64(long int64) dbghelp.SymUnloadModule64
@ stdcall SymUnloadModule(long long) dbghelp.SymUnloadModule
@ stdcall TouchFileTimes(long ptr)
@ stdcall UnDecorateSymbolName(str str long long) dbghelp.UnDecorateSymbolName
@ stdcall UnMapAndLoad(ptr)
@ stdcall UnmapDebugInformation(ptr) dbghelp.UnmapDebugInformation
@ stdcall UpdateDebugInfoFile(str str str ptr)
@ stdcall UpdateDebugInfoFileEx(str str str ptr long)
