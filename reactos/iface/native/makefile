#
#
#

NAPI_FILES = ../../lib/ntdll/napi.c ../../include/ntdll/napi.h

all: genntdll$(EXE_POSTFIX) $(NAPI_FILES)

$(NAPI_FILES): sysfuncs.lst genntdll$(EXE_POSTFIX) 
	genntdll$(EXE_POSTFIX) sysfuncs.lst ../../lib/ntdll/napi.c ../../include/ntdll/napi.h

# (rjj) i removed the following option from line below: -If:\gnu\mingw32\include 
genntdll$(EXE_POSTFIX): genntdll.c
	$(NATIVE_CC) -g genntdll.c -o genntdll$(EXE_POSTFIX)	

clean:
	- $(RM) genntdll$(EXE_POSTFIX)
ifeq ($(DOSCLI),yes)
	- $(RM) ..\..\lib\ntdll\napi.c
	- $(RM) ..\..\include\ntdll\napi.h
else
	- $(RM) ../../lib/ntdll/napi.c
	- $(RM) ../../include/ntdll/napi.h
endif
	
.PHONY: all clean 

include ../../rules.mak
