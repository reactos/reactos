WMC_BASE = $(TOOLS_BASE)$(SEP)wmc

WMC_BASE_DIR = $(INTERMEDIATE)$(WMC_BASE)
WMC_BASE_DIR_EXISTS = $(WMC_BASE_DIR)$(SEP)$(EXISTS)

$(WMC_BASE_DIR_EXISTS): $(TOOLS_BASE_DIR_EXISTS)
	$(ECHO_MKDIR)
	${mkdir} $(WMC_BASE_DIR)
	@echo . >$@

WMC_TARGET = \
	$(INTERMEDIATE)$(WMC_BASE)$(SEP)wmc$(EXEPOSTFIX)

WMC_SOURCES = $(addprefix $(WMC_BASE)$(SEP), \
	getopt.c \
	lang.c \
	mcl.c \
	utils.c \
	wmc.c \
	write.c \
	y_tab.c \
	misc.c \
	)

WMC_OBJECTS = \
  $(addprefix $(INTERMEDIATE), $(WMC_SOURCES:.c=.o))

WMC_HOST_CXXFLAGS = -I$(WMC_BASE) -g -Werror -Wall

WMC_HOST_LFLAGS = -g

.PHONY: wmc
wmc: $(WMC_TARGET)

$(WMC_TARGET): $(WMC_OBJECTS)
	$(ECHO_LD)
	${host_gcc} $(WMC_OBJECTS) $(WMC_HOST_LFLAGS) -o $@

$(WMC_BASE_DIR)$(SEP)getopt.o: $(WMC_BASE)$(SEP)getopt.c $(WMC_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WMC_HOST_CXXFLAGS) -c $< -o $@

$(WMC_BASE_DIR)$(SEP)lang.o: $(WMC_BASE)$(SEP)lang.c $(WMC_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WMC_HOST_CXXFLAGS) -c $< -o $@

$(WMC_BASE_DIR)$(SEP)mcl.o: $(WMC_BASE)$(SEP)mcl.c $(WMC_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WMC_HOST_CXXFLAGS) -c $< -o $@

$(WMC_BASE_DIR)$(SEP)utils.o: $(WMC_BASE)$(SEP)utils.c $(WMC_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WMC_HOST_CXXFLAGS) -c $< -o $@

$(WMC_BASE_DIR)$(SEP)wmc.o: $(WMC_BASE)$(SEP)wmc.c $(WMC_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WMC_HOST_CXXFLAGS) -c $< -o $@

$(WMC_BASE_DIR)$(SEP)write.o: $(WMC_BASE)$(SEP)write.c $(WMC_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WMC_HOST_CXXFLAGS) -c $< -o $@

$(WMC_BASE_DIR)$(SEP)y_tab.o: $(WMC_BASE)$(SEP)y_tab.c $(WMC_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WMC_HOST_CXXFLAGS) -c $< -o $@

$(WMC_BASE_DIR)$(SEP)misc.o: $(WMC_BASE)$(SEP)misc.c $(WMC_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WMC_HOST_CXXFLAGS) -c $< -o $@

.PHONY: wmc_clean
wmc_clean:
	-@$(rm) $(WMC_TARGET) $(WMC_OBJECTS) 2>$(NUL)
clean: wmc_clean
