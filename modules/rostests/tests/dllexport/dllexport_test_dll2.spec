
# Normal export of a cdecl function
 @ cdecl CdeclFunc0()
 @ cdecl CdeclFunc1(ptr)

# Normal export of a stdcall function
 @ stdcall StdcallFunc0()
 @ stdcall StdcallFunc1(ptr)

# Decorated export of a stdcall function
 @ stdcall _DecoratedStdcallFunc1@4(ptr)

# Normal export of a fastcall function
 @ fastcall FastcallFunc0()
 @ fastcall FastcallFunc1(ptr)

# Decorated export of a fastcall function
 @ fastcall -arch=i386 @DecoratedFastcallFunc1@4(ptr)

# Normal export of data
 @ extern DataItem1
 @ extern DataItem2

