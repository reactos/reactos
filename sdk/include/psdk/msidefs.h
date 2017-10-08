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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_MSIDEFS_H
#define __WINE_MSIDEFS_H

enum msidbUpgradeAttributes {
    msidbUpgradeAttributesMigrateFeatures = 0x0000001,
    msidbUpgradeAttributesOnlyDetect = 0x00000002,
    msidbUpgradeAttributesIgnoreRemoveFailure = 0x00000004,
    msidbUpgradeAttributesVersionMinInclusive = 0x00000100,
    msidbUpgradeAttributesVersionMaxInclusive = 0x00000200,
    msidbUpgradeAttributesLanguagesExclusive = 0x00000400
};

enum msidbFileAttributes {
    msidbFileAttributesReadOnly = 0x00000001,
    msidbFileAttributesHidden = 0x00000002,
    msidbFileAttributesSystem = 0x00000004,
    msidbFileAttributesVital = 0x00000200,
    msidbFileAttributesChecksum = 0x00000400,
    msidbFileAttributesPatchAdded = 0x00001000,
    msidbFileAttributesNoncompressed = 0x00002000,
    msidbFileAttributesCompressed = 0x00004000
};

enum msidbPatchAttributes {
    msidbPatchAttributesNonVital = 0x00000001
};

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

enum msidbControlAttributes {
    msidbControlAttributesVisible = 0x00000001,
    msidbControlAttributesEnabled = 0x00000002,
    msidbControlAttributesSunken = 0x00000004,
    msidbControlAttributesIndirect = 0x00000008,
    msidbControlAttributesInteger = 0x00000010,
    msidbControlAttributesRTLRO = 0x00000020,
    msidbControlAttributesRightAligned = 0x00000040,
    msidbControlAttributesLeftScroll = 0x00000080,
    msidbControlAttributesBiDi = 0x000000c0,

    msidbControlAttributesTransparent = 0x00010000,
    msidbControlAttributesNoPrefix = 0x00020000,
    msidbControlAttributesNoWrap = 0x00040000,
    msidbControlAttributesFormatSize = 0x00080000,
    msidbControlAttributesUsersLanguage = 0x00100000,

    msidbControlAttributesMultiline = 0x00010000,
    msidbControlAttributesPasswordInput = 0x00200000,

    msidbControlAttributesProgress95 = 0x00010000,

    msidbControlAttributesRemovableVolume = 0x00010000,
    msidbControlAttributesFixedVolume = 0x00020000,
    msidbControlAttributesRemoteVolume = 0x00040000,
    msidbControlAttributesCDROMVolume = 0x00080000,
    msidbControlAttributesRAMdiskVolume = 0x00100000,
    msidbControlAttributesFloppyVolume = 0x00200000,
    msidbControlShowRollbackCost = 0x00400000,

    msidbControlAttributesSorted = 0x00010000,
    msidbControlAttributesComboList = 0x00020000,

    msidbControlAttributesImageHandle = 0x00010000,
    msidbControlAttributesPushLike = 0x00020000,
    msidbControlAttributesBitmap = 0x00040000,
    msidbControlAttributesIcon = 0x00080000,
    msidbControlAttributesFixedSize = 0x00100000,
    msidbControlAttributesIconSize16 = 0x00200000,
    msidbControlAttributesIconSize32 = 0x00400000,
    msidbControlAttributesIconSize48 = 0x00600000,

