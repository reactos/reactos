# $Id$
#
#  ReactOS makefile for RegExpl
#

PATH_TO_TOP := ../../../reactos

include $(PATH_TO_TOP)/rules.mak

TARGET_NAME=regexpl

all: $(TARGET_NAME).exe

ROS_DIR=../../../reactos
ROS_INC=$(ROS_DIR)/include
ROS_LIB=$(ROS_DIR)/dk/w32/lib
IMPORT_NTDLL=$(ROS_LIB)/libntdll.a
IMPORT_FMIFS=$(ROS_LIB)/libfmifs.a
IMPORT_KERNEL32=$(ROS_LIB)/libkernel32.a
IMPORT_ADVAPI32=$(ROS_LIB)/libadvapi32.a
IMPORT_USER32=$(ROS_LIB)/libuser32.a

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
	Prompt.o			

CLEAN_FILES = \
	*.o				\
	$(TARGET_NAME).exe		\
	$(TARGET_NAME).sym		

$(TARGET_NAME).exe: $(OBJECTS)
	$(CXX) -Wl,--subsystem,console -o $(TARGET_NAME).exe $(OBJECTS)$(IMPORT_KERNEL32) $(IMPORT_USER32)
	$(NM) --numeric-sort $(TARGET_NAME).exe > $(TARGET_NAME).sym

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
