CABMAN_BASE = tools$(SEP)cabman

CABMAN_BASE_DIR = $(INTERMEDIATE)$(CABMAN_BASE)

$(CABMAN_BASE_DIR): $(RMKDIR_TARGET)
	${mkdir} $(INTERMEDIATE)$(CABMAN_BASE)

CABMAN_TARGET = \
	$(CABMAN_BASE_DIR)$(SEP)cabman$(EXEPOSTFIX)

CABMAN_SOURCES = \
	$(CABMAN_BASE)$(SEP)cabinet.cxx \
	$(CABMAN_BASE)$(SEP)dfp.cxx \
	$(CABMAN_BASE)$(SEP)main.cxx \
	$(CABMAN_BASE)$(SEP)mszip.cxx \
	$(CABMAN_BASE)$(SEP)raw.cxx

CABMAN_OBJECTS = \
  $(addprefix $(INTERMEDIATE), $(CABMAN_SOURCES:.cxx=.o))

CABMAN_HOST_CFLAGS = -Iinclude/reactos -Ilib/zlib -g -Werror -Wall

CABMAN_HOST_LFLAGS = -g $(ZLIB_HOST_TARGET)

.PHONY: cabman
cabman: $(CABMAN_TARGET)

$(CABMAN_TARGET): $(CABMAN_OBJECTS) $(ZLIB_HOST_TARGET)
	$(ECHO_LD)
	${host_gpp} $(CABMAN_OBJECTS) $(CABMAN_HOST_LFLAGS) -o $@

$(CABMAN_BASE_DIR)$(SEP)cabinet.o: $(CABMAN_BASE)$(SEP)cabinet.cxx $(CABMAN_BASE_DIR)
	$(ECHO_CC)
	${host_gpp} $(CABMAN_HOST_CFLAGS) -c $< -o $@

$(CABMAN_BASE_DIR)$(SEP)dfp.o: $(CABMAN_BASE)$(SEP)dfp.cxx $(CABMAN_BASE_DIR)
	$(ECHO_CC)
	${host_gpp} $(CABMAN_HOST_CFLAGS) -c $< -o $@

$(CABMAN_BASE_DIR)$(SEP)main.o: $(CABMAN_BASE)$(SEP)main.cxx $(CABMAN_BASE_DIR)
	$(ECHO_CC)
	${host_gpp} $(CABMAN_HOST_CFLAGS) -c $< -o $@

$(CABMAN_BASE_DIR)$(SEP)mszip.o: $(CABMAN_BASE)$(SEP)mszip.cxx $(CABMAN_BASE_DIR)
	$(ECHO_CC)
	${host_gpp} $(CABMAN_HOST_CFLAGS) -c $< -o $@

$(CABMAN_BASE_DIR)$(SEP)raw.o: $(CABMAN_BASE)$(SEP)raw.cxx $(CABMAN_BASE_DIR)
	$(ECHO_CC)
	${host_gpp} $(CABMAN_HOST_CFLAGS) -c $< -o $@

.PHONY: cabman_clean
cabman_clean:
	-@$(rm) $(CABMAN_TARGET) $(CABMAN_OBJECTS) 2>$(NUL)
