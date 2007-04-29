MKHIVE_BASE = $(TOOLS_BASE_)mkhive
MKHIVE_BASE_ = $(MKHIVE_BASE)$(SEP)
MKHIVE_INT = $(INTERMEDIATE_)$(MKHIVE_BASE)
MKHIVE_INT_ = $(MKHIVE_INT)$(SEP)
MKHIVE_OUT = $(OUTPUT_)$(MKHIVE_BASE)
MKHIVE_OUT_ = $(MKHIVE_OUT)$(SEP)

$(MKHIVE_INT): | $(TOOLS_INT)
	$(ECHO_MKDIR)
	${mkdir} $@

ifneq ($(INTERMEDIATE),$(OUTPUT))
$(MKHIVE_OUT): | $(TOOLS_OUT)
	$(ECHO_MKDIR)
	${mkdir} $@
endif

MKHIVE_TARGET = \
	$(EXEPREFIX)$(MKHIVE_OUT_)mkhive$(EXEPOSTFIX)

MKHIVE_SOURCES = $(addprefix $(MKHIVE_BASE_), \
	binhive.cpp \
	mkhive.cpp \
	reginf.cpp \
	registry.cpp \
	)

MKHIVE_OBJECTS = \
	$(addprefix $(INTERMEDIATE_), $(MKHIVE_SOURCES:.cpp=.o))

MKHIVE_HOST_CXXFLAGS = $(xTOOLS_CFLAGS) -I$(INFLIB_BASE) -g3

MKHIVE_HOST_LFLAGS = $(xTOOLS_LFLAGS) -g3 -lstdc++

.PHONY: mkhive
mkhive: $(MKHIVE_TARGET)

$(MKHIVE_TARGET): $(MKHIVE_OBJECTS) $(INFLIB_HOST_OBJECTS) | $(MKHIVE_OUT)
	$(ECHO_LD)
	${host_gcc} $(MKHIVE_OBJECTS) $(INFLIB_HOST_OBJECTS) $(MKHIVE_HOST_LFLAGS) -o $@

$(MKHIVE_INT_)binhive.o: $(MKHIVE_BASE_)binhive.cpp | $(MKHIVE_INT)
	$(ECHO_CC)
	${host_gpp} $(MKHIVE_HOST_CXXFLAGS) -c $< -o $@

$(MKHIVE_INT_)mkhive.o: $(MKHIVE_BASE_)mkhive.cpp | $(MKHIVE_INT)
	$(ECHO_CC)
	${host_gpp} $(MKHIVE_HOST_CXXFLAGS) -c $< -o $@

$(MKHIVE_INT_)reginf.o: $(MKHIVE_BASE_)reginf.cpp | $(MKHIVE_INT)
	$(ECHO_CC)
	${host_gpp} $(MKHIVE_HOST_CXXFLAGS) -c $< -o $@

$(MKHIVE_INT_)registry.o: $(MKHIVE_BASE_)registry.cpp | $(MKHIVE_INT)
	$(ECHO_CC)
	${host_gpp} $(MKHIVE_HOST_CXXFLAGS) -c $< -o $@

.PHONY: mkhive_clean
mkhive_clean:
	-@$(rm) $(MKHIVE_TARGET) $(MKHIVE_OBJECTS) 2>$(NUL)
clean: mkhive_clean
