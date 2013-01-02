/*
 * Mesa 3-D graphics library
 * Version:  7.6
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * \file
 * \brief Extension handling
 */


#include "glheader.h"
#include "imports.h"
#include "context.h"
#include "extensions.h"
#include "mfeatures.h"
#include "mtypes.h"

#define ALIGN(value, alignment)  (((value) + alignment - 1) & ~(alignment - 1))

enum {
   DISABLE = 0,
   GL  = 1 << API_OPENGL,
   ES1 = 1 << API_OPENGLES,
   ES2 = 1 << API_OPENGLES2,
};

/**
 * \brief An element of the \c extension_table.
 */
struct extension {
   /** Name of extension, such as "GL_ARB_depth_clamp". */
   const char *name;

   /** Offset (in bytes) of the corresponding member in struct gl_extensions. */
   size_t offset;

   /** Set of API's in which the extension exists, as a bitset. */
   uint8_t api_set;

   /** Year the extension was proposed or approved.  Used to sort the 
    * extension string chronologically. */
   uint16_t year;
};


/**
 * Given a member \c x of struct gl_extensions, return offset of
 * \c x in bytes.
 */
#define o(x) offsetof(struct gl_extensions, x)


/**
 * \brief Table of supported OpenGL extensions for all API's.
 */
