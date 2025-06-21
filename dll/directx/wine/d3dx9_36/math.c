/*
 * Mathematical operations specific to D3DX9.
 *
 * Copyright (C) 2008 David Adam
 * Copyright (C) 2008 Luis Busquets
 * Copyright (C) 2008 Jérôme Gardou
 * Copyright (C) 2008 Philip Nilsson
 * Copyright (C) 2008 Henri Verbeet
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


#include <float.h>

#include "d3dx9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

struct ID3DXMatrixStackImpl
{
  ID3DXMatrixStack ID3DXMatrixStack_iface;
  LONG ref;

  unsigned int current;
  unsigned int stack_size;
  D3DXMATRIX *stack;
};

static const unsigned int INITIAL_STACK_SIZE = 32;

/*_________________D3DXColor____________________*/

D3DXCOLOR* WINAPI D3DXColorAdjustContrast(D3DXCOLOR *pout, const D3DXCOLOR *pc, FLOAT s)
{
    TRACE("pout %p, pc %p, s %f\n", pout, pc, s);

    pout->r = 0.5f + s * (pc->r - 0.5f);
    pout->g = 0.5f + s * (pc->g - 0.5f);
    pout->b = 0.5f + s * (pc->b - 0.5f);
    pout->a = pc->a;
    return pout;
}

D3DXCOLOR* WINAPI D3DXColorAdjustSaturation(D3DXCOLOR *pout, const D3DXCOLOR *pc, FLOAT s)
{
    FLOAT grey;

    TRACE("pout %p, pc %p, s %f\n", pout, pc, s);

    grey = pc->r * 0.2125f + pc->g * 0.7154f + pc->b * 0.0721f;
    pout->r = grey + s * (pc->r - grey);
    pout->g = grey + s * (pc->g - grey);
    pout->b = grey + s * (pc->b - grey);
    pout->a = pc->a;
    return pout;
}

/*_________________Misc__________________________*/

FLOAT WINAPI D3DXFresnelTerm(FLOAT costheta, FLOAT refractionindex)
{
    FLOAT a, d, g, result;

    TRACE("costheta %f, refractionindex %f\n", costheta, refractionindex);

    g = sqrtf(refractionindex * refractionindex + costheta * costheta - 1.0f);
    a = g + costheta;
    d = g - costheta;
    result = (costheta * a - 1.0f) * (costheta * a - 1.0f) / ((costheta * d + 1.0f) * (costheta * d + 1.0f)) + 1.0f;
    result *= 0.5f * d * d / (a * a);

    return result;
}

/*_________________D3DXMatrix____________________*/

D3DXMATRIX * WINAPI D3DXMatrixAffineTransformation(D3DXMATRIX *out, FLOAT scaling, const D3DXVECTOR3 *rotationcenter,
        const D3DXQUATERNION *rotation, const D3DXVECTOR3 *translation)
{
    TRACE("out %p, scaling %f, rotationcenter %p, rotation %p, translation %p\n",
            out, scaling, rotationcenter, rotation, translation);

    D3DXMatrixIdentity(out);

    if (rotation)
    {
        FLOAT temp00, temp01, temp02, temp10, temp11, temp12, temp20, temp21, temp22;

        temp00 = 1.0f - 2.0f * (rotation->y * rotation->y + rotation->z * rotation->z);
        temp01 = 2.0f * (rotation->x * rotation->y + rotation->z * rotation->w);
        temp02 = 2.0f * (rotation->x * rotation->z - rotation->y * rotation->w);
        temp10 = 2.0f * (rotation->x * rotation->y - rotation->z * rotation->w);
        temp11 = 1.0f - 2.0f * (rotation->x * rotation->x + rotation->z * rotation->z);
        temp12 = 2.0f * (rotation->y * rotation->z + rotation->x * rotation->w);
        temp20 = 2.0f * (rotation->x * rotation->z + rotation->y * rotation->w);
        temp21 = 2.0f * (rotation->y * rotation->z - rotation->x * rotation->w);
        temp22 = 1.0f - 2.0f * (rotation->x * rotation->x + rotation->y * rotation->y);

        out->m[0][0] = scaling * temp00;
        out->m[0][1] = scaling * temp01;
        out->m[0][2] = scaling * temp02;
        out->m[1][0] = scaling * temp10;
        out->m[1][1] = scaling * temp11;
        out->m[1][2] = scaling * temp12;
        out->m[2][0] = scaling * temp20;
        out->m[2][1] = scaling * temp21;
        out->m[2][2] = scaling * temp22;

        if (rotationcenter)
        {
            out->m[3][0] = rotationcenter->x * (1.0f - temp00) - rotationcenter->y * temp10
                    - rotationcenter->z * temp20;
            out->m[3][1] = rotationcenter->y * (1.0f - temp11) - rotationcenter->x * temp01
                    - rotationcenter->z * temp21;
            out->m[3][2] = rotationcenter->z * (1.0f - temp22) - rotationcenter->x * temp02
                    - rotationcenter->y * temp12;
        }
    }
    else
    {
        out->m[0][0] = scaling;
        out->m[1][1] = scaling;
        out->m[2][2] = scaling;
    }

    if (translation)
    {
        out->m[3][0] += translation->x;
        out->m[3][1] += translation->y;
        out->m[3][2] += translation->z;
    }

    return out;
}

D3DXMATRIX * WINAPI D3DXMatrixAffineTransformation2D(D3DXMATRIX *out, FLOAT scaling,
        const D3DXVECTOR2 *rotationcenter, FLOAT rotation, const D3DXVECTOR2 *translation)
{
    FLOAT tmp1, tmp2, s;

    TRACE("out %p, scaling %f, rotationcenter %p, rotation %f, translation %p\n",
            out, scaling, rotationcenter, rotation, translation);

    s = sinf(rotation / 2.0f);
    tmp1 = 1.0f - 2.0f * s * s;
    tmp2 = 2.0f * s * cosf(rotation / 2.0f);

    D3DXMatrixIdentity(out);
    out->m[0][0] = scaling * tmp1;
    out->m[0][1] = scaling * tmp2;
    out->m[1][0] = -scaling * tmp2;
    out->m[1][1] = scaling * tmp1;

    if (rotationcenter)
    {
        FLOAT x, y;

        x = rotationcenter->x;
        y = rotationcenter->y;

        out->m[3][0] = y * tmp2 - x * tmp1 + x;
        out->m[3][1] = -x * tmp2 - y * tmp1 + y;
    }

    if (translation)
    {
        out->m[3][0] += translation->x;
        out->m[3][1] += translation->y;
    }

    return out;
}

HRESULT WINAPI D3DXMatrixDecompose(D3DXVECTOR3 *poutscale, D3DXQUATERNION *poutrotation, D3DXVECTOR3 *pouttranslation, const D3DXMATRIX *pm)
{
    D3DXMATRIX normalized;
    D3DXVECTOR3 vec;

    TRACE("poutscale %p, poutrotation %p, pouttranslation %p, pm %p\n", poutscale, poutrotation, pouttranslation, pm);

    /*Compute the scaling part.*/
    vec.x = pm->m[0][0];
    vec.y = pm->m[0][1];
    vec.z = pm->m[0][2];
    poutscale->x = D3DXVec3Length(&vec);

    vec.x = pm->m[1][0];
    vec.y = pm->m[1][1];
    vec.z = pm->m[1][2];
    poutscale->y = D3DXVec3Length(&vec);

    vec.x = pm->m[2][0];
    vec.y = pm->m[2][1];
    vec.z = pm->m[2][2];
    poutscale->z = D3DXVec3Length(&vec);

    /*Compute the translation part.*/
    pouttranslation->x = pm->m[3][0];
    pouttranslation->y = pm->m[3][1];
    pouttranslation->z = pm->m[3][2];

    /*Let's calculate the rotation now*/
    if ( (poutscale->x == 0.0f) || (poutscale->y == 0.0f) || (poutscale->z == 0.0f) ) return D3DERR_INVALIDCALL;

    normalized.m[0][0] = pm->m[0][0]/poutscale->x;
    normalized.m[0][1] = pm->m[0][1]/poutscale->x;
    normalized.m[0][2] = pm->m[0][2]/poutscale->x;
    normalized.m[1][0] = pm->m[1][0]/poutscale->y;
    normalized.m[1][1] = pm->m[1][1]/poutscale->y;
    normalized.m[1][2] = pm->m[1][2]/poutscale->y;
    normalized.m[2][0] = pm->m[2][0]/poutscale->z;
    normalized.m[2][1] = pm->m[2][1]/poutscale->z;
    normalized.m[2][2] = pm->m[2][2]/poutscale->z;

    D3DXQuaternionRotationMatrix(poutrotation,&normalized);
    return S_OK;
}

FLOAT WINAPI D3DXMatrixDeterminant(const D3DXMATRIX *pm)
{
    FLOAT t[3], v[4];

    TRACE("pm %p\n", pm);

    t[0] = pm->m[2][2] * pm->m[3][3] - pm->m[2][3] * pm->m[3][2];
    t[1] = pm->m[1][2] * pm->m[3][3] - pm->m[1][3] * pm->m[3][2];
    t[2] = pm->m[1][2] * pm->m[2][3] - pm->m[1][3] * pm->m[2][2];
    v[0] = pm->m[1][1] * t[0] - pm->m[2][1] * t[1] + pm->m[3][1] * t[2];
    v[1] = -pm->m[1][0] * t[0] + pm->m[2][0] * t[1] - pm->m[3][0] * t[2];

    t[0] = pm->m[1][0] * pm->m[2][1] - pm->m[2][0] * pm->m[1][1];
    t[1] = pm->m[1][0] * pm->m[3][1] - pm->m[3][0] * pm->m[1][1];
    t[2] = pm->m[2][0] * pm->m[3][1] - pm->m[3][0] * pm->m[2][1];
    v[2] = pm->m[3][3] * t[0] - pm->m[2][3] * t[1] + pm->m[1][3] * t[2];
    v[3] = -pm->m[3][2] * t[0] + pm->m[2][2] * t[1] - pm->m[1][2] * t[2];

    return pm->m[0][0] * v[0] + pm->m[0][1] * v[1] +
        pm->m[0][2] * v[2] + pm->m[0][3] * v[3];
}

D3DXMATRIX* WINAPI D3DXMatrixInverse(D3DXMATRIX *pout, FLOAT *pdeterminant, const D3DXMATRIX *pm)
{
    FLOAT det, t[3], v[16];
    UINT i, j;

    TRACE("pout %p, pdeterminant %p, pm %p\n", pout, pdeterminant, pm);

    t[0] = pm->m[2][2] * pm->m[3][3] - pm->m[2][3] * pm->m[3][2];
    t[1] = pm->m[1][2] * pm->m[3][3] - pm->m[1][3] * pm->m[3][2];
    t[2] = pm->m[1][2] * pm->m[2][3] - pm->m[1][3] * pm->m[2][2];
    v[0] = pm->m[1][1] * t[0] - pm->m[2][1] * t[1] + pm->m[3][1] * t[2];
    v[4] = -pm->m[1][0] * t[0] + pm->m[2][0] * t[1] - pm->m[3][0] * t[2];

    t[0] = pm->m[1][0] * pm->m[2][1] - pm->m[2][0] * pm->m[1][1];
    t[1] = pm->m[1][0] * pm->m[3][1] - pm->m[3][0] * pm->m[1][1];
    t[2] = pm->m[2][0] * pm->m[3][1] - pm->m[3][0] * pm->m[2][1];
    v[8] = pm->m[3][3] * t[0] - pm->m[2][3] * t[1] + pm->m[1][3] * t[2];
    v[12] = -pm->m[3][2] * t[0] + pm->m[2][2] * t[1] - pm->m[1][2] * t[2];

    det = pm->m[0][0] * v[0] + pm->m[0][1] * v[4] +
        pm->m[0][2] * v[8] + pm->m[0][3] * v[12];
    if (det == 0.0f)
        return NULL;
    if (pdeterminant)
        *pdeterminant = det;

    t[0] = pm->m[2][2] * pm->m[3][3] - pm->m[2][3] * pm->m[3][2];
    t[1] = pm->m[0][2] * pm->m[3][3] - pm->m[0][3] * pm->m[3][2];
    t[2] = pm->m[0][2] * pm->m[2][3] - pm->m[0][3] * pm->m[2][2];
    v[1] = -pm->m[0][1] * t[0] + pm->m[2][1] * t[1] - pm->m[3][1] * t[2];
    v[5] = pm->m[0][0] * t[0] - pm->m[2][0] * t[1] + pm->m[3][0] * t[2];

    t[0] = pm->m[0][0] * pm->m[2][1] - pm->m[2][0] * pm->m[0][1];
    t[1] = pm->m[3][0] * pm->m[0][1] - pm->m[0][0] * pm->m[3][1];
    t[2] = pm->m[2][0] * pm->m[3][1] - pm->m[3][0] * pm->m[2][1];
    v[9] = -pm->m[3][3] * t[0] - pm->m[2][3] * t[1]- pm->m[0][3] * t[2];
    v[13] = pm->m[3][2] * t[0] + pm->m[2][2] * t[1] + pm->m[0][2] * t[2];

    t[0] = pm->m[1][2] * pm->m[3][3] - pm->m[1][3] * pm->m[3][2];
    t[1] = pm->m[0][2] * pm->m[3][3] - pm->m[0][3] * pm->m[3][2];
    t[2] = pm->m[0][2] * pm->m[1][3] - pm->m[0][3] * pm->m[1][2];
    v[2] = pm->m[0][1] * t[0] - pm->m[1][1] * t[1] + pm->m[3][1] * t[2];
    v[6] = -pm->m[0][0] * t[0] + pm->m[1][0] * t[1] - pm->m[3][0] * t[2];

    t[0] = pm->m[0][0] * pm->m[1][1] - pm->m[1][0] * pm->m[0][1];
    t[1] = pm->m[3][0] * pm->m[0][1] - pm->m[0][0] * pm->m[3][1];
    t[2] = pm->m[1][0] * pm->m[3][1] - pm->m[3][0] * pm->m[1][1];
    v[10] = pm->m[3][3] * t[0] + pm->m[1][3] * t[1] + pm->m[0][3] * t[2];
    v[14] = -pm->m[3][2] * t[0] - pm->m[1][2] * t[1] - pm->m[0][2] * t[2];

    t[0] = pm->m[1][2] * pm->m[2][3] - pm->m[1][3] * pm->m[2][2];
    t[1] = pm->m[0][2] * pm->m[2][3] - pm->m[0][3] * pm->m[2][2];
    t[2] = pm->m[0][2] * pm->m[1][3] - pm->m[0][3] * pm->m[1][2];
    v[3] = -pm->m[0][1] * t[0] + pm->m[1][1] * t[1] - pm->m[2][1] * t[2];
    v[7] = pm->m[0][0] * t[0] - pm->m[1][0] * t[1] + pm->m[2][0] * t[2];

    v[11] = -pm->m[0][0] * (pm->m[1][1] * pm->m[2][3] - pm->m[1][3] * pm->m[2][1]) +
        pm->m[1][0] * (pm->m[0][1] * pm->m[2][3] - pm->m[0][3] * pm->m[2][1]) -
        pm->m[2][0] * (pm->m[0][1] * pm->m[1][3] - pm->m[0][3] * pm->m[1][1]);

    v[15] = pm->m[0][0] * (pm->m[1][1] * pm->m[2][2] - pm->m[1][2] * pm->m[2][1]) -
        pm->m[1][0] * (pm->m[0][1] * pm->m[2][2] - pm->m[0][2] * pm->m[2][1]) +
        pm->m[2][0] * (pm->m[0][1] * pm->m[1][2] - pm->m[0][2] * pm->m[1][1]);

    det = 1.0f / det;

    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            pout->m[i][j] = v[4 * i + j] * det;

    return pout;
}

D3DXMATRIX * WINAPI D3DXMatrixLookAtLH(D3DXMATRIX *out, const D3DXVECTOR3 *eye, const D3DXVECTOR3 *at,
        const D3DXVECTOR3 *up)
{
    D3DXVECTOR3 right, upn, vec;

    TRACE("out %p, eye %p, at %p, up %p\n", out, eye, at, up);

    D3DXVec3Subtract(&vec, at, eye);
    D3DXVec3Normalize(&vec, &vec);
    D3DXVec3Cross(&right, up, &vec);
    D3DXVec3Cross(&upn, &vec, &right);
    D3DXVec3Normalize(&right, &right);
    D3DXVec3Normalize(&upn, &upn);
    out->m[0][0] = right.x;
    out->m[1][0] = right.y;
    out->m[2][0] = right.z;
    out->m[3][0] = -D3DXVec3Dot(&right, eye);
    out->m[0][1] = upn.x;
    out->m[1][1] = upn.y;
    out->m[2][1] = upn.z;
    out->m[3][1] = -D3DXVec3Dot(&upn, eye);
    out->m[0][2] = vec.x;
    out->m[1][2] = vec.y;
    out->m[2][2] = vec.z;
    out->m[3][2] = -D3DXVec3Dot(&vec, eye);
    out->m[0][3] = 0.0f;
    out->m[1][3] = 0.0f;
    out->m[2][3] = 0.0f;
    out->m[3][3] = 1.0f;

    return out;
}

D3DXMATRIX * WINAPI D3DXMatrixLookAtRH(D3DXMATRIX *out, const D3DXVECTOR3 *eye, const D3DXVECTOR3 *at,
        const D3DXVECTOR3 *up)
{
    D3DXVECTOR3 right, upn, vec;

    TRACE("out %p, eye %p, at %p, up %p\n", out, eye, at, up);

    D3DXVec3Subtract(&vec, at, eye);
    D3DXVec3Normalize(&vec, &vec);
    D3DXVec3Cross(&right, up, &vec);
    D3DXVec3Cross(&upn, &vec, &right);
    D3DXVec3Normalize(&right, &right);
    D3DXVec3Normalize(&upn, &upn);
    out->m[0][0] = -right.x;
    out->m[1][0] = -right.y;
    out->m[2][0] = -right.z;
    out->m[3][0] = D3DXVec3Dot(&right, eye);
    out->m[0][1] = upn.x;
    out->m[1][1] = upn.y;
    out->m[2][1] = upn.z;
    out->m[3][1] = -D3DXVec3Dot(&upn, eye);
    out->m[0][2] = -vec.x;
    out->m[1][2] = -vec.y;
    out->m[2][2] = -vec.z;
    out->m[3][2] = D3DXVec3Dot(&vec, eye);
    out->m[0][3] = 0.0f;
    out->m[1][3] = 0.0f;
    out->m[2][3] = 0.0f;
    out->m[3][3] = 1.0f;

    return out;
}

