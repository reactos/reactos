/*
 * Copyright 2000 Corel Corporation
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "twain.h"
#include "twain_i.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(twain);

/* DG_AUDIO/DAT_AUDIOFILEXFER/MSG_GET */
TW_UINT16 TWAIN_AudioFileXferGet (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_AUDIO/DAT_AUDIOINFO/MSG_GET */
TW_UINT16 TWAIN_AudioInfoGet (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                              TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_AUDIO/DAT_AUDIONATIVEXFER/MSG_GET */
TW_UINT16 TWAIN_AudioNativeXferGet (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                                    TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