static const struct extension extension_table[] = {
   /* ARB Extensions */
   { "GL_ARB_ES2_compatibility",                   o(ARB_ES2_compatibility),                   GL,             2009 },
   { "GL_ARB_blend_func_extended",                 o(ARB_blend_func_extended),                 GL,             2009 },
   { "GL_ARB_color_buffer_float",                  o(ARB_color_buffer_float),                  GL,             2004 },
   { "GL_ARB_copy_buffer",                         o(ARB_copy_buffer),                         GL,             2008 },
   { "GL_ARB_conservative_depth",                  o(ARB_conservative_depth),                  GL,             2011 },
   { "GL_ARB_depth_buffer_float",                  o(ARB_depth_buffer_float),                  GL,             2008 },
   { "GL_ARB_depth_clamp",                         o(ARB_depth_clamp),                         GL,             2003 },
   { "GL_ARB_depth_texture",                       o(ARB_depth_texture),                       GL,             2001 },
   { "GL_ARB_draw_buffers",                        o(dummy_true),                              GL,             2002 },
   { "GL_ARB_draw_buffers_blend",                  o(ARB_draw_buffers_blend),                  GL,             2009 },
   { "GL_ARB_draw_elements_base_vertex",           o(ARB_draw_elements_base_vertex),           GL,             2009 },
   { "GL_ARB_draw_instanced",                      o(ARB_draw_instanced),                      GL,             2008 },
   { "GL_ARB_explicit_attrib_location",            o(ARB_explicit_attrib_location),            GL,             2009 },
   { "GL_ARB_fragment_coord_conventions",          o(ARB_fragment_coord_conventions),          GL,             2009 },
   { "GL_ARB_fragment_program",                    o(ARB_fragment_program),                    GL,             2002 },
   { "GL_ARB_fragment_program_shadow",             o(ARB_fragment_program_shadow),             GL,             2003 },
   { "GL_ARB_fragment_shader",                     o(ARB_fragment_shader),                     GL,             2002 },
   { "GL_ARB_framebuffer_object",                  o(ARB_framebuffer_object),                  GL,             2005 },
   { "GL_ARB_framebuffer_sRGB",                    o(EXT_framebuffer_sRGB),                    GL,             1998 },
   { "GL_ARB_half_float_pixel",                    o(ARB_half_float_pixel),                    GL,             2003 },
   { "GL_ARB_half_float_vertex",                   o(ARB_half_float_vertex),                   GL,             2008 },
   { "GL_ARB_instanced_arrays",                    o(ARB_instanced_arrays),                    GL,             2008 },
   { "GL_ARB_map_buffer_range",                    o(ARB_map_buffer_range),                    GL,             2008 },
   { "GL_ARB_multisample",                         o(dummy_true),                              GL,             1994 },
   { "GL_ARB_multitexture",                        o(dummy_true),                              GL,             1998 },
   { "GL_ARB_occlusion_query2",                    o(ARB_occlusion_query2),                    GL,             2003 },
   { "GL_ARB_occlusion_query",                     o(ARB_occlusion_query),                     GL,             2001 },
   { "GL_ARB_pixel_buffer_object",                 o(EXT_pixel_buffer_object),                 GL,             2004 },
   { "GL_ARB_point_parameters",                    o(EXT_point_parameters),                    GL,             1997 },
   { "GL_ARB_point_sprite",                        o(ARB_point_sprite),                        GL,             2003 },
   { "GL_ARB_provoking_vertex",                    o(EXT_provoking_vertex),                    GL,             2009 },
   { "GL_ARB_robustness",                          o(dummy_true),                              GL,             2010 },
   { "GL_ARB_sampler_objects",                     o(ARB_sampler_objects),                     GL,             2009 },
   { "GL_ARB_seamless_cube_map",                   o(ARB_seamless_cube_map),                   GL,             2009 },
   { "GL_ARB_shader_objects",                      o(ARB_shader_objects),                      GL,             2002 },
   { "GL_ARB_shader_stencil_export",               o(ARB_shader_stencil_export),               GL,             2009 },
   { "GL_ARB_shader_texture_lod",                  o(ARB_shader_texture_lod),                  GL,             2009 },
   { "GL_ARB_shading_language_100",                o(ARB_shading_language_100),                GL,             2003 },
   { "GL_ARB_shadow_ambient",                      o(ARB_shadow_ambient),                      GL,             2001 },
   { "GL_ARB_shadow",                              o(ARB_shadow),                              GL,             2001 },
   { "GL_ARB_sync",                                o(ARB_sync),                                GL,             2003 },
   { "GL_ARB_texture_border_clamp",                o(ARB_texture_border_clamp),                GL,             2000 },
   { "GL_ARB_texture_buffer_object",               o(ARB_texture_buffer_object),               GL,             2008 },
   { "GL_ARB_texture_compression",                 o(dummy_true),                              GL,             2000 },
   { "GL_ARB_texture_compression_rgtc",            o(ARB_texture_compression_rgtc),            GL,             2004 },
   { "GL_ARB_texture_cube_map",                    o(ARB_texture_cube_map),                    GL,             1999 },
   { "GL_ARB_texture_env_add",                     o(dummy_true),                              GL,             1999 },
   { "GL_ARB_texture_env_combine",                 o(ARB_texture_env_combine),                 GL,             2001 },
   { "GL_ARB_texture_env_crossbar",                o(ARB_texture_env_crossbar),                GL,             2001 },
   { "GL_ARB_texture_env_dot3",                    o(ARB_texture_env_dot3),                    GL,             2001 },
   { "GL_ARB_texture_float",                       o(ARB_texture_float),                       GL,             2004 },
   { "GL_ARB_texture_mirrored_repeat",             o(dummy_true),                              GL,             2001 },
   { "GL_ARB_texture_multisample",                 o(ARB_texture_multisample),                 GL,             2009 },
   { "GL_ARB_texture_non_power_of_two",            o(ARB_texture_non_power_of_two),            GL,             2003 },
   { "GL_ARB_texture_rectangle",                   o(NV_texture_rectangle),                    GL,             2004 },
   { "GL_ARB_texture_rgb10_a2ui",                  o(ARB_texture_rgb10_a2ui),                  GL,             2009 },
   { "GL_ARB_texture_rg",                          o(ARB_texture_rg),                          GL,             2008 },
   { "GL_ARB_texture_storage",                     o(ARB_texture_storage),                     GL,             2011 },
   { "GL_ARB_texture_swizzle",                     o(EXT_texture_swizzle),                     GL,             2008 },
   { "GL_ARB_transform_feedback2",                 o(ARB_transform_feedback2),                 GL,             2010 },
   { "GL_ARB_transpose_matrix",                    o(ARB_transpose_matrix),                    GL,             1999 },
   { "GL_ARB_uniform_buffer_object",               o(ARB_uniform_buffer_object),               GL,             2002 },
   { "GL_ARB_vertex_array_bgra",                   o(EXT_vertex_array_bgra),                   GL,             2008 },
   { "GL_ARB_vertex_array_object",                 o(ARB_vertex_array_object),                 GL,             2006 },
   { "GL_ARB_vertex_buffer_object",                o(dummy_true),                              GL,             2003 },
   { "GL_ARB_vertex_program",                      o(ARB_vertex_program),                      GL,             2002 },
   { "GL_ARB_vertex_shader",                       o(ARB_vertex_shader),                       GL,             2002 },
   { "GL_ARB_vertex_type_2_10_10_10_rev",          o(ARB_vertex_type_2_10_10_10_rev),          GL,             2009 },
   { "GL_ARB_window_pos",                          o(ARB_window_pos),                          GL,             2001 },
   /* EXT extensions */
   { "GL_EXT_abgr",                                o(dummy_true),                              GL,             1995 },
   { "GL_EXT_bgra",                                o(dummy_true),                              GL,             1995 },
   { "GL_EXT_blend_color",                         o(EXT_blend_color),                         GL,             1995 },
   { "GL_EXT_blend_equation_separate",             o(EXT_blend_equation_separate),             GL,             2003 },
   { "GL_EXT_blend_func_separate",                 o(EXT_blend_func_separate),                 GL,             1999 },
   { "GL_EXT_blend_minmax",                        o(EXT_blend_minmax),                        GL | ES1 | ES2, 1995 },
   { "GL_EXT_blend_subtract",                      o(dummy_true),                              GL,             1995 },
   { "GL_EXT_clip_volume_hint",                    o(EXT_clip_volume_hint),                    GL,             1996 },
   { "GL_EXT_compiled_vertex_array",               o(EXT_compiled_vertex_array),               GL,             1996 },
   { "GL_EXT_copy_texture",                        o(dummy_true),                              GL,             1995 },
   { "GL_EXT_depth_bounds_test",                   o(EXT_depth_bounds_test),                   GL,             2002 },
   { "GL_EXT_draw_buffers2",                       o(EXT_draw_buffers2),                       GL,             2006 },
   { "GL_EXT_draw_instanced",                      o(ARB_draw_instanced),                      GL,             2006 },
   { "GL_EXT_draw_range_elements",                 o(EXT_draw_range_elements),                 GL,             1997 },
   { "GL_EXT_fog_coord",                           o(EXT_fog_coord),                           GL,             1999 },
   { "GL_EXT_framebuffer_blit",                    o(EXT_framebuffer_blit),                    GL,             2005 },
   { "GL_EXT_framebuffer_multisample",             o(EXT_framebuffer_multisample),             GL,             2005 },
   { "GL_EXT_framebuffer_object",                  o(EXT_framebuffer_object),                  GL,             2000 },
   { "GL_EXT_framebuffer_sRGB",                    o(EXT_framebuffer_sRGB),                    GL,             1998 },
   { "GL_EXT_gpu_program_parameters",              o(EXT_gpu_program_parameters),              GL,             2006 },
   { "GL_EXT_gpu_shader4",                         o(EXT_gpu_shader4),                         GL,             2006 },
   { "GL_EXT_multi_draw_arrays",                   o(dummy_true),                              GL | ES1 | ES2, 1999 },
   { "GL_EXT_packed_depth_stencil",                o(EXT_packed_depth_stencil),                GL,             2005 },
   { "GL_EXT_packed_float",                        o(EXT_packed_float),                        GL,             2004 },
   { "GL_EXT_packed_pixels",                       o(EXT_packed_pixels),                       GL,             1997 },
   { "GL_EXT_pixel_buffer_object",                 o(EXT_pixel_buffer_object),                 GL,             2004 },
   { "GL_EXT_point_parameters",                    o(EXT_point_parameters),                    GL,             1997 },
   { "GL_EXT_polygon_offset",                      o(dummy_true),                              GL,             1995 },
   { "GL_EXT_provoking_vertex",                    o(EXT_provoking_vertex),                    GL,             2009 },
   { "GL_EXT_rescale_normal",                      o(EXT_rescale_normal),                      GL,             1997 },
   { "GL_EXT_secondary_color",                     o(EXT_secondary_color),                     GL,             1999 },
   { "GL_EXT_separate_shader_objects",             o(EXT_separate_shader_objects),             GL,             2008 },
   { "GL_EXT_separate_specular_color",             o(EXT_separate_specular_color),             GL,             1997 },
   { "GL_EXT_shadow_funcs",                        o(EXT_shadow_funcs),                        GL,             2002 },
   { "GL_EXT_stencil_two_side",                    o(EXT_stencil_two_side),                    GL,             2001 },
   { "GL_EXT_stencil_wrap",                        o(dummy_true),                              GL,             2002 },
   { "GL_EXT_subtexture",                          o(dummy_true),                              GL,             1995 },
   { "GL_EXT_texture3D",                           o(EXT_texture3D),                           GL,             1996 },
   { "GL_EXT_texture_array",                       o(EXT_texture_array),                       GL,             2006 },
   { "GL_EXT_texture_compression_dxt1",            o(EXT_texture_compression_s3tc),            GL | ES1 | ES2, 2004 },
   { "GL_EXT_texture_compression_latc",            o(EXT_texture_compression_latc),            GL,             2006 },
   { "GL_EXT_texture_compression_rgtc",            o(ARB_texture_compression_rgtc),            GL,             2004 },
   { "GL_EXT_texture_compression_s3tc",            o(EXT_texture_compression_s3tc),            GL,             2000 },
   { "GL_EXT_texture_cube_map",                    o(ARB_texture_cube_map),                    GL,             2001 },
   { "GL_EXT_texture_edge_clamp",                  o(dummy_true),                              GL,             1997 },
   { "GL_EXT_texture_env_add",                     o(dummy_true),                              GL,             1999 },
   { "GL_EXT_texture_env_combine",                 o(dummy_true),                              GL,             2000 },
   { "GL_EXT_texture_env_dot3",                    o(EXT_texture_env_dot3),                    GL,             2000 },
   { "GL_EXT_texture_filter_anisotropic",          o(EXT_texture_filter_anisotropic),          GL | ES1 | ES2, 1999 },
   { "GL_EXT_texture_format_BGRA8888",             o(dummy_true),                                   ES1 | ES2, 2005 },
   { "GL_EXT_texture_integer",                     o(EXT_texture_integer),                     GL,             2006 },
   { "GL_EXT_texture_lod_bias",                    o(dummy_true),                              GL | ES1,       1999 },
   { "GL_EXT_texture_mirror_clamp",                o(EXT_texture_mirror_clamp),                GL,             2004 },
   { "GL_EXT_texture_object",                      o(dummy_true),                              GL,             1995 },
   { "GL_EXT_texture",                             o(dummy_true),                              GL,             1996 },
   { "GL_EXT_texture_rectangle",                   o(NV_texture_rectangle),                    GL,             2004 },
   { "GL_EXT_texture_shared_exponent",             o(EXT_texture_shared_exponent),             GL,             2004 },
   { "GL_EXT_texture_snorm",                       o(EXT_texture_snorm),                       GL,             2009 },
   { "GL_EXT_texture_sRGB",                        o(EXT_texture_sRGB),                        GL,             2004 },
   { "GL_EXT_texture_sRGB_decode",                 o(EXT_texture_sRGB_decode),                        GL,      2006 },
   { "GL_EXT_texture_swizzle",                     o(EXT_texture_swizzle),                     GL,             2008 },
   { "GL_EXT_texture_type_2_10_10_10_REV",         o(dummy_true),                                         ES2, 2008 },
   { "GL_EXT_timer_query",                         o(EXT_timer_query),                         GL,             2006 },
   { "GL_EXT_transform_feedback",                  o(EXT_transform_feedback),                  GL,             2011 },
   { "GL_EXT_vertex_array_bgra",                   o(EXT_vertex_array_bgra),                   GL,             2008 },
   { "GL_EXT_vertex_array",                        o(dummy_true),                              GL,             1995 },

   /* OES extensions */
   { "GL_OES_blend_equation_separate",             o(EXT_blend_equation_separate),                  ES1,       2009 },
   { "GL_OES_blend_func_separate",                 o(EXT_blend_func_separate),                      ES1,       2009 },
   { "GL_OES_blend_subtract",                      o(dummy_true),                                   ES1,       2009 },
   { "GL_OES_byte_coordinates",                    o(dummy_true),                                   ES1,       2002 },
   { "GL_OES_compressed_ETC1_RGB8_texture",        o(OES_compressed_ETC1_RGB8_texture),             ES1 | ES2, 2005 },
   { "GL_OES_compressed_paletted_texture",         o(dummy_true),                                   ES1,       2003 },
   { "GL_OES_depth24",                             o(EXT_framebuffer_object),                       ES1 | ES2, 2005 },
   { "GL_OES_depth32",                             o(dummy_false),                     DISABLE,                2005 },
   { "GL_OES_depth_texture",                       o(ARB_depth_texture),                                  ES2, 2006 },
#if FEATURE_OES_draw_texture
   { "GL_OES_draw_texture",                        o(OES_draw_texture),                             ES1 | ES2, 2004 },
#endif
#if FEATURE_OES_EGL_image
   /*  FIXME: Mesa expects GL_OES_EGL_image to be available in OpenGL contexts. */
   { "GL_OES_EGL_image",                           o(OES_EGL_image),                           GL | ES1 | ES2, 2006 },
   { "GL_OES_EGL_image_external",                  o(OES_EGL_image_external),                       ES1 | ES2, 2010 },
#endif
   { "GL_OES_element_index_uint",                  o(dummy_true),                                   ES1 | ES2, 2005 },
   { "GL_OES_fbo_render_mipmap",                   o(EXT_framebuffer_object),                       ES1 | ES2, 2005 },
   { "GL_OES_fixed_point",                         o(dummy_true),                                   ES1,       2002 },
   { "GL_OES_framebuffer_object",                  o(EXT_framebuffer_object),                       ES1,       2005 },
   { "GL_OES_mapbuffer",                           o(dummy_true),                                   ES1 | ES2, 2005 },
   { "GL_OES_matrix_get",                          o(dummy_true),                                   ES1,       2004 },
   { "GL_OES_packed_depth_stencil",                o(EXT_packed_depth_stencil),                     ES1 | ES2, 2007 },
   { "GL_OES_point_size_array",                    o(dummy_true),                                   ES1,       2004 },
   { "GL_OES_point_sprite",                        o(ARB_point_sprite),                             ES1,       2004 },
   { "GL_OES_query_matrix",                        o(dummy_true),                                   ES1,       2003 },
   { "GL_OES_read_format",                         o(dummy_true),                              GL | ES1,       2003 },
   { "GL_OES_rgb8_rgba8",                          o(EXT_framebuffer_object),                       ES1 | ES2, 2005 },
   { "GL_OES_single_precision",                    o(dummy_true),                                   ES1,       2003 },
   { "GL_OES_standard_derivatives",                o(OES_standard_derivatives),                           ES2, 2005 },
   { "GL_OES_stencil1",                            o(dummy_false),                     DISABLE,                2005 },
   { "GL_OES_stencil4",                            o(dummy_false),                     DISABLE,                2005 },
   { "GL_OES_stencil8",                            o(EXT_framebuffer_object),                       ES1 | ES2, 2005 },
   { "GL_OES_stencil_wrap",                        o(dummy_true),                                   ES1,       2002 },
   { "GL_OES_texture_3D",                          o(EXT_texture3D),                                      ES2, 2005 },
   { "GL_OES_texture_cube_map",                    o(ARB_texture_cube_map),                         ES1,       2007 },
   { "GL_OES_texture_env_crossbar",                o(ARB_texture_env_crossbar),                     ES1,       2005 },
   { "GL_OES_texture_mirrored_repeat",             o(dummy_true),                                   ES1,       2005 },
   { "GL_OES_texture_npot",                        o(ARB_texture_non_power_of_two),                       ES2, 2005 },

   /* Vendor extensions */
   { "GL_3DFX_texture_compression_FXT1",           o(TDFX_texture_compression_FXT1),           GL,             1999 },
   { "GL_AMD_conservative_depth",                  o(ARB_conservative_depth),                  GL,             2009 },
   { "GL_AMD_draw_buffers_blend",                  o(ARB_draw_buffers_blend),                  GL,             2009 },
   { "GL_AMD_seamless_cubemap_per_texture",        o(AMD_seamless_cubemap_per_texture),        GL,             2009 },
   { "GL_AMD_shader_stencil_export",               o(ARB_shader_stencil_export),               GL,             2009 },
   { "GL_APPLE_object_purgeable",                  o(APPLE_object_purgeable),                  GL,             2006 },
   { "GL_APPLE_packed_pixels",                     o(APPLE_packed_pixels),                     GL,             2002 },
   { "GL_APPLE_vertex_array_object",               o(APPLE_vertex_array_object),               GL,             2002 },
   { "GL_ATI_blend_equation_separate",             o(EXT_blend_equation_separate),             GL,             2003 },
   { "GL_ATI_draw_buffers",                        o(dummy_true),                              GL,             2002 },
   { "GL_ATI_envmap_bumpmap",                      o(ATI_envmap_bumpmap),                      GL,             2001 },
   { "GL_ATI_fragment_shader",                     o(ATI_fragment_shader),                     GL,             2001 },
   { "GL_ATI_separate_stencil",                    o(ATI_separate_stencil),                    GL,             2006 },
   { "GL_ATI_texture_compression_3dc",             o(ATI_texture_compression_3dc),             GL,             2004 },
   { "GL_ATI_texture_env_combine3",                o(ATI_texture_env_combine3),                GL,             2002 },
   { "GL_ATI_texture_float",                       o(ARB_texture_float),                       GL,             2002 },
   { "GL_ATI_texture_mirror_once",                 o(ATI_texture_mirror_once),                 GL,             2006 },
   { "GL_IBM_multimode_draw_arrays",               o(IBM_multimode_draw_arrays),               GL,             1998 },
   { "GL_IBM_rasterpos_clip",                      o(IBM_rasterpos_clip),                      GL,             1996 },
   { "GL_IBM_texture_mirrored_repeat",             o(dummy_true),                              GL,             1998 },
   { "GL_INGR_blend_func_separate",                o(EXT_blend_func_separate),                 GL,             1999 },
   { "GL_MESA_pack_invert",                        o(MESA_pack_invert),                        GL,             2002 },
   { "GL_MESA_resize_buffers",                     o(MESA_resize_buffers),                     GL,             1999 },
   { "GL_MESA_texture_array",                      o(MESA_texture_array),                      GL,             2007 },
   { "GL_MESA_texture_signed_rgba",                o(EXT_texture_snorm),                       GL,             2009 },
   { "GL_MESA_window_pos",                         o(ARB_window_pos),                          GL,             2000 },
   { "GL_MESA_ycbcr_texture",                      o(MESA_ycbcr_texture),                      GL,             2002 },
   { "GL_NV_blend_square",                         o(NV_blend_square),                         GL,             1999 },
   { "GL_NV_conditional_render",                   o(NV_conditional_render),                   GL,             2008 },
   { "GL_NV_depth_clamp",                          o(ARB_depth_clamp),                         GL,             2001 },
   { "GL_NV_draw_buffers",                         o(dummy_true),                                         ES2, 2011 },
   { "GL_NV_fbo_color_attachments",                o(EXT_framebuffer_object),                             ES2, 2010 },
   { "GL_NV_fog_distance",                         o(NV_fog_distance),                         GL,             2001 },
   { "GL_NV_fragment_program",                     o(NV_fragment_program),                     GL,             2001 },
   { "GL_NV_fragment_program_option",              o(NV_fragment_program_option),              GL,             2005 },
   { "GL_NV_light_max_exponent",                   o(NV_light_max_exponent),                   GL,             1999 },
   { "GL_NV_packed_depth_stencil",                 o(EXT_packed_depth_stencil),                GL,             2000 },
   { "GL_NV_point_sprite",                         o(NV_point_sprite),                         GL,             2001 },
   { "GL_NV_primitive_restart",                    o(NV_primitive_restart),                    GL,             2002 },
   { "GL_NV_texgen_reflection",                    o(NV_texgen_reflection),                    GL,             1999 },
   { "GL_NV_texture_barrier",                      o(NV_texture_barrier),                      GL,             2009 },
   { "GL_NV_texture_env_combine4",                 o(NV_texture_env_combine4),                 GL,             1999 },
   { "GL_NV_texture_rectangle",                    o(NV_texture_rectangle),                    GL,             2000 },
   { "GL_NV_vertex_program1_1",                    o(NV_vertex_program1_1),                    GL,             2001 },
   { "GL_NV_vertex_program",                       o(NV_vertex_program),                       GL,             2000 },
   { "GL_S3_s3tc",                                 o(S3_s3tc),                                 GL,             1999 },
   { "GL_SGIS_generate_mipmap",                    o(dummy_true),                              GL,             1997 },
   { "GL_SGIS_texture_border_clamp",               o(ARB_texture_border_clamp),                GL,             1997 },
   { "GL_SGIS_texture_edge_clamp",                 o(dummy_true),                              GL,             1997 },
   { "GL_SGIS_texture_lod",                        o(SGIS_texture_lod),                        GL,             1997 },
   { "GL_SUN_multi_draw_arrays",                   o(dummy_true),                              GL,             1999 },

   { 0, 0, 0, 0 },
};


