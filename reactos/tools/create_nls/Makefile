#
#  ReactOS/NLS conversion tool
#
PATH_TO_TOP = ../..

TARGET=create_nls$(EXE_POSTFIX)

all: $(TARGET)

OBJECTS = create_nls.o

CLEAN_FILES = *.o create_nls$(EXE_POSTFIX)

create_nls$(EXE_POSTFIX): $(OBJECTS)
	$(HOST_CC) $(OBJECTS) -o create_nls$(EXE_POSTFIX)

HOST_CFLAGS = -I.

create_nls.o: create_nls.c
	$(HOST_CC) $(HOST_CFLAGS) -c create_nls.c -o create_nls.o

ifeq ($(HOST),mingw32-linux)
clean:
	rm -f *.o
	rm -f create_nls$(EXE_POSTFIX)
endif
ifeq ($(HOST),mingw32-windows)
clean:
	del *.o
	del create_nls$(EXE_POSTFIX)
endif

.phony: clean

nls:
	create_nls$(EXE_POSTFIX)

.phony: nls

ifeq ($(HOST),mingw32-linux)
clean_nls:
	rm -f ../../media/nls/*.nls
endif
ifeq ($(HOST),mingw32-windows)
clean_nls:
	del ..\..\media\nls\*.nls
endif

.phony: clean_nls


include $(PATH_TO_TOP)/rules.mak

# EOF
