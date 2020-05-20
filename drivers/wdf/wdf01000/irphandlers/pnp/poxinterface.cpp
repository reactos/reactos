/*
 * PROJECT:     ReactOS Wdf01000 driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Power-framework-related logic in WDF.
 * COPYRIGHT:   Copyright 2020 mrmks04 (mrmks04@yandex.ru)
 */


#include "common/fxpoxinterface.h"


FxPoxInterface::FxPoxInterface(
    __in FxPkgPnp* PkgPnp
    )
{
    m_PkgPnp = PkgPnp;
    //m_PoHandle = NULL;
    m_DevicePowerRequired = TRUE;
    m_DevicePowerRequirementMachine = NULL;
    m_CurrentIdleTimeoutHint = 0;
    m_NextIdleTimeoutHint = 0;
}

FxPoxInterface::~FxPoxInterface(
    VOID
    )
{
    if (NULL != m_DevicePowerRequirementMachine)
    {
        delete m_DevicePowerRequirementMachine;
    }
}