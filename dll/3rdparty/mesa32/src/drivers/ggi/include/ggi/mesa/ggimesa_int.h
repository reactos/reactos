#ifndef _GGI_MESA_INT_H
#define _GGI_MESA_INT_H

#include <ggi/internal/internal.h>
#include "ggimesa.h"


extern ggi_extid _ggiMesaID;

ggifunc_setmode GGIMesa_setmode;
ggifunc_getapi GGIMesa_getapi;

typedef struct ggi_mesa_ext
{
	/*
	 * How mesa extends this visual; i.e., size of the depth buffer etc.
	 *
	 * By default (upon attaching) this structure is initialized to what
	 * libggi is guaranteed to handle without any help: single buffered
	 * visual without any ancilary buffers.
	 */
	struct ggi_mesa_visual mesa_visual;

	/*
	 * Mesa framebuffer is a collection of all ancilary buffers required.
	 *
	 * This structure contains the ancilary buffers provided in in
	 * software. On each mode change it is loaded with the list of
	 * required buffers and the target is expected to clear the ones
	 * it can provide in hw. The remaining ones are then provided in sw.
	 *
	 */
	GLframebuffer mesa_buffer;

	void (*update_state)(ggi_mesa_context_t ctx);
	int (*setup_driver)(ggi_mesa_context_t ctx);
	
	void *private;
} ggi_mesa_ext_t;

#define LIBGGI_MESAEXT(vis) ((ggi_mesa_ext_t *)LIBGGI_EXT(vis,_ggiMesaID))
#define GGIMESA_PRIV(vis) ((LIBGGI_MESAEXT(vis)->priv))

#endif /* _GGI_MISC_INT_H */
