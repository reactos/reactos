RMKDIR_BASE = tools

RMKDIR_TARGET = \
	$(INTERMEDIATE)rmkdir$(EXEPOSTFIX)

RMKDIR_SOURCES = \
	$(RMKDIR_BASE)$(SEP)rmkdir.c

RMKDIR_OBJECTS = \
	$(INTERMEDIATE)rmkdir.o

RMKDIR_HOST_CFLAGS = -g -Werror -Wall

RMKDIR_HOST_LFLAGS = -g

$(RMKDIR_TARGET): $(INTERMEDIATE_NO_SLASH) $(RMKDIR_OBJECTS)
	$(ECHO_LD)
	${host_gcc} $(RMKDIR_OBJECTS) $(RMKDIR_HOST_LFLAGS) -o $(RMKDIR_TARGET)

$(INTERMEDIATE)rmkdir.o: $(INTERMEDIATE_NO_SLASH) $(RMKDIR_BASE)$(SEP)rmkdir.c
	$(ECHO_CC)
	${host_gcc} $(RMKDIR_HOST_CFLAGS) -c $(RMKDIR_BASE)$(SEP)rmkdir.c -o $(INTERMEDIATE)rmkdir.o

.PHONY: rmkdir_clean
rmkdir_clean:
	-@$(rm) $(RMKDIR_TARGET) $(RMKDIR_OBJECTS) 2>$(NUL)
clean: rmkdir_clean


RSYM_BASE = tools

RSYM_TARGET = \
	$(INTERMEDIATE)$(RSYM_BASE)$(SEP)rsym$(EXEPOSTFIX)

RSYM_SOURCES = \
	$(RSYM_BASE)$(SEP)rsym.c

RSYM_OBJECTS = \
	$(RSYM_SOURCES:.c=.o)

RSYM_HOST_CFLAGS = -g -Werror -Wall

RSYM_HOST_LFLAGS = -g

$(RSYM_TARGET): $(RSYM_OBJECTS)
	$(ECHO_LD)
	${host_gcc} $(RSYM_OBJECTS) $(RSYM_HOST_LFLAGS) -o $(RSYM_TARGET)

$(RSYM_OBJECTS): %.o : %.c
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
include lib/zlib/zlib.mak
