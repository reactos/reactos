/*
 * The World -- fitz resources and trees
 */

#ifdef _FITZ_WORLD_H_
#error "fitz-world.h must only be included once"
#endif
#define _FITZ_WORLD_H_

#ifndef _FITZ_BASE_H_
#error "fitz-base.h must be included before fitz-world.h"
#endif

#ifdef  __cplusplus
extern "C" {
#endif

#include "fitz/wld_font.h"
#include "fitz/wld_color.h"
#include "fitz/wld_image.h"
#include "fitz/wld_shade.h"
#include "fitz/wld_tree.h"
#include "fitz/wld_path.h"
#include "fitz/wld_text.h"

#ifdef  __cplusplus
}
#endif
