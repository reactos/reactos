
#include "xboxogl.h"
#include <string.h>
#include <math.h>
#include <debug.h>

static DWORD g_tlsSlot = TLS_OUT_OF_INDEXES;

PXGL_CONTEXT XglCurrent(void)
{
    if (g_tlsSlot == TLS_OUT_OF_INDEXES)
        return NULL;
    return (PXGL_CONTEXT)TlsGetValue(g_tlsSlot);
}

void XglSetCurrent(PXGL_CONTEXT ctx)
{
    if (g_tlsSlot == TLS_OUT_OF_INDEXES)
        g_tlsSlot = TlsAlloc();
    if (g_tlsSlot != TLS_OUT_OF_INDEXES)
        TlsSetValue(g_tlsSlot, ctx);
}

static void SetError(PXGL_CONTEXT c, GLenum e)
{
    if (c && c->lastError == GL_NO_ERROR)
        c->lastError = e;
}

static __inline BYTE F2B(float v)
{
    int i = (int)(v * 255.0f + 0.5f);
    if (i < 0) i = 0; else if (i > 255) i = 255;
    return (BYTE)i;
}

void XglMatIdentity(XGL_MAT4 *m)
{
    static const float I[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
    memcpy(m->m, I, sizeof(I));
}

void XglMatMul(XGL_MAT4 *out, const XGL_MAT4 *a, const XGL_MAT4 *b)
{
    XGL_MAT4 r;
    int i, j, k;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
        {
            float s = 0;
            for (k = 0; k < 4; k++)
                s += a->m[i + k*4] * b->m[k + j*4];
            r.m[i + j*4] = s;
        }
    *out = r;
}

void XglMatTransform(const XGL_MAT4 *m, const float in[4], float out[4])
{
    int i, k;
    for (i = 0; i < 4; i++)
    {
        float s = 0;
        for (k = 0; k < 4; k++)
            s += m->m[i + k*4] * in[k];
        out[i] = s;
    }
}

static XGL_MAT4 *CurMat(PXGL_CONTEXT c)
{
    return &c->matStack[c->matMode][c->matTop[c->matMode]];
}

/* out = current * m  (post-multiply, as glMultMatrix/Translate/etc. do) */
static void MulCurrentBy(PXGL_CONTEXT c, const XGL_MAT4 *m)
{
    XGL_MAT4 *cur = CurMat(c);
    XglMatMul(cur, cur, m);
}

/* NV2A 3D engine submission! */

/* Target rectangle (the GL viewport mapped onto the desktop) + full-screen
 * surface dimensions used to express the geometry in full-surface clip space. */
static void GetTargetRect(PXGL_CONTEXT c, float *ox, float *oy,
                          float *surfW, float *surfH,
                          ULONG *dstX, ULONG *dstY, ULONG *dstW, ULONG *dstH)
{
    POINT org;
    org.x = 0; org.y = 0;
    GetDCOrgEx(c->hdc, &org);
    *ox = (float)(org.x + c->vpX);
    *oy = (float)(org.y + c->vpY);
    *surfW = (float)GetDeviceCaps(c->hdc, HORZRES);
    *surfH = (float)GetDeviceCaps(c->hdc, VERTRES);
    if (*surfW < 1.0f) *surfW = 640.0f;
    if (*surfH < 1.0f) *surfH = 480.0f;
    *dstX = (ULONG)(org.x + c->vpX);
    *dstY = (ULONG)(org.y + c->vpY);
    *dstW = (ULONG)c->vpW;
    *dstH = (ULONG)c->vpH;
}

/* General 4x4 inverse (column-major).  Returns FALSE if singular. */
static BOOL XglMat4Inverse(float out[16], const float m[16])
{
    float inv[16], det;
    int i;
    inv[0]  =  m[5]*m[10]*m[15]-m[5]*m[11]*m[14]-m[9]*m[6]*m[15]+m[9]*m[7]*m[14]+m[13]*m[6]*m[11]-m[13]*m[7]*m[10];
    inv[4]  = -m[4]*m[10]*m[15]+m[4]*m[11]*m[14]+m[8]*m[6]*m[15]-m[8]*m[7]*m[14]-m[12]*m[6]*m[11]+m[12]*m[7]*m[10];
    inv[8]  =  m[4]*m[9]*m[15]-m[4]*m[11]*m[13]-m[8]*m[5]*m[15]+m[8]*m[7]*m[13]+m[12]*m[5]*m[11]-m[12]*m[7]*m[9];
    inv[12] = -m[4]*m[9]*m[14]+m[4]*m[10]*m[13]+m[8]*m[5]*m[14]-m[8]*m[6]*m[13]-m[12]*m[5]*m[10]+m[12]*m[6]*m[9];
    inv[1]  = -m[1]*m[10]*m[15]+m[1]*m[11]*m[14]+m[9]*m[2]*m[15]-m[9]*m[3]*m[14]-m[13]*m[2]*m[11]+m[13]*m[3]*m[10];
    inv[5]  =  m[0]*m[10]*m[15]-m[0]*m[11]*m[14]-m[8]*m[2]*m[15]+m[8]*m[3]*m[14]+m[12]*m[2]*m[11]-m[12]*m[3]*m[10];
    inv[9]  = -m[0]*m[9]*m[15]+m[0]*m[11]*m[13]+m[8]*m[1]*m[15]-m[8]*m[3]*m[13]-m[12]*m[1]*m[11]+m[12]*m[3]*m[9];
    inv[13] =  m[0]*m[9]*m[14]-m[0]*m[10]*m[13]-m[8]*m[1]*m[14]+m[8]*m[2]*m[13]+m[12]*m[1]*m[10]-m[12]*m[2]*m[9];
    inv[2]  =  m[1]*m[6]*m[15]-m[1]*m[7]*m[14]-m[5]*m[2]*m[15]+m[5]*m[3]*m[14]+m[13]*m[2]*m[7]-m[13]*m[3]*m[6];
    inv[6]  = -m[0]*m[6]*m[15]+m[0]*m[7]*m[14]+m[4]*m[2]*m[15]-m[4]*m[3]*m[14]-m[12]*m[2]*m[7]+m[12]*m[3]*m[6];
    inv[10] =  m[0]*m[5]*m[15]-m[0]*m[7]*m[13]-m[4]*m[1]*m[15]+m[4]*m[3]*m[13]+m[12]*m[1]*m[7]-m[12]*m[3]*m[5];
    inv[14] = -m[0]*m[5]*m[14]+m[0]*m[6]*m[13]+m[4]*m[1]*m[14]-m[4]*m[2]*m[13]-m[12]*m[1]*m[6]+m[12]*m[2]*m[5];
    inv[3]  = -m[1]*m[6]*m[11]+m[1]*m[7]*m[10]+m[5]*m[2]*m[11]-m[5]*m[3]*m[10]-m[9]*m[2]*m[7]+m[9]*m[3]*m[6];
    inv[7]  =  m[0]*m[6]*m[11]-m[0]*m[7]*m[10]-m[4]*m[2]*m[11]+m[4]*m[3]*m[10]+m[8]*m[2]*m[7]-m[8]*m[3]*m[6];
    inv[11] = -m[0]*m[5]*m[11]+m[0]*m[7]*m[9]+m[4]*m[1]*m[11]-m[4]*m[3]*m[9]-m[8]*m[1]*m[7]+m[8]*m[3]*m[5];
    inv[15] =  m[0]*m[5]*m[10]-m[0]*m[6]*m[9]-m[4]*m[1]*m[10]+m[4]*m[2]*m[9]+m[8]*m[1]*m[6]-m[8]*m[2]*m[5];
    det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12];
    if (det > -1e-12f && det < 1e-12f) return FALSE;
    det = 1.0f / det;
    for (i = 0; i < 16; i++) out[i] = inv[i] * det;
    return TRUE;
}

/* Transform a plane (object coords) to eye space by the current modelview, so
 * later `plane . eyeCoord` reproduces GL's clip-plane / EYE_LINEAR semantics:
 * p_eye = (M^-1)^T p_obj.  Falls back to the raw plane if the modelview is singular. */
static void XglPlaneToEye(PXGL_CONTEXT c, const double in[4], double out[4])
{
    const XGL_MAT4 *mv = &c->matStack[XGL_MAT_MODELVIEW][c->matTop[XGL_MAT_MODELVIEW]];
    float invm[16];
    int i;
    if (!XglMat4Inverse(invm, mv->m))
    {
        for (i = 0; i < 4; i++) out[i] = in[i];
        return;
    }
    for (i = 0; i < 4; i++)
        out[i] = (double)invm[i*4+0]*in[0] + (double)invm[i*4+1]*in[1] +
                 (double)invm[i*4+2]*in[2] + (double)invm[i*4+3]*in[3];
}

/* Evaluate one generated texture coordinate (glTexGen) for coord 0=S or 1=T. */
static float TexGenValue(PXGL_CONTEXT c, int coord, const float obj[4],
                         const float eye[4], const float neye[3])
{
    switch (c->texGenMode[coord])
    {
        case GL_OBJECT_LINEAR:
        {
            const float *p = c->texGenObjPlane[coord];
            return p[0]*obj[0]+p[1]*obj[1]+p[2]*obj[2]+p[3]*obj[3];
        }
        case GL_SPHERE_MAP:
        {
            float ux=eye[0], uy=eye[1], uz=eye[2], ul=(float)sqrt(eye[0]*eye[0]+eye[1]*eye[1]+eye[2]*eye[2]);
            float ndu, rx, ry, rz, m;
            if (ul > 1e-6f) { ux/=ul; uy/=ul; uz/=ul; }
            ndu = neye[0]*ux + neye[1]*uy + neye[2]*uz;
            rx = ux - 2.0f*neye[0]*ndu; ry = uy - 2.0f*neye[1]*ndu; rz = uz - 2.0f*neye[2]*ndu;
            m = 2.0f*(float)sqrt(rx*rx + ry*ry + (rz+1.0f)*(rz+1.0f));
            if (m < 1e-6f) m = 1.0f;
            return (coord == 0) ? (rx/m + 0.5f) : (ry/m + 0.5f);
        }
        case GL_EYE_LINEAR:
        default:
        {
            const float *p = c->texGenEyePlane[coord];
            return p[0]*eye[0]+p[1]*eye[1]+p[2]*eye[2]+p[3]*eye[3];
        }
    }
}

/* Transform an object-space vertex to a full-surface clip-space NV2A vertex.
 * Returns FALSE (reject the triangle) if the vertex is at/behind the eye. */
static BOOL XformObj(PXGL_CONTEXT c, const XGL_VERTEX *v,
                     float ox, float oy, float surfW, float surfH,
                     NV2A_3D_VERTEX *out)
{
    XGL_MAT4 mvp;
    float in[4], clip[4], iw, ndcx, ndcy, ndcz, sx, sy;

    XglMatMul(&mvp,
              &c->matStack[XGL_MAT_PROJECTION][c->matTop[XGL_MAT_PROJECTION]],
              &c->matStack[XGL_MAT_MODELVIEW] [c->matTop[XGL_MAT_MODELVIEW]]);
    in[0] = v->x; in[1] = v->y; in[2] = v->z; in[3] = v->w;
    XglMatTransform(&mvp, in, clip);

    if (clip[3] <= 0.0001f)
        return FALSE;

    iw = 1.0f / clip[3];
    ndcx = clip[0] * iw;
    ndcy = clip[1] * iw;
    ndcz = clip[2] * iw;

    /* GL viewport -> window pixel (Y-down) -> full-surface clip space. */
    sx = ox + (ndcx * 0.5f + 0.5f) * c->vpW;
    sy = oy + (0.5f - ndcy * 0.5f) * c->vpH;
    out->x = (2.0f * sx / surfW) - 1.0f;
    out->y = 1.0f - (2.0f * sy / surfH);
    out->z = ndcz * 0.5f + 0.5f;
    out->w = 1.0f;   /* vertices are screen-space (affine); perspective is recovered per-texel below */
    out->r = v->r; out->g = v->g; out->b = v->b; out->a = v->a;

    /* Clip planes and texgen work in eye space; compute eye pos/normal once if
     * either is on.  A clipped vertex fails XformObj, dropping the whole triangle
     * (no splitting — fine for fully in/out tris). */
    {
        float genS = v->s, genT = v->t;
        int ci;
        BOOL clipOn = FALSE;
        BOOL genOn = c->texGenEnabled[0] || c->texGenEnabled[1];
        for (ci = 0; ci < XGL_MAX_CLIP_PLANES; ci++)
            if (c->clipPlaneEnabled[ci]) { clipOn = TRUE; break; }
        if (clipOn || genOn)
        {
            const XGL_MAT4 *mv = &c->matStack[XGL_MAT_MODELVIEW][c->matTop[XGL_MAT_MODELVIEW]];
            float eyeC[4], neye[3];
            neye[0]=0.0f; neye[1]=0.0f; neye[2]=1.0f;
            XglMatTransform(mv, in, eyeC);
            if (clipOn)
                for (ci = 0; ci < XGL_MAX_CLIP_PLANES; ci++)
                    if (c->clipPlaneEnabled[ci])
                    {
                        double d = c->clipPlane[ci][0]*eyeC[0] + c->clipPlane[ci][1]*eyeC[1] +
                                   c->clipPlane[ci][2]*eyeC[2] + c->clipPlane[ci][3]*eyeC[3];
                        if (d < 0.0) return FALSE;
                    }
            if (genOn)
            {
                float nx = mv->m[0]*v->nx + mv->m[4]*v->ny + mv->m[8] *v->nz;
                float ny = mv->m[1]*v->nx + mv->m[5]*v->ny + mv->m[9] *v->nz;
                float nz = mv->m[2]*v->nx + mv->m[6]*v->ny + mv->m[10]*v->nz;
                float nl = (float)sqrt(nx*nx + ny*ny + nz*nz);
                if (nl > 1e-6f) { nx/=nl; ny/=nl; nz/=nl; }
                neye[0]=nx; neye[1]=ny; neye[2]=nz;
                if (c->texGenEnabled[0]) genS = TexGenValue(c, 0, in, eyeC, neye);
                if (c->texGenEnabled[1]) genT = TexGenValue(c, 1, in, eyeC, neye);
            }
        }

        /* HARDWARE perspective-correct texturing.  Our vertices are pre-projected to
         * screen space, so the GPU interpolates them affinely (PS1-style warping if
         * we fed plain texcoords).  Instead carry texel coords PREMULTIPLIED by 1/w
         * and q=1/w; the NV2A PROJECT2D textureProj divides by the interpolated q,
         * which equals perspective-correct interpolation:
         *   (Σ uv_i/w_i·λ) / (Σ 1/w_i·λ). */
        out->u = genS; out->v = genT; out->q = 1.0f;
        if (c->enTexture2D && c->boundTexture < XGL_MAX_TEXTURES)
        {
            XGL_TEXTURE *tx = &c->textures[c->boundTexture];
            if (tx->gpuOffset && tx->width > 0 && tx->height > 0)
            {
                /* Apply the GL_TEXTURE matrix to the coord, then texel-scale and
                 * premultiply by 1/w for hardware perspective correction. */
                const XGL_MAT4 *tm = &c->matStack[XGL_MAT_TEXTURE][c->matTop[XGL_MAT_TEXTURE]];
                float tin[4], tout[4];
                tin[0]=genS; tin[1]=genT; tin[2]=0.0f; tin[3]=1.0f;
                XglMatTransform(tm, tin, tout);
                out->u = tout[0] * (float)tx->width  * iw;   /* texel * (1/w) */
                out->v = tout[1] * (float)tx->height * iw;
                out->q = iw;                                 /* 1/w */
            }
        }
    }

    /* Fixed-function multi-light (ambient + diffuse), computed per vertex on the
     * CPU.  Normal -> eye space via modelview upper-3x3; result =
     *   emission + globalAmbient*matAmbient
     *   + Σ_lights [ light.ambient*matAmbient + light.diffuse*matDiffuse*max(0,N·L) ]. */
    if (c->enLighting)
    {
        const XGL_MAT4 *mv = &c->matStack[XGL_MAT_MODELVIEW][c->matTop[XGL_MAT_MODELVIEW]];
        float nx = mv->m[0]*v->nx + mv->m[4]*v->ny + mv->m[8] *v->nz;
        float ny = mv->m[1]*v->nx + mv->m[5]*v->ny + mv->m[9] *v->nz;
        float nz = mv->m[2]*v->nx + mv->m[6]*v->ny + mv->m[10]*v->nz;
        float len = (float)sqrt(nx*nx + ny*ny + nz*nz);
        float rr, gg, bb, eye[4];
        int li;
        if (len > 1e-6f) { nx/=len; ny/=len; nz/=len; }
        XglMatTransform(mv, in, eye);   /* eye-space vertex position (for positional lights) */

        /* Eye direction for specular (local viewer: toward the origin). */
        float edx = -eye[0], edy = -eye[1], edz = -eye[2];
        float el = (float)sqrt(edx*edx + edy*edy + edz*edz);
        if (el > 1e-6f) { edx/=el; edy/=el; edz/=el; }

        rr = c->matEmission[0] + c->globalAmbient[0]*c->matAmbient[0];
        gg = c->matEmission[1] + c->globalAmbient[1]*c->matAmbient[1];
        bb = c->matEmission[2] + c->globalAmbient[2]*c->matAmbient[2];

        for (li = 0; li < XGL_MAX_LIGHTS; li++)
        {
            const XGL_LIGHT *L = &c->lights[li];
            float lx, ly, lz, ll, ndl;
            if (!L->enabled) continue;
            if (L->pos[3] == 0.0f) { lx = L->pos[0]; ly = L->pos[1]; lz = L->pos[2]; }  /* directional */
            else { lx = L->pos[0]-eye[0]; ly = L->pos[1]-eye[1]; lz = L->pos[2]-eye[2]; } /* positional */
            ll = (float)sqrt(lx*lx + ly*ly + lz*lz);
            if (ll > 1e-6f) { lx/=ll; ly/=ll; lz/=ll; }
            ndl = nx*lx + ny*ly + nz*lz;
            if (ndl < 0.0f) ndl = 0.0f;
            rr += L->ambient[0]*c->matAmbient[0] + L->diffuse[0]*c->matDiffuse[0]*ndl;
            gg += L->ambient[1]*c->matAmbient[1] + L->diffuse[1]*c->matDiffuse[1]*ndl;
            bb += L->ambient[2]*c->matAmbient[2] + L->diffuse[2]*c->matDiffuse[2]*ndl;
            /* Blinn-Phong specular: pow(max(0,N·H), shininess), H = norm(L + eyeDir). */
            if (ndl > 0.0f && c->matShininess > 0.0f)
            {
                float hx = lx+edx, hy = ly+edy, hz = lz+edz;
                float hl = (float)sqrt(hx*hx + hy*hy + hz*hz), ndh, sp;
                if (hl > 1e-6f) { hx/=hl; hy/=hl; hz/=hl; }
                ndh = nx*hx + ny*hy + nz*hz;
                if (ndh < 0.0f) ndh = 0.0f;
                sp = (float)pow(ndh, c->matShininess);
                rr += L->specular[0]*c->matSpecular[0]*sp;
                gg += L->specular[1]*c->matSpecular[1]*sp;
                bb += L->specular[2]*c->matSpecular[2]*sp;
            }
        }
        if (rr > 1.0f) rr = 1.0f;
        if (gg > 1.0f) gg = 1.0f;
        if (bb > 1.0f) bb = 1.0f;
        out->r = rr; out->g = gg; out->b = bb; out->a = c->matDiffuse[3];
    }

    /* Fog: blend the (lit) vertex colour toward the fog colour by the fog factor
     * derived from the eye-space distance.  LINEAR/EXP/EXP2 per glFog. */
    if (c->enFog)
    {
        const XGL_MAT4 *mv = &c->matStack[XGL_MAT_MODELVIEW][c->matTop[XGL_MAT_MODELVIEW]];
        float feye[4], fz, f;
        XglMatTransform(mv, in, feye);
        fz = feye[2] < 0.0f ? -feye[2] : feye[2];
        if (c->fogMode == GL_LINEAR)
        {
            float denom = c->fogEnd - c->fogStart;
            f = (denom != 0.0f) ? (c->fogEnd - fz) / denom : 1.0f;
        }
        else if (c->fogMode == GL_EXP2)
        {
            float e = c->fogDensity * fz;
            f = (float)exp(-(e * e));
        }
        else /* GL_EXP */
        {
            f = (float)exp(-c->fogDensity * fz);
        }
        if (f < 0.0f) f = 0.0f;
        if (f > 1.0f) f = 1.0f;
        out->r = f*out->r + (1.0f-f)*c->fogColor[0];
        out->g = f*out->g + (1.0f-f)*c->fogColor[1];
        out->b = f*out->b + (1.0f-f)*c->fogColor[2];
    }

    /* Selection: a vertex inside the (pick) view volume registers a hit for the
     * current name stack; the record is flushed on the next name change / RenderMode. */
    if (c->renderMode == GL_SELECT &&
        ndcx >= -1.0f && ndcx <= 1.0f && ndcy >= -1.0f && ndcy <= 1.0f &&
        ndcz >= -1.0f && ndcz <= 1.0f)
    {
        float depth = ndcz * 0.5f + 0.5f;
        if (!c->selHitActive) { c->selHitActive = TRUE; c->selHitZMin = depth; c->selHitZMax = depth; }
        else
        {
            if (depth < c->selHitZMin) c->selHitZMin = depth;
            if (depth > c->selHitZMax) c->selHitZMax = depth;
        }
    }
    return TRUE;
}

/* Map a GL texture wrap enum to the NV2A address mode (1=wrap, 2=mirror, 3=clamp). */
static ULONG WrapToNv(GLenum w)
{
    switch (w)
    {
        case GL_REPEAT:          return 1;
        case GL_MIRRORED_REPEAT: return 2;
        case GL_CLAMP:
        case GL_CLAMP_TO_EDGE:   return 3;
        default:                 return 3;
    }
}

/* Feedback-mode vertex emission (defined with the rest of the GL 1.1 tail). */
static void FeedbackWritePrimitive(PXGL_CONTEXT c, const NV2A_3D_VERTEX *verts, int n, ULONG topology);
/* Offscreen-surface read/write helpers (defined in the pixel-ops section). */
static BOOL XglReadbackRect(PXGL_CONTEXT c, ULONG sx, ULONG sy, ULONG w, ULONG h, DWORD *out);
static BOOL XglWritebackRect(PXGL_CONTEXT c, ULONG sx, ULONG sy, ULONG w, ULONG h, ULONG flags, const DWORD *in);
static void XglGlToScreen(PXGL_CONTEXT c, GLint xgl, GLint ygl, GLsizei h, ULONG *sx, ULONG *sytop);

/* Submit a vertex batch (and/or a frame clear / present) to the 3D engine.
 * `topology` is one of NV2A_TOPO_* (TRIANGLES for the expanded primitives). */
