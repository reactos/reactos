RMKDIR_BASE = tools

RMKDIR_BASE_DIR = $(INTERMEDIATE)
RMKDIR_BASE_DIR_EXISTS = $(RMKDIR_BASE_DIR)$(EXISTS)

RMKDIR_TARGET = \
	$(RMKDIR_BASE_DIR)rmkdir$(EXEPOSTFIX)

RMKDIR_SOURCES = \
	$(RMKDIR_BASE)$(SEP)rmkdir.c

RMKDIR_OBJECTS = \
	$(RMKDIR_BASE_DIR)rmkdir.o

RMKDIR_HOST_CFLAGS = -g -Werror -Wall

RMKDIR_HOST_LFLAGS = -g

.PHONY: rmkdir
rmkdir: $(RMKDIR_TARGET)

$(RMKDIR_TARGET): $(RMKDIR_OBJECTS)
	$(ECHO_LD)
	${host_gcc} $(RMKDIR_OBJECTS) $(RMKDIR_HOST_LFLAGS) -o $@

$(RMKDIR_BASE_DIR)rmkdir.o: $(RMKDIR_BASE)$(SEP)rmkdir.c $(RMKDIR_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(RMKDIR_HOST_CFLAGS) -c $< -o $@

.PHONY: rmkdir_clean
rmkdir_clean:
	-@$(rm) $(RMKDIR_TARGET) $(RMKDIR_OBJECTS) 2>$(NUL)
clean: rmkdir_clean


TOOLS_BASE = tools
TOOLS_BASE_DIR = $(INTERMEDIATE)$(TOOLS_BASE)
TOOLS_BASE_DIR_EXISTS = $(TOOLS_BASE_DIR)$(EXISTS)

$(TOOLS_BASE_DIR_EXISTS): $(INTERMEDIATE_EXISTS) $(RMKDIR_TARGET)
	${mkdir} $(TOOLS_BASE_DIR)
	@echo . >$@


RSYM_BASE = $(TOOLS_BASE)

RSYM_BASE_DIR = $(INTERMEDIATE)$(RSYM_BASE)
RSYM_BASE_DIR_EXISTS = $(RSYM_BASE_DIR)$(EXISTS)

RSYM_TARGET = \
	$(RSYM_BASE_DIR)$(SEP)rsym$(EXEPOSTFIX)

RSYM_SOURCES = \
	$(RSYM_BASE)$(SEP)rsym.c

RSYM_OBJECTS = \
	$(RSYM_SOURCES:.c=.o)

RSYM_HOST_CFLAGS = -g -Werror -Wall

RSYM_HOST_LFLAGS = -g

.PHONY: rsym
rsym: $(RSYM_TARGET)

$(RSYM_TARGET): $(RSYM_OBJECTS)
	$(ECHO_LD)
	${host_gcc} $(RSYM_OBJECTS) $(RSYM_HOST_LFLAGS) -o $@

$(RSYM_OBJECTS): %.o : %.c $(RSYM_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(RSYM_HOST_CFLAGS) -c $< -o $@

.PHONY: rsym_clean
rsym_clean:
	-@$(rm) $(RSYM_TARGET) $(RSYM_OBJECTS) 2>$(NUL)
clean: rsym_clean


include tools/bin2res/bin2res.mak
include tools/buildno/buildno.mak
include tools/cdmake/cdmake.mak
include tools/nci/nci.mak
include tools/rbuild/rbuild.mak
include tools/unicode/unicode.mak
include tools/winebuild/winebuild.mak
include tools/wmc/wmc.mak
include tools/wpp/wpp.mak
include tools/wrc/wrc.mak
include lib/zlib/zlib.mak

# cabman must come after zlib
include tools/cabman/cabman.mak
