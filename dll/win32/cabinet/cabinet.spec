1 stub GetDllVersion
2 stdcall -private DllGetVersion (ptr)
3 stdcall Extract(ptr str)
4 stub DeleteExtractedFiles
10 cdecl FCICreate(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
11 cdecl FCIAddFile(long ptr ptr long ptr ptr ptr long)
12 cdecl FCIFlushFolder(long ptr ptr)
13 cdecl FCIFlushCabinet(long long ptr ptr)
14 cdecl FCIDestroy(long)
20 cdecl FDICreate(ptr ptr ptr ptr ptr ptr ptr long ptr)
21 cdecl FDIIsCabinet(long long ptr)
22 cdecl FDICopy(long ptr ptr long ptr ptr ptr)
23 cdecl FDIDestroy(long)
24 cdecl FDITruncateCabinet(long ptr long)
