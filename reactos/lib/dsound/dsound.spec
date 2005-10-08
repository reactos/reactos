1 stdcall DirectSoundCreate(ptr ptr ptr)
2 stdcall DirectSoundEnumerateA(ptr ptr)
3 stdcall DirectSoundEnumerateW(ptr ptr)
4 stdcall -private DllCanUnloadNow() DSOUND_DllCanUnloadNow
5 stdcall -private DllGetClassObject(ptr ptr ptr) DSOUND_DllGetClassObject
6 stdcall DirectSoundCaptureCreate(ptr ptr ptr) DirectSoundCaptureCreate8
7 stdcall DirectSoundCaptureEnumerateA(ptr ptr)
8 stdcall DirectSoundCaptureEnumerateW(ptr ptr)
9 stdcall GetDeviceID(ptr ptr)
10 stdcall DirectSoundFullDuplexCreate(ptr ptr ptr ptr long long ptr ptr ptr ptr)
11 stdcall DirectSoundCreate8(ptr ptr ptr)
12 stdcall DirectSoundCaptureCreate8(ptr ptr ptr)
@ stdcall -private DllRegisterServer() DSOUND_DllRegisterServer
@ stdcall -private DllUnregisterServer() DSOUND_DllUnregisterServer
