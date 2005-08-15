# Copyright (C) 2005 Casper S. Hornstrup
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

RBUILD_BASE = $(TOOLS_BASE_)rbuild
RBUILD_BASE_ = $(RBUILD_BASE)$(SEP)
RBUILD_INT = $(INTERMEDIATE_)$(RBUILD_BASE)
RBUILD_INT_ = $(RBUILD_INT)$(SEP)
RBUILD_OUT = $(OUTPUT_)$(RBUILD_BASE)
RBUILD_OUT_ = $(RBUILD_OUT)$(SEP)

$(RBUILD_INT): | $(TOOLS_INT)
	$(ECHO_MKDIR)
	${mkdir} $@

ifneq ($(INTERMEDIATE),$(OUTPUT))
$(RBUILD_OUT): | $(TOOLS_OUT)
	$(ECHO_MKDIR)
	${mkdir} $@
endif


RBUILD_BACKEND_BASE = $(RBUILD_BASE_)backend
RBUILD_BACKEND_BASE_ = $(RBUILD_BACKEND_BASE)$(SEP)
RBUILD_BACKEND_INT = $(INTERMEDIATE_)$(RBUILD_BACKEND_BASE)
RBUILD_BACKEND_INT_ = $(RBUILD_BACKEND_INT)$(SEP)
RBUILD_BACKEND_OUT = $(OUTPUT_)$(RBUILD_BACKEND_BASE)
RBUILD_BACKEND_OUT_ = $(RBUILD_BACKEND_OUT)$(SEP)

$(RBUILD_BACKEND_INT): | $(RBUILD_INT)
	$(ECHO_MKDIR)
	${mkdir} $@

ifneq ($(INTERMEDIATE),$(OUTPUT))
$(RBUILD_BACKEND_OUT): | $(RBUILD_OUT)
	$(ECHO_MKDIR)
	${mkdir} $@
endif


RBUILD_MINGW_BASE = $(RBUILD_BACKEND_BASE_)mingw
RBUILD_MINGW_BASE_ = $(RBUILD_MINGW_BASE)$(SEP)
RBUILD_MINGW_INT = $(INTERMEDIATE_)$(RBUILD_MINGW_BASE)
RBUILD_MINGW_INT_ = $(RBUILD_MINGW_INT)$(SEP)
RBUILD_MINGW_OUT = $(OUTPUT_)$(RBUILD_MINGW_BASE)
RBUILD_MINGW_OUT_ = $(RBUILD_MINGW_OUT)$(SEP)

$(RBUILD_MINGW_INT): | $(RBUILD_BACKEND_INT)
	$(ECHO_MKDIR)
	${mkdir} $@

ifneq ($(INTERMEDIATE),$(OUTPUT))
$(RBUILD_MINGW_OUT): | $(RBUILD_BACKEND_OUT)
	$(ECHO_MKDIR)
	${mkdir} $@
endif


RBUILD_TESTS_BASE = $(RBUILD_BASE_)tests
RBUILD_TESTS_BASE_ = $(RBUILD_TESTS_BASE)$(SEP)
RBUILD_TESTS_INT = $(INTERMEDIATE_)$(RBUILD_TESTS_BASE)
RBUILD_TESTS_INT_ = $(RBUILD_TESTS_INT)$(SEP)
RBUILD_TESTS_OUT = $(OUTPUT_)$(RBUILD_TESTS_BASE)

$(RBUILD_TESTS_INT): | $(RBUILD_INT)
	$(ECHO_MKDIR)
	${mkdir} $@

ifneq ($(INTERMEDIATE),$(OUTPUT))
$(RBUILD_TESTS_OUT): | $(RBUILD_OUT)
	$(ECHO_MKDIR)
	${mkdir} $@
endif


RBUILD_DEVCPP_BASE = $(RBUILD_BACKEND_BASE_)devcpp
RBUILD_DEVCPP_BASE_ = $(RBUILD_DEVCPP_BASE)$(SEP)
RBUILD_DEVCPP_INT = $(INTERMEDIATE_)$(RBUILD_DEVCPP_BASE)
RBUILD_DEVCPP_INT_ = $(RBUILD_DEVCPP_INT)$(SEP)
RBUILD_DEVCPP_OUT = $(OUTPUT_)$(RBUILD_DEVCPP_BASE)
RBUILD_DEVCPP_OUT_ = $(RBUILD_DEVCPP_OUT)$(SEP)

