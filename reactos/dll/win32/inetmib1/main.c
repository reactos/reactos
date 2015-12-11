/*
 * Copyright 2008 Juan Lang
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

#define WIN32_NO_STATUS

#include <config.h>
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
//#include <limits.h>

#define NONAMELESSUNION

#include <windef.h>
#include <winbase.h>
#include <snmp.h>
#include <iphlpapi.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(inetmib1);

/**
 * Utility functions
 */
static DWORD copyInt(AsnAny *value, void *src)
{
    value->asnType = ASN_INTEGER;
    value->asnValue.number = *(DWORD *)src;
    return SNMP_ERRORSTATUS_NOERROR;
}

static void setStringValue(AsnAny *value, BYTE type, DWORD len, BYTE *str)
{
    AsnAny strValue;

    strValue.asnType = type;
    strValue.asnValue.string.stream = str;
    strValue.asnValue.string.length = len;
    strValue.asnValue.string.dynamic = FALSE;
    SnmpUtilAsnAnyCpy(value, &strValue);
}

typedef DWORD (*copyValueFunc)(AsnAny *value, void *src);

struct structToAsnValue
{
    size_t        offset;
    copyValueFunc copy;
};

static AsnInteger32 mapStructEntryToValue(struct structToAsnValue *map,
    UINT mapLen, void *record, UINT id, SnmpVarBind *pVarBind)
{
    /* OIDs are 1-based */
    if (!id)
        return SNMP_ERRORSTATUS_NOSUCHNAME;
    --id;
    if (id >= mapLen)
        return SNMP_ERRORSTATUS_NOSUCHNAME;
    if (!map[id].copy)
        return SNMP_ERRORSTATUS_NOSUCHNAME;
    return map[id].copy(&pVarBind->value, (BYTE *)record + map[id].offset);
}

static DWORD copyIpAddr(AsnAny *value, void *src)
{
    setStringValue(value, ASN_IPADDRESS, sizeof(DWORD), src);
    return SNMP_ERRORSTATUS_NOERROR;
}

static UINT mib2[] = { 1,3,6,1,2,1 };
static UINT mib2System[] = { 1,3,6,1,2,1,1 };

typedef BOOL (*varqueryfunc)(BYTE bPduType, SnmpVarBind *pVarBind,
    AsnInteger32 *pErrorStatus);

struct mibImplementation
{
    AsnObjectIdentifier name;
    void              (*init)(void);
    varqueryfunc        query;
    void              (*cleanup)(void);
};

static UINT mib2IfNumber[] = { 1,3,6,1,2,1,2,1 };
static PMIB_IFTABLE ifTable;

static void mib2IfNumberInit(void)
{
    DWORD size = 0, ret = GetIfTable(NULL, &size, FALSE);

    if (ret == ERROR_INSUFFICIENT_BUFFER)
    {
        MIB_IFTABLE *table = HeapAlloc(GetProcessHeap(), 0, size);
        if (table)
        {
            if (!GetIfTable(table, &size, FALSE)) ifTable = table;
            else HeapFree(GetProcessHeap(), 0, table );
        }
    }
}

static void mib2IfNumberCleanup(void)
{
    HeapFree(GetProcessHeap(), 0, ifTable);
}

static BOOL mib2IfNumberQuery(BYTE bPduType, SnmpVarBind *pVarBind,
    AsnInteger32 *pErrorStatus)
{
    AsnObjectIdentifier numberOid = DEFINE_OID(mib2IfNumber);
    BOOL ret = TRUE;

    TRACE("(0x%02x, %s, %p)\n", bPduType, SnmpUtilOidToA(&pVarBind->name),
        pErrorStatus);

    switch (bPduType)
    {
    case SNMP_PDU_GET:
    case SNMP_PDU_GETNEXT:
        if ((bPduType == SNMP_PDU_GET &&
            !SnmpUtilOidNCmp(&pVarBind->name, &numberOid, numberOid.idLength))
            || SnmpUtilOidNCmp(&pVarBind->name, &numberOid, numberOid.idLength)
            < 0)
        {
            DWORD numIfs = ifTable ? ifTable->dwNumEntries : 0;

            copyInt(&pVarBind->value, &numIfs);
            if (bPduType == SNMP_PDU_GETNEXT)
            {
                SnmpUtilOidFree(&pVarBind->name);
                SnmpUtilOidCpy(&pVarBind->name, &numberOid);
            }
            *pErrorStatus = SNMP_ERRORSTATUS_NOERROR;
        }
        else
        {
            *pErrorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
            /* Caller deals with OID if bPduType == SNMP_PDU_GETNEXT, so don't
             * need to set it here.
             */
        }
        break;
    case SNMP_PDU_SET:
        *pErrorStatus = SNMP_ERRORSTATUS_READONLY;
        ret = FALSE;
        break;
    default:
        FIXME("0x%02x: unsupported PDU type\n", bPduType);
        *pErrorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
    }
    return ret;
}

static DWORD copyOperStatus(AsnAny *value, void *src)
{
    value->asnType = ASN_INTEGER;
    /* The IPHlpApi definition of operational status differs from the MIB2 one,
     * so map it to the MIB2 value.
     */
    switch (*(DWORD *)src)
    {
    case MIB_IF_OPER_STATUS_OPERATIONAL:
        value->asnValue.number = MIB_IF_ADMIN_STATUS_UP;
        break;
    case MIB_IF_OPER_STATUS_CONNECTING:
    case MIB_IF_OPER_STATUS_CONNECTED:
        value->asnValue.number = MIB_IF_ADMIN_STATUS_TESTING;
        break;
    default:
        value->asnValue.number = MIB_IF_ADMIN_STATUS_DOWN;
    };
    return SNMP_ERRORSTATUS_NOERROR;
}

/* Given an OID and a base OID that it must begin with, finds the item and
 * integer instance from the OID.  E.g., given an OID foo.1.2 and a base OID
 * foo, returns item 1 and instance 2.
 * If bPduType is not SNMP_PDU_GETNEXT and either the item or instance is
 * missing, returns SNMP_ERRORSTATUS_NOSUCHNAME.
 * If bPduType is SNMP_PDU_GETNEXT, returns the successor to the item and
 * instance, or item 1, instance 1 if either is missing.
 */
static AsnInteger32 getItemAndIntegerInstanceFromOid(AsnObjectIdentifier *oid,
    AsnObjectIdentifier *base, BYTE bPduType, UINT *item, UINT *instance)
{
    AsnInteger32 ret = SNMP_ERRORSTATUS_NOERROR;

    switch (bPduType)
    {
    case SNMP_PDU_GETNEXT:
        if (SnmpUtilOidNCmp(oid, base, base->idLength) < 0)
        {
            *item = 1;
            *instance = 1;
        }
        else if (!SnmpUtilOidNCmp(oid, base, base->idLength))
        {
            if (oid->idLength == base->idLength ||
                oid->idLength == base->idLength + 1)
            {
                /* Either the table or an item within the table is specified,
                 * but the instance is not.  Get the first instance.
                 */
                *instance = 1;
                if (oid->idLength == base->idLength + 1)
                    *item = oid->ids[base->idLength];
                else
                    *item = 1;
            }
            else
            {
                *item = oid->ids[base->idLength];
                *instance = oid->ids[base->idLength + 1] + 1;
            }
        }
        else
            ret = SNMP_ERRORSTATUS_NOSUCHNAME;
        break;
    default:
        if (!SnmpUtilOidNCmp(oid, base, base->idLength))
        {
            if (oid->idLength == base->idLength ||
                oid->idLength == base->idLength + 1)
            {
                /* Either the table or an item within the table is specified,
                 * but the instance is not.
                 */
                ret = SNMP_ERRORSTATUS_NOSUCHNAME;
            }
            else
            {
                *item = oid->ids[base->idLength];
                *instance = oid->ids[base->idLength + 1];
            }
        }
        else
            ret = SNMP_ERRORSTATUS_NOSUCHNAME;
    }
    return ret;
}