/**
 * Given an extension name, lookup up the corresponding member of struct
 * gl_extensions and return that member's offset (in bytes).  If the name is
 * not found in the \c extension_table, return 0.
 *
 * \param name Name of extension.
 * \return Offset of member in struct gl_extensions.
 */
static size_t
name_to_offset(const char* name)
{
   const struct extension *i;

   if (name == 0)
      return 0;

   for (i = extension_table; i->name != 0; ++i) {
      if (strcmp(name, i->name) == 0)
	 return i->offset;
   }

   return 0;
}


/**
 * \brief Extensions enabled by default.
 *
 * These extensions are enabled by _mesa_init_extensions().
 *
 * XXX: Should these defaults also apply to GLES?
 */
static const size_t default_extensions[] = {
   o(ARB_copy_buffer),
   o(ARB_transpose_matrix),
   o(ARB_window_pos),

   o(EXT_compiled_vertex_array),
   o(EXT_draw_range_elements),
   o(EXT_packed_pixels),
   o(EXT_rescale_normal),
   o(EXT_separate_specular_color),
   o(EXT_texture3D),

   o(OES_standard_derivatives),

   /* Vendor Extensions */
   o(APPLE_packed_pixels),
   o(IBM_multimode_draw_arrays),
   o(IBM_rasterpos_clip),
   o(NV_light_max_exponent),
   o(NV_texgen_reflection),
   o(SGIS_texture_lod),

   0,
};