$(RBUILD_DEVCPP_INT): | $(RBUILD_BACKEND_INT)
	$(ECHO_MKDIR)
	${mkdir} $@

ifneq ($(INTERMEDIATE),$(OUTPUT))
$(RBUILD_DEVCPP_OUT): | $(RBUILD_BACKEND_OUT)
	$(ECHO_MKDIR)
	${mkdir} $@
endif


RBUILD_TARGET = \
	$(EXEPREFIX)$(RBUILD_OUT_)rbuild$(EXEPOSTFIX)

RBUILD_TEST_TARGET = \
	$(EXEPREFIX)$(RBUILD_OUT_)rbuild_test$(EXEPOSTFIX)

RBUILD_BACKEND_MINGW_BASE_SOURCES = $(addprefix $(RBUILD_MINGW_BASE_), \
	mingw.cpp \
	modulehandler.cpp \
	proxymakefile.cpp \
	)

RBUILD_BACKEND_DEVCPP_BASE_SOURCES = $(addprefix $(RBUILD_DEVCPP_BASE_), \
	devcpp.cpp \
	)

RBUILD_BACKEND_SOURCES = \
	$(RBUILD_BACKEND_MINGW_BASE_SOURCES) \
	$(RBUILD_BACKEND_DEVCPP_BASE_SOURCES) \
	$(RBUILD_BACKEND_BASE_)backend.cpp

RBUILD_COMMON_SOURCES = \
	$(RBUILD_BACKEND_SOURCES) \
	$(addprefix $(RBUILD_BASE_), \
		automaticdependency.cpp \
		bootstrap.cpp \
		cdfile.cpp \
		compilerflag.cpp \
		configuration.cpp \
		define.cpp \
		exception.cpp \
		filesupportcode.cpp \
		include.cpp \
		installfile.cpp \
		linkerflag.cpp \
		module.cpp \
		project.cpp \
		ssprintf.cpp \
		stubbedcomponent.cpp \
		testsupportcode.cpp \
		wineresource.cpp \
		XML.cpp \
		)

RBUILD_SPECIAL_SOURCES = \
	$(RBUILD_BASE_)rbuild.cpp

RBUILD_SOURCES = \
	$(RBUILD_COMMON_SOURCES) \
	$(RBUILD_SPECIAL_SOURCES)

RBUILD_COMMON_OBJECTS = \
	$(addprefix $(INTERMEDIATE_), $(RBUILD_COMMON_SOURCES:.cpp=.o))

RBUILD_SPECIAL_OBJECTS = \
	$(addprefix $(INTERMEDIATE_), $(RBUILD_SPECIAL_SOURCES:.cpp=.o))

RBUILD_OBJECTS = \
	$(RBUILD_COMMON_OBJECTS) \
	$(RBUILD_SPECIAL_OBJECTS)

RBUILD_BACKEND_DEVCPP_HEADERS = \
	devcpp.h

RBUILD_BACKEND_MINGW_HEADERS = \
	mingw.h \
	modulehandler.h

RBUILD_BACKEND_HEADERS = \
	backend.h \
	$(addprefix devcpp$(SEP), $(RBUILD_BACKEND_DEVCPP_HEADERS)) \
	$(addprefix mingw$(SEP), $(RBUILD_BACKEND_MINGW_HEADERS))

RBUILD_HEADERS = \
	$(addprefix $(RBUILD_BASE_), \
		exception.h \
		pch.h \
		rbuild.h \
		ssprintf.h \
		test.h \
		XML.h \
		$(addprefix backend$(SEP), $(RBUILD_BACKEND_HEADERS)) \
	)

RBUILD_TESTS = \
	tests$(SEP)cdfiletest.cpp \
	tests$(SEP)definetest.cpp \
	tests$(SEP)functiontest.cpp \
	tests$(SEP)iftest.cpp \
	tests$(SEP)includetest.cpp \
	tests$(SEP)invoketest.cpp \
	tests$(SEP)linkerflagtest.cpp \
	tests$(SEP)moduletest.cpp \
	tests$(SEP)projecttest.cpp \
	tests$(SEP)sourcefiletest.cpp \
	tests$(SEP)symboltest.cpp

