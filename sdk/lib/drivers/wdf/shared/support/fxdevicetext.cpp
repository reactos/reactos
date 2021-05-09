/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDeviceText.cpp

Abstract:

    This module implements the device text object.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#include "fxsupportpch.hpp"

FxDeviceText::FxDeviceText(
    VOID
    ) :
    m_Description(NULL),
    m_LocationInformation(NULL),
    m_LocaleId(0)
{
    m_Entry.Next = NULL;
}

FxDeviceText::~FxDeviceText()
{
    ASSERT(Mx::MxGetCurrentIrql() == PASSIVE_LEVEL);

    ASSERT(m_Entry.Next == NULL);

    if (m_Description != NULL) {
        FxPoolFree(m_Description);
        m_Description = NULL;
    }

    if (m_LocationInformation != NULL) {
        FxPoolFree(m_LocationInformation);
        m_LocationInformation = NULL;
    }
}
