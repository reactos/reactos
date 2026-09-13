@ stdcall AVIBuildFilter(str long long) AVIBuildFilterA
@ stdcall AVIBuildFilterA(str long long)
@ stdcall AVIBuildFilterW(wstr long long)
@ stdcall AVIClearClipboard()
@ stdcall AVIFileAddRef(ptr)
@ stdcall AVIFileCreateStream(ptr ptr ptr) AVIFileCreateStreamA
@ stdcall AVIFileCreateStreamA(ptr ptr ptr)
@ stdcall AVIFileCreateStreamW(ptr ptr ptr)
@ stdcall AVIFileEndRecord(ptr)
@ stdcall AVIFileExit()
@ stdcall AVIFileGetStream(ptr ptr long long)
@ stdcall AVIFileInfo (ptr ptr long) AVIFileInfoA # A in both Win95 and NT
@ stdcall AVIFileInfoA(ptr ptr long)
@ stdcall AVIFileInfoW(ptr ptr long)
@ stdcall AVIFileInit()
@ stdcall AVIFileOpen(ptr str long ptr) AVIFileOpenA
@ stdcall AVIFileOpenA(ptr str long ptr)
@ stdcall AVIFileOpenW(ptr wstr long ptr)
@ stdcall AVIFileReadData(ptr long ptr ptr)
@ stdcall AVIFileRelease(ptr)
@ stdcall AVIFileWriteData(ptr long ptr long)
@ stdcall AVIGetFromClipboard(ptr)
@ stdcall AVIMakeCompressedStream(ptr ptr ptr ptr)
@ stdcall AVIMakeFileFromStreams(ptr long ptr)
@ stdcall AVIMakeStreamFromClipboard(long long ptr)
@ stdcall AVIPutFileOnClipboard(ptr)
@ varargs AVISave(str ptr ptr long ptr ptr) AVISaveA
@ varargs AVISaveA(str ptr ptr long ptr ptr)
@ stdcall AVISaveOptions(long long long ptr ptr)
@ stdcall AVISaveOptionsFree(long ptr)
@ stdcall AVISaveV(str ptr ptr long ptr ptr) AVISaveVA
@ stdcall AVISaveVA(str ptr ptr long ptr ptr)
@ stdcall AVISaveVW(wstr ptr ptr long ptr ptr)
@ varargs AVISaveW(wstr ptr ptr long ptr ptr)
@ stdcall AVIStreamAddRef(ptr)
@ stdcall AVIStreamBeginStreaming(ptr long long long)
@ stdcall AVIStreamCreate(ptr long long ptr)
@ stdcall AVIStreamEndStreaming(ptr) 
@ stdcall AVIStreamFindSample(ptr long long)
@ stdcall AVIStreamGetFrame(ptr long)
@ stdcall AVIStreamGetFrameClose(ptr)
@ stdcall AVIStreamGetFrameOpen(ptr ptr)
@ stdcall AVIStreamInfo (ptr ptr long) AVIStreamInfoA
@ stdcall AVIStreamInfoA(ptr ptr long)
@ stdcall AVIStreamInfoW(ptr ptr long)
@ stdcall AVIStreamLength(ptr)
@ stdcall AVIStreamOpenFromFile (ptr str long long long ptr) AVIStreamOpenFromFileA
@ stdcall AVIStreamOpenFromFileA(ptr str long long long ptr)
@ stdcall AVIStreamOpenFromFileW(ptr wstr long long long ptr)
@ stdcall AVIStreamRead(ptr long long ptr long ptr ptr)
@ stdcall AVIStreamReadData(ptr long ptr ptr)
@ stdcall AVIStreamReadFormat(ptr long ptr ptr)
@ stdcall AVIStreamRelease(ptr)
@ stdcall AVIStreamSampleToTime(ptr long)
@ stdcall AVIStreamSetFormat(ptr long ptr long)
@ stdcall AVIStreamStart(ptr)
@ stdcall AVIStreamTimeToSample(ptr long)
@ stdcall AVIStreamWrite(ptr long long ptr long long ptr ptr)
@ stdcall AVIStreamWriteData(ptr long ptr long)
@ extern  CLSID_AVISimpleUnMarshal
@ stdcall CreateEditableStream(ptr ptr)
@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stdcall EditStreamClone(ptr ptr)
@ stdcall EditStreamCopy(ptr ptr ptr ptr)
@ stdcall EditStreamCut(ptr ptr ptr ptr)
@ stdcall EditStreamPaste(ptr ptr ptr ptr long long)
@ stdcall EditStreamSetInfo(ptr ptr long) EditStreamSetInfoA
@ stdcall EditStreamSetInfoA(ptr ptr long)
@ stdcall EditStreamSetInfoW(ptr ptr long)
@ stdcall EditStreamSetName(ptr str) EditStreamSetNameA
@ stdcall EditStreamSetNameA(ptr str)
@ stdcall EditStreamSetNameW(ptr wstr)
@ extern  IID_IAVIEditStream
@ extern  IID_IAVIFile
@ extern  IID_IAVIStream
@ extern  IID_IGetFrame
