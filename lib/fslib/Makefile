#
# ReactOS Driver Support Libraries
#

PATH_TO_TOP = ../..

include $(PATH_TO_TOP)/rules.mak

FSLIBS = vfatlib

all: $(FSLIBS)

depends: 

implib: $(FSLIBS:%=%_implib)

clean: 	$(FSLIBS:%=%_clean)

install: $(FSLIBS:%=%_install)

bootcd: $(FSLIBS:%=%_bootcd)

.PHONY: all depends implib clean install bootcd


#
# FILE SYSTEM LIBRARIES
#
$(FSLIBS): %:
	$(MAKE) -C $*

$(FSLIBS:%=%_implib): %_implib:
	$(MAKE) -C $* implib

$(FSLIBS:%=%_clean): %_clean:
	$(MAKE) -C $* clean

$(FSLIBS:%=%_install): %_install:
	$(MAKE) -C $* install

$(FSLIBS:%=%_bootcd): %_bootcd:
	$(MAKE) -C $* bootcd

.PHONY: $(FSLIBS) $(FSLIBS:%=%_implib) $(FSLIBS:%=%_clean) $(FSLIBS:%=%_install) $(FSLIBS:%=%_bootcd)

etags:
	find . -name "*.[ch]" -print | etags --language=c -
# EOF
