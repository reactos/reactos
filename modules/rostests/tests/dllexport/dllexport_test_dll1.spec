
# Normal export of a cdecl function
 @ cdecl CdeclFunc0()
 @ cdecl CdeclFunc1(ptr)

# Redirected cdecl function
 @ cdecl CdeclFunc2() CdeclFunc1
 @ cdecl CdeclFunc3() dllexport_test_dll2.CdeclFunc1

# Normal export of a stdcall function
 @ stdcall StdcallFunc0()
 @ stdcall StdcallFunc1(ptr)

# Decorated export of a stdcall function
 @ stdcall -arch=i386 _StdcallFunc1@4(ptr) # Reuse function name on x86 only!
 @ stdcall _DecoratedStdcallFunc1@4(ptr)

# Redirected stdcall function
 @ stdcall StdcallFunc2(ptr) StdcallFunc1
 @ stdcall StdcallFunc3(ptr) dllexport_test_dll2.StdcallFunc1
 @ stdcall -arch=i386 StdcallFunc4(ptr) _DecoratedStdcallFunc1@4
 @ stdcall StdcallFunc5(ptr) dllexport_test_dll2._DecoratedStdcallFunc1@4
 @ stdcall -arch=i386 _DecoratedStdcallFunc2@4(ptr) StdcallFunc1
; @ stdcall -arch=i386 _DecoratedStdcallFunc3@4(ptr) dllexport_test_dll2.StdcallFunc1 # This doesn't work with MSVC!
 @ stdcall -arch=i386 _DecoratedStdcallFunc4@4(ptr) _DecoratedStdcallFunc1@4
 @ stdcall -arch=i386 _DecoratedStdcallFunc5@4(ptr) dllexport_test_dll2._DecoratedStdcallFunc1@4

# Normal export of a fastcall function
 @ fastcall FastcallFunc0()
 @ fastcall FastcallFunc1(ptr)

# Decorated export of a fastcall function
 @ fastcall -arch=i386 @DecoratedFastcallFunc1@4(ptr)

# Redirected fastcall function
 @ fastcall FastcallFunc2(ptr) FastcallFunc1
 @ fastcall FastcallFunc3(ptr) dllexport_test_dll2.FastcallFunc1
 @ fastcall -arch=i386 FastcallFunc4(ptr) @DecoratedFastcallFunc1@4
 @ fastcall -arch=i386 FastcallFunc5(ptr) dllexport_test_dll2.@DecoratedFastcallFunc1@4

 @ fastcall -arch=i386 @DecoratedFastcallFunc2@4(ptr) FastcallFunc1
; @ fastcall -arch=i386 @DecoratedFastcallFunc3@4(ptr) dllexport_test_dll2.FastcallFunc1 # This doesn't work with MSVC!
 @ fastcall -arch=i386 @DecoratedFastcallFunc4@4(ptr) @DecoratedFastcallFunc1@4
 @ fastcall -arch=i386 @DecoratedFastcallFunc5@4(ptr) dllexport_test_dll2.@DecoratedFastcallFunc1@4

# Normal export of data
 @ extern DataItem1

# Redirected data
 @ extern DataItem2 DataItem1
 @ extern DataItem3 dllexport_test_dll2.DataItem1

# other stuff
 123 stdcall @(ptr) ExportByOrdinal1
 218 stdcall -noname ExportByOrdinal1(ptr)
