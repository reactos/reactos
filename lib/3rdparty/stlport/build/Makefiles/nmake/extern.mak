# Time-stamp: <03/09/28 13:44:57 ptr>
# $Id$

#!ifndef MSVC_LIB_DIR
#MSVC_LIB_DIR = $(MSVC_DIR)\Lib
#!endif

!ifndef SOAP_DIR
SOAP_DIR = /opt/gSOAP-2.2.3
!endif
!ifndef BOOST_DIR
BOOST_DIR = $(SRCROOT)/../extern/boost
!endif
!ifndef STLPORT_DIR
STLPORT_DIR = ../../stlport
!endif
!ifndef CoMT_DIR
CoMT_DIR = $(SRCROOT)
!endif

# This is Complement project (really not extern):

!ifndef CoMT_LIB_DIR
CoMT_LIB_DIR        = $(INSTALL_LIB_DIR)
!endif
!ifndef CoMT_LIB_DIR_DBG
CoMT_LIB_DIR_DBG    = $(INSTALL_LIB_DIR_DBG)
!endif
!ifndef CoMT_LIB_DIR_STLDBG
CoMT_LIB_DIR_STLDBG = $(INSTALL_LIB_DIR_STLDBG)
!endif
!ifndef CoMT_BIN_DIR
CoMT_BIN_DIR        = $(INSTALL_BIN_DIR)
!endif
!ifndef CoMT_BIN_DIR_DBG
CoMT_BIN_DIR_DBG    = $(INSTALL_BIN_DIR_DBG)
!endif
!ifndef CoMT_BIN_DIR_STLDBG
CoMT_BIN_DIR_STLDBG = $(INSTALL_BIN_DIR_STLDBG)
!endif

!ifndef CoMT_INCLUDE_DIR
CoMT_INCLUDE_DIR = $(CoMT_DIR)/include
!endif

# This file reflect versions of third-party libraries that
# used in projects

# STLport library
!ifndef STLPORT_LIB_DIR
!ifdef CROSS_COMPILING
STLPORT_LIB_DIR = $(STLPORT_DIR)\lib\$(TARGET_NAME)
!else
STLPORT_LIB_DIR = $(STLPORT_DIR)\lib
!endif
!endif
!ifndef STLPORT_INCLUDE_DIR
STLPORT_INCLUDE_DIR = $(STLPORT_DIR)/stlport
!endif
!ifndef STLPORT_VER
STLPORT_VER = 4.5.5
!endif

# PostgreSQL library version:

#PG_INCLUDE ?= $(PG_DIR)/include
#PG_LIB ?= $(PG_DIR)/lib
#PG_LIB_VER_MAJOR = 2
#PG_LIB_VER_MINOR = 1

# Readline libraries version:

#RL_INCLUDE ?= /usr/local/include/readline
#RL_LIB ?= /usr/local/lib
#RL_LIB_VER_MAJOR = 4
#RL_LIB_VER_MINOR = 2

# gSOAP (http://gsoap2.sourceforge.net)

#gSOAP_INCLUDE_DIR ?= ${gSOAP_DIR}/include
#gSOAP_LIB_DIR ?= ${gSOAP_DIR}/lib
#gSOAP_BIN_DIR ?= ${gSOAP_DIR}/bin

# boost (http://www.boost.org, http://boost.sourceforge.net)
!ifdef STLP_BUILD_BOOST_PATH
INCLUDES=$(INCLUDES) /I$(STLP_BUILD_BOOST_PATH)
!endif

# This file reflect versions of third-party libraries that
# used in projects, with make-depend style