static void SubmitBatch(PXGL_CONTEXT c, const NV2A_3D_VERTEX *verts, int n,
                        ULONG extraFlags, ULONG topology)
{
    NV2A_DRAW_3D draw;
    ULONG flags = extraFlags, dstX, dstY, dstW, dstH, need;
    float ox, oy, surfW, surfH;
    int i;

    if (!c->hdc)
        return;
    if (n < 0) n = 0;
    if (n > NV2A_3D_MAX_VERTS) n = NV2A_3D_MAX_VERTS;
    /* Selection / feedback render modes never touch the GPU: selection hits are
     * accumulated in XformObj; feedback emits vertex tokens here. */
    if (c->renderMode != GL_RENDER)
    {
        if (c->renderMode == GL_FEEDBACK && n > 0)
            FeedbackWritePrimitive(c, verts, n, topology);
        return;
    }
    if (c->clearPending)
        flags |= NV2A_DRAW3D_FLAG_CLEAR;
    if (c->clearDepthPending)
        flags |= NV2A_DRAW3D_FLAG_CLEAR_DEPTH;
    if (c->clearStencilPending)
        flags |= NV2A_DRAW3D_FLAG_CLEAR_STENCIL;
    if (n == 0 && flags == 0)
        return;   /* nothing to do */

    GetTargetRect(c, &ox, &oy, &surfW, &surfH, &dstX, &dstY, &dstW, &dstH);

    draw.VertexCount = (ULONG)n;
    draw.Topology    = topology ? topology : NV2A_TOPO_TRIANGLES;
    draw.Flags       = flags;
    draw.ClearColor  = c->clearPacked;
    /* Per-batch depth state from the GL state machine.  GL depth-func enums
     * (GL_NEVER..GL_ALWAYS == 0x200..0x207) are the NV2A SET_DEPTH_FUNC values. */
    draw.DepthTestEnable = c->enDepthTest ? 1 : 0;
    draw.DepthFunc       = (ULONG)c->depthFunc;
    draw.DepthMask       = c->depthMask ? 1 : 0;
    draw.CullEnable      = c->enCullFace ? 1 : 0;
    /* Texturing: a bound, uploaded 2D texture with GL_TEXTURE_2D enabled. */
    {
        XGL_TEXTURE *tx = (c->boundTexture < XGL_MAX_TEXTURES)
                          ? &c->textures[c->boundTexture] : NULL;
        BOOL textured = c->enTexture2D && tx && tx->gpuOffset &&
                        tx->width > 0 && tx->height > 0;
        draw.TexEnable = textured ? 1 : 0;
        {
            static int s_logn;
            if (n > 0 && s_logn < 24) {
                DPRINT1("xboxogl: draw n=%d topo=%lu textured=%d en2d=%d boundTex=%u gpuOff=0x%x %dx%d env=0x%x\n",
                        n, topology, textured?1:0, c->enTexture2D?1:0, c->boundTexture,
                        tx?tx->gpuOffset:0, tx?tx->width:0, tx?tx->height:0, tx?(unsigned)tx->envMode:0);
                s_logn++;
            }
        }
        draw.TexOffset = textured ? tx->gpuOffset : 0;
        draw.TexWidth  = textured ? (ULONG)tx->width  : 0;
        draw.TexHeight = textured ? (ULONG)tx->height : 0;
        /* Honour glTexParameteri filtering: GL_LINEAR->NV2A 2, else NEAREST->1.
         * (No mipmaps, so LINEAR_MIPMAP_* collapse to LINEAR.) */
        if (textured)
        {
            ULONG mag = (tx->magFilter == GL_LINEAR) ? 2 : 1;
            ULONG min = (tx->minFilter == GL_LINEAR ||
                         tx->minFilter == GL_LINEAR_MIPMAP_NEAREST ||
                         tx->minFilter == GL_LINEAR_MIPMAP_LINEAR) ? 2 : 1;
            draw.TexFilter = (mag << 24) | (min << 16);
        }
        else
        {
            draw.TexFilter = 0;
        }
        draw.TexReplace = (textured && tx->envMode == GL_REPLACE) ? 1 : 0;
        /* glTexEnv mode -> miniport combiner selector (0=MODULATE,1=REPLACE,2=DECAL,3=BLEND). */
        if (textured)
        {
            switch (tx->envMode) {
                case GL_REPLACE: draw.TexEnvMode = 1; break;
                case GL_DECAL:   draw.TexEnvMode = 2; break;
                case GL_BLEND:   draw.TexEnvMode = 3; break;
                default:         draw.TexEnvMode = 0; break;
            }
        }
        else draw.TexEnvMode = 0;
        draw.TexEnvColor = ((ULONG)(c->texEnvColor[3]*255.0f+0.5f) << 24) |
                           ((ULONG)(c->texEnvColor[0]*255.0f+0.5f) << 16) |
                           ((ULONG)(c->texEnvColor[1]*255.0f+0.5f) <<  8) |
                            (ULONG)(c->texEnvColor[2]*255.0f+0.5f);
        /* Texture wrap (GL 1.2 CLAMP_TO_EDGE + REPEAT/MIRROR).  NV2A: 1=wrap,
         * 2=mirror, 3=clamp-to-edge.  P (3rd axis) clamped. */
        if (textured)
        {
            ULONG u = WrapToNv(tx->wrapS), v = WrapToNv(tx->wrapT);
            draw.TexAddress = u | (v << 8) | (3u << 16);
        }
        else
        {
            draw.TexAddress = 0;
        }
    }
    /* Alpha blending (NV2A blend-factor enums == GL enums, incl. CONSTANT_*). */
    draw.BlendEnable = c->enBlend ? 1 : 0;
    draw.BlendSrc    = (ULONG)c->blendSrc;
    draw.BlendDst    = (ULONG)c->blendDst;
    /* GL 1.2 glBlendEquation: pass the GL enum through; the miniport remaps it to
     * the NV2A equation value (ADD/MIN/MAX share the enum, SUBTRACT/REVERSE differ). */
    draw.BlendEquation = (ULONG)c->blendEquation;
    /* glBlendColor — pack the constant to 0xAARRGGBB for the constant-* factors. */
    draw.BlendColor =
        ((ULONG)(c->blendColor[3]*255.0f+0.5f) << 24) |
        ((ULONG)(c->blendColor[0]*255.0f+0.5f) << 16) |
        ((ULONG)(c->blendColor[1]*255.0f+0.5f) <<  8) |
         (ULONG)(c->blendColor[2]*255.0f+0.5f);
    /* Alpha test (GL alpha-func enums == NV2A) + polygon mode (GL == NV2A). */
    draw.AlphaTestEnable = c->enAlphaTest ? 1 : 0;
    draw.AlphaFunc       = (ULONG)c->alphaFunc;
    draw.AlphaRef        = (ULONG)(c->alphaRef * 255.0f);
    draw.PolygonMode     = (ULONG)c->polygonMode;
    /* glPolygonOffset (gated: 0 = off, so ordinary draws are unaffected). */
    draw.PolyOffsetEnable = (c->enPolyOffsetFill ? 1u : 0u) |
                            (c->enPolyOffsetLine ? 2u : 0u) |
                            (c->enPolyOffsetPoint ? 4u : 0u);
    draw.PolyOffsetFactor = *(const ULONG *)&c->polyOffsetFactor;
    draw.PolyOffsetUnits  = *(const ULONG *)&c->polyOffsetUnits;
    draw.PointSize       = *(const ULONG *)&c->pointSize;  /* float size for the VP's oPts (program mode) */
    /* glColorMask -> NV2A SET_COLOR_MASK (alpha<<24 | red<<16 | green<<8 | blue). */
    draw.ColorMask = ((c->colorMask[3] ? 1u : 0u) << 24) |
                     ((c->colorMask[0] ? 1u : 0u) << 16) |
                     ((c->colorMask[1] ? 1u : 0u) <<  8) |
                      (c->colorMask[2] ? 1u : 0u);
    /* Stencil (glStencil*) — GL func/op enums pass straight to the NV2A.  The
     * miniport only programs these when the zeta is Z24S8. */
    draw.StencilEnable    = c->enStencilTest ? 1 : 0;
    draw.StencilFunc      = (ULONG)c->stencilFunc;
    draw.StencilRef       = (ULONG)((c->stencilRef < 0 ? 0 : c->stencilRef) & 0xFF);
    draw.StencilFuncMask  = c->stencilValueMask;
    draw.StencilWriteMask = c->stencilWriteMask;
    draw.StencilOpFail    = (ULONG)c->stencilFail;
    draw.StencilOpZFail   = (ULONG)c->stencilZFail;
    draw.StencilOpZPass   = (ULONG)c->stencilZPass;
    draw.ClearStencil     = (ULONG)(c->clearStencil & 0xFF);
    draw.DstX = dstX; draw.DstY = dstY; draw.DstW = dstW; draw.DstH = dstH;
    for (i = 0; i < n; i++)
        draw.Verts[i] = verts[i];

    need = FIELD_OFFSET(NV2A_DRAW_3D, Verts) + (ULONG)n * sizeof(NV2A_3D_VERTEX);
    ExtEscape(c->hdc, XBOX_ESC_NV2A_DRAW3D, (int)need, (LPCSTR)&draw, 0, NULL);

    c->clearPending = FALSE;        /* the clears rode with this submission */
    c->clearDepthPending = FALSE;
    c->clearStencilPending = FALSE;
}

/* Assemble the accumulated primitive into triangles and submit them. */
static void FlushPrimitive(PXGL_CONTEXT c)
{
    NV2A_3D_VERTEX batch[NV2A_3D_MAX_VERTS];
    int bn = 0;
    UINT i;
    XGL_VERTEX *V = c->verts;
    UINT n = c->vertCount;
    float ox, oy, surfW, surfH;
    ULONG dx, dy, dw, dh;

    GetTargetRect(c, &ox, &oy, &surfW, &surfH, &dx, &dy, &dw, &dh);

#define EMIT_TRI(A,B,D) do {                                                  \
        NV2A_3D_VERTEX t0, t1, t2;                                            \
        if (XformObj(c,(A),ox,oy,surfW,surfH,&t0) &&                          \
            XformObj(c,(B),ox,oy,surfW,surfH,&t1) &&                          \
            XformObj(c,(D),ox,oy,surfW,surfH,&t2)) {                          \
            if (bn + 3 > NV2A_3D_MAX_VERTS) { SubmitBatch(c, batch, bn, 0, NV2A_TOPO_TRIANGLES); bn = 0; } \
            batch[bn++] = t0; batch[bn++] = t1; batch[bn++] = t2;             \
        }                                                                     \
    } while (0)

    switch (c->primitive)
    {
        case GL_TRIANGLES:
            for (i = 0; i + 3 <= n; i += 3) EMIT_TRI(&V[i], &V[i+1], &V[i+2]);
            break;
        case GL_TRIANGLE_STRIP:
            for (i = 0; i + 3 <= n; i++)
            {
                if (i & 1) EMIT_TRI(&V[i+1], &V[i], &V[i+2]);
                else       EMIT_TRI(&V[i], &V[i+1], &V[i+2]);
            }
            break;
        case GL_TRIANGLE_FAN:
        case GL_POLYGON:
            for (i = 1; i + 1 < n; i++) EMIT_TRI(&V[0], &V[i], &V[i+1]);
            break;
        case GL_QUADS:
            for (i = 0; i + 4 <= n; i += 4)
            {
                EMIT_TRI(&V[i], &V[i+1], &V[i+2]);
                EMIT_TRI(&V[i], &V[i+2], &V[i+3]);
            }
            break;
        case GL_QUAD_STRIP:
            for (i = 0; i + 4 <= n; i += 2)
            {
                EMIT_TRI(&V[i],   &V[i+1], &V[i+2]);
                EMIT_TRI(&V[i+2], &V[i+1], &V[i+3]);
            }
            break;
        case GL_POINTS:
        case GL_LINES:
        case GL_LINE_STRIP:
        case GL_LINE_LOOP:
        {
            /* Points/lines pass through to the GPU with the matching topology
             * (no triangle expansion).  GL_LINE_LOOP gets a closing vertex. */
            ULONG topo = (c->primitive == GL_POINTS)    ? NV2A_TOPO_POINTS :
                         (c->primitive == GL_LINES)     ? NV2A_TOPO_LINES  :
                         (c->primitive == GL_LINE_LOOP) ? NV2A_TOPO_LINE_LOOP
                                                        : NV2A_TOPO_LINE_STRIP;
            for (i = 0; i < n; i++)
            {
                NV2A_3D_VERTEX t;
                if (XformObj(c, &V[i], ox, oy, surfW, surfH, &t))
                {
                    if (bn >= NV2A_3D_MAX_VERTS) { SubmitBatch(c, batch, bn, 0, topo); bn = 0; }
                    batch[bn++] = t;
                }
            }
            if (c->primitive == GL_LINE_LOOP && bn > 0 && bn < NV2A_3D_MAX_VERTS)
            {
                NV2A_3D_VERTEX t;
                if (XformObj(c, &V[0], ox, oy, surfW, surfH, &t))
                    batch[bn++] = t;
            }
            if (bn > 0) SubmitBatch(c, batch, bn, 0, topo);
            bn = 0;
            break;
        }
        default:
            break;
    }
#undef EMIT_TRI

    if (bn > 0)
        SubmitBatch(c, batch, bn, 0, NV2A_TOPO_TRIANGLES);
}

/* ===================================================================== */
/* Type-variant entry points (thin wrappers over the canonical forms)    */
/* ===================================================================== */

/* Vertex */
void GLAPIENTRY xgl_real_Vertex2d(GLdouble x, GLdouble y){ xgl_real_Vertex2f((float)x,(float)y); }
void GLAPIENTRY xgl_real_Vertex3d(GLdouble x, GLdouble y, GLdouble z){ xgl_real_Vertex3f((float)x,(float)y,(float)z); }
void GLAPIENTRY xgl_real_Vertex2s(GLshort x, GLshort y){ xgl_real_Vertex2f((float)x,(float)y); }
void GLAPIENTRY xgl_real_Vertex3s(GLshort x, GLshort y, GLshort z){ xgl_real_Vertex3f((float)x,(float)y,(float)z); }
void GLAPIENTRY xgl_real_Vertex4i(GLint x, GLint y, GLint z, GLint w){ xgl_real_Vertex4f((float)x,(float)y,(float)z,(float)w); }
void GLAPIENTRY xgl_real_Vertex2dv(const GLdouble *v){ if(v) xgl_real_Vertex2f((float)v[0],(float)v[1]); }
void GLAPIENTRY xgl_real_Vertex3dv(const GLdouble *v){ if(v) xgl_real_Vertex3f((float)v[0],(float)v[1],(float)v[2]); }
void GLAPIENTRY xgl_real_Vertex4fv(const GLfloat *v){ if(v) xgl_real_Vertex4f(v[0],v[1],v[2],v[3]); }

/* Color */
void GLAPIENTRY xgl_real_Color4d(GLdouble r, GLdouble g, GLdouble b, GLdouble a){ xgl_real_Color4f((float)r,(float)g,(float)b,(float)a); }
void GLAPIENTRY xgl_real_Color3b(GLbyte r, GLbyte g, GLbyte b){ xgl_real_Color4f(r/127.0f,g/127.0f,b/127.0f,1.0f); }
void GLAPIENTRY xgl_real_Color4b(GLbyte r, GLbyte g, GLbyte b, GLbyte a){ xgl_real_Color4f(r/127.0f,g/127.0f,b/127.0f,a/127.0f); }
void GLAPIENTRY xgl_real_Color4dv(const GLdouble *v){ if(v) xgl_real_Color4f((float)v[0],(float)v[1],(float)v[2],(float)v[3]); }
void GLAPIENTRY xgl_real_Color3dv(const GLdouble *v){ if(v) xgl_real_Color4f((float)v[0],(float)v[1],(float)v[2],1.0f); }

/* Normal */
void GLAPIENTRY xgl_real_Normal3d(GLdouble x, GLdouble y, GLdouble z){ xgl_real_Normal3f((float)x,(float)y,(float)z); }
void GLAPIENTRY xgl_real_Normal3b(GLbyte x, GLbyte y, GLbyte z){ xgl_real_Normal3f(x/127.0f,y/127.0f,z/127.0f); }
void GLAPIENTRY xgl_real_Normal3dv(const GLdouble *v){ if(v) xgl_real_Normal3f((float)v[0],(float)v[1],(float)v[2]); }

/* TexCoord (we sample 2D, so only s,t matter) */
void GLAPIENTRY xgl_real_TexCoord1f(GLfloat s){ xgl_real_TexCoord2f(s,0.0f); }
void GLAPIENTRY xgl_real_TexCoord3f(GLfloat s, GLfloat t, GLfloat r){ (void)r; xgl_real_TexCoord2f(s,t); }
void GLAPIENTRY xgl_real_TexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q){ (void)r; (void)q; xgl_real_TexCoord2f(s,t); }
void GLAPIENTRY xgl_real_TexCoord2d(GLdouble s, GLdouble t){ xgl_real_TexCoord2f((float)s,(float)t); }
void GLAPIENTRY xgl_real_TexCoord1d(GLdouble s){ xgl_real_TexCoord2f((float)s,0.0f); }
void GLAPIENTRY xgl_real_TexCoord3fv(const GLfloat *v){ if(v) xgl_real_TexCoord2f(v[0],v[1]); }
void GLAPIENTRY xgl_real_TexCoord4fv(const GLfloat *v){ if(v) xgl_real_TexCoord2f(v[0],v[1]); }

/* glRect: draws a rectangle as a quad (GL spec: as a filled polygon). */
void GLAPIENTRY xgl_real_Rectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
    xgl_real_Begin(GL_QUADS);
    xgl_real_Vertex2f(x1,y1); xgl_real_Vertex2f(x2,y1);
    xgl_real_Vertex2f(x2,y2); xgl_real_Vertex2f(x1,y2);
    xgl_real_End();
}
void GLAPIENTRY xgl_real_Recti(GLint x1, GLint y1, GLint x2, GLint y2){ xgl_real_Rectf((float)x1,(float)y1,(float)x2,(float)y2); }
void GLAPIENTRY xgl_real_Rects(GLshort x1, GLshort y1, GLshort x2, GLshort y2){ xgl_real_Rectf((float)x1,(float)y1,(float)x2,(float)y2); }
void GLAPIENTRY xgl_real_Rectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2){ xgl_real_Rectf((float)x1,(float)y1,(float)x2,(float)y2); }

void GLAPIENTRY xgl_real_PointSize(GLfloat size){ PXGL_CONTEXT c = XglCurrent(); if (c && size > 0.0f) c->pointSize = size; }
void GLAPIENTRY xgl_real_LineWidth(GLfloat width){ PXGL_CONTEXT c = XglCurrent(); if (c && width > 0.0f) c->lineWidth = width; }  /* NV2A 3D lines are 1px; tracked only */

/* glPushAttrib/glPopAttrib — snapshot/restore the common enable + render state
 * (ENABLE/DEPTH/COLOR/POLYGON/POINT/LINE/CURRENT bits).  mask is ignored: the
 * full snapshot is saved and restored, which is correct for symmetric push/pop. */
void GLAPIENTRY xgl_real_PushAttrib(GLbitfield mask)
{
    PXGL_CONTEXT c = XglCurrent(); XGL_ATTRIB *a;
    (void)mask;
    if (!c || c->attribTop >= XGL_ATTRIB_STACK) return;
    a = &c->attribStack[c->attribTop++];
    a->enDepthTest=c->enDepthTest; a->enBlend=c->enBlend; a->enCullFace=c->enCullFace;
    a->enTexture2D=c->enTexture2D; a->enScissor=c->enScissor; a->enLighting=c->enLighting;
    a->enFog=c->enFog; a->enAlphaTest=c->enAlphaTest;
    a->depthFunc=c->depthFunc; a->depthMask=c->depthMask; a->shadeModel=c->shadeModel;
    a->cullFaceMode=c->cullFaceMode; a->frontFace=c->frontFace;
    a->blendSrc=c->blendSrc; a->blendDst=c->blendDst; a->polygonMode=c->polygonMode;
    a->alphaFunc=c->alphaFunc; a->alphaRef=c->alphaRef;
    a->pointSize=c->pointSize; a->lineWidth=c->lineWidth;
    memcpy(a->curColor, c->curColor, sizeof(a->curColor));
}
void GLAPIENTRY xgl_real_PopAttrib(void)
{
    PXGL_CONTEXT c = XglCurrent(); const XGL_ATTRIB *a;
    if (!c || c->attribTop == 0) return;
    a = &c->attribStack[--c->attribTop];
    c->enDepthTest=a->enDepthTest; c->enBlend=a->enBlend; c->enCullFace=a->enCullFace;
    c->enTexture2D=a->enTexture2D; c->enScissor=a->enScissor; c->enLighting=a->enLighting;
    c->enFog=a->enFog; c->enAlphaTest=a->enAlphaTest;
    c->depthFunc=a->depthFunc; c->depthMask=a->depthMask; c->shadeModel=a->shadeModel;
    c->cullFaceMode=a->cullFaceMode; c->frontFace=a->frontFace;
    c->blendSrc=a->blendSrc; c->blendDst=a->blendDst; c->polygonMode=a->polygonMode;
    c->alphaFunc=a->alphaFunc; c->alphaRef=a->alphaRef;
    c->pointSize=a->pointSize; c->lineWidth=a->lineWidth;
    memcpy(c->curColor, a->curColor, sizeof(c->curColor));
}

/* ===================================================================== */
/* glClear / glClearColor / glClearDepth                                 */
/* ===================================================================== */

void GLAPIENTRY xgl_real_ClearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a)
{
    PXGL_CONTEXT c = XglCurrent();
    if (!c) return;
    c->clearColor[0]=r; c->clearColor[1]=g; c->clearColor[2]=b; c->clearColor[3]=a;
}

void GLAPIENTRY xgl_real_ClearDepth(GLclampd d)
{
    PXGL_CONTEXT c = XglCurrent();
    if (!c) return;
    c->clearDepth = (float)d;
}

void GLAPIENTRY xgl_real_Clear(GLbitfield mask)
{
    PXGL_CONTEXT c = XglCurrent();
    if (!c) return;
    if (mask & GL_COLOR_BUFFER_BIT)
    {
        c->clearPacked = ((ULONG)F2B(c->clearColor[0]) << 16) |
                         ((ULONG)F2B(c->clearColor[1]) << 8)  |
                          (ULONG)F2B(c->clearColor[2]);
        c->clearPending = TRUE;   /* applied to the offscreen with the next batch */
    }
    if (mask & GL_DEPTH_BUFFER_BIT)
        c->clearDepthPending = TRUE;  /* rides the next batch, clears the zeta buffer */
    if (mask & GL_STENCIL_BUFFER_BIT)
        c->clearStencilPending = TRUE;  /* rides the next batch (Z24S8 stencil) */
    if ((mask & GL_ACCUM_BUFFER_BIT) && c->accumBuf)
    {
        int i, n = c->accumW * c->accumH;
        for (i = 0; i < n; i++)
        {
            c->accumBuf[i*4]   = c->accumClear[0]; c->accumBuf[i*4+1] = c->accumClear[1];
            c->accumBuf[i*4+2] = c->accumClear[2]; c->accumBuf[i*4+3] = c->accumClear[3];
        }
    }
}

/* ===================================================================== */
/* Display-list recording                                                */
/* ===================================================================== */
/* A list is a flat float buffer of [opcode, args...] records.  Recording
 * happens at the public entry points (below); replay (xgl_real_CallList) feeds
 * the same entry points with listRecording==0 so they execute normally. */
#define DL_END        1.0f   /* (no args)        */
#define DL_BEGIN      2.0f   /* + mode           */
#define DL_VERTEX     3.0f   /* + x,y,z,w        */
#define DL_NORMAL     4.0f   /* + x,y,z          */
#define DL_COLOR      5.0f   /* + r,g,b,a        */
#define DL_SHADEMODEL 6.0f   /* + mode           */
#define DL_MATERIAL   7.0f   /* + face,pname,p0,p1,p2,p3 */
#define DL_TEXCOORD   8.0f   /* + s,t            */

static void DlAppend(PXGL_CONTEXT c, const float *vals, int n)
{
    XGL_DLIST *L;
    if (c->listRecording == 0 || c->listRecording >= XGL_MAX_LISTS)
        return;
    L = &c->lists[c->listRecording];
    if (L->count + n > L->cap)
    {
        int ncap = L->cap ? L->cap * 2 : 256;
        float *nd;
        while (ncap < L->count + n) ncap *= 2;
        nd = L->data ? (float *)HeapReAlloc(GetProcessHeap(), 0, L->data, ncap * sizeof(float))
                     : (float *)HeapAlloc(GetProcessHeap(), 0, ncap * sizeof(float));
        if (!nd) return;
        L->data = nd; L->cap = ncap;
    }
    memcpy(L->data + L->count, vals, n * sizeof(float));
    L->count += n;
}

/* TRUE if the call should NOT also execute (pure GL_COMPILE recording). */
#define DL_RECORDING(c)  ((c)->listRecording != 0)
#define DL_SKIP_EXEC(c)  ((c)->listRecording != 0 && (c)->listMode == GL_COMPILE)

/* ===================================================================== */
/* Immediate-mode current attributes                                     */
/* ===================================================================== */

void GLAPIENTRY xgl_real_Color4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    PXGL_CONTEXT c = XglCurrent();
    if (!c) return;
    if (DL_RECORDING(c)) { float t[5]={DL_COLOR,r,g,b,a}; DlAppend(c,t,5); if (DL_SKIP_EXEC(c)) return; }
    c->curColor[0]=r; c->curColor[1]=g; c->curColor[2]=b; c->curColor[3]=a;
    /* GL_COLOR_MATERIAL: while enabled, glColor tracks the material parameter
     * selected by glColorMaterial (default GL_AMBIENT_AND_DIFFUSE) so lit geometry
     * can be coloured per-vertex with glColor instead of glMaterial. */
    if (c->enColorMaterial)
    {
        float col[4] = { r, g, b, a };
        switch (c->colorMaterialMode)
        {
            case GL_AMBIENT:  memcpy(c->matAmbient, col, sizeof(col)); break;
            case GL_DIFFUSE:  memcpy(c->matDiffuse, col, sizeof(col)); break;
            case GL_SPECULAR: memcpy(c->matSpecular, col, sizeof(col)); break;
            case GL_EMISSION: memcpy(c->matEmission, col, sizeof(col)); break;
            case GL_AMBIENT_AND_DIFFUSE:
            default: memcpy(c->matAmbient, col, sizeof(col)); memcpy(c->matDiffuse, col, sizeof(col)); break;
        }
    }
}
void GLAPIENTRY xgl_real_Color3f(GLfloat r, GLfloat g, GLfloat b)
{ xgl_real_Color4f(r, g, b, 1.0f); }
void GLAPIENTRY xgl_real_Color4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a)
{ xgl_real_Color4f(r/255.0f, g/255.0f, b/255.0f, a/255.0f); }
void GLAPIENTRY xgl_real_Color3ub(GLubyte r, GLubyte g, GLubyte b)
{ xgl_real_Color4f(r/255.0f, g/255.0f, b/255.0f, 1.0f); }
void GLAPIENTRY xgl_real_Color4fv(const GLfloat *v) { if (v) xgl_real_Color4f(v[0],v[1],v[2],v[3]); }
void GLAPIENTRY xgl_real_Color3fv(const GLfloat *v) { if (v) xgl_real_Color4f(v[0],v[1],v[2],1.0f); }
void GLAPIENTRY xgl_real_Color3ubv(const GLubyte *v){ if (v) xgl_real_Color3ub(v[0],v[1],v[2]); }
void GLAPIENTRY xgl_real_Color4ubv(const GLubyte *v){ if (v) xgl_real_Color4ub(v[0],v[1],v[2],v[3]); }
void GLAPIENTRY xgl_real_Color3d(GLdouble r, GLdouble g, GLdouble b)
{ xgl_real_Color4f((float)r,(float)g,(float)b,1.0f); }

void GLAPIENTRY xgl_real_Normal3f(GLfloat x, GLfloat y, GLfloat z)
{
    PXGL_CONTEXT c = XglCurrent();
    if (!c) return;
    if (DL_RECORDING(c)) { float t[4]={DL_NORMAL,x,y,z}; DlAppend(c,t,4); if (DL_SKIP_EXEC(c)) return; }
    c->curNormal[0]=x; c->curNormal[1]=y; c->curNormal[2]=z;
}
void GLAPIENTRY xgl_real_Normal3fv(const GLfloat *v){ if (v) xgl_real_Normal3f(v[0],v[1],v[2]); }

void GLAPIENTRY xgl_real_TexCoord2f(GLfloat s, GLfloat t)
{
    PXGL_CONTEXT c = XglCurrent();
    if (!c) return;
    /* Record into a display list (all TexCoord* variants funnel through here).
     * Apps that build textured geometry into a glNewList (e.g. mcclone's per-chunk
     * lists) rely on the per-vertex texcoord being replayed; without this every
     * replayed vertex reused one stale texcoord and the texture collapsed to a
     * single texel (surfaces looked flat/untextured). */
    if (DL_RECORDING(c)) { float td[3]={DL_TEXCOORD,s,t}; DlAppend(c,td,3); if (DL_SKIP_EXEC(c)) return; }
    c->curTexCoord[0]=s; c->curTexCoord[1]=t; c->curTexCoord[2]=0; c->curTexCoord[3]=1;
}
void GLAPIENTRY xgl_real_TexCoord2fv(const GLfloat *v){ if (v) xgl_real_TexCoord2f(v[0],v[1]); }
void GLAPIENTRY xgl_real_TexCoord2i(GLint s, GLint t){ xgl_real_TexCoord2f((float)s,(float)t); }

/* ===================================================================== */
/* glBegin / glVertex / glEnd                                            */
/* ===================================================================== */

void GLAPIENTRY xgl_real_Begin(GLenum mode)
{
    PXGL_CONTEXT c = XglCurrent();
    if (!c) return;
    if (DL_RECORDING(c)) { float t[2]={DL_BEGIN,(float)mode}; DlAppend(c,t,2); if (DL_SKIP_EXEC(c)) return; }
    c->primitive = mode;
    c->vertCount = 0;
}

