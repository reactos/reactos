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

#include <precomp.h>

#define ALIGN(value, alignment)  (((value) + alignment - 1) & ~(alignment - 1))

/**
 * \brief An element of the \c extension_table.
 */
struct extension {
   /** Name of extension, such as "GL_ARB_depth_clamp". */
   const char *name;

   /** Offset (in bytes) of the corresponding member in struct gl_extensions. */
   size_t offset;

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
   { "GL_ARB_map_buffer_range",                    o(ARB_map_buffer_range),                    2008 },
   { "GL_ARB_multisample",                         o(dummy_true),                              1994 },
   { "GL_ARB_point_parameters",                    o(EXT_point_parameters),                    1997 },
   { "GL_ARB_point_sprite",                        o(ARB_point_sprite),                        2003 },
   { "GL_ARB_texture_cube_map",                    o(ARB_texture_cube_map),                    1999 },
   { "GL_ARB_texture_env_add",                     o(dummy_true),                              1999 },
   { "GL_ARB_texture_env_combine",                 o(ARB_texture_env_combine),                 2001 },
   { "GL_ARB_texture_env_crossbar",                o(ARB_texture_env_crossbar),                2001 },
   { "GL_ARB_texture_env_dot3",                    o(ARB_texture_env_dot3),                    2001 },
   { "GL_ARB_texture_mirrored_repeat",             o(dummy_true),                              2001 },
   { "GL_ARB_texture_storage",                     o(ARB_texture_storage),                     2011 },
   { "GL_ARB_transpose_matrix",                    o(ARB_transpose_matrix),                    1999 },
   { "GL_ARB_vertex_buffer_object",                o(dummy_true),                              2003 },
   { "GL_ARB_window_pos",                          o(ARB_window_pos),                          2001 },
   /* EXT extensions */
   { "GL_EXT_abgr",                                o(dummy_true),                              1995 },
   { "GL_EXT_bgra",                                o(dummy_true),                              1995 },
   { "GL_EXT_blend_color",                         o(EXT_blend_color),                         1995 },
   { "GL_EXT_blend_equation_separate",             o(EXT_blend_equation_separate),             2003 },
   { "GL_EXT_blend_func_separate",                 o(EXT_blend_func_separate),                 1999 },
   { "GL_EXT_blend_minmax",                        o(EXT_blend_minmax),                        1995 },
   { "GL_EXT_blend_subtract",                      o(dummy_true),                              1995 },
   { "GL_EXT_clip_volume_hint",                    o(EXT_clip_volume_hint),                    1996 },
   { "GL_EXT_compiled_vertex_array",               o(EXT_compiled_vertex_array),               1996 },
   { "GL_EXT_copy_texture",                        o(dummy_true),                              1995 },
   { "GL_EXT_depth_bounds_test",                   o(EXT_depth_bounds_test),                   2002 },
   { "GL_EXT_draw_range_elements",                 o(EXT_draw_range_elements),                 1997 },
   { "GL_EXT_fog_coord",                           o(EXT_fog_coord),                           1999 },
   { "GL_EXT_packed_pixels",                       o(EXT_packed_pixels),                       1997 },
   { "GL_EXT_point_parameters",                    o(EXT_point_parameters),                    1997 },
   { "GL_EXT_polygon_offset",                      o(dummy_true),                              1995 },
   { "GL_EXT_rescale_normal",                      o(EXT_rescale_normal),                      1997 },
   { "GL_EXT_secondary_color",                     o(EXT_secondary_color),                     1999 },
   { "GL_EXT_separate_shader_objects",             o(EXT_separate_shader_objects),             2008 },
   { "GL_EXT_separate_specular_color",             o(EXT_separate_specular_color),             1997 },
   { "GL_EXT_shadow_funcs",                        o(EXT_shadow_funcs),                        2002 },
   { "GL_EXT_stencil_wrap",                        o(dummy_true),                              2002 },
   { "GL_EXT_subtexture",                          o(dummy_true),                              1995 },
   { "GL_EXT_texture_cube_map",                    o(ARB_texture_cube_map),                    2001 },
   { "GL_EXT_texture_env_add",                     o(dummy_true),                              1999 },
   { "GL_EXT_texture_env_combine",                 o(dummy_true),                              2000 },
   { "GL_EXT_texture_env_dot3",                    o(EXT_texture_env_dot3),                    2000 },
   { "GL_EXT_texture_filter_anisotropic",          o(EXT_texture_filter_anisotropic),          1999 },
   { "GL_EXT_texture_integer",                     o(EXT_texture_integer),                     2006 },
   { "GL_EXT_texture_object",                      o(dummy_true),                              1995 },
   { "GL_EXT_texture",                             o(dummy_true),                              1996 },
   { "GL_EXT_vertex_array",                        o(dummy_true),                              1995 },

