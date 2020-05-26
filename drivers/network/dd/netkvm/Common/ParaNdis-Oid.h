/*
 * This file contains common for NDIS5/NDIS6 definition,
 * related to OID support
 *
 * Copyright (c) 2008-2017 Red Hat, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met :
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and / or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of their contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef PARANDIS_COMMON_OID_H
#define PARANDIS_COMMON_OID_H

#include "ndis56common.h"

/**********************************************************
Wrapper for all the data, related to any OID request
***********************************************************/
typedef struct _tagOidDesc
{
    NDIS_OID                Oid;                        // oid code
    ULONG                   ulToDoFlags;                // combination of eOidHelperFlags
    PVOID                   InformationBuffer;          // buffer received from NDIS
    UINT                    InformationBufferLength;    // its length
    PUINT                   pBytesWritten;              // OUT for query/method
    PUINT                   pBytesNeeded;               // OUT for query/set/method when length of buffer is wrong
    PUINT                   pBytesRead;                 // OUT for set/method
    PVOID                   Reserved;                   // Reserved for pending requests
} tOidDesc;

typedef NDIS_STATUS (*OIDHANDLERPROC)(PARANDIS_ADAPTER *pContext, tOidDesc *pOid);

typedef struct _tagOidWhatToDo
{
    NDIS_OID            oid;                // oid number
    int                 nEntryLevel;        // do print on entry level
    int                 nExitFailLevel;     // do print on exit if failed
    int                 nExitOKLevel;       // do print on exit if OK
    UINT                Flags;
    OIDHANDLERPROC      OidSetProc;         // procedure to call on SET
    const char          *name;              // printable name
}tOidWhatToDo;


typedef enum _tageOidHelperFlags {
    ohfQuery            = 1,                        // can be queried
    ohfSet              = 2,                        // can be set
    ohfQuerySet         = ohfQuery | ohfSet,
    ohfQueryStatOnly    = 4,                        // redirect query stat to query
    ohfQueryStat        = ohfQueryStatOnly | ohfQuery,
    ohfQuery3264        = 8 | ohfQuery,             // convert 64 to 32 on query
    ohfQueryStat3264    = 8 | ohfQueryStat,         // convert 64 to 32 on query stat
    ohfSetLessOK        = 16,                       // on set: if buffer is smaller, cleanup and copy
    ohfSetMoreOK        = 32                        // on set: if buffer is larger, copy anyway
} eOidHelperFlags;




/**********************************************************
Common procedures related to OID support
***********************************************************/
NDIS_STATUS ParaNdis_OidQueryCommon(PARANDIS_ADAPTER *pContext, tOidDesc *pOid);
NDIS_STATUS ParaNdis_OidQueryCopy(tOidDesc *pOid, PVOID pInfo, ULONG ulSize, BOOLEAN bFreeInfo);
static NDIS_STATUS ParaNdis_OidQuery(PARANDIS_ADAPTER *pContext, tOidDesc *pOid);
NDIS_STATUS ParaNdis_OnOidSetMulticastList(PARANDIS_ADAPTER *pContext, tOidDesc *pOid);
NDIS_STATUS ParaNdis_OnSetPacketFilter(PARANDIS_ADAPTER *pContext, tOidDesc *pOid);
NDIS_STATUS ParaNdis_OnAddWakeupPattern(PARANDIS_ADAPTER *pContext, tOidDesc *pOid);
NDIS_STATUS ParaNdis_OnRemoveWakeupPattern(PARANDIS_ADAPTER *pContext, tOidDesc *pOid);
NDIS_STATUS ParaNdis_OnEnableWakeup(PARANDIS_ADAPTER *pContext, tOidDesc *pOid);
NDIS_STATUS ParaNdis_OnSetLookahead(PARANDIS_ADAPTER *pContext, tOidDesc *pOid);
NDIS_STATUS ParaNdis_OnSetVlanId(PARANDIS_ADAPTER *pContext, tOidDesc *pOid);
NDIS_STATUS ParaNdis_OidSetCopy(tOidDesc *pOid, PVOID pDest, ULONG ulSize);
void ParaNdis_FillPowerCapabilities(PNDIS_PNP_CAPABILITIES pCaps);
void ParaNdis_GetOidSupportRules(NDIS_OID oid, tOidWhatToDo *pRule, const tOidWhatToDo *Table);


const char *ParaNdis_OidName(NDIS_OID oid);
/**********************************************************
Procedures to be implemented in NDIS5/NDIS6 specific modules
***********************************************************/
void ParaNdis_GetSupportedOid(PVOID *pOidsArray, PULONG pLength);
NDIS_STATUS ParaNdis_OnSetPower(PARANDIS_ADAPTER *pContext, tOidDesc *pOid);

#endif
