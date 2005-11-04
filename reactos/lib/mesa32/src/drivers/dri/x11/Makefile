# src/mesa/drivers/dri/x11/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = x11_dri.so

DRIVER_SOURCES = x11_dri.c \
		 $(TOP)/src/mesa/drivers/common/driverfuncs.c \
		 ../common/dri_util.c \
		 ../../x11/xm_api.c \
		 ../../x11/xm_dd.c \
		 ../../x11/xm_line.c \
		 ../../x11/xm_span.c \
		 ../../x11/xm_tri.c

C_SOURCES = \
	$(DRIVER_SOURCES) \
	$(DRI_SOURCES) 


# Include directories
INCLUDE_DIRS = \
	-I. \
	-I../common \
	-I../../x11 \
	-I../dri_client \
	-I../dri_client/imports \
	-Iserver \
	-I$(TOP)/include \
	-I$(DRM_SOURCE_PATH)/shared-core \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mesa/main \
	-I$(TOP)/src/mesa/glapi \
	-I$(TOP)/src/mesa/math \
	-I$(TOP)/src/mesa/transform \
	-I$(TOP)/src/mesa/shader \
	-I$(TOP)/src/mesa/swrast \
	-I$(TOP)/src/mesa/swrast_setup

# Core Mesa objects
MESA_MODULES = $(TOP)/src/mesa/mesa.a

# Libraries that the driver shared lib depends on
LIB_DEPS = -lm -lpthread -lc
# LIB_DEPS = -lGL -lm -lpthread -lc


ASM_SOURCES = 

OBJECTS = $(C_SOURCES:.c=.o) \
	  $(ASM_SOURCES:.S=.o) 


##### RULES #####

.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $(DEFINES) $< -o $@

.S.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $(DEFINES)  $< -o $@


##### TARGETS #####

default: depend $(LIB_DIR)/$(LIBNAME)


$(LIB_DIR)/$(LIBNAME): $(OBJECTS) $(MESA_MODULES) $(WINOBJ) Makefile
	CC="$(CC)" CXX="$(CXX)" $(TOP)/bin/mklib -o $(LIBNAME) -noprefix -install $(LIB_DIR) \
		$(OBJECTS) $(WINLIB) $(LIB_DEPS) $(WINOBJ) $(MESA_MODULES)


depend: $(C_SOURCES) $(ASM_SOURCES)
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(INCLUDE_DIRS) $(C_SOURCES) $(ASM_SOURCES) \
		> /dev/null 


# Emacs tags
tags:
	etags `find . -name \*.[ch]` `find ../include`


clean:
	-rm -f *.o server/*.o


include depend
