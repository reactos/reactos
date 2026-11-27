@ stdcall VidBitBlt(ptr long long)

;; Always export this -version=0x602+ routine for easier cross-platform testing.
@ stdcall VidBitBltEx(long long long long ptr)

@ stdcall VidBufferToScreenBlt(ptr long long long long long)
@ stdcall VidCleanUp()
@ stdcall VidDisplayString(str)
@ stdcall VidDisplayStringXY(str long long long)

;; The VidInitialize() export signature depends on the NT version.
;; VidInitialize() initially uses a version-agnostic trampoline.
@ stdcall -norelay -version=0x500-0x502 VidInitialize(long) #VidInitialize_NT5
@ stdcall -norelay -version=0x600+ VidInitialize(long long) #VidInitialize_NT6

@ stdcall VidResetDisplay(long)
@ stdcall VidScreenToBufferBlt(ptr long long long long long)
@ stdcall VidSetScrollRegion(long long long long)
@ stdcall VidSetTextColor(long)
@ stdcall VidSolidColorFill(long long long long long)
