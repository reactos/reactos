# src/mesa/x86/Makefile

TOP = ../../..
include $(TOP)/configs/current


INCLUDE_DIRS = \
	-I$(TOP)/include/GL \
	-I$(TOP)/include \
	-I.. \
	-I../main \
	-I../math \
	-I../glapi \
	-I../tnl


default: gen_matypes matypes.h

clean:
	-rm -f matypes.h gen_matypes


gen_matypes: gen_matypes.c
	$(CC) $(INCLUDE_DIRS) $(CFLAGS) gen_matypes.c -o gen_matypes

# need some special rules here, unfortunately
matypes.h: ../main/mtypes.h ../tnl/t_context.h gen_matypes
	./gen_matypes > matypes.h

common_x86_asm.o: matypes.h
3dnow_normal.o: matypes.h
3dnow_xform1.o: matypes.h
3dnow_xform2.o: matypes.h
3dnow_xform3.o: matypes.h
3dnow_xform4.o: matypes.h
mmx_blend.o: matypes.h
sse_normal.o: matypes.h
sse_xform1.o: matypes.h
sse_xform2.o: matypes.h
sse_xform3.o: matypes.h
sse_xform4.o: matypes.h
x86_cliptest.o: matypes.h
x86_xform2.o: matypes.h
x86_xform3.o: matypes.h
x86_xform4.o: matypes.h

