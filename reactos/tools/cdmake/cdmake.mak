CDMAKE_BASE = .$(SEP)tools$(SEP)cdmake

CDMAKE_TARGET = \
	$(ROS_INTERMEDIATE)$(CDMAKE_BASE)$(SEP)cdmake$(EXEPOSTFIX)

CDMAKE_SOURCES = \
	$(CDMAKE_BASE)$(SEP)cdmake.c \
	$(CDMAKE_BASE)$(SEP)llmosrt.c

CDMAKE_OBJECTS = \
	$(CDMAKE_SOURCES:.c=.o)

CDMAKE_HOST_CFLAGS = -Iinclude -g -Werror -Wall

CDMAKE_HOST_LFLAGS = -g

$(CDMAKE_TARGET): $(CDMAKE_OBJECTS)
	$(ECHO_LD)
	${host_gcc} $(CDMAKE_OBJECTS) $(CDMAKE_HOST_CFLAGS) -o $(CDMAKE_TARGET)

$(CDMAKE_OBJECTS): %.o : %.c
	$(ECHO_CC)
	${host_gcc} $(CDMAKE_CFLAGS) -c $< -o $@

.PHONY: cdmake_clean
cdmake_clean:
	-@$(rm) $(CDMAKE_TARGET) $(CDMAKE_OBJECTS) 2>$(NUL)
clean: cdmake_clean
