UNICODE_BASE = tools$(SEP)unicode

UNICODE_TARGET = \
	$(ROS_INTERMEDIATE)$(UNICODE_BASE)$(SEP)libunicode.a

UNICODE_CODEPAGES = \
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

UNICODE_SOURCES = \
	$(UNICODE_BASE)$(SEP)casemap.c \
	$(UNICODE_BASE)$(SEP)compose.c \
	$(UNICODE_BASE)$(SEP)cptable.c \
	$(UNICODE_BASE)$(SEP)mbtowc.c \
	$(UNICODE_BASE)$(SEP)string.c \
	$(UNICODE_BASE)$(SEP)wctomb.c \
	$(UNICODE_BASE)$(SEP)wctype.c \
  $(addprefix $(UNICODE_BASE)$(SEP), $(UNICODE_CODEPAGES:%=c_%.o))

UNICODE_OBJECTS = \
	$(UNICODE_SOURCES:.c=.o)

UNICODE_HOST_CFLAGS = \
	-D__USE_W32API -DWINVER=0x501 -DWINE_UNICODE_API= \
	-Dwchar_t="unsigned short" -D_WCHAR_T_DEFINED \
	-I$(UNICODE_BASE) -Iinclude/wine -Iw32api/include

$(UNICODE_TARGET): $(UNICODE_OBJECTS)
	$(HALFVERBOSEECHO) [AR]      $@
	${host_ar} -rc $(UNICODE_TARGET) $(UNICODE_OBJECTS)

$(UNICODE_OBJECTS): %.o : %.c
	$(HALFVERBOSEECHO) [CC]      $<
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

.PHONY: unicode_clean
unicode_clean:
	-@$(rm) $(UNICODE_TARGET) $(UNICODE_OBJECTS) 2>$(NUL)
clean: unicode_clean
