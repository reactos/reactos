# $Id: makefile,v 1.8 2001/08/21 20:13:08 chorns Exp $

PATH_TO_TOP = ../..

include $(PATH_TO_TOP)/rules.mak

all: loadros.com

loadros.com: loadros.asm
	$(NASM_CMD) -fbin -o loadros.com loadros.asm

clean:
	- $(RM) loadros.com

.PHONY: clean

install: $(INSTALL_DIR)/loadros.com

$(INSTALL_DIR)/loadros.com: loadros.com
	$(CP) loadros.com $(INSTALL_DIR)/loadros.com

.PHONY: install

dist: $(DIST_DIR)/loadros.com

$(DIST_DIR)/loadros.com: loadros.com
	$(CP) loadros.com $(DIST_DIR)/loadros.com

.PHONY: dist

# EOF
