WPP_BASE = tools$(SEP)wpp

$(INTERMEDIATE)$(WPP_BASE): $(RMKDIR_TARGET)
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

$(WPP_TARGET): $(INTERMEDIATE)$(WPP_BASE) $(WPP_OBJECTS)
	$(ECHO_AR)
	${host_ar} -rc $(WPP_TARGET) $(WPP_OBJECTS)

$(INTERMEDIATE)$(WPP_BASE)$(SEP)lex.yy.o: $(INTERMEDIATE)$(WPP_BASE) $(WPP_BASE)$(SEP)lex.yy.c
	$(ECHO_CC)
	${host_gcc} $(WPP_HOST_CFLAGS) -c $(WPP_BASE)$(SEP)lex.yy.c -o $(INTERMEDIATE)$(WPP_BASE)$(SEP)lex.yy.o

$(INTERMEDIATE)$(WPP_BASE)$(SEP)preproc.o: $(INTERMEDIATE)$(WPP_BASE) $(WPP_BASE)$(SEP)preproc.c
	$(ECHO_CC)
	${host_gcc} $(WPP_HOST_CFLAGS) -c $(WPP_BASE)$(SEP)preproc.c -o $(INTERMEDIATE)$(WPP_BASE)$(SEP)preproc.o

$(INTERMEDIATE)$(WPP_BASE)$(SEP)wpp.o: $(INTERMEDIATE)$(WPP_BASE) $(WPP_BASE)$(SEP)wpp.c
	$(ECHO_CC)
	${host_gcc} $(WPP_HOST_CFLAGS) -c $(WPP_BASE)$(SEP)wpp.c -o $(INTERMEDIATE)$(WPP_BASE)$(SEP)wpp.o

$(INTERMEDIATE)$(WPP_BASE)$(SEP)wpp.tab.o: $(INTERMEDIATE)$(WPP_BASE) $(WPP_BASE)$(SEP)wpp.tab.c
	$(ECHO_CC)
	${host_gcc} $(WPP_HOST_CFLAGS) -c $(WPP_BASE)$(SEP)wpp.tab.c -o $(INTERMEDIATE)$(WPP_BASE)$(SEP)wpp.tab.o

.PHONY: wpp_clean
wpp_clean:
	-@$(rm) $(WPP_TARGET) $(WPP_OBJECTS) 2>$(NUL)
clean: wpp_clean
