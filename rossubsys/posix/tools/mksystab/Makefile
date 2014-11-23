# $Id: Makefile,v 1.1 2003/01/05 18:08:11 robd Exp $
#
# Tool to generate the POSIX+ server system calls table
# and other auxiliary C files.
#

PATH_TO_TOP = ../../../reactos

PATH_TO_PSX_TOP = ../..

TARGET_TYPE = program

TARGET_NAME = mksystab

TARGET_APPTYPE = console

TARGET_NORC = yes

SYSCALL_DB=syscall.db
SYSTAB_C=$(PATH_TO_PSX_TOP)/server/call/syscall.c
SYSTAB_H=$(PATH_TO_PSX_TOP)/server/include/syscall.h
SYSCALL_H=$(PATH_TO_PSX_TOP)/include/psx/syscall.h
STUBS_C=$(PATH_TO_PSX_TOP)/server/call/stubs.c

TARGET_OBJECTS = \
	$(TARGET_NAME).o


#all: systab

#$(TARGET_NAME)$(EXE_POSTFIX): $(TARGET_NAME).c
#	$(CC) $(CFLAGS) -o $(TARGET_NAME)$(EXE_POSTFIX) $(TARGET_NAME).c

#systab: $(TARGET_NAME)$(EXE_POSTFIX)
#	$(TARGET_NAME)$(EXE_POSTFIX) \
#		$(SYSCALL_DB) \
#		$(SYSTAB_C) \
#		$(SYSTAB_H) \
#		$(SYSCALL_H) \
#		$(STUBS_C)

#clean:
#	- $(RM) $(TARGET_NAME)$(EXE_POSTFIX)
#	- $(RM) *.o
#	- $(RM) $(SYSTAB_C)
#	- $(RM) $(SYSTAB_H)
#	- $(RM) $(SYSCALL_H)
#	- $(RM) $(STUBS_C)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
