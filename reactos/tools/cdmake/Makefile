#
#  CD-Maker
#
PATH_TO_TOP = ../..

TARGET=cdmake$(EXE_POSTFIX)

all: $(TARGET)

OBJECTS = cdmake.o llmosrt.o

CLEAN_FILES = *.o cdmake$(EXE_POSTFIX)

HOST_CFLAGS = -I.

cdmake.o: cdmake.c
	$(HOST_CC) $(HOST_CFLAGS) -c cdmake.c -o cdmake.o

llmosrt.o: llmosrt.c
	$(HOST_CC) $(HOST_CFLAGS) -c llmosrt.c -o llmosrt.o

cdmake$(EXE_POSTFIX): $(OBJECTS)
	$(HOST_CC) $(OBJECTS) -o cdmake$(EXE_POSTFIX)

ifeq ($(HOST),mingw32-linux)
clean:
	rm -f *.o
	rm -f cdmake$(EXE_POSTFIX)
endif
ifeq ($(HOST),mingw32-windows)
clean:
	del *.o
	del cdmake$(EXE_POSTFIX)
endif

.phony: clean

include $(PATH_TO_TOP)/rules.mak

# EOF
