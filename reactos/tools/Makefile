RMKDIR_BASE = tools

RMKDIR_TARGET = \
	$(ROS_INTERMEDIATE)$(RMKDIR_BASE)$(SEP)rmkdir$(EXEPOSTFIX)

RMKDIR_SOURCES = \
	$(RMKDIR_BASE)$(SEP)rmkdir.c

RMKDIR_OBJECTS = \
	$(RMKDIR_SOURCES:.c=.o)

RMKDIR_HOST_CFLAGS = -g -Werror -Wall

RMKDIR_HOST_LFLAGS = -g

$(RMKDIR_TARGET): $(RMKDIR_OBJECTS)
	${host_gcc} $(RMKDIR_OBJECTS) $(RMKDIR_HOST_LFLAGS) -o $(RMKDIR_TARGET)

$(RMKDIR_OBJECTS): %.o : %.c
	${host_gcc} $(RMKDIR_HOST_CFLAGS) -c $< -o $@

.PHONY: rmkdir_clean
rmkdir_clean:
	-@$(rm) $(RMKDIR_TARGET) $(RMKDIR_OBJECTS) 2>$(NUL)
clean: rmkdir_clean


RSYM_BASE = tools

RSYM_TARGET = \
	$(ROS_INTERMEDIATE)$(RSYM_BASE)$(SEP)rsym$(EXEPOSTFIX)

RSYM_SOURCES = \
	$(RSYM_BASE)$(SEP)rsym.c

RSYM_OBJECTS = \
	$(RSYM_SOURCES:.c=.o)

RSYM_HOST_CFLAGS = -g -Werror -Wall

RSYM_HOST_LFLAGS = -g

$(RSYM_TARGET): $(RSYM_OBJECTS)
	${host_gcc} $(RSYM_OBJECTS) $(RSYM_HOST_LFLAGS) -o $(RSYM_TARGET)

$(RSYM_OBJECTS): %.o : %.c
	${host_gcc} $(RSYM_HOST_CFLAGS) -c $< -o $@

.PHONY: rsym_clean
rsym_clean:
	-@$(rm) $(RSYM_TARGET) $(RSYM_OBJECTS) 2>$(NUL)
clean: rsym_clean

include tools/buildno/makefile
include tools/cdmake/makefile
include tools/nci/makefile
include tools/rbuild/makefile
include tools/unicode/makefile
include tools/wmc/makefile
include tools/wpp/makefile
include tools/wrc/makefile
