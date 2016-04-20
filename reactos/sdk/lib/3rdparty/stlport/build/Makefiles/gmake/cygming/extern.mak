# Time-stamp: <03/07/31 14:20:16 ptr>
# $Id: extern.mak 1459 2005-04-18 21:25:32Z ptr $

# This is Complement project (really not extern):

CoMT_LIB_DIR        ?= ${INSTALL_LIB_DIR}
CoMT_LIB_DIR_DBG    ?= ${INSTALL_LIB_DIR_DBG}
CoMT_LIB_DIR_STLDBG ?= ${INSTALL_LIB_DIR_STLDBG}
CoMT_BIN_DIR        ?= ${INSTALL_BIN_DIR}
CoMT_BIN_DIR_DBG    ?= ${INSTALL_BIN_DIR}
CoMT_BIN_DIR_STLDBG ?= ${INSTALL_BIN_DIR}

CoMT_INCLUDE_DIR ?= ${CoMT_DIR}/include

# This file reflect versions of third-party libraries that
# used in projects

# STLport library
#STLPORT_LIB_DIR ?= /usr/local/lib
#STLPORT_INCLUDE_DIR ?= /usr/local/include/stlport
#STLPORT_VER ?= 4.5
STLPORT_LIB_DIR ?= $(STLPORT_DIR)/lib
STLPORT_INCLUDE_DIR ?= $(STLPORT_DIR)/stlport
STLPORT_VER ?= 4.5.5

# PostgreSQL library version:

PG_INCLUDE ?= $(PG_DIR)/include
PG_LIB ?= $(PG_DIR)/lib
PG_LIB_VER_MAJOR = 2
PG_LIB_VER_MINOR = 1

# Readline libraries version:

RL_INCLUDE ?= /usr/local/include/readline
RL_LIB ?= /usr/local/lib
RL_LIB_VER_MAJOR = 4
RL_LIB_VER_MINOR = 2

# gSOAP (http://gsoap2.sourceforge.net)

gSOAP_INCLUDE_DIR ?= ${gSOAP_DIR}/include
gSOAP_LIB_DIR ?= ${gSOAP_DIR}/lib
gSOAP_BIN_DIR ?= ${gSOAP_DIR}/bin

# boost (http://www.boost.org, http://boost.sourceforge.net)
BOOST_INCLUDE_DIR ?= ${BOOST_DIR}

# This file reflect versions of third-party libraries that
# used in projects, with make-depend style

ifeq ($(OSNAME),sunos)
PG_DIR ?= /opt/PGpgsql
endif
ifeq ($(OSNAME),linux)
PG_DIR ?= /usr/local/pgsql
endif

gSOAP_DIR ?= /opt/gSOAP-2.2.3
BOOST_DIR ?= ${SRCROOT}/../extern/boost
STLPORT_DIR ?= e:/STLlab/STLport
CoMT_DIR ?= ${SRCROOT}

