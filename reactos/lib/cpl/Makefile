#
# ReactOS control panels makefile
#

PATH_TO_TOP = ../..

include $(PATH_TO_TOP)/rules.mak

CONTROL_PANELS = sysdm

all: $(CONTROL_PANELS)

depends: 

implib: $(CONTROL_PANELS:%=%_implib)

clean: 	$(CONTROL_PANELS:%=%_clean)

install: $(CONTROL_PANELS:%=%_install)

bootcd: $(CONTROL_PANELS:%=%_bootcd)

.PHONY: all depends implib clean install bootcd


#
# Control Panels
#
$(CONTROL_PANELS): %:
	$(MAKE) -C $*

$(CONTROL_PANELS:%=%_implib): %_implib:
	$(MAKE) -C $* implib

$(CONTROL_PANELS:%=%_clean): %_clean:
	$(MAKE) -C $* clean

$(CONTROL_PANELS:%=%_install): %_install:
	$(MAKE) -C $* install

$(CONTROL_PANELS:%=%_bootcd): %_bootcd:
	$(MAKE) -C $* bootcd

.PHONY: $(CONTROL_PANELS) $(CONTROL_PANELS:%=%_implib) $(CONTROL_PANELS:%=%_clean) $(CONTROL_PANELS:%=%_install) $(CONTROL_PANELS:%=%_bootcd)


etags:
	find . -name "*.[ch]" -print | etags --language=c -

# EOF
