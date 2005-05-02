# $Id$
#
# ReactOS VMS Personality
#

PATH_TO_TOP = ../reactos

include $(PATH_TO_TOP)/rules.mak

VMS_PATH = .

VMS_OTHER = server

VMS_TOOLS = mksystab

VMS_LIBS = vmsdll

VMS_APPS = # vmsw32

VMS_MODULES = $(VMS_OTHER) $(VMS_TOOLS) $(VMS_LIBS) $(VMS_APPS)

all:  implib $(VMS_MODULES)

implib: $(VMS_MODULES:%=%_implib)

clean: $(VMS_MODULES:%=%_clean)

install: $(VMS_MODULES:%=%_install)

.PHONY: all implib clean


#
# Other VMS+ Modules
#
$(VMS_OTHER): %:
	make -f Makefile -C $(VMS_PATH)/$*

$(VMS_OTHER:%=%_implib): %_implib:
	make -f Makefile -C $(VMS_PATH)/$* implib

$(VMS_OTHER:%=%_clean): %_clean:
	make -f Makefile -C $(VMS_PATH)/$* clean

$(VMS_OTHER:%=%_dist): %_dist:
	make -f Makefile -C $(VMS_PATH)/$* dist

$(VMS_OTHER:%=%_install): %_install:
	make -f Makefile -C $(VMS_PATH)/$* install

.PHONY: $(VMS_OTHER) $(VMS_OTHER:%=%_implib) $(VMS_OTHER:%=%_clean) $(VMS_OTHER:%=%_install) $(VMS_OTHER:%=%_dist)


#
# VMS+ Tools
#
$(VMS_TOOLS): %:
	make -f Makefile -C $(VMS_PATH)/tools/$*

$(VMS_TOOLS:%=%_implib): %_implib:
	make -f Makefile -C $(VMS_PATH)/tools/$* implib

$(VMS_TOOLS:%=%_clean): %_clean:
	make -f Makefile -C $(VMS_PATH)/tools/$* clean

$(VMS_TOOLS:%=%_dist): %_dist:
	make -f Makefile -C $(VMS_PATH)/tools/$* dist

$(VMS_TOOLS:%=%_install): %_install:
	make -f Makefile -C $(VMS_PATH)/tools/$* install

.PHONY: $(VMS_LIBS) $(VMS_LIBS:%=%_implib) $(VMS_LIBS:%=%_clean) $(VMS_LIBS:%=%_install) $(VMS_LIBS:%=%_dist)


#
# VMS+ DLLs
#
$(VMS_LIBS): %:
	make -f Makefile -C $(VMS_PATH)/lib/$*

$(VMS_LIBS:%=%_implib): %_implib:
	make -f Makefile -C $(VMS_PATH)/lib/$* implib

$(VMS_LIBS:%=%_clean): %_clean:
	make -f Makefile -C $(VMS_PATH)/lib/$* clean

$(VMS_LIBS:%=%_dist): %_dist:
	make -f Makefile -C $(VMS_PATH)/lib/$* dist

$(VMS_LIBS:%=%_install): %_install:
	make -f Makefile -C $(VMS_PATH)/lib/$* install

.PHONY: $(VMS_LIBS) $(VMS_LIBS:%=%_implib) $(VMS_LIBS:%=%_clean) $(VMS_LIBS:%=%_install) $(VMS_LIBS:%=%_dist)


#
# VMS+ Programs
#
$(VMS_APPS): %:
	make -f Makefile -C $(VMS_PATH)/apps/$*

$(VMS_APPS:%=%_implib): %_implib:
	make -f Makefile -C $(VMS_PATH)/apps/$* implib

$(VMS_APPS:%=%_clean): %_clean:
	make -f Makefile -C $(VMS_PATH)/apps/$* clean

$(VMS_APPS:%=%_dist): %_dist:
	make -f Makefile -C $(VMS_PATH)/apps/$* dist

$(VMS_APPS:%=%_install): %_install:
	make -f Makefile -C $(VMS_PATH)/apps/$* install

.PHONY: $(VMS_APPS) $(VMS_APPS:%=%_implib) $(VMS_APPS:%=%_clean) $(VMS_APPS:%=%_install) $(VMS_APPS:%=%_dist)


etags:
	find . -name "*.[ch]" -print | etags --language=c -

# EOF
