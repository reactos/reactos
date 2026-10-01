/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Snapin alias class
 * COPYRIGHT:   Copyright 2026 Eric Kohl (eric.kohl@reactos.org)
 */

#pragma once

class CSnapinAlias
{
private:
    CSnapinAlias *m_ParentAlias;
    CSnapin *m_Snapin;
    BOOL m_Deleted;

public:
    CAtlList<CSnapinAlias*> m_SubNodes;
    HTREEITEM hTreeItem;

    CSnapinAlias(CSnapinAlias *ParentAlias, CSnapin *Snapin);
    ~CSnapinAlias();

    CSnapinAlias *Parent();
    CSnapin *Snapin();
    BOOL IsDeleted();
    VOID Deleted();
};
