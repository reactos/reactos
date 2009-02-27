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
#include <stdio.h>
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <snmp.h>

#include "wine/test.h"

static HMODULE inetmib1;

static void testInit(void)
{
    BOOL (WINAPI *pInit)(DWORD, HANDLE *, AsnObjectIdentifier *);
    BOOL ret;
    HANDLE event;
    AsnObjectIdentifier oid;

    pInit = (void *)GetProcAddress(inetmib1, "SnmpExtensionInit");
    if (!pInit)
    {
        skip("no SnmpExtensionInit\n");
        return;
    }
    /* Crash
    ret = pInit(0, NULL, NULL);
    ret = pInit(0, NULL, &oid);
    ret = pInit(0, &event, NULL);
     */
    ret = pInit(0, &event, &oid);
    ok(ret, "SnmpExtensionInit failed: %d\n", GetLastError());
    ok(!strcmp("1.3.6.1.2.1.1", SnmpUtilOidToA(&oid)),
        "Expected 1.3.6.1.2.1.1, got %s\n", SnmpUtilOidToA(&oid));
}

static void testQuery(void)
{
    BOOL (WINAPI *pQuery)(BYTE, SnmpVarBindList *, AsnInteger32 *,
        AsnInteger32 *);
    BOOL ret, moreData, noChange;
    SnmpVarBindList list;
    AsnInteger32 error, index;
    UINT bogus[] = { 1,2,3,4 };
    UINT mib2System[] = { 1,3,6,1,2,1,1 };
    UINT mib2If[] = { 1,3,6,1,2,1,2 };
    UINT mib2IfTable[] = { 1,3,6,1,2,1,2,2 };
    UINT mib2IfDescr[] = { 1,3,6,1,2,1,2,2,1,2 };
    UINT mib2IfAdminStatus[] = { 1,3,6,1,2,1,2,2,1,7 };
    UINT mib2IfOperStatus[] = { 1,3,6,1,2,1,2,2,1,8 };
    UINT mib2IpAddr[] = { 1,3,6,1,2,1,4,20,1,1 };
    UINT mib2IpRouteTable[] = { 1,3,6,1,2,1,4,21,1,1 };
    UINT mib2UdpTable[] = { 1,3,6,1,2,1,7,5,1,1 };
    SnmpVarBind vars[3], vars2[3], vars3[3];
    UINT entry;

    pQuery = (void *)GetProcAddress(inetmib1, "SnmpExtensionQuery");
    if (!pQuery)
    {
        skip("couldn't find SnmpExtensionQuery\n");
        return;
    }
    /* Crash
    ret = pQuery(0, NULL, NULL, NULL);
    ret = pQuery(0, NULL, &error, NULL);
    ret = pQuery(0, NULL, NULL, &index);
    ret = pQuery(0, &list, NULL, NULL);
    ret = pQuery(0, &list, &error, NULL);
     */

    /* An empty list succeeds */
    list.len = 0;
    error = 0xdeadbeef;
    index = 0xdeadbeef;
    ret = pQuery(SNMP_PDU_GET, &list, &error, &index);
    ok(ret, "SnmpExtensionQuery failed: %d\n", GetLastError());
    ok(error == SNMP_ERRORSTATUS_NOERROR,
        "expected SNMP_ERRORSTATUS_NOERROR, got %d\n", error);
    ok(index == 0, "expected index 0, got %d\n", index);

    /* Oddly enough, this "succeeds," even though the OID is clearly
     * unsupported.
     */
    vars[0].name.idLength = sizeof(bogus) / sizeof(bogus[0]);
    vars[0].name.ids = bogus;
    vars[0].value.asnType = 0;
    list.len = 1;
    list.list = vars;
    SetLastError(0xdeadbeef);
    error = 0xdeadbeef;
    index = 0xdeadbeef;
    ret = pQuery(SNMP_PDU_GET, &list, &error, &index);
    ok(ret, "SnmpExtensionQuery failed: %d\n", GetLastError());
    ok(error == SNMP_ERRORSTATUS_NOERROR ||
        error == ERROR_FILE_NOT_FOUND /* Win9x */,
        "expected SNMP_ERRORSTATUS_NOERROR or ERROR_FILE_NOT_FOUND, got %d\n",
        error);
    if (error == SNMP_ERRORSTATUS_NOERROR)
        ok(index == 0, "expected index 0, got %d\n", index);
    else if (error == ERROR_FILE_NOT_FOUND)
        ok(index == 1, "expected index 1, got %d\n", index);
    /* The OID isn't changed either: */
    ok(!strcmp("1.2.3.4", SnmpUtilOidToA(&vars[0].name)),
        "expected 1.2.3.4, got %s\n", SnmpUtilOidToA(&vars[0].name));

    /* The table is not an accessible variable, so it fails */
    vars[0].name.idLength = sizeof(mib2IfTable) / sizeof(mib2IfTable[0]);
    vars[0].name.ids = mib2IfTable;
    SetLastError(0xdeadbeef);
    error = 0xdeadbeef;
    index = 0xdeadbeef;
    ret = pQuery(SNMP_PDU_GET, &list, &error, &index);
    ok(ret, "SnmpExtensionQuery failed: %d\n", GetLastError());
    ok(error == SNMP_ERRORSTATUS_NOSUCHNAME,
        "expected SNMP_ERRORSTATUS_NOSUCHNAME, got %d\n", error);
    /* The index is 1-based rather than 0-based */
    ok(index == 1, "expected index 1, got %d\n", index);

    /* A Get fails on something that specifies a table (but not a particular
     * entry in it)...
     */
    vars[0].name.idLength = sizeof(mib2IfDescr) / sizeof(mib2IfDescr[0]);
    vars[0].name.ids = mib2IfDescr;
    vars[1].name.idLength =
        sizeof(mib2IfAdminStatus) / sizeof(mib2IfAdminStatus[0]);
    vars[1].name.ids = mib2IfAdminStatus;
    vars[2].name.idLength =
        sizeof(mib2IfOperStatus) / sizeof(mib2IfOperStatus[0]);
    vars[2].name.ids = mib2IfOperStatus;
    list.len = 3;
    SetLastError(0xdeadbeef);
    error = 0xdeadbeef;
    index = 0xdeadbeef;
    ret = pQuery(SNMP_PDU_GET, &list, &error, &index);
    ok(ret, "SnmpExtensionQuery failed: %d\n", GetLastError());
    ok(error == SNMP_ERRORSTATUS_NOSUCHNAME,
        "expected SNMP_ERRORSTATUS_NOSUCHNAME, got %d\n", error);
    ok(index == 1, "expected index 1, got %d\n", index);
    /* but a GetNext succeeds with the same values, because GetNext gets the
     * entry after the specified OID, not the entry specified by it.  The
     * successor to the table is the first entry in the table.
     * The OIDs need to be allocated, because GetNext modifies them to indicate
     * the end of data.
     */
    SnmpUtilOidCpy(&vars2[0].name, &vars[0].name);
    SnmpUtilOidCpy(&vars2[1].name, &vars[1].name);
    SnmpUtilOidCpy(&vars2[2].name, &vars[2].name);
    list.list = vars2;
    moreData = TRUE;
    noChange = FALSE;
    entry = 0;
    do {
        SetLastError(0xdeadbeef);
        error = 0xdeadbeef;
        index = 0xdeadbeef;
        ret = pQuery(SNMP_PDU_GETNEXT, &list, &error, &index);
        ok(ret, "SnmpExtensionQuery failed: %d\n", GetLastError());
        ok(error == SNMP_ERRORSTATUS_NOERROR,
            "expected SNMP_ERRORSTATUS_NOERROR, got %d\n", error);
        ok(index == 0, "expected index 0, got %d\n", index);
        if (!ret)
            moreData = FALSE;
        else if (error)
            moreData = FALSE;
        else if (SnmpUtilOidNCmp(&vars2[0].name, &vars[0].name,
            vars[0].name.idLength))
            moreData = FALSE;
        else if (SnmpUtilOidNCmp(&vars2[1].name, &vars[1].name,
            vars[1].name.idLength))
            moreData = FALSE;
        else if (SnmpUtilOidNCmp(&vars2[2].name, &vars[2].name,
            vars[2].name.idLength))
            moreData = FALSE;
        else if (!SnmpUtilOidCmp(&vars[0].name, &vars2[0].name) ||
         !SnmpUtilOidCmp(&vars[0].name, &vars2[0].name) ||
         !SnmpUtilOidCmp(&vars[0].name, &vars2[0].name))
        {
            /* If the OID isn't modified, the function isn't implemented on this
             * platform, skip the remaining tests.
             */
            noChange = TRUE;
        }
        if (moreData)
        {
            UINT lastID;

            /* Check the OIDs.  For these types of values (display strings and
             * integers) they should increase by 1 for each element of the table
             * according to RFC 1158.  Windows sometimes has a weird value in the
             * table, so allow any value as long as it's greater than the previous
             * value on Windows.
             */
            ok(vars2[0].name.idLength == vars[0].name.idLength + 1,
                "expected length %d, got %d\n", vars[0].name.idLength + 1,
                vars2[0].name.idLength);
            lastID = vars2[0].name.ids[vars2[0].name.idLength - 1];
            ok(lastID == entry + 1 || broken(lastID > entry),
                "expected %d, got %d\n", entry + 1, lastID);
            ok(vars2[1].name.idLength == vars[1].name.idLength + 1,
                "expected length %d, got %d\n", vars[1].name.idLength + 1,
                vars2[1].name.idLength);
            lastID = vars2[1].name.ids[vars2[1].name.idLength - 1];
            ok(lastID == entry + 1 || broken(lastID > entry),
                "expected %d, got %d\n", entry + 1, lastID);
            ok(vars2[2].name.idLength == vars[2].name.idLength + 1,
                "expected length %d, got %d\n", vars[2].name.idLength + 1,
                vars2[2].name.idLength);
            lastID = vars2[2].name.ids[vars2[2].name.idLength - 1];
            ok(lastID == entry + 1 || broken(lastID > entry),
                "expected %d, got %d\n", entry + 1, lastID);
            entry = lastID;
            /* Check the types while we're at it */
            ok(vars2[0].value.asnType == ASN_OCTETSTRING,
                "expected ASN_OCTETSTRING, got %02x\n", vars2[0].value.asnType);
            ok(vars2[1].value.asnType == ASN_INTEGER,
                "expected ASN_INTEGER, got %02x\n", vars2[1].value.asnType);
            ok(vars2[2].value.asnType == ASN_INTEGER,
                "expected ASN_INTEGER, got %02x\n", vars2[2].value.asnType);
        }
        else if (noChange)
            skip("no change in OID, no MIB2 IF table implementation\n");
    } while (moreData && !noChange);
    SnmpUtilVarBindFree(&vars2[0]);
    SnmpUtilVarBindFree(&vars2[1]);
    SnmpUtilVarBindFree(&vars2[2]);

    /* Even though SnmpExtensionInit says this DLL supports the MIB2 system
     * variables, on recent systems (at least Win2k) the first variable it
     * returns a value for is the first interface.
     */
    vars[0].name.idLength = sizeof(mib2System) / sizeof(mib2System[0]);
    vars[0].name.ids = mib2System;
    SnmpUtilOidCpy(&vars2[0].name, &vars[0].name);
    vars2[0].value.asnType = 0;
    list.len = 1;
    list.list = vars2;
    moreData = TRUE;
    noChange = FALSE;
    ret = pQuery(SNMP_PDU_GETNEXT, &list, &error, &index);
    ok(ret, "SnmpExtensionQuery failed: %d\n", GetLastError());
    ok(error == SNMP_ERRORSTATUS_NOERROR,
        "expected SNMP_ERRORSTATUS_NOERROR, got %d\n", error);
    ok(index == 0, "expected index 0, got %d\n", index);
    vars3[0].name.idLength = sizeof(mib2If) / sizeof(mib2If[0]);
    vars3[0].name.ids = mib2If;
    ok(!SnmpUtilOidNCmp(&vars2[0].name, &vars[0].name, vars[0].name.idLength) ||
       !SnmpUtilOidNCmp(&vars2[0].name, &vars3[0].name, vars3[0].name.idLength),
        "expected 1.3.6.1.2.1.1 or 1.3.6.1.2.1.2, got %s\n",
        SnmpUtilOidToA(&vars2[0].name));
    SnmpUtilVarBindFree(&vars2[0]);

    /* Check the type and OIDs of the IP address table */
    vars[0].name.idLength = sizeof(mib2IpAddr) / sizeof(mib2IpAddr[0]);
    vars[0].name.ids = mib2IpAddr;
    SnmpUtilOidCpy(&vars2[0].name, &vars[0].name);
    vars2[0].value.asnType = 0;
    list.len = 1;
    list.list = vars2;
    moreData = TRUE;
    do {
        ret = pQuery(SNMP_PDU_GETNEXT, &list, &error, &index);
        ok(ret, "SnmpExtensionQuery failed: %d\n", GetLastError());
        ok(error == SNMP_ERRORSTATUS_NOERROR,
            "expected SNMP_ERRORSTATUS_NOERROR, got %d\n", error);
        ok(index == 0, "expected index 0, got %d\n", index);
        if (!ret)
            moreData = FALSE;
        else if (error)
            moreData = FALSE;
        else if (SnmpUtilOidNCmp(&vars2[0].name, &vars[0].name,
            vars[0].name.idLength))
            moreData = FALSE;
        else if (!SnmpUtilOidCmp(&vars2[0].name, &vars[0].name))
        {
            /* If the OID isn't modified, the function isn't implemented on this
             * platform, skip the remaining tests.
             */
            noChange = TRUE;
        }
        if (moreData)
        {
            /* Make sure the size of the OID is right.
             * FIXME: don't know if IPv6 addrs are shared with this table.
             * Don't think so, but I'm not certain.
             */
            ok(vars2[0].name.idLength == vars[0].name.idLength + 4,
                "expected length %d, got %d\n", vars[0].name.idLength + 4,
                vars2[0].name.idLength);
            /* Make sure the type is right */
            ok(vars2[0].value.asnType == ASN_IPADDRESS,
                "expected type ASN_IPADDRESS, got %02x\n",
                vars2[0].value.asnType);
            if (vars2[0].value.asnType == ASN_IPADDRESS)
            {
                UINT i;

                /* This looks uglier than it is:  the base OID for the IP
                 * address, 1.3.6.1.2.1.4.20.1.1, is appended with the IP
                 * address of the entry.  So e.g. the loopback address is
                 * identified in MIB2 as 1.3.6.1.2.1.4.20.1.1.127.0.0.1
                 */
                for (i = 0; i < vars2[0].value.asnValue.address.length; i++)
                {
                    ok(vars2[0].value.asnValue.address.stream[i] ==
                        vars2[0].name.ids[vars2[0].name.idLength - 4 + i],
                        "expected ident byte %d to be %d, got %d\n", i,
                        vars2[0].value.asnValue.address.stream[i],
                        vars2[0].name.ids[vars2[0].name.idLength - 4 + i]);
                }
            }
        }
        else if (noChange)
            skip("no change in OID, no MIB2 IP address table implementation\n");
    } while (moreData && !noChange);
    SnmpUtilVarBindFree(&vars2[0]);

    /* Check the type and OIDs of the IP route table */
    vars[0].name.idLength = DEFINE_SIZEOF(mib2IpRouteTable);
    vars[0].name.ids = mib2IpRouteTable;
    SnmpUtilOidCpy(&vars2[0].name, &vars[0].name);
    vars2[0].value.asnType = 0;
    list.len = 1;
    list.list = vars2;
    moreData = TRUE;
    noChange = FALSE;
    do {
        ret = pQuery(SNMP_PDU_GETNEXT, &list, &error, &index);
        ok(ret, "SnmpExtensionQuery failed: %d\n", GetLastError());
        ok(error == SNMP_ERRORSTATUS_NOERROR,
            "expected SNMP_ERRORSTATUS_NOERROR, got %d\n", error);
        ok(index == 0, "expected index 0, got %d\n", index);
        if (!ret)
            moreData = FALSE;
        else if (error)
            moreData = FALSE;
        else if (SnmpUtilOidNCmp(&vars2[0].name, &vars[0].name,
            vars[0].name.idLength))
            moreData = FALSE;
        else if (!SnmpUtilOidCmp(&vars2[0].name, &vars[0].name))
        {
            /* If the OID isn't modified, the function isn't implemented on this
             * platform, skip the remaining tests.
             */
            noChange = TRUE;
        }
        if (moreData)
        {
            /* Make sure the size of the OID is right.
             * FIXME: don't know if IPv6 addrs are shared with this table.
             * Don't think so, but I'm not certain.
             */
            ok(vars2[0].name.idLength == vars[0].name.idLength + 4,
                "expected length %d, got %d\n", vars[0].name.idLength + 4,
                vars2[0].name.idLength);
            /* Make sure the type is right */
            ok(vars2[0].value.asnType == ASN_IPADDRESS,
                "expected type ASN_IPADDRESS, got %02x\n",
                vars2[0].value.asnType);
            if (vars2[0].value.asnType == ASN_IPADDRESS)
            {
                UINT i;

                /* The base OID for the route table, 1.3.6.1.2.1.4.21.1.1, is
                 * appended with the dest IP address of the entry.  So e.g. a
                 * route entry for 224.0.0.0 is identified in MIB2 as
                 * 1.3.6.1.2.1.4.21.1.1.224.0.0.0
                 */
                for (i = 0; i < vars2[0].value.asnValue.address.length; i++)
                {
                    ok(vars2[0].value.asnValue.address.stream[i] ==
                        vars2[0].name.ids[vars2[0].name.idLength - 4 + i],
                        "expected ident byte %d to be %d, got %d\n", i,
                        vars2[0].value.asnValue.address.stream[i],
                        vars2[0].name.ids[vars2[0].name.idLength - 4 + i]);
                }
            }
        }
        else if (noChange)
            skip("no change in OID, no MIB2 IP route table implementation\n");
    } while (moreData && !noChange);
    SnmpUtilVarBindFree(&vars2[0]);

    /* Check the type and OIDs of the UDP table */
    vars[0].name.idLength = DEFINE_SIZEOF(mib2UdpTable);
    vars[0].name.ids = mib2UdpTable;
    SnmpUtilOidCpy(&vars2[0].name, &vars[0].name);
    vars2[0].value.asnType = 0;
    list.len = 1;
    list.list = vars2;
    moreData = TRUE;
    noChange = FALSE;
    do {
        ret = pQuery(SNMP_PDU_GETNEXT, &list, &error, &index);
        ok(ret, "SnmpExtensionQuery failed: %d\n", GetLastError());
        /* FIXME:  error and index aren't checked here because the UDP table is
         * the last OID currently supported by Wine, so the last GetNext fails.
         * todo_wine is also not effective because it will succeed for all but
         * the last GetNext.  Remove the if (0) if any later OID is supported
         * by Wine.
         */
        if (0) {
        ok(error == SNMP_ERRORSTATUS_NOERROR,
            "expected SNMP_ERRORSTATUS_NOERROR, got %d\n", error);
        ok(index == 0, "expected index 0, got %d\n", index);
        }
        if (!ret)
            moreData = FALSE;
        else if (error)
            moreData = FALSE;
        else if (SnmpUtilOidNCmp(&vars2[0].name, &vars[0].name,
            vars[0].name.idLength))
            moreData = FALSE;
        else if (!SnmpUtilOidCmp(&vars2[0].name, &vars[0].name))
        {
            /* If the OID isn't modified, the function isn't implemented on this
             * platform, skip the remaining tests.
             */
            noChange = TRUE;
        }
        if (moreData)
        {
            /* Make sure the size of the OID is right. */
            ok(vars2[0].name.idLength == vars[0].name.idLength + 5,
                "expected length %d, got %d\n", vars[0].name.idLength + 5,
                vars2[0].name.idLength);
            /* Make sure the type is right */
            ok(vars2[0].value.asnType == ASN_IPADDRESS,
                "expected type ASN_IPADDRESS, got %02x\n",
                vars2[0].value.asnType);
            if (vars2[0].value.asnType == ASN_IPADDRESS)
            {
                UINT i;

                /* Again with the ugly:  the base OID for the UDP table,
                 * 1.3.6.1.2.1.7.5.1, is appended with the local IP address and
                 * port number of the entry.  So e.g. an entry for
                 * 192.168.1.1:4000 is identified in MIB2 as
                 * 1.3.6.1.2.1.7.5.1.192.168.1.1.4000
                 */
                for (i = 0; i < vars2[0].value.asnValue.address.length; i++)
                {
                    ok(vars2[0].value.asnValue.address.stream[i] ==
                        vars2[0].name.ids[vars2[0].name.idLength - 5 + i],
                        "expected ident byte %d to be %d, got %d\n", i,
                        vars2[0].value.asnValue.address.stream[i],
                        vars2[0].name.ids[vars2[0].name.idLength - 5 + i]);
                }
            }
        }
        else if (noChange)
            skip("no change in OID, no MIB2 UDP table implementation\n");
    } while (moreData && !noChange);
    SnmpUtilVarBindFree(&vars2[0]);
}

START_TEST(main)
{
    inetmib1 = LoadLibraryA("inetmib1");
    testInit();
    testQuery();
}
