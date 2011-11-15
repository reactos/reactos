# Time-stamp: <05/09/09 21:12:38 ptr>
# $Id: sys.mak 1802 2005-11-01 08:25:57Z complement $

RC := windres
INSTALL := install

INSTALL_SO := ${INSTALL} -m 0755
INSTALL_A := ${INSTALL} -m 0644
INSTALL_EXE := ${INSTALL} -m 0755

EXT_TEST := test
