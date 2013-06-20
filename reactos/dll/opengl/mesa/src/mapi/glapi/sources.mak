# src/mapi/glapi/sources.mak

GLAPI_SOURCES = \
	glapi_dispatch.c \
	glapi_entrypoint.c \
	glapi_gentable.c \
	glapi_getproc.c \
	glapi_nop.c \
	glthread.c \
	glapi.c

X86_API =			\
	glapi_x86.S

X86-64_API =			\
	glapi_x86-64.S

SPARC_API =			\
	glapi_sparc.S
