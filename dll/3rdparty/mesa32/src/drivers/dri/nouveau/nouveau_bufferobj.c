#include "bufferobj.h"
#include "enums.h"

#include "nouveau_bufferobj.h"
#include "nouveau_buffers.h"
#include "nouveau_context.h"
#include "nouveau_drm.h"
#include "nouveau_object.h"
#include "nouveau_msg.h"

#define NOUVEAU_MEM_FREE(mem) do {      \
	nouveau_mem_free(ctx, (mem));   \
	(mem) = NULL;                   \
} while(0)

#define DEBUG(fmt,args...) do {                \
	if (NOUVEAU_DEBUG & DEBUG_BUFFEROBJ) { \
		fprintf(stderr, "%s: "fmt, __func__, ##args);  \
	}                                      \
} while(0)

static GLboolean
nouveau_bo_download_from_screen(GLcontext *ctx,	GLuint offset, GLuint size,
						struct gl_buffer_object *bo)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	nouveau_buffer_object *nbo = (nouveau_buffer_object *)bo;
	nouveau_mem *in_mem;

	DEBUG("bo=%p, offset=%d, size=%d\n", bo, offset, size);

	/* If there's a permanent backing store, blit directly into it */
	if (nbo->cpu_mem) {
		if (nbo->cpu_mem != nbo->gpu_mem) {
			DEBUG("..cpu_mem\n");
			nouveau_memformat_flat_emit(ctx, nbo->cpu_mem,
						    nbo->gpu_mem,
						    offset, offset, size);
		}
	} else {
		DEBUG("..sys_mem\n");
		in_mem = nouveau_mem_alloc(ctx, NOUVEAU_MEM_AGP, size, 0);
		if (in_mem) {
			DEBUG("....via AGP\n");
			/* otherwise, try blitting to faster memory and
			 * copying from there
			 */
			nouveau_memformat_flat_emit(ctx, in_mem, nbo->gpu_mem,
							 0, offset, size);
			nouveau_notifier_wait_nop(ctx, nmesa->syncNotifier,
						       NvSubMemFormat);
			_mesa_memcpy(nbo->cpu_mem_sys + offset,
					in_mem->map, size);
			NOUVEAU_MEM_FREE(in_mem);
		} else {
			DEBUG("....direct VRAM copy\n");
			/* worst case, copy directly from vram */
			_mesa_memcpy(nbo->cpu_mem_sys + offset,
				     nbo->gpu_mem + offset,
				     size);
		}
	}

	return GL_TRUE;
}

static GLboolean
nouveau_bo_upload_to_screen(GLcontext *ctx, GLuint offset, GLuint size,
					    struct gl_buffer_object *bo)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	nouveau_buffer_object *nbo = (nouveau_buffer_object *)bo;
	nouveau_mem *out_mem;

	DEBUG("bo=%p, offset=%d, size=%d\n", bo, offset, size);

	if (nbo->cpu_mem) {
		if (nbo->cpu_mem != nbo->gpu_mem) {
			DEBUG("..cpu_mem\n");
			nouveau_memformat_flat_emit(ctx, nbo->gpu_mem,
						    nbo->cpu_mem,
						    offset, offset, size);
		}
	} else {
		out_mem = nouveau_mem_alloc(ctx, NOUVEAU_MEM_AGP |
						 NOUVEAU_MEM_MAPPED,
						 size, 0);
		if (out_mem) {
			DEBUG("....via AGP\n");
			_mesa_memcpy(out_mem->map,
					nbo->cpu_mem_sys + offset, size);
			nouveau_memformat_flat_emit(ctx, nbo->gpu_mem, out_mem,
						    offset, 0, size);
			nouveau_notifier_wait_nop(ctx, nmesa->syncNotifier,
						       NvSubMemFormat);
			NOUVEAU_MEM_FREE(out_mem);
		} else {
			DEBUG("....direct VRAM copy\n");
			_mesa_memcpy(nbo->gpu_mem->map + offset,
				     nbo->cpu_mem_sys + offset,
				     size);
		}
	}

	return GL_TRUE;
}

