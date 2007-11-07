WRC_BASE = $(TOOLS_BASE_)wrc
WRC_BASE_ = $(WRC_BASE)$(SEP)
WRC_INT = $(INTERMEDIATE_)$(WRC_BASE)
WRC_INT_ = $(WRC_INT)$(SEP)
WRC_OUT = $(OUTPUT_)$(WRC_BASE)
WRC_OUT_ = $(WRC_OUT)$(SEP)

$(WRC_INT): | $(TOOLS_INT)
	$(ECHO_MKDIR)
	${mkdir} $@

ifneq ($(INTERMEDIATE),$(OUTPUT))
$(WRC_OUT): | $(TOOLS_OUT)
	$(ECHO_MKDIR)
	${mkdir} $@
endif

WRC_PORT_BASE = $(WRC_BASE)$(SEP)port
WRC_PORT_BASE_ = $(WRC_PORT_BASE)$(SEP)
WRC_PORT_INT = $(INTERMEDIATE_)$(WRC_PORT_BASE)
WRC_PORT_INT_ = $(WRC_PORT_INT)$(SEP)
WRC_PORT_OUT = $(OUTPUT_)$(WRC_PORT_BASE)
WRC_PORT_OUT_ = $(WRC_PORT_OUT)$(SEP)

$(WRC_PORT_INT): | $(WRC_INT)
	$(ECHO_MKDIR)
	${mkdir} $@

ifneq ($(INTERMEDIATE),$(OUTPUT))
$(WRC_PORT_OUT): | $(WRC_OUT)
	$(ECHO_MKDIR)
	${mkdir} $@
endif

WRC_TARGET = \
	$(WRC_OUT_)wrc$(EXEPOSTFIX)

WRC_DEPENDS = $(BUILDNO_H)

WRC_SOURCES = $(addprefix $(WRC_BASE_), \
	dumpres.c \
	genres.c \
	newstruc.c \
	readres.c \
	translation.c \
	utils.c \
	wrc.c \
	writeres.c \
	parser.tab.c \
	lex.yy.c \
	port$(SEP)mkstemps.c \
	)

WRC_OBJECTS = \
  $(addprefix $(INTERMEDIATE_), $(WRC_SOURCES:.c=.o))

WRC_HOST_CFLAGS = -I$(WRC_BASE) $(TOOLS_CFLAGS) \
                  -D__USE_W32API -DWINE_UNICODE_API= \
                  -I$(UNICODE_BASE) -I$(WPP_BASE) \
                  -Iinclude/reactos/wine -Iinclude/reactos -Iinclude \
                  -I$(INTERMEDIATE_)include

WRC_HOST_LFLAGS = $(TOOLS_LFLAGS)

WRC_LIBS = $(UNICODE_TARGET) $(WPP_TARGET)

.PHONY: wrc
wrc: $(WRC_TARGET)

$(WRC_TARGET): $(WRC_OBJECTS) $(WRC_LIBS) | $(WRC_OUT)
	$(ECHO_LD)
	${host_gcc} $(WRC_OBJECTS) $(WRC_LIBS) $(WRC_HOST_LFLAGS) -o $@

$(WRC_INT_)dumpres.o: $(WRC_BASE_)dumpres.c $(WRC_DEPENDS) | $(WRC_INT)
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $< -o $@

$(WRC_INT_)genres.o: $(WRC_BASE_)genres.c $(WRC_DEPENDS) | $(WRC_INT)
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $< -o $@

$(WRC_INT_)newstruc.o: $(WRC_BASE_)newstruc.c $(WRC_DEPENDS) | $(WRC_INT)
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $< -o $@

$(WRC_INT_)readres.o: $(WRC_BASE_)readres.c $(WRC_DEPENDS) | $(WRC_INT)
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $< -o $@

$(WRC_INT_)translation.o: $(WRC_BASE_)translation.c $(WRC_DEPENDS) | $(WRC_INT)
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $< -o $@

$(WRC_INT_)utils.o: $(WRC_BASE_)utils.c $(WRC_DEPENDS) | $(WRC_INT)
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $< -o $@

$(WRC_INT_)wrc.o: $(WRC_BASE_)wrc.c $(WRC_DEPENDS) | $(WRC_INT)
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $< -o $@

$(WRC_INT_)writeres.o: $(WRC_BASE_)writeres.c $(WRC_DEPENDS) | $(WRC_INT)
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $< -o $@

$(WRC_INT_)parser.tab.o: $(WRC_BASE_)parser.tab.c $(WRC_DEPENDS) | $(WRC_INT)
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $< -o $@

$(WRC_INT_)lex.yy.o: $(WRC_BASE_)lex.yy.c $(WRC_DEPENDS) | $(WRC_INT)
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $< -o $@

$(WRC_PORT_INT_)mkstemps.o: $(WRC_PORT_BASE_)mkstemps.c $(WRC_DEPENDS) | $(WRC_PORT_INT)
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $< -o $@

.PHONY: wrc_clean
wrc_clean:
	-@$(rm) $(WRC_TARGET) $(WRC_OBJECTS) 2>$(NUL)
clean: wrc_clean
