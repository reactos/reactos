PATH_TO_TOP = ../../..

TARGET=isapnp

CFLAGS = -D__NTDRIVER__ -I.

LIBS = $(PATH_TO_TOP)/ntoskrnl/ntoskrnl.a

OBJECTS = \
  isapnp.o \
	$(TARGET).coff \
  $(LIBS)


all: $(TARGET).sys.unstripped $(TARGET).sys

.phony: all


$(TARGET).coff: $(TARGET).rc $(PATH_TO_TOP)/include/reactos/resource.h

$(TARGET).sys $(TARGET).sys.unstripped: $(OBJECTS)


CLEAN_FILES = *.o $(TARGET).o $(TARGET).a junk.tmp base.tmp temp.exp \
              $(TARGET).exe $(TARGET).nostrip.exe $(TARGET).sym $(TARGET).coff

clean: 
	- $(RM) $(CLEAN_FILES)

.PHONY: clean 

install: $(FLOPPY_DIR)/drivers/$(TARGET).sys

$(FLOPPY_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys $(FLOPPY_DIR)/drivers/$(TARGET).sys

dist: $(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGET).sys

$(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys $(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGET).sys

WITH_DEBUGGING=yes
WARNINGS_ARE_ERRORS = yes

include $(PATH_TO_TOP)/rules.mak
