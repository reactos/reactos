#
#  pipetools Makefile
#

CC = gcc
CXX = g++
LINK = g++

TOOLS_PATH = ..

CFLAGS	= -DWIN32 -Wall
RCFLAGS	= -DWIN32
LFLAGS	=

ifdef DEBUG
CFLAGS	+= -D_DEBUG -g
RCFLAGS	+= -D_DEBUG
LFLAGS	+= -g
else
CFLAGS	+= -DNDEBUG -Os
RCFLAGS	+= -DNDEBUG
LFLAGS	+= -s
endif

CXXFLAGS = $(CFLAGS)

LIBS = wsock32


%.exe: %.c
	$(CC) -o$@ $(CFLAGS) $^ 

%.exe: %.cpp
	$(CXX) -o$@ $(CXXFLAGS) $^ $(addprefix -l,$(LIBS))


TARGETS = piperead.exe pipetunnel.exe

all: $(TARGETS)


clean:
	$(TOOLS_PATH)/rdel $(TARGETS) *.o