/* Given an OID and a base OID that it must begin with, finds the item from the
 * OID.  E.g., given an OID foo.1 and a base OID foo, returns item 1.
 * If bPduType is not SNMP_PDU_GETNEXT and the item is missing, returns
 * SNMP_ERRORSTATUS_NOSUCHNAME.
 * If bPduType is SNMP_PDU_GETNEXT, returns the successor to the item, or item
 * 1 if the item is missing.
 */
static AsnInteger32 getItemFromOid(AsnObjectIdentifier *oid,
    AsnObjectIdentifier *base, BYTE bPduType, UINT *item)
{
    AsnInteger32 ret = SNMP_ERRORSTATUS_NOERROR;

    switch (bPduType)
    {
    case SNMP_PDU_GETNEXT:
        if (SnmpUtilOidNCmp(oid, base, base->idLength) < 0)
            *item = 1;
        else if (!SnmpUtilOidNCmp(oid, base, base->idLength))
        {
            if (oid->idLength == base->idLength)
            {
                /* The item is missing, assume the first item */
                *item = 1;
            }
            else
                *item = oid->ids[base->idLength] + 1;
        }
        else
            ret = SNMP_ERRORSTATUS_NOSUCHNAME;
        break;
    default:
        if (!SnmpUtilOidNCmp(oid, base, base->idLength))
        {
            if (oid->idLength == base->idLength)
            {
                /* The item is missing */
                ret = SNMP_ERRORSTATUS_NOSUCHNAME;
            }
            else
            {
                *item = oid->ids[base->idLength];
                if (!*item)
                    ret = SNMP_ERRORSTATUS_NOSUCHNAME;
            }
        }
        else
            ret = SNMP_ERRORSTATUS_NOSUCHNAME;
    }
    return ret;
}

struct GenericTable
{
    DWORD numEntries;
    BYTE  entries[1];
};

static DWORD oidToIpAddr(AsnObjectIdentifier *oid)
{
    assert(oid && oid->idLength >= 4);
    /* Map the IDs to an IP address in little-endian order */
    return (BYTE)oid->ids[3] << 24 | (BYTE)oid->ids[2] << 16 |
        (BYTE)oid->ids[1] << 8 | (BYTE)oid->ids[0];
}

typedef void (*oidToKeyFunc)(AsnObjectIdentifier *oid, void *dst);
typedef int (*compareFunc)(const void *key, const void *value);

/* Finds the first value in the table that matches key.  Returns its 1-based
 * index if found, or 0 if not found.
 */
static UINT findValueInTable(const void *key,
    struct GenericTable *table, size_t tableEntrySize, compareFunc compare)
{
    UINT index = 0;
    void *value;

    value = bsearch(key, table->entries, table->numEntries, tableEntrySize,
        compare);
    if (value)
        index = ((BYTE *)value - (BYTE *)table->entries) / tableEntrySize + 1;
    return index;
}

/* Finds the first value in the table that matches oid, using makeKey to
 * convert the oid to a key for comparison.  Returns the value's 1-based
 * index if found, or 0 if not found.
 */
static UINT findOidInTable(AsnObjectIdentifier *oid,
    struct GenericTable *table, size_t tableEntrySize, oidToKeyFunc makeKey,
    compareFunc compare)
{
    UINT index = 0;
    void *key = HeapAlloc(GetProcessHeap(), 0, tableEntrySize);

    if (key)
    {
        makeKey(oid, key);
        index = findValueInTable(key, table, tableEntrySize, compare);
        HeapFree(GetProcessHeap(), 0, key);
    }
    return index;
}

/* Finds the first successor to the value in the table that does matches oid,
 * using makeKey to convert the oid to a key for comparison.  A successor is
 * a value that does not match oid, so if multiple entries match an oid, only
 * the first will ever be returned using this method.
 * Returns the successor's 1-based index if found, or 0 if not found.
 */
static UINT findNextOidInTable(AsnObjectIdentifier *oid,
    struct GenericTable *table, size_t tableEntrySize, oidToKeyFunc makeKey,
    compareFunc compare)
{
    UINT index = 0;
    void *key = HeapAlloc(GetProcessHeap(), 0, tableEntrySize);

    if (key)
    {
        makeKey(oid, key);
        index = findValueInTable(key, table, tableEntrySize, compare);
        if (index == 0)
        {
            /* Not found in table.  If it's less than the first entry, return
             * the first index.  Otherwise just return 0 and let the caller
             * handle finding the successor.
             */
            if (compare(key, table->entries) < 0)
                index = 1;
        }
        else
        {
            /* Skip any entries that match the same key.  This enumeration will
             * be incomplete, but it's what Windows appears to do if there are
             * multiple entries with the same index in a table, and it avoids
             * an infinite loop.
             */
            for (++index; index <= table->numEntries && compare(key,
                &table->entries[tableEntrySize * (index - 1)]) == 0; ++index)
                ;
        }
        HeapFree(GetProcessHeap(), 0, key);
    }
    return index;
}

/* Given an OID and a base OID that it must begin with, finds the item and
 * element of the table whose value matches the instance from the OID.
 * The OID is converted to a key with the function makeKey, and compared
 * against entries in the table with the function compare.
 * If bPduType is not SNMP_PDU_GETNEXT and either the item or instance is
 * missing, returns SNMP_ERRORSTATUS_NOSUCHNAME.
 * If bPduType is SNMP_PDU_GETNEXT, returns the successor to the item and
 * instance, or item 1, instance 1 if either is missing.
 */
