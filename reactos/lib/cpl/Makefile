#
# ReactOS control panels makefile
#

PATH_TO_TOP = ../..

include $(PATH_TO_TOP)/rules.mak

CONTROL_PANELS = 

all: $(CONTROL_PANELS)

depends: 

implib: $(CONTROL_PANELS:%=%_implib)

clean: 	$(CONTROL_PANELS:%=%_clean)

install: $(CONTROL_PANELS:%=%_install)

.PHONY: all depends implib clean install


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

.PHONY: $(CONTROL_PANELS) $(CONTROL_PANELS:%=%_implib) $(CONTROL_PANELS:%=%_clean) $(CONTROL_PANELS:%=%_install)


etags:
	find . -name "*.[ch]" -print | etags --language=c -

# EOF
