# $Id: Makefile,v 1.7 2003/01/05 18:29:41 robd Exp $
#
# ReactOS POSIX+ Personality
#

PATH_TO_TOP = ../reactos

include $(PATH_TO_TOP)/rules.mak


POSIX_OTHER = server #lib

POSIX_TOOLS = mksystab

POSIX_LIBS = psxdll psxx

POSIX_APPS = baresh posixw32

POSIX_MODULES = $(POSIX_OTHER) $(POSIX_TOOLS) $(POSIX_LIBS) $(POSIX_APPS)

all:  implib $(POSIX_MODULES)

implib: $(POSIX_MODULES:%=%_implib)

clean: $(POSIX_MODULES:%=%_clean)

install: $(POSIX_MODULES:%=%_install)

#dist: $(TOOLS_PATH)/rcopy$(EXE_POSTFIX) dist_clean dist_dirs \
#      $(POSIX_MODULES:%=%_dist)

.PHONY: all implib clean
#.PHONY: all implib install dist


#
# Other POSIX+ Modules
#
$(POSIX_OTHER): %:
	make -f Makefile -C $(POSIX_PATH)/$*

$(POSIX_OTHER:%=%_implib): %_implib:
	make -f Makefile -C $(POSIX_PATH)/$* implib

$(POSIX_OTHER:%=%_clean): %_clean:
	make -f Makefile -C $(POSIX_PATH)/$* clean

$(POSIX_OTHER:%=%_dist): %_dist:
	make -f Makefile -C $(POSIX_PATH)/$* dist

$(POSIX_OTHER:%=%_install): %_install:
	make -f Makefile -C $(POSIX_PATH)/$* install

.PHONY: $(POSIX_OTHER) $(POSIX_OTHER:%=%_implib) $(POSIX_OTHER:%=%_clean) $(POSIX_OTHER:%=%_install) $(POSIX_OTHER:%=%_dist)


#
# POSIX+ Tools
#
$(POSIX_TOOLS): %:
	make -f Makefile -C $(POSIX_PATH)/tools/$*

$(POSIX_TOOLS:%=%_implib): %_implib:
	make -f Makefile -C $(POSIX_PATH)/tools/$* implib

$(POSIX_TOOLS:%=%_clean): %_clean:
	make -f Makefile -C $(POSIX_PATH)/tools/$* clean

$(POSIX_TOOLS:%=%_dist): %_dist:
	make -f Makefile -C $(POSIX_PATH)/tools/$* dist

$(POSIX_TOOLS:%=%_install): %_install:
	make -f Makefile -C $(POSIX_PATH)/tools/$* install

.PHONY: $(POSIX_LIBS) $(POSIX_LIBS:%=%_implib) $(POSIX_LIBS:%=%_clean) $(POSIX_LIBS:%=%_install) $(POSIX_LIBS:%=%_dist)


#
# POSIX+ DLLs
#
$(POSIX_LIBS): %:
	make -f Makefile -C $(POSIX_PATH)/lib/$*

$(POSIX_LIBS:%=%_implib): %_implib:
	make -f Makefile -C $(POSIX_PATH)/lib/$* implib

$(POSIX_LIBS:%=%_clean): %_clean:
	make -f Makefile -C $(POSIX_PATH)/lib/$* clean

$(POSIX_LIBS:%=%_dist): %_dist:
	make -f Makefile -C $(POSIX_PATH)/lib/$* dist

$(POSIX_LIBS:%=%_install): %_install:
	make -f Makefile -C $(POSIX_PATH)/lib/$* install

.PHONY: $(POSIX_LIBS) $(POSIX_LIBS:%=%_implib) $(POSIX_LIBS:%=%_clean) $(POSIX_LIBS:%=%_install) $(POSIX_LIBS:%=%_dist)


#
# POSIX+ Programs
#
$(POSIX_APPS): %:
	make -f Makefile -C $(POSIX_PATH)/apps/$*

$(POSIX_APPS:%=%_implib): %_implib:
	make -f Makefile -C $(POSIX_PATH)/apps/$* implib

$(POSIX_APPS:%=%_clean): %_clean:
	make -f Makefile -C $(POSIX_PATH)/apps/$* clean

$(POSIX_APPS:%=%_dist): %_dist:
	make -f Makefile -C $(POSIX_PATH)/apps/$* dist

$(POSIX_APPS:%=%_install): %_install:
	make -f Makefile -C $(POSIX_PATH)/apps/$* install

.PHONY: $(POSIX_APPS) $(POSIX_APPS:%=%_implib) $(POSIX_APPS:%=%_clean) $(POSIX_APPS:%=%_install) $(POSIX_APPS:%=%_dist)


etags:
	find . -name "*.[ch]" -print | etags --language=c -

# EOF



#CFLAGS=-Iinclude

#all: lib/crt0w32.o
#	make -C tools
#	make -C lib
#	make -C server
#	make -C lib/psxdll
#	make -C lib/psxx
#	make -C apps/baresh
#	make -C apps/posixw32

#lib/crt0w32.o: lib/crt0w32.c

#implib:

#clean:
#	make -C lib clean
#	make -C tools clean
#	make -C server clean
#	make -C lib/psxdll clean
#	make -C lib/psxx clean
#	make -C apps/baresh clean
#	make -C apps/posixw32 clean
#	- $(RM) lib/crt0w32.o

#include $(PATH_TO_TOP)/rules.mak

# EOF