static AsnInteger32 getItemAndInstanceFromTable(AsnObjectIdentifier *oid,
    AsnObjectIdentifier *base, UINT instanceLen, BYTE bPduType,
    struct GenericTable *table, size_t tableEntrySize, oidToKeyFunc makeKey,
    compareFunc compare, UINT *item, UINT *instance)
{
    AsnInteger32 ret = SNMP_ERRORSTATUS_NOERROR;

    if (!table)
        return SNMP_ERRORSTATUS_NOSUCHNAME;

    switch (bPduType)
    {
    case SNMP_PDU_GETNEXT:
        if (SnmpUtilOidNCmp(oid, base, base->idLength) < 0)
        {
            /* Return the first item and instance from the table */
            *item = 1;
            *instance = 1;
        }
        else if (!SnmpUtilOidNCmp(oid, base, base->idLength) &&
            oid->idLength < base->idLength + instanceLen + 1)
        {
            /* Either the table or an item is specified, but the instance is
             * not.
             */
            *instance = 1;
            if (oid->idLength >= base->idLength + 1)
            {
                *item = oid->ids[base->idLength];
                if (!*item)
                    *item = 1;
            }
            else
                *item = 1;
        }
        else if (!SnmpUtilOidNCmp(oid, base, base->idLength) &&
            oid->idLength == base->idLength + instanceLen + 1)
        {
            *item = oid->ids[base->idLength];
            if (!*item)
            {
                *instance = 1;
                *item = 1;
            }
            else
            {
                AsnObjectIdentifier instanceOid = { instanceLen,
                    oid->ids + base->idLength + 1 };

                *instance = findNextOidInTable(&instanceOid, table,
                    tableEntrySize, makeKey, compare);
                if (!*instance || *instance > table->numEntries)
                    ret = SNMP_ERRORSTATUS_NOSUCHNAME;
            }
        }
        else
            ret = SNMP_ERRORSTATUS_NOSUCHNAME;
        break;
    default:
        if (!SnmpUtilOidNCmp(oid, base, base->idLength) &&
            oid->idLength == base->idLength + instanceLen + 1)
        {
            *item = oid->ids[base->idLength];
            if (!*item)
                ret = SNMP_ERRORSTATUS_NOSUCHNAME;
            else
            {
                AsnObjectIdentifier instanceOid = { instanceLen,
                    oid->ids + base->idLength + 1 };

                *instance = findOidInTable(&instanceOid, table, tableEntrySize,
                    makeKey, compare);
                if (!*instance)
                    ret = SNMP_ERRORSTATUS_NOSUCHNAME;
            }
        }
        else
            ret = SNMP_ERRORSTATUS_NOSUCHNAME;
    }
    return ret;
}

static INT setOidWithItem(AsnObjectIdentifier *dst, AsnObjectIdentifier *base,
    UINT item)
{
    UINT id;
    AsnObjectIdentifier oid;
    INT ret;

    SnmpUtilOidFree(dst);
    ret = SnmpUtilOidCpy(dst, base);
    if (ret)
    {
        oid.idLength = 1;
        oid.ids = &id;
        id = item;
        ret = SnmpUtilOidAppend(dst, &oid);
    }
    return ret;
}

static INT setOidWithItemAndIpAddr(AsnObjectIdentifier *dst,
    AsnObjectIdentifier *base, UINT item, DWORD addr)
{
    UINT id;
    BYTE *ptr;
    AsnObjectIdentifier oid;
    INT ret;

    ret = setOidWithItem(dst, base, item);
    if (ret)
    {
        oid.idLength = 1;
        oid.ids = &id;
        for (ptr = (BYTE *)&addr; ret && ptr < (BYTE *)&addr + sizeof(DWORD);
         ptr++)
        {
            id = *ptr;
            ret = SnmpUtilOidAppend(dst, &oid);
        }
    }
    return ret;
}

static INT setOidWithItemAndInteger(AsnObjectIdentifier *dst,
    AsnObjectIdentifier *base, UINT item, UINT instance)
{
    AsnObjectIdentifier oid;
    INT ret;

    ret = setOidWithItem(dst, base, item);
    if (ret)
    {
        oid.idLength = 1;
        oid.ids = &instance;
        ret = SnmpUtilOidAppend(dst, &oid);
    }
    return ret;
}

static DWORD copyIfRowDescr(AsnAny *value, void *src)
{
    PMIB_IFROW row = (PMIB_IFROW)((BYTE *)src -
                                  FIELD_OFFSET(MIB_IFROW, dwDescrLen));
    DWORD ret;

    if (row->dwDescrLen)
    {
        setStringValue(value, ASN_OCTETSTRING, row->dwDescrLen, row->bDescr);
        ret = SNMP_ERRORSTATUS_NOERROR;
    }
    else
        ret = SNMP_ERRORSTATUS_NOSUCHNAME;
    return ret;
}

static DWORD copyIfRowPhysAddr(AsnAny *value, void *src)
{
    PMIB_IFROW row = (PMIB_IFROW)((BYTE *)src -
                                  FIELD_OFFSET(MIB_IFROW, dwPhysAddrLen));
    DWORD ret;

    if (row->dwPhysAddrLen)
    {
        setStringValue(value, ASN_OCTETSTRING, row->dwPhysAddrLen,
                       row->bPhysAddr);
        ret = SNMP_ERRORSTATUS_NOERROR;
    }
    else
        ret = SNMP_ERRORSTATUS_NOSUCHNAME;
    return ret;
}

static struct structToAsnValue mib2IfEntryMap[] = {
    { FIELD_OFFSET(MIB_IFROW, dwIndex), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwDescrLen), copyIfRowDescr },
    { FIELD_OFFSET(MIB_IFROW, dwType), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwMtu), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwSpeed), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwPhysAddrLen), copyIfRowPhysAddr },
    { FIELD_OFFSET(MIB_IFROW, dwAdminStatus), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwOperStatus), copyOperStatus },
    { FIELD_OFFSET(MIB_IFROW, dwLastChange), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwInOctets), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwInUcastPkts), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwInNUcastPkts), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwInDiscards), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwInErrors), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwInUnknownProtos), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwOutOctets), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwOutUcastPkts), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwOutNUcastPkts), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwOutDiscards), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwOutErrors), copyInt },
    { FIELD_OFFSET(MIB_IFROW, dwOutQLen), copyInt },
};

static UINT mib2IfEntry[] = { 1,3,6,1,2,1,2,2,1 };

static BOOL mib2IfEntryQuery(BYTE bPduType, SnmpVarBind *pVarBind,
    AsnInteger32 *pErrorStatus)
{
    AsnObjectIdentifier entryOid = DEFINE_OID(mib2IfEntry);
    BOOL ret = TRUE;

    TRACE("(0x%02x, %s, %p)\n", bPduType, SnmpUtilOidToA(&pVarBind->name),
        pErrorStatus);

    switch (bPduType)
    {
    case SNMP_PDU_GET:
    case SNMP_PDU_GETNEXT:
        if (!ifTable)
        {
            /* There is no interface present, so let the caller deal
             * with finding the successor.
             */
            *pErrorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
        }
        else
        {
            UINT tableIndex = 0, item = 0;

            *pErrorStatus = getItemAndIntegerInstanceFromOid(&pVarBind->name,
                &entryOid, bPduType, &item, &tableIndex);
            if (!*pErrorStatus)
            {
                assert(tableIndex);
                assert(item);
                if (tableIndex > ifTable->dwNumEntries)
                    *pErrorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
                else
                {
                    *pErrorStatus = mapStructEntryToValue(mib2IfEntryMap,
                        DEFINE_SIZEOF(mib2IfEntryMap),
                        &ifTable->table[tableIndex - 1], item,
                        pVarBind);
                    if (bPduType == SNMP_PDU_GETNEXT)
                        ret = setOidWithItemAndInteger(&pVarBind->name,
                            &entryOid, item, tableIndex);
                }
            }
        }
        break;
    case SNMP_PDU_SET:
        *pErrorStatus = SNMP_ERRORSTATUS_READONLY;
        ret = FALSE;
        break;
    default:
        FIXME("0x%02x: unsupported PDU type\n", bPduType);
        *pErrorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
    }
    return ret;
}

