@ stdcall VidBitBlt(ptr long long)
@ stdcall VidBufferToScreenBlt(ptr long long long long long)
@ stdcall VidCleanUp()
@ stdcall VidDisplayString(str)
@ stdcall VidDisplayStringXY(str long long long)

;; The several VidInitialize() versions depending on the NT kernel version
;@ stdcall -version=0x500-0x502 VidInitialize(long) VidInitialize_NT5
;@ stdcall -version=0x600  VidInitialize(long long) VidInitialize_NT60
;@ stdcall -version=0x601+ VidInitialize(long long ptr) VidInitialize_NT61
;; Export a version-agnostic VidInitialize() trampoline
@ cdecl VidInitialize() VidInitialize_Stub

@ stdcall VidResetDisplay(long)
@ stdcall VidScreenToBufferBlt(ptr long long long long long)
@ stdcall VidSetScrollRegion(long long long long)
@ stdcall VidSetTextColor(long)
@ stdcall VidSolidColorFill(long long long long long)
