/*
** gluos.h - operating system dependencies for GLU
**
*/
#ifdef __VMS
#ifdef __cplusplus
#pragma message disable nocordel
#pragma message disable codeunreachable
#pragma message disable codcauunr
#endif
#endif

#ifdef __WATCOMC__
/* Disable *lots* of warnings to get a clean build. I can't be bothered fixing the
 * code at the moment, as it is pretty ugly.
 */
#pragma warning 7   10
#pragma warning 13  10
#pragma warning 14  10
#pragma warning 367 10
#pragma warning 379 10
#pragma warning 726 10
#pragma warning 836 10
#endif

#ifdef BUILD_FOR_SNAP

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>

#elif defined(_WIN32)

#include <stdlib.h>	    /* For _MAX_PATH definition */
#include <stdio.h>
#include <malloc.h>

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOIME
#define NOMINMAX

#ifndef STRICT
  #define STRICT 1
#endif

#include <windows.h>

/* Non documented stuff in MS Headers */
/* NurbsCallback */
#define GLU_NURBS_ERROR                    100103
#define GLU_ERROR                          100103
#define GLU_NURBS_BEGIN                    100164
#define GLU_NURBS_BEGIN_EXT                100164
#define GLU_NURBS_VERTEX                   100165
#define GLU_NURBS_VERTEX_EXT               100165
#define GLU_NURBS_NORMAL                   100166
#define GLU_NURBS_NORMAL_EXT               100166
#define GLU_NURBS_COLOR                    100167
#define GLU_NURBS_COLOR_EXT                100167
#define GLU_NURBS_TEXTURE_COORD            100168
#define GLU_NURBS_TEX_COORD_EXT            100168
#define GLU_NURBS_END                      100169
#define GLU_NURBS_END_EXT                  100169
#define GLU_NURBS_BEGIN_DATA               100170
#define GLU_NURBS_BEGIN_DATA_EXT           100170
#define GLU_NURBS_VERTEX_DATA              100171
#define GLU_NURBS_VERTEX_DATA_EXT          100171
#define GLU_NURBS_NORMAL_DATA              100172
#define GLU_NURBS_NORMAL_DATA_EXT          100172
#define GLU_NURBS_COLOR_DATA               100173
#define GLU_NURBS_COLOR_DATA_EXT           100173
#define GLU_NURBS_TEXTURE_COORD_DATA       100174
#define GLU_NURBS_TEX_COORD_DATA_EXT       100174
#define GLU_NURBS_END_DATA                 100175
#define GLU_NURBS_END_DATA_EXT             100175

/* NurbsProperty */
#define GLU_AUTO_LOAD_MATRIX               100200
#define GLU_CULLING                        100201
#define GLU_SAMPLING_TOLERANCE             100203
#define GLU_DISPLAY_MODE                   100204
#define GLU_PARAMETRIC_TOLERANCE           100202
#define GLU_SAMPLING_METHOD                100205
#define GLU_U_STEP                         100206
#define GLU_V_STEP                         100207
#define GLU_NURBS_MODE                     100160
#define GLU_NURBS_MODE_EXT                 100160
#define GLU_NURBS_TESSELLATOR              100161
#define GLU_NURBS_TESSELLATOR_EXT          100161
#define GLU_NURBS_RENDERER                 100162
#define GLU_NURBS_RENDERER_EXT             100162

/* NurbsSampling */
#define GLU_OBJECT_PARAMETRIC_ERROR        100208
#define GLU_OBJECT_PARAMETRIC_ERROR_EXT    100208
#define GLU_OBJECT_PATH_LENGTH             100209
#define GLU_OBJECT_PATH_LENGTH_EXT         100209
#define GLU_PATH_LENGTH                    100215
#define GLU_PARAMETRIC_ERROR               100216
#define GLU_DOMAIN_DISTANCE                100217

/* Disable warnings */
#ifdef _MSC_VER
#pragma warning(disable : 4101)
#pragma warning(disable : 4244)
#pragma warning(disable : 4761)
#endif

#if defined(_MSC_VER) && _MSC_VER >= 1200
#pragma comment(linker, "/OPT:NOWIN98")
#endif

#ifndef GLAPIENTRY
#define GLAPIENTRY APIENTRY
#endif

#elif defined(__OS2__)

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#define WINGDIAPI

#else

/* Disable Microsoft-specific keywords */
#define GLAPIENTRY
#define WINGDIAPI

#endif