RBUILD_TEST_SPECIAL_SOURCES = \
	$(addprefix $(RBUILD_BASE_), $(RBUILD_TESTS)) \
	$(RBUILD_BASE_)tests$(SEP)alltests.cpp

RBUILD_TEST_SOURCES = \
	$(RBUILD_COMMON_SOURCES) \
	$(RBUILD_TEST_SPECIAL_SOURCES)

RBUILD_TEST_SPECIAL_OBJECTS = \
	$(addprefix $(INTERMEDIATE_), $(RBUILD_TEST_SPECIAL_SOURCES:.cpp=.o))

RBUILD_TEST_OBJECTS = \
	$(RBUILD_COMMON_OBJECTS) \
	$(RBUILD_TEST_SPECIAL_OBJECTS)

RBUILD_HOST_CXXFLAGS = -I$(RBUILD_BASE) $(TOOLS_CPPFLAGS)

RBUILD_HOST_LFLAGS = $(TOOLS_LFLAGS)

.PHONY: rbuild
rbuild: $(RBUILD_TARGET)

$(RBUILD_TARGET): $(RBUILD_OBJECTS) | $(RBUILD_OUT)
	$(ECHO_LD)
	${host_gpp} $(RBUILD_OBJECTS) $(RBUILD_HOST_LFLAGS) -o $@

$(RBUILD_INT_)automaticdependency.o: $(RBUILD_BASE_)automaticdependency.cpp $(RBUILD_HEADERS) | $(RBUILD_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_INT_)bootstrap.o: $(RBUILD_BASE_)bootstrap.cpp $(RBUILD_HEADERS) | $(RBUILD_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_INT_)cdfile.o: $(RBUILD_BASE_)cdfile.cpp $(RBUILD_HEADERS) | $(RBUILD_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_INT_)compilerflag.o: $(RBUILD_BASE_)compilerflag.cpp $(RBUILD_HEADERS) | $(RBUILD_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_INT_)configuration.o: $(RBUILD_BASE_)configuration.cpp $(RBUILD_HEADERS) | $(RBUILD_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_INT_)define.o: $(RBUILD_BASE_)define.cpp $(RBUILD_HEADERS) | $(RBUILD_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_INT_)exception.o: $(RBUILD_BASE_)exception.cpp $(RBUILD_HEADERS) | $(RBUILD_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_INT_)filesupportcode.o: $(RBUILD_BASE_)filesupportcode.cpp $(RBUILD_HEADERS) | $(RBUILD_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_INT_)include.o: $(RBUILD_BASE_)include.cpp $(RBUILD_HEADERS) | $(RBUILD_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_INT_)installfile.o: $(RBUILD_BASE_)installfile.cpp $(RBUILD_HEADERS) | $(RBUILD_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_INT_)linkerflag.o: $(RBUILD_BASE_)linkerflag.cpp $(RBUILD_HEADERS) | $(RBUILD_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_INT_)module.o: $(RBUILD_BASE_)module.cpp $(RBUILD_HEADERS) | $(RBUILD_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_INT_)project.o: $(RBUILD_BASE_)project.cpp $(RBUILD_HEADERS) | $(RBUILD_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_INT_)rbuild.o: $(RBUILD_BASE_)rbuild.cpp $(RBUILD_HEADERS) | $(RBUILD_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_INT_)ssprintf.o: $(RBUILD_BASE_)ssprintf.cpp $(RBUILD_HEADERS) | $(RBUILD_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_INT_)stubbedcomponent.o: $(RBUILD_BASE_)stubbedcomponent.cpp $(RBUILD_HEADERS) | $(RBUILD_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_INT_)wineresource.o: $(RBUILD_BASE_)wineresource.cpp $(RBUILD_HEADERS) | $(RBUILD_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_INT_)testsupportcode.o: $(RBUILD_BASE_)testsupportcode.cpp $(RBUILD_HEADERS) | $(RBUILD_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_INT_)XML.o: $(RBUILD_BASE_)XML.cpp $(RBUILD_HEADERS) | $(RBUILD_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_BACKEND_INT_)backend.o: $(RBUILD_BACKEND_BASE_)backend.cpp $(RBUILD_HEADERS) | $(RBUILD_BACKEND_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_MINGW_INT_)mingw.o: $(RBUILD_MINGW_BASE_)mingw.cpp $(RBUILD_HEADERS) | $(RBUILD_MINGW_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_MINGW_INT_)modulehandler.o: $(RBUILD_MINGW_BASE_)modulehandler.cpp $(RBUILD_HEADERS) | $(RBUILD_MINGW_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_MINGW_INT_)proxymakefile.o: $(RBUILD_MINGW_BASE_)proxymakefile.cpp $(RBUILD_HEADERS) | $(RBUILD_MINGW_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_DEVCPP_INT_)devcpp.o: $(RBUILD_DEVCPP_BASE_)devcpp.cpp $(RBUILD_HEADERS) | $(RBUILD_DEVCPP_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_TEST_TARGET): $(RBUILD_TEST_OBJECTS) $(RBUILD_HEADERS) | $(RBUILD_OUT)
	$(ECHO_LD)
	${host_gpp} $(RBUILD_TEST_OBJECTS) $(RBUILD_HOST_LFLAGS) -o $@

