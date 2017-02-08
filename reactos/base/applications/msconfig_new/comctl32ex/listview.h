/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig_new/comctl32ex/listview.h
 * PURPOSE:     List-View helper functions.
 * COPYRIGHT:   Copyright 2011-2012 Hermes BELUSCA - MAITO <hermes.belusca@sfr.fr>
 */

#ifndef __LISTVIEW_H__
#define __LISTVIEW_H__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "comctl32supp.h"

/////////////  ListView Sorting  /////////////

int CALLBACK
SortListView(LPARAM lItemParam1,
             LPARAM lItemParam2,
             LPARAM lPSort_S);

BOOL
ListView_SortEx(HWND hListView,
                int iSortingColumn,
                int iSortedColumn);

#define ListView_Sort(hListView, iSortingColumn) \
    ListView_SortEx((hListView), (iSortingColumn), -1)

//////////////////////////////////////////////

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __LISTVIEW_H__

/* EOF */
