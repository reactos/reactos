/*
 * Direct3D state management
 *
 * Copyright 2002 Lionel Ulmer
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006 Henri Verbeet
 * Copyright 2006-2007 Stefan Dösinger for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include <stdio.h>
#ifdef HAVE_FLOAT_H
# include <float.h>
#endif
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(d3d_shader);

#define GLINFO_LOCATION stateblock->wineD3DDevice->adapter->gl_info

static void state_nogl(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    /* Used for states which are not mapped to a gl state as-is, but used somehow different,
     * e.g as a parameter for drawing, or which are unimplemented in windows d3d
     */
    if(STATE_IS_RENDER(state)) {
        WINED3DRENDERSTATETYPE RenderState = state - STATE_RENDER(0);
        TRACE("(%s,%d) no direct mapping to gl\n", debug_d3drenderstate(RenderState), stateblock->renderState[RenderState]);
    } else {
        /* Shouldn't have an unknown type here */
        FIXME("%d no direct mapping to gl of state with unknown type\n", state);
    }
}

static void state_undefined(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    /* Print a WARN, this allows the stateblock code to loop over all states to generate a display
     * list without causing confusing terminal output. Deliberately no special debug name here
     * because its undefined.
     */
    WARN("undefined state %d\n", state);
}

static void state_fillmode(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    WINED3DFILLMODE Value = stateblock->renderState[WINED3DRS_FILLMODE];

    switch(Value) {
        case WINED3DFILL_POINT:
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            checkGLcall("glPolygonMode(GL_FRONT_AND_BACK, GL_POINT)");
            break;
        case WINED3DFILL_WIREFRAME:
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            checkGLcall("glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)");
            break;
        case WINED3DFILL_SOLID:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            checkGLcall("glPolygonMode(GL_FRONT_AND_BACK, GL_FILL)");
            break;
        default:
            FIXME("Unrecognized WINED3DRS_FILLMODE value %d\n", Value);
    }
}

static void state_lighting(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    BOOL transformed;

    /* Lighting is not enabled if transformed vertices are drawn
     * but lighting does not affect the stream sources, so it is not grouped for performance reasons.
     * This state reads the decoded vertex decl, so if it is dirty don't do anything. The
     * vertex declaration appplying function calls this function for updating
     */

    if(isStateDirty(context, STATE_VDECL)) {
        return;
    }

    transformed = ((stateblock->wineD3DDevice->strided_streams.u.s.position.lpData != NULL ||
                    stateblock->wineD3DDevice->strided_streams.u.s.position.VBO != 0) &&
                    stateblock->wineD3DDevice->strided_streams.u.s.position_transformed) ? TRUE : FALSE;

    if (stateblock->renderState[WINED3DRS_LIGHTING] && !transformed) {
        glEnable(GL_LIGHTING);
        checkGLcall("glEnable GL_LIGHTING");
    } else {
        glDisable(GL_LIGHTING);
        checkGLcall("glDisable GL_LIGHTING");
    }
}

static void state_zenable(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    /* No z test without depth stencil buffers */
    if(stateblock->wineD3DDevice->stencilBufferTarget == NULL) {
        glDisable(GL_DEPTH_TEST); /* This also disables z writing in gl */
        checkGLcall("glDisable GL_DEPTH_TEST");
        return;
    }

    switch ((WINED3DZBUFFERTYPE) stateblock->renderState[WINED3DRS_ZENABLE]) {
        case WINED3DZB_FALSE:
            glDisable(GL_DEPTH_TEST);
            checkGLcall("glDisable GL_DEPTH_TEST");
            break;
        case WINED3DZB_TRUE:
            glEnable(GL_DEPTH_TEST);
            checkGLcall("glEnable GL_DEPTH_TEST");
            break;
        case WINED3DZB_USEW:
            glEnable(GL_DEPTH_TEST);
            checkGLcall("glEnable GL_DEPTH_TEST");
            FIXME("W buffer is not well handled\n");
            break;
        default:
            FIXME("Unrecognized D3DZBUFFERTYPE value %d\n", stateblock->renderState[WINED3DRS_ZENABLE]);
    }
}

static void state_cullmode(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    /* glFrontFace() is set in context.c at context init and on an offscreen / onscreen rendering
     * switch
     */
    switch ((WINED3DCULL) stateblock->renderState[WINED3DRS_CULLMODE]) {
        case WINED3DCULL_NONE:
            glDisable(GL_CULL_FACE);
            checkGLcall("glDisable GL_CULL_FACE");
            break;
        case WINED3DCULL_CW:
            glEnable(GL_CULL_FACE);
            checkGLcall("glEnable GL_CULL_FACE");
            glCullFace(GL_FRONT);
            checkGLcall("glCullFace(GL_FRONT)");
            break;
        case WINED3DCULL_CCW:
            glEnable(GL_CULL_FACE);
            checkGLcall("glEnable GL_CULL_FACE");
            glCullFace(GL_BACK);
            checkGLcall("glCullFace(GL_BACK)");
            break;
        default:
            FIXME("Unrecognized/Unhandled WINED3DCULL value %d\n", stateblock->renderState[WINED3DRS_CULLMODE]);
    }
}

static void state_shademode(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    switch ((WINED3DSHADEMODE) stateblock->renderState[WINED3DRS_SHADEMODE]) {
        case WINED3DSHADE_FLAT:
            glShadeModel(GL_FLAT);
            checkGLcall("glShadeModel(GL_FLAT)");
            break;
        case WINED3DSHADE_GOURAUD:
            glShadeModel(GL_SMOOTH);
            checkGLcall("glShadeModel(GL_SMOOTH)");
            break;
        case WINED3DSHADE_PHONG:
            FIXME("WINED3DSHADE_PHONG isn't supported\n");
            break;
        default:
            FIXME("Unrecognized/Unhandled WINED3DSHADEMODE value %d\n", stateblock->renderState[WINED3DRS_SHADEMODE]);
    }
}

static void state_ditherenable(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    if (stateblock->renderState[WINED3DRS_DITHERENABLE]) {
        glEnable(GL_DITHER);
        checkGLcall("glEnable GL_DITHER");
    } else {
        glDisable(GL_DITHER);
        checkGLcall("glDisable GL_DITHER");
    }
}

static void state_zwritenable(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    /* TODO: Test if in d3d z writing is enabled even if ZENABLE is off. If yes,
     * this has to be merged with ZENABLE and ZFUNC
     */
    if (stateblock->renderState[WINED3DRS_ZWRITEENABLE]) {
        glDepthMask(1);
        checkGLcall("glDepthMask(1)");
    } else {
        glDepthMask(0);
        checkGLcall("glDepthMask(0)");
    }
}

static void state_zfunc(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    int glParm = CompareFunc(stateblock->renderState[WINED3DRS_ZFUNC]);

    if(glParm) {
        if(glParm == GL_EQUAL || glParm == GL_NOTEQUAL) {
            static BOOL once = FALSE;
            /* There are a few issues with this: First, our inability to
             * select a proper Z depth, most of the time we're stuck with
             * D24S8, even if the app selects D32 or D16. There seem to be
             * some other precision problems which have to be debugged to
             * make NOTEQUAL and EQUAL work properly
             */
            if(!once) {
                once = TRUE;
                FIXME("D3DCMP_NOTEQUAL and D3DCMP_EQUAL do not work correctly yet\n");
            }
        }

        glDepthFunc(glParm);
        checkGLcall("glDepthFunc");
    }
}

static void state_ambient(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    float col[4];
    D3DCOLORTOGLFLOAT4(stateblock->renderState[WINED3DRS_AMBIENT], col);

    TRACE("Setting ambient to (%f,%f,%f,%f)\n", col[0], col[1], col[2], col[3]);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, col);
    checkGLcall("glLightModel for MODEL_AMBIENT");
}

static void state_blend(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    int srcBlend = GL_ZERO;
    int dstBlend = GL_ZERO;

    /* GL_LINE_SMOOTH needs GL_BLEND to work, according to the red book, and special blending params */
    if (stateblock->renderState[WINED3DRS_ALPHABLENDENABLE]      ||
        stateblock->renderState[WINED3DRS_EDGEANTIALIAS]         ||
        stateblock->renderState[WINED3DRS_ANTIALIASEDLINEENABLE]) {
        glEnable(GL_BLEND);
        checkGLcall("glEnable GL_BLEND");
    } else {
        glDisable(GL_BLEND);
        checkGLcall("glDisable GL_BLEND");
        /* Nothing more to do - get out */
        return;
    };

    switch (stateblock->renderState[WINED3DRS_DESTBLEND]) {
        case WINED3DBLEND_ZERO               : dstBlend = GL_ZERO;  break;
        case WINED3DBLEND_ONE                : dstBlend = GL_ONE;  break;
        case WINED3DBLEND_SRCCOLOR           : dstBlend = GL_SRC_COLOR;  break;
        case WINED3DBLEND_INVSRCCOLOR        : dstBlend = GL_ONE_MINUS_SRC_COLOR;  break;
        case WINED3DBLEND_SRCALPHA           : dstBlend = GL_SRC_ALPHA;  break;
        case WINED3DBLEND_INVSRCALPHA        : dstBlend = GL_ONE_MINUS_SRC_ALPHA;  break;
        case WINED3DBLEND_DESTALPHA          : dstBlend = GL_DST_ALPHA;  break;
        case WINED3DBLEND_INVDESTALPHA       : dstBlend = GL_ONE_MINUS_DST_ALPHA;  break;
        case WINED3DBLEND_DESTCOLOR          : dstBlend = GL_DST_COLOR;  break;
        case WINED3DBLEND_INVDESTCOLOR       : dstBlend = GL_ONE_MINUS_DST_COLOR;  break;

        case WINED3DBLEND_SRCALPHASAT        :
            dstBlend = GL_SRC_ALPHA_SATURATE;
            WARN("Application uses SRCALPHASAT as dest blend factor, expect problems\n");
            break;

        /* WINED3DBLEND_BOTHSRCALPHA and WINED3DBLEND_BOTHINVSRCALPHA are legacy source blending
         * values which are still valid up to d3d9. They should not occur as dest blend values
         */
        case WINED3DBLEND_BOTHSRCALPHA       : dstBlend = GL_SRC_ALPHA;
            srcBlend = GL_SRC_ALPHA;
            FIXME("WINED3DRS_DESTBLEND = WINED3DBLEND_BOTHSRCALPHA, what to do?\n");
            break;

        case WINED3DBLEND_BOTHINVSRCALPHA    : dstBlend = GL_ONE_MINUS_SRC_ALPHA;
            srcBlend = GL_ONE_MINUS_SRC_ALPHA;
            FIXME("WINED3DRS_DESTBLEND = WINED3DBLEND_BOTHINVSRCALPHA, what to do?\n");
            break;

        case WINED3DBLEND_BLENDFACTOR        : dstBlend = GL_CONSTANT_COLOR;   break;
        case WINED3DBLEND_INVBLENDFACTOR     : dstBlend = GL_ONE_MINUS_CONSTANT_COLOR;  break;
        default:
            FIXME("Unrecognized dst blend value %d\n", stateblock->renderState[WINED3DRS_DESTBLEND]);
    }

    switch (stateblock->renderState[WINED3DRS_SRCBLEND]) {
        case WINED3DBLEND_ZERO               : srcBlend = GL_ZERO;  break;
        case WINED3DBLEND_ONE                : srcBlend = GL_ONE;  break;
        case WINED3DBLEND_SRCCOLOR           : srcBlend = GL_SRC_COLOR;  break;
        case WINED3DBLEND_INVSRCCOLOR        : srcBlend = GL_ONE_MINUS_SRC_COLOR;  break;
        case WINED3DBLEND_SRCALPHA           : srcBlend = GL_SRC_ALPHA;  break;
        case WINED3DBLEND_INVSRCALPHA        : srcBlend = GL_ONE_MINUS_SRC_ALPHA;  break;
        case WINED3DBLEND_DESTALPHA          : srcBlend = GL_DST_ALPHA;  break;
        case WINED3DBLEND_INVDESTALPHA       : srcBlend = GL_ONE_MINUS_DST_ALPHA;  break;
        case WINED3DBLEND_DESTCOLOR          : srcBlend = GL_DST_COLOR;  break;
        case WINED3DBLEND_INVDESTCOLOR       : srcBlend = GL_ONE_MINUS_DST_COLOR;  break;
        case WINED3DBLEND_SRCALPHASAT        : srcBlend = GL_SRC_ALPHA_SATURATE;  break;

        case WINED3DBLEND_BOTHSRCALPHA       : srcBlend = GL_SRC_ALPHA;
            dstBlend = GL_ONE_MINUS_SRC_ALPHA;
            break;

        case WINED3DBLEND_BOTHINVSRCALPHA    : srcBlend = GL_ONE_MINUS_SRC_ALPHA;
            dstBlend = GL_SRC_ALPHA;
            break;

        case WINED3DBLEND_BLENDFACTOR        : srcBlend = GL_CONSTANT_COLOR;   break;
        case WINED3DBLEND_INVBLENDFACTOR     : srcBlend = GL_ONE_MINUS_CONSTANT_COLOR;  break;
        default:
            FIXME("Unrecognized src blend value %d\n", stateblock->renderState[WINED3DRS_SRCBLEND]);
    }


    if(stateblock->renderState[WINED3DRS_EDGEANTIALIAS] ||
       stateblock->renderState[WINED3DRS_ANTIALIASEDLINEENABLE]) {
        glEnable(GL_LINE_SMOOTH);
        checkGLcall("glEnable(GL_LINE_SMOOTH)");
        if(srcBlend != GL_SRC_ALPHA) {
            WARN("WINED3DRS_EDGEANTIALIAS enabled, but unexpected src blending param\n");
        }
        if(dstBlend != GL_ONE_MINUS_SRC_ALPHA && dstBlend != GL_ONE) {
            WARN("WINED3DRS_EDGEANTIALIAS enabled, but unexpected dst blending param\n");
        }
    } else {
        glDisable(GL_LINE_SMOOTH);
        checkGLcall("glDisable(GL_LINE_SMOOTH)");
    }

    TRACE("glBlendFunc src=%x, dst=%x\n", srcBlend, dstBlend);
    glBlendFunc(srcBlend, dstBlend);
    checkGLcall("glBlendFunc");
}

static void state_blendfactor(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    float col[4];

    TRACE("Setting BlendFactor to %d\n", stateblock->renderState[WINED3DRS_BLENDFACTOR]);
    D3DCOLORTOGLFLOAT4(stateblock->renderState[WINED3DRS_BLENDFACTOR], col);
    GL_EXTCALL(glBlendColorEXT (col[0],col[1],col[2],col[3]));
    checkGLcall("glBlendColor");
}

static void state_alpha(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    int glParm = 0;
    float ref;
    BOOL enable_ckey = FALSE;

    IWineD3DSurfaceImpl *surf;

    /* Find out if the texture on the first stage has a ckey set
     * The alpha state func reads the texture settings, even though alpha and texture are not grouped
     * together. This is to avoid making a huge alpha+texture+texture stage+ckey block due to the hardly
     * used WINED3DRS_COLORKEYENABLE state(which is d3d <= 3 only). The texture function will call alpha
     * in case it finds some texture+colorkeyenable combination which needs extra care.
     */
    if(stateblock->textures[0] && stateblock->textureDimensions[0] == GL_TEXTURE_2D) {
        surf = (IWineD3DSurfaceImpl *) ((IWineD3DTextureImpl *)stateblock->textures[0])->surfaces[0];

        if(surf->CKeyFlags & WINEDDSD_CKSRCBLT) {
            const StaticPixelFormatDesc *fmt = getFormatDescEntry(surf->resource.format, NULL, NULL);
            /* The surface conversion does not do color keying conversion for surfaces that have an alpha
             * channel on their own. Likewise, the alpha test shouldn't be set up for color keying if the
             * surface has alpha bits
             */
            if(fmt->alphaMask == 0x00000000) {
                enable_ckey = TRUE;
            }
        }
    }

    if(enable_ckey || context->last_was_ckey) {
        StateTable[STATE_TEXTURESTAGE(0, WINED3DTSS_ALPHAOP)].apply(STATE_TEXTURESTAGE(0, WINED3DTSS_ALPHAOP), stateblock, context);
    }
    context->last_was_ckey = enable_ckey;

    if (stateblock->renderState[WINED3DRS_ALPHATESTENABLE] ||
        (stateblock->renderState[WINED3DRS_COLORKEYENABLE] && enable_ckey)) {
        glEnable(GL_ALPHA_TEST);
        checkGLcall("glEnable GL_ALPHA_TEST");
    } else {
        glDisable(GL_ALPHA_TEST);
        checkGLcall("glDisable GL_ALPHA_TEST");
        /* Alpha test is disabled, don't bother setting the params - it will happen on the next
         * enable call
         */
        return;
    }

    if(stateblock->renderState[WINED3DRS_COLORKEYENABLE] && enable_ckey) {
        glParm = GL_NOTEQUAL;
        ref = 0.0;
    } else {
        ref = ((float) stateblock->renderState[WINED3DRS_ALPHAREF]) / 255.0f;
        glParm = CompareFunc(stateblock->renderState[WINED3DRS_ALPHAFUNC]);
    }
    if(glParm) {
        glAlphaFunc(glParm, ref);
        checkGLcall("glAlphaFunc");
    }
}

static void state_clipping(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    DWORD enable  = 0xFFFFFFFF;
    DWORD disable = 0x00000000;

    if (use_vs(stateblock->wineD3DDevice)) {
        /* The spec says that opengl clipping planes are disabled when using shaders. Direct3D planes aren't,
         * so that is an issue. The MacOS ATI driver keeps clipping planes activated with shaders in some
         * contitions I got sick of tracking down. The shader state handler disables all clip planes because
         * of that - don't do anything here and keep them disabled
         */
        if(stateblock->renderState[WINED3DRS_CLIPPLANEENABLE]) {
            static BOOL warned = FALSE;
            if(!warned) {
                FIXME("Clipping not supported with vertex shaders\n");
                warned = TRUE;
            }
        }
        return;
    }

    /* TODO: Keep track of previously enabled clipplanes to avoid unnecessary resetting
     * of already set values
     */

    /* If enabling / disabling all
     * TODO: Is this correct? Doesn't D3DRS_CLIPPING disable clipping on the viewport frustrum?
     */
    if (stateblock->renderState[WINED3DRS_CLIPPING]) {
        enable  = stateblock->renderState[WINED3DRS_CLIPPLANEENABLE];
        disable = ~stateblock->renderState[WINED3DRS_CLIPPLANEENABLE];
        if(GL_SUPPORT(NV_DEPTH_CLAMP)) {
            glDisable(GL_DEPTH_CLAMP_NV);
            checkGLcall("glDisable(GL_DEPTH_CLAMP_NV)");
        }
    } else {
        disable = 0xffffffff;
        enable  = 0x00;
        if(GL_SUPPORT(NV_DEPTH_CLAMP)) {
            glEnable(GL_DEPTH_CLAMP_NV);
            checkGLcall("glEnable(GL_DEPTH_CLAMP_NV)");
        }
    }

    if (enable & WINED3DCLIPPLANE0)  { glEnable(GL_CLIP_PLANE0);  checkGLcall("glEnable(clip plane 0)"); }
    if (enable & WINED3DCLIPPLANE1)  { glEnable(GL_CLIP_PLANE1);  checkGLcall("glEnable(clip plane 1)"); }
    if (enable & WINED3DCLIPPLANE2)  { glEnable(GL_CLIP_PLANE2);  checkGLcall("glEnable(clip plane 2)"); }
    if (enable & WINED3DCLIPPLANE3)  { glEnable(GL_CLIP_PLANE3);  checkGLcall("glEnable(clip plane 3)"); }
    if (enable & WINED3DCLIPPLANE4)  { glEnable(GL_CLIP_PLANE4);  checkGLcall("glEnable(clip plane 4)"); }
    if (enable & WINED3DCLIPPLANE5)  { glEnable(GL_CLIP_PLANE5);  checkGLcall("glEnable(clip plane 5)"); }

    if (disable & WINED3DCLIPPLANE0) { glDisable(GL_CLIP_PLANE0); checkGLcall("glDisable(clip plane 0)"); }
    if (disable & WINED3DCLIPPLANE1) { glDisable(GL_CLIP_PLANE1); checkGLcall("glDisable(clip plane 1)"); }
    if (disable & WINED3DCLIPPLANE2) { glDisable(GL_CLIP_PLANE2); checkGLcall("glDisable(clip plane 2)"); }
    if (disable & WINED3DCLIPPLANE3) { glDisable(GL_CLIP_PLANE3); checkGLcall("glDisable(clip plane 3)"); }
    if (disable & WINED3DCLIPPLANE4) { glDisable(GL_CLIP_PLANE4); checkGLcall("glDisable(clip plane 4)"); }
    if (disable & WINED3DCLIPPLANE5) { glDisable(GL_CLIP_PLANE5); checkGLcall("glDisable(clip plane 5)"); }

    /** update clipping status */
    if (enable) {
        stateblock->clip_status.ClipUnion = 0;
        stateblock->clip_status.ClipIntersection = 0xFFFFFFFF;
    } else {
        stateblock->clip_status.ClipUnion = 0;
        stateblock->clip_status.ClipIntersection = 0;
    }
}

static void state_blendop(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    int glParm = GL_FUNC_ADD;

    if(!GL_SUPPORT(EXT_BLEND_MINMAX)) {
        WARN("Unsupported in local OpenGL implementation: glBlendEquation\n");
        return;
    }

    switch ((WINED3DBLENDOP) stateblock->renderState[WINED3DRS_BLENDOP]) {
        case WINED3DBLENDOP_ADD              : glParm = GL_FUNC_ADD;              break;
        case WINED3DBLENDOP_SUBTRACT         : glParm = GL_FUNC_SUBTRACT;         break;
        case WINED3DBLENDOP_REVSUBTRACT      : glParm = GL_FUNC_REVERSE_SUBTRACT; break;
        case WINED3DBLENDOP_MIN              : glParm = GL_MIN;                   break;
        case WINED3DBLENDOP_MAX              : glParm = GL_MAX;                   break;
        default:
            FIXME("Unrecognized/Unhandled D3DBLENDOP value %d\n", stateblock->renderState[WINED3DRS_BLENDOP]);
    }

    TRACE("glBlendEquation(%x)\n", glParm);
    GL_EXTCALL(glBlendEquationEXT(glParm));
    checkGLcall("glBlendEquation");
}

static void
state_specularenable(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    /* Originally this used glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SEPARATE_SPECULAR_COLOR)
     * and (GL_LIGHT_MODEL_COLOR_CONTROL,GL_SINGLE_COLOR) to swap between enabled/disabled
     * specular color. This is wrong:
     * Separate specular color means the specular colour is maintained separately, whereas
     * single color means it is merged in. However in both cases they are being used to
     * some extent.
     * To disable specular color, set it explicitly to black and turn off GL_COLOR_SUM_EXT
     * NOTE: If not supported don't give FIXMEs the impact is really minimal and very few people are
     * running 1.4 yet!
     *
     *
     * If register combiners are enabled, enabling / disabling GL_COLOR_SUM has no effect.
     * Instead, we need to setup the FinalCombiner properly.
     *
     * The default setup for the FinalCombiner is:
     *
     * <variable>       <input>                             <mapping>               <usage>
     * GL_VARIABLE_A_NV GL_FOG,                             GL_UNSIGNED_IDENTITY_NV GL_ALPHA
     * GL_VARIABLE_B_NV GL_SPARE0_PLUS_SECONDARY_COLOR_NV   GL_UNSIGNED_IDENTITY_NV GL_RGB
     * GL_VARIABLE_C_NV GL_FOG                              GL_UNSIGNED_IDENTITY_NV GL_RGB
     * GL_VARIABLE_D_NV GL_ZERO                             GL_UNSIGNED_IDENTITY_NV GL_RGB
     * GL_VARIABLE_E_NV GL_ZERO                             GL_UNSIGNED_IDENTITY_NV GL_RGB
     * GL_VARIABLE_F_NV GL_ZERO                             GL_UNSIGNED_IDENTITY_NV GL_RGB
     * GL_VARIABLE_G_NV GL_SPARE0_NV                        GL_UNSIGNED_IDENTITY_NV GL_ALPHA
     *
     * That's pretty much fine as it is, except for variable B, which needs to take
     * either GL_SPARE0_PLUS_SECONDARY_COLOR_NV or GL_SPARE0_NV, depending on
     * whether WINED3DRS_SPECULARENABLE is enabled or not.
     */

    TRACE("Setting specular enable state and materials\n");
    if (stateblock->renderState[WINED3DRS_SPECULARENABLE]) {
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, (float*) &stateblock->material.Specular);
        checkGLcall("glMaterialfv");

        if(stateblock->material.Power > 128.0) {
            /* glMaterialf man page says that the material says that GL_SHININESS must be between 0.0
             * and 128.0, although in d3d neither -1 nor 129 produce an error. For values > 128 clamp
             * them, since 128 results in a hardly visible specular highlight, so it should be safe to
             * to clamp to 128
             */
            WARN("Material power > 128\n");
            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 128.0);
        } else {
            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, stateblock->material.Power);
        }
        checkGLcall("glMaterialf(GL_SHININESS");

        if (GL_SUPPORT(EXT_SECONDARY_COLOR)) {
            glEnable(GL_COLOR_SUM_EXT);
        } else {
            TRACE("Specular colors cannot be enabled in this version of opengl\n");
        }
        checkGLcall("glEnable(GL_COLOR_SUM)");

        if (GL_SUPPORT(NV_REGISTER_COMBINERS)) {
            GL_EXTCALL(glFinalCombinerInputNV(GL_VARIABLE_B_NV, GL_SPARE0_PLUS_SECONDARY_COLOR_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB));
            checkGLcall("glFinalCombinerInputNV()");
        }
    } else {
        float black[4] = {0.0f, 0.0f, 0.0f, 0.0f};

        /* for the case of enabled lighting: */
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &black[0]);
        checkGLcall("glMaterialfv");

        /* for the case of disabled lighting: */
        if (GL_SUPPORT(EXT_SECONDARY_COLOR)) {
            glDisable(GL_COLOR_SUM_EXT);
        } else {
            TRACE("Specular colors cannot be disabled in this version of opengl\n");
        }
        checkGLcall("glDisable(GL_COLOR_SUM)");

        if (GL_SUPPORT(NV_REGISTER_COMBINERS)) {
            GL_EXTCALL(glFinalCombinerInputNV(GL_VARIABLE_B_NV, GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB));
            checkGLcall("glFinalCombinerInputNV()");
        }
    }

    TRACE("(%p) : Diffuse (%f,%f,%f,%f)\n", stateblock->wineD3DDevice, stateblock->material.Diffuse.r, stateblock->material.Diffuse.g,
          stateblock->material.Diffuse.b, stateblock->material.Diffuse.a);
    TRACE("(%p) : Ambient (%f,%f,%f,%f)\n", stateblock->wineD3DDevice, stateblock->material.Ambient.r, stateblock->material.Ambient.g,
          stateblock->material.Ambient.b, stateblock->material.Ambient.a);
    TRACE("(%p) : Specular (%f,%f,%f,%f)\n", stateblock->wineD3DDevice, stateblock->material.Specular.r, stateblock->material.Specular.g,
          stateblock->material.Specular.b, stateblock->material.Specular.a);
    TRACE("(%p) : Emissive (%f,%f,%f,%f)\n", stateblock->wineD3DDevice, stateblock->material.Emissive.r, stateblock->material.Emissive.g,
          stateblock->material.Emissive.b, stateblock->material.Emissive.a);

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, (float*) &stateblock->material.Ambient);
    checkGLcall("glMaterialfv(GL_AMBIENT)");
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, (float*) &stateblock->material.Diffuse);
    checkGLcall("glMaterialfv(GL_DIFFUSE)");
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, (float*) &stateblock->material.Emissive);
    checkGLcall("glMaterialfv(GL_EMISSION)");
}

static void state_texfactor(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    unsigned int i;

    /* Note the texture color applies to all textures whereas
     * GL_TEXTURE_ENV_COLOR applies to active only
     */
    float col[4];
    D3DCOLORTOGLFLOAT4(stateblock->renderState[WINED3DRS_TEXTUREFACTOR], col);

    if (!GL_SUPPORT(NV_REGISTER_COMBINERS)) {
        /* And now the default texture color as well */
        for (i = 0; i < GL_LIMITS(texture_stages); i++) {
            /* Note the WINED3DRS value applies to all textures, but GL has one
             * per texture, so apply it now ready to be used!
             */
            if (GL_SUPPORT(ARB_MULTITEXTURE)) {
                GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + i));
                checkGLcall("glActiveTextureARB");
            } else if (i>0) {
                FIXME("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
            }

            glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, &col[0]);
            checkGLcall("glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);");
        }
    } else {
        GL_EXTCALL(glCombinerParameterfvNV(GL_CONSTANT_COLOR0_NV, &col[0]));
    }
}

static void
renderstate_stencil_twosided(IWineD3DStateBlockImpl *stateblock, GLint face, GLint func, GLint ref, GLuint mask, GLint stencilFail, GLint depthFail, GLint stencilPass ) {
#if 0 /* Don't use OpenGL 2.0 calls for now */
            if(GL_EXTCALL(glStencilFuncSeparate) && GL_EXTCALL(glStencilOpSeparate)) {
                GL_EXTCALL(glStencilFuncSeparate(face, func, ref, mask));
                checkGLcall("glStencilFuncSeparate(...)");
                GL_EXTCALL(glStencilOpSeparate(face, stencilFail, depthFail, stencilPass));
                checkGLcall("glStencilOpSeparate(...)");
        }
            else
#endif
    if(GL_SUPPORT(EXT_STENCIL_TWO_SIDE)) {
        glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);
        checkGLcall("glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT)");
        GL_EXTCALL(glActiveStencilFaceEXT(face));
        checkGLcall("glActiveStencilFaceEXT(...)");
        glStencilFunc(func, ref, mask);
        checkGLcall("glStencilFunc(...)");
        glStencilOp(stencilFail, depthFail, stencilPass);
        checkGLcall("glStencilOp(...)");
    } else if(GL_SUPPORT(ATI_SEPARATE_STENCIL)) {
        GL_EXTCALL(glStencilFuncSeparateATI(face, func, ref, mask));
        checkGLcall("glStencilFuncSeparateATI(...)");
        GL_EXTCALL(glStencilOpSeparateATI(face, stencilFail, depthFail, stencilPass));
        checkGLcall("glStencilOpSeparateATI(...)");
    } else {
        ERR("Separate (two sided) stencil not supported on this version of opengl. Caps weren't honored?\n");
    }
}

static void
state_stencil(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    DWORD onesided_enable = FALSE;
    DWORD twosided_enable = FALSE;
    GLint func = GL_ALWAYS;
    GLint func_ccw = GL_ALWAYS;
    GLint ref = 0;
    GLuint mask = 0;
    GLint stencilFail = GL_KEEP;
    GLint depthFail = GL_KEEP;
    GLint stencilPass = GL_KEEP;
    GLint stencilFail_ccw = GL_KEEP;
    GLint depthFail_ccw = GL_KEEP;
    GLint stencilPass_ccw = GL_KEEP;

    /* No stencil test without a stencil buffer */
    if(stateblock->wineD3DDevice->stencilBufferTarget == NULL) {
        glDisable(GL_STENCIL_TEST);
        checkGLcall("glDisable GL_STENCIL_TEST");
        return;
    }

    onesided_enable = stateblock->renderState[WINED3DRS_STENCILENABLE];
    twosided_enable = stateblock->renderState[WINED3DRS_TWOSIDEDSTENCILMODE];
    if( !( func = CompareFunc(stateblock->renderState[WINED3DRS_STENCILFUNC]) ) )
        func = GL_ALWAYS;
    if( !( func_ccw = CompareFunc(stateblock->renderState[WINED3DRS_CCW_STENCILFUNC]) ) )
        func_ccw = GL_ALWAYS;
    ref = stateblock->renderState[WINED3DRS_STENCILREF];
    mask = stateblock->renderState[WINED3DRS_STENCILMASK];
    stencilFail = StencilOp(stateblock->renderState[WINED3DRS_STENCILFAIL]);
    depthFail = StencilOp(stateblock->renderState[WINED3DRS_STENCILZFAIL]);
    stencilPass = StencilOp(stateblock->renderState[WINED3DRS_STENCILPASS]);
    stencilFail_ccw = StencilOp(stateblock->renderState[WINED3DRS_CCW_STENCILFAIL]);
    depthFail_ccw = StencilOp(stateblock->renderState[WINED3DRS_CCW_STENCILZFAIL]);
    stencilPass_ccw = StencilOp(stateblock->renderState[WINED3DRS_CCW_STENCILPASS]);

    TRACE("(onesided %d, twosided %d, ref %x, mask %x, "
          "GL_FRONT: func: %x, fail %x, zfail %x, zpass %x "
          "GL_BACK: func: %x, fail %x, zfail %x, zpass %x )\n",
    onesided_enable, twosided_enable, ref, mask,
    func, stencilFail, depthFail, stencilPass,
    func_ccw, stencilFail_ccw, depthFail_ccw, stencilPass_ccw);

    if (twosided_enable && onesided_enable) {
        glEnable(GL_STENCIL_TEST);
        checkGLcall("glEnable GL_STENCIL_TEST");

        /* Apply back first, then front. This function calls glActiveStencilFaceEXT,
         * which has an effect on the code below too. If we apply the front face
         * afterwards, we are sure that the active stencil face is set to front,
         * and other stencil functions which do not use two sided stencil do not have
         * to set it back
         */
        renderstate_stencil_twosided(stateblock, GL_BACK, func_ccw, ref, mask, stencilFail_ccw, depthFail_ccw, stencilPass_ccw);
        renderstate_stencil_twosided(stateblock, GL_FRONT, func, ref, mask, stencilFail, depthFail, stencilPass);
    } else if(onesided_enable) {
        if(GL_SUPPORT(EXT_STENCIL_TWO_SIDE)) {
            glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
            checkGLcall("glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT)");
        }

        glEnable(GL_STENCIL_TEST);
        checkGLcall("glEnable GL_STENCIL_TEST");
        glStencilFunc(func, ref, mask);
        checkGLcall("glStencilFunc(...)");
        glStencilOp(stencilFail, depthFail, stencilPass);
        checkGLcall("glStencilOp(...)");
    } else {
        glDisable(GL_STENCIL_TEST);
        checkGLcall("glDisable GL_STENCIL_TEST");
    }
}

