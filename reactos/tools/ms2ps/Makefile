#
#  Simple preprocessor which converts a source file from ms-seh to pseh.
#
PATH_TO_TOP = ../..

TARGET = ms2ps$(EXE_POSTFIX)

all: $(TARGET)

OBJECTS = ms2ps.o

CLEAN_FILES = *.o ms2ps$(EXE_POSTFIX)

HOST_CFLAGS = -I. -Werror -Wall

ms2ps.o: ms2ps.cpp
	$(HOST_CC) $(HOST_CFLAGS) -c ms2ps.cpp -o ms2ps.o

ms2ps$(EXE_POSTFIX): $(OBJECTS)
	$(HOST_CC) $(OBJECTS) -o ms2ps$(EXE_POSTFIX) -lstdc++

ifeq ($(HOST),mingw32-linux)
clean:
	rm -f *.o
	rm -f ms2ps$(EXE_POSTFIX)
endif
ifeq ($(HOST),mingw32-windows)
clean:
	-del *.o
	-del ms2ps$(EXE_POSTFIX)
endif

.phony: clean

include $(PATH_TO_TOP)/rules.mak

# EOF
