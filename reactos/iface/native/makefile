all: genntdll$(EXE_POSTFIX) ../../lib/ntdll/napi.asm

../../lib/ntdll/napi.asm: sysfuncs.lst genntdll$(EXE_POSTFIX) ../../include/ntdll/napi.h
	genntdll$(EXE_POSTFIX) sysfuncs.lst ../../lib/ntdll/napi.c ../../include/ntdll/napi.h

../../include/ntdll/napi.h: dummy

genntdll$(EXE_POSTFIX): genntdll.c
	$(NATIVE_CC) -If:\gnu\mingw32\include -g genntdll.c -o genntdll$(EXE_POSTFIX)	

sysfuncs.lst: dummy

clean: dummy
	$(RM) ../../lib/ntdll/sysfuncs.lst
	$(RM) ../../include/ntdll/napi.h
	
dummy:

include ../../rules.mak
