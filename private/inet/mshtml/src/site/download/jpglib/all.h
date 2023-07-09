/*
 * ALL.H
 *  used when compiling the library.
 * Added because when building from ie/core, there is a
 *  precompiled header - all.h that is used, and these files
 *  must include that.
 * When building the library, we are using the same source
 *  files and must also include all.h, but we dont need
 *  the one in ie/core. We dont need anything extra, so
 *  just go with what we got.
 */

#ifdef WIN16
#define HUGEP huge
#else
#define HUGEP
#endif

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