static UINT mib2Ip[] = { 1,3,6,1,2,1,4 };
static MIB_IPSTATS ipStats;

static void mib2IpStatsInit(void)
{
    GetIpStatistics(&ipStats);
}

static struct structToAsnValue mib2IpMap[] = {
    { FIELD_OFFSET(MIB_IPSTATS, u.dwForwarding), copyInt }, /* 1 */
    { FIELD_OFFSET(MIB_IPSTATS, dwDefaultTTL), copyInt }, /* 2 */
    { FIELD_OFFSET(MIB_IPSTATS, dwInReceives), copyInt }, /* 3 */
    { FIELD_OFFSET(MIB_IPSTATS, dwInHdrErrors), copyInt }, /* 4 */
    { FIELD_OFFSET(MIB_IPSTATS, dwInAddrErrors), copyInt }, /* 5 */
    { FIELD_OFFSET(MIB_IPSTATS, dwForwDatagrams), copyInt }, /* 6 */
    { FIELD_OFFSET(MIB_IPSTATS, dwInUnknownProtos), copyInt }, /* 7 */
    { FIELD_OFFSET(MIB_IPSTATS, dwInDiscards), copyInt }, /* 8 */
    { FIELD_OFFSET(MIB_IPSTATS, dwInDelivers), copyInt }, /* 9 */
    { FIELD_OFFSET(MIB_IPSTATS, dwOutRequests), copyInt }, /* 10 */
    { FIELD_OFFSET(MIB_IPSTATS, dwOutDiscards), copyInt }, /* 11 */
    { FIELD_OFFSET(MIB_IPSTATS, dwOutNoRoutes), copyInt }, /* 12 */
    { FIELD_OFFSET(MIB_IPSTATS, dwReasmTimeout), copyInt }, /* 13 */
    { FIELD_OFFSET(MIB_IPSTATS, dwReasmReqds), copyInt }, /* 14 */
    { FIELD_OFFSET(MIB_IPSTATS, dwReasmOks), copyInt }, /* 15 */
    { FIELD_OFFSET(MIB_IPSTATS, dwReasmFails), copyInt }, /* 16 */
    { FIELD_OFFSET(MIB_IPSTATS, dwFragOks), copyInt }, /* 17 */
    { FIELD_OFFSET(MIB_IPSTATS, dwFragFails), copyInt }, /* 18 */
    { FIELD_OFFSET(MIB_IPSTATS, dwFragCreates), copyInt }, /* 19 */
    { 0, NULL }, /* 20: not used, IP addr table */
    { 0, NULL }, /* 21: not used, route table */
    { 0, NULL }, /* 22: not used, net to media (ARP) table */
    { FIELD_OFFSET(MIB_IPSTATS, dwRoutingDiscards), copyInt }, /* 23 */
};

static BOOL mib2IpStatsQuery(BYTE bPduType, SnmpVarBind *pVarBind,
    AsnInteger32 *pErrorStatus)
{
    AsnObjectIdentifier myOid = DEFINE_OID(mib2Ip);
    UINT item = 0;
    BOOL ret = TRUE;

    TRACE("(0x%02x, %s, %p)\n", bPduType, SnmpUtilOidToA(&pVarBind->name),
        pErrorStatus);

    switch (bPduType)
    {
    case SNMP_PDU_GET:
    case SNMP_PDU_GETNEXT:
        *pErrorStatus = getItemFromOid(&pVarBind->name, &myOid, bPduType,
            &item);
        if (!*pErrorStatus)
        {
            *pErrorStatus = mapStructEntryToValue(mib2IpMap,
                DEFINE_SIZEOF(mib2IpMap), &ipStats, item, pVarBind);
            if (!*pErrorStatus && bPduType == SNMP_PDU_GETNEXT)
                ret = setOidWithItem(&pVarBind->name, &myOid, item);
        }
        break;
    case SNMP_PDU_SET:
        *pErrorStatus = SNMP_ERRORSTATUS_READONLY;
        ret = FALSE;
        break;
    default:
        FIXME("0x%02x: unsupported PDU type\n", bPduType);
        *pErrorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
    }
    return ret;
}

static UINT mib2IpAddr[] = { 1,3,6,1,2,1,4,20,1 };
static PMIB_IPADDRTABLE ipAddrTable;

static struct structToAsnValue mib2IpAddrMap[] = {
    { FIELD_OFFSET(MIB_IPADDRROW, dwAddr), copyIpAddr },
    { FIELD_OFFSET(MIB_IPADDRROW, dwIndex), copyInt },
    { FIELD_OFFSET(MIB_IPADDRROW, dwMask), copyIpAddr },
    { FIELD_OFFSET(MIB_IPADDRROW, dwBCastAddr), copyInt },
    { FIELD_OFFSET(MIB_IPADDRROW, dwReasmSize), copyInt },
};

static void mib2IpAddrInit(void)
{
    DWORD size = 0, ret = GetIpAddrTable(NULL, &size, TRUE);

    if (ret == ERROR_INSUFFICIENT_BUFFER)
    {
        MIB_IPADDRTABLE *table = HeapAlloc(GetProcessHeap(), 0, size);
        if (table)
        {
            if (!GetIpAddrTable(table, &size, TRUE)) ipAddrTable = table;
            else HeapFree(GetProcessHeap(), 0, table );
        }
    }
}

static void mib2IpAddrCleanup(void)
{
    HeapFree(GetProcessHeap(), 0, ipAddrTable);
}

static void oidToIpAddrRow(AsnObjectIdentifier *oid, void *dst)
{
    MIB_IPADDRROW *row = dst;

    row->dwAddr = oidToIpAddr(oid);
}

static int compareIpAddrRow(const void *a, const void *b)
{
    const MIB_IPADDRROW *key = a, *value = b;

    return key->dwAddr - value->dwAddr;
}

static BOOL mib2IpAddrQuery(BYTE bPduType, SnmpVarBind *pVarBind,
    AsnInteger32 *pErrorStatus)
{
    AsnObjectIdentifier myOid = DEFINE_OID(mib2IpAddr);
    UINT tableIndex = 0, item = 0;
    BOOL ret = TRUE;

    TRACE("(0x%02x, %s, %p)\n", bPduType, SnmpUtilOidToA(&pVarBind->name),
        pErrorStatus);

    switch (bPduType)
    {
    case SNMP_PDU_GET:
    case SNMP_PDU_GETNEXT:
        *pErrorStatus = getItemAndInstanceFromTable(&pVarBind->name,
            &myOid, 4, bPduType, (struct GenericTable *)ipAddrTable,
            sizeof(MIB_IPADDRROW), oidToIpAddrRow, compareIpAddrRow, &item,
            &tableIndex);
        if (!*pErrorStatus)
        {
            assert(tableIndex);
            assert(item);
            *pErrorStatus = mapStructEntryToValue(mib2IpAddrMap,
                DEFINE_SIZEOF(mib2IpAddrMap),
                &ipAddrTable->table[tableIndex - 1], item, pVarBind);
            if (!*pErrorStatus && bPduType == SNMP_PDU_GETNEXT)
                ret = setOidWithItemAndIpAddr(&pVarBind->name, &myOid, item,
                    ipAddrTable->table[tableIndex - 1].dwAddr);
        }
        break;
    case SNMP_PDU_SET:
        *pErrorStatus = SNMP_ERRORSTATUS_READONLY;
        ret = FALSE;
        break;
    default:
        FIXME("0x%02x: unsupported PDU type\n", bPduType);
        *pErrorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
    }
    return ret;
}

