MODULE    = d3dcompiler_40.dll
IMPORTS   = wined3d
EXTRADEFS = -DD3D_COMPILER_VERSION=40
PARENTSRC = ../d3dcompiler_43
EXTRAINCL = $(VKD3D_PE_CFLAGS)

EXTRADLLFLAGS = -Wb,--prefer-native

SOURCES = \
	asmparser.c \
	asmshader.l \
	asmshader.y \
	blob.c \
	bytecodewriter.c \
	compiler.c \
	reflection.c \
	utils.c \
	version.rc
