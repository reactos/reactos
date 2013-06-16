/**************************************************************************
 *
 * Copyright 2010 Vmware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "pipe/p_screen.h"
#include "util/u_format.h"
#include "util/u_debug.h"
#include "u_caps.h"

/**
 * Iterates over a list of caps checks as defined in u_caps.h. Should
 * all checks pass returns TRUE and out is set to the last element of
 * the list (TERMINATE). Should any check fail returns FALSE and set
 * out to the index of the start of the first failing check.
 */
boolean
util_check_caps_out(struct pipe_screen *screen, const unsigned *list, int *out)
{
   int i, tmpi;
   float tmpf;

   for (i = 0; list[i];) {
      switch(list[i++]) {
      case UTIL_CAPS_CHECK_CAP:
         if (!screen->get_param(screen, list[i++])) {
            *out = i - 2;
            return FALSE;
         }
         break;
      case UTIL_CAPS_CHECK_INT:
         tmpi = screen->get_param(screen, list[i++]);
         if (tmpi < (int)list[i++]) {
            *out = i - 3;
            return FALSE;
         }
         break;
      case UTIL_CAPS_CHECK_FLOAT:
         tmpf = screen->get_paramf(screen, list[i++]);
         if (tmpf < (float)list[i++]) {
            *out = i - 3;
            return FALSE;
         }
         break;
      case UTIL_CAPS_CHECK_FORMAT:
         if (!screen->is_format_supported(screen,
                                          list[i++],
                                          PIPE_TEXTURE_2D,
                                          0,
                                          PIPE_BIND_SAMPLER_VIEW)) {
            *out = i - 2;
            return FALSE;
         }
         break;
      case UTIL_CAPS_CHECK_SHADER:
         tmpi = screen->get_shader_param(screen, list[i] >> 24, list[i] & ((1 << 24) - 1));
         ++i;
         if (tmpi < (int)list[i++]) {
            *out = i - 3;
            return FALSE;
         }
         break;
      case UTIL_CAPS_CHECK_UNIMPLEMENTED:
         *out = i - 1;
         return FALSE;
      default:
         assert(!"Unsupported check");
         return FALSE;
      }
   }

   *out = i;
   return TRUE;
}

/**
 * Iterates over a list of caps checks as defined in u_caps.h.
 * Returns TRUE if all caps checks pass returns FALSE otherwise.
 */
boolean
util_check_caps(struct pipe_screen *screen, const unsigned *list)
{
   int out;
   return util_check_caps_out(screen, list, &out);
}


/*
 * Below follows some demo lists.
 *
 * None of these lists are exhausting lists of what is
 * actually needed to support said API and more here for
 * as example on how to uses the above functions. Especially
 * for DX10 and DX11 where Gallium is missing features.
 */

/* DX 9_1 */
static unsigned caps_dx_9_1[] = {
   UTIL_CHECK_INT(MAX_RENDER_TARGETS, 1),
   UTIL_CHECK_INT(MAX_TEXTURE_2D_LEVELS, 12),    /* 2048 */
   UTIL_CHECK_INT(MAX_TEXTURE_3D_LEVELS, 9),     /* 256 */
   UTIL_CHECK_INT(MAX_TEXTURE_CUBE_LEVELS, 10),  /* 512 */
   UTIL_CHECK_FLOAT(MAX_TEXTURE_ANISOTROPY, 2),
   UTIL_CHECK_TERMINATE
};

/* DX 9_2 */
static unsigned caps_dx_9_2[] = {
   UTIL_CHECK_CAP(OCCLUSION_QUERY),
   UTIL_CHECK_CAP(BLEND_EQUATION_SEPARATE),
   UTIL_CHECK_INT(MAX_RENDER_TARGETS, 1),
   UTIL_CHECK_INT(MAX_TEXTURE_2D_LEVELS, 12),    /* 2048 */
   UTIL_CHECK_INT(MAX_TEXTURE_3D_LEVELS, 9),     /* 256 */
   UTIL_CHECK_INT(MAX_TEXTURE_CUBE_LEVELS, 10),  /* 512 */
   UTIL_CHECK_FLOAT(MAX_TEXTURE_ANISOTROPY, 16),
   UTIL_CHECK_TERMINATE
};

/* DX 9_3 */
static unsigned caps_dx_9_3[] = {
   UTIL_CHECK_CAP(SM3),
 //UTIL_CHECK_CAP(INSTANCING),
   UTIL_CHECK_CAP(OCCLUSION_QUERY),
   UTIL_CHECK_INT(MAX_RENDER_TARGETS, 4),
   UTIL_CHECK_INT(MAX_TEXTURE_2D_LEVELS, 13),    /* 4096 */
   UTIL_CHECK_INT(MAX_TEXTURE_3D_LEVELS, 9),     /* 256 */
   UTIL_CHECK_INT(MAX_TEXTURE_CUBE_LEVELS, 10),  /* 512 */
   UTIL_CHECK_FLOAT(MAX_TEXTURE_ANISOTROPY, 16),
   UTIL_CHECK_TERMINATE
};

