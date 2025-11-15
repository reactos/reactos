17 stub InitErrors
18 stub PostError
19 stub InitSSAutoEnterThread
20 stub UpdateError
22 stdcall LoadStringRC(long ptr long long)
23 stub ReOpenMetaDataWithMemory

@ stub CallFunctionShim
@ stub CloseCtrs
@ stdcall CLRCreateInstance(ptr ptr ptr)
@ stdcall ClrCreateManagedInstance(wstr ptr ptr)
@ stdcall CoEEShutDownCOM()
@ stdcall CoInitializeCor(long)
@ stub CoInitializeEE
@ stub CoUninitializeCor
@ stub CoUninitializeEE
@ stub CollectCtrs
@ stdcall CorBindToCurrentRuntime(wstr ptr ptr ptr)
@ stub CorBindToRuntime
@ stub CorBindToRuntimeByCfg
@ stub CorBindToRuntimeByPath
@ stub CorBindToRuntimeByPathEx
@ stdcall CorBindToRuntimeEx(wstr wstr long ptr ptr ptr)
@ stdcall CorBindToRuntimeHost(wstr wstr wstr ptr long ptr ptr ptr)
@ stub CorDllMainWorker
@ stdcall CorExitProcess(long)
@ stdcall CorGetSvc(ptr)
@ stdcall CorIsLatestSvc(ptr ptr)
@ stub CorMarkThreadInThreadPool
@ stub CorTickleSvc
@ stdcall CreateConfigStream(wstr ptr)
@ stdcall CreateDebuggingInterfaceFromVersion(long wstr ptr)
@ stdcall CreateInterface(ptr ptr ptr)
@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stub EEDllGetClassObjectFromClass
@ stub EEDllRegisterServer
@ stub EEDllUnregisterServer
@ stdcall GetAssemblyMDImport(wstr ptr ptr)
@ stub GetCORRequiredVersion
@ stub GetCORRootDirectory
@ stdcall GetCORSystemDirectory(ptr long ptr)
@ stdcall GetCORVersion(ptr long ptr)
@ stub GetCompileInfo
@ stdcall GetFileVersion(wstr ptr long ptr)
@ stub GetHashFromAssemblyFile
@ stub GetHashFromAssemblyFileW
@ stub GetHashFromBlob
@ stub GetHashFromFile
@ stub GetHashFromFileW
@ stub GetHashFromHandle
@ stub GetHostConfigurationFile
@ stub GetMetaDataInternalInterface
@ stub GetMetaDataInternalInterfaceFromPublic
@ stub GetMetaDataPublicInterfaceFromInternal
@ stub GetPermissionRequests
@ stub GetPrivateContextsPerfCounters
@ stub GetProcessExecutableHeap
@ stdcall GetRealProcAddress(str ptr)
@ stdcall GetRequestedRuntimeInfo(wstr wstr wstr long long ptr long ptr ptr long ptr)
@ stdcall GetRequestedRuntimeVersion(wstr ptr long ptr)
@ stub GetRequestedRuntimeVersionForCLSID
@ stub GetStartupFlags
@ stub GetTargetForVTableEntry
@ stdcall GetTokenForVTableEntry(ptr ptr)
@ stdcall GetVersionFromProcess(ptr ptr long ptr)
@ stub GetXMLElement
@ stub GetXMLElementAttribute
@ stub GetXMLObject
@ stdcall LoadLibraryShim(wstr wstr ptr ptr)
@ stub LoadLibraryWithPolicyShim
@ stdcall LoadStringRCEx(long long ptr long long ptr)
@ stdcall LockClrVersion(ptr ptr ptr)
@ stub MetaDataGetDispenser
@ stdcall ND_CopyObjDst(ptr ptr long long)
@ stdcall ND_CopyObjSrc(ptr long ptr long)
@ stdcall ND_RI2(ptr long)
@ stdcall ND_RI4(ptr long)
@ stdcall -ret64 ND_RI8(ptr long)
@ stdcall ND_RU1(ptr long)
@ stdcall ND_WI2(ptr long long)
@ stdcall ND_WI4(ptr long long)
@ stdcall ND_WI8(ptr long int64)
@ stdcall ND_WU1(ptr long long)
@ stub OpenCtrs
@ stub ReOpenMetaDataWithMemoryEx
@ stub RunDll@ShimW
@ stub RuntimeOSHandle
@ stub RuntimeOpenImage
@ stub RuntimeReleaseHandle
@ stub SetTargetForVTableEntry
@ stub StrongNameCompareAssemblies
@ stub StrongNameErrorInfo
@ stub StrongNameFreeBuffer
@ stub StrongNameGetBlob
@ stub StrongNameGetBlobFromImage
@ stub StrongNameGetPublicKey
@ stub StrongNameHashSize
@ stub StrongNameKeyDelete
@ stub StrongNameKeyGen
@ stub StrongNameKeyGenEx
@ stub StrongNameKeyInstall
@ stub StrongNameSignatureGeneration
@ stub StrongNameSignatureGenerationEx
@ stub StrongNameSignatureSize
@ stdcall StrongNameSignatureVerification(wstr long ptr)
@ stdcall StrongNameSignatureVerificationEx(wstr long ptr)
@ stub StrongNameSignatureVerificationFromImage
@ stdcall StrongNameTokenFromAssembly(wstr ptr ptr)
@ stub StrongNameTokenFromAssemblyEx
@ stub StrongNameTokenFromPublicKey
@ stub TranslateSecurityAttributes
@ stdcall _CorDllMain(long long ptr)
@ stdcall _CorExeMain2(ptr long ptr ptr ptr)
@ stdcall _CorExeMain()
@ stdcall _CorImageUnloading(ptr)
@ stdcall _CorValidateImage(ptr wstr)