static void state_stencilwrite(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    DWORD mask;

    if(stateblock->wineD3DDevice->stencilBufferTarget) {
        mask = stateblock->renderState[WINED3DRS_STENCILWRITEMASK];
    } else {
        mask = 0;
    }

    if(GL_SUPPORT(EXT_STENCIL_TWO_SIDE)) {
        GL_EXTCALL(glActiveStencilFaceEXT(GL_BACK));
        checkGLcall("glActiveStencilFaceEXT(GL_BACK)");
        glStencilMask(mask);
        checkGLcall("glStencilMask");
        GL_EXTCALL(glActiveStencilFaceEXT(GL_FRONT));
        checkGLcall("glActiveStencilFaceEXT(GL_FRONT)");
        glStencilMask(mask);
    } else {
        glStencilMask(mask);
    }
    checkGLcall("glStencilMask");
}

static void state_fog(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    /* TODO: Put this into the vertex type block once that is in the state table */
    BOOL fogenable = stateblock->renderState[WINED3DRS_FOGENABLE];
    BOOL is_ps3 = use_ps(stateblock->wineD3DDevice)
                  && ((IWineD3DPixelShaderImpl *)stateblock->pixelShader)->baseShader.hex_version >= WINED3DPS_VERSION(3,0);
    float fogstart, fogend;

    union {
        DWORD d;
        float f;
    } tmpvalue;

    if (!fogenable) {
        /* No fog? Disable it, and we're done :-) */
        glDisable(GL_FOG);
        checkGLcall("glDisable GL_FOG");
        if( use_ps(stateblock->wineD3DDevice)
                && ((IWineD3DPixelShaderImpl *)stateblock->pixelShader)->baseShader.hex_version < WINED3DPS_VERSION(3,0) ) {
            /* disable fog in the pixel shader
             * NOTE: For pixel shader, GL_FOG_START and GL_FOG_END don't hold fog start s and end e but
             * -1/(e-s) and e/(e-s) respectively.
             */
            glFogf(GL_FOG_START, 0.0f);
            checkGLcall("glFogf(GL_FOG_START, fogstart");
            glFogf(GL_FOG_END, 1.0f);
            checkGLcall("glFogf(GL_FOG_END, fogend");
        }
        return;
    }

    tmpvalue.d = stateblock->renderState[WINED3DRS_FOGSTART];
    fogstart = tmpvalue.f;
    tmpvalue.d = stateblock->renderState[WINED3DRS_FOGEND];
    fogend = tmpvalue.f;

    /* Fog Rules:
     *
     * With fixed function vertex processing, Direct3D knows 2 different fog input sources.
     * It can use the Z value of the vertex, or the alpha component of the specular color.
     * This depends on the fog vertex, fog table and the vertex declaration. If the Z value
     * is used, fogstart, fogend and the equation type are used, otherwise linear fog with
     * start = 255, end = 0 is used. Obviously the msdn is not very clear on that.
     *
     * FOGTABLEMODE != NONE:
     *  The Z value is used, with the equation specified, no matter what vertex type.
     *
     * FOGTABLEMODE == NONE, FOGVERTEXMODE != NONE, untransformed:
     *  Per vertex fog is calculated using the specified fog equation and the parameters
     *
     * FOGTABLEMODE == NONE, FOGVERTEXMODE != NONE, transformed, OR
     * FOGTABLEMODE == NONE, FOGVERTEXMODE == NONE, untransformed:
     *  Linear fog with start = 255.0, end = 0.0, input comes from the specular color
     *
     *
     * Rules for vertex fog with shaders:
     *
     * When mixing fixed function functionality with the programmable pipeline, D3D expects
     * the fog computation to happen during transformation while openGL expects it to happen
     * during rasterization. Also, prior to pixel shader 3.0 D3D handles fog blending after
     * the pixel shader while openGL always expects the pixel shader to handle the blending.
     * To solve this problem, WineD3D does:
     * 1) implement a linear fog equation and fog blending at the end of every pre 3.0 pixel
     * shader,
     * and 2) disables the fog computation (in either the fixed function or programmable
     * rasterizer) if using a vertex program.
     *
     *
     * If a fogtablemode and a fogvertexmode are specified, table fog is applied (with or
     * without shaders).
     */

    if( is_ps3 ) {
        if( !use_vs(stateblock->wineD3DDevice)
                && stateblock->renderState[WINED3DRS_FOGTABLEMODE] == WINED3DFOG_NONE ) {
            FIXME("Implement vertex fog for pixel shader >= 3.0 and fixed function pipeline\n");
        }
    }

    if (use_vs(stateblock->wineD3DDevice)
            && ((IWineD3DVertexShaderImpl *)stateblock->vertexShader)->baseShader.reg_maps.fog) {
        if( stateblock->renderState[WINED3DRS_FOGTABLEMODE] != WINED3DFOG_NONE ) {
            if(!is_ps3) FIXME("Implement table fog for foggy vertex shader\n");
            /* Disable fog */
            fogenable = FALSE;
        } else {
            /* Set fog computation in the rasterizer to pass through the value (just blend it) */
            glFogi(GL_FOG_MODE, GL_LINEAR);
            checkGLcall("glFogi(GL_FOG_MODE, GL_LINEAR)");
            fogstart = 1.0;
            fogend = 0.0;
        }

        if(GL_SUPPORT(EXT_FOG_COORD) && context->fog_coord) {
            glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT);
            checkGLcall("glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT");
            context->fog_coord = FALSE;
        }
        context->last_was_foggy_shader = TRUE;
    }
    else if( use_ps(stateblock->wineD3DDevice) ) {
        /* NOTE: For pixel shader, GL_FOG_START and GL_FOG_END don't hold fog start s and end e but
         * -1/(e-s) and e/(e-s) respectively to simplify fog computation in the shader.
         */
        WINED3DFOGMODE mode;
        context->last_was_foggy_shader = FALSE;

        /* If both fogmodes are set use the table fog mode */
        if(stateblock->renderState[WINED3DRS_FOGTABLEMODE] == WINED3DFOG_NONE)
            mode = stateblock->renderState[WINED3DRS_FOGVERTEXMODE];
        else
            mode = stateblock->renderState[WINED3DRS_FOGTABLEMODE];

        switch (mode) {
            case WINED3DFOG_EXP:
            case WINED3DFOG_EXP2:
                if(!is_ps3) FIXME("Implement non linear fog for pixel shader < 3.0\n");
                /* Disable fog */
                fogenable = FALSE;
                break;

            case WINED3DFOG_LINEAR:
                fogstart = -1.0f/(fogend-fogstart);
                fogend *= -fogstart;
                break;

            case WINED3DFOG_NONE:
                if(!is_ps3) FIXME("Implement software vertex fog for pixel shader < 3.0\n");
                /* Disable fog */
                fogenable = FALSE;
                break;
            default: FIXME("Unexpected WINED3DRS_FOGVERTEXMODE %d\n", stateblock->renderState[WINED3DRS_FOGVERTEXMODE]);
        }

        if(GL_SUPPORT(EXT_FOG_COORD) && context->fog_coord) {
            glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT);
            checkGLcall("glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT");
            context->fog_coord = FALSE;
        }
    }
    /* DX 7 sdk: "If both render states(vertex and table fog) are set to valid modes,
     * the system will apply only pixel(=table) fog effects."
     */
    else if(stateblock->renderState[WINED3DRS_FOGTABLEMODE] == WINED3DFOG_NONE) {
        glHint(GL_FOG_HINT, GL_FASTEST);
        checkGLcall("glHint(GL_FOG_HINT, GL_FASTEST)");
        context->last_was_foggy_shader = FALSE;

        switch (stateblock->renderState[WINED3DRS_FOGVERTEXMODE]) {
            /* If processed vertices are used, fall through to the NONE case */
            case WINED3DFOG_EXP:  {
                if(!context->last_was_rhw) {
                    glFogi(GL_FOG_MODE, GL_EXP);
                    checkGLcall("glFogi(GL_FOG_MODE, GL_EXP");
                    if(GL_SUPPORT(EXT_FOG_COORD) && context->fog_coord) {
                        glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT);
                        checkGLcall("glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT");
                        context->fog_coord = FALSE;
                    }
                    break;
                }
            }
            case WINED3DFOG_EXP2: {
                if(!context->last_was_rhw) {
                    glFogi(GL_FOG_MODE, GL_EXP2);
                    checkGLcall("glFogi(GL_FOG_MODE, GL_EXP2");
                    if(GL_SUPPORT(EXT_FOG_COORD) && context->fog_coord) {
                        glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT);
                        checkGLcall("glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT");
                        context->fog_coord = FALSE;
                    }
                    break;
                }
            }
            case WINED3DFOG_LINEAR: {
                if(!context->last_was_rhw) {
                    glFogi(GL_FOG_MODE, GL_LINEAR);
                    checkGLcall("glFogi(GL_FOG_MODE, GL_LINEAR");
                    if(GL_SUPPORT(EXT_FOG_COORD) && context->fog_coord) {
                        glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT);
                        checkGLcall("glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT");
                        context->fog_coord = FALSE;
                    }
                    break;
                }
            }
            case WINED3DFOG_NONE: {
                /* Both are none? According to msdn the alpha channel of the specular
                 * color contains a fog factor. Set it in drawStridedSlow.
                 * Same happens with Vertexfog on transformed vertices
                 */
                if(GL_SUPPORT(EXT_FOG_COORD)) {
                    if(context->fog_coord == FALSE) {
                        glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FOG_COORDINATE_EXT);
                        checkGLcall("glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FOG_COORDINATE_EXT)\n");
                        context->fog_coord = TRUE;
                    }
                    glFogi(GL_FOG_MODE, GL_LINEAR);
                    checkGLcall("glFogi(GL_FOG_MODE, GL_LINEAR)");
                    fogstart = 0xff;
                    fogend = 0x0;
                } else {
                    /* Disable GL fog, handle this in software in drawStridedSlow */
                    fogenable = FALSE;
                }
                break;
            }
            default: FIXME("Unexpected WINED3DRS_FOGVERTEXMODE %d\n", stateblock->renderState[WINED3DRS_FOGVERTEXMODE]);
        }
    } else {
        glHint(GL_FOG_HINT, GL_NICEST);
        checkGLcall("glHint(GL_FOG_HINT, GL_NICEST)");
        context->last_was_foggy_shader = FALSE;

        switch (stateblock->renderState[WINED3DRS_FOGTABLEMODE]) {
            case WINED3DFOG_EXP:
                glFogi(GL_FOG_MODE, GL_EXP);
                checkGLcall("glFogi(GL_FOG_MODE, GL_EXP");
                if(GL_SUPPORT(EXT_FOG_COORD) && context->fog_coord) {
                    glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT);
                    checkGLcall("glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT");
                    context->fog_coord = FALSE;
                }
                break;

            case WINED3DFOG_EXP2:
                glFogi(GL_FOG_MODE, GL_EXP2);
                checkGLcall("glFogi(GL_FOG_MODE, GL_EXP2");
                if(GL_SUPPORT(EXT_FOG_COORD) && context->fog_coord) {
                    glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT);
                    checkGLcall("glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT");
                    context->fog_coord = FALSE;
                }
                break;

            case WINED3DFOG_LINEAR:
                glFogi(GL_FOG_MODE, GL_LINEAR);
                checkGLcall("glFogi(GL_FOG_MODE, GL_LINEAR");
                if(GL_SUPPORT(EXT_FOG_COORD) && context->fog_coord) {
                    glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT);
                    checkGLcall("glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT");
                    context->fog_coord = FALSE;
                }
                break;

            case WINED3DFOG_NONE:   /* Won't happen */
            default:
                FIXME("Unexpected WINED3DRS_FOGTABLEMODE %d\n", stateblock->renderState[WINED3DRS_FOGTABLEMODE]);
        }
    }

    if(fogenable) {
        glEnable(GL_FOG);
        checkGLcall("glEnable GL_FOG");

        if(fogstart != fogend)
        {
            glFogfv(GL_FOG_START, &fogstart);
            checkGLcall("glFogf(GL_FOG_START, fogstart");
            TRACE("Fog Start == %f\n", fogstart);

            glFogfv(GL_FOG_END, &fogend);
            checkGLcall("glFogf(GL_FOG_END, fogend");
            TRACE("Fog End == %f\n", fogend);
        }
        else
        {
            glFogf(GL_FOG_START, -1.0 / 0.0);
            checkGLcall("glFogf(GL_FOG_START, fogstart");
            TRACE("Fog Start == %f\n", fogstart);

            glFogf(GL_FOG_END, 0.0);
            checkGLcall("glFogf(GL_FOG_END, fogend");
            TRACE("Fog End == %f\n", fogend);
        }
    } else {
        glDisable(GL_FOG);
        checkGLcall("glDisable GL_FOG");
        if( use_ps(stateblock->wineD3DDevice) ) {
            /* disable fog in the pixel shader
             * NOTE: For pixel shader, GL_FOG_START and GL_FOG_END don't hold fog start s and end e but
             * -1/(e-s) and e/(e-s) respectively.
             */
            glFogf(GL_FOG_START, 0.0f);
            checkGLcall("glFogf(GL_FOG_START, fogstart");
            glFogf(GL_FOG_END, 1.0f);
            checkGLcall("glFogf(GL_FOG_END, fogend");
        }
    }
}

static void state_rangefog(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    if(stateblock->renderState[WINED3DRS_RANGEFOGENABLE]) {
        if (GL_SUPPORT(NV_FOG_DISTANCE)) {
            glFogi(GL_FOG_DISTANCE_MODE_NV, GL_EYE_RADIAL_NV);
            checkGLcall("glFogi(GL_FOG_DISTANCE_MODE_NV, GL_EYE_RADIAL_NV)");
        } else {
            WARN("Range fog enabled, but not supported by this opengl implementation\n");
        }
    } else {
        if (GL_SUPPORT(NV_FOG_DISTANCE)) {
            glFogi(GL_FOG_DISTANCE_MODE_NV, GL_EYE_PLANE_ABSOLUTE_NV);
            checkGLcall("glFogi(GL_FOG_DISTANCE_MODE_NV, GL_EYE_PLANE_ABSOLUTE_NV)");
        }
    }
}

static void state_fogcolor(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    float col[4];
    D3DCOLORTOGLFLOAT4(stateblock->renderState[WINED3DRS_FOGCOLOR], col);
    glFogfv(GL_FOG_COLOR, &col[0]);
    checkGLcall("glFog GL_FOG_COLOR");
}

static void state_fogdensity(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    union {
        DWORD d;
        float f;
    } tmpvalue;
    tmpvalue.d = stateblock->renderState[WINED3DRS_FOGDENSITY];
    glFogfv(GL_FOG_DENSITY, &tmpvalue.f);
    checkGLcall("glFogf(GL_FOG_DENSITY, (float) Value)");
}

/* TODO: Merge with primitive type + init_materials()!! */
static void state_colormat(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *)stateblock->wineD3DDevice;
    GLenum Parm = 0;
    WineDirect3DStridedData *diffuse = &device->strided_streams.u.s.diffuse;
    BOOL isDiffuseSupplied;

    /* Depends on the decoded vertex declaration to read the existence of diffuse data.
     * The vertex declaration will call this function if the fixed function pipeline is used.
     */

    if(isStateDirty(context, STATE_VDECL)) {
        return;
    }

    isDiffuseSupplied = diffuse->lpData || diffuse->VBO;

    context->num_untracked_materials = 0;
    if (isDiffuseSupplied && stateblock->renderState[WINED3DRS_COLORVERTEX]) {
        TRACE("diff %d, amb %d, emis %d, spec %d\n",
              stateblock->renderState[WINED3DRS_DIFFUSEMATERIALSOURCE],
              stateblock->renderState[WINED3DRS_AMBIENTMATERIALSOURCE],
              stateblock->renderState[WINED3DRS_EMISSIVEMATERIALSOURCE],
              stateblock->renderState[WINED3DRS_SPECULARMATERIALSOURCE]);

        if (stateblock->renderState[WINED3DRS_DIFFUSEMATERIALSOURCE] == WINED3DMCS_COLOR1) {
            if (stateblock->renderState[WINED3DRS_AMBIENTMATERIALSOURCE] == WINED3DMCS_COLOR1) {
                Parm = GL_AMBIENT_AND_DIFFUSE;
            } else {
                Parm = GL_DIFFUSE;
            }
            if(stateblock->renderState[WINED3DRS_EMISSIVEMATERIALSOURCE] == WINED3DMCS_COLOR1) {
                context->untracked_materials[context->num_untracked_materials] = GL_EMISSION;
                context->num_untracked_materials++;
            }
            if(stateblock->renderState[WINED3DRS_SPECULARMATERIALSOURCE] == WINED3DMCS_COLOR1) {
                context->untracked_materials[context->num_untracked_materials] = GL_SPECULAR;
                context->num_untracked_materials++;
            }
        } else if (stateblock->renderState[WINED3DRS_AMBIENTMATERIALSOURCE] == WINED3DMCS_COLOR1) {
            Parm = GL_AMBIENT;
            if(stateblock->renderState[WINED3DRS_EMISSIVEMATERIALSOURCE] == WINED3DMCS_COLOR1) {
                context->untracked_materials[context->num_untracked_materials] = GL_EMISSION;
                context->num_untracked_materials++;
            }
            if(stateblock->renderState[WINED3DRS_SPECULARMATERIALSOURCE] == WINED3DMCS_COLOR1) {
                context->untracked_materials[context->num_untracked_materials] = GL_SPECULAR;
                context->num_untracked_materials++;
            }
        } else if (stateblock->renderState[WINED3DRS_EMISSIVEMATERIALSOURCE] == WINED3DMCS_COLOR1) {
            Parm = GL_EMISSION;
            if(stateblock->renderState[WINED3DRS_SPECULARMATERIALSOURCE] == WINED3DMCS_COLOR1) {
                context->untracked_materials[context->num_untracked_materials] = GL_SPECULAR;
                context->num_untracked_materials++;
            }
        } else if (stateblock->renderState[WINED3DRS_SPECULARMATERIALSOURCE] == WINED3DMCS_COLOR1) {
            Parm = GL_SPECULAR;
        }
    }

    /* Nothing changed, return. */
    if (Parm == context->tracking_parm) return;

    if(!Parm) {
        glDisable(GL_COLOR_MATERIAL);
        checkGLcall("glDisable GL_COLOR_MATERIAL");
    } else {
        glColorMaterial(GL_FRONT_AND_BACK, Parm);
        checkGLcall("glColorMaterial(GL_FRONT_AND_BACK, Parm)");
        glEnable(GL_COLOR_MATERIAL);
        checkGLcall("glEnable(GL_COLOR_MATERIAL)");
    }

    /* Apparently calls to glMaterialfv are ignored for properties we're
     * tracking with glColorMaterial, so apply those here. */
    switch (context->tracking_parm) {
        case GL_AMBIENT_AND_DIFFUSE:
            glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, (float*)&device->updateStateBlock->material.Ambient);
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, (float*)&device->updateStateBlock->material.Diffuse);
            checkGLcall("glMaterialfv");
            break;

        case GL_DIFFUSE:
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, (float*)&device->updateStateBlock->material.Diffuse);
            checkGLcall("glMaterialfv");
            break;

        case GL_AMBIENT:
            glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, (float*)&device->updateStateBlock->material.Ambient);
            checkGLcall("glMaterialfv");
            break;

        case GL_EMISSION:
            glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, (float*)&device->updateStateBlock->material.Emissive);
            checkGLcall("glMaterialfv");
            break;

        case GL_SPECULAR:
            /* Only change material color if specular is enabled, otherwise it is set to black */
            if (device->stateBlock->renderState[WINED3DRS_SPECULARENABLE]) {
                glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, (float*)&device->updateStateBlock->material.Specular);
                checkGLcall("glMaterialfv");
            } else {
                float black[4] = {0.0f, 0.0f, 0.0f, 0.0f};
                glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &black[0]);
                checkGLcall("glMaterialfv");
            }
            break;
    }

    context->tracking_parm = Parm;
}

static void state_linepattern(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    union {
        DWORD                 d;
        WINED3DLINEPATTERN    lp;
    } tmppattern;
    tmppattern.d = stateblock->renderState[WINED3DRS_LINEPATTERN];

    TRACE("Line pattern: repeat %d bits %x\n", tmppattern.lp.wRepeatFactor, tmppattern.lp.wLinePattern);

    if (tmppattern.lp.wRepeatFactor) {
        glLineStipple(tmppattern.lp.wRepeatFactor, tmppattern.lp.wLinePattern);
        checkGLcall("glLineStipple(repeat, linepattern)");
        glEnable(GL_LINE_STIPPLE);
        checkGLcall("glEnable(GL_LINE_STIPPLE);");
    } else {
        glDisable(GL_LINE_STIPPLE);
        checkGLcall("glDisable(GL_LINE_STIPPLE);");
    }
}

static void state_zbias(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    union {
        DWORD d;
        float f;
    } tmpvalue;

    if (stateblock->renderState[WINED3DRS_ZBIAS]) {
        tmpvalue.d = stateblock->renderState[WINED3DRS_ZBIAS];
        TRACE("ZBias value %f\n", tmpvalue.f);
        glPolygonOffset(0, -tmpvalue.f);
        checkGLcall("glPolygonOffset(0, -Value)");
        glEnable(GL_POLYGON_OFFSET_FILL);
        checkGLcall("glEnable(GL_POLYGON_OFFSET_FILL);");
        glEnable(GL_POLYGON_OFFSET_LINE);
        checkGLcall("glEnable(GL_POLYGON_OFFSET_LINE);");
        glEnable(GL_POLYGON_OFFSET_POINT);
        checkGLcall("glEnable(GL_POLYGON_OFFSET_POINT);");
    } else {
        glDisable(GL_POLYGON_OFFSET_FILL);
        checkGLcall("glDisable(GL_POLYGON_OFFSET_FILL);");
        glDisable(GL_POLYGON_OFFSET_LINE);
        checkGLcall("glDisable(GL_POLYGON_OFFSET_LINE);");
        glDisable(GL_POLYGON_OFFSET_POINT);
        checkGLcall("glDisable(GL_POLYGON_OFFSET_POINT);");
    }
}


static void state_normalize(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    if(isStateDirty(context, STATE_VDECL)) {
        return;
    }
    /* Without vertex normals, we set the current normal to 0/0/0 to remove the diffuse factor
     * from the opengl lighting equation, as d3d does. Normalization of 0/0/0 can lead to a division
     * by zero and is not properly defined in opengl, so avoid it
     */
    if (stateblock->renderState[WINED3DRS_NORMALIZENORMALS] && (
        stateblock->wineD3DDevice->strided_streams.u.s.normal.lpData ||
        stateblock->wineD3DDevice->strided_streams.u.s.normal.VBO)) {
        glEnable(GL_NORMALIZE);
        checkGLcall("glEnable(GL_NORMALIZE);");
    } else {
        glDisable(GL_NORMALIZE);
        checkGLcall("glDisable(GL_NORMALIZE);");
    }
}

static void state_psizemin(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    union {
        DWORD d;
        float f;
    } tmpvalue;

    tmpvalue.d = stateblock->renderState[WINED3DRS_POINTSIZE_MIN];
    if(GL_SUPPORT(ARB_POINT_PARAMETERS)) {
        GL_EXTCALL(glPointParameterfARB)(GL_POINT_SIZE_MIN_ARB, tmpvalue.f);
        checkGLcall("glPointParameterfARB(...");
    }
    else if(GL_SUPPORT(EXT_POINT_PARAMETERS)) {
        GL_EXTCALL(glPointParameterfEXT)(GL_POINT_SIZE_MIN_EXT, tmpvalue.f);
        checkGLcall("glPointParameterfEXT(...);");
    } else if(tmpvalue.f != 1.0) {
        FIXME("WINED3DRS_POINTSIZE_MIN not supported on this opengl, value is %f\n", tmpvalue.f);
    }
}

static void state_psizemax(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    union {
        DWORD d;
        float f;
    } tmpvalue;

    tmpvalue.d = stateblock->renderState[WINED3DRS_POINTSIZE_MAX];
    if(GL_SUPPORT(ARB_POINT_PARAMETERS)) {
        GL_EXTCALL(glPointParameterfARB)(GL_POINT_SIZE_MAX_ARB, tmpvalue.f);
        checkGLcall("glPointParameterfARB(...");
    }
    else if(GL_SUPPORT(EXT_POINT_PARAMETERS)) {
        GL_EXTCALL(glPointParameterfEXT)(GL_POINT_SIZE_MAX_EXT, tmpvalue.f);
        checkGLcall("glPointParameterfEXT(...);");
    } else if(tmpvalue.f != 64.0) {
        FIXME("WINED3DRS_POINTSIZE_MAX not supported on this opengl, value is %f\n", tmpvalue.f);
    }
}

static void state_pscale(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    /* TODO: Group this with the viewport */
    /*
     * POINTSCALEENABLE controls how point size value is treated. If set to
     * true, the point size is scaled with respect to height of viewport.
     * When set to false point size is in pixels.
     *
     * http://msdn.microsoft.com/library/en-us/directx9_c/point_sprites.asp
     */

    /* Default values */
    GLfloat att[3] = {1.0f, 0.0f, 0.0f};
    union {
        DWORD d;
        float f;
    } pointSize, A, B, C;

    pointSize.d = stateblock->renderState[WINED3DRS_POINTSIZE];
    A.d = stateblock->renderState[WINED3DRS_POINTSCALE_A];
    B.d = stateblock->renderState[WINED3DRS_POINTSCALE_B];
    C.d = stateblock->renderState[WINED3DRS_POINTSCALE_C];

    if(stateblock->renderState[WINED3DRS_POINTSCALEENABLE]) {
        GLfloat scaleFactor;
        float h = stateblock->viewport.Height;

        if(pointSize.f < GL_LIMITS(pointsizemin)) {
            /*
             * Minimum valid point size for OpenGL is driver specific. For Direct3D it is
             * 0.0f. This means that OpenGL will clamp really small point sizes to the
             * driver minimum. To correct for this we need to multiply by the scale factor when sizes
             * are less than 1.0f. scale_factor =  1.0f / point_size.
             */
            scaleFactor = pointSize.f / GL_LIMITS(pointsizemin);
            /* Clamp the point size, don't rely on the driver to do it. MacOS says min point size
             * is 1.0, but then accepts points below that and draws too small points
             */
            pointSize.f = GL_LIMITS(pointsizemin);
        } else if(pointSize.f > GL_LIMITS(pointsize)) {
            /* gl already scales the input to glPointSize,
             * d3d scales the result after the point size scale.
             * If the point size is bigger than the max size, use the
             * scaling to scale it bigger, and set the gl point size to max
             */
            scaleFactor = pointSize.f / GL_LIMITS(pointsize);
            TRACE("scale: %f\n", scaleFactor);
            pointSize.f = GL_LIMITS(pointsize);
        } else {
            scaleFactor = 1.0f;
        }
        scaleFactor = pow(h * scaleFactor, 2);

        att[0] = A.f / scaleFactor;
        att[1] = B.f / scaleFactor;
        att[2] = C.f / scaleFactor;
    }

    if(GL_SUPPORT(ARB_POINT_PARAMETERS)) {
        GL_EXTCALL(glPointParameterfvARB)(GL_POINT_DISTANCE_ATTENUATION_ARB, att);
        checkGLcall("glPointParameterfvARB(GL_DISTANCE_ATTENUATION_ARB, ...");
    }
    else if(GL_SUPPORT(EXT_POINT_PARAMETERS)) {
        GL_EXTCALL(glPointParameterfvEXT)(GL_DISTANCE_ATTENUATION_EXT, att);
        checkGLcall("glPointParameterfvEXT(GL_DISTANCE_ATTENUATION_EXT, ...");
    } else if(stateblock->renderState[WINED3DRS_POINTSCALEENABLE]) {
        WARN("POINT_PARAMETERS not supported in this version of opengl\n");
    }

    glPointSize(pointSize.f);
    checkGLcall("glPointSize(...);");
}

static void state_colorwrite(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    DWORD Value = stateblock->renderState[WINED3DRS_COLORWRITEENABLE];

    TRACE("Color mask: r(%d) g(%d) b(%d) a(%d)\n",
        Value & WINED3DCOLORWRITEENABLE_RED   ? 1 : 0,
        Value & WINED3DCOLORWRITEENABLE_GREEN ? 1 : 0,
        Value & WINED3DCOLORWRITEENABLE_BLUE  ? 1 : 0,
        Value & WINED3DCOLORWRITEENABLE_ALPHA ? 1 : 0);
    glColorMask(Value & WINED3DCOLORWRITEENABLE_RED   ? GL_TRUE : GL_FALSE,
                Value & WINED3DCOLORWRITEENABLE_GREEN ? GL_TRUE : GL_FALSE,
                Value & WINED3DCOLORWRITEENABLE_BLUE  ? GL_TRUE : GL_FALSE,
                Value & WINED3DCOLORWRITEENABLE_ALPHA ? GL_TRUE : GL_FALSE);
    checkGLcall("glColorMask(...)");

    /* depends on WINED3DRS_COLORWRITEENABLE. */
    if(stateblock->renderState[WINED3DRS_COLORWRITEENABLE1] != 0x0000000F ||
       stateblock->renderState[WINED3DRS_COLORWRITEENABLE2] != 0x0000000F ||
       stateblock->renderState[WINED3DRS_COLORWRITEENABLE3] != 0x0000000F ) {
        ERR("(WINED3DRS_COLORWRITEENABLE1/2/3,%d,%d,%d) not yet implemented. Missing of cap D3DPMISCCAPS_INDEPENDENTWRITEMASKS wasn't honored?\n",
            stateblock->renderState[WINED3DRS_COLORWRITEENABLE1],
            stateblock->renderState[WINED3DRS_COLORWRITEENABLE2],
            stateblock->renderState[WINED3DRS_COLORWRITEENABLE3]);
    }
}

static void state_localviewer(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    if(stateblock->renderState[WINED3DRS_LOCALVIEWER]) {
        glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
        checkGLcall("glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1)");
    } else {
        glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 0);
        checkGLcall("glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 0)");
    }
}

static void state_lastpixel(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    if(stateblock->renderState[WINED3DRS_LASTPIXEL]) {
        TRACE("Last Pixel Drawing Enabled\n");
    } else {
        static BOOL first = TRUE;
        if(first) {
            FIXME("Last Pixel Drawing Disabled, not handled yet\n");
            first = FALSE;
        } else {
            TRACE("Last Pixel Drawing Disabled, not handled yet\n");
        }
    }
}

static void state_pointsprite(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    /* TODO: NV_POINT_SPRITE */
    if (!GL_SUPPORT(ARB_POINT_SPRITE)) {
        TRACE("Point sprites not supported\n");
        return;
    }

    if (stateblock->renderState[WINED3DRS_POINTSPRITEENABLE]) {
        glEnable(GL_POINT_SPRITE_ARB);
        checkGLcall("glEnable(GL_POINT_SPRITE_ARB)\n");
    } else {
        glDisable(GL_POINT_SPRITE_ARB);
        checkGLcall("glDisable(GL_POINT_SPRITE_ARB)\n");
    }
}

static void state_wrap(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    /**
     http://www.cosc.brocku.ca/Offerings/3P98/course/lectures/texture/
     http://msdn.microsoft.com/archive/default.asp?url=/archive/en-us/directx9_c/directx/graphics/programmingguide/FixedFunction/Textures/texturewrapping.asp
     http://www.gamedev.net/reference/programming/features/rendererdll3/page2.asp
     Descussion that ways to turn on WRAPing to solve an opengl conversion problem.
     http://www.flipcode.org/cgi-bin/fcmsg.cgi?thread_show=10248

     so far as I can tell, wrapping and texture-coordinate generate go hand in hand,
     */
    TRACE("Stub\n");
    if(stateblock->renderState[WINED3DRS_WRAP0] ||
       stateblock->renderState[WINED3DRS_WRAP1] ||
       stateblock->renderState[WINED3DRS_WRAP2] ||
       stateblock->renderState[WINED3DRS_WRAP3] ||
       stateblock->renderState[WINED3DRS_WRAP4] ||
       stateblock->renderState[WINED3DRS_WRAP5] ||
       stateblock->renderState[WINED3DRS_WRAP6] ||
       stateblock->renderState[WINED3DRS_WRAP7] ||
       stateblock->renderState[WINED3DRS_WRAP8] ||
       stateblock->renderState[WINED3DRS_WRAP9] ||
       stateblock->renderState[WINED3DRS_WRAP10] ||
       stateblock->renderState[WINED3DRS_WRAP11] ||
       stateblock->renderState[WINED3DRS_WRAP12] ||
       stateblock->renderState[WINED3DRS_WRAP13] ||
       stateblock->renderState[WINED3DRS_WRAP14] ||
       stateblock->renderState[WINED3DRS_WRAP15] ) {
        FIXME("(WINED3DRS_WRAP0) Texture wraping not yet supported\n");
    }
}