/**
 * Enable all extensions suitable for a software-only renderer.
 * This is a convenience function used by the XMesa, OSMesa, GGI drivers, etc.
 */
void
_mesa_enable_sw_extensions(struct gl_context *ctx)
{
   /*ctx->Extensions.ARB_copy_buffer = GL_TRUE;*/
   ctx->Extensions.ARB_depth_clamp = GL_TRUE;
   ctx->Extensions.ARB_depth_texture = GL_TRUE;
   ctx->Extensions.ARB_draw_elements_base_vertex = GL_TRUE;
   ctx->Extensions.ARB_draw_instanced = GL_TRUE;
   ctx->Extensions.ARB_explicit_attrib_location = GL_TRUE;
   ctx->Extensions.ARB_fragment_coord_conventions = GL_TRUE;
#if FEATURE_ARB_fragment_program
   ctx->Extensions.ARB_fragment_program = GL_TRUE;
   ctx->Extensions.ARB_fragment_program_shadow = GL_TRUE;
#endif
#if FEATURE_ARB_fragment_shader
   ctx->Extensions.ARB_fragment_shader = GL_TRUE;
#endif
#if FEATURE_ARB_framebuffer_object
   ctx->Extensions.ARB_framebuffer_object = GL_TRUE;
#endif
#if FEATURE_ARB_geometry_shader4 && 0
   /* XXX re-enable when GLSL compiler again supports geometry shaders */
   ctx->Extensions.ARB_geometry_shader4 = GL_TRUE;
#endif
   ctx->Extensions.ARB_half_float_pixel = GL_TRUE;
   ctx->Extensions.ARB_half_float_vertex = GL_TRUE;
   ctx->Extensions.ARB_map_buffer_range = GL_TRUE;
#if FEATURE_queryobj
   ctx->Extensions.ARB_occlusion_query = GL_TRUE;
   ctx->Extensions.ARB_occlusion_query2 = GL_TRUE;
#endif
   ctx->Extensions.ARB_point_sprite = GL_TRUE;
#if FEATURE_ARB_shader_objects
   ctx->Extensions.ARB_shader_objects = GL_TRUE;
   ctx->Extensions.EXT_separate_shader_objects = GL_TRUE;
#endif
#if FEATURE_ARB_shading_language_100
   ctx->Extensions.ARB_shading_language_100 = GL_TRUE;
#endif
   ctx->Extensions.ARB_shadow = GL_TRUE;
   ctx->Extensions.ARB_shadow_ambient = GL_TRUE;
   ctx->Extensions.ARB_texture_border_clamp = GL_TRUE;
   ctx->Extensions.ARB_texture_cube_map = GL_TRUE;
   ctx->Extensions.ARB_texture_env_combine = GL_TRUE;
   ctx->Extensions.ARB_texture_env_crossbar = GL_TRUE;
   ctx->Extensions.ARB_texture_env_dot3 = GL_TRUE;
   /*ctx->Extensions.ARB_texture_float = GL_TRUE;*/
   ctx->Extensions.ARB_texture_non_power_of_two = GL_TRUE;
   ctx->Extensions.ARB_texture_rg = GL_TRUE;
   ctx->Extensions.ARB_texture_compression_rgtc = GL_TRUE;
   ctx->Extensions.ARB_texture_storage = GL_TRUE;
   ctx->Extensions.ARB_vertex_array_object = GL_TRUE;
#if FEATURE_ARB_vertex_program
   ctx->Extensions.ARB_vertex_program = GL_TRUE;
#endif
#if FEATURE_ARB_vertex_shader
   ctx->Extensions.ARB_vertex_shader = GL_TRUE;
#endif
#if FEATURE_ARB_sync
   ctx->Extensions.ARB_sync = GL_TRUE;
#endif
   ctx->Extensions.APPLE_vertex_array_object = GL_TRUE;
#if FEATURE_APPLE_object_purgeable
   ctx->Extensions.APPLE_object_purgeable = GL_TRUE;
#endif
   ctx->Extensions.ATI_envmap_bumpmap = GL_TRUE;
#if FEATURE_ATI_fragment_shader
   ctx->Extensions.ATI_fragment_shader = GL_TRUE;
#endif
   ctx->Extensions.ATI_texture_compression_3dc = GL_TRUE;
   ctx->Extensions.ATI_texture_env_combine3 = GL_TRUE;
   ctx->Extensions.ATI_texture_mirror_once = GL_TRUE;
   ctx->Extensions.ATI_separate_stencil = GL_TRUE;
   ctx->Extensions.EXT_blend_color = GL_TRUE;
   ctx->Extensions.EXT_blend_equation_separate = GL_TRUE;
   ctx->Extensions.EXT_blend_func_separate = GL_TRUE;
   ctx->Extensions.EXT_blend_minmax = GL_TRUE;
   ctx->Extensions.EXT_depth_bounds_test = GL_TRUE;
   ctx->Extensions.EXT_draw_buffers2 = GL_TRUE;
   ctx->Extensions.EXT_fog_coord = GL_TRUE;
#if FEATURE_EXT_framebuffer_object
   ctx->Extensions.EXT_framebuffer_object = GL_TRUE;
#endif
#if FEATURE_EXT_framebuffer_blit
   ctx->Extensions.EXT_framebuffer_blit = GL_TRUE;
#endif
#if FEATURE_ARB_framebuffer_object
   ctx->Extensions.EXT_framebuffer_multisample = GL_TRUE;
#endif
   ctx->Extensions.EXT_packed_depth_stencil = GL_TRUE;
#if FEATURE_EXT_pixel_buffer_object
   ctx->Extensions.EXT_pixel_buffer_object = GL_TRUE;
#endif
   ctx->Extensions.EXT_point_parameters = GL_TRUE;
   ctx->Extensions.EXT_provoking_vertex = GL_TRUE;
   ctx->Extensions.EXT_shadow_funcs = GL_TRUE;
   ctx->Extensions.EXT_secondary_color = GL_TRUE;
   ctx->Extensions.EXT_stencil_two_side = GL_TRUE;
   ctx->Extensions.EXT_texture_array = GL_TRUE;
   ctx->Extensions.EXT_texture_compression_latc = GL_TRUE;
   ctx->Extensions.EXT_texture_env_dot3 = GL_TRUE;
   ctx->Extensions.EXT_texture_filter_anisotropic = GL_TRUE;
   ctx->Extensions.EXT_texture_mirror_clamp = GL_TRUE;
   ctx->Extensions.EXT_texture_shared_exponent = GL_TRUE;
#if FEATURE_EXT_texture_sRGB
   ctx->Extensions.EXT_texture_sRGB = GL_TRUE;
   ctx->Extensions.EXT_texture_sRGB_decode = GL_TRUE;
#endif
   ctx->Extensions.EXT_texture_swizzle = GL_TRUE;
#if FEATURE_EXT_transform_feedback
   /*ctx->Extensions.EXT_transform_feedback = GL_TRUE;*/
#endif
   ctx->Extensions.EXT_vertex_array_bgra = GL_TRUE;
   /*ctx->Extensions.IBM_multimode_draw_arrays = GL_TRUE;*/
   ctx->Extensions.MESA_pack_invert = GL_TRUE;
   ctx->Extensions.MESA_resize_buffers = GL_TRUE;
   ctx->Extensions.MESA_texture_array = GL_TRUE;
   ctx->Extensions.MESA_ycbcr_texture = GL_TRUE;
   ctx->Extensions.NV_blend_square = GL_TRUE;
   ctx->Extensions.NV_conditional_render = GL_TRUE;
   /*ctx->Extensions.NV_light_max_exponent = GL_TRUE;*/
   ctx->Extensions.NV_point_sprite = GL_TRUE;
   ctx->Extensions.NV_texture_env_combine4 = GL_TRUE;
   ctx->Extensions.NV_texture_rectangle = GL_TRUE;
   /*ctx->Extensions.NV_texgen_reflection = GL_TRUE;*/
#if FEATURE_NV_vertex_program
   ctx->Extensions.NV_vertex_program = GL_TRUE;
   ctx->Extensions.NV_vertex_program1_1 = GL_TRUE;
#endif
#if FEATURE_NV_fragment_program
   ctx->Extensions.NV_fragment_program = GL_TRUE;
#endif
#if FEATURE_NV_fragment_program && FEATURE_ARB_fragment_program
   ctx->Extensions.NV_fragment_program_option = GL_TRUE;
#endif
#if FEATURE_ARB_vertex_program || FEATURE_ARB_fragment_program
   ctx->Extensions.EXT_gpu_program_parameters = GL_TRUE;
#endif
#if FEATURE_texture_fxt1
   _mesa_enable_extension(ctx, "GL_3DFX_texture_compression_FXT1");
#endif
#if FEATURE_texture_s3tc
   if (ctx->Mesa_DXTn) {
      _mesa_enable_extension(ctx, "GL_EXT_texture_compression_s3tc");
      _mesa_enable_extension(ctx, "GL_S3_s3tc");
   }
#endif
}


