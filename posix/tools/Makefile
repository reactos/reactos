# $Id: Makefile,v 1.4 2002/10/29 04:45:58 rex Exp $
#
# Tool to generate the POSIX+ server system calls table
# and other auxiliary C files.
#
PATH_TO_TOP=../../reactos
PATH_TO_PSX_TOP=..

TARGET_NAME=mksystab
SYSCALL_DB=syscall.db
SYSTAB_C=$(PATH_TO_PSX_TOP)/server/call/syscall.c
SYSTAB_H=$(PATH_TO_PSX_TOP)/server/include/syscall.h
SYSCALL_H=$(PATH_TO_PSX_TOP)/include/psx/syscall.h
STUBS_C=$(PATH_TO_PSX_TOP)/server/call/stubs.c

all: systab

$(TARGET_NAME)$(EXE_POSTFIX): $(TARGET_NAME).c
	$(CC) $(CFLAGS) -o $(TARGET_NAME)$(EXE_POSTFIX) $(TARGET_NAME).c

systab: $(TARGET_NAME)$(EXE_POSTFIX)
	$(TARGET_NAME)$(EXE_POSTFIX) \
		$(SYSCALL_DB) \
		$(SYSTAB_C) \
		$(SYSTAB_H) \
		$(SYSCALL_H) \
		$(STUBS_C)

clean:
	- $(RM) $(TARGET_NAME)$(EXE_POSTFIX)
	- $(RM) *.o
	- $(RM) $(SYSTAB_C)
	- $(RM) $(SYSTAB_H)
	- $(RM) $(SYSCALL_H)
	- $(RM) $(STUBS_C)

include $(PATH_TO_TOP)/rules.mak

# EOF
