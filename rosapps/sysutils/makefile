# $Id: makefile,v 1.1 1999/05/16 07:27:35 ea Exp $
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

ROSINC=../../reactos/include

all: chkdsk.exe format.exe \
	chklib.exe shutdown.exe regnav.exe
#	login.exe chklib.exe shutdown.exe regnav.exe

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

#login.exe: login.o
#	$(CC) login.o \
#		-o login.exe \
#		-lntdll \
#		-Wl,--subsystem native:4.0
#	$(NM) --numeric-sort login.exe > login.sym

chklib.exe: chklib.o win32err.o
	$(CC) chklib.o win32err.o \
		-o chklib.exe \
		-lkernel32 \
		-lcrtdll
	$(NM) --numeric-sort chklib.exe > chklib.sym


regnav.exe: regnav.o
	$(CC) regnav.o \
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

#---

CLEAN_FILES = *.o *.exe *.sym

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.phony: clean $(CLEAN_FILES:%=%_clean)

	
include ../rules.mak
	
# EOF
