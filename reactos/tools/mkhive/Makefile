#
#  Hive-Maker
#
PATH_TO_TOP = ../..

TARGET = mkhive$(EXE_POSTFIX)

all: $(TARGET)

OBJECTS = mkhive.o binhive.o infcache.o reginf.o registry.o

CLEAN_FILES = *.o mkhive$(EXE_POSTFIX)

HOST_CFLAGS = -I.

mkhive.o: mkhive.c
	$(HOST_CC) $(HOST_CFLAGS) -c mkhive.c -o mkhive.o

binhive.o: binhive.c
	$(HOST_CC) $(HOST_CFLAGS) -c binhive.c -o binhive.o

infcache.o: infcache.c
	$(HOST_CC) $(HOST_CFLAGS) -c infcache.c -o infcache.o

reginf.o: reginf.c
	$(HOST_CC) $(HOST_CFLAGS) -c reginf.c -o reginf.o

registry.o: registry.c
	$(HOST_CC) $(HOST_CFLAGS) -c registry.c -o registry.o

mkhive$(EXE_POSTFIX): $(OBJECTS)
	$(HOST_CC) $(OBJECTS) -o mkhive$(EXE_POSTFIX)

ifeq ($(HOST),mingw32-linux)
clean:
	rm -f *.o
	rm -f mkhive$(EXE_POSTFIX)
endif
ifeq ($(HOST),mingw32-windows)
clean:
	del *.o
	del mkhive$(EXE_POSTFIX)
endif

.phony: clean

include $(PATH_TO_TOP)/rules.mak

# EOF
