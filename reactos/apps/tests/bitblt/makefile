# Makefile - Proj_Listing1_5.dsp

ifndef CFG
CFG=Proj_Listing1_5 - Win32 Debug
endif
CC=gcc
CFLAGS=
CXX=g++
CXXFLAGS=$(CFLAGS)
RC=windres -O COFF
ifeq "$(CFG)"  "Proj_Listing1_5 - Win32 Release"
CFLAGS+=-fexceptions -O2 -DWIN32 -DNDEBUG -D_WINDOWS -D_MBCS -W
LD=$(CXX) $(CXXFLAGS)
LDFLAGS=
LDFLAGS+=-Wl,--subsystem,windows
LIBS+=-lkernel32 -luser32 -lgdi32
else
ifeq "$(CFG)"  "Proj_Listing1_5 - Win32 Debug"
CFLAGS+=-fexceptions -g -O0 -DWIN32 -D_DEBUG -D_WINDOWS -D_MBCS -W
LD=$(CXX) $(CXXFLAGS)
LDFLAGS=
LDFLAGS+=-Wl,--subsystem,windows
LIBS+=-lkernel32 -luser32 -lgdi32
endif
endif

ifndef TARGET
TARGET=bitblt.exe
endif

.PHONY: all
all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ -c $<

%.res: %.rc
	$(RC) $(CPPFLAGS) -o $@ -i $<

SOURCE_FILES= \
	bitblt.cpp

HEADER_FILES=

RESOURCE_FILES=

SRCS=$(SOURCE_FILES) $(HEADER_FILES) $(RESOURCE_FILES) 

OBJS=$(patsubst %.rc,%.res,$(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(filter %.c %.cpp %.rc,$(SRCS)))))

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

.PHONY: clean
clean:
	-del $(OBJS) $(TARGET)

.PHONY: depends
depends:
	-$(CXX) $(CXXFLAGS) $(CPPFLAGS) -MM $(filter %.c %.cpp,$(SRCS)) > Proj_Listing1_5.dep

-include Proj_Listing1_5.dep

