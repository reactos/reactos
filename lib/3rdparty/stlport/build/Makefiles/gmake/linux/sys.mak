# Time-stamp: <06/11/10 23:43:27 ptr>
#
# Copyright (c) 1997-1999, 2002, 2003, 2005-2007
# Petr Ovtchenkov
#
# Portion Copyright (c) 1999-2001
# Parallel Graphics Ltd.
#
# Licensed under the Academic Free License version 3.0
#

INSTALL := /usr/bin/install

STRIP := /usr/bin/strip

install-strip:	_INSTALL_STRIP_OPTION = -s

install-strip:	_SO_STRIP_OPTION = -S

INSTALL_SO := ${INSTALL} -c -m 0755 ${_INSTALL_STRIP_OPTION}
INSTALL_A := ${INSTALL} -c -m 0644
INSTALL_EXE := ${INSTALL} -c -m 0755
INSTALL_D := ${INSTALL} -d -m 0755
INSTALL_F := ${INSTALL} -c -p -m 0644

# bash's built-in test is like extern
# EXT_TEST := /usr/bin/test
EXT_TEST := test
