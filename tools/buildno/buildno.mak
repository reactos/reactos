BUILDNO_BASE = $(TOOLS_BASE_)buildno
BUILDNO_BASE_ = $(BUILDNO_BASE)$(SEP)
BUILDNO_INT = $(INTERMEDIATE_)$(BUILDNO_BASE)
BUILDNO_INT_ = $(BUILDNO_INT)$(SEP)
BUILDNO_OUT = $(OUTPUT_)$(BUILDNO_BASE)
BUILDNO_OUT_ = $(BUILDNO_OUT)$(SEP)

$(BUILDNO_INT): | $(TOOLS_INT)
	$(ECHO_MKDIR)
	${mkdir} $@

ifneq ($(INTERMEDIATE),$(OUTPUT))
$(BUILDNO_OUT): | $(TOOLS_OUT)
	$(ECHO_MKDIR)
	${mkdir} $@
endif

BUILDNO_TARGET = \
	$(BUILDNO_OUT_)buildno$(EXEPOSTFIX)

BUILDNO_SOURCES = $(addprefix $(BUILDNO_BASE_), \
	buildno.cpp \
	)

BUILDNO_OBJECTS = \
	$(addprefix $(INTERMEDIATE_), $(BUILDNO_SOURCES:.cpp=.o))

BUILDNO_HOST_CXXFLAGS = -I$(TOOLS_BASE) -Iinclude/reactos $(TOOLS_CPPFLAGS)

BUILDNO_HOST_LFLAGS = $(TOOLS_LFLAGS)

BUILDNO_VERSION = include$(SEP)reactos$(SEP)version.h

.PHONY: buildno
buildno: $(BUILDNO_TARGET)

$(BUILDNO_TARGET): $(BUILDNO_OBJECTS) $(XML_SSPRINTF_OBJECTS) | $(BUILDNO_OUT)
	$(ECHO_HOSTLD)
	${host_gpp} $^ $(BUILDNO_HOST_LFLAGS) -o $@

$(BUILDNO_INT_)buildno.o: $(BUILDNO_BASE_)buildno.cpp $(BUILDNO_VERSION) | $(BUILDNO_INT)
	$(ECHO_HOSTCC)
	${host_gpp} $(BUILDNO_HOST_CXXFLAGS) -c $< -o $@

.PHONY: buildno_clean
buildno_clean:
	-@$(rm) $(BUILDNO_TARGET) $(BUILDNO_OBJECTS) 2>$(NUL)
clean: buildno_clean

# Uncomment the following line if you want to automatically
# update build number after SVN update
#.PHONY: $(BUILDNO_TARGET)

$(BUILDNO_H): $(BUILDNO_TARGET)
	${mkdir} $(INTERMEDIATE_)include$(SEP)reactos 2>$(NUL)
	$(ECHO_BUILDNO)
	$(Q)$(BUILDNO_TARGET) $(BUILDNO_QUIET) $(BUILDNO_H)
