# $Id: makefile,v 1.7 2000/11/20 19:59:10 ekohl Exp $
#
# loadros makefile
#
PATH_TO_TOP = ../..

all: loadros.com

loadros.com: loadros.asm
	$(NASM_CMD) -fbin loadros.asm -o loadros.com

clean:
	- $(RM) loadros.com

.PHONY: clean

install: $(FLOPPY_DIR)/loadros.com

$(FLOPPY_DIR)/loadros.com: loadros.com
	$(CP) loadros.com $(FLOPPY_DIR)/loadros.com

.PHONY: install

dist: ../../$(DIST_DIR)/loadros.com

../../$(DIST_DIR)/loadros.com: loadros.com
	$(CP) loadros.com ../../$(DIST_DIR)/loadros.com

.PHONY: dist


include ../../rules.mak
#EOF