/**
 * Enable all OpenGL 1.3 features and extensions.
 * A convenience function to be called by drivers.
 */
void
_mesa_enable_1_3_extensions(struct gl_context *ctx)
{
   ctx->Extensions.ARB_texture_border_clamp = GL_TRUE;
   ctx->Extensions.ARB_texture_cube_map = GL_TRUE;
   ctx->Extensions.ARB_texture_env_combine = GL_TRUE;
   ctx->Extensions.ARB_texture_env_dot3 = GL_TRUE;
   /*ctx->Extensions.ARB_transpose_matrix = GL_TRUE;*/
}



/**
 * Enable all OpenGL 1.4 features and extensions.
 * A convenience function to be called by drivers.
 */
void
_mesa_enable_1_4_extensions(struct gl_context *ctx)
{
   ctx->Extensions.ARB_depth_texture = GL_TRUE;
   ctx->Extensions.ARB_shadow = GL_TRUE;
   ctx->Extensions.ARB_texture_env_crossbar = GL_TRUE;
   ctx->Extensions.ARB_window_pos = GL_TRUE;
   ctx->Extensions.EXT_blend_color = GL_TRUE;
   ctx->Extensions.EXT_blend_func_separate = GL_TRUE;
   ctx->Extensions.EXT_blend_minmax = GL_TRUE;
   ctx->Extensions.EXT_fog_coord = GL_TRUE;
   ctx->Extensions.EXT_point_parameters = GL_TRUE;
   ctx->Extensions.EXT_secondary_color = GL_TRUE;
}


