PATH_TO_TOP = ../..

TARGET = regtests2xml$(EXE_POSTFIX)

all: $(TARGET)

OBJECTS = regtests2xml.o

CLEAN_FILES = *.o regtests2xml$(EXE_POSTFIX)

HOST_CFLAGS = -I. -Wall -Werror

regtests2xml.o: regtests2xml.c
	$(HOST_CC) -g $(HOST_CFLAGS) -c regtests2xml.c -o regtests2xml.o

regtests2xml$(EXE_POSTFIX): $(OBJECTS)
	$(HOST_CC) -g $(OBJECTS) -o $(TARGET)

ifeq ($(HOST),mingw32-linux)
clean:
	rm -f *.o $(TARGET)
endif
ifneq ($(HOST),mingw32-linux)
clean:
	del *.o $(TARGET)
endif

.PHONY: clean

include $(PATH_TO_TOP)/rules.mak
