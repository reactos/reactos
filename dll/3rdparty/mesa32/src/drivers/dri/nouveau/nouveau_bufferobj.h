#ifndef __NOUVEAU_BUFFEROBJ_H__
#define __NOUVEAU_BUFFEROBJ_H__

#include "mtypes.h"
#include "nouveau_buffers.h"

#define NOUVEAU_BO_VRAM_OK (NOUVEAU_MEM_FB | NOUVEAU_MEM_FB_ACCEPTABLE)
#define NOUVEAU_BO_AGP_OK  (NOUVEAU_MEM_AGP | NOUVEAU_MEM_AGP_ACCEPTABLE)

typedef struct nouveau_bufferobj_region_t {
	uint32_t start;
	uint32_t end;
} nouveau_bufferobj_region;

typedef struct nouveau_bufferobj_dirty_t {
	nouveau_bufferobj_region *dirty;
	int nr_dirty;
} nouveau_bufferobj_dirty;

typedef struct nouveau_buffer_object_t {
	/* Base class, must be first */
	struct gl_buffer_object mesa;

	GLboolean		mapped;
	GLenum			usage;

	/* Memory used for GPU access to the buffer*/
	GLuint			gpu_mem_flags;
	nouveau_mem *		gpu_mem;
	nouveau_bufferobj_dirty	gpu_dirty;

	/* Memory used for CPU access to the buffer */
	GLuint			cpu_mem_flags;
	nouveau_mem *		cpu_mem;
	GLvoid *		cpu_mem_sys;
	nouveau_bufferobj_dirty	cpu_dirty;
} nouveau_buffer_object;

extern void
nouveau_bo_init_storage(GLcontext *ctx, GLuint valid_gpu_access,
			GLsizeiptrARB size, const GLvoid *data, GLenum usage,
			struct gl_buffer_object *bo);

extern GLboolean
nouveau_bo_move_in(GLcontext *ctx, struct gl_buffer_object *bo);

extern GLboolean
nouveau_bo_move_out(GLcontext *ctx, struct gl_buffer_object *bo);

extern void *
nouveau_bo_map(GLcontext *ctx, GLenum usage, struct gl_buffer_object *bo);

extern void
nouveau_bo_unmap(GLcontext *ctx, struct gl_buffer_object *bo);

extern uint32_t
nouveau_bo_gpu_ref(GLcontext *ctx, struct gl_buffer_object *bo);

extern void
nouveau_bo_dirty_linear(GLcontext *ctx, GLboolean on_card,
			uint32_t offset, uint32_t size,
			struct gl_buffer_object *bo);

extern void
nouveau_bo_dirty_all(GLcontext *ctx, GLboolean on_card,
		     struct gl_buffer_object *bo);

extern GLuint
nouveau_bo_upload_dirty(GLcontext *ctx, struct gl_buffer_object *bo);

extern GLuint
nouveau_bo_download_dirty(GLcontext *ctx, struct gl_buffer_object *bo);

extern void
nouveauInitBufferObjects(GLcontext *ctx);

#endif
