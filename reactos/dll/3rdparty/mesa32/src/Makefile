# src/mesa/Makefile

TOP = ../..
include $(TOP)/configs/current

include sources


GL_MAJOR = 1
GL_MINOR = 5
GL_TINY = 0$(MESA_MAJOR)0$(MESA_MINOR)0$(MESA_TINY)


.SUFFIXES : .cpp

.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@

.cpp.o:
	$(CXX) -c $(INCLUDE_DIRS) $(CXXFLAGS) $< -o $@

.S.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@


# Figure out what to make here
default:
	@if [ "${DRIVER_DIRS}" = "dri" ] ; then \
		$(MAKE) linux-solo ; \
	elif [ "${DRIVER_DIRS}" = "osmesa" ] ; then \
		$(MAKE) osmesa-only ; \
	elif [ "$(DRIVER_DIRS)" = "beos" ]; then \
		$(MAKE) beos ; \
	elif [ "$(DRIVER_DIRS)" = "directfb" ]; then \
		$(MAKE) directfb ; \
	elif [ "$(DRIVER_DIRS)" = "fbdev osmesa" ]; then \
		$(MAKE) fbdev ; $(MAKE) osmesa-only ; \
	else \
		$(MAKE) stand-alone ; \
	fi


######################################################################
# BeOS driver target

beos: depend subdirs libmesa.a
	cd drivers/beos; $(MAKE)


######################################################################
# Linux DRI drivers

# Make archive of core object files
libmesa.a: $(SOLO_OBJECTS)
	@ $(TOP)/bin/mklib -o mesa -static $(SOLO_OBJECTS);
	@if [ "${CONFIG_NAME}" = "beos" ] ; then \
		mimeset -f "$@" ; \
	fi

linux-solo: depend subdirs libmesa.a
	cd drivers/dri ; $(MAKE)


#####################################################################
# Stand-alone Mesa libGL, no built-in drivers (DirectFB)

libgl-core: $(CORE_OBJECTS)
	@ $(TOP)/bin/mklib -o $(GL_LIB) -linker '$(CC)' \
		-major $(MESA_MAJOR) -minor $(MESA_MINOR) -patch $(MESA_TINY) \
		-install $(TOP)/$(LIB_DIR) $(MKLIB_OPTIONS) $(CORE_OBJECTS) \
		$(GL_LIB_DEPS)

directfb: depend subdirs libgl-core
	cd drivers/directfb ; $(MAKE)


#####################################################################
# fbdev Mesa driver (libGL.so)

fbdev: $(CORE_OBJECTS) $(FBDEV_DRIVER_OBJECTS) $(COMMON_DRIVER_OBJECTS)
	@ $(TOP)/bin/mklib -o $(GL_LIB) -linker '$(CC)' \
		-major $(MESA_MAJOR) -minor $(MESA_MINOR) -patch $(MESA_TINY) \
		-install $(TOP)/$(LIB_DIR) $(MKLIB_OPTIONS) \
		$(CORE_OBJECTS) $(FBDEV_DRIVER_OBJECTS) \
		$(COMMON_DRIVER_OBJECTS) $(GL_LIB_DEPS)


######################################################################
# Stand-alone Mesa libGL and libOSMesa
STAND_ALONE_DRIVER_SOURCES = \
	$(COMMON_DRIVER_SOURCES) \
	$(X11_DRIVER_SOURCES)

STAND_ALONE_DRIVER_OBJECTS = $(STAND_ALONE_DRIVER_SOURCES:.c=.o)

STAND_ALONE_OBJECTS = \
	$(CORE_OBJECTS) \
	$(STAND_ALONE_DRIVER_OBJECTS)

# For libOSMesa16 or libOSMesa32 we link _all_ the objects into the library,
# not just the osmesa.o object (i.e. we don't have a libGL).
OSMESA16_OBJECTS = \
	$(CORE_OBJECTS) \
	$(COMMON_DRIVER_OBJECTS) \
	$(OSMESA_DRIVER_OBJECTS)


