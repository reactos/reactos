; Comments be here()

# 2 stdcall CommentedOutFunction(ptr long ptr ptr) # comment 2
6 fastcall FastcallFunction(ptr ptr ptr long)
@ stdcall StdcallFunction(long)
8 fastcall -ret64 Ret64Function(double)

7 fastcall -arch=i386 Fastcalli386(long)
10 stdcall -arch=i386,x86_64 Stdcalli386x64()

@ stdcall -version=0x351-0x502 StdcallVersionRange(long long wstr wstr)
@ stdcall -stub -version=0x600+ StdcallStubVersion600(ptr)

@ stdcall StdcallForwarderToSameDll() StdcallTargetInSameDll
@ stdcall -version=0x600+ StdcallForwarder(ptr ptr ptr) ntdll.RtlStdcallForwarder

@ stub StubFunction
@ stdcall -stub StdcallSuccessStub(ptr)

