#
# winebuild
#
PATH_TO_TOP = ../..

include $(PATH_TO_TOP)/rules.mak

TARGET = winebuild$(EXE_POSTFIX)

all: $(TARGET)

# relay.o spec16.o
OBJECTS = \
	import.o \
	main.o \
	parser.o \
	res16.o \
	res32.o \
	spec32.o \
	utils.o \
	mkstemps.o

CLEAN_FILES = *.o $(TARGET)

HOST_CFLAGS = -D__USE_W32API -I$(PATH_TO_TOP)/include/wine

%.o: %.c
	$(HOST_CC) $(HOST_CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(HOST_CC) $(OBJECTS) -o $(TARGET)

ifeq ($(HOST),mingw32-linux)
clean:
	rm -f $(CLEAN_FILES)
endif
ifneq ($(HOST),mingw32-linux)
clean:
	del $(CLEAN_FILES)
endif

.PHONY: clean

