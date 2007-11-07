WMC_BASE = $(TOOLS_BASE_)wmc
WMC_BASE_ = $(WMC_BASE)$(SEP)
WMC_INT = $(INTERMEDIATE_)$(WMC_BASE)
WMC_INT_ = $(WMC_INT)$(SEP)
WMC_OUT = $(OUTPUT_)$(WMC_BASE)
WMC_OUT_ = $(WMC_OUT)$(SEP)

$(WMC_INT): | $(TOOLS_INT)
	$(ECHO_MKDIR)
	${mkdir} $@

ifneq ($(INTERMEDIATE),$(OUTPUT))
$(WMC_OUT): | $(TOOLS_OUT)
	$(ECHO_MKDIR)
	${mkdir} $@
endif

WMC_TARGET = \
	$(WMC_OUT_)wmc$(EXEPOSTFIX)

WMC_DEPENDS = $(BUILDNO_H)

WMC_SOURCES = $(addprefix $(WMC_BASE_), \
	lang.c \
	mcl.c \
	utils.c \
	wmc.c \
	write.c \
	mcy.tab.c \
	)

WMC_OBJECTS = \
  $(addprefix $(INTERMEDIATE_), $(WMC_SOURCES:.c=.o))

WMC_HOST_CFLAGS = -I$(WMC_BASE) $(TOOLS_CFLAGS) \
                  -D__USE_W32API -DWINE_UNICODE_API= -I$(UNICODE_BASE) \
                  -Iinclude/reactos/wine -Iinclude/reactos -Iinclude \
                  -I$(INTERMEDIATE_)/include

WMC_HOST_LFLAGS = $(TOOLS_LFLAGS)

WMC_LIBS = $(UNICODE_TARGET)

.PHONY: wmc
wmc: $(WMC_TARGET)

$(WMC_TARGET): $(WMC_OBJECTS) $(WMC_LIBS) | $(WMC_OUT)
	$(ECHO_LD)
	${host_gcc} $(WMC_OBJECTS) $(WMC_LIBS) $(WMC_HOST_LFLAGS) -o $@

$(WMC_INT_)lang.o: $(WMC_BASE_)lang.c $(WMC_DEPENDS) | $(WMC_INT)
	$(ECHO_CC)
	${host_gcc} $(WMC_HOST_CFLAGS) -c $< -o $@

$(WMC_INT_)mcl.o: $(WMC_BASE_)mcl.c $(WMC_DEPENDS) | $(WMC_INT)
	$(ECHO_CC)
	${host_gcc} $(WMC_HOST_CFLAGS) -c $< -o $@

$(WMC_INT_)utils.o: $(WMC_BASE_)utils.c $(WMC_DEPENDS) | $(WMC_INT)
	$(ECHO_CC)
	${host_gcc} $(WMC_HOST_CFLAGS) -c $< -o $@

$(WMC_INT_)wmc.o: $(WMC_BASE_)wmc.c $(WMC_DEPENDS) | $(WMC_INT)
	$(ECHO_CC)
	${host_gcc} $(WMC_HOST_CFLAGS) -c $< -o $@

$(WMC_INT_)write.o: $(WMC_BASE_)write.c $(WMC_DEPENDS) | $(WMC_INT)
	$(ECHO_CC)
	${host_gcc} $(WMC_HOST_CFLAGS) -c $< -o $@

$(WMC_INT_)mcy.tab.o: $(WMC_BASE_)mcy.tab.c $(WMC_DEPENDS) | $(WMC_INT)
	$(ECHO_CC)
	${host_gcc} $(WMC_HOST_CFLAGS) -c $< -o $@

.PHONY: wmc_clean
wmc_clean:
	-@$(rm) $(WMC_TARGET) $(WMC_OBJECTS) 2>$(NUL)
clean: wmc_clean
