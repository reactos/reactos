@ stdcall BindMoniker(ptr long ptr ptr)
@ stub CLIPFORMAT_UserFree
@ stub CLIPFORMAT_UserMarshal
@ stub CLIPFORMAT_UserSize
@ stub CLIPFORMAT_UserUnmarshal
@ stdcall CLSIDFromProgID(wstr ptr)
@ stdcall CLSIDFromString(wstr ptr)
@ stub CoAddRefServerProcess
@ stdcall CoBuildVersion()
@ stub CoCopyProxy                #@ stdcall (ptr ptr) return 0,ERR_NOTIMPLEMENTED
@ stdcall CoCreateFreeThreadedMarshaler(ptr ptr)
@ stdcall CoCreateGuid(ptr)
@ stdcall CoCreateInstance(ptr ptr long ptr ptr)
@ stdcall CoCreateInstanceEx(ptr ptr long ptr long ptr)
@ stdcall CoDisconnectObject(ptr long)
@ stdcall CoDosDateTimeToFileTime(long long ptr) kernel32.DosDateTimeToFileTime
@ stdcall CoFileTimeNow(ptr)
@ stdcall CoFileTimeToDosDateTime(ptr ptr ptr) kernel32.FileTimeToDosDateTime
@ stdcall CoFreeAllLibraries()
@ stdcall CoFreeLibrary(long)
@ stdcall CoFreeUnusedLibraries()
@ stub CoGetCallContext           #@ stdcall (ptr ptr) return 0,ERR_NOTIMPLEMENTED
@ stub CoGetCallerTID
@ stdcall CoGetClassObject(ptr long ptr ptr ptr)
@ stub CoGetCurrentLogicalThreadId
@ stdcall CoGetCurrentProcess()
@ stub CoGetInstanceFromFile      #@ stdcall (ptr ptr ptr long wstr long ptr) return 0,ERR_NOTIMPLEMENTED
@ stub CoGetInstanceFromIStorage  #@ stdcall (ptr ptr ptr long ptr long ptr) return 0,ERR_NOTIMPLEMENTED
@ stdcall CoGetInterfaceAndReleaseStream(ptr ptr ptr)
@ stdcall CoGetMalloc(long ptr)
@ stdcall CoGetMarshalSizeMax(ptr ptr ptr long ptr long)
@ stub CoGetObject
@ stdcall CoGetPSClsid(ptr ptr)
@ stdcall CoGetStandardMarshal(ptr ptr long ptr long ptr)
@ stdcall CoGetState(ptr)
@ stub CoGetTIDFromIPID
@ stdcall CoGetTreatAsClass(ptr ptr)
@ stub CoImpersonateClient
@ stdcall CoInitialize(ptr)
@ stdcall CoInitializeEx(ptr long)
@ stdcall CoInitializeSecurity(ptr long ptr ptr long long ptr long ptr)
@ stdcall CoInitializeWOW(long long)
@ stub CoIsHandlerConnected       #@ stdcall (ptr) return 0,ERR_NOTIMPLEMENTED
@ stdcall CoIsOle1Class (ptr)
@ stdcall CoLoadLibrary(wstr long)
@ stdcall CoLockObjectExternal(ptr long long)
@ stub CoMarshalHresult           #@ stdcall (ptr ptr) return 0,ERR_NOTIMPLEMENTED
@ stdcall CoMarshalInterThreadInterfaceInStream(ptr ptr ptr)
@ stdcall CoMarshalInterface(ptr ptr ptr long ptr long)
@ stub CoQueryAuthenticationServices
@ stub CoQueryClientBlanket
@ stub CoQueryProxyBlanket
@ stub CoQueryReleaseObject
@ stub CoRegisterChannelHook
@ stdcall CoRegisterClassObject(ptr ptr long long ptr)
@ stdcall CoRegisterMallocSpy (ptr)
@ stdcall CoRegisterMessageFilter(ptr ptr)
@ stub CoRegisterPSClsid          #@ stdcall (ptr ptr) return 0,ERR_NOTIMPLEMENTED
@ stub CoRegisterSurrogate
@ stdcall CoReleaseMarshalData(ptr)
@ stub CoReleaseServerProcess     #@ stdcall () return 0,ERR_NOTIMPLEMENTED
@ stdcall CoResumeClassObjects()
@ stub CoRevertToSelf             #@ stdcall () return 0,ERR_NOTIMPLEMENTED
@ stdcall CoRevokeClassObject(long)
@ stdcall CoRevokeMallocSpy()
@ stub CoSetProxyBlanket          #@ stdcall (ptr long long wstr long long ptr long) return 0,ERR_NOTIMPLEMENTED
@ stdcall CoSetState(ptr)
@ stub CoSwitchCallContext
@ stub CoSuspendClassObjects      #@ stdcall () return 0,ERR_NOTIMPLEMENTED
@ stdcall CoTaskMemAlloc(long)
@ stdcall CoTaskMemFree(ptr)
@ stdcall CoTaskMemRealloc(ptr long)
@ stdcall CoTreatAsClass(ptr ptr)
@ stdcall CoUninitialize()
@ stub CoUnloadingWOW
@ stub CoUnmarshalHresult         #@ stdcall (ptr ptr) return 0,ERR_NOTIMPLEMENTED
@ stdcall CoUnmarshalInterface(ptr ptr ptr)
@ stdcall CreateAntiMoniker(ptr)
@ stdcall CreateBindCtx(long ptr)
@ stdcall CreateClassMoniker(ptr ptr)
@ stdcall CreateDataAdviseHolder(ptr)
@ stdcall CreateDataCache(ptr ptr ptr ptr)
@ stdcall CreateErrorInfo(ptr)
@ stdcall CreateFileMoniker(wstr ptr)
@ stdcall CreateGenericComposite(ptr ptr ptr)
@ stdcall CreateILockBytesOnHGlobal(ptr long ptr)
@ stdcall CreateItemMoniker(wstr wstr ptr)
@ stub CreateObjrefMoniker
@ stdcall CreateOleAdviseHolder(ptr)
@ stub CreatePointerMoniker       #@ stdcall (ptr ptr) return 0,ERR_NOTIMPLEMENTED
@ stdcall CreateStreamOnHGlobal(ptr long ptr)
@ stdcall DllDebugObjectRPCHook(long ptr)
@ stdcall -private DllGetClassObject (ptr ptr ptr) OLE32_DllGetClassObject
@ stub DllGetClassObjectWOW
@ stdcall -private DllRegisterServer() OLE32_DllRegisterServer
@ stdcall -private DllUnregisterServer() OLE32_DllUnregisterServer
@ stdcall DoDragDrop(ptr ptr long ptr)
@ stub EnableHookObject
@ stdcall FreePropVariantArray(long ptr)
@ stdcall GetClassFile(wstr ptr)
@ stdcall GetConvertStg(ptr)
@ stub GetDocumentBitStg
@ stdcall GetErrorInfo(long ptr)
@ stdcall GetHGlobalFromILockBytes(ptr ptr)
@ stdcall GetHGlobalFromStream(ptr ptr)
@ stub GetHookInterface
@ stdcall GetRunningObjectTable(long ptr)
@ stub HACCEL_UserFree
@ stub HACCEL_UserMarshal
@ stub HACCEL_UserSize
@ stub HACCEL_UserUnmarshal
@ stub HBITMAP_UserFree
@ stub HBITMAP_UserMarshal
@ stub HBITMAP_UserSize
@ stub HBITMAP_UserUnmarshal
@ stub HBRUSH_UserFree
@ stub HBRUSH_UserMarshal
@ stub HBRUSH_UserSize
@ stub HBRUSH_UserUnmarshal
@ stub HENHMETAFILE_UserFree
@ stub HENHMETAFILE_UserMarshal
@ stub HENHMETAFILE_UserSize
@ stub HENHMETAFILE_UserUnmarshal
@ stub HGLOBAL_UserFree
@ stub HGLOBAL_UserMarshal
@ stub HGLOBAL_UserSize
@ stub HGLOBAL_UserUnmarshal
@ stub HMENU_UserFree
@ stub HMENU_UserMarshal
@ stub HMENU_UserSize
@ stub HMENU_UserUnmarshal
@ stub HMETAFILEPICT_UserFree
@ stub HMETAFILEPICT_UserMarshal
@ stub HMETAFILEPICT_UserSize
@ stub HMETAFILEPICT_UserUnmarshal
@ stub HMETAFILE_UserFree
@ stub HMETAFILE_UserMarshal
@ stub HMETAFILE_UserSize
@ stub HMETAFILE_UserUnmarshal
@ stub HPALETTE_UserFree
@ stub HPALETTE_UserMarshal
@ stub HPALETTE_UserSize
@ stub HPALETTE_UserUnmarshal
@ stub HWND_UserFree
@ stub HWND_UserMarshal
@ stub HWND_UserSize
@ stub HWND_UserUnmarshal
@ stub I_RemoteMain
@ stdcall IIDFromString(wstr ptr) CLSIDFromString
@ stdcall IsAccelerator(long long ptr long)
@ stdcall IsEqualGUID(ptr ptr)
@ stub IsValidIid
@ stdcall IsValidInterface(ptr)
@ stub IsValidPtrIn
@ stub IsValidPtrOut
@ stdcall MkParseDisplayName(ptr ptr ptr ptr)
@ stdcall MonikerCommonPrefixWith(ptr ptr ptr)
@ stub MonikerRelativePathTo
@ stdcall OleBuildVersion()
@ stdcall OleConvertIStorageToOLESTREAM(ptr ptr)
@ stub OleConvertIStorageToOLESTREAMEx
@ stdcall OleConvertOLESTREAMToIStorage(ptr ptr ptr)
@ stub OleConvertOLESTREAMToIStorageEx
@ stdcall OleCreate(ptr ptr long ptr ptr ptr ptr)
@ stdcall OleCreateDefaultHandler(ptr ptr ptr ptr)
@ stub OleCreateEmbeddingHelper
@ stub OleCreateEx
@ stdcall OleCreateFromData(ptr ptr long ptr ptr ptr ptr)
@ stub OleCreateFromDataEx
@ stdcall OleCreateFromFile(ptr ptr ptr long ptr ptr ptr ptr)
@ stub OleCreateFromFileEx
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
@ stub OleGetIconOfFile
@ stdcall OleInitialize(ptr)
@ stdcall OleInitializeWOW(long)
@ stdcall OleIsCurrentClipboard(ptr)
@ stdcall OleIsRunning(ptr)
@ stdcall OleLoad(ptr ptr ptr ptr)
@ stdcall OleLoadFromStream(ptr ptr ptr)
@ stdcall OleLockRunning(ptr long long)
@ stdcall OleMetafilePictFromIconAndLabel(long ptr ptr long)
@ stub OleNoteObjectVisible
@ stdcall OleQueryCreateFromData(ptr)
@ stdcall OleQueryLinkFromData(ptr)
@ stdcall OleRegEnumFormatEtc(ptr long ptr)
@ stdcall OleRegEnumVerbs(long ptr)
@ stdcall OleRegGetMiscStatus(ptr long ptr)
@ stdcall OleRegGetUserType(long long ptr)
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
@ stdcall ProgIDFromCLSID(ptr ptr)
@ stub PropSysAllocString
@ stub PropSysFreeString
@ stdcall PropVariantClear(ptr)
@ stdcall PropVariantCopy(ptr ptr)
@ stdcall ReadClassStg(ptr ptr)
@ stdcall ReadClassStm(ptr ptr)
@ stdcall ReadFmtUserTypeStg(ptr ptr ptr)
@ stub ReadOleStg
@ stub ReadStringStream
@ stdcall RegisterDragDrop(long ptr)
@ stdcall ReleaseStgMedium(ptr)
@ stdcall RevokeDragDrop(long)
@ stdcall SetConvertStg(ptr long)
@ stub SetDocumentBitStg
@ stdcall SetErrorInfo(long ptr)
@ stub SNB_UserFree
@ stub SNB_UserMarshal
@ stub SNB_UserSize
@ stub SNB_UserUnmarshal
@ stdcall StgCreateDocfile(wstr long long ptr)
@ stdcall StgCreateDocfileOnILockBytes(ptr long long ptr)
@ stdcall StgCreateStorageEx(wstr long long long ptr ptr ptr ptr)
@ stub StgGetIFillLockBytesOnFile
@ stub StgGetIFillLockBytesOnILockBytes
@ stdcall StgIsStorageFile(wstr)
@ stdcall StgIsStorageILockBytes(ptr)
@ stub STGMEDIUM_UserFree
@ stub STGMEDIUM_UserMarshal
@ stub STGMEDIUM_UserSize
@ stub STGMEDIUM_UserUnmarshal
@ stub StgOpenAsyncDocfileOnIFillLockBytes
@ stdcall StgOpenStorage(wstr ptr long ptr long ptr)
@ stub StgOpenStorageEx
@ stdcall StgOpenStorageOnILockBytes(ptr ptr long long long ptr)
@ stdcall StgSetTimes(wstr ptr ptr ptr )
@ stdcall StringFromCLSID(ptr ptr)
@ stdcall StringFromGUID2(ptr ptr long)
@ stdcall StringFromIID(ptr ptr) StringFromCLSID
@ stub UpdateDCOMSettings
@ stub UtConvertDvtd16toDvtd32
@ stub UtConvertDvtd32toDvtd16
@ stub UtGetDvtd16Info
@ stub UtGetDvtd32Info
@ stub WdtpInterfacePointer_UserFree
@ stub WdtpInterfacePointer_UserMarshal
@ stub WdtpInterfacePointer_UserSize
@ stub WdtpInterfacePointer_UserUnmarshal
@ stdcall WriteClassStg(ptr ptr)
@ stdcall WriteClassStm(ptr ptr)
@ stdcall WriteFmtUserTypeStg(ptr long ptr)
@ stub WriteOleStg
@ stub WriteStringStream
