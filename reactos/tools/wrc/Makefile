#
# wrc
#
PATH_TO_TOP = ../..

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

WPP_OBJECTS = \
	wpp/preproc.o \
	wpp/wpp.o \
	wpp/wpp.tab.o \
	wpp/lex.yy.o

CODEPAGES = \
	037 \
	424 \
	437 \
	500 \
	737 \
	775 \
	850 \
	852 \
	855 \
	856 \
	857 \
	860 \
	861 \
	862 \
	863 \
	864 \
	865 \
	866 \
	869 \
	874 \
	875 \
	878 \
	932 \
	936 \
	949 \
	950 \
	1006 \
	1026 \
	1250 \
	1251 \
	1252 \
	1253 \
	1254 \
	1255 \
	1256 \
	1257 \
	1258 \
	10000 \
	10006 \
	10007 \
	10029 \
	10079 \
	10081 \
	20866 \
	20932 \
	21866 \
	28591 \
	28592 \
	28593 \
	28594 \
	28595 \
	28596 \
	28597 \
	28598 \
	28599 \
	28600 \
	28603 \
	28604 \
	28605 \
	28606

UNICODE_OBJECTS = \
	unicode/casemap.o \
	unicode/compose.o \
	unicode/cptable.o \
	unicode/mbtowc.o \
	unicode/string.o \
	unicode/wctomb.o \
	unicode/wctype.o \
	$(CODEPAGES:%=unicode/c_%.o)

PORT_OBJECTS = \
	port/mkstemps.o

OBJECTS = $(BASE_OBJECTS) $(WPP_OBJECTS) $(UNICODE_OBJECTS) $(PORT_OBJECTS)

CLEAN_FILES = *.o $(TARGET)

HOST_CFLAGS = -D__USE_W32API -DWINVER=0x501 -DWINE_UNICODE_API= \
              -Dwchar_t="unsigned short" -D_WCHAR_T_DEFINED \
              -Iunicode -Iwpp -I$(PATH_TO_TOP)/include/wine \
              -I$(PATH_TO_TOP)/include -I$(PATH_TO_TOP)/w32api/include

%.o: %.c
	$(HOST_CC) $(HOST_CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(HOST_CC) $(OBJECTS) -o $(TARGET)

ifeq ($(HOST),mingw32-linux)
clean:
	-rm -f $(OBJECTS) $(TARGET)
endif
ifneq ($(HOST),mingw32-linux)
clean:
	-del $(TARGET)
	-del *.o
	-del wpp\*.o
	-del unicode\*.o
	-del port\*.o
endif

.PHONY: clean

LEXOPT    = -Cf #-w -b
YACCOPT   = #-v
#
#	Optional use of bison, this will allow independent building from
#	Wine.
#
#	Bison is requiered for building msi.dll. If MingW32 for windows,
#	download bison from http://gnuwin32.sourceforge.net/
#	Make sure bison.exe is placed in your command path for execution.
#
#
#
#y.tab.c y.tab.h: parser.y
#	bison $(YACCOPT) -d -t parser.y -o y.tab.c
#
#lex.yy.c: parser.l
#	flex $(LEXOPT) -d -8 parser.l
#
#wpp/y.tab.c wpp/y.tab.h: wpp/ppl.l
#	bison $(YACCOPT) -ppp -d -t wpp/ppy.y -o wpp/wpp.tab.c
#
#wpp/lex.yy.c: wpp/ppl.l
#	flex $(LEXOPT) -d -8 -Ppp -owpp/lex.yy.c wpp/ppl.l
