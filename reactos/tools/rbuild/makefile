PATH_TO_TOP = ../..

TARGET = rbuild$(EXE_POSTFIX)

all: $(TARGET)

OBJECTS = rbuild.o

CLEAN_FILES = *.o rbuild$(EXE_POSTFIX)

HOST_CFLAGS = -g -I. -Werror -Wall

HOST_LFLAGS = -g

rbuild$(EXE_POSTFIX): $(OBJECTS)
	$(HOST_CXX) $(OBJECTS) $(HOST_LFLAGS) -o rbuild$(EXE_POSTFIX)

ifeq ($(HOST),mingw32-linux)
clean:
	-rm -f *.o
	-rm -f rbuild$(EXE_POSTFIX)
endif
ifeq ($(HOST),mingw32-windows)
clean:
	-del *.o
	-del rbuild$(EXE_POSTFIX)
endif

.phony: clean

%.o: %.cpp
	$(HALFVERBOSEECHO) [CXX]     $<
	$(HOST_CXX) $(HOST_CFLAGS) -c $< -o $@

include $(PATH_TO_TOP)/rules.mak
