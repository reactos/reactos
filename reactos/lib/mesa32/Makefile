# configuration vars:
#

# MESA_HAVE_MMX
# MESA_HAVE_SSE
# MESA_HAVE_3DNOW
# MESA_HAVE_X86

#MESA_HAVE_MMX=1
#MESA_HAVE_SSE=1
#MESA_HAVE_3DNOW=1
#MESA_HAVE_X86=1

MESA_INCLUDE_DIRS = \
	-Iinclude \
	-Isrc \
	-Isrc/main \
	-Isrc/glapi \
	-Isrc/math \
	-Isrc/tnl \
	-Isrc/swrast \
	-Isrc/swrast_setup

MESA_MAIN_OBJECTS = \
	src/main/api_arrayelt.o \
	src/main/api_loopback.o \
	src/main/api_noop.o \
	src/main/api_validate.o \
	src/main/accum.o \
	src/main/arbparse.o \
	src/main/arbprogram.o \
	src/main/arbfragparse.o \
	src/main/arbvertparse.o \
	src/main/attrib.o \
	src/main/blend.o \
	src/main/bufferobj.o \
	src/main/buffers.o \
	src/main/clip.o \
	src/main/colortab.o \
	src/main/context.o \
	src/main/convolve.o \
	src/main/debug.o \
	src/main/depth.o \
	src/main/dispatch.o \
	src/main/dlist.o \
	src/main/drawpix.o \
	src/main/enable.o \
	src/main/enums.o \
	src/main/eval.o \
	src/main/extensions.o \
	src/main/feedback.o \
	src/main/fog.o \
	src/main/get.o \
	src/main/hash.o \
	src/main/hint.o \
	src/main/histogram.o \
	src/main/image.o \
	src/main/imports.o \
	src/main/light.o \
	src/main/lines.o \
	src/main/matrix.o \
	src/main/nvprogram.o \
	src/main/nvfragparse.o \
	src/main/nvvertexec.o \
	src/main/nvvertparse.o \
	src/main/occlude.o \
	src/main/pixel.o \
	src/main/points.o \
	src/main/polygon.o \
	src/main/program.o \
	src/main/rastpos.o \
	src/main/state.o \
	src/main/stencil.o \
	src/main/texcompress.o \
	src/main/texformat.o \
	src/main/teximage.o \
	src/main/texobj.o \
	src/main/texstate.o \
	src/main/texstore.o \
	src/main/texutil.o \
	src/main/varray.o \
	src/main/vtxfmt.o

MESA_GLAPI_OBJECTS = \
	src/glapi/glapi.o \
	src/glapi/glthread.o

MESA_MATH_OBJECTS = \
	src/math/m_debug_clip.o \
	src/math/m_debug_norm.o \
	src/math/m_debug_xform.o \
	src/math/m_eval.o \
	src/math/m_matrix.o \
	src/math/m_translate.o \
	src/math/m_vector.o \
	src/math/m_xform.o

MESA_ARRAY_CACHE_OBJECTS = \
	src/array_cache/ac_context.o \
	src/array_cache/ac_import.o

MESA_SWRAST_OBJECTS = \
	src/swrast/s_aaline.o \
	src/swrast/s_aatriangle.o \
	src/swrast/s_accum.o \
	src/swrast/s_alpha.o \
	src/swrast/s_alphabuf.o \
	src/swrast/s_bitmap.o \
	src/swrast/s_blend.o \
	src/swrast/s_buffers.o \
	src/swrast/s_copypix.o \
	src/swrast/s_context.o \
	src/swrast/s_depth.o \
	src/swrast/s_drawpix.o \
	src/swrast/s_feedback.o \
	src/swrast/s_fog.o \
	src/swrast/s_imaging.o \
	src/swrast/s_lines.o \
	src/swrast/s_logic.o \
	src/swrast/s_masking.o \
	src/swrast/s_nvfragprog.o \
	src/swrast/s_pixeltex.o \
	src/swrast/s_points.o \
	src/swrast/s_readpix.o \
	src/swrast/s_span.o \
	src/swrast/s_stencil.o \
	src/swrast/s_texture.o \
	src/swrast/s_texstore.o \
	src/swrast/s_triangle.o \
	src/swrast/s_zoom.o

MESA_SWRAST_SETUP_OBJECTS = \
	src/swrast_setup/ss_context.o \
	src/swrast_setup/ss_triangle.o

