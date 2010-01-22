LIB_BASE = sdk\libs
LIB_BASE_ = $(LIB_BASE)$(SEP)

include sdk/libs/inflib/inflib.mak
#ifeq ($(ARCH),powerpc)
#include lib/ppcmmu/ppcmmu.mak
#endif
