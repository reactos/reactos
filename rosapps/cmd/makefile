#
#  ReactOS makefile for CMD
#

TARGET=cmd.exe

all: $(TARGET)

OBJECTS = cmd.o attrib.o alias.o batch.o beep.o call.o chcp.o choice.o \
          cls.o cmdinput.o cmdtable.o color.o console.o copy.o date.o del.o \
          delay.o dir.o dirstack.o echo.o error.o filecomp.o for.o free.o \
          goto.o history.o if.o internal.o label.o locale.o memory.o misc.o \
          move.o msgbox.o path.o pause.o prompt.o redir.o ren.o screen.o \
          set.o shift.o start.o strtoclr.o time.o timer.o title.o type.o \
          ver.o verify.o vol.o where.o window.o cmd.coff

CLEAN_FILES = *.o cmd.exe cmd.sym cmd.coff


cmd.exe: $(OBJECTS)
	$(CC) $(OBJECTS) -lkernel32 -lcrtdll -luser32 -o cmd.exe
	$(NM) --numeric-sort cmd.exe > cmd.sym


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


dist: $(TARGET:%=../$(DIST_DIR)/apps/%)

$(TARGET:%=../$(DIST_DIR)/apps/%): ../$(DIST_DIR)/apps/%: %
ifeq ($(DOSCLI),yes)
	$(CP) $* ..\$(DIST_DIR)\apps\$*
else
	$(CP) $* ../$(DIST_DIR)/apps\$*
endif

include ../rules.mak

# EOF
