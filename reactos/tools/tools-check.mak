PATH_TO_TOP = ..
include $(PATH_TO_TOP)/rules.mak

#
# Get the binutils version
#
# The "ld -v" output can be in either of these two formats:
#    "GNU ld version 050113 20050113" (nightly build)
#    "GNU ld version 2.15.94 20050118" (official release)
#    

BINUTILS_VERSION_DATE=$(word 5,$(shell $(PREFIX)ld -v))

all: 
	@echo #define BINUTILS_VERSION_DATE $(BINUTILS_VERSION_DATE) > tools-check.h
	$(HOST_CC) -c tools-check.c -o tools-check.temp
	$(RM) tools-check.temp
	$(RM) tools-check.h
