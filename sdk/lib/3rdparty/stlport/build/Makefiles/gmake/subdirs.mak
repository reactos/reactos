# Time-stamp: <06/11/01 22:55:23 ptr>
#
# Copyright (c) 2006, 2007
# Petr Ovtchenkov
#
# Licensed under the Academic Free License version 3.0
#

# Do the same target in all catalogs as arg
define doinsubdirs
$(foreach d,$(1),${MAKE} -C ${d} $@;)
endef
