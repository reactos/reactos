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

include tools/buildno/Makefile
include tools/cdmake/makefile
include tools/nci/makefile
include tools/rbuild/makefile
include tools/unicode/Makefile
include tools/wmc/makefile
include tools/wpp/Makefile
include tools/wrc/makefile