GLboolean
nouveau_bo_move_in(GLcontext *ctx, struct gl_buffer_object *bo)
{
	nouveau_buffer_object *nbo = (nouveau_buffer_object *)bo;

	DEBUG("bo=%p\n", bo);

	if (bo->OnCard)
		return GL_TRUE;
	assert(nbo->gpu_mem_flags);

	nbo->gpu_mem = nouveau_mem_alloc(ctx, nbo->gpu_mem_flags |
					      NOUVEAU_MEM_MAPPED,
					      bo->Size, 0);
	assert(nbo->gpu_mem);

	if (nbo->cpu_mem_flags) {
		if ((nbo->cpu_mem_flags|NOUVEAU_MEM_MAPPED) != nbo->gpu_mem->type) {
			DEBUG("..need cpu_mem buffer\n");

			nbo->cpu_mem = nouveau_mem_alloc(ctx,
							 nbo->cpu_mem_flags |
							 NOUVEAU_MEM_MAPPED,
							 bo->Size, 0);

			if (nbo->cpu_mem) {
				DEBUG("....alloc ok, kill sys_mem buffer\n");
				_mesa_memcpy(nbo->cpu_mem->map,
					     nbo->cpu_mem_sys, bo->Size);
				FREE(nbo->cpu_mem_sys);
			}
		} else {
			DEBUG("..cpu direct access to GPU buffer\n");
			nbo->cpu_mem = nbo->gpu_mem;
		}
	}
	nouveau_bo_upload_to_screen(ctx, 0, bo->Size, bo);

	bo->OnCard = GL_TRUE;
	return GL_TRUE;
}

GLboolean
nouveau_bo_move_out(GLcontext *ctx, struct gl_buffer_object *bo)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	nouveau_buffer_object *nbo = (nouveau_buffer_object *)bo;
	GLuint nr_dirty;

	DEBUG("bo=%p\n", bo);
	if (!bo->OnCard)
		return GL_TRUE;

	nr_dirty = nouveau_bo_download_dirty(ctx, bo);
	if (nbo->cpu_mem) {
		if (nr_dirty && nbo->cpu_mem != nbo->gpu_mem)
			nouveau_notifier_wait_nop(ctx, nmesa->syncNotifier,
						       NvSubMemFormat);
		DEBUG("..destroy cpu_mem buffer\n");
		nbo->cpu_mem_sys = malloc(bo->Size);
		assert(nbo->cpu_mem_sys);
		_mesa_memcpy(nbo->cpu_mem_sys, nbo->cpu_mem->map, bo->Size);
		if (nbo->cpu_mem == nbo->gpu_mem)
			nbo->cpu_mem = NULL;
		else
			NOUVEAU_MEM_FREE(nbo->cpu_mem);
	}
	NOUVEAU_MEM_FREE(nbo->gpu_mem);

	bo->OnCard = GL_FALSE;
	return GL_TRUE;
}

static void
nouveau_bo_choose_storage_method(GLcontext *ctx, GLenum usage,
						 struct gl_buffer_object *bo)
{
	nouveau_buffer_object *nbo = (nouveau_buffer_object *)bo;
	GLuint gpu_type = 0;
	GLuint cpu_type = 0;

	switch (usage) {
	/* Client source, changes often, used by GL many times */
	case GL_DYNAMIC_DRAW_ARB:
		gpu_type = NOUVEAU_MEM_AGP | NOUVEAU_MEM_FB_ACCEPTABLE;
		cpu_type = NOUVEAU_MEM_AGP;
		break;
	/* GL source, changes often, client reads many times */
	case GL_DYNAMIC_READ_ARB:
	/* Client source, specified once, used by GL many times */
	case GL_STATIC_DRAW_ARB:
	/* GL source, specified once, client reads many times */
	case GL_STATIC_READ_ARB:
	/* Client source, specified once, used by GL a few times */
	case GL_STREAM_DRAW_ARB:
	/* GL source, specified once, client reads a few times */
	case GL_STREAM_READ_ARB:
	/* GL source, changes often, used by GL many times*/
	case GL_DYNAMIC_COPY_ARB:
	/* GL source, specified once, used by GL many times */
	case GL_STATIC_COPY_ARB:
	/* GL source, specified once, used by GL a few times */
	case GL_STREAM_COPY_ARB:
		gpu_type = NOUVEAU_MEM_FB;
		break;
	default: 
		assert(0);
	}

	nbo->gpu_mem_flags = gpu_type;
	nbo->cpu_mem_flags = cpu_type;
	nbo->usage	   = usage;
}

void
nouveau_bo_init_storage(GLcontext *ctx,	GLuint valid_gpu_access,
					GLsizeiptrARB size,
					const GLvoid *data,
					GLenum usage,
					struct gl_buffer_object *bo)
{
	nouveau_buffer_object *nbo = (nouveau_buffer_object *)bo;

	DEBUG("bo=%p\n", bo);

