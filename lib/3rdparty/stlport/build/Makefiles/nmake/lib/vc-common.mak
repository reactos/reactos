# -*- makefile -*- Time-stamp: <03/10/17 14:09:57 ptr>
# $Id$


# Oh, the commented below work for gmake 3.78.1 and above,
# but phrase without tag not work for it. Since gmake 3.79 
# tag with assignment fail, but work assignment for all tags
# (really that more correct).

!ifndef LDLIBS
LDLIBS =
!endif

#Per default MSVC vcvars32.bat script set the LIB environment
#variable to get the native library, there is no need to add
#them here
#LDSEARCH = $(LDSEARCH) /LIBPATH:"$(MSVC_LIB_DIR)"

LDFLAGS_REL = $(LDFLAGS_REL) /dll $(LDSEARCH)
LDFLAGS_DBG = $(LDFLAGS_DBG) /dll $(LDSEARCH)
LDFLAGS_STLDBG = $(LDFLAGS_STLDBG) /dll $(LDSEARCH)
# LDFLAGS_STATIC = $(LDSEARCH)

LDFLAGS_REL = $(LDFLAGS_REL) /version:$(MAJOR).$(MINOR)
LDFLAGS_DBG = $(LDFLAGS_DBG) /version:$(MAJOR).$(MINOR)
LDFLAGS_STLDBG = $(LDFLAGS_STLDBG) /version:$(MAJOR).$(MINOR)
