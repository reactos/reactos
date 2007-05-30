INFLIB_BASE = $(LIB_BASE_)inflib
INFLIB_BASE_ = $(INFLIB_BASE)$(SEP)
INFLIB_INT = $(INTERMEDIATE_)$(INFLIB_BASE)_host
INFLIB_INT_ = $(INTERMEDIATE_)$(INFLIB_BASE)_host$(SEP)
INFLIB_OUT = $(OUTPUT_)$(INFLIB_BASE)_host
INFLIB_OUT_ = $(OUTPUT_)$(INFLIB_BASE)_host$(SEP)

$(INFLIB_INT): | $(LIB_INT)
	$(ECHO_MKDIR)
	${mkdir} $@

ifneq ($(INTERMEDIATE),$(OUTPUT))
$(INFLIB_OUT): | $(OUTPUT_)$(LIB_BASE)
	$(ECHO_MKDIR)
	${mkdir} $@
endif

INFLIB_HOST_TARGET = \
	$(INFLIB_OUT)$(SEP)inflib.a

INFLIB_HOST_SOURCES = $(addprefix $(INFLIB_BASE_), \
	infcore.c \
	infget.c \
	infput.c \
	infhostgen.c \
	infhostget.c \
	infhostglue.c \
	infhostput.c \
	)

INFLIB_HOST_OBJECTS = \
	$(subst $(INFLIB_BASE), $(INFLIB_INT), $(INFLIB_HOST_SOURCES:.c=.o))

INFLIB_HOST_CFLAGS = -O3 -Wall -Wpointer-arith -Wconversion \
  -Wstrict-prototypes -Wmissing-prototypes -DINFLIB_HOST -D_M_IX86 \
  -I$(INFLIB_BASE) -Iinclude/reactos -DDBG

$(INFLIB_HOST_TARGET): $(INFLIB_HOST_OBJECTS) | $(INFLIB_OUT)
	$(ECHO_AR)
	$(host_ar) -r $@ $(INFLIB_HOST_OBJECTS)

$(INFLIB_INT_)infcore.o: $(INFLIB_BASE_)infcore.c | $(INFLIB_INT)
	$(ECHO_CC)
	${host_gcc} $(INFLIB_HOST_CFLAGS) -c $< -o $@

$(INFLIB_INT_)infget.o: $(INFLIB_BASE_)infget.c | $(INFLIB_INT)
	$(ECHO_CC)
	${host_gcc} $(INFLIB_HOST_CFLAGS) -c $< -o $@

$(INFLIB_INT_)infput.o: $(INFLIB_BASE_)infput.c | $(INFLIB_INT)
	$(ECHO_CC)
	${host_gcc} $(INFLIB_HOST_CFLAGS) -c $< -o $@

$(INFLIB_INT_)infhostgen.o: $(INFLIB_BASE_)infhostgen.c | $(INFLIB_INT)
	$(ECHO_CC)
	${host_gcc} $(INFLIB_HOST_CFLAGS) -c $< -o $@

$(INFLIB_INT_)infhostget.o: $(INFLIB_BASE_)infhostget.c | $(INFLIB_INT)
	$(ECHO_CC)
	${host_gcc} $(INFLIB_HOST_CFLAGS) -c $< -o $@

$(INFLIB_INT_)infhostglue.o: $(INFLIB_BASE_)infhostglue.c | $(INFLIB_INT)
	$(ECHO_CC)
	${host_gcc} $(INFLIB_HOST_CFLAGS) -c $< -o $@

$(INFLIB_INT_)infhostput.o: $(INFLIB_BASE_)infhostput.c | $(INFLIB_INT)
	$(ECHO_CC)
	${host_gcc} $(INFLIB_HOST_CFLAGS) -c $< -o $@

.PHONY: inflib_host
inflib_host: $(INFLIB_HOST_TARGET)

.PHONY: inflib_host_clean
inflib_host_clean:
	-@$(rm) $(INFLIB_HOST_TARGET) $(INFLIB_HOST_OBJECTS) 2>$(NUL)
clean: inflib_host_clean
