MODULE    = d3dcompiler_43.dll
IMPORTLIB = d3dcompiler_43
EXTRADEFS = -DD3D_COMPILER_VERSION=43
IMPORTS   = wined3d
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
