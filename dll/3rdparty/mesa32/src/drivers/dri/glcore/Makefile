# src/mesa/drivers/dri/glcore/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = glcore_dri.so

DRIVER_SOURCES = glcore_driver.c \
		 $(TOP)/src/mesa/drivers/common/driverfuncs.c \
		 ../common/dri_util.c

C_SOURCES = \
	$(DRIVER_SOURCES) \
	$(DRI_SOURCES) 


# Include directories
INCLUDE_DIRS = \
	-I. \
	-I../common \
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
MESA_MODULES = $(TOP)/src/mesa/libmesa.a

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

default: depend $(TOP)/$(LIB_DIR)/$(LIBNAME)


$(TOP)/$(LIB_DIR)/$(LIBNAME): $(OBJECTS) $(MESA_MODULES) $(WINOBJ) Makefile
	CC="$(CC)" CXX="$(CXX)" $(TOP)/bin/mklib -o $(LIBNAME) -noprefix \
		-ldflags '$(LDFLAGS)' -install $(TOP)/$(LIB_DIR) \
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
