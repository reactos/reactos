# Time-stamp: <03/09/28 18:59:23 ptr>
# $Id$

# shared library:
SO  = dll
# The cooool Microsoft programmers pass LIB from line below into environment var!!!!
# LIB = lib
LIBEXT = lib
EXP = exp
# executable:
EXE = .exe

# static library extention:
ARCH = lib
AR = lib /nologo
AR_INS_R = 
AR_EXTR = 
AR_OUT = /out:$@

INSTALL = copy

INSTALL_SO = $(INSTALL)
INSTALL_A = $(INSTALL)
INSTALL_EXE = $(INSTALL)

# compiler, compiler options
!include $(RULESBASE)/$(USE_MAKE)/$(COMPILER_NAME).mak

