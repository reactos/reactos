CC = gcc
#MODCFLAGS := -Wall -c -m486 -g -fomit-frame-pointer -O2 -DMODULE -D__KERNEL__ -DLINUX -DDEBUG -DEXPORT_SYMTAB
MODCFLAGS := -Wall -c -m486 -fomit-frame-pointer -O2 -DMODULE -D__KERNEL__ -DLINUX -DEXPORT_SYMTAB
#MODCFLAGS := -Wall -g -c -m486 -O2 -DMODULE -D__KERNEL__ -DLINUX -DEXPORT_SYMTAB
#MODCFLAGS := -Wall -g -c -m486 -O2 -DMODULE -D__KERNEL__ -DLINUX -DDEBUG -DEXPORT_SYMTAB

TARGET = pice.o
OBJS = disassembler.o debug.o privateice.o hardware.o hooks.o init.o shell.o parse.o trace.o\
       serial.o utils.o patch.o symbols.o syscall.o bp.o output.o dblflt.o pgflt.o gpfault.o\
	   hercules.o vga.o 

defaultrule: $(TARGET)

# real world targets
$(TARGET):		$(OBJS) debug
		ld -m elf_i386 -r -o ./pice.o $(OBJS)

debug:
		mkdir debug

bp.o: 	bp.c /usr/include/linux/version.h
		$(CC) $(MODCFLAGS) bp.c

dblflt.o: 	dblflt.c /usr/include/linux/version.h
		$(CC) $(MODCFLAGS) dblflt.c

debug.o: 	debug.c /usr/include/linux/version.h
		$(CC) $(MODCFLAGS) debug.c

disassembler.o:	disassembler.c /usr/include/linux/version.h
		$(CC) $(MODCFLAGS) disassembler.c

gpfault.o: 	gpfault.c /usr/include/linux/version.h
		$(CC) $(MODCFLAGS) gpfault.c

hardware.o: 	hardware.c /usr/include/linux/version.h
		$(CC) $(MODCFLAGS) hardware.c

hercules.o:	hercules.c /usr/include/linux/version.h
		$(CC) $(MODCFLAGS) hercules.c

hooks.o: 	hooks.c /usr/include/linux/version.h
		$(CC) $(MODCFLAGS) hooks.c

init.o: 	init.c /usr/include/linux/version.h
		$(CC) $(MODCFLAGS) init.c

shell.o: 	shell.c /usr/include/linux/version.h
		$(CC) $(MODCFLAGS) shell.c

output.o: 	output.c /usr/include/linux/version.h
		$(CC) $(MODCFLAGS) output.c

parse.o: 	parse.c /usr/include/linux/version.h
		$(CC) $(MODCFLAGS) parse.c

patch.o: 	patch.c /usr/include/linux/version.h
		$(CC) $(MODCFLAGS) patch.c

pgflt.o: 	pgflt.c /usr/include/linux/version.h
		$(CC) $(MODCFLAGS) pgflt.c

privateice.o: 	privateice.c /usr/include/linux/version.h
		$(CC) $(MODCFLAGS) privateice.c

serial.o: 	serial.c /usr/include/linux/version.h
		$(CC) $(MODCFLAGS) serial.c

symbols.o: 	symbols.c /usr/include/linux/version.h
		$(CC) $(MODCFLAGS) symbols.c

syscall.o: 	syscall.c /usr/include/linux/version.h
		$(CC) $(MODCFLAGS) syscall.c

trace.o: 	trace.c /usr/include/linux/version.h
		$(CC) $(MODCFLAGS) trace.c

utils.o: 	utils.c /usr/include/linux/version.h
		$(CC) $(MODCFLAGS) utils.c

vga.o:	vga.c /usr/include/linux/version.h
		$(CC) $(MODCFLAGS) vga.c

all:   clean
		@rm -f $(OBJS)
		@rm -f *~
		@chmod -x *
		@touch *
		@make 2> make.txt
		@objdump --disassemble ./pice.o > ./pice.o.txt

distclean: clean
		@rm -f $(TARGET)
		@rm -f ./pice.txt
		@echo target files cleaned up

clean:
		@rm -f $(OBJS)
		@rm -f *~
		@rm -f make.txt
		@echo intermediate files cleaned up


