# $Id: makefile,v 1.11 2003/02/28 22:51:17 gvg Exp $
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

dist:
	$(MAKE) -C display dist
	$(MAKE) -C miniport dist

install:
	$(MAKE) -C display install
	$(MAKE) -C miniport install
