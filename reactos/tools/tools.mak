TOOLS_BASE = tools
TOOLS_BASE_ = $(TOOLS_BASE)$(SEP)
TOOLS_INT = $(INTERMEDIATE_)$(TOOLS_BASE)
TOOLS_INT_ = $(TOOLS_INT)$(SEP)
TOOLS_OUT = $(OUTPUT_)$(TOOLS_BASE)
TOOLS_OUT_ = $(TOOLS_OUT)$(SEP)

TOOLS_CFLAGS = -Wall -Wpointer-arith -Wno-strict-aliasing -D__REACTOS__ $(HOST_CFLAGS)
TOOLS_CPPFLAGS = -Wall -Wpointer-arith -D__REACTOS__ $(HOST_CPPFLAGS)
TOOLS_LFLAGS = $(HOST_LFLAGS)

$(TOOLS_INT): | $(INTERMEDIATE)
	$(ECHO_MKDIR)
	${mkdir} $@

ifneq ($(INTERMEDIATE),$(OUTPUT))
$(TOOLS_OUT): | $(OUTPUT)
	$(ECHO_MKDIR)
	${mkdir} $@
endif

XML_SSPRINTF_SOURCES = $(addprefix $(TOOLS_BASE_), \
	ssprintf.cpp \
	xml.cpp \
	)

XML_SSPRINTF_HEADERS = $(addprefix $(TOOLS_BASE_), \
	ssprintf.h \
	xml.h \
	)

XML_SSPRINTF_OBJECTS = \
	$(addprefix $(INTERMEDIATE_), $(XML_SSPRINTF_SOURCES:.cpp=.o))

$(TOOLS_INT_)ssprintf.o: $(TOOLS_BASE_)ssprintf.cpp $(XML_SSPRINTF_HEADERS) | $(TOOLS_INT)
	$(ECHO_HOSTCC)
	${host_gpp} $(TOOLS_CPPFLAGS) -c $< -o $@

$(TOOLS_INT_)xml.o: $(TOOLS_BASE_)xml.cpp $(XML_SSPRINTF_HEADERS) | $(TOOLS_INT)
	$(ECHO_HOSTCC)
	${host_gpp} $(TOOLS_CPPFLAGS) -c $< -o $@

include tools/bin2c.mak
include tools/buildno/buildno.mak
include tools/gendib/gendib.mak
include tools/log2lines.mak
include tools/nci/nci.mak
ifeq ($(ARCH),powerpc)
include tools/ofw_interface/ofw_interface.mak
endif
include tools/pefixup.mak
include tools/raddr2line.mak
include tools/rbuild/rbuild.mak
include tools/rsym.mak
