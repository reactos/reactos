PATH_TO_TOP = ../..

include $(PATH_TO_TOP)/config

TARGET_TYPE = host_library

TARGET_NAME = unicode

TARGET_CFLAGS = -D__USE_W32API -DWINVER=0x501 -DWINE_UNICODE_API= \
                -Dwchar_t="unsigned short" -D_WCHAR_T_DEFINED \
                -I$(PATH_TO_TOP)/include/wine -Wall -Werror

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

TARGET_OBJECTS = \
	casemap.o \
	compose.o \
	cptable.o \
	mbtowc.o \
	string.o \
	wctomb.o \
	wctype.o \
	$(CODEPAGES:%=c_%.o)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
