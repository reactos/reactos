WPP_BASE = $(TOOLS_BASE_)wpp
WPP_BASE_ = $(WPP_BASE)$(SEP)
WPP_INT = $(INTERMEDIATE_)$(WPP_BASE)
WPP_INT_ = $(WPP_INT)$(SEP)
WPP_OUT = $(OUTPUT_)$(WPP_BASE)
WPP_OUT_ = $(WPP_OUT)$(SEP)

$(WPP_INT): | $(TOOLS_INT)
	$(ECHO_MKDIR)
	${mkdir} $@

ifneq ($(INTERMEDIATE),$(OUTPUT))
$(WPP_OUT): | $(TOOLS_OUT)
	$(ECHO_MKDIR)
	${mkdir} $@
endif

WPP_TARGET = \
	$(WPP_OUT_)libwpp.a

WPP_SOURCES = $(addprefix $(WPP_BASE_), \
	lex.yy.c \
	preproc.c \
	wpp.c \
	ppy.tab.c \
	)

WPP_OBJECTS = \
    $(addprefix $(INTERMEDIATE_), $(WPP_SOURCES:.c=.o))

WPP_HOST_CFLAGS = -D__USE_W32API -I$(WPP_BASE) -Iinclude/reactos/wine -Iinclude/reactos -Iinclude -Iinclude/psdk $(TOOLS_CFLAGS)

.PHONY: wpp
wpp: $(WPP_TARGET)

$(WPP_TARGET): $(WPP_OBJECTS) | $(WPP_OUT)
	$(ECHO_AR)
	${host_ar} -rc $(WPP_TARGET) $(WPP_OBJECTS)

$(WPP_INT_)lex.yy.o: $(WPP_BASE_)lex.yy.c | $(WPP_INT)
	$(ECHO_CC)
	${host_gcc} $(WPP_HOST_CFLAGS) -c $< -o $@

$(WPP_INT_)preproc.o: $(WPP_BASE_)preproc.c | $(WPP_INT)
	$(ECHO_CC)
	${host_gcc} $(WPP_HOST_CFLAGS) -c $< -o $@

$(WPP_INT_)wpp.o: $(WPP_BASE_)wpp.c | $(WPP_INT)
	$(ECHO_CC)
	${host_gcc} $(WPP_HOST_CFLAGS) -c $< -o $@

$(WPP_INT_)ppy.tab.o: $(WPP_BASE_)ppy.tab.c | $(WPP_INT)
	$(ECHO_CC)
	${host_gcc} $(WPP_HOST_CFLAGS) -c $< -o $@

#
#$(WPP_BASE_)ppy.tab.c: $(WPP_BASE_)ppy.y
#	bison -p ppy_ -d -o tools\wpp\ppy.tab.c tools\wpp\ppy.y
#

#
#$(WPP_BASE_)lex.yy.c: $(WPP_BASE_)ppl.l
#	flex -otools\wpp\lex.yy.c tools\wpp\ppl.l
#

.PHONY: wpp_clean
wpp_clean:
	-@$(rm) $(WPP_TARGET) $(WPP_OBJECTS) 2>$(NUL)
clean: wpp_clean