stand-alone: depend subdirs $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME) $(TOP)/$(LIB_DIR)/$(OSMESA_LIB_NAME)

osmesa-only: depend subdirs $(TOP)/$(LIB_DIR)/$(OSMESA_LIB_NAME)

# Make the GL library
$(TOP)/$(LIB_DIR)/$(GL_LIB_NAME): $(STAND_ALONE_OBJECTS)
	@ $(TOP)/bin/mklib -o $(GL_LIB) -linker '$(CC)' \
		-major $(GL_MAJOR) -minor $(GL_MINOR) -patch $(GL_TINY) \
		-install $(TOP)/$(LIB_DIR) \
		$(MKLIB_OPTIONS) $(GL_LIB_DEPS) $(STAND_ALONE_OBJECTS)

# Make the OSMesa library
# Note: version is kept at 6.5.3 to simplify app/linking issues
$(TOP)/$(LIB_DIR)/$(OSMESA_LIB_NAME): $(OSMESA_DRIVER_OBJECTS) $(OSMESA16_OBJECTS)
	@ if [ "${DRIVER_DIRS}" = "osmesa" ] ; then \
		$(TOP)/bin/mklib -o $(OSMESA_LIB) -linker '$(CC)' \
			-major 6 -minor 5 -patch 3 \
			-install $(TOP)/$(LIB_DIR) $(MKLIB_OPTIONS) \
			$(OSMESA_LIB_DEPS) $(OSMESA16_OBJECTS) ; \
	else \
		$(TOP)/bin/mklib -o $(OSMESA_LIB) -linker '$(CC)' \
			-major 6 -minor 5 -patch 3 \
			-install $(TOP)/$(LIB_DIR) $(MKLIB_OPTIONS) \
			$(OSMESA_LIB_DEPS) $(OSMESA_DRIVER_OBJECTS) ; \
	fi


######################################################################
# Generic stuff

depend: $(ALL_SOURCES)
	@ echo "running $(MKDEP)"
	@ touch depend
	@$(MKDEP) $(MKDEP_OPTIONS) $(DEFINES) $(INCLUDE_DIRS) $(ALL_SOURCES) \
		> /dev/null 2>/dev/null


subdirs:
	@ (cd x86 ; $(MAKE))
	@ (cd x86-64 ; $(MAKE))


install: default
	$(INSTALL) -d $(INSTALL_DIR)/include/GL
	$(INSTALL) -d $(INSTALL_DIR)/$(LIB_DIR)
	$(INSTALL) -m 644 $(TOP)/include/GL/*.h $(INSTALL_DIR)/include/GL
	@if [ -e $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME) ]; then \
		$(INSTALL) $(TOP)/$(LIB_DIR)/libGL* $(INSTALL_DIR)/$(LIB_DIR); \
	fi
	@if [ -e $(TOP)/$(LIB_DIR)/$(OSMESA_LIB_NAME) ]; then \
		$(INSTALL) $(TOP)/$(LIB_DIR)/libOSMesa* $(INSTALL_DIR)/$(LIB_DIR); \
	fi
	@if [ "${DRIVER_DIRS}" = "dri" ] ; then \
		cd drivers/dri ; $(MAKE) install ; \
	fi

## NOT INSTALLED YET:
## $(INSTALL) -d $(INSTALL_DIR)/include/GLES
## $(INSTALL) -m 644 include/GLES/*.h $(INSTALL_DIR)/include/GLES


# Emacs tags
tags:
	etags `find . -name \*.[ch]` $(TOP)/include/GL/*.h

clean:
	-rm -f */*.o
	-rm -f */*/*.o
	-rm -f depend depend.bak libmesa.a
	-rm -f drivers/*/*.o
	(cd drivers/dri ; $(MAKE) clean)
	(cd x86 ; $(MAKE) clean)
	(cd x86-64 ; $(MAKE) clean)


include depend