D3DXMATRIX* WINAPI D3DXMatrixMultiply(D3DXMATRIX *pout, const D3DXMATRIX *pm1, const D3DXMATRIX *pm2)
{
    D3DXMATRIX out;
    int i,j;

    TRACE("pout %p, pm1 %p, pm2 %p\n", pout, pm1, pm2);

    for (i=0; i<4; i++)
    {
        for (j=0; j<4; j++)
        {
            out.m[i][j] = pm1->m[i][0] * pm2->m[0][j] + pm1->m[i][1] * pm2->m[1][j] + pm1->m[i][2] * pm2->m[2][j] + pm1->m[i][3] * pm2->m[3][j];
        }
    }

    *pout = out;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixMultiplyTranspose(D3DXMATRIX *pout, const D3DXMATRIX *pm1, const D3DXMATRIX *pm2)
{
    D3DXMATRIX temp;
    int i, j;

    TRACE("pout %p, pm1 %p, pm2 %p\n", pout, pm1, pm2);

    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            temp.m[j][i] = pm1->m[i][0] * pm2->m[0][j] + pm1->m[i][1] * pm2->m[1][j] + pm1->m[i][2] * pm2->m[2][j] + pm1->m[i][3] * pm2->m[3][j];

    *pout = temp;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixOrthoLH(D3DXMATRIX *pout, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf)
{
    TRACE("pout %p, w %f, h %f, zn %f, zf %f\n", pout, w, h, zn, zf);

    D3DXMatrixIdentity(pout);
    pout->m[0][0] = 2.0f / w;
    pout->m[1][1] = 2.0f / h;
    pout->m[2][2] = 1.0f / (zf - zn);
    pout->m[3][2] = zn / (zn - zf);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixOrthoOffCenterLH(D3DXMATRIX *pout, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn, FLOAT zf)
{
    TRACE("pout %p, l %f, r %f, b %f, t %f, zn %f, zf %f\n", pout, l, r, b, t, zn, zf);

    D3DXMatrixIdentity(pout);
    pout->m[0][0] = 2.0f / (r - l);
    pout->m[1][1] = 2.0f / (t - b);
    pout->m[2][2] = 1.0f / (zf -zn);
    pout->m[3][0] = -1.0f -2.0f *l / (r - l);
    pout->m[3][1] = 1.0f + 2.0f * t / (b - t);
    pout->m[3][2] = zn / (zn -zf);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixOrthoOffCenterRH(D3DXMATRIX *pout, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn, FLOAT zf)
{
    TRACE("pout %p, l %f, r %f, b %f, t %f, zn %f, zf %f\n", pout, l, r, b, t, zn, zf);

    D3DXMatrixIdentity(pout);
    pout->m[0][0] = 2.0f / (r - l);
    pout->m[1][1] = 2.0f / (t - b);
    pout->m[2][2] = 1.0f / (zn -zf);
    pout->m[3][0] = -1.0f -2.0f *l / (r - l);
    pout->m[3][1] = 1.0f + 2.0f * t / (b - t);
    pout->m[3][2] = zn / (zn -zf);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixOrthoRH(D3DXMATRIX *pout, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf)
{
    TRACE("pout %p, w %f, h %f, zn %f, zf %f\n", pout, w, h, zn, zf);

    D3DXMatrixIdentity(pout);
    pout->m[0][0] = 2.0f / w;
    pout->m[1][1] = 2.0f / h;
    pout->m[2][2] = 1.0f / (zn - zf);
    pout->m[3][2] = zn / (zn - zf);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixPerspectiveFovLH(D3DXMATRIX *pout, FLOAT fovy, FLOAT aspect, FLOAT zn, FLOAT zf)
{
    TRACE("pout %p, fovy %f, aspect %f, zn %f, zf %f\n", pout, fovy, aspect, zn, zf);

    D3DXMatrixIdentity(pout);
    pout->m[0][0] = 1.0f / (aspect * tanf(fovy/2.0f));
    pout->m[1][1] = 1.0f / tanf(fovy/2.0f);
    pout->m[2][2] = zf / (zf - zn);
    pout->m[2][3] = 1.0f;
    pout->m[3][2] = (zf * zn) / (zn - zf);
    pout->m[3][3] = 0.0f;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixPerspectiveFovRH(D3DXMATRIX *pout, FLOAT fovy, FLOAT aspect, FLOAT zn, FLOAT zf)
{
    TRACE("pout %p, fovy %f, aspect %f, zn %f, zf %f\n", pout, fovy, aspect, zn, zf);

    D3DXMatrixIdentity(pout);
    pout->m[0][0] = 1.0f / (aspect * tanf(fovy/2.0f));
    pout->m[1][1] = 1.0f / tanf(fovy/2.0f);
    pout->m[2][2] = zf / (zn - zf);
    pout->m[2][3] = -1.0f;
    pout->m[3][2] = (zf * zn) / (zn - zf);
    pout->m[3][3] = 0.0f;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixPerspectiveLH(D3DXMATRIX *pout, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf)
{
    TRACE("pout %p, w %f, h %f, zn %f, zf %f\n", pout, w, h, zn, zf);

    D3DXMatrixIdentity(pout);
    pout->m[0][0] = 2.0f * zn / w;
    pout->m[1][1] = 2.0f * zn / h;
    pout->m[2][2] = zf / (zf - zn);
    pout->m[3][2] = (zn * zf) / (zn - zf);
    pout->m[2][3] = 1.0f;
    pout->m[3][3] = 0.0f;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixPerspectiveOffCenterLH(D3DXMATRIX *pout, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn, FLOAT zf)
{
    TRACE("pout %p, l %f, r %f, b %f, t %f, zn %f, zf %f\n", pout, l, r, b, t, zn, zf);

    D3DXMatrixIdentity(pout);
    pout->m[0][0] = 2.0f * zn / (r - l);
    pout->m[1][1] = -2.0f * zn / (b - t);
    pout->m[2][0] = -1.0f - 2.0f * l / (r - l);
    pout->m[2][1] = 1.0f + 2.0f * t / (b - t);
    pout->m[2][2] = - zf / (zn - zf);
    pout->m[3][2] = (zn * zf) / (zn -zf);
    pout->m[2][3] = 1.0f;
    pout->m[3][3] = 0.0f;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixPerspectiveOffCenterRH(D3DXMATRIX *pout, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn, FLOAT zf)
{
    TRACE("pout %p, l %f, r %f, b %f, t %f, zn %f, zf %f\n", pout, l, r, b, t, zn, zf);

    D3DXMatrixIdentity(pout);
    pout->m[0][0] = 2.0f * zn / (r - l);
    pout->m[1][1] = -2.0f * zn / (b - t);
    pout->m[2][0] = 1.0f + 2.0f * l / (r - l);
    pout->m[2][1] = -1.0f -2.0f * t / (b - t);
    pout->m[2][2] = zf / (zn - zf);
    pout->m[3][2] = (zn * zf) / (zn -zf);
    pout->m[2][3] = -1.0f;
    pout->m[3][3] = 0.0f;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixPerspectiveRH(D3DXMATRIX *pout, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf)
{
    TRACE("pout %p, w %f, h %f, zn %f, zf %f\n", pout, w, h, zn, zf);

    D3DXMatrixIdentity(pout);
    pout->m[0][0] = 2.0f * zn / w;
    pout->m[1][1] = 2.0f * zn / h;
    pout->m[2][2] = zf / (zn - zf);
    pout->m[3][2] = (zn * zf) / (zn - zf);
    pout->m[2][3] = -1.0f;
    pout->m[3][3] = 0.0f;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixReflect(D3DXMATRIX *pout, const D3DXPLANE *pplane)
{
    D3DXPLANE Nplane;

    TRACE("pout %p, pplane %p\n", pout, pplane);

    D3DXPlaneNormalize(&Nplane, pplane);
    D3DXMatrixIdentity(pout);
    pout->m[0][0] = 1.0f - 2.0f * Nplane.a * Nplane.a;
    pout->m[0][1] = -2.0f * Nplane.a * Nplane.b;
    pout->m[0][2] = -2.0f * Nplane.a * Nplane.c;
    pout->m[1][0] = -2.0f * Nplane.a * Nplane.b;
    pout->m[1][1] = 1.0f - 2.0f * Nplane.b * Nplane.b;
    pout->m[1][2] = -2.0f * Nplane.b * Nplane.c;
    pout->m[2][0] = -2.0f * Nplane.c * Nplane.a;
    pout->m[2][1] = -2.0f * Nplane.c * Nplane.b;
    pout->m[2][2] = 1.0f - 2.0f * Nplane.c * Nplane.c;
    pout->m[3][0] = -2.0f * Nplane.d * Nplane.a;
    pout->m[3][1] = -2.0f * Nplane.d * Nplane.b;
    pout->m[3][2] = -2.0f * Nplane.d * Nplane.c;
    return pout;
}

D3DXMATRIX * WINAPI D3DXMatrixRotationAxis(D3DXMATRIX *out, const D3DXVECTOR3 *v, FLOAT angle)
{
    D3DXVECTOR3 nv;
    FLOAT sangle, cangle, cdiff;

    TRACE("out %p, v %p, angle %f\n", out, v, angle);

    D3DXVec3Normalize(&nv, v);
    sangle = sinf(angle);
    cangle = cosf(angle);
    cdiff = 1.0f - cangle;

    out->m[0][0] = cdiff * nv.x * nv.x + cangle;
    out->m[1][0] = cdiff * nv.x * nv.y - sangle * nv.z;
    out->m[2][0] = cdiff * nv.x * nv.z + sangle * nv.y;
    out->m[3][0] = 0.0f;
    out->m[0][1] = cdiff * nv.y * nv.x + sangle * nv.z;
    out->m[1][1] = cdiff * nv.y * nv.y + cangle;
    out->m[2][1] = cdiff * nv.y * nv.z - sangle * nv.x;
    out->m[3][1] = 0.0f;
    out->m[0][2] = cdiff * nv.z * nv.x - sangle * nv.y;
    out->m[1][2] = cdiff * nv.z * nv.y + sangle * nv.x;
    out->m[2][2] = cdiff * nv.z * nv.z + cangle;
    out->m[3][2] = 0.0f;
    out->m[0][3] = 0.0f;
    out->m[1][3] = 0.0f;
    out->m[2][3] = 0.0f;
    out->m[3][3] = 1.0f;

    return out;
}

D3DXMATRIX* WINAPI D3DXMatrixRotationQuaternion(D3DXMATRIX *pout, const D3DXQUATERNION *pq)
{
    TRACE("pout %p, pq %p\n", pout, pq);

    D3DXMatrixIdentity(pout);
    pout->m[0][0] = 1.0f - 2.0f * (pq->y * pq->y + pq->z * pq->z);
    pout->m[0][1] = 2.0f * (pq->x *pq->y + pq->z * pq->w);
    pout->m[0][2] = 2.0f * (pq->x * pq->z - pq->y * pq->w);
    pout->m[1][0] = 2.0f * (pq->x * pq->y - pq->z * pq->w);
    pout->m[1][1] = 1.0f - 2.0f * (pq->x * pq->x + pq->z * pq->z);
    pout->m[1][2] = 2.0f * (pq->y *pq->z + pq->x *pq->w);
    pout->m[2][0] = 2.0f * (pq->x * pq->z + pq->y * pq->w);
    pout->m[2][1] = 2.0f * (pq->y *pq->z - pq->x *pq->w);
    pout->m[2][2] = 1.0f - 2.0f * (pq->x * pq->x + pq->y * pq->y);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixRotationX(D3DXMATRIX *pout, FLOAT angle)
{
    TRACE("pout %p, angle %f\n", pout, angle);

    D3DXMatrixIdentity(pout);
    pout->m[1][1] = cosf(angle);
    pout->m[2][2] = cosf(angle);
    pout->m[1][2] = sinf(angle);
    pout->m[2][1] = -sinf(angle);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixRotationY(D3DXMATRIX *pout, FLOAT angle)
{
    TRACE("pout %p, angle %f\n", pout, angle);

    D3DXMatrixIdentity(pout);
    pout->m[0][0] = cosf(angle);
    pout->m[2][2] = cosf(angle);
    pout->m[0][2] = -sinf(angle);
    pout->m[2][0] = sinf(angle);
    return pout;
}

D3DXMATRIX * WINAPI D3DXMatrixRotationYawPitchRoll(D3DXMATRIX *out, FLOAT yaw, FLOAT pitch, FLOAT roll)
{
    FLOAT sroll, croll, spitch, cpitch, syaw, cyaw;

    TRACE("out %p, yaw %f, pitch %f, roll %f\n", out, yaw, pitch, roll);

    sroll = sinf(roll);
    croll = cosf(roll);
    spitch = sinf(pitch);
    cpitch = cosf(pitch);
    syaw = sinf(yaw);
    cyaw = cosf(yaw);

    out->m[0][0] = sroll * spitch * syaw + croll * cyaw;
    out->m[0][1] = sroll * cpitch;
    out->m[0][2] = sroll * spitch * cyaw - croll * syaw;
    out->m[0][3] = 0.0f;
    out->m[1][0] = croll * spitch * syaw - sroll * cyaw;
    out->m[1][1] = croll * cpitch;
    out->m[1][2] = croll * spitch * cyaw + sroll * syaw;
    out->m[1][3] = 0.0f;
    out->m[2][0] = cpitch * syaw;
    out->m[2][1] = -spitch;
    out->m[2][2] = cpitch * cyaw;
    out->m[2][3] = 0.0f;
    out->m[3][0] = 0.0f;
    out->m[3][1] = 0.0f;
    out->m[3][2] = 0.0f;
    out->m[3][3] = 1.0f;

    return out;
}

D3DXMATRIX* WINAPI D3DXMatrixRotationZ(D3DXMATRIX *pout, FLOAT angle)
{
    TRACE("pout %p, angle %f\n", pout, angle);

    D3DXMatrixIdentity(pout);
    pout->m[0][0] = cosf(angle);
    pout->m[1][1] = cosf(angle);
    pout->m[0][1] = sinf(angle);
    pout->m[1][0] = -sinf(angle);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixScaling(D3DXMATRIX *pout, FLOAT sx, FLOAT sy, FLOAT sz)
{
    TRACE("pout %p, sx %f, sy %f, sz %f\n", pout, sx, sy, sz);

    D3DXMatrixIdentity(pout);
    pout->m[0][0] = sx;
    pout->m[1][1] = sy;
    pout->m[2][2] = sz;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixShadow(D3DXMATRIX *pout, const D3DXVECTOR4 *plight, const D3DXPLANE *pplane)
{
    D3DXPLANE Nplane;
    FLOAT dot;

    TRACE("pout %p, plight %p, pplane %p\n", pout, plight, pplane);

    D3DXPlaneNormalize(&Nplane, pplane);
    dot = D3DXPlaneDot(&Nplane, plight);
    pout->m[0][0] = dot - Nplane.a * plight->x;
    pout->m[0][1] = -Nplane.a * plight->y;
    pout->m[0][2] = -Nplane.a * plight->z;
    pout->m[0][3] = -Nplane.a * plight->w;
    pout->m[1][0] = -Nplane.b * plight->x;
    pout->m[1][1] = dot - Nplane.b * plight->y;
    pout->m[1][2] = -Nplane.b * plight->z;
    pout->m[1][3] = -Nplane.b * plight->w;
    pout->m[2][0] = -Nplane.c * plight->x;
    pout->m[2][1] = -Nplane.c * plight->y;
    pout->m[2][2] = dot - Nplane.c * plight->z;
    pout->m[2][3] = -Nplane.c * plight->w;
    pout->m[3][0] = -Nplane.d * plight->x;
    pout->m[3][1] = -Nplane.d * plight->y;
    pout->m[3][2] = -Nplane.d * plight->z;
    pout->m[3][3] = dot - Nplane.d * plight->w;
    return pout;
}

D3DXMATRIX * WINAPI D3DXMatrixTransformation(D3DXMATRIX *out, const D3DXVECTOR3 *scaling_center,
        const D3DXQUATERNION *scaling_rotation, const D3DXVECTOR3 *scaling,
        const D3DXVECTOR3 *rotation_center, const D3DXQUATERNION *rotation,
        const D3DXVECTOR3 *translation)
{
    static const D3DXVECTOR3 zero_vector;
    D3DXMATRIX m1, msr1, ms, msr, msc, mrc1, mr, mrc, mt;
    D3DXVECTOR3 sc, rc;
    D3DXQUATERNION q;

    TRACE("out %p, scaling_center %p, scaling_rotation %p, scaling %p, rotation_center %p,"
            " rotation %p, translation %p.\n",
            out, scaling_center, scaling_rotation, scaling, rotation_center, rotation, translation);

    if (scaling)
    {
        sc = scaling_center ? *scaling_center : zero_vector;
        D3DXMatrixTranslation(&m1, -sc.x, -sc.y, -sc.z);
        if (scaling_rotation)
        {
            q.x = -scaling_rotation->x;
            q.y = -scaling_rotation->y;
            q.z = -scaling_rotation->z;
            q.w = scaling_rotation->w;
            D3DXMatrixRotationQuaternion(&msr1, &q);
            D3DXMatrixMultiply(&m1, &m1, &msr1);
        }
        D3DXMatrixScaling(&ms, scaling->x, scaling->y, scaling->z);
        D3DXMatrixMultiply(&m1, &m1, &ms);
        if (scaling_rotation)
        {
            D3DXMatrixRotationQuaternion(&msr, scaling_rotation);
            D3DXMatrixMultiply(&m1, &m1, &msr);
        }
        D3DXMatrixTranslation(&msc, sc.x, sc.y, sc.z);
        D3DXMatrixMultiply(&m1, &m1, &msc);
    }
    else
    {
        D3DXMatrixIdentity(&m1);
    }

    if (rotation)
    {
        rc = rotation_center ? *rotation_center : zero_vector;
        D3DXMatrixTranslation(&mrc1, -rc.x, -rc.y, -rc.z);
        D3DXMatrixMultiply(&m1, &m1, &mrc1);
        D3DXMatrixRotationQuaternion(&mr, rotation);
        D3DXMatrixMultiply(&m1, &m1, &mr);
        D3DXMatrixTranslation(&mrc, rc.x, rc.y, rc.z);
        D3DXMatrixMultiply(&m1, &m1, &mrc);
    }

    if (translation)
    {
        D3DXMatrixTranslation(&mt, translation->x, translation->y, translation->z);
        D3DXMatrixMultiply(out, &m1, &mt);
    }
    else
    {
        *out = m1;
    }

    return out;
}

static void vec3_from_vec2(D3DXVECTOR3 *v3, const D3DXVECTOR2 *v2)
{
    if (!v2)
        return;

    v3->x = v2->x;
    v3->y = v2->y;
    v3->z = 0.0f;
}

D3DXMATRIX * WINAPI D3DXMatrixTransformation2D(D3DXMATRIX *out, const D3DXVECTOR2 *scaling_center,
        float scaling_rotation, const D3DXVECTOR2 *scaling, const D3DXVECTOR2 *rotation_center,
        float rotation, const D3DXVECTOR2 *translation)
{
    D3DXVECTOR3 r_c, s, s_c, t;
    D3DXQUATERNION r, s_r;

    TRACE("out %p, scaling_center %p, scaling_rotation %.8e, scaling %p, rotation_center %p, "
            "rotation %.8e, translation %p.\n",
            out, scaling_center, scaling_rotation, scaling, rotation_center, rotation, translation);

    vec3_from_vec2(&s_c, scaling_center);
    vec3_from_vec2(&s, scaling);
    if (scaling)
        s.z = 1.0f;
    vec3_from_vec2(&r_c, rotation_center);
    vec3_from_vec2(&t, translation);

    if (rotation)
    {
        r.w = cosf(rotation / 2.0f);
        r.x = 0.0f;
        r.y = 0.0f;
        r.z = sinf(rotation / 2.0f);
    }

    if (scaling_rotation)
    {
        s_r.w = cosf(scaling_rotation / 2.0f);
        s_r.x = 0.0f;
        s_r.y = 0.0f;
        s_r.z = sinf(scaling_rotation / 2.0f);
    }

    return D3DXMatrixTransformation(out, scaling_center ? &s_c : NULL,
            scaling_rotation ? &s_r : NULL, scaling ? &s : NULL, rotation_center ? &r_c: NULL,
            rotation ? &r : NULL, translation ? &t : NULL);
}

D3DXMATRIX* WINAPI D3DXMatrixTranslation(D3DXMATRIX *pout, FLOAT x, FLOAT y, FLOAT z)
{
    TRACE("pout %p, x %f, y %f, z %f\n", pout, x, y, z);

    D3DXMatrixIdentity(pout);
    pout->m[3][0] = x;
    pout->m[3][1] = y;
    pout->m[3][2] = z;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixTranspose(D3DXMATRIX *out, const D3DXMATRIX *in)
{
    unsigned int i, j;
    D3DXMATRIX m;

    TRACE("out %p, in %p.\n", out, in);

    m = *in;

    for (i = 0; i < 4; ++i)
        for (j = 0; j < 4; ++j) out->m[i][j] = m.m[j][i];

    return out;
}

/*_________________D3DXMatrixStack____________________*/


static inline struct ID3DXMatrixStackImpl *impl_from_ID3DXMatrixStack(ID3DXMatrixStack *iface)
{
  return CONTAINING_RECORD(iface, struct ID3DXMatrixStackImpl, ID3DXMatrixStack_iface);
}

static HRESULT WINAPI ID3DXMatrixStackImpl_QueryInterface(ID3DXMatrixStack *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_ID3DXMatrixStack)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3DXMatrixStack_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ID3DXMatrixStackImpl_AddRef(ID3DXMatrixStack *iface)
{
    struct ID3DXMatrixStackImpl *stack = impl_from_ID3DXMatrixStack(iface);
    ULONG refcount = InterlockedIncrement(&stack->ref);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);
    return refcount;
}

static ULONG WINAPI ID3DXMatrixStackImpl_Release(ID3DXMatrixStack *iface)
{
    struct ID3DXMatrixStackImpl *stack = impl_from_ID3DXMatrixStack(iface);
    ULONG refcount = InterlockedDecrement(&stack->ref);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);
    if (!refcount)
    {
        free(stack->stack);
        free(stack);
    }
    return refcount;
}

static D3DXMATRIX* WINAPI ID3DXMatrixStackImpl_GetTop(ID3DXMatrixStack *iface)
{
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    return &This->stack[This->current];
}

static HRESULT WINAPI ID3DXMatrixStackImpl_LoadIdentity(ID3DXMatrixStack *iface)
{
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    D3DXMatrixIdentity(&This->stack[This->current]);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_LoadMatrix(ID3DXMatrixStack *iface, const D3DXMATRIX *pm)
{
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p, pm %p\n", iface, pm);

    This->stack[This->current] = *pm;

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_MultMatrix(ID3DXMatrixStack *iface, const D3DXMATRIX *pm)
{
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p, pm %p\n", iface, pm);

    D3DXMatrixMultiply(&This->stack[This->current], &This->stack[This->current], pm);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_MultMatrixLocal(ID3DXMatrixStack *iface, const D3DXMATRIX *pm)
{
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p, pm %p\n", iface, pm);

    D3DXMatrixMultiply(&This->stack[This->current], pm, &This->stack[This->current]);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_Pop(ID3DXMatrixStack *iface)
{
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    /* Popping the last element on the stack returns D3D_OK, but does nothing. */
    if (!This->current) return D3D_OK;

    if (This->current <= This->stack_size / 4 && This->stack_size >= INITIAL_STACK_SIZE * 2)
    {
        unsigned int new_size;
        D3DXMATRIX *new_stack;

        new_size = This->stack_size / 2;
        new_stack = realloc(This->stack, new_size * sizeof(*new_stack));
        if (new_stack)
        {
            This->stack_size = new_size;
            This->stack = new_stack;
        }
    }

    --This->current;

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_Push(ID3DXMatrixStack *iface)
{
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    if (This->current == This->stack_size - 1)
    {
        unsigned int new_size;
        D3DXMATRIX *new_stack;

        if (This->stack_size > UINT_MAX / 2) return E_OUTOFMEMORY;

        new_size = This->stack_size * 2;
        new_stack = realloc(This->stack, new_size * sizeof(*new_stack));
        if (!new_stack) return E_OUTOFMEMORY;

        This->stack_size = new_size;
        This->stack = new_stack;
    }

    ++This->current;
    This->stack[This->current] = This->stack[This->current - 1];

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_RotateAxis(ID3DXMatrixStack *iface, const D3DXVECTOR3 *pv, FLOAT angle)
{
    D3DXMATRIX temp;
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p, pv %p, angle %f\n", iface, pv, angle);

    D3DXMatrixRotationAxis(&temp, pv, angle);
    D3DXMatrixMultiply(&This->stack[This->current], &This->stack[This->current], &temp);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_RotateAxisLocal(ID3DXMatrixStack *iface, const D3DXVECTOR3 *pv, FLOAT angle)
{
    D3DXMATRIX temp;
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p, pv %p, angle %f\n", iface, pv, angle);

    D3DXMatrixRotationAxis(&temp, pv, angle);
    D3DXMatrixMultiply(&This->stack[This->current], &temp, &This->stack[This->current]);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_RotateYawPitchRoll(ID3DXMatrixStack *iface, FLOAT x, FLOAT y, FLOAT z)
{
    D3DXMATRIX temp;
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p, x %f, y %f, z %f\n", iface, x, y, z);

    D3DXMatrixRotationYawPitchRoll(&temp, x, y, z);
    D3DXMatrixMultiply(&This->stack[This->current], &This->stack[This->current], &temp);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_RotateYawPitchRollLocal(ID3DXMatrixStack *iface, FLOAT x, FLOAT y, FLOAT z)
{
    D3DXMATRIX temp;
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p, x %f, y %f, z %f\n", iface, x, y, z);

    D3DXMatrixRotationYawPitchRoll(&temp, x, y, z);
    D3DXMatrixMultiply(&This->stack[This->current], &temp, &This->stack[This->current]);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_Scale(ID3DXMatrixStack *iface, FLOAT x, FLOAT y, FLOAT z)
{
    D3DXMATRIX temp;
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p,x %f, y %f, z %f\n", iface, x, y, z);

    D3DXMatrixScaling(&temp, x, y, z);
    D3DXMatrixMultiply(&This->stack[This->current], &This->stack[This->current], &temp);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_ScaleLocal(ID3DXMatrixStack *iface, FLOAT x, FLOAT y, FLOAT z)
{
    D3DXMATRIX temp;
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p, x %f, y %f, z %f\n", iface, x, y, z);

    D3DXMatrixScaling(&temp, x, y, z);
    D3DXMatrixMultiply(&This->stack[This->current], &temp, &This->stack[This->current]);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_Translate(ID3DXMatrixStack *iface, FLOAT x, FLOAT y, FLOAT z)
{
    D3DXMATRIX temp;
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p, x %f, y %f, z %f\n", iface, x, y, z);

    D3DXMatrixTranslation(&temp, x, y, z);
    D3DXMatrixMultiply(&This->stack[This->current], &This->stack[This->current], &temp);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_TranslateLocal(ID3DXMatrixStack *iface, FLOAT x, FLOAT y, FLOAT z)
{
    D3DXMATRIX temp;
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p, x %f, y %f, z %f\n", iface, x, y, z);

    D3DXMatrixTranslation(&temp, x, y, z);
    D3DXMatrixMultiply(&This->stack[This->current], &temp,&This->stack[This->current]);

    return D3D_OK;
}

static const ID3DXMatrixStackVtbl ID3DXMatrixStack_Vtbl =
{
    ID3DXMatrixStackImpl_QueryInterface,
    ID3DXMatrixStackImpl_AddRef,
    ID3DXMatrixStackImpl_Release,
    ID3DXMatrixStackImpl_Pop,
    ID3DXMatrixStackImpl_Push,
    ID3DXMatrixStackImpl_LoadIdentity,
    ID3DXMatrixStackImpl_LoadMatrix,
    ID3DXMatrixStackImpl_MultMatrix,
    ID3DXMatrixStackImpl_MultMatrixLocal,
    ID3DXMatrixStackImpl_RotateAxis,
    ID3DXMatrixStackImpl_RotateAxisLocal,
    ID3DXMatrixStackImpl_RotateYawPitchRoll,
    ID3DXMatrixStackImpl_RotateYawPitchRollLocal,
    ID3DXMatrixStackImpl_Scale,
    ID3DXMatrixStackImpl_ScaleLocal,
    ID3DXMatrixStackImpl_Translate,
    ID3DXMatrixStackImpl_TranslateLocal,
    ID3DXMatrixStackImpl_GetTop
};

HRESULT WINAPI D3DXCreateMatrixStack(DWORD flags, ID3DXMatrixStack **stack)
{
    struct ID3DXMatrixStackImpl *object;

    TRACE("flags %#lx, stack %p.\n", flags, stack);

    if (!(object = calloc(1, sizeof(*object))))
    {
        *stack = NULL;
        return E_OUTOFMEMORY;
    }
    object->ID3DXMatrixStack_iface.lpVtbl = &ID3DXMatrixStack_Vtbl;
    object->ref = 1;

    if (!(object->stack = malloc(INITIAL_STACK_SIZE * sizeof(*object->stack))))
    {
        free(object);
        *stack = NULL;
        return E_OUTOFMEMORY;
    }

    object->current = 0;
    object->stack_size = INITIAL_STACK_SIZE;
    D3DXMatrixIdentity(&object->stack[0]);

    TRACE("Created matrix stack %p.\n", object);

    *stack = &object->ID3DXMatrixStack_iface;
    return D3D_OK;
}

/*_________________D3DXPLANE________________*/

D3DXPLANE* WINAPI D3DXPlaneFromPointNormal(D3DXPLANE *pout, const D3DXVECTOR3 *pvpoint, const D3DXVECTOR3 *pvnormal)
{
    TRACE("pout %p, pvpoint %p, pvnormal %p\n", pout, pvpoint, pvnormal);

    pout->a = pvnormal->x;
    pout->b = pvnormal->y;
    pout->c = pvnormal->z;
    pout->d = -D3DXVec3Dot(pvpoint, pvnormal);
    return pout;
}

D3DXPLANE* WINAPI D3DXPlaneFromPoints(D3DXPLANE *pout, const D3DXVECTOR3 *pv1, const D3DXVECTOR3 *pv2, const D3DXVECTOR3 *pv3)
{
    D3DXVECTOR3 edge1, edge2, normal, Nnormal;

    TRACE("pout %p, pv1 %p, pv2 %p, pv3 %p\n", pout, pv1, pv2, pv3);

    edge1.x = 0.0f; edge1.y = 0.0f; edge1.z = 0.0f;
    edge2.x = 0.0f; edge2.y = 0.0f; edge2.z = 0.0f;
    D3DXVec3Subtract(&edge1, pv2, pv1);
    D3DXVec3Subtract(&edge2, pv3, pv1);
    D3DXVec3Cross(&normal, &edge1, &edge2);
    D3DXVec3Normalize(&Nnormal, &normal);
    D3DXPlaneFromPointNormal(pout, pv1, &Nnormal);
    return pout;
}

D3DXVECTOR3* WINAPI D3DXPlaneIntersectLine(D3DXVECTOR3 *pout, const D3DXPLANE *pp, const D3DXVECTOR3 *pv1, const D3DXVECTOR3 *pv2)
{
    D3DXVECTOR3 direction, normal;
    FLOAT dot, temp;

    TRACE("pout %p, pp %p, pv1 %p, pv2 %p\n", pout, pp, pv1, pv2);

    normal.x = pp->a;
    normal.y = pp->b;
    normal.z = pp->c;
    direction.x = pv2->x - pv1->x;
    direction.y = pv2->y - pv1->y;
    direction.z = pv2->z - pv1->z;
    dot = D3DXVec3Dot(&normal, &direction);
    if ( !dot ) return NULL;
    temp = ( pp->d + D3DXVec3Dot(&normal, pv1) ) / dot;
    pout->x = pv1->x - temp * direction.x;
    pout->y = pv1->y - temp * direction.y;
    pout->z = pv1->z - temp * direction.z;
    return pout;
}

D3DXPLANE * WINAPI D3DXPlaneNormalize(D3DXPLANE *out, const D3DXPLANE *p)
{
    FLOAT norm;

    TRACE("out %p, p %p\n", out, p);

    norm = sqrtf(p->a * p->a + p->b * p->b + p->c * p->c);
    if (norm)
    {
        out->a = p->a / norm;
        out->b = p->b / norm;
        out->c = p->c / norm;
        out->d = p->d / norm;
    }
    else
    {
        out->a = 0.0f;
        out->b = 0.0f;
        out->c = 0.0f;
        out->d = 0.0f;
    }

    return out;
}

D3DXPLANE* WINAPI D3DXPlaneTransform(D3DXPLANE *pout, const D3DXPLANE *pplane, const D3DXMATRIX *pm)
{
    const D3DXPLANE plane = *pplane;

    TRACE("pout %p, pplane %p, pm %p\n", pout, pplane, pm);

    pout->a = pm->m[0][0] * plane.a + pm->m[1][0] * plane.b + pm->m[2][0] * plane.c + pm->m[3][0] * plane.d;
    pout->b = pm->m[0][1] * plane.a + pm->m[1][1] * plane.b + pm->m[2][1] * plane.c + pm->m[3][1] * plane.d;
    pout->c = pm->m[0][2] * plane.a + pm->m[1][2] * plane.b + pm->m[2][2] * plane.c + pm->m[3][2] * plane.d;
    pout->d = pm->m[0][3] * plane.a + pm->m[1][3] * plane.b + pm->m[2][3] * plane.c + pm->m[3][3] * plane.d;
    return pout;
}

D3DXPLANE* WINAPI D3DXPlaneTransformArray(D3DXPLANE* out, UINT outstride, const D3DXPLANE* in, UINT instride, const D3DXMATRIX* matrix, UINT elements)
{
    UINT i;

    TRACE("out %p, outstride %u, in %p, instride %u, matrix %p, elements %u\n", out, outstride, in, instride, matrix, elements);

    for (i = 0; i < elements; ++i) {
        D3DXPlaneTransform(
            (D3DXPLANE*)((char*)out + outstride * i),
            (const D3DXPLANE*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

/*_________________D3DXQUATERNION________________*/

D3DXQUATERNION* WINAPI D3DXQuaternionBaryCentric(D3DXQUATERNION *pout, const D3DXQUATERNION *pq1, const D3DXQUATERNION *pq2, const D3DXQUATERNION *pq3, FLOAT f, FLOAT g)
{
    D3DXQUATERNION temp1, temp2;

     TRACE("pout %p, pq1 %p, pq2 %p, pq3 %p, f %f, g %f\n", pout, pq1, pq2, pq3, f, g);

    D3DXQuaternionSlerp(pout, D3DXQuaternionSlerp(&temp1, pq1, pq2, f + g), D3DXQuaternionSlerp(&temp2, pq1, pq3, f+g), g / (f + g));
    return pout;
}

D3DXQUATERNION * WINAPI D3DXQuaternionExp(D3DXQUATERNION *out, const D3DXQUATERNION *q)
{
    FLOAT norm;

    TRACE("out %p, q %p\n", out, q);

    norm = sqrtf(q->x * q->x + q->y * q->y + q->z * q->z);
    if (norm)
    {
        out->x = sinf(norm) * q->x / norm;
        out->y = sinf(norm) * q->y / norm;
        out->z = sinf(norm) * q->z / norm;
        out->w = cosf(norm);
    }
    else
    {
        out->x = 0.0f;
        out->y = 0.0f;
        out->z = 0.0f;
        out->w = 1.0f;
    }

    return out;
}

D3DXQUATERNION* WINAPI D3DXQuaternionInverse(D3DXQUATERNION *pout, const D3DXQUATERNION *pq)
{
    FLOAT norm;

    TRACE("pout %p, pq %p\n", pout, pq);

    norm = D3DXQuaternionLengthSq(pq);

    pout->x = -pq->x / norm;
    pout->y = -pq->y / norm;
    pout->z = -pq->z / norm;
    pout->w = pq->w / norm;
    return pout;
}

D3DXQUATERNION * WINAPI D3DXQuaternionLn(D3DXQUATERNION *out, const D3DXQUATERNION *q)
{
    FLOAT t;

    TRACE("out %p, q %p\n", out, q);

    if ((q->w >= 1.0f) || (q->w == -1.0f))
        t = 1.0f;
    else
        t = acosf(q->w) / sqrtf(1.0f - q->w * q->w);

    out->x = t * q->x;
    out->y = t * q->y;
    out->z = t * q->z;
    out->w = 0.0f;

    return out;
}

D3DXQUATERNION* WINAPI D3DXQuaternionMultiply(D3DXQUATERNION *pout, const D3DXQUATERNION *pq1, const D3DXQUATERNION *pq2)
{
    D3DXQUATERNION out;

    TRACE("pout %p, pq1 %p, pq2 %p\n", pout, pq1, pq2);

    out.x = pq2->w * pq1->x + pq2->x * pq1->w + pq2->y * pq1->z - pq2->z * pq1->y;
    out.y = pq2->w * pq1->y - pq2->x * pq1->z + pq2->y * pq1->w + pq2->z * pq1->x;
    out.z = pq2->w * pq1->z + pq2->x * pq1->y - pq2->y * pq1->x + pq2->z * pq1->w;
    out.w = pq2->w * pq1->w - pq2->x * pq1->x - pq2->y * pq1->y - pq2->z * pq1->z;
    *pout = out;
    return pout;
}

D3DXQUATERNION * WINAPI D3DXQuaternionNormalize(D3DXQUATERNION *out, const D3DXQUATERNION *q)
{
    FLOAT norm;

    TRACE("out %p, q %p\n", out, q);

    norm = D3DXQuaternionLength(q);

    out->x = q->x / norm;
    out->y = q->y / norm;
    out->z = q->z / norm;
    out->w = q->w / norm;

    return out;
}

D3DXQUATERNION * WINAPI D3DXQuaternionRotationAxis(D3DXQUATERNION *out, const D3DXVECTOR3 *v, FLOAT angle)
{
    D3DXVECTOR3 temp;

    TRACE("out %p, v %p, angle %f\n", out, v, angle);

    D3DXVec3Normalize(&temp, v);

    out->x = sinf(angle / 2.0f) * temp.x;
    out->y = sinf(angle / 2.0f) * temp.y;
    out->z = sinf(angle / 2.0f) * temp.z;
    out->w = cosf(angle / 2.0f);

    return out;
}

D3DXQUATERNION * WINAPI D3DXQuaternionRotationMatrix(D3DXQUATERNION *out, const D3DXMATRIX *m)
{
    FLOAT s, trace;

    TRACE("out %p, m %p\n", out, m);

    trace = m->m[0][0] + m->m[1][1] + m->m[2][2] + 1.0f;
    if (trace > 1.0f)
    {
        s = 2.0f * sqrtf(trace);
        out->x = (m->m[1][2] - m->m[2][1]) / s;
        out->y = (m->m[2][0] - m->m[0][2]) / s;
        out->z = (m->m[0][1] - m->m[1][0]) / s;
        out->w = 0.25f * s;
    }
    else
    {
        int i, maxi = 0;

        for (i = 1; i < 3; i++)
        {
            if (m->m[i][i] > m->m[maxi][maxi])
                maxi = i;
        }

        switch (maxi)
        {
            case 0:
                s = 2.0f * sqrtf(1.0f + m->m[0][0] - m->m[1][1] - m->m[2][2]);
                out->x = 0.25f * s;
                out->y = (m->m[0][1] + m->m[1][0]) / s;
                out->z = (m->m[0][2] + m->m[2][0]) / s;
                out->w = (m->m[1][2] - m->m[2][1]) / s;
                break;

            case 1:
                s = 2.0f * sqrtf(1.0f + m->m[1][1] - m->m[0][0] - m->m[2][2]);
                out->x = (m->m[0][1] + m->m[1][0]) / s;
                out->y = 0.25f * s;
                out->z = (m->m[1][2] + m->m[2][1]) / s;
                out->w = (m->m[2][0] - m->m[0][2]) / s;
                break;

            case 2:
                s = 2.0f * sqrtf(1.0f + m->m[2][2] - m->m[0][0] - m->m[1][1]);
                out->x = (m->m[0][2] + m->m[2][0]) / s;
                out->y = (m->m[1][2] + m->m[2][1]) / s;
                out->z = 0.25f * s;
                out->w = (m->m[0][1] - m->m[1][0]) / s;
                break;
        }
    }

    return out;
}

D3DXQUATERNION * WINAPI D3DXQuaternionRotationYawPitchRoll(D3DXQUATERNION *out, FLOAT yaw, FLOAT pitch, FLOAT roll)
{
    FLOAT syaw, cyaw, spitch, cpitch, sroll, croll;

    TRACE("out %p, yaw %f, pitch %f, roll %f\n", out, yaw, pitch, roll);

    syaw = sinf(yaw / 2.0f);
    cyaw = cosf(yaw / 2.0f);
    spitch = sinf(pitch / 2.0f);
    cpitch = cosf(pitch / 2.0f);
    sroll = sinf(roll / 2.0f);
    croll = cosf(roll / 2.0f);

    out->x = syaw * cpitch * sroll + cyaw * spitch * croll;
    out->y = syaw * cpitch * croll - cyaw * spitch * sroll;
    out->z = cyaw * cpitch * sroll - syaw * spitch * croll;
    out->w = cyaw * cpitch * croll + syaw * spitch * sroll;

    return out;
}

D3DXQUATERNION * WINAPI D3DXQuaternionSlerp(D3DXQUATERNION *out, const D3DXQUATERNION *q1,
        const D3DXQUATERNION *q2, FLOAT t)
{
    FLOAT dot, temp;

    TRACE("out %p, q1 %p, q2 %p, t %f\n", out, q1, q2, t);

    temp = 1.0f - t;
    dot = D3DXQuaternionDot(q1, q2);
    if (dot < 0.0f)
    {
        t = -t;
        dot = -dot;
    }

    if (1.0f - dot > 0.001f)
    {
        FLOAT theta = acosf(dot);

        temp = sinf(theta * temp) / sinf(theta);
        t = sinf(theta * t) / sinf(theta);
    }

    out->x = temp * q1->x + t * q2->x;
    out->y = temp * q1->y + t * q2->y;
    out->z = temp * q1->z + t * q2->z;
    out->w = temp * q1->w + t * q2->w;

    return out;
}

D3DXQUATERNION* WINAPI D3DXQuaternionSquad(D3DXQUATERNION *pout, const D3DXQUATERNION *pq1, const D3DXQUATERNION *pq2, const D3DXQUATERNION *pq3, const D3DXQUATERNION *pq4, FLOAT t)
{
    D3DXQUATERNION temp1, temp2;

    TRACE("pout %p, pq1 %p, pq2 %p, pq3 %p, pq4 %p, t %f\n", pout, pq1, pq2, pq3, pq4, t);

    D3DXQuaternionSlerp(pout, D3DXQuaternionSlerp(&temp1, pq1, pq4, t), D3DXQuaternionSlerp(&temp2, pq2, pq3, t), 2.0f * t * (1.0f - t));
    return pout;
}

static D3DXQUATERNION add_diff(const D3DXQUATERNION *q1, const D3DXQUATERNION *q2, const FLOAT add)
{
    D3DXQUATERNION temp;

    temp.x = q1->x + add * q2->x;
    temp.y = q1->y + add * q2->y;
    temp.z = q1->z + add * q2->z;
    temp.w = q1->w + add * q2->w;

    return temp;
}

void WINAPI D3DXQuaternionSquadSetup(D3DXQUATERNION *paout, D3DXQUATERNION *pbout, D3DXQUATERNION *pcout, const D3DXQUATERNION *pq0, const D3DXQUATERNION *pq1, const D3DXQUATERNION *pq2, const D3DXQUATERNION *pq3)
{
    D3DXQUATERNION q, temp1, temp2, temp3, zero;
    D3DXQUATERNION aout, cout;

    TRACE("paout %p, pbout %p, pcout %p, pq0 %p, pq1 %p, pq2 %p, pq3 %p\n", paout, pbout, pcout, pq0, pq1, pq2, pq3);

    zero.x = 0.0f;
    zero.y = 0.0f;
    zero.z = 0.0f;
    zero.w = 0.0f;

    if (D3DXQuaternionDot(pq0, pq1) < 0.0f)
        temp2 = add_diff(&zero, pq0, -1.0f);
    else
        temp2 = *pq0;

    if (D3DXQuaternionDot(pq1, pq2) < 0.0f)
        cout = add_diff(&zero, pq2, -1.0f);
    else
        cout = *pq2;

    if (D3DXQuaternionDot(&cout, pq3) < 0.0f)
        temp3 = add_diff(&zero, pq3, -1.0f);
    else
        temp3 = *pq3;

    D3DXQuaternionInverse(&temp1, pq1);
    D3DXQuaternionMultiply(&temp2, &temp1, &temp2);
    D3DXQuaternionLn(&temp2, &temp2);
    D3DXQuaternionMultiply(&q, &temp1, &cout);
    D3DXQuaternionLn(&q, &q);
    temp1 = add_diff(&temp2, &q, 1.0f);
    temp1.x *= -0.25f;
    temp1.y *= -0.25f;
    temp1.z *= -0.25f;
    temp1.w *= -0.25f;
    D3DXQuaternionExp(&temp1, &temp1);
    D3DXQuaternionMultiply(&aout, pq1, &temp1);

    D3DXQuaternionInverse(&temp1, &cout);
    D3DXQuaternionMultiply(&temp2, &temp1, pq1);
    D3DXQuaternionLn(&temp2, &temp2);
    D3DXQuaternionMultiply(&q, &temp1, &temp3);
    D3DXQuaternionLn(&q, &q);
    temp1 = add_diff(&temp2, &q, 1.0f);
    temp1.x *= -0.25f;
    temp1.y *= -0.25f;
    temp1.z *= -0.25f;
    temp1.w *= -0.25f;
    D3DXQuaternionExp(&temp1, &temp1);
    D3DXQuaternionMultiply(pbout, &cout, &temp1);
    *paout = aout;
    *pcout = cout;
}

void WINAPI D3DXQuaternionToAxisAngle(const D3DXQUATERNION *pq, D3DXVECTOR3 *paxis, FLOAT *pangle)
{
    TRACE("pq %p, paxis %p, pangle %p\n", pq, paxis, pangle);

    if (paxis)
    {
        paxis->x = pq->x;
        paxis->y = pq->y;
        paxis->z = pq->z;
    }
    if (pangle)
        *pangle = 2.0f * acosf(pq->w);
}

/*_________________D3DXVec2_____________________*/

D3DXVECTOR2* WINAPI D3DXVec2BaryCentric(D3DXVECTOR2 *pout, const D3DXVECTOR2 *pv1, const D3DXVECTOR2 *pv2, const D3DXVECTOR2 *pv3, FLOAT f, FLOAT g)
{
    TRACE("pout %p, pv1 %p, pv2 %p, pv3 %p, f %f, g %f\n", pout, pv1, pv2, pv3, f, g);

    pout->x = (1.0f-f-g) * (pv1->x) + f * (pv2->x) + g * (pv3->x);
    pout->y = (1.0f-f-g) * (pv1->y) + f * (pv2->y) + g * (pv3->y);
    return pout;
}

D3DXVECTOR2* WINAPI D3DXVec2CatmullRom(D3DXVECTOR2 *pout, const D3DXVECTOR2 *pv0, const D3DXVECTOR2 *pv1, const D3DXVECTOR2 *pv2, const D3DXVECTOR2 *pv3, FLOAT s)
{
    TRACE("pout %p, pv0 %p, pv1 %p, pv2 %p, pv3 %p, s %f\n", pout, pv0, pv1, pv2, pv3, s);

    pout->x = 0.5f * (2.0f * pv1->x + (pv2->x - pv0->x) *s + (2.0f *pv0->x - 5.0f * pv1->x + 4.0f * pv2->x - pv3->x) * s * s + (pv3->x -3.0f * pv2->x + 3.0f * pv1->x - pv0->x) * s * s * s);
    pout->y = 0.5f * (2.0f * pv1->y + (pv2->y - pv0->y) *s + (2.0f *pv0->y - 5.0f * pv1->y + 4.0f * pv2->y - pv3->y) * s * s + (pv3->y -3.0f * pv2->y + 3.0f * pv1->y - pv0->y) * s * s * s);
    return pout;
}

D3DXVECTOR2* WINAPI D3DXVec2Hermite(D3DXVECTOR2 *pout, const D3DXVECTOR2 *pv1, const D3DXVECTOR2 *pt1, const D3DXVECTOR2 *pv2, const D3DXVECTOR2 *pt2, FLOAT s)
{
    FLOAT h1, h2, h3, h4;

    TRACE("pout %p, pv1 %p, pt1 %p, pv2 %p, pt2 %p, s %f\n", pout, pv1, pt1, pv2, pt2, s);

    h1 = 2.0f * s * s * s - 3.0f * s * s + 1.0f;
    h2 = s * s * s - 2.0f * s * s + s;
    h3 = -2.0f * s * s * s + 3.0f * s * s;
    h4 = s * s * s - s * s;

    pout->x = h1 * (pv1->x) + h2 * (pt1->x) + h3 * (pv2->x) + h4 * (pt2->x);
    pout->y = h1 * (pv1->y) + h2 * (pt1->y) + h3 * (pv2->y) + h4 * (pt2->y);
    return pout;
}

D3DXVECTOR2* WINAPI D3DXVec2Normalize(D3DXVECTOR2 *pout, const D3DXVECTOR2 *pv)
{
    FLOAT norm;

    TRACE("pout %p, pv %p\n", pout, pv);

    norm = D3DXVec2Length(pv);
    if ( !norm )
    {
        pout->x = 0.0f;
        pout->y = 0.0f;
    }
    else
    {
        pout->x = pv->x / norm;
        pout->y = pv->y / norm;
    }

    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec2Transform(D3DXVECTOR4 *pout, const D3DXVECTOR2 *pv, const D3DXMATRIX *pm)
{
    D3DXVECTOR4 out;

    TRACE("pout %p, pv %p, pm %p\n", pout, pv, pm);

    out.x = pm->m[0][0] * pv->x + pm->m[1][0] * pv->y  + pm->m[3][0];
    out.y = pm->m[0][1] * pv->x + pm->m[1][1] * pv->y  + pm->m[3][1];
    out.z = pm->m[0][2] * pv->x + pm->m[1][2] * pv->y  + pm->m[3][2];
    out.w = pm->m[0][3] * pv->x + pm->m[1][3] * pv->y  + pm->m[3][3];
    *pout = out;
    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec2TransformArray(D3DXVECTOR4* out, UINT outstride, const D3DXVECTOR2* in, UINT instride, const D3DXMATRIX* matrix, UINT elements)
{
    UINT i;

    TRACE("out %p, outstride %u, in %p, instride %u, matrix %p, elements %u\n", out, outstride, in, instride, matrix, elements);

    for (i = 0; i < elements; ++i) {
        D3DXVec2Transform(
            (D3DXVECTOR4*)((char*)out + outstride * i),
            (const D3DXVECTOR2*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

D3DXVECTOR2* WINAPI D3DXVec2TransformCoord(D3DXVECTOR2 *pout, const D3DXVECTOR2 *pv, const D3DXMATRIX *pm)
{
    D3DXVECTOR2 v;
    FLOAT norm;

    TRACE("pout %p, pv %p, pm %p\n", pout, pv, pm);

    v = *pv;
    norm = pm->m[0][3] * pv->x + pm->m[1][3] * pv->y + pm->m[3][3];

    pout->x = (pm->m[0][0] * v.x + pm->m[1][0] * v.y + pm->m[3][0]) / norm;
    pout->y = (pm->m[0][1] * v.x + pm->m[1][1] * v.y + pm->m[3][1]) / norm;

    return pout;
}

D3DXVECTOR2* WINAPI D3DXVec2TransformCoordArray(D3DXVECTOR2* out, UINT outstride, const D3DXVECTOR2* in, UINT instride, const D3DXMATRIX* matrix, UINT elements)
{
    UINT i;

    TRACE("out %p, outstride %u, in %p, instride %u, matrix %p, elements %u\n", out, outstride, in, instride, matrix, elements);

    for (i = 0; i < elements; ++i) {
        D3DXVec2TransformCoord(
            (D3DXVECTOR2*)((char*)out + outstride * i),
            (const D3DXVECTOR2*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

D3DXVECTOR2* WINAPI D3DXVec2TransformNormal(D3DXVECTOR2 *pout, const D3DXVECTOR2 *pv, const D3DXMATRIX *pm)
{
    const D3DXVECTOR2 v = *pv;

    TRACE("pout %p, pv %p, pm %p\n", pout, pv, pm);

    pout->x = pm->m[0][0] * v.x + pm->m[1][0] * v.y;
    pout->y = pm->m[0][1] * v.x + pm->m[1][1] * v.y;
    return pout;
}

D3DXVECTOR2* WINAPI D3DXVec2TransformNormalArray(D3DXVECTOR2* out, UINT outstride, const D3DXVECTOR2 *in, UINT instride, const D3DXMATRIX *matrix, UINT elements)
{
    UINT i;

    TRACE("out %p, outstride %u, in %p, instride %u, matrix %p, elements %u\n", out, outstride, in, instride, matrix, elements);

    for (i = 0; i < elements; ++i) {
        D3DXVec2TransformNormal(
            (D3DXVECTOR2*)((char*)out + outstride * i),
            (const D3DXVECTOR2*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

/*_________________D3DXVec3_____________________*/

D3DXVECTOR3* WINAPI D3DXVec3BaryCentric(D3DXVECTOR3 *pout, const D3DXVECTOR3 *pv1, const D3DXVECTOR3 *pv2, const D3DXVECTOR3 *pv3, FLOAT f, FLOAT g)
{
    TRACE("pout %p, pv1 %p, pv2 %p, pv3 %p, f %f, g %f\n", pout, pv1, pv2, pv3, f, g);

    pout->x = (1.0f-f-g) * (pv1->x) + f * (pv2->x) + g * (pv3->x);
    pout->y = (1.0f-f-g) * (pv1->y) + f * (pv2->y) + g * (pv3->y);
    pout->z = (1.0f-f-g) * (pv1->z) + f * (pv2->z) + g * (pv3->z);
    return pout;
}

D3DXVECTOR3* WINAPI D3DXVec3CatmullRom( D3DXVECTOR3 *pout, const D3DXVECTOR3 *pv0, const D3DXVECTOR3 *pv1, const D3DXVECTOR3 *pv2, const D3DXVECTOR3 *pv3, FLOAT s)
{
    TRACE("pout %p, pv0 %p, pv1 %p, pv2 %p, pv3 %p, s %f\n", pout, pv0, pv1, pv2, pv3, s);

    pout->x = 0.5f * (2.0f * pv1->x + (pv2->x - pv0->x) *s + (2.0f *pv0->x - 5.0f * pv1->x + 4.0f * pv2->x - pv3->x) * s * s + (pv3->x -3.0f * pv2->x + 3.0f * pv1->x - pv0->x) * s * s * s);
    pout->y = 0.5f * (2.0f * pv1->y + (pv2->y - pv0->y) *s + (2.0f *pv0->y - 5.0f * pv1->y + 4.0f * pv2->y - pv3->y) * s * s + (pv3->y -3.0f * pv2->y + 3.0f * pv1->y - pv0->y) * s * s * s);
    pout->z = 0.5f * (2.0f * pv1->z + (pv2->z - pv0->z) *s + (2.0f *pv0->z - 5.0f * pv1->z + 4.0f * pv2->z - pv3->z) * s * s + (pv3->z -3.0f * pv2->z + 3.0f * pv1->z - pv0->z) * s * s * s);
    return pout;
}

D3DXVECTOR3* WINAPI D3DXVec3Hermite(D3DXVECTOR3 *pout, const D3DXVECTOR3 *pv1, const D3DXVECTOR3 *pt1, const D3DXVECTOR3 *pv2, const D3DXVECTOR3 *pt2, FLOAT s)
{
    FLOAT h1, h2, h3, h4;

    TRACE("pout %p, pv1 %p, pt1 %p, pv2 %p, pt2 %p, s %f\n", pout, pv1, pt1, pv2, pt2, s);

    h1 = 2.0f * s * s * s - 3.0f * s * s + 1.0f;
    h2 = s * s * s - 2.0f * s * s + s;
    h3 = -2.0f * s * s * s + 3.0f * s * s;
    h4 = s * s * s - s * s;

    pout->x = h1 * (pv1->x) + h2 * (pt1->x) + h3 * (pv2->x) + h4 * (pt2->x);
    pout->y = h1 * (pv1->y) + h2 * (pt1->y) + h3 * (pv2->y) + h4 * (pt2->y);
    pout->z = h1 * (pv1->z) + h2 * (pt1->z) + h3 * (pv2->z) + h4 * (pt2->z);
    return pout;
}

D3DXVECTOR3* WINAPI D3DXVec3Normalize(D3DXVECTOR3 *pout, const D3DXVECTOR3 *pv)
{
    FLOAT norm;

    TRACE("pout %p, pv %p\n", pout, pv);

    norm = D3DXVec3Length(pv);
    if ( !norm )
    {
        pout->x = 0.0f;
        pout->y = 0.0f;
        pout->z = 0.0f;
    }
    else
    {
        pout->x = pv->x / norm;
        pout->y = pv->y / norm;
        pout->z = pv->z / norm;
    }

    return pout;
}

D3DXVECTOR3* WINAPI D3DXVec3Project(D3DXVECTOR3 *pout, const D3DXVECTOR3 *pv, const D3DVIEWPORT9 *pviewport, const D3DXMATRIX *pprojection, const D3DXMATRIX *pview, const D3DXMATRIX *pworld)
{
    D3DXMATRIX m;

    TRACE("pout %p, pv %p, pviewport %p, pprojection %p, pview %p, pworld %p\n", pout, pv, pviewport, pprojection, pview, pworld);

    D3DXMatrixIdentity(&m);
    if (pworld) D3DXMatrixMultiply(&m, &m, pworld);
    if (pview) D3DXMatrixMultiply(&m, &m, pview);
    if (pprojection) D3DXMatrixMultiply(&m, &m, pprojection);

    D3DXVec3TransformCoord(pout, pv, &m);

    if (pviewport)
    {
        pout->x = pviewport->X +  ( 1.0f + pout->x ) * pviewport->Width / 2.0f;
        pout->y = pviewport->Y +  ( 1.0f - pout->y ) * pviewport->Height / 2.0f;
        pout->z = pviewport->MinZ + pout->z * ( pviewport->MaxZ - pviewport->MinZ );
    }
    return pout;
}

D3DXVECTOR3* WINAPI D3DXVec3ProjectArray(D3DXVECTOR3* out, UINT outstride, const D3DXVECTOR3* in, UINT instride, const D3DVIEWPORT9* viewport, const D3DXMATRIX* projection, const D3DXMATRIX* view, const D3DXMATRIX* world, UINT elements)
{
    UINT i;

    TRACE("out %p, outstride %u, in %p, instride %u, viewport %p, projection %p, view %p, world %p, elements %u\n",
        out, outstride, in, instride, viewport, projection, view, world, elements);

    for (i = 0; i < elements; ++i) {
        D3DXVec3Project(
            (D3DXVECTOR3*)((char*)out + outstride * i),
            (const D3DXVECTOR3*)((const char*)in + instride * i),
            viewport, projection, view, world);
    }
    return out;
}

D3DXVECTOR4* WINAPI D3DXVec3Transform(D3DXVECTOR4 *pout, const D3DXVECTOR3 *pv, const D3DXMATRIX *pm)
{
    D3DXVECTOR4 out;

    TRACE("pout %p, pv %p, pm %p\n", pout, pv, pm);

    out.x = pm->m[0][0] * pv->x + pm->m[1][0] * pv->y + pm->m[2][0] * pv->z + pm->m[3][0];
    out.y = pm->m[0][1] * pv->x + pm->m[1][1] * pv->y + pm->m[2][1] * pv->z + pm->m[3][1];
    out.z = pm->m[0][2] * pv->x + pm->m[1][2] * pv->y + pm->m[2][2] * pv->z + pm->m[3][2];
    out.w = pm->m[0][3] * pv->x + pm->m[1][3] * pv->y + pm->m[2][3] * pv->z + pm->m[3][3];
    *pout = out;
    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec3TransformArray(D3DXVECTOR4* out, UINT outstride, const D3DXVECTOR3* in, UINT instride, const D3DXMATRIX* matrix, UINT elements)
{
    UINT i;

    TRACE("out %p, outstride %u, in %p, instride %u, matrix %p, elements %u\n", out, outstride, in, instride, matrix, elements);

    for (i = 0; i < elements; ++i) {
        D3DXVec3Transform(
            (D3DXVECTOR4*)((char*)out + outstride * i),
            (const D3DXVECTOR3*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

D3DXVECTOR3* WINAPI D3DXVec3TransformCoord(D3DXVECTOR3 *pout, const D3DXVECTOR3 *pv, const D3DXMATRIX *pm)
{
    D3DXVECTOR3 out;
    FLOAT norm;

    TRACE("pout %p, pv %p, pm %p\n", pout, pv, pm);

    norm = pm->m[0][3] * pv->x + pm->m[1][3] * pv->y + pm->m[2][3] *pv->z + pm->m[3][3];

    out.x = (pm->m[0][0] * pv->x + pm->m[1][0] * pv->y + pm->m[2][0] * pv->z + pm->m[3][0]) / norm;
    out.y = (pm->m[0][1] * pv->x + pm->m[1][1] * pv->y + pm->m[2][1] * pv->z + pm->m[3][1]) / norm;
    out.z = (pm->m[0][2] * pv->x + pm->m[1][2] * pv->y + pm->m[2][2] * pv->z + pm->m[3][2]) / norm;

    *pout = out;

    return pout;
}

D3DXVECTOR3* WINAPI D3DXVec3TransformCoordArray(D3DXVECTOR3* out, UINT outstride, const D3DXVECTOR3* in, UINT instride, const D3DXMATRIX* matrix, UINT elements)
{
    UINT i;

    TRACE("out %p, outstride %u, in %p, instride %u, matrix %p, elements %u\n", out, outstride, in, instride, matrix, elements);

    for (i = 0; i < elements; ++i) {
        D3DXVec3TransformCoord(
            (D3DXVECTOR3*)((char*)out + outstride * i),
            (const D3DXVECTOR3*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

D3DXVECTOR3* WINAPI D3DXVec3TransformNormal(D3DXVECTOR3 *pout, const D3DXVECTOR3 *pv, const D3DXMATRIX *pm)
{
    const D3DXVECTOR3 v = *pv;

    TRACE("pout %p, pv %p, pm %p\n", pout, pv, pm);

    pout->x = pm->m[0][0] * v.x + pm->m[1][0] * v.y + pm->m[2][0] * v.z;
    pout->y = pm->m[0][1] * v.x + pm->m[1][1] * v.y + pm->m[2][1] * v.z;
    pout->z = pm->m[0][2] * v.x + pm->m[1][2] * v.y + pm->m[2][2] * v.z;
    return pout;

}

D3DXVECTOR3* WINAPI D3DXVec3TransformNormalArray(D3DXVECTOR3* out, UINT outstride, const D3DXVECTOR3* in, UINT instride, const D3DXMATRIX* matrix, UINT elements)
{
    UINT i;

    TRACE("out %p, outstride %u, in %p, instride %u, matrix %p, elements %u\n", out, outstride, in, instride, matrix, elements);

    for (i = 0; i < elements; ++i) {
        D3DXVec3TransformNormal(
            (D3DXVECTOR3*)((char*)out + outstride * i),
            (const D3DXVECTOR3*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

D3DXVECTOR3 * WINAPI D3DXVec3Unproject(D3DXVECTOR3 *out, const D3DXVECTOR3 *v,
        const D3DVIEWPORT9 *viewport, const D3DXMATRIX *projection, const D3DXMATRIX *view,
        const D3DXMATRIX *world)
{
    D3DXMATRIX m;

    TRACE("out %p, v %p, viewport %p, projection %p, view %p, world %p.\n",
            out, v, viewport, projection, view, world);

    D3DXMatrixIdentity(&m);
    if (world)
        D3DXMatrixMultiply(&m, &m, world);
    if (view)
        D3DXMatrixMultiply(&m, &m, view);
    if (projection)
        D3DXMatrixMultiply(&m, &m, projection);
    D3DXMatrixInverse(&m, NULL, &m);

    *out = *v;
    if (viewport)
    {
        out->x = 2.0f * (out->x - viewport->X) / viewport->Width - 1.0f;
        out->y = 1.0f - 2.0f * (out->y - viewport->Y) / viewport->Height;
        out->z = (out->z - viewport->MinZ) / (viewport->MaxZ - viewport->MinZ);
    }
    D3DXVec3TransformCoord(out, out, &m);
    return out;
}

D3DXVECTOR3* WINAPI D3DXVec3UnprojectArray(D3DXVECTOR3* out, UINT outstride, const D3DXVECTOR3* in, UINT instride, const D3DVIEWPORT9* viewport, const D3DXMATRIX* projection, const D3DXMATRIX* view, const D3DXMATRIX* world, UINT elements)
{
    UINT i;

    TRACE("out %p, outstride %u, in %p, instride %u, viewport %p, projection %p, view %p, world %p, elements %u\n",
        out, outstride, in, instride, viewport, projection, view, world, elements);

    for (i = 0; i < elements; ++i) {
        D3DXVec3Unproject(
            (D3DXVECTOR3*)((char*)out + outstride * i),
            (const D3DXVECTOR3*)((const char*)in + instride * i),
            viewport, projection, view, world);
    }
    return out;
}

/*_________________D3DXVec4_____________________*/

D3DXVECTOR4* WINAPI D3DXVec4BaryCentric(D3DXVECTOR4 *pout, const D3DXVECTOR4 *pv1, const D3DXVECTOR4 *pv2, const D3DXVECTOR4 *pv3, FLOAT f, FLOAT g)
{
    TRACE("pout %p, pv1 %p, pv2 %p, pv3 %p, f %f, g %f\n", pout, pv1, pv2, pv3, f, g);

    pout->x = (1.0f-f-g) * (pv1->x) + f * (pv2->x) + g * (pv3->x);
    pout->y = (1.0f-f-g) * (pv1->y) + f * (pv2->y) + g * (pv3->y);
    pout->z = (1.0f-f-g) * (pv1->z) + f * (pv2->z) + g * (pv3->z);
    pout->w = (1.0f-f-g) * (pv1->w) + f * (pv2->w) + g * (pv3->w);
    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec4CatmullRom(D3DXVECTOR4 *pout, const D3DXVECTOR4 *pv0, const D3DXVECTOR4 *pv1, const D3DXVECTOR4 *pv2, const D3DXVECTOR4 *pv3, FLOAT s)
{
    TRACE("pout %p, pv0 %p, pv1 %p, pv2 %p, pv3 %p, s %f\n", pout, pv0, pv1, pv2, pv3, s);

    pout->x = 0.5f * (2.0f * pv1->x + (pv2->x - pv0->x) *s + (2.0f *pv0->x - 5.0f * pv1->x + 4.0f * pv2->x - pv3->x) * s * s + (pv3->x -3.0f * pv2->x + 3.0f * pv1->x - pv0->x) * s * s * s);
    pout->y = 0.5f * (2.0f * pv1->y + (pv2->y - pv0->y) *s + (2.0f *pv0->y - 5.0f * pv1->y + 4.0f * pv2->y - pv3->y) * s * s + (pv3->y -3.0f * pv2->y + 3.0f * pv1->y - pv0->y) * s * s * s);
    pout->z = 0.5f * (2.0f * pv1->z + (pv2->z - pv0->z) *s + (2.0f *pv0->z - 5.0f * pv1->z + 4.0f * pv2->z - pv3->z) * s * s + (pv3->z -3.0f * pv2->z + 3.0f * pv1->z - pv0->z) * s * s * s);
    pout->w = 0.5f * (2.0f * pv1->w + (pv2->w - pv0->w) *s + (2.0f *pv0->w - 5.0f * pv1->w + 4.0f * pv2->w - pv3->w) * s * s + (pv3->w -3.0f * pv2->w + 3.0f * pv1->w - pv0->w) * s * s * s);
    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec4Cross(D3DXVECTOR4 *pout, const D3DXVECTOR4 *pv1, const D3DXVECTOR4 *pv2, const D3DXVECTOR4 *pv3)
{
    D3DXVECTOR4 out;

    TRACE("pout %p, pv1 %p, pv2 %p, pv3 %p\n", pout, pv1, pv2, pv3);

    out.x = pv1->y * (pv2->z * pv3->w - pv3->z * pv2->w) - pv1->z * (pv2->y * pv3->w - pv3->y * pv2->w) + pv1->w * (pv2->y * pv3->z - pv2->z *pv3->y);
    out.y = -(pv1->x * (pv2->z * pv3->w - pv3->z * pv2->w) - pv1->z * (pv2->x * pv3->w - pv3->x * pv2->w) + pv1->w * (pv2->x * pv3->z - pv3->x * pv2->z));
    out.z = pv1->x * (pv2->y * pv3->w - pv3->y * pv2->w) - pv1->y * (pv2->x *pv3->w - pv3->x * pv2->w) + pv1->w * (pv2->x * pv3->y - pv3->x * pv2->y);
    out.w = -(pv1->x * (pv2->y * pv3->z - pv3->y * pv2->z) - pv1->y * (pv2->x * pv3->z - pv3->x *pv2->z) + pv1->z * (pv2->x * pv3->y - pv3->x * pv2->y));
    *pout = out;
    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec4Hermite(D3DXVECTOR4 *pout, const D3DXVECTOR4 *pv1, const D3DXVECTOR4 *pt1, const D3DXVECTOR4 *pv2, const D3DXVECTOR4 *pt2, FLOAT s)
{
    FLOAT h1, h2, h3, h4;

    TRACE("pout %p, pv1 %p, pt1 %p, pv2 %p, pt2 %p, s %f\n", pout, pv1, pt1, pv2, pt2, s);

    h1 = 2.0f * s * s * s - 3.0f * s * s + 1.0f;
    h2 = s * s * s - 2.0f * s * s + s;
    h3 = -2.0f * s * s * s + 3.0f * s * s;
    h4 = s * s * s - s * s;

    pout->x = h1 * (pv1->x) + h2 * (pt1->x) + h3 * (pv2->x) + h4 * (pt2->x);
    pout->y = h1 * (pv1->y) + h2 * (pt1->y) + h3 * (pv2->y) + h4 * (pt2->y);
    pout->z = h1 * (pv1->z) + h2 * (pt1->z) + h3 * (pv2->z) + h4 * (pt2->z);
    pout->w = h1 * (pv1->w) + h2 * (pt1->w) + h3 * (pv2->w) + h4 * (pt2->w);
    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec4Normalize(D3DXVECTOR4 *pout, const D3DXVECTOR4 *pv)
{
    FLOAT norm;

    TRACE("pout %p, pv %p\n", pout, pv);

    norm = D3DXVec4Length(pv);

    pout->x = pv->x / norm;
    pout->y = pv->y / norm;
    pout->z = pv->z / norm;
    pout->w = pv->w / norm;

    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec4Transform(D3DXVECTOR4 *pout, const D3DXVECTOR4 *pv, const D3DXMATRIX *pm)
{
    D3DXVECTOR4 out;

    TRACE("pout %p, pv %p, pm %p\n", pout, pv, pm);

    out.x = pm->m[0][0] * pv->x + pm->m[1][0] * pv->y + pm->m[2][0] * pv->z + pm->m[3][0] * pv->w;
    out.y = pm->m[0][1] * pv->x + pm->m[1][1] * pv->y + pm->m[2][1] * pv->z + pm->m[3][1] * pv->w;
    out.z = pm->m[0][2] * pv->x + pm->m[1][2] * pv->y + pm->m[2][2] * pv->z + pm->m[3][2] * pv->w;
    out.w = pm->m[0][3] * pv->x + pm->m[1][3] * pv->y + pm->m[2][3] * pv->z + pm->m[3][3] * pv->w;
    *pout = out;
    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec4TransformArray(D3DXVECTOR4* out, UINT outstride, const D3DXVECTOR4* in, UINT instride, const D3DXMATRIX* matrix, UINT elements)
{
    UINT i;

    TRACE("out %p, outstride %u, in %p, instride %u, matrix %p, elements %u\n", out, outstride, in, instride, matrix, elements);

    for (i = 0; i < elements; ++i) {
        D3DXVec4Transform(
            (D3DXVECTOR4*)((char*)out + outstride * i),
            (const D3DXVECTOR4*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

unsigned short float_32_to_16(const float in)
{
    int exp = 0, origexp;
    float tmp = fabsf(in);
    int sign = (copysignf(1, in) < 0);
    unsigned int mantissa;
    unsigned short ret;

    /* Deal with special numbers */
    if (isinf(in)) return (sign ? 0xffff : 0x7fff);
    if (isnan(in)) return (sign ? 0xffff : 0x7fff);
    if (in == 0.0f) return (sign ? 0x8000 : 0x0000);

    if (tmp < (float)(1u << 10))
    {
        do
        {
            tmp *= 2.0f;
            exp--;
        } while (tmp < (float)(1u << 10));
    }
    else if (tmp >= (float)(1u << 11))
    {
        do
        {
            tmp /= 2.0f;
            exp++;
        } while (tmp >= (float)(1u << 11));
    }

    exp += 10;  /* Normalize the mantissa */
    exp += 15;  /* Exponent is encoded with excess 15 */

    origexp = exp;

    mantissa = (unsigned int) tmp;
    if ((tmp - mantissa == 0.5f && mantissa % 2 == 1) || /* round half to even */
        (tmp - mantissa > 0.5f))
    {
        mantissa++; /* round to nearest, away from zero */
    }
    if (mantissa == 2048)
    {
        mantissa = 1024;
        exp++;
    }

    if (exp > 31)
    {
        /* too big */
        ret = 0x7fff; /* INF */
    }
    else if (exp <= 0)
    {
        unsigned int rounding = 0;

        /* Denormalized half float */

        /* return 0x0000 (=0.0) for numbers too small to represent in half floats */
        if (exp < -11)
            return (sign ? 0x8000 : 0x0000);

        exp = origexp;

        /* the 13 extra bits from single precision are used for rounding */
        mantissa = (unsigned int)(tmp * (1u << 13));
        mantissa >>= 1 - exp; /* denormalize */

        mantissa -= ~(mantissa >> 13) & 1; /* round half to even */
        /* remove 13 least significant bits to get half float precision */
        mantissa >>= 12;
        rounding = mantissa & 1;
        mantissa >>= 1;

        ret = mantissa + rounding;
    }
    else
    {
        ret = (exp << 10) | (mantissa & 0x3ff);
    }

    ret |= ((sign ? 1 : 0) << 15); /* Add the sign */
    return ret;
}

D3DXFLOAT16 *WINAPI D3DXFloat32To16Array(D3DXFLOAT16 *pout, const FLOAT *pin, UINT n)
{
    unsigned int i;

    TRACE("pout %p, pin %p, n %u\n", pout, pin, n);

    for (i = 0; i < n; ++i)
    {
        pout[i].value = float_32_to_16(pin[i]);
    }

    return pout;
}

/* Native d3dx9's D3DXFloat16to32Array lacks support for NaN and Inf. Specifically, e = 16 is treated as a
 * regular number - e.g., 0x7fff is converted to 131008.0 and 0xffff to -131008.0. */
float float_16_to_32(const unsigned short in)
{
    const unsigned short s = (in & 0x8000);
    const unsigned short e = (in & 0x7C00) >> 10;
    const unsigned short m = in & 0x3FF;
    const float sgn = (s ? -1.0f : 1.0f);

    if (e == 0)
    {
        if (m == 0) return sgn * 0.0f; /* +0.0 or -0.0 */
        else return sgn * powf(2, -14.0f) * (m / 1024.0f);
    }
    else
    {
        return sgn * powf(2, e - 15.0f) * (1.0f + (m / 1024.0f));
    }
}

FLOAT *WINAPI D3DXFloat16To32Array(FLOAT *pout, const D3DXFLOAT16 *pin, UINT n)
{
    unsigned int i;

    TRACE("pout %p, pin %p, n %u\n", pout, pin, n);

    for (i = 0; i < n; ++i)
    {
        pout[i] = float_16_to_32(pin[i].value);
    }

    return pout;
}

/*_________________D3DXSH________________*/

FLOAT* WINAPI D3DXSHAdd(FLOAT *out, UINT order, const FLOAT *a, const FLOAT *b)
{
    UINT i;

    TRACE("out %p, order %u, a %p, b %p\n", out, order, a, b);

    for (i = 0; i < order * order; i++)
        out[i] = a[i] + b[i];

    return out;
}

FLOAT WINAPI D3DXSHDot(UINT order, const FLOAT *a, const FLOAT *b)
{
    FLOAT s;
    UINT i;

    TRACE("order %u, a %p, b %p\n", order, a, b);

    s = a[0] * b[0];
    for (i = 1; i < order * order; i++)
        s += a[i] * b[i];

    return s;
}

static void weightedcapintegrale(FLOAT *out, UINT order, FLOAT angle)
{
    FLOAT coeff[3];

    coeff[0] = cosf(angle);

    out[0] = 2.0f * D3DX_PI * (1.0f - coeff[0]);
    out[1] = D3DX_PI * sinf(angle) * sinf(angle);
    if (order <= 2)
        return;

    out[2] = coeff[0] * out[1];
    if (order == 3)
        return;

    coeff[1] = coeff[0] * coeff[0];
    coeff[2] = coeff[1] * coeff[1];

    out[3] = D3DX_PI * (-1.25f * coeff[2] + 1.5f * coeff[1] - 0.25f);
    if (order == 4)
        return;

    out[4] = -0.25f * D3DX_PI * coeff[0] * (7.0f * coeff[2] - 10.0f * coeff[1] + 3.0f);
    if (order == 5)
        return;

    out[5] = D3DX_PI * (-2.625f * coeff[2] * coeff[1] + 4.375f * coeff[2] - 1.875f * coeff[1] + 0.125f);
}

HRESULT WINAPI D3DXSHEvalConeLight(UINT order, const D3DXVECTOR3 *dir, FLOAT radius,
    FLOAT Rintensity, FLOAT Gintensity, FLOAT Bintensity, FLOAT *rout, FLOAT *gout, FLOAT *bout)
{
    FLOAT cap[6], clamped_angle, norm, scale, temp;
    UINT i, index, j;

    TRACE("order %u, dir %p, radius %f, red %f, green %f, blue %f, rout %p, gout %p, bout %p\n",
        order, dir, radius, Rintensity, Gintensity, Bintensity, rout, gout, bout);

    if (radius <= 0.0f)
        return D3DXSHEvalDirectionalLight(order, dir, Rintensity, Gintensity, Bintensity, rout, gout, bout);

    clamped_angle = (radius > D3DX_PI / 2.0f) ? (D3DX_PI / 2.0f) : radius;
    norm = sinf(clamped_angle) * sinf(clamped_angle);

    if (order > D3DXSH_MAXORDER)
    {
        WARN("Order clamped at D3DXSH_MAXORDER\n");
        order = D3DXSH_MAXORDER;
    }

    weightedcapintegrale(cap, order, radius);
    D3DXSHEvalDirection(rout, order, dir);

    for (i = 0; i < order; i++)
    {
        scale = cap[i] / norm;

        for (j = 0; j < 2 * i + 1; j++)
        {
            index = i * i + j;
            temp = rout[index] * scale;

            rout[index] = temp * Rintensity;
            if (gout)
                gout[index] = temp * Gintensity;
            if (bout)
                bout[index] = temp * Bintensity;
        }
    }

    return D3D_OK;
}

FLOAT* WINAPI D3DXSHEvalDirection(FLOAT *out, UINT order, const D3DXVECTOR3 *dir)
{
    const FLOAT dirxx = dir->x * dir->x;
    const FLOAT dirxy = dir->x * dir->y;
    const FLOAT dirxz = dir->x * dir->z;
    const FLOAT diryy = dir->y * dir->y;
    const FLOAT diryz = dir->y * dir->z;
    const FLOAT dirzz = dir->z * dir->z;
    const FLOAT dirxxxx = dirxx * dirxx;
    const FLOAT diryyyy = diryy * diryy;
    const FLOAT dirzzzz = dirzz * dirzz;
    const FLOAT dirxyxy = dirxy * dirxy;

    TRACE("out %p, order %u, dir %p\n", out, order, dir);

    if ((order < D3DXSH_MINORDER) || (order > D3DXSH_MAXORDER))
        return out;

    out[0] = 0.5f / sqrtf(D3DX_PI);
    out[1] = -0.5f / sqrtf(D3DX_PI / 3.0f) * dir->y;
    out[2] = 0.5f / sqrtf(D3DX_PI / 3.0f) * dir->z;
    out[3] = -0.5f / sqrtf(D3DX_PI / 3.0f) * dir->x;
    if (order == 2)
        return out;

    out[4] = 0.5f / sqrtf(D3DX_PI / 15.0f) * dirxy;
    out[5] = -0.5f / sqrtf(D3DX_PI / 15.0f) * diryz;
    out[6] = 0.25f / sqrtf(D3DX_PI / 5.0f) * (3.0f * dirzz - 1.0f);
    out[7] = -0.5f / sqrtf(D3DX_PI / 15.0f) * dirxz;
    out[8] = 0.25f / sqrtf(D3DX_PI / 15.0f) * (dirxx - diryy);
    if (order == 3)
        return out;

    out[9] = -sqrtf(70.0f / D3DX_PI) / 8.0f * dir->y * (3.0f * dirxx - diryy);
    out[10] = sqrtf(105.0f / D3DX_PI) / 2.0f * dirxy * dir->z;
    out[11] = -sqrtf(42.0f / D3DX_PI) / 8.0f * dir->y * (-1.0f + 5.0f * dirzz);
    out[12] = sqrtf(7.0f / D3DX_PI) / 4.0f * dir->z * (5.0f * dirzz - 3.0f);
    out[13] = sqrtf(42.0f / D3DX_PI) / 8.0f * dir->x * (1.0f - 5.0f * dirzz);
    out[14] = sqrtf(105.0f / D3DX_PI) / 4.0f * dir->z * (dirxx - diryy);
    out[15] = -sqrtf(70.0f / D3DX_PI) / 8.0f * dir->x * (dirxx - 3.0f * diryy);
    if (order == 4)
        return out;

    out[16] = 0.75f * sqrtf(35.0f / D3DX_PI) * dirxy * (dirxx - diryy);
    out[17] = 3.0f * dir->z * out[9];
    out[18] = 0.75f * sqrtf(5.0f / D3DX_PI) * dirxy * (7.0f * dirzz - 1.0f);
    out[19] = 0.375f * sqrtf(10.0f / D3DX_PI) * diryz * (3.0f - 7.0f * dirzz);
    out[20] = 3.0f / (16.0f * sqrtf(D3DX_PI)) * (35.0f * dirzzzz - 30.f * dirzz + 3.0f);
    out[21] = 0.375f * sqrtf(10.0f / D3DX_PI) * dirxz * (3.0f - 7.0f * dirzz);
    out[22] = 0.375f * sqrtf(5.0f / D3DX_PI) * (dirxx - diryy) * (7.0f * dirzz - 1.0f);
    out[23] = 3.0f * dir->z * out[15];
    out[24] = 3.0f / 16.0f * sqrtf(35.0f / D3DX_PI) * (dirxxxx - 6.0f * dirxyxy + diryyyy);
    if (order == 5)
        return out;

    out[25] = -3.0f/ 32.0f * sqrtf(154.0f / D3DX_PI) * dir->y * (5.0f * dirxxxx - 10.0f * dirxyxy + diryyyy);
    out[26] = 0.75f * sqrtf(385.0f / D3DX_PI) * dirxy * dir->z * (dirxx - diryy);
    out[27] = sqrtf(770.0f / D3DX_PI) / 32.0f * dir->y * (3.0f * dirxx - diryy) * (1.0f - 9.0f * dirzz);
    out[28] = sqrtf(1155.0f / D3DX_PI) / 4.0f * dirxy * dir->z * (3.0f * dirzz - 1.0f);
    out[29] = sqrtf(165.0f / D3DX_PI) / 16.0f * dir->y * (14.0f * dirzz - 21.0f * dirzzzz - 1.0f);
    out[30] = sqrtf(11.0f / D3DX_PI) / 16.0f * dir->z * (63.0f * dirzzzz - 70.0f * dirzz + 15.0f);
    out[31] = sqrtf(165.0f / D3DX_PI) / 16.0f * dir->x * (14.0f * dirzz - 21.0f * dirzzzz - 1.0f);
    out[32] = sqrtf(1155.0f / D3DX_PI) / 8.0f * dir->z * (dirxx - diryy) * (3.0f * dirzz - 1.0f);
    out[33] = sqrtf(770.0f / D3DX_PI) / 32.0f * dir->x * (dirxx - 3.0f * diryy) * (1.0f - 9.0f * dirzz);
    out[34] = 3.0f / 16.0f * sqrtf(385.0f / D3DX_PI) * dir->z * (dirxxxx - 6.0f * dirxyxy + diryyyy);
    out[35] = -3.0f/ 32.0f * sqrtf(154.0f / D3DX_PI) * dir->x * (dirxxxx - 10.0f * dirxyxy + 5.0f * diryyyy);

    return out;
}

HRESULT WINAPI D3DXSHEvalDirectionalLight(UINT order, const D3DXVECTOR3 *dir, FLOAT Rintensity, FLOAT Gintensity, FLOAT Bintensity, FLOAT *Rout, FLOAT *Gout, FLOAT *Bout)
{
    FLOAT s, temp;
    UINT j;

    TRACE("Order %u, Vector %p, Red %f, Green %f, Blue %f, Rout %p, Gout %p, Bout %p\n", order, dir, Rintensity, Gintensity, Bintensity, Rout, Gout, Bout);

    s = 0.75f;
    if ( order > 2 )
        s += 5.0f / 16.0f;
    if ( order > 4 )
        s -= 3.0f / 32.0f;
    s /= D3DX_PI;

    D3DXSHEvalDirection(Rout, order, dir);
    for (j = 0; j < order * order; j++)
    {
        temp = Rout[j] / s;

        Rout[j] = Rintensity * temp;
        if ( Gout )
            Gout[j] = Gintensity * temp;
        if ( Bout )
            Bout[j] = Bintensity * temp;
    }

    return D3D_OK;
}

HRESULT WINAPI D3DXSHEvalHemisphereLight(UINT order, const D3DXVECTOR3 *dir, D3DXCOLOR top, D3DXCOLOR bottom,
    FLOAT *rout, FLOAT *gout, FLOAT *bout)
{
    FLOAT a[2], temp[4];
    UINT i, j;

    TRACE("order %u, dir %p, rout %p, gout %p, bout %p\n", order, dir, rout, gout, bout);

    D3DXSHEvalDirection(temp, 2, dir);

    a[0] = (top.r + bottom.r) * 3.0f * D3DX_PI;
    a[1] = (top.r - bottom.r) * D3DX_PI;
    for (i = 0; i < order; i++)
        for (j = 0; j < 2 * i + 1; j++)
            if (i < 2)
                rout[i * i + j] = temp[i * i + j] * a[i];
            else
                rout[i * i + j] = 0.0f;

    if (gout)
    {
        a[0] = (top.g + bottom.g) * 3.0f * D3DX_PI;
        a[1] = (top.g - bottom.g) * D3DX_PI;
        for (i = 0; i < order; i++)
            for (j = 0; j < 2 * i + 1; j++)
                if (i < 2)
                    gout[i * i + j] = temp[i * i + j] * a[i];
                else
                    gout[i * i + j] = 0.0f;
    }

    if (bout)
    {
        a[0] = (top.b + bottom.b) * 3.0f * D3DX_PI;
        a[1] = (top.b - bottom.b) * D3DX_PI;
        for (i = 0; i < order; i++)
            for (j = 0; j < 2 * i + 1; j++)
                if (i < 2)
                    bout[i * i + j] = temp[i * i + j] * a[i];
                else
                    bout[i * i + j] = 0.0f;
    }

    return D3D_OK;
}

HRESULT WINAPI D3DXSHEvalSphericalLight(UINT order, const D3DXVECTOR3 *dir, FLOAT radius,
    FLOAT Rintensity, FLOAT Gintensity, FLOAT Bintensity, FLOAT *rout, FLOAT *gout, FLOAT *bout)
{
    D3DXVECTOR3 normal;
    FLOAT cap[6], clamped_angle, dist, temp;
    UINT i, index, j;

    TRACE("order %u, dir %p, radius %f, red %f, green %f, blue %f, rout %p, gout %p, bout %p\n",
        order, dir, radius, Rintensity, Gintensity, Bintensity, rout, gout, bout);

    if (order > D3DXSH_MAXORDER)
    {
        WARN("Order clamped at D3DXSH_MAXORDER\n");
        order = D3DXSH_MAXORDER;
    }

    if (radius < 0.0f)
        radius = -radius;

    dist = D3DXVec3Length(dir);
    clamped_angle = (dist <= radius) ? D3DX_PI / 2.0f : asinf(radius / dist);

    weightedcapintegrale(cap, order, clamped_angle);
    D3DXVec3Normalize(&normal, dir);
    D3DXSHEvalDirection(rout, order, &normal);

    for (i = 0; i < order; i++)
        for (j = 0; j < 2 * i + 1; j++)
        {
            index = i * i + j;
            temp = rout[index] * cap[i];

            rout[index] = temp * Rintensity;
            if (gout)
                gout[index] = temp * Gintensity;
            if (bout)
                bout[index] = temp * Bintensity;
        }

    return D3D_OK;
}

FLOAT * WINAPI D3DXSHMultiply2(FLOAT *out, const FLOAT *a, const FLOAT *b)
{
    FLOAT ta, tb;

    TRACE("out %p, a %p, b %p\n", out, a, b);

    ta = 0.28209479f * a[0];
    tb = 0.28209479f * b[0];

    out[0] = 0.28209479f * D3DXSHDot(2, a, b);
    out[1] = ta * b[1] + tb * a[1];
    out[2] = ta * b[2] + tb * a[2];
    out[3] = ta * b[3] + tb * a[3];

    return out;
}

FLOAT * WINAPI D3DXSHMultiply3(FLOAT *out, const FLOAT *a, const FLOAT *b)
{
    FLOAT t, ta, tb;

    TRACE("out %p, a %p, b %p\n", out, a, b);

    out[0] = 0.28209479f * a[0] * b[0];

    ta = 0.28209479f * a[0] - 0.12615663f * a[6] - 0.21850969f * a[8];
    tb = 0.28209479f * b[0] - 0.12615663f * b[6] - 0.21850969f * b[8];
    out[1] = ta * b[1] + tb * a[1];
    t = a[1] * b[1];
    out[0] += 0.28209479f * t;
    out[6] = -0.12615663f * t;
    out[8] = -0.21850969f * t;

    ta = 0.21850969f * a[5];
    tb = 0.21850969f * b[5];
    out[1] += ta * b[2] + tb * a[2];
    out[2] = ta * b[1] + tb * a[1];
    t = a[1] * b[2] +a[2] * b[1];
    out[5] = 0.21850969f * t;

    ta = 0.21850969f * a[4];
    tb = 0.21850969f * b[4];
    out[1] += ta * b[3] + tb * a[3];
    out[3]  = ta * b[1] + tb * a[1];
    t = a[1] * b[3] + a[3] * b[1];
    out[4] = 0.21850969f * t;

    ta = 0.28209480f * a[0] + 0.25231326f * a[6];
    tb = 0.28209480f * b[0] + 0.25231326f * b[6];
    out[2] += ta * b[2] + tb * a[2];
    t = a[2] * b[2];
    out[0] += 0.28209480f * t;
    out[6] += 0.25231326f * t;

    ta = 0.21850969f * a[7];
    tb = 0.21850969f * b[7];
    out[2] += ta * b[3] + tb * a[3];
    out[3] += ta * b[2] + tb * a[2];
    t = a[2] * b[3] + a[3] * b[2];
    out[7] = 0.21850969f * t;

    ta = 0.28209479f * a[0] - 0.12615663f * a[6] + 0.21850969f * a[8];
    tb = 0.28209479f * b[0] - 0.12615663f * b[6] + 0.21850969f * b[8];
    out[3] += ta * b[3] + tb * a[3];
    t = a[3] * b[3];
    out[0] += 0.28209479f * t;
    out[6] -= 0.12615663f * t;
    out[8] += 0.21850969f * t;

    ta = 0.28209479f * a[0] - 0.18022375f * a[6];
    tb = 0.28209479f * b[0] - 0.18022375f * b[6];
    out[4] += ta * b[4] + tb * a[4];
    t = a[4] * b[4];
    out[0] += 0.28209479f * t;
    out[6] -= 0.18022375f * t;

    ta = 0.15607835f * a[7];
    tb = 0.15607835f * b[7];
    out[4] += ta * b[5] + tb * a[5];
    out[5] += ta * b[4] + tb * a[4];
    t = a[4] * b[5] + a[5] * b[4];
    out[7] += 0.15607835f * t;

    ta = 0.28209479f * a[0] + 0.09011188f * a[6] - 0.15607835f * a[8];
    tb = 0.28209479f * b[0] + 0.09011188f * b[6] - 0.15607835f * b[8];
    out[5] += ta * b[5] + tb * a[5];
    t = a[5] * b[5];
    out[0] += 0.28209479f * t;
    out[6] += 0.09011188f * t;
    out[8] -= 0.15607835f * t;

    ta = 0.28209480f * a[0];
    tb = 0.28209480f * b[0];
    out[6] += ta * b[6] + tb * a[6];
    t = a[6] * b[6];
    out[0] += 0.28209480f * t;
    out[6] += 0.18022376f * t;

    ta = 0.28209479f * a[0] + 0.09011188f * a[6] + 0.15607835f * a[8];
    tb = 0.28209479f * b[0] + 0.09011188f * b[6] + 0.15607835f * b[8];
    out[7] += ta * b[7] + tb * a[7];
    t = a[7] * b[7];
    out[0] += 0.28209479f * t;
    out[6] += 0.09011188f * t;
    out[8] += 0.15607835f * t;

    ta = 0.28209479f * a[0] - 0.18022375f * a[6];
    tb = 0.28209479f * b[0] - 0.18022375f * b[6];
    out[8] += ta * b[8] + tb * a[8];
    t = a[8] * b[8];
    out[0] += 0.28209479f * t;
    out[6] -= 0.18022375f * t;

    return out;
}

FLOAT * WINAPI D3DXSHMultiply4(FLOAT *out, const FLOAT *a, const FLOAT *b)
{
    FLOAT ta, tb, t;

    TRACE("out %p, a %p, b %p\n", out, a, b);

    out[0] = 0.28209479f * a[0] * b[0];

    ta = 0.28209479f * a[0] - 0.12615663f * a[6] - 0.21850969f * a[8];
    tb = 0.28209479f * b[0] - 0.12615663f * b[6] - 0.21850969f * b[8];
    out[1] = ta * b[1] + tb * a[1];
    t = a[1] * b[1];
    out[0] += 0.28209479f * t;
    out[6] = -0.12615663f * t;
    out[8] = -0.21850969f * t;

    ta = 0.21850969f * a[3] - 0.05839917f * a[13] - 0.22617901f * a[15];
    tb = 0.21850969f * b[3] - 0.05839917f * b[13] - 0.22617901f * b[15];
    out[1] += ta * b[4] + tb * a[4];
    out[4] = ta * b[1] + tb * a[1];
    t = a[1] * b[4] + a[4] * b[1];
    out[3] = 0.21850969f * t;
    out[13] = -0.05839917f * t;
    out[15] = -0.22617901f * t;

    ta = 0.21850969f * a[2] - 0.14304817f * a[12] - 0.18467439f * a[14];
    tb = 0.21850969f * b[2] - 0.14304817f * b[12] - 0.18467439f * b[14];
    out[1] += ta * b[5] + tb * a[5];
    out[5] = ta * b[1] + tb * a[1];
    t = a[1] * b[5] + a[5] * b[1];
    out[2] = 0.21850969f * t;
    out[12] = -0.14304817f * t;
    out[14] = -0.18467439f * t;

    ta = 0.20230066f * a[11];
    tb = 0.20230066f * b[11];
    out[1] += ta * b[6] + tb * a[6];
    out[6] += ta * b[1] + tb * a[1];
    t = a[1] * b[6] + a[6] * b[1];
    out[11] = 0.20230066f * t;

    ta = 0.22617901f * a[9] + 0.05839917f * a[11];
    tb = 0.22617901f * b[9] + 0.05839917f * b[11];
    out[1] += ta * b[8] + tb * a[8];
    out[8] += ta * b[1] + tb * a[1];
    t = a[1] * b[8] + a[8] * b[1];
    out[9] = 0.22617901f * t;
    out[11] += 0.05839917f * t;

    ta = 0.28209480f * a[0] + 0.25231326f * a[6];
    tb = 0.28209480f * b[0] + 0.25231326f * b[6];
    out[2] += ta * b[2] + tb * a[2];
    t = a[2] * b[2];
    out[0] += 0.28209480f * t;
    out[6] += 0.25231326f * t;

    ta = 0.24776671f * a[12];
    tb = 0.24776671f * b[12];
    out[2] += ta * b[6] + tb * a[6];
    out[6] += ta * b[2] + tb * a[2];
    t = a[2] * b[6] + a[6] * b[2];
    out[12] += 0.24776671f * t;

    ta = 0.28209480f * a[0] - 0.12615663f * a[6] + 0.21850969f * a[8];
    tb = 0.28209480f * b[0] - 0.12615663f * b[6] + 0.21850969f * b[8];
    out[3] += ta * b[3] + tb * a[3];
    t = a[3] * b[3];
    out[0] += 0.28209480f * t;
    out[6] -= 0.12615663f * t;
    out[8] += 0.21850969f * t;

    ta = 0.20230066f * a[13];
    tb = 0.20230066f * b[13];
    out[3] += ta * b[6] + tb * a[6];
    out[6] += ta * b[3] + tb * a[3];
    t = a[3] * b[6] + a[6] * b[3];
    out[13] += 0.20230066f * t;

    ta = 0.21850969f * a[2] - 0.14304817f * a[12] + 0.18467439f * a[14];
    tb = 0.21850969f * b[2] - 0.14304817f * b[12] + 0.18467439f * b[14];
    out[3] += ta * b[7] + tb * a[7];
    out[7] = ta * b[3] + tb * a[3];
    t = a[3] * b[7] + a[7] * b[3];
    out[2] += 0.21850969f * t;
    out[12] -= 0.14304817f * t;
    out[14] += 0.18467439f * t;

    ta = -0.05839917f * a[13] + 0.22617901f * a[15];
    tb = -0.05839917f * b[13] + 0.22617901f * b[15];
    out[3] += ta * b[8] + tb * a[8];
    out[8] += ta * b[3] + tb * a[3];
    t = a[3] * b[8] + a[8] * b[3];
    out[13] -= 0.05839917f * t;
    out[15] += 0.22617901f * t;

    ta = 0.28209479f * a[0] - 0.18022375f * a[6];
    tb = 0.28209479f * b[0] - 0.18022375f * b[6];
    out[4] += ta * b[4] + tb * a[4];
    t = a[4] * b[4];
    out[0] += 0.28209479f * t;
    out[6] -= 0.18022375f * t;

    ta = 0.15607835f * a[7];
    tb = 0.15607835f * b[7];
    out[4] += ta * b[5] + tb * a[5];
    out[5] += ta * b[4] + tb * a[4];
    t = a[4] * b[5] + a[5] * b[4];
    out[7] += 0.15607835f * t;

    ta = 0.22617901f * a[3] - 0.09403160f * a[13];
    tb = 0.22617901f * b[3] - 0.09403160f * b[13];
    out[4] += ta * b[9] + tb * a[9];
    out[9] += ta * b[4] + tb * a[4];
    t = a[4] * b[9] + a[9] * b[4];
    out[3] += 0.22617901f * t;
    out[13] -= 0.09403160f * t;

    ta = 0.18467439f * a[2] - 0.18806319f * a[12];
    tb = 0.18467439f * b[2] - 0.18806319f * b[12];
    out[4] += ta * b[10] + tb * a [10];
    out[10] = ta * b[4] + tb * a[4];
    t = a[4] * b[10] + a[10] * b[4];
    out[2] += 0.18467439f * t;
    out[12] -= 0.18806319f * t;

    ta = -0.05839917f * a[3] + 0.14567312f * a[13] + 0.09403160f * a[15];
    tb = -0.05839917f * b[3] + 0.14567312f * b[13] + 0.09403160f * b[15];
    out[4] += ta * b[11] + tb * a[11];
    out[11] += ta * b[4] + tb * a[4];
    t = a[4] * b[11] + a[11] * b[4];
    out[3] -= 0.05839917f * t;
    out[13] += 0.14567312f * t;
    out[15] += 0.09403160f * t;

    ta = 0.28209479f * a[0] + 0.09011186f * a[6] - 0.15607835f * a[8];
    tb = 0.28209479f * b[0] + 0.09011186f * b[6] - 0.15607835f * b[8];
    out[5] += ta * b[5] + tb * a[5];
    t = a[5] * b[5];
    out[0] += 0.28209479f * t;
    out[6] += 0.09011186f * t;
    out[8] -= 0.15607835f * t;

    ta = 0.14867701f * a[14];
    tb = 0.14867701f * b[14];
    out[5] += ta * b[9] + tb * a[9];
    out[9] += ta * b[5] + tb * a[5];
    t = a[5] * b[9] + a[9] * b[5];
    out[14] += 0.14867701f * t;

    ta = 0.18467439f * a[3] + 0.11516472f * a[13] - 0.14867701f * a[15];
    tb = 0.18467439f * b[3] + 0.11516472f * b[13] - 0.14867701f * b[15];
    out[5] += ta * b[10] + tb * a[10];
    out[10] += ta * b[5] + tb * a[5];
    t = a[5] * b[10] + a[10] * b[5];
    out[3] += 0.18467439f * t;
    out[13] += 0.11516472f * t;
    out[15] -= 0.14867701f * t;

    ta = 0.23359668f * a[2] + 0.05947080f * a[12] - 0.11516472f * a[14];
    tb = 0.23359668f * b[2] + 0.05947080f * b[12] - 0.11516472f * b[14];
    out[5] += ta * b[11] + tb * a[11];
    out[11] += ta * b[5] + tb * a[5];
    t = a[5] * b[11] + a[11] * b[5];
    out[2] += 0.23359668f * t;
    out[12] += 0.05947080f * t;
    out[14] -= 0.11516472f * t;

    ta = 0.28209479f * a[0];
    tb = 0.28209479f * b[0];
    out[6] += ta * b[6] + tb * a[6];
    t = a[6] * b[6];
    out[0] += 0.28209479f * t;
    out[6] += 0.18022376f * t;

    ta = 0.09011186f * a[6] + 0.28209479f * a[0] + 0.15607835f * a[8];
    tb = 0.09011186f * b[6] + 0.28209479f * b[0] + 0.15607835f * b[8];
    out[7] += ta * b[7] + tb * a[7];
    t = a[7] * b[7];
    out[6] += 0.09011186f * t;
    out[0] += 0.28209479f * t;
    out[8] += 0.15607835f * t;

    ta = 0.14867701f * a[9] + 0.18467439f * a[1] + 0.11516472f * a[11];
    tb = 0.14867701f * b[9] + 0.18467439f * b[1] + 0.11516472f * b[11];
    out[7] += ta * b[10] + tb * a[10];
    out[10] += ta * b[7] + tb * a[7];
    t = a[7] * b[10] + a[10] * b[7];
    out[9] += 0.14867701f * t;
    out[1] += 0.18467439f * t;
    out[11] += 0.11516472f * t;

    ta = 0.05947080f * a[12] + 0.23359668f * a[2] + 0.11516472f * a[14];
    tb = 0.05947080f * b[12] + 0.23359668f * b[2] + 0.11516472f * b[14];
    out[7] += ta * b[13] + tb * a[13];
    out[13] += ta * b[7]+ tb * a[7];
    t = a[7] * b[13] + a[13] * b[7];
    out[12] += 0.05947080f * t;
    out[2] += 0.23359668f * t;
    out[14] += 0.11516472f * t;

    ta = 0.14867701f * a[15];
    tb = 0.14867701f * b[15];
    out[7] += ta * b[14] + tb * a[14];
    out[14] += ta * b[7] + tb * a[7];
    t = a[7] * b[14] + a[14] * b[7];
    out[15] += 0.14867701f * t;

    ta = 0.28209479f * a[0] - 0.18022375f * a[6];
    tb = 0.28209479f * b[0] - 0.18022375f * b[6];
    out[8] += ta * b[8] + tb * a[8];
    t = a[8] * b[8];
    out[0] += 0.28209479f * t;
    out[6] -= 0.18022375f * t;

    ta = -0.09403160f * a[11];
    tb = -0.09403160f * b[11];
    out[8] += ta * b[9] + tb * a[9];
    out[9] += ta * b[8] + tb * a[8];
    t = a[8] * b[9] + a[9] * b[8];
    out[11] -= 0.09403160f * t;

    ta = -0.09403160f * a[15];
    tb = -0.09403160f * b[15];
    out[8] += ta * b[13] + tb * a[13];
    out[13] += ta * b[8] + tb * a[8];
    t = a[8] * b[13] + a[13] * b[8];
    out[15] -= 0.09403160f * t;

    ta = 0.18467439f * a[2] - 0.18806319f * a[12];
    tb = 0.18467439f * b[2] - 0.18806319f * b[12];
    out[8] += ta * b[14] + tb * a[14];
    out[14] += ta * b[8] + tb * a[8];
    t = a[8] * b[14] + a[14] * b[8];
    out[2] += 0.18467439f * t;
    out[12] -= 0.18806319f * t;

    ta = -0.21026104f * a[6] + 0.28209479f * a[0];
    tb = -0.21026104f * b[6] + 0.28209479f * b[0];
    out[9] += ta * b[9] + tb * a[9];
    t = a[9] * b[9];
    out[6] -= 0.21026104f * t;
    out[0] += 0.28209479f * t;

    ta = 0.28209479f * a[0];
    tb = 0.28209479f * b[0];
    out[10] += ta * b[10] + tb * a[10];
    t = a[10] * b[10];
    out[0] += 0.28209479f * t;

    ta = 0.28209479f * a[0] + 0.12615663f * a[6] - 0.14567312f * a[8];
    tb = 0.28209479f * b[0] + 0.12615663f * b[6] - 0.14567312f * b[8];
    out[11] += ta * b[11] + tb * a[11];
    t = a[11] * b[11];
    out[0] += 0.28209479f * t;
    out[6] += 0.12615663f * t;
    out[8] -= 0.14567312f * t;

    ta = 0.28209479f * a[0] + 0.16820885f * a[6];
    tb = 0.28209479f * b[0] + 0.16820885f * b[6];
    out[12] += ta * b[12] + tb * a[12];
    t = a[12] * b[12];
    out[0] += 0.28209479f * t;
    out[6] += 0.16820885f * t;

    ta =0.28209479f * a[0] + 0.14567312f * a[8] + 0.12615663f * a[6];
    tb =0.28209479f * b[0] + 0.14567312f * b[8] + 0.12615663f * b[6];
    out[13] += ta * b[13] + tb * a[13];
    t = a[13] * b[13];
    out[0] += 0.28209479f * t;
    out[8] += 0.14567312f * t;
    out[6] += 0.12615663f * t;

    ta = 0.28209479f * a[0];
    tb = 0.28209479f * b[0];
    out[14] += ta * b[14] + tb * a[14];
    t = a[14] * b[14];
    out[0] += 0.28209479f * t;

    ta = 0.28209479f * a[0] - 0.21026104f * a[6];
    tb = 0.28209479f * b[0] - 0.21026104f * b[6];
    out[15] += ta * b[15] + tb * a[15];
    t = a[15] * b[15];
    out[0] += 0.28209479f * t;
    out[6] -= 0.21026104f * t;

    return out;
}

static void rotate_X(FLOAT *out, UINT order, FLOAT a, FLOAT *in)
{
    out[0] = in[0];

    out[1] = a * in[2];
    out[2] = -a * in[1];
    out[3] = in[3];

    out[4] = a * in[7];
    out[5] = -in[5];
    out[6] = -0.5f * in[6] - 0.8660253882f * in[8];
    out[7] = -a * in[4];
    out[8] = -0.8660253882f * in[6] + 0.5f * in[8];
    out[9] = -a * 0.7905694842f * in[12] + a * 0.6123724580f * in[14];

    out[10] = -in[10];
    out[11] = -a * 0.6123724580f * in[12] - a * 0.7905694842f * in[14];
    out[12] = a * 0.7905694842f * in[9] + a * 0.6123724580f * in[11];
    out[13] = -0.25f * in[13] - 0.9682458639f * in[15];
    out[14] = -a * 0.6123724580f * in[9] + a * 0.7905694842f * in[11];
    out[15] = -0.9682458639f * in[13] + 0.25f * in[15];
    if (order == 4)
        return;

    out[16] = -a * 0.9354143739f * in[21] + a * 0.3535533845f * in[23];
    out[17] = -0.75f * in[17] + 0.6614378095f * in[19];
    out[18] = -a * 0.3535533845f * in[21] - a * 0.9354143739f * in[23];
    out[19] = 0.6614378095f * in[17] + 0.75f * in[19];
    out[20] = 0.375f * in[20] + 0.5590170026f * in[22] + 0.7395099998f * in[24];
    out[21] = a * 0.9354143739f * in[16] + a * 0.3535533845f * in[18];
    out[22] = 0.5590170026f * in[20] + 0.5f * in[22] - 0.6614378691f * in[24];
    out[23] = -a * 0.3535533845f * in[16] + a * 0.9354143739f * in[18];
    out[24] = 0.7395099998f * in[20] - 0.6614378691f * in[22] + 0.125f * in[24];
    if (order == 5)
        return;

    out[25] = a * 0.7015607357f * in[30] - a * 0.6846531630f * in[32] + a * 0.1976423711f * in[34];
    out[26] = -0.5f * in[26] + 0.8660253882f * in[28];
    out[27] = a * 0.5229125023f * in[30] + a * 0.3061861992f * in[32] - a * 0.7954951525f * in[34];
    out[28] = 0.8660253882f * in[26] + 0.5f * in[28];
    out[29] = a * 0.4841229022f * in[30] + a * 0.6614378691f * in[32] + a * 0.5728219748f * in[34];
    out[30] = -a * 0.7015607357f * in[25] - a * 0.5229125023f * in[27] - a * 0.4841229022f * in[29];
    out[31] = 0.125f * in[31] + 0.4050463140f * in[33] + 0.9057110548f * in[35];
    out[32] = a * 0.6846531630f * in[25] - a * 0.3061861992f * in[27] - a * 0.6614378691f * in[29];
    out[33] = 0.4050463140f * in[31] + 0.8125f * in[33] - 0.4192627370f * in[35];
    out[34] = -a * 0.1976423711f * in[25] + a * 0.7954951525f * in[27] - a * 0.5728219748f * in[29];
    out[35] = 0.9057110548f * in[31] - 0.4192627370f * in[33] + 0.0624999329f * in[35];
}

static void set_vec3(D3DXVECTOR3 *v, float x, float y, float z)
{
    v->x = x;
    v->y = y;
    v->z = z;
}

/*
 * The following implementation of D3DXSHProjectCubeMap is based on the
 * SHProjectCubeMap() implementation from Microsoft's DirectXMath library,
 * covered under the following copyright:
 *
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
HRESULT WINAPI D3DXSHProjectCubeMap(unsigned int order, IDirect3DCubeTexture9 *texture, float *red, float *green, float *blue)
{
    const unsigned int order_square = order * order;
    const struct pixel_format_desc *format;
    unsigned int x, y, i, face;
    float B, S, proj_normal;
    D3DSURFACE_DESC desc;
    float Wt = 0.0f;
    float *temp;
    HRESULT hr;

    TRACE("order %u, texture %p, red %p, green %p, blue %p.\n", order, texture, red, green, blue);

    if (!texture || !red || order < D3DXSH_MINORDER || order > D3DXSH_MAXORDER)
        return D3DERR_INVALIDCALL;

    memset(red, 0, order_square * sizeof(float));
    if (green)
        memset(green, 0, order_square * sizeof(float));
    if (blue)
        memset(blue, 0, order_square * sizeof(float));

    if (FAILED(hr = IDirect3DCubeTexture9_GetLevelDesc(texture, 0, &desc)))
    {
        ERR("Failed to get level desc, hr %#lx.\n", hr);
        return hr;
    }

    format = get_format_info(desc.Format);
    if (is_unknown_format(format) || is_index_format(format) || is_compressed_format(format))
    {
        FIXME("Unsupported texture format %#x.\n", desc.Format);
        return D3DERR_INVALIDCALL;
    }

    if (!(temp = malloc(order_square * sizeof(*temp))))
        return E_OUTOFMEMORY;

    B = -1.0f + 1.0f / desc.Width;
    if (desc.Width > 1)
        S = 2.0f * (1.0f - 1.0f / desc.Width) / (desc.Width - 1.0f);
    else
        S = 0.0f;

    for (face = 0; face < 6; ++face)
    {
        D3DLOCKED_RECT map_desc;

        if (FAILED(hr = IDirect3DCubeTexture9_LockRect(texture, face, 0, &map_desc, NULL, D3DLOCK_READONLY)))
        {
            ERR("Failed to map texture, hr %#lx.\n", hr);
            free(temp);
            return hr;
        }

        for (y = 0; y < desc.Height; ++y)
        {
            const BYTE *row = (const BYTE *)map_desc.pBits + y * map_desc.Pitch;

            for (x = 0; x < desc.Width; ++x)
            {
                float diff_solid, x_3d, y_3d;
                const float u = x * S + B;
                const float v = y * S + B;
                struct d3dx_color colour;
                D3DXVECTOR3 dir;

                x_3d = (x * 2.0f + 1.0f) / desc.Width - 1.0f;
                y_3d = (y * 2.0f + 1.0f) / desc.Width - 1.0f;

                switch (face)
                {
                    case D3DCUBEMAP_FACE_POSITIVE_X:
                        set_vec3(&dir, 1.0f, -y_3d, -x_3d);
                        break;

                    case D3DCUBEMAP_FACE_NEGATIVE_X:
                        set_vec3(&dir, -1.0f, -y_3d, x_3d);
                        break;

                    case D3DCUBEMAP_FACE_POSITIVE_Y:
                        set_vec3(&dir, x_3d, 1.0f, y_3d);
                        break;

                    case D3DCUBEMAP_FACE_NEGATIVE_Y:
                        set_vec3(&dir, x_3d, -1.0f, -y_3d);
                        break;

                    case D3DCUBEMAP_FACE_POSITIVE_Z:
                        set_vec3(&dir, x_3d, -y_3d, 1.0f);
                        break;

                    case D3DCUBEMAP_FACE_NEGATIVE_Z:
                        set_vec3(&dir, -x_3d, -y_3d, -1.0f);
                        break;
                }

                /* This is more complex than powf(..., 1.5f), but also happens
                 * to be slightly more accurate, and slightly faster as well. */
                diff_solid = 4.0f / ((1.0f + u * u + v * v) * sqrtf(1.0f + u * u + v * v));
                Wt += diff_solid;

                D3DXVec3Normalize(&dir, &dir);
                D3DXSHEvalDirection(temp, order, &dir);

                format_to_d3dx_color(format, &row[x * format->block_byte_count], NULL, &colour);

                for (i = 0; i < order_square; ++i)
                {
                    red[i] += temp[i] * colour.value.x * diff_solid;
                    if (green)
                        green[i] += temp[i] * colour.value.y * diff_solid;
                    if (blue)
                        blue[i] += temp[i] * colour.value.z * diff_solid;
                }
            }
        }

        IDirect3DCubeTexture9_UnlockRect(texture, face, 0);
    }

    proj_normal = (4.0f * M_PI) / Wt;
    D3DXSHScale(red, order, red, proj_normal);
    if (green)
        D3DXSHScale(green, order, green, proj_normal);
    if (blue)
        D3DXSHScale(blue, order, blue, proj_normal);

    free(temp);
    return D3D_OK;
}

FLOAT* WINAPI D3DXSHRotate(FLOAT *out, UINT order, const D3DXMATRIX *matrix, const FLOAT *in)
{
    FLOAT alpha, beta, gamma, sinb, temp[36], temp1[36];

    TRACE("out %p, order %u, matrix %p, in %p\n", out, order, matrix, in);

    out[0] = in[0];

    if ((order > D3DXSH_MAXORDER) || (order < D3DXSH_MINORDER))
        return out;

    if (order <= 3)
    {
        out[1] = matrix->m[1][1] * in[1] - matrix->m[2][1] * in[2] + matrix->m[0][1] * in[3];
        out[2] = -matrix->m[1][2] * in[1] + matrix->m[2][2] * in[2] - matrix->m[0][2] * in[3];
        out[3] = matrix->m[1][0] * in[1] - matrix->m[2][0] * in[2] + matrix->m[0][0] * in[3];

        if (order == 3)
        {
            float coeff[] =
            {
                matrix->m[1][0] * matrix->m[0][0], matrix->m[1][1] * matrix->m[0][1],
                matrix->m[1][1] * matrix->m[2][1], matrix->m[1][0] * matrix->m[2][0],
                matrix->m[2][0] * matrix->m[2][0], matrix->m[2][1] * matrix->m[2][1],
                matrix->m[0][0] * matrix->m[2][0], matrix->m[0][1] * matrix->m[2][1],
                matrix->m[0][1] * matrix->m[0][1], matrix->m[1][0] * matrix->m[1][0],
                matrix->m[1][1] * matrix->m[1][1], matrix->m[0][0] * matrix->m[0][0],
            };

            out[4] = (matrix->m[1][1] * matrix->m[0][0] + matrix->m[0][1] * matrix->m[1][0]) * in[4];
            out[4] -= (matrix->m[1][0] * matrix->m[2][1] + matrix->m[1][1] * matrix->m[2][0]) * in[5];
            out[4] += 1.7320508076f * matrix->m[2][0] * matrix->m[2][1] * in[6];
            out[4] -= (matrix->m[0][1] * matrix->m[2][0] + matrix->m[0][0] * matrix->m[2][1]) * in[7];
            out[4] += (matrix->m[0][0] * matrix->m[0][1] - matrix->m[1][0] * matrix->m[1][1]) * in[8];

            out[5] = (matrix->m[1][1] * matrix->m[2][2] + matrix->m[1][2] * matrix->m[2][1]) * in[5];
            out[5] -= (matrix->m[1][1] * matrix->m[0][2] + matrix->m[1][2] * matrix->m[0][1]) * in[4];
            out[5] -= 1.7320508076f * matrix->m[2][2] * matrix->m[2][1] * in[6];
            out[5] += (matrix->m[0][2] * matrix->m[2][1] + matrix->m[0][1] * matrix->m[2][2]) * in[7];
            out[5] -= (matrix->m[0][1] * matrix->m[0][2] - matrix->m[1][1] * matrix->m[1][2]) * in[8];

            out[6] = (matrix->m[2][2] * matrix->m[2][2] - 0.5f * (coeff[4] + coeff[5])) * in[6];
            out[6] -= (0.5773502692f * (coeff[0] + coeff[1]) - 1.1547005384f * matrix->m[1][2] * matrix->m[0][2]) * in[4];
            out[6] += (0.5773502692f * (coeff[2] + coeff[3]) - 1.1547005384f * matrix->m[1][2] * matrix->m[2][2]) * in[5];
            out[6] += (0.5773502692f * (coeff[6] + coeff[7]) - 1.1547005384f * matrix->m[0][2] * matrix->m[2][2]) * in[7];
            out[6] += (0.2886751347f * (coeff[9] - coeff[8] + coeff[10] - coeff[11]) - 0.5773502692f *
                  (matrix->m[1][2] * matrix->m[1][2] - matrix->m[0][2] * matrix->m[0][2])) * in[8];

            out[7] = (matrix->m[0][0] * matrix->m[2][2] + matrix->m[0][2] * matrix->m[2][0]) * in[7];
            out[7] -= (matrix->m[1][0] * matrix->m[0][2] + matrix->m[1][2] * matrix->m[0][0]) * in[4];
            out[7] += (matrix->m[1][0] * matrix->m[2][2] + matrix->m[1][2] * matrix->m[2][0]) * in[5];
            out[7] -= 1.7320508076f * matrix->m[2][2] * matrix->m[2][0] * in[6];
            out[7] -= (matrix->m[0][0] * matrix->m[0][2] - matrix->m[1][0] * matrix->m[1][2]) * in[8];

            out[8] = 0.5f * (coeff[11] - coeff[8] - coeff[9] + coeff[10]) * in[8];
            out[8] += (coeff[0] - coeff[1]) * in[4];
            out[8] += (coeff[2] - coeff[3]) * in[5];
            out[8] += 0.86602540f * (coeff[4] - coeff[5]) * in[6];
            out[8] += (coeff[7] - coeff[6]) * in[7];
        }

        return out;
    }

    if (fabsf(matrix->m[2][2]) != 1.0f)
    {
        sinb = sqrtf(1.0f - matrix->m[2][2] * matrix->m[2][2]);
        alpha = atan2f(matrix->m[2][1] / sinb, matrix->m[2][0] / sinb);
        beta = atan2f(sinb, matrix->m[2][2]);
        gamma = atan2f(matrix->m[1][2] / sinb, -matrix->m[0][2] / sinb);
    }
    else
    {
        alpha = atan2f(matrix->m[0][1], matrix->m[0][0]);
        beta = 0.0f;
        gamma = 0.0f;
    }

    D3DXSHRotateZ(temp, order, gamma, in);
    rotate_X(temp1, order, 1.0f, temp);
    D3DXSHRotateZ(temp, order, beta, temp1);
    rotate_X(temp1, order, -1.0f, temp);
    D3DXSHRotateZ(out, order, alpha, temp1);

    return out;
}

FLOAT * WINAPI D3DXSHRotateZ(FLOAT *out, UINT order, FLOAT angle, const FLOAT *in)
{
    UINT i, sum = 0;
    FLOAT c[5], s[5];

    TRACE("out %p, order %u, angle %f, in %p\n", out, order, angle, in);

    order = min(max(order, D3DXSH_MINORDER), D3DXSH_MAXORDER);

    out[0] = in[0];

    for (i = 1; i < order; i++)
    {
        UINT j;

        c[i - 1] = cosf(i * angle);
        s[i - 1] = sinf(i * angle);
        sum += i * 2;

        out[sum - i] = c[i - 1] * in[sum - i];
        out[sum - i] += s[i - 1] * in[sum + i];
        for (j = i - 1; j > 0; j--)
        {
            out[sum - j] = 0.0f;
            out[sum - j] = c[j - 1] * in[sum - j];
            out[sum - j] += s[j - 1] * in[sum + j];
        }

        if (in == out)
            out[sum] = 0.0f;
        else
            out[sum] = in[sum];

        for (j = 1; j < i; j++)
        {
            out[sum + j] = 0.0f;
            out[sum + j] = -s[j - 1] * in[sum - j];
            out[sum + j] += c[j - 1] * in[sum + j];
        }
        out[sum + i] = -s[i - 1] * in[sum - i];
        out[sum + i] += c[i - 1] * in[sum + i];
    }

    return out;
}

FLOAT* WINAPI D3DXSHScale(FLOAT *out, UINT order, const FLOAT *a, const FLOAT scale)
{
    UINT i;

    TRACE("out %p, order %u, a %p, scale %f\n", out, order, a, scale);

    for (i = 0; i < order * order; i++)
        out[i] = a[i] * scale;

    return out;
}
