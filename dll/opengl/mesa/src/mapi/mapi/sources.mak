# src/mapi/mapi/sources.mak
#
# mapi may be used in several ways
#
#  - In default mode, mapi implements the interface defined by mapi.h.  To use
#    this mode, compile MAPI_SOURCES.
#
#  - In util mode, mapi provides utility functions for use with glapi.  To use
#    this mode, compile MAPI_UTIL_SOURCES with MAPI_MODE_UTIL defined.
#
#  - In glapi mode, mapi implements the interface defined by glapi.h.  To use
#    this mode, compile MAPI_GLAPI_SOURCES with MAPI_MODE_GLAPI defined.
#
#  - In bridge mode, mapi provides entry points calling into glapi.  To use
#    this mode, compile MAPI_BRIDGE_SOURCES with MAPI_MODE_BRIDGE defined.

MAPI_UTIL_SOURCES = \
	u_current.c \
	u_execmem.c \
	u_thread.c

MAPI_SOURCES = \
	entry.c \
	mapi.c \
	stub.c \
	table.c \
	$(MAPI_UTIL_SOURCES)

MAPI_GLAPI_SOURCES = \
	entry.c \
	mapi_glapi.c \
	stub.c \
	table.c \
	$(MAPI_UTIL_SOURCES)

MAPI_BRIDGE_SOURCES = \
	entry.c