static void EmitVertex(PXGL_CONTEXT c, float x, float y, float z, float w)
{
    XGL_VERTEX *v;
    if (c->vertCount >= XGL_VERTEX_BUFFER_CAP)
    {
        /* Buffer full mid-primitive.  The independent primitive types can be
         * split at a primitive boundary: flush what we have and keep going, so a
         * single huge glBegin/glEnd (e.g. an mcclone chunk emits thousands of
         * GL_QUADS verts in ONE batch) doesn't lose every face past 1024 verts.
         * Strips/fans/polygon can't be split without losing continuity, so they
         * still cap (rare in practice). */
        int boundary;
        switch (c->primitive) {
            case GL_QUADS:     boundary = (c->vertCount & 3) == 0; break;
            case GL_TRIANGLES: boundary = (c->vertCount % 3) == 0; break;
            case GL_LINES:     boundary = (c->vertCount & 1) == 0; break;
            case GL_POINTS:    boundary = 1; break;
            default:           boundary = 0; break;
        }
        if (!boundary)
            return;
        FlushPrimitive(c);
        c->vertCount = 0;
    }
    v = &c->verts[c->vertCount++];
    v->x=x; v->y=y; v->z=z; v->w=w;
    v->r=c->curColor[0]; v->g=c->curColor[1]; v->b=c->curColor[2]; v->a=c->curColor[3];
    v->nx=c->curNormal[0]; v->ny=c->curNormal[1]; v->nz=c->curNormal[2];
    v->s=c->curTexCoord[0]; v->t=c->curTexCoord[1];
}

/* Common vertex path: record into the open list, otherwise accumulate. */
static void VertexImpl(PXGL_CONTEXT c, float x, float y, float z, float w)
{
    if (DL_RECORDING(c)) { float t[5]={DL_VERTEX,x,y,z,w}; DlAppend(c,t,5); if (DL_SKIP_EXEC(c)) return; }
    EmitVertex(c, x, y, z, w);
}

void GLAPIENTRY xgl_real_Vertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{ PXGL_CONTEXT c = XglCurrent(); if (c) VertexImpl(c, x, y, z, w); }
void GLAPIENTRY xgl_real_Vertex3f(GLfloat x, GLfloat y, GLfloat z)
{ PXGL_CONTEXT c = XglCurrent(); if (c) VertexImpl(c, x, y, z, 1.0f); }
void GLAPIENTRY xgl_real_Vertex2f(GLfloat x, GLfloat y)
{ PXGL_CONTEXT c = XglCurrent(); if (c) VertexImpl(c, x, y, 0.0f, 1.0f); }
void GLAPIENTRY xgl_real_Vertex2i(GLint x, GLint y)
{ PXGL_CONTEXT c = XglCurrent(); if (c) VertexImpl(c, (float)x, (float)y, 0.0f, 1.0f); }
void GLAPIENTRY xgl_real_Vertex3i(GLint x, GLint y, GLint z)
{ PXGL_CONTEXT c = XglCurrent(); if (c) VertexImpl(c, (float)x, (float)y, (float)z, 1.0f); }
void GLAPIENTRY xgl_real_Vertex3fv(const GLfloat *v)
{ PXGL_CONTEXT c = XglCurrent(); if (c && v) VertexImpl(c, v[0], v[1], v[2], 1.0f); }
void GLAPIENTRY xgl_real_Vertex2fv(const GLfloat *v)
{ PXGL_CONTEXT c = XglCurrent(); if (c && v) VertexImpl(c, v[0], v[1], 0.0f, 1.0f); }

void GLAPIENTRY xgl_real_End(void)
{
    PXGL_CONTEXT c = XglCurrent();
    if (!c) return;
    if (DL_RECORDING(c)) { float t[1]={DL_END}; DlAppend(c,t,1); if (DL_SKIP_EXEC(c)) return; }
    if (c->primitive == 0) return;
    FlushPrimitive(c);
    c->primitive = 0;
    c->vertCount = 0;
}

/* ===================================================================== */
/* Vertex arrays (GL 1.1)                                                */
/* ===================================================================== */

void GLAPIENTRY xgl_real_VertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *p)
{ PXGL_CONTEXT c = XglCurrent(); if (!c) return; c->arrVertex.size=size; c->arrVertex.type=type; c->arrVertex.stride=stride; c->arrVertex.ptr=p; }
void GLAPIENTRY xgl_real_ColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *p)
{ PXGL_CONTEXT c = XglCurrent(); if (!c) return; c->arrColor.size=size; c->arrColor.type=type; c->arrColor.stride=stride; c->arrColor.ptr=p; }
void GLAPIENTRY xgl_real_NormalPointer(GLenum type, GLsizei stride, const GLvoid *p)
{ PXGL_CONTEXT c = XglCurrent(); if (!c) return; c->arrNormal.size=3; c->arrNormal.type=type; c->arrNormal.stride=stride; c->arrNormal.ptr=p; }
void GLAPIENTRY xgl_real_TexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *p)
{ PXGL_CONTEXT c = XglCurrent(); if (!c) return; c->arrTexCoord.size=size; c->arrTexCoord.type=type; c->arrTexCoord.stride=stride; c->arrTexCoord.ptr=p; }

void GLAPIENTRY xgl_real_EnableClientState(GLenum a)
{
    PXGL_CONTEXT c = XglCurrent(); if (!c) return;
    switch (a) {
        case GL_VERTEX_ARRAY:        c->arrVertex.enabled = TRUE; break;
        case GL_COLOR_ARRAY:         c->arrColor.enabled = TRUE; break;
        case GL_NORMAL_ARRAY:        c->arrNormal.enabled = TRUE; break;
        case GL_TEXTURE_COORD_ARRAY: c->arrTexCoord.enabled = TRUE; break;
    }
}
void GLAPIENTRY xgl_real_DisableClientState(GLenum a)
{
    PXGL_CONTEXT c = XglCurrent(); if (!c) return;
    switch (a) {
        case GL_VERTEX_ARRAY:        c->arrVertex.enabled = FALSE; break;
        case GL_COLOR_ARRAY:         c->arrColor.enabled = FALSE; break;
        case GL_NORMAL_ARRAY:        c->arrNormal.enabled = FALSE; break;
        case GL_TEXTURE_COORD_ARRAY: c->arrTexCoord.enabled = FALSE; break;
    }
}

static float ReadComponent(const XGL_ARRAY *a, int index, int comp)
{
    const BYTE *base;
    GLsizei stride = a->stride;
    if (stride == 0)
    {
        int bytes = (a->type == GL_UNSIGNED_BYTE) ? 1 : 4;
        stride = a->size * bytes;
    }
    base = (const BYTE*)a->ptr + (SIZE_T)index * stride;
    switch (a->type)
    {
        case GL_FLOAT:         return ((const float*)base)[comp];
        case GL_UNSIGNED_BYTE: return ((const GLubyte*)base)[comp] / 255.0f;
        case GL_INT:           return (float)((const GLint*)base)[comp];
        case GL_SHORT:         return (float)((const GLshort*)base)[comp];
        case GL_DOUBLE:        return (float)((const double*)base)[comp];
        default:               return 0.0f;
    }
}

static void FetchArrayVertex(PXGL_CONTEXT c, int index, XGL_VERTEX *v)
{
    v->x = v->y = v->z = 0.0f; v->w = 1.0f;
    v->r = c->curColor[0]; v->g = c->curColor[1]; v->b = c->curColor[2]; v->a = c->curColor[3];
    v->nx = c->curNormal[0]; v->ny = c->curNormal[1]; v->nz = c->curNormal[2];
    v->s = c->curTexCoord[0]; v->t = c->curTexCoord[1];

    if (c->arrColor.enabled && c->arrColor.ptr)
    {
        v->r = ReadComponent(&c->arrColor, index, 0);
        v->g = ReadComponent(&c->arrColor, index, 1);
        v->b = ReadComponent(&c->arrColor, index, 2);
        v->a = (c->arrColor.size >= 4) ? ReadComponent(&c->arrColor, index, 3) : 1.0f;
    }
    if (c->arrTexCoord.enabled && c->arrTexCoord.ptr)
    {
        v->s = ReadComponent(&c->arrTexCoord, index, 0);
        v->t = (c->arrTexCoord.size >= 2) ? ReadComponent(&c->arrTexCoord, index, 1) : 0.0f;
    }
    if (c->arrVertex.enabled && c->arrVertex.ptr)
    {
        v->x = ReadComponent(&c->arrVertex, index, 0);
        v->y = ReadComponent(&c->arrVertex, index, 1);
        v->z = (c->arrVertex.size >= 3) ? ReadComponent(&c->arrVertex, index, 2) : 0.0f;
        v->w = (c->arrVertex.size >= 4) ? ReadComponent(&c->arrVertex, index, 3) : 1.0f;
    }
}

/* Push one array vertex through immediate mode.  This is deliberate: when a list
 * is open the funnel records the dereferenced geometry, so glDrawElements inside
 * glNewList bakes correctly (ClassiCube builds its chunks that way).  Emit only
 * the attributes whose array is enabled. */
static void EmitArrayElement(PXGL_CONTEXT c, int index)
{
    XGL_VERTEX v;
    FetchArrayVertex(c, index, &v);
    if (c->arrColor.enabled    && c->arrColor.ptr)    xgl_real_Color4f(v.r, v.g, v.b, v.a);
    if (c->arrNormal.enabled   && c->arrNormal.ptr)   xgl_real_Normal3f(v.nx, v.ny, v.nz);
    if (c->arrTexCoord.enabled && c->arrTexCoord.ptr) xgl_real_TexCoord2f(v.s, v.t);
    xgl_real_Vertex4f(v.x, v.y, v.z, v.w);
}

/* count elements: sequential from `first` if indices==NULL, else indexed by type. */
static void DrawArrayRange(PXGL_CONTEXT c, GLenum mode, GLenum type,
                           const GLvoid *indices, GLint first, GLsizei count)
{
    GLsizei i;
    xgl_real_Begin(mode);
    for (i = 0; i < count; i++)
    {
        GLint idx;
        if (!indices)                       idx = first + i;
        else if (type == GL_UNSIGNED_BYTE)  idx = ((const GLubyte*)indices)[i];
        else if (type == GL_UNSIGNED_SHORT) idx = ((const GLushort*)indices)[i];
        else                                idx = (GLint)((const GLuint*)indices)[i];
        EmitArrayElement(c, idx);
    }
    xgl_real_End();
}

void GLAPIENTRY xgl_real_DrawArrays(GLenum mode, GLint first, GLsizei count)
{
    PXGL_CONTEXT c = XglCurrent();
    if (!c || !c->arrVertex.enabled || count <= 0) return;
    DrawArrayRange(c, mode, 0, NULL, first, count);
}

void GLAPIENTRY xgl_real_DrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
    PXGL_CONTEXT c = XglCurrent();
    if (!c || !c->arrVertex.enabled || !indices || count <= 0) return;
    DrawArrayRange(c, mode, type, indices, 0, count);
}

/* GL 1.2 glDrawRangeElements: identical to glDrawElements; the start/end range
 * is only an optimisation hint for the implementation. */
void GLAPIENTRY xgl_real_DrawRangeElements(GLenum mode, GLuint start, GLuint end,
                                           GLsizei count, GLenum type, const GLvoid *indices)
{
    (void)start; (void)end;
    xgl_real_DrawElements(mode, count, type, indices);
}

/* ===================================================================== */
/* Matrix stack                                                          */
/* ===================================================================== */

void GLAPIENTRY xgl_real_MatrixMode(GLenum mode)
{
    PXGL_CONTEXT c = XglCurrent();
    if (!c) return;
    switch (mode)
    {
        case GL_MODELVIEW:  c->matMode = XGL_MAT_MODELVIEW;  break;
        case GL_PROJECTION: c->matMode = XGL_MAT_PROJECTION; break;
        case GL_TEXTURE:    c->matMode = XGL_MAT_TEXTURE;    break;
        default: SetError(c, GL_INVALID_ENUM); return;
    }
    c->matrixMode = mode;
}

void GLAPIENTRY xgl_real_LoadIdentity(void)
{ PXGL_CONTEXT c = XglCurrent(); if (c) XglMatIdentity(CurMat(c)); }

void GLAPIENTRY xgl_real_LoadMatrixf(const GLfloat *m)
{ PXGL_CONTEXT c = XglCurrent(); if (c && m) memcpy(CurMat(c)->m, m, sizeof(float)*16); }

void GLAPIENTRY xgl_real_MultMatrixf(const GLfloat *m)
{ PXGL_CONTEXT c = XglCurrent(); XGL_MAT4 t; if (c && m) { memcpy(t.m, m, sizeof(float)*16); MulCurrentBy(c, &t); } }

void GLAPIENTRY xgl_real_PushMatrix(void)
{
    PXGL_CONTEXT c = XglCurrent();
    if (!c) return;
    if (c->matTop[c->matMode] + 1 < XGL_MATRIX_STACK_DEPTH)
    {
        c->matStack[c->matMode][c->matTop[c->matMode]+1] = c->matStack[c->matMode][c->matTop[c->matMode]];
        c->matTop[c->matMode]++;
    }
    else SetError(c, GL_STACK_OVERFLOW);
}

void GLAPIENTRY xgl_real_PopMatrix(void)
{
    PXGL_CONTEXT c = XglCurrent();
    if (!c) return;
    if (c->matTop[c->matMode] > 0) c->matTop[c->matMode]--;
    else SetError(c, GL_STACK_UNDERFLOW);
}

void GLAPIENTRY xgl_real_Ortho(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f)
{
    PXGL_CONTEXT c = XglCurrent();
    XGL_MAT4 m;
    if (!c) return;
    XglMatIdentity(&m);
    m.m[0]  = (float)(2.0/(r-l));
    m.m[5]  = (float)(2.0/(t-b));
    m.m[10] = (float)(-2.0/(f-n));
    m.m[12] = (float)(-(r+l)/(r-l));
    m.m[13] = (float)(-(t+b)/(t-b));
    m.m[14] = (float)(-(f+n)/(f-n));
    MulCurrentBy(c, &m);
}

void GLAPIENTRY xgl_real_Frustum(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f)
{
    PXGL_CONTEXT c = XglCurrent();
    XGL_MAT4 m;
    if (!c) return;
    memset(&m, 0, sizeof(m));
    m.m[0]  = (float)(2.0*n/(r-l));
    m.m[5]  = (float)(2.0*n/(t-b));
    m.m[8]  = (float)((r+l)/(r-l));
    m.m[9]  = (float)((t+b)/(t-b));
    m.m[10] = (float)(-(f+n)/(f-n));
    m.m[11] = -1.0f;
    m.m[14] = (float)(-2.0*f*n/(f-n));
    MulCurrentBy(c, &m);
}

void GLAPIENTRY xgl_real_Translatef(GLfloat x, GLfloat y, GLfloat z)
{
    PXGL_CONTEXT c = XglCurrent();
    XGL_MAT4 m;
    if (!c) return;
    XglMatIdentity(&m);
    m.m[12]=x; m.m[13]=y; m.m[14]=z;
    MulCurrentBy(c, &m);
}

void GLAPIENTRY xgl_real_Scalef(GLfloat x, GLfloat y, GLfloat z)
{
    PXGL_CONTEXT c = XglCurrent();
    XGL_MAT4 m;
    if (!c) return;
    XglMatIdentity(&m);
    m.m[0]=x; m.m[5]=y; m.m[10]=z;
    MulCurrentBy(c, &m);
}

void GLAPIENTRY xgl_real_Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    PXGL_CONTEXT c = XglCurrent();
    XGL_MAT4 m;
    float rad = angle * 3.14159265358979f / 180.0f;
    float s = sinf(rad), co = cosf(rad), one = 1.0f - co;
    float len = sqrtf(x*x + y*y + z*z);
    if (!c) return;
    if (len < 1e-8f) return;
    x/=len; y/=len; z/=len;
    XglMatIdentity(&m);
    m.m[0]=x*x*one+co;    m.m[4]=x*y*one-z*s;  m.m[8]=x*z*one+y*s;
    m.m[1]=y*x*one+z*s;   m.m[5]=y*y*one+co;   m.m[9]=y*z*one-x*s;
    m.m[2]=x*z*one-y*s;   m.m[6]=y*z*one+x*s;  m.m[10]=z*z*one+co;
    MulCurrentBy(c, &m);
}

void GLAPIENTRY xgl_real_LoadMatrixd(const GLdouble *m)
{ float f[16]; int i; if (!m) return; for (i=0;i<16;i++) f[i]=(float)m[i]; xgl_real_LoadMatrixf(f); }
void GLAPIENTRY xgl_real_MultMatrixd(const GLdouble *m)
{ float f[16]; int i; if (!m) return; for (i=0;i<16;i++) f[i]=(float)m[i]; xgl_real_MultMatrixf(f); }
void GLAPIENTRY xgl_real_Translated(GLdouble x, GLdouble y, GLdouble z){ xgl_real_Translatef((float)x,(float)y,(float)z); }
void GLAPIENTRY xgl_real_Scaled(GLdouble x, GLdouble y, GLdouble z){ xgl_real_Scalef((float)x,(float)y,(float)z); }
void GLAPIENTRY xgl_real_Rotated(GLdouble a, GLdouble x, GLdouble y, GLdouble z){ xgl_real_Rotatef((float)a,(float)x,(float)y,(float)z); }

/* ===================================================================== */
/* Viewport / depth range / scissor                                      */
/* ===================================================================== */

void GLAPIENTRY xgl_real_Viewport(GLint x, GLint y, GLsizei w, GLsizei h)
{
    PXGL_CONTEXT c = XglCurrent();
    if (!c) return;
    c->vpX=x; c->vpY=y; c->vpW=w; c->vpH=h;
}
void GLAPIENTRY xgl_real_DepthRange(GLclampd n, GLclampd f)
{ PXGL_CONTEXT c = XglCurrent(); if (!c) return; c->depthNear=(float)n; c->depthFar=(float)f; }
void GLAPIENTRY xgl_real_Scissor(GLint x, GLint y, GLsizei w, GLsizei h)
{ PXGL_CONTEXT c = XglCurrent(); if (!c) return; c->scissorX=x; c->scissorY=y; c->scissorW=w; c->scissorH=h; }

/* ===================================================================== */
/* Capability + render state (tracked; HW application is a follow-up)    */
/* ===================================================================== */

void GLAPIENTRY xgl_real_Enable(GLenum cap)
{
    PXGL_CONTEXT c = XglCurrent(); if (!c) return;
    switch (cap) {
        case GL_DEPTH_TEST:   c->enDepthTest = TRUE; break;
        case GL_BLEND:        c->enBlend = TRUE; break;
        case GL_CULL_FACE:    c->enCullFace = TRUE; break;
        case GL_TEXTURE_2D:   c->enTexture2D = TRUE; break;
        case GL_SCISSOR_TEST: c->enScissor = TRUE; break;
        case GL_LIGHTING:     c->enLighting = TRUE; break;
        case GL_FOG:          c->enFog = TRUE; break;
        case GL_ALPHA_TEST:   c->enAlphaTest = TRUE; break;
        case GL_RESCALE_NORMAL:
        case GL_NORMALIZE:    c->enRescaleNormal = TRUE; break; /* T&L always normalises */
        case GL_POLYGON_OFFSET_FILL:  c->enPolyOffsetFill = TRUE; break;
        case GL_POLYGON_OFFSET_LINE:  c->enPolyOffsetLine = TRUE; break;
        case GL_POLYGON_OFFSET_POINT: c->enPolyOffsetPoint = TRUE; break;
        case GL_COLOR_MATERIAL:       c->enColorMaterial = TRUE; break;
        case GL_TEXTURE_1D:           c->enTexture2D = TRUE; break;   /* 1D stored as h=1 2D */
        case GL_AUTO_NORMAL:          c->enAutoNormal = TRUE; break;
        case GL_STENCIL_TEST:         c->enStencilTest = TRUE; break; /* tracked (no stencil bits) */
        case GL_COLOR_LOGIC_OP:
        case GL_INDEX_LOGIC_OP:       c->enColorLogicOp = TRUE; break;
        case GL_LINE_STIPPLE:         c->enLineStipple = TRUE; break;
        case GL_POLYGON_STIPPLE:      c->enPolygonStipple = TRUE; break;
        case GL_TEXTURE_GEN_S: case GL_TEXTURE_GEN_T:
        case GL_TEXTURE_GEN_R: case GL_TEXTURE_GEN_Q: c->texGenEnabled[cap - GL_TEXTURE_GEN_S] = TRUE; break;
        default:
            if (cap >= GL_LIGHT0 && cap < GL_LIGHT0 + XGL_MAX_LIGHTS)
                c->lights[cap - GL_LIGHT0].enabled = TRUE;
            else if (cap >= GL_CLIP_PLANE0 && cap < GL_CLIP_PLANE0 + XGL_MAX_CLIP_PLANES)
                c->clipPlaneEnabled[cap - GL_CLIP_PLANE0] = TRUE;
            else if (cap >= GL_MAP1_COLOR_4 && cap <= GL_MAP1_VERTEX_4)
                c->evalMap1Enabled[cap - GL_MAP1_COLOR_4] = TRUE;
            else if (cap >= GL_MAP2_COLOR_4 && cap <= GL_MAP2_VERTEX_4)
                c->evalMap2Enabled[cap - GL_MAP2_COLOR_4] = TRUE;
            break;
    }
}
void GLAPIENTRY xgl_real_Disable(GLenum cap)
{
    PXGL_CONTEXT c = XglCurrent(); if (!c) return;
    switch (cap) {
        case GL_DEPTH_TEST:   c->enDepthTest = FALSE; break;
        case GL_BLEND:        c->enBlend = FALSE; break;
        case GL_CULL_FACE:    c->enCullFace = FALSE; break;
        case GL_TEXTURE_2D:   c->enTexture2D = FALSE; break;
        case GL_SCISSOR_TEST: c->enScissor = FALSE; break;
        case GL_LIGHTING:     c->enLighting = FALSE; break;
        case GL_FOG:          c->enFog = FALSE; break;
        case GL_ALPHA_TEST:   c->enAlphaTest = FALSE; break;
        case GL_RESCALE_NORMAL:
        case GL_NORMALIZE:    c->enRescaleNormal = FALSE; break;
        case GL_POLYGON_OFFSET_FILL:  c->enPolyOffsetFill = FALSE; break;
        case GL_POLYGON_OFFSET_LINE:  c->enPolyOffsetLine = FALSE; break;
        case GL_POLYGON_OFFSET_POINT: c->enPolyOffsetPoint = FALSE; break;
        case GL_COLOR_MATERIAL:       c->enColorMaterial = FALSE; break;
        case GL_TEXTURE_1D:           c->enTexture2D = FALSE; break;
        case GL_AUTO_NORMAL:          c->enAutoNormal = FALSE; break;
        case GL_STENCIL_TEST:         c->enStencilTest = FALSE; break;
        case GL_COLOR_LOGIC_OP:
        case GL_INDEX_LOGIC_OP:       c->enColorLogicOp = FALSE; break;
        case GL_LINE_STIPPLE:         c->enLineStipple = FALSE; break;
        case GL_POLYGON_STIPPLE:      c->enPolygonStipple = FALSE; break;
        case GL_TEXTURE_GEN_S: case GL_TEXTURE_GEN_T:
        case GL_TEXTURE_GEN_R: case GL_TEXTURE_GEN_Q: c->texGenEnabled[cap - GL_TEXTURE_GEN_S] = FALSE; break;
        default:
            if (cap >= GL_LIGHT0 && cap < GL_LIGHT0 + XGL_MAX_LIGHTS)
                c->lights[cap - GL_LIGHT0].enabled = FALSE;
            else if (cap >= GL_CLIP_PLANE0 && cap < GL_CLIP_PLANE0 + XGL_MAX_CLIP_PLANES)
                c->clipPlaneEnabled[cap - GL_CLIP_PLANE0] = FALSE;
            else if (cap >= GL_MAP1_COLOR_4 && cap <= GL_MAP1_VERTEX_4)
                c->evalMap1Enabled[cap - GL_MAP1_COLOR_4] = FALSE;
            else if (cap >= GL_MAP2_COLOR_4 && cap <= GL_MAP2_VERTEX_4)
                c->evalMap2Enabled[cap - GL_MAP2_COLOR_4] = FALSE;
            break;
    }
}
GLboolean GLAPIENTRY xgl_real_IsEnabled(GLenum cap)
{
    PXGL_CONTEXT c = XglCurrent(); if (!c) return GL_FALSE;
    switch (cap) {
        case GL_DEPTH_TEST:   return c->enDepthTest ? GL_TRUE : GL_FALSE;
        case GL_BLEND:        return c->enBlend ? GL_TRUE : GL_FALSE;
        case GL_CULL_FACE:    return c->enCullFace ? GL_TRUE : GL_FALSE;
        case GL_TEXTURE_2D:   return c->enTexture2D ? GL_TRUE : GL_FALSE;
        case GL_TEXTURE_1D:   return c->enTexture2D ? GL_TRUE : GL_FALSE;
        case GL_SCISSOR_TEST: return c->enScissor ? GL_TRUE : GL_FALSE;
        case GL_LIGHTING:     return c->enLighting ? GL_TRUE : GL_FALSE;
        case GL_FOG:          return c->enFog ? GL_TRUE : GL_FALSE;
        case GL_ALPHA_TEST:   return c->enAlphaTest ? GL_TRUE : GL_FALSE;
        case GL_COLOR_MATERIAL: return c->enColorMaterial ? GL_TRUE : GL_FALSE;
        case GL_STENCIL_TEST: return c->enStencilTest ? GL_TRUE : GL_FALSE;
        case GL_AUTO_NORMAL:  return c->enAutoNormal ? GL_TRUE : GL_FALSE;
        case GL_LINE_STIPPLE: return c->enLineStipple ? GL_TRUE : GL_FALSE;
        case GL_POLYGON_STIPPLE: return c->enPolygonStipple ? GL_TRUE : GL_FALSE;
        case GL_COLOR_LOGIC_OP:
        case GL_INDEX_LOGIC_OP: return c->enColorLogicOp ? GL_TRUE : GL_FALSE;
        case GL_TEXTURE_GEN_S: case GL_TEXTURE_GEN_T:
        case GL_TEXTURE_GEN_R: case GL_TEXTURE_GEN_Q:
            return c->texGenEnabled[cap - GL_TEXTURE_GEN_S] ? GL_TRUE : GL_FALSE;
        default:
            if (cap >= GL_LIGHT0 && cap < GL_LIGHT0 + XGL_MAX_LIGHTS)
                return c->lights[cap - GL_LIGHT0].enabled ? GL_TRUE : GL_FALSE;
            if (cap >= GL_CLIP_PLANE0 && cap < GL_CLIP_PLANE0 + XGL_MAX_CLIP_PLANES)
                return c->clipPlaneEnabled[cap - GL_CLIP_PLANE0] ? GL_TRUE : GL_FALSE;
            if (cap >= GL_MAP1_COLOR_4 && cap <= GL_MAP1_VERTEX_4)
                return c->evalMap1Enabled[cap - GL_MAP1_COLOR_4] ? GL_TRUE : GL_FALSE;
            if (cap >= GL_MAP2_COLOR_4 && cap <= GL_MAP2_VERTEX_4)
                return c->evalMap2Enabled[cap - GL_MAP2_COLOR_4] ? GL_TRUE : GL_FALSE;
            return GL_FALSE;
    }
}
void GLAPIENTRY xgl_real_DepthFunc(GLenum f){ PXGL_CONTEXT c = XglCurrent(); if (c) c->depthFunc=f; }
void GLAPIENTRY xgl_real_DepthMask(GLboolean m){ PXGL_CONTEXT c = XglCurrent(); if (c) c->depthMask=m; }
void GLAPIENTRY xgl_real_ShadeModel(GLenum m)
{
    PXGL_CONTEXT c = XglCurrent(); if (!c) return;
    if (DL_RECORDING(c)) { float t[2]={DL_SHADEMODEL,(float)m}; DlAppend(c,t,2); if (DL_SKIP_EXEC(c)) return; }
    c->shadeModel=m;
}
void GLAPIENTRY xgl_real_CullFace(GLenum m){ PXGL_CONTEXT c = XglCurrent(); if (c) c->cullFaceMode=m; }
void GLAPIENTRY xgl_real_FrontFace(GLenum m){ PXGL_CONTEXT c = XglCurrent(); if (c) c->frontFace=m; }
void GLAPIENTRY xgl_real_BlendFunc(GLenum s, GLenum d){ PXGL_CONTEXT c = XglCurrent(); if (c){ c->blendSrc=s; c->blendDst=d; } }
/* glHint is advisory only.  Our texturing is already perspective-correct and fog
 * is computed per-vertex, so GL_PERSPECTIVE_CORRECTION_HINT / GL_FOG_HINT (which
 * ClassiCube sets to GL_NICEST) are honoured implicitly — accept and ignore. */
