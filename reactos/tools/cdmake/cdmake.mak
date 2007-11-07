CDMAKE_BASE = $(TOOLS_BASE_)cdmake
CDMAKE_BASE_ = $(CDMAKE_BASE)$(SEP)
CDMAKE_INT = $(INTERMEDIATE_)$(CDMAKE_BASE)
CDMAKE_INT_ = $(CDMAKE_INT)$(SEP)
CDMAKE_OUT = $(OUTPUT_)$(CDMAKE_BASE)
CDMAKE_OUT_ = $(CDMAKE_OUT)$(SEP)

$(CDMAKE_INT): | $(TOOLS_INT)
	$(ECHO_MKDIR)
	${mkdir} $@

ifneq ($(INTERMEDIATE),$(OUTPUT))
$(CDMAKE_OUT): | $(TOOLS_OUT)
	$(ECHO_MKDIR)
	${mkdir} $@
endif

CDMAKE_TARGET = \
	$(CDMAKE_OUT_)cdmake$(EXEPOSTFIX)

CDMAKE_SOURCES = $(addprefix $(CDMAKE_BASE_), \
	cdmake.c \
	llmosrt.c \
	)

CDMAKE_OBJECTS = \
	$(addprefix $(INTERMEDIATE_), $(CDMAKE_SOURCES:.c=.o))

CDMAKE_HOST_CFLAGS = -Iinclude $(TOOLS_CFLAGS)

CDMAKE_HOST_LFLAGS = $(TOOLS_LFLAGS)

.PHONY: cdmake
cdmake: $(CDMAKE_TARGET)

$(CDMAKE_TARGET): $(CDMAKE_OBJECTS) | $(CDMAKE_OUT)
	$(ECHO_LD)
	${host_gcc} $(CDMAKE_OBJECTS) $(CDMAKE_HOST_LFLAGS) -o $@

$(CDMAKE_INT_)cdmake.o: $(CDMAKE_BASE_)cdmake.c | $(CDMAKE_INT)
	$(ECHO_CC)
	${host_gcc} $(CDMAKE_HOST_CFLAGS) -c $< -o $@

$(CDMAKE_INT_)llmosrt.o: $(CDMAKE_BASE_)llmosrt.c | $(CDMAKE_INT)
	$(ECHO_CC)
	${host_gcc} $(CDMAKE_HOST_CFLAGS) -c $< -o $@

.PHONY: cdmake_clean
cdmake_clean:
	-@$(rm) $(CDMAKE_TARGET) $(CDMAKE_OBJECTS) 2>$(NUL)
clean: cdmake_clean
