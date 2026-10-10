/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Snapin alias class
 * COPYRIGHT:   Copyright 2026 Eric Kohl (eric.kohl@reactos.org)
 */

#include "precomp.h"


CSnapinAlias::CSnapinAlias(CSnapinAlias *ParentAlias, CSnapin *Snapin)
{
    m_ParentAlias = ParentAlias;
    m_Snapin = Snapin;
    m_Deleted = FALSE;
}

CSnapinAlias::~CSnapinAlias()
{
    m_ParentAlias = NULL;
    m_Snapin = NULL;
}

CSnapinAlias *
CSnapinAlias::Parent()
{
    return m_ParentAlias;
}

CSnapin *
CSnapinAlias::Snapin()
{
    return m_Snapin;
}

BOOL
CSnapinAlias::IsDeleted()
{
    return m_Deleted;
}

VOID
CSnapinAlias::Deleted()
{
    m_Deleted = TRUE;
}