void GLAPIENTRY xgl_real_Hint(GLenum target, GLenum mode){ (void)target; (void)mode; }

/* ===================================================================== */
/* Textures                                                              */
/* ===================================================================== */

/* Upload a texture's ARGB texels into the miniport's VRAM texture heap and cache
 * the returned GPU offset.  ARGB DWORDs (0xAARRGGBB) are exactly NV2A A8R8G8B8. */
static void XglUploadTexture(PXGL_CONTEXT c, XGL_TEXTURE *tx)
{
    SIZE_T hdr = sizeof(NV2A_TEX_UPLOAD);
    SIZE_T px;
    BYTE *buf;
    NV2A_TEX_UPLOAD *up;
    ULONG offset = 0;
    int ret;

    if (!c->hdc || !tx->texels || tx->width <= 0 || tx->height <= 0)
        return;
    px = (SIZE_T)tx->width * tx->height * sizeof(DWORD);
    buf = (BYTE*)HeapAlloc(GetProcessHeap(), 0, hdr + px);
    if (!buf) return;
    up = (NV2A_TEX_UPLOAD*)buf;
    up->Width  = (ULONG)tx->width;
    up->Height = (ULONG)tx->height;
    /* Reuse our prior VRAM slot when set (callers only keep gpuOffset when the
     * dimensions are unchanged), so re-spec / glTexSubImage2D doesn't leak. */
    up->ExistingOffset = tx->gpuOffset;
    memcpy(buf + hdr, tx->texels, px);

    ret = ExtEscape(c->hdc, XBOX_ESC_NV2A_TEXUPLOAD, (int)(hdr + px), (LPCSTR)buf,
                    sizeof(ULONG), (LPSTR)&offset);
    HeapFree(GetProcessHeap(), 0, buf);
    if (ret > 0 && offset != 0)
        tx->gpuOffset = offset;
}

void GLAPIENTRY xgl_real_GenTextures(GLsizei n, GLuint *out)
{
    PXGL_CONTEXT c = XglCurrent();
    GLsizei i;
    if (!c || !out) return;
    for (i = 0; i < n; i++)
    {
        GLuint name = c->nextTexture++;
        if (name >= XGL_MAX_TEXTURES) { out[i] = 0; continue; }
        c->textures[name].name = name;
        c->textures[name].used = TRUE;
        c->textures[name].minFilter = GL_NEAREST;
        c->textures[name].magFilter = GL_NEAREST;
        c->textures[name].wrapS = GL_REPEAT;
        c->textures[name].wrapT = GL_REPEAT;
        c->textures[name].envMode = GL_MODULATE;
        out[i] = name;
    }
}

void GLAPIENTRY xgl_real_DeleteTextures(GLsizei n, const GLuint *names)
{
    PXGL_CONTEXT c = XglCurrent();
    GLsizei i;
    if (!c || !names) return;
    for (i = 0; i < n; i++)
    {
        GLuint nm = names[i];
        if (nm < XGL_MAX_TEXTURES && c->textures[nm].used)
        {
            if (c->textures[nm].texels) HeapFree(GetProcessHeap(), 0, c->textures[nm].texels);
            memset(&c->textures[nm], 0, sizeof(c->textures[nm]));
        }
    }
}

void GLAPIENTRY xgl_real_BindTexture(GLenum target, GLuint name)
{
    PXGL_CONTEXT c = XglCurrent();
    if (!c || (target != GL_TEXTURE_2D && target != GL_TEXTURE_1D)) return;
    if (name < XGL_MAX_TEXTURES)
    {
        if (!c->textures[name].used && name != 0)
        {
            c->textures[name].used = TRUE;
            c->textures[name].minFilter = GL_NEAREST;
            c->textures[name].magFilter = GL_NEAREST;
            c->textures[name].wrapS = GL_REPEAT;
            c->textures[name].wrapT = GL_REPEAT;
            c->textures[name].envMode = GL_MODULATE;
        }
        c->boundTexture = name;
    }
}

/* Decode one source image (w*h texels) of an arbitrary GL pixel format/type into
 * ARGB8888.  Handles GL_UNSIGNED_BYTE component data plus all the GL 1.2 packed
 * pixel types; BGR/BGRA formats swap R<->B after extraction (so e.g. the very
 * common BGRA + GL_UNSIGNED_INT_8_8_8_8_REV combo lands as native ARGB). */
static void XglDecodeImage(DWORD *dst, const GLubyte *src, int w, int h,
                           GLenum format, GLenum type)
{
    int comps = (format == GL_RGBA || format == GL_BGRA) ? 4 :
                (format == GL_RGB  || format == GL_BGR ) ? 3 :
                (format == GL_LUMINANCE_ALPHA) ? 2 : 1;
    int bgr = (format == GL_BGR || format == GL_BGRA);
    int packed = 1, psize, i, n = w * h;

    switch (type) {
        case GL_UNSIGNED_BYTE_3_3_2: case GL_UNSIGNED_BYTE_2_3_3_REV: psize = 1; break;
        case GL_UNSIGNED_SHORT_5_6_5: case GL_UNSIGNED_SHORT_5_6_5_REV:
        case GL_UNSIGNED_SHORT_4_4_4_4: case GL_UNSIGNED_SHORT_4_4_4_4_REV:
        case GL_UNSIGNED_SHORT_5_5_5_1: case GL_UNSIGNED_SHORT_1_5_5_5_REV: psize = 2; break;
        case GL_UNSIGNED_INT_8_8_8_8: case GL_UNSIGNED_INT_8_8_8_8_REV:
        case GL_UNSIGNED_INT_10_10_10_2: case GL_UNSIGNED_INT_2_10_10_10_REV: psize = 4; break;
        default: packed = 0; psize = comps; break;   /* GL_UNSIGNED_BYTE & fallback */
    }

    for (i = 0; i < n; i++)
    {
        unsigned r=255,g=255,b=255,a=255,t;
        if (!src) { dst[i] = 0xFFFFFFFF; continue; }
        if (!packed)
        {
            const GLubyte *p = src + (SIZE_T)i * comps;
            if (comps >= 3) { r=p[0]; g=p[1]; b=p[2]; a=(comps>=4)?p[3]:255; }
            else            { r=g=b=p[0]; a=(comps>=2)?p[1]:255; }
        }
        else
        {
            const GLubyte *p = src + (SIZE_T)i * psize;
            unsigned v = 0; int k;
            for (k = 0; k < psize; k++) v |= ((unsigned)p[k]) << (8*k); /* little-endian host */
            switch (type) {
              case GL_UNSIGNED_BYTE_3_3_2:        r=((v>>5)&7)*255/7;  g=((v>>2)&7)*255/7;  b=(v&3)*85; a=255; break;
              case GL_UNSIGNED_BYTE_2_3_3_REV:    r=(v&7)*255/7;       g=((v>>3)&7)*255/7;  b=((v>>6)&3)*85; a=255; break;
              case GL_UNSIGNED_SHORT_5_6_5:       r=((v>>11)&31)*255/31; g=((v>>5)&63)*255/63; b=(v&31)*255/31; a=255; break;
              case GL_UNSIGNED_SHORT_5_6_5_REV:   r=(v&31)*255/31; g=((v>>5)&63)*255/63; b=((v>>11)&31)*255/31; a=255; break;
              case GL_UNSIGNED_SHORT_4_4_4_4:     r=((v>>12)&15)*17; g=((v>>8)&15)*17; b=((v>>4)&15)*17; a=(v&15)*17; break;
              case GL_UNSIGNED_SHORT_4_4_4_4_REV: r=(v&15)*17; g=((v>>4)&15)*17; b=((v>>8)&15)*17; a=((v>>12)&15)*17; break;
              case GL_UNSIGNED_SHORT_5_5_5_1:     r=((v>>11)&31)*255/31; g=((v>>6)&31)*255/31; b=((v>>1)&31)*255/31; a=(v&1)?255:0; break;
              case GL_UNSIGNED_SHORT_1_5_5_5_REV: r=(v&31)*255/31; g=((v>>5)&31)*255/31; b=((v>>10)&31)*255/31; a=((v>>15)&1)?255:0; break;
              case GL_UNSIGNED_INT_8_8_8_8:       r=(v>>24)&0xFF; g=(v>>16)&0xFF; b=(v>>8)&0xFF; a=v&0xFF; break;
              case GL_UNSIGNED_INT_8_8_8_8_REV:   r=v&0xFF; g=(v>>8)&0xFF; b=(v>>16)&0xFF; a=(v>>24)&0xFF; break;
              case GL_UNSIGNED_INT_10_10_10_2:    r=((v>>22)&0x3FF)*255/1023; g=((v>>12)&0x3FF)*255/1023; b=((v>>2)&0x3FF)*255/1023; a=(v&3)*85; break;
              case GL_UNSIGNED_INT_2_10_10_10_REV:r=(v&0x3FF)*255/1023; g=((v>>10)&0x3FF)*255/1023; b=((v>>20)&0x3FF)*255/1023; a=((v>>30)&3)*85; break;
              default: break;
            }
        }
        if (bgr) { t=r; r=b; b=t; }
        dst[i] = ((DWORD)(a&0xFF)<<24)|((DWORD)(r&0xFF)<<16)|((DWORD)(g&0xFF)<<8)|(b&0xFF);
    }
}

void GLAPIENTRY xgl_real_TexImage2D(GLenum target, GLint level, GLint internalFormat,
                                    GLsizei w, GLsizei h, GLint border, GLenum format,
                                    GLenum type, const GLvoid *pixels)
{
    PXGL_CONTEXT c = XglCurrent();
    XGL_TEXTURE *tx;
    int oldW, oldH; ULONG oldOff;
    (void)internalFormat; (void)border;

    /* level>0 are mipmap levels — we have no mip chain, so only the base matters. */
    if (!c || target != GL_TEXTURE_2D || level != 0) return;
    if (c->boundTexture >= XGL_MAX_TEXTURES) return;
    tx = &c->textures[c->boundTexture];
    oldW = tx->width; oldH = tx->height; oldOff = tx->gpuOffset;
    if (tx->texels) { HeapFree(GetProcessHeap(), 0, tx->texels); tx->texels = NULL; }
    tx->width = w; tx->height = h;
    if (w <= 0 || h <= 0) return;
    tx->texels = (DWORD*)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)w*h*sizeof(DWORD));
    if (!tx->texels) return;

    XglDecodeImage(tx->texels, (const GLubyte*)pixels, w, h, format, type);

    /* Push the texels into VRAM so the NV2A can sample them (the offset is cached
     * in tx->gpuOffset and referenced by textured draws).  Re-specifying the same
     * dimensions reuses the existing VRAM slot; a size change allocates fresh. */
    tx->gpuOffset = (oldOff && oldW == w && oldH == h) ? oldOff : 0;
    XglUploadTexture(c, tx);
    DPRINT1("xboxogl: TexImage2D tex=%u %dx%d fmt=0x%x type=0x%x pixels=%p -> gpuOffset=0x%x\n",
            c->boundTexture, w, h, (unsigned)format, (unsigned)type, pixels, tx->gpuOffset);
}

/* GL 1.2 3D textures: the NV2A's PROJECT2D sampler path we drive is 2D-only, so we
 * degrade a 3D texture to its first depth slice (z=0).  This keeps apps that bind
 * a GL_TEXTURE_3D working (showing the base slice) instead of crashing or sampling
 * garbage.  True volumetric sampling is out of scope for this ICD. */
void GLAPIENTRY xgl_real_TexImage3D(GLenum target, GLint level, GLint internalFormat,
                                    GLsizei w, GLsizei h, GLsizei d, GLint border,
                                    GLenum format, GLenum type, const GLvoid *pixels)
{
    PXGL_CONTEXT c = XglCurrent();
    XGL_TEXTURE *tx;
    int oldW, oldH; ULONG oldOff;
    (void)target; (void)internalFormat; (void)border; (void)d;
    if (!c || level != 0 || c->boundTexture >= XGL_MAX_TEXTURES) return;
    tx = &c->textures[c->boundTexture];
    oldW = tx->width; oldH = tx->height; oldOff = tx->gpuOffset;
    if (tx->texels) { HeapFree(GetProcessHeap(), 0, tx->texels); tx->texels = NULL; }
    tx->width = w; tx->height = h;
    if (w <= 0 || h <= 0) return;
    tx->texels = (DWORD*)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)w*h*sizeof(DWORD));
    if (!tx->texels) return;
    XglDecodeImage(tx->texels, (const GLubyte*)pixels, w, h, format, type); /* slice 0 */
    tx->gpuOffset = (oldOff && oldW == w && oldH == h) ? oldOff : 0;
    XglUploadTexture(c, tx);
}

void GLAPIENTRY xgl_real_TexSubImage3D(GLenum target, GLint level, GLint xoff, GLint yoff,
                                       GLint zoff, GLsizei w, GLsizei h, GLsizei d,
                                       GLenum format, GLenum type, const GLvoid *pixels)
{
    /* Only the base slice (zoff==0) maps to our 2D degrade; ignore the rest. */
    (void)xoff; (void)yoff; (void)d;
    if (zoff == 0)
        xgl_real_TexImage3D(target, level, GL_RGBA, w, h, 1, 0, format, type, pixels);
}

void GLAPIENTRY xgl_real_CopyTexSubImage3D(GLenum target, GLint level, GLint xoff, GLint yoff,
                                           GLint zoff, GLint x, GLint y, GLsizei w, GLsizei h)
{
    /* No framebuffer-readback texture path; accept silently. */
    (void)target; (void)level; (void)xoff; (void)yoff; (void)zoff;
    (void)x; (void)y; (void)w; (void)h;
}

/* glTexSubImage2D: update a sub-rectangle of the bound texture in place, then
 * re-upload the whole image to VRAM. */
void GLAPIENTRY xgl_real_TexSubImage2D(GLenum target, GLint level, GLint xoff, GLint yoff,
                                       GLsizei w, GLsizei h, GLenum format, GLenum type,
                                       const GLvoid *pixels)
{
    PXGL_CONTEXT c = XglCurrent();
    XGL_TEXTURE *tx;
    DWORD *tmp;
    int y;
    if (!c || target != GL_TEXTURE_2D || level != 0 || c->boundTexture >= XGL_MAX_TEXTURES) return;
    tx = &c->textures[c->boundTexture];
    if (!tx->texels || w <= 0 || h <= 0) return;
    /* clamp the sub-rect to the existing image */
    if (xoff < 0 || yoff < 0 || xoff + w > tx->width || yoff + h > tx->height) {
        DPRINT1("xboxogl: TexSubImage2D REJECT tex=%u rect %d,%d %dx%d vs tex %dx%d\n",
                c->boundTexture, xoff, yoff, w, h, tx->width, tx->height);
        return;
    }
    tmp = (DWORD*)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)w*h*sizeof(DWORD));
    if (!tmp) return;
    XglDecodeImage(tmp, (const GLubyte*)pixels, w, h, format, type);
    for (y = 0; y < h; y++)
        memcpy(tx->texels + (SIZE_T)(yoff + y) * tx->width + xoff,
               tmp + (SIZE_T)y * w, (SIZE_T)w * sizeof(DWORD));
    HeapFree(GetProcessHeap(), 0, tmp);
    XglUploadTexture(c, tx);   /* reuse the existing slot (size unchanged) */
    DPRINT1("xboxogl: TexSubImage2D tex=%u rect %d,%d %dx%d fmt=0x%x type=0x%x\n",
            c->boundTexture, xoff, yoff, w, h, (unsigned)format, (unsigned)type);
}

/* GL 1.2 imaging subset: constant blend colour + blend equation. */
void GLAPIENTRY xgl_real_BlendColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a)
{
    PXGL_CONTEXT c = XglCurrent(); if (!c) return;
    c->blendColor[0]=r; c->blendColor[1]=g; c->blendColor[2]=b; c->blendColor[3]=a;
}
void GLAPIENTRY xgl_real_BlendEquation(GLenum mode)
{
    PXGL_CONTEXT c = XglCurrent(); if (!c) return;
    c->blendEquation = mode;
}

void GLAPIENTRY xgl_real_TexParameteri(GLenum target, GLenum pname, GLint param)
{
    PXGL_CONTEXT c = XglCurrent();
    XGL_TEXTURE *tx;
    if (!c || (target != GL_TEXTURE_2D && target != GL_TEXTURE_1D) || c->boundTexture >= XGL_MAX_TEXTURES) return;
    tx = &c->textures[c->boundTexture];
    switch (pname) {
        case GL_TEXTURE_MIN_FILTER: tx->minFilter = (GLenum)param; break;
        case GL_TEXTURE_MAG_FILTER: tx->magFilter = (GLenum)param; break;
        case GL_TEXTURE_WRAP_S:     tx->wrapS = (GLenum)param; break;
        case GL_TEXTURE_WRAP_T:     tx->wrapT = (GLenum)param; break;
        /* GL 1.2 LOD / level controls: accepted but inert (no mip chain — we
         * upload only the base level and sample it at every LOD). */
        case GL_TEXTURE_BASE_LEVEL: case GL_TEXTURE_MAX_LEVEL:
        case GL_TEXTURE_MIN_LOD:    case GL_TEXTURE_MAX_LOD:   break;
    }
}
void GLAPIENTRY xgl_real_TexParameterf(GLenum t, GLenum p, GLfloat v){ xgl_real_TexParameteri(t,p,(GLint)v); }

void GLAPIENTRY xgl_real_TexEnvi(GLenum target, GLenum pname, GLint param)
{
    PXGL_CONTEXT c = XglCurrent();
    (void)target;
    if (!c || c->boundTexture >= XGL_MAX_TEXTURES) return;
    if (pname == GL_TEXTURE_ENV_MODE)
        c->textures[c->boundTexture].envMode = (GLenum)param;
}
void GLAPIENTRY xgl_real_TexEnvf(GLenum t, GLenum p, GLfloat v){ xgl_real_TexEnvi(t,p,(GLint)v); }

/* ===================================================================== */
/* Queries                                                               */
/* ===================================================================== */

GLenum GLAPIENTRY xgl_real_GetError(void)
{
    PXGL_CONTEXT c = XglCurrent();
    GLenum e;
    if (!c) return GL_NO_ERROR;
    e = c->lastError;
    c->lastError = GL_NO_ERROR;
    return e;
}

void GLAPIENTRY xgl_real_GetIntegerv(GLenum pname, GLint *p)
{
    PXGL_CONTEXT c = XglCurrent();
    if (!c || !p) return;
    switch (pname)
    {
        case GL_VIEWPORT: p[0]=c->vpX; p[1]=c->vpY; p[2]=c->vpW; p[3]=c->vpH; break;
        case GL_MAX_TEXTURE_SIZE:        p[0]=4096; break;
        case GL_MAX_MODELVIEW_STACK_DEPTH:
        case GL_MAX_PROJECTION_STACK_DEPTH:
        case GL_MAX_TEXTURE_STACK_DEPTH: p[0]=XGL_MATRIX_STACK_DEPTH; break;
        case GL_DEPTH_BITS:   p[0]=24; break;   /* Z24S8 zeta (16 on the Z16 fallback) */
        case GL_STENCIL_BITS: p[0]=8; break;    /* Z24S8 stencil */
        case GL_RED_BITS: case GL_GREEN_BITS: case GL_BLUE_BITS: p[0]=8; break;
        case GL_ALPHA_BITS:   p[0]=8; break;
        case GL_TEXTURE_BINDING_2D: p[0]=(GLint)c->boundTexture; break;
        case GL_MATRIX_MODE:  p[0]=(GLint)c->matrixMode; break;
        case GL_DEPTH_FUNC:   p[0]=(GLint)c->depthFunc; break;
        case GL_DEPTH_WRITEMASK: p[0]=c->depthMask ? 1 : 0; break;
        case GL_BLEND_SRC:    p[0]=(GLint)c->blendSrc; break;
        case GL_BLEND_DST:    p[0]=(GLint)c->blendDst; break;
        case GL_CULL_FACE_MODE: p[0]=(GLint)c->cullFaceMode; break;
        case GL_FRONT_FACE:   p[0]=(GLint)c->frontFace; break;
        case GL_SHADE_MODEL:  p[0]=(GLint)c->shadeModel; break;
        case GL_ALPHA_TEST_FUNC: p[0]=(GLint)c->alphaFunc; break;
        case GL_MAX_LIGHTS:   p[0]=XGL_MAX_LIGHTS; break;
        case GL_MAX_CLIP_PLANES: p[0]=XGL_MAX_CLIP_PLANES; break;
        case GL_MAX_ATTRIB_STACK_DEPTH:
        case GL_MAX_CLIENT_ATTRIB_STACK_DEPTH: p[0]=XGL_ATTRIB_STACK; break;
        case GL_RENDER_MODE:  p[0]=(GLint)c->renderMode; break;
        case GL_NAME_STACK_DEPTH: p[0]=c->nameStackDepth; break;
        case GL_MAX_NAME_STACK_DEPTH: p[0]=XGL_NAME_STACK_DEPTH; break;
        case GL_MAX_EVAL_ORDER: p[0]=16; break;
        case GL_LIST_BASE:    p[0]=(GLint)c->listBase; break;
        case GL_LIST_INDEX:   p[0]=(GLint)c->listRecording; break;
        case GL_UNPACK_ALIGNMENT: p[0]=c->unpackAlignment; break;
        case GL_PACK_ALIGNMENT:   p[0]=c->packAlignment; break;
        case GL_AUX_BUFFERS:  p[0]=0; break;
        case GL_INDEX_BITS:   p[0]=0; break;
        case GL_COLOR_WRITEMASK:
            p[0]=c->colorMask[0]; p[1]=c->colorMask[1]; p[2]=c->colorMask[2]; p[3]=c->colorMask[3]; break;
        case GL_SCISSOR_BOX:  p[0]=c->scissorX; p[1]=c->scissorY; p[2]=c->scissorW; p[3]=c->scissorH; break;
        case GL_LINE_STIPPLE_PATTERN: p[0]=(GLint)c->lineStipplePattern; break;
        case GL_LINE_STIPPLE_REPEAT:  p[0]=c->lineStippleFactor; break;
        case GL_LOGIC_OP_MODE: p[0]=(GLint)c->logicOp; break;
        /* GL 1.2 queries */
        case GL_MAX_ELEMENTS_VERTICES: p[0]=NV2A_3D_MAX_VERTS; break;
        case GL_MAX_ELEMENTS_INDICES:  p[0]=XGL_VERTEX_BUFFER_CAP; break;
        case GL_MAX_3D_TEXTURE_SIZE:   p[0]=4096; break;
        case GL_BLEND_EQUATION:        p[0]=(GLint)c->blendEquation; break;
        case GL_LIGHT_MODEL_COLOR_CONTROL: p[0]=(GLint)c->colorControl; break;
        default: p[0]=0; break;
    }
}

void GLAPIENTRY xgl_real_GetFloatv(GLenum pname, GLfloat *p)
{
    PXGL_CONTEXT c = XglCurrent();
    if (!c || !p) return;
    switch (pname)
    {
        case GL_MODELVIEW_MATRIX:  memcpy(p, c->matStack[XGL_MAT_MODELVIEW][c->matTop[XGL_MAT_MODELVIEW]].m, sizeof(float)*16); break;
        case GL_PROJECTION_MATRIX: memcpy(p, c->matStack[XGL_MAT_PROJECTION][c->matTop[XGL_MAT_PROJECTION]].m, sizeof(float)*16); break;
        case GL_TEXTURE_MATRIX:    memcpy(p, c->matStack[XGL_MAT_TEXTURE][c->matTop[XGL_MAT_TEXTURE]].m, sizeof(float)*16); break;
        case GL_CURRENT_COLOR:     p[0]=c->curColor[0]; p[1]=c->curColor[1]; p[2]=c->curColor[2]; p[3]=c->curColor[3]; break;
        case GL_COLOR_CLEAR_VALUE: memcpy(p, c->clearColor, sizeof(float)*4); break;
        case GL_FOG_COLOR:         memcpy(p, c->fogColor, sizeof(float)*4); break;
        case GL_POINT_SIZE:        p[0]=c->pointSize; break;
        case GL_LINE_WIDTH:        p[0]=c->lineWidth; break;
        case GL_DEPTH_CLEAR_VALUE: p[0]=c->clearDepth; break;
        case GL_CURRENT_NORMAL:    p[0]=c->curNormal[0]; p[1]=c->curNormal[1]; p[2]=c->curNormal[2]; break;
        /* GL 1.2 queries */
        case GL_BLEND_COLOR:       memcpy(p, c->blendColor, sizeof(float)*4); break;
        case GL_ALIASED_POINT_SIZE_RANGE:
        case GL_SMOOTH_POINT_SIZE_RANGE: p[0]=1.0f; p[1]=64.0f; break;
        case GL_ALIASED_LINE_WIDTH_RANGE:
        case GL_SMOOTH_LINE_WIDTH_RANGE: p[0]=1.0f; p[1]=1.0f; break;
        default: p[0]=0.0f; break;
    }
}

