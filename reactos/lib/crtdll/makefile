all: crtdll.a

STRING_OBJECTS = string/memchr.o string/memcmp.o string/strcat.o \
                 string/strchr.o string/strcmp.o string/strcoll.o \
		 string/strcpy.o string/strcspn.o string/memcpy.o \
		 string/strlen.o string/strncat.o string/strncmp.o \
		 string/strncpy.o string/strpbrk.o string/strrchr.o \
		 string/strspn.o string/strstr.o string/strtok.o \
		 string/strxfrm.o string/memmove.o string/memset.o \
		 string/strdup.o


STDIO_OBJECTS = stdio/vsprintf.o

STDLIB_OBJECTS = stdlib/malloc.o

OBJECTS = $(STRING_OBJECTS) $(STDIO_OBJECTS) $(STDLIB_OBJECTS)

crtdll.a: $(OBJECTS)
	$(AR) vrcs crtdll.a $(OBJECTS)

dummy:

include ../../rules.mak
