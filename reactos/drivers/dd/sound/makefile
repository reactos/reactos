#
#
#
PATH_TO_TOP = ../../..

TARGET = sound

OBJECTS = dsp.o mixer.o sb_waveout.o sound.o wave.o

LIBS = $(PATH_TO_TOP)/ntoskrnl/ntoskrnl.a

all: $(TARGET).sys $(TARGET).sys.unstripped

.phony: all

clean:
	- $(RM) *.o *.sym *.coff junk.tmp base.tmp temp.exp $(TARGET).sys $(TARGET).sys.unstripped

.phony: clean

install: $(FLOPPY_DIR)/drivers/$(TARGET).sys

$(FLOPPY_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys $(FLOPPY_DIR)/drivers/$(TARGET).sys

dist: $(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGET).sys

$(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys $(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGET).sys

$(TARGET).sys $(TARGET).sys.unstripped: $(OBJECTS) $(LIBS)

include $(PATH_TO_TOP)/rules.mak
