PATH_TO_TOP = ..

all:
	$(MAKE) -C regtests all
	$(MAKE) -C shared all

implib:
	- $(MAKE) -C regtests implib

clean:
	- $(MAKE) -C regtests clean
	- $(MAKE) -C shared clean

install:
	$(MAKE) -C regtests install
	$(MAKE) -C shared install

.PHONY: all clean install

include $(PATH_TO_TOP)/rules.mak
