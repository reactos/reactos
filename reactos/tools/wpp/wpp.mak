WPP_BASE = $(TOOLS_BASE)$(SEP)wpp

WPP_BASE_DIR = $(INTERMEDIATE)$(WPP_BASE)
WPP_BASE_DIR_EXISTS = $(WPP_BASE_DIR)$(SEP)$(EXISTS)

$(WPP_BASE_DIR_EXISTS): $(TOOLS_BASE_DIR_EXISTS)
	$(ECHO_MKDIR)
	${mkdir} $(WPP_BASE_DIR)
	@echo . > $@

WPP_TARGET = \
	$(WPP_BASE_DIR)$(SEP)libwpp.a

WPP_SOURCES = \
	$(WPP_BASE)$(SEP)lex.yy.c \
	$(WPP_BASE)$(SEP)preproc.c \
	$(WPP_BASE)$(SEP)wpp.c \
	$(WPP_BASE)$(SEP)wpp.tab.c

WPP_OBJECTS = \
    $(addprefix $(INTERMEDIATE), $(WPP_SOURCES:.c=.o))

WPP_HOST_CFLAGS = -D__USE_W32API -I$(WPP_BASE) -Iinclude -Iinclude/wine -g

.PHONY: wpp
wpp: $(WPP_TARGET)

$(WPP_TARGET): $(WPP_OBJECTS)
	$(ECHO_AR)
	${host_ar} -rc $(WPP_TARGET) $(WPP_OBJECTS)

$(WPP_BASE_DIR)$(SEP)lex.yy.o: $(WPP_BASE)$(SEP)lex.yy.c $(WPP_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WPP_HOST_CFLAGS) -c $< -o $@

$(WPP_BASE_DIR)$(SEP)preproc.o: $(WPP_BASE)$(SEP)preproc.c $(WPP_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WPP_HOST_CFLAGS) -c $< -o $@

$(WPP_BASE_DIR)$(SEP)wpp.o: $(WPP_BASE)$(SEP)wpp.c $(WPP_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WPP_HOST_CFLAGS) -c $< -o $@

$(WPP_BASE_DIR)$(SEP)wpp.tab.o: $(WPP_BASE)$(SEP)wpp.tab.c $(WPP_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WPP_HOST_CFLAGS) -c $< -o $@

.PHONY: wpp_clean
wpp_clean:
	-@$(rm) $(WPP_TARGET) $(WPP_OBJECTS) 2>$(NUL)
clean: wpp_clean
