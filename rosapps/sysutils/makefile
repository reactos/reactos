# $Id: makefile,v 1.14 2003/06/27 21:17:35 gvg Exp $
#
# ReactOS System Utilities
#
# 1999-02-16 (Emanuele Aliberti)
#	Added chkdskx.c and formatx.c by Mark Russinovich
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
# 2000-08-4 (ea)
#	Added ldd.c; removed login.c
#
#
PATH_TO_TOP=..
PATH_TO_CVSROOT=../$(PATH_TO_TOP)

BASE_CFLAGS = -I$(PATH_TO_CVSROOT)/reactos/include -D_M_IX86


ROS_DIR=$(PATH_TO_CVSROOT)/reactos
ROS_INC=$(ROS_DIR)/include
ROS_LIB=$(ROS_DIR)/dk/w32/lib
IMPORT_NTDLL=$(ROS_LIB)/ntdll.a
IMPORT_FMIFS=$(ROS_LIB)/fmifs.a
IMPORT_KERNEL32=$(ROS_LIB)/kernel32.a
IMPORT_ADVAPI32=$(ROS_LIB)/advapi32.a
IMPORT_USER32=$(ROS_LIB)/user32.a
IMPORT_MSVCRT=

TARGET=\
	chklib.exe \
	ldd.exe \
	pedump.exe \
	shutdown.exe \
	chkdsk.exe \
	format.exe 

# Removed due to dependance on C++ compiler
#
#	regexpl/regexpl.exe
#

all: $(TARGET)


# By Mark Russinovich

chkdsk.exe: chkdsk.o win32err.o wmain.o chkdsk.coff
	$(CC) \
		chkdsk.o	\
		win32err.o		\
		wmain.o			\
		chkdsk.coff		\
		$(IMPORT_FMIFS)		\
		$(IMPORT_KERNEL32)	\
		$(IMPORT_MSVCRT)	\
		-o chkdsk.exe 
	$(NM) --numeric-sort chkdsk.exe > chkdsk.sym

format.exe: format.o win32err.o wmain.o format.coff
	$(CC) \
		format.o		\
		win32err.o		\
		wmain.o			\
		format.coff		\
		$(IMPORT_FMIFS)		\
		$(IMPORT_KERNEL32)	\
		$(IMPORT_MSVCRT)	\
		-o format.exe
	$(NM) --numeric-sort format.exe > format.sym

#---

chklib.exe: chklib.o win32err.o chklib.coff
	$(CC) \
		chklib.o		\
		win32err.o		\
		chklib.coff		\
		$(IMPORT_KERNEL32)	\
		$(IMPORT_MSVCRT)	\
		-o chklib.exe
	$(NM) --numeric-sort chklib.exe > chklib.sym


shutdown.exe: shutdown.o win32err.o shutdown.coff
	$(CC) \
		shutdown.o		\
		win32err.o		\
		shutdown.coff		\
		$(IMPORT_KERNEL32)	\
		$(IMPORT_USER32)	\
		$(IMPORT_MSVCRT)	\
		-o shutdown.exe
	$(NM) --numeric-sort shutdown.exe > shutdown.sym

pedump.exe: pedump.o pedump.coff
	$(CC) \
		pedump.o		\
		pedump.coff		\
		$(IMPORT_NTDLL)		\
		$(IMPORT_KERNEL32)	\
		$(IMPORT_MSVCRT)	\
		-o pedump.exe
	$(NM) --numeric-sort pedump.exe > pedump.sym

pedump.o: pedump.c

ldd.exe: ldd.o win32err.o ldd.coff
	$(CC) \
		ldd.o			\
		win32err.o		\
		ldd.coff		\
		$(IMPORT_KERNEL32)	\
		$(IMPORT_MSVCRT)	\
		-o ldd.exe
	$(NM) --numeric-sort ldd.exe > ldd.sym

ldd.o: ldd.c

# By Nedko Arnaoudov

regexpl/regexpl.exe:
	make -C regexpl

#---

install:

.phony: install

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
