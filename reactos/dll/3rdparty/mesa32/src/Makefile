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
		$(MAKE) directfb; \
	else \
		$(MAKE) stand-alone ; \
	fi

# BeOS driver target
beos: depend subdirs mesa.a
	cd drivers/beos; $(MAKE)


install: default
	@if [ "${DRIVER_DIRS}" = "dri" ] ; then \
		cd drivers/dri ; $(MAKE) install ; \
	fi

######################################################################
# Linux solo DRI drivers

SOLO_C_SOURCES = \
	$(MAIN_SOURCES)		\
	$(MATH_SOURCES)		\
	$(ARRAY_CACHE_SOURCES)	\
	$(TNL_SOURCES)		\
	$(SWRAST_SOURCES)	\
	$(SWRAST_SETUP_SOURCES)	\
	$(SHADER_SOURCES)       \
	$(SLANG_SOURCES)	\
	$(ASM_C_SOURCES)	

SOLO_OBJECTS = $(SOLO_C_SOURCES:.c=.o) \
		$(ASM_SOURCES:.S=.o)

linux-solo: depend subdirs mesa.a drivers-dri

# Make archive of core object files
mesa.a:	$(SOLO_OBJECTS)
	rm -f $@
	ar rcv $@ $(SOLO_OBJECTS)
	ranlib $@
	@if [ "${CONFIG_NAME}" = "beos" ] ; then \
		mimeset -f "$@" ; \
	fi

# note: cannot use the name of the directory ("drivers") as the
# target, otherwise make will look & see that the "drivers" directory
# already exists, so not try to rebuild it...
drivers-dri:
	cd drivers/dri ; $(MAKE)



#####################################################################
# Stand-alone Mesa libGL, no built-in drivers (DirectFB)

LIBGL_CORE_SOURCES =	\
	$(CORE_SOURCES) 	\
	$(ASM_SOURCES)

LIBGL_CORE_OBJECTS = 	\
	$(CORE_OBJECTS) 	\
	$(ASM_SOURCES:.S=.o)

# Make libGL from core object files
libgl-core: $(LIBGL_CORE_OBJECTS)
	@ $(TOP)/bin/mklib -o $(GL_LIB) -linker '$(CC)' \
		-major $(MESA_MAJOR) -minor $(MESA_MINOR) -patch $(MESA_TINY) \
		-install $(LIB_DIR) $(MKLIB_OPTIONS) $(LIBGL_CORE_OBJECTS) $(GL_LIB_DEPS)

# DirectFB driver target
directfb: depend subdirs libgl-core
	cd drivers/directfb; $(MAKE)



######################################################################
# Stand-alone Mesa libGL and libOSMesa

STAND_ALONE_DRIVER_SOURCES = \
	$(COMMON_DRIVER_SOURCES) \
	$(X11_DRIVER_SOURCES) \
	$(GLIDE_DRIVER_SOURCES) \
	$(SVGA_DRIVER_SOURCES) \
	$(FBDEV_DRIVER_SOURCES)

STAND_ALONE_SOURCES = \
	$(CORE_SOURCES) \
	$(ASM_SOURCES) \
	$(STAND_ALONE_DRIVER_SOURCES)

# ASM_SOURCES is defined in the config file
ASM_OBJECTS = $(ASM_SOURCES:.S=.o)

STAND_ALONE_DRIVER_OBJECTS = $(STAND_ALONE_DRIVER_SOURCES:.c=.o)

STAND_ALONE_OBJECTS = \
	$(CORE_OBJECTS) \
	$(ASM_OBJECTS) \
	$(STAND_ALONE_DRIVER_OBJECTS)

# For libOSMesa16 or libOSMesa32 we link _all_ the objects into the library,
# not just the osmesa.o object (i.e. we don't have a libGL).
OSMESA16_OBJECTS = \
	$(CORE_OBJECTS) \
	$(ASM_OBJECTS) \
	$(COMMON_DRIVER_OBJECTS) \
	$(OSMESA_DRIVER_OBJECTS)


