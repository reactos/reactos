# $Id: makefile,v 1.2 1999/08/29 13:44:52 dwelch Exp $
#
# ReactOS Operating System
#
# Generate:
# - defedf
#
TARGET = defedf

BASE_CFLAGS = -I../../include

all: $(TARGET)$(EXE_POSTFIX)

$(TARGET)$(EXE_POSTFIX): $(TARGET).c
	$(CC) -g $(TARGET).c -o $(TARGET)$(EXE_POSTFIX)	

clean:
	- $(RM) $(TARGET)$(EXE_POSTFIX)

.PHONY: all clean 

include ../../rules.mak