const GLubyte * GLAPIENTRY xgl_real_GetString(GLenum name)
{
    static const GLubyte vendor[]   = "ReactOS / Xbox";
    static const GLubyte renderer[] = "NVIDIA NV2A (xboxogl)";
    static const GLubyte version[]  = "1.2.0";
    static const GLubyte ext[]      =
        "GL_EXT_bgra GL_EXT_texture_edge_clamp GL_EXT_draw_range_elements "
        "GL_EXT_blend_minmax GL_EXT_blend_subtract GL_EXT_blend_color "
        "GL_EXT_packed_pixels GL_EXT_texture3D GL_EXT_separate_specular_color "
        "GL_EXT_rescale_normal";
    switch (name)
    {
        case GL_VENDOR:     return vendor;
        case GL_RENDERER:   return renderer;
        case GL_VERSION:    return version;
        case GL_EXTENSIONS: return ext;
        default:            return ext;
    }
}

/* ===================================================================== */
/* Present                                                               */
/* ===================================================================== */

/* ===================================================================== */
/* Display lists (GL 1.1)                                                */
/* ===================================================================== */

GLuint GLAPIENTRY xgl_real_GenLists(GLsizei range)
{
    PXGL_CONTEXT c = XglCurrent();
    GLuint base;
    GLsizei k;
    if (!c || range <= 0) return 0;
    /* Find the first run of `range` consecutive free slots (1-based; 0 is invalid). */
    for (base = 1; (GLuint)(base + range) <= XGL_MAX_LISTS; base++)
    {
        BOOL ok = TRUE;
        for (k = 0; k < range; k++)
            if (c->lists[base + k].used) { ok = FALSE; break; }
        if (ok)
        {
            for (k = 0; k < range; k++)
            {
                c->lists[base + k].used  = TRUE;
                c->lists[base + k].count = 0;
            }
            return base;
        }
    }
    return 0;
}

void GLAPIENTRY xgl_real_NewList(GLuint list, GLenum mode)
{
    PXGL_CONTEXT c = XglCurrent();
    if (!c || list == 0 || list >= XGL_MAX_LISTS) return;
    c->lists[list].used  = TRUE;
    c->lists[list].count = 0;    /* re-record overwrites prior contents */
    c->listRecording = list;
    c->listMode = mode;
}

void GLAPIENTRY xgl_real_EndList(void)
{
    PXGL_CONTEXT c = XglCurrent();
    if (!c) return;
    c->listRecording = 0;
}

void GLAPIENTRY xgl_real_CallList(GLuint list)
{
    PXGL_CONTEXT c = XglCurrent();
    XGL_DLIST *L;
    int i;
    if (!c || list == 0 || list >= XGL_MAX_LISTS) return;
    L = &c->lists[list];
    if (!L->used || !L->data) return;
    for (i = 0; i < L->count; )
    {
        int op = (int)L->data[i];
        switch (op)
        {
            case (int)DL_BEGIN:      xgl_real_Begin((GLenum)L->data[i+1]); i += 2; break;
            case (int)DL_END:        xgl_real_End();                       i += 1; break;
            case (int)DL_VERTEX:     xgl_real_Vertex4f(L->data[i+1], L->data[i+2], L->data[i+3], L->data[i+4]); i += 5; break;
            case (int)DL_NORMAL:     xgl_real_Normal3f(L->data[i+1], L->data[i+2], L->data[i+3]); i += 4; break;
            case (int)DL_COLOR:      xgl_real_Color4f(L->data[i+1], L->data[i+2], L->data[i+3], L->data[i+4]); i += 5; break;
            case (int)DL_SHADEMODEL: xgl_real_ShadeModel((GLenum)L->data[i+1]); i += 2; break;
            case (int)DL_MATERIAL: {
                GLfloat m[4] = { L->data[i+3], L->data[i+4], L->data[i+5], L->data[i+6] };
                xgl_real_Materialfv((GLenum)L->data[i+1], (GLenum)L->data[i+2], m);
                i += 7; break;
            }
            case (int)DL_TEXCOORD: xgl_real_TexCoord2f(L->data[i+1], L->data[i+2]); i += 3; break;
            default: i = L->count; break;   /* corrupt stream: stop */
        }
    }
}

void GLAPIENTRY xgl_real_DeleteLists(GLuint list, GLsizei range)
{
    PXGL_CONTEXT c = XglCurrent();
    GLsizei k;
    if (!c || list == 0) return;
    for (k = 0; k < range; k++)
    {
        GLuint id = list + k;
        if (id == 0 || id >= XGL_MAX_LISTS) continue;
        if (c->lists[id].data) HeapFree(GetProcessHeap(), 0, c->lists[id].data);
        c->lists[id].data  = NULL;
        c->lists[id].used  = FALSE;
        c->lists[id].count = 0;
        c->lists[id].cap   = 0;
    }
}

GLboolean GLAPIENTRY xgl_real_IsList(GLuint list)
{
    PXGL_CONTEXT c = XglCurrent();
    if (!c || list == 0 || list >= XGL_MAX_LISTS) return GL_FALSE;
    return c->lists[list].used ? GL_TRUE : GL_FALSE;
}

void XglFreeLists(PXGL_CONTEXT c)
{
    GLuint i;
    if (!c) return;
    for (i = 0; i < XGL_MAX_LISTS; i++)
        if (c->lists[i].data) { HeapFree(GetProcessHeap(), 0, c->lists[i].data); c->lists[i].data = NULL; }
}

/* ===================================================================== */
/* Lighting / materials (fixed-function, computed per-vertex on the CPU)  */
/* ===================================================================== */
/* Multi-light ambient+diffuse model (up to 8 lights).  Specular is not modelled.
 * Vertex lighting is software T&L feeding the GPU rasteriser (see XformObj). */

void GLAPIENTRY xgl_real_Materialfv(GLenum face, GLenum pname, const GLfloat *params)
{
    PXGL_CONTEXT c = XglCurrent();
    (void)face;
    if (!c || !params) return;
    /* Record into a display list so per-list materials (e.g. glxgears sets each
     * gear's colour with glMaterialfv inside its glNewList) survive replay.
     * Without this the material executed at compile time and the LAST one stuck,
     * so every glCallList drew with the same colour. */
    if (DL_RECORDING(c))
    {
        float scalar = (pname == GL_SHININESS);
        float t[7] = { DL_MATERIAL, (float)face, (float)pname,
                       params[0],
                       scalar ? 0.0f : params[1],
                       scalar ? 0.0f : params[2],
                       scalar ? 0.0f : params[3] };
        DlAppend(c, t, 7);
        if (DL_SKIP_EXEC(c)) return;
    }
    switch (pname)
    {
        case GL_AMBIENT_AND_DIFFUSE:
            memcpy(c->matAmbient, params, 4*sizeof(float));
            memcpy(c->matDiffuse, params, 4*sizeof(float));
            break;
        case GL_DIFFUSE:  memcpy(c->matDiffuse, params, 4*sizeof(float)); break;
        case GL_AMBIENT:  memcpy(c->matAmbient, params, 4*sizeof(float)); break;
        case GL_EMISSION: memcpy(c->matEmission, params, 4*sizeof(float)); break;
        case GL_SPECULAR: memcpy(c->matSpecular, params, 4*sizeof(float)); break;
        case GL_SHININESS: c->matShininess = params[0]; break;
        default: break;
    }
    /* When lighting is OFF, material diffuse doubles as the flat colour (the
     * common glColorMaterial idiom apps rely on). */
    if (!c->enLighting && (pname == GL_AMBIENT_AND_DIFFUSE || pname == GL_DIFFUSE))
        xgl_real_Color4f(params[0], params[1], params[2], params[3]);
}
void GLAPIENTRY xgl_real_Materialf(GLenum face, GLenum pname, GLfloat param)
{ GLfloat p[4] = { param, 0.0f, 0.0f, 0.0f }; xgl_real_Materialfv(face, pname, p); }

void GLAPIENTRY xgl_real_Lightfv(GLenum light, GLenum pname, const GLfloat *params)
{
    PXGL_CONTEXT c = XglCurrent();
    const XGL_MAT4 *mv;
    int idx;
    if (!c || !params) return;
    idx = (int)light - GL_LIGHT0;
    if (idx < 0 || idx >= XGL_MAX_LIGHTS) return;

    switch (pname)
    {
        case GL_POSITION:
        {
            /* Transform to EYE space by the modelview in effect now (GL semantics). */
            float x=params[0], y=params[1], z=params[2], w=params[3];
            mv = &c->matStack[XGL_MAT_MODELVIEW][c->matTop[XGL_MAT_MODELVIEW]];
            c->lights[idx].pos[0] = mv->m[0]*x + mv->m[4]*y + mv->m[8] *z + mv->m[12]*w;
            c->lights[idx].pos[1] = mv->m[1]*x + mv->m[5]*y + mv->m[9] *z + mv->m[13]*w;
            c->lights[idx].pos[2] = mv->m[2]*x + mv->m[6]*y + mv->m[10]*z + mv->m[14]*w;
            c->lights[idx].pos[3] = w;
            break;
        }
        case GL_AMBIENT:  memcpy(c->lights[idx].ambient, params, 4*sizeof(float)); break;
        case GL_DIFFUSE:  memcpy(c->lights[idx].diffuse, params, 4*sizeof(float)); break;
        case GL_SPECULAR: memcpy(c->lights[idx].specular, params, 4*sizeof(float)); break;
        default: break;  /* spot / attenuation: not modelled */
    }
}
void GLAPIENTRY xgl_real_Lightf(GLenum light, GLenum pname, GLfloat param)
{ (void)light; (void)pname; (void)param; }
void GLAPIENTRY xgl_real_LightModelfv(GLenum pname, const GLfloat *params)
{
    PXGL_CONTEXT c = XglCurrent();
    if (!c || !params) return;
    if (pname == GL_LIGHT_MODEL_AMBIENT)
        memcpy(c->globalAmbient, params, 4*sizeof(float));
    else if (pname == GL_LIGHT_MODEL_COLOR_CONTROL)  /* GL 1.2 separate specular */
        c->colorControl = (GLenum)params[0];
}
void GLAPIENTRY xgl_real_ColorMaterial(GLenum face, GLenum mode)
{
    PXGL_CONTEXT c = XglCurrent(); (void)face;   /* we always track FRONT_AND_BACK */
    if (c) c->colorMaterialMode = mode;
}

void GLAPIENTRY xgl_real_Fogf(GLenum pname, GLfloat param)
{
    PXGL_CONTEXT c = XglCurrent(); if (!c) return;
    switch (pname) {
        case GL_FOG_MODE:    c->fogMode = (GLenum)param; break;
        case GL_FOG_DENSITY: c->fogDensity = param; break;
        case GL_FOG_START:   c->fogStart = param; break;
        case GL_FOG_END:     c->fogEnd = param; break;
    }
}
void GLAPIENTRY xgl_real_Fogi(GLenum pname, GLint param) { xgl_real_Fogf(pname, (GLfloat)param); }
void GLAPIENTRY xgl_real_AlphaFunc(GLenum func, GLclampf ref)
{ PXGL_CONTEXT c = XglCurrent(); if (c) { c->alphaFunc = func; c->alphaRef = (float)ref; } }
void GLAPIENTRY xgl_real_PolygonMode(GLenum face, GLenum mode)
{ PXGL_CONTEXT c = XglCurrent(); (void)face; if (c) c->polygonMode = mode; }
void GLAPIENTRY xgl_real_PolygonOffset(GLfloat factor, GLfloat units)
{ PXGL_CONTEXT c = XglCurrent(); if (c) { c->polyOffsetFactor = factor; c->polyOffsetUnits = units; } }
void GLAPIENTRY xgl_real_Fogfv(GLenum pname, const GLfloat *params)
{
    PXGL_CONTEXT c = XglCurrent(); if (!c || !params) return;
    if (pname == GL_FOG_COLOR) memcpy(c->fogColor, params, 4*sizeof(float));
    else xgl_real_Fogf(pname, params[0]);
}

/* glFinish/glFlush/SwapBuffers all present the frame (blit the offscreen dst
 * rect to the visible framebuffer).  A pending clear with no geometry rides
 * with the present so glClear-only frames still take effect. */
void GLAPIENTRY xgl_real_Finish(void) { PXGL_CONTEXT c = XglCurrent(); if (c) SubmitBatch(c, NULL, 0, NV2A_DRAW3D_FLAG_PRESENT, NV2A_TOPO_TRIANGLES); }
void GLAPIENTRY xgl_real_Flush(void)  { PXGL_CONTEXT c = XglCurrent(); if (c) SubmitBatch(c, NULL, 0, NV2A_DRAW3D_FLAG_PRESENT, NV2A_TOPO_TRIANGLES); }
void XglPresentCurrent(void)          { PXGL_CONTEXT c = XglCurrent(); if (c) SubmitBatch(c, NULL, 0, NV2A_DRAW3D_FLAG_PRESENT, NV2A_TOPO_TRIANGLES); }

/* ---------- the rest of the GL 1.1 surface ---------- */
/* Integer component -> float per the GL conversion (c / max_of_type). */
#define XGL_B2F(c)  ((float)(c)/127.0f)
#define XGL_UB2F(c) ((float)(c)/255.0f)
#define XGL_S2F(c)  ((float)(c)/32767.0f)
#define XGL_US2F(c) ((float)(c)/65535.0f)
#define XGL_I2F(c)  ((float)(c)/2147483647.0f)
#define XGL_UI2F(c) ((float)(c)/4294967295.0f)

/* ---- Colour type variants ---- */
void GLAPIENTRY xgl_real_Color3bv(const GLbyte *v){ if(v) xgl_real_Color4f(XGL_B2F(v[0]),XGL_B2F(v[1]),XGL_B2F(v[2]),1.0f); }
void GLAPIENTRY xgl_real_Color3i(GLint r,GLint g,GLint b){ xgl_real_Color4f(XGL_I2F(r),XGL_I2F(g),XGL_I2F(b),1.0f); }
void GLAPIENTRY xgl_real_Color3iv(const GLint *v){ if(v) xgl_real_Color3i(v[0],v[1],v[2]); }
void GLAPIENTRY xgl_real_Color3s(GLshort r,GLshort g,GLshort b){ xgl_real_Color4f(XGL_S2F(r),XGL_S2F(g),XGL_S2F(b),1.0f); }
void GLAPIENTRY xgl_real_Color3sv(const GLshort *v){ if(v) xgl_real_Color3s(v[0],v[1],v[2]); }
void GLAPIENTRY xgl_real_Color3ui(GLuint r,GLuint g,GLuint b){ xgl_real_Color4f(XGL_UI2F(r),XGL_UI2F(g),XGL_UI2F(b),1.0f); }
void GLAPIENTRY xgl_real_Color3uiv(const GLuint *v){ if(v) xgl_real_Color3ui(v[0],v[1],v[2]); }
void GLAPIENTRY xgl_real_Color3us(GLushort r,GLushort g,GLushort b){ xgl_real_Color4f(XGL_US2F(r),XGL_US2F(g),XGL_US2F(b),1.0f); }
void GLAPIENTRY xgl_real_Color3usv(const GLushort *v){ if(v) xgl_real_Color3us(v[0],v[1],v[2]); }
void GLAPIENTRY xgl_real_Color4bv(const GLbyte *v){ if(v) xgl_real_Color4f(XGL_B2F(v[0]),XGL_B2F(v[1]),XGL_B2F(v[2]),XGL_B2F(v[3])); }
void GLAPIENTRY xgl_real_Color4i(GLint r,GLint g,GLint b,GLint a){ xgl_real_Color4f(XGL_I2F(r),XGL_I2F(g),XGL_I2F(b),XGL_I2F(a)); }
void GLAPIENTRY xgl_real_Color4iv(const GLint *v){ if(v) xgl_real_Color4i(v[0],v[1],v[2],v[3]); }
void GLAPIENTRY xgl_real_Color4s(GLshort r,GLshort g,GLshort b,GLshort a){ xgl_real_Color4f(XGL_S2F(r),XGL_S2F(g),XGL_S2F(b),XGL_S2F(a)); }
void GLAPIENTRY xgl_real_Color4sv(const GLshort *v){ if(v) xgl_real_Color4s(v[0],v[1],v[2],v[3]); }
void GLAPIENTRY xgl_real_Color4ui(GLuint r,GLuint g,GLuint b,GLuint a){ xgl_real_Color4f(XGL_UI2F(r),XGL_UI2F(g),XGL_UI2F(b),XGL_UI2F(a)); }
void GLAPIENTRY xgl_real_Color4uiv(const GLuint *v){ if(v) xgl_real_Color4ui(v[0],v[1],v[2],v[3]); }
void GLAPIENTRY xgl_real_Color4us(GLushort r,GLushort g,GLushort b,GLushort a){ xgl_real_Color4f(XGL_US2F(r),XGL_US2F(g),XGL_US2F(b),XGL_US2F(a)); }
void GLAPIENTRY xgl_real_Color4usv(const GLushort *v){ if(v) xgl_real_Color4us(v[0],v[1],v[2],v[3]); }

/* ---- Normal type variants ---- */
void GLAPIENTRY xgl_real_Normal3bv(const GLbyte *v){ if(v) xgl_real_Normal3f(XGL_B2F(v[0]),XGL_B2F(v[1]),XGL_B2F(v[2])); }
void GLAPIENTRY xgl_real_Normal3i(GLint x,GLint y,GLint z){ xgl_real_Normal3f(XGL_I2F(x),XGL_I2F(y),XGL_I2F(z)); }
void GLAPIENTRY xgl_real_Normal3iv(const GLint *v){ if(v) xgl_real_Normal3i(v[0],v[1],v[2]); }
void GLAPIENTRY xgl_real_Normal3s(GLshort x,GLshort y,GLshort z){ xgl_real_Normal3f(XGL_S2F(x),XGL_S2F(y),XGL_S2F(z)); }
void GLAPIENTRY xgl_real_Normal3sv(const GLshort *v){ if(v) xgl_real_Normal3s(v[0],v[1],v[2]); }

/* ---- Vertex type variants ---- */
void GLAPIENTRY xgl_real_Vertex2iv(const GLint *v){ if(v) xgl_real_Vertex2f((float)v[0],(float)v[1]); }
void GLAPIENTRY xgl_real_Vertex2sv(const GLshort *v){ if(v) xgl_real_Vertex2f((float)v[0],(float)v[1]); }
void GLAPIENTRY xgl_real_Vertex3iv(const GLint *v){ if(v) xgl_real_Vertex3f((float)v[0],(float)v[1],(float)v[2]); }
void GLAPIENTRY xgl_real_Vertex3sv(const GLshort *v){ if(v) xgl_real_Vertex3f((float)v[0],(float)v[1],(float)v[2]); }
void GLAPIENTRY xgl_real_Vertex4d(GLdouble x,GLdouble y,GLdouble z,GLdouble w){ xgl_real_Vertex4f((float)x,(float)y,(float)z,(float)w); }
void GLAPIENTRY xgl_real_Vertex4dv(const GLdouble *v){ if(v) xgl_real_Vertex4f((float)v[0],(float)v[1],(float)v[2],(float)v[3]); }
void GLAPIENTRY xgl_real_Vertex4iv(const GLint *v){ if(v) xgl_real_Vertex4f((float)v[0],(float)v[1],(float)v[2],(float)v[3]); }
void GLAPIENTRY xgl_real_Vertex4s(GLshort x,GLshort y,GLshort z,GLshort w){ xgl_real_Vertex4f((float)x,(float)y,(float)z,(float)w); }
void GLAPIENTRY xgl_real_Vertex4sv(const GLshort *v){ if(v) xgl_real_Vertex4f((float)v[0],(float)v[1],(float)v[2],(float)v[3]); }

/* ---- TexCoord type variants (we sample 2D, so only s,t matter) ---- */
void GLAPIENTRY xgl_real_TexCoord1fv(const GLfloat *v){ if(v) xgl_real_TexCoord2f(v[0],0.0f); }
void GLAPIENTRY xgl_real_TexCoord1i(GLint s){ xgl_real_TexCoord2f((float)s,0.0f); }
void GLAPIENTRY xgl_real_TexCoord1iv(const GLint *v){ if(v) xgl_real_TexCoord2f((float)v[0],0.0f); }
void GLAPIENTRY xgl_real_TexCoord1s(GLshort s){ xgl_real_TexCoord2f((float)s,0.0f); }
void GLAPIENTRY xgl_real_TexCoord1sv(const GLshort *v){ if(v) xgl_real_TexCoord2f((float)v[0],0.0f); }
void GLAPIENTRY xgl_real_TexCoord1dv(const GLdouble *v){ if(v) xgl_real_TexCoord2f((float)v[0],0.0f); }
void GLAPIENTRY xgl_real_TexCoord2dv(const GLdouble *v){ if(v) xgl_real_TexCoord2f((float)v[0],(float)v[1]); }
void GLAPIENTRY xgl_real_TexCoord2iv(const GLint *v){ if(v) xgl_real_TexCoord2f((float)v[0],(float)v[1]); }
void GLAPIENTRY xgl_real_TexCoord2s(GLshort s,GLshort t){ xgl_real_TexCoord2f((float)s,(float)t); }
void GLAPIENTRY xgl_real_TexCoord2sv(const GLshort *v){ if(v) xgl_real_TexCoord2f((float)v[0],(float)v[1]); }
void GLAPIENTRY xgl_real_TexCoord3d(GLdouble s,GLdouble t,GLdouble r){ (void)r; xgl_real_TexCoord2f((float)s,(float)t); }
void GLAPIENTRY xgl_real_TexCoord3dv(const GLdouble *v){ if(v) xgl_real_TexCoord2f((float)v[0],(float)v[1]); }
void GLAPIENTRY xgl_real_TexCoord3i(GLint s,GLint t,GLint r){ (void)r; xgl_real_TexCoord2f((float)s,(float)t); }
void GLAPIENTRY xgl_real_TexCoord3iv(const GLint *v){ if(v) xgl_real_TexCoord2f((float)v[0],(float)v[1]); }
void GLAPIENTRY xgl_real_TexCoord3s(GLshort s,GLshort t,GLshort r){ (void)r; xgl_real_TexCoord2f((float)s,(float)t); }
void GLAPIENTRY xgl_real_TexCoord3sv(const GLshort *v){ if(v) xgl_real_TexCoord2f((float)v[0],(float)v[1]); }
void GLAPIENTRY xgl_real_TexCoord4d(GLdouble s,GLdouble t,GLdouble r,GLdouble q){ (void)r;(void)q; xgl_real_TexCoord2f((float)s,(float)t); }
void GLAPIENTRY xgl_real_TexCoord4dv(const GLdouble *v){ if(v) xgl_real_TexCoord2f((float)v[0],(float)v[1]); }
void GLAPIENTRY xgl_real_TexCoord4i(GLint s,GLint t,GLint r,GLint q){ (void)r;(void)q; xgl_real_TexCoord2f((float)s,(float)t); }
void GLAPIENTRY xgl_real_TexCoord4iv(const GLint *v){ if(v) xgl_real_TexCoord2f((float)v[0],(float)v[1]); }
void GLAPIENTRY xgl_real_TexCoord4s(GLshort s,GLshort t,GLshort r,GLshort q){ (void)r;(void)q; xgl_real_TexCoord2f((float)s,(float)t); }
void GLAPIENTRY xgl_real_TexCoord4sv(const GLshort *v){ if(v) xgl_real_TexCoord2f((float)v[0],(float)v[1]); }

/* ---- glRect array variants ---- */
void GLAPIENTRY xgl_real_Rectdv(const GLdouble *a,const GLdouble *b){ if(a&&b) xgl_real_Rectf((float)a[0],(float)a[1],(float)b[0],(float)b[1]); }
void GLAPIENTRY xgl_real_Rectfv(const GLfloat *a,const GLfloat *b){ if(a&&b) xgl_real_Rectf(a[0],a[1],b[0],b[1]); }
void GLAPIENTRY xgl_real_Rectiv(const GLint *a,const GLint *b){ if(a&&b) xgl_real_Rectf((float)a[0],(float)a[1],(float)b[0],(float)b[1]); }
void GLAPIENTRY xgl_real_Rectsv(const GLshort *a,const GLshort *b){ if(a&&b) xgl_real_Rectf((float)a[0],(float)a[1],(float)b[0],(float)b[1]); }

/* ---- glEdgeFlag (tracked; we always render filled triangles) ---- */
void GLAPIENTRY xgl_real_EdgeFlag(GLboolean f){ PXGL_CONTEXT c=XglCurrent(); if(c) c->edgeFlag=f; }
void GLAPIENTRY xgl_real_EdgeFlagv(const GLboolean *f){ if(f) xgl_real_EdgeFlag(*f); }

/* ---- Colour-index mode (RGBA-only visual: tracked) ---- */
void GLAPIENTRY xgl_real_Indexf(GLfloat ci){ PXGL_CONTEXT c=XglCurrent(); if(c) c->curIndex=ci; }
void GLAPIENTRY xgl_real_Indexd(GLdouble ci){ xgl_real_Indexf((GLfloat)ci); }
void GLAPIENTRY xgl_real_Indexi(GLint ci){ xgl_real_Indexf((GLfloat)ci); }
void GLAPIENTRY xgl_real_Indexs(GLshort ci){ xgl_real_Indexf((GLfloat)ci); }
void GLAPIENTRY xgl_real_Indexub(GLubyte ci){ xgl_real_Indexf((GLfloat)ci); }
void GLAPIENTRY xgl_real_Indexdv(const GLdouble *ci){ if(ci) xgl_real_Indexf((GLfloat)*ci); }
void GLAPIENTRY xgl_real_Indexfv(const GLfloat *ci){ if(ci) xgl_real_Indexf(*ci); }
void GLAPIENTRY xgl_real_Indexiv(const GLint *ci){ if(ci) xgl_real_Indexf((GLfloat)*ci); }
void GLAPIENTRY xgl_real_Indexsv(const GLshort *ci){ if(ci) xgl_real_Indexf((GLfloat)*ci); }
void GLAPIENTRY xgl_real_Indexubv(const GLubyte *ci){ if(ci) xgl_real_Indexf((GLfloat)*ci); }
void GLAPIENTRY xgl_real_IndexMask(GLuint mask){ PXGL_CONTEXT c=XglCurrent(); if(c) c->indexWriteMask=mask; }
void GLAPIENTRY xgl_real_ClearIndex(GLfloat ci){ PXGL_CONTEXT c=XglCurrent(); if(c) c->clearIndexVal=ci; }