static UINT mib2IpRoute[] = { 1,3,6,1,2,1,4,21,1 };
static PMIB_IPFORWARDTABLE ipRouteTable;

static struct structToAsnValue mib2IpRouteMap[] = {
    { FIELD_OFFSET(MIB_IPFORWARDROW, dwForwardDest), copyIpAddr },
    { FIELD_OFFSET(MIB_IPFORWARDROW, dwForwardIfIndex), copyInt },
    { FIELD_OFFSET(MIB_IPFORWARDROW, dwForwardMetric1), copyInt },
    { FIELD_OFFSET(MIB_IPFORWARDROW, dwForwardMetric2), copyInt },
    { FIELD_OFFSET(MIB_IPFORWARDROW, dwForwardMetric3), copyInt },
    { FIELD_OFFSET(MIB_IPFORWARDROW, dwForwardMetric4), copyInt },
    { FIELD_OFFSET(MIB_IPFORWARDROW, dwForwardNextHop), copyIpAddr },
    { FIELD_OFFSET(MIB_IPFORWARDROW, u1.dwForwardType), copyInt },
    { FIELD_OFFSET(MIB_IPFORWARDROW, u2.dwForwardProto), copyInt },
    { FIELD_OFFSET(MIB_IPFORWARDROW, dwForwardAge), copyInt },
    { FIELD_OFFSET(MIB_IPFORWARDROW, dwForwardMask), copyIpAddr },
    { FIELD_OFFSET(MIB_IPFORWARDROW, dwForwardMetric5), copyInt },
};

static void mib2IpRouteInit(void)
{
    DWORD size = 0, ret = GetIpForwardTable(NULL, &size, TRUE);

    if (ret == ERROR_INSUFFICIENT_BUFFER)
    {
        MIB_IPFORWARDTABLE *table = HeapAlloc(GetProcessHeap(), 0, size);
        if (table)
        {
            if (!GetIpForwardTable(table, &size, TRUE)) ipRouteTable = table;
            else HeapFree(GetProcessHeap(), 0, table );
        }
    }
}

static void mib2IpRouteCleanup(void)
{
    HeapFree(GetProcessHeap(), 0, ipRouteTable);
}

static void oidToIpForwardRow(AsnObjectIdentifier *oid, void *dst)
{
    MIB_IPFORWARDROW *row = dst;

    row->dwForwardDest = oidToIpAddr(oid);
}

static int compareIpForwardRow(const void *a, const void *b)
{
    const MIB_IPFORWARDROW *key = a, *value = b;

    return key->dwForwardDest - value->dwForwardDest;
}

static BOOL mib2IpRouteQuery(BYTE bPduType, SnmpVarBind *pVarBind,
    AsnInteger32 *pErrorStatus)
{
    AsnObjectIdentifier myOid = DEFINE_OID(mib2IpRoute);
    UINT tableIndex = 0, item = 0;
    BOOL ret = TRUE;

    TRACE("(0x%02x, %s, %p)\n", bPduType, SnmpUtilOidToA(&pVarBind->name),
        pErrorStatus);

    switch (bPduType)
    {
    case SNMP_PDU_GET:
    case SNMP_PDU_GETNEXT:
        *pErrorStatus = getItemAndInstanceFromTable(&pVarBind->name,
            &myOid, 4, bPduType, (struct GenericTable *)ipRouteTable,
            sizeof(MIB_IPFORWARDROW), oidToIpForwardRow, compareIpForwardRow,
            &item, &tableIndex);
        if (!*pErrorStatus)
        {
            assert(tableIndex);
            assert(item);
            *pErrorStatus = mapStructEntryToValue(mib2IpRouteMap,
                DEFINE_SIZEOF(mib2IpRouteMap),
                &ipRouteTable->table[tableIndex - 1], item, pVarBind);
            if (!*pErrorStatus && bPduType == SNMP_PDU_GETNEXT)
                ret = setOidWithItemAndIpAddr(&pVarBind->name, &myOid, item,
                    ipRouteTable->table[tableIndex - 1].dwForwardDest);
        }
        break;
    case SNMP_PDU_SET:
        *pErrorStatus = SNMP_ERRORSTATUS_READONLY;
        ret = FALSE;
        break;
    default:
        FIXME("0x%02x: unsupported PDU type\n", bPduType);
        *pErrorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
    }
    return ret;
}

static UINT mib2IpNet[] = { 1,3,6,1,2,1,4,22,1 };
static PMIB_IPNETTABLE ipNetTable;

static DWORD copyIpNetPhysAddr(AsnAny *value, void *src)
{
    PMIB_IPNETROW row = (PMIB_IPNETROW)((BYTE *)src - FIELD_OFFSET(MIB_IPNETROW,
                                        dwPhysAddrLen));

    setStringValue(value, ASN_OCTETSTRING, row->dwPhysAddrLen, row->bPhysAddr);
    return SNMP_ERRORSTATUS_NOERROR;
}

static struct structToAsnValue mib2IpNetMap[] = {
    { FIELD_OFFSET(MIB_IPNETROW, dwIndex), copyInt },
    { FIELD_OFFSET(MIB_IPNETROW, dwPhysAddrLen), copyIpNetPhysAddr },
    { FIELD_OFFSET(MIB_IPNETROW, dwAddr), copyIpAddr },
    { FIELD_OFFSET(MIB_IPNETROW, u.dwType), copyInt },
};

static void mib2IpNetInit(void)
{
    DWORD size = 0, ret = GetIpNetTable(NULL, &size, FALSE);

    if (ret == ERROR_INSUFFICIENT_BUFFER)
    {
        MIB_IPNETTABLE *table = HeapAlloc(GetProcessHeap(), 0, size);
        if (table)
        {
            if (!GetIpNetTable(table, &size, FALSE)) ipNetTable = table;
            else HeapFree(GetProcessHeap(), 0, table );
        }
    }
}

static void mib2IpNetCleanup(void)
{
    HeapFree(GetProcessHeap(), 0, ipNetTable);
}

