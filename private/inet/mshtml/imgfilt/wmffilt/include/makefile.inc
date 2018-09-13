MIDL= midl.exe

.idl.h:
   $(MIDL) $? /h $(*B).H /tlb ..\Daytona\$(O)\$(*B).tlb /I$(INCLUDES) /iid $(*B).ic

.SUFFIXES: .idl .h
