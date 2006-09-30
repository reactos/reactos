WIDL_BASE = $(TOOLS_BASE)$(SEP)widl
WIDL_BASE_ = $(WIDL_BASE)$(SEP)
WIDL_INT = $(INTERMEDIATE_)$(WIDL_BASE)
WIDL_INT_ = $(WIDL_INT)$(SEP)
WIDL_OUT = $(OUTPUT_)$(WIDL_BASE)
WIDL_OUT_ = $(WIDL_OUT)$(SEP)

$(WIDL_INT): | $(TOOLS_INT)
	$(ECHO_MKDIR)
	${mkdir} $@

ifneq ($(INTERMEDIATE),$(OUTPUT))
$(WIDL_OUT): | $(TOOLS_OUT)
	$(ECHO_MKDIR)
	${mkdir} $@
endif

WIDL_PORT_BASE = $(WIDL_BASE)$(SEP)port
WIDL_PORT_BASE_ = $(WIDL_PORT_BASE)$(SEP)
WIDL_PORT_INT = $(INTERMEDIATE_)$(WIDL_PORT_BASE)
WIDL_PORT_INT_ = $(WIDL_PORT_INT)$(SEP)
WIDL_PORT_OUT = $(OUTPUT_)$(WIDL_PORT_BASE)
WIDL_PORT_OUT_ = $(WIDL_PORT_OUT)$(SEP)

$(WIDL_PORT_INT): | $(WIDL_INT)
	$(ECHO_MKDIR)
	${mkdir} $@

ifneq ($(INTERMEDIATE),$(OUTPUT))
$(WIDL_PORT_OUT): | $(WIDL_OUT)
	$(ECHO_MKDIR)
	${mkdir} $@
endif

WIDL_TARGET = \
	$(EXEPREFIX)$(WIDL_OUT_)widl$(EXEPOSTFIX)

WIDL_SOURCES = $(addprefix $(WIDL_BASE_), \
	client.c \
	hash.c \
	header.c \
	lex.yy.c \
	proxy.c \
	server.c \
	typegen.c \
	typelib.c \
	utils.c \
	widl.c \
	write_msft.c \
	parser.tab.c \
	port$(SEP)mkstemps.c \
	)

WIDL_OBJECTS = \
  $(addprefix $(INTERMEDIATE_), $(WIDL_SOURCES:.c=.o))

WIDL_HOST_CFLAGS = $(TOOLS_CFLAGS) \
	-DINT16=SHORT -D__USE_W32API -DYYDEBUG=1 -D__REACTOS__=1 \
	-I$(WIDL_BASE) -I$(WPP_BASE) \
	-Iinclude/reactos/wine -Iinclude/reactos -Iinclude -Iinclude/psdk

WIDL_HOST_LFLAGS = $(TOOLS_LFLAGS)

WIDL_LIBS = $(WPP_TARGET)

.PHONY: widl
widl: $(WIDL_TARGET)

$(WIDL_TARGET): $(WIDL_OBJECTS) $(WIDL_LIBS) | $(WIDL_OUT)
	$(ECHO_LD)
	${host_gcc} $(WIDL_OBJECTS) $(WIDL_LIBS) $(WIDL_HOST_LFLAGS) -o $@

$(WIDL_INT_)client.o: $(WIDL_BASE_)client.c | $(WIDL_INT)
	$(ECHO_CC)
	${host_gcc} $(WIDL_HOST_CFLAGS) -c $< -o $@

$(WIDL_INT_)hash.o: $(WIDL_BASE_)hash.c | $(WIDL_INT)
	$(ECHO_CC)
	${host_gcc} $(WIDL_HOST_CFLAGS) -c $< -o $@

$(WIDL_INT_)header.o: $(WIDL_BASE_)header.c | $(WIDL_INT)
	$(ECHO_CC)
	${host_gcc} $(WIDL_HOST_CFLAGS) -c $< -o $@

$(WIDL_INT_)lex.yy.o: $(WIDL_BASE_)lex.yy.c | $(WIDL_INT)
	$(ECHO_CC)
	${host_gcc} $(WIDL_HOST_CFLAGS) -c $< -o $@

$(WIDL_INT_)proxy.o: $(WIDL_BASE_)proxy.c | $(WIDL_INT)
	$(ECHO_CC)
	${host_gcc} $(WIDL_HOST_CFLAGS) -c $< -o $@

$(WIDL_INT_)server.o: $(WIDL_BASE_)server.c | $(WIDL_INT)
	$(ECHO_CC)
	${host_gcc} $(WIDL_HOST_CFLAGS) -c $< -o $@

$(WIDL_INT_)typegen.o: $(WIDL_BASE_)typegen.c | $(WIDL_INT)
	$(ECHO_CC)
	${host_gcc} $(WIDL_HOST_CFLAGS) -c $< -o $@

$(WIDL_INT_)typelib.o: $(WIDL_BASE_)typelib.c | $(WIDL_INT)
	$(ECHO_CC)
	${host_gcc} $(WIDL_HOST_CFLAGS) -c $< -o $@

$(WIDL_INT_)utils.o: $(WIDL_BASE_)utils.c | $(WIDL_INT)
	$(ECHO_CC)
	${host_gcc} $(WIDL_HOST_CFLAGS) -c $< -o $@

$(WIDL_INT_)widl.o: $(WIDL_BASE_)widl.c | $(WIDL_INT)
	$(ECHO_CC)
	${host_gcc} $(WIDL_HOST_CFLAGS) -c $< -o $@

$(WIDL_INT_)write_msft.o: $(WIDL_BASE_)write_msft.c | $(WIDL_INT)
	$(ECHO_CC)
	${host_gcc} $(WIDL_HOST_CFLAGS) -c $< -o $@

$(WIDL_INT_)parser.tab.o: $(WIDL_BASE_)parser.tab.c | $(WIDL_INT)
	$(ECHO_CC)
	${host_gcc} $(WIDL_HOST_CFLAGS) -c $< -o $@

$(WIDL_PORT_INT_)mkstemps.o: $(WIDL_PORT_BASE_)mkstemps.c | $(WIDL_PORT_INT)
	$(ECHO_CC)
	${host_gcc} $(WIDL_HOST_CFLAGS) -c $< -o $@

.PHONY: widl_clean
widl_clean:
	-@$(rm) $(WIDL_TARGET) $(WIDL_OBJECTS) 2>$(NUL)
clean: widl_clean
