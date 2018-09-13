MIDL= $(DEVTOOLS)\midl.exe

BitmapSurfaces.h: ..\BitmapSurfaces.idl

.idl.h:
   $(MIDL) $*.idl /h $*.h /tlb $*.tlb

..\imgutil.rc : ..\selfreg.inf