   /* Vendor extensions */
   { "GL_APPLE_packed_pixels",                     o(APPLE_packed_pixels),                     2002 },
   { "GL_ATI_blend_equation_separate",             o(EXT_blend_equation_separate),             2003 },
   { "GL_ATI_texture_env_combine3",                o(ATI_texture_env_combine3),                2002 },
   { "GL_IBM_multimode_draw_arrays",               o(IBM_multimode_draw_arrays),               1998 },
   { "GL_IBM_rasterpos_clip",                      o(IBM_rasterpos_clip),                      1996 },
   { "GL_IBM_texture_mirrored_repeat",             o(dummy_true),                              1998 },
   { "GL_INGR_blend_func_separate",                o(EXT_blend_func_separate),                 1999 },
   { "GL_MESA_pack_invert",                        o(MESA_pack_invert),                        2002 },
   { "GL_MESA_resize_buffers",                     o(MESA_resize_buffers),                     1999 },
   { "GL_MESA_window_pos",                         o(ARB_window_pos),                          2000 },
   { "GL_MESA_ycbcr_texture",                      o(MESA_ycbcr_texture),                      2002 },
   { "GL_NV_blend_square",                         o(NV_blend_square),                         1999 },
   { "GL_NV_fog_distance",                         o(NV_fog_distance),                         2001 },
   { "GL_NV_light_max_exponent",                   o(NV_light_max_exponent),                   1999 },
   { "GL_NV_point_sprite",                         o(NV_point_sprite),                         2001 },
   { "GL_NV_texgen_reflection",                    o(NV_texgen_reflection),                    1999 },
   { "GL_NV_texture_env_combine4",                 o(NV_texture_env_combine4),                 1999 },

   { 0, 0, 0},
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
   o(ARB_transpose_matrix),
   o(ARB_window_pos),

   o(EXT_compiled_vertex_array),
   o(EXT_draw_range_elements),
   o(EXT_packed_pixels),
   o(EXT_rescale_normal),
   o(EXT_separate_specular_color),

   /* Vendor Extensions */
   o(APPLE_packed_pixels),
   o(IBM_multimode_draw_arrays),
   o(IBM_rasterpos_clip),
   o(NV_light_max_exponent),
   o(NV_texgen_reflection),

   0,
};


/**
 * Enable all extensions suitable for a software-only renderer.
 * This is a convenience function used by the XMesa, OSMesa, GGI drivers, etc.
 */
void
_mesa_enable_sw_extensions(struct gl_context *ctx)
{
   ctx->Extensions.ARB_map_buffer_range = GL_TRUE;
   ctx->Extensions.ARB_point_sprite = GL_TRUE;
   ctx->Extensions.ARB_texture_cube_map = GL_TRUE;
   ctx->Extensions.ARB_texture_env_combine = GL_TRUE;
   ctx->Extensions.ARB_texture_env_crossbar = GL_TRUE;
   ctx->Extensions.ARB_texture_env_dot3 = GL_TRUE;
   /*ctx->Extensions.ARB_texture_float = GL_TRUE;*/
   ctx->Extensions.ARB_texture_storage = GL_TRUE;
   ctx->Extensions.ATI_texture_env_combine3 = GL_TRUE;
   ctx->Extensions.EXT_blend_color = GL_TRUE;
   ctx->Extensions.EXT_blend_equation_separate = GL_TRUE;
   ctx->Extensions.EXT_blend_func_separate = GL_TRUE;
   ctx->Extensions.EXT_blend_minmax = GL_TRUE;
   ctx->Extensions.EXT_depth_bounds_test = GL_TRUE;
   ctx->Extensions.EXT_fog_coord = GL_TRUE;
   ctx->Extensions.EXT_point_parameters = GL_TRUE;
   ctx->Extensions.EXT_shadow_funcs = GL_TRUE;
   ctx->Extensions.EXT_secondary_color = GL_TRUE;
   ctx->Extensions.EXT_texture_env_dot3 = GL_TRUE;
   ctx->Extensions.EXT_texture_filter_anisotropic = GL_TRUE;
   /*ctx->Extensions.IBM_multimode_draw_arrays = GL_TRUE;*/
   ctx->Extensions.MESA_pack_invert = GL_TRUE;
   ctx->Extensions.MESA_resize_buffers = GL_TRUE;
   ctx->Extensions.MESA_ycbcr_texture = GL_TRUE;
   ctx->Extensions.NV_blend_square = GL_TRUE;
   /*ctx->Extensions.NV_light_max_exponent = GL_TRUE;*/
   ctx->Extensions.NV_point_sprite = GL_TRUE;
   ctx->Extensions.NV_texture_env_combine4 = GL_TRUE;
   /*ctx->Extensions.NV_texgen_reflection = GL_TRUE;*/
}


/**
 * Enable all OpenGL 1.3 features and extensions.
 * A convenience function to be called by drivers.
 */
void
_mesa_enable_1_3_extensions(struct gl_context *ctx)
{
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
   ctx->Extensions.EXT_shadow_funcs = GL_TRUE;
}


/**
 * Enable all OpenGL 2.0 features and extensions.
 * A convenience function to be called by drivers.
 */
void
_mesa_enable_2_0_extensions(struct gl_context *ctx)
{
   ctx->Extensions.ARB_point_sprite = GL_TRUE;
   ctx->Extensions.EXT_blend_equation_separate = GL_TRUE;
}


/**
 * Enable all OpenGL 2.1 features and extensions.
 * A convenience function to be called by drivers.
 */
void
_mesa_enable_2_1_extensions(struct gl_context *ctx)
{
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
   env = _strdup(env_const);
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
          i->year <= maxYear) {
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
          i->year <= maxYear) {
         extension_indices[j++] = i - extension_table;
      }
   }
   assert(j == count);
   qsort(extension_indices, count, sizeof *extension_indices, extension_compare);

   /* Build the extension string.*/
   for (j = 0; j < count; ++j) {
      i = &extension_table[extension_indices[j]];
      assert(base[i->offset]);
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
