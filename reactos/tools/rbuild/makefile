PATH_TO_TOP = ../..

TARGET = rbuild$(EXE_POSTFIX)

all: $(TARGET)

BASE_OBJECTS = \
	exception.o \
	module.o \
	project.o \
	ssprintf.o \
	xml.o

OBJECTS = $(BASE_OBJECTS) rbuild.o

TESTS = \
	tests/moduletest.o \
	tests/projecttest.o

TEST_OBJECTS = $(BASE_OBJECTS) $(TESTS) tests/alltests.o

HOST_CFLAGS = -g -I. -Werror -Wall

HOST_LFLAGS = -g

rbuild$(EXE_POSTFIX): $(OBJECTS)
	$(HOST_CXX) $(OBJECTS) $(HOST_LFLAGS) -o rbuild$(EXE_POSTFIX)

ifeq ($(HOST),mingw32-linux)
clean:
	-rm -f *.o
	-rm -f rbuild$(EXE_POSTFIX)
	-rm -f tests/*.o
	-rm -f rbuild_tests$(EXE_POSTFIX)
endif
ifeq ($(HOST),mingw32-windows)
clean:
	-del *.o
	-del rbuild$(EXE_POSTFIX)
	-del tests\*.o
	-del rbuild_tests$(EXE_POSTFIX)
endif

.phony: clean

%.o: %.cpp
	$(HALFVERBOSEECHO) [CXX]     $<
	$(HOST_CXX) $(HOST_CFLAGS) -c $< -o $@

test: rbuild_tests$(EXE_POSTFIX)
	$(EXE_PREFIX)rbuild_tests$(EXE_POSTFIX)

rbuild_tests$(EXE_POSTFIX): $(TEST_OBJECTS)
	$(HOST_CXX) $(TEST_OBJECTS) $(HOST_LFLAGS) -o rbuild_tests$(EXE_POSTFIX)

include $(PATH_TO_TOP)/rules.mak
