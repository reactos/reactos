# Time-stamp: <07/05/31 10:14:29 ptr>
#
# Copyright (c) 1997-1999, 2002, 2003, 2005, 2006
# Petr Ovtchenkov
#
# Portion Copyright (c) 1999-2001
# Parallel Graphics Ltd.
#
# Licensed under the Academic Free License version 3.0
#

# boost (http://www.boost.org, http://boost.sourceforge.net)

# ifdef BOOST_DIR
# BOOST_INCLUDE_DIR ?= ${BOOST_DIR}
# endif

ifdef STLP_BUILD_BOOST_PATH
BOOST_INCLUDE_DIR ?= ${STLP_BUILD_BOOST_PATH}
endif

# STLport library

ifndef WITHOUT_STLPORT
STLPORT_DIR ?= ${SRCROOT}/..
endif

ifdef STLPORT_DIR
STLPORT_LIB_DIR ?= $(STLPORT_DIR)/${TARGET_NAME}lib
STLPORT_INCLUDE_DIR ?= $(STLPORT_DIR)/stlport
endif
