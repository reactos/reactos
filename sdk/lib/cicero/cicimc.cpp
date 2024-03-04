/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Locking and Unlocking IMC and IMCC handles
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include <imm.h>
#include <immdev.h>
#include <imm32_undoc.h>
#include "cicimc.h"

#define CUSTOM_CAND_INFO_SIZE 1964

BOOL CicIMCLock::ClearCand()
{
    HIMCC hNewCandInfo, hCandInfo = m_pIC->hCandInfo;
    if (hCandInfo)
    {
        hNewCandInfo = ImmReSizeIMCC(hCandInfo, CUSTOM_CAND_INFO_SIZE);
        if (!hNewCandInfo)
        {
            ImmDestroyIMCC(m_pIC->hCandInfo);
            m_pIC->hCandInfo = ImmCreateIMCC(CUSTOM_CAND_INFO_SIZE);
            return FALSE;
        }
    }
    else
    {
        hNewCandInfo = ImmCreateIMCC(CUSTOM_CAND_INFO_SIZE);
    }

    m_pIC->hCandInfo = hNewCandInfo;
    if (!m_pIC->hCandInfo)
        return FALSE;

    CicIMCCLock<CANDIDATEINFO> candInfo(m_pIC->hCandInfo);
    if (!candInfo)
    {
        ImmDestroyIMCC(m_pIC->hCandInfo);
        m_pIC->hCandInfo = ImmCreateIMCC(CUSTOM_CAND_INFO_SIZE);
        return FALSE;
    }

    candInfo.get().dwSize = CUSTOM_CAND_INFO_SIZE;
    candInfo.get().dwCount = 0;
    candInfo.get().dwOffset[0] = sizeof(CANDIDATEINFO);

    // FIXME: Something is trailing after CANDIDATEINFO...

    return TRUE;
}
