PATH_TO_TOP = ../..

TARGET = buildno$(EXE_POSTFIX)

all: $(TARGET)

OBJECTS = buildno.o XML.o exception.o ssprintf.o

CLEAN_FILES = *.o buildno$(EXE_POSTFIX)

HOST_CXXFLAGS = -I$(PATH_TO_TOP)/include/reactos -g -Wall -Werror

buildno.o: buildno.cpp $(PATH_TO_TOP)/include/reactos/version.h
	$(HOST_CXX) $(HOST_CXXFLAGS) -o buildno.o -c buildno.cpp

XML.o: XML.cpp
	$(HOST_CXX) $(HOST_CXXFLAGS) -o XML.o -c XML.cpp

exception.o: exception.cpp
	$(HOST_CXX) $(HOST_CXXFLAGS) -o exception.o -c exception.cpp

ssprintf.o: ssprintf.cpp
	$(HOST_CXX) $(HOST_CXXFLAGS) -o ssprintf.o -c ssprintf.cpp

buildno$(EXE_POSTFIX): $(OBJECTS)
	$(HOST_CXX) -g $(OBJECTS) -o $(TARGET)
	@$(EXE_PREFIX)buildno$(EXE_POSTFIX)

ifeq ($(HOST),mingw32-linux)
clean:
	-rm -f *.o $(TARGET)
	-rm $(PATH_TO_TOP)/include/reactos/buildno.h
endif
ifneq ($(HOST),mingw32-linux)
clean:
	-del *.o $(TARGET)
	-del ..\..\include\reactos\buildno.h
endif

.PHONY: clean

include $(PATH_TO_TOP)/rules.mak
