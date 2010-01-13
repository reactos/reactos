  1 stdcall DirectPlayCreate(ptr ptr ptr)
  2 stdcall DirectPlayEnumerateA(ptr ptr)
  3 stdcall DirectPlayEnumerateW(ptr ptr)
  4 stdcall DirectPlayLobbyCreateA(ptr ptr ptr ptr long)
  5 stdcall DirectPlayLobbyCreateW(ptr ptr ptr ptr long)
  6 extern gdwDPlaySPRefCount
  9 stdcall DirectPlayEnumerate(ptr ptr) DirectPlayEnumerateA

@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
