#
#  ReactOS makefile for D-Flat/32
#

PATH_TO_TOP=..

TARGET=edit.exe

all: $(TARGET)

OBJECTS = applicat.o barchart.o box.o button.o calendar.o checkbox.o clipbord.o \
          combobox.o config.o console.o decomp.o dfalloc.o dialbox.o dialogs.o \
          direct.o edit.o editbox.o fileopen.o helpbox.o htree.o keys.o \
          listbox.o lists.o log.o menu.o menubar.o menus.o message.o msgbox.o \
          normal.o pictbox.o popdown.o radio.o rect.o search.o slidebox.o spinbutt.o \
          statbar.o sysmenu.o text.o textbox.o video.o watch.o window.o \
	  edit.coff

CLEAN_FILES = *.o *.exe *.sym *.coff


edit.exe: $(OBJECTS)
	$(CC) $(OBJECTS) -lkernel32 -lcrtdll -luser32 -o edit.exe
	$(NM) --numeric-sort edit.exe > edit.sym


clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.phony: clean $(CLEAN_FILES:%=%_clean)


floppy: $(TARGET:%=$(FLOPPY_DIR)/apps/%)

$(TARGET:%=$(FLOPPY_DIR)/apps/%): $(FLOPPY_DIR)/apps/%: %
ifeq ($(DOSCLI),yes)
	$(CP) $* $(FLOPPY_DIR)\apps\$*
else
	$(CP) $* $(FLOPPY_DIR)/apps/$*
endif


dist: $(TARGET:%=$(PATH_TO_TOP)/$(DIST_DIR)/apps/%)

$(TARGET:%=../$(DIST_DIR)/apps/%): $(PATH_TO_TOP)/$(DIST_DIR)/apps/%: %
ifeq ($(DOSCLI),yes)
	$(CP) $* $(PATH_TO_TOP)\$(DIST_DIR)\apps\$*
else
	$(CP) $* $(PATH_TO_TOP)/$(DIST_DIR)/apps\$*
endif


#WITH_DEBUGGING=yes

include $(PATH_TO_TOP)/rules.mak

# EOF
