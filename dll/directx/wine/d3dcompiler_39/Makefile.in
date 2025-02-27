MODULE    = d3dcompiler_39.dll
IMPORTLIB = d3dcompiler_39
IMPORTS   = wined3d
EXTRADEFS = -DD3D_COMPILER_VERSION=39
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
