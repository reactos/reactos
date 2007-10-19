/*
 * Rasterizer
 */

#ifdef _FITZ_DRAW_H_
#error "fitz-draw.h must only be included once"
#endif
#define _FITZ_DRAW_H_

#ifndef _FITZ_BASE_H_
#error "fitz-base.h must be included before fitz-draw.h"
#endif

#ifndef _FITZ_WORLD_H_
#error "fitz-world.h must be included before fitz-draw.h"
#endif

#ifdef  __cplusplus
extern "C" {
#endif

#include "fitz/draw_path.h"
#include "fitz/draw_misc.h"

#ifdef  __cplusplus
}
#endif
