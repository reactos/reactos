# $Id: makefile,v 1.3.34.1 2004/10/25 01:29:47 ion Exp $
#
# ReactOS Operating System
#
# Generate:
# - defedf
#
PATH_TO_TOP = ../..

TARGET = defedf

BASE_CFLAGS = -I../../include

all: $(TARGET)$(EXE_POSTFIX)

$(TARGET)$(EXE_POSTFIX): $(TARGET).c
	$(CC) -g $(TARGET).c -o $(TARGET)$(EXE_POSTFIX)	

clean:
	- $(RM) $(TARGET)$(EXE_POSTFIX)

.PHONY: all clean 

include ../../rules.mak