	/* Free up previous buffers if we can't reuse them */
	if (nbo->usage != usage ||
			(nbo->gpu_mem && (nbo->gpu_mem->size != size))) {
		if (nbo->cpu_mem_sys)
			FREE(nbo->cpu_mem_sys);
		if (nbo->cpu_mem) {
			if (nbo->cpu_mem != nbo->gpu_mem)
				NOUVEAU_MEM_FREE(nbo->cpu_mem);
			else
				nbo->cpu_mem = NULL;
		}
		if (nbo->gpu_mem)
			NOUVEAU_MEM_FREE(nbo->gpu_mem);

		bo->OnCard = GL_FALSE;
		nbo->cpu_mem_sys = calloc(1, size);
	}

	nouveau_bo_choose_storage_method(ctx, usage, bo);
	/* Force off flags that may not be ok for a given buffer */
	nbo->gpu_mem_flags &= valid_gpu_access;

	bo->Usage  = usage;
	bo->Size   = size;

	if (data) {
		GLvoid *map = nouveau_bo_map(ctx, GL_WRITE_ONLY_ARB, bo);
		_mesa_memcpy(map, data, size);
		nouveau_bo_dirty_all(ctx, GL_FALSE, bo);
		nouveau_bo_unmap(ctx, bo);
	}
}

void *
nouveau_bo_map(GLcontext *ctx, GLenum access, struct gl_buffer_object *bo)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	nouveau_buffer_object *nbo = (nouveau_buffer_object *)bo;

	DEBUG("bo=%p, access=%s\n", bo, _mesa_lookup_enum_by_nr(access));

	if (bo->OnCard && 
		(access == GL_READ_ONLY_ARB || access == GL_READ_WRITE_ARB)) {
		GLuint nr_dirty;

		DEBUG("..on card\n");
		nr_dirty = nouveau_bo_download_dirty(ctx, bo);

		/* nouveau_bo_download_dirty won't wait unless it needs to
		 * free a temp buffer, which isn't the case if cpu_mem is
		 * present.
		 */
		if (nr_dirty && nbo->cpu_mem && nbo->cpu_mem != nbo->gpu_mem)
			nouveau_notifier_wait_nop(ctx, nmesa->syncNotifier,
						       NvSubMemFormat);
	}

	if (nbo->cpu_mem) {
		DEBUG("..access via cpu_mem\n");
		return nbo->cpu_mem->map;
	} else {
		DEBUG("..access via cpu_mem_sys\n");
		return nbo->cpu_mem_sys;
	}
}

void
nouveau_bo_unmap(GLcontext *ctx, struct gl_buffer_object *bo)
{
	DEBUG("unmap bo=%p\n", bo);
}

uint32_t
nouveau_bo_gpu_ref(GLcontext *ctx, struct gl_buffer_object *bo)
{
	nouveau_buffer_object *nbo = (nouveau_buffer_object *)bo;

	assert(nbo->mapped == GL_FALSE);

	DEBUG("gpu_ref\n");
	
	if (!bo->OnCard) {
		nouveau_bo_move_in(ctx, bo);
		bo->OnCard = GL_TRUE;
	}
	nouveau_bo_upload_dirty(ctx, bo);

	return nouveau_mem_gpu_offset_get(ctx, nbo->gpu_mem);
}

void
nouveau_bo_dirty_linear(GLcontext *ctx, GLboolean on_card,
			uint32_t offset, uint32_t size,
			struct gl_buffer_object *bo)
{
	nouveau_buffer_object *nbo = (nouveau_buffer_object *)bo;
	nouveau_bufferobj_dirty *dirty;
	uint32_t start = offset;
	uint32_t end = offset + size;
	int i;

	if (nbo->cpu_mem == nbo->gpu_mem)
		return;

	dirty = on_card ? &nbo->gpu_dirty : &nbo->cpu_dirty;

	DEBUG("on_card=%d, offset=%d, size=%d, bo=%p\n",
			on_card, offset, size, bo);

	for (i=0; i<dirty->nr_dirty; i++) {
		nouveau_bufferobj_region *r = &dirty->dirty[i];

		/* already dirty */
		if (start >= r->start && end <= r->end) {
			DEBUG("..already dirty\n");
			return;
		}

		/* add to the end of a region */
		if (start >= r->start && start <= r->end) {
			if (end > r->end) {
				DEBUG("..extend end of region\n");
				r->end = end;
				return;
			}
		}

		/* add to the start of a region */
		if (start < r->start && end >= r->end) {
			DEBUG("..extend start of region\n");
			r->start = start;
			/* .. and to the end */
			if (end > r->end) {
				DEBUG("....and end\n");
				r->end = end;
			}
			return;
		}
	}

