# src/mesa/x86-64/Makefile

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


default: matypes.h

clean:
	-rm -f matypes.h


# need some special rules here, unfortunately
matypes.h: ../main/mtypes.h ../tnl/t_context.h ../x86/gen_matypes
	../x86/gen_matypes | grep -v '#include "assyntax.h' > matypes.h

xform4.o: matypes.h
