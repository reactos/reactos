ZLIB_BASE = $(LIB_BASE_)zlib
ZLIB_BASE_ = $(ZLIB_BASE)$(SEP)

ZLIB_INT = $(INTERMEDIATE_)$(ZLIB_BASE)
ZLIB_OUT = $(OUTPUT_)$(ZLIB_BASE)

#$(ZLIB_INT): $(INTERMEDIATE_NO_SLASH) $(RMKDIR_TARGET)
#	${mkdir} $(INTERMEDIATE)$(ZLIB_BASE)

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
	infblock.c \
	inftrees.c \
	infcodes.c \
	infutil.c \
	inffast.c \
	)

ZLIB_HOST_OBJECTS = \
	$(ZLIB_HOST_SOURCES:.c=.o)

ZLIB_HOST_CFLAGS = -MMD -O3 -Wall -Wwrite-strings -Wpointer-arith -Wconversion \
  -Wstrict-prototypes -Wmissing-prototypes

.PHONY: zlib_host
zlib_host: $(ZLIB_HOST_TARGET)

$(ZLIB_HOST_TARGET): $(ZLIB_HOST_OBJECTS) $(ZLIB_OUT)
	$(ECHO_AR)
	$(host_ar) -r $@ $(ZLIB_HOST_OBJECTS)

$(ZLIB_HOST_OBJECTS): %.o : %.c $(ZLIB_BASE_DIR)
	$(ECHO_CC)
	${host_gcc} $(ZLIB_HOST_CFLAGS) -c $< -o $@

.PHONY: zlib_host_clean
zlib_host_clean:
	-@$(rm) $(ZLIB_HOST_TARGET) $(ZLIB_HOST_OBJECTS) 2>$(NUL)
clean: zlib_clean
