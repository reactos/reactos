#
#

LIB=../../lib
COMMON=../common

# target: raw binary (does not work right now)
#all: cmd.bin

# target: executable (not tested/experimental)
all: cmd.exe

OBJECTS = $(COMMON)/crt0.o cmd.o attrib.o alias.o batch.o beep.o call.o cls.o cmdinput.o cmdtable.o\
        color.o date.o del.o dir.o echo.o err_hand.o error.o filecomp.o for.o goto.o history.o if.o\
        internal.o label.o misc.o path.o pause.o redir.o ren.o set.o shift.o tempfile.o time.o type.o ver.o\
        verify.o where.o

LIBS = $(LIB)/mingw32/mingw32.a $(LIB)/crtdll/crtdll.a \
       $(LIB)/kernel32/kernel32.a $(LIB)/ntdll/ntdll.a

cmd.bin: $(OBJECTS)
	$(LD) -Ttext 0x10000 $(OBJECTS) $(LIBS) -o cmd.exe
	$(OBJCOPY) -O binary cmd.exe cmd.bin

cmd.exe: $(OBJECTS)
	$(LD) $(OBJECTS) $(LIBS) -o cmd.exe

include ../../rules.mak

