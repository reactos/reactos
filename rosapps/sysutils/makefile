# $Id: makefile,v 1.4 2000/04/25 23:22:57 ea Exp $
#
# ReactOS System Utilities
#
# 1999-02-16 (Emanuele Aliberti)
#	Added chkdskx.c and formatx.c by by Mark Russinovich
#	(mark@sysinternals.com) and shutdown.c
#
# 1999-03-03 (Emanuele Aliberti)
#	Added login.c and chklib.c
#
# 1999-03-16 (Emanuele Aliberti)
#	Added regnav.c
#
# 1999-12-19 (ea)
#	Added qsi.c
#
# 2000-04-23 (ea)
#	Added pedump.c
#
BASE_CFLAGS = -I../../reactos/include


ROS_DIR=../../reactos
ROS_INC=$(ROS_DIR)/include
ROS_LIB=$(ROS_DIR)/lib
IMPORT_NTDLL=$(ROS_LIB)/ntdll/ntdll.a
IMPORT_FMIFS=$(ROS_LIB)/fmifs/fmifs.a
IMPORT_KERNEL32=$(ROS_LIB)/kernel32/kernel32.a
IMPORT_USER32=$(ROS_LIB)/user32/user32.a
IMPORT_CRTDLL=$(ROS_LIB)/crtdll/crtdll.a

TARGET=\
	regnav.exe \
	chklib.exe \
	qsi.exe
#	pedump.exe \
#	shutdown.exe \
#	chkdsk.exe \
#	format.exe \

all: $(TARGET)

# By Mark Russinovich

chkdsk.exe: chkdsk.o win32err.o wmain.o
	$(CC) \
		chkdsk.o \
		win32err.o \
		wmain.o \
		$(IMPORT_FMIFS) \
		$(IMPORT_KERNEL32) \
		$(IMPORT_CRTDLL) \
		-o chkdsk.exe 
	$(NM) --numeric-sort chkdsk.exe > chkdsk.sym

format.exe: format.o win32err.o wmain.o
	$(CC) \
		format.o \
		win32err.o \
		wmain.o \
		$(IMPORT_FMIFS) \
		$(IMPORT_KERNEL32) \
		$(IMPORT_CRTDLL) \
		-o format.exe
	$(NM) --numeric-sort format.exe > format.sym

#---

chklib.exe: chklib.o win32err.o
	$(CC) \
		chklib.o \
		win32err.o \
		$(IMPORT_KERNEL32) \
		$(IMPORT_CRTDLL) \
		-o chklib.exe
	$(NM) --numeric-sort chklib.exe > chklib.sym


regnav.exe: regnav.o win32err.o
	$(CC) \
		regnav.o \
		win32err.o \
		$(IMPORT_KERNEL32) \
		$(IMPORT_CRTDLL) \
		-o regnav.exe
	$(NM) --numeric-sort regnav.exe > regnav.sym
	

shutdown.exe: shutdown.o win32err.o
	$(CC) \
		shutdown.o \
		win32err.o \
		$(IMPORT_KERNEL32) \
		$(IMPORT_USER32) \
		$(IMPORT_CRTDLL) \
		-o shutdown.exe
	$(NM) --numeric-sort shutdown.exe > shutdown.sym

qsi.exe: qsi.o
	echo $(BASE_CFLAGS)
	$(CC) \
		qsi.o \
		$(IMPORT_NTDLL) \
		$(IMPORT_KERNEL32) \
		$(IMPORT_CRTDLL) \
		-o qsi.exe
	$(NM) --numeric-sort qsi.exe > qsi.sym

qsi.o: qsi.c $(ROS_INC)/ddk/zwtypes.h

pedump.exe: pedump.o
	$(CC) \
		pedump.o \
		$(IMPORT_NTDLL) \
		$(IMPORT_KERNEL32) \
		$(IMPORT_CRTDLL) \
		-o pedump.exe
	$(NM) --numeric-sort pedump.exe > pedump.sym

pedump.o: pedump.c

#---

CLEAN_FILES = *.o *.exe *.sym

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
	$(CP) $* ../$(DIST_DIR)/apps/$*
endif
	
include ../rules.mak
	
# EOF
