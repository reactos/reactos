# Time-stamp: <08/06/12 14:28:42 ptr>
#
# Copyright (c) 2004-2008
# Petr Ovtchenkov
#
# Licensed under the Academic Free License version 3.0
#

SRCROOT := build
SUBDIRS := build/lib

include ${SRCROOT}/Makefiles/gmake/subdirs.mak

all install depend clean clobber distclean check::
	+$(call doinsubdirs,${SUBDIRS})

distclean clean depend clobber::
	+$(call doinsubdirs,build/test/unit)

release-shared install-release-shared:
	+$(call doinsubdirs,${SUBDIRS})

install::
	${MAKE} -C build/lib install-headers

.PHONY: all install depend clean clobber distclean check release-shared install-release-shared
