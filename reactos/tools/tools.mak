TOOLS_BASE = tools
TOOLS_BASE_ = $(TOOLS_BASE)$(SEP)
TOOLS_INT = $(INTERMEDIATE_)$(TOOLS_BASE)
TOOLS_INT_ = $(TOOLS_INT)$(SEP)
TOOLS_OUT = $(OUTPUT_)$(TOOLS_BASE)
TOOLS_OUT_ = $(TOOLS_OUT)$(SEP)

$(TOOLS_INT): $(INTERMEDIATE)
	$(ECHO_MKDIR)
	${mkdir} $@

ifneq ($(INTERMEDIATE),$(OUTPUT))
$(TOOLS_OUT): $(OUTPUT)
	$(ECHO_MKDIR)
	${mkdir} $@
endif

RSYM_BASE = $(TOOLS_BASE)
RSYM_BASE_ = $(RSYM_BASE)$(SEP)

RSYM_INT = $(INTERMEDIATE_)$(RSYM_BASE)
RSYM_INT_ = $(RSYM_INT)$(SEP)
RSYM_OUT = $(OUTPUT_)$(RSYM_BASE)
RSYM_OUT_ = $(RSYM_OUT)$(SEP)

RSYM_TARGET = \
	$(EXEPREFIX)$(RSYM_OUT_)rsym$(EXEPOSTFIX)

RSYM_SOURCES = \
	$(RSYM_BASE_)rsym.c

RSYM_OBJECTS = \
	$(addprefix $(INTERMEDIATE_), $(RSYM_SOURCES:.c=.o))

RSYM_HOST_CFLAGS = -g -Werror -Wall

RSYM_HOST_LFLAGS = -g

.PHONY: rsym
rsym: $(RSYM_TARGET)

$(RSYM_TARGET): $(RSYM_OBJECTS) $(RSYM_OUT)
	$(ECHO_LD)
	${host_gcc} $(RSYM_OBJECTS) $(RSYM_HOST_LFLAGS) -o $@

$(RSYM_INT_)rsym.o: $(RSYM_BASE_)rsym.c $(RSYM_INT)
	$(ECHO_CC)
	${host_gcc} $(RSYM_HOST_CFLAGS) -c $< -o $@

.PHONY: rsym_clean
rsym_clean:
	-@$(rm) $(RSYM_TARGET) $(RSYM_OBJECTS) 2>$(NUL)
clean: rsym_clean


include tools/bin2res/bin2res.mak
include tools/buildno/buildno.mak
include tools/cabman/cabman.mak
include tools/cdmake/cdmake.mak
include tools/nci/nci.mak
include tools/rbuild/rbuild.mak
include tools/unicode/unicode.mak
include tools/winebuild/winebuild.mak
include tools/wmc/wmc.mak
include tools/wpp/wpp.mak
include tools/wrc/wrc.mak