stand-alone: depend subdirs $(LIB_DIR)/$(GL_LIB_NAME) $(LIB_DIR)/$(OSMESA_LIB_NAME)

osmesa-only: depend subdirs $(LIB_DIR)/$(OSMESA_LIB_NAME)

subdirs:
	@ (cd x86 ; $(MAKE))
	@ (cd x86-64 ; $(MAKE))

# Make the GL library
$(LIB_DIR)/$(GL_LIB_NAME): $(STAND_ALONE_OBJECTS)
	@ $(TOP)/bin/mklib -o $(GL_LIB) -linker '$(CC)' \
		-major $(GL_MAJOR) -minor $(GL_MINOR) -patch $(GL_TINY) \
		-install $(LIB_DIR) \
		$(MKLIB_OPTIONS) $(GL_LIB_DEPS) $(STAND_ALONE_OBJECTS)

# Make the OSMesa library
$(LIB_DIR)/$(OSMESA_LIB_NAME): $(OSMESA_DRIVER_OBJECTS) $(OSMESA16_OBJECTS)
	@ if [ "${DRIVER_DIRS}" = "osmesa" ] ; then \
		$(TOP)/bin/mklib -o $(OSMESA_LIB) -linker '$(CC)' \
			-major $(MESA_MAJOR) \
			-minor $(MESA_MINOR) -patch $(MESA_TINY) \
			-install $(LIB_DIR) $(MKLIB_OPTIONS) \
			$(OSMESA_LIB_DEPS) $(OSMESA16_OBJECTS) ; \
	else \
		$(TOP)/bin/mklib -o $(OSMESA_LIB) -linker '$(CC)' \
			-major $(MESA_MAJOR) \
			-minor $(MESA_MINOR) -patch $(GL_TINY) \
			-install $(LIB_DIR) $(MKLIB_OPTIONS) \
			$(OSMESA_LIB_DEPS) $(OSMESA_DRIVER_OBJECTS) ; \
	fi


######################################################################
# 16 (or 32) bit/channel OSMesa

# $(LIB_DIR)/$(OSMESA_LIB_NAME): $(OSMESA16_OBJECTS)
#	$(TOP)/bin/mklib -o $(OSMESA_LIB) \
#		-major $(MESA_MAJOR) -minor $(MESA_MINOR) -patch $(MESA_TINY) \
#		-install $(LIB_DIR) \
#		$(MKLIB_OPTIONS) $(GL_LIB_DEPS) $(OSMESA16_OBJECTS)



######################################################################
# Generic stuff

ALL_SOURCES = \
	$(GLAPI_SOURCES)	\
	$(MAIN_SOURCES)		\
	$(MATH_SOURCES)		\
	$(ARRAY_CACHE_SOURCES)	\
	$(TNL_SOURCES)		\
	$(SHADER_SOURCES)	\
	$(SWRAST_SOURCES)	\
	$(SWRAST_SETUP_SOURCES)	\
	$(ASM_C_SOURCES)	\
	$(X86_SOURCES)		\
	$(COMMON_DRIVER_SOURCES)\
	$(X11_DRIVER_SOURCES)	\
	$(OSMESA_DRIVER_SOURCES)


SUBDIRS = array_cache drivers/dri glapi main math sparc swrast swrast_setup \
	tnl tnl_dd


# Update dependencies
depend: $(ALL_SOURCES)
	@ echo "running $(MKDEP)"
	@ touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(DEFINES) $(INCLUDE_DIRS) $(ALL_SOURCES) \
		> /dev/null 


# Emacs tags
tags:
	etags `find . -name \*.[ch]` $(TOP)/include/GL/*.h

clean:
	-rm -f */*.o
	-rm -f */*/*.o
	-rm -f depend depend.bak mesa.a
	-rm -f drivers/*/*.o
	(cd drivers/dri ; $(MAKE) clean)
	(cd x86 ; $(MAKE) clean)
	(cd x86-64 ; $(MAKE) clean)

include depend
