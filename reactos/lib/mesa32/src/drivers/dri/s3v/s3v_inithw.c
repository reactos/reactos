/*
 * Author: Max Lingua <sunmax@libero.it>
 */

#include <sys/ioctl.h>

#include "s3v_context.h"

void s3vInitHW( s3vContextPtr vmesa )
{
	int i;
	static short _reset = 1;

	DEBUG(("vmesa->driDrawable = %p\n", vmesa->driDrawable));
	DEBUG(("stride = %i\n",
		vmesa->driScreen->fbWidth*vmesa->s3vScreen->cpp));
	DEBUG(("frontOffset = 0x%x\n", vmesa->s3vScreen->frontOffset));
	DEBUG(("backOffset = 0x%x\n", vmesa->s3vScreen->backOffset));
	DEBUG(("depthOffset = 0x%x\n", vmesa->s3vScreen->depthOffset));
	DEBUG(("textureOffset = 0x%x\n", vmesa->s3vScreen->texOffset));

/*	if (_reset) { */
/*	ioctl(vmesa->driFd, 0x4a); */
		ioctl(vmesa->driFd, 0x41); /* reset */
		_reset = 0;
/*	ioctl(vmesa->driFd, 0x4c); */
/*	} */

	/* FIXME */
	switch (vmesa->s3vScreen->cpp) {
		case 2:
			break;
		case 4:
			break;
	}

	/* FIXME for stencil, gid, etc */
	switch (vmesa->DepthSize) {
		case 15:
		case 16:
			break;
		case 24:
			break;
		case 32:
			break;
	}

	vmesa->FogMode = 1;
	vmesa->ClearDepth = 0xffff;
	vmesa->x = 0;
	vmesa->y = 0;
	vmesa->w = 0;
	vmesa->h = 0;
	vmesa->FrameCount = 0;
	vmesa->MatrixMode = GL_MODELVIEW;
	vmesa->ModelViewCount = 0;
	vmesa->ProjCount = 0;
	vmesa->TextureCount = 0;


	/* FIXME: do we need the following? */

	for (i = 0; i < 16; i++)
		if (i % 5 == 0)
			vmesa->ModelView[i] =
			vmesa->Proj[i] =
			vmesa->ModelViewProj[i] =
			vmesa->Texture[i] = 1.0;
		else
			vmesa->ModelView[i] =
			vmesa->Proj[i] =
			vmesa->ModelViewProj[i] =
			vmesa->Texture[i] = 0.0;

	vmesa->LBWindowBase = vmesa->driScreen->fbWidth *
				(vmesa->driScreen->fbHeight - 1);
	vmesa->FBWindowBase = vmesa->driScreen->fbWidth * 
				(vmesa->driScreen->fbHeight - 1);
}