/* ---- Integer / array variants of state setters (delegate to float forms) ---- */
void GLAPIENTRY xgl_real_Fogiv(GLenum pname, const GLint *params)
{
    if (!params) return;
    if (pname == GL_FOG_COLOR) { GLfloat f[4]={XGL_I2F(params[0]),XGL_I2F(params[1]),XGL_I2F(params[2]),XGL_I2F(params[3])}; xgl_real_Fogfv(pname,f); }
    else xgl_real_Fogf(pname,(GLfloat)params[0]);
}
void GLAPIENTRY xgl_real_Lighti(GLenum light, GLenum pname, GLint param){ xgl_real_Lightf(light,pname,(GLfloat)param); }
void GLAPIENTRY xgl_real_Lightiv(GLenum light, GLenum pname, const GLint *params)
{
    GLfloat f[4];
    if (!params) return;
    /* Colour params (ambient/diffuse/specular) convert as [0,1]; position/direction as-is. */
    if (pname==GL_AMBIENT||pname==GL_DIFFUSE||pname==GL_SPECULAR)
        { f[0]=XGL_I2F(params[0]); f[1]=XGL_I2F(params[1]); f[2]=XGL_I2F(params[2]); f[3]=XGL_I2F(params[3]); }
    else { f[0]=(float)params[0]; f[1]=(float)params[1]; f[2]=(float)params[2]; f[3]=(float)params[3]; }
    xgl_real_Lightfv(light,pname,f);
}
void GLAPIENTRY xgl_real_LightModelf(GLenum pname, GLfloat param){ GLfloat f[4]={param,0,0,0}; xgl_real_LightModelfv(pname,f); }
void GLAPIENTRY xgl_real_LightModeli(GLenum pname, GLint param){ xgl_real_LightModelf(pname,(GLfloat)param); }
void GLAPIENTRY xgl_real_LightModeliv(GLenum pname, const GLint *params)
{
    GLfloat f[4];
    if (!params) return;
    if (pname==GL_LIGHT_MODEL_AMBIENT) { f[0]=XGL_I2F(params[0]); f[1]=XGL_I2F(params[1]); f[2]=XGL_I2F(params[2]); f[3]=XGL_I2F(params[3]); }
    else { f[0]=(float)params[0]; f[1]=f[2]=f[3]=0.0f; }
    xgl_real_LightModelfv(pname,f);
}
void GLAPIENTRY xgl_real_Materiali(GLenum face, GLenum pname, GLint param){ xgl_real_Materialf(face,pname,(GLfloat)param); }
void GLAPIENTRY xgl_real_Materialiv(GLenum face, GLenum pname, const GLint *params)
{
    GLfloat f[4];
    if (!params) return;
    if (pname==GL_SHININESS) { xgl_real_Materialf(face,pname,(GLfloat)params[0]); return; }
    f[0]=XGL_I2F(params[0]); f[1]=XGL_I2F(params[1]); f[2]=XGL_I2F(params[2]); f[3]=XGL_I2F(params[3]);
    xgl_real_Materialfv(face,pname,f);
}
void GLAPIENTRY xgl_real_TexParameterfv(GLenum t, GLenum p, const GLfloat *v){ if(v) xgl_real_TexParameteri(t,p,(GLint)v[0]); }
void GLAPIENTRY xgl_real_TexParameteriv(GLenum t, GLenum p, const GLint *v){ if(v) xgl_real_TexParameteri(t,p,v[0]); }
void GLAPIENTRY xgl_real_TexEnvfv(GLenum t, GLenum p, const GLfloat *v)
{
    PXGL_CONTEXT c = XglCurrent();
    if (!v) return;
    if (p == GL_TEXTURE_ENV_COLOR) { if (c) memcpy(c->texEnvColor, v, 4*sizeof(float)); return; }
    xgl_real_TexEnvi(t, p, (GLint)v[0]);
}
void GLAPIENTRY xgl_real_TexEnviv(GLenum t, GLenum p, const GLint *v){ if(v) xgl_real_TexEnvi(t,p,v[0]); }

/* ===================================================================== */
/* Texture coordinate generation (glTexGen)                              */
/* ===================================================================== */
void GLAPIENTRY xgl_real_TexGeniv(GLenum coord, GLenum pname, const GLint *params)
{
    PXGL_CONTEXT c = XglCurrent();
    int idx = (int)coord - GL_S;
    if (!c || !params || idx < 0 || idx > 3) return;
    if (pname == GL_TEXTURE_GEN_MODE) c->texGenMode[idx] = (GLenum)params[0];
}
void GLAPIENTRY xgl_real_TexGenfv(GLenum coord, GLenum pname, const GLfloat *params)
{
    PXGL_CONTEXT c = XglCurrent();
    int idx = (int)coord - GL_S, i;
    if (!c || !params || idx < 0 || idx > 3) return;
    if (pname == GL_TEXTURE_GEN_MODE) { c->texGenMode[idx] = (GLenum)params[0]; return; }
    if (pname == GL_OBJECT_PLANE)
        for (i = 0; i < 4; i++) c->texGenObjPlane[idx][i] = params[i];
    else if (pname == GL_EYE_PLANE)
    {
        /* Store transformed to eye space by the current inverse modelview. */
        double pin[4], pout[4];
        for (i = 0; i < 4; i++) pin[i] = params[i];
        XglPlaneToEye(c, pin, pout);
        for (i = 0; i < 4; i++) c->texGenEyePlane[idx][i] = (float)pout[i];
    }
}
void GLAPIENTRY xgl_real_TexGendv(GLenum coord, GLenum pname, const GLdouble *params)
{
    GLfloat f[4];
    if (!params) return;
    f[0]=(float)params[0]; f[1]=(float)params[1]; f[2]=(float)params[2]; f[3]=(float)params[3];
    xgl_real_TexGenfv(coord,pname,f);
}
void GLAPIENTRY xgl_real_TexGeni(GLenum coord, GLenum pname, GLint param){ GLint p[4]={param,0,0,0}; xgl_real_TexGeniv(coord,pname,p); }
void GLAPIENTRY xgl_real_TexGenf(GLenum coord, GLenum pname, GLfloat param){ GLfloat p[4]={param,0,0,0}; xgl_real_TexGenfv(coord,pname,p); }
void GLAPIENTRY xgl_real_TexGend(GLenum coord, GLenum pname, GLdouble param){ xgl_real_TexGenf(coord,pname,(GLfloat)param); }
void GLAPIENTRY xgl_real_GetTexGeniv(GLenum coord, GLenum pname, GLint *params)
{
    PXGL_CONTEXT c = XglCurrent();
    int idx = (int)coord - GL_S;
    if (!c || !params || idx < 0 || idx > 3) return;
    if (pname == GL_TEXTURE_GEN_MODE) params[0] = (GLint)c->texGenMode[idx];
}
void GLAPIENTRY xgl_real_GetTexGenfv(GLenum coord, GLenum pname, GLfloat *params)
{
    PXGL_CONTEXT c = XglCurrent();
    int idx = (int)coord - GL_S, i;
    if (!c || !params || idx < 0 || idx > 3) return;
    if (pname == GL_TEXTURE_GEN_MODE) params[0] = (GLfloat)c->texGenMode[idx];
    else if (pname == GL_OBJECT_PLANE) for (i=0;i<4;i++) params[i]=c->texGenObjPlane[idx][i];
    else if (pname == GL_EYE_PLANE)    for (i=0;i<4;i++) params[i]=c->texGenEyePlane[idx][i];
}
void GLAPIENTRY xgl_real_GetTexGendv(GLenum coord, GLenum pname, GLdouble *params)
{
    GLfloat f[4]={0,0,0,0}; int i;
    if (!params) return;
    xgl_real_GetTexGenfv(coord,pname,f);
    for (i=0;i<4;i++) params[i]=f[i];
}

/* ===================================================================== */
/* 1D textures (degrade to a height-1 2D texture)                        */
/* ===================================================================== */
void GLAPIENTRY xgl_real_TexImage1D(GLenum target, GLint level, GLint internalFormat,
                                    GLsizei w, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
    (void)target;
    xgl_real_TexImage2D(GL_TEXTURE_2D, level, internalFormat, w, 1, border, format, type, pixels);
}
void GLAPIENTRY xgl_real_TexSubImage1D(GLenum target, GLint level, GLint xoff,
                                       GLsizei w, GLenum format, GLenum type, const GLvoid *pixels)
{
    (void)target;
    xgl_real_TexSubImage2D(GL_TEXTURE_2D, level, xoff, 0, w, 1, format, type, pixels);
}

/* ===================================================================== */
/* User clip planes                                                      */
/* ===================================================================== */
void GLAPIENTRY xgl_real_ClipPlane(GLenum plane, const GLdouble *eqn)
{
    PXGL_CONTEXT c = XglCurrent();
    int idx = (int)plane - GL_CLIP_PLANE0;
    if (!c || !eqn || idx < 0 || idx >= XGL_MAX_CLIP_PLANES) return;
    XglPlaneToEye(c, eqn, c->clipPlane[idx]);   /* store in eye space */
}
void GLAPIENTRY xgl_real_GetClipPlane(GLenum plane, GLdouble *eqn)
{
    PXGL_CONTEXT c = XglCurrent();
    int idx = (int)plane - GL_CLIP_PLANE0, i;
    if (!c || !eqn || idx < 0 || idx >= XGL_MAX_CLIP_PLANES) return;
    for (i = 0; i < 4; i++) eqn[i] = c->clipPlane[idx][i];
}

/* ===================================================================== */
/* Framebuffer write masks / misc render state (tracked)                 */
/* ===================================================================== */
void GLAPIENTRY xgl_real_ColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a)
{ PXGL_CONTEXT c=XglCurrent(); if(c){ c->colorMask[0]=r; c->colorMask[1]=g; c->colorMask[2]=b; c->colorMask[3]=a; } }
void GLAPIENTRY xgl_real_LineStipple(GLint factor, GLushort pattern)
{ PXGL_CONTEXT c=XglCurrent(); if(c){ c->lineStippleFactor=factor; c->lineStipplePattern=pattern; } }
void GLAPIENTRY xgl_real_PolygonStipple(const GLubyte *mask)
{ PXGL_CONTEXT c=XglCurrent(); if(c&&mask) memcpy(c->polygonStipple,mask,128); }
void GLAPIENTRY xgl_real_GetPolygonStipple(GLubyte *mask)
{ PXGL_CONTEXT c=XglCurrent(); if(c&&mask) memcpy(mask,c->polygonStipple,128); }
void GLAPIENTRY xgl_real_LogicOp(GLenum opcode){ PXGL_CONTEXT c=XglCurrent(); if(c) c->logicOp=opcode; }
void GLAPIENTRY xgl_real_DrawBuffer(GLenum mode){ PXGL_CONTEXT c=XglCurrent(); if(c) c->drawBuffer=mode; }
void GLAPIENTRY xgl_real_ReadBuffer(GLenum mode){ PXGL_CONTEXT c=XglCurrent(); if(c) c->readBuffer=mode; }
void GLAPIENTRY xgl_real_ClearAccum(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{ PXGL_CONTEXT c=XglCurrent(); if(c){ c->accumClear[0]=r; c->accumClear[1]=g; c->accumClear[2]=b; c->accumClear[3]=a; } }
/* CPU accumulation buffer backed by the readback/writeback escapes:
 * LOAD/ACCUM read the colour buffer, RETURN writes it, ADD/MULT are scalar. */
void GLAPIENTRY xgl_real_Accum(GLenum op, GLfloat value)
{
    PXGL_CONTEXT c = XglCurrent();
    int w, h, n, i; ULONG sx, sytop; DWORD *cbuf;
    if (!c) return;
    w = (int)c->width; h = (int)c->height;
    if (w <= 0 || h <= 0) return;
    if (!c->accumBuf || c->accumW != w || c->accumH != h)
    {
        if (c->accumBuf) HeapFree(GetProcessHeap(), 0, c->accumBuf);
        c->accumBuf = (float*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (SIZE_T)w*h*4*sizeof(float));
        c->accumW = w; c->accumH = h;
    }
    if (!c->accumBuf) return;
    n = w * h;
    if (op == GL_MULT) { for (i = 0; i < n*4; i++) c->accumBuf[i] *= value; return; }
    if (op == GL_ADD)  { for (i = 0; i < n*4; i++) c->accumBuf[i] += value; return; }
    cbuf = (DWORD*)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)n*sizeof(DWORD));
    if (!cbuf) return;
    XglGlToScreen(c, 0, 0, h, &sx, &sytop);
    if (op == GL_RETURN)
    {
        for (i = 0; i < n; i++)
        {
            float r=c->accumBuf[i*4]*value, g=c->accumBuf[i*4+1]*value, b=c->accumBuf[i*4+2]*value, a=c->accumBuf[i*4+3]*value;
            cbuf[i] = ((DWORD)F2B(a)<<24)|((DWORD)F2B(r)<<16)|((DWORD)F2B(g)<<8)|(DWORD)F2B(b);
        }
        XglWritebackRect(c, sx, sytop, (ULONG)w, (ULONG)h, 0, cbuf);
    }
    else if (op == GL_LOAD || op == GL_ACCUM)
    {
        if (XglReadbackRect(c, sx, sytop, (ULONG)w, (ULONG)h, cbuf))
            for (i = 0; i < n; i++)
            {
                DWORD p = cbuf[i];
                float r=((p>>16)&0xFF)/255.0f, g=((p>>8)&0xFF)/255.0f, b=(p&0xFF)/255.0f, a=((p>>24)&0xFF)/255.0f;
                if (op == GL_LOAD) { c->accumBuf[i*4]=r*value; c->accumBuf[i*4+1]=g*value; c->accumBuf[i*4+2]=b*value; c->accumBuf[i*4+3]=a*value; }
                else               { c->accumBuf[i*4]+=r*value; c->accumBuf[i*4+1]+=g*value; c->accumBuf[i*4+2]+=b*value; c->accumBuf[i*4+3]+=a*value; }
            }
    }
    HeapFree(GetProcessHeap(), 0, cbuf);
}
void GLAPIENTRY xgl_real_ClearStencil(GLint s){ PXGL_CONTEXT c=XglCurrent(); if(c) c->clearStencil=s; }
void GLAPIENTRY xgl_real_StencilMask(GLuint mask){ PXGL_CONTEXT c=XglCurrent(); if(c) c->stencilWriteMask=mask; }
void GLAPIENTRY xgl_real_StencilFunc(GLenum func, GLint ref, GLuint mask)
{ PXGL_CONTEXT c=XglCurrent(); if(c){ c->stencilFunc=func; c->stencilRef=ref; c->stencilValueMask=mask; } }
void GLAPIENTRY xgl_real_StencilOp(GLenum sfail, GLenum zfail, GLenum zpass)
{ PXGL_CONTEXT c=XglCurrent(); if(c){ c->stencilFail=sfail; c->stencilZFail=zfail; c->stencilZPass=zpass; } }

/* ===================================================================== */
/* Raster position / pixel store / pixel rectangles                      */
/* ===================================================================== */
static void RasterPosImpl(PXGL_CONTEXT c, float x, float y, float z, float w)
{
    XGL_MAT4 mvp;
    float in[4], clip[4], iw;
    XglMatMul(&mvp,
              &c->matStack[XGL_MAT_PROJECTION][c->matTop[XGL_MAT_PROJECTION]],
              &c->matStack[XGL_MAT_MODELVIEW] [c->matTop[XGL_MAT_MODELVIEW]]);
    in[0]=x; in[1]=y; in[2]=z; in[3]=w;
    XglMatTransform(&mvp, in, clip);
    if (clip[3] == 0.0f) { c->rasterValid = FALSE; return; }
    iw = 1.0f / clip[3];
    c->rasterPos[0] = (float)c->vpX + ((clip[0]*iw)*0.5f + 0.5f) * c->vpW;  /* window x */
    c->rasterPos[1] = (float)c->vpY + (0.5f - (clip[1]*iw)*0.5f) * c->vpH;  /* window y, top-down (matches XformObj) */
    c->rasterPos[2] = (clip[2]*iw)*0.5f + 0.5f;
    c->rasterPos[3] = clip[3];
    c->rasterValid = TRUE;
    memcpy(c->rasterColor, c->curColor, sizeof(c->rasterColor));
}
void GLAPIENTRY xgl_real_RasterPos4f(GLfloat x,GLfloat y,GLfloat z,GLfloat w){ PXGL_CONTEXT c=XglCurrent(); if(c) RasterPosImpl(c,x,y,z,w); }
void GLAPIENTRY xgl_real_RasterPos3f(GLfloat x,GLfloat y,GLfloat z){ xgl_real_RasterPos4f(x,y,z,1.0f); }
void GLAPIENTRY xgl_real_RasterPos2f(GLfloat x,GLfloat y){ xgl_real_RasterPos4f(x,y,0.0f,1.0f); }
void GLAPIENTRY xgl_real_RasterPos2i(GLint x,GLint y){ xgl_real_RasterPos4f((float)x,(float)y,0.0f,1.0f); }
void GLAPIENTRY xgl_real_RasterPos2d(GLdouble x,GLdouble y){ xgl_real_RasterPos4f((float)x,(float)y,0.0f,1.0f); }
void GLAPIENTRY xgl_real_RasterPos2s(GLshort x,GLshort y){ xgl_real_RasterPos4f((float)x,(float)y,0.0f,1.0f); }
void GLAPIENTRY xgl_real_RasterPos3i(GLint x,GLint y,GLint z){ xgl_real_RasterPos4f((float)x,(float)y,(float)z,1.0f); }
void GLAPIENTRY xgl_real_RasterPos3d(GLdouble x,GLdouble y,GLdouble z){ xgl_real_RasterPos4f((float)x,(float)y,(float)z,1.0f); }
void GLAPIENTRY xgl_real_RasterPos3s(GLshort x,GLshort y,GLshort z){ xgl_real_RasterPos4f((float)x,(float)y,(float)z,1.0f); }
void GLAPIENTRY xgl_real_RasterPos4i(GLint x,GLint y,GLint z,GLint w){ xgl_real_RasterPos4f((float)x,(float)y,(float)z,(float)w); }
void GLAPIENTRY xgl_real_RasterPos4d(GLdouble x,GLdouble y,GLdouble z,GLdouble w){ xgl_real_RasterPos4f((float)x,(float)y,(float)z,(float)w); }
void GLAPIENTRY xgl_real_RasterPos4s(GLshort x,GLshort y,GLshort z,GLshort w){ xgl_real_RasterPos4f((float)x,(float)y,(float)z,(float)w); }
void GLAPIENTRY xgl_real_RasterPos2fv(const GLfloat *v){ if(v) xgl_real_RasterPos4f(v[0],v[1],0.0f,1.0f); }
void GLAPIENTRY xgl_real_RasterPos2iv(const GLint *v){ if(v) xgl_real_RasterPos4f((float)v[0],(float)v[1],0.0f,1.0f); }
void GLAPIENTRY xgl_real_RasterPos2dv(const GLdouble *v){ if(v) xgl_real_RasterPos4f((float)v[0],(float)v[1],0.0f,1.0f); }
void GLAPIENTRY xgl_real_RasterPos2sv(const GLshort *v){ if(v) xgl_real_RasterPos4f((float)v[0],(float)v[1],0.0f,1.0f); }
void GLAPIENTRY xgl_real_RasterPos3fv(const GLfloat *v){ if(v) xgl_real_RasterPos4f(v[0],v[1],v[2],1.0f); }
void GLAPIENTRY xgl_real_RasterPos3iv(const GLint *v){ if(v) xgl_real_RasterPos4f((float)v[0],(float)v[1],(float)v[2],1.0f); }
void GLAPIENTRY xgl_real_RasterPos3dv(const GLdouble *v){ if(v) xgl_real_RasterPos4f((float)v[0],(float)v[1],(float)v[2],1.0f); }
void GLAPIENTRY xgl_real_RasterPos3sv(const GLshort *v){ if(v) xgl_real_RasterPos4f((float)v[0],(float)v[1],(float)v[2],1.0f); }
void GLAPIENTRY xgl_real_RasterPos4fv(const GLfloat *v){ if(v) xgl_real_RasterPos4f(v[0],v[1],v[2],v[3]); }
void GLAPIENTRY xgl_real_RasterPos4iv(const GLint *v){ if(v) xgl_real_RasterPos4f((float)v[0],(float)v[1],(float)v[2],(float)v[3]); }
void GLAPIENTRY xgl_real_RasterPos4dv(const GLdouble *v){ if(v) xgl_real_RasterPos4f((float)v[0],(float)v[1],(float)v[2],(float)v[3]); }
void GLAPIENTRY xgl_real_RasterPos4sv(const GLshort *v){ if(v) xgl_real_RasterPos4f((float)v[0],(float)v[1],(float)v[2],(float)v[3]); }

void GLAPIENTRY xgl_real_PixelZoom(GLfloat xf, GLfloat yf){ PXGL_CONTEXT c=XglCurrent(); if(c){ c->pixelZoomX=xf; c->pixelZoomY=yf; } }
void GLAPIENTRY xgl_real_PixelStorei(GLenum pname, GLint param)
{
    PXGL_CONTEXT c=XglCurrent(); if(!c) return;
    switch(pname){
        case GL_UNPACK_ALIGNMENT:    c->unpackAlignment=param; break;
        case GL_UNPACK_ROW_LENGTH:   c->unpackRowLength=param; break;
        case GL_UNPACK_SKIP_PIXELS:  c->unpackSkipPixels=param; break;
        case GL_UNPACK_SKIP_ROWS:    c->unpackSkipRows=param; break;
        case GL_PACK_ALIGNMENT:      c->packAlignment=param; break;
        default: break;
    }
}
void GLAPIENTRY xgl_real_PixelStoref(GLenum pname, GLfloat param){ xgl_real_PixelStorei(pname,(GLint)param); }
void GLAPIENTRY xgl_real_PixelTransferi(GLenum pname, GLint param){ (void)pname;(void)param; }   /* no pixel-transfer pipeline */
void GLAPIENTRY xgl_real_PixelTransferf(GLenum pname, GLfloat param){ (void)pname;(void)param; }
void GLAPIENTRY xgl_real_PixelMapfv(GLenum map, GLint n, const GLfloat *v){ (void)map;(void)n;(void)v; }
void GLAPIENTRY xgl_real_PixelMapuiv(GLenum map, GLint n, const GLuint *v){ (void)map;(void)n;(void)v; }
void GLAPIENTRY xgl_real_PixelMapusv(GLenum map, GLint n, const GLushort *v){ (void)map;(void)n;(void)v; }
void GLAPIENTRY xgl_real_GetPixelMapfv(GLenum map, GLfloat *values){ (void)map;(void)values; }
void GLAPIENTRY xgl_real_GetPixelMapuiv(GLenum map, GLuint *values){ (void)map;(void)values; }
void GLAPIENTRY xgl_real_GetPixelMapusv(GLenum map, GLushort *values){ (void)map;(void)values; }

/* ---- Framebuffer read/write via the miniport offscreen-surface escapes ---- */
/* Read a screen-pixel rect (top-down ARGB) from the rendered offscreen surface. */
static BOOL XglReadbackRect(PXGL_CONTEXT c, ULONG sx, ULONG sy, ULONG w, ULONG h, DWORD *out)
{
    NV2A_READBACK rb;
    if (!c->hdc) return FALSE;
    rb.X = sx; rb.Y = sy; rb.Width = w; rb.Height = h;
    return ExtEscape(c->hdc, XBOX_ESC_NV2A_READBACK, sizeof(rb), (LPCSTR)&rb,
                     (int)(w*h*sizeof(DWORD)), (LPSTR)out) > 0;
}
/* Write a screen-pixel rect (top-down ARGB) into the offscreen surface. */
static BOOL XglWritebackRect(PXGL_CONTEXT c, ULONG sx, ULONG sy, ULONG w, ULONG h,
                             ULONG flags, const DWORD *in)
{
    SIZE_T hdr = sizeof(NV2A_WRITEBACK), px = (SIZE_T)w*h*sizeof(DWORD);
    BYTE *buf; NV2A_WRITEBACK *wb; int ret;
    if (!c->hdc) return FALSE;
    buf = (BYTE*)HeapAlloc(GetProcessHeap(), 0, hdr + px);
    if (!buf) return FALSE;
    wb = (NV2A_WRITEBACK*)buf;
    wb->X = sx; wb->Y = sy; wb->Width = w; wb->Height = h; wb->Flags = flags;
    memcpy(buf + hdr, in, px);
    ret = ExtEscape(c->hdc, XBOX_ESC_NV2A_WRITEBACK, (int)(hdr + px), (LPCSTR)buf, 0, NULL);
    HeapFree(GetProcessHeap(), 0, buf);
    return ret > 0;
}
/* GL drawable coords (x,y; origin lower-left) -> offscreen top-left screen coords
 * of a height-h rect.  c->height is the drawable height; the offscreen is top-down. */
