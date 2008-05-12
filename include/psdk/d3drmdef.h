/*
 * Copyright 2007 Vijay Kiran Kamuju
 * Copyright 2007 David ADAM
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

#ifndef __D3DRMDEFS_H__
#define __D3DRMDEFS_H__

#include <stddef.h>
#include <d3dtypes.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef D3DVALUE D3DRMMATRIX4D[4][4];
typedef struct _D3DRMQUATERNION
{
    D3DVALUE s;
    D3DVECTOR v;
} D3DRMQUATERNION, *LPD3DRMQUATERNION;

void WINAPI D3DRMMatrixFromQuaternion(D3DRMMATRIX4D, LPD3DRMQUATERNION);

LPD3DRMQUATERNION WINAPI D3DRMQuaternionFromRotation(LPD3DRMQUATERNION ,LPD3DVECTOR,D3DVALUE);
LPD3DRMQUATERNION WINAPI D3DRMQuaternionMultiply(LPD3DRMQUATERNION, LPD3DRMQUATERNION, LPD3DRMQUATERNION);
LPD3DRMQUATERNION WINAPI D3DRMQuaternionSlerp(LPD3DRMQUATERNION, LPD3DRMQUATERNION, LPD3DRMQUATERNION, D3DVALUE);

LPD3DVECTOR WINAPI D3DRMVectorAdd(LPD3DVECTOR, LPD3DVECTOR, LPD3DVECTOR);
LPD3DVECTOR WINAPI D3DRMVectorCrossProduct(LPD3DVECTOR, LPD3DVECTOR, LPD3DVECTOR);
D3DVALUE WINAPI D3DRMVectorDotProduct(LPD3DVECTOR, LPD3DVECTOR);
LPD3DVECTOR WINAPI D3DRMVectorNormalize(LPD3DVECTOR);

#define D3DRMVectorNormalise D3DRMVectorNormalize

D3DVALUE WINAPI D3DRMVectorModulus(LPD3DVECTOR);
LPD3DVECTOR WINAPI D3DRMVectorRandom(LPD3DVECTOR);
LPD3DVECTOR WINAPI D3DRMVectorRotate(LPD3DVECTOR, LPD3DVECTOR, LPD3DVECTOR, D3DVALUE);
LPD3DVECTOR WINAPI D3DRMVectorReflect(LPD3DVECTOR, LPD3DVECTOR, LPD3DVECTOR);
LPD3DVECTOR WINAPI D3DRMVectorScale(LPD3DVECTOR, LPD3DVECTOR, D3DVALUE);
LPD3DVECTOR WINAPI D3DRMVectorSubtract(LPD3DVECTOR, LPD3DVECTOR, LPD3DVECTOR);

D3DCOLOR WINAPI D3DRMCreateColorRGB(D3DVALUE, D3DVALUE, D3DVALUE);
D3DCOLOR WINAPI D3DRMCreateColorRGBA(D3DVALUE, D3DVALUE, D3DVALUE, D3DVALUE);
D3DVALUE WINAPI D3DRMColorGetAlpha(D3DCOLOR);
D3DVALUE WINAPI D3DRMColorGetBlue(D3DCOLOR);
D3DVALUE WINAPI D3DRMColorGetGreen(D3DCOLOR);
D3DVALUE WINAPI D3DRMColorGetRed(D3DCOLOR);

#if defined(__cplusplus)
}
#endif

#endif
