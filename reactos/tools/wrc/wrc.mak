WRC_BASE = tools$(SEP)wrc

WRC_BASE_DIR = $(INTERMEDIATE)$(WRC_BASE)$(SEP)$(CREATED)

$(WRC_BASE_DIR): $(RMKDIR_TARGET)
	${mkdir} $(INTERMEDIATE)$(WRC_BASE)

WRC_BASE_PORT_DIR = $(INTERMEDIATE)$(WRC_BASE)$(SEP)port$(SEP)$(CREATED)

$(WRC_BASE_PORT_DIR): $(RMKDIR_TARGET) $(WRC_BASE_DIR)
	${mkdir} $(INTERMEDIATE)$(WRC_BASE)$(SEP)port

WRC_TARGET = \
	$(INTERMEDIATE)$(WRC_BASE)$(SEP)wrc$(EXEPOSTFIX)

WRC_SOURCES = \
	$(WRC_BASE)$(SEP)dumpres.c \
	$(WRC_BASE)$(SEP)genres.c \
	$(WRC_BASE)$(SEP)newstruc.c \
	$(WRC_BASE)$(SEP)readres.c \
	$(WRC_BASE)$(SEP)translation.c \
	$(WRC_BASE)$(SEP)utils.c \
	$(WRC_BASE)$(SEP)wrc.c \
	$(WRC_BASE)$(SEP)writeres.c \
	$(WRC_BASE)$(SEP)y.tab.c \
	$(WRC_BASE)$(SEP)lex.yy.c \
	$(WRC_BASE)$(SEP)port$(SEP)mkstemps.o

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

$(WRC_TARGET): $(WRC_BASE_DIR) $(WRC_OBJECTS) $(UNICODE_TARGET) $(WPP_TARGET)
	$(ECHO_LD)
	${host_gcc} $(WRC_OBJECTS) $(UNICODE_TARGET) $(WPP_TARGET) $(WRC_HOST_LFLAGS) -o $(WRC_TARGET)

$(INTERMEDIATE)$(WRC_BASE)$(SEP)dumpres.o: $(WRC_BASE_DIR) $(WRC_BASE)$(SEP)dumpres.c
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $(WRC_BASE)$(SEP)dumpres.c -o $(INTERMEDIATE)$(WRC_BASE)$(SEP)dumpres.o

$(INTERMEDIATE)$(WRC_BASE)$(SEP)genres.o: $(WRC_BASE_DIR) $(WRC_BASE)$(SEP)genres.c
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $(WRC_BASE)$(SEP)genres.c -o $(INTERMEDIATE)$(WRC_BASE)$(SEP)genres.o

$(INTERMEDIATE)$(WRC_BASE)$(SEP)newstruc.o: $(WRC_BASE_DIR) $(WRC_BASE)$(SEP)newstruc.c
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $(WRC_BASE)$(SEP)newstruc.c -o $(INTERMEDIATE)$(WRC_BASE)$(SEP)newstruc.o

$(INTERMEDIATE)$(WRC_BASE)$(SEP)readres.o: $(WRC_BASE_DIR) $(WRC_BASE)$(SEP)readres.c
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $(WRC_BASE)$(SEP)readres.c -o $(INTERMEDIATE)$(WRC_BASE)$(SEP)readres.o

$(INTERMEDIATE)$(WRC_BASE)$(SEP)translation.o: $(WRC_BASE_DIR) $(WRC_BASE)$(SEP)translation.c
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $(WRC_BASE)$(SEP)translation.c -o $(INTERMEDIATE)$(WRC_BASE)$(SEP)translation.o

$(INTERMEDIATE)$(WRC_BASE)$(SEP)utils.o: $(WRC_BASE_DIR) $(WRC_BASE)$(SEP)utils.c
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $(WRC_BASE)$(SEP)utils.c -o $(INTERMEDIATE)$(WRC_BASE)$(SEP)utils.o

$(INTERMEDIATE)$(WRC_BASE)$(SEP)wrc.o: $(WRC_BASE_DIR) $(WRC_BASE)$(SEP)wrc.c
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $(WRC_BASE)$(SEP)wrc.c -o $(INTERMEDIATE)$(WRC_BASE)$(SEP)wrc.o

$(INTERMEDIATE)$(WRC_BASE)$(SEP)writeres.o: $(WRC_BASE_DIR) $(WRC_BASE)$(SEP)writeres.c
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $(WRC_BASE)$(SEP)writeres.c -o $(INTERMEDIATE)$(WRC_BASE)$(SEP)writeres.o

$(INTERMEDIATE)$(WRC_BASE)$(SEP)y.tab.o: $(WRC_BASE_DIR) $(WRC_BASE)$(SEP)y.tab.c
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $(WRC_BASE)$(SEP)y.tab.c -o $(INTERMEDIATE)$(WRC_BASE)$(SEP)y.tab.o

$(INTERMEDIATE)$(WRC_BASE)$(SEP)lex.yy.o: $(WRC_BASE_DIR) $(WRC_BASE)$(SEP)lex.yy.c
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $(WRC_BASE)$(SEP)lex.yy.c -o $(INTERMEDIATE)$(WRC_BASE)$(SEP)lex.yy.o

$(INTERMEDIATE)$(WRC_BASE)$(SEP)port$(SEP)mkstemps.o: $(WRC_BASE_PORT_DIR) $(WRC_BASE)$(SEP)port$(SEP)mkstemps.c
	$(ECHO_CC)
	${host_gcc} $(WRC_HOST_CFLAGS) -c $(WRC_BASE)$(SEP)port$(SEP)mkstemps.c -o $(INTERMEDIATE)$(WRC_BASE)$(SEP)port$(SEP)mkstemps.o

.PHONY: wrc_clean
wrc_clean:
	-@$(rm) $(WRC_TARGET) $(WRC_OBJECTS) 2>$(NUL)
clean: wrc_clean
