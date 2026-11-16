; Thin forwarder for LoadLibrary("cryptbase.dll"). Bodies live in advapi32.
; Export list matches Wine cryptbase.dll (source.winehq.org/WineAPI/cryptbase.html).
@ stdcall SystemFunction001(ptr ptr ptr) advapi32.SystemFunction001
@ stdcall SystemFunction002(ptr ptr ptr) advapi32.SystemFunction002
@ stdcall SystemFunction003(ptr ptr) advapi32.SystemFunction003
@ stdcall SystemFunction004(ptr ptr ptr) advapi32.SystemFunction004
@ stdcall SystemFunction005(ptr ptr ptr) advapi32.SystemFunction005
@ stdcall SystemFunction028(long long) advapi32.SystemFunction028
@ stdcall SystemFunction029(long long) advapi32.SystemFunction029
@ stdcall SystemFunction034(long long) advapi32.SystemFunction034
@ stdcall SystemFunction036(ptr long) advapi32.SystemFunction036
@ stdcall SystemFunction040(ptr long long) advapi32.SystemFunction040
@ stdcall SystemFunction041(ptr long long) advapi32.SystemFunction041
