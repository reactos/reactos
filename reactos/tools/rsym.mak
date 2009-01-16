RSYM_BASE = $(TOOLS_BASE)
RSYM_BASE_ = $(RSYM_BASE)$(SEP)

RSYM_INT = $(INTERMEDIATE_)$(RSYM_BASE)
RSYM_INT_ = $(RSYM_INT)$(SEP)
RSYM_OUT = $(OUTPUT_)$(RSYM_BASE)
RSYM_OUT_ = $(RSYM_OUT)$(SEP)

RSYM_TARGET = \
	$(RSYM_OUT_)rsym$(EXEPOSTFIX)

RSYM_SOURCES = \
	$(RSYM_BASE_)rsym.c \
	$(RSYM_BASE_)rsym_common.c

RSYM_OBJECTS = \
	$(addprefix $(INTERMEDIATE_), $(RSYM_SOURCES:.c=.o))

ifeq ($(ARCH),amd64)
RSYM_HOST_CFLAGS = $(TOOLS_CFLAGS) -D_TARGET_PE64
else
RSYM_HOST_CFLAGS = $(TOOLS_CFLAGS)
endif

RSYM_HOST_LFLAGS = $(TOOLS_LFLAGS)

.PHONY: rsym
rsym: $(RSYM_TARGET)

$(RSYM_TARGET): $(RSYM_OBJECTS) | $(RSYM_OUT)
	$(ECHO_HOSTLD)
	${host_gcc} $(RSYM_OBJECTS) $(RSYM_HOST_LFLAGS) -o $@

$(RSYM_INT_)rsym.o: $(RSYM_BASE_)rsym.c | $(RSYM_INT)
	$(ECHO_HOSTCC)
	${host_gcc} $(RSYM_HOST_CFLAGS) -c $< -o $@

$(RSYM_INT_)rsym_common.o: $(RSYM_BASE_)rsym_common.c | $(RSYM_INT)
	$(ECHO_HOSTCC)
	${host_gcc} $(RSYM_HOST_CFLAGS) -c $< -o $@

.PHONY: rsym_clean
rsym_clean:
	-@$(rm) $(RSYM_TARGET) $(RSYM_OBJECTS) 2>$(NUL)
clean: rsym_clean
