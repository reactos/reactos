# $Id: Makefile,v 1.1 2001/11/04 21:53:20 ea Exp $
#
#  ReactOS makefile for TList
#
PATH_TO_TOP=../..

include $(PATH_TO_TOP)/rules.mak

TARGET_NAME=tlist

all: $(TARGET_NAME)$(EXE_POSTFIX)

ROS_DIR=../$(PATH_TO_TOP)/reactos
ROS_INC=$(ROS_DIR)/include
ROS_LIB=$(ROS_DIR)/dk/w32/lib
IMPORT_NTDLL=$(ROS_LIB)/ntdll.a
IMPORT_KERNEL32=$(ROS_LIB)/kernel32.a
IMPORT_CRTDLL=$(ROS_LIB)/msvcrt.a


BASE_CFLAGS=-I$(ROS_INC)

OBJECTS = \
	tlist.o \
	$(TARGET_NAME).coff

CLEAN_FILES = \
	*.o				\
	$(TARGET_NAME)$(EXE_POSTFIX)	\
	$(TARGET_NAME).sym		\
	$(TARGET_NAME).coff

$(TARGET_NAME)$(EXE_POSTFIX): $(OBJECTS)
	$(CC) \
		-Wl,--subsystem,console	\
		-o $@			\
		$(OBJECTS)		\
		$(IMPORT_NTDLL)
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

