all: ntdll.a

OBJECTS = napi.o stubs/stubs.o string/wstring.o stdio/vsprintf.o

ntdll.a: $(OBJECTS)
	$(AR) vcsr ntdll.a $(OBJECTS)
	
dummy:

include ../../rules.mak
