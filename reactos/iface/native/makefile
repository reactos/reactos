#
#
#

NAPI_FILES = ../../lib/ntdll/napi.c ../../include/ntdll/napi.h

all: genntdll$(EXE_POSTFIX) $(NAPI_FILES)

$(NAPI_FILES): sysfuncs.lst genntdll$(EXE_POSTFIX) 
	genntdll$(EXE_POSTFIX) sysfuncs.lst ../../lib/ntdll/napi.c ../../include/ntdll/napi.h

genntdll$(EXE_POSTFIX): genntdll.c
	$(NATIVE_CC) -If:\gnu\mingw32\include -g genntdll.c -o genntdll$(EXE_POSTFIX)	

clean:
	- $(RM) ../../lib/ntdll/sysfuncs.lst
	- $(RM) ../../include/ntdll/napi.h
	
.PHONY: all clean 

include ../../rules.mak
