# $Id: makefile,v 1.13 2003/11/24 16:45:16 ekohl Exp $
#
#
all:
	$(MAKE) -C display
	$(MAKE) -C miniport

implib:
	$(MAKE) -C display implib
	$(MAKE) -C miniport implib
	
clean:
	$(MAKE) -C display clean
	$(MAKE) -C miniport clean

install:
	$(MAKE) -C display install
	$(MAKE) -C miniport install

bootcd:
	$(MAKE) -C display bootcd
	$(MAKE) -C miniport bootcd
