/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxPkgIoShared.hpp

Abstract:

    This file contains portion of FxPkgIo.hpp that is need in shared code.

Author:

Environment:

Revision History:



--*/

#ifndef _FXPKGIOSHARED_HPP_
#define _FXPKGIOSHARED_HPP_

enum FxIoStopProcessingForPowerAction {
    FxIoStopProcessingForPowerHold = 1,
    FxIoStopProcessingForPowerPurgeManaged,
    FxIoStopProcessingForPowerPurgeNonManaged,
};

// begin_wpp config
// CUSTOM_TYPE(FxIoStopProcessingForPowerAction, ItemEnum(FxIoStopProcessingForPowerAction));
// end_wpp

#endif // _FXPKGIOSHARED_HPP
