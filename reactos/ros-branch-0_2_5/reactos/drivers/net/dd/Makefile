#
# ReactOS Networking Device Drivers
#

PATH_TO_TOP = ../../..

include $(PATH_TO_TOP)/rules.mak

DRIVERS = ne2000 pcnet

all: $(DRIVERS)

depends: 

implib: $(DRIVERS:%=%_implib)

clean: 	$(DRIVERS:%=%_clean)

install: $(DRIVERS:%=%_install)

bootcd: $(DRIVERS:%=%_bootcd)

.PHONY: all depends implib clean install bootcd


#
# NETWORKING DEVICE DRIVERS
#
$(DRIVERS): %:
	$(MAKE) -C $*

$(DRIVERS:%=%_implib): %_implib:
	$(MAKE) -C $* implib

$(DRIVERS:%=%_clean): %_clean:
	$(MAKE) -C $* clean

$(DRIVERS:%=%_install): %_install:
	$(MAKE) -C $* install

$(DRIVERS:%=%_bootcd): %_bootcd:
	$(MAKE) -C $* bootcd

.PHONY: $(DRIVERS) $(DRIVERS:%=%_implib) $(DRIVERS:%=%_clean) $(DRIVERS:%=%_install) $(DRIVERS:%=%_bootcd)


etags:
	find . -name "*.[ch]" -print | etags --language=c -

# EOF
