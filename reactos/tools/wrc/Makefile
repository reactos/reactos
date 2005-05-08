#
# wrc
#
PATH_TO_TOP = ../..

LIB_WPP_DIR = ../wpp
LIB_UNICODE_DIR = ../unicode

include $(PATH_TO_TOP)/rules.mak

TARGET = wrc$(EXE_POSTFIX)

all: $(TARGET)

BASE_OBJECTS = \
	dumpres.o \
	genres.o \
	newstruc.o \
	readres.o \
	translation.o \
	utils.o \
	wrc.o \
	writeres.o \
	y.tab.o \
	lex.yy.o

PORT_OBJECTS = \
	port/mkstemps.o

OBJECTS = $(BASE_OBJECTS) $(PORT_OBJECTS)

LIBS = $(LIB_UNICODE_DIR)/libunicode.a $(LIB_WPP_DIR)/libwpp.a

CLEAN_FILES = *.o $(TARGET)

HOST_CFLAGS = $(HOSTARCH) -D__USE_W32API -DWINE_UNICODE_API= \
              -Dwchar_t="unsigned short" -D_WCHAR_T_DEFINED \
              -I$(LIB_UNICODE_DIR) -I$(LIB_WPP_DIR) -I$(PATH_TO_TOP)/include/wine \
              -I$(PATH_TO_TOP)/include -I$(PATH_TO_TOP)/w32api/include

%.o: %.c
	$(HOST_CC) $(HOST_CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS) $(LIBS)
	$(HOST_CC) $(HOSTARCH) -o $(TARGET) $(OBJECTS) $(LIBS)

ifeq ($(HOST_TYPE),unix)
clean:
	-rm -f $(OBJECTS) $(TARGET)
else
clean:
	-del $(TARGET)
	-del *.o
	-del port\*.o
endif

.PHONY: clean

LEXOPT    = -Cf #-w -b
YACCOPT   = #-v
#
#	Optional use of bison, this will allow independent building from
#	Wine.
#
#y.tab.c y.tab.h: parser.y
#	bison $(YACCOPT) -d -t parser.y -o y.tab.c
#
#lex.yy.c: parser.l
#	flex $(LEXOPT) -d -8 parser.l

# EOF