/**
 * Enable all OpenGL 1.5 features and extensions.
 * A convenience function to be called by drivers.
 */
void
_mesa_enable_1_5_extensions(struct gl_context *ctx)
{
   ctx->Extensions.ARB_occlusion_query = GL_TRUE;
   ctx->Extensions.EXT_shadow_funcs = GL_TRUE;
}


/**
 * Enable all OpenGL 2.0 features and extensions.
 * A convenience function to be called by drivers.
 */
void
_mesa_enable_2_0_extensions(struct gl_context *ctx)
{
#if FEATURE_ARB_fragment_shader
   ctx->Extensions.ARB_fragment_shader = GL_TRUE;
#endif
   ctx->Extensions.ARB_point_sprite = GL_TRUE;
   ctx->Extensions.EXT_blend_equation_separate = GL_TRUE;
   ctx->Extensions.ARB_texture_non_power_of_two = GL_TRUE;
#if FEATURE_ARB_shader_objects
   ctx->Extensions.ARB_shader_objects = GL_TRUE;
#endif
#if FEATURE_ARB_shading_language_100
   ctx->Extensions.ARB_shading_language_100 = GL_TRUE;
#endif
   ctx->Extensions.EXT_stencil_two_side = GL_TRUE;
#if FEATURE_ARB_vertex_shader
   ctx->Extensions.ARB_vertex_shader = GL_TRUE;
#endif
}