static void state_multisampleaa(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    if( GL_SUPPORT(ARB_MULTISAMPLE) ) {
        if(stateblock->renderState[WINED3DRS_MULTISAMPLEANTIALIAS]) {
            glEnable(GL_MULTISAMPLE_ARB);
            checkGLcall("glEnable(GL_MULTISAMPLE_ARB)");
        } else {
            glDisable(GL_MULTISAMPLE_ARB);
            checkGLcall("glDisable(GL_MULTISAMPLE_ARB)");
        }
    } else {
        if(stateblock->renderState[WINED3DRS_MULTISAMPLEANTIALIAS]) {
            WARN("Multisample antialiasing not supported by gl\n");
        }
    }
}

static void state_scissor(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    if(stateblock->renderState[WINED3DRS_SCISSORTESTENABLE]) {
        glEnable(GL_SCISSOR_TEST);
        checkGLcall("glEnable(GL_SCISSOR_TEST)");
    } else {
        glDisable(GL_SCISSOR_TEST);
        checkGLcall("glDisable(GL_SCISSOR_TEST)");
    }
}

static void state_depthbias(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    union {
        DWORD d;
        float f;
    } tmpvalue;

    if(stateblock->renderState[WINED3DRS_SLOPESCALEDEPTHBIAS] ||
       stateblock->renderState[WINED3DRS_DEPTHBIAS]) {
        tmpvalue.d = stateblock->renderState[WINED3DRS_SLOPESCALEDEPTHBIAS];
        glEnable(GL_POLYGON_OFFSET_FILL);
        checkGLcall("glEnable(GL_POLYGON_OFFSET_FILL)");
        glPolygonOffset(tmpvalue.f, *((float*)&stateblock->renderState[WINED3DRS_DEPTHBIAS]));
        checkGLcall("glPolygonOffset(...)");
    } else {
        glDisable(GL_POLYGON_OFFSET_FILL);
        checkGLcall("glDisable(GL_POLYGON_OFFSET_FILL)");
    }
}

static void state_perspective(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    if (stateblock->renderState[WINED3DRS_TEXTUREPERSPECTIVE]) {
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
        checkGLcall("glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST)");
    } else {
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
        checkGLcall("glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST)");
    }
}

static void state_stippledalpha(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    TRACE("Stub\n");
    if (stateblock->renderState[WINED3DRS_STIPPLEDALPHA])
        FIXME(" Stippled Alpha not supported yet.\n");
}

static void state_antialias(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    TRACE("Stub\n");
    if (stateblock->renderState[WINED3DRS_ANTIALIAS])
        FIXME(" Antialias not supported yet.\n");
}

static void state_multisampmask(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    TRACE("Stub\n");
    if (stateblock->renderState[WINED3DRS_MULTISAMPLEMASK] != 0xFFFFFFFF)
        FIXME("(WINED3DRS_MULTISAMPLEMASK,%d) not yet implemented\n", stateblock->renderState[WINED3DRS_MULTISAMPLEMASK]);
}

static void state_patchedgestyle(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    TRACE("Stub\n");
    if (stateblock->renderState[WINED3DRS_PATCHEDGESTYLE] != WINED3DPATCHEDGE_DISCRETE)
        FIXME("(WINED3DRS_PATCHEDGESTYLE,%d) not yet implemented\n", stateblock->renderState[WINED3DRS_PATCHEDGESTYLE]);
}

static void state_patchsegments(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    union {
        DWORD d;
        float f;
    } tmpvalue;
    tmpvalue.f = 1.0f;

    TRACE("Stub\n");
    if (stateblock->renderState[WINED3DRS_PATCHSEGMENTS] != tmpvalue.d)
    {
        static BOOL displayed = FALSE;

        tmpvalue.d = stateblock->renderState[WINED3DRS_PATCHSEGMENTS];
        if(!displayed)
            FIXME("(WINED3DRS_PATCHSEGMENTS,%f) not yet implemented\n", tmpvalue.f);

        displayed = TRUE;
    }
}

static void state_positiondegree(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    TRACE("Stub\n");
    if (stateblock->renderState[WINED3DRS_POSITIONDEGREE] != WINED3DDEGREE_CUBIC)
        FIXME("(WINED3DRS_POSITIONDEGREE,%d) not yet implemented\n", stateblock->renderState[WINED3DRS_POSITIONDEGREE]);
}

static void state_normaldegree(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    TRACE("Stub\n");
    if (stateblock->renderState[WINED3DRS_NORMALDEGREE] != WINED3DDEGREE_LINEAR)
        FIXME("(WINED3DRS_NORMALDEGREE,%d) not yet implemented\n", stateblock->renderState[WINED3DRS_NORMALDEGREE]);
}

static void state_tessellation(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    TRACE("Stub\n");
    if(stateblock->renderState[WINED3DRS_ENABLEADAPTIVETESSELLATION])
        FIXME("(WINED3DRS_ENABLEADAPTIVETESSELLATION,%d) not yet implemented\n", stateblock->renderState[WINED3DRS_ENABLEADAPTIVETESSELLATION]);
}

static void state_separateblend(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    TRACE("Stub\n");
    if(stateblock->renderState[WINED3DRS_SEPARATEALPHABLENDENABLE])
        FIXME("(WINED3DRS_SEPARATEALPHABLENDENABLE,%d) not yet implemented\n", stateblock->renderState[WINED3DRS_SEPARATEALPHABLENDENABLE]);
}

static void state_wrapu(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    if(stateblock->renderState[WINED3DRS_WRAPU]) {
        FIXME("Render state WINED3DRS_WRAPU not implemented yet\n");
    }
}

static void state_wrapv(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    if(stateblock->renderState[WINED3DRS_WRAPV]) {
        FIXME("Render state WINED3DRS_WRAPV not implemented yet\n");
    }
}

static void state_monoenable(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    if(stateblock->renderState[WINED3DRS_MONOENABLE]) {
        FIXME("Render state WINED3DRS_MONOENABLE not implemented yet\n");
    }
}

static void state_rop2(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    if(stateblock->renderState[WINED3DRS_ROP2]) {
        FIXME("Render state WINED3DRS_ROP2 not implemented yet\n");
    }
}

static void state_planemask(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    if(stateblock->renderState[WINED3DRS_PLANEMASK]) {
        FIXME("Render state WINED3DRS_PLANEMASK not implemented yet\n");
    }
}

static void state_subpixel(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    if(stateblock->renderState[WINED3DRS_SUBPIXEL]) {
        FIXME("Render state WINED3DRS_SUBPIXEL not implemented yet\n");
    }
}

static void state_subpixelx(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    if(stateblock->renderState[WINED3DRS_SUBPIXELX]) {
        FIXME("Render state WINED3DRS_SUBPIXELX not implemented yet\n");
    }
}

static void state_stippleenable(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    if(stateblock->renderState[WINED3DRS_STIPPLEENABLE]) {
        FIXME("Render state WINED3DRS_STIPPLEENABLE not implemented yet\n");
    }
}

static void state_bordercolor(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    if(stateblock->renderState[WINED3DRS_BORDERCOLOR]) {
        FIXME("Render state WINED3DRS_BORDERCOLOR not implemented yet\n");
    }
}

static void state_mipmaplodbias(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    if(stateblock->renderState[WINED3DRS_MIPMAPLODBIAS]) {
        FIXME("Render state WINED3DRS_MIPMAPLODBIAS not implemented yet\n");
    }
}

static void state_anisotropy(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    if(stateblock->renderState[WINED3DRS_ANISOTROPY]) {
        FIXME("Render state WINED3DRS_ANISOTROPY not implemented yet\n");
    }
}

static void state_flushbatch(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    if(stateblock->renderState[WINED3DRS_FLUSHBATCH]) {
        FIXME("Render state WINED3DRS_FLUSHBATCH not implemented yet\n");
    }
}

static void state_translucentsi(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    if(stateblock->renderState[WINED3DRS_TRANSLUCENTSORTINDEPENDENT]) {
        FIXME("Render state WINED3DRS_TRANSLUCENTSORTINDEPENDENT not implemented yet\n");
    }
}

static void state_extents(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    if(stateblock->renderState[WINED3DRS_EXTENTS]) {
        FIXME("Render state WINED3DRS_EXTENTS not implemented yet\n");
    }
}

static void state_ckeyblend(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    if(stateblock->renderState[WINED3DRS_COLORKEYBLENDENABLE]) {
        FIXME("Render state WINED3DRS_COLORKEYBLENDENABLE not implemented yet\n");
    }
}

/* Activates the texture dimension according to the bound D3D texture.
 * Does not care for the colorop or correct gl texture unit(when using nvrc)
 * Requires the caller to activate the correct unit before
 */
static void activate_dimensions(DWORD stage, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    BOOL bumpmap = FALSE;

    if(stage > 0 && (stateblock->textureState[stage - 1][WINED3DTSS_COLOROP] == WINED3DTOP_BUMPENVMAPLUMINANCE ||
                     stateblock->textureState[stage - 1][WINED3DTSS_COLOROP] == WINED3DTOP_BUMPENVMAP)) {
        bumpmap = TRUE;
        context->texShaderBumpMap |= (1 << stage);
    } else {
        context->texShaderBumpMap &= ~(1 << stage);
    }

    if(stateblock->textures[stage]) {
        switch(stateblock->textureDimensions[stage]) {
            case GL_TEXTURE_2D:
                if(GL_SUPPORT(NV_TEXTURE_SHADER2)) {
                    glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, bumpmap ? GL_OFFSET_TEXTURE_2D_NV : GL_TEXTURE_2D);
                } else {
                    glDisable(GL_TEXTURE_3D);
                    checkGLcall("glDisable(GL_TEXTURE_3D)");
                    if(GL_SUPPORT(ARB_TEXTURE_CUBE_MAP)) {
                        glDisable(GL_TEXTURE_CUBE_MAP_ARB);
                        checkGLcall("glDisable(GL_TEXTURE_CUBE_MAP_ARB)");
                    }
                    glEnable(GL_TEXTURE_2D);
                    checkGLcall("glEnable(GL_TEXTURE_2D)");
                }
                break;
            case GL_TEXTURE_3D:
                if(GL_SUPPORT(NV_TEXTURE_SHADER2)) {
                    glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_TEXTURE_3D);
                } else {
                    if(GL_SUPPORT(ARB_TEXTURE_CUBE_MAP)) {
                        glDisable(GL_TEXTURE_CUBE_MAP_ARB);
                        checkGLcall("glDisable(GL_TEXTURE_CUBE_MAP_ARB)");
                    }
                    glDisable(GL_TEXTURE_2D);
                    checkGLcall("glDisable(GL_TEXTURE_2D)");
                    glEnable(GL_TEXTURE_3D);
                    checkGLcall("glEnable(GL_TEXTURE_3D)");
                }
                break;
            case GL_TEXTURE_CUBE_MAP_ARB:
                if(GL_SUPPORT(NV_TEXTURE_SHADER2)) {
                    glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_TEXTURE_CUBE_MAP_ARB);
                } else {
                    glDisable(GL_TEXTURE_2D);
                    checkGLcall("glDisable(GL_TEXTURE_2D)");
                    glDisable(GL_TEXTURE_3D);
                    checkGLcall("glDisable(GL_TEXTURE_3D)");
                    glEnable(GL_TEXTURE_CUBE_MAP_ARB);
                    checkGLcall("glEnable(GL_TEXTURE_CUBE_MAP_ARB)");
                }
              break;
        }
    } else {
        if(GL_SUPPORT(NV_TEXTURE_SHADER2)) {
            glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_NONE);
        } else {
            glEnable(GL_TEXTURE_2D);
            checkGLcall("glEnable(GL_TEXTURE_2D)");
            glDisable(GL_TEXTURE_3D);
            checkGLcall("glDisable(GL_TEXTURE_3D)");
            if(GL_SUPPORT(ARB_TEXTURE_CUBE_MAP)) {
                glDisable(GL_TEXTURE_CUBE_MAP_ARB);
                checkGLcall("glDisable(GL_TEXTURE_CUBE_MAP_ARB)");
            }
            /* Binding textures is done by samplers. A dummy texture will be bound */
        }
    }
}

static void tex_colorop(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    DWORD stage = (state - STATE_TEXTURESTAGE(0, 0)) / WINED3D_HIGHEST_TEXTURE_STATE;
    DWORD mapped_stage = stateblock->wineD3DDevice->texUnitMap[stage];
    BOOL tex_used = stateblock->wineD3DDevice->fixed_function_usage_map[stage];

    TRACE("Setting color op for stage %d\n", stage);

    if (stateblock->pixelShader && stateblock->wineD3DDevice->ps_selected_mode != SHADER_NONE &&
        ((IWineD3DPixelShaderImpl *)stateblock->pixelShader)->baseShader.function) {
        /* Using a pixel shader? Don't care for anything here, the shader applying does it */
        return;
    }

    if (stage != mapped_stage) WARN("Using non 1:1 mapping: %d -> %d!\n", stage, mapped_stage);

    if (mapped_stage != -1) {
        if (GL_SUPPORT(ARB_MULTITEXTURE)) {
            if (tex_used && mapped_stage >= GL_LIMITS(textures)) {
                FIXME("Attempt to enable unsupported stage!\n");
                return;
            }
            GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + mapped_stage));
            checkGLcall("glActiveTextureARB");
        } else if (stage > 0) {
            WARN("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
            return;
        }
    }

    if (GL_SUPPORT(NV_REGISTER_COMBINERS)) {
        if(stateblock->lowest_disabled_stage > 0) {
            glEnable(GL_REGISTER_COMBINERS_NV);
            GL_EXTCALL(glCombinerParameteriNV(GL_NUM_GENERAL_COMBINERS_NV, stateblock->lowest_disabled_stage));
        } else {
            glDisable(GL_REGISTER_COMBINERS_NV);
        }
    }
    if(stage >= stateblock->lowest_disabled_stage) {
        TRACE("Stage disabled\n");
        if (mapped_stage != -1) {
            /* Disable everything here */
            glDisable(GL_TEXTURE_2D);
            checkGLcall("glDisable(GL_TEXTURE_2D)");
            glDisable(GL_TEXTURE_3D);
            checkGLcall("glDisable(GL_TEXTURE_3D)");
            if(GL_SUPPORT(ARB_TEXTURE_CUBE_MAP)) {
                glDisable(GL_TEXTURE_CUBE_MAP_ARB);
                checkGLcall("glDisable(GL_TEXTURE_CUBE_MAP_ARB)");
            }
            if(GL_SUPPORT(NV_TEXTURE_SHADER2) && mapped_stage < GL_LIMITS(textures)) {
                glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_NONE);
            }
        }
        /* All done */
        return;
    }

    /* The sampler will also activate the correct texture dimensions, so no need to do it here
     * if the sampler for this stage is dirty
     */
    if(!isStateDirty(context, STATE_SAMPLER(stage))) {
        if (tex_used) activate_dimensions(stage, stateblock, context);
    }

    /* Set the texture combiners */
    if (GL_SUPPORT(NV_REGISTER_COMBINERS)) {
        set_tex_op_nvrc((IWineD3DDevice *)stateblock->wineD3DDevice, FALSE, stage,
                         stateblock->textureState[stage][WINED3DTSS_COLOROP],
                         stateblock->textureState[stage][WINED3DTSS_COLORARG1],
                         stateblock->textureState[stage][WINED3DTSS_COLORARG2],
                         stateblock->textureState[stage][WINED3DTSS_COLORARG0],
                         mapped_stage);

        /* In register combiners bump mapping is done in the stage AFTER the one that has the bump map operation set,
         * thus the texture shader may have to be updated
         */
        if(GL_SUPPORT(NV_TEXTURE_SHADER2)) {
            BOOL usesBump = (stateblock->textureState[stage][WINED3DTSS_COLOROP] == WINED3DTOP_BUMPENVMAPLUMINANCE ||
                             stateblock->textureState[stage][WINED3DTSS_COLOROP] == WINED3DTOP_BUMPENVMAP) ? TRUE : FALSE;
            BOOL usedBump = (context->texShaderBumpMap & 1 << (stage + 1)) ? TRUE : FALSE;
            if(usesBump != usedBump) {
                GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + mapped_stage + 1));
                checkGLcall("glActiveTextureARB");
                activate_dimensions(stage + 1, stateblock, context);
                GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + mapped_stage));
                checkGLcall("glActiveTextureARB");
            }
        }
    } else {
        set_tex_op((IWineD3DDevice *)stateblock->wineD3DDevice, FALSE, stage,
                    stateblock->textureState[stage][WINED3DTSS_COLOROP],
                    stateblock->textureState[stage][WINED3DTSS_COLORARG1],
                    stateblock->textureState[stage][WINED3DTSS_COLORARG2],
                    stateblock->textureState[stage][WINED3DTSS_COLORARG0]);
    }
}

static void tex_alphaop(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    DWORD stage = (state - STATE_TEXTURESTAGE(0, 0)) / WINED3D_HIGHEST_TEXTURE_STATE;
    DWORD mapped_stage = stateblock->wineD3DDevice->texUnitMap[stage];
    BOOL tex_used = stateblock->wineD3DDevice->fixed_function_usage_map[stage];
    DWORD op, arg1, arg2, arg0;

    TRACE("Setting alpha op for stage %d\n", stage);
    /* Do not care for enabled / disabled stages, just assign the settigns. colorop disables / enables required stuff */
    if (mapped_stage != -1) {
        if (GL_SUPPORT(ARB_MULTITEXTURE)) {
            if (tex_used && mapped_stage >= GL_LIMITS(textures)) {
                FIXME("Attempt to enable unsupported stage!\n");
                return;
            }
            GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + mapped_stage));
            checkGLcall("glActiveTextureARB");
        } else if (stage > 0) {
            /* We can't do anything here */
            WARN("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
            return;
        }
    }

    op = stateblock->textureState[stage][WINED3DTSS_ALPHAOP];
    arg1 = stateblock->textureState[stage][WINED3DTSS_ALPHAARG1];
    arg2 = stateblock->textureState[stage][WINED3DTSS_ALPHAARG2];
    arg0 = stateblock->textureState[stage][WINED3DTSS_ALPHAARG0];

    if(stateblock->renderState[WINED3DRS_COLORKEYENABLE] && stage == 0 &&
       stateblock->textures[0] && stateblock->textureDimensions[0] == GL_TEXTURE_2D) {
        IWineD3DSurfaceImpl *surf = (IWineD3DSurfaceImpl *) ((IWineD3DTextureImpl *) stateblock->textures[0])->surfaces[0];

        if(surf->CKeyFlags & WINEDDSD_CKSRCBLT &&
           getFormatDescEntry(surf->resource.format, NULL, NULL)->alphaMask == 0x00000000) {

            /* Color keying needs to pass alpha values from the texture through to have the alpha test work properly.
             * On the other hand applications can still use texture combiners apparently. This code takes care that apps
             * cannot remove the texture's alpha channel entirely.
             *
             * The fixup is required for Prince of Persia 3D(prison bars), while Moto racer 2 requires D3DTOP_MODULATE to work
             * on color keyed surfaces.
             *
             * What to do with multitexturing? So far no app has been found that uses color keying with multitexturing
             */
            if(op == WINED3DTOP_DISABLE) op = WINED3DTOP_SELECTARG1;
            if(op == WINED3DTOP_SELECTARG1) arg1 = WINED3DTA_TEXTURE;
            else if(op == WINED3DTOP_SELECTARG2) arg2 = WINED3DTA_TEXTURE;
        }
    }

    TRACE("Setting alpha op for stage %d\n", stage);
    if (GL_SUPPORT(NV_REGISTER_COMBINERS)) {
        set_tex_op_nvrc((IWineD3DDevice *)stateblock->wineD3DDevice, TRUE, stage,
                         op, arg1, arg2, arg0,
                         mapped_stage);
    } else {
        set_tex_op((IWineD3DDevice *)stateblock->wineD3DDevice, TRUE, stage,
                    op, arg1, arg2, arg0);
    }
}

static void transform_texture(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    DWORD texUnit = state - STATE_TRANSFORM(WINED3DTS_TEXTURE0);
    DWORD mapped_stage = stateblock->wineD3DDevice->texUnitMap[texUnit];

    /* Ignore this when a vertex shader is used, or if the streams aren't sorted out yet */
    if(use_vs(stateblock->wineD3DDevice) ||
       isStateDirty(context, STATE_VDECL)) {
        TRACE("Using a vertex shader, or stream sources not sorted out yet, skipping\n");
        return;
    }

    if (mapped_stage < 0) return;

    if (GL_SUPPORT(ARB_MULTITEXTURE)) {
        if(mapped_stage >= GL_LIMITS(textures)) {
            return;
        }
        GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + mapped_stage));
        checkGLcall("glActiveTextureARB");
    } else if (mapped_stage > 0) {
        /* We can't do anything here */
        WARN("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
        return;
    }

    set_texture_matrix((float *)&stateblock->transforms[WINED3DTS_TEXTURE0 + texUnit].u.m[0][0],
                        stateblock->textureState[texUnit][WINED3DTSS_TEXTURETRANSFORMFLAGS],
                        (stateblock->textureState[texUnit][WINED3DTSS_TEXCOORDINDEX] & 0xFFFF0000) != WINED3DTSS_TCI_PASSTHRU,
                        context->last_was_rhw,
                        stateblock->wineD3DDevice->strided_streams.u.s.texCoords[texUnit].dwStride ?
                            stateblock->wineD3DDevice->strided_streams.u.s.texCoords[texUnit].dwType:
                            WINED3DDECLTYPE_UNUSED);

}

static void unloadTexCoords(IWineD3DStateBlockImpl *stateblock) {
    int texture_idx;

    for (texture_idx = 0; texture_idx < GL_LIMITS(texture_stages); ++texture_idx) {
        GL_EXTCALL(glClientActiveTextureARB(GL_TEXTURE0_ARB + texture_idx));
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
}

static void loadTexCoords(IWineD3DStateBlockImpl *stateblock, WineDirect3DVertexStridedData *sd, GLint *curVBO) {
    UINT *offset = stateblock->streamOffset;
    unsigned int mapped_stage = 0;
    unsigned int textureNo = 0;

    /* The code below uses glClientActiveTexture and glMultiTexCoord* which are all part of the GL_ARB_multitexture extension. */
    /* Abort if we don't support the extension. */
    if (!GL_SUPPORT(ARB_MULTITEXTURE)) {
        FIXME("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
        return;
    }

    for (textureNo = 0; textureNo < GL_LIMITS(texture_stages); ++textureNo) {
        int coordIdx = stateblock->textureState[textureNo][WINED3DTSS_TEXCOORDINDEX];

        mapped_stage = stateblock->wineD3DDevice->texUnitMap[textureNo];
        if (mapped_stage == -1) continue;

        if (coordIdx < MAX_TEXTURES && (sd->u.s.texCoords[coordIdx].lpData || sd->u.s.texCoords[coordIdx].VBO)) {
            TRACE("Setting up texture %u, idx %d, cordindx %u, data %p\n",
                    textureNo, mapped_stage, coordIdx, sd->u.s.texCoords[coordIdx].lpData);

            if (*curVBO != sd->u.s.texCoords[coordIdx].VBO) {
                GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, sd->u.s.texCoords[coordIdx].VBO));
                checkGLcall("glBindBufferARB");
                *curVBO = sd->u.s.texCoords[coordIdx].VBO;
            }

            GL_EXTCALL(glClientActiveTextureARB(GL_TEXTURE0_ARB + mapped_stage));
            checkGLcall("glClientActiveTextureARB");

            /* The coords to supply depend completely on the fvf / vertex shader */
            glTexCoordPointer(
                    WINED3D_ATR_SIZE(sd->u.s.texCoords[coordIdx].dwType),
                    WINED3D_ATR_GLTYPE(sd->u.s.texCoords[coordIdx].dwType),
                    sd->u.s.texCoords[coordIdx].dwStride,
                    sd->u.s.texCoords[coordIdx].lpData + stateblock->loadBaseVertexIndex * sd->u.s.texCoords[coordIdx].dwStride + offset[sd->u.s.texCoords[coordIdx].streamNo]);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        } else {
            GL_EXTCALL(glMultiTexCoord4fARB(GL_TEXTURE0_ARB + mapped_stage, 0, 0, 0, 1));
        }
    }
    if (GL_SUPPORT(NV_REGISTER_COMBINERS)) {
        /* The number of the mapped stages increases monotonically, so it's fine to use the last used one */
        for (textureNo = mapped_stage + 1; textureNo < GL_LIMITS(textures); ++textureNo) {
            GL_EXTCALL(glMultiTexCoord4fARB(GL_TEXTURE0_ARB + textureNo, 0, 0, 0, 1));
        }
    }
}

static void tex_coordindex(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    DWORD stage = (state - STATE_TEXTURESTAGE(0, 0)) / WINED3D_HIGHEST_TEXTURE_STATE;
    DWORD mapped_stage = stateblock->wineD3DDevice->texUnitMap[stage];

    if (mapped_stage == -1) {
        TRACE("No texture unit mapped to stage %d. Skipping texture coordinates.\n", stage);
        return;
    }

    if (GL_SUPPORT(ARB_MULTITEXTURE)) {
        if(mapped_stage >= GL_LIMITS(fragment_samplers)) {
            return;
        }
        GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + mapped_stage));
        checkGLcall("glActiveTextureARB");
    } else if (stage > 0) {
        /* We can't do anything here */
        WARN("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
        return;
    }

    /* Values 0-7 are indexes into the FVF tex coords - See comments in DrawPrimitive
     *
     * FIXME: From MSDN: The WINED3DTSS_TCI_* flags are mutually exclusive. If you include
     * one flag, you can still specify an index value, which the system uses to
     * determine the texture wrapping mode.
     * eg. SetTextureStageState( 0, WINED3DTSS_TEXCOORDINDEX, WINED3DTSS_TCI_CAMERASPACEPOSITION | 1 );
     * means use the vertex position (camera-space) as the input texture coordinates
     * for this texture stage, and the wrap mode set in the WINED3DRS_WRAP1 render
     * state. We do not (yet) support the WINED3DRENDERSTATE_WRAPx values, nor tie them up
     * to the TEXCOORDINDEX value
     */

    /*
     * Be careful the value of the mask 0xF0000 come from d3d8types.h infos
     */
    switch (stateblock->textureState[stage][WINED3DTSS_TEXCOORDINDEX] & 0xFFFF0000) {
    case WINED3DTSS_TCI_PASSTHRU:
        /*Use the specified texture coordinates contained within the vertex format. This value resolves to zero.*/
        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);
        glDisable(GL_TEXTURE_GEN_R);
        glDisable(GL_TEXTURE_GEN_Q);
        checkGLcall("glDisable(GL_TEXTURE_GEN_S,T,R,Q)");
        break;

    case WINED3DTSS_TCI_CAMERASPACEPOSITION:
        /* CameraSpacePosition means use the vertex position, transformed to camera space,
         * as the input texture coordinates for this stage's texture transformation. This
         * equates roughly to EYE_LINEAR
         */
        {
            float s_plane[] = { 1.0, 0.0, 0.0, 0.0 };
            float t_plane[] = { 0.0, 1.0, 0.0, 0.0 };
            float r_plane[] = { 0.0, 0.0, 1.0, 0.0 };
            float q_plane[] = { 0.0, 0.0, 0.0, 1.0 };
            TRACE("WINED3DTSS_TCI_CAMERASPACEPOSITION - Set eye plane\n");

            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glLoadIdentity();
            glTexGenfv(GL_S, GL_EYE_PLANE, s_plane);
            glTexGenfv(GL_T, GL_EYE_PLANE, t_plane);
            glTexGenfv(GL_R, GL_EYE_PLANE, r_plane);
            glTexGenfv(GL_Q, GL_EYE_PLANE, q_plane);
            glPopMatrix();

            TRACE("WINED3DTSS_TCI_CAMERASPACEPOSITION - Set GL_TEXTURE_GEN_x and GL_x, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR\n");
            glEnable(GL_TEXTURE_GEN_S);
            checkGLcall("glEnable(GL_TEXTURE_GEN_S);");
            glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
            checkGLcall("glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR)");
            glEnable(GL_TEXTURE_GEN_T);
            checkGLcall("glEnable(GL_TEXTURE_GEN_T);");
            glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
            checkGLcall("glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR)");
            glEnable(GL_TEXTURE_GEN_R);
            checkGLcall("glEnable(GL_TEXTURE_GEN_R);");
            glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
            checkGLcall("glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR)");
        }
        break;

    case WINED3DTSS_TCI_CAMERASPACENORMAL:
        {
            if (GL_SUPPORT(NV_TEXGEN_REFLECTION)) {
                float s_plane[] = { 1.0, 0.0, 0.0, 0.0 };
                float t_plane[] = { 0.0, 1.0, 0.0, 0.0 };
                float r_plane[] = { 0.0, 0.0, 1.0, 0.0 };
                float q_plane[] = { 0.0, 0.0, 0.0, 1.0 };
                TRACE("WINED3DTSS_TCI_CAMERASPACENORMAL - Set eye plane\n");

                glMatrixMode(GL_MODELVIEW);
                glPushMatrix();
                glLoadIdentity();
                glTexGenfv(GL_S, GL_EYE_PLANE, s_plane);
                glTexGenfv(GL_T, GL_EYE_PLANE, t_plane);
                glTexGenfv(GL_R, GL_EYE_PLANE, r_plane);
                glTexGenfv(GL_Q, GL_EYE_PLANE, q_plane);
                glPopMatrix();

                glEnable(GL_TEXTURE_GEN_S);
                checkGLcall("glEnable(GL_TEXTURE_GEN_S);");
                glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_NV);
                checkGLcall("glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_NV)");
                glEnable(GL_TEXTURE_GEN_T);
                checkGLcall("glEnable(GL_TEXTURE_GEN_T);");
                glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_NV);
                checkGLcall("glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_NV)");
                glEnable(GL_TEXTURE_GEN_R);
                checkGLcall("glEnable(GL_TEXTURE_GEN_R);");
                glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_NV);
                checkGLcall("glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_NV)");
            }
        }
        break;

    case WINED3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR:
        {
            if (GL_SUPPORT(NV_TEXGEN_REFLECTION)) {
            float s_plane[] = { 1.0, 0.0, 0.0, 0.0 };
            float t_plane[] = { 0.0, 1.0, 0.0, 0.0 };
            float r_plane[] = { 0.0, 0.0, 1.0, 0.0 };
            float q_plane[] = { 0.0, 0.0, 0.0, 1.0 };
            TRACE("WINED3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR - Set eye plane\n");

            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glLoadIdentity();
            glTexGenfv(GL_S, GL_EYE_PLANE, s_plane);
            glTexGenfv(GL_T, GL_EYE_PLANE, t_plane);
            glTexGenfv(GL_R, GL_EYE_PLANE, r_plane);
            glTexGenfv(GL_Q, GL_EYE_PLANE, q_plane);
            glPopMatrix();

            glEnable(GL_TEXTURE_GEN_S);
            checkGLcall("glEnable(GL_TEXTURE_GEN_S);");
            glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_NV);
            checkGLcall("glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_NV)");
            glEnable(GL_TEXTURE_GEN_T);
            checkGLcall("glEnable(GL_TEXTURE_GEN_T);");
            glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_NV);
            checkGLcall("glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_NV)");
            glEnable(GL_TEXTURE_GEN_R);
            checkGLcall("glEnable(GL_TEXTURE_GEN_R);");
            glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_NV);
            checkGLcall("glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_NV)");
            }
        }
        break;

    /* Unhandled types: */
    default:
        /* Todo: */
        /* ? disable GL_TEXTURE_GEN_n ? */
        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);
        glDisable(GL_TEXTURE_GEN_R);
        glDisable(GL_TEXTURE_GEN_Q);
        FIXME("Unhandled WINED3DTSS_TEXCOORDINDEX %x\n", stateblock->textureState[stage][WINED3DTSS_TEXCOORDINDEX]);
        break;
    }

    /* Update the texture matrix */
    if(!isStateDirty(context, STATE_TRANSFORM(WINED3DTS_TEXTURE0 + stage))) {
        transform_texture(STATE_TRANSFORM(WINED3DTS_TEXTURE0 + stage), stateblock, context);
    }

    if(!isStateDirty(context, STATE_VDECL) && context->namedArraysLoaded) {
        /* Reload the arrays if we are using fixed function arrays to reflect the selected coord input
         * source. Call loadTexCoords directly because there is no need to reparse the vertex declaration
         * and do all the things linked to it
         * TODO: Tidy that up to reload only the arrays of the changed unit
         */
        GLint curVBO = GL_SUPPORT(ARB_VERTEX_BUFFER_OBJECT) ? -1 : 0;

        unloadTexCoords(stateblock);
        loadTexCoords(stateblock, &stateblock->wineD3DDevice->strided_streams, &curVBO);
    }
}

static void shaderconstant(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    IWineD3DDeviceImpl *device = stateblock->wineD3DDevice;

    /* Vertex and pixel shader states will call a shader upload, don't do anything as long one of them
     * has an update pending
     */
    if(isStateDirty(context, STATE_VDECL) ||
       isStateDirty(context, STATE_PIXELSHADER)) {
       return;
    }

    device->shader_backend->shader_load_constants((IWineD3DDevice *) device, use_ps(device), use_vs(device));
}

