# $Id: makefile,v 1.3 2000/02/29 23:57:47 ea Exp $
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

ROSINC=../../reactos/include

TARGET=regnav.exe chklib.exe
# chkdsk.exe format.exe \
#        chklib.exe shutdown.exe regnav.exe
#	login.exe chklib.exe shutdown.exe regnav.exe

all: $(TARGET)

# By Mark Russinovich

chkdsk.exe: chkdsk.o win32err.o wmain.o
	$(CC) chkdsk.o win32err.o wmain.o \
		-o chkdsk.exe \
		-lkernel32 \
		-lcrtdll \
		-lfmifs
	$(NM) --numeric-sort chkdsk.exe > chkdsk.sym

format.exe: format.o win32err.o wmain.o
	$(CC) format.o win32err.o wmain.o \
		-o format.exe \
		-lkernel32 \
		-lcrtdll \
		-lfmifs
	$(NM) --numeric-sort format.exe > format.sym

#---

chklib.exe: chklib.o win32err.o
	$(CC) chklib.o win32err.o \
		-o chklib.exe \
		-lkernel32 \
		-lcrtdll
	$(NM) --numeric-sort chklib.exe > chklib.sym


regnav.exe: regnav.o win32err.o
	$(CC) regnav.o win32err.o \
		-o regnav.exe \
		-lkernel32 \
		-lcrtdll
	$(NM) --numeric-sort regnav.exe > regnav.sym
	

shutdown.exe: shutdown.o win32err.o
	$(CC) shutdown.o win32err.o \
		-o shutdown.exe \
		-lkernel32 \
		-luser32 \
		-lcrtdll
	$(NM) --numeric-sort shutdown.exe > shutdown.sym

qsi.exe: qsi.o
	$(CC) qsi.o \
		../../reactos/lib/ntdll/ntdll.a
		-o qsi.exe \
		-lkernel32 \
		-lcrtdll
	$(NM) --numeric-sort qsi.exe > qsi.sym

qsi.o: qsi.c

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
