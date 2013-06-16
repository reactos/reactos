# SOFTX87 Makefile for Linux/GCC 3.xx
# (C) 2003 Jonathan Campbell
#
# NOTE: Assumes subdirectory ./obj/ and places all object files there
#       rather than mix them up with the source code. It's less messy
#       that way. The only exception is the final output, softx86.o.

all:: __obj ../lib/libsoftx87.a ../lib/libsoftx87.so

ALL_OBJS = obj/optable.o obj/softx87.o

INCLUDZ = -I../include

clean::
	rm -f $(ALL_OBJS)

distclean:: clean
	rm -f ../lib/libsoftx87.a
	rm -f ../lib/libsoftx87.so
	if [[ -d obj ]]; then rmdir obj; fi
	if [[ -d Release ]]; then rm -R Release; fi
	if [[ -d Debug ]]; then rm -R Debug; fi
	rm -f *.plg
	rm -f *.aps

# make sure there is an "obj" subdirectory
__obj:
	if [[ !(-d obj) ]]; then mkdir obj; fi

../lib/libsoftx87.a: $(ALL_OBJS)
	ld -aarchive -r -o ../lib/libsoftx87.a $(ALL_OBJS)

../lib/libsoftx87.so: $(ALL_OBJS)
	ld -shared -o ../lib/libsoftx87.so $(ALL_OBJS)

obj/optable.o: optable.c
	gcc $(INCLUDZ) -c -o obj/optable.o optable.c

obj/softx87.o: softx87.c
	gcc $(INCLUDZ) -c -o obj/softx87.o softx87.c