static void XglGlToScreen(PXGL_CONTEXT c, GLint xgl, GLint ygl, GLsizei h, ULONG *sx, ULONG *sytop)
{
    POINT org; org.x = 0; org.y = 0;
    GetDCOrgEx(c->hdc, &org);
    *sx    = (ULONG)(org.x + xgl);
    *sytop = (ULONG)(org.y + ((GLint)c->height - (ygl + h)));
}
static int XglFormatComps(GLenum f)
{ return (f==GL_RGBA||f==GL_BGRA)?4 : (f==GL_RGB||f==GL_BGR)?3 : (f==GL_LUMINANCE_ALPHA)?2 : 1; }
/* Write one ARGB pixel into a GL_UNSIGNED_BYTE destination of the given format. */
static void XglWritePixelUB(GLubyte *d, DWORD argb, GLenum format)
{
    GLubyte a=(argb>>24)&0xFF, r=(argb>>16)&0xFF, g=(argb>>8)&0xFF, b=argb&0xFF;
    switch (format) {
        case GL_RGBA:      d[0]=r; d[1]=g; d[2]=b; d[3]=a; break;
        case GL_BGRA:      d[0]=b; d[1]=g; d[2]=r; d[3]=a; break;
        case GL_RGB:       d[0]=r; d[1]=g; d[2]=b; break;
        case GL_BGR:       d[0]=b; d[1]=g; d[2]=r; break;
        case GL_LUMINANCE_ALPHA: d[0]=r; d[1]=a; break;
        case GL_ALPHA:     d[0]=a; break;
        case GL_LUMINANCE: case GL_RED: d[0]=r; break;
        case GL_GREEN:     d[0]=g; break;
        case GL_BLUE:      d[0]=b; break;
        default:           d[0]=r; d[1]=g; d[2]=b; break;
    }
}

/* Read the colour buffer, convert to format/type (byte components; non-byte types
 * approximated), flip to GL bottom-up.  Pack alignment ignored. */
void GLAPIENTRY xgl_real_ReadPixels(GLint x, GLint y, GLsizei w, GLsizei h, GLenum format, GLenum type, GLvoid *pixels)
{
    PXGL_CONTEXT c = XglCurrent();
    DWORD *tmp; ULONG sx, sytop; GLsizei row, col; int comps;
    if (!c || !pixels || w <= 0 || h <= 0) return;
    comps = XglFormatComps(format);
    tmp = (DWORD*)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)w*h*sizeof(DWORD));
    if (!tmp) return;
    XglGlToScreen(c, x, y, h, &sx, &sytop);
    if (!XglReadbackRect(c, sx, sytop, (ULONG)w, (ULONG)h, tmp))
    {
        memset(pixels, 0, (SIZE_T)w*h*comps);   /* readback unavailable */
        HeapFree(GetProcessHeap(), 0, tmp);
        return;
    }
    for (row = 0; row < h; row++)
    {
        const DWORD *src = tmp + (SIZE_T)(h - 1 - row) * w;   /* top-down -> bottom-up */
        GLubyte *dst = (GLubyte*)pixels + (SIZE_T)row * w * comps;
        for (col = 0; col < w; col++)
            XglWritePixelUB(dst + (SIZE_T)col * comps, src[col], format);
    }
    HeapFree(GetProcessHeap(), 0, tmp);
}

/* Decode to ARGB and blit into the offscreen at the raster pos (image origin =
 * lower-left).  Placement/flip may need tuning. */
void GLAPIENTRY xgl_real_DrawPixels(GLsizei w, GLsizei h, GLenum format, GLenum type, const GLvoid *pixels)
{
    PXGL_CONTEXT c = XglCurrent();
    DWORD *img, *flip; ULONG sx, sytop; POINT org; GLsizei row;
    if (!c || !pixels || w <= 0 || h <= 0 || !c->rasterValid) return;
    img = (DWORD*)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)w*h*sizeof(DWORD));
    if (!img) return;
    flip = (DWORD*)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)w*h*sizeof(DWORD));
    if (!flip) { HeapFree(GetProcessHeap(), 0, img); return; }
    XglDecodeImage(img, (const GLubyte*)pixels, w, h, format, type);  /* -> ARGB, source order */
    for (row = 0; row < h; row++)   /* GL image is bottom-up; offscreen is top-down */
        memcpy(flip + (SIZE_T)row * w, img + (SIZE_T)(h - 1 - row) * w, (SIZE_T)w*sizeof(DWORD));
    org.x = 0; org.y = 0; GetDCOrgEx(c->hdc, &org);
    sx    = (ULONG)(org.x + (GLint)c->rasterPos[0]);
    sytop = (ULONG)(org.y + (GLint)c->rasterPos[1] - h);   /* raster pos is the lower-left */
    XglWritebackRect(c, sx, sytop, (ULONG)w, (ULONG)h, 0, flip);
    HeapFree(GetProcessHeap(), 0, flip);
    HeapFree(GetProcessHeap(), 0, img);
}

/* Draw a 1-bit bitmap in the raster colour (set bits only), then advance the
 * raster pos.  This is the wglUseFontBitmaps text path. */
void GLAPIENTRY xgl_real_Bitmap(GLsizei w, GLsizei h, GLfloat xo, GLfloat yo, GLfloat xm, GLfloat ym, const GLubyte *bitmap)
{
    PXGL_CONTEXT c = XglCurrent();
    DWORD *argb; ULONG sx, sytop; POINT org; GLsizei row, col, stride;
    DWORD col32;
    if (!c) return;
    if (bitmap && w > 0 && h > 0 && c->rasterValid)
    {
        col32 = ((DWORD)F2B(c->rasterColor[3])<<24) | ((DWORD)F2B(c->rasterColor[0])<<16) |
                ((DWORD)F2B(c->rasterColor[1])<<8)  |  (DWORD)F2B(c->rasterColor[2]);
        argb = (DWORD*)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)w*h*sizeof(DWORD));
        if (argb)
        {
            stride = (w + 7) / 8;   /* bitmap rows are byte-aligned, GL bottom-up */
            for (row = 0; row < h; row++)
            {
                const GLubyte *brow = bitmap + (SIZE_T)(h - 1 - row) * stride;  /* -> top-down */
                for (col = 0; col < w; col++)
                {
                    int bit = (brow[col >> 3] >> (7 - (col & 7))) & 1;
                    argb[(SIZE_T)row * w + col] = bit ? col32 : 0;   /* alpha 0 = skip */
                }
            }
            org.x = 0; org.y = 0; GetDCOrgEx(c->hdc, &org);
            sx    = (ULONG)(org.x + (GLint)(c->rasterPos[0] - xo));
            sytop = (ULONG)(org.y + (GLint)(c->rasterPos[1] + yo) - h);
            XglWritebackRect(c, sx, sytop, (ULONG)w, (ULONG)h,
                             NV2A_WRITEBACK_FLAG_SKIP_TRANSPARENT, argb);
            HeapFree(GetProcessHeap(), 0, argb);
        }
    }
    if (c->rasterValid) { c->rasterPos[0] += xm; c->rasterPos[1] -= ym; }   /* ym is up (top-down) */
}

/* glCopyPixels: read a rect of the colour buffer and write it at the raster pos. */
void GLAPIENTRY xgl_real_CopyPixels(GLint x, GLint y, GLsizei w, GLsizei h, GLenum type)
{
    PXGL_CONTEXT c = XglCurrent();
    DWORD *tmp; ULONG ssx, ssytop, dsx, dsytop; POINT org;
    (void)type;
    if (!c || w <= 0 || h <= 0 || !c->rasterValid) return;
    tmp = (DWORD*)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)w*h*sizeof(DWORD));
    if (!tmp) return;
    XglGlToScreen(c, x, y, h, &ssx, &ssytop);
    if (XglReadbackRect(c, ssx, ssytop, (ULONG)w, (ULONG)h, tmp))
    {
        org.x = 0; org.y = 0; GetDCOrgEx(c->hdc, &org);
        dsx    = (ULONG)(org.x + (GLint)c->rasterPos[0]);
        dsytop = (ULONG)(org.y + (GLint)c->rasterPos[1] - h);
        XglWritebackRect(c, dsx, dsytop, (ULONG)w, (ULONG)h, 0, tmp);
    }
    HeapFree(GetProcessHeap(), 0, tmp);
}

/* glCopyTexImage2D / glCopyTexSubImage2D: read a rect of the colour buffer and
 * store it into the bound texture (via the existing VRAM texture-upload path). */
void GLAPIENTRY xgl_real_CopyTexImage2D(GLenum target, GLint level, GLenum internalFormat,
                                        GLint x, GLint y, GLsizei w, GLsizei h, GLint border)
{
    PXGL_CONTEXT c = XglCurrent();
    XGL_TEXTURE *tx; ULONG sx, sytop; int oldW, oldH; ULONG oldOff;
    (void)internalFormat; (void)border;
    if (!c || (target != GL_TEXTURE_2D && target != GL_TEXTURE_1D) || level != 0) return;
    if (c->boundTexture >= XGL_MAX_TEXTURES || w <= 0 || h <= 0) return;
    tx = &c->textures[c->boundTexture];
    oldW = tx->width; oldH = tx->height; oldOff = tx->gpuOffset;
    if (tx->texels) { HeapFree(GetProcessHeap(), 0, tx->texels); tx->texels = NULL; }
    tx->width = w; tx->height = h;
    tx->texels = (DWORD*)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)w*h*sizeof(DWORD));
    if (!tx->texels) return;
    XglGlToScreen(c, x, y, h, &sx, &sytop);
    if (!XglReadbackRect(c, sx, sytop, (ULONG)w, (ULONG)h, tx->texels))
        memset(tx->texels, 0, (SIZE_T)w*h*sizeof(DWORD));
    tx->gpuOffset = (oldOff && oldW == w && oldH == h) ? oldOff : 0;
    XglUploadTexture(c, tx);
}
/* 1D copies degrade to a height-1 2D copy (like glTexImage1D). */
void GLAPIENTRY xgl_real_CopyTexImage1D(GLenum target, GLint level, GLenum internalFormat,
                                        GLint x, GLint y, GLsizei w, GLint border)
{ (void)target; xgl_real_CopyTexImage2D(GL_TEXTURE_2D, level, internalFormat, x, y, w, 1, border); }
void GLAPIENTRY xgl_real_CopyTexSubImage1D(GLenum target, GLint level, GLint xoff, GLint x, GLint y, GLsizei w)
{ (void)target; xgl_real_CopyTexSubImage2D(GL_TEXTURE_2D, level, xoff, 0, x, y, w, 1); }

void GLAPIENTRY xgl_real_CopyTexSubImage2D(GLenum target, GLint level, GLint xoff, GLint yoff,
                                           GLint x, GLint y, GLsizei w, GLsizei h)
{
    PXGL_CONTEXT c = XglCurrent();
    XGL_TEXTURE *tx; DWORD *tmp; ULONG sx, sytop; GLsizei row;
    if (!c || (target != GL_TEXTURE_2D && target != GL_TEXTURE_1D) || level != 0) return;
    if (c->boundTexture >= XGL_MAX_TEXTURES || w <= 0 || h <= 0) return;
    tx = &c->textures[c->boundTexture];
    if (!tx->texels || xoff < 0 || yoff < 0 || xoff + w > tx->width || yoff + h > tx->height) return;
    tmp = (DWORD*)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)w*h*sizeof(DWORD));
    if (!tmp) return;
    XglGlToScreen(c, x, y, h, &sx, &sytop);
    if (XglReadbackRect(c, sx, sytop, (ULONG)w, (ULONG)h, tmp))
    {
        for (row = 0; row < h; row++)
            memcpy(tx->texels + (SIZE_T)(yoff + row) * tx->width + xoff,
                   tmp + (SIZE_T)row * w, (SIZE_T)w * sizeof(DWORD));
        XglUploadTexture(c, tx);
    }
    HeapFree(GetProcessHeap(), 0, tmp);
}

/* ===================================================================== */
/* glCallLists / glListBase                                              */
/* ===================================================================== */
void GLAPIENTRY xgl_real_ListBase(GLuint base){ PXGL_CONTEXT c=XglCurrent(); if(c) c->listBase=base; }
void GLAPIENTRY xgl_real_CallLists(GLsizei n, GLenum type, const GLvoid *lists)
{
    PXGL_CONTEXT c = XglCurrent();
    GLsizei i;
    if (!c || !lists || n <= 0) return;
    for (i = 0; i < n; i++)
    {
        GLuint id;
        switch (type) {
            case GL_BYTE:           id = (GLuint)((const GLbyte*)lists)[i]; break;
            case GL_UNSIGNED_BYTE:  id = ((const GLubyte*)lists)[i]; break;
            case GL_SHORT:          id = (GLuint)((const GLshort*)lists)[i]; break;
            case GL_UNSIGNED_SHORT: id = ((const GLushort*)lists)[i]; break;
            case GL_INT:            id = (GLuint)((const GLint*)lists)[i]; break;
            case GL_UNSIGNED_INT:   id = ((const GLuint*)lists)[i]; break;
            case GL_FLOAT:          id = (GLuint)((const GLfloat*)lists)[i]; break;
            default:                id = ((const GLubyte*)lists)[i]; break;
        }
        xgl_real_CallList(c->listBase + id);
    }
}

/* ===================================================================== */
/* Vertex-array completeness                                             */
/* ===================================================================== */
void GLAPIENTRY xgl_real_ArrayElement(GLint i){ PXGL_CONTEXT c=XglCurrent(); if(c) EmitArrayElement(c,i); }
void GLAPIENTRY xgl_real_IndexPointer(GLenum type, GLsizei stride, const GLvoid *pointer){ (void)type;(void)stride;(void)pointer; } /* colour-index arrays unused */
void GLAPIENTRY xgl_real_EdgeFlagPointer(GLsizei stride, const GLvoid *pointer){ (void)stride;(void)pointer; } /* edge flags don't affect filled tris */
void GLAPIENTRY xgl_real_InterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer)
{
    /* Set up the vertex/colour/texcoord pointers for the common interleaved
     * formats GLUT/games use.  Offsets are in floats; colours are 4 ubytes. */
    PXGL_CONTEXT c = XglCurrent();
    const BYTE *p = (const BYTE*)pointer;
    int hasT=0, hasC=0, cComp=4; GLenum cType=GL_FLOAT; int vComp=3, defStride;
    int tOff=0, cOff=0, vOff=0, fsz=(int)sizeof(GLfloat);
    if (!c) return;
    switch (format) {
        case GL_V2F:            vComp=2; defStride=2*fsz; vOff=0; break;
        case GL_V3F:            vComp=3; defStride=3*fsz; vOff=0; break;
        case GL_C4UB_V2F:       hasC=1; cType=GL_UNSIGNED_BYTE; cComp=4; vComp=2; cOff=0; vOff=4; defStride=4+2*fsz; break;
        case GL_C4UB_V3F:       hasC=1; cType=GL_UNSIGNED_BYTE; cComp=4; vComp=3; cOff=0; vOff=4; defStride=4+3*fsz; break;
        case GL_C3F_V3F:        hasC=1; cType=GL_FLOAT; cComp=3; vComp=3; cOff=0; vOff=3*fsz; defStride=6*fsz; break;
        case GL_T2F_V3F:        hasT=1; vComp=3; tOff=0; vOff=2*fsz; defStride=5*fsz; break;
        case GL_T2F_C4UB_V3F:   hasT=1; hasC=1; cType=GL_UNSIGNED_BYTE; cComp=4; vComp=3; tOff=0; cOff=2*fsz; vOff=2*fsz+4; defStride=2*fsz+4+3*fsz; break;
        case GL_T2F_C3F_V3F:    hasT=1; hasC=1; cType=GL_FLOAT; cComp=3; vComp=3; tOff=0; cOff=2*fsz; vOff=5*fsz; defStride=8*fsz; break;
        default:                vComp=3; defStride=3*fsz; vOff=0; break;
    }
    if (stride == 0) stride = defStride;
    xgl_real_DisableClientState(GL_COLOR_ARRAY);
    xgl_real_DisableClientState(GL_TEXTURE_COORD_ARRAY);
    xgl_real_DisableClientState(GL_NORMAL_ARRAY);
    if (hasT) { xgl_real_EnableClientState(GL_TEXTURE_COORD_ARRAY); xgl_real_TexCoordPointer(2, GL_FLOAT, stride, p+tOff); }
    if (hasC) { xgl_real_EnableClientState(GL_COLOR_ARRAY); xgl_real_ColorPointer(cComp, cType, stride, p+cOff); }
    xgl_real_EnableClientState(GL_VERTEX_ARRAY);
    xgl_real_VertexPointer(vComp, GL_FLOAT, stride, p+vOff);
}
void GLAPIENTRY xgl_real_PushClientAttrib(GLbitfield mask)
{
    PXGL_CONTEXT c = XglCurrent(); XGL_CLIENT_ATTRIB *s;
    (void)mask;
    if (!c || c->clientAttribTop >= XGL_ATTRIB_STACK) return;
    s = &c->clientAttribStack[c->clientAttribTop++];
    s->av=c->arrVertex; s->ac=c->arrColor; s->an=c->arrNormal; s->at=c->arrTexCoord;
    s->unpackAlignment=c->unpackAlignment; s->unpackRowLength=c->unpackRowLength;
    s->unpackSkipPixels=c->unpackSkipPixels; s->unpackSkipRows=c->unpackSkipRows; s->packAlignment=c->packAlignment;
}
void GLAPIENTRY xgl_real_PopClientAttrib(void)
{
    PXGL_CONTEXT c = XglCurrent(); const XGL_CLIENT_ATTRIB *s;
    if (!c || c->clientAttribTop == 0) return;
    s = &c->clientAttribStack[--c->clientAttribTop];
    c->arrVertex=s->av; c->arrColor=s->ac; c->arrNormal=s->an; c->arrTexCoord=s->at;
    c->unpackAlignment=s->unpackAlignment; c->unpackRowLength=s->unpackRowLength;
    c->unpackSkipPixels=s->unpackSkipPixels; c->unpackSkipRows=s->unpackSkipRows; c->packAlignment=s->packAlignment;
}
void GLAPIENTRY xgl_real_GetPointerv(GLenum pname, GLvoid **params)
{
    PXGL_CONTEXT c = XglCurrent();
    if (!c || !params) return;
    switch (pname) {
        case GL_VERTEX_ARRAY_POINTER:        *params=(GLvoid*)c->arrVertex.ptr; break;
        case GL_COLOR_ARRAY_POINTER:         *params=(GLvoid*)c->arrColor.ptr; break;
        case GL_NORMAL_ARRAY_POINTER:        *params=(GLvoid*)c->arrNormal.ptr; break;
        case GL_TEXTURE_COORD_ARRAY_POINTER: *params=(GLvoid*)c->arrTexCoord.ptr; break;
        default: *params=NULL; break;
    }
}

/* ===================================================================== */
/* Texture object queries                                                */
/* ===================================================================== */
GLboolean GLAPIENTRY xgl_real_IsTexture(GLuint texture)
{
    PXGL_CONTEXT c = XglCurrent();
    if (!c || texture == 0 || texture >= XGL_MAX_TEXTURES) return GL_FALSE;
    return c->textures[texture].used ? GL_TRUE : GL_FALSE;
}
GLboolean GLAPIENTRY xgl_real_AreTexturesResident(GLsizei n, const GLuint *textures, GLboolean *residences)
{
    GLsizei i;
    if (residences && textures) for (i=0;i<n;i++) residences[i]=GL_TRUE;   /* all kept resident */
    return GL_TRUE;
}
void GLAPIENTRY xgl_real_PrioritizeTextures(GLsizei n, const GLuint *textures, const GLclampf *priorities)
{ (void)n;(void)textures;(void)priorities; }   /* no eviction: priorities ignored */

/* ===================================================================== */
/* Complete glGet* family                                                */
/* ===================================================================== */
void GLAPIENTRY xgl_real_GetBooleanv(GLenum pname, GLboolean *params)
{
    GLint tmp[16]; int n = 1, i;
    if (!params) return;
    memset(tmp, 0, sizeof(tmp));
    if (pname==GL_VIEWPORT || pname==GL_COLOR_WRITEMASK || pname==GL_SCISSOR_BOX) n=4;
    xgl_real_GetIntegerv(pname, tmp);
    for (i = 0; i < n; i++) params[i] = tmp[i] ? GL_TRUE : GL_FALSE;
}
void GLAPIENTRY xgl_real_GetDoublev(GLenum pname, GLdouble *params)
{
    GLfloat tmp[16]; int n = 1, i;
    if (!params) return;
    memset(tmp, 0, sizeof(tmp));
    if (pname==GL_MODELVIEW_MATRIX||pname==GL_PROJECTION_MATRIX||pname==GL_TEXTURE_MATRIX) n=16;
    else if (pname==GL_CURRENT_COLOR||pname==GL_COLOR_CLEAR_VALUE||pname==GL_FOG_COLOR||pname==GL_VIEWPORT) n=4;
    else if (pname==GL_CURRENT_NORMAL) n=3;
    else if (pname==GL_DEPTH_RANGE) n=2;
    xgl_real_GetFloatv(pname, tmp);
    for (i = 0; i < n; i++) params[i] = (GLdouble)tmp[i];
}
void GLAPIENTRY xgl_real_GetLightfv(GLenum light, GLenum pname, GLfloat *params)
{
    PXGL_CONTEXT c = XglCurrent();
    int idx = (int)light - GL_LIGHT0;
    if (!c || !params || idx < 0 || idx >= XGL_MAX_LIGHTS) return;
    switch (pname) {
        case GL_AMBIENT:  memcpy(params, c->lights[idx].ambient, 4*sizeof(float)); break;
        case GL_DIFFUSE:  memcpy(params, c->lights[idx].diffuse, 4*sizeof(float)); break;
        case GL_SPECULAR: memcpy(params, c->lights[idx].specular,4*sizeof(float)); break;
        case GL_POSITION: memcpy(params, c->lights[idx].pos,     4*sizeof(float)); break;
        default: params[0]=0.0f; break;
    }
}
void GLAPIENTRY xgl_real_GetLightiv(GLenum light, GLenum pname, GLint *params)
{ GLfloat f[4]={0,0,0,0}; int i; if(!params) return; xgl_real_GetLightfv(light,pname,f); for(i=0;i<4;i++) params[i]=(GLint)f[i]; }
void GLAPIENTRY xgl_real_GetMaterialfv(GLenum face, GLenum pname, GLfloat *params)
{
    PXGL_CONTEXT c = XglCurrent();
    (void)face;
    if (!c || !params) return;
    switch (pname) {
        case GL_AMBIENT:  memcpy(params, c->matAmbient, 4*sizeof(float)); break;
        case GL_DIFFUSE:  memcpy(params, c->matDiffuse, 4*sizeof(float)); break;
        case GL_SPECULAR: memcpy(params, c->matSpecular,4*sizeof(float)); break;
        case GL_EMISSION: memcpy(params, c->matEmission,4*sizeof(float)); break;
        case GL_SHININESS: params[0]=c->matShininess; break;
        default: params[0]=0.0f; break;
    }
}
void GLAPIENTRY xgl_real_GetMaterialiv(GLenum face, GLenum pname, GLint *params)
{ GLfloat f[4]={0,0,0,0}; int i; if(!params) return; xgl_real_GetMaterialfv(face,pname,f); for(i=0;i<4;i++) params[i]=(GLint)f[i]; }
void GLAPIENTRY xgl_real_GetTexEnviv(GLenum t, GLenum pname, GLint *params)
{
    PXGL_CONTEXT c = XglCurrent();
    (void)t;
    if (!c || !params || c->boundTexture >= XGL_MAX_TEXTURES) return;
    if (pname == GL_TEXTURE_ENV_MODE) params[0] = (GLint)c->textures[c->boundTexture].envMode;
    else params[0] = 0;
}
void GLAPIENTRY xgl_real_GetTexEnvfv(GLenum t, GLenum pname, GLfloat *params)
{ GLint i=0; if(!params) return; xgl_real_GetTexEnviv(t,pname,&i); params[0]=(GLfloat)i; }
void GLAPIENTRY xgl_real_GetTexParameteriv(GLenum t, GLenum pname, GLint *params)
{
    PXGL_CONTEXT c = XglCurrent(); XGL_TEXTURE *tx;
    (void)t;
    if (!c || !params || c->boundTexture >= XGL_MAX_TEXTURES) return;
    tx = &c->textures[c->boundTexture];
    switch (pname) {
        case GL_TEXTURE_MIN_FILTER: params[0]=(GLint)tx->minFilter; break;
        case GL_TEXTURE_MAG_FILTER: params[0]=(GLint)tx->magFilter; break;
        case GL_TEXTURE_WRAP_S:     params[0]=(GLint)tx->wrapS; break;
        case GL_TEXTURE_WRAP_T:     params[0]=(GLint)tx->wrapT; break;
        default: params[0]=0; break;
    }
}
void GLAPIENTRY xgl_real_GetTexParameterfv(GLenum t, GLenum pname, GLfloat *params)
{ GLint i=0; if(!params) return; xgl_real_GetTexParameteriv(t,pname,&i); params[0]=(GLfloat)i; }
void GLAPIENTRY xgl_real_GetTexLevelParameteriv(GLenum t, GLint level, GLenum pname, GLint *params)
{
    PXGL_CONTEXT c = XglCurrent(); XGL_TEXTURE *tx;
    (void)t;(void)level;
    if (!c || !params || c->boundTexture >= XGL_MAX_TEXTURES) return;
    tx = &c->textures[c->boundTexture];
    switch (pname) {
        case GL_TEXTURE_WIDTH:  params[0]=tx->width; break;
        case GL_TEXTURE_HEIGHT: params[0]=tx->height; break;
        case GL_TEXTURE_INTERNAL_FORMAT: params[0]=GL_RGBA; break;
        default: params[0]=0; break;
    }
}
void GLAPIENTRY xgl_real_GetTexLevelParameterfv(GLenum t, GLint level, GLenum pname, GLfloat *params)
{ GLint i=0; if(!params) return; xgl_real_GetTexLevelParameteriv(t,level,pname,&i); params[0]=(GLfloat)i; }
void GLAPIENTRY xgl_real_GetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels)
{
    /* Return the stored ARGB texels of the bound texture (we keep a CPU copy). */
    PXGL_CONTEXT c = XglCurrent(); XGL_TEXTURE *tx; int i, n;
    (void)target;(void)level;
    if (!c || !pixels || c->boundTexture >= XGL_MAX_TEXTURES) return;
    tx = &c->textures[c->boundTexture];
    if (!tx->texels || tx->width<=0 || tx->height<=0) return;
    n = tx->width * tx->height;
    if (type == GL_UNSIGNED_BYTE && (format==GL_RGBA||format==GL_BGRA))
    {
        GLubyte *d = (GLubyte*)pixels;
        int bgr = (format==GL_BGRA);
        for (i = 0; i < n; i++) {
            DWORD argb = tx->texels[i];
            GLubyte a=(argb>>24)&0xFF, r=(argb>>16)&0xFF, g=(argb>>8)&0xFF, b=argb&0xFF;
            if (bgr) { d[i*4+0]=b; d[i*4+1]=g; d[i*4+2]=r; d[i*4+3]=a; }
            else     { d[i*4+0]=r; d[i*4+1]=g; d[i*4+2]=b; d[i*4+3]=a; }
        }
    }
    else memcpy(pixels, tx->texels, (SIZE_T)n*sizeof(DWORD));   /* default: raw ARGB DWORDs */
}

