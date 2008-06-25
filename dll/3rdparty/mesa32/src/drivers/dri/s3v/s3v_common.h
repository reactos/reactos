/*
 * Author: Max Lingua <sunmax@libero.it>
 */

/* WARNING: If you change any of these defines, make sure to change
 * the kernel include file as well (s3v_drm.h)
 */

#ifndef _XF86DRI_S3V_H_
#define _XF86DRI_S3V_H_

#ifndef _S3V_DEFINES_
#define _S3V_DEFINES_
#define S3V_USE_BATCH 1

/* #define S3V_BUF_4K 1 */

#ifdef S3V_BUF_4K
#define S3V_DMA_BUF_ORDER 12
#define S3V_DMA_BUF_NR    256
#else
#define S3V_DMA_BUF_ORDER 16 /* -much- better */
#define S3V_DMA_BUF_NR    16
#endif
/* on s3virge you can only choose between *
 * 4k (2^12) and 64k (2^16) dma bufs      */
#define S3V_DMA_BUF_SZ        (1<<S3V_DMA_BUF_ORDER)

#define S3V_NR_SAREA_CLIPRECTS 8

/* Each region is a minimum of 16k (64*64@4bpp)
 * and there are at most 40 of them.
 */
#define S3V_NR_TEX_REGIONS 64 /* was 40 */
#define S3V_LOG_TEX_GRANULARITY 16 /* was 4 */
/* 40 * (2 ^ 4) = 640k, that's all we have for tex on 4mb gfx card */
/* FIXME: will it work with card with less than 4mb? */
/* FIXME: we should set this at run time */

#endif  /* _S3V_DEFINES */

/*
 * WARNING: If you change any of these defines, make sure to change
 * the kernel include file as well (gamma_drm.h)
 */

/* Driver specific DRM command indices
 * NOTE: these are not OS specific, but they are driver specific
 */
#define DRM_S3V_INIT_DMA              0x00
#define DRM_S3V_CLEANUP_DMA           0x01

typedef struct _drmS3VInit {
    enum {
        S3V_INIT_DMA = 0x01,
        S3V_CLEANUP_DMA = 0x02
    } func;

    unsigned int pcimode;   /* bool: 1=pci 0=agp */

    unsigned int mmio_offset;
    unsigned int buffers_offset;
    unsigned int sarea_priv_offset;

    unsigned int front_offset;
    unsigned int front_width;
    unsigned int front_height;
    unsigned int front_pitch;

    unsigned int back_offset;
    unsigned int back_width;
    unsigned int back_height;
    unsigned int back_pitch;

    unsigned int depth_offset;
    unsigned int depth_width;
    unsigned int depth_height;
    unsigned int depth_pitch;

    unsigned int texture_offset;
} drmS3VInit;

#endif
