BUILDNO_BASE = tools$(SEP)buildno

BUILDNO_TARGET = \
	$(EXEPREFIX)$(BUILDNO_BASE)$(SEP)buildno$(EXEPOSTFIX)

BUILDNO_SOURCES = \
	$(BUILDNO_BASE)$(SEP)buildno.cpp \
	$(BUILDNO_BASE)$(SEP)exception.cpp \
	$(BUILDNO_BASE)$(SEP)ssprintf.cpp \
	$(BUILDNO_BASE)$(SEP)XML.cpp

BUILDNO_OBJECTS = \
	$(BUILDNO_SOURCES:.cpp=.o)

BUILDNO_HOST_CFLAGS = -Iinclude/reactos -g -Werror -Wall

BUILDNO_HOST_LFLAGS = -g

$(BUILDNO_TARGET): $(BUILDNO_OBJECTS)
	$(ECHO_LD)
	${host_gpp} $(BUILDNO_OBJECTS) $(BUILDNO_HOST_CFLAGS) -o $(BUILDNO_TARGET)

$(BUILDNO_OBJECTS): %.o : %.cpp include$(SEP)reactos$(SEP)version.h
	$(ECHO_CC)
	${host_gpp} $(BUILDNO_HOST_CFLAGS) -c $< -o $@

.PHONY: buildno_clean
buildno_clean:
	-@$(rm) $(BUILDNO_TARGET) $(BUILDNO_OBJECTS) 2>$(NUL)
clean: buildno_clean

# BUILDNO_H is defined from the top-level makefile now...
#BUILDNO_H = .$(SEP)include$(SEP)reactos$(SEP)buildno.h

.PHONY: buildno_h
buildno_h: $(BUILDNO_H)

$(BUILDNO_H): $(BUILDNO_TARGET)
	$(BUILDNO_TARGET) $(BUILDNO_H)

.PHONY: buildno_h_clean
buildno_h_clean:
	-@$(rm) $(BUILDNO_H)
clean: buildno_h_clean