static void tex_bumpenvlscale(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    DWORD stage = (state - STATE_TEXTURESTAGE(0, 0)) / WINED3D_HIGHEST_TEXTURE_STATE;
    union {
        DWORD d;
        float f;
    } tmpvalue;

    if(stateblock->pixelShader && stage != 0 &&
       ((IWineD3DPixelShaderImpl *) stateblock->pixelShader)->baseShader.reg_maps.luminanceparams == stage) {
        /* The pixel shader has to know the luminance scale. Do a constants update if it
         * isn't scheduled anyway
         */
        if(!isStateDirty(context, STATE_PIXELSHADERCONSTANT) &&
           !isStateDirty(context, STATE_PIXELSHADER)) {
            shaderconstant(STATE_PIXELSHADERCONSTANT, stateblock, context);
        }
    }

    tmpvalue.d = stateblock->textureState[stage][WINED3DTSS_BUMPENVLSCALE];
    if(tmpvalue.f != 0.0) {
        FIXME("WINED3DTSS_BUMPENVLSCALE not supported yet\n");
    }
}

static void tex_bumpenvloffset(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    DWORD stage = (state - STATE_TEXTURESTAGE(0, 0)) / WINED3D_HIGHEST_TEXTURE_STATE;
    union {
        DWORD d;
        float f;
    } tmpvalue;

    if(stateblock->pixelShader && stage != 0 &&
       ((IWineD3DPixelShaderImpl *) stateblock->pixelShader)->baseShader.reg_maps.luminanceparams == stage) {
        /* The pixel shader has to know the luminance offset. Do a constants update if it
         * isn't scheduled anyway
         */
        if(!isStateDirty(context, STATE_PIXELSHADERCONSTANT) &&
           !isStateDirty(context, STATE_PIXELSHADER)) {
            shaderconstant(STATE_PIXELSHADERCONSTANT, stateblock, context);
        }
    }

    tmpvalue.d = stateblock->textureState[stage][WINED3DTSS_BUMPENVLOFFSET];
    if(tmpvalue.f != 0.0) {
        FIXME("WINED3DTSS_BUMPENVLOFFSET not supported yet\n");
    }
}

static void tex_resultarg(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    DWORD stage = (state - STATE_TEXTURESTAGE(0, 0)) / WINED3D_HIGHEST_TEXTURE_STATE;

    if(stage >= GL_LIMITS(texture_stages)) {
        return;
    }

    if(stateblock->textureState[stage][WINED3DTSS_RESULTARG] != WINED3DTA_CURRENT) {
        FIXME("WINED3DTSS_RESULTARG not supported yet\n");
    }
}

static void sampler(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    DWORD sampler = state - STATE_SAMPLER(0);
    DWORD mapped_stage = stateblock->wineD3DDevice->texUnitMap[sampler];
    union {
        float f;
        DWORD d;
    } tmpvalue;

    TRACE("Sampler: %d\n", sampler);
    /* Enabling and disabling texture dimensions is done by texture stage state / pixel shader setup, this function
     * only has to bind textures and set the per texture states
     */

    if (mapped_stage == -1) {
        TRACE("No sampler mapped to stage %d. Returning.\n", sampler);
        return;
    }

    if (GL_SUPPORT(ARB_MULTITEXTURE)) {
        if (mapped_stage >= GL_LIMITS(combined_samplers)) {
            return;
        }
        GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + mapped_stage));
        checkGLcall("glActiveTextureARB");
    } else if (sampler > 0) {
        /* We can't do anything here */
        WARN("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
        return;
    }

    if(stateblock->textures[sampler]) {
        BOOL texIsPow2 = FALSE;

        /* The fixed function np2 texture emulation uses the texture matrix to fix up the coordinates
         * IWineD3DBaseTexture::ApplyStateChanges multiplies the set matrix with a fixup matrix. Before the
         * scaling is reapplied or removed, the texture matrix has to be reapplied
         */
        if(!GL_SUPPORT(ARB_TEXTURE_NON_POWER_OF_TWO) && sampler < MAX_TEXTURES) {
            if(stateblock->textureDimensions[sampler] == GL_TEXTURE_2D) {
                if(((IWineD3DTextureImpl *) stateblock->textures[sampler])->pow2scalingFactorX != 1.0 ||
                   ((IWineD3DTextureImpl *) stateblock->textures[sampler])->pow2scalingFactorY != 1.0 ) {
                    texIsPow2 = TRUE;
                }
            } else if(stateblock->textureDimensions[sampler] == GL_TEXTURE_CUBE_MAP_ARB) {
                if(((IWineD3DCubeTextureImpl *) stateblock->textures[sampler])->pow2scalingFactor != 1.0) {
                    texIsPow2 = TRUE;
                }
            }

            if(texIsPow2 || context->lastWasPow2Texture[sampler]) {
                transform_texture(STATE_TRANSFORM(WINED3DTS_TEXTURE0 + stateblock->wineD3DDevice->texUnitMap[sampler]), stateblock, context);
                context->lastWasPow2Texture[sampler] = texIsPow2;
            }
        }

        IWineD3DBaseTexture_PreLoad((IWineD3DBaseTexture *) stateblock->textures[sampler]);
        IWineD3DBaseTexture_ApplyStateChanges(stateblock->textures[sampler], stateblock->textureState[sampler], stateblock->samplerState[sampler]);

        if (GL_SUPPORT(EXT_TEXTURE_LOD_BIAS)) {
            tmpvalue.d = stateblock->samplerState[sampler][WINED3DSAMP_MIPMAPLODBIAS];
            glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT,
                      GL_TEXTURE_LOD_BIAS_EXT,
                      tmpvalue.f);
            checkGLcall("glTexEnvi GL_TEXTURE_LOD_BIAS_EXT ...");
        }

        if (stateblock->wineD3DDevice->ps_selected_mode != SHADER_NONE && stateblock->pixelShader &&
            ((IWineD3DPixelShaderImpl *)stateblock->pixelShader)->baseShader.function) {
            /* Using a pixel shader? Verify the sampler types */

            /* Make sure that the texture dimensions are enabled. I don't have to disable the other
             * dimensions because the shader knows from which texture type to sample from. For the sake of
             * debugging all dimensions could be enabled and a texture with some ugly pink bound to the unused
             * dimensions. This should make wrong sampling sources visible :-)
             */
            glEnable(stateblock->textureDimensions[sampler]);
            checkGLcall("glEnable(stateblock->textureDimensions[sampler])");
        } else if(sampler < stateblock->lowest_disabled_stage) {
            if(!isStateDirty(context, STATE_TEXTURESTAGE(sampler, WINED3DTSS_COLOROP))) {
                activate_dimensions(sampler, stateblock, context);
            }

            if(stateblock->renderState[WINED3DRS_COLORKEYENABLE] && sampler == 0) {
                /* If color keying is enabled update the alpha test, it depends on the existence
                 * of a color key in stage 0
                 */
                state_alpha(WINED3DRS_COLORKEYENABLE, stateblock, context);
            }
        }
    } else if(sampler < GL_LIMITS(texture_stages)) {
        if(sampler < stateblock->lowest_disabled_stage) {
            /* TODO: What should I do with pixel shaders here ??? */
            if(!isStateDirty(context, STATE_TEXTURESTAGE(sampler, WINED3DTSS_COLOROP))) {
                activate_dimensions(sampler, stateblock, context);
            }
        } /* Otherwise tex_colorop disables the stage */
        glBindTexture(GL_TEXTURE_2D, stateblock->wineD3DDevice->dummyTextureName[sampler]);
        checkGLcall("glBindTexture(GL_TEXTURE_2D, stateblock->wineD3DDevice->dummyTextureName[sampler])");
    }
}

static void pixelshader(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    IWineD3DDeviceImpl *device = stateblock->wineD3DDevice;
    BOOL use_pshader = use_ps(device);
    BOOL use_vshader = use_vs(device);
    BOOL update_fog = FALSE;
    int i;

    if (use_pshader) {
        if(!context->last_was_pshader) {
            /* Former draw without a pixel shader, some samplers
             * may be disabled because of WINED3DTSS_COLOROP = WINED3DTOP_DISABLE
             * make sure to enable them
             */
            for(i=0; i < MAX_FRAGMENT_SAMPLERS; i++) {
                if(!isStateDirty(context, STATE_SAMPLER(i))) {
                    sampler(STATE_SAMPLER(i), stateblock, context);
                }
            }
            update_fog = TRUE;
        } else {
           /* Otherwise all samplers were activated by the code above in earlier draws, or by sampler()
            * if a different texture was bound. I don't have to do anything.
            */
        }

        /* Compile and bind the shader */
        IWineD3DPixelShader_CompileShader(stateblock->pixelShader);
    } else {
        /* Disabled the pixel shader - color ops weren't applied
         * while it was enabled, so re-apply them.
         */
        for(i=0; i < MAX_TEXTURES; i++) {
            if(!isStateDirty(context, STATE_TEXTURESTAGE(i, WINED3DTSS_COLOROP))) {
                tex_colorop(STATE_TEXTURESTAGE(i, WINED3DTSS_COLOROP), stateblock, context);
            }
        }
        if(context->last_was_pshader)
            update_fog = TRUE;
    }

    if(!isStateDirty(context, StateTable[STATE_VSHADER].representative)) {
        device->shader_backend->shader_select((IWineD3DDevice *)stateblock->wineD3DDevice, use_pshader, use_vshader);

        if (!isStateDirty(context, STATE_VERTEXSHADERCONSTANT) && (use_vshader || use_pshader)) {
            shaderconstant(STATE_VERTEXSHADERCONSTANT, stateblock, context);
        }
    }

    if(update_fog)
        state_fog(state, stateblock, context);

    context->last_was_pshader = use_pshader;
}

static void tex_bumpenvmat(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    DWORD stage = (state - STATE_TEXTURESTAGE(0, 0)) / WINED3D_HIGHEST_TEXTURE_STATE;

    if(stateblock->pixelShader && stage != 0 &&
       ((IWineD3DPixelShaderImpl *) stateblock->pixelShader)->needsbumpmat == stage) {
        /* The pixel shader has to know the bump env matrix. Do a constants update if it isn't scheduled
         * anyway
         */
        if(!isStateDirty(context, STATE_PIXELSHADERCONSTANT) &&
           !isStateDirty(context, STATE_PIXELSHADER)) {
            shaderconstant(STATE_PIXELSHADERCONSTANT, stateblock, context);
        }
    }

    if(GL_SUPPORT(ATI_ENVMAP_BUMPMAP)) {
        if(stage >= GL_LIMITS(texture_stages)) {
            WARN("Bump env matrix of unsupported stage set\n");
        } else if(GL_SUPPORT(ARB_MULTITEXTURE)) {
            GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + stage));
            checkGLcall("GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + stage))");
        }
        GL_EXTCALL(glTexBumpParameterfvATI(GL_BUMP_ROT_MATRIX_ATI,
                   (float *) &(stateblock->textureState[stage][WINED3DTSS_BUMPENVMAT00])));
        checkGLcall("glTexBumpParameterfvATI");
    }
    if(GL_SUPPORT(NV_TEXTURE_SHADER2)) {
        /* Direct3D sets the matrix in the stage reading the perturbation map. The result is used to
         * offset the destination stage(always stage + 1 in d3d). In GL_NV_texture_shader, the bump
         * map offseting is done in the stage reading the bump mapped texture, and the perturbation
         * map is read from a specified source stage(always stage - 1 for d3d). Thus set the matrix
         * for stage + 1. Keep the nvrc tex unit mapping in mind too
         */
        DWORD mapped_stage = stateblock->wineD3DDevice->texUnitMap[stage + 1];

        if(mapped_stage < GL_LIMITS(textures)) {
            GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + mapped_stage));
            checkGLcall("GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + mapped_stage))");

            glTexEnvfv(GL_TEXTURE_SHADER_NV, GL_OFFSET_TEXTURE_MATRIX_NV,
                      (float *) &(stateblock->textureState[stage][WINED3DTSS_BUMPENVMAT00]));
            checkGLcall("glTexEnvfv(GL_TEXTURE_SHADER_NV, GL_OFFSET_TEXTURE_MATRIX_NV, mat)\n");
        }
    }
}

static void transform_world(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    /* This function is called by transform_view below if the view matrix was changed too
     *
     * Deliberately no check if the vertex declaration is dirty because the vdecl state
     * does not always update the world matrix, only on a switch between transformed
     * and untrannsformed draws. It *may* happen that the world matrix is set 2 times during one
     * draw, but that should be rather rare and cheaper in total.
     */
    glMatrixMode(GL_MODELVIEW);
    checkGLcall("glMatrixMode");

    if(context->last_was_rhw) {
        glLoadIdentity();
        checkGLcall("glLoadIdentity()");
    } else {
        /* In the general case, the view matrix is the identity matrix */
        if (stateblock->wineD3DDevice->view_ident) {
            glLoadMatrixf((float *) &stateblock->transforms[WINED3DTS_WORLDMATRIX(0)].u.m[0][0]);
            checkGLcall("glLoadMatrixf");
        } else {
            glLoadMatrixf((float *) &stateblock->transforms[WINED3DTS_VIEW].u.m[0][0]);
            checkGLcall("glLoadMatrixf");
            glMultMatrixf((float *) &stateblock->transforms[WINED3DTS_WORLDMATRIX(0)].u.m[0][0]);
            checkGLcall("glMultMatrixf");
        }
    }
}

static void clipplane(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    UINT index = state - STATE_CLIPPLANE(0);

    if(isStateDirty(context, STATE_TRANSFORM(WINED3DTS_VIEW)) || index >= GL_LIMITS(clipplanes)) {
        return;
    }

    /* Clip Plane settings are affected by the model view in OpenGL, the View transform in direct3d */
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixf((float *) &stateblock->transforms[WINED3DTS_VIEW].u.m[0][0]);

    TRACE("Clipplane [%f,%f,%f,%f]\n",
          stateblock->clipplane[index][0],
          stateblock->clipplane[index][1],
          stateblock->clipplane[index][2],
          stateblock->clipplane[index][3]);
    glClipPlane(GL_CLIP_PLANE0 + index, stateblock->clipplane[index]);
    checkGLcall("glClipPlane");

    glPopMatrix();
}

static void transform_worldex(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    UINT matrix = state - STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(0));
    GLenum glMat;
    TRACE("Setting world matrix %d\n", matrix);

    if(matrix >= GL_LIMITS(blends)) {
        WARN("Unsupported blend matrix set\n");
        return;
    } else if(isStateDirty(context, STATE_TRANSFORM(WINED3DTS_VIEW))) {
        return;
    }

    /* GL_MODELVIEW0_ARB:  0x1700
     * GL_MODELVIEW1_ARB:  0x0x850a
     * GL_MODELVIEW2_ARB:  0x8722
     * GL_MODELVIEW3_ARB:  0x8723
     * etc
     * GL_MODELVIEW31_ARB: 0x873F
     */
    if(matrix == 1) glMat = GL_MODELVIEW1_ARB;
    else glMat = GL_MODELVIEW2_ARB - 2 + matrix;

    glMatrixMode(glMat);
    checkGLcall("glMatrixMode(glMat)");

    /* World matrix 0 is multiplied with the view matrix because d3d uses 3 matrices while gl uses only 2. To avoid
     * weighting the view matrix incorrectly it has to be multiplied into every gl modelview matrix
     */
    if(stateblock->wineD3DDevice->view_ident) {
        glLoadMatrixf(&stateblock->transforms[WINED3DTS_WORLDMATRIX(matrix)].u.m[0][0]);
        checkGLcall("glLoadMatrixf")
    } else {
        glLoadMatrixf(&stateblock->transforms[WINED3DTS_VIEW].u.m[0][0]);
        checkGLcall("glLoadMatrixf")
        glMultMatrixf(&stateblock->transforms[WINED3DTS_WORLDMATRIX(matrix)].u.m[0][0]);
        checkGLcall("glMultMatrixf")
    }
}

static void state_vertexblend(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    WINED3DVERTEXBLENDFLAGS val = stateblock->renderState[WINED3DRS_VERTEXBLEND];

    switch(val) {
        case WINED3DVBF_1WEIGHTS:
        case WINED3DVBF_2WEIGHTS:
        case WINED3DVBF_3WEIGHTS:
            if(GL_SUPPORT(ARB_VERTEX_BLEND)) {
                glEnable(GL_VERTEX_BLEND_ARB);
                checkGLcall("glEnable(GL_VERTEX_BLEND_ARB)");

                /* D3D adds one more matrix which has weight (1 - sum(weights)). This is enabled at context
                 * creation with enabling GL_WEIGHT_SUM_UNITY_ARB.
                 */
                GL_EXTCALL(glVertexBlendARB(stateblock->renderState[WINED3DRS_VERTEXBLEND] + 1));

                if(!stateblock->wineD3DDevice->vertexBlendUsed) {
                    int i;
                    for(i = 1; i < GL_LIMITS(blends); i++) {
                        if(!isStateDirty(context, STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(i)))) {
                            transform_worldex(STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(i)), stateblock, context);
                        }
                    }
                    stateblock->wineD3DDevice->vertexBlendUsed = TRUE;
                }
            } else {
                static BOOL once = FALSE;
                if(!once) {
                    once = TRUE;
                    /* TODO: Implement vertex blending in drawStridedSlow */
                    FIXME("Vertex blending enabled, but not supported by hardware\n");
                }
            }
            break;

        case WINED3DVBF_DISABLE:
        case WINED3DVBF_0WEIGHTS: /* for Indexed vertex blending - not supported */
            if(GL_SUPPORT(ARB_VERTEX_BLEND)) {
                glDisable(GL_VERTEX_BLEND_ARB);
                checkGLcall("glDisable(GL_VERTEX_BLEND_ARB)");
            } else {
                TRACE("Vertex blending disabled\n");
            }
            break;

        case WINED3DVBF_TWEENING:
            /* Just set the vertex weight for weight 0, enable vertex blending and hope the app doesn't have
             * vertex weights in the vertices?
             * For now we don't report that as supported, so a warn should suffice
             */
            WARN("Tweening not supported yet\n");
            break;
    }
}

static void transform_view(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    unsigned int k;

    /* If we are changing the View matrix, reset the light and clipping planes to the new view
     * NOTE: We have to reset the positions even if the light/plane is not currently
     *       enabled, since the call to enable it will not reset the position.
     * NOTE2: Apparently texture transforms do NOT need reapplying
     */

    PLIGHTINFOEL *light = NULL;

    glMatrixMode(GL_MODELVIEW);
    checkGLcall("glMatrixMode(GL_MODELVIEW)");
    glLoadMatrixf((float *)(float *) &stateblock->transforms[WINED3DTS_VIEW].u.m[0][0]);
    checkGLcall("glLoadMatrixf(...)");

    /* Reset lights. TODO: Call light apply func */
    for(k = 0; k < stateblock->wineD3DDevice->maxConcurrentLights; k++) {
        light = stateblock->activeLights[k];
        if(!light) continue;
        glLightfv(GL_LIGHT0 + light->glIndex, GL_POSITION, light->lightPosn);
        checkGLcall("glLightfv posn");
        glLightfv(GL_LIGHT0 + light->glIndex, GL_SPOT_DIRECTION, light->lightDirn);
        checkGLcall("glLightfv dirn");
    }

    /* Reset Clipping Planes  */
    for (k = 0; k < GL_LIMITS(clipplanes); k++) {
        if(!isStateDirty(context, STATE_CLIPPLANE(k))) {
            clipplane(STATE_CLIPPLANE(k), stateblock, context);
        }
    }

    if(context->last_was_rhw) {
        glLoadIdentity();
        checkGLcall("glLoadIdentity()");
        /* No need to update the world matrix, the identity is fine */
        return;
    }

    /* Call the world matrix state, this will apply the combined WORLD + VIEW matrix
     * No need to do it here if the state is scheduled for update.
     */
    if(!isStateDirty(context, STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(0)))) {
        transform_world(STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(0)), stateblock, context);
    }

    /* Avoid looping over a number of matrices if the app never used the functionality */
    if(stateblock->wineD3DDevice->vertexBlendUsed) {
        for(k = 1; k < GL_LIMITS(blends); k++) {
            if(!isStateDirty(context, STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(k)))) {
                transform_worldex(STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(k)), stateblock, context);
            }
        }
    }
}

static const GLfloat invymat[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, -1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f};

static void transform_projection(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    glMatrixMode(GL_PROJECTION);
    checkGLcall("glMatrixMode(GL_PROJECTION)");
    glLoadIdentity();
    checkGLcall("glLoadIdentity");

    if(context->last_was_rhw) {
        double X, Y, height, width, minZ, maxZ;

        X      = stateblock->viewport.X;
        Y      = stateblock->viewport.Y;
        height = stateblock->viewport.Height;
        width  = stateblock->viewport.Width;
        minZ   = stateblock->viewport.MinZ;
        maxZ   = stateblock->viewport.MaxZ;

        if(!stateblock->wineD3DDevice->untransformed) {
            /* Transformed vertices are supposed to bypass the whole transform pipeline including
             * frustum clipping. This can't be done in opengl, so this code adjusts the Z range to
             * suppress depth clipping. This can be done because it is an orthogonal projection and
             * the Z coordinate does not affect the size of the primitives. Half Life 1 and Prince of
             * Persia 3D need this.
             *
             * Note that using minZ and maxZ here doesn't entirely fix the problem, since view frustum
             * clipping is still enabled, but it seems to fix it for all apps tested so far. A minor
             * problem can be witnessed in half-life 1 engine based games, the weapon is clipped close
             * to the viewer.
             *
             * Also note that this breaks z comparison against z values filled in with clear,
             * but no app depending on that and disabled clipping has been found yet. Comparing
             * primitives against themselves works, so the Z buffer is still intact for normal hidden
             * surface removal.
             *
             * We could disable clipping entirely by setting the near to infinity and far to -infinity,
             * but this would break Z buffer operation. Raising the range to something less than
             * infinity would help a bit at the cost of Z precision, but it wouldn't eliminate the
             * problem either.
             */
            TRACE("Calling glOrtho with %f, %f, %f, %f\n", width, height, -minZ, -maxZ);
            if(stateblock->wineD3DDevice->render_offscreen) {
                glOrtho(X, X + width, -Y, -Y - height, -minZ, -maxZ);
            } else {
                glOrtho(X, X + width, Y + height, Y, -minZ, -maxZ);
            }
        } else {
            /* If the app mixes transformed and untransformed primitives we can't use the coordinate system
             * trick above because this would mess up transformed and untransformed Z order. Pass the z position
             * unmodified to opengl.
             *
             * If the app depends on mixed types and disabled clipping we're out of luck without a pipeline
             * replacement shader.
             */
            TRACE("Calling glOrtho with %f, %f, %f, %f\n", width, height, 1.0, -1.0);
            if(stateblock->wineD3DDevice->render_offscreen) {
                glOrtho(X, X + width, -Y, -Y - height, 0.0, -1.0);
            } else {
                glOrtho(X, X + width, Y + height, Y, 0.0, -1.0);
            }
        }
        checkGLcall("glOrtho");

        /* Window Coord 0 is the middle of the first pixel, so translate by 1/2 pixels */
        glTranslatef(0.5, 0.5, 0);
        checkGLcall("glTranslatef(0.5, 0.5, 0)");
        /* D3D texture coordinates are flipped compared to OpenGL ones, so
         * render everything upside down when rendering offscreen. */
        if (stateblock->wineD3DDevice->render_offscreen) {
            glMultMatrixf(invymat);
            checkGLcall("glMultMatrixf(invymat)");
        }
    } else {
        /* The rule is that the window coordinate 0 does not correspond to the
            beginning of the first pixel, but the center of the first pixel.
            As a consequence if you want to correctly draw one line exactly from
            the left to the right end of the viewport (with all matrices set to
            be identity), the x coords of both ends of the line would be not
            -1 and 1 respectively but (-1-1/viewport_widh) and (1-1/viewport_width)
            instead.

            1.0 / Width is used because the coord range goes from -1.0 to 1.0, then we
            divide by the Width/Height, so we need the half range(1.0) to translate by
            half a pixel.

            The other fun is that d3d's output z range after the transformation is [0;1],
            but opengl's is [-1;1]. Since the z buffer is in range [0;1] for both, gl
            scales [-1;1] to [0;1]. This would mean that we end up in [0.5;1] and loose a lot
            of Z buffer precision and the clear values do not match in the z test. Thus scale
            [0;1] to [-1;1], so when gl undoes that we utilize the full z range
         */
        glTranslatef(1.0 / stateblock->viewport.Width, -1.0/ stateblock->viewport.Height, -1.0);
        checkGLcall("glTranslatef (1.0 / width, -1.0 / height, -1.0)");
        glScalef(1.0, 1.0, 2.0);

        /* D3D texture coordinates are flipped compared to OpenGL ones, so
            * render everything upside down when rendering offscreen. */
        if (stateblock->wineD3DDevice->render_offscreen) {
            glMultMatrixf(invymat);
            checkGLcall("glMultMatrixf(invymat)");
        }
        glMultMatrixf((float *) &stateblock->transforms[WINED3DTS_PROJECTION].u.m[0][0]);
        checkGLcall("glLoadMatrixf");
    }
}

/* This should match any arrays loaded in loadVertexData.
 * stateblock impl is required for GL_SUPPORT
 * TODO: Only load / unload arrays if we have to.
 */
static inline void unloadVertexData(IWineD3DStateBlockImpl *stateblock) {
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    if (GL_SUPPORT(EXT_SECONDARY_COLOR)) {
        glDisableClientState(GL_SECONDARY_COLOR_ARRAY_EXT);
    }
    if (GL_SUPPORT(ARB_VERTEX_BLEND)) {
        glDisableClientState(GL_WEIGHT_ARRAY_ARB);
    } else if (GL_SUPPORT(EXT_VERTEX_WEIGHTING)) {
        glDisableClientState(GL_VERTEX_WEIGHT_ARRAY_EXT);
    }
    unloadTexCoords(stateblock);
}

/* This should match any arrays loaded in loadNumberedArrays
 * TODO: Only load / unload arrays if we have to.
 */
static inline void unloadNumberedArrays(IWineD3DStateBlockImpl *stateblock) {
    /* disable any attribs (this is the same for both GLSL and ARB modes) */
    GLint maxAttribs;
    int i;

    /* Leave all the attribs disabled */
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS_ARB, &maxAttribs);
    /* MESA does not support it right not */
    if (glGetError() != GL_NO_ERROR)
        maxAttribs = 16;
    for (i = 0; i < maxAttribs; ++i) {
        GL_EXTCALL(glDisableVertexAttribArrayARB(i));
        checkGLcall("glDisableVertexAttribArrayARB(reg);");
    }
}

static inline void loadNumberedArrays(IWineD3DStateBlockImpl *stateblock, WineDirect3DVertexStridedData *strided) {
    GLint curVBO = GL_SUPPORT(ARB_VERTEX_BUFFER_OBJECT) ? -1 : 0;
    int i;
    UINT *offset = stateblock->streamOffset;

    /* Default to no instancing */
    stateblock->wineD3DDevice->instancedDraw = FALSE;

    for (i = 0; i < MAX_ATTRIBS; i++) {

        if (!strided->u.input[i].lpData && !strided->u.input[i].VBO)
            continue;

        /* Do not load instance data. It will be specified using glTexCoord by drawprim */
        if(stateblock->streamFlags[strided->u.input[i].streamNo] & WINED3DSTREAMSOURCE_INSTANCEDATA) {
            GL_EXTCALL(glDisableVertexAttribArrayARB(i));
            stateblock->wineD3DDevice->instancedDraw = TRUE;
            continue;
        }

        TRACE_(d3d_shader)("Loading array %u [VBO=%u]\n", i, strided->u.input[i].VBO);

        if(strided->u.input[i].dwStride) {
            if(curVBO != strided->u.input[i].VBO) {
                GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, strided->u.input[i].VBO));
                checkGLcall("glBindBufferARB");
                curVBO = strided->u.input[i].VBO;
            }
            GL_EXTCALL(glVertexAttribPointerARB(i,
                            WINED3D_ATR_SIZE(strided->u.input[i].dwType),
                            WINED3D_ATR_GLTYPE(strided->u.input[i].dwType),
                            WINED3D_ATR_NORMALIZED(strided->u.input[i].dwType),
                            strided->u.input[i].dwStride,
                            strided->u.input[i].lpData + stateblock->loadBaseVertexIndex * strided->u.input[i].dwStride + offset[strided->u.input[i].streamNo]) );
            GL_EXTCALL(glEnableVertexAttribArrayARB(i));
        } else {
            /* Stride = 0 means always the same values. glVertexAttribPointerARB doesn't do that. Instead disable the pointer and
             * set up the attribute statically. But we have to figure out the system memory address.
             */
            BYTE *ptr = strided->u.input[i].lpData + offset[strided->u.input[i].streamNo];
            if(strided->u.input[i].VBO) {
                IWineD3DVertexBufferImpl *vb = (IWineD3DVertexBufferImpl *) stateblock->streamSource[strided->u.input[i].streamNo];
                ptr += (long) vb->resource.allocatedMemory;
            }
            GL_EXTCALL(glDisableVertexAttribArrayARB(i));

            switch(strided->u.input[i].dwType) {
                case WINED3DDECLTYPE_FLOAT1:
                    GL_EXTCALL(glVertexAttrib1fvARB(i, (float *) ptr));
                    break;
                case WINED3DDECLTYPE_FLOAT2:
                    GL_EXTCALL(glVertexAttrib2fvARB(i, (float *) ptr));
                    break;
                case WINED3DDECLTYPE_FLOAT3:
                    GL_EXTCALL(glVertexAttrib3fvARB(i, (float *) ptr));
                    break;
                case WINED3DDECLTYPE_FLOAT4:
                    GL_EXTCALL(glVertexAttrib4fvARB(i, (float *) ptr));
                    break;

                case WINED3DDECLTYPE_UBYTE4:
                    GL_EXTCALL(glVertexAttrib4NubvARB(i, ptr));
                    break;
                case WINED3DDECLTYPE_UBYTE4N:
                case WINED3DDECLTYPE_D3DCOLOR:
                    GL_EXTCALL(glVertexAttrib4NubvARB(i, ptr));
                    break;

                case WINED3DDECLTYPE_SHORT2:
                    GL_EXTCALL(glVertexAttrib4svARB(i, (GLshort *) ptr));
                    break;
                case WINED3DDECLTYPE_SHORT4:
                    GL_EXTCALL(glVertexAttrib4svARB(i, (GLshort *) ptr));
                    break;

                case WINED3DDECLTYPE_SHORT2N:
                {
                    GLshort s[4] = {((short *) ptr)[0], ((short *) ptr)[1], 0, 1};
                    GL_EXTCALL(glVertexAttrib4NsvARB(i, s));
                    break;
                }
                case WINED3DDECLTYPE_USHORT2N:
                {
                    GLushort s[4] = {((unsigned short *) ptr)[0], ((unsigned short *) ptr)[1], 0, 1};
                    GL_EXTCALL(glVertexAttrib4NusvARB(i, s));
                    break;
                }
                case WINED3DDECLTYPE_SHORT4N:
                    GL_EXTCALL(glVertexAttrib4NsvARB(i, (GLshort *) ptr));
                    break;
                case WINED3DDECLTYPE_USHORT4N:
                    GL_EXTCALL(glVertexAttrib4NusvARB(i, (GLushort *) ptr));
                    break;

                case WINED3DDECLTYPE_UDEC3:
                    FIXME("Unsure about WINED3DDECLTYPE_UDEC3\n");
                    /*glVertexAttrib3usvARB(i, (GLushort *) ptr); Does not exist */
                    break;
                case WINED3DDECLTYPE_DEC3N:
                    FIXME("Unsure about WINED3DDECLTYPE_DEC3N\n");
                    /*glVertexAttrib3NusvARB(i, (GLushort *) ptr); Does not exist */
                    break;

                case WINED3DDECLTYPE_FLOAT16_2:
                    /* Are those 16 bit floats. C doesn't have a 16 bit float type. I could read the single bits and calculate a 4
                     * byte float according to the IEEE standard
                     */
                    FIXME("Unsupported WINED3DDECLTYPE_FLOAT16_2\n");
                    break;
                case WINED3DDECLTYPE_FLOAT16_4:
                    FIXME("Unsupported WINED3DDECLTYPE_FLOAT16_4\n");
                    break;

                case WINED3DDECLTYPE_UNUSED:
                default:
                    ERR("Unexpected declaration in stride 0 attributes\n");
                    break;

            }
        }
    }
}

/* Used from 2 different functions, and too big to justify making it inlined */
static void loadVertexData(IWineD3DStateBlockImpl *stateblock, WineDirect3DVertexStridedData *sd) {
    UINT *offset = stateblock->streamOffset;
    GLint curVBO = GL_SUPPORT(ARB_VERTEX_BUFFER_OBJECT) ? -1 : 0;

    TRACE("Using fast vertex array code\n");

    /* This is fixed function pipeline only, and the fixed function pipeline doesn't do instancing */
    stateblock->wineD3DDevice->instancedDraw = FALSE;

    /* Blend Data ---------------------------------------------- */
    if( (sd->u.s.blendWeights.lpData) || (sd->u.s.blendWeights.VBO) ||
        (sd->u.s.blendMatrixIndices.lpData) || (sd->u.s.blendMatrixIndices.VBO) ) {

        if (GL_SUPPORT(ARB_VERTEX_BLEND)) {
            TRACE("Blend %d %p %d\n", WINED3D_ATR_SIZE(sd->u.s.blendWeights.dwType),
                sd->u.s.blendWeights.lpData + stateblock->loadBaseVertexIndex * sd->u.s.blendWeights.dwStride, sd->u.s.blendWeights.dwStride + offset[sd->u.s.blendWeights.streamNo]);

            glEnableClientState(GL_WEIGHT_ARRAY_ARB);
            checkGLcall("glEnableClientState(GL_WEIGHT_ARRAY_ARB)");

            GL_EXTCALL(glVertexBlendARB(WINED3D_ATR_SIZE(sd->u.s.blendWeights.dwType) + 1));

            VTRACE(("glWeightPointerARB(%d, GL_FLOAT, %d, %p)\n",
                WINED3D_ATR_SIZE(sd->u.s.blendWeights.dwType) ,
                sd->u.s.blendWeights.dwStride,
                sd->u.s.blendWeights.lpData + stateblock->loadBaseVertexIndex * sd->u.s.blendWeights.dwStride + offset[sd->u.s.blendWeights.streamNo]));

            if(curVBO != sd->u.s.blendWeights.VBO) {
                GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, sd->u.s.blendWeights.VBO));
                checkGLcall("glBindBufferARB");
                curVBO = sd->u.s.blendWeights.VBO;
            }

            GL_EXTCALL(glWeightPointerARB)(
                WINED3D_ATR_SIZE(sd->u.s.blendWeights.dwType),
                WINED3D_ATR_GLTYPE(sd->u.s.blendWeights.dwType),
                sd->u.s.blendWeights.dwStride,
                sd->u.s.blendWeights.lpData + stateblock->loadBaseVertexIndex * sd->u.s.blendWeights.dwStride + offset[sd->u.s.blendWeights.streamNo]);

            checkGLcall("glWeightPointerARB");

            if((sd->u.s.blendMatrixIndices.lpData) || (sd->u.s.blendMatrixIndices.VBO)){
                static BOOL showfixme = TRUE;
                if(showfixme){
                    FIXME("blendMatrixIndices support\n");
                    showfixme = FALSE;
                }
            }
        } else if (GL_SUPPORT(EXT_VERTEX_WEIGHTING)) {
            /* FIXME("TODO\n");*/
#if 0

            GL_EXTCALL(glVertexWeightPointerEXT)(
                WINED3D_ATR_SIZE(sd->u.s.blendWeights.dwType),
                WINED3D_ATR_GLTYPE(sd->u.s.blendWeights.dwType),
                sd->u.s.blendWeights.dwStride,
                sd->u.s.blendWeights.lpData + stateblock->loadBaseVertexIndex * sd->u.s.blendWeights.dwStride);
            checkGLcall("glVertexWeightPointerEXT(numBlends, ...)");
            glEnableClientState(GL_VERTEX_WEIGHT_ARRAY_EXT);
            checkGLcall("glEnableClientState(GL_VERTEX_WEIGHT_ARRAY_EXT)");
#endif

        } else {
            /* TODO: support blends in drawStridedSlow
             * No need to write a FIXME here, this is done after the general vertex decl decoding
             */
            WARN("unsupported blending in openGl\n");
        }
    } else {
        if (GL_SUPPORT(ARB_VERTEX_BLEND)) {
            static const GLbyte one = 1;
            GL_EXTCALL(glWeightbvARB(1, &one));
            checkGLcall("glWeightivARB(GL_LIMITS(blends), weights)");
        }
    }

#if 0 /* FOG  ----------------------------------------------*/
    if (sd->u.s.fog.lpData || sd->u.s.fog.VBO) {
        /* TODO: fog*/
    if (GL_SUPPORT(EXT_FOG_COORD) {
             glEnableClientState(GL_FOG_COORDINATE_EXT);
            (GL_EXTCALL)(FogCoordPointerEXT)(
                WINED3D_ATR_GLTYPE(sd->u.s.fog.dwType),
                sd->u.s.fog.dwStride,
                sd->u.s.fog.lpData + stateblock->loadBaseVertexIndex * sd->u.s.fog.dwStride);
        } else {
            /* don't bother falling back to 'slow' as we don't support software FOG yet. */
            /* FIXME: fixme once */
            TRACE("Hardware support for FOG is not avaiable, FOG disabled.\n");
        }
    } else {
        if (GL_SUPPRT(EXT_FOR_COORD) {
             /* make sure fog is disabled */
             glDisableClientState(GL_FOG_COORDINATE_EXT);
        }
    }
#endif

#if 0 /* tangents  ----------------------------------------------*/
    if (sd->u.s.tangent.lpData || sd->u.s.tangent.VBO ||
        sd->u.s.binormal.lpData || sd->u.s.binormal.VBO) {
        /* TODO: tangents*/
        if (GL_SUPPORT(EXT_COORDINATE_FRAME) {
            if (sd->u.s.tangent.lpData || sd->u.s.tangent.VBO) {
                glEnable(GL_TANGENT_ARRAY_EXT);
                (GL_EXTCALL)(TangentPointerEXT)(
                    WINED3D_ATR_GLTYPE(sd->u.s.tangent.dwType),
                    sd->u.s.tangent.dwStride,
                    sd->u.s.tangent.lpData + stateblock->loadBaseVertexIndex * sd->u.s.tangent.dwStride);
            } else {
                    glDisable(GL_TANGENT_ARRAY_EXT);
            }
            if (sd->u.s.binormal.lpData || sd->u.s.binormal.VBO) {
                    glEnable(GL_BINORMAL_ARRAY_EXT);
                    (GL_EXTCALL)(BinormalPointerEXT)(
                        WINED3D_ATR_GLTYPE(sd->u.s.binormal.dwType),
                        sd->u.s.binormal.dwStride,
                        sd->u.s.binormal.lpData + stateblock->loadBaseVertexIndex * sd->u.s.binormal.dwStride);
            } else{
                    glDisable(GL_BINORMAL_ARRAY_EXT);
            }

        } else {
            /* don't bother falling back to 'slow' as we don't support software tangents and binormals yet . */
            /* FIXME: fixme once */
            TRACE("Hardware support for tangents and binormals is not avaiable, tangents and binormals disabled.\n");
        }
    } else {
        if (GL_SUPPORT(EXT_COORDINATE_FRAME) {
             /* make sure fog is disabled */
             glDisable(GL_TANGENT_ARRAY_EXT);
             glDisable(GL_BINORMAL_ARRAY_EXT);
        }
    }
#endif

    /* Point Size ----------------------------------------------*/
    if (sd->u.s.pSize.lpData || sd->u.s.pSize.VBO) {

        /* no such functionality in the fixed function GL pipeline */
        TRACE("Cannot change ptSize here in openGl\n");
        /* TODO: Implement this function in using shaders if they are available */

    }

    /* Vertex Pointers -----------------------------------------*/
    if (sd->u.s.position.lpData != NULL || sd->u.s.position.VBO != 0) {
        /* Note dwType == float3 or float4 == 2 or 3 */
        VTRACE(("glVertexPointer(%d, GL_FLOAT, %d, %p)\n",
                sd->u.s.position.dwStride,
                sd->u.s.position.dwType + 1,
                sd->u.s.position.lpData));

        if(curVBO != sd->u.s.position.VBO) {
            GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, sd->u.s.position.VBO));
            checkGLcall("glBindBufferARB");
            curVBO = sd->u.s.position.VBO;
        }

        /* min(WINED3D_ATR_SIZE(position),3) to Disable RHW mode as 'w' coord
           handling for rhw mode should not impact screen position whereas in GL it does.
           This may  result in very slightly distored textures in rhw mode, but
           a very minimal different. There's always the other option of
           fixing the view matrix to prevent w from having any effect

           This only applies to user pointer sources, in VBOs the vertices are fixed up
         */
        if(sd->u.s.position.VBO == 0) {
            glVertexPointer(3 /* min(WINED3D_ATR_SIZE(sd->u.s.position.dwType),3) */,
                WINED3D_ATR_GLTYPE(sd->u.s.position.dwType),
                sd->u.s.position.dwStride, sd->u.s.position.lpData + stateblock->loadBaseVertexIndex * sd->u.s.position.dwStride + offset[sd->u.s.position.streamNo]);
        } else {
            glVertexPointer(
                WINED3D_ATR_SIZE(sd->u.s.position.dwType),
                WINED3D_ATR_GLTYPE(sd->u.s.position.dwType),
                sd->u.s.position.dwStride, sd->u.s.position.lpData + stateblock->loadBaseVertexIndex * sd->u.s.position.dwStride + offset[sd->u.s.position.streamNo]);
        }
        checkGLcall("glVertexPointer(...)");
        glEnableClientState(GL_VERTEX_ARRAY);
        checkGLcall("glEnableClientState(GL_VERTEX_ARRAY)");
    }

    /* Normals -------------------------------------------------*/
    if (sd->u.s.normal.lpData || sd->u.s.normal.VBO) {
        /* Note dwType == float3 or float4 == 2 or 3 */
        VTRACE(("glNormalPointer(GL_FLOAT, %d, %p)\n",
                sd->u.s.normal.dwStride,
                sd->u.s.normal.lpData));
        if(curVBO != sd->u.s.normal.VBO) {
            GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, sd->u.s.normal.VBO));
            checkGLcall("glBindBufferARB");
            curVBO = sd->u.s.normal.VBO;
        }
        glNormalPointer(
            WINED3D_ATR_GLTYPE(sd->u.s.normal.dwType),
            sd->u.s.normal.dwStride,
            sd->u.s.normal.lpData + stateblock->loadBaseVertexIndex * sd->u.s.normal.dwStride + offset[sd->u.s.normal.streamNo]);
        checkGLcall("glNormalPointer(...)");
        glEnableClientState(GL_NORMAL_ARRAY);
        checkGLcall("glEnableClientState(GL_NORMAL_ARRAY)");

    } else {
        glNormal3f(0, 0, 0);
        checkGLcall("glNormal3f(0, 0, 0)");
    }

    /* Diffuse Colour --------------------------------------------*/
    /*  WARNING: Data here MUST be in RGBA format, so cannot      */
    /*     go directly into fast mode from app pgm, because       */
    /*     directx requires data in BGRA format.                  */
    /* currently fixupVertices swizels the format, but this isn't */
    /* very practical when using VBOS                             */
    /* NOTE: Unless we write a vertex shader to swizel the colour */
    /* , or the user doesn't care and wants the speed advantage   */

    if (sd->u.s.diffuse.lpData || sd->u.s.diffuse.VBO) {
        /* Note dwType == float3 or float4 == 2 or 3 */
        VTRACE(("glColorPointer(4, GL_UNSIGNED_BYTE, %d, %p)\n",
                sd->u.s.diffuse.dwStride,
                sd->u.s.diffuse.lpData));

        if(curVBO != sd->u.s.diffuse.VBO) {
            GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, sd->u.s.diffuse.VBO));
            checkGLcall("glBindBufferARB");
            curVBO = sd->u.s.diffuse.VBO;
        }
        glColorPointer(4, GL_UNSIGNED_BYTE,
                       sd->u.s.diffuse.dwStride,
                       sd->u.s.diffuse.lpData + stateblock->loadBaseVertexIndex * sd->u.s.diffuse.dwStride + offset[sd->u.s.diffuse.streamNo]);
        checkGLcall("glColorPointer(4, GL_UNSIGNED_BYTE, ...)");
        glEnableClientState(GL_COLOR_ARRAY);
        checkGLcall("glEnableClientState(GL_COLOR_ARRAY)");

    } else {
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        checkGLcall("glColor4f(1, 1, 1, 1)");
    }

    /* Specular Colour ------------------------------------------*/
    if (sd->u.s.specular.lpData || sd->u.s.specular.VBO) {
        TRACE("setting specular colour\n");
        /* Note dwType == float3 or float4 == 2 or 3 */
        VTRACE(("glSecondaryColorPointer(4, GL_UNSIGNED_BYTE, %d, %p)\n",
                sd->u.s.specular.dwStride,
                sd->u.s.specular.lpData));
        if (GL_SUPPORT(EXT_SECONDARY_COLOR)) {
            if(curVBO != sd->u.s.specular.VBO) {
                GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, sd->u.s.specular.VBO));
                checkGLcall("glBindBufferARB");
                curVBO = sd->u.s.specular.VBO;
            }
            GL_EXTCALL(glSecondaryColorPointerEXT)(4, GL_UNSIGNED_BYTE,
                                                   sd->u.s.specular.dwStride,
                                                   sd->u.s.specular.lpData + stateblock->loadBaseVertexIndex * sd->u.s.specular.dwStride + offset[sd->u.s.specular.streamNo]);
            vcheckGLcall("glSecondaryColorPointerEXT(4, GL_UNSIGNED_BYTE, ...)");
            glEnableClientState(GL_SECONDARY_COLOR_ARRAY_EXT);
            vcheckGLcall("glEnableClientState(GL_SECONDARY_COLOR_ARRAY_EXT)");
        } else {

        /* Missing specular color is not critical, no warnings */
        VTRACE(("Specular colour is not supported in this GL implementation\n"));
        }

    } else {
        if (GL_SUPPORT(EXT_SECONDARY_COLOR)) {
            GL_EXTCALL(glSecondaryColor3fEXT)(0, 0, 0);
            checkGLcall("glSecondaryColor3fEXT(0, 0, 0)");
        } else {

            /* Missing specular color is not critical, no warnings */
            VTRACE(("Specular colour is not supported in this GL implementation\n"));
        }
    }

    /* Texture coords -------------------------------------------*/
    loadTexCoords(stateblock, sd, &curVBO);
}

static inline void drawPrimitiveTraceDataLocations(
    WineDirect3DVertexStridedData *dataLocations) {

    /* Dump out what parts we have supplied */
    TRACE("Strided Data:\n");
    TRACE_STRIDED((dataLocations), position);
    TRACE_STRIDED((dataLocations), blendWeights);
    TRACE_STRIDED((dataLocations), blendMatrixIndices);
    TRACE_STRIDED((dataLocations), normal);
    TRACE_STRIDED((dataLocations), pSize);
    TRACE_STRIDED((dataLocations), diffuse);
    TRACE_STRIDED((dataLocations), specular);
    TRACE_STRIDED((dataLocations), texCoords[0]);
    TRACE_STRIDED((dataLocations), texCoords[1]);
    TRACE_STRIDED((dataLocations), texCoords[2]);
    TRACE_STRIDED((dataLocations), texCoords[3]);
    TRACE_STRIDED((dataLocations), texCoords[4]);
    TRACE_STRIDED((dataLocations), texCoords[5]);
    TRACE_STRIDED((dataLocations), texCoords[6]);
    TRACE_STRIDED((dataLocations), texCoords[7]);
    TRACE_STRIDED((dataLocations), position2);
    TRACE_STRIDED((dataLocations), normal2);
    TRACE_STRIDED((dataLocations), tangent);
    TRACE_STRIDED((dataLocations), binormal);
    TRACE_STRIDED((dataLocations), tessFactor);
    TRACE_STRIDED((dataLocations), fog);
    TRACE_STRIDED((dataLocations), depth);
    TRACE_STRIDED((dataLocations), sample);

    return;
}

/* Helper for vertexdeclaration() */
static inline void handleStreams(IWineD3DStateBlockImpl *stateblock, BOOL useVertexShaderFunction, WineD3DContext *context) {
    IWineD3DDeviceImpl *device = stateblock->wineD3DDevice;
    BOOL fixup = FALSE;
    WineDirect3DVertexStridedData *dataLocations = &device->strided_streams;

    if(device->up_strided) {
        /* Note: this is a ddraw fixed-function code path */
        TRACE("================ Strided Input ===================\n");
        memcpy(dataLocations, device->up_strided, sizeof(*dataLocations));

        if(TRACE_ON(d3d)) {
            drawPrimitiveTraceDataLocations(dataLocations);
        }
    } else {
        /* Note: This is a fixed function or shader codepath.
         * This means it must handle both types of strided data.
         * Shaders must go through here to zero the strided data, even if they
         * don't set any declaration at all
         */
        TRACE("================ Vertex Declaration  ===================\n");
        memset(dataLocations, 0, sizeof(*dataLocations));
        primitiveDeclarationConvertToStridedData((IWineD3DDevice *) device,
                useVertexShaderFunction, dataLocations, &fixup);
    }

    if (dataLocations->u.s.position_transformed) {
        useVertexShaderFunction = FALSE;
    }

    /* Unload the old arrays before loading the new ones to get old junk out */
    if(context->numberedArraysLoaded) {
        unloadNumberedArrays(stateblock);
        context->numberedArraysLoaded = FALSE;
    }
    if(context->namedArraysLoaded) {
        unloadVertexData(stateblock);
        context->namedArraysLoaded = FALSE;
    }

    if(useVertexShaderFunction) {
        TRACE("Loading numbered arrays\n");
        loadNumberedArrays(stateblock, dataLocations);
        device->useDrawStridedSlow = FALSE;
        context->numberedArraysLoaded = TRUE;
    } else if (fixup ||
               (dataLocations->u.s.pSize.lpData == NULL &&
                dataLocations->u.s.diffuse.lpData == NULL &&
                dataLocations->u.s.specular.lpData == NULL)) {
        /* Load the vertex data using named arrays */
        TRACE("Loading vertex data\n");
        loadVertexData(stateblock, dataLocations);
        device->useDrawStridedSlow = FALSE;
        context->namedArraysLoaded = TRUE;
    } else {
        TRACE("Not loading vertex data\n");
        device->useDrawStridedSlow = TRUE;
    }

/* Generate some fixme's if unsupported functionality is being used */
#define BUFFER_OR_DATA(_attribute) dataLocations->u.s._attribute.lpData
    /* TODO: Either support missing functionality in fixupVertices or by creating a shader to replace the pipeline. */
    if (!useVertexShaderFunction && (BUFFER_OR_DATA(position2) || BUFFER_OR_DATA(normal2))) {
        FIXME("Tweening is only valid with vertex shaders\n");
    }
    if (!useVertexShaderFunction && (BUFFER_OR_DATA(tangent) || BUFFER_OR_DATA(binormal))) {
        FIXME("Tangent and binormal bump mapping is only valid with vertex shaders\n");
    }
    if (!useVertexShaderFunction && (BUFFER_OR_DATA(tessFactor) || BUFFER_OR_DATA(fog) || BUFFER_OR_DATA(depth) || BUFFER_OR_DATA(sample))) {
        FIXME("Extended attributes are only valid with vertex shaders\n");
    }
#undef BUFFER_OR_DATA
}

static void vertexdeclaration(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    BOOL useVertexShaderFunction = FALSE, updateFog = FALSE;
    BOOL usePixelShaderFunction = stateblock->wineD3DDevice->ps_selected_mode != SHADER_NONE && stateblock->pixelShader
            && ((IWineD3DPixelShaderImpl *)stateblock->pixelShader)->baseShader.function;
    BOOL transformed;
    /* Some stuff is in the device until we have per context tracking */
    IWineD3DDeviceImpl *device = stateblock->wineD3DDevice;
    BOOL wasrhw = context->last_was_rhw;

    /* Shaders can be implemented using ARB_PROGRAM, GLSL, or software -
     * here simply check whether a shader was set, or the user disabled shaders
     */
    if (device->vs_selected_mode != SHADER_NONE && stateblock->vertexShader &&
       ((IWineD3DVertexShaderImpl *)stateblock->vertexShader)->baseShader.function != NULL) {
        useVertexShaderFunction = TRUE;

        if(((IWineD3DVertexShaderImpl *)stateblock->vertexShader)->baseShader.reg_maps.fog != context->last_was_foggy_shader) {
            updateFog = TRUE;
        }
    } else if(context->last_was_foggy_shader) {
        updateFog = TRUE;
    }

    handleStreams(stateblock, useVertexShaderFunction, context);

    transformed = device->strided_streams.u.s.position_transformed;
    if (transformed) useVertexShaderFunction = FALSE;

    if(transformed != context->last_was_rhw && !useVertexShaderFunction) {
        updateFog = TRUE;
    }

    /* Reapply lighting if it is not scheduled for reapplication already */
    if(!isStateDirty(context, STATE_RENDER(WINED3DRS_LIGHTING))) {
        state_lighting(STATE_RENDER(WINED3DRS_LIGHTING), stateblock, context);
    }

    if (transformed) {
        context->last_was_rhw = TRUE;
    } else {

        /* Untransformed, so relies on the view and projection matrices */
        context->last_was_rhw = FALSE;
        /* This turns off the Z scale trick to 'disable' viewport frustum clipping in rhw mode*/
        device->untransformed = TRUE;

        /* Todo for sw shaders: Vertex Shader output is already transformed, so set up identity matrices
         * Not needed as long as only hw shaders are supported
         */

        /* This sets the shader output position correction constants.
         * TODO: Move to the viewport state
         */
        if (useVertexShaderFunction) {
            device->posFixup[1] = device->render_offscreen ? -1.0 : 1.0;
        }
    }

    /* Don't have to apply the matrices when vertex shaders are used. When vshaders are turned
     * off this function will be called again anyway to make sure they're properly set
     */
    if(!useVertexShaderFunction) {
        /* TODO: Move this mainly to the viewport state and only apply when the vp has changed
         * or transformed / untransformed was switched
         */
       if(wasrhw != context->last_was_rhw &&
          !isStateDirty(context, STATE_TRANSFORM(WINED3DTS_PROJECTION)) &&
          !isStateDirty(context, STATE_VIEWPORT)) {
            transform_projection(STATE_TRANSFORM(WINED3DTS_PROJECTION), stateblock, context);
        }
        /* World matrix needs reapplication here only if we're switching between rhw and non-rhw
         * mode.
         *
         * If a vertex shader is used, the world matrix changed and then vertex shader unbound
         * this check will fail and the matrix not applied again. This is OK because a simple
         * world matrix change reapplies the matrix - These checks here are only to satisfy the
         * needs of the vertex declaration.
         *
         * World and view matrix go into the same gl matrix, so only apply them when neither is
         * dirty
         */
        if(transformed != wasrhw &&
           !isStateDirty(context, STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(0))) &&
           !isStateDirty(context, STATE_TRANSFORM(WINED3DTS_VIEW))) {
            transform_world(STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(0)), stateblock, context);
        }

        if(!isStateDirty(context, STATE_RENDER(WINED3DRS_COLORVERTEX))) {
            state_colormat(STATE_RENDER(WINED3DRS_COLORVERTEX), stateblock, context);
        }

        if(context->last_was_vshader && !isStateDirty(context, STATE_RENDER(WINED3DRS_CLIPPLANEENABLE))) {
            state_clipping(STATE_RENDER(WINED3DRS_CLIPPLANEENABLE), stateblock, context);
        }
        if(!isStateDirty(context, STATE_RENDER(WINED3DRS_NORMALIZENORMALS))) {
            state_normalize(STATE_RENDER(WINED3DRS_NORMALIZENORMALS), stateblock, context);
        }
    } else {
        /* We compile the shader here because we need the vertex declaration
         * in order to determine if we need to do any swizzling for D3DCOLOR
         * registers. If the shader is already compiled this call will do nothing. */
        IWineD3DVertexShader_CompileShader(stateblock->vertexShader);

        if(!context->last_was_vshader) {
            int i;
            static BOOL warned = FALSE;
            /* Disable all clip planes to get defined results on all drivers. See comment in the
             * state_clipping state handler
             */
            for(i = 0; i < GL_LIMITS(clipplanes); i++) {
                glDisable(GL_CLIP_PLANE0 + i);
                checkGLcall("glDisable(GL_CLIP_PLANE0 + i)");
            }

            if(!warned && stateblock->renderState[WINED3DRS_CLIPPLANEENABLE]) {
                FIXME("Clipping not supported with vertex shaders\n");
                warned = TRUE;
            }
        }
    }

    /* Vertex and pixel shaders are applied together for now, so let the last dirty state do the
     * application
     */
    if (!isStateDirty(context, STATE_PIXELSHADER)) {
        device->shader_backend->shader_select((IWineD3DDevice *)device, usePixelShaderFunction, useVertexShaderFunction);

        if (!isStateDirty(context, STATE_VERTEXSHADERCONSTANT) && (useVertexShaderFunction || usePixelShaderFunction)) {
            shaderconstant(STATE_VERTEXSHADERCONSTANT, stateblock, context);
        }
    }

    context->last_was_vshader = useVertexShaderFunction;

    if(updateFog) {
        state_fog(STATE_RENDER(WINED3DRS_FOGENABLE), stateblock, context);
    }
    if(!useVertexShaderFunction) {
        int i;
        for(i = 0; i < MAX_TEXTURES; i++) {
            if(!isStateDirty(context, STATE_TRANSFORM(WINED3DTS_TEXTURE0 + i))) {
                transform_texture(STATE_TRANSFORM(WINED3DTS_TEXTURE0 + i), stateblock, context);
            }
        }
    }
}

static void viewport(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    glDepthRange(stateblock->viewport.MinZ, stateblock->viewport.MaxZ);
    checkGLcall("glDepthRange");
    /* Note: GL requires lower left, DirectX supplies upper left. This is reversed when using offscreen rendering
     */
    if(stateblock->wineD3DDevice->render_offscreen) {
        glViewport(stateblock->viewport.X,
                   stateblock->viewport.Y,
                   stateblock->viewport.Width, stateblock->viewport.Height);
    } else {
        glViewport(stateblock->viewport.X,
                   (((IWineD3DSurfaceImpl *)stateblock->wineD3DDevice->render_targets[0])->currentDesc.Height - (stateblock->viewport.Y + stateblock->viewport.Height)),
                   stateblock->viewport.Width, stateblock->viewport.Height);
    }

    checkGLcall("glViewport");

    stateblock->wineD3DDevice->posFixup[2] = 1.0 / stateblock->viewport.Width;
    stateblock->wineD3DDevice->posFixup[3] = -1.0 / stateblock->viewport.Height;
    if(!isStateDirty(context, STATE_TRANSFORM(WINED3DTS_PROJECTION))) {
        transform_projection(STATE_TRANSFORM(WINED3DTS_PROJECTION), stateblock, context);
    }
    if(!isStateDirty(context, STATE_RENDER(WINED3DRS_POINTSCALEENABLE))) {
        state_pscale(STATE_RENDER(WINED3DRS_POINTSCALEENABLE), stateblock, context);
    }
}

static void light(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    UINT Index = state - STATE_ACTIVELIGHT(0);
    PLIGHTINFOEL *lightInfo = stateblock->activeLights[Index];

    if(!lightInfo) {
        glDisable(GL_LIGHT0 + Index);
        checkGLcall("glDisable(GL_LIGHT0 + Index)");
    } else {
        float quad_att;
        float colRGBA[] = {0.0, 0.0, 0.0, 0.0};

        /* Light settings are affected by the model view in OpenGL, the View transform in direct3d*/
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadMatrixf((float *)&stateblock->transforms[WINED3DTS_VIEW].u.m[0][0]);

        /* Diffuse: */
        colRGBA[0] = lightInfo->OriginalParms.Diffuse.r;
        colRGBA[1] = lightInfo->OriginalParms.Diffuse.g;
        colRGBA[2] = lightInfo->OriginalParms.Diffuse.b;
        colRGBA[3] = lightInfo->OriginalParms.Diffuse.a;
        glLightfv(GL_LIGHT0 + Index, GL_DIFFUSE, colRGBA);
        checkGLcall("glLightfv");

        /* Specular */
        colRGBA[0] = lightInfo->OriginalParms.Specular.r;
        colRGBA[1] = lightInfo->OriginalParms.Specular.g;
        colRGBA[2] = lightInfo->OriginalParms.Specular.b;
        colRGBA[3] = lightInfo->OriginalParms.Specular.a;
        glLightfv(GL_LIGHT0 + Index, GL_SPECULAR, colRGBA);
        checkGLcall("glLightfv");

        /* Ambient */
        colRGBA[0] = lightInfo->OriginalParms.Ambient.r;
        colRGBA[1] = lightInfo->OriginalParms.Ambient.g;
        colRGBA[2] = lightInfo->OriginalParms.Ambient.b;
        colRGBA[3] = lightInfo->OriginalParms.Ambient.a;
        glLightfv(GL_LIGHT0 + Index, GL_AMBIENT, colRGBA);
        checkGLcall("glLightfv");

        if ((lightInfo->OriginalParms.Range *lightInfo->OriginalParms.Range) >= FLT_MIN) {
            quad_att = 1.4/(lightInfo->OriginalParms.Range *lightInfo->OriginalParms.Range);
        } else {
            quad_att = 0; /*  0 or  MAX?  (0 seems to be ok) */
        }

        /* Do not assign attenuation values for lights that do not use them. D3D apps are free to pass any junk,
         * but gl drivers use them and may crash due to bad Attenuation values. Need for Speed most wanted sets
         * Attenuation0 to NaN and crashes in the gl lib
         */

        switch (lightInfo->OriginalParms.Type) {
            case WINED3DLIGHT_POINT:
                /* Position */
                glLightfv(GL_LIGHT0 + Index, GL_POSITION, &lightInfo->lightPosn[0]);
                checkGLcall("glLightfv");
                glLightf(GL_LIGHT0 + Index, GL_SPOT_CUTOFF, lightInfo->cutoff);
                checkGLcall("glLightf");
                /* Attenuation - Are these right? guessing... */
                glLightf(GL_LIGHT0 + Index, GL_CONSTANT_ATTENUATION,  lightInfo->OriginalParms.Attenuation0);
                checkGLcall("glLightf");
                glLightf(GL_LIGHT0 + Index, GL_LINEAR_ATTENUATION,    lightInfo->OriginalParms.Attenuation1);
                checkGLcall("glLightf");
                if (quad_att < lightInfo->OriginalParms.Attenuation2) quad_att = lightInfo->OriginalParms.Attenuation2;
                glLightf(GL_LIGHT0 + Index, GL_QUADRATIC_ATTENUATION, quad_att);
                checkGLcall("glLightf");
                /* FIXME: Range */
                break;

            case WINED3DLIGHT_SPOT:
                /* Position */
                glLightfv(GL_LIGHT0 + Index, GL_POSITION, &lightInfo->lightPosn[0]);
                checkGLcall("glLightfv");
                /* Direction */
                glLightfv(GL_LIGHT0 + Index, GL_SPOT_DIRECTION, &lightInfo->lightDirn[0]);
                checkGLcall("glLightfv");
                glLightf(GL_LIGHT0 + Index, GL_SPOT_EXPONENT, lightInfo->exponent);
                checkGLcall("glLightf");
                glLightf(GL_LIGHT0 + Index, GL_SPOT_CUTOFF, lightInfo->cutoff);
                checkGLcall("glLightf");
                /* Attenuation - Are these right? guessing... */
                glLightf(GL_LIGHT0 + Index, GL_CONSTANT_ATTENUATION,  lightInfo->OriginalParms.Attenuation0);
                checkGLcall("glLightf");
                glLightf(GL_LIGHT0 + Index, GL_LINEAR_ATTENUATION,    lightInfo->OriginalParms.Attenuation1);
                checkGLcall("glLightf");
                if (quad_att < lightInfo->OriginalParms.Attenuation2) quad_att = lightInfo->OriginalParms.Attenuation2;
                glLightf(GL_LIGHT0 + Index, GL_QUADRATIC_ATTENUATION, quad_att);
                checkGLcall("glLightf");
                /* FIXME: Range */
                break;

            case WINED3DLIGHT_DIRECTIONAL:
                /* Direction */
                glLightfv(GL_LIGHT0 + Index, GL_POSITION, &lightInfo->lightPosn[0]); /* Note gl uses w position of 0 for direction! */
                checkGLcall("glLightfv");
                glLightf(GL_LIGHT0 + Index, GL_SPOT_CUTOFF, lightInfo->cutoff);
                checkGLcall("glLightf");
                glLightf(GL_LIGHT0 + Index, GL_SPOT_EXPONENT, 0.0f);
                checkGLcall("glLightf");
                break;

            default:
                FIXME("Unrecognized light type %d\n", lightInfo->OriginalParms.Type);
        }

        /* Restore the modelview matrix */
        glPopMatrix();

        glEnable(GL_LIGHT0 + Index);
        checkGLcall("glEnable(GL_LIGHT0 + Index)");
    }

    return;
}

static void scissorrect(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    IWineD3DSwapChainImpl *swapchain = (IWineD3DSwapChainImpl *) stateblock->wineD3DDevice->swapchains[0];
    RECT *pRect = &stateblock->scissorRect;
    RECT windowRect;
    UINT winHeight;

    GetClientRect(swapchain->win_handle, &windowRect);
    /* Warning: glScissor uses window coordinates, not viewport coordinates, so our viewport correction does not apply
     * Warning2: Even in windowed mode the coords are relative to the window, not the screen
     */
    winHeight = windowRect.bottom - windowRect.top;
    TRACE("(%p) Setting new Scissor Rect to %d:%d-%d:%d\n", stateblock->wineD3DDevice, pRect->left, pRect->bottom - winHeight,
          pRect->right - pRect->left, pRect->bottom - pRect->top);

    if (stateblock->wineD3DDevice->render_offscreen) {
        glScissor(pRect->left, pRect->top, pRect->right - pRect->left, pRect->bottom - pRect->top);
    } else {
        glScissor(pRect->left, winHeight - pRect->bottom, pRect->right - pRect->left, pRect->bottom - pRect->top);
    }
    checkGLcall("glScissor");
}

static void indexbuffer(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    if(GL_SUPPORT(ARB_VERTEX_BUFFER_OBJECT)) {
        if(stateblock->streamIsUP || stateblock->pIndexData == NULL ) {
            GL_EXTCALL(glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0));
        } else {
            IWineD3DIndexBufferImpl *ib = (IWineD3DIndexBufferImpl *) stateblock->pIndexData;
            GL_EXTCALL(glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, ib->vbo));
        }
    }
}

