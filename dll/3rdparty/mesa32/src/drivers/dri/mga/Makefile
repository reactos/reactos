# src/mesa/drivers/dri/mga/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = mga_dri.so

MINIGLX_SOURCES = server/mga_dri.c 

DRIVER_SOURCES = \
	mgadd.c \
	mgaioctl.c \
	mgarender.c \
	mgastate.c \
	mgatris.c \
	mgapixel.c \
	mgaspan.c \
	mgatex.c \
	mgatexmem.c \
	mga_texstate.c \
	mga_texcombine.c \
	mgavb.c \
	mga_xmesa.c

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(DRIVER_SOURCES) 


ASM_SOURCES = 

include ../Makefile.template

symlinks:
