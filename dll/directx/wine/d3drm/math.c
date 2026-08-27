/*
 * Copyright 2007 David Adam
 * Copyright 2007 Vijay Kiran Kamuju
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

#include "d3drm_private.h"

/* Create a RGB color from its components */
D3DCOLOR WINAPI D3DRMCreateColorRGB(D3DVALUE red, D3DVALUE green, D3DVALUE blue)
{
    return D3DRMCreateColorRGBA(red, green, blue, 1.0f);
}
/* Create a RGBA color from its components */
D3DCOLOR WINAPI D3DRMCreateColorRGBA(D3DVALUE red, D3DVALUE green, D3DVALUE blue, D3DVALUE alpha)
{
    D3DCOLOR color;

    d3drm_set_color(&color, red, green, blue, alpha);

    return color;
}

/* Determine the alpha part of a color */
D3DVALUE WINAPI D3DRMColorGetAlpha(D3DCOLOR color)
{
    return (RGBA_GETALPHA(color)/255.0);
}

/* Determine the blue part of a color */
D3DVALUE WINAPI D3DRMColorGetBlue(D3DCOLOR color)
{
    return (RGBA_GETBLUE(color)/255.0);
}

/* Determine the green part of a color */
D3DVALUE WINAPI D3DRMColorGetGreen(D3DCOLOR color)
{
    return (RGBA_GETGREEN(color)/255.0);
}

/* Determine the red part of a color */
D3DVALUE WINAPI D3DRMColorGetRed(D3DCOLOR color)
{
    return (RGBA_GETRED(color)/255.0);
}

/* Product of 2 quaternions */
D3DRMQUATERNION * WINAPI D3DRMQuaternionMultiply(D3DRMQUATERNION *q, D3DRMQUATERNION *a, D3DRMQUATERNION *b)
{
    D3DRMQUATERNION temp;
    D3DVECTOR cross_product;

    D3DRMVectorCrossProduct(&cross_product, &a->v, &b->v);
    temp.s = a->s * b->s - D3DRMVectorDotProduct(&a->v, &b->v);
    temp.v.x = a->s * b->v.x + b->s * a->v.x + cross_product.x;
    temp.v.y = a->s * b->v.y + b->s * a->v.y + cross_product.y;
    temp.v.z = a->s * b->v.z + b->s * a->v.z + cross_product.z;

    *q = temp;
    return q;
}

/* Matrix for the Rotation that a unit quaternion represents */
void WINAPI D3DRMMatrixFromQuaternion(D3DRMMATRIX4D m, D3DRMQUATERNION *q)
{
    D3DVALUE w,x,y,z;
    w = q->s;
    x = q->v.x;
    y = q->v.y;
    z = q->v.z;
    m[0][0] = 1.0-2.0*(y*y+z*z);
    m[1][1] = 1.0-2.0*(x*x+z*z);
    m[2][2] = 1.0-2.0*(x*x+y*y);
    m[1][0] = 2.0*(x*y+z*w);
    m[0][1] = 2.0*(x*y-z*w);
    m[2][0] = 2.0*(x*z-y*w);
    m[0][2] = 2.0*(x*z+y*w);
    m[2][1] = 2.0*(y*z+x*w);
    m[1][2] = 2.0*(y*z-x*w);
    m[3][0] = 0.0;
    m[3][1] = 0.0;
    m[3][2] = 0.0;
    m[0][3] = 0.0;
    m[1][3] = 0.0;
    m[2][3] = 0.0;
    m[3][3] = 1.0;
}

/* Return a unit quaternion that represents a rotation of an angle around an axis */
D3DRMQUATERNION * WINAPI D3DRMQuaternionFromRotation(D3DRMQUATERNION *q, D3DVECTOR *v, D3DVALUE theta)
{
    q->s = cos(theta/2.0);
    D3DRMVectorScale(&q->v, D3DRMVectorNormalize(v), sin(theta/2.0));
    return q;
}

/* Interpolation between two quaternions */
D3DRMQUATERNION * WINAPI D3DRMQuaternionSlerp(D3DRMQUATERNION *q,
        D3DRMQUATERNION *a, D3DRMQUATERNION *b, D3DVALUE alpha)
{
    D3DVALUE dot, epsilon, temp, theta, u;
    D3DVECTOR v1, v2;

    dot = a->s * b->s + D3DRMVectorDotProduct(&a->v, &b->v);
    epsilon = 1.0f;
    temp = 1.0f - alpha;
    u = alpha;
    if (dot < 0.0)
    {
     epsilon = -1.0;
     dot = -dot;
    }
    if( 1.0f - dot > 0.001f )
    {
        theta = acos(dot);
        temp  = sin(theta * temp) / sin(theta);
        u = sin(theta * alpha) / sin(theta);
    }
    q->s = temp * a->s + epsilon * u * b->s;
    D3DRMVectorScale(&v1, &a->v, temp);
    D3DRMVectorScale(&v2, &b->v, epsilon * u);
    D3DRMVectorAdd(&q->v, &v1, &v2);
    return q;
}

/* Add Two Vectors */
D3DVECTOR * WINAPI D3DRMVectorAdd(D3DVECTOR *d, D3DVECTOR *s1, D3DVECTOR *s2)
{
    D3DVECTOR temp;

    temp.x=s1->x + s2->x;
    temp.y=s1->y + s2->y;
    temp.z=s1->z + s2->z;

    *d = temp;
    return d;
}

/* Subtract Two Vectors */
D3DVECTOR * WINAPI D3DRMVectorSubtract(D3DVECTOR *d, D3DVECTOR *s1, D3DVECTOR *s2)
{
    D3DVECTOR temp;

    temp.x=s1->x - s2->x;
    temp.y=s1->y - s2->y;
    temp.z=s1->z - s2->z;

    *d = temp;
    return d;
}

/* Cross Product of Two Vectors */
D3DVECTOR * WINAPI D3DRMVectorCrossProduct(D3DVECTOR *d, D3DVECTOR *s1, D3DVECTOR *s2)
{
    D3DVECTOR temp;

    temp.x=s1->y * s2->z - s1->z * s2->y;
    temp.y=s1->z * s2->x - s1->x * s2->z;
    temp.z=s1->x * s2->y - s1->y * s2->x;

    *d = temp;
    return d;
}

/* Dot Product of Two vectors */
D3DVALUE WINAPI D3DRMVectorDotProduct(D3DVECTOR *s1, D3DVECTOR *s2)
{
    D3DVALUE dot_product;
    dot_product=s1->x * s2->x + s1->y * s2->y + s1->z * s2->z;
    return dot_product;
}

/* Norm of a vector */
D3DVALUE WINAPI D3DRMVectorModulus(D3DVECTOR *v)
{
    D3DVALUE result;
    result=sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
    return result;
}

/* Normalize a vector.  Returns (1,0,0) if INPUT is the NULL vector. */
D3DVECTOR * WINAPI D3DRMVectorNormalize(D3DVECTOR *u)
{
    D3DVALUE modulus = D3DRMVectorModulus(u);
    if(modulus)
    {
        D3DRMVectorScale(u,u,1.0/modulus);
    }
    else
    {
        u->x=1.0;
        u->y=0.0;
        u->z=0.0;
    }
    return u;
}

/* Returns a random unit vector */
D3DVECTOR * WINAPI D3DRMVectorRandom(D3DVECTOR *d)
{
    d->x = rand();
    d->y = rand();
    d->z = rand();
    D3DRMVectorNormalize(d);
    return d;
}

/* Reflection of a vector on a surface */
D3DVECTOR * WINAPI D3DRMVectorReflect(D3DVECTOR *r, D3DVECTOR *ray, D3DVECTOR *norm)
{
    D3DVECTOR sca, temp;
    D3DRMVectorSubtract(&temp, D3DRMVectorScale(&sca, norm, 2.0*D3DRMVectorDotProduct(ray,norm)), ray);

    *r = temp;
    return r;
}

/* Rotation of a vector */
D3DVECTOR * WINAPI D3DRMVectorRotate(D3DVECTOR *r, D3DVECTOR *v, D3DVECTOR *axis, D3DVALUE theta)
{
    D3DRMQUATERNION quaternion1, quaternion2, quaternion3;
    D3DVECTOR norm;

    quaternion1.s = cos(theta * 0.5f);
    quaternion2.s = cos(theta * 0.5f);
    norm = *D3DRMVectorNormalize(axis);
    D3DRMVectorScale(&quaternion1.v, &norm, sin(theta * 0.5f));
    D3DRMVectorScale(&quaternion2.v, &norm, -sin(theta * 0.5f));
    quaternion3.s = 0.0;
    quaternion3.v = *v;
    D3DRMQuaternionMultiply(&quaternion1, &quaternion1, &quaternion3);
    D3DRMQuaternionMultiply(&quaternion1, &quaternion1, &quaternion2);

    *r = *D3DRMVectorNormalize(&quaternion1.v);
    return r;
}

/* Scale a vector */
D3DVECTOR * WINAPI D3DRMVectorScale(D3DVECTOR *d, D3DVECTOR *s, D3DVALUE factor)
{
    D3DVECTOR temp;

    temp.x=factor * s->x;
    temp.y=factor * s->y;
    temp.z=factor * s->z;

    *d = temp;
    return d;
}
