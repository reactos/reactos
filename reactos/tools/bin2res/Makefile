PATH_TO_TOP = ../..

TARGET = bin2res$(EXE_POSTFIX)

all: $(TARGET)

OBJECTS = bin2res.o mkstemps.o

CLEAN_FILES = *.o bin2res$(EXE_POSTFIX)

HOST_CFLAGS = -I$(PATH_TO_TOP)/include/wine -D__REACTOS__ -Wall -Werror

bin2res.o: bin2res.c
	$(HOST_CC) -g $(HOST_CFLAGS) -c bin2res.c -o bin2res.o

mkstemps.o: mkstemps.c
	$(HOST_CC) -g $(HOST_CFLAGS) -c mkstemps.c -o mkstemps.o

bin2res$(EXE_POSTFIX): $(OBJECTS)
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
