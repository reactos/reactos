#
#  ReactOS makefile for CMD
#

all: cmd.exe

OBJECTS = ../common/crt0.o cmd.o attrib.o alias.o batch.o beep.o call.o \
        chcp.o cls.o cmdinput.o cmdtable.o color.o console.o copy.o date.o \
        del.o dir.o dirstack.o echo.o error.o filecomp.o for.o goto.o \
        history.o if.o internal.o label.o locale.o misc.o move.o path.o \
        pause.o prompt.o redir.o ren.o set.o shift.o start.o time.o title.o \
        type.o ver.o verify.o vol.o where.o

LIBS= ../../lib/kernel32/kernel32.a ../../lib/crtdll/crtdll.a 

CLEAN_FILES = *.o cmd.exe cmd.sym

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.phony: clean $(CLEAN_FILES:%=%_clean)

cmd.exe: $(OBJECTS) $(LIBS)
	$(CC) -specs=../../specs $(OBJECTS) $(LIBS) -lgcc -o cmd.exe
	$(NM) --numeric-sort cmd.exe > cmd.sym

include ../../rules.mak



