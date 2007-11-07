SYSREGBUILD_BASE = $(TOOLS_BASE)$(SEP)sysreg
SYSREGBUILD_BASE_ = $(SYSREGBUILD_BASE)$(SEP)
SYSREGBUILD_INT = $(INTERMEDIATE_)$(SYSREGBUILD_BASE)
SYSREGBUILD_INT_ = $(SYSREGBUILD_INT)$(SEP)
SYSREGBUILD_OUT = $(OUTPUT_)$(SYSREGBUILD_BASE)
SYSREGBUILD_OUT_ = $(SYSREGBUILD_OUT)$(SEP)

$(SYSREGBUILD_INT): | $(TOOLS_INT)
	$(ECHO_MKDIR)
	${mkdir} $@

ifneq ($(INTERMEDIATE),$(OUTPUT))
$(SYSREGBUILD_OUT): | $(TOOLS_OUT)
	$(ECHO_MKDIR)
	${mkdir} $@
endif

SYSREGBUILD_TARGET = \
	$(SYSREGBUILD_OUT_)sysreg$(EXEPOSTFIX)

SYSREGBUILD_SOURCES = $(addprefix $(SYSREGBUILD_BASE_),\
	conf_parser.cpp \
	env_var.cpp \
	rosboot_test.cpp \
	namedpipe_reader.cpp \
	sysreg.cpp \
	file_reader.cpp \
	os_support.cpp \
	)

SYSREGBUILD_OBJECTS = \
  $(addprefix $(INTERMEDIATE_), $(SYSREGBUILD_SOURCES:.cpp=.o))


ifeq ($(HOST),mingw32-linux)
SYSREGBUILD_HOST_CFLAGS = $(TOOLS_CPPFLAGS) -D__LINUX__ -Wall
else
SYSREGBUILD_HOST_CFLAGS = $(TOOLS_CPPFLAGS) -D__USE_W32API -Iinclude -Iinclude/reactos -Iinclude/psdk -Iinclude$(SEP)crt -Iinclude/reactos/libs -I$(INTERMEDIATE_)$(SEP)include$(SEP)psdk -Wall
endif

SYSREGBUILD_HOST_LFLAGS = $(TOOLS_LFLAGS)

.PHONY: sysreg
sysreg: $(SYSREGBUILD_TARGET)
host_gpp += -g

$(SYSREGBUILD_TARGET): $(SYSREGBUILD_OBJECTS) | $(SYSREGBUILD_OUT)
	$(ECHO_LD)
	${host_gpp} $(SYSREGBUILD_OBJECTS) $(SYSREGBUILD_HOST_LFLAGS) -o $@

$(SYSREGBUILD_INT_)conf_parser.o: $(SYSREGBUILD_BASE_)conf_parser.cpp | $(SYSREGBUILD_INT)
	$(ECHO_CC)
	${host_gpp} $(SYSREGBUILD_HOST_CFLAGS) -c $< -o $@

$(SYSREGBUILD_INT_)env_var.o: $(SYSREGBUILD_BASE_)env_var.cpp | $(SYSREGBUILD_INT)
	$(ECHO_CC)
	${host_gpp} $(SYSREGBUILD_HOST_CFLAGS) -c $< -o $@

$(SYSREGBUILD_INT_)pipe_reader.o: $(SYSREGBUILD_BASE_)pipe_reader.cpp | $(SYSREGBUILD_INT)
	$(ECHO_CC)
	${host_gpp} $(SYSREGBUILD_HOST_CFLAGS) -c $< -o $@
	
$(SYSREGBUILD_INT_)namedpipe_reader.o: $(SYSREGBUILD_BASE_)namedpipe_reader.cpp | $(SYSREGBUILD_INT)
	$(ECHO_CC)
	${host_gpp} $(SYSREGBUILD_HOST_CFLAGS) -c $< -o $@

$(SYSREGBUILD_INT_)rosboot_test.o: $(SYSREGBUILD_BASE_)rosboot_test.cpp | $(SYSREGBUILD_INT)
	$(ECHO_CC)
	${host_gpp} $(SYSREGBUILD_HOST_CFLAGS) -c $< -o $@

$(SYSREGBUILD_INT_)sym_file.o: $(SYSREGBUILD_BASE_)sym_file.cpp | $(SYSREGBUILD_INT)
	$(ECHO_CC)
	${host_gpp} $(SYSREGBUILD_HOST_CFLAGS) -c $< -o $@

$(SYSREGBUILD_INT_)sysreg.o: $(SYSREGBUILD_BASE_)sysreg.cpp | $(SYSREGBUILD_INT)
	$(ECHO_CC)
	${host_gpp} $(SYSREGBUILD_HOST_CFLAGS) -c $< -o $@

$(SYSREGBUILD_INT_)file_reader.o: $(SYSREGBUILD_BASE_)file_reader.cpp | $(SYSREGBUILD_INT)
	$(ECHO_CC)
	${host_gpp} $(SYSREGBUILD_HOST_CFLAGS) -c $< -o $@

$(SYSREGBUILD_INT_)os_support.o: $(SYSREGBUILD_BASE_)os_support.cpp | $(SYSREGBUILD_INT)
	$(ECHO_CC)
	${host_gpp} $(SYSREGBUILD_HOST_CFLAGS) -c $< -o $@

.PHONY: sysregbuild_clean
sysreg_clean:
	-@$(rm) $(SYSREGBUILD_TARGET) $(SYSREGBUILD_OBJECTS) 2>$(NUL)
clean: sysreg_clean
