# $Id: makefile,v 1.9.10.1 2004/06/03 22:29:31 arty Exp $

PATH_TO_TOP = ../..

include $(PATH_TO_TOP)/rules.mak

all: loadros.com

loadros.com: loadros.asm
	$(NASM) -fbin -o loadros.com loadros.asm

clean:
	- $(RM) loadros.com

.PHONY: clean

install: $(INSTALL_DIR)/loadros.com

$(INSTALL_DIR)/loadros.com: loadros.com
	$(CP) loadros.com $(INSTALL_DIR)/loadros.com

.PHONY: install

# EOF
