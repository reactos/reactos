# $Id: Makefile,v 1.1 2003/07/11 18:13:57 chorns Exp $

all:
	$(MAKE) -C shared all
	$(MAKE) -C win32base all
	$(MAKE) -C kmregtests all
	$(MAKE) -C kmrtint all
	$(MAKE) -C regtests all

clean:
	$(MAKE) -C shared clean
	$(MAKE) -C win32base clean
	$(MAKE) -C kmregtests clean
	$(MAKE) -C kmrtint clean
	$(MAKE) -C regtests clean

install:
	$(MAKE) -C shared install
	$(MAKE) -C win32base install
	$(MAKE) -C kmregtests install
	$(MAKE) -C kmrtint install
	$(MAKE) -C regtests install

.PHONY: all clean install
	