MESA_TNL_OBJECTS = \
	src/tnl/t_array_api.o \
	src/tnl/t_array_import.o \
	src/tnl/t_context.o \
	src/tnl/t_pipeline.o \
	src/tnl/t_save_api.o \
	src/tnl/t_save_loopback.o \
	src/tnl/t_save_playback.o \
	src/tnl/t_vb_fog.o \
	src/tnl/t_vb_light.o \
	src/tnl/t_vb_normals.o \
	src/tnl/t_vb_points.o \
	src/tnl/t_vb_program.o \
	src/tnl/t_vb_render.o \
	src/tnl/t_vb_texgen.o \
	src/tnl/t_vb_texmat.o \
	src/tnl/t_vb_vertex.o \
	src/tnl/t_vertex.o \
	src/tnl/t_vtx_api.o \
	src/tnl/t_vtx_eval.o \
	src/tnl/t_vtx_exec.o 

MESA_SPARC_OBJECTS =			\
	src/sparc/clip.o		\
	src/sparc/glapi_sparc.o	\
	src/sparc/norm.o		\
	src/sparc/xform.o

MESA_X86_OBJECTS = \
	src/x86/x86.o \
	src/x86/glapi_x86.o \
	src/x86/common_x86.o \
	src/x86/common_x86_asm.o \
	src/x86/x86_xform2.o \
	src/x86/x86_xform3.o \
	src/x86/x86_xform4.o \
	src/x86/x86_cliptest.o

MESA_X86_MMX_OBJECTS = \
	src/x86/mmx_blend.o

MESA_X86_SSE_OBJECTS = \
	src/x86/sse.o \
	src/x86/sse_xform2.o \
	src/x86/sse_xform3.o \
	src/x86/sse_xform4.o \
	src/x86/sse_normal.o

MESA_X86_K3D_OBJECTS = \
	src/x86/3dnow.o \
	src/x86/3dnow_xform2.o \
	src/x86/3dnow_xform3.o \
	src/x86/3dnow_xform4.o \
	src/x86/3dnow_normal.o

MESA_CORE_OBJECTS = \
	$(MESA_MAIN_OBJECTS) \
	$(MESA_GLAPI_OBJECTS) \
	$(MESA_MATH_OBJECTS) \
	$(MESA_ARRAY_CACHE_OBJECTS) \
	$(MESA_TNL_OBJECTS) \
	$(MESA_SWRAST_OBJECTS) \
	$(MESA_SWRAST_SETUP_OBJECTS)

MESA_DRIVER_OBJECTS = \
	src/drivers/windows/gdi/wmesa.o \
	src/drivers/windows/icd/icd.o

MESA_CFLAGS = -DBUILD_GL32 -D_OPENGL32_ -D_MSC_VER -DUSE_MGL_NAMESPACE $(MESA_INCLUDE_DIRS)

ifeq ($(MESA_HAVE_MMX),1)
MESA_X86_OBJECTS += $(MESA_X86_MMX_OBJECTS)
MESA_CFLAGS += -DUSE_MMX_ASM
MESA_HAVE_X86 = 1
endif

ifeq ($(MESA_HAVE_SSE),1)
MESA_X86_OBJECTS += $(MESA_X86_SSE_OBJECTS)
MESA_CFLAGS += -DUSE_SSE_ASM
MESA_HAVE_X86 = 1
endif

ifeq ($(MESA_HAVE_3DNOW),1)
MESA_X86_OBJECTS += $(MESA_X86_K3D_OBJECTS)
MESA_CFLAGS += -DUSE_3DNOW_ASM
MESA_HAVE_X86 = 1
endif

ifeq ($(MESA_HAVE_X86),1)
MESA_CFLAGS += -DUSE_X86_ASM
else
MESA_X86_OBJECTS =
endif


PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NORC = yes

TARGET_NAME = mesa32

TARGET_BASE = $(TARGET_BASE_LIB_MESA32)

TARGET_CFLAGS = -D__USE_W32API -Wall $(MESA_CFLAGS)

TARGET_GCCLIBS = gdi32

TARGET_OBJECTS = $(MESA_CORE_OBJECTS) $(MESA_X86_OBJECTS) $(MESA_DRIVER_OBJECTS)


DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

#include $(TOOLS_PATH)/depend.mk

# EOF
