#
#
#

all: loadros.com

loadros.com: loadros.asm
	$(NASM_CMD) -fbin loadros.asm -o loadros.com

clean:
	- $(RM) loadros.com

.PHONY: clean

floppy: $(FLOPPY_DIR)/loadros.com

$(FLOPPY_DIR)/loadros.com: loadros.com
ifeq ($(DOSCLI),yes)
	$(CP) loadros.com $(FLOPPY_DIR)\loadros.com
else
	$(CP) loadros.com $(FLOPPY_DIR)/loadros.com
endif

dist: ../../$(DIST_DIR)/loadros.com

../../$(DIST_DIR)/loadros.com: loadros.com
ifeq ($(DOSCLI),yes)
	$(CP) loadros.com ..\..\$(DIST_DIR)\loadros.com
else
	$(CP) loadros.com ../../$(DIST_DIR)/loadros.com
endif

include ../../rules.mak
