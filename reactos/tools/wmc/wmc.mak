WMC_BASE = tools$(SEP)wmc

WMC_BASE_DIR = $(INTERMEDIATE)$(WMC_BASE)$(SEP)$(CREATED)

$(WMC_BASE_DIR): $(RMKDIR_TARGET)
	${mkdir} $(INTERMEDIATE)$(WMC_BASE)

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
  $(addprefix $(INTERMEDIATE), $(WMC_SOURCES:.c=.o))

WMC_HOST_CXXFLAGS = -I$(WMC_BASE) -g -Werror -Wall

WMC_HOST_LFLAGS = -g

$(WMC_TARGET): $(WMC_BASE_DIR) $(WMC_OBJECTS)
	$(ECHO_LD)
	${host_gcc} $(WMC_OBJECTS) $(WMC_HOST_LFLAGS) -o $(WMC_TARGET)

$(INTERMEDIATE)$(WMC_BASE)$(SEP)getopt.o: $(WMC_BASE_DIR) $(WMC_BASE)$(SEP)getopt.c
	$(ECHO_CC)
	${host_gcc} $(WMC_HOST_CXXFLAGS) -c $(WMC_BASE)$(SEP)getopt.c -o $(INTERMEDIATE)$(WMC_BASE)$(SEP)getopt.o

$(INTERMEDIATE)$(WMC_BASE)$(SEP)lang.o: $(WMC_BASE_DIR) $(WMC_BASE)$(SEP)lang.c
	$(ECHO_CC)
	${host_gcc} $(WMC_HOST_CXXFLAGS) -c $(WMC_BASE)$(SEP)lang.c -o $(INTERMEDIATE)$(WMC_BASE)$(SEP)lang.o

$(INTERMEDIATE)$(WMC_BASE)$(SEP)mcl.o: $(WMC_BASE_DIR) $(WMC_BASE)$(SEP)mcl.c
	$(ECHO_CC)
	${host_gcc} $(WMC_HOST_CXXFLAGS) -c $(WMC_BASE)$(SEP)mcl.c -o $(INTERMEDIATE)$(WMC_BASE)$(SEP)mcl.o

$(INTERMEDIATE)$(WMC_BASE)$(SEP)utils.o: $(WMC_BASE_DIR) $(WMC_BASE)$(SEP)utils.c
	$(ECHO_CC)
	${host_gcc} $(WMC_HOST_CXXFLAGS) -c $(WMC_BASE)$(SEP)utils.c -o $(INTERMEDIATE)$(WMC_BASE)$(SEP)utils.o

$(INTERMEDIATE)$(WMC_BASE)$(SEP)wmc.o: $(WMC_BASE_DIR) $(WMC_BASE)$(SEP)wmc.c
	$(ECHO_CC)
	${host_gcc} $(WMC_HOST_CXXFLAGS) -c $(WMC_BASE)$(SEP)wmc.c -o $(INTERMEDIATE)$(WMC_BASE)$(SEP)wmc.o

$(INTERMEDIATE)$(WMC_BASE)$(SEP)write.o: $(WMC_BASE_DIR) $(WMC_BASE)$(SEP)write.c
	$(ECHO_CC)
	${host_gcc} $(WMC_HOST_CXXFLAGS) -c $(WMC_BASE)$(SEP)write.c -o $(INTERMEDIATE)$(WMC_BASE)$(SEP)write.o

$(INTERMEDIATE)$(WMC_BASE)$(SEP)y_tab.o: $(WMC_BASE_DIR) $(WMC_BASE)$(SEP)y_tab.c
	$(ECHO_CC)
	${host_gcc} $(WMC_HOST_CXXFLAGS) -c $(WMC_BASE)$(SEP)y_tab.c -o $(INTERMEDIATE)$(WMC_BASE)$(SEP)y_tab.o

$(INTERMEDIATE)$(WMC_BASE)$(SEP)misc.o: $(WMC_BASE_DIR) $(WMC_BASE)$(SEP)misc.c
	$(ECHO_CC)
	${host_gcc} $(WMC_HOST_CXXFLAGS) -c $(WMC_BASE)$(SEP)misc.c -o $(INTERMEDIATE)$(WMC_BASE)$(SEP)misc.o

.PHONY: wmc_clean
wmc_clean:
	-@$(rm) $(WMC_TARGET) $(WMC_OBJECTS) 2>$(NUL)
clean: wmc_clean
