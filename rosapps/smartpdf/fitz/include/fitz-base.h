#ifdef _FITZ_BASE_H_
#error "fitz-base.h must only be included once"
#endif
#define _FITZ_BASE_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "fitz/base_sysdep.h"
#include "fitz/base_cpudep.h"
#include "fitz/base_runtime.h"
#include "fitz/base_math.h"
#include "fitz/base_geom.h"
#include "fitz/base_hash.h"
#include "fitz/base_pixmap.h"

#ifdef  __cplusplus
}
#endif
