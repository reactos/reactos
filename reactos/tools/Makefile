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



BUILDNO_BASE = tools

BUILDNO_TARGET = \
	$(ROS_INTERMEDIATE)$(BUILDNO_BASE)$(SEP)buildno$(EXEPOSTFIX)

BUILDNO_SOURCES = \
	$(BUILDNO_BASE)$(SEP)buildno.c

BUILDNO_OBJECTS = \
	$(BUILDNO_SOURCES:.c=.o)

BUILDNO_HOST_CFLAGS = -g -Werror -Wall

BUILDNO_HOST_LFLAGS = -g

$(BUILDNO_TARGET): $(BUILDNO_OBJECTS)
	${host_gcc} $(BUILDNO_OBJECTS) $(BUILDNO_HOST_LFLAGS) -o $(BUILDNO_TARGET)

$(BUILDNO_OBJECTS): %.o : %.c
	${host_gcc} $(BUILDNO_HOST_CFLAGS) -c $< -o $@

include$(SEP)reactos$(SEP)buildno.h: $(BUILDNO_TARGET)
	$(EXEPREFIX)$(BUILDNO_TARGET) include$(SEP)reactos$(SEP)buildno.h

include tools/nci/makefile
include tools/rbuild/makefile
include tools/wmc/makefile