static void frontface(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    if(stateblock->wineD3DDevice->render_offscreen) {
        glFrontFace(GL_CCW);
        checkGLcall("glFrontFace(GL_CCW)");
    } else {
        glFrontFace(GL_CW);
        checkGLcall("glFrontFace(GL_CW)");
    }
}

const struct StateEntry StateTable[] =
{
      /* State name                                         representative,                                     apply function */
    { /* 0,  Undefined                              */      0,                                                  state_undefined     },
    { /* 1,  WINED3DRS_TEXTUREHANDLE                */      0 /* Handled in ddraw */,                           state_undefined     },
    { /* 2,  WINED3DRS_ANTIALIAS                    */      STATE_RENDER(WINED3DRS_ANTIALIAS),                  state_antialias     },
    { /* 3,  WINED3DRS_TEXTUREADDRESS               */      0 /* Handled in ddraw */,                           state_undefined     },
    { /* 4,  WINED3DRS_TEXTUREPERSPECTIVE           */      STATE_RENDER(WINED3DRS_TEXTUREPERSPECTIVE),         state_perspective   },
    { /* 5,  WINED3DRS_WRAPU                        */      STATE_RENDER(WINED3DRS_WRAPU),                      state_wrapu         },
    { /* 6,  WINED3DRS_WRAPV                        */      STATE_RENDER(WINED3DRS_WRAPV),                      state_wrapv         },
    { /* 7,  WINED3DRS_ZENABLE                      */      STATE_RENDER(WINED3DRS_ZENABLE),                    state_zenable       },
    { /* 8,  WINED3DRS_FILLMODE                     */      STATE_RENDER(WINED3DRS_FILLMODE),                   state_fillmode      },
    { /* 9,  WINED3DRS_SHADEMODE                    */      STATE_RENDER(WINED3DRS_SHADEMODE),                  state_shademode     },
    { /* 10, WINED3DRS_LINEPATTERN                  */      STATE_RENDER(WINED3DRS_LINEPATTERN),                state_linepattern   },
    { /* 11, WINED3DRS_MONOENABLE                   */      STATE_RENDER(WINED3DRS_MONOENABLE),                 state_monoenable    },
    { /* 12, WINED3DRS_ROP2                         */      STATE_RENDER(WINED3DRS_ROP2),                       state_rop2          },
    { /* 13, WINED3DRS_PLANEMASK                    */      STATE_RENDER(WINED3DRS_PLANEMASK),                  state_planemask     },
    { /* 14, WINED3DRS_ZWRITEENABLE                 */      STATE_RENDER(WINED3DRS_ZWRITEENABLE),               state_zwritenable   },
    { /* 15, WINED3DRS_ALPHATESTENABLE              */      STATE_RENDER(WINED3DRS_ALPHATESTENABLE),            state_alpha         },
    { /* 16, WINED3DRS_LASTPIXEL                    */      STATE_RENDER(WINED3DRS_LASTPIXEL),                  state_lastpixel     },
    { /* 17, WINED3DRS_TEXTUREMAG                   */      0 /* Handled in ddraw */,                           state_undefined     },
    { /* 18, WINED3DRS_TEXTUREMIN                   */      0 /* Handled in ddraw */,                           state_undefined     },
    { /* 19, WINED3DRS_SRCBLEND                     */      STATE_RENDER(WINED3DRS_ALPHABLENDENABLE),           state_blend         },
    { /* 20, WINED3DRS_DESTBLEND                    */      STATE_RENDER(WINED3DRS_ALPHABLENDENABLE),           state_blend         },
    { /* 21, WINED3DRS_TEXTUREMAPBLEND              */      0 /* Handled in ddraw */,                           state_undefined     },
    { /* 22, WINED3DRS_CULLMODE                     */      STATE_RENDER(WINED3DRS_CULLMODE),                   state_cullmode      },
    { /* 23, WINED3DRS_ZFUNC                        */      STATE_RENDER(WINED3DRS_ZFUNC),                      state_zfunc         },
    { /* 24, WINED3DRS_ALPHAREF                     */      STATE_RENDER(WINED3DRS_ALPHATESTENABLE),            state_alpha         },
    { /* 25, WINED3DRS_ALPHAFUNC                    */      STATE_RENDER(WINED3DRS_ALPHATESTENABLE),            state_alpha         },
    { /* 26, WINED3DRS_DITHERENABLE                 */      STATE_RENDER(WINED3DRS_DITHERENABLE),               state_ditherenable  },
    { /* 27, WINED3DRS_ALPHABLENDENABLE             */      STATE_RENDER(WINED3DRS_ALPHABLENDENABLE),           state_blend         },
    { /* 28, WINED3DRS_FOGENABLE                    */      STATE_RENDER(WINED3DRS_FOGENABLE),                  state_fog           },
    { /* 29, WINED3DRS_SPECULARENABLE               */      STATE_RENDER(WINED3DRS_SPECULARENABLE),             state_specularenable},
    { /* 30, WINED3DRS_ZVISIBLE                     */      0 /* Not supported according to the msdn */,        state_nogl          },
    { /* 31, WINED3DRS_SUBPIXEL                     */      STATE_RENDER(WINED3DRS_SUBPIXEL),                   state_subpixel      },
    { /* 32, WINED3DRS_SUBPIXELX                    */      STATE_RENDER(WINED3DRS_SUBPIXELX),                  state_subpixelx     },
    { /* 33, WINED3DRS_STIPPLEDALPHA                */      STATE_RENDER(WINED3DRS_STIPPLEDALPHA),              state_stippledalpha },
    { /* 34, WINED3DRS_FOGCOLOR                     */      STATE_RENDER(WINED3DRS_FOGCOLOR),                   state_fogcolor      },
    { /* 35, WINED3DRS_FOGTABLEMODE                 */      STATE_RENDER(WINED3DRS_FOGENABLE),                  state_fog           },
    { /* 36, WINED3DRS_FOGSTART                     */      STATE_RENDER(WINED3DRS_FOGENABLE),                  state_fog           },
    { /* 37, WINED3DRS_FOGEND                       */      STATE_RENDER(WINED3DRS_FOGENABLE),                  state_fog           },
    { /* 38, WINED3DRS_FOGDENSITY                   */      STATE_RENDER(WINED3DRS_FOGDENSITY),                 state_fogdensity    },
    { /* 39, WINED3DRS_STIPPLEENABLE                */      STATE_RENDER(WINED3DRS_STIPPLEENABLE),              state_stippleenable },
    { /* 40, WINED3DRS_EDGEANTIALIAS                */      STATE_RENDER(WINED3DRS_ALPHABLENDENABLE),           state_blend         },
    { /* 41, WINED3DRS_COLORKEYENABLE               */      STATE_RENDER(WINED3DRS_ALPHATESTENABLE),            state_alpha         },
    { /* 42, undefined                              */      0,                                                  state_undefined     },
    { /* 43, WINED3DRS_BORDERCOLOR                  */      STATE_RENDER(WINED3DRS_BORDERCOLOR),                state_bordercolor   },
    { /* 44, WINED3DRS_TEXTUREADDRESSU              */      0, /* Handled in ddraw */                           state_undefined     },
    { /* 45, WINED3DRS_TEXTUREADDRESSV              */      0, /* Handled in ddraw */                           state_undefined     },
    { /* 46, WINED3DRS_MIPMAPLODBIAS                */      STATE_RENDER(WINED3DRS_MIPMAPLODBIAS),              state_mipmaplodbias },
    { /* 47, WINED3DRS_ZBIAS                        */      STATE_RENDER(WINED3DRS_ZBIAS),                      state_zbias         },
    { /* 48, WINED3DRS_RANGEFOGENABLE               */      STATE_RENDER(WINED3DRS_RANGEFOGENABLE),             state_rangefog      },
    { /* 49, WINED3DRS_ANISOTROPY                   */      STATE_RENDER(WINED3DRS_ANISOTROPY),                 state_anisotropy    },
    { /* 50, WINED3DRS_FLUSHBATCH                   */      STATE_RENDER(WINED3DRS_FLUSHBATCH),                 state_flushbatch    },
    { /* 51, WINED3DRS_TRANSLUCENTSORTINDEPENDENT   */      STATE_RENDER(WINED3DRS_TRANSLUCENTSORTINDEPENDENT), state_translucentsi },
    { /* 52, WINED3DRS_STENCILENABLE                */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_stencil       },
    { /* 53, WINED3DRS_STENCILFAIL                  */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_stencil       },
    { /* 54, WINED3DRS_STENCILZFAIL                 */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_stencil       },
    { /* 55, WINED3DRS_STENCILPASS                  */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_stencil       },
    { /* 56, WINED3DRS_STENCILFUNC                  */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_stencil       },
    { /* 57, WINED3DRS_STENCILREF                   */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_stencil       },
    { /* 58, WINED3DRS_STENCILMASK                  */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_stencil       },
    { /* 59, WINED3DRS_STENCILWRITEMASK             */      STATE_RENDER(WINED3DRS_STENCILWRITEMASK),           state_stencilwrite  },
    { /* 60, WINED3DRS_TEXTUREFACTOR                */      STATE_RENDER(WINED3DRS_TEXTUREFACTOR),              state_texfactor     },
    { /* 61, Undefined                              */      0,                                                  state_undefined     },
    { /* 62, Undefined                              */      0,                                                  state_undefined     },
    { /* 63, Undefined                              */      0,                                                  state_undefined     },
    { /* 64, WINED3DRS_STIPPLEPATTERN00             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 65, WINED3DRS_STIPPLEPATTERN01             */      0 /* Obsolete, should he handled by ddraw */,       state_undefined     },
    { /* 66, WINED3DRS_STIPPLEPATTERN02             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 67, WINED3DRS_STIPPLEPATTERN03             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 68, WINED3DRS_STIPPLEPATTERN04             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 69, WINED3DRS_STIPPLEPATTERN05             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 70, WINED3DRS_STIPPLEPATTERN06             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 71, WINED3DRS_STIPPLEPATTERN07             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 72, WINED3DRS_STIPPLEPATTERN08             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 73, WINED3DRS_STIPPLEPATTERN09             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 74, WINED3DRS_STIPPLEPATTERN10             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 75, WINED3DRS_STIPPLEPATTERN11             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 76, WINED3DRS_STIPPLEPATTERN12             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 77, WINED3DRS_STIPPLEPATTERN13             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 78, WINED3DRS_STIPPLEPATTERN14             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 79, WINED3DRS_STIPPLEPATTERN15             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 80, WINED3DRS_STIPPLEPATTERN16             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 81, WINED3DRS_STIPPLEPATTERN17             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 82, WINED3DRS_STIPPLEPATTERN18             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 83, WINED3DRS_STIPPLEPATTERN19             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 84, WINED3DRS_STIPPLEPATTERN20             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 85, WINED3DRS_STIPPLEPATTERN21             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 86, WINED3DRS_STIPPLEPATTERN22             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 87, WINED3DRS_STIPPLEPATTERN23             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 88, WINED3DRS_STIPPLEPATTERN24             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 89, WINED3DRS_STIPPLEPATTERN25             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 90, WINED3DRS_STIPPLEPATTERN26             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 91, WINED3DRS_STIPPLEPATTERN27             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 92, WINED3DRS_STIPPLEPATTERN28             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 93, WINED3DRS_STIPPLEPATTERN29             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 94, WINED3DRS_STIPPLEPATTERN30             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 95, WINED3DRS_STIPPLEPATTERN31             */      0 /* Obsolete, should be handled by ddraw */,       state_undefined     },
    { /* 96, Undefined                              */      0,                                                  state_undefined     },
    { /* 97, Undefined                              */      0,                                                  state_undefined     },
    { /* 98, Undefined                              */      0,                                                  state_undefined     },
    { /* 99, Undefined                              */      0,                                                  state_undefined     },
    { /*100, Undefined                              */      0,                                                  state_undefined     },
    { /*101, Undefined                              */      0,                                                  state_undefined     },
    { /*102, Undefined                              */      0,                                                  state_undefined     },
    { /*103, Undefined                              */      0,                                                  state_undefined     },
    { /*104, Undefined                              */      0,                                                  state_undefined     },
    { /*105, Undefined                              */      0,                                                  state_undefined     },
    { /*106, Undefined                              */      0,                                                  state_undefined     },
    { /*107, Undefined                              */      0,                                                  state_undefined     },
    { /*108, Undefined                              */      0,                                                  state_undefined     },
    { /*109, Undefined                              */      0,                                                  state_undefined     },
    { /*110, Undefined                              */      0,                                                  state_undefined     },
    { /*111, Undefined                              */      0,                                                  state_undefined     },
    { /*112, Undefined                              */      0,                                                  state_undefined     },
    { /*113, Undefined                              */      0,                                                  state_undefined     },
    { /*114, Undefined                              */      0,                                                  state_undefined     },
    { /*115, Undefined                              */      0,                                                  state_undefined     },
    { /*116, Undefined                              */      0,                                                  state_undefined     },
    { /*117, Undefined                              */      0,                                                  state_undefined     },
    { /*118, Undefined                              */      0,                                                  state_undefined     },
    { /*119, Undefined                              */      0,                                                  state_undefined     },
    { /*120, Undefined                              */      0,                                                  state_undefined     },
    { /*121, Undefined                              */      0,                                                  state_undefined     },
    { /*122, Undefined                              */      0,                                                  state_undefined     },
    { /*123, Undefined                              */      0,                                                  state_undefined     },
    { /*124, Undefined                              */      0,                                                  state_undefined     },
    { /*125, Undefined                              */      0,                                                  state_undefined     },
    { /*126, Undefined                              */      0,                                                  state_undefined     },
    { /*127, Undefined                              */      0,                                                  state_undefined     },
    /* Big hole ends */
    { /*128, WINED3DRS_WRAP0                        */      STATE_RENDER(WINED3DRS_WRAP0),                      state_wrap          },
    { /*129, WINED3DRS_WRAP1                        */      STATE_RENDER(WINED3DRS_WRAP0),                      state_wrap          },
    { /*130, WINED3DRS_WRAP2                        */      STATE_RENDER(WINED3DRS_WRAP0),                      state_wrap          },
    { /*131, WINED3DRS_WRAP3                        */      STATE_RENDER(WINED3DRS_WRAP0),                      state_wrap          },
    { /*132, WINED3DRS_WRAP4                        */      STATE_RENDER(WINED3DRS_WRAP0),                      state_wrap          },
    { /*133, WINED3DRS_WRAP5                        */      STATE_RENDER(WINED3DRS_WRAP0),                      state_wrap          },
    { /*134, WINED3DRS_WRAP6                        */      STATE_RENDER(WINED3DRS_WRAP0),                      state_wrap          },
    { /*135, WINED3DRS_WRAP7                        */      STATE_RENDER(WINED3DRS_WRAP0),                      state_wrap          },
    { /*136, WINED3DRS_CLIPPING                     */      STATE_RENDER(WINED3DRS_CLIPPING),                   state_clipping      },
    { /*137, WINED3DRS_LIGHTING                     */      STATE_RENDER(WINED3DRS_LIGHTING),                   state_lighting      },
    { /*138, WINED3DRS_EXTENTS                      */      STATE_RENDER(WINED3DRS_EXTENTS),                    state_extents       },
    { /*139, WINED3DRS_AMBIENT                      */      STATE_RENDER(WINED3DRS_AMBIENT),                    state_ambient       },
    { /*140, WINED3DRS_FOGVERTEXMODE                */      STATE_RENDER(WINED3DRS_FOGENABLE),                  state_fog           },
    { /*141, WINED3DRS_COLORVERTEX                  */      STATE_RENDER(WINED3DRS_COLORVERTEX),                state_colormat      },
    { /*142, WINED3DRS_LOCALVIEWER                  */      STATE_RENDER(WINED3DRS_LOCALVIEWER),                state_localviewer   },
    { /*143, WINED3DRS_NORMALIZENORMALS             */      STATE_RENDER(WINED3DRS_NORMALIZENORMALS),           state_normalize     },
    { /*144, WINED3DRS_COLORKEYBLENDENABLE          */      STATE_RENDER(WINED3DRS_COLORKEYBLENDENABLE),        state_ckeyblend     },
    { /*145, WINED3DRS_DIFFUSEMATERIALSOURCE        */      STATE_RENDER(WINED3DRS_COLORVERTEX),                state_colormat      },
    { /*146, WINED3DRS_SPECULARMATERIALSOURCE       */      STATE_RENDER(WINED3DRS_COLORVERTEX),                state_colormat      },
    { /*147, WINED3DRS_AMBIENTMATERIALSOURCE        */      STATE_RENDER(WINED3DRS_COLORVERTEX),                state_colormat      },
    { /*148, WINED3DRS_EMISSIVEMATERIALSOURCE       */      STATE_RENDER(WINED3DRS_COLORVERTEX),                state_colormat      },
    { /*149, Undefined                              */      0,                                                  state_undefined     },
    { /*150, Undefined                              */      0,                                                  state_undefined     },
    { /*151, WINED3DRS_VERTEXBLEND                  */      STATE_RENDER(WINED3DRS_VERTEXBLEND),                state_vertexblend   },
    { /*152, WINED3DRS_CLIPPLANEENABLE              */      STATE_RENDER(WINED3DRS_CLIPPING),                   state_clipping      },
    { /*153, WINED3DRS_SOFTWAREVERTEXPROCESSING     */      0,                                                  state_nogl          },
    { /*154, WINED3DRS_POINTSIZE                    */      STATE_RENDER(WINED3DRS_POINTSCALEENABLE),           state_pscale        },
    { /*155, WINED3DRS_POINTSIZE_MIN                */      STATE_RENDER(WINED3DRS_POINTSIZE_MIN),              state_psizemin      },
    { /*156, WINED3DRS_POINTSPRITEENABLE            */      STATE_RENDER(WINED3DRS_POINTSPRITEENABLE),          state_pointsprite   },
    { /*157, WINED3DRS_POINTSCALEENABLE             */      STATE_RENDER(WINED3DRS_POINTSCALEENABLE),           state_pscale        },
    { /*158, WINED3DRS_POINTSCALE_A                 */      STATE_RENDER(WINED3DRS_POINTSCALEENABLE),           state_pscale        },
    { /*159, WINED3DRS_POINTSCALE_B                 */      STATE_RENDER(WINED3DRS_POINTSCALEENABLE),           state_pscale        },
    { /*160, WINED3DRS_POINTSCALE_C                 */      STATE_RENDER(WINED3DRS_POINTSCALEENABLE),           state_pscale        },
    { /*161, WINED3DRS_MULTISAMPLEANTIALIAS         */      STATE_RENDER(WINED3DRS_MULTISAMPLEANTIALIAS),       state_multisampleaa },
    { /*162, WINED3DRS_MULTISAMPLEMASK              */      STATE_RENDER(WINED3DRS_MULTISAMPLEMASK),            state_multisampmask },
    { /*163, WINED3DRS_PATCHEDGESTYLE               */      STATE_RENDER(WINED3DRS_PATCHEDGESTYLE),             state_patchedgestyle},
    { /*164, WINED3DRS_PATCHSEGMENTS                */      STATE_RENDER(WINED3DRS_PATCHSEGMENTS),              state_patchsegments },
    { /*165, WINED3DRS_DEBUGMONITORTOKEN            */      STATE_RENDER(WINED3DRS_DEBUGMONITORTOKEN),          state_nogl          },
    { /*166, WINED3DRS_POINTSIZE_MAX                */      STATE_RENDER(WINED3DRS_POINTSIZE_MAX),              state_psizemax      },
    { /*167, WINED3DRS_INDEXEDVERTEXBLENDENABLE     */      0,                                                  state_nogl          },
    { /*168, WINED3DRS_COLORWRITEENABLE             */      STATE_RENDER(WINED3DRS_COLORWRITEENABLE),           state_colorwrite    },
    { /*169, Undefined                              */      0,                                                  state_undefined     },
    { /*170, WINED3DRS_TWEENFACTOR                  */      0,                                                  state_nogl          },
    { /*171, WINED3DRS_BLENDOP                      */      STATE_RENDER(WINED3DRS_BLENDOP),                    state_blendop       },
    { /*172, WINED3DRS_POSITIONDEGREE               */      STATE_RENDER(WINED3DRS_POSITIONDEGREE),             state_positiondegree},
    { /*173, WINED3DRS_NORMALDEGREE                 */      STATE_RENDER(WINED3DRS_NORMALDEGREE),               state_normaldegree  },
      /*172, WINED3DRS_POSITIONORDER                */      /* Value assigned to 2 state names */
      /*173, WINED3DRS_NORMALORDER                  */      /* Value assigned to 2 state names */
    { /*174, WINED3DRS_SCISSORTESTENABLE            */      STATE_RENDER(WINED3DRS_SCISSORTESTENABLE),          state_scissor       },
    { /*175, WINED3DRS_SLOPESCALEDEPTHBIAS          */      STATE_RENDER(WINED3DRS_DEPTHBIAS),                  state_depthbias     },
    { /*176, WINED3DRS_ANTIALIASEDLINEENABLE        */      STATE_RENDER(WINED3DRS_ALPHABLENDENABLE),           state_blend         },
    { /*177, undefined                              */      0,                                                  state_undefined     },
    { /*178, WINED3DRS_MINTESSELLATIONLEVEL         */      STATE_RENDER(WINED3DRS_ENABLEADAPTIVETESSELLATION), state_tessellation  },
    { /*179, WINED3DRS_MAXTESSELLATIONLEVEL         */      STATE_RENDER(WINED3DRS_ENABLEADAPTIVETESSELLATION), state_tessellation  },
    { /*180, WINED3DRS_ADAPTIVETESS_X               */      STATE_RENDER(WINED3DRS_ENABLEADAPTIVETESSELLATION), state_tessellation  },
    { /*181, WINED3DRS_ADAPTIVETESS_Y               */      STATE_RENDER(WINED3DRS_ENABLEADAPTIVETESSELLATION), state_tessellation  },
    { /*182, WINED3DRS_ADAPTIVETESS_Z               */      STATE_RENDER(WINED3DRS_ENABLEADAPTIVETESSELLATION), state_tessellation  },
    { /*183, WINED3DRS_ADAPTIVETESS_W               */      STATE_RENDER(WINED3DRS_ENABLEADAPTIVETESSELLATION), state_tessellation  },
    { /*184, WINED3DRS_ENABLEADAPTIVETESSELLATION   */      STATE_RENDER(WINED3DRS_ENABLEADAPTIVETESSELLATION), state_tessellation  },
    { /*185, WINED3DRS_TWOSIDEDSTENCILMODE          */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_stencil       },
    { /*186, WINED3DRS_CCW_STENCILFAIL              */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_stencil       },
    { /*187, WINED3DRS_CCW_STENCILZFAIL             */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_stencil       },
    { /*188, WINED3DRS_CCW_STENCILPASS              */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_stencil       },
    { /*189, WINED3DRS_CCW_STENCILFUNC              */      STATE_RENDER(WINED3DRS_STENCILENABLE),              state_stencil       },
    { /*190, WINED3DRS_COLORWRITEENABLE1            */      STATE_RENDER(WINED3DRS_COLORWRITEENABLE),           state_colorwrite    },
    { /*191, WINED3DRS_COLORWRITEENABLE2            */      STATE_RENDER(WINED3DRS_COLORWRITEENABLE),           state_colorwrite    },
    { /*192, WINED3DRS_COLORWRITEENABLE3            */      STATE_RENDER(WINED3DRS_COLORWRITEENABLE),           state_colorwrite    },
    { /*193, WINED3DRS_BLENDFACTOR                  */      STATE_RENDER(WINED3DRS_BLENDFACTOR),                state_blendfactor   },
    { /*194, WINED3DRS_SRGBWRITEENABLE              */      STATE_PIXELSHADER,                                  pixelshader         },
    { /*195, WINED3DRS_DEPTHBIAS                    */      STATE_RENDER(WINED3DRS_DEPTHBIAS),                  state_depthbias     },
    { /*196, undefined                              */      0,                                                  state_undefined     },
    { /*197, undefined                              */      0,                                                  state_undefined     },
    { /*198, WINED3DRS_WRAP8                        */      STATE_RENDER(WINED3DRS_WRAP0),                      state_wrap          },
    { /*199, WINED3DRS_WRAP9                        */      STATE_RENDER(WINED3DRS_WRAP0),                      state_wrap          },
    { /*200, WINED3DRS_WRAP10                       */      STATE_RENDER(WINED3DRS_WRAP0),                      state_wrap          },
    { /*201, WINED3DRS_WRAP11                       */      STATE_RENDER(WINED3DRS_WRAP0),                      state_wrap          },
    { /*202, WINED3DRS_WRAP12                       */      STATE_RENDER(WINED3DRS_WRAP0),                      state_wrap          },
    { /*203, WINED3DRS_WRAP13                       */      STATE_RENDER(WINED3DRS_WRAP0),                      state_wrap          },
    { /*204, WINED3DRS_WRAP14                       */      STATE_RENDER(WINED3DRS_WRAP0),                      state_wrap          },
    { /*205, WINED3DRS_WRAP15                       */      STATE_RENDER(WINED3DRS_WRAP0),                      state_wrap          },
    { /*206, WINED3DRS_SEPARATEALPHABLENDENABLE     */      STATE_RENDER(WINED3DRS_SEPARATEALPHABLENDENABLE),   state_separateblend },
    { /*207, WINED3DRS_SRCBLENDALPHA                */      STATE_RENDER(WINED3DRS_SEPARATEALPHABLENDENABLE),   state_separateblend },
    { /*208, WINED3DRS_DESTBLENDALPHA               */      STATE_RENDER(WINED3DRS_SEPARATEALPHABLENDENABLE),   state_separateblend },
    { /*209, WINED3DRS_BLENDOPALPHA                 */      STATE_RENDER(WINED3DRS_SEPARATEALPHABLENDENABLE),   state_separateblend },
    /* Texture stage states */
    { /*0, 01, WINED3DTSS_COLOROP                   */      STATE_TEXTURESTAGE(0, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*0, 02, WINED3DTSS_COLORARG1                 */      STATE_TEXTURESTAGE(0, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*0, 03, WINED3DTSS_COLORARG2                 */      STATE_TEXTURESTAGE(0, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*0, 04, WINED3DTSS_ALPHAOP                   */      STATE_TEXTURESTAGE(0, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*0, 05, WINED3DTSS_ALPHAARG1                 */      STATE_TEXTURESTAGE(0, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*0, 06, WINED3DTSS_ALPHAARG2                 */      STATE_TEXTURESTAGE(0, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*0, 07, WINED3DTSS_BUMPENVMAT00              */      STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*0, 08, WINED3DTSS_BUMPENVMAT01              */      STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*0, 09, WINED3DTSS_BUMPENVMAT10              */      STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*0, 10, WINED3DTSS_BUMPENVMAT11              */      STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*0, 11, WINED3DTSS_TEXCOORDINDEX             */      STATE_TEXTURESTAGE(0, WINED3DTSS_TEXCOORDINDEX),    tex_coordindex      },
    { /*0, 12, WINED3DTSS_ADDRESS                   */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*0, 13, WINED3DTSS_ADDRESSU                  */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*0, 14, WINED3DTSS_ADDRESSV                  */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*0, 15, WINED3DTSS_BORDERCOLOR               */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*0, 16, WINED3DTSS_MAGFILTER                 */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*0, 17, WINED3DTSS_MINFILTER                 */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*0, 18, WINED3DTSS_MIPFILTER                 */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*0, 19, WINED3DTSS_MIPMAPLODBIAS             */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*0, 20, WINED3DTSS_MAXMIPLEVEL               */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*0, 21, WINED3DTSS_MAXANISOTROPY             */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*0, 22, WINED3DTSS_BUMPENVLSCALE             */      STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlscale   },
    { /*0, 23, WINED3DTSS_BUMPENVLOFFSET            */      STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVLOFFSET),   tex_bumpenvloffset  },
    { /*0, 24, WINED3DTSS_TEXTURETRANSFORMFLAGS     */      STATE_TRANSFORM(WINED3DTS_TEXTURE0),                transform_texture   },
    { /*0, 25, WINED3DTSS_ADDRESSW                  */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*0, 26, WINED3DTSS_COLORARG0                 */      STATE_TEXTURESTAGE(0, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*0, 27, WINED3DTSS_ALPHAARG0                 */      STATE_TEXTURESTAGE(0, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*0, 28, WINED3DTSS_RESULTARG                 */      STATE_TEXTURESTAGE(0, WINED3DTSS_RESULTARG),        tex_resultarg       },
    { /*0, 29, undefined                            */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*0, 30, undefined                            */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*0, 31, undefined                            */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*0, 32, WINED3DTSS_CONSTANT                  */      0 /* As long as we don't support D3DTA_CONSTANT */, state_nogl          },

    { /*1, 01, WINED3DTSS_COLOROP                   */      STATE_TEXTURESTAGE(1, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*1, 02, WINED3DTSS_COLORARG1                 */      STATE_TEXTURESTAGE(1, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*1, 03, WINED3DTSS_COLORARG2                 */      STATE_TEXTURESTAGE(1, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*1, 04, WINED3DTSS_ALPHAOP                   */      STATE_TEXTURESTAGE(1, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*1, 05, WINED3DTSS_ALPHAARG1                 */      STATE_TEXTURESTAGE(1, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*1, 06, WINED3DTSS_ALPHAARG2                 */      STATE_TEXTURESTAGE(1, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*1, 07, WINED3DTSS_BUMPENVMAT00              */      STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*1, 08, WINED3DTSS_BUMPENVMAT01              */      STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*1, 09, WINED3DTSS_BUMPENVMAT10              */      STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*1, 10, WINED3DTSS_BUMPENVMAT11              */      STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*1, 11, WINED3DTSS_TEXCOORDINDEX             */      STATE_TEXTURESTAGE(1, WINED3DTSS_TEXCOORDINDEX),    tex_coordindex      },
    { /*1, 12, WINED3DTSS_ADDRESS                   */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*1, 13, WINED3DTSS_ADDRESSU                  */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*1, 14, WINED3DTSS_ADDRESSV                  */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*1, 15, WINED3DTSS_BORDERCOLOR               */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*1, 16, WINED3DTSS_MAGFILTER                 */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*1, 17, WINED3DTSS_MINFILTER                 */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*1, 18, WINED3DTSS_MIPFILTER                 */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*1, 19, WINED3DTSS_MIPMAPLODBIAS             */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*1, 20, WINED3DTSS_MAXMIPLEVEL               */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*1, 21, WINED3DTSS_MAXANISOTROPY             */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*1, 22, WINED3DTSS_BUMPENVLSCALE             */      STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlscale   },
    { /*1, 23, WINED3DTSS_BUMPENVLOFFSET            */      STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVLOFFSET),   tex_bumpenvloffset  },
    { /*1, 24, WINED3DTSS_TEXTURETRANSFORMFLAGS     */      STATE_TRANSFORM(WINED3DTS_TEXTURE1),                transform_texture   },
    { /*1, 25, WINED3DTSS_ADDRESSW                  */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*1, 26, WINED3DTSS_COLORARG0                 */      STATE_TEXTURESTAGE(1, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*1, 27, WINED3DTSS_ALPHAARG0                 */      STATE_TEXTURESTAGE(1, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*1, 28, WINED3DTSS_RESULTARG                 */      STATE_TEXTURESTAGE(1, WINED3DTSS_RESULTARG),        tex_resultarg       },
    { /*1, 29, undefined                            */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*1, 30, undefined                            */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*1, 31, undefined                            */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*1, 32, WINED3DTSS_CONSTANT                  */      0 /* As long as we don't support D3DTA_CONSTANT */, state_nogl          },

    { /*2, 01, WINED3DTSS_COLOROP                   */      STATE_TEXTURESTAGE(2, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*2, 02, WINED3DTSS_COLORARG1                 */      STATE_TEXTURESTAGE(2, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*2, 03, WINED3DTSS_COLORARG2                 */      STATE_TEXTURESTAGE(2, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*2, 04, WINED3DTSS_ALPHAOP                   */      STATE_TEXTURESTAGE(2, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*2, 05, WINED3DTSS_ALPHAARG1                 */      STATE_TEXTURESTAGE(2, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*2, 06, WINED3DTSS_ALPHAARG2                 */      STATE_TEXTURESTAGE(2, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*2, 07, WINED3DTSS_BUMPENVMAT00              */      STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*2, 08, WINED3DTSS_BUMPENVMAT01              */      STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*2, 09, WINED3DTSS_BUMPENVMAT10              */      STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*2, 10, WINED3DTSS_BUMPENVMAT11              */      STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*2, 11, WINED3DTSS_TEXCOORDINDEX             */      STATE_TEXTURESTAGE(2, WINED3DTSS_TEXCOORDINDEX),    tex_coordindex      },
    { /*2, 12, WINED3DTSS_ADDRESS                   */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*2, 13, WINED3DTSS_ADDRESSU                  */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*2, 14, WINED3DTSS_ADDRESSV                  */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*2, 15, WINED3DTSS_BORDERCOLOR               */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*2, 16, WINED3DTSS_MAGFILTER                 */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*2, 17, WINED3DTSS_MINFILTER                 */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*2, 18, WINED3DTSS_MIPFILTER                 */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*2, 19, WINED3DTSS_MIPMAPLODBIAS             */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*2, 20, WINED3DTSS_MAXMIPLEVEL               */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*2, 21, WINED3DTSS_MAXANISOTROPY             */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*2, 22, WINED3DTSS_BUMPENVLSCALE             */      STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlscale   },
    { /*2, 23, WINED3DTSS_BUMPENVLOFFSET            */      STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVLOFFSET),   tex_bumpenvloffset  },
    { /*2, 24, WINED3DTSS_TEXTURETRANSFORMFLAGS     */      STATE_TRANSFORM(WINED3DTS_TEXTURE2),                transform_texture   },
    { /*2, 25, WINED3DTSS_ADDRESSW                  */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*2, 26, WINED3DTSS_COLORARG0                 */      STATE_TEXTURESTAGE(2, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*2, 27, WINED3DTSS_ALPHAARG0                 */      STATE_TEXTURESTAGE(2, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*2, 28, WINED3DTSS_RESULTARG                 */      STATE_TEXTURESTAGE(2, WINED3DTSS_RESULTARG),        tex_resultarg       },
    { /*2, 29, undefined                            */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*2, 30, undefined                            */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*2, 31, undefined                            */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*2, 32, WINED3DTSS_CONSTANT                  */      0 /* As long as we don't support D3DTA_CONSTANT */, state_nogl          },

    { /*3, 01, WINED3DTSS_COLOROP                   */      STATE_TEXTURESTAGE(3, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*3, 02, WINED3DTSS_COLORARG1                 */      STATE_TEXTURESTAGE(3, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*3, 03, WINED3DTSS_COLORARG2                 */      STATE_TEXTURESTAGE(3, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*3, 04, WINED3DTSS_ALPHAOP                   */      STATE_TEXTURESTAGE(3, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*3, 05, WINED3DTSS_ALPHAARG1                 */      STATE_TEXTURESTAGE(3, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*3, 06, WINED3DTSS_ALPHAARG2                 */      STATE_TEXTURESTAGE(3, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*3, 07, WINED3DTSS_BUMPENVMAT00              */      STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*3, 08, WINED3DTSS_BUMPENVMAT01              */      STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*3, 09, WINED3DTSS_BUMPENVMAT10              */      STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*3, 10, WINED3DTSS_BUMPENVMAT11              */      STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*3, 11, WINED3DTSS_TEXCOORDINDEX             */      STATE_TEXTURESTAGE(3, WINED3DTSS_TEXCOORDINDEX),    tex_coordindex      },
    { /*3, 12, WINED3DTSS_ADDRESS                   */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*3, 13, WINED3DTSS_ADDRESSU                  */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*3, 14, WINED3DTSS_ADDRESSV                  */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*3, 15, WINED3DTSS_BORDERCOLOR               */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*3, 16, WINED3DTSS_MAGFILTER                 */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*3, 17, WINED3DTSS_MINFILTER                 */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*3, 18, WINED3DTSS_MIPFILTER                 */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*3, 19, WINED3DTSS_MIPMAPLODBIAS             */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*3, 20, WINED3DTSS_MAXMIPLEVEL               */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*3, 21, WINED3DTSS_MAXANISOTROPY             */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*3, 22, WINED3DTSS_BUMPENVLSCALE             */      STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlscale   },
    { /*3, 23, WINED3DTSS_BUMPENVLOFFSET            */      STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVLOFFSET),   tex_bumpenvloffset  },
    { /*3, 24, WINED3DTSS_TEXTURETRANSFORMFLAGS     */      STATE_TRANSFORM(WINED3DTS_TEXTURE3),                transform_texture   },
    { /*3, 25, WINED3DTSS_ADDRESSW                  */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*3, 26, WINED3DTSS_COLORARG0                 */      STATE_TEXTURESTAGE(3, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*3, 27, WINED3DTSS_ALPHAARG0                 */      STATE_TEXTURESTAGE(3, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*3, 28, WINED3DTSS_RESULTARG                 */      STATE_TEXTURESTAGE(3, WINED3DTSS_RESULTARG),        tex_resultarg       },
    { /*3, 29, undefined                            */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*3, 30, undefined                            */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*3, 31, undefined                            */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*3, 32, WINED3DTSS_CONSTANT                  */      0 /* As long as we don't support D3DTA_CONSTANT */, state_nogl          },

    { /*4, 01, WINED3DTSS_COLOROP                   */      STATE_TEXTURESTAGE(4, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*4, 02, WINED3DTSS_COLORARG1                 */      STATE_TEXTURESTAGE(4, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*4, 03, WINED3DTSS_COLORARG2                 */      STATE_TEXTURESTAGE(4, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*4, 04, WINED3DTSS_ALPHAOP                   */      STATE_TEXTURESTAGE(4, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*4, 05, WINED3DTSS_ALPHAARG1                 */      STATE_TEXTURESTAGE(4, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*4, 06, WINED3DTSS_ALPHAARG2                 */      STATE_TEXTURESTAGE(4, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*4, 07, WINED3DTSS_BUMPENVMAT00              */      STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*4, 08, WINED3DTSS_BUMPENVMAT01              */      STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*4, 09, WINED3DTSS_BUMPENVMAT10              */      STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*4, 10, WINED3DTSS_BUMPENVMAT11              */      STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*4, 11, WINED3DTSS_TEXCOORDINDEX             */      STATE_TEXTURESTAGE(4, WINED3DTSS_TEXCOORDINDEX),    tex_coordindex      },
    { /*4, 12, WINED3DTSS_ADDRESS                   */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*4, 13, WINED3DTSS_ADDRESSU                  */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*4, 14, WINED3DTSS_ADDRESSV                  */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*4, 15, WINED3DTSS_BORDERCOLOR               */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*4, 16, WINED3DTSS_MAGFILTER                 */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*4, 17, WINED3DTSS_MINFILTER                 */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*4, 18, WINED3DTSS_MIPFILTER                 */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*4, 19, WINED3DTSS_MIPMAPLODBIAS             */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*4, 20, WINED3DTSS_MAXMIPLEVEL               */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*4, 21, WINED3DTSS_MAXANISOTROPY             */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*4, 22, WINED3DTSS_BUMPENVLSCALE             */      STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlscale   },
    { /*4, 23, WINED3DTSS_BUMPENVLOFFSET            */      STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVLOFFSET),   tex_bumpenvloffset  },
    { /*4, 24, WINED3DTSS_TEXTURETRANSFORMFLAGS     */      STATE_TRANSFORM(WINED3DTS_TEXTURE4),                transform_texture   },
    { /*4, 25, WINED3DTSS_ADDRESSW                  */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*4, 26, WINED3DTSS_COLORARG0                 */      STATE_TEXTURESTAGE(4, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*4, 27, WINED3DTSS_ALPHAARG0                 */      STATE_TEXTURESTAGE(4, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*4, 28, WINED3DTSS_RESULTARG                 */      STATE_TEXTURESTAGE(4, WINED3DTSS_RESULTARG),        tex_resultarg       },
    { /*4, 29, undefined                            */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*4, 30, undefined                            */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*4, 31, undefined                            */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*4, 32, WINED3DTSS_CONSTANT                  */      0 /* As long as we don't support D3DTA_CONSTANT */, state_nogl          },

    { /*5, 01, WINED3DTSS_COLOROP                   */      STATE_TEXTURESTAGE(5, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*5, 02, WINED3DTSS_COLORARG1                 */      STATE_TEXTURESTAGE(5, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*5, 03, WINED3DTSS_COLORARG2                 */      STATE_TEXTURESTAGE(5, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*5, 04, WINED3DTSS_ALPHAOP                   */      STATE_TEXTURESTAGE(5, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*5, 05, WINED3DTSS_ALPHAARG1                 */      STATE_TEXTURESTAGE(5, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*5, 06, WINED3DTSS_ALPHAARG2                 */      STATE_TEXTURESTAGE(5, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*5, 07, WINED3DTSS_BUMPENVMAT00              */      STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*5, 08, WINED3DTSS_BUMPENVMAT01              */      STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*5, 09, WINED3DTSS_BUMPENVMAT10              */      STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*5, 10, WINED3DTSS_BUMPENVMAT11              */      STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*5, 11, WINED3DTSS_TEXCOORDINDEX             */      STATE_TEXTURESTAGE(5, WINED3DTSS_TEXCOORDINDEX),    tex_coordindex      },
    { /*5, 12, WINED3DTSS_ADDRESS                   */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*5, 13, WINED3DTSS_ADDRESSU                  */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*5, 14, WINED3DTSS_ADDRESSV                  */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*5, 15, WINED3DTSS_BORDERCOLOR               */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*5, 16, WINED3DTSS_MAGFILTER                 */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*5, 17, WINED3DTSS_MINFILTER                 */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*5, 18, WINED3DTSS_MIPFILTER                 */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*5, 19, WINED3DTSS_MIPMAPLODBIAS             */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*5, 20, WINED3DTSS_MAXMIPLEVEL               */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*5, 21, WINED3DTSS_MAXANISOTROPY             */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*5, 22, WINED3DTSS_BUMPENVLSCALE             */      STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlscale   },
    { /*5, 23, WINED3DTSS_BUMPENVLOFFSET            */      STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVLOFFSET),   tex_bumpenvloffset  },
    { /*5, 24, WINED3DTSS_TEXTURETRANSFORMFLAGS     */      STATE_TRANSFORM(WINED3DTS_TEXTURE5),                transform_texture   },
    { /*5, 25, WINED3DTSS_ADDRESSW                  */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*5, 26, WINED3DTSS_COLORARG0                 */      STATE_TEXTURESTAGE(5, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*5, 27, WINED3DTSS_ALPHAARG0                 */      STATE_TEXTURESTAGE(5, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*5, 28, WINED3DTSS_RESULTARG                 */      STATE_TEXTURESTAGE(5, WINED3DTSS_RESULTARG),        tex_resultarg       },
    { /*5, 29, undefined                            */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*5, 30, undefined                            */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*5, 31, undefined                            */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*5, 32, WINED3DTSS_CONSTANT                  */      0 /* As long as we don't support D3DTA_CONSTANT */, state_nogl          },

    { /*6, 01, WINED3DTSS_COLOROP                   */      STATE_TEXTURESTAGE(6, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*6, 02, WINED3DTSS_COLORARG1                 */      STATE_TEXTURESTAGE(6, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*6, 03, WINED3DTSS_COLORARG2                 */      STATE_TEXTURESTAGE(6, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*6, 04, WINED3DTSS_ALPHAOP                   */      STATE_TEXTURESTAGE(6, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*6, 05, WINED3DTSS_ALPHAARG1                 */      STATE_TEXTURESTAGE(6, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*6, 06, WINED3DTSS_ALPHAARG2                 */      STATE_TEXTURESTAGE(6, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*6, 07, WINED3DTSS_BUMPENVMAT00              */      STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*6, 08, WINED3DTSS_BUMPENVMAT01              */      STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*6, 09, WINED3DTSS_BUMPENVMAT10              */      STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*6, 10, WINED3DTSS_BUMPENVMAT11              */      STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*6, 11, WINED3DTSS_TEXCOORDINDEX             */      STATE_TEXTURESTAGE(6, WINED3DTSS_TEXCOORDINDEX),    tex_coordindex      },
    { /*6, 12, WINED3DTSS_ADDRESS                   */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*6, 13, WINED3DTSS_ADDRESSU                  */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*6, 14, WINED3DTSS_ADDRESSV                  */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*6, 15, WINED3DTSS_BORDERCOLOR               */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*6, 16, WINED3DTSS_MAGFILTER                 */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*6, 17, WINED3DTSS_MINFILTER                 */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*6, 18, WINED3DTSS_MIPFILTER                 */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*6, 19, WINED3DTSS_MIPMAPLODBIAS             */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*6, 20, WINED3DTSS_MAXMIPLEVEL               */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*6, 21, WINED3DTSS_MAXANISOTROPY             */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*6, 22, WINED3DTSS_BUMPENVLSCALE             */      STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlscale   },
    { /*6, 23, WINED3DTSS_BUMPENVLOFFSET            */      STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVLOFFSET),   tex_bumpenvloffset  },
    { /*6, 24, WINED3DTSS_TEXTURETRANSFORMFLAGS     */      STATE_TRANSFORM(WINED3DTS_TEXTURE6),                transform_texture   },
    { /*6, 25, WINED3DTSS_ADDRESSW                  */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*6, 26, WINED3DTSS_COLORARG0                 */      STATE_TEXTURESTAGE(6, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*6, 27, WINED3DTSS_ALPHAARG0                 */      STATE_TEXTURESTAGE(6, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*6, 28, WINED3DTSS_RESULTARG                 */      STATE_TEXTURESTAGE(6, WINED3DTSS_RESULTARG),        tex_resultarg       },
    { /*6, 29, undefined                            */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*6, 30, undefined                            */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*6, 31, undefined                            */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*6, 32, WINED3DTSS_CONSTANT                  */      0 /* As long as we don't support D3DTA_CONSTANT */, state_nogl          },

    { /*7, 01, WINED3DTSS_COLOROP                   */      STATE_TEXTURESTAGE(7, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*7, 02, WINED3DTSS_COLORARG1                 */      STATE_TEXTURESTAGE(7, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*7, 03, WINED3DTSS_COLORARG2                 */      STATE_TEXTURESTAGE(7, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*7, 04, WINED3DTSS_ALPHAOP                   */      STATE_TEXTURESTAGE(7, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*7, 05, WINED3DTSS_ALPHAARG1                 */      STATE_TEXTURESTAGE(7, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*7, 06, WINED3DTSS_ALPHAARG2                 */      STATE_TEXTURESTAGE(7, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*7, 07, WINED3DTSS_BUMPENVMAT00              */      STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*7, 08, WINED3DTSS_BUMPENVMAT01              */      STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*7, 09, WINED3DTSS_BUMPENVMAT10              */      STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*7, 10, WINED3DTSS_BUMPENVMAT11              */      STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVMAT00),     tex_bumpenvmat      },
    { /*7, 11, WINED3DTSS_TEXCOORDINDEX             */      STATE_TEXTURESTAGE(7, WINED3DTSS_TEXCOORDINDEX),    tex_coordindex      },
    { /*7, 12, WINED3DTSS_ADDRESS                   */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*7, 13, WINED3DTSS_ADDRESSU                  */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*7, 14, WINED3DTSS_ADDRESSV                  */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*7, 15, WINED3DTSS_BORDERCOLOR               */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*7, 16, WINED3DTSS_MAGFILTER                 */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*7, 17, WINED3DTSS_MINFILTER                 */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*7, 18, WINED3DTSS_MIPFILTER                 */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*7, 19, WINED3DTSS_MIPMAPLODBIAS             */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*7, 20, WINED3DTSS_MAXMIPLEVEL               */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*7, 21, WINED3DTSS_MAXANISOTROPY             */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*7, 22, WINED3DTSS_BUMPENVLSCALE             */      STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlscale   },
    { /*7, 23, WINED3DTSS_BUMPENVLOFFSET            */      STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVLOFFSET),   tex_bumpenvloffset  },
    { /*7, 24, WINED3DTSS_TEXTURETRANSFORMFLAGS     */      STATE_TRANSFORM(WINED3DTS_TEXTURE7),                transform_texture   },
    { /*7, 25, WINED3DTSS_ADDRESSW                  */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*7, 26, WINED3DTSS_COLORARG0                 */      STATE_TEXTURESTAGE(7, WINED3DTSS_COLOROP),          tex_colorop         },
    { /*7, 27, WINED3DTSS_ALPHAARG0                 */      STATE_TEXTURESTAGE(7, WINED3DTSS_ALPHAOP),          tex_alphaop         },
    { /*7, 28, WINED3DTSS_RESULTARG                 */      STATE_TEXTURESTAGE(7, WINED3DTSS_RESULTARG),        tex_resultarg       },
    { /*7, 29, undefined                            */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*7, 30, undefined                            */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*7, 31, undefined                            */      0 /* -> sampler state in ddraw / d3d8 */,           state_undefined     },
    { /*7, 32, WINED3DTSS_CONSTANT                  */      0 /* As long as we don't support D3DTA_CONSTANT */, state_nogl          },
    /* Sampler states */
    { /* 0, Sampler 0                               */      STATE_SAMPLER(0),                                   sampler             },
    { /* 1, Sampler 1                               */      STATE_SAMPLER(1),                                   sampler             },
    { /* 2, Sampler 2                               */      STATE_SAMPLER(2),                                   sampler             },
    { /* 3, Sampler 3                               */      STATE_SAMPLER(3),                                   sampler             },
    { /* 4, Sampler 3                               */      STATE_SAMPLER(4),                                   sampler             },
    { /* 5, Sampler 5                               */      STATE_SAMPLER(5),                                   sampler             },
    { /* 6, Sampler 6                               */      STATE_SAMPLER(6),                                   sampler             },
    { /* 7, Sampler 7                               */      STATE_SAMPLER(7),                                   sampler             },
    { /* 8, Sampler 8                               */      STATE_SAMPLER(8),                                   sampler             },
    { /* 9, Sampler 9                               */      STATE_SAMPLER(9),                                   sampler             },
    { /*10, Sampler 10                              */      STATE_SAMPLER(10),                                  sampler             },
    { /*11, Sampler 11                              */      STATE_SAMPLER(11),                                  sampler             },
    { /*12, Sampler 12                              */      STATE_SAMPLER(12),                                  sampler             },
    { /*13, Sampler 13                              */      STATE_SAMPLER(13),                                  sampler             },
    { /*14, Sampler 14                              */      STATE_SAMPLER(14),                                  sampler             },
    { /*15, Sampler 15                              */      STATE_SAMPLER(15),                                  sampler             },
    { /*16, Vertex sampler 0                        */      STATE_SAMPLER(16),                                  sampler             },
    { /*17, Vertex sampler 1                        */      STATE_SAMPLER(17),                                  sampler             },
    { /*18, Vertex sampler 2                        */      STATE_SAMPLER(18),                                  sampler             },
    { /*19, Vertex sampler 3                        */      STATE_SAMPLER(19),                                  sampler             },
    /* Pixel shader */
    { /*  , Pixel Shader                            */      STATE_PIXELSHADER,                                  pixelshader         },
      /* Transform states follow                    */
    { /*  1, undefined                              */      0,                                                  state_undefined     },
    { /*  2, WINED3DTS_VIEW                         */      STATE_TRANSFORM(WINED3DTS_VIEW),                    transform_view      },
    { /*  3, WINED3DTS_PROJECTION                   */      STATE_TRANSFORM(WINED3DTS_PROJECTION),              transform_projection},
    { /*  4, undefined                              */      0,                                                  state_undefined     },
    { /*  5, undefined                              */      0,                                                  state_undefined     },
    { /*  6, undefined                              */      0,                                                  state_undefined     },
    { /*  7, undefined                              */      0,                                                  state_undefined     },
    { /*  8, undefined                              */      0,                                                  state_undefined     },
    { /*  9, undefined                              */      0,                                                  state_undefined     },
    { /* 10, undefined                              */      0,                                                  state_undefined     },
    { /* 11, undefined                              */      0,                                                  state_undefined     },
    { /* 12, undefined                              */      0,                                                  state_undefined     },
    { /* 13, undefined                              */      0,                                                  state_undefined     },
    { /* 14, undefined                              */      0,                                                  state_undefined     },
    { /* 15, undefined                              */      0,                                                  state_undefined     },
    { /* 16, WINED3DTS_TEXTURE0                     */      STATE_TRANSFORM(WINED3DTS_TEXTURE0),                transform_texture   },
    { /* 17, WINED3DTS_TEXTURE1                     */      STATE_TRANSFORM(WINED3DTS_TEXTURE1),                transform_texture   },
    { /* 18, WINED3DTS_TEXTURE2                     */      STATE_TRANSFORM(WINED3DTS_TEXTURE2),                transform_texture   },
    { /* 19, WINED3DTS_TEXTURE3                     */      STATE_TRANSFORM(WINED3DTS_TEXTURE3),                transform_texture   },
    { /* 20, WINED3DTS_TEXTURE4                     */      STATE_TRANSFORM(WINED3DTS_TEXTURE4),                transform_texture   },
    { /* 21, WINED3DTS_TEXTURE5                     */      STATE_TRANSFORM(WINED3DTS_TEXTURE5),                transform_texture   },
    { /* 22, WINED3DTS_TEXTURE6                     */      STATE_TRANSFORM(WINED3DTS_TEXTURE6),                transform_texture   },
    { /* 23, WINED3DTS_TEXTURE7                     */      STATE_TRANSFORM(WINED3DTS_TEXTURE7),                transform_texture   },
      /* A huge gap between TEXTURE7 and WORLDMATRIX(0) :-( But entries are needed to catch then if a broken app sets them */
    { /* 24, undefined                              */      0,                                                  state_undefined     },
    { /* 25, undefined                              */      0,                                                  state_undefined     },
    { /* 26, undefined                              */      0,                                                  state_undefined     },
    { /* 27, undefined                              */      0,                                                  state_undefined     },
    { /* 28, undefined                              */      0,                                                  state_undefined     },
    { /* 29, undefined                              */      0,                                                  state_undefined     },
    { /* 30, undefined                              */      0,                                                  state_undefined     },
    { /* 31, undefined                              */      0,                                                  state_undefined     },
    { /* 32, undefined                              */      0,                                                  state_undefined     },
    { /* 33, undefined                              */      0,                                                  state_undefined     },
    { /* 34, undefined                              */      0,                                                  state_undefined     },
    { /* 35, undefined                              */      0,                                                  state_undefined     },
    { /* 36, undefined                              */      0,                                                  state_undefined     },
    { /* 37, undefined                              */      0,                                                  state_undefined     },
    { /* 38, undefined                              */      0,                                                  state_undefined     },
    { /* 39, undefined                              */      0,                                                  state_undefined     },
    { /* 40, undefined                              */      0,                                                  state_undefined     },
    { /* 41, undefined                              */      0,                                                  state_undefined     },
    { /* 42, undefined                              */      0,                                                  state_undefined     },
    { /* 43, undefined                              */      0,                                                  state_undefined     },
    { /* 44, undefined                              */      0,                                                  state_undefined     },
    { /* 45, undefined                              */      0,                                                  state_undefined     },
    { /* 46, undefined                              */      0,                                                  state_undefined     },
    { /* 47, undefined                              */      0,                                                  state_undefined     },
    { /* 48, undefined                              */      0,                                                  state_undefined     },
    { /* 49, undefined                              */      0,                                                  state_undefined     },
    { /* 50, undefined                              */      0,                                                  state_undefined     },
    { /* 51, undefined                              */      0,                                                  state_undefined     },
    { /* 52, undefined                              */      0,                                                  state_undefined     },
    { /* 53, undefined                              */      0,                                                  state_undefined     },
    { /* 54, undefined                              */      0,                                                  state_undefined     },
    { /* 55, undefined                              */      0,                                                  state_undefined     },
    { /* 56, undefined                              */      0,                                                  state_undefined     },
    { /* 57, undefined                              */      0,                                                  state_undefined     },
    { /* 58, undefined                              */      0,                                                  state_undefined     },
    { /* 59, undefined                              */      0,                                                  state_undefined     },
    { /* 60, undefined                              */      0,                                                  state_undefined     },
    { /* 61, undefined                              */      0,                                                  state_undefined     },
    { /* 62, undefined                              */      0,                                                  state_undefined     },
    { /* 63, undefined                              */      0,                                                  state_undefined     },
    { /* 64, undefined                              */      0,                                                  state_undefined     },
    { /* 65, undefined                              */      0,                                                  state_undefined     },
    { /* 66, undefined                              */      0,                                                  state_undefined     },
    { /* 67, undefined                              */      0,                                                  state_undefined     },
    { /* 68, undefined                              */      0,                                                  state_undefined     },
    { /* 69, undefined                              */      0,                                                  state_undefined     },
    { /* 70, undefined                              */      0,                                                  state_undefined     },
    { /* 71, undefined                              */      0,                                                  state_undefined     },
    { /* 72, undefined                              */      0,                                                  state_undefined     },
    { /* 73, undefined                              */      0,                                                  state_undefined     },
    { /* 74, undefined                              */      0,                                                  state_undefined     },
    { /* 75, undefined                              */      0,                                                  state_undefined     },
    { /* 76, undefined                              */      0,                                                  state_undefined     },
    { /* 77, undefined                              */      0,                                                  state_undefined     },
    { /* 78, undefined                              */      0,                                                  state_undefined     },
    { /* 79, undefined                              */      0,                                                  state_undefined     },
    { /* 80, undefined                              */      0,                                                  state_undefined     },
    { /* 81, undefined                              */      0,                                                  state_undefined     },
    { /* 82, undefined                              */      0,                                                  state_undefined     },
    { /* 83, undefined                              */      0,                                                  state_undefined     },
    { /* 84, undefined                              */      0,                                                  state_undefined     },
    { /* 85, undefined                              */      0,                                                  state_undefined     },
    { /* 86, undefined                              */      0,                                                  state_undefined     },
    { /* 87, undefined                              */      0,                                                  state_undefined     },
    { /* 88, undefined                              */      0,                                                  state_undefined     },
    { /* 89, undefined                              */      0,                                                  state_undefined     },
    { /* 90, undefined                              */      0,                                                  state_undefined     },
    { /* 91, undefined                              */      0,                                                  state_undefined     },
    { /* 92, undefined                              */      0,                                                  state_undefined     },
    { /* 93, undefined                              */      0,                                                  state_undefined     },
    { /* 94, undefined                              */      0,                                                  state_undefined     },
    { /* 95, undefined                              */      0,                                                  state_undefined     },
    { /* 96, undefined                              */      0,                                                  state_undefined     },
    { /* 97, undefined                              */      0,                                                  state_undefined     },
    { /* 98, undefined                              */      0,                                                  state_undefined     },
    { /* 99, undefined                              */      0,                                                  state_undefined     },
    { /*100, undefined                              */      0,                                                  state_undefined     },
    { /*101, undefined                              */      0,                                                  state_undefined     },
    { /*102, undefined                              */      0,                                                  state_undefined     },
    { /*103, undefined                              */      0,                                                  state_undefined     },
    { /*104, undefined                              */      0,                                                  state_undefined     },
    { /*105, undefined                              */      0,                                                  state_undefined     },
    { /*106, undefined                              */      0,                                                  state_undefined     },
    { /*107, undefined                              */      0,                                                  state_undefined     },
    { /*108, undefined                              */      0,                                                  state_undefined     },
    { /*109, undefined                              */      0,                                                  state_undefined     },
    { /*110, undefined                              */      0,                                                  state_undefined     },
    { /*111, undefined                              */      0,                                                  state_undefined     },
    { /*112, undefined                              */      0,                                                  state_undefined     },
    { /*113, undefined                              */      0,                                                  state_undefined     },
    { /*114, undefined                              */      0,                                                  state_undefined     },
    { /*115, undefined                              */      0,                                                  state_undefined     },
    { /*116, undefined                              */      0,                                                  state_undefined     },
    { /*117, undefined                              */      0,                                                  state_undefined     },
    { /*118, undefined                              */      0,                                                  state_undefined     },
    { /*119, undefined                              */      0,                                                  state_undefined     },
    { /*120, undefined                              */      0,                                                  state_undefined     },
    { /*121, undefined                              */      0,                                                  state_undefined     },
    { /*122, undefined                              */      0,                                                  state_undefined     },
    { /*123, undefined                              */      0,                                                  state_undefined     },
    { /*124, undefined                              */      0,                                                  state_undefined     },
    { /*125, undefined                              */      0,                                                  state_undefined     },
    { /*126, undefined                              */      0,                                                  state_undefined     },
    { /*127, undefined                              */      0,                                                  state_undefined     },
    { /*128, undefined                              */      0,                                                  state_undefined     },
    { /*129, undefined                              */      0,                                                  state_undefined     },
    { /*130, undefined                              */      0,                                                  state_undefined     },
    { /*131, undefined                              */      0,                                                  state_undefined     },
    { /*132, undefined                              */      0,                                                  state_undefined     },
    { /*133, undefined                              */      0,                                                  state_undefined     },
    { /*134, undefined                              */      0,                                                  state_undefined     },
    { /*135, undefined                              */      0,                                                  state_undefined     },
    { /*136, undefined                              */      0,                                                  state_undefined     },
    { /*137, undefined                              */      0,                                                  state_undefined     },
    { /*138, undefined                              */      0,                                                  state_undefined     },
    { /*139, undefined                              */      0,                                                  state_undefined     },
    { /*140, undefined                              */      0,                                                  state_undefined     },
    { /*141, undefined                              */      0,                                                  state_undefined     },
    { /*142, undefined                              */      0,                                                  state_undefined     },
    { /*143, undefined                              */      0,                                                  state_undefined     },
    { /*144, undefined                              */      0,                                                  state_undefined     },
    { /*145, undefined                              */      0,                                                  state_undefined     },
    { /*146, undefined                              */      0,                                                  state_undefined     },
    { /*147, undefined                              */      0,                                                  state_undefined     },
    { /*148, undefined                              */      0,                                                  state_undefined     },
    { /*149, undefined                              */      0,                                                  state_undefined     },
    { /*150, undefined                              */      0,                                                  state_undefined     },
    { /*151, undefined                              */      0,                                                  state_undefined     },
    { /*152, undefined                              */      0,                                                  state_undefined     },
    { /*153, undefined                              */      0,                                                  state_undefined     },
    { /*154, undefined                              */      0,                                                  state_undefined     },
    { /*155, undefined                              */      0,                                                  state_undefined     },
    { /*156, undefined                              */      0,                                                  state_undefined     },
    { /*157, undefined                              */      0,                                                  state_undefined     },
    { /*158, undefined                              */      0,                                                  state_undefined     },
    { /*159, undefined                              */      0,                                                  state_undefined     },
    { /*160, undefined                              */      0,                                                  state_undefined     },
    { /*161, undefined                              */      0,                                                  state_undefined     },
    { /*162, undefined                              */      0,                                                  state_undefined     },
    { /*163, undefined                              */      0,                                                  state_undefined     },
    { /*164, undefined                              */      0,                                                  state_undefined     },
    { /*165, undefined                              */      0,                                                  state_undefined     },
    { /*166, undefined                              */      0,                                                  state_undefined     },
    { /*167, undefined                              */      0,                                                  state_undefined     },
    { /*168, undefined                              */      0,                                                  state_undefined     },
    { /*169, undefined                              */      0,                                                  state_undefined     },
    { /*170, undefined                              */      0,                                                  state_undefined     },
    { /*171, undefined                              */      0,                                                  state_undefined     },
    { /*172, undefined                              */      0,                                                  state_undefined     },
    { /*173, undefined                              */      0,                                                  state_undefined     },
    { /*174, undefined                              */      0,                                                  state_undefined     },
    { /*175, undefined                              */      0,                                                  state_undefined     },
    { /*176, undefined                              */      0,                                                  state_undefined     },
    { /*177, undefined                              */      0,                                                  state_undefined     },
    { /*178, undefined                              */      0,                                                  state_undefined     },
    { /*179, undefined                              */      0,                                                  state_undefined     },
    { /*180, undefined                              */      0,                                                  state_undefined     },
    { /*181, undefined                              */      0,                                                  state_undefined     },
    { /*182, undefined                              */      0,                                                  state_undefined     },
    { /*183, undefined                              */      0,                                                  state_undefined     },
    { /*184, undefined                              */      0,                                                  state_undefined     },
    { /*185, undefined                              */      0,                                                  state_undefined     },
    { /*186, undefined                              */      0,                                                  state_undefined     },
    { /*187, undefined                              */      0,                                                  state_undefined     },
    { /*188, undefined                              */      0,                                                  state_undefined     },
    { /*189, undefined                              */      0,                                                  state_undefined     },
    { /*190, undefined                              */      0,                                                  state_undefined     },
    { /*191, undefined                              */      0,                                                  state_undefined     },
    { /*192, undefined                              */      0,                                                  state_undefined     },
    { /*193, undefined                              */      0,                                                  state_undefined     },
    { /*194, undefined                              */      0,                                                  state_undefined     },
    { /*195, undefined                              */      0,                                                  state_undefined     },
    { /*196, undefined                              */      0,                                                  state_undefined     },
    { /*197, undefined                              */      0,                                                  state_undefined     },
    { /*198, undefined                              */      0,                                                  state_undefined     },
    { /*199, undefined                              */      0,                                                  state_undefined     },
    { /*200, undefined                              */      0,                                                  state_undefined     },
    { /*201, undefined                              */      0,                                                  state_undefined     },
    { /*202, undefined                              */      0,                                                  state_undefined     },
    { /*203, undefined                              */      0,                                                  state_undefined     },
    { /*204, undefined                              */      0,                                                  state_undefined     },
    { /*205, undefined                              */      0,                                                  state_undefined     },
    { /*206, undefined                              */      0,                                                  state_undefined     },
    { /*207, undefined                              */      0,                                                  state_undefined     },
    { /*208, undefined                              */      0,                                                  state_undefined     },
    { /*209, undefined                              */      0,                                                  state_undefined     },
    { /*210, undefined                              */      0,                                                  state_undefined     },
    { /*211, undefined                              */      0,                                                  state_undefined     },
    { /*212, undefined                              */      0,                                                  state_undefined     },
    { /*213, undefined                              */      0,                                                  state_undefined     },
    { /*214, undefined                              */      0,                                                  state_undefined     },
    { /*215, undefined                              */      0,                                                  state_undefined     },
    { /*216, undefined                              */      0,                                                  state_undefined     },
    { /*217, undefined                              */      0,                                                  state_undefined     },
    { /*218, undefined                              */      0,                                                  state_undefined     },
    { /*219, undefined                              */      0,                                                  state_undefined     },
    { /*220, undefined                              */      0,                                                  state_undefined     },
    { /*221, undefined                              */      0,                                                  state_undefined     },
    { /*222, undefined                              */      0,                                                  state_undefined     },
    { /*223, undefined                              */      0,                                                  state_undefined     },
    { /*224, undefined                              */      0,                                                  state_undefined     },
    { /*225, undefined                              */      0,                                                  state_undefined     },
    { /*226, undefined                              */      0,                                                  state_undefined     },
    { /*227, undefined                              */      0,                                                  state_undefined     },
    { /*228, undefined                              */      0,                                                  state_undefined     },
    { /*229, undefined                              */      0,                                                  state_undefined     },
    { /*230, undefined                              */      0,                                                  state_undefined     },
    { /*231, undefined                              */      0,                                                  state_undefined     },
    { /*232, undefined                              */      0,                                                  state_undefined     },
    { /*233, undefined                              */      0,                                                  state_undefined     },
    { /*234, undefined                              */      0,                                                  state_undefined     },
    { /*235, undefined                              */      0,                                                  state_undefined     },
    { /*236, undefined                              */      0,                                                  state_undefined     },
    { /*237, undefined                              */      0,                                                  state_undefined     },
    { /*238, undefined                              */      0,                                                  state_undefined     },
    { /*239, undefined                              */      0,                                                  state_undefined     },
    { /*240, undefined                              */      0,                                                  state_undefined     },
    { /*241, undefined                              */      0,                                                  state_undefined     },
    { /*242, undefined                              */      0,                                                  state_undefined     },
    { /*243, undefined                              */      0,                                                  state_undefined     },
    { /*244, undefined                              */      0,                                                  state_undefined     },
    { /*245, undefined                              */      0,                                                  state_undefined     },
    { /*246, undefined                              */      0,                                                  state_undefined     },
    { /*247, undefined                              */      0,                                                  state_undefined     },
    { /*248, undefined                              */      0,                                                  state_undefined     },
    { /*249, undefined                              */      0,                                                  state_undefined     },
    { /*250, undefined                              */      0,                                                  state_undefined     },
    { /*251, undefined                              */      0,                                                  state_undefined     },
    { /*252, undefined                              */      0,                                                  state_undefined     },
    { /*253, undefined                              */      0,                                                  state_undefined     },
    { /*254, undefined                              */      0,                                                  state_undefined     },
    { /*255, undefined                              */      0,                                                  state_undefined     },
      /* End huge gap */
    { /*256, WINED3DTS_WORLDMATRIX(0)               */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(0)),          transform_world     },
    { /*257, WINED3DTS_WORLDMATRIX(1)               */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(1)),          transform_worldex   },
    { /*258, WINED3DTS_WORLDMATRIX(2)               */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(2)),          transform_worldex   },
    { /*259, WINED3DTS_WORLDMATRIX(3)               */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(3)),          transform_worldex   },
    { /*260, WINED3DTS_WORLDMATRIX(4)               */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(4)),          transform_worldex   },
    { /*261, WINED3DTS_WORLDMATRIX(5)               */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(5)),          transform_worldex   },
    { /*262, WINED3DTS_WORLDMATRIX(6)               */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(6)),          transform_worldex   },
    { /*263, WINED3DTS_WORLDMATRIX(7)               */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(7)),          transform_worldex   },
    { /*264, WINED3DTS_WORLDMATRIX(8)               */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(8)),          transform_worldex   },
    { /*265, WINED3DTS_WORLDMATRIX(9)               */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(9)),          transform_worldex   },
    { /*266, WINED3DTS_WORLDMATRIX(10)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(10)),         transform_worldex   },
    { /*267, WINED3DTS_WORLDMATRIX(11)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(11)),         transform_worldex   },
    { /*268, WINED3DTS_WORLDMATRIX(12)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(12)),         transform_worldex   },
    { /*269, WINED3DTS_WORLDMATRIX(13)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(13)),         transform_worldex   },
    { /*270, WINED3DTS_WORLDMATRIX(14)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(14)),         transform_worldex   },
    { /*271, WINED3DTS_WORLDMATRIX(15)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(15)),         transform_worldex   },
    { /*272, WINED3DTS_WORLDMATRIX(16)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(16)),         transform_worldex   },
    { /*273, WINED3DTS_WORLDMATRIX(17)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(17)),         transform_worldex   },
    { /*274, WINED3DTS_WORLDMATRIX(18)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(18)),         transform_worldex   },
    { /*275, WINED3DTS_WORLDMATRIX(19)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(19)),         transform_worldex   },
    { /*276, WINED3DTS_WORLDMATRIX(20)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(20)),         transform_worldex   },
    { /*277, WINED3DTS_WORLDMATRIX(21)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(21)),         transform_worldex   },
    { /*278, WINED3DTS_WORLDMATRIX(22)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(22)),         transform_worldex   },
    { /*279, WINED3DTS_WORLDMATRIX(23)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(23)),         transform_worldex   },
    { /*280, WINED3DTS_WORLDMATRIX(24)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(24)),         transform_worldex   },
    { /*281, WINED3DTS_WORLDMATRIX(25)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(25)),         transform_worldex   },
    { /*282, WINED3DTS_WORLDMATRIX(26)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(26)),         transform_worldex   },
    { /*283, WINED3DTS_WORLDMATRIX(27)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(27)),         transform_worldex   },
    { /*284, WINED3DTS_WORLDMATRIX(28)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(28)),         transform_worldex   },
    { /*285, WINED3DTS_WORLDMATRIX(29)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(29)),         transform_worldex   },
    { /*286, WINED3DTS_WORLDMATRIX(30)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(30)),         transform_worldex   },
    { /*287, WINED3DTS_WORLDMATRIX(31)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(31)),         transform_worldex   },
    { /*288, WINED3DTS_WORLDMATRIX(32)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(32)),         transform_worldex   },
    { /*289, WINED3DTS_WORLDMATRIX(33)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(33)),         transform_worldex   },
    { /*290, WINED3DTS_WORLDMATRIX(34)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(34)),         transform_worldex   },
    { /*291, WINED3DTS_WORLDMATRIX(35)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(35)),         transform_worldex   },
    { /*292, WINED3DTS_WORLDMATRIX(36)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(36)),         transform_worldex   },
    { /*293, WINED3DTS_WORLDMATRIX(37)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(37)),         transform_worldex   },
    { /*294, WINED3DTS_WORLDMATRIX(38)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(38)),         transform_worldex   },
    { /*295, WINED3DTS_WORLDMATRIX(39)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(39)),         transform_worldex   },
    { /*296, WINED3DTS_WORLDMATRIX(40)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(40)),         transform_worldex   },
    { /*297, WINED3DTS_WORLDMATRIX(41)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(41)),         transform_worldex   },
    { /*298, WINED3DTS_WORLDMATRIX(42)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(42)),         transform_worldex   },
    { /*299, WINED3DTS_WORLDMATRIX(43)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(43)),         transform_worldex   },
    { /*300, WINED3DTS_WORLDMATRIX(44)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(44)),         transform_worldex   },
    { /*301, WINED3DTS_WORLDMATRIX(45)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(45)),         transform_worldex   },
    { /*302, WINED3DTS_WORLDMATRIX(46)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(46)),         transform_worldex   },
    { /*303, WINED3DTS_WORLDMATRIX(47)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(47)),         transform_worldex   },
    { /*304, WINED3DTS_WORLDMATRIX(48)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(48)),         transform_worldex   },
    { /*305, WINED3DTS_WORLDMATRIX(49)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(49)),         transform_worldex   },
    { /*306, WINED3DTS_WORLDMATRIX(50)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(50)),         transform_worldex   },
    { /*307, WINED3DTS_WORLDMATRIX(51)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(51)),         transform_worldex   },
    { /*308, WINED3DTS_WORLDMATRIX(52)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(52)),         transform_worldex   },
    { /*309, WINED3DTS_WORLDMATRIX(53)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(53)),         transform_worldex   },
    { /*310, WINED3DTS_WORLDMATRIX(54)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(54)),         transform_worldex   },
    { /*311, WINED3DTS_WORLDMATRIX(55)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(55)),         transform_worldex   },
    { /*312, WINED3DTS_WORLDMATRIX(56)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(56)),         transform_worldex   },
    { /*313, WINED3DTS_WORLDMATRIX(57)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(57)),         transform_worldex   },
    { /*314, WINED3DTS_WORLDMATRIX(58)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(58)),         transform_worldex   },
    { /*315, WINED3DTS_WORLDMATRIX(59)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(59)),         transform_worldex   },
    { /*316, WINED3DTS_WORLDMATRIX(60)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(60)),         transform_worldex   },
    { /*317, WINED3DTS_WORLDMATRIX(61)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(61)),         transform_worldex   },
    { /*318, WINED3DTS_WORLDMATRIX(62)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(62)),         transform_worldex   },
    { /*319, WINED3DTS_WORLDMATRIX(63)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(63)),         transform_worldex   },
    { /*320, WINED3DTS_WORLDMATRIX(64)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(64)),         transform_worldex   },
    { /*321, WINED3DTS_WORLDMATRIX(65)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(65)),         transform_worldex   },
    { /*322, WINED3DTS_WORLDMATRIX(66)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(66)),         transform_worldex   },
    { /*323, WINED3DTS_WORLDMATRIX(67)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(67)),         transform_worldex   },
    { /*324, WINED3DTS_WORLDMATRIX(68)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(68)),         transform_worldex   },
    { /*325, WINED3DTS_WORLDMATRIX(68)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(69)),         transform_worldex   },
    { /*326, WINED3DTS_WORLDMATRIX(70)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(70)),         transform_worldex   },
    { /*327, WINED3DTS_WORLDMATRIX(71)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(71)),         transform_worldex   },
    { /*328, WINED3DTS_WORLDMATRIX(72)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(72)),         transform_worldex   },
    { /*329, WINED3DTS_WORLDMATRIX(73)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(73)),         transform_worldex   },
    { /*330, WINED3DTS_WORLDMATRIX(74)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(74)),         transform_worldex   },
    { /*331, WINED3DTS_WORLDMATRIX(75)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(75)),         transform_worldex   },
    { /*332, WINED3DTS_WORLDMATRIX(76)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(76)),         transform_worldex   },
    { /*333, WINED3DTS_WORLDMATRIX(77)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(77)),         transform_worldex   },
    { /*334, WINED3DTS_WORLDMATRIX(78)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(78)),         transform_worldex   },
    { /*335, WINED3DTS_WORLDMATRIX(79)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(79)),         transform_worldex   },
    { /*336, WINED3DTS_WORLDMATRIX(80)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(80)),         transform_worldex   },
    { /*337, WINED3DTS_WORLDMATRIX(81)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(81)),         transform_worldex   },
    { /*338, WINED3DTS_WORLDMATRIX(82)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(82)),         transform_worldex   },
    { /*339, WINED3DTS_WORLDMATRIX(83)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(83)),         transform_worldex   },
    { /*340, WINED3DTS_WORLDMATRIX(84)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(84)),         transform_worldex   },
    { /*341, WINED3DTS_WORLDMATRIX(85)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(85)),         transform_worldex   },
    { /*341, WINED3DTS_WORLDMATRIX(86)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(86)),         transform_worldex   },
    { /*343, WINED3DTS_WORLDMATRIX(87)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(87)),         transform_worldex   },
    { /*344, WINED3DTS_WORLDMATRIX(88)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(88)),         transform_worldex   },
    { /*345, WINED3DTS_WORLDMATRIX(89)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(89)),         transform_worldex   },
    { /*346, WINED3DTS_WORLDMATRIX(90)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(90)),         transform_worldex   },
    { /*347, WINED3DTS_WORLDMATRIX(91)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(91)),         transform_worldex   },
    { /*348, WINED3DTS_WORLDMATRIX(92)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(92)),         transform_worldex   },
    { /*349, WINED3DTS_WORLDMATRIX(93)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(93)),         transform_worldex   },
    { /*350, WINED3DTS_WORLDMATRIX(94)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(94)),         transform_worldex   },
    { /*351, WINED3DTS_WORLDMATRIX(95)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(95)),         transform_worldex   },
    { /*352, WINED3DTS_WORLDMATRIX(96)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(96)),         transform_worldex   },
    { /*353, WINED3DTS_WORLDMATRIX(97)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(97)),         transform_worldex   },
    { /*354, WINED3DTS_WORLDMATRIX(98)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(98)),         transform_worldex   },
    { /*355, WINED3DTS_WORLDMATRIX(99)              */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(99)),         transform_worldex   },
    { /*356, WINED3DTS_WORLDMATRIX(100)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(100)),        transform_worldex   },
    { /*357, WINED3DTS_WORLDMATRIX(101)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(101)),        transform_worldex   },
    { /*358, WINED3DTS_WORLDMATRIX(102)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(102)),        transform_worldex   },
    { /*359, WINED3DTS_WORLDMATRIX(103)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(103)),        transform_worldex   },
    { /*360, WINED3DTS_WORLDMATRIX(104)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(104)),        transform_worldex   },
    { /*361, WINED3DTS_WORLDMATRIX(105)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(105)),        transform_worldex   },
    { /*362, WINED3DTS_WORLDMATRIX(106)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(106)),        transform_worldex   },
    { /*363, WINED3DTS_WORLDMATRIX(107)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(107)),        transform_worldex   },
    { /*364, WINED3DTS_WORLDMATRIX(108)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(108)),        transform_worldex   },
    { /*365, WINED3DTS_WORLDMATRIX(109)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(109)),        transform_worldex   },
    { /*366, WINED3DTS_WORLDMATRIX(110)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(110)),        transform_worldex   },
    { /*367, WINED3DTS_WORLDMATRIX(111)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(111)),        transform_worldex   },
    { /*368, WINED3DTS_WORLDMATRIX(112)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(112)),        transform_worldex   },
    { /*369, WINED3DTS_WORLDMATRIX(113)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(113)),        transform_worldex   },
    { /*370, WINED3DTS_WORLDMATRIX(114)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(114)),        transform_worldex   },
    { /*371, WINED3DTS_WORLDMATRIX(115)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(115)),        transform_worldex   },
    { /*372, WINED3DTS_WORLDMATRIX(116)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(116)),        transform_worldex   },
    { /*373, WINED3DTS_WORLDMATRIX(117)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(117)),        transform_worldex   },
    { /*374, WINED3DTS_WORLDMATRIX(118)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(118)),        transform_worldex   },
    { /*375, WINED3DTS_WORLDMATRIX(119)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(119)),        transform_worldex   },
    { /*376, WINED3DTS_WORLDMATRIX(120)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(120)),        transform_worldex   },
    { /*377, WINED3DTS_WORLDMATRIX(121)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(121)),        transform_worldex   },
    { /*378, WINED3DTS_WORLDMATRIX(122)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(122)),        transform_worldex   },
    { /*379, WINED3DTS_WORLDMATRIX(123)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(123)),        transform_worldex   },
    { /*380, WINED3DTS_WORLDMATRIX(124)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(124)),        transform_worldex   },
    { /*381, WINED3DTS_WORLDMATRIX(125)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(125)),        transform_worldex   },
    { /*382, WINED3DTS_WORLDMATRIX(126)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(126)),        transform_worldex   },
    { /*383, WINED3DTS_WORLDMATRIX(127)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(127)),        transform_worldex   },
    { /*384, WINED3DTS_WORLDMATRIX(128)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(128)),        transform_worldex   },
    { /*385, WINED3DTS_WORLDMATRIX(129)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(129)),        transform_worldex   },
    { /*386, WINED3DTS_WORLDMATRIX(130)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(130)),        transform_worldex   },
    { /*387, WINED3DTS_WORLDMATRIX(131)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(131)),        transform_worldex   },
    { /*388, WINED3DTS_WORLDMATRIX(132)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(132)),        transform_worldex   },
    { /*389, WINED3DTS_WORLDMATRIX(133)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(133)),        transform_worldex   },
    { /*390, WINED3DTS_WORLDMATRIX(134)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(134)),        transform_worldex   },
    { /*391, WINED3DTS_WORLDMATRIX(135)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(135)),        transform_worldex   },
    { /*392, WINED3DTS_WORLDMATRIX(136)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(136)),        transform_worldex   },
    { /*393, WINED3DTS_WORLDMATRIX(137)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(137)),        transform_worldex   },
    { /*394, WINED3DTS_WORLDMATRIX(138)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(138)),        transform_worldex   },
    { /*395, WINED3DTS_WORLDMATRIX(139)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(139)),        transform_worldex   },
    { /*396, WINED3DTS_WORLDMATRIX(140)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(140)),        transform_worldex   },
    { /*397, WINED3DTS_WORLDMATRIX(141)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(141)),        transform_worldex   },
    { /*398, WINED3DTS_WORLDMATRIX(142)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(142)),        transform_worldex   },
    { /*399, WINED3DTS_WORLDMATRIX(143)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(143)),        transform_worldex   },
    { /*400, WINED3DTS_WORLDMATRIX(144)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(144)),        transform_worldex   },
    { /*401, WINED3DTS_WORLDMATRIX(145)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(145)),        transform_worldex   },
    { /*402, WINED3DTS_WORLDMATRIX(146)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(146)),        transform_worldex   },
    { /*403, WINED3DTS_WORLDMATRIX(147)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(147)),        transform_worldex   },
    { /*404, WINED3DTS_WORLDMATRIX(148)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(148)),        transform_worldex   },
    { /*405, WINED3DTS_WORLDMATRIX(149)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(149)),        transform_worldex   },
    { /*406, WINED3DTS_WORLDMATRIX(150)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(150)),        transform_worldex   },
    { /*407, WINED3DTS_WORLDMATRIX(151)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(151)),        transform_worldex   },
    { /*408, WINED3DTS_WORLDMATRIX(152)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(152)),        transform_worldex   },
    { /*409, WINED3DTS_WORLDMATRIX(153)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(153)),        transform_worldex   },
    { /*410, WINED3DTS_WORLDMATRIX(154)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(154)),        transform_worldex   },
    { /*411, WINED3DTS_WORLDMATRIX(155)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(155)),        transform_worldex   },
    { /*412, WINED3DTS_WORLDMATRIX(156)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(156)),        transform_worldex   },
    { /*413, WINED3DTS_WORLDMATRIX(157)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(157)),        transform_worldex   },
    { /*414, WINED3DTS_WORLDMATRIX(158)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(158)),        transform_worldex   },
    { /*415, WINED3DTS_WORLDMATRIX(159)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(159)),        transform_worldex   },
    { /*416, WINED3DTS_WORLDMATRIX(160)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(160)),        transform_worldex   },
    { /*417, WINED3DTS_WORLDMATRIX(161)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(161)),        transform_worldex   },
    { /*418, WINED3DTS_WORLDMATRIX(162)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(162)),        transform_worldex   },
    { /*419, WINED3DTS_WORLDMATRIX(163)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(163)),        transform_worldex   },
    { /*420, WINED3DTS_WORLDMATRIX(164)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(164)),        transform_worldex   },
    { /*421, WINED3DTS_WORLDMATRIX(165)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(165)),        transform_worldex   },
    { /*422, WINED3DTS_WORLDMATRIX(166)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(166)),        transform_worldex   },
    { /*423, WINED3DTS_WORLDMATRIX(167)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(167)),        transform_worldex   },
    { /*424, WINED3DTS_WORLDMATRIX(168)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(168)),        transform_worldex   },
    { /*425, WINED3DTS_WORLDMATRIX(168)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(169)),        transform_worldex   },
    { /*426, WINED3DTS_WORLDMATRIX(170)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(170)),        transform_worldex   },
    { /*427, WINED3DTS_WORLDMATRIX(171)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(171)),        transform_worldex   },
    { /*428, WINED3DTS_WORLDMATRIX(172)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(172)),        transform_worldex   },
    { /*429, WINED3DTS_WORLDMATRIX(173)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(173)),        transform_worldex   },
    { /*430, WINED3DTS_WORLDMATRIX(174)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(174)),        transform_worldex   },
    { /*431, WINED3DTS_WORLDMATRIX(175)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(175)),        transform_worldex   },
    { /*432, WINED3DTS_WORLDMATRIX(176)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(176)),        transform_worldex   },
    { /*433, WINED3DTS_WORLDMATRIX(177)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(177)),        transform_worldex   },
    { /*434, WINED3DTS_WORLDMATRIX(178)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(178)),        transform_worldex   },
    { /*435, WINED3DTS_WORLDMATRIX(179)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(179)),        transform_worldex   },
    { /*436, WINED3DTS_WORLDMATRIX(180)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(180)),        transform_worldex   },
    { /*437, WINED3DTS_WORLDMATRIX(181)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(181)),        transform_worldex   },
    { /*438, WINED3DTS_WORLDMATRIX(182)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(182)),        transform_worldex   },
    { /*439, WINED3DTS_WORLDMATRIX(183)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(183)),        transform_worldex   },
    { /*440, WINED3DTS_WORLDMATRIX(184)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(184)),        transform_worldex   },
    { /*441, WINED3DTS_WORLDMATRIX(185)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(185)),        transform_worldex   },
    { /*441, WINED3DTS_WORLDMATRIX(186)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(186)),        transform_worldex   },
    { /*443, WINED3DTS_WORLDMATRIX(187)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(187)),        transform_worldex   },
    { /*444, WINED3DTS_WORLDMATRIX(188)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(188)),        transform_worldex   },
    { /*445, WINED3DTS_WORLDMATRIX(189)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(189)),        transform_worldex   },
    { /*446, WINED3DTS_WORLDMATRIX(190)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(190)),        transform_worldex   },
    { /*447, WINED3DTS_WORLDMATRIX(191)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(191)),        transform_worldex   },
    { /*448, WINED3DTS_WORLDMATRIX(192)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(192)),        transform_worldex   },
    { /*449, WINED3DTS_WORLDMATRIX(193)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(193)),        transform_worldex   },
    { /*450, WINED3DTS_WORLDMATRIX(194)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(194)),        transform_worldex   },
    { /*451, WINED3DTS_WORLDMATRIX(195)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(195)),        transform_worldex   },
    { /*452, WINED3DTS_WORLDMATRIX(196)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(196)),        transform_worldex   },
    { /*453, WINED3DTS_WORLDMATRIX(197)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(197)),        transform_worldex   },
    { /*454, WINED3DTS_WORLDMATRIX(198)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(198)),        transform_worldex   },
    { /*455, WINED3DTS_WORLDMATRIX(199)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(199)),        transform_worldex   },
    { /*356, WINED3DTS_WORLDMATRIX(200)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(200)),        transform_worldex   },
    { /*457, WINED3DTS_WORLDMATRIX(201)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(201)),        transform_worldex   },
    { /*458, WINED3DTS_WORLDMATRIX(202)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(202)),        transform_worldex   },
    { /*459, WINED3DTS_WORLDMATRIX(203)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(203)),        transform_worldex   },
    { /*460, WINED3DTS_WORLDMATRIX(204)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(204)),        transform_worldex   },
    { /*461, WINED3DTS_WORLDMATRIX(205)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(205)),        transform_worldex   },
    { /*462, WINED3DTS_WORLDMATRIX(206)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(206)),        transform_worldex   },
    { /*463, WINED3DTS_WORLDMATRIX(207)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(207)),        transform_worldex   },
    { /*464, WINED3DTS_WORLDMATRIX(208)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(208)),        transform_worldex   },
    { /*465, WINED3DTS_WORLDMATRIX(209)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(209)),        transform_worldex   },
    { /*466, WINED3DTS_WORLDMATRIX(210)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(210)),        transform_worldex   },
    { /*467, WINED3DTS_WORLDMATRIX(211)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(211)),        transform_worldex   },
    { /*468, WINED3DTS_WORLDMATRIX(212)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(212)),        transform_worldex   },
    { /*469, WINED3DTS_WORLDMATRIX(213)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(213)),        transform_worldex   },
    { /*470, WINED3DTS_WORLDMATRIX(214)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(214)),        transform_worldex   },
    { /*471, WINED3DTS_WORLDMATRIX(215)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(215)),        transform_worldex   },
    { /*472, WINED3DTS_WORLDMATRIX(216)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(216)),        transform_worldex   },
    { /*473, WINED3DTS_WORLDMATRIX(217)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(217)),        transform_worldex   },
    { /*474, WINED3DTS_WORLDMATRIX(218)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(218)),        transform_worldex   },
    { /*475, WINED3DTS_WORLDMATRIX(219)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(219)),        transform_worldex   },
    { /*476, WINED3DTS_WORLDMATRIX(220)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(220)),        transform_worldex   },
    { /*477, WINED3DTS_WORLDMATRIX(221)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(221)),        transform_worldex   },
    { /*478, WINED3DTS_WORLDMATRIX(222)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(222)),        transform_worldex   },
    { /*479, WINED3DTS_WORLDMATRIX(223)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(223)),        transform_worldex   },
    { /*480, WINED3DTS_WORLDMATRIX(224)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(224)),        transform_worldex   },
    { /*481, WINED3DTS_WORLDMATRIX(225)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(225)),        transform_worldex   },
    { /*482, WINED3DTS_WORLDMATRIX(226)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(226)),        transform_worldex   },
    { /*483, WINED3DTS_WORLDMATRIX(227)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(227)),        transform_worldex   },
    { /*484, WINED3DTS_WORLDMATRIX(228)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(228)),        transform_worldex   },
    { /*485, WINED3DTS_WORLDMATRIX(229)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(229)),        transform_worldex   },
    { /*486, WINED3DTS_WORLDMATRIX(230)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(230)),        transform_worldex   },
    { /*487, WINED3DTS_WORLDMATRIX(231)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(231)),        transform_worldex   },
    { /*488, WINED3DTS_WORLDMATRIX(232)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(232)),        transform_worldex   },
    { /*489, WINED3DTS_WORLDMATRIX(233)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(233)),        transform_worldex   },
    { /*490, WINED3DTS_WORLDMATRIX(234)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(234)),        transform_worldex   },
    { /*491, WINED3DTS_WORLDMATRIX(235)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(235)),        transform_worldex   },
    { /*492, WINED3DTS_WORLDMATRIX(236)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(236)),        transform_worldex   },
    { /*493, WINED3DTS_WORLDMATRIX(237)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(237)),        transform_worldex   },
    { /*494, WINED3DTS_WORLDMATRIX(238)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(238)),        transform_worldex   },
    { /*495, WINED3DTS_WORLDMATRIX(239)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(239)),        transform_worldex   },
    { /*496, WINED3DTS_WORLDMATRIX(240)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(240)),        transform_worldex   },
    { /*497, WINED3DTS_WORLDMATRIX(241)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(241)),        transform_worldex   },
    { /*498, WINED3DTS_WORLDMATRIX(242)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(242)),        transform_worldex   },
    { /*499, WINED3DTS_WORLDMATRIX(243)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(243)),        transform_worldex   },
    { /*500, WINED3DTS_WORLDMATRIX(244)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(244)),        transform_worldex   },
    { /*501, WINED3DTS_WORLDMATRIX(245)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(245)),        transform_worldex   },
    { /*502, WINED3DTS_WORLDMATRIX(246)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(246)),        transform_worldex   },
    { /*503, WINED3DTS_WORLDMATRIX(247)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(247)),        transform_worldex   },
    { /*504, WINED3DTS_WORLDMATRIX(248)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(248)),        transform_worldex   },
    { /*505, WINED3DTS_WORLDMATRIX(249)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(249)),        transform_worldex   },
    { /*506, WINED3DTS_WORLDMATRIX(250)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(250)),        transform_worldex   },
    { /*507, WINED3DTS_WORLDMATRIX(251)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(251)),        transform_worldex   },
    { /*508, WINED3DTS_WORLDMATRIX(252)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(252)),        transform_worldex   },
    { /*509, WINED3DTS_WORLDMATRIX(253)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(253)),        transform_worldex   },
    { /*510, WINED3DTS_WORLDMATRIX(254)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(254)),        transform_worldex   },
    { /*511, WINED3DTS_WORLDMATRIX(255)             */      STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(255)),        transform_worldex   },
      /* Various Vertex states follow */
    { /*   , STATE_STREAMSRC                        */      STATE_VDECL,                                        vertexdeclaration   },
    { /*   , STATE_INDEXBUFFER                      */      STATE_INDEXBUFFER,                                  indexbuffer         },
    { /*   , STATE_VDECL                            */      STATE_VDECL,                                        vertexdeclaration   },
    { /*   , STATE_VSHADER                          */      STATE_VDECL,                                        vertexdeclaration   },
    { /*   , STATE_VIEWPORT                         */      STATE_VIEWPORT,                                     viewport            },
    { /*   , STATE_VERTEXSHADERCONSTANT             */      STATE_VERTEXSHADERCONSTANT,                         shaderconstant      },
    { /*   , STATE_PIXELSHADERCONSTANT              */      STATE_VERTEXSHADERCONSTANT,                         shaderconstant      },
      /* Lights */
    { /*   , STATE_ACTIVELIGHT(0)                   */      STATE_ACTIVELIGHT(0),                               light               },
    { /*   , STATE_ACTIVELIGHT(1)                   */      STATE_ACTIVELIGHT(1),                               light               },
    { /*   , STATE_ACTIVELIGHT(2)                   */      STATE_ACTIVELIGHT(2),                               light               },
    { /*   , STATE_ACTIVELIGHT(3)                   */      STATE_ACTIVELIGHT(3),                               light               },
    { /*   , STATE_ACTIVELIGHT(4)                   */      STATE_ACTIVELIGHT(4),                               light               },
    { /*   , STATE_ACTIVELIGHT(5)                   */      STATE_ACTIVELIGHT(5),                               light               },
    { /*   , STATE_ACTIVELIGHT(6)                   */      STATE_ACTIVELIGHT(6),                               light               },
    { /*   , STATE_ACTIVELIGHT(7)                   */      STATE_ACTIVELIGHT(7),                               light               },

    { /* Scissor rect                               */      STATE_SCISSORRECT,                                  scissorrect         },
      /* Clip planes */
    { /* STATE_CLIPPLANE(0)                         */      STATE_CLIPPLANE(0),                                 clipplane           },
    { /* STATE_CLIPPLANE(1)                         */      STATE_CLIPPLANE(1),                                 clipplane           },
    { /* STATE_CLIPPLANE(2)                         */      STATE_CLIPPLANE(2),                                 clipplane           },
    { /* STATE_CLIPPLANE(3)                         */      STATE_CLIPPLANE(3),                                 clipplane           },
    { /* STATE_CLIPPLANE(4)                         */      STATE_CLIPPLANE(4),                                 clipplane           },
    { /* STATE_CLIPPLANE(5)                         */      STATE_CLIPPLANE(5),                                 clipplane           },
    { /* STATE_CLIPPLANE(6)                         */      STATE_CLIPPLANE(6),                                 clipplane           },
    { /* STATE_CLIPPLANE(7)                         */      STATE_CLIPPLANE(7),                                 clipplane           },
    { /* STATE_CLIPPLANE(8)                         */      STATE_CLIPPLANE(8),                                 clipplane           },
    { /* STATE_CLIPPLANE(9)                         */      STATE_CLIPPLANE(9),                                 clipplane           },
    { /* STATE_CLIPPLANE(10)                        */      STATE_CLIPPLANE(10),                                clipplane           },
    { /* STATE_CLIPPLANE(11)                        */      STATE_CLIPPLANE(11),                                clipplane           },
    { /* STATE_CLIPPLANE(12)                        */      STATE_CLIPPLANE(12),                                clipplane           },
    { /* STATE_CLIPPLANE(13)                        */      STATE_CLIPPLANE(13),                                clipplane           },
    { /* STATE_CLIPPLANE(14)                        */      STATE_CLIPPLANE(14),                                clipplane           },
    { /* STATE_CLIPPLANE(15)                        */      STATE_CLIPPLANE(15),                                clipplane           },
    { /* STATE_CLIPPLANE(16)                        */      STATE_CLIPPLANE(16),                                clipplane           },
    { /* STATE_CLIPPLANE(17)                        */      STATE_CLIPPLANE(17),                                clipplane           },
    { /* STATE_CLIPPLANE(18)                        */      STATE_CLIPPLANE(18),                                clipplane           },
    { /* STATE_CLIPPLANE(19)                        */      STATE_CLIPPLANE(19),                                clipplane           },
    { /* STATE_CLIPPLANE(20)                        */      STATE_CLIPPLANE(20),                                clipplane           },
    { /* STATE_CLIPPLANE(21)                        */      STATE_CLIPPLANE(21),                                clipplane           },
    { /* STATE_CLIPPLANE(22)                        */      STATE_CLIPPLANE(22),                                clipplane           },
    { /* STATE_CLIPPLANE(23)                        */      STATE_CLIPPLANE(23),                                clipplane           },
    { /* STATE_CLIPPLANE(24)                        */      STATE_CLIPPLANE(24),                                clipplane           },
    { /* STATE_CLIPPLANE(25)                        */      STATE_CLIPPLANE(25),                                clipplane           },
    { /* STATE_CLIPPLANE(26)                        */      STATE_CLIPPLANE(26),                                clipplane           },
    { /* STATE_CLIPPLANE(27)                        */      STATE_CLIPPLANE(27),                                clipplane           },
    { /* STATE_CLIPPLANE(28)                        */      STATE_CLIPPLANE(28),                                clipplane           },
    { /* STATE_CLIPPLANE(29)                        */      STATE_CLIPPLANE(29),                                clipplane           },
    { /* STATE_CLIPPLANE(30)                        */      STATE_CLIPPLANE(30),                                clipplane           },
    { /* STATE_CLIPPLANE(31)                        */      STATE_CLIPPLANE(31),                                clipplane           },

    { /* STATE_MATERIAL                             */      STATE_RENDER(WINED3DRS_SPECULARENABLE),             state_specularenable},
    { /* STATE_FRONTFACE                            */      STATE_FRONTFACE,                                    frontface           },
};
