/*
 * Copyright (C) 2005 Mike McCormack
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

#ifndef __WINE_MSIDEFS_H
#define __WINE_MSIDEFS_H

#ifdef __cplusplus
extern "C" {
#endif

enum msidbDialogAttributes {
    msidbDialogAttributesVisible = 0x00000001,
    msidbDialogAttributesModal = 0x00000002,
    msidbDialogAttributesMinimize = 0x00000004,
    msidbDialogAttributesSysModal = 0x00000008,
    msidbDialogAttributesKeepModeless = 0x00000010,
    msidbDialogAttributesTrackDiskSpace = 0x00000020,
    msidbDialogAttributesUseCustomPalette = 0x00000040,
    msidbDialogAttributesRTLRO = 0x00000080,
    msidbDialogAttributesRightAligned = 0x00000100,
    msidbDialogAttributesLeftScroll = 0x00000200,
    msidbDialogAttributesBidi = 0x00000380,
    msidbDialogAttributesError = 0x00010000
};

enum msidbTextStyleStyleBits
{
    msidbTextStyleStyleBitsBold = 0x00000001,
    msidbTextStyleStyleBitsItalic = 0x00000002,
    msidbTextStyleStyleBitsUnderline = 0x00000004,
    msidbTextStyleStyleBitsStrike = 0x00000008,
};

enum msidbCustomActionType
{
    msidbCustomActionTypeDll = 0x00000001,
    msidbCustomActionTypeExe = 0x00000002,
    msidbCustomActionTypeTextData = 0x00000003,
    msidbCustomActionTypeJScript = 0x00000005,
    msidbCustomActionTypeVBScript = 0x00000006,
    msidbCustomActionTypeInstall = 0x00000007,

    msidbCustomActionTypeBinaryData = 0x00000000,
    msidbCustomActionTypeSourceFile = 0x00000010,
    msidbCustomActionTypeDirectory = 0x00000020,
    msidbCustomActionTypeProperty = 0x00000030,

    msidbCustomActionTypeContinue = 0x00000040,
    msidbCustomActionTypeAsync = 0x00000080,

    msidbCustomActionTypeFirstSequence = 0x00000100,
    msidbCustomActionTypeOncePerProcess = 0x00000200,
    msidbCustomActionTypeClientRepeat = 0x00000300,
    msidbCustomActionTypeInScript = 0x00000400,

    msidbCustomActionTypeRollback = 0x00000100,
    msidbCustomActionTypeCommit = 0x00000200,

    msidbCustomActionTypeNoImpersonate = 0x00000800,
    msidbCustomActionTypeTSAware = 0x00004000,

    msidbCustomActionType64BitScript = 0x00001000,
    msidbCustomActionTypeHideTarget = 0x00002000
};

enum msidbFeatureAttributes
{
    msidbFeatureAttributesFavorLocal = 0x00000000,
    msidbFeatureAttributesFavorSource = 0x00000001,
    msidbFeatureAttributesFollowParent = 0x00000002,
    msidbFeatureAttributesFavorAdvertise = 0x00000004,
    msidbFeatureAttributesDisallowAdvertise = 0x00000008,
    msidbFeatureAttributesUIDisallowAbsent = 0x00000010,
    msidbFeatureAttributesNoUnsupportedAdvertise = 0x00000020
};

#ifdef __cplusplus
}
#endif

#endif /* __WINE_MSIDEFS_H */
