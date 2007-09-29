/*
 * Streams and dynamic objects
 */

#ifdef _FITZ_STREAM_H_
#error "fitz-stream.h must only be included once"
#endif
#define _FITZ_STREAM_H_

#ifndef _FITZ_BASE_H_
#error "fitz-base.h must be included before fitz-stream.h"
#endif

#ifdef  __cplusplus
extern "C" {
#endif

#include "fitz/stm_crypt.h"
#include "fitz/stm_object.h"
#include "fitz/stm_buffer.h"
#include "fitz/stm_filter.h"
#include "fitz/stm_stream.h"

#ifdef  __cplusplus
}
#endif
