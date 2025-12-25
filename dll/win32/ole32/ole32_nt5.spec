@ stdcall BindMoniker(ptr long ptr ptr)
@ stdcall CLIPFORMAT_UserFree(ptr ptr) combase_win10.CLIPFORMAT_UserFree
@ stdcall CLIPFORMAT_UserMarshal(ptr ptr ptr) combase_win10.CLIPFORMAT_UserMarshal
@ stdcall CLIPFORMAT_UserSize(ptr long ptr) combase_win10.CLIPFORMAT_UserSize
@ stdcall CLIPFORMAT_UserUnmarshal(ptr ptr ptr) combase_win10.CLIPFORMAT_UserUnmarshal
@ stdcall CLSIDFromProgID(wstr ptr) combase_win10.CLSIDFromProgID
@ stdcall CLSIDFromProgIDEx(wstr ptr) combase_win10.CLSIDFromProgIDEx
@ stdcall CLSIDFromString(wstr ptr) combase_win10.CLSIDFromString
@ stdcall CoAddRefServerProcess() combase_win10.CoAddRefServerProcess
@ stdcall CoAllowSetForegroundWindow(ptr ptr)
@ stdcall CoBuildVersion()
@ stdcall CoCopyProxy(ptr ptr) combase_win10.CoCopyProxy
@ stdcall CoCreateFreeThreadedMarshaler(ptr ptr) combase_win10.CoCreateFreeThreadedMarshaler
@ stdcall CoCreateGuid(ptr) combase_win10.CoCreateGuid
@ stdcall CoCreateInstance(ptr ptr long ptr ptr) combase_win10.CoCreateInstance
@ stdcall CoCreateInstanceEx(ptr ptr long ptr long ptr) combase_win10.CoCreateInstanceEx
@ stdcall CoCreateInstanceFromApp(ptr ptr long ptr long ptr) combase_win10.CoCreateInstanceFromApp
@ stdcall CoDecrementMTAUsage(ptr) combase_win10.CoDecrementMTAUsage
@ stdcall CoDisableCallCancellation(ptr) combase_win10.CoDisableCallCancellation
@ stdcall CoDisconnectObject(ptr long) combase_win10.CoDisconnectObject
@ stdcall CoDosDateTimeToFileTime(long long ptr) kernel32.DosDateTimeToFileTime
@ stdcall CoEnableCallCancellation(ptr) combase_win10.CoEnableCallCancellation
@ stdcall CoFileTimeNow(ptr) combase_win10.CoFileTimeNow
@ stdcall CoFileTimeToDosDateTime(ptr ptr ptr) kernel32.FileTimeToDosDateTime
@ stdcall CoFreeAllLibraries()
@ stdcall CoFreeLibrary(long)
@ stdcall CoFreeUnusedLibraries() combase_win10.CoFreeUnusedLibraries
@ stdcall CoFreeUnusedLibrariesEx(long long) combase_win10.CoFreeUnusedLibrariesEx
@ stdcall CoGetActivationState(int128 long ptr) combase_win10.CoGetActivationState
@ stdcall CoGetApartmentType(ptr ptr) combase_win10.CoGetApartmentType
@ stdcall CoGetCallContext(ptr ptr) combase_win10.CoGetCallContext
@ stdcall CoGetCallState(long ptr) combase_win10.CoGetCallState
@ stdcall CoGetCallerTID(ptr) combase_win10.CoGetCallerTID
@ stdcall CoGetClassObject(ptr long ptr ptr ptr) combase_win10.CoGetClassObject
@ stdcall CoGetContextToken(ptr) combase_win10.CoGetContextToken
@ stdcall CoGetCurrentLogicalThreadId(ptr) combase_win10.CoGetCurrentLogicalThreadId
@ stdcall CoGetCurrentProcess() combase_win10.CoGetCurrentProcess
@ stdcall CoGetDefaultContext(long ptr ptr) combase_win10.CoGetDefaultContext
@ stdcall CoGetInstanceFromFile(ptr ptr ptr long long wstr long ptr) combase_win10.CoGetInstanceFromFile
@ stdcall CoGetInstanceFromIStorage(ptr ptr ptr long ptr long ptr) combase_win10.CoGetInstanceFromIStorage
@ stdcall CoGetInterfaceAndReleaseStream(ptr ptr ptr) combase_win10.CoGetInterfaceAndReleaseStream
@ stdcall CoGetMalloc(long ptr) combase_win10.CoGetMalloc
@ stdcall CoGetMarshalSizeMax(ptr ptr ptr long ptr long) combase_win10.CoGetMarshalSizeMax
@ stdcall CoGetObject(wstr ptr ptr ptr)
@ stdcall CoGetObjectContext(ptr ptr) combase_win10.CoGetObjectContext
@ stdcall CoGetPSClsid(ptr ptr) combase_win10.CoGetPSClsid
@ stdcall CoGetStandardMarshal(ptr ptr long ptr long ptr) combase_win10.CoGetStandardMarshal
@ stdcall CoGetState(ptr)
@ stdcall -stub CoGetStdMarshalEx(ptr long ptr)
@ stub CoGetTIDFromIPID
@ stdcall CoGetTreatAsClass(ptr ptr) combase_win10.CoGetTreatAsClass
@ stdcall CoImpersonateClient() combase_win10.CoImpersonateClient
@ stdcall CoIncrementMTAUsage(ptr) combase_win10.CoIncrementMTAUsage
@ stdcall CoInitialize(ptr)
@ stdcall CoInitializeEx(ptr long) combase_win10.CoInitializeEx
@ stdcall CoInitializeSecurity(ptr long ptr ptr long long ptr long ptr) combase_win10.CoInitializeSecurity
@ stdcall CoInitializeWOW(long long) combase_win10.CoInitializeWOW
@ stdcall CoIsHandlerConnected(ptr) combase_win10.CoIsHandlerConnected
@ stdcall CoIsOle1Class (ptr)
@ stdcall CoLoadLibrary(wstr long)
@ stdcall CoLockObjectExternal(ptr long long) combase_win10.CoLockObjectExternal
@ stdcall CoMarshalHresult(ptr long) combase_win10.CoMarshalHresult
@ stdcall CoMarshalInterThreadInterfaceInStream(ptr ptr ptr) combase_win10.CoMarshalInterThreadInterfaceInStream
@ stdcall CoMarshalInterface(ptr ptr ptr long ptr long) combase_win10.CoMarshalInterface
@ stub CoQueryAuthenticationServices
@ stdcall CoQueryClientBlanket(ptr ptr ptr ptr ptr ptr ptr) combase_win10.CoQueryClientBlanket
@ stdcall CoQueryProxyBlanket(ptr ptr ptr ptr ptr ptr ptr ptr) combase_win10.CoQueryProxyBlanket
@ stub CoQueryReleaseObject
@ stdcall CoRegisterChannelHook(ptr ptr) combase_win10.CoRegisterChannelHook
@ stdcall CoRegisterClassObject(ptr ptr long long ptr) combase_win10.CoRegisterClassObject
@ stdcall CoRegisterInitializeSpy(ptr ptr) combase_win10.CoRegisterInitializeSpy
@ stdcall CoRegisterMallocSpy(ptr) combase_win10.CoRegisterMallocSpy
@ stdcall CoRegisterMessageFilter(ptr ptr) combase_win10.CoRegisterMessageFilter
@ stdcall CoRegisterPSClsid(ptr ptr) combase_win10.CoRegisterPSClsid
@ stdcall CoRegisterSurrogate(ptr) combase_win10.CoRegisterSurrogate
@ stdcall CoRegisterSurrogateEx(ptr ptr) combase_win10.CoRegisterSurrogateEx
@ stdcall CoReleaseMarshalData(ptr) combase_win10.CoReleaseMarshalData
@ stdcall CoReleaseServerProcess() combase_win10.CoReleaseServerProcess
@ stdcall CoResumeClassObjects() combase_win10.CoResumeClassObjects
@ stdcall CoRevertToSelf() combase_win10.CoRevertToSelf
@ stdcall CoRevokeClassObject(long) combase_win10.CoRevokeClassObject
@ stdcall CoRevokeInitializeSpy(int64) combase_win10.CoRevokeInitializeSpy
@ stdcall CoRevokeMallocSpy() combase_win10.CoRevokeMallocSpy
@ stdcall CoSetProxyBlanket(ptr long long ptr long long ptr long) combase_win10.CoSetProxyBlanket
@ stdcall CoSetState(ptr)
@ stdcall CoSuspendClassObjects() combase_win10.CoSuspendClassObjects
@ stdcall CoSwitchCallContext(ptr ptr) combase_win10.CoSwitchCallContext
@ stdcall CoTaskMemAlloc(long) combase_win10.CoTaskMemAlloc
@ stdcall CoTaskMemFree(ptr) combase_win10.CoTaskMemFree
@ stdcall CoTaskMemRealloc(ptr long) combase_win10.CoTaskMemRealloc
@ stdcall CoTreatAsClass(ptr ptr)
@ stdcall CoUninitialize() combase_win10.CoUninitialize
@ stub CoUnloadingWOW
@ stdcall CoUnmarshalHresult(ptr ptr) combase_win10.CoUnmarshalHresult
@ stdcall CoUnmarshalInterface(ptr ptr ptr) combase_win10.CoUnmarshalInterface
@ stdcall CoWaitForMultipleHandles(long long long ptr ptr) combase_win10.CoWaitForMultipleHandles
@ stdcall CreateAntiMoniker(ptr)
@ stdcall CreateBindCtx(long ptr)
@ stdcall CreateClassMoniker(ptr ptr)
@ stdcall CreateDataAdviseHolder(ptr)
@ stdcall CreateDataCache(ptr ptr ptr ptr)
@ stdcall CreateErrorInfo(ptr) combase_win10.CreateErrorInfo
@ stdcall CreateFileMoniker(wstr ptr)
@ stdcall CreateGenericComposite(ptr ptr ptr)
@ stdcall CreateILockBytesOnHGlobal(ptr long ptr)
@ stdcall CreateItemMoniker(wstr wstr ptr)
@ stdcall CreateObjrefMoniker(ptr ptr)
@ stdcall CreateOleAdviseHolder(ptr)
@ stdcall CreatePointerMoniker(ptr ptr)
@ stdcall CreateStreamOnHGlobal(ptr long ptr) combase_win10.CreateStreamOnHGlobal
@ stdcall DestroyRunningObjectTable()
@ stdcall DllDebugObjectRPCHook(long ptr) combase_win10.DllDebugObjectRPCHook
@ stdcall DllGetClassObject (ptr ptr ptr)
@ stub DllGetClassObjectWOW
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stdcall DoDragDrop(ptr ptr long ptr)
@ stub EnableHookObject
@ stdcall FmtIdToPropStgName(ptr wstr)
@ stdcall FreePropVariantArray(long ptr) combase_win10.FreePropVariantArray
@ stdcall GetClassFile(wstr ptr)
@ stdcall GetConvertStg(ptr)
@ stub GetDocumentBitStg
@ stdcall GetErrorInfo(long ptr) combase_win10.GetErrorInfo
@ stdcall GetHGlobalFromILockBytes(ptr ptr)
@ stdcall GetHGlobalFromStream(ptr ptr) combase_win10.GetHGlobalFromStream
@ stub GetHookInterface
@ stdcall GetRunningObjectTable(long ptr)
@ stdcall HACCEL_UserFree(ptr ptr) combase_win10.HACCEL_UserFree
@ stdcall HACCEL_UserMarshal(ptr ptr ptr) combase_win10.HACCEL_UserMarshal
@ stdcall HACCEL_UserSize(ptr long ptr) combase_win10.HACCEL_UserSize
@ stdcall HACCEL_UserUnmarshal(ptr ptr ptr) combase_win10.HACCEL_UserUnmarshal
@ stdcall HBITMAP_UserFree(ptr ptr) combase_win10.HBITMAP_UserFree
@ stdcall HBITMAP_UserMarshal(ptr ptr ptr) combase_win10.HBITMAP_UserMarshal
@ stdcall HBITMAP_UserSize(ptr long ptr) combase_win10.HBITMAP_UserSize
@ stdcall HBITMAP_UserUnmarshal(ptr ptr ptr) combase_win10.HBITMAP_UserUnmarshal
@ stdcall HBRUSH_UserFree(ptr ptr) combase_win10.HBRUSH_UserFree
@ stdcall HBRUSH_UserMarshal(ptr ptr ptr) combase_win10.HBRUSH_UserMarshal
@ stdcall HBRUSH_UserSize(ptr long ptr) combase_win10.HBRUSH_UserSize
@ stdcall HBRUSH_UserUnmarshal(ptr ptr ptr) combase_win10.HBRUSH_UserUnmarshal
@ stdcall HDC_UserFree(ptr ptr) combase_win10.HDC_UserFree
@ stdcall HDC_UserMarshal(ptr ptr ptr) combase_win10.HDC_UserMarshal
@ stdcall HDC_UserSize(ptr long ptr) combase_win10.HDC_UserSize
@ stdcall HDC_UserUnmarshal(ptr ptr ptr) combase_win10.HDC_UserUnmarshal
@ stdcall HENHMETAFILE_UserFree(ptr ptr)
@ stdcall HENHMETAFILE_UserMarshal(ptr ptr ptr)
@ stdcall HENHMETAFILE_UserSize(ptr long ptr)
@ stdcall HENHMETAFILE_UserUnmarshal(ptr ptr ptr)
@ stdcall HGLOBAL_UserFree(ptr ptr) combase_win10.HGLOBAL_UserFree
@ stdcall HGLOBAL_UserMarshal(ptr ptr ptr) combase_win10.HGLOBAL_UserMarshal
@ stdcall HGLOBAL_UserSize(ptr long ptr) combase_win10.HGLOBAL_UserSize
@ stdcall HGLOBAL_UserUnmarshal(ptr ptr ptr) combase_win10.HGLOBAL_UserUnmarshal
@ stdcall HICON_UserFree(ptr ptr) combase_win10.HICON_UserFree
@ stdcall HICON_UserMarshal(ptr ptr ptr) combase_win10.HICON_UserMarshal
@ stdcall HICON_UserSize(ptr long ptr) combase_win10.HICON_UserSize
@ stdcall HICON_UserUnmarshal(ptr ptr ptr) combase_win10.HICON_UserUnmarshal
@ stdcall HMENU_UserFree(ptr ptr) combase_win10.HMENU_UserFree
@ stdcall HMENU_UserMarshal(ptr ptr ptr) combase_win10.HMENU_UserMarshal
@ stdcall HMENU_UserSize(ptr long ptr) combase_win10.HMENU_UserSize
@ stdcall HMENU_UserUnmarshal(ptr ptr ptr) combase_win10.HMENU_UserUnmarshal
@ stdcall HMETAFILEPICT_UserFree(ptr ptr)
@ stdcall HMETAFILEPICT_UserMarshal(ptr ptr ptr)
@ stdcall HMETAFILEPICT_UserSize(ptr long ptr)
@ stdcall HMETAFILEPICT_UserUnmarshal(ptr ptr ptr)
@ stdcall HMETAFILE_UserFree(ptr ptr)
@ stdcall HMETAFILE_UserMarshal(ptr ptr ptr)
@ stdcall HMETAFILE_UserSize(ptr long ptr)
@ stdcall HMETAFILE_UserUnmarshal(ptr ptr ptr)
@ stdcall HPALETTE_UserFree(ptr ptr) combase_win10.HPALETTE_UserFree
@ stdcall HPALETTE_UserMarshal(ptr ptr ptr) combase_win10.HPALETTE_UserMarshal
@ stdcall HPALETTE_UserSize(ptr long ptr) combase_win10.HPALETTE_UserSize
@ stdcall HPALETTE_UserUnmarshal(ptr ptr ptr) combase_win10.HPALETTE_UserUnmarshal
@ stdcall HWND_UserFree(ptr ptr) combase_win10.HWND_UserFree
@ stdcall HWND_UserMarshal(ptr ptr ptr) combase_win10.HWND_UserMarshal
@ stdcall HWND_UserSize(ptr long ptr) combase_win10.HWND_UserSize
@ stdcall HWND_UserUnmarshal(ptr ptr ptr) combase_win10.HWND_UserUnmarshal
@ stdcall IIDFromString(wstr ptr) combase_win10.IIDFromString
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
@ stdcall OleConvertIStorageToOLESTREAM(ptr ptr)
@ stdcall OleConvertIStorageToOLESTREAMEx(ptr long long long long ptr ptr)
@ stdcall OleConvertOLESTREAMToIStorage(ptr ptr ptr)
@ stub OleConvertOLESTREAMToIStorageEx
@ stdcall OleCreate(ptr ptr long ptr ptr ptr ptr)
@ stdcall OleCreateDefaultHandler(ptr ptr ptr ptr)
@ stdcall OleCreateEmbeddingHelper(ptr ptr long ptr ptr ptr)
@ stub OleCreateEx
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
@ stdcall ProgIDFromCLSID(ptr ptr) combase_win10.ProgIDFromCLSID
@ stdcall PropStgNameToFmtId(wstr ptr)
@ stdcall PropSysAllocString(wstr)
@ stdcall PropSysFreeString(wstr)
@ stdcall PropVariantClear(ptr) combase_win10.PropVariantClear
@ stdcall PropVariantCopy(ptr ptr) combase_win10.PropVariantCopy
@ stdcall ReadClassStg(ptr ptr)
@ stdcall ReadClassStm(ptr ptr)
@ stdcall ReadFmtUserTypeStg(ptr ptr ptr)
@ stub ReadOleStg
@ stub ReadStringStream
@ stdcall RegisterDragDrop(long ptr)
@ stdcall ReleaseStgMedium(ptr)
@ stdcall RevokeDragDrop(long)
@ stdcall RoGetAgileReference(long ptr ptr ptr) combase_win10.RoGetAgileReference
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
@ stdcall SetErrorInfo(long ptr) combase_win10.SetErrorInfo
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
@ stdcall StringFromCLSID(ptr ptr) combase_win10.StringFromCLSID
@ stdcall StringFromGUID2(ptr ptr long) combase_win10.StringFromGUID2
@ stdcall StringFromIID(ptr ptr) combase_win10.StringFromIID
@ stub UpdateDCOMSettings
@ stub UtConvertDvtd16toDvtd32
@ stub UtConvertDvtd32toDvtd16
@ stub UtGetDvtd16Info
@ stub UtGetDvtd32Info
@ stdcall WdtpInterfacePointer_UserFree(ptr) combase_win10.WdtpInterfacePointer_UserFree
@ stdcall WdtpInterfacePointer_UserMarshal(ptr long ptr ptr ptr) combase_win10.WdtpInterfacePointer_UserMarshal
@ stdcall WdtpInterfacePointer_UserSize(ptr long long ptr ptr) combase_win10.WdtpInterfacePointer_UserSize
@ stdcall WdtpInterfacePointer_UserUnmarshal(ptr ptr ptr ptr) combase_win10.WdtpInterfacePointer_UserUnmarshal
@ stdcall WriteClassStg(ptr ptr)
@ stdcall WriteClassStm(ptr ptr)
@ stdcall WriteFmtUserTypeStg(ptr long ptr)
@ stub WriteOleStg
@ stub WriteStringStream
