all: ntdll.a

OBJECTS = napi.o stubs/stubs.o string/wstring.o stdio/vsprintf.o \
          rtl/unicode.o rtl/namespc.o string/ctype.o string/strlen.o \
	  string/strncmp.o string/strcpy.o string/strncpy.o string/strcmp.o \
	  string/memcpy.o string/memset.o string/strcat.o string/stricmp.o \
	  string/strnlen.o string/strrchr.o rtl/largeint.o

ntdll.a: $(OBJECTS)
	$(AR) csr ntdll.a $(OBJECTS)
	
dummy:

include ../../rules.mak
