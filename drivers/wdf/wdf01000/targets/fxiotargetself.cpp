/*
 * PROJECT:     ReactOS Wdf01000 driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Framework IO target api implementation
 * COPYRIGHT:   Copyright 2020 mrmks04 (mrmks04@yandex.ru)
 */


#include "common/fxiotargetself.h"


FxIoTargetSelf::FxIoTargetSelf(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ USHORT ObjectSize
    ) :
    FxIoTarget(FxDriverGlobals, ObjectSize, FX_TYPE_IO_TARGET_SELF),
    m_DispatchQueue(NULL)
{
}

FxIoTargetSelf::~FxIoTargetSelf()
{
}