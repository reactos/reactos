/*
 * PROJECT:     Original Xbox onboard hardware
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     nVidia NV2A (XGPU) header file
 * COPYRIGHT:   Copyright 2020 Stanislav Motylkov (x86corez@gmail.com)
 */

#ifndef _XGPU_H_
#define _XGPU_H_

#pragma once

/*
 * Registers and definitions
 */
#define NV2A_VIDEO_MEMORY_SIZE  (4 * 1024 * 1024) /* FIXME: obtain fb size from firmware somehow (Cromwell reserves high 4 MB of RAM) */

#define NV2A_FB_OFFSET                 0x100000
#define   NV2A_FB_CFG0                   (0x200 + NV2A_FB_OFFSET)
#define NV2A_CRTC_OFFSET               0x600000
#define   NV2A_CRTC_FRAMEBUFFER_START    (0x800 + NV2A_CRTC_OFFSET)
#define   NV2A_CRTC_REGISTER_INDEX      (0x13D4 + NV2A_CRTC_OFFSET)
#define   NV2A_CRTC_REGISTER_VALUE      (0x13D5 + NV2A_CRTC_OFFSET)
#define NV2A_RAMDAC_OFFSET             0x680000
#define   NV2A_RAMDAC_FP_HVALID_END      (0x838 + NV2A_RAMDAC_OFFSET)
#define   NV2A_RAMDAC_FP_VVALID_END      (0x818 + NV2A_RAMDAC_OFFSET)

#endif /* _XGPU_H_ */
