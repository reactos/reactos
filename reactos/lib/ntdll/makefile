all: ntdll.a

OBJECTS = napi.o stubs/stubs.o string/wstring.o stdio/vsprintf.o \
          rtl/unicode.o rtl/namespc.o string/ctype.o string/strlen.o \
          rtl/time.o string/memcpy.o

ntdll.a: $(OBJECTS)
	$(AR) vcsr ntdll.a $(OBJECTS)
	
dummy:

include ../../rules.mak
