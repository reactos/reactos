# SOFTX86 Makefile for Linux/GCC 3.xx
# (C) 2003 Jonathan Campbell
#
# NOTE: Assumes subdirectory ./obj/ and places all object files there
#       rather than mix them up with the source code. It's less messy
#       that way. The only exception is the final output, softx86.o.

all:: __obj ../lib/libsoftx86.a ../lib/libsoftx86.so

ALL_OBJS = obj/aaa.o obj/add.o obj/cbw.o obj/clc.o obj/drooling_duh.o obj/optable.o obj/prefixes.o obj/procframe.o obj/pushpop.o obj/softx86.o obj/mov.o obj/ioport.o obj/inc.o obj/groupies.o obj/interrupts.o obj/binops.o obj/jumpy.o obj/shovel.o obj/fpu.o

INCLUDZ = -I../include

clean::
	rm -f $(ALL_OBJS)

distclean:: clean
	rm -f ../lib/libsoftx86.a
	rm -f ../lib/libsoftx86.so
	if [[ -d obj ]]; then rmdir obj; fi
	if [[ -d Release ]]; then rm -R Release; fi	# clean up mess
	if [[ -d Debug ]]; then rm -R Debug; fi		# made by MSVC++
	rm -f *.plg
	rm -f *.opt
	rm -f *.aps

# make sure there is an "obj" subdirectory
__obj:
	if [[ !(-d obj) ]]; then mkdir obj; fi

../lib/libsoftx86.a: $(ALL_OBJS)
	ld -aarchive -r -o ../lib/libsoftx86.a $(ALL_OBJS)

../lib/libsoftx86.so: $(ALL_OBJS)
	ld -shared -o ../lib/libsoftx86.so $(ALL_OBJS)

obj/shovel.o: shovel.c
	gcc $(INCLUDZ) -c -o obj/shovel.o shovel.c

obj/jumpy.o: jumpy.c
	gcc $(INCLUDZ) -c -o obj/jumpy.o jumpy.c

obj/binops.o: binops.c
	gcc $(INCLUDZ) -c -o obj/binops.o binops.c

obj/interrupts.o: interrupts.c
	gcc $(INCLUDZ) -c -o obj/interrupts.o interrupts.c

obj/groupies.o: groupies.c
	gcc $(INCLUDZ) -c -o obj/groupies.o groupies.c

obj/aaa.o: aaa.c
	gcc $(INCLUDZ) -c -o obj/aaa.o aaa.c

obj/add.o: add.c
	gcc $(INCLUDZ) -c -o obj/add.o add.c

obj/cbw.o: cbw.c
	gcc $(INCLUDZ) -c -o obj/cbw.o cbw.c

obj/clc.o: clc.c
	gcc $(INCLUDZ) -c -o obj/clc.o clc.c

obj/drooling_duh.o: drooling_duh.c
	gcc $(INCLUDZ) -c -o obj/drooling_duh.o drooling_duh.c

obj/optable.o: optable.c
	gcc $(INCLUDZ) -c -o obj/optable.o optable.c

obj/prefixes.o: prefixes.c
	gcc $(INCLUDZ) -c -o obj/prefixes.o prefixes.c

obj/procframe.o: procframe.c
	gcc $(INCLUDZ) -c -o obj/procframe.o procframe.c

obj/pushpop.o: pushpop.c
	gcc $(INCLUDZ) -c -o obj/pushpop.o pushpop.c

obj/softx86.o: softx86.c
	gcc $(INCLUDZ) -c -o obj/softx86.o softx86.c

obj/mov.o: mov.c
	gcc $(INCLUDZ) -c -o obj/mov.o mov.c

obj/ioport.o: ioport.c
	gcc $(INCLUDZ) -c -o obj/ioport.o ioport.c

obj/inc.o: inc.c
	gcc $(INCLUDZ) -c -o obj/inc.o inc.c

obj/fpu.o: fpu.c
	gcc $(INCLUDZ) -c -o obj/fpu.o fpu.c

