EXTLIB    = libvkd3d.a
EXTRAINCL = $(VKD3D_PE_CFLAGS) -I$(srcdir)/include/private -I$(srcdir)/libs/vkd3d -I$(srcdir)/libs/vkd3d-shader
EXTRADEFS = \
	-DWINE_NO_NAMELESS_EXTENSION \
	-DLIBVKD3D_SOURCE \
	-DLIBVKD3D_SHADER_SOURCE \
	-DLIBVKD3D_UTILS_SOURCE

SOURCES = \
	libs/vkd3d-common/blob.c \
	libs/vkd3d-common/debug.c \
	libs/vkd3d-common/error.c \
	libs/vkd3d-common/memory.c \
	libs/vkd3d-common/utf8.c \
	libs/vkd3d-shader/checksum.c \
	libs/vkd3d-shader/d3d_asm.c \
	libs/vkd3d-shader/d3dbc.c \
	libs/vkd3d-shader/dxbc.c \
	libs/vkd3d-shader/dxil.c \
	libs/vkd3d-shader/fx.c \
	libs/vkd3d-shader/glsl.c \
	libs/vkd3d-shader/hlsl.c \
	libs/vkd3d-shader/hlsl.l \
	libs/vkd3d-shader/hlsl.y \
	libs/vkd3d-shader/hlsl_codegen.c \
	libs/vkd3d-shader/hlsl_constant_ops.c \
	libs/vkd3d-shader/ir.c \
	libs/vkd3d-shader/msl.c \
	libs/vkd3d-shader/preproc.l \
	libs/vkd3d-shader/preproc.y \
	libs/vkd3d-shader/spirv.c \
	libs/vkd3d-shader/tpf.c \
	libs/vkd3d-shader/vkd3d_shader_main.c \
	libs/vkd3d/cache.c \
	libs/vkd3d/command.c \
	libs/vkd3d/device.c \
	libs/vkd3d/resource.c \
	libs/vkd3d/state.c \
	libs/vkd3d/utils.c \
	libs/vkd3d/vkd3d_main.c
