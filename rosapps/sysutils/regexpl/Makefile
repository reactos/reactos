# $Id: Makefile,v 1.7 2002/09/03 18:44:18 chorns Exp $
#
#  ReactOS makefile for RegExpl
#

PATH_TO_TOP = ../..

include $(PATH_TO_TOP)/rules.mak

TARGET_NAME=regexpl

all: $(TARGET_NAME).exe

include depend.mak

ROS_DIR=../../../reactos
ROS_INC=$(ROS_DIR)/include
ROS_LIB=$(ROS_DIR)/dk/w32/lib
IMPORT_NTDLL=$(ROS_LIB)/ntdll.a
IMPORT_FMIFS=$(ROS_LIB)/fmifs.a
IMPORT_KERNEL32=$(ROS_LIB)/kernel32.a
IMPORT_ADVAPI32=$(ROS_LIB)/advapi32.a
IMPORT_USER32=$(ROS_LIB)/user32.a

depend.mak : *.h
	$(CC) \
		-MM *.cpp *.c > $@

OBJECTS = \
	ArgumentParser.o		\
	Console.o			\
	RegistryExplorer.o		\
	RegistryKey.o			\
	RegistryTree.o			\
	SecurityDescriptor.o		\
	ShellCommand.o			\
	ShellCommandChangeKey.o		\
	ShellCommandConnect.o		\
	ShellCommandDACL.o		\
	ShellCommandDeleteKey.o		\
	ShellCommandDeleteValue.o	\
	ShellCommandDir.o		\
	ShellCommandExit.o		\
	ShellCommandHelp.o		\
	ShellCommandNewKey.o		\
	ShellCommandOwner.o		\
	ShellCommandSACL.o		\
	ShellCommandSetValue.o		\
	ShellCommandValue.o		\
	ShellCommandVersion.o		\
	ShellCommandsLinkedList.o	\
	CrtSupplement.c			\
	TextHistory.o			\
	Completion.o			\
	Pattern.o			\
	Settings.o			\
	Prompt.o			\
	$(TARGET_NAME).coff

CLEAN_FILES = \
	*.o				\
	$(TARGET_NAME)$(EXE_POSTFIX)	\
	$(TARGET_NAME).sym		\
	$(TARGET_NAME).coff\
	depend.mak


$(TARGET_NAME)$(EXE_POSTFIX): $(OBJECTS)
	$(CPP) \
		-Wl,--subsystem,console	\
		-o $@				\
		$(IMPORT_KERNEL32)	\
		$(IMPORT_USER32)	\
		$(OBJECTS)
	$(NM) --numeric-sort $(TARGET_NAME)$(EXE_POSTFIX) > $(TARGET_NAME).sym

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.phony: clean $(CLEAN_FILES:%=%_clean)


floppy: $(TARGET:%=$(FLOPPY_DIR)/apps/%)

$(TARGET:%=$(FLOPPY_DIR)/apps/%): $(FLOPPY_DIR)/apps/%: %
ifeq ($(DOSCLI),yes)
	$(CP) $* $(FLOPPY_DIR)\apps\$*
else
	$(CP) $* $(FLOPPY_DIR)/apps/$*
endif


dist: $(TARGET:%=../$(DIST_DIR)/apps/%)

$(TARGET:%=../$(DIST_DIR)/apps/%): ../$(DIST_DIR)/apps/%: %
ifeq ($(DOSCLI),yes)
	$(CP) $* ..\$(DIST_DIR)\apps\$*
else
	$(CP) $* ../$(DIST_DIR)/apps\$*
endif

# EOF