static BOOL mib2IpNetQuery(BYTE bPduType, SnmpVarBind *pVarBind,
    AsnInteger32 *pErrorStatus)
{
    AsnObjectIdentifier myOid = DEFINE_OID(mib2IpNet);
    BOOL ret = TRUE;

    TRACE("(0x%02x, %s, %p)\n", bPduType, SnmpUtilOidToA(&pVarBind->name),
        pErrorStatus);

    switch (bPduType)
    {
    case SNMP_PDU_GET:
    case SNMP_PDU_GETNEXT:
        if (!ipNetTable)
            *pErrorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
        else
        {
            UINT tableIndex = 0, item = 0;

            *pErrorStatus = getItemAndIntegerInstanceFromOid(&pVarBind->name,
                &myOid, bPduType, &item, &tableIndex);
            if (!*pErrorStatus)
            {
                assert(tableIndex);
                assert(item);
                if (tableIndex > ipNetTable->dwNumEntries)
                    *pErrorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
                else
                {
                    *pErrorStatus = mapStructEntryToValue(mib2IpNetMap,
                        DEFINE_SIZEOF(mib2IpNetMap),
                        &ipNetTable[tableIndex - 1], item, pVarBind);
                    if (!*pErrorStatus && bPduType == SNMP_PDU_GETNEXT)
                        ret = setOidWithItemAndInteger(&pVarBind->name, &myOid,
                            item, tableIndex);
                }
            }
        }
        break;
    case SNMP_PDU_SET:
        *pErrorStatus = SNMP_ERRORSTATUS_READONLY;
        ret = FALSE;
        break;
    default:
        FIXME("0x%02x: unsupported PDU type\n", bPduType);
        *pErrorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
    }
    return ret;
}

static UINT mib2Icmp[] = { 1,3,6,1,2,1,5 };
static MIB_ICMP icmpStats;

static void mib2IcmpInit(void)
{
    GetIcmpStatistics(&icmpStats);
}

static struct structToAsnValue mib2IcmpMap[] = {
    { FIELD_OFFSET(MIBICMPINFO, icmpInStats.dwMsgs), copyInt },
    { FIELD_OFFSET(MIBICMPINFO, icmpInStats.dwErrors), copyInt },
    { FIELD_OFFSET(MIBICMPINFO, icmpInStats.dwDestUnreachs), copyInt },
    { FIELD_OFFSET(MIBICMPINFO, icmpInStats.dwTimeExcds), copyInt },
    { FIELD_OFFSET(MIBICMPINFO, icmpInStats.dwParmProbs), copyInt },
    { FIELD_OFFSET(MIBICMPINFO, icmpInStats.dwSrcQuenchs), copyInt },
    { FIELD_OFFSET(MIBICMPINFO, icmpInStats.dwRedirects), copyInt },
    { FIELD_OFFSET(MIBICMPINFO, icmpInStats.dwEchos), copyInt },
    { FIELD_OFFSET(MIBICMPINFO, icmpInStats.dwEchoReps), copyInt },
    { FIELD_OFFSET(MIBICMPINFO, icmpInStats.dwTimestamps), copyInt },
    { FIELD_OFFSET(MIBICMPINFO, icmpInStats.dwTimestampReps), copyInt },
    { FIELD_OFFSET(MIBICMPINFO, icmpInStats.dwAddrMasks), copyInt },
    { FIELD_OFFSET(MIBICMPINFO, icmpInStats.dwAddrMaskReps), copyInt },
    { FIELD_OFFSET(MIBICMPINFO, icmpOutStats.dwMsgs), copyInt },
    { FIELD_OFFSET(MIBICMPINFO, icmpOutStats.dwErrors), copyInt },
    { FIELD_OFFSET(MIBICMPINFO, icmpOutStats.dwDestUnreachs), copyInt },
    { FIELD_OFFSET(MIBICMPINFO, icmpOutStats.dwTimeExcds), copyInt },
    { FIELD_OFFSET(MIBICMPINFO, icmpOutStats.dwParmProbs), copyInt },
    { FIELD_OFFSET(MIBICMPINFO, icmpOutStats.dwSrcQuenchs), copyInt },
    { FIELD_OFFSET(MIBICMPINFO, icmpOutStats.dwRedirects), copyInt },
    { FIELD_OFFSET(MIBICMPINFO, icmpOutStats.dwEchos), copyInt },
    { FIELD_OFFSET(MIBICMPINFO, icmpOutStats.dwEchoReps), copyInt },
    { FIELD_OFFSET(MIBICMPINFO, icmpOutStats.dwTimestamps), copyInt },
    { FIELD_OFFSET(MIBICMPINFO, icmpOutStats.dwTimestampReps), copyInt },
    { FIELD_OFFSET(MIBICMPINFO, icmpOutStats.dwAddrMasks), copyInt },
    { FIELD_OFFSET(MIBICMPINFO, icmpOutStats.dwAddrMaskReps), copyInt },
};

static BOOL mib2IcmpQuery(BYTE bPduType, SnmpVarBind *pVarBind,
    AsnInteger32 *pErrorStatus)
{
    AsnObjectIdentifier myOid = DEFINE_OID(mib2Icmp);
    UINT item = 0;
    BOOL ret = TRUE;

    TRACE("(0x%02x, %s, %p)\n", bPduType, SnmpUtilOidToA(&pVarBind->name),
        pErrorStatus);

    switch (bPduType)
    {
    case SNMP_PDU_GET:
    case SNMP_PDU_GETNEXT:
        *pErrorStatus = getItemFromOid(&pVarBind->name, &myOid, bPduType,
            &item);
        if (!*pErrorStatus)
        {
            *pErrorStatus = mapStructEntryToValue(mib2IcmpMap,
                DEFINE_SIZEOF(mib2IcmpMap), &icmpStats, item,
                pVarBind);
            if (!*pErrorStatus && bPduType == SNMP_PDU_GETNEXT)
                ret = setOidWithItem(&pVarBind->name, &myOid, item);
        }
        break;
    case SNMP_PDU_SET:
        *pErrorStatus = SNMP_ERRORSTATUS_READONLY;
        ret = FALSE;
        break;
    default:
        FIXME("0x%02x: unsupported PDU type\n", bPduType);
        *pErrorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
    }
    return ret;
}

static UINT mib2Tcp[] = { 1,3,6,1,2,1,6 };
static MIB_TCPSTATS tcpStats;

static void mib2TcpInit(void)
{
    GetTcpStatistics(&tcpStats);
}

static struct structToAsnValue mib2TcpMap[] = {
    { FIELD_OFFSET(MIB_TCPSTATS, u.dwRtoAlgorithm), copyInt },
    { FIELD_OFFSET(MIB_TCPSTATS, dwRtoMin), copyInt },
    { FIELD_OFFSET(MIB_TCPSTATS, dwRtoMax), copyInt },
    { FIELD_OFFSET(MIB_TCPSTATS, dwMaxConn), copyInt },
    { FIELD_OFFSET(MIB_TCPSTATS, dwActiveOpens), copyInt },
    { FIELD_OFFSET(MIB_TCPSTATS, dwPassiveOpens), copyInt },
    { FIELD_OFFSET(MIB_TCPSTATS, dwAttemptFails), copyInt },
    { FIELD_OFFSET(MIB_TCPSTATS, dwEstabResets), copyInt },
    { FIELD_OFFSET(MIB_TCPSTATS, dwCurrEstab), copyInt },
    { FIELD_OFFSET(MIB_TCPSTATS, dwInSegs), copyInt },
    { FIELD_OFFSET(MIB_TCPSTATS, dwOutSegs), copyInt },
    { FIELD_OFFSET(MIB_TCPSTATS, dwRetransSegs), copyInt },
    { FIELD_OFFSET(MIB_TCPSTATS, dwInErrs), copyInt },
    { FIELD_OFFSET(MIB_TCPSTATS, dwOutRsts), copyInt },
    { FIELD_OFFSET(MIB_TCPSTATS, dwNumConns), copyInt },
};