    msidbControlAttributesHasBorder = 0x01000000,
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

enum msidbComponentAttributes
{
    msidbComponentAttributesLocalOnly = 0x00000000,
    msidbComponentAttributesSourceOnly = 0x00000001,
    msidbComponentAttributesOptional = 0x00000002,
    msidbComponentAttributesRegistryKeyPath = 0x00000004,
    msidbComponentAttributesSharedDllRefCount = 0x00000008,
    msidbComponentAttributesPermanent = 0x00000010,
    msidbComponentAttributesODBCDataSource = 0x00000020,
    msidbComponentAttributesTransitive = 0x00000040,
    msidbComponentAttributesNeverOverwrite = 0x00000080,
    msidbComponentAttributes64bit = 0x00000100
};

enum msidbODBCDataSourceRegistration
{
    msidbODBCDataSourceRegistrationPerMachine = 0x00000000,
    msidbODBCDataSourceRegistrationPerUser = 0x00000001
};

enum msidbRegistryRoot
{
    msidbRegistryRootClassesRoot = 0,
    msidbRegistryRootCurrentUser = 1,
    msidbRegistryRootLocalMachine = 2,
    msidbRegistryRootUsers = 3,
};

enum msidbLocatorType
{
    msidbLocatorTypeDirectory = 0x000,
    msidbLocatorTypeFileName = 0x001,
    msidbLocatorTypeRawValue = 0x002,
    msidbLocatorType64bit = 0x010,
};

enum msidbServiceControlEvent
{
    msidbServiceControlEventStart = 0x00000001,
    msidbServiceControlEventStop = 0x00000002,
    msidbServiceControlEventDelete = 0x00000008,
    msidbServiceControlEventUninstallStart = 0x00000010,
    msidbServiceControlEventUninstallStop = 0x00000020,
    msidbServiceControlEventUninstallDelete = 0x00000080,
};

enum msidbServiceInstallErrorControl
{
    msidbServiceInstallErrorControlVital = 0x00008000
};

enum msidbMoveFileOptions
{
    msidbMoveFileOptionsMove = 0x00000001,
};

enum msidbAssemblyAttributes
{
    msidbAssemblyAttributesURT = 0x00000000,
    msidbAssemblyAttributesWin32 = 0x00000001,
};

enum msidbSumInfoSourceType
{
    msidbSumInfoSourceTypeSFN = 0x00000001,
    msidbSumInfoSourceTypeCompressed = 0x00000002,
    msidbSumInfoSourceTypeAdminImage = 0x00000004,
    msidbSumInfoSourceTypeLUAPackage = 0x00000008,
};

enum msidbRemoveFileInstallMode
{
    msidbRemoveFileInstallModeOnInstall = 0x00000001,
    msidbRemoveFileInstallModeOnRemove = 0x00000002,
    msidbRemoveFileInstallModeOnBoth = 0x00000003,
};

enum
{
    msidbIniFileActionAddLine    = 0x00000000,
    msidbIniFileActionCreateLine = 0x00000001,
    msidbIniFileActionRemoveLine = 0x00000002,
    msidbIniFileActionAddTag     = 0x00000003,
    msidbIniFileActionRemoveTag  = 0x00000004
};

/*
 * Windows SDK braindamage alert
 *
 * PID_DICTIONARY and PID_CODEPAGE are defined by propidl.h too
 * PID_SECURITY is defined in propidl.h with a different value!
 * So these need to be undefined first.
 */
#ifdef PID_DICTIONARY
#undef PID_DICTIONARY
#endif

#ifdef PID_CODEPAGE
#undef PID_CODEPAGE
#endif

#ifdef PID_SECURITY
#undef PID_SECURITY
#endif

#define PID_DICTIONARY 0
#define PID_CODEPAGE 1
#define PID_TITLE 2
#define PID_SUBJECT 3
#define PID_AUTHOR 4
#define PID_KEYWORDS 5
#define PID_COMMENTS 6
#define PID_TEMPLATE 7
#define PID_LASTAUTHOR 8
#define PID_REVNUMBER 9
#define PID_EDITTIME 10
#define PID_LASTPRINTED 11
#define PID_CREATE_DTM 12
#define PID_LASTSAVE_DTM 13
#define PID_PAGECOUNT 14
#define PID_WORDCOUNT 15
#define PID_CHARCOUNT 16
#define PID_THUMBNAIL 17
#define PID_APPNAME 18
#define PID_SECURITY 19
#define PID_MSIVERSION PID_PAGECOUNT
#define PID_MSISOURCE PID_WORDCOUNT
#define PID_MSIRESTRICT PID_CHARCOUNT

#endif /* __WINE_MSIDEFS_H */
