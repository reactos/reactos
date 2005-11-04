# src/mesa/drivers/dri/gamma/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

# Get rid of this:
#
DEFINES += -DGLX_DIRECT_RENDERING 

C_SOURCES = \
	xf86drm.c \
	xf86drmHash.c \
	xf86drmRandom.c \
	xf86drmSL.c \



OBJECTS = $(C_SOURCES:.c=.o)  

INCLUDES = \
	-I$(TOP)/include \
	-I$(DRM_SOURCE_PATH)/shared-core \
	-I$(TOP)/include/GL/internal \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mesa/main \
	-I$(TOP)/src/mesa/glapi \
	-Iimports


##### RULES #####

.c.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@


##### TARGETS #####

default: depend dri.a


# Run 'make dep' to update the dependencies if you change
# what's included by any source file.
depend: $(C_SOURCES) $(ASM_SOURCES)
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(INCLUDES) \
		$(C_SOURCES) $(ASM_SOURCES) 


dri.a:	$(OBJECTS)
	rm -f $@
	ar rcv $@ $(OBJECTS)
	ranlib $@

# Remove .o and backup files
clean:
	-rm -f *.o */*.o *~ *.so *.a depend depend.bak

install:

include depend
