# $Id: makefile,v 1.1 1999/07/04 11:14:15 ea Exp $
#
# ReactOS Operating System
#
# Generate:
# - defedf
#
TARGET = defedf

all: $(TARGET)$(EXE_POSTFIX)

$(TARGET)$(EXE_POSTFIX): $(TARGET).c
	$(CC) -g $(TARGET).c -o $(TARGET)$(EXE_POSTFIX)	

clean:
	- $(RM) $(TARGET)$(EXE_POSTFIX)

.PHONY: all clean 

include ../../rules.mak