	/* new region */
	DEBUG("..new dirty\n");
	dirty->nr_dirty++;
	dirty->dirty = realloc(dirty->dirty,
			       sizeof(nouveau_bufferobj_region) *
			       dirty->nr_dirty);
	dirty->dirty[dirty->nr_dirty - 1].start = start;
	dirty->dirty[dirty->nr_dirty - 1].end   = end;
}

void
nouveau_bo_dirty_all(GLcontext *ctx, GLboolean on_card,
		     struct gl_buffer_object *bo)
{
	nouveau_buffer_object *nbo = (nouveau_buffer_object *)bo;
	nouveau_bufferobj_dirty *dirty;

	dirty = on_card ? &nbo->gpu_dirty : &nbo->cpu_dirty;
	
	DEBUG("dirty all\n");
	if (dirty->nr_dirty) {
		FREE(dirty->dirty);
		dirty->dirty    = NULL;
		dirty->nr_dirty = 0;
	}

	nouveau_bo_dirty_linear(ctx, on_card, 0, bo->Size, bo);
}

GLuint
nouveau_bo_upload_dirty(GLcontext *ctx, struct gl_buffer_object *bo)
{
	nouveau_buffer_object *nbo = (nouveau_buffer_object *)bo;
	nouveau_bufferobj_dirty *dirty = &nbo->cpu_dirty;
	GLuint nr_dirty;
	int i;

	nr_dirty = dirty->nr_dirty;
	if (!nr_dirty) {
		DEBUG("clean\n");
		return nr_dirty;
	}

	for (i=0; i<nr_dirty; i++) {
		nouveau_bufferobj_region *r = &dirty->dirty[i];

		DEBUG("dirty %d: o=0x%08x, s=0x%08x\n",
				i, r->start, r->end - r->start);
		nouveau_bo_upload_to_screen(ctx,
					    r->start, r->end - r->start, bo);
	}

	FREE(dirty->dirty);
	dirty->dirty    = NULL;
	dirty->nr_dirty = 0;

	return nr_dirty;
}

GLuint
nouveau_bo_download_dirty(GLcontext *ctx, struct gl_buffer_object *bo)
{
	nouveau_buffer_object *nbo = (nouveau_buffer_object *)bo;
	nouveau_bufferobj_dirty *dirty = &nbo->gpu_dirty;
	GLuint nr_dirty;
	int i;

	nr_dirty = dirty->nr_dirty;
	if (nr_dirty) {
		DEBUG("clean\n");
		return nr_dirty;
	}
	
	for (i=0; i<nr_dirty; i++) {
		nouveau_bufferobj_region *r = &dirty->dirty[i];

		DEBUG("dirty %d: o=0x%08x, s=0x%08x\n",
				i, r->start, r->end - r->start);
		nouveau_bo_download_from_screen(ctx,
						r->start,
						r->end - r->start, bo);
	}

	FREE(dirty->dirty);
	dirty->dirty    = NULL;
	dirty->nr_dirty = 0;

	return nr_dirty;
}

static void
nouveauBindBuffer(GLcontext *ctx, GLenum target, struct gl_buffer_object *obj)
{
}

static struct gl_buffer_object *
nouveauNewBufferObject(GLcontext *ctx, GLuint buffer, GLenum target)
{
	nouveau_buffer_object *nbo;

	nbo = CALLOC_STRUCT(nouveau_buffer_object_t);
	if (nbo)
		_mesa_initialize_buffer_object(&nbo->mesa, buffer, target);
	DEBUG("bo=%p\n", nbo);

	return nbo ? &nbo->mesa : NULL;
}

static void
nouveauDeleteBuffer(GLcontext *ctx, struct gl_buffer_object *obj)
{
	nouveau_buffer_object *nbo = (nouveau_buffer_object *)obj;

	if (nbo->gpu_dirty.nr_dirty)
		FREE(nbo->gpu_dirty.dirty);
	if (nbo->cpu_dirty.nr_dirty)
		FREE(nbo->cpu_dirty.dirty);
	if (nbo->cpu_mem) nouveau_mem_free(ctx, nbo->cpu_mem);
	if (nbo->gpu_mem) nouveau_mem_free(ctx, nbo->gpu_mem);

	_mesa_delete_buffer_object(ctx, obj);
}

static void
nouveauBufferData(GLcontext *ctx, GLenum target, GLsizeiptrARB size,
		  const GLvoid *data, GLenum usage,
		  struct gl_buffer_object *obj)
{
	GLuint gpu_flags;