static BOOL mib2TcpQuery(BYTE bPduType, SnmpVarBind *pVarBind,
    AsnInteger32 *pErrorStatus)
{
    AsnObjectIdentifier myOid = DEFINE_OID(mib2Tcp);
    UINT item = 0;
    BOOL ret = TRUE;

    TRACE("(0x%02x, %s, %p)\n", bPduType, SnmpUtilOidToA(&pVarBind->name),
        pErrorStatus);

    switch (bPduType)
    {
    case SNMP_PDU_GET:
    case SNMP_PDU_GETNEXT:
        *pErrorStatus = getItemFromOid(&pVarBind->name, &myOid, bPduType,
            &item);
        if (!*pErrorStatus)
        {
            *pErrorStatus = mapStructEntryToValue(mib2TcpMap,
                DEFINE_SIZEOF(mib2TcpMap), &tcpStats, item, pVarBind);
            if (!*pErrorStatus && bPduType == SNMP_PDU_GETNEXT)
                ret = setOidWithItem(&pVarBind->name, &myOid, item);
        }
        break;
    case SNMP_PDU_SET:
        *pErrorStatus = SNMP_ERRORSTATUS_READONLY;
        ret = FALSE;
        break;
    default:
        FIXME("0x%02x: unsupported PDU type\n", bPduType);
        *pErrorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
    }
    return ret;
}

static UINT mib2Udp[] = { 1,3,6,1,2,1,7 };
static MIB_UDPSTATS udpStats;

static void mib2UdpInit(void)
{
    GetUdpStatistics(&udpStats);
}

static struct structToAsnValue mib2UdpMap[] = {
    { FIELD_OFFSET(MIB_UDPSTATS, dwInDatagrams), copyInt },
    { FIELD_OFFSET(MIB_UDPSTATS, dwNoPorts), copyInt },
    { FIELD_OFFSET(MIB_UDPSTATS, dwInErrors), copyInt },
    { FIELD_OFFSET(MIB_UDPSTATS, dwOutDatagrams), copyInt },
};

static BOOL mib2UdpQuery(BYTE bPduType, SnmpVarBind *pVarBind,
    AsnInteger32 *pErrorStatus)
{
    AsnObjectIdentifier myOid = DEFINE_OID(mib2Udp);
    UINT item;
    BOOL ret = TRUE;

    TRACE("(0x%02x, %s, %p)\n", bPduType, SnmpUtilOidToA(&pVarBind->name),
        pErrorStatus);

    switch (bPduType)
    {
    case SNMP_PDU_GET:
    case SNMP_PDU_GETNEXT:
        *pErrorStatus = getItemFromOid(&pVarBind->name, &myOid, bPduType,
            &item);
        if (!*pErrorStatus)
        {
            *pErrorStatus = mapStructEntryToValue(mib2UdpMap,
                DEFINE_SIZEOF(mib2UdpMap), &udpStats, item, pVarBind);
            if (!*pErrorStatus && bPduType == SNMP_PDU_GETNEXT)
                ret = setOidWithItem(&pVarBind->name, &myOid, item);
        }
        break;
    case SNMP_PDU_SET:
        *pErrorStatus = SNMP_ERRORSTATUS_READONLY;
        ret = FALSE;
        break;
    default:
        FIXME("0x%02x: unsupported PDU type\n", bPduType);
        *pErrorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
    }
    return ret;
}

static UINT mib2UdpEntry[] = { 1,3,6,1,2,1,7,5,1 };
static PMIB_UDPTABLE udpTable;

static void mib2UdpEntryInit(void)
{
    DWORD size = 0, ret = GetUdpTable(NULL, &size, TRUE);

    if (ret == ERROR_INSUFFICIENT_BUFFER)
    {
        MIB_UDPTABLE *table = HeapAlloc(GetProcessHeap(), 0, size);
        if (table)
        {
            if (!GetUdpTable(table, &size, TRUE)) udpTable = table;
            else HeapFree(GetProcessHeap(), 0, table);
        }
    }
}

static void mib2UdpEntryCleanup(void)
{
    HeapFree(GetProcessHeap(), 0, udpTable);
}

static struct structToAsnValue mib2UdpEntryMap[] = {
    { FIELD_OFFSET(MIB_UDPROW, dwLocalAddr), copyIpAddr },
    { FIELD_OFFSET(MIB_UDPROW, dwLocalPort), copyInt },
};

static void oidToUdpRow(AsnObjectIdentifier *oid, void *dst)
{
    MIB_UDPROW *row = dst;

    assert(oid && oid->idLength >= 5);
    row->dwLocalAddr = oidToIpAddr(oid);
    row->dwLocalPort = oid->ids[4];
}

static int compareUdpRow(const void *a, const void *b)
{
    const MIB_UDPROW *key = a, *value = b;
    int ret;

    ret = key->dwLocalAddr - value->dwLocalAddr;
    if (ret == 0)
        ret = key->dwLocalPort - value->dwLocalPort;
    return ret;
}

static BOOL mib2UdpEntryQuery(BYTE bPduType, SnmpVarBind *pVarBind,
    AsnInteger32 *pErrorStatus)
{
    AsnObjectIdentifier myOid = DEFINE_OID(mib2UdpEntry);
    BOOL ret = TRUE;

    TRACE("(0x%02x, %s, %p)\n", bPduType, SnmpUtilOidToA(&pVarBind->name),
        pErrorStatus);

    switch (bPduType)
    {
    case SNMP_PDU_GET:
    case SNMP_PDU_GETNEXT:
        if (!udpTable)
            *pErrorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
        else
        {
            UINT tableIndex = 0, item = 0;

            *pErrorStatus = getItemAndInstanceFromTable(&pVarBind->name, &myOid,
                5, bPduType, (struct GenericTable *)udpTable,
                sizeof(MIB_UDPROW), oidToUdpRow, compareUdpRow, &item,
                &tableIndex);
            if (!*pErrorStatus)
            {
                assert(tableIndex);
                assert(item);
                *pErrorStatus = mapStructEntryToValue(mib2UdpEntryMap,
                    DEFINE_SIZEOF(mib2UdpEntryMap),
                    &udpTable->table[tableIndex - 1], item, pVarBind);
                if (!*pErrorStatus && bPduType == SNMP_PDU_GETNEXT)
                {
                    AsnObjectIdentifier oid;

                    ret = setOidWithItemAndIpAddr(&pVarBind->name, &myOid, item,
                        udpTable->table[tableIndex - 1].dwLocalAddr);
                    if (ret)
                    {
                        oid.idLength = 1;
                        oid.ids = &udpTable->table[tableIndex - 1].dwLocalPort;
                        ret = SnmpUtilOidAppend(&pVarBind->name, &oid);
                    }
                }
            }
        }
        break;
    case SNMP_PDU_SET:
        *pErrorStatus = SNMP_ERRORSTATUS_READONLY;
        ret = FALSE;
        break;
    default:
        FIXME("0x%02x: unsupported PDU type\n", bPduType);
        *pErrorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
    }
    return ret;
}

