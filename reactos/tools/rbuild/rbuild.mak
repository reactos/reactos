RBUILD_BASE = $(TOOLS_BASE)$(SEP)rbuild

RBUILD_BASE_DIR = $(INTERMEDIATE)$(RBUILD_BASE)
RBUILD_BASE_DIR_EXISTS = $(RBUILD_BASE_DIR)$(SEP)$(EXISTS)

$(RBUILD_BASE_DIR_EXISTS): $(TOOLS_BASE_DIR_EXISTS)
	$(ECHO_MKDIR)
	${mkdir} $(RBUILD_BASE_DIR)
	@echo . >$@

RBUILD_TARGET = \
	$(RBUILD_BASE_DIR)$(SEP)rbuild$(EXEPOSTFIX)

RBUILD_TEST_TARGET = \
	$(RBUILD_BASE_DIR)$(SEP)rbuild_test$(EXEPOSTFIX)

RBUILD_BACKEND_MINGW_BASE_SOURCES = \
	backend$(SEP)mingw$(SEP)mingw.cpp \
	backend$(SEP)mingw$(SEP)modulehandler.cpp

RBUILD_BACKEND_DEVCPP_BASE_SOURCES = \
	backend$(SEP)devcpp$(SEP)devcpp.cpp

RBUILD_BACKEND_BASE_SOURCES = \
	$(RBUILD_BACKEND_MINGW_BASE_SOURCES) \
	$(RBUILD_BACKEND_DEVCPP_BASE_SOURCES) \
	backend$(SEP)backend.cpp

RBUILD_BASE_SOURCES = \
	$(RBUILD_BACKEND_BASE_SOURCES) \
	automaticdependency.cpp \
	bootstrap.cpp \
	cdfile.cpp \
	compilerflag.cpp \
	define.cpp \
	exception.cpp \
	include.cpp \
	linkerflag.cpp \
	module.cpp \
	project.cpp \
	ssprintf.cpp \
	XML.cpp

RBUILD_COMMON_SOURCES = \
	$(addprefix $(RBUILD_BASE)$(SEP), $(RBUILD_BASE_SOURCES)) \

RBUILD_SPECIAL_SOURCES = \
	$(RBUILD_BASE)$(SEP)rbuild.cpp

RBUILD_SOURCES = \
	$(RBUILD_COMMON_SOURCES) \
	$(RBUILD_SPECIAL_SOURCES)

RBUILD_COMMON_OBJECTS = \
	$(addprefix $(ROS_INTERMEDIATE), $(RBUILD_COMMON_SOURCES:.cpp=.o))

RBUILD_SPECIAL_OBJECTS = \
	$(addprefix $(ROS_INTERMEDIATE), $(RBUILD_SPECIAL_SOURCES:.cpp=.o))

RBUILD_OBJECTS = \
	$(RBUILD_COMMON_OBJECTS) \
	$(RBUILD_SPECIAL_OBJECTS)

RBUILD_TESTS = \
	tests$(SEP)definetest.cpp \
	tests$(SEP)functiontest.cpp \
	tests$(SEP)iftest.cpp \
	tests$(SEP)includetest.cpp \
	tests$(SEP)invoketest.cpp \
	tests$(SEP)linkerflagtest.cpp \
	tests$(SEP)moduletest.cpp \
	tests$(SEP)projecttest.cpp \
	tests$(SEP)sourcefiletest.cpp \
	tests$(SEP)cdfiletest.cpp

RBUILD_TEST_SPECIAL_SOURCES = \
	$(addprefix $(RBUILD_BASE)$(SEP), $(RBUILD_TESTS)) \
	$(RBUILD_BASE)$(SEP)tests$(SEP)alltests.cpp

RBUILD_TEST_SOURCES = \
	$(RBUILD_COMMON_SOURCES) \
	$(RBUILD_TEST_SPECIAL_SOURCES)

RBUILD_TEST_SPECIAL_OBJECTS = \
	$(addprefix $(ROS_INTERMEDIATE), $(RBUILD_TEST_SPECIAL_SOURCES:.cpp=.o))

RBUILD_TEST_OBJECTS = \
	$(RBUILD_COMMON_OBJECTS) \
	$(RBUILD_TEST_SPECIAL_OBJECTS)

RBUILD_HOST_CXXFLAGS = -g -I$(RBUILD_BASE) -Werror -Wall

RBUILD_HOST_LFLAGS = -g

.PHONY: rbuild
rbuild: $(RBUILD_TARGET)

$(RBUILD_TARGET): $(RBUILD_OBJECTS) $(RBUILD_BASE_DIR_EXISTS)
	$(ECHO_LD)
	${host_gpp} $(RBUILD_OBJECTS) $(RBUILD_HOST_LFLAGS) -o $(RBUILD_TARGET)

$(RBUILD_COMMON_OBJECTS): %.o: %.cpp
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_SPECIAL_OBJECTS): %.o: %.cpp
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@

$(RBUILD_TEST_TARGET): $(RBUILD_TEST_OBJECTS) $(RBUILD_BASE_DIR_EXISTS)
	$(ECHO_LD)
	${host_gpp} $(RBUILD_TEST_OBJECTS) $(RBUILD_HOST_LFLAGS) -o $(RBUILD_TEST_TARGET)

$(RBUILD_TEST_SPECIAL_OBJECTS): %.o: %.cpp
	$(ECHO_CC)
	${host_gpp} $(RBUILD_HOST_CXXFLAGS) -c $< -o $@



.PHONY: rbuild_test

rbuild_test: $(RBUILD_TEST_TARGET)
	$(ECHO_TEST)
	$(Q)$(RBUILD_TEST_TARGET)

.PHONY: rbuild_clean
rbuild_clean:
	-@$(rm) $(RBUILD_TARGET) $(RBUILD_OBJECTS) $(RBUILD_TEST_TARGET) $(RBUILD_TEST_OBJECTS) 2>$(NUL)
clean: rbuild_clean