/* DX 10 */
static unsigned caps_dx_10[] = {
   UTIL_CHECK_CAP(SM3),
 //UTIL_CHECK_CAP(INSTANCING),
   UTIL_CHECK_CAP(OCCLUSION_QUERY),
   UTIL_CHECK_INT(MAX_RENDER_TARGETS, 8),
   UTIL_CHECK_INT(MAX_TEXTURE_2D_LEVELS, 14),    /* 8192 */
   UTIL_CHECK_INT(MAX_TEXTURE_3D_LEVELS, 12),    /* 2048 */
   UTIL_CHECK_INT(MAX_TEXTURE_CUBE_LEVELS, 14),  /* 8192 */
   UTIL_CHECK_FLOAT(MAX_TEXTURE_ANISOTROPY, 16),
   UTIL_CHECK_UNIMPLEMENTED, /* XXX Unimplemented features in Gallium */
   UTIL_CHECK_TERMINATE
};

/* DX11 */
static unsigned caps_dx_11[] = {
   UTIL_CHECK_CAP(SM3),
 //UTIL_CHECK_CAP(INSTANCING),
   UTIL_CHECK_CAP(OCCLUSION_QUERY),
   UTIL_CHECK_INT(MAX_RENDER_TARGETS, 8),
   UTIL_CHECK_INT(MAX_TEXTURE_2D_LEVELS, 14),    /* 16384 */
   UTIL_CHECK_INT(MAX_TEXTURE_3D_LEVELS, 12),    /* 2048 */
   UTIL_CHECK_INT(MAX_TEXTURE_CUBE_LEVELS, 14),  /* 16384 */
   UTIL_CHECK_FLOAT(MAX_TEXTURE_ANISOTROPY, 16),
   UTIL_CHECK_FORMAT(B8G8R8A8_UNORM),
   UTIL_CHECK_UNIMPLEMENTED, /* XXX Unimplemented features in Gallium */
   UTIL_CHECK_TERMINATE
};

/* OpenGL 2.1 */
static unsigned caps_opengl_2_1[] = {
   UTIL_CHECK_CAP(OCCLUSION_QUERY),
   UTIL_CHECK_CAP(TWO_SIDED_STENCIL),
   UTIL_CHECK_CAP(BLEND_EQUATION_SEPARATE),
   UTIL_CHECK_INT(MAX_RENDER_TARGETS, 2),
   UTIL_CHECK_TERMINATE
};

/* OpenGL 3.0 */
/* UTIL_CHECK_INT(MAX_RENDER_TARGETS, 8), */

/* Shader Model 3 */
static unsigned caps_sm3[] = {
    UTIL_CHECK_SHADER(FRAGMENT, MAX_INSTRUCTIONS, 512),
    UTIL_CHECK_SHADER(FRAGMENT, MAX_INPUTS, 10),
    UTIL_CHECK_SHADER(FRAGMENT, MAX_TEMPS, 32),
    UTIL_CHECK_SHADER(FRAGMENT, MAX_ADDRS, 1),
    UTIL_CHECK_SHADER(FRAGMENT, MAX_CONSTS, 224),

    UTIL_CHECK_SHADER(VERTEX, MAX_INSTRUCTIONS, 512),
    UTIL_CHECK_SHADER(VERTEX, MAX_INPUTS, 16),
    UTIL_CHECK_SHADER(VERTEX, MAX_TEMPS, 32),
    UTIL_CHECK_SHADER(VERTEX, MAX_ADDRS, 2),
    UTIL_CHECK_SHADER(VERTEX, MAX_CONSTS, 256),

    UTIL_CHECK_TERMINATE
};

/**
 * Demo function which checks against theoretical caps needed for different APIs.
 */
void util_caps_demo_print(struct pipe_screen *screen)
{
   struct {
      char* name;
      unsigned *list;
   } list[] = {
      {"DX 9.1", caps_dx_9_1},
      {"DX 9.2", caps_dx_9_2},
      {"DX 9.3", caps_dx_9_3},
      {"DX 10", caps_dx_10},
      {"DX 11", caps_dx_11},
      {"OpenGL 2.1", caps_opengl_2_1},
/*    {"OpenGL 3.0", caps_opengl_3_0},*/
      {"SM3", caps_sm3},
      {NULL, NULL}
   };
   int i, out = 0;

   for (i = 0; list[i].name; i++) {
      if (util_check_caps_out(screen, list[i].list, &out)) {
         debug_printf("%s: %s yes\n", __FUNCTION__, list[i].name);
         continue;
      }
      switch (list[i].list[out]) {
      case UTIL_CAPS_CHECK_CAP:
         debug_printf("%s: %s no (cap %u not supported)\n", __FUNCTION__,
                      list[i].name,
                      list[i].list[out + 1]);
         break;
      case UTIL_CAPS_CHECK_INT:
         debug_printf("%s: %s no (cap %u less then %u)\n", __FUNCTION__,
                      list[i].name,
                      list[i].list[out + 1],
                      list[i].list[out + 2]);
         break;
      case UTIL_CAPS_CHECK_FLOAT:
         debug_printf("%s: %s no (cap %u less then %f)\n", __FUNCTION__,
                      list[i].name,
                      list[i].list[out + 1],
                      (double)(int)list[i].list[out + 2]);
         break;
      case UTIL_CAPS_CHECK_FORMAT:
         debug_printf("%s: %s no (format %s not supported)\n", __FUNCTION__,
                      list[i].name,
                      util_format_name(list[i].list[out + 1]) + 12);
         break;
      case UTIL_CAPS_CHECK_UNIMPLEMENTED:
         debug_printf("%s: %s no (not implemented in gallium or state tracker)\n",
                      __FUNCTION__, list[i].name);
         break;
      default:
            assert(!"Unsupported check");
      }
   }
}
