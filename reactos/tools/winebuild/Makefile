#
# winebuild
#
PATH_TO_TOP = ../..

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

HOST_CFLAGS = -D__USE_W32API -D__REACTOS__ -I$(PATH_TO_TOP)/include -I$(PATH_TO_TOP)/include/wine -I$(W32API_PATH)/include

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

include $(PATH_TO_TOP)/rules.mak
