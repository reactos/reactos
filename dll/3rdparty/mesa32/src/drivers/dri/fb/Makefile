# src/mesa/drivers/dri/fb/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = fb_dri.so

ifeq ($(USING_EGL), 1)
EGL_SOURCES = server/radeon_egl.c
endif

DRIVER_SOURCES = \
	fb_dri.c \
	$(EGL_SOURCES)


C_SOURCES = \
	$(COMMON_SOURCES) \
	$(DRIVER_SOURCES)

ASM_SOURCES = 

# XXX not 100% sure this is right
#WINDOW_SYSTEM = solo

include ../Makefile.template

symlinks:

