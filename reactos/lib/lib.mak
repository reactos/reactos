LIB_BASE = lib
LIB_BASE_ = $(LIB_BASE)$(SEP)

include lib/inflib/inflib.mak
include lib/3rdparty/zlib/zlib.mak
include lib/cmlib/cmlib.mak
ifeq ($(ARCH),powerpc)
include lib/ppcmmu/ppcmmu.mak
endif
