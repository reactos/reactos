PATH_TO_TOP = ../..

include $(PATH_TO_TOP)/config

TARGET_TYPE = host_library

TARGET_NAME = wpp

TARGET_CFLAGS = -D__USE_W32API -I$(PATH_TO_TOP)/include/wine

TARGET_OBJECTS = preproc.o wpp.o wpp.tab.o lex.yy.o

#DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

#include $(TOOLS_PATH)/depend.mk

LEXOPT    = -Cf #-w -b
YACCOPT   = #-v
#
#	Optional use of bison, this will allow independent building from
#	Wine.
#
#y.tab.c y.tab.h: ppl.l
#	bison $(YACCOPT) -ppp -d -t ppy.y -o wpp.tab.c
#
#lex.yy.c: ppl.l
#	flex $(LEXOPT) -d -8 -Ppp -olex.yy.c ppl.l

# EOF
