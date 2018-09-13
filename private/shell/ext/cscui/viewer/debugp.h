//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       debugp.h
//
//--------------------------------------------------------------------------

#ifndef _INC_CSCVIEW_DEBUGP_H
#define _INC_CSCVIEW_DEBUGP_H

//
// Define some application-specific debug mask values.
//
#define DM_LOADMGR      DBGCREATEMASK(0x00000001)
#define DM_LOADWORKER   DBGCREATEMASK(0x00000002)
#define DM_OBJTREE      DBGCREATEMASK(0x00000004)
#define DM_VIEW         DBGCREATEMASK(0x00000008)
#define DM_ICONHOUND    DBGCREATEMASK(0x00000010)
#define DM_CONFIG       DBGCREATEMASK(0x00000020)
#define DM_CPL          DBGCREATEMASK(0x00000040)
#define DM_OPTDLG       DBGCREATEMASK(0x00000080)


#endif // _INC_CSCVIEW_DEBUGP_H

