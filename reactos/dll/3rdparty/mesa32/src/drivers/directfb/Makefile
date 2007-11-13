# src/mesa/drivers/directfb/Makefile

TOP = ../../../..

include $(TOP)/configs/current


INCLUDE_DIRS = \
	-I$(TOP)/include \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mesa/main \
	-I$(TOP)/src/mesa/glapi \
	-I$(TOP)/src/mesa/math \
	-I$(TOP)/src/mesa/tnl \
	-I$(TOP)/src/mesa/shader \
	-I$(TOP)/src/mesa/swrast \
	-I$(TOP)/src/mesa/swrast_setup

DFB_CFLAGS    = $(shell pkg-config --cflags directfb)
DFB_MODULEDIR = $(shell pkg-config --variable=moduledir directfb-internal)

DIRECTFBGL_MESA_SOURCES = ../common/driverfuncs.c idirectfbgl_mesa.c

DIRECTFBGL_MESA_OBJECTS = $(DIRECTFBGL_MESA_SOURCES:.c=.o)

DIRECTFBGL_MESA = libidirectfbgl_mesa.so

.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $(DFB_CFLAGS) $< -o $@


default: directfbgl_mesa

# Mesa DirectFBGL module
directfbgl_mesa: $(DIRECTFBGL_MESA_OBJECTS)
	$(CC) -shared $(CFLAGS) $(DIRECTFBGL_MESA_OBJECTS) -o $(DIRECTFBGL_MESA) \
	-Wl,-soname -Wl,$(DIRECTFBGL_MESA) -L$(TOP)/$(LIB_DIR) -lGL -lm


install:
	@if test -d $(DFB_MODULEDIR); then \
		echo "Installing DirectFBGL module."; \
	else \
		echo "*** Failed to determine DirectFB module's directory."; \
		echo "*** Installation aborted."; \
		exit 1; \
	fi;
	test -d $(DFB_MODULEDIR)/interfaces/IDirectFBGL/ || mkdir $(DFB_MODULEDIR)/interfaces/IDirectFBGL/
	install -m 755 $(DIRECTFBGL_MESA) $(DFB_MODULEDIR)/interfaces/IDirectFBGL/


clean:
	rm -f *.o *.so

