/*
 * Copyright 2006 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * This implements a software-only "DRI" driver.  It doesn't actually speak
 * any DRI protocol or talk to the DRM, it just looks enough like a DRI driver
 * that libglx in the server can load it for software rendering in the
 * unaccelerated case.
 */

static GLboolean
glcoreInitDriver(__DRIscreenPrivate *driScreenPriv)
{
}

static void
glcoreDestroyScreen(__DRIScreenPrivate *driScreenPriv)
{
}

static GLboolean
glcoreCreateContext(const __GLcontextModes *glVisual,
                    __DRIcontextPrivate *driContextPriv,
                    void *shared_context)
{
}

static void
glcoreDestroyContext(__DRIcontextPrivate *driContextPriv)
{
}

static GLboolean
glcoreCreateBuffer(__DRIscreenPrivate *driScreenPriv,
                   __DRIdrawablePrivate *driDrawablePriv,
                   const __GLcontextModes *mesaVisual,
                   GLboolean isPixmap)
{
}

static void
glcoreDestroyBuffer(__DRIdrawablePrivate *driDrawablePriv)
{
}

static void
glcoreSwapBuffers(__DRIdrawablePrivate *driDrawablePriv)
{
}

static GLboolean
glcoreMakeCurrent(__DRIcontextPrivate *driContextPriv,
                  __DRIdrawablePrivate *driDrawablePriv,
                  __DRIdrawablePrivate *driReadablePriv)
{
}

static GLboolean
glcoreUnbindContext(__DRIcontextPrivate *driContextPriv)
{
}

static struct __DriverAPIRec glcore_api = {
    .InitDriver     = glcoreInitDriver,
    .DestroyScreen  = glcoreDestroyScreen,
    .CreateContext  = glcoreCreateContext,
    .DestroyContext = glcoreDestroyContext,
    .CreateBuffer   = glcoreCreateBuffer,
    .DestroyBuffer  = glcoreDestroyBuffer,
    .SwapBuffers    = glcoreSwapBuffers,
    .MakeCurrent    = glcoreMakeCurrent,
    .UnbindContext  = glcoreUnbindContext,
};  

static __GLcontextModes *
glcoreFillInModes(unsigned pixel_bits)
{
}

PUBLIC void *
__driCreateNewScreen_20050727(__DRInativeDisplay *dpy, int scrn,
                              __DRIscreen *psc, const __GLcontextModes *modes,
                              const __DRIversion *ddx_version,
                              const __DRIversion *dri_version,
                              const __DRIversion *drm_version,
                              const __DRIframebuffer *fb, drmAddress pSarea,
                              int fd, int internal_api_version,
                              const ___DRIinterfaceMethods *interface,
                              __GLcontextModes **driver_modes)
{
    __DRIscreenPrivate *driScreenPriv;
    glcoreDriverPrivate *glcoreDriverPriv;
    
    /* would normally check ddx/dri/drm versions here */

    driScreenPriv = __driUtilCreateNewScreen(dpy, scrn, psc, NULL, ddx_version,
                                             dri_version, drm_version, fb,
                                             internal_api_version, &glcore_api);
    if (!driScreenPriv)
        return NULL;

    glcoreDriverPriv = driScreenPriv->pDrvPriv;

    *driver_modes = glcoreFillInModes(glcoreDriverPriv->bpp);

    driInitExtensions(NULL, NULL, GL_FALSE);

    return driScreenPriv;
}
