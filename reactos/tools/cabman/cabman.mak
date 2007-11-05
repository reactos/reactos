CABMAN_BASE = $(TOOLS_BASE_)cabman
CABMAN_BASE_ = $(CABMAN_BASE)$(SEP)
CABMAN_INT = $(INTERMEDIATE_)$(CABMAN_BASE)
CABMAN_INT_ = $(CABMAN_INT)$(SEP)
CABMAN_OUT = $(OUTPUT_)$(CABMAN_BASE)
CABMAN_OUT_ = $(CABMAN_OUT)$(SEP)

$(CABMAN_INT): | $(TOOLS_INT)
	$(ECHO_MKDIR)
	${mkdir} $@

ifneq ($(INTERMEDIATE),$(OUTPUT))
$(CABMAN_OUT): | $(TOOLS_OUT)
	$(ECHO_MKDIR)
	${mkdir} $@
endif

CABMAN_TARGET = \
	$(EXEPREFIX)$(CABMAN_OUT_)cabman$(EXEPOSTFIX)

CABMAN_SOURCES = $(addprefix $(CABMAN_BASE_), \
	cabinet.cxx \
	dfp.cxx \
	main.cxx \
	mszip.cxx \
	raw.cxx \
	)

CABMAN_OBJECTS = \
  $(addprefix $(INTERMEDIATE_), $(CABMAN_SOURCES:.cxx=.o))

CABMAN_HOST_CXXFLAGS = -Iinclude -Iinclude/reactos -Ilib/3rdparty/zlib $(TOOLS_CPPFLAGS)

CABMAN_HOST_LIBS = $(ZLIB_HOST_TARGET)

CABMAN_HOST_LFLAGS = $(TOOLS_LFLAGS) $(CABMAN_HOST_LIBS)

.PHONY: cabman
cabman: $(CABMAN_TARGET)

$(CABMAN_TARGET): $(CABMAN_OBJECTS) $(CABMAN_HOST_LIBS) | $(CABMAN_OUT)
	$(ECHO_LD)
	${host_gpp} $(CABMAN_OBJECTS) $(CABMAN_HOST_LFLAGS) -o $@

$(CABMAN_INT_)cabinet.o: $(CABMAN_BASE_)cabinet.cxx | $(CABMAN_INT)
	$(ECHO_CC)
	${host_gpp} $(CABMAN_HOST_CXXFLAGS) -c $< -o $@

$(CABMAN_INT_)dfp.o: $(CABMAN_BASE_)dfp.cxx | $(CABMAN_INT)
	$(ECHO_CC)
	${host_gpp} $(CABMAN_HOST_CXXFLAGS) -c $< -o $@

$(CABMAN_INT_)main.o: $(CABMAN_BASE_)main.cxx | $(CABMAN_INT)
	$(ECHO_CC)
	${host_gpp} $(CABMAN_HOST_CXXFLAGS) -c $< -o $@

$(CABMAN_INT_)mszip.o: $(CABMAN_BASE_)mszip.cxx | $(CABMAN_INT)
	$(ECHO_CC)
	${host_gpp} $(CABMAN_HOST_CXXFLAGS) -c $< -o $@

$(CABMAN_INT_)raw.o: $(CABMAN_BASE_)raw.cxx | $(CABMAN_INT)
	$(ECHO_CC)
	${host_gpp} $(CABMAN_HOST_CXXFLAGS) -c $< -o $@

.PHONY: cabman_clean
cabman_clean:
	-@$(rm) $(CABMAN_TARGET) $(CABMAN_OBJECTS) 2>$(NUL)
clean: cabman_clean
