WRC_BASE = $(TOOLS_BASE)$(SEP)wrc

WRC_BASE_DIR = $(INTERMEDIATE)$(WRC_BASE)
WRC_BASE_DIR_EXISTS = $(WRC_BASE_DIR)$(SEP)$(EXISTS)

$(WRC_BASE_DIR_EXISTS): $(TOOLS_BASE_DIR_EXISTS)
	$(ECHO_MKDIR)
	${mkdir} $(WRC_BASE_DIR)
	@echo . > $@

WRC_PORT_BASE = $(WRC_BASE)$(SEP)port

WRC_PORT_BASE_DIR = $(INTERMEDIATE)$(WRC_PORT_BASE)
WRC_PORT_BASE_DIR_EXISTS = $(WRC_PORT_BASE_DIR)$(SEP)$(EXISTS)

$(WRC_PORT_BASE_DIR_EXISTS): $(WRC_BASE_DIR_EXISTS)
	$(ECHO_MKDIR)
	${mkdir} $(WRC_PORT_BASE_DIR)
	@echo . > $@

WRC_TARGET = \
	$(WRC_BASE_DIR)$(SEP)wrc$(EXEPOSTFIX)

WRC_SOURCES = $(addprefix $(WRC_BASE)$(SEP), \
	dumpres.c \
	genres.c \
	newstruc.c \
	readres.c \
	translation.c \
	utils.c \
	wrc.c \
	writeres.c \
	y.tab.c \
	lex.yy.c \
	port$(SEP)mkstemps.c \
	)

WRC_OBJECTS = \
  $(addprefix $(INTERMEDIATE), $(WRC_SOURCES:.c=.o))

WRC_HOST_CFLAGS = -I$(WRC_BASE) -g -Werror -Wall \
                  -D__USE_W32API -DWINE_UNICODE_API= \
                  -Dwchar_t="unsigned short" -D_WCHAR_T_DEFINED \
                  -I$(UNICODE_BASE) -I$(WPP_BASE) -I$(WRC_BASE) \
                  -Iinclude/wine -Iinclude -Iw32api/include

WRC_HOST_LFLAGS = -g

.PHONY: wrc
wrc: $(WRC_TARGET)

$(WRC_TARGET): $(WRC_OBJECTS) $(UNICODE_TARGET) $(WPP_TARGET)
	$(ECHO_LD)
	${host_gcc} $(WRC_OBJECTS) $(UNICODE_TARGET) $(WPP_TARGET) $(WRC_HOST_LFLAGS) -o $@

$(WRC_BASE_DIR)$(SEP)dumpres.o: $(WRC_BASE)$(SEP)dumpres.c $(WRC_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $< -o $@

$(WRC_BASE_DIR)$(SEP)genres.o: $(WRC_BASE)$(SEP)genres.c $(WRC_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $< -o $@

$(WRC_BASE_DIR)$(SEP)newstruc.o: $(WRC_BASE)$(SEP)newstruc.c $(WRC_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $< -o $@

$(WRC_BASE_DIR)$(SEP)readres.o: $(WRC_BASE)$(SEP)readres.c $(WRC_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $< -o $@

$(WRC_BASE_DIR)$(SEP)translation.o: $(WRC_BASE)$(SEP)translation.c $(WRC_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $< -o $@

$(WRC_BASE_DIR)$(SEP)utils.o: $(WRC_BASE)$(SEP)utils.c $(WRC_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $< -o $@

$(WRC_BASE_DIR)$(SEP)wrc.o: $(WRC_BASE)$(SEP)wrc.c $(WRC_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $< -o $@

$(WRC_BASE_DIR)$(SEP)writeres.o: $(WRC_BASE)$(SEP)writeres.c $(WRC_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $< -o $@

$(WRC_BASE_DIR)$(SEP)y.tab.o: $(WRC_BASE)$(SEP)y.tab.c $(WRC_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $< -o $@

$(WRC_BASE_DIR)$(SEP)lex.yy.o: $(WRC_BASE)$(SEP)lex.yy.c $(WRC_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $< -o $@

$(WRC_PORT_BASE_DIR)$(SEP)mkstemps.o: $(WRC_PORT_BASE)$(SEP)mkstemps.c $(WRC_PORT_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $< -o $@

.PHONY: wrc_clean
wrc_clean:
	-@$(rm) $(WRC_TARGET) $(WRC_OBJECTS) 2>$(NUL)
clean: wrc_clean
