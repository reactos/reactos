@ stdcall BindMoniker(ptr long ptr ptr)
@ stub CLIPFORMAT_UserFree64
@ stdcall CLIPFORMAT_UserFree(ptr ptr) combase.CLIPFORMAT_UserFree
@ stub CLIPFORMAT_UserMarshal64
@ stdcall CLIPFORMAT_UserMarshal(ptr ptr ptr) combase.CLIPFORMAT_UserMarshal
@ stub CLIPFORMAT_UserSize64
@ stdcall CLIPFORMAT_UserSize(ptr long ptr) combase.CLIPFORMAT_UserSize
@ stub CLIPFORMAT_UserUnmarshal64
@ stdcall CLIPFORMAT_UserUnmarshal(ptr ptr ptr) combase.CLIPFORMAT_UserUnmarshal
@ stdcall CLSIDFromProgID(wstr ptr) combase.CLSIDFromProgID
@ stdcall CLSIDFromProgIDEx(wstr ptr) combase.CLSIDFromProgIDEx
@ stdcall CLSIDFromString(wstr ptr) combase.CLSIDFromString
@ stdcall CoAddRefServerProcess() combase.CoAddRefServerProcess
@ stdcall CoAllowSetForegroundWindow(ptr ptr)
@ stdcall CoBuildVersion()
@ stub CoGetComCatalog
@ stdcall CoCopyProxy(ptr ptr) combase.CoCopyProxy
@ stdcall CoCreateFreeThreadedMarshaler(ptr ptr) combase.CoCreateFreeThreadedMarshaler
@ stdcall CoCreateGuid(ptr) combase.CoCreateGuid
@ stdcall CoCreateInstance(ptr ptr long ptr ptr) combase.CoCreateInstance
@ stdcall CoCreateInstanceEx(ptr ptr long ptr long ptr) combase.CoCreateInstanceEx
@ stdcall CoCreateInstanceFromApp(ptr ptr long ptr long ptr) combase.CoCreateInstanceFromApp
@ stdcall CoDecrementMTAUsage(ptr) combase.CoDecrementMTAUsage
@ stdcall CoDisableCallCancellation(ptr) combase.CoDisableCallCancellation
@ stdcall CoDisconnectObject(ptr long) combase.CoDisconnectObject
@ stdcall CoDosDateTimeToFileTime(long long ptr) kernel32.DosDateTimeToFileTime
@ stdcall CoEnableCallCancellation(ptr) combase.CoEnableCallCancellation
@ stdcall CoFileTimeNow(ptr) combase.CoFileTimeNow
@ stdcall CoFileTimeToDosDateTime(ptr ptr ptr) kernel32.FileTimeToDosDateTime
@ stdcall CoFreeAllLibraries()
@ stdcall CoFreeLibrary(long)
@ stdcall CoFreeUnusedLibraries() combase.CoFreeUnusedLibraries
@ stdcall CoFreeUnusedLibrariesEx(long long) combase.CoFreeUnusedLibrariesEx
@ stdcall CoGetActivationState(int128 long ptr) combase.CoGetActivationState
@ stdcall CoGetApartmentType(ptr ptr) combase.CoGetApartmentType
@ stdcall CoGetCallContext(ptr ptr) combase.CoGetCallContext
@ stdcall CoGetCallState(long ptr) combase.CoGetCallState
@ stdcall CoGetCallerTID(ptr) combase.CoGetCallerTID
@ stdcall CoGetClassObject(ptr long ptr ptr ptr) combase.CoGetClassObject
@ stdcall CoGetContextToken(ptr) combase.CoGetContextToken
@ stdcall CoGetCurrentLogicalThreadId(ptr) combase.CoGetCurrentLogicalThreadId
@ stdcall CoGetCurrentProcess() combase.CoGetCurrentProcess
@ stdcall CoGetDefaultContext(long ptr ptr) combase.CoGetDefaultContext
@ stdcall CoGetInstanceFromFile(ptr ptr ptr long long wstr long ptr) combase.CoGetInstanceFromFile
@ stdcall CoGetInstanceFromIStorage(ptr ptr ptr long ptr long ptr) combase.CoGetInstanceFromIStorage
@ stdcall -stub CoGetInterceptor(ptr ptr ptr ptr)
@ stdcall -stub CoGetInterceptorFromTypeInfo(ptr ptr ptr ptr ptr)
@ stdcall CoGetInterfaceAndReleaseStream(ptr ptr ptr) combase.CoGetInterfaceAndReleaseStream
@ stdcall CoGetMalloc(long ptr) combase.CoGetMalloc
@ stdcall CoGetMarshalSizeMax(ptr ptr ptr long ptr long) combase.CoGetMarshalSizeMax
@ stdcall CoGetObject(wstr ptr ptr ptr)
@ stdcall CoGetObjectContext(ptr ptr) combase.CoGetObjectContext
@ stdcall CoGetPSClsid(ptr ptr) combase.CoGetPSClsid
@ stdcall CoGetStandardMarshal(ptr ptr long ptr long ptr) combase.CoGetStandardMarshal
@ stdcall CoGetState(ptr)
@ stdcall -stub CoGetStdMarshalEx(ptr long ptr)
@ stub CoGetTIDFromIPID
@ stdcall CoGetTreatAsClass(ptr ptr) combase.CoGetTreatAsClass
@ stdcall CoImpersonateClient() combase.CoImpersonateClient
@ stdcall CoIncrementMTAUsage(ptr) combase.CoIncrementMTAUsage
@ stdcall CoInitialize(ptr)
@ stdcall CoInitializeEx(ptr long) combase.CoInitializeEx
@ stdcall CoInitializeSecurity(ptr long ptr ptr long long ptr long ptr) combase.CoInitializeSecurity
@ stdcall CoInitializeWOW(long long) combase.CoInitializeWOW
@ stdcall CoIsHandlerConnected(ptr) combase.CoIsHandlerConnected
@ stdcall CoIsOle1Class (ptr)
@ stdcall CoLoadLibrary(wstr long)
@ stdcall CoLockObjectExternal(ptr long long) combase.CoLockObjectExternal
@ stdcall CoMarshalHresult(ptr long) combase.CoMarshalHresult
@ stdcall CoMarshalInterThreadInterfaceInStream(ptr ptr ptr) combase.CoMarshalInterThreadInterfaceInStream
@ stdcall CoMarshalInterface(ptr ptr ptr long ptr long) combase.CoMarshalInterface
@ stub CoInstall
@ stub CoQueryAuthenticationServices
@ stdcall CoQueryClientBlanket(ptr ptr ptr ptr ptr ptr ptr) combase.CoQueryClientBlanket
@ stdcall CoQueryProxyBlanket(ptr ptr ptr ptr ptr ptr ptr ptr) combase.CoQueryProxyBlanket
@ stub CoQueryReleaseObject
@ stdcall CoRegisterChannelHook(ptr ptr) combase.CoRegisterChannelHook
@ stdcall CoRegisterClassObject(ptr ptr long long ptr) combase.CoRegisterClassObject
@ stdcall CoRegisterInitializeSpy(ptr ptr) combase.CoRegisterInitializeSpy
@ stdcall CoRegisterMallocSpy(ptr) combase.CoRegisterMallocSpy
@ stdcall CoRegisterMessageFilter(ptr ptr) combase.CoRegisterMessageFilter
@ stdcall CoRegisterPSClsid(ptr ptr) combase.CoRegisterPSClsid
@ stdcall CoRegisterSurrogate(ptr) combase.CoRegisterSurrogate
@ stdcall CoRegisterSurrogateEx(ptr ptr) combase.CoRegisterSurrogateEx
@ stdcall CoReleaseMarshalData(ptr) combase.CoReleaseMarshalData
@ stdcall CoReleaseServerProcess() combase.CoReleaseServerProcess
@ stdcall CoResumeClassObjects() combase.CoResumeClassObjects
@ stdcall CoRevertToSelf() combase.CoRevertToSelf
@ stdcall CoRevokeClassObject(long) combase.CoRevokeClassObject
@ stdcall CoRevokeInitializeSpy(int64) combase.CoRevokeInitializeSpy
@ stdcall CoRevokeMallocSpy() combase.CoRevokeMallocSpy
@ stdcall CoSetProxyBlanket(ptr long long ptr long long ptr long) combase.CoSetProxyBlanket
@ stdcall CoSetState(ptr)
@ stdcall CoSuspendClassObjects() combase.CoSuspendClassObjects
@ stdcall CoSwitchCallContext(ptr ptr) combase.CoSwitchCallContext
@ stdcall CoTaskMemAlloc(long) combase.CoTaskMemAlloc
@ stdcall CoTaskMemFree(ptr) combase.CoTaskMemFree
@ stdcall CoTaskMemRealloc(ptr long) combase.CoTaskMemRealloc
@ stdcall CoTreatAsClass(ptr ptr)
@ stdcall CoUninitialize() combase.CoUninitialize
@ stub CoUnloadingWOW
@ stdcall CoUnmarshalHresult(ptr ptr) combase.CoUnmarshalHresult
@ stdcall CoUnmarshalInterface(ptr ptr ptr) combase.CoUnmarshalInterface
@ stdcall CoWaitForMultipleHandles(long long long ptr ptr) combase.CoWaitForMultipleHandles
@ stub CreateStdProgressIndicator
@ stdcall CreateAntiMoniker(ptr)
@ stdcall CreateBindCtx(long ptr)
@ stdcall CreateClassMoniker(ptr ptr)
@ stdcall CreateDataAdviseHolder(ptr)
@ stdcall CreateDataCache(ptr ptr ptr ptr)
@ stdcall CreateErrorInfo(ptr) combase.CreateErrorInfo
@ stdcall CreateFileMoniker(wstr ptr)
@ stdcall CreateGenericComposite(ptr ptr ptr)
@ stdcall CreateILockBytesOnHGlobal(ptr long ptr)
@ stdcall CreateItemMoniker(wstr wstr ptr)
@ stdcall CreateObjrefMoniker(ptr ptr)
@ stdcall CreateOleAdviseHolder(ptr)
@ stdcall CreatePointerMoniker(ptr ptr)
@ stdcall CreateStreamOnHGlobal(ptr long ptr) combase.CreateStreamOnHGlobal
@ stdcall DestroyRunningObjectTable()
@ stdcall DllDebugObjectRPCHook(long ptr) combase.DllDebugObjectRPCHook
@ stdcall DllGetClassObject (ptr ptr ptr)
@ stub DllGetClassObjectWOW
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stdcall DoDragDrop(ptr ptr long ptr)
@ stub EnableHookObject
@ stdcall FmtIdToPropStgName(ptr wstr)
@ stdcall FreePropVariantArray(long ptr) combase.FreePropVariantArray
@ stdcall GetClassFile(wstr ptr)
@ stdcall GetConvertStg(ptr)
@ stub GetDocumentBitStg
@ stdcall GetErrorInfo(long ptr) combase.GetErrorInfo
@ stdcall GetHGlobalFromILockBytes(ptr ptr)
@ stdcall GetHGlobalFromStream(ptr ptr) combase.GetHGlobalFromStream
@ stub GetHookInterface
@ stdcall GetRunningObjectTable(long ptr)
@ stub HACCEL_UserFree64
@ stdcall HACCEL_UserFree(ptr ptr) combase.HACCEL_UserFree
@ stub HACCEL_UserMarshal64
@ stdcall HACCEL_UserMarshal(ptr ptr ptr) combase.HACCEL_UserMarshal
@ stub HACCEL_UserSize64
@ stdcall HACCEL_UserSize(ptr long ptr) combase.HACCEL_UserSize
@ stub HACCEL_UserUnmarshal64
@ stdcall HACCEL_UserUnmarshal(ptr ptr ptr) combase.HACCEL_UserUnmarshal
@ stub HBITMAP_UserFree64
@ stdcall HBITMAP_UserFree(ptr ptr) combase.HBITMAP_UserFree
@ stub HBITMAP_UserMarshal64
@ stdcall HBITMAP_UserMarshal(ptr ptr ptr) combase.HBITMAP_UserMarshal
@ stub HBITMAP_UserSize64
@ stdcall HBITMAP_UserSize(ptr long ptr) combase.HBITMAP_UserSize
@ stub HBITMAP_UserUnmarshal64
@ stdcall HBITMAP_UserUnmarshal(ptr ptr ptr) combase.HBITMAP_UserUnmarshal
@ stub HBRUSH_UserFree64
@ stdcall HBRUSH_UserFree(ptr ptr) combase.HBRUSH_UserFree
@ stub HBRUSH_UserMarshal64
@ stdcall HBRUSH_UserMarshal(ptr ptr ptr) combase.HBRUSH_UserMarshal
@ stub HBRUSH_UserSize64
@ stdcall HBRUSH_UserSize(ptr long ptr) combase.HBRUSH_UserSize
@ stub HBRUSH_UserUnmarshal64
@ stdcall HBRUSH_UserUnmarshal(ptr ptr ptr) combase.HBRUSH_UserUnmarshal
@ stub HDC_UserFree64
@ stdcall HDC_UserFree(ptr ptr) combase.HDC_UserFree
@ stub HDC_UserMarshal64
@ stdcall HDC_UserMarshal(ptr ptr ptr) combase.HDC_UserMarshal
@ stub HDC_UserSize64
@ stdcall HDC_UserSize(ptr long ptr) combase.HDC_UserSize
@ stub HDC_UserUnmarshal64
@ stdcall HDC_UserUnmarshal(ptr ptr ptr) combase.HDC_UserUnmarshal
@ stub HENHMETAFILE_UserFree64
@ stdcall HENHMETAFILE_UserFree(ptr ptr)
@ stub HENHMETAFILE_UserMarshal64
@ stdcall HENHMETAFILE_UserMarshal(ptr ptr ptr)
@ stub HENHMETAFILE_UserSize64
@ stdcall HENHMETAFILE_UserSize(ptr long ptr)
@ stub HENHMETAFILE_UserUnmarshal64
@ stdcall HENHMETAFILE_UserUnmarshal(ptr ptr ptr)
@ stub HGLOBAL_UserFree64
@ stdcall HGLOBAL_UserFree(ptr ptr) combase.HGLOBAL_UserFree
@ stub HGLOBAL_UserMarshal64
@ stdcall HGLOBAL_UserMarshal(ptr ptr ptr) combase.HGLOBAL_UserMarshal
@ stub HGLOBAL_UserSize64
@ stdcall HGLOBAL_UserSize(ptr long ptr) combase.HGLOBAL_UserSize
@ stub HGLOBAL_UserUnmarshal64
@ stdcall HGLOBAL_UserUnmarshal(ptr ptr ptr) combase.HGLOBAL_UserUnmarshal
@ stub HICON_UserFree64
@ stdcall HICON_UserFree(ptr ptr) combase.HICON_UserFree
@ stub HICON_UserMarshal64
@ stdcall HICON_UserMarshal(ptr ptr ptr) combase.HICON_UserMarshal
@ stub HICON_UserSize64
@ stdcall HICON_UserSize(ptr long ptr) combase.HICON_UserSize
@ stub HICON_UserUnmarshal64
@ stdcall HICON_UserUnmarshal(ptr ptr ptr) combase.HICON_UserUnmarshal
@ stub HMENU_UserFree64
@ stdcall HMENU_UserFree(ptr ptr) combase.HMENU_UserFree
@ stub HMENU_UserMarshal64
@ stdcall HMENU_UserMarshal(ptr ptr ptr) combase.HMENU_UserMarshal
@ stub HMENU_UserSize64
@ stdcall HMENU_UserSize(ptr long ptr) combase.HMENU_UserSize
@ stub HMENU_UserUnmarshal64
@ stdcall HMENU_UserUnmarshal(ptr ptr ptr) combase.HMENU_UserUnmarshal
@ stub HMETAFILE_UserFree64
@ stdcall HMETAFILE_UserFree(ptr ptr)
@ stub HMETAFILE_UserMarshal64
@ stdcall HMETAFILE_UserMarshal(ptr ptr ptr)
@ stub HMETAFILE_UserSize64
@ stdcall HMETAFILE_UserSize(ptr long ptr)
@ stub HMETAFILE_UserUnmarshal64
@ stdcall HMETAFILE_UserUnmarshal(ptr ptr ptr)
@ stub HMETAFILEPICT_UserFree64
@ stdcall HMETAFILEPICT_UserFree(ptr ptr)
@ stub HMETAFILEPICT_UserMarshal64
@ stdcall HMETAFILEPICT_UserMarshal(ptr ptr ptr)
@ stub HMETAFILEPICT_UserSize64
@ stdcall HMETAFILEPICT_UserSize(ptr long ptr)
@ stub HMETAFILEPICT_UserUnmarshal64
@ stdcall HMETAFILEPICT_UserUnmarshal(ptr ptr ptr)
@ stub HPALETTE_UserFree64
@ stdcall HPALETTE_UserFree(ptr ptr) combase.HPALETTE_UserFree
@ stub HPALETTE_UserMarshal64
@ stdcall HPALETTE_UserMarshal(ptr ptr ptr) combase.HPALETTE_UserMarshal
@ stub HPALETTE_UserSize64
@ stdcall HPALETTE_UserSize(ptr long ptr) combase.HPALETTE_UserSize
@ stub HPALETTE_UserUnmarshal64
@ stdcall HPALETTE_UserUnmarshal(ptr ptr ptr) combase.HPALETTE_UserUnmarshal
@ stub HRGN_UserFree
@ stub HRGN_UserMarshal
@ stub HRGN_UserSize
@ stub HRGN_UserUnmarshal
@ stub HWND_UserFree64
@ stdcall HWND_UserFree(ptr ptr) combase.HWND_UserFree
@ stub HWND_UserMarshal64
@ stdcall HWND_UserMarshal(ptr ptr ptr) combase.HWND_UserMarshal
@ stub HWND_UserSize64
@ stdcall HWND_UserSize(ptr long ptr) combase.HWND_UserSize
@ stub HWND_UserUnmarshal64
@ stdcall HWND_UserUnmarshal(ptr ptr ptr) combase.HWND_UserUnmarshal
@ stdcall IIDFromString(wstr ptr) combase.IIDFromString
@ stub I_RemoteMain
@ stdcall IsAccelerator(long long ptr ptr)
@ stdcall IsEqualGUID(ptr ptr)
@ stub IsValidIid
@ stdcall IsValidInterface(ptr)
@ stub IsValidPtrIn
@ stub IsValidPtrOut
@ stdcall MkParseDisplayName(ptr wstr ptr ptr)
@ stdcall MonikerCommonPrefixWith(ptr ptr ptr)
@ stub MonikerRelativePathTo
@ stdcall Ole32DllGetClassObject(ptr ptr ptr)
@ stdcall OleBuildVersion()
@ stub OleConvertOLESTREAMToIStorageEx
@ stdcall OleConvertIStorageToOLESTREAM(ptr ptr)
@ stdcall OleConvertIStorageToOLESTREAMEx(ptr long long long long ptr ptr)
@ stdcall OleConvertOLESTREAMToIStorage(ptr ptr ptr)
@ stub OleCreateEx
@ stdcall OleCreate(ptr ptr long ptr ptr ptr ptr)
@ stdcall OleCreateDefaultHandler(ptr ptr ptr ptr)
@ stdcall OleCreateEmbeddingHelper(ptr ptr long ptr ptr ptr)
@ stdcall OleCreateFromData(ptr ptr long ptr ptr ptr ptr)
@ stdcall OleCreateFromDataEx(ptr ptr long long long ptr ptr ptr ptr ptr ptr ptr)
@ stdcall OleCreateFromFile(ptr wstr ptr long ptr ptr ptr ptr)
@ stdcall OleCreateFromFileEx(ptr wstr ptr long long long ptr ptr ptr ptr ptr ptr ptr)
@ stdcall OleCreateLink(ptr ptr long ptr ptr ptr ptr)
@ stub OleCreateLinkEx
@ stdcall OleCreateLinkFromData(ptr ptr long ptr ptr ptr ptr)
@ stub OleCreateLinkFromDataEx
@ stdcall OleCreateLinkToFile(ptr ptr long ptr ptr ptr ptr)
@ stub OleCreateLinkToFileEx
@ stdcall OleCreateMenuDescriptor(long ptr)
@ stdcall OleCreateStaticFromData(ptr ptr long ptr ptr ptr ptr)
@ stdcall OleDestroyMenuDescriptor(long)
@ stdcall OleDoAutoConvert(ptr ptr)
@ stdcall OleDraw(ptr long long ptr)
@ stdcall OleDuplicateData(long long long)
@ stdcall OleFlushClipboard()
@ stdcall OleGetAutoConvert(ptr ptr)
@ stdcall OleGetClipboard(ptr)
@ stdcall OleGetIconOfClass(ptr ptr long)
@ stdcall OleGetIconOfFile(wstr long)
@ stdcall OleInitialize(ptr)
@ stdcall OleInitializeWOW(long long)
@ stdcall OleIsCurrentClipboard(ptr)
@ stdcall OleIsRunning(ptr)
@ stdcall OleLoad(ptr ptr ptr ptr)
@ stdcall OleLoadFromStream(ptr ptr ptr)
@ stdcall OleLockRunning(ptr long long)
@ stdcall OleMetafilePictFromIconAndLabel(long ptr ptr long)
@ stdcall OleNoteObjectVisible(ptr long)
@ stdcall OleQueryCreateFromData(ptr)
@ stdcall OleQueryLinkFromData(ptr)
@ stdcall OleRegEnumFormatEtc(ptr long ptr)
@ stdcall OleRegEnumVerbs(ptr ptr)
@ stdcall OleRegGetMiscStatus(ptr long ptr)
@ stdcall OleRegGetUserType(ptr long ptr)
@ stdcall OleRun(ptr)
@ stdcall OleSave(ptr ptr long)
@ stdcall OleSaveToStream(ptr ptr)
@ stdcall OleSetAutoConvert(ptr ptr)
@ stdcall OleSetClipboard(ptr)
@ stdcall OleSetContainedObject(ptr long)
@ stdcall OleSetMenuDescriptor(long long long ptr ptr)
@ stdcall OleTranslateAccelerator(ptr ptr ptr)
@ stdcall OleUninitialize()
@ stub OpenOrCreateStream
@ stdcall ProgIDFromCLSID(ptr ptr) combase.ProgIDFromCLSID
@ stdcall PropStgNameToFmtId(wstr ptr)
@ stdcall PropSysAllocString(wstr)
@ stdcall PropSysFreeString(wstr)
@ stdcall PropVariantClear(ptr) combase.PropVariantClear
@ stdcall PropVariantCopy(ptr ptr) combase.PropVariantCopy
@ stdcall ReadClassStg(ptr ptr)
@ stdcall ReadClassStm(ptr ptr)
@ stdcall ReadFmtUserTypeStg(ptr ptr ptr)
@ stub ReadOleStg
@ stub ReadStringStream
@ stdcall RegisterDragDrop(long ptr)
@ stdcall ReleaseStgMedium(ptr)
@ stdcall RevokeDragDrop(long)
@ stdcall RoGetAgileReference(long ptr ptr ptr) combase.RoGetAgileReference
@ stdcall SNB_UserFree(ptr ptr)
@ stdcall SNB_UserMarshal(ptr ptr ptr)
@ stdcall SNB_UserSize(ptr long ptr)
@ stdcall SNB_UserUnmarshal(ptr ptr ptr)
@ stdcall STGMEDIUM_UserFree(ptr ptr)
@ stdcall STGMEDIUM_UserMarshal(ptr ptr ptr)
@ stdcall STGMEDIUM_UserSize(ptr long ptr)
@ stdcall STGMEDIUM_UserUnmarshal(ptr ptr ptr)
@ stdcall SetConvertStg(ptr long)
@ stub SetDocumentBitStg
@ stdcall SetErrorInfo(long ptr) combase.SetErrorInfo
@ stdcall StgConvertPropertyToVariant(ptr long ptr ptr)
@ stdcall StgConvertVariantToProperty(ptr long ptr ptr long long ptr)
@ stdcall StgCreateDocfile(wstr long long ptr)
@ stdcall StgCreateDocfileOnILockBytes(ptr long long ptr)
@ stdcall StgCreatePropSetStg(ptr long ptr)
@ stdcall StgCreatePropStg(ptr ptr ptr long long ptr)
@ stdcall StgCreateStorageEx(wstr long long long ptr ptr ptr ptr)
@ stub StgGetIFillLockBytesOnFile
@ stub StgGetIFillLockBytesOnILockBytes
@ stdcall StgIsStorageFile(wstr)
@ stdcall StgIsStorageILockBytes(ptr)
@ stub StgOpenAsyncDocfileOnIFillLockBytes
@ stdcall StgOpenPropStg(ptr ptr long long ptr)
@ stdcall StgOpenStorage(wstr ptr long ptr long ptr)
@ stdcall StgOpenStorageEx(wstr long long long ptr ptr ptr ptr)
@ stdcall StgOpenStorageOnILockBytes(ptr ptr long ptr long ptr)
@ stdcall StgSetTimes(wstr ptr ptr ptr )
@ stdcall StringFromCLSID(ptr ptr) combase.StringFromCLSID
@ stdcall StringFromGUID2(ptr ptr long) combase.StringFromGUID2
@ stdcall StringFromIID(ptr ptr) combase.StringFromIID
@ stub UpdateDCOMSettings
@ stub UtConvertDvtd16toDvtd32
@ stub UtConvertDvtd32toDvtd16
@ stub UtGetDvtd16Info
@ stub UtGetDvtd32Info
@ stdcall WdtpInterfacePointer_UserFree(ptr) combase.WdtpInterfacePointer_UserFree
@ stdcall WdtpInterfacePointer_UserMarshal(ptr long ptr ptr ptr) combase.WdtpInterfacePointer_UserMarshal
@ stdcall WdtpInterfacePointer_UserSize(ptr long long ptr ptr) combase.WdtpInterfacePointer_UserSize
@ stdcall WdtpInterfacePointer_UserUnmarshal(ptr ptr ptr ptr) combase.WdtpInterfacePointer_UserUnmarshal
@ stub WdtpInterfacePointer_UserFree64
@ stub WdtpInterfacePointer_UserMarshal64
@ stub WdtpInterfacePointer_UserSize64
@ stub WdtpInterfacePointer_UserUnmarshal64
@ stdcall WriteClassStg(ptr ptr)
@ stdcall WriteClassStm(ptr ptr)
@ stdcall WriteFmtUserTypeStg(ptr long ptr)
@ stub WriteOleStg
@ stub WriteStringStream
