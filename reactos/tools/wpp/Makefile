WPP_BASE = tools$(SEP)wpp

WPP_TARGET = \
	$(ROS_INTERMEDIATE)$(WPP_BASE)$(SEP)libwpp.a

WPP_SOURCES = \
	$(WPP_BASE)$(SEP)lex.yy.c \
	$(WPP_BASE)$(SEP)preproc.c \
	$(WPP_BASE)$(SEP)wpp.c \
	$(WPP_BASE)$(SEP)wpp.tab.c

WPP_OBJECTS = \
	$(WPP_SOURCES:.c=.o)

WPP_HOST_CFLAGS = -D__USE_W32API -I$(WPP_BASE) -Iinclude -Iinclude/wine -g

$(WPP_TARGET): $(WPP_OBJECTS)
	${host_ar} -rc $(WPP_TARGET) $(WPP_OBJECTS)

$(WPP_OBJECTS): %.o : %.c
	${host_gcc} $(WPP_HOST_CFLAGS) -c $< -o $@

.PHONY: wpp_clean
wpp_clean:
	-@$(rm) $(WPP_TARGET) $(WPP_OBJECTS) 2>$(NUL)
clean: wpp_clean
