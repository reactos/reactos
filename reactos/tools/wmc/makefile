WMC_BASE = tools$(SEP)wmc

WMC_TARGET = \
	$(ROS_INTERMEDIATE)$(WMC_BASE)$(SEP)wmc$(EXEPOSTFIX)

WMC_SOURCES = \
	$(WMC_BASE)$(SEP)getopt.c \
	$(WMC_BASE)$(SEP)lang.c \
	$(WMC_BASE)$(SEP)mcl.c \
	$(WMC_BASE)$(SEP)utils.c \
	$(WMC_BASE)$(SEP)wmc.c \
	$(WMC_BASE)$(SEP)write.c \
	$(WMC_BASE)$(SEP)y_tab.c \
	$(WMC_BASE)$(SEP)misc.c

WMC_OBJECTS = \
	$(WMC_SOURCES:.c=.o)

WMC_HOST_CXXFLAGS = -I$(WMC_BASE) -g -Werror -Wall

WMC_HOST_LFLAGS = -g

$(WMC_TARGET): $(WMC_OBJECTS)
	$(HALFVERBOSEECHO) [LD]      $@
	${host_gcc} $(WMC_OBJECTS) $(WMC_HOST_LFLAGS) -o $(WMC_TARGET)

$(WMC_OBJECTS): %.o : %.c
	$(HALFVERBOSEECHO) [CC]      $<
	${host_gcc} $(WMC_HOST_CXXFLAGS) -c $< -o $@

.PHONY: wmc_clean
wmc_clean:
	-@$(rm) $(WMC_TARGET) $(WMC_OBJECTS) 2>$(NUL)
clean: wmc_clean
