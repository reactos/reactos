WRC_BASE = tools$(SEP)wrc

WRC_TARGET = \
	$(ROS_INTERMEDIATE)$(WRC_BASE)$(SEP)wrc$(EXEPOSTFIX)

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
	$(WRC_SOURCES:.c=.o)

WRC_HOST_CFLAGS = -I$(WRC_BASE) -g -Werror -Wall \
                  -D__USE_W32API -DWINE_UNICODE_API= \
                  -Dwchar_t="unsigned short" -D_WCHAR_T_DEFINED \
                  -I$(UNICODE_BASE) -I$(WPP_BASE) -I$(WRC_BASE) \
                  -Iinclude/wine -Iinclude -Iw32api/include

WRC_HOST_LFLAGS = -g

$(WRC_TARGET): $(WRC_OBJECTS) $(UNICODE_TARGET) $(WPP_TARGET)
	$(HALFVERBOSEECHO) [LD]      $@
	${host_gcc} $(WRC_OBJECTS) $(UNICODE_TARGET) $(WPP_TARGET) $(WRC_HOST_LFLAGS) -o $(WRC_TARGET)

$(WRC_OBJECTS): %.o : %.c
	$(HALFVERBOSEECHO) [CC]      $<
	${host_gcc} $(WRC_HOST_CFLAGS) -c $< -o $@

.PHONY: wrc_clean
wrc_clean:
	-@$(rm) $(WRC_TARGET) $(WRC_OBJECTS) 2>$(NUL)
clean: wrc_clean
