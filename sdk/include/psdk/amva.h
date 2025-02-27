/*
 * Copyright (C) 2022 Zebediah Figura for CodeWeavers
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

#ifndef __AMVA_INCLUDED__
#define __AMVA_INCLUDED__

typedef struct _tag_AMVABeginFrameInfo
{
    DWORD dwDestSurfaceIndex;
    void *pInputData;
    DWORD dwSizeInputData;
    void *pOutputData;
    DWORD dwSizeOutputData;
} AMVABeginFrameInfo, *LPAMVABeginFrameInfo;

typedef struct _tag_AMVABUFFERINFO
{
    DWORD dwTypeIndex;
    DWORD dwBufferIndex;
    DWORD dwDataOffset;
    DWORD dwDataSize;
} AMVABUFFERINFO, *LPAMVABUFFERINFO;

typedef struct _tag_AMVACompBufferInfo
{
    DWORD dwNumCompBuffers;
    DWORD dwWidthToCreate;
    DWORD dwHeightToCreate;
    DWORD dwBytesToAllocate;
    DDSCAPS2 ddCompCaps;
    DDPIXELFORMAT ddPixelFormat;
} AMVACompBufferInfo, *LPAMVACompBufferInfo;

typedef struct _tag_AMVAEndFrameInfo
{
    DWORD dwSizeMiscData;
    void *pMiscData;
} AMVAEndFrameInfo, *LPAMVAEndFrameInfo;

typedef struct _tag_AMVAInternalMemInfo
{
    DWORD dwScratchMemAlloc;
} AMVAInternalMemInfo, *LPAMVAInternalMemInfo;

typedef struct _tag_AMVAUncompBufferInfo
{
    DWORD dwMinNumSurfaces;
    DWORD dwMaxNumSurfaces;
    DDPIXELFORMAT ddUncompPixelFormat;
} AMVAUncompBufferInfo, *LPAMVAUncompBufferInfo;

typedef struct _tag_AMVAUncompDataInfo
{
    DWORD dwUncompWidth;
    DWORD dwUncompHeight;
    DDPIXELFORMAT ddUncompPixelFormat;
} AMVAUncompDataInfo, *LPAMVAUncompDataInfo;

#endif
