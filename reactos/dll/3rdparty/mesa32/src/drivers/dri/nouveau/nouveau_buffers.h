#ifndef __NOUVEAU_BUFFERS_H__
#define __NOUVEAU_BUFFERS_H__

#include <stdint.h>
#include "mtypes.h"
#include "utils.h"
#include "renderbuffer.h"

typedef struct nouveau_mem_t {
   int type;
   uint64_t offset;
   uint64_t size;
   void*    map;
} nouveau_mem;

extern nouveau_mem *nouveau_mem_alloc(GLcontext *ctx, int type,
      				      GLuint size, GLuint align);
extern void nouveau_mem_free(GLcontext *ctx, nouveau_mem *mem);
extern uint32_t nouveau_mem_gpu_offset_get(GLcontext *ctx, nouveau_mem *mem);

extern GLboolean nouveau_memformat_flat_emit(GLcontext *ctx,
      					     nouveau_mem *dst,
					     nouveau_mem *src,
					     GLuint dst_offset,
					     GLuint src_offset,
					     GLuint size);

typedef struct nouveau_renderbuffer_t {
   struct gl_renderbuffer mesa; /* must be first! */
   __DRIdrawablePrivate  *dPriv;

   nouveau_mem *mem;
   void *	map;

   int		cpp;
   uint32_t	offset;
   uint32_t	pitch;
} nouveau_renderbuffer;

extern nouveau_renderbuffer *nouveau_renderbuffer_new(GLenum internalFormat,
      GLvoid *map, GLuint offset, GLuint pitch, __DRIdrawablePrivate *dPriv);
extern void nouveau_window_moved(GLcontext *ctx);
extern GLboolean nouveau_build_framebuffer(GLcontext *, struct gl_framebuffer *);
extern nouveau_renderbuffer *nouveau_current_draw_buffer(GLcontext *ctx);

extern void nouveauInitBufferFuncs(struct dd_function_table *func);

#endif