/**
 * Enable all OpenGL 2.1 features and extensions.
 * A convenience function to be called by drivers.
 */
void
_mesa_enable_2_1_extensions(struct gl_context *ctx)
{
#if FEATURE_EXT_pixel_buffer_object
   ctx->Extensions.EXT_pixel_buffer_object = GL_TRUE;
#endif
#if FEATURE_EXT_texture_sRGB
   ctx->Extensions.EXT_texture_sRGB = GL_TRUE;
#endif
}


/**
 * Either enable or disable the named extension.
 * \return GL_TRUE for success, GL_FALSE if invalid extension name
 */
static GLboolean
set_extension( struct gl_context *ctx, const char *name, GLboolean state )
{
   size_t offset;

   if (ctx->Extensions.String) {
      /* The string was already queried - can't change it now! */
      _mesa_problem(ctx, "Trying to enable/disable extension after glGetString(GL_EXTENSIONS): %s", name);
      return GL_FALSE;
   }

   offset = name_to_offset(name);
   if (offset == 0) {
      _mesa_problem(ctx, "Trying to enable/disable unknown extension %s",
	            name);
      return GL_FALSE;
   } else if (offset == o(dummy_true) && state == GL_FALSE) {
      _mesa_problem(ctx, "Trying to disable a permanently enabled extension: "
	                  "%s", name);
      return GL_FALSE;
   } else {
      GLboolean *base = (GLboolean *) &ctx->Extensions;
      base[offset] = state;
      return GL_TRUE;
   }
}


/**
 * Enable the named extension.
 * Typically called by drivers.
 */
void
_mesa_enable_extension( struct gl_context *ctx, const char *name )
{
   if (!set_extension(ctx, name, GL_TRUE))
      _mesa_problem(ctx, "Trying to enable unknown extension: %s", name);
}


/**
 * Disable the named extension.
 * XXX is this really needed???
 */
void
_mesa_disable_extension( struct gl_context *ctx, const char *name )
{
   if (!set_extension(ctx, name, GL_FALSE))
      _mesa_problem(ctx, "Trying to disable unknown extension: %s", name);
}


/**
 * Test if the named extension is enabled in this context.
 */
GLboolean
_mesa_extension_is_enabled( struct gl_context *ctx, const char *name )
{
   size_t offset;
   GLboolean *base;

   if (name == 0)
      return GL_FALSE;

   offset = name_to_offset(name);
   if (offset == 0)
      return GL_FALSE;
   base = (GLboolean *) &ctx->Extensions;
   return base[offset];
}


/**
 * \brief Apply the \c MESA_EXTENSION_OVERRIDE environment variable.
 *
 * \c MESA_EXTENSION_OVERRIDE is a space-separated list of extensions to
 * enable or disable. The list is processed thus:
 *    - Enable recognized extension names that are prefixed with '+'.
 *    - Disable recognized extension names that are prefixed with '-'.
 *    - Enable recognized extension names that are not prefixed.
 *    - Collect unrecognized extension names in a new string.
 *
 * \return Space-separated list of unrecognized extension names (which must
 *    be freed). Does not return \c NULL.
 */