	DEBUG("target=%s, size=%d, data=%p, usage=%s, obj=%p\n",
			_mesa_lookup_enum_by_nr(target),
			(GLuint)size, data,
			_mesa_lookup_enum_by_nr(usage),
			obj);

	switch (target) {
	case GL_ELEMENT_ARRAY_BUFFER_ARB:
		gpu_flags = 0;
		break;
	default:
		gpu_flags = NOUVEAU_BO_VRAM_OK | NOUVEAU_BO_AGP_OK;
		break;
	}
	nouveau_bo_init_storage(ctx, gpu_flags, size, data, usage, obj);
}

static void
nouveauBufferSubData(GLcontext *ctx, GLenum target, GLintptrARB offset,
		     GLsizeiptrARB size, const GLvoid *data,
		     struct gl_buffer_object *obj)
{
	GLvoid *out;

	DEBUG("target=%s, offset=0x%x, size=%d, data=%p, obj=%p\n",
			_mesa_lookup_enum_by_nr(target),
			(GLuint)offset, (GLuint)size, data, obj);

	out = nouveau_bo_map(ctx, GL_WRITE_ONLY_ARB, obj);
	_mesa_memcpy(out + offset, data, size);
	nouveau_bo_dirty_linear(ctx, GL_FALSE, offset, size, obj);
	nouveau_bo_unmap(ctx, obj);
}

static void
nouveauGetBufferSubData(GLcontext *ctx, GLenum target, GLintptrARB offset,
		     GLsizeiptrARB size, GLvoid *data,
		     struct gl_buffer_object *obj)
{
	const GLvoid *in;

	DEBUG("target=%s, offset=0x%x, size=%d, data=%p, obj=%p\n",
			_mesa_lookup_enum_by_nr(target),
			(GLuint)offset, (GLuint)size, data, obj);

	in = nouveau_bo_map(ctx, GL_READ_ONLY_ARB, obj);
	_mesa_memcpy(data, in + offset, size);
	nouveau_bo_unmap(ctx, obj);
}

static void *
nouveauMapBuffer(GLcontext *ctx, GLenum target, GLenum access,
		 struct gl_buffer_object *obj)
{
	DEBUG("target=%s, access=%s, obj=%p\n",
			_mesa_lookup_enum_by_nr(target),
			_mesa_lookup_enum_by_nr(access),
			obj
			);

	/* Already mapped.. */
	if (obj->Pointer)
		return NULL;

	/* Have to pass READ_WRITE here, nouveau_bo_map will only ensure that
	 * the cpu_mem buffer is up-to-date if we ask for read access.
	 *
	 * However, even if the client only asks for write access, we're still
	 * forced to reupload the entire buffer.  So, we need the cpu_mem buffer
	 * to have the correct data all the time.
	 */
	obj->Pointer = nouveau_bo_map(ctx, GL_READ_WRITE_ARB, obj);

	/* The GL spec says that a client attempting to write to a bufferobj
	 * mapped READ_ONLY object may have unpredictable results, possibly
	 * even program termination.
	 *
	 * We're going to use this, and only mark the buffer as dirtied if
	 * the client asks for write access.
	 */
	if (target != GL_READ_ONLY_ARB) {
		/* We have no way of knowing what was modified by the client,
		 * so the entire buffer gets dirtied. */
		nouveau_bo_dirty_all(ctx, GL_FALSE, obj);
	}

	return obj->Pointer;
}

static GLboolean
nouveauUnmapBuffer(GLcontext *ctx, GLenum target, struct gl_buffer_object *obj)
{
	DEBUG("target=%s, obj=%p\n", _mesa_lookup_enum_by_nr(target), obj);

	assert(obj->Pointer);

	nouveau_bo_unmap(ctx, obj);
	obj->Pointer = NULL;
	return GL_TRUE;
}
	  
void
nouveauInitBufferObjects(GLcontext *ctx)
{
	ctx->Driver.BindBuffer		= nouveauBindBuffer;
	ctx->Driver.NewBufferObject	= nouveauNewBufferObject;
	ctx->Driver.DeleteBuffer	= nouveauDeleteBuffer;
	ctx->Driver.BufferData		= nouveauBufferData;
	ctx->Driver.BufferSubData	= nouveauBufferSubData;
	ctx->Driver.GetBufferSubData	= nouveauGetBufferSubData;
	ctx->Driver.MapBuffer		= nouveauMapBuffer;
	ctx->Driver.UnmapBuffer		= nouveauUnmapBuffer;
}

