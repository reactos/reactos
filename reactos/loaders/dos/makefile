# $Id: makefile,v 1.10 2004/06/02 18:24:08 hyperion Exp $

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
