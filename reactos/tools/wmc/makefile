#
#  ReactOS/Wine message compiler
#
PATH_TO_TOP = ../..

TARGET=wmc$(EXE_POSTFIX)

all: $(TARGET)

OBJECTS = getopt.o lang.o mcl.o utils.o wmc.o write.o y_tab.o

CLEAN_FILES = *.o wmc$(EXE_POSTFIX)

wmc$(EXE_POSTFIX): $(OBJECTS)
	$(HOST_CC) $(OBJECTS) -o wmc$(EXE_POSTFIX)

HOST_CFLAGS = -I.

getopt.o: getopt.c
	$(HOST_CC) $(HOST_CFLAGS) -c getopt.c -o getopt.o

lang.o: lang.c
	$(HOST_CC) $(HOST_CFLAGS) -c lang.c -o lang.o

mcl.o: mcl.c
	$(HOST_CC) $(HOST_CFLAGS) -c mcl.c -o mcl.o

utils.o: utils.c
	$(HOST_CC) $(HOST_CFLAGS) -c utils.c -o utils.o

wmc.o: wmc.c
	$(HOST_CC) $(HOST_CFLAGS) -c wmc.c -o wmc.o

write.o: write.c
	$(HOST_CC) $(HOST_CFLAGS) -c write.c -o write.o

y_tab.o: y_tab.c
	$(HOST_CC) $(HOST_CFLAGS) -c y_tab.c -o y_tab.o

ifeq ($(HOST),mingw32-linux)
clean:
	rm -f *.o
	rm -f wmc$(EXE_POSTFIX)
endif
ifeq ($(HOST),mingw32-windows)
clean:
	del *.o
	del wmc$(EXE_POSTFIX)
endif

.phony: clean

include $(PATH_TO_TOP)/rules.mak

# EOF
