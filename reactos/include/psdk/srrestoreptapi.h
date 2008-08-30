/*
 * Definitions for the System File Checker (Windows File Protection)
 *
 * Copyright 2008 Pierre Schweitzer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef _SRRESTOREPTAPI_H
#define _SRRESTOREPTAPI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Defines */

#define MAX_DESC    64
#define MAX_DESC_W  256

/* Types */

typedef struct _RESTOREPTINFOA {
  DWORD  dwEventType;
  DWORD  dwRestorePtType;
  INT64  llSequenceNumber;
  CHAR   szDescription[MAX_DESC];
} RESTOREPOINTINFOA, *PRESTOREPOINTINFOA;

typedef struct _RESTOREPTINFOW {
    DWORD  dwEventType;       
    DWORD  dwRestorePtType;   
    INT64  llSequenceNumber;  
    WCHAR  szDescription[MAX_DESC_W]; 
} RESTOREPOINTINFOW, *PRESTOREPOINTINFOW;

typedef struct _SMGRSTATUS {
    DWORD  nStatus;
    INT64  llSequenceNumber;
} STATEMGRSTATUS, *PSTATEMGRSTATUS;

/* Functions */

BOOL WINAPI SRSetRestorePointA(PRESTOREPOINTINFOA, PSTATEMGRSTATUS);
BOOL WINAPI SRSetRestorePointW(PRESTOREPOINTINFOW, PSTATEMGRSTATUS);
DWORD WINAPI SRRemoveRestorePoint(DWORD);
                  
#ifdef __cplusplus
}
#endif

#endif