/* This list MUST BE lexicographically sorted */
static struct mibImplementation supportedIDs[] = {
    { DEFINE_OID(mib2IfNumber), mib2IfNumberInit, mib2IfNumberQuery,
      mib2IfNumberCleanup },
    { DEFINE_OID(mib2IfEntry), NULL, mib2IfEntryQuery, NULL },
    { DEFINE_OID(mib2Ip), mib2IpStatsInit, mib2IpStatsQuery, NULL },
    { DEFINE_OID(mib2IpAddr), mib2IpAddrInit, mib2IpAddrQuery,
      mib2IpAddrCleanup },
    { DEFINE_OID(mib2IpRoute), mib2IpRouteInit, mib2IpRouteQuery,
      mib2IpRouteCleanup },
    { DEFINE_OID(mib2IpNet), mib2IpNetInit, mib2IpNetQuery, mib2IpNetCleanup },
    { DEFINE_OID(mib2Icmp), mib2IcmpInit, mib2IcmpQuery, NULL },
    { DEFINE_OID(mib2Tcp), mib2TcpInit, mib2TcpQuery, NULL },
    { DEFINE_OID(mib2Udp), mib2UdpInit, mib2UdpQuery, NULL },
    { DEFINE_OID(mib2UdpEntry), mib2UdpEntryInit, mib2UdpEntryQuery,
      mib2UdpEntryCleanup },
};
static UINT minSupportedIDLength;

/*****************************************************************************
 * SnmpExtensionInit [INETMIB1.@]
 */
BOOL WINAPI SnmpExtensionInit(DWORD dwUptimeReference,
    HANDLE *phSubagentTrapEvent, AsnObjectIdentifier *pFirstSupportedRegion)
{
    AsnObjectIdentifier myOid = DEFINE_OID(mib2System);
    UINT i;

    TRACE("(%d, %p, %p)\n", dwUptimeReference, phSubagentTrapEvent,
        pFirstSupportedRegion);

    minSupportedIDLength = UINT_MAX;
    for (i = 0; i < sizeof(supportedIDs) / sizeof(supportedIDs[0]); i++)
    {
        if (supportedIDs[i].init)
            supportedIDs[i].init();
        if (supportedIDs[i].name.idLength < minSupportedIDLength)
            minSupportedIDLength = supportedIDs[i].name.idLength;
    }
    *phSubagentTrapEvent = NULL;
    SnmpUtilOidCpy(pFirstSupportedRegion, &myOid);
    return TRUE;
}

static void cleanup(void)
{
    UINT i;

    for (i = 0; i < sizeof(supportedIDs) / sizeof(supportedIDs[0]); i++)
        if (supportedIDs[i].cleanup)
            supportedIDs[i].cleanup();
}

static struct mibImplementation *findSupportedQuery(UINT *ids, UINT idLength,
    UINT *matchingIndex)
{
    int indexHigh = DEFINE_SIZEOF(supportedIDs) - 1, indexLow = 0;
    AsnObjectIdentifier oid1 = { idLength, ids};

    if (!idLength)
        return NULL;

    while (indexLow <= indexHigh)
    {
        INT cmp, i = (indexLow + indexHigh) / 2;
        if (!(cmp = SnmpUtilOidNCmp(&oid1, &supportedIDs[i].name, idLength)))
        {
            *matchingIndex = i;
            return &supportedIDs[i];
        }
        if (cmp > 0)
            indexLow = i + 1;
        else
            indexHigh = i - 1;
    }
    return NULL;
}

/*****************************************************************************
 * SnmpExtensionQuery [INETMIB1.@]
 */
BOOL WINAPI SnmpExtensionQuery(BYTE bPduType, SnmpVarBindList *pVarBindList,
    AsnInteger32 *pErrorStatus, AsnInteger32 *pErrorIndex)
{
    AsnObjectIdentifier mib2oid = DEFINE_OID(mib2);
    AsnInteger32 error = SNMP_ERRORSTATUS_NOERROR, errorIndex = 0;
    UINT i;
    BOOL ret = TRUE;

    TRACE("(0x%02x, %p, %p, %p)\n", bPduType, pVarBindList,
        pErrorStatus, pErrorIndex);

    for (i = 0; !error && i < pVarBindList->len; i++)
    {
        /* Ignore any OIDs not in MIB2 */
        if (!SnmpUtilOidNCmp(&pVarBindList->list[i].name, &mib2oid,
            mib2oid.idLength))
        {
            struct mibImplementation *impl = NULL;
            UINT len, matchingIndex = 0;

            TRACE("%s\n", SnmpUtilOidToA(&pVarBindList->list[i].name));
            /* Search for an implementation matching as many octets as possible
             */
            for (len = pVarBindList->list[i].name.idLength;
                len >= minSupportedIDLength && !impl; len--)
                impl = findSupportedQuery(pVarBindList->list[i].name.ids, len,
                    &matchingIndex);
            if (impl && impl->query)
                ret = impl->query(bPduType, &pVarBindList->list[i], &error);
            else
                error = SNMP_ERRORSTATUS_NOSUCHNAME;
            if (error == SNMP_ERRORSTATUS_NOSUCHNAME &&
                bPduType == SNMP_PDU_GETNEXT)
            {
                /* GetNext is special: it finds the successor to the given OID,
                 * so we have to continue until an implementation handles the
                 * query or we exhaust the table of supported OIDs.
                 */
                for (matchingIndex++; error == SNMP_ERRORSTATUS_NOSUCHNAME &&
                    matchingIndex < DEFINE_SIZEOF(supportedIDs);
                    matchingIndex++)
                {
                    error = SNMP_ERRORSTATUS_NOERROR;
                    impl = &supportedIDs[matchingIndex];
                    if (impl->query)
                        ret = impl->query(bPduType, &pVarBindList->list[i],
                            &error);
                    else
                        error = SNMP_ERRORSTATUS_NOSUCHNAME;
                }
                /* If the query still isn't resolved, set the OID to the
                 * successor to the last entry in the table.
                 */
                if (error == SNMP_ERRORSTATUS_NOSUCHNAME)
                {
                    SnmpUtilOidFree(&pVarBindList->list[i].name);
                    ret = SnmpUtilOidCpy(&pVarBindList->list[i].name,
                        &supportedIDs[matchingIndex - 1].name);
                    pVarBindList->list[i].name.ids[
                        pVarBindList->list[i].name.idLength - 1] += 1;
                }
            }
            if (error)
                errorIndex = i + 1;
        }
    }
    *pErrorStatus = error;
    *pErrorIndex = errorIndex;
    return ret;
}

/*****************************************************************************
 * DllMain [INETMIB1.@]
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
        case DLL_PROCESS_DETACH:
            if (lpvReserved) break;
            cleanup();
            break;
    }

    return TRUE;
}
