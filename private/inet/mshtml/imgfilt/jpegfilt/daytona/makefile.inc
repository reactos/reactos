MIDL= $(DEVTOOLS)\midl.exe

BitmapSurfaces.h: ..\BitmapSurfaces.idl

.idl.h:
   $(MIDL) $*.idl /h $*.h /tlb $*.tlb

..\jpegfilt.rc : ..\selfreg.inf