typedef struct _gammaRegion {
    drm_handle_t  handle;
    drmSize    size;
    drmAddress map;
} gammaRegion, *gammaRegionPtr;

typedef struct {

    int           regionCount;       /* Count of register regions */
    gammaRegion  *regions;           /* Vector of mapped region info */

    drmBufMapPtr  bufs;              /* Map of DMA buffers */

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

    int		textureSize;
    int		logTextureGranularity;
} gammaScreenRec, *gammaScreenPtr;
