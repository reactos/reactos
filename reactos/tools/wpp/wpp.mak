WPP_BASE = tools$(SEP)wpp

$(INTERMEDIATE)$(WPP_BASE): $(INTERMEDIATE_NO_SLASH) $(RMKDIR_TARGET)
	${mkdir} $(INTERMEDIATE)$(WPP_BASE)

WPP_TARGET = \
	$(INTERMEDIATE)$(WPP_BASE)$(SEP)libwpp.a

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

$(WPP_TARGET): $(INTERMEDIATE)$(WPP_BASE) $(WPP_OBJECTS)
	$(ECHO_AR)
	${host_ar} -rc $(WPP_TARGET) $(WPP_OBJECTS)

$(INTERMEDIATE)$(WPP_BASE)$(SEP)lex.yy.o: $(WPP_BASE)$(SEP)lex.yy.c $(INTERMEDIATE)$(WPP_BASE)
	$(ECHO_CC)
	${host_gcc} $(WPP_HOST_CFLAGS) -c $< -o $@

$(INTERMEDIATE)$(WPP_BASE)$(SEP)preproc.o: $(WPP_BASE)$(SEP)preproc.c $(INTERMEDIATE)$(WPP_BASE)
	$(ECHO_CC)
	${host_gcc} $(WPP_HOST_CFLAGS) -c $< -o $@

$(INTERMEDIATE)$(WPP_BASE)$(SEP)wpp.o: $(WPP_BASE)$(SEP)wpp.c $(INTERMEDIATE)$(WPP_BASE)
	$(ECHO_CC)
	${host_gcc} $(WPP_HOST_CFLAGS) -c $< -o $@

$(INTERMEDIATE)$(WPP_BASE)$(SEP)wpp.tab.o: $(WPP_BASE)$(SEP)wpp.tab.c $(INTERMEDIATE)$(WPP_BASE)
	$(ECHO_CC)
	${host_gcc} $(WPP_HOST_CFLAGS) -c $< -o $@

.PHONY: wpp_clean
wpp_clean:
	-@$(rm) $(WPP_TARGET) $(WPP_OBJECTS) 2>$(NUL)
clean: wpp_clean
