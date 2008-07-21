/*
 * Author: Max Lingua <sunmax@libero.it>
 */

#include "mtypes.h"

typedef struct _s3vRegion {
    drm_handle_t  handle;
    drmSize    size;
    drmAddress map;
} s3vRegion, *s3vRegionPtr;

typedef struct {

    int          regionCount;  	   /* Count of register regions */
    s3vRegion  	 *regions;         /* Vector of mapped region info */

    drmBufMapPtr bufs;             /* Map of DMA buffers */

    __DRIscreenPrivate *driScreen; /* Back pointer to DRI screen */

    int		cpp;
    int		frontPitch;
    int		frontOffset;

    int		backPitch;
    int		backOffset;
    int		backX;
    int		backY;

    int		depthOffset;
    int		depthPitch;

    int		texOffset;
    int		textureOffset;
    int		textureSize;
    int		logTextureGranularity;
} s3vScreenRec, *s3vScreenPtr;

