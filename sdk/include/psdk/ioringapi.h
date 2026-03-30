/*
 * Copyright (C) 2023 Paul Gofman for CodeWeavers
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

#ifndef __IORINGAPI_H_
#define __IORINGAPI_H_

#include <ntioring_x.h>

struct IORING_CAPABILITIES
{
    IORING_VERSION       MaxVersion;
    UINT32               MaxSubmissionQueueSize;
    UINT32               MaxCompletionQueueSize;
    IORING_FEATURE_FLAGS FeatureFlags;
};
typedef struct IORING_CAPABILITIES IORING_CAPABILITIES;

#ifdef __cplusplus
extern "C" {
#endif

HRESULT WINAPI QueryIoRingCapabilities(IORING_CAPABILITIES *caps);

#ifdef __cplusplus
}
#endif
#endif
