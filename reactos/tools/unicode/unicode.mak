UNICODE_BASE = $(TOOLS_BASE_)unicode
UNICODE_BASE_ = $(UNICODE_BASE)$(SEP)
UNICODE_INT = $(INTERMEDIATE_)$(UNICODE_BASE)
UNICODE_INT_ = $(UNICODE_INT)$(SEP)
UNICODE_OUT = $(OUTPUT_)$(UNICODE_BASE)
UNICODE_OUT_ = $(UNICODE_OUT)$(SEP)

$(UNICODE_INT): | $(TOOLS_INT)
	$(ECHO_MKDIR)
	${mkdir} $@

ifneq ($(INTERMEDIATE),$(OUTPUT))
$(UNICODE_OUT): | $(TOOLS_OUT)
	$(ECHO_MKDIR)
	${mkdir} $@
endif

UNICODE_TARGET = \
	$(UNICODE_OUT_)libunicode.a

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

UNICODE_SOURCES = $(addprefix $(UNICODE_BASE_), \
	casemap.c \
	compose.c \
	cptable.c \
	mbtowc.c \
	string.c \
	wctomb.c \
	wctype.c \
	$(UNICODE_CODEPAGES:%=c_%.o) \
	)

UNICODE_OBJECTS = \
  $(addprefix $(INTERMEDIATE_), $(UNICODE_SOURCES:.c=.o))

UNICODE_HOST_CFLAGS = \
	-D__USE_W32API -DWINVER=0x501 -DWINE_UNICODE_API= \
	-I$(UNICODE_BASE) -Iinclude/reactos/wine -Iinclude -Iinclude/reactos \
	$(TOOLS_CFLAGS)

.PHONY: unicode
unicode: $(UNICODE_TARGET)

$(UNICODE_TARGET): $(UNICODE_OBJECTS) | $(UNICODE_OUT)
	$(ECHO_AR)
	${host_ar} -rc $@ $(UNICODE_OBJECTS)

$(UNICODE_INT_)casemap.o: $(UNICODE_BASE_)casemap.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)compose.o: $(UNICODE_BASE_)compose.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)cptable.o: $(UNICODE_BASE_)cptable.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)mbtowc.o: $(UNICODE_BASE_)mbtowc.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)string.o: $(UNICODE_BASE_)string.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)wctomb.o: $(UNICODE_BASE_)wctomb.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)wctype.o: $(UNICODE_BASE_)wctype.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_037.o: $(UNICODE_BASE_)c_037.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_424.o: $(UNICODE_BASE_)c_424.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_437.o: $(UNICODE_BASE_)c_437.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_500.o: $(UNICODE_BASE_)c_500.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_737.o: $(UNICODE_BASE_)c_737.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_775.o: $(UNICODE_BASE_)c_775.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_850.o: $(UNICODE_BASE_)c_850.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_852.o: $(UNICODE_BASE_)c_852.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_855.o: $(UNICODE_BASE_)c_855.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_856.o: $(UNICODE_BASE_)c_856.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_857.o: $(UNICODE_BASE_)c_857.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_860.o: $(UNICODE_BASE_)c_860.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_861.o: $(UNICODE_BASE_)c_861.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_862.o: $(UNICODE_BASE_)c_862.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_863.o: $(UNICODE_BASE_)c_863.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_864.o: $(UNICODE_BASE_)c_864.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_865.o: $(UNICODE_BASE_)c_865.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_866.o: $(UNICODE_BASE_)c_866.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_869.o: $(UNICODE_BASE_)c_869.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_874.o: $(UNICODE_BASE_)c_874.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_875.o: $(UNICODE_BASE_)c_875.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_878.o: $(UNICODE_BASE_)c_878.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_932.o: $(UNICODE_BASE_)c_932.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_936.o: $(UNICODE_BASE_)c_936.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_949.o: $(UNICODE_BASE_)c_949.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_950.o: $(UNICODE_BASE_)c_950.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_1006.o: $(UNICODE_BASE_)c_1006.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_1026.o: $(UNICODE_BASE_)c_1026.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_1250.o: $(UNICODE_BASE_)c_1250.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_1251.o: $(UNICODE_BASE_)c_1251.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_1252.o: $(UNICODE_BASE_)c_1252.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_1253.o: $(UNICODE_BASE_)c_1253.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_1254.o: $(UNICODE_BASE_)c_1254.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_1255.o: $(UNICODE_BASE_)c_1255.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_1256.o: $(UNICODE_BASE_)c_1256.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_1257.o: $(UNICODE_BASE_)c_1257.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_1258.o: $(UNICODE_BASE_)c_1258.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_10000.o: $(UNICODE_BASE_)c_10000.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_10006.o: $(UNICODE_BASE_)c_10006.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_10007.o: $(UNICODE_BASE_)c_10007.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_10029.o: $(UNICODE_BASE_)c_10029.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_10079.o: $(UNICODE_BASE_)c_10079.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_10081.o: $(UNICODE_BASE_)c_10081.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_20866.o: $(UNICODE_BASE_)c_20866.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_20932.o: $(UNICODE_BASE_)c_20932.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_21866.o: $(UNICODE_BASE_)c_21866.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_28591.o: $(UNICODE_BASE_)c_28591.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_28592.o: $(UNICODE_BASE_)c_28592.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_28593.o: $(UNICODE_BASE_)c_28593.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_28594.o: $(UNICODE_BASE_)c_28594.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_28595.o: $(UNICODE_BASE_)c_28595.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_28596.o: $(UNICODE_BASE_)c_28596.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_28597.o: $(UNICODE_BASE_)c_28597.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_28598.o: $(UNICODE_BASE_)c_28598.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_28599.o: $(UNICODE_BASE_)c_28599.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_28600.o: $(UNICODE_BASE_)c_28600.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_28603.o: $(UNICODE_BASE_)c_28603.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_28604.o: $(UNICODE_BASE_)c_28604.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_28605.o: $(UNICODE_BASE_)c_28605.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

$(UNICODE_INT_)c_28606.o: $(UNICODE_BASE_)c_28606.c | $(UNICODE_INT)
	$(ECHO_CC)
	${host_gcc} $(UNICODE_HOST_CFLAGS) -c $< -o $@

.PHONY: unicode_clean
unicode_clean:
	-@$(rm) $(UNICODE_TARGET) $(UNICODE_OBJECTS) 2>$(NUL)
clean: unicode_clean
