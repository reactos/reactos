/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig_new/comctl32ex/listviewfuncs.h
 * PURPOSE:     List-View helper functions.
 * COPYRIGHT:   Copyright 2011-2012 Hermes BELUSCA - MAITO <hermes.belusca@sfr.fr>
 */

#ifndef __LISTVIEWFUNCS_H__
#define __LISTVIEWFUNCS_H__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "commctrldefs.h"

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

#endif // __LISTVIEWFUNCS_H__

/* EOF */