$(RBUILD_TESTS_INT_)cdfiletest.o: $(RBUILD_TESTS_BASE_)cdfiletest.cpp $(RBUILD_HEADERS) | $(RBUILD_TESTS_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_TESTS_INT_)definetest.o: $(RBUILD_TESTS_BASE_)definetest.cpp $(RBUILD_HEADERS) | $(RBUILD_TESTS_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_TESTS_INT_)functiontest.o: $(RBUILD_TESTS_BASE_)functiontest.cpp $(RBUILD_HEADERS) | $(RBUILD_TESTS_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_TESTS_INT_)iftest.o: $(RBUILD_TESTS_BASE_)iftest.cpp $(RBUILD_HEADERS) | $(RBUILD_TESTS_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_TESTS_INT_)includetest.o: $(RBUILD_TESTS_BASE_)includetest.cpp $(RBUILD_HEADERS) | $(RBUILD_TESTS_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_TESTS_INT_)invoketest.o: $(RBUILD_TESTS_BASE_)invoketest.cpp $(RBUILD_HEADERS) | $(RBUILD_TESTS_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_TESTS_INT_)linkerflagtest.o: $(RBUILD_TESTS_BASE_)linkerflagtest.cpp $(RBUILD_HEADERS) | $(RBUILD_TESTS_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_TESTS_INT_)moduletest.o: $(RBUILD_TESTS_BASE_)moduletest.cpp $(RBUILD_HEADERS) | $(RBUILD_TESTS_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_TESTS_INT_)projecttest.o: $(RBUILD_TESTS_BASE_)projecttest.cpp $(RBUILD_HEADERS) | $(RBUILD_TESTS_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_TESTS_INT_)sourcefiletest.o: $(RBUILD_TESTS_BASE_)sourcefiletest.cpp $(RBUILD_HEADERS) | $(RBUILD_TESTS_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_TESTS_INT_)symboltest.o: $(RBUILD_TESTS_BASE_)symboltest.cpp $(RBUILD_HEADERS) | $(RBUILD_TESTS_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_TESTS_INT_)alltests.o: $(RBUILD_TESTS_BASE_)alltests.cpp $(RBUILD_HEADERS) | $(RBUILD_TESTS_INT)
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@


.PHONY: rbuild_test
rbuild_test: $(RBUILD_TEST_TARGET)
	$(ECHO_TEST)
	$(Q)$(RBUILD_TEST_TARGET)

.PHONY: rbuild_test_clean
rbuild_test_clean: $(RBUILD_TEST_TARGET) $(RBUILD_TESTS_INT)
	-@$(rm) $(RBUILD_TEST_TARGET) $(RBUILD_TEST_SPECIAL_OBJECTS) 2>$(NUL)

.PHONY: rbuild_clean
rbuild_clean: $(RBUILD_TARGET) $(RBUILD_OBJECTS) $(RBUILD_TESTS_INT)
	-@$(rm) $(RBUILD_TARGET) $(RBUILD_OBJECTS) 2>$(NUL)
clean: rbuild_clean
