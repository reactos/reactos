#
# widl
#

PATH_TO_TOP = ../..

LIB_WPP_DIR = ../wpp

include $(PATH_TO_TOP)/rules.mak

TARGET = widl$(EXE_POSTFIX)

all: $(TARGET)

BASE_OBJECTS = \
	client.o \
	hash.o \
	header.o \
	lex.yy.o \
	proxy.o \
	server.o \
	typelib.o \
	utils.o \
	widl.o \
	write_msft.o \
	y.tab.o

PORT_OBJECTS = \
	port/mkstemps.o

OBJECTS = $(BASE_OBJECTS) $(PORT_OBJECTS)

LIBS = $(LIB_WPP_DIR)/libwpp.a

CLEAN_FILES = *.o $(TARGET)

HOST_CFLAGS = -DYYDEBUG=1 -DINT16=SHORT -D__USE_W32API \
              -I$(LIB_WPP_DIR) -I$(PATH_TO_TOP)/include/wine \
              -I$(PATH_TO_TOP)/include -I$(PATH_TO_TOP)/w32api/include

%.o: %.c
	$(HOST_CC) $(HOST_CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS) $(LIBS)
	$(HOST_CC) -o $(TARGET) $(OBJECTS) $(LIBS)

ifeq ($(HOST),mingw32-linux)
clean:
	-rm -f $(OBJECTS) $(TARGET)
endif
ifneq ($(HOST),mingw32-linux)
clean:
	-del $(TARGET)
	-del *.o
	-del port\*.o
endif

.PHONY: clean

LEXOPT    = -Cf #-w -b
YACCOPT   = #-v
#
#	Optional use of flex and bison, this will allow independent building from
#	Wine.
#
y.tab.c y.tab.h: parser.y
	bison $(YACCOPT) -d -o y.tab.c parser.y
#
lex.yy.c: parser.l y.tab.h
	flex $(LEXOPT) -d -8 -olex.yy.c parser.l

# EOF