static char *
get_extension_override( struct gl_context *ctx )
{
   const char *env_const = _mesa_getenv("MESA_EXTENSION_OVERRIDE");
   char *env;
   char *ext;
   char *extra_exts;
   int len;

   if (env_const == NULL) {
      /* Return the empty string rather than NULL. This simplifies the logic
       * of client functions. */
      return calloc(4, sizeof(char));
   }

   /* extra_exts: List of unrecognized extensions. */
   extra_exts = calloc(ALIGN(strlen(env_const) + 2, 4), sizeof(char));

   /* Copy env_const because strtok() is destructive. */
   env = strdup(env_const);
   for (ext = strtok(env, " "); ext != NULL; ext = strtok(NULL, " ")) {
      int enable;
      int recognized;
      switch (ext[0]) {
      case '+':
         enable = 1;
         ++ext;
         break;
      case '-':
         enable = 0;
         ++ext;
         break;
      default:
         enable = 1;
         break;
      }
      recognized = set_extension(ctx, ext, enable);
      if (!recognized) {
         strcat(extra_exts, ext);
         strcat(extra_exts, " ");
      }
   }

   free(env);

   /* Remove trailing space. */
   len = strlen(extra_exts);
   if (extra_exts[len - 1] == ' ')
      extra_exts[len - 1] = '\0';

   return extra_exts;
}


/**
 * \brief Initialize extension tables and enable default extensions.
 *
 * This should be called during context initialization.
 * Note: Sets gl_extensions.dummy_true to true.
 */
void
_mesa_init_extensions( struct gl_context *ctx )
{
   GLboolean *base = (GLboolean *) &ctx->Extensions;
   GLboolean *sentinel = base + o(extension_sentinel);
   GLboolean *i;
   const size_t *j;

   /* First, turn all extensions off. */
   for (i = base; i != sentinel; ++i)
      *i = GL_FALSE;

   /* Then, selectively turn default extensions on. */
   ctx->Extensions.dummy_true = GL_TRUE;
   for (j = default_extensions; *j != 0; ++j)
      base[*j] = GL_TRUE;
}


typedef unsigned short extension_index;


/**
 * Compare two entries of the extensions table.  Sorts first by year,
 * then by name.
 *
 * Arguments are indices into extension_table.
 */
static int
extension_compare(const void *p1, const void *p2)
{
   extension_index i1 = * (const extension_index *) p1;
   extension_index i2 = * (const extension_index *) p2;
   const struct extension *e1 = &extension_table[i1];
   const struct extension *e2 = &extension_table[i2];
   int res;

   res = (int)e1->year - (int)e2->year;

   if (res == 0) {
      res = strcmp(e1->name, e2->name);
   }

   return res;
}


/**
 * Construct the GL_EXTENSIONS string.  Called the first time that
 * glGetString(GL_EXTENSIONS) is called.
 */
GLubyte*
_mesa_make_extension_string(struct gl_context *ctx)
{
   /* The extension string. */
   char *exts = 0;
   /* Length of extension string. */
   size_t length = 0;
   /* Number of extensions */
   unsigned count;
   /* Indices of the extensions sorted by year */
   extension_index *extension_indices;
   /* String of extra extensions. */
   char *extra_extensions = get_extension_override(ctx);
   GLboolean *base = (GLboolean *) &ctx->Extensions;
   const struct extension *i;
   unsigned j;
   unsigned maxYear = ~0;

   /* Check if the MESA_EXTENSION_MAX_YEAR env var is set */
   {
      const char *env = getenv("MESA_EXTENSION_MAX_YEAR");
      if (env) {
         maxYear = atoi(env);
         _mesa_debug(ctx, "Note: limiting GL extensions to %u or earlier\n",
                     maxYear);
      }
   }

   /* Compute length of the extension string. */
   count = 0;
   for (i = extension_table; i->name != 0; ++i) {
      if (base[i->offset] &&
          i->year <= maxYear &&
          (i->api_set & (1 << ctx->API))) {
	 length += strlen(i->name) + 1; /* +1 for space */
	 ++count;
      }
   }
   if (extra_extensions != NULL)
      length += 1 + strlen(extra_extensions); /* +1 for space */

   exts = (char *) calloc(ALIGN(length + 1, 4), sizeof(char));
   if (exts == NULL) {
      free(extra_extensions);
      return NULL;
   }

   extension_indices = malloc(count * sizeof(extension_index));
   if (extension_indices == NULL) {
      free(exts);
      free(extra_extensions);
      return NULL;
   }

   /* Sort extensions in chronological order because certain old applications (e.g.,
    * Quake3 demo) store the extension list in a static size buffer so chronologically
    * order ensure that the extensions that such applications expect will fit into
    * that buffer.
    */
   j = 0;
   for (i = extension_table; i->name != 0; ++i) {
      if (base[i->offset] &&
          i->year <= maxYear &&
          (i->api_set & (1 << ctx->API))) {
         extension_indices[j++] = i - extension_table;
      }
   }
   assert(j == count);
   qsort(extension_indices, count, sizeof *extension_indices, extension_compare);

   /* Build the extension string.*/
   for (j = 0; j < count; ++j) {
      i = &extension_table[extension_indices[j]];
      assert(base[i->offset] && (i->api_set & (1 << ctx->API)));
      strcat(exts, i->name);
      strcat(exts, " ");
   }
   free(extension_indices);
   if (extra_extensions != 0) {
      strcat(exts, extra_extensions);
      free(extra_extensions);
   }

   return (GLubyte *) exts;
}

/**
 * Return number of enabled extensions.
 */
GLuint
_mesa_get_extension_count(struct gl_context *ctx)
{
   GLboolean *base;
   const struct extension *i;

   /* only count once */
   if (ctx->Extensions.Count != 0)
      return ctx->Extensions.Count;

   base = (GLboolean *) &ctx->Extensions;
   for (i = extension_table; i->name != 0; ++i) {
      if (base[i->offset]) {
	 ctx->Extensions.Count++;
      }
   }
   return ctx->Extensions.Count;
}

/**
 * Return name of i-th enabled extension
 */
const GLubyte *
_mesa_get_enabled_extension(struct gl_context *ctx, GLuint index)
{
   const GLboolean *base;
   size_t n;
   const struct extension *i;

   if (index < 0)
      return NULL;

   base = (GLboolean*) &ctx->Extensions;
   n = 0;
   for (i = extension_table; i->name != 0; ++i) {
      if (n == index && base[i->offset]) {
	 return (GLubyte*) i->name;
      } else if (base[i->offset]) {
	 ++n;
      }
   }

   return NULL;
}
