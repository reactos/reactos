ZLIB_BASE = $(LIB_BASE_)3rdparty$(SEP)zlib
ZLIB_BASE_ = $(ZLIB_BASE)$(SEP)
ZLIB_INT = $(INTERMEDIATE_)$(ZLIB_BASE)
ZLIB_INT_ = $(INTERMEDIATE_)$(ZLIB_BASE)$(SEP)
ZLIB_OUT = $(OUTPUT_)$(ZLIB_BASE)
ZLIB_OUT_ = $(OUTPUT_)$(ZLIB_BASE)$(SEP)

ifneq ($(INTERMEDIATE),$(OUTPUT))
$(ZLIB_OUT): | $(OUTPUT_)$(LIB_BASE)
	$(ECHO_MKDIR)
	${mkdir} $@
endif

ZLIB_HOST_TARGET = \
	$(ZLIB_OUT)$(SEP)zlib.host.a

ZLIB_HOST_SOURCES = $(addprefix $(ZLIB_BASE_), \
	adler32.c \
	compress.c \
	crc32.c \
	gzio.c \
	uncompr.c \
	deflate.c \
	trees.c \
	zutil.c \
	inflate.c \
	infback.c \
	inftrees.c \
	inffast.c \
	)

ZLIB_HOST_OBJECTS = \
	$(addprefix $(INTERMEDIATE_), $(ZLIB_HOST_SOURCES:.c=.host.o))

ZLIB_HOST_CFLAGS = -MMD -O3 -Wall -Wwrite-strings -Wpointer-arith -Wconversion \
  -Wstrict-prototypes -Wmissing-prototypes

$(ZLIB_HOST_TARGET): $(ZLIB_HOST_OBJECTS) | $(ZLIB_OUT)
	$(ECHO_AR)
	$(host_ar) -r $@ $(ZLIB_HOST_OBJECTS)

$(ZLIB_INT_)adler32.host.o: $(ZLIB_BASE_)adler32.c | $(ZLIB_INT)
	$(ECHO_CC)
	${host_gcc} $(ZLIB_HOST_CFLAGS) -c $< -o $@

$(ZLIB_INT_)compress.host.o: $(ZLIB_BASE_)compress.c | $(ZLIB_INT)
	$(ECHO_CC)
	${host_gcc} $(ZLIB_HOST_CFLAGS) -c $< -o $@

$(ZLIB_INT_)crc32.host.o: $(ZLIB_BASE_)crc32.c | $(ZLIB_INT)
	$(ECHO_CC)
	${host_gcc} $(ZLIB_HOST_CFLAGS) -c $< -o $@

$(ZLIB_INT_)gzio.host.o: $(ZLIB_BASE_)gzio.c | $(ZLIB_INT)
	$(ECHO_CC)
	${host_gcc} $(ZLIB_HOST_CFLAGS) -c $< -o $@

$(ZLIB_INT_)uncompr.host.o: $(ZLIB_BASE_)uncompr.c | $(ZLIB_INT)
	$(ECHO_CC)
	${host_gcc} $(ZLIB_HOST_CFLAGS) -c $< -o $@

$(ZLIB_INT_)deflate.host.o: $(ZLIB_BASE_)deflate.c | $(ZLIB_INT)
	$(ECHO_CC)
	${host_gcc} $(ZLIB_HOST_CFLAGS) -c $< -o $@

$(ZLIB_INT_)trees.host.o: $(ZLIB_BASE_)trees.c | $(ZLIB_INT)
	$(ECHO_CC)
	${host_gcc} $(ZLIB_HOST_CFLAGS) -c $< -o $@

$(ZLIB_INT_)zutil.host.o: $(ZLIB_BASE_)zutil.c | $(ZLIB_INT)
	$(ECHO_CC)
	${host_gcc} $(ZLIB_HOST_CFLAGS) -c $< -o $@

$(ZLIB_INT_)inflate.host.o: $(ZLIB_BASE_)inflate.c | $(ZLIB_INT)
	$(ECHO_CC)
	${host_gcc} $(ZLIB_HOST_CFLAGS) -c $< -o $@

$(ZLIB_INT_)infback.host.o: $(ZLIB_BASE_)infback.c | $(ZLIB_INT)
	$(ECHO_CC)
	${host_gcc} $(ZLIB_HOST_CFLAGS) -c $< -o $@

$(ZLIB_INT_)inftrees.host.o: $(ZLIB_BASE_)inftrees.c | $(ZLIB_INT)
	$(ECHO_CC)
	${host_gcc} $(ZLIB_HOST_CFLAGS) -c $< -o $@

$(ZLIB_INT_)inffast.host.o: $(ZLIB_BASE_)inffast.c | $(ZLIB_INT)
	$(ECHO_CC)
	${host_gcc} $(ZLIB_HOST_CFLAGS) -c $< -o $@

.PHONY: zlib_host
zlib_host: $(ZLIB_HOST_TARGET)

.PHONY: zlib_host_clean
zlib_host_clean:
	-@$(rm) $(ZLIB_HOST_TARGET) $(ZLIB_HOST_OBJECTS) 2>$(NUL)
clean: zlib_host_clean
