PATH_TO_TOP = ../..

TARGET = wine2ros$(EXE_POSTFIX)

all: $(TARGET)

OBJECTS = wine2ros.o

CLEAN_FILES = *.o wine2ros$(EXE_POSTFIX)

HOST_CFLAGS = -I. -Wall -Werror

wine2ros.o: wine2ros.c
	$(HOST_CC) -g $(HOST_CFLAGS) -c wine2ros.c -o wine2ros.o

wine2ros$(EXE_POSTFIX): $(OBJECTS)
	$(HOST_CC) -g $(OBJECTS) -o $(TARGET)

ifeq ($(HOST_TYPE),unix)
clean:
	rm -f *.o $(TARGET)
else
clean:
	del *.o $(TARGET)
endif

.PHONY: clean

include $(PATH_TO_TOP)/rules.mak
