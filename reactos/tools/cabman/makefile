#
#  ReactOS Cabinet Manager
#
PATH_TO_TOP = ../..

TARGET=cabman$(EXE_POSTFIX)

all: $(TARGET)

OBJECTS = cabinet.o mszip.o raw.o  main.o dfp.o

CLEAN_FILES = *.o cabman$(EXE_POSTFIX)

HOST_CFLAGS = -g -I. -I$(PATH_TO_TOP)/lib/zlib -I$(W32API_PATH)/include -Werror -Wall

HOST_LFLAGS = -g $(PATH_TO_TOP)/lib/zlib/zlib.host.a

cabinet.o: cabinet.cxx
	$(HOST_CXX) $(HOST_CFLAGS) -c cabinet.cxx -o cabinet.o

mszip.o: mszip.cxx
	$(HOST_CXX) $(HOST_CFLAGS) -c mszip.cxx -o mszip.o

raw.o: raw.cxx
	$(HOST_CXX) $(HOST_CFLAGS) -c raw.cxx -o raw.o

main.o: main.cxx
	$(HOST_CXX) $(HOST_CFLAGS) -c main.cxx -o main.o

dfp.o: dfp.cxx
	$(HOST_CXX) $(HOST_CFLAGS) -c dfp.cxx -o dfp.o

cabman$(EXE_POSTFIX): $(OBJECTS)
	$(HOST_CXX) $(OBJECTS) $(HOST_LFLAGS) -o cabman$(EXE_POSTFIX)

ifeq ($(HOST),mingw32-linux)
clean:
	rm -f *.o
	rm -f cabman$(EXE_POSTFIX)
endif
ifeq ($(HOST),mingw32-windows)
clean:
	-del *.o
	-del cabman$(EXE_POSTFIX)
endif

.phony: clean

include $(PATH_TO_TOP)/rules.mak
