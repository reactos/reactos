#
#  ReactOS/Wine message compiler
#
PATH_TO_TOP = ../..

TARGET=wmc$(EXE_POSTFIX)

all: $(TARGET)

OBJECTS = getopt.o lang.o mcl.o utils.o wmc.o write.o y_tab.o

CLEAN_FILES = *.o wmc$(EXE_POSTFIX)

wmc$(EXE_POSTFIX): $(OBJECTS)
	$(HOST_CC) $(OBJECTS) -lkernel32 -lcrtdll -luser32 -o wmc$(EXE_POSTFIX)

ifeq ($(HOST),mingw32-linux)
clean:
	rm *.o
	rm wmc$(EXE_POSTFIX)
endif
ifeq ($(HOST),mingw32-windows)
clean:
	del *.o
	del wmc$(EXE_POSTFIX)
endif

.phony: clean

include $(PATH_TO_TOP)/rules.mak

# EOF