/* ===================================================================== */
/* Selection                                                             */
/* ===================================================================== */
/* Flush the pending hit record (if any geometry hit since the last name change). */
static void SelFlushHit(PXGL_CONTEXT c)
{
    GLsizei i;
    if (!c->selHitActive) return;
    c->selHitActive = FALSE;
    c->selHitCount++;
    /* record = [nameCount, zmin*2^32, zmax*2^32, name0, name1, ...] */
    #define SEL_PUT(val) do { if (c->selectIndex < c->selectBufferSize) c->selectBuffer[c->selectIndex++] = (GLuint)(val); else c->selOverflow = TRUE; } while (0)
    SEL_PUT((GLuint)c->nameStackDepth);
    SEL_PUT((GLuint)(c->selHitZMin * 4294967295.0f));
    SEL_PUT((GLuint)(c->selHitZMax * 4294967295.0f));
    for (i = 0; i < c->nameStackDepth; i++) SEL_PUT(c->nameStack[i]);
    #undef SEL_PUT
}
void GLAPIENTRY xgl_real_SelectBuffer(GLsizei size, GLuint *buffer)
{ PXGL_CONTEXT c=XglCurrent(); if(!c) return; c->selectBuffer=buffer; c->selectBufferSize=size; }
void GLAPIENTRY xgl_real_InitNames(void)
{ PXGL_CONTEXT c=XglCurrent(); if(!c) return; SelFlushHit(c); c->nameStackDepth=0; }
void GLAPIENTRY xgl_real_LoadName(GLuint name)
{ PXGL_CONTEXT c=XglCurrent(); if(!c||c->nameStackDepth==0) return; SelFlushHit(c); c->nameStack[c->nameStackDepth-1]=name; }
void GLAPIENTRY xgl_real_PushName(GLuint name)
{ PXGL_CONTEXT c=XglCurrent(); if(!c||c->nameStackDepth>=XGL_NAME_STACK_DEPTH) return; SelFlushHit(c); c->nameStack[c->nameStackDepth++]=name; }
void GLAPIENTRY xgl_real_PopName(void)
{ PXGL_CONTEXT c=XglCurrent(); if(!c||c->nameStackDepth==0) return; SelFlushHit(c); c->nameStackDepth--; }

/* ===================================================================== */
/* Feedback                                                              */
/* ===================================================================== */
void GLAPIENTRY xgl_real_FeedbackBuffer(GLsizei size, GLenum type, GLfloat *buffer)
{ PXGL_CONTEXT c=XglCurrent(); if(!c) return; c->feedbackBuffer=buffer; c->feedbackBufferSize=size; c->feedbackType=type; }
void GLAPIENTRY xgl_real_PassThrough(GLfloat token)
{
    PXGL_CONTEXT c = XglCurrent();
    if (!c || c->renderMode != GL_FEEDBACK) return;
    if (c->feedbackIndex+2 <= c->feedbackBufferSize)
    { c->feedbackBuffer[c->feedbackIndex++]=(GLfloat)GL_PASS_THROUGH_TOKEN; c->feedbackBuffer[c->feedbackIndex++]=token; }
    else c->feedbackOverflow = TRUE;
}
static void FbPush(PXGL_CONTEXT c, float v)
{ if (c->feedbackIndex < c->feedbackBufferSize) c->feedbackBuffer[c->feedbackIndex++]=v; else c->feedbackOverflow=TRUE; }
/* Defined here (forward-declared near SubmitBatch): write a primitive's vertices. */
static void FeedbackWritePrimitive(PXGL_CONTEXT c, const NV2A_3D_VERTEX *verts, int n, ULONG topology)
{
    float surfW = (float)GetDeviceCaps(c->hdc, HORZRES), surfH = (float)GetDeviceCaps(c->hdc, VERTRES);
    int perPrim, i, j, vtxPer;
    float tok;
    if (!c->feedbackBuffer) return;
    if (surfW < 1.0f) surfW = 640.0f;
    if (surfH < 1.0f) surfH = 480.0f;
    if (topology == NV2A_TOPO_POINTS) { perPrim = 1; tok = (float)GL_POINT_TOKEN; }
    else if (topology == NV2A_TOPO_LINES) { perPrim = 2; tok = (float)GL_LINE_TOKEN; }
    else if (topology == NV2A_TOPO_TRIANGLES) { perPrim = 3; tok = (float)GL_POLYGON_TOKEN; }
    else { perPrim = n; tok = (float)GL_LINE_TOKEN; }   /* strips/loops: one run */
    for (i = 0; i + perPrim <= n; i += perPrim)
    {
        vtxPer = perPrim;
        if (topology == NV2A_TOPO_TRIANGLES) { FbPush(c, tok); FbPush(c, (float)vtxPer); }
        else FbPush(c, tok);
        for (j = 0; j < vtxPer; j++)
        {
            const NV2A_3D_VERTEX *v = &verts[i+j];
            float wx = (v->x*0.5f+0.5f)*surfW, wy = (v->y*0.5f+0.5f)*surfH;
            FbPush(c, wx); FbPush(c, wy);
            if (c->feedbackType != GL_2D) FbPush(c, v->z);
            if (c->feedbackType == GL_4D_COLOR_TEXTURE) FbPush(c, 1.0f);   /* w */
            if (c->feedbackType==GL_3D_COLOR || c->feedbackType==GL_3D_COLOR_TEXTURE || c->feedbackType==GL_4D_COLOR_TEXTURE)
            { FbPush(c, v->r); FbPush(c, v->g); FbPush(c, v->b); FbPush(c, v->a); }
            if (c->feedbackType==GL_3D_COLOR_TEXTURE || c->feedbackType==GL_4D_COLOR_TEXTURE)
            { FbPush(c, v->u); FbPush(c, v->v); FbPush(c, 0.0f); FbPush(c, v->q); }
        }
    }
}

/* ===================================================================== */
/* glRenderMode — switch RENDER / SELECT / FEEDBACK, returning the count  */
/* ===================================================================== */
GLint GLAPIENTRY xgl_real_RenderMode(GLenum mode)
{
    PXGL_CONTEXT c = XglCurrent();
    GLint result = 0;
    if (!c) return 0;
    /* Returning to GL_RENDER yields the count accumulated in the prior mode. */
    if (c->renderMode == GL_SELECT) { SelFlushHit(c); result = c->selOverflow ? -1 : c->selHitCount; }
    else if (c->renderMode == GL_FEEDBACK) { result = c->feedbackOverflow ? -1 : c->feedbackIndex; }
    /* Entering a mode resets its counters. */
    if (mode == GL_SELECT) { c->selectIndex=0; c->selHitCount=0; c->selHitActive=FALSE; c->selOverflow=FALSE; c->nameStackDepth=0; }
    else if (mode == GL_FEEDBACK) { c->feedbackIndex=0; c->feedbackOverflow=FALSE; }
    c->renderMode = mode;
    return result;
}

/* ===================================================================== */
/* Evaluators (glMap1/glMap2 + grids)                                    */
/* ===================================================================== */
/* Map a GL_MAP{1,2}_* target to a slot index (0..8) and component count. */
static int EvalTargetInfo(GLenum target, int *comps, int *is2d)
{
    GLenum base; int idx;
    static const int cc[9] = { 4, 1, 3, 1, 2, 3, 4, 3, 4 };  /* COLOR4,INDEX,NORMAL,TC1..4,VERTEX3,VERTEX4 */
    if (target >= GL_MAP2_COLOR_4) { *is2d = 1; base = GL_MAP2_COLOR_4; }
    else { *is2d = 0; base = GL_MAP1_COLOR_4; }
    idx = (int)target - (int)base;
    if (idx < 0 || idx > 8) return -1;
    *comps = cc[idx];
    return idx;
}
/* 1D Bezier (de Casteljau) over `order` control points of `comps` values each
 * (contiguous), at parameter t in [0,1].  Writes out[comps]; if deriv, the
 * tangent (unnormalised) into deriv[comps]. */
static void Bez1(int comps, int order, const float *cp, float t, float *out, float *deriv)
{
    float a[16*4], d[16*4];
    int n = order, m = order - 1, r, i, cc;
    if (n > 16) n = 16;
    for (i = 0; i < n*comps; i++) a[i] = cp[i];
    if (deriv)
    {
        for (i = 0; i < m; i++) for (cc = 0; cc < comps; cc++) d[i*comps+cc] = (float)m*(cp[(i+1)*comps+cc]-cp[i*comps+cc]);
        for (r = 1; r < m; r++) for (i = 0; i < m-r; i++) for (cc = 0; cc < comps; cc++) d[i*comps+cc] = d[i*comps+cc]*(1.0f-t)+d[(i+1)*comps+cc]*t;
        for (cc = 0; cc < comps; cc++) deriv[cc] = (m > 0) ? d[cc] : 0.0f;
    }
    for (r = 1; r < n; r++) for (i = 0; i < n-r; i++) for (cc = 0; cc < comps; cc++) a[i*comps+cc] = a[i*comps+cc]*(1.0f-t)+a[(i+1)*comps+cc]*t;
    for (cc = 0; cc < comps; cc++) out[cc] = a[cc];
}
/* Evaluate a 1D map at u. */
static void EvalMap1At(const XGL_EVALMAP *m, float u, float *out)
{
    float t = (m->u2 != m->u1) ? (u - m->u1)/(m->u2 - m->u1) : 0.0f;
    Bez1(m->components, m->uorder, m->points, t, out, NULL);
}
/* Evaluate a 2D map at (u,v); optionally the u/v partials (for auto-normal). */
static void EvalMap2At(const XGL_EVALMAP *m, float u, float v, float *out, float *du, float *dv)
{
    float q[16*4], r[16*4], tu, tv;
    int i, comps = m->components, uo = m->uorder, vo = m->vorder;
    tu = (m->u2 != m->u1) ? (u - m->u1)/(m->u2 - m->u1) : 0.0f;
    tv = (m->v2 != m->v1) ? (v - m->v1)/(m->v2 - m->v1) : 0.0f;
    if (uo > 16) uo = 16;
    for (i = 0; i < uo; i++)
    {
        /* row i: vorder control points packed as points[(i*vo + j)*comps + c] */
        Bez1(comps, vo, m->points + (size_t)i*vo*comps, tv, q + i*comps, dv ? r + i*comps : NULL);
    }
    Bez1(comps, uo, q, tu, out, du);                 /* position (+ du) */
    if (dv) Bez1(comps, uo, r, tu, dv, NULL);        /* v-partial */
}
/* Generate one evaluated vertex (and its companion attributes) for EvalCoord. */
static void EvalDoCoord(PXGL_CONTEXT c, int is2d, float u, float v)
{
    int idx, comps, two;
    float pos[4] = {0,0,0,1};
    BOOL havePos = FALSE;
    /* Companion attributes first (so they're current when the vertex emits). */
    for (idx = 0; idx <= 6; idx++)
    {
        const XGL_EVALMAP *m = is2d ? &c->evalMap2[idx] : &c->evalMap1[idx];
        BOOL en = is2d ? c->evalMap2Enabled[idx] : c->evalMap1Enabled[idx];
        float val[4] = {0,0,0,0};
        if (!en || !m->defined) continue;
        if (is2d) EvalMap2At(m, u, v, val, NULL, NULL); else EvalMap1At(m, u, val);
        switch (idx) {
            case 0: xgl_real_Color4f(val[0],val[1],val[2],val[3]); break;    /* COLOR_4 */
            case 2: xgl_real_Normal3f(val[0],val[1],val[2]); break;          /* NORMAL */
            case 3: case 4: case 5: case 6: xgl_real_TexCoord2f(val[0],val[1]); break; /* TEXTURE_COORD_* */
            default: break;
        }
    }
    /* Position from VERTEX_4 (idx 8) or VERTEX_3 (idx 7), with optional auto-normal. */
    {
        int vidx = -1;
        comps = 3; two = is2d;
        if ((is2d ? c->evalMap2Enabled[8] : c->evalMap1Enabled[8]) && (is2d?c->evalMap2[8].defined:c->evalMap1[8].defined)) { vidx=8; comps=4; }
        else if ((is2d ? c->evalMap2Enabled[7] : c->evalMap1Enabled[7]) && (is2d?c->evalMap2[7].defined:c->evalMap1[7].defined)) { vidx=7; comps=3; }
        if (vidx >= 0)
        {
            const XGL_EVALMAP *m = is2d ? &c->evalMap2[vidx] : &c->evalMap1[vidx];
            float du[4], dv[4];
            if (two && c->enAutoNormal && comps >= 3 &&
                !(c->evalMap2Enabled[2] && c->evalMap2[2].defined))   /* explicit normal map wins */
            {
                float nx, ny, nz, nl;
                EvalMap2At(m, u, v, pos, du, dv);
                nx = du[1]*dv[2]-du[2]*dv[1]; ny = du[2]*dv[0]-du[0]*dv[2]; nz = du[0]*dv[1]-du[1]*dv[0];
                nl = (float)sqrt(nx*nx+ny*ny+nz*nz);
                if (nl > 1e-6f) { nx/=nl; ny/=nl; nz/=nl; }
                xgl_real_Normal3f(nx, ny, nz);
            }
            else if (two) EvalMap2At(m, u, v, pos, NULL, NULL);
            else EvalMap1At(m, u, pos);
            havePos = TRUE;
        }
    }
    if (havePos) xgl_real_Vertex4f(pos[0], pos[1], pos[2], (comps>=4)?pos[3]:1.0f);
}
void GLAPIENTRY xgl_real_EvalCoord1f(GLfloat u){ PXGL_CONTEXT c=XglCurrent(); if(c) EvalDoCoord(c,0,u,0.0f); }
void GLAPIENTRY xgl_real_EvalCoord1d(GLdouble u){ xgl_real_EvalCoord1f((GLfloat)u); }
void GLAPIENTRY xgl_real_EvalCoord1fv(const GLfloat *u){ if(u) xgl_real_EvalCoord1f(u[0]); }
void GLAPIENTRY xgl_real_EvalCoord1dv(const GLdouble *u){ if(u) xgl_real_EvalCoord1f((GLfloat)u[0]); }
void GLAPIENTRY xgl_real_EvalCoord2f(GLfloat u, GLfloat v){ PXGL_CONTEXT c=XglCurrent(); if(c) EvalDoCoord(c,1,u,v); }
void GLAPIENTRY xgl_real_EvalCoord2d(GLdouble u, GLdouble v){ xgl_real_EvalCoord2f((GLfloat)u,(GLfloat)v); }
void GLAPIENTRY xgl_real_EvalCoord2fv(const GLfloat *u){ if(u) xgl_real_EvalCoord2f(u[0],u[1]); }
void GLAPIENTRY xgl_real_EvalCoord2dv(const GLdouble *u){ if(u) xgl_real_EvalCoord2f((GLfloat)u[0],(GLfloat)u[1]); }

static void StoreMap1(PXGL_CONTEXT c, GLenum target, float u1, float u2, int stride, int order, const float *pts)
{
    int comps, is2d, idx = EvalTargetInfo(target, &comps, &is2d), i, k;
    XGL_EVALMAP *m;
    if (idx < 0 || is2d || order < 1 || order > 16 || !pts) return;
    m = &c->evalMap1[idx];
    if (m->points) HeapFree(GetProcessHeap(), 0, m->points);
    m->points = (float*)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)order*comps*sizeof(float));
    if (!m->points) { m->defined = FALSE; return; }
    for (i = 0; i < order; i++) for (k = 0; k < comps; k++) m->points[i*comps+k] = pts[i*stride+k];
    m->components=comps; m->uorder=order; m->vorder=1; m->u1=u1; m->u2=u2; m->v1=0; m->v2=1; m->defined=TRUE;
}
static void StoreMap2(PXGL_CONTEXT c, GLenum target, float u1, float u2, int ustride, int uorder,
                      float v1, float v2, int vstride, int vorder, const float *pts)
{
    int comps, is2d, idx = EvalTargetInfo(target, &comps, &is2d), i, j, k;
    XGL_EVALMAP *m;
    if (idx < 0 || !is2d || uorder < 1 || uorder > 16 || vorder < 1 || vorder > 16 || !pts) return;
    m = &c->evalMap2[idx];
    if (m->points) HeapFree(GetProcessHeap(), 0, m->points);
    m->points = (float*)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)uorder*vorder*comps*sizeof(float));
    if (!m->points) { m->defined = FALSE; return; }
    for (i = 0; i < uorder; i++) for (j = 0; j < vorder; j++) for (k = 0; k < comps; k++)
        m->points[(i*vorder+j)*comps+k] = pts[i*ustride + j*vstride + k];
    m->components=comps; m->uorder=uorder; m->vorder=vorder; m->u1=u1; m->u2=u2; m->v1=v1; m->v2=v2; m->defined=TRUE;
}
void GLAPIENTRY xgl_real_Map1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points)
{ PXGL_CONTEXT c=XglCurrent(); if(c) StoreMap1(c,target,u1,u2,stride,order,points); }
void GLAPIENTRY xgl_real_Map1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points)
{
    PXGL_CONTEXT c=XglCurrent(); float buf[16*4]; int comps,is2d,idx,i,k;
    if(!c||!points) return;
    idx=EvalTargetInfo(target,&comps,&is2d);
    if(idx<0||is2d||order<1||order>16) return;
    for(i=0;i<order;i++) for(k=0;k<comps;k++) buf[i*comps+k]=(float)points[i*stride+k];
    StoreMap1(c,target,(float)u1,(float)u2,comps,order,buf);
}
void GLAPIENTRY xgl_real_Map2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder,
                               GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points)
{ PXGL_CONTEXT c=XglCurrent(); if(c) StoreMap2(c,target,u1,u2,ustride,uorder,v1,v2,vstride,vorder,points); }
void GLAPIENTRY xgl_real_Map2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder,
                               GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points)
{
    PXGL_CONTEXT c=XglCurrent(); float *buf; int comps,is2d,idx,i,j,k;
    if(!c||!points) return;
    idx=EvalTargetInfo(target,&comps,&is2d);
    if(idx<0||!is2d||uorder<1||uorder>16||vorder<1||vorder>16) return;
    buf=(float*)HeapAlloc(GetProcessHeap(),0,(SIZE_T)uorder*vorder*comps*sizeof(float));
    if(!buf) return;
    for(i=0;i<uorder;i++) for(j=0;j<vorder;j++) for(k=0;k<comps;k++) buf[(i*vorder+j)*comps+k]=(float)points[i*ustride+j*vstride+k];
    StoreMap2(c,target,(float)u1,(float)u2,vorder*comps,uorder,(float)v1,(float)v2,comps,vorder,buf);
    HeapFree(GetProcessHeap(),0,buf);
}
void GLAPIENTRY xgl_real_MapGrid1f(GLint un, GLfloat u1, GLfloat u2)
{ PXGL_CONTEXT c=XglCurrent(); if(c){ c->mapGrid1n=un; c->mapGrid1u1=u1; c->mapGrid1u2=u2; } }
void GLAPIENTRY xgl_real_MapGrid1d(GLint un, GLdouble u1, GLdouble u2){ xgl_real_MapGrid1f(un,(GLfloat)u1,(GLfloat)u2); }
void GLAPIENTRY xgl_real_MapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{ PXGL_CONTEXT c=XglCurrent(); if(c){ c->mapGrid2un=un; c->mapGrid2u1=u1; c->mapGrid2u2=u2; c->mapGrid2vn=vn; c->mapGrid2v1=v1; c->mapGrid2v2=v2; } }
void GLAPIENTRY xgl_real_MapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{ xgl_real_MapGrid2f(un,(GLfloat)u1,(GLfloat)u2,vn,(GLfloat)v1,(GLfloat)v2); }
void GLAPIENTRY xgl_real_EvalPoint1(GLint i)
{
    PXGL_CONTEXT c=XglCurrent(); float du;
    if(!c||c->mapGrid1n<=0) return;
    du=(c->mapGrid1u2-c->mapGrid1u1)/(float)c->mapGrid1n;
    xgl_real_EvalCoord1f(c->mapGrid1u1 + (float)i*du);
}
void GLAPIENTRY xgl_real_EvalPoint2(GLint i, GLint j)
{
    PXGL_CONTEXT c=XglCurrent(); float du, dv;
    if(!c||c->mapGrid2un<=0||c->mapGrid2vn<=0) return;
    du=(c->mapGrid2u2-c->mapGrid2u1)/(float)c->mapGrid2un;
    dv=(c->mapGrid2v2-c->mapGrid2v1)/(float)c->mapGrid2vn;
    xgl_real_EvalCoord2f(c->mapGrid2u1 + (float)i*du, c->mapGrid2v1 + (float)j*dv);
}
void GLAPIENTRY xgl_real_EvalMesh1(GLenum mode, GLint i1, GLint i2)
{
    PXGL_CONTEXT c=XglCurrent(); float du; GLint i;
    if(!c||c->mapGrid1n<=0) return;
    du=(c->mapGrid1u2-c->mapGrid1u1)/(float)c->mapGrid1n;
    xgl_real_Begin(mode==GL_POINT ? GL_POINTS : GL_LINE_STRIP);
    for(i=i1;i<=i2;i++) xgl_real_EvalCoord1f(c->mapGrid1u1 + (float)i*du);
    xgl_real_End();
}
void GLAPIENTRY xgl_real_EvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
    PXGL_CONTEXT c=XglCurrent(); float du, dv; GLint i, j;
    if(!c||c->mapGrid2un<=0||c->mapGrid2vn<=0) return;
    du=(c->mapGrid2u2-c->mapGrid2u1)/(float)c->mapGrid2un;
    dv=(c->mapGrid2v2-c->mapGrid2v1)/(float)c->mapGrid2vn;
    if (mode == GL_POINT)
    {
        xgl_real_Begin(GL_POINTS);
        for(i=i1;i<=i2;i++) for(j=j1;j<=j2;j++) xgl_real_EvalCoord2f(c->mapGrid2u1+(float)i*du, c->mapGrid2v1+(float)j*dv);
        xgl_real_End();
        return;
    }
    /* GL_FILL / GL_LINE: emit a quad per grid cell (filled or as the polygon outline). */
    for (i = i1; i < i2; i++)
    {
        xgl_real_Begin(mode==GL_LINE ? GL_LINE_LOOP : GL_QUADS);
        for (j = j1; j < j2; j++)
        {
            float u0=c->mapGrid2u1+(float)i*du, u1v=c->mapGrid2u1+(float)(i+1)*du;
            float v0=c->mapGrid2v1+(float)j*dv, v1v=c->mapGrid2v1+(float)(j+1)*dv;
            xgl_real_EvalCoord2f(u0,v0); xgl_real_EvalCoord2f(u1v,v0);
            xgl_real_EvalCoord2f(u1v,v1v); xgl_real_EvalCoord2f(u0,v1v);
        }
        xgl_real_End();
    }
}

/* Free evaluator control-point storage (icd.c DrvDeleteContext). */
void XglFreeEvalMaps(PXGL_CONTEXT c)
{
    int i;
    if (!c) return;
    for (i = 0; i < XGL_EVAL_TARGETS; i++)
    {
        if (c->evalMap1[i].points) { HeapFree(GetProcessHeap(),0,c->evalMap1[i].points); c->evalMap1[i].points=NULL; }
        if (c->evalMap2[i].points) { HeapFree(GetProcessHeap(),0,c->evalMap2[i].points); c->evalMap2[i].points=NULL; }
    }
}

/* glGetMap* — query an evaluator map's order / domain / control points. */
void GLAPIENTRY xgl_real_GetMapfv(GLenum target, GLenum query, GLfloat *v)
{
    PXGL_CONTEXT c = XglCurrent();
    int comps, is2d, idx, i, n;
    const XGL_EVALMAP *m;
    if (!c || !v) return;
    idx = EvalTargetInfo(target, &comps, &is2d);
    if (idx < 0) return;
    m = is2d ? &c->evalMap2[idx] : &c->evalMap1[idx];
    switch (query) {
        case GL_ORDER:  v[0]=(GLfloat)m->uorder; if (is2d) v[1]=(GLfloat)m->vorder; break;
        case GL_DOMAIN: v[0]=m->u1; v[1]=m->u2; if (is2d) { v[2]=m->v1; v[3]=m->v2; } break;
        case GL_COEFF:  if (m->points) { n=m->uorder*(is2d?m->vorder:1)*comps; for(i=0;i<n;i++) v[i]=m->points[i]; } break;
        default: break;
    }
}
void GLAPIENTRY xgl_real_GetMapdv(GLenum target, GLenum query, GLdouble *v)
{
    PXGL_CONTEXT c = XglCurrent();
    int comps, is2d, idx, i, n;
    const XGL_EVALMAP *m;
    if (!c || !v) return;
    idx = EvalTargetInfo(target, &comps, &is2d);
    if (idx < 0) return;
    m = is2d ? &c->evalMap2[idx] : &c->evalMap1[idx];
    switch (query) {
        case GL_ORDER:  v[0]=(GLdouble)m->uorder; if (is2d) v[1]=(GLdouble)m->vorder; break;
        case GL_DOMAIN: v[0]=m->u1; v[1]=m->u2; if (is2d) { v[2]=m->v1; v[3]=m->v2; } break;
        case GL_COEFF:  if (m->points) { n=m->uorder*(is2d?m->vorder:1)*comps; for(i=0;i<n;i++) v[i]=(GLdouble)m->points[i]; } break;
        default: break;
    }
}
void GLAPIENTRY xgl_real_GetMapiv(GLenum target, GLenum query, GLint *v)
{
    PXGL_CONTEXT c = XglCurrent();
    int comps, is2d, idx, i, n;
    const XGL_EVALMAP *m;
    if (!c || !v) return;
    idx = EvalTargetInfo(target, &comps, &is2d);
    if (idx < 0) return;
    m = is2d ? &c->evalMap2[idx] : &c->evalMap1[idx];
    switch (query) {
        case GL_ORDER:  v[0]=m->uorder; if (is2d) v[1]=m->vorder; break;
        case GL_DOMAIN: v[0]=(GLint)m->u1; v[1]=(GLint)m->u2; if (is2d) { v[2]=(GLint)m->v1; v[3]=(GLint)m->v2; } break;
        case GL_COEFF:  if (m->points) { n=m->uorder*(is2d?m->vorder:1)*comps; for(i=0;i<n;i++) v[i]=(GLint)m->points[i]; } break;
        default: break;
    }
}
