# $Id: Makefile,v 1.2 2003/12/07 11:34:41 chorns Exp $

PATH_TO_TOP = ..

all: registrations
	$(MAKE) -C shared all
	$(MAKE) -C win32base all
	$(MAKE) -C kmregtests all
	$(MAKE) -C kmrtint all
	$(MAKE) -C regtests all

clean:
	- $(MAKE) -C shared clean
	- $(MAKE) -C win32base clean
	- $(MAKE) -C kmregtests clean
	- $(MAKE) -C kmrtint clean
	- $(MAKE) -C regtests clean
	- $(RM) ./kmregtests/_regtests.c ./kmregtests/Makefile.tests
	- $(RM) ./win32base/_regtests.c ./win32base/Makefile.tests

install:
	$(MAKE) -C shared install
	$(MAKE) -C win32base install
	$(MAKE) -C kmregtests install
	$(MAKE) -C kmrtint install
	$(MAKE) -C regtests install

registrations:
	$(REGTESTS) ./kmregtests/tests ./kmregtests/_regtests.c ./kmregtests/Makefile.tests
	$(REGTESTS) ./win32base/tests ./win32base/_regtests.c ./win32base/Makefile.tests

.PHONY: all clean install registrations

include $(PATH_TO_TOP)/rules.mak
