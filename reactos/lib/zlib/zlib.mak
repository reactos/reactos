ZLIB_BASE = lib$(SEP)zlib

ZLIB_BASE_DIR = $(INTERMEDIATE)$(ZLIB_BASE)

#$(ZLIB_BASE_DIR): $(INTERMEDIATE_NO_SLASH) $(RMKDIR_TARGET)
#	${mkdir} $(INTERMEDIATE)$(ZLIB_BASE)

ZLIB_HOST_TARGET = \
	$(INTERMEDIATE)$(ZLIB_BASE)$(SEP)zlib.host.a

ZLIB_HOST_SOURCES = \
	$(ZLIB_BASE)$(SEP)cdmake.c \
	$(ZLIB_BASE)$(SEP)llmosrt.c \
	$(ZLIB_BASE)$(SEP)adler32.c \
	$(ZLIB_BASE)$(SEP)compress.c \
	$(ZLIB_BASE)$(SEP)crc32.c \
	$(ZLIB_BASE)$(SEP)gzio.c \
	$(ZLIB_BASE)$(SEP)uncompr.c \
	$(ZLIB_BASE)$(SEP)deflate.c \
	$(ZLIB_BASE)$(SEP)trees.c \
	$(ZLIB_BASE)$(SEP)zutil.c \
	$(ZLIB_BASE)$(SEP)inflate.c \
	$(ZLIB_BASE)$(SEP)infblock.c \
	$(ZLIB_BASE)$(SEP)inftrees.c \
	$(ZLIB_BASE)$(SEP)infcodes.c \
	$(ZLIB_BASE)$(SEP)infutil.c \
	$(ZLIB_BASE)$(SEP)inffast.c

ZLIB_HOST_OBJECTS = \
	$(ZLIB_HOST_SOURCES:.c=.o)

ZLIB_HOST_CFLAGS = -MMD -O3 -Wall -Wwrite-strings -Wpointer-arith -Wconversion \
  -Wstrict-prototypes -Wmissing-prototypes

$(ZLIB_HOST_TARGET): $(ZLIB_HOST_BASE_DIR) $(ZLIB_HOST_OBJECTS)
	$(ECHO_AR)
	$(host_ar) -r $(ZLIB_HOST_TARGET) $(ZLIB_HOST_OBJECTS)

$(ZLIB_HOST_OBJECTS): %.o : %.c $(ZLIB_BASE_DIR)
	$(ECHO_CC)
	${host_gcc} $(ZLIB_HOST_CFLAGS) -c $< -o $@

.PHONY: zlib_host_clean
zlib_host_clean:
	-@$(rm) $(ZLIB_HOST_TARGET) $(ZLIB_HOST_OBJECTS) 2>$(NUL)
clean: zlib_clean
