# Makefile - Proj_Listing5_1.dsp

ifndef CFG
CFG=Proj_Listing5_1 - Win32 Debug
endif
CC=gcc
CFLAGS=
CXX=g++
CXXFLAGS=$(CFLAGS)
RC=windres -O COFF
ifeq "$(CFG)"  "Proj_Listing5_1 - Win32 Release"
CFLAGS+=-fexceptions -O2 -DWIN32 -DNDEBUG -D_WINDOWS -D_MBCS -W
LD=$(CXX) $(CXXFLAGS)
LDFLAGS=
LDFLAGS+=-Wl,--subsystem,windows
LIBS+=-lkernel32 -luser32 -lgdi32
else
ifeq "$(CFG)"  "Proj_Listing5_1 - Win32 Debug"
CFLAGS+=-fexceptions -g -O0 -DWIN32 -D_DEBUG -D_WINDOWS -D_MBCS -W
LD=$(CXX) $(CXXFLAGS)
LDFLAGS=
LDFLAGS+=-Wl,--subsystem,windows
LIBS+=-lkernel32 -luser32 -lgdi32
endif
endif

ifndef TARGET
TARGET=primitives.exe
endif

.PHONY: all
all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

%.o: %.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ -c $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ -c $<

%.o: %.cxx
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ -c $<

%.res: %.rc
	$(RC) $(CPPFLAGS) -o $@ -i $<

SOURCE_FILES= \
	mk_font.cpp \
	primitives.cpp

HEADER_FILES=

RESOURCE_FILES=

SRCS=$(SOURCE_FILES) $(HEADER_FILES) $(RESOURCE_FILES) 

OBJS=$(patsubst %.rc,%.res,$(patsubst %.cxx,%.o,$(patsubst %.cpp,%.o,$(patsubst %.cc,%.o,$(patsubst %.c,%.o,$(filter %.c %.cc %.cpp %.cxx %.rc,$(SRCS)))))))

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

.PHONY: clean
clean:
	-del $(OBJS) $(TARGET)



