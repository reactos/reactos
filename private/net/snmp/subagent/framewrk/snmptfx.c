/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    snmptfx.c

Abstract:

    Provides common varbind resolution functionality for subagents.

Environment:

    User Mode - Win32

Revision History:

    02-Oct-1996 DonRyan
        Moved from extensible agent in anticipation of SNMPv2 SPI.

--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <snmp.h>
#include <snmputil.h>
#include <snmpexts.h>
#include <winsock.h>

#define HASH_TABLE_SIZE     101
#define HASH_TABLE_RADIX    18

#define INVALID_INDEX       ((DWORD)(-1))

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private type definitions                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef struct _SnmpVarBindXlat {

    UINT                   vlIndex;  // index into view list
    UINT                   vblIndex; // index into varbind list
    SnmpMibEntry *         mibEntry; // pointer to mib information
    struct _SnmpExtQuery * extQuery; // pointer to followup query

} SnmpVarBindXlat;

typedef struct _SnmpGenericList {

    VOID * data; // context-specific pointer
    UINT   len;  // context-specific length

} SnmpGenericList;

typedef struct _SnmpTableXlat {

    AsnObjectIdentifier txOid;   // table index oid
    SnmpMibTable *      txInfo;  // table description
    UINT                txIndex; // index into table list

} SnmpTableXlat;

typedef struct _SnmpExtQuery {

    UINT              mibAction; // type of query
    UINT              viewType;  // type of view
    UINT              vblNum;    // number of varbinds
    SnmpVarBindXlat * vblXlat;   // info to reorder varbinds
    SnmpTableXlat *   tblXlat;   // info to parse table oids
    SnmpGenericList   extData;   // context-specific buffer
    FARPROC           extFunc;   // instrumentation callback

} SnmpExtQuery;

#define INVALID_QUERY ((SnmpExtQuery*)(DWORD)(-1))

typedef struct _SnmpExtQueryList {

    SnmpExtQuery * query;  // list of subagent queries
    UINT           len;    // number of queries in list
    UINT           action; // original query request

} SnmpExtQueryList;

typedef struct _SnmpHashNode {

    SnmpMibEntry *         mibEntry;
    struct _SnmpHashNode * nextEntry;

} SnmpHashNode;

typedef struct _SnmpTfxView {

    SnmpMibView  *  mibView;
    SnmpHashNode ** hashTable;

} SnmpTfxView;

typedef struct _SnmpTfxInfo {

    UINT          numViews;
    SnmpTfxView * tfxViews;

} SnmpTfxInfo;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private prototypes                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

VOID
ValidateQueryList(
    SnmpTfxInfo *        tfxInfo,
    SnmpExtQueryList *   ql,
    UINT                 q,
    RFC1157VarBindList * vbl,
    UINT *               errorStatus,
    UINT *               errorIndex
    );

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private procedures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

VOID
SNMP_FUNC_TYPE
SnmpUtilAsnAnyFree(
    AsnAny * asnAny
    )
{
    switch (asnAny->asnType) {

    case ASN_OBJECTIDENTIFIER:
      SnmpUtilOidFree(&asnAny->asnValue.object);
      // make sure no multiple releases
      asnAny->asnValue.object.idLength = 0;
      asnAny->asnValue.object.ids      = NULL;
      break;

    case ASN_RFC1155_IPADDRESS:
    case ASN_RFC1155_OPAQUE:
    case ASN_OCTETSTRING:
      if (asnAny->asnValue.string.dynamic) {
         SnmpUtilMemFree(asnAny->asnValue.string.stream);
          // make sure no multiple releases
         asnAny->asnValue.string.dynamic = FALSE;
         asnAny->asnValue.string.stream  = NULL;
      }
      break;

    default:
       break;
    }

    asnAny->asnType = ASN_NULL;
}


SNMPAPI
SNMP_FUNC_TYPE
SnmpUtilAsnAnyCpy(
    AsnAny * asnDst,
    AsnAny * asnSrc
    )
{
    switch (asnSrc->asnType) {

    case ASN_OBJECTIDENTIFIER:
        SnmpUtilOidCpy(
            &asnDst->asnValue.object,
            &asnSrc->asnValue.object
            );
        break;

    case ASN_RFC1155_IPADDRESS:
    case ASN_RFC1155_OPAQUE:
    case ASN_OCTETSTRING:
        asnDst->asnValue.string.stream =
            SnmpUtilMemAlloc(asnSrc->asnValue.string.length);

        asnDst->asnValue.string.length = asnSrc->asnValue.string.length;
        memcpy(
            asnDst->asnValue.string.stream,
            asnSrc->asnValue.string.stream,
            asnDst->asnValue.string.length
            );

        asnDst->asnValue.string.dynamic = TRUE;
        break;

    default:
        asnDst->asnValue = asnSrc->asnValue;
        break;
    }

    asnDst->asnType = asnSrc->asnType;
    return TRUE;
}


UINT
OidToHashTableIndex(
    AsnObjectIdentifier * hashOid
    )

/*++

Routine Description:

    Hash function for mib entry access.

Arguments:

    hashOid - object identifer to hash into table position.

Return Values:

    Returns hash table position.

--*/

{
    UINT i;
    UINT j;

    // process each element of the oid
    for (i=0, j=0; i < hashOid->idLength; i++) {

        // determine table position by summing oid
        j = (j * HASH_TABLE_RADIX) + hashOid->ids[i];
    }

    // adjust to within table
    return (j % HASH_TABLE_SIZE);
}


VOID
FreeHashTable(
    SnmpHashNode ** hashTable
    )

/*++

Routine Description:

    Destroys hash table used for accessing views.

Arguments:

    hashTable - table of hash nodes.

Return Values:

    None.

--*/

{
    UINT i;

    SnmpHashNode * nextNode;
    SnmpHashNode * hashNode;

    if (hashTable == NULL) {
        return;
    }

    // free hash table and nodes
    for (i=0; i < HASH_TABLE_SIZE; i++) {

        // point to first item
        hashNode = hashTable[i];

        // find end of node list
        while (hashNode->nextEntry) {

            // save pointer to next node
            nextNode = hashNode->nextEntry;

            // free current node
            SnmpUtilMemFree(hashNode);

            // retrieve next
            hashNode = nextNode;
        }

        // free last node
        SnmpUtilMemFree(hashNode);
    }

    // release table itself
    SnmpUtilMemFree(hashTable);
}


SnmpHashNode **
AllocHashTable(
    SnmpMibView * mibView
    )

/*++

Routine Description:

    Initializes view hash table.

Arguments:

    mibView - mib view information.

Return Values:

    Returns pointer to first entry if successful.

--*/

{
    UINT i;
    UINT j;

    UINT numItems;
    BOOL fInitedOk;

    SnmpMibEntry *  mibEntry;
    SnmpHashNode *  hashNode;
    SnmpHashNode ** hashTable = NULL;

    // validate parameter
    if (mibView == NULL) {
        return NULL;
    }

    // determine how many items in view
    numItems = mibView->viewScalars.len;

    // load the first entry in the view
    mibEntry = mibView->viewScalars.list;

    // allocate hash table using predefined size
    hashTable = (SnmpHashNode **)SnmpUtilMemAlloc(
                                    HASH_TABLE_SIZE * sizeof(SnmpHashNode *)
                                    );

    // make sure table is allocated
    fInitedOk = (hashTable != NULL);

    SNMPDBG((
        SNMP_LOG_VERBOSE,
        "SNMP: TFX: initializing hash table 0x%08lx (%d items).\n",
        hashTable,
        numItems
        ));

    // process each item in the subagent's supported view
    for (i = 0; (i < numItems) && fInitedOk; i++, mibEntry++) {

        // hash into table index
        j = OidToHashTableIndex(&mibEntry->mibOid);

        // check if table entry taken
        if (hashTable[j] == NULL) {

            // allocate new node
            hashNode = (SnmpHashNode *)SnmpUtilMemAlloc(
                            sizeof(SnmpHashNode)
                            );

            // save hash node
            hashTable[j] = hashNode;

            SNMPDBG((
                SNMP_LOG_VERBOSE,
                "SNMP: TFX: adding hash node 0x%08lx to empty slot %d (0x%08lx).\n",
                hashNode, j, mibEntry
                ));

        } else {

            // point to first item
            hashNode = hashTable[j];

            // find end of node list
            while (hashNode->nextEntry) {
                hashNode = hashNode->nextEntry;
            }

            // allocate new node entry
            hashNode->nextEntry = (SnmpHashNode *)SnmpUtilMemAlloc(
                                        sizeof(SnmpHashNode)
                                        );

            // re-init node to edit below
            hashNode = hashNode->nextEntry;

            SNMPDBG((
                SNMP_LOG_VERBOSE,
                "SNMP: TFX: adding hash node 0x%08lx to full slot %d (0x%08lx).\n",
                hashNode, j, mibEntry
                ));
        }

        // make sure allocation succeeded
        fInitedOk = (hashNode != NULL);

        if (fInitedOk) {

            // fill in node values
            hashNode->mibEntry = mibEntry;
        }
    }

    SNMPDBG((
        SNMP_LOG_VERBOSE,
        "SNMP: TFX: %s initialized hash table 0x%08lx.\n",
        fInitedOk ? "successfully" : "unsuccessfully",
        hashTable
        ));

    if (!fInitedOk) {

        // free view hash table
        FreeHashTable(hashTable);

        // reinitialize
        hashTable = NULL;
    }

    return hashTable;
}


VOID
OidToMibEntry(
    AsnObjectIdentifier * hashOid,
    SnmpHashNode **       hashTable,
    SnmpMibEntry **       mibEntry
    )

/*++

Routine Description:

    Returns mib entry associated with given object identifier.

Arguments:

    hashOid   - oid to convert to table index.
    hashTable - table to look up entry.
    mibEntry  - pointer to mib entry information.

Return Values:

    None.

--*/

{
    UINT i;
    SnmpHashNode * hashNode;
    AsnObjectIdentifier newOid;

    // create index
    i = OidToHashTableIndex(hashOid);

    SNMPDBG((
        SNMP_LOG_VERBOSE,
        "SNMP: TFX: searching hash table 0x%08lx slot %d for %s.\n",
        hashTable, i, SnmpUtilOidToA(hashOid)
        ));

    // retrieve node
    hashNode = hashTable[i];

    // initialize
    *mibEntry = NULL;

    // search list
    while (hashNode) {

        SNMPDBG((
            SNMP_LOG_VERBOSE,
            "SNMP: TFX: searching hash node 0x%08lx (mibe=0x%08lx) - %s.\n",
            hashNode, hashNode->mibEntry,
			SnmpUtilOidToA(&hashNode->mibEntry->mibOid)
            ));

        // retrieve mib identifier
        newOid = hashNode->mibEntry->mibOid;

        // make sure that the oid matches
        if (!SnmpUtilOidCmp(&newOid, hashOid)) {

            SNMPDBG((
                SNMP_LOG_VERBOSE,
                "SNMP: TFX: returning mib entry 0x%08lx.\n",
                hashNode->mibEntry
                ));

            // return node data
            *mibEntry = hashNode->mibEntry;
            return;

        }

        // check next node
        hashNode = hashNode->nextEntry;
    }
}


int
ValidateInstanceIdentifier(
    AsnObjectIdentifier * indexOid,
    SnmpMibTable *        tableInfo
    )

/*++

Routine Description:

    Validates that oid can be successfully parsed into index entries.

Arguments:

    indexOid  - object indentifier of potential index.
    tableInfo - information describing conceptual table.

Return Values:

    Returns the comparision between the length of the indexOid
	and the cumulated lengths of all the indices of the table tableInfo.
	{-1, 0, 1}

--*/

{
    UINT i = 0;
    UINT j = 0;

    int nComp;

    BOOL fFixed;
    BOOL fLimit;
    BOOL fIndex;

    SnmpMibEntry * mibEntry;

    SNMPDBG((
        SNMP_LOG_VERBOSE,
        "SNMP: TFX: validating index %s via table 0x%08lx.\n",
        SnmpUtilOidToA(indexOid), tableInfo
        ));

    // see if the table indices are specified
    fIndex = (tableInfo->tableIndices != NULL);

    // scan mib entries of table indices ensuring match of given oid
    for (i = 0; (i < tableInfo->numIndices) && (j < indexOid->idLength); i++) {

        // get mib entry from table or directly
        mibEntry = fIndex ?  tableInfo->tableIndices[i]
                          : &tableInfo->tableEntry[i+1]
                          ;

        // determine type
        switch (mibEntry->mibType) {

        // variable length types
        case ASN_OBJECTIDENTIFIER:
        case ASN_RFC1155_OPAQUE:
        case ASN_OCTETSTRING:

            // check whether this is a fixed length variable or not
            fLimit = (mibEntry->mibMinimum || mibEntry->mibMaximum);
            fFixed = (fLimit && (mibEntry->mibMinimum == mibEntry->mibMaximum));

            // validate
            if (fFixed) {

                // increment fixed length
                j += mibEntry->mibMaximum;

            } else if (fLimit) {

                // check whether the length of the variable is valid
                if (((INT)indexOid->ids[j] >= mibEntry->mibMinimum) &&
                    ((INT)indexOid->ids[j] <= mibEntry->mibMaximum)) {

                    // increment given length
                    j += (indexOid->ids[j] + 1);

                } else {

                    // invalidate
                    j = INVALID_INDEX;
                }

            } else {

                // increment given length
                j += (indexOid->ids[j] + 1);
            }

            break;

        // implicit fixed size
        case ASN_RFC1155_IPADDRESS:
            // increment
            j += 4;
            break;

        case ASN_RFC1155_COUNTER:
        case ASN_RFC1155_GAUGE:
        case ASN_RFC1155_TIMETICKS:
        case ASN_INTEGER:
            // increment
            j++;
            break;

        default:
            // invalidate
            j = INVALID_INDEX;
            break;
        }
    }

	if (i<tableInfo->numIndices)
		nComp = -1;
	else if (j < indexOid->idLength)
		nComp = 1;
	else if (j > indexOid->idLength)
		nComp = -1;
	else
		nComp = 0;

    SNMPDBG((
        SNMP_LOG_VERBOSE,
        "SNMP: TFX: ValidateInstanceIdentifier; OID %s %s table indices.\n",
		SnmpUtilOidToA(indexOid),
		nComp > 0 ? "over-covers" : nComp < 0 ? "shorter than" : "matches"
		));

    return nComp;
}


VOID
ValidateAsnAny(
    AsnAny *       asnAny,
    SnmpMibEntry * mibEntry,
    UINT           mibAction,
    UINT *         errorStatus
    )

/*++

Routine Description:

    Validates asn value with given mib entry.

Arguments:

    asnAny      - value to set.
    mibEntry    - mib information.
    mibAction   - mib action to be taken.
    errorStatus - used to indicate success or failure.

Return Values:

    None.

--*/

{
    BOOL fLimit;
    BOOL fFixed;

    INT asnLen;

    BOOL fOk = TRUE;

    SNMPDBG((
        SNMP_LOG_VERBOSE,
        "SNMP: TFX: validating value for %s request using entry 0x%08lx.\n",
        (mibAction == MIB_ACTION_SET) ? "write" : "read", mibEntry
        ));

    // validating gets is trivial
    if (mibAction != MIB_ACTION_SET) {

        // validate instrumentation info
        if ((mibEntry->mibGetBufLen == 0) ||
            (mibEntry->mibGetFunc == NULL) ||
           !(mibEntry->mibAccess & MIB_ACCESS_READ)) {

            // variable is not available for reading
            *errorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;

            SNMPDBG((
                SNMP_LOG_VERBOSE,
                "SNMP: TFX: entry 0x%08lx not read-enabled.\n",
                mibEntry
                ));

            return; // bail...
        }

    } else {

        // validate instrumentation info
        if ((mibEntry->mibSetBufLen == 0) ||
            (mibEntry->mibSetFunc == NULL) ||
           !(mibEntry->mibAccess & MIB_ACCESS_WRITE)) {

            // variable is not avaiable for writing
            *errorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;

            SNMPDBG((
                SNMP_LOG_VERBOSE,
                "SNMP: TFX: entry 0x%08lx not write-enabled.\n",
                mibEntry
                ));

            return; // bail...
        }

        // check whether this is a fixed length variable or not
        fLimit = (mibEntry->mibMinimum || mibEntry->mibMaximum);
        fFixed = (fLimit && (mibEntry->mibMinimum == mibEntry->mibMaximum));

        // determine value type
        switch (asnAny->asnType) {

        // variable length types
        case ASN_OBJECTIDENTIFIER:

            // retrieve the objects id length
            asnLen = asnAny->asnValue.object.idLength;

            // fixed?
            if (fFixed) {

                // make sure the length is correct
                fOk = (asnLen == mibEntry->mibMaximum);

            } else if (fLimit) {

                // make sure the length is correct
                fOk = ((asnLen >= mibEntry->mibMinimum) &&
                       (asnLen <= mibEntry->mibMaximum));
            }

            break;

        case ASN_RFC1155_OPAQUE:
        case ASN_OCTETSTRING:

            // retrieve the arbitrary length
            asnLen = asnAny->asnValue.string.length;

            // fixed?
            if (fFixed) {

                // make sure the length is correct
                fOk = (asnLen == mibEntry->mibMaximum);

            } else if (fLimit) {

                // make sure the length is correct
                fOk = ((asnLen >= mibEntry->mibMinimum) &&
                       (asnLen <= mibEntry->mibMaximum));
            }

            break;

        case ASN_RFC1155_IPADDRESS:

            // make sure the length is correct
            fOk = (asnAny->asnValue.address.length == 4);
            break;

        case ASN_INTEGER:

            // limited?
            if (fLimit) {

                // make sure the value in range
                fOk = ((asnAny->asnValue.number >= mibEntry->mibMinimum) &&
                       (asnAny->asnValue.number <= mibEntry->mibMaximum));
            }

            break;

        default:
            // error...
            fOk = FALSE;
            break;
        }
    }

    SNMPDBG((
        SNMP_LOG_VERBOSE,
        "SNMP: TFX: value is %s using entry 0x%08lx.\n",
        fOk ? "valid" : "invalid", mibEntry
        ));

    // report results
    *errorStatus = fOk
                    ? SNMP_ERRORSTATUS_NOERROR
                    : SNMP_ERRORSTATUS_BADVALUE
                    ;
}


VOID
FindMibEntry(
    SnmpTfxInfo *    tfxInfo,
    RFC1157VarBind * vb,
    SnmpMibEntry **  mibEntry,
    UINT *           mibAction,
    SnmpTableXlat ** tblXlat,
    UINT             vlIndex,
    UINT *           errorStatus
    )

/*++

Routine Description:

    Locates mib entry associated with given varbind.

Arguments:

    tfxInfo     - context info.
    vb          - variable to locate.
    mibEntry    - mib entry information.
    mibAction   - mib action (may be updated).
    tblXlat     - table translation info.
    vlIndex     - index into view list.
    errorStatus - used to indicate success or failure.

Return Values:

    None.

--*/

{
    UINT i;
    UINT j;

    UINT newIndex;
    UINT numItems;
    UINT numTables;

    BOOL fFoundOk;
	int  indexComp;

    AsnObjectIdentifier hashOid;
    AsnObjectIdentifier indexOid;
    AsnObjectIdentifier * viewOid;

    SnmpMibTable  * viewTables;
    SnmpTfxView   * tfxView;

    SnmpMibEntry  * newEntry = NULL;
    SnmpTableXlat * newXlat  = NULL;

    // initialize
    *mibEntry = NULL;
    *tblXlat  = NULL;

    // retrieve view information
    tfxView = &tfxInfo->tfxViews[vlIndex];

    // retrieve view object identifier
    viewOid = &tfxView->mibView->viewOid;

	SNMPDBG((
		SNMP_LOG_VERBOSE,
		"SNMP: TFX: FindMibEntry; comp(%s, ",
		SnmpUtilOidToA(&vb->name)
		));

	SNMPDBG((
		SNMP_LOG_VERBOSE,
		"%s).\n",
		SnmpUtilOidToA(viewOid)
		));

    // if the prefix exactly matchs it is root oid
    if (!SnmpUtilOidCmp(&vb->name, viewOid)) {
        SNMPDBG((SNMP_LOG_VERBOSE, "SNMP: TFX: requested oid is root.\n"));
        *errorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
        return;
    }

    // if the prefix does not match it is not in hash table
    if (SnmpUtilOidNCmp(&vb->name, viewOid, viewOid->idLength)) {
        SNMPDBG((SNMP_LOG_TRACE, "SNMP: TFX: requested oid not in view.\n"));
        *errorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
        return;
    }

    // construct new oid sans root prefix
    hashOid.ids = &vb->name.ids[viewOid->idLength];
    hashOid.idLength = vb->name.idLength - viewOid->idLength;

    // retrieve mib entry and index via hash table
    OidToMibEntry(&hashOid, tfxView->hashTable, &newEntry);

    // check if mib entry found
    fFoundOk = (newEntry != NULL);

    // try mib tables
    if (!fFoundOk) {

        SNMPDBG((
            SNMP_LOG_VERBOSE,
            "SNMP: TFX: searching mib tables for %s.\n",
            SnmpUtilOidToA(&hashOid)
            ));

        // retrieve mib table information
        numTables  = tfxView->mibView->viewTables.len;
        viewTables = tfxView->mibView->viewTables.list;

        // scan mib tables for a match to the given oid
        for (i=0; (i < numTables) && !fFoundOk; i++, viewTables++) {

            // retrieve entry for table entry
            numItems = viewTables->numColumns;
            newEntry = viewTables->tableEntry;

            if (!SnmpUtilOidNCmp(
                    &hashOid,
                    &newEntry->mibOid,
                    newEntry->mibOid.idLength)) {

                SNMPDBG((
                    SNMP_LOG_VERBOSE,
                    "SNMP: TFX: searching table 0x%08lx (%s).\n",
                    newEntry, SnmpUtilOidToA(&newEntry->mibOid)
                    ));

                // next
                ++newEntry;

                // scan mib table entries for a match
                for (j=0; j < numItems; j++, newEntry++) {

                    // compare with oid of table entry
                    if (!SnmpUtilOidNCmp(
                            &hashOid,
                            &newEntry->mibOid,
                            newEntry->mibOid.idLength)) {

                        SNMPDBG((
                            SNMP_LOG_VERBOSE,
                            "SNMP: TFX: validating mib entry 0x%08lx (%s).\n",
                            newEntry, SnmpUtilOidToA(&newEntry->mibOid)
                            ));

                         // construct new oid sans table entry prefix
                        indexOid.ids =
                            &hashOid.ids[newEntry->mibOid.idLength];
                        indexOid.idLength =
                            hashOid.idLength - newEntry->mibOid.idLength;

                        // verify rest of oid is valid index
                        indexComp = ValidateInstanceIdentifier(
                                        &indexOid,
                                        viewTables
                                        );
						fFoundOk = (indexComp < 0 && *mibAction == MIB_ACTION_GETNEXT) ||
								   (indexComp == 0);

                        // is index?
                        if (fFoundOk) {

                            SNMPDBG((
                                SNMP_LOG_VERBOSE,
                                "SNMP: TFX: saving index oid %s.\n",
                                SnmpUtilOidToA(&indexOid)
                                ));

                            // allocate table translation structure
                            newXlat = (SnmpTableXlat *)SnmpUtilMemAlloc(
                                            sizeof(SnmpTableXlat)
                                            );

                            // save table information
                            newXlat->txInfo  = viewTables;
                            newXlat->txIndex = i;

                            // copy index object identifier
                            SnmpUtilOidCpy(&newXlat->txOid, &indexOid);

                            break; // finished...
                        }
                    }
                }
            }
        }

    } else {

        UINT newOff;

        SNMPDBG((
            SNMP_LOG_VERBOSE,
            "SNMP: TFX: searching mib tables for %s.\n",
            SnmpUtilOidToA(&hashOid)
            ));

        // retrieve mib table information
        numTables  = tfxView->mibView->viewTables.len;
        viewTables = tfxView->mibView->viewTables.list;

        // scan mib tables for an entry in table
        for (i=0; i < numTables; i++, viewTables++) {

            // columns are positioned after entry
            if (newEntry > viewTables->tableEntry) {

                // calculate the difference between pointers
                newOff = (UINT)newEntry - (UINT)viewTables->tableEntry;

                // calculate table offset
                newOff /= sizeof(SnmpMibEntry);

                // determine if entry within region
                if (newOff <= viewTables->numColumns) {

                    // allocate table translation structure
                    newXlat = (SnmpTableXlat *)SnmpUtilMemAlloc(
                                    sizeof(SnmpTableXlat)
                                    );

                    // save table information
                    newXlat->txInfo  = viewTables;
                    newXlat->txIndex = i;

                    // initialize index oid
                    newXlat->txOid.ids = NULL;
                    newXlat->txOid.idLength = 0;

                    SNMPDBG((
                        SNMP_LOG_TRACE,
                        "SNMP: TFX: mib entry is in table 0x%08lx (%s).\n",
                        viewTables->tableEntry,
                        SnmpUtilOidToA(&viewTables->tableEntry->mibOid)
                        ));

                    break; // finished...
                }
            }
        }
    }

    // found entry?
    if (fFoundOk) {

        SNMPDBG((
            SNMP_LOG_VERBOSE,
            "SNMP: TFX: FindMibEntry; found %s\n",
			SnmpUtilOidToA(&newEntry->mibOid)
            ));
        // pass back results
        *mibEntry = newEntry;
        *tblXlat  = newXlat;

    } else {

        SNMPDBG((
            SNMP_LOG_VERBOSE,
            "SNMP: TFX: unable to exactly match varbind.\n"
            ));

        // unable to locate varbind in mib table
        *errorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
    }
}


VOID
FindNextMibEntry(
    SnmpTfxInfo *     tfxInfo,
    RFC1157VarBind *  vb,
    SnmpMibEntry **   mibEntry,
    UINT *            mibAction,
    SnmpTableXlat **  tblXlat,
    UINT              vlIndex,
    UINT *            errorStatus
    )

/*++

Routine Description:

    Locates next mib entry associated with given varbind.

Arguments:

    tfxInfo     - context info.
    vb          - variable to locate.
    mibEntry    - mib entry information.
    mibAction   - mib action (may be updated).
    tblXlat     - table translation info.
    vlIndex     - index into view list.
    errorStatus - used to indicate success or failure.

Return Values:

    None.

--*/

{
    UINT mibStatus;

    SnmpMibEntry  * newEntry = NULL;
    SnmpTableXlat * newXlat  = NULL;

    SnmpTfxView * tfxView;

    // table?
    if (*tblXlat) {
        SNMPDBG((SNMP_LOG_VERBOSE, "SNMP: TFX: querying table.\n"));
        return; // simply query table...
    }

    // retrieve view information
    tfxView = &tfxInfo->tfxViews[vlIndex];

    // retrieve entry
    newEntry = *mibEntry;

    // initialize
    *mibEntry = NULL;
    *tblXlat  = NULL;

    // continuing?
    if (newEntry) {
        // next
        ++newEntry;
        SNMPDBG((
            SNMP_LOG_TRACE,
            "SNMP: TFX: searching mib at next entry 0x%08lx (%s).\n",
            newEntry,
            SnmpUtilOidToA(&newEntry->mibOid)
            ));
    } else {
        // retrieve first mib entry in supported view
        newEntry = tfxView->mibView->viewScalars.list;
        SNMPDBG((
            SNMP_LOG_TRACE,
            "SNMP: TFX: searching mib at first entry 0x%08lx.\n",
            newEntry
            ));
    }

    // initialize status to start search
    mibStatus = SNMP_ERRORSTATUS_NOSUCHNAME;

    // scan
   for (;; newEntry++) {

	   SNMPDBG((
		   SNMP_LOG_VERBOSE,
		   "SNMP: TFX: FindNextMibEntry; scanning view %s ",
		   SnmpUtilOidToA(&tfxView->mibView->viewOid)
		   ));

	   SNMPDBG((
		   SNMP_LOG_VERBOSE,
		   " scalar %s.\n",
		   SnmpUtilOidToA(&newEntry->mibOid)
		   ));

        // if last entry then we stop looking
        if (newEntry->mibType == ASN_PRIVATE_EOM) {

            SNMPDBG((SNMP_LOG_TRACE, "SNMP: TFX: encountered end of mib.\n"));

            *errorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
            return; // bail...
        }

        // skip over place holder mib entries
        if (newEntry->mibType != ASN_PRIVATE_NODE) {

            // validate asn value against info in mib entry
            ValidateAsnAny(&vb->value, newEntry, *mibAction, &mibStatus);

            // bail if we found a valid entry...
            if (mibStatus == SNMP_ERRORSTATUS_NOERROR) {
                break;
            }
        }
    }

    // retrieved an entry but is it in a table?
    if (mibStatus == SNMP_ERRORSTATUS_NOERROR) {

        UINT i;
        UINT newOff;
        UINT numTables;

        SnmpMibTable * viewTables;

        SNMPDBG((
            SNMP_LOG_TRACE,
            "SNMP: TFX: mib entry 0x%08lx found (%s).\n",
            newEntry, SnmpUtilOidToA(&newEntry->mibOid)
            ));

        // retrieve table information from view
        numTables  = tfxView->mibView->viewTables.len;
        viewTables = tfxView->mibView->viewTables.list;

        // scan mib tables for an entry in table
        for (i=0; i < numTables; i++, viewTables++) {

            // columns are positioned after entry
            if (newEntry > viewTables->tableEntry) {

                // calculate the difference between pointers
                newOff = (UINT)newEntry - (UINT)viewTables->tableEntry;

                // calculate table offset
                newOff /= sizeof(SnmpMibEntry);

                // determine if entry within region
                if (newOff <= viewTables->numColumns) {

                    // allocate table translation structure
                    newXlat = (SnmpTableXlat *)SnmpUtilMemAlloc(
                                    sizeof(SnmpTableXlat)
                                    );

                    // save table information
                    newXlat->txInfo  = viewTables;
                    newXlat->txIndex = i;

                    // initialize index oid
                    newXlat->txOid.ids = NULL;
                    newXlat->txOid.idLength = 0;

                    SNMPDBG((
                        SNMP_LOG_TRACE,
                        "SNMP: TFX: mib entry is in table 0x%08lx (%s).\n",
                        viewTables->tableEntry,
                        SnmpUtilOidToA(&viewTables->tableEntry->mibOid)
                        ));

                    break; // finished...
                }
            }
        }

        // pass back results
        *mibEntry  = newEntry;
        *tblXlat   = newXlat;

        // update mib action of scalar getnext
        if (!newXlat && (*mibAction == MIB_ACTION_GETNEXT)) {

            *mibAction = MIB_ACTION_GET;

            SNMPDBG((
                SNMP_LOG_TRACE,
                "SNMP: TFX: altered mib action to MIB_ACTION_GET.\n"
                ));
        }
    }

    // pass back status
    *errorStatus = mibStatus;
}


VOID
FindAnyMibEntry(
    SnmpTfxInfo *     tfxInfo,
    RFC1157VarBind *  vb,
    SnmpMibEntry **   mibEntry,
    UINT *            mibAction,
    SnmpTableXlat **  tblXlat,
    UINT              vlIndex,
    UINT *            errorStatus
    )

/*++

Routine Description:

    Locates any mib entry associated with given varbind.

Arguments:

    tfxInfo     - context info.
    vb          - variable to locate.
    mibEntry    - mib entry information.
    mibAction   - mib action (may be updated).
    tblXlat     - table translation info.
    vlIndex     - index into view list.
    errorStatus - used to indicate success or failure.

Return Values:

    None.

--*/

{
    BOOL fExact;
    BOOL fBefore;

    SnmpTfxView * tfxView;

    // retrieve view information
    tfxView = &tfxInfo->tfxViews[vlIndex];

	SNMPDBG((
		SNMP_LOG_VERBOSE,
		"SNMP: TFX: FindAnyMibEntry; comp(%s, ",
		SnmpUtilOidToA(&vb->name)
		));
	SNMPDBG((
		SNMP_LOG_VERBOSE,
		"%s[, %d]).\n",
		SnmpUtilOidToA(&tfxView->mibView->viewOid),
		tfxView->mibView->viewOid.idLength
		));

    // look for oid before view
    fBefore = (0 > SnmpUtilOidNCmp(
                         &vb->name,
                         &tfxView->mibView->viewOid,
                         tfxView->mibView->viewOid.idLength
                         ));

    // look for exact match
    fExact = !fBefore && !SnmpUtilOidCmp(
                            &vb->name,
                            &tfxView->mibView->viewOid
                            );

	SNMPDBG((
		SNMP_LOG_VERBOSE,
		"SNMP: TFX: fBefore=%d fExact=%d\n",
		fBefore,
		fExact
		));
    // check for random oid...
    if (!fBefore && !fExact) {

        AsnObjectIdentifier relOid;
        AsnObjectIdentifier * viewOid;
        SnmpMibEntry * newEntry = NULL;

        // point to the first item in the list
        newEntry = tfxView->mibView->viewScalars.list;

        SNMPDBG((
            SNMP_LOG_TRACE,
            "SNMP: TFX: linear search from first entry 0x%08lx.\n",
            newEntry
            ));

        // retrieve the view object identifier
        viewOid = &tfxView->mibView->viewOid;

        // construct new oid sans root prefix
        relOid.ids = &vb->name.ids[viewOid->idLength];
        relOid.idLength = vb->name.idLength - viewOid->idLength;

        // scan mib entries
        while ((newEntry->mibType != ASN_PRIVATE_EOM) &&
               (SnmpUtilOidCmp(&relOid, &newEntry->mibOid) > 0)) {

            SNMPDBG((
                SNMP_LOG_VERBOSE,
                "SNMP: TFX: skipping %s.\n",
                SnmpUtilOidToA(&newEntry->mibOid)
                ));

            // next
            newEntry++;
        }

        // if last entry then we stop looking
        if (newEntry->mibType == ASN_PRIVATE_EOM) {

            SNMPDBG((
                SNMP_LOG_TRACE,
                "SNMP: TFX: encountered end of mib.\n"
                ));

            *errorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
            return; // bail...
        }

        // backup to find next
        *mibEntry = --newEntry;
        *tblXlat  = NULL;

        // find next
        FindNextMibEntry(
               tfxInfo,
               vb,
               mibEntry,
               mibAction,
               tblXlat,
               vlIndex,
               errorStatus
               );

    } else {

        // initialize
        *mibEntry = NULL;
        *tblXlat  = NULL;

        SNMPDBG((
            SNMP_LOG_TRACE,
            "SNMP: TFX: searching for first entry.\n"
            ));

        // find next
        FindNextMibEntry(
               tfxInfo,
               vb,
               mibEntry,
               mibAction,
               tblXlat,
               vlIndex,
               errorStatus
               );
		SNMPDBG((
			SNMP_LOG_VERBOSE,
			"SNMP: TFX: FindAnyMibEntry; error %d on %s(.",
			*errorStatus,
			SnmpUtilOidToA(&tfxInfo->tfxViews[vlIndex].mibView->viewOid)
			));
		SNMPDBG((
			SNMP_LOG_VERBOSE,
			"%s).\n",
			SnmpUtilOidToA(&(*mibEntry)->mibOid)
			));
    }
}


VOID
VarBindToMibEntry(
    SnmpTfxInfo *    tfxInfo,
    RFC1157VarBind * vb,
    SnmpMibEntry **  mibEntry,
    UINT *           mibAction,
    SnmpTableXlat ** tblXlat,
    UINT             vlIndex,
    UINT *           errorStatus
    )

/*++

Routine Description:

    Locates mib entry associated with given varbind.

Arguments:

    tfxInfo     - context information.
    vb          - variable to locate.
    mibEntry    - mib entry information.
    mibAction   - mib action (may be updated).
    tblXlat     - table translation info.
    vlIndex     - index into view list.
    errorStatus - used to indicate success or failure.

Return Values:

    None.

--*/

{
    BOOL fAnyOk;
    BOOL fFoundOk;
    BOOL fErrorOk;

    // determine whether we need exact match
    fAnyOk = (*mibAction == MIB_ACTION_GETNEXT);

    // find match
    FindMibEntry(
        tfxInfo,
        vb,
        mibEntry,
        mibAction,
        tblXlat,
        vlIndex,
        errorStatus
        );
	
	SNMPDBG((
		SNMP_LOG_VERBOSE,
		"SNMP: TFX: VarBindToMibEntry; errorStatus=%d.\n",
		*errorStatus
		));

    // get next?
    if (fAnyOk) {

        // search again
        if (*errorStatus == SNMP_ERRORSTATUS_NOERROR) {

            // find next entry
            FindNextMibEntry(
                tfxInfo,
                vb,
                mibEntry,
                mibAction,
                tblXlat,
                vlIndex,
                errorStatus
                );

        } else if (*errorStatus == SNMP_ERRORSTATUS_NOSUCHNAME) {

            // find any entry
            FindAnyMibEntry(
                tfxInfo,
                vb,
                mibEntry,
                mibAction,
                tblXlat,
                vlIndex,
                errorStatus
                );
        }

    } else if (*errorStatus == SNMP_ERRORSTATUS_NOERROR) {

        // validate asn value against mib entry information
        ValidateAsnAny(&vb->value, *mibEntry, *mibAction, errorStatus);

        // make sure valid before passing back entry
        if (*errorStatus != SNMP_ERRORSTATUS_NOERROR) {

            // table entry?
            if (*tblXlat) {

                SNMPDBG((
                    SNMP_LOG_VERBOSE,
                    "SNMP: TFX: freeing index info (%s).\n",
                    SnmpUtilOidToA(&(*tblXlat)->txOid)
                    ));

                // free index oid
                SnmpUtilOidFree(&(*tblXlat)->txOid);

                // free table info
                SnmpUtilMemFree(*tblXlat);

            }

            // nullify results
            *mibEntry = NULL;
            *tblXlat  = NULL;
        }
    }
}

BOOL
CheckUpdateIndex(
	AsnObjectIdentifier	*indexOid,
	UINT				nStartFrom,
	UINT				nExpecting
	)
/*++
Routine Description:

	Checks if an index OID contains all the components expected.
	If not, the index is updated to point before the very first
	OID requested.

Arguments:
	
	indexOid  - pointer to the index to be checked.
	nStartFrom - the point from where the index is checked.
	nExpecting  - the index should have at least expectTo components from startFrom.

Return value:
	TRUE if index was valid or has been updated successfully.
	FALSE otherwise (index was shorter then expected and all filled with 0s).

--*/
{
	int i;

	SNMPDBG((
		SNMP_LOG_VERBOSE,
		"SNMP: TFX: CheckUpdateIndex; checking %s.\n",
		SnmpUtilOidToA(indexOid)
		));

	if (indexOid->idLength >= nStartFrom + nExpecting)
	{
		SNMPDBG((
			SNMP_LOG_VERBOSE,
			"SNMP: TFX: CheckUpdateIndex; valid, unchanged.\n"
			));

		return TRUE;
	}

	for (i = indexOid->idLength-1; i >= (int)nStartFrom; i--)
	{
		if (indexOid->ids[i] > 0)
		{
			indexOid->ids[i]--;
			indexOid->idLength = i+1;

			SNMPDBG((
				SNMP_LOG_VERBOSE,
				"SNMP: TFX: CheckUpdateIndex; valid, changed to %s.\n",
				SnmpUtilOidToA(indexOid)
				));

			return TRUE;
		}
	}

	SNMPDBG((
		SNMP_LOG_VERBOSE,
		"SNMP: TFX: CheckUpdateIndex; invalid, to be removed.\n"
		));

	return FALSE;
}

VOID
ParseInstanceIdentifier(
    SnmpTableXlat * tblXlat,
    AsnAny *        objArray,
    UINT            mibAction
    )

/*++

Routine Description:

    Converts table index oid into object array.

Arguments:

    tblXlat   - table translation information.
    objArray  - instrumentation object array.
    mibAction - action requested of subagent.

Return Values:

    None.

--*/

{
    UINT i;
    UINT j;
    UINT k;
    UINT l;
    UINT m;

    BOOL fFixed;
    BOOL fLimit;
    BOOL fIndex;
    BOOL fEmpty;
	BOOL fExceed;

    UINT numItems;

    SnmpMibEntry * mibEntry;
    AsnObjectIdentifier * indexOid;

    LPDWORD lpIpAddress;

    // retrieve index oid
    indexOid = &tblXlat->txOid;

    // is this valid oid
    fEmpty = (indexOid->idLength == 0);

    SNMPDBG((
        SNMP_LOG_TRACE,
        "SNMP: TFX: converting index %s to obj array via table 0x%08lx.\n",
        fEmpty ? "<tbd>" : SnmpUtilOidToA(indexOid), tblXlat->txInfo
        ));

    // retrieve root entry and entry count
    numItems = tblXlat->txInfo->numIndices;

    // see if the table indices are specified
    fIndex = (tblXlat->txInfo->tableIndices != NULL);
	fExceed = FALSE;
    // scan mib entries of table indices
    for (i=0, j=0; (i < numItems) && (j < indexOid->idLength); i++) {

        // get mib entry from table or directly
        mibEntry = fIndex ?  tblXlat->txInfo->tableIndices[i]
                          : &tblXlat->txInfo->tableEntry[i+1]
                          ;

        // retrieve array index
        k = (mibAction == MIB_ACTION_SET)
                ? (UINT)(CHAR)mibEntry->mibSetBufOff
                : (UINT)(CHAR)mibEntry->mibGetBufOff
                ;

        // determine type
        switch (mibEntry->mibType) {

        // variable length types
        case ASN_OBJECTIDENTIFIER:

            // check whether this is a fixed length variable or not
            fLimit = (mibEntry->mibMinimum || mibEntry->mibMaximum);
            fFixed = (fLimit && (mibEntry->mibMinimum == mibEntry->mibMaximum));

            // validate
            if (fFixed) {

                // fixed length; indexOid should have at least l components more
                l = mibEntry->mibMaximum;

				if (!CheckUpdateIndex(indexOid, j, l))
				{
					// out from switch and for
					j+=l;
					break;
				}

            } else {
                // variable length
                l = indexOid->ids[j];

				if (!CheckUpdateIndex(indexOid, j, l+1))
				{
					// out from switch and for
					j+=l+1;
					break;
				}
				j++;
            }

			// copy the type of asn variable
			objArray[k].asnType = mibEntry->mibType;

            // allocate object using length above
            objArray[k].asnValue.object.idLength = l;
            objArray[k].asnValue.object.ids = SnmpUtilMemAlloc(
                objArray[k].asnValue.object.idLength * sizeof(UINT)
                );

            // transfer data
            for (m=0; m < l; m++, j++) {

				// transfer oid element to buffer
				if (!fExceed && j < indexOid->idLength)
				{
					objArray[k].asnValue.object.ids[m] = indexOid->ids[j];
				}
				else
				{
					if (!fExceed)
						fExceed = TRUE;
					// this certainly is the last index from the request
				}

				if (fExceed)
				{
					objArray[k].asnValue.object.ids[m] = (UINT)(-1);
				}
            }

            break;

        case ASN_RFC1155_OPAQUE:
        case ASN_OCTETSTRING:

            // check whether this is a fixed length variable or not
            fLimit = (mibEntry->mibMinimum || mibEntry->mibMaximum);
            fFixed = (fLimit && (mibEntry->mibMinimum == mibEntry->mibMaximum));

            // validate
            if (fFixed) {

                // fixed length
                l = mibEntry->mibMaximum;

				if (!CheckUpdateIndex(indexOid, j, l))
				{
					// out from switch and for
					j+=l;
					break;
				}

            } else {

                // variable length
                l = indexOid->ids[j];

				if (!CheckUpdateIndex(indexOid, j, l+1))
				{
					j+=l+1;
					break;
				}
				j++;
            }

			// copy the type of asn variable
			objArray[k].asnType = mibEntry->mibType;

            // allocate object
            objArray[k].asnValue.string.length = l;
            objArray[k].asnValue.string.dynamic = TRUE;
            objArray[k].asnValue.string.stream = SnmpUtilMemAlloc(
                objArray[k].asnValue.string.length * sizeof(CHAR)
                );

            // transfer data
            for (m=0; m < l; m++, j++) {

                // convert oid element to character
				if (j < indexOid->idLength)
				{
					if (!fExceed && indexOid->ids[j] <= (UCHAR)(-1))
						objArray[k].asnValue.string.stream[m] = (BYTE)(indexOid->ids[j]);
					else
						fExceed=TRUE;
				}
				else
				{
					if (!fExceed)
						fExceed = TRUE;
					// this certainly is the last index from the request
				}

				if (fExceed)
				{
					objArray[k].asnValue.string.stream[m] = (UCHAR)(-1);
				}
            }

            break;

        // implicit fixed size
        case ASN_RFC1155_IPADDRESS:

			if (!CheckUpdateIndex(indexOid, j, 4))
			{
				// out from switch and for
				j+=4;
				break;
			}

			// copy the type of asn variable
			objArray[k].asnType = mibEntry->mibType;

            // allocate object
            objArray[k].asnValue.string.length = 4;
            objArray[k].asnValue.string.dynamic = TRUE;
            objArray[k].asnValue.string.stream = SnmpUtilMemAlloc(
                objArray[k].asnValue.string.length * sizeof(CHAR)
                );

            // cast to dword in order to manipulate ip address
            lpIpAddress = (LPDWORD)objArray[k].asnValue.string.stream;

            // transfer data into buffer
			for (m=0; m<4; m++, j++)
			{
				*lpIpAddress <<= 8;

				if (!fExceed && j < indexOid->idLength)
				{
					if (indexOid->ids[j] <= (UCHAR)(-1))
						*lpIpAddress += indexOid->ids[j];
					else
						fExceed = TRUE;
				}
				else
				{
					if (!fExceed)
						fExceed = TRUE;
					// this certainly is the last index from the request
				}
				if (fExceed)
				{
					*lpIpAddress += (UCHAR)(-1);
				}
			}

            // ensure network byte order
            *lpIpAddress = htonl(*lpIpAddress);

            break;

        case ASN_RFC1155_COUNTER:
        case ASN_RFC1155_GAUGE:
        case ASN_RFC1155_TIMETICKS:
        case ASN_INTEGER:

			// copy the type of asn variable
			objArray[k].asnType = mibEntry->mibType;

            // transfer value as integer
			objArray[k].asnValue.number = fExceed ? (UINT)(-1) : indexOid->ids[j];
			j++;
            break;

        default:
            // invalidate
            j = INVALID_INDEX;
            break;
        }
    }
}



BOOL
IsTableIndex(
    SnmpMibEntry *  mibEntry,
    SnmpTableXlat * tblXlat
    )
{
    UINT newOff;
    BOOL fFoundOk = FALSE;
    BOOL fIndex;

    SNMPDBG((
        SNMP_LOG_VERBOSE,
        "SNMP: TFX: comparing mibEntry 0x%08lx to table 0x%08lx.\n",
        mibEntry,
        tblXlat->txInfo
        ));

    // see if the table indices are specified
    fIndex = (tblXlat->txInfo->tableIndices != NULL);

    if (fIndex) {

        // rummage through index list looking for match
        for (newOff = 0; (newOff < tblXlat->txInfo->numIndices) && !fFoundOk; newOff++ ) {

            // compare mib entry with the next specified index
            fFoundOk = (mibEntry == tblXlat->txInfo->tableIndices[newOff]);
        }

    } else {

        // make sure pointer greater than table entry
        if (mibEntry > tblXlat->txInfo->tableEntry) {

            // calculate the difference between pointers
            newOff = (UINT)mibEntry - (UINT)tblXlat->txInfo->tableEntry;

            // calculate table offset
            newOff /= sizeof(SnmpMibEntry);

            // determine whether entry within region
            fFoundOk = (newOff <= tblXlat->txInfo->numIndices);
        }
    }

    SNMPDBG((
        SNMP_LOG_VERBOSE,
        "SNMP: TFX: mibEntry %s a component of the table's index (off=%d, len=%d).\n",
        fFoundOk ? "is" : "is not",
        newOff,
        tblXlat->txInfo->numIndices
        ));

    return fFoundOk;
}


VOID
MibEntryToQueryList(
    SnmpMibEntry *       mibEntry,
    UINT                 mibAction,
    SnmpExtQueryList *   ql,
    SnmpTableXlat *      tblXlat,
    UINT                 vlIndex,
    RFC1157VarBindList * vbl,
    UINT                 vb,
    UINT *               errorStatus
    )

/*++

Routine Description:

    Converts mib entry information into subagent query.

Arguments:

    tfxInfo     - context info.
    mibEntry    - mib information.
    mibAction   - action to perform.
    ql          - list of subagent queries.
    tableXlat   - table translation info.
    vlIndex     - index into view list.
    vbl         - original varbind list.
    vb          - original varbind.
    errorStatus - used to indicate success or failure.

Return Values:

    None.

--*/

{
    UINT i;
    UINT j;
    UINT viewType;

    FARPROC extFunc;

    AsnAny * objArray;
    SnmpExtQuery * extQuery;

    BOOL fFoundOk = FALSE;

    // determine instrumentation callback
    extFunc = (mibAction == MIB_ACTION_SET)
                ? mibEntry->mibSetFunc
                : mibEntry->mibGetFunc
                ;

    // process existing queries
    for (i=0; (i < ql->len) && !fFoundOk; i++) {

        // retrieve query ptr
        extQuery = &ql->query[i];

        // determine if a similar query exists
        fFoundOk = ((extQuery->extFunc == extFunc) &&
                    (extQuery->mibAction == mibAction));

        // compare table indices (if any)
        if (fFoundOk && extQuery->tblXlat) {

            // make sure
            if (tblXlat) {

                // compare index oids...
                fFoundOk = !SnmpUtilOidCmp(
                                &extQuery->tblXlat->txOid,
                                &tblXlat->txOid
                                );

            } else {

                // hmmm...
                fFoundOk = FALSE;
            }

        }
    }

    // append entry
    if (!fFoundOk) {

        ql->len++; // add new query to end of list
        ql->query = (SnmpExtQuery *)SnmpUtilMemReAlloc(
                                       ql->query,
                                       ql->len * sizeof(SnmpExtQuery)
                                       );

        // retrieve new query pointer
        extQuery = &ql->query[ql->len-1];

        // save common information
        extQuery->mibAction = mibAction;
        extQuery->viewType  = MIB_VIEW_NORMAL;
        extQuery->extFunc   = extFunc;

        // initialize list
        extQuery->vblNum  = 0;
        extQuery->vblXlat = NULL;

        // size the instrumentation buffer
        extQuery->extData.len = (mibAction == MIB_ACTION_SET)
                                    ? mibEntry->mibSetBufLen
                                    : mibEntry->mibGetBufLen
                                    ;

        // allocate the instrumentation buffer
        extQuery->extData.data = SnmpUtilMemAlloc(
                                    extQuery->extData.len
                                    );

        // check memory allocation
        if (extQuery->extData.data) {

            // table?
            if (tblXlat) {

                // retrieve object array pointer
                objArray = (AsnAny *)(extQuery->extData.data);

                // initialize asn array
                ParseInstanceIdentifier(tblXlat, objArray, mibAction);

                // save table info
                extQuery->tblXlat = tblXlat;
            }

        } else {

            // free table oid
            SnmpUtilOidFree(&tblXlat->txOid);

            // free table info
            SnmpUtilMemFree(tblXlat);

            // report memory allocation problem
            *errorStatus = SNMP_ERRORSTATUS_GENERR;
            return; // bail...
        }

    } else if (tblXlat != NULL) {

        SNMPDBG((
            SNMP_LOG_TRACE,
            "SNMP: TFX: releasing duplicate table info 0x%08lx.\n",
            tblXlat
            ));

        // free table oid
        SnmpUtilOidFree(&tblXlat->txOid);

        // free table info
        SnmpUtilMemFree(tblXlat);
    }

    SNMPDBG((
        SNMP_LOG_TRACE,
        "SNMP: TFX: %s query 0x%08lx.\n",
        fFoundOk ? "editing" : "adding",
        extQuery
        ));

    // copy to index
    i = extQuery->vblNum;

    // allocate entry
    extQuery->vblNum++;
    extQuery->vblXlat = (SnmpVarBindXlat *)SnmpUtilMemReAlloc(
                            extQuery->vblXlat,
                            extQuery->vblNum * sizeof(SnmpVarBindXlat)
                            );

    // copy common xlate information
    extQuery->vblXlat[i].vblIndex = vb;
    extQuery->vblXlat[i].vlIndex  = vlIndex;
    extQuery->vblXlat[i].extQuery = NULL;

    // save translation info
    extQuery->vblXlat[i].mibEntry = mibEntry;

    // determine offset used
    i = (mibAction == MIB_ACTION_SET)
          ? (UINT)(CHAR)mibEntry->mibSetBufOff
          : (UINT)(CHAR)mibEntry->mibGetBufOff
          ;

    // retrieve object array pointer
    objArray = (AsnAny *)(extQuery->extData.data);

    // fill in only asn type if get
    if (mibAction != MIB_ACTION_SET) {

        // ignore table indices
        if (extQuery->tblXlat &&
            IsTableIndex(mibEntry,extQuery->tblXlat)) {

            SNMPDBG((
                SNMP_LOG_VERBOSE,
                "SNMP: TFX: requesting index value.\n"
                ));

        } else {

            // initialize asn type to match entry
            objArray[i].asnType = mibEntry->mibType;
        }

    } else {

        // copy user-supplied value into buffer
        SnmpUtilAsnAnyCpy(&objArray[i], &vbl->list[vb].value);
    }
}


VOID
VarBindToQueryList(
    SnmpTfxInfo *        tfxInfo,
    RFC1157VarBindList * vbl,
    SnmpExtQueryList *   ql,
    UINT                 vb,
    UINT *               errorStatus,
    UINT *               errorIndex,
    UINT                 queryView
    )

/*++

Routine Description:

    Adds varbind to query list.

Arguments:

    tfxInfo     - context info.
    vbl         - list of varbinds.
    ql          - list of subagent queries.
    vb          - index of varbind to add to query.
    errorStatus - used to indicate success or failure.
    errorIndex  - used to identify an errant varbind.
    queryView   - view of query requested.

Return Values:

    None.

--*/

{
    INT i;
    INT nDiff;
    INT lastViewIndex;

    BOOL fAnyOk;
    BOOL fFoundOk = FALSE;

    UINT mibAction;

    SnmpMibView   * mibView;
    SnmpMibEntry  * mibEntry = NULL;
    SnmpTableXlat * tblXlat  = NULL;

    // copy request type
    mibAction = ql->action;

    // determine whether we need exact match
    fAnyOk = (mibAction == MIB_ACTION_GETNEXT);

    SNMPDBG((
        SNMP_LOG_VERBOSE,
        "SNMP: TFX: searching subagents to resolve %s (%s).\n",
        SnmpUtilOidToA(&vbl->list[vb].name),
		fAnyOk ? "AnyOk" : "AnyNOk"
        ));

	SNMPDBG((
		SNMP_LOG_VERBOSE,
		"SNMP: TFX: VarBindToQueryList; scanning views from %d to %d.\n",
		queryView,
		(INT)tfxInfo->numViews
    ));

    // init to NonView
    lastViewIndex = -1;

    // locate appropriate view (starting at queryView)
    for (i = queryView; (i < (INT)tfxInfo->numViews) && !fFoundOk; i++) {

        // retrieve the mib view information
        mibView = tfxInfo->tfxViews[i].mibView;

        // compare root oids
        nDiff = SnmpUtilOidNCmp(
                    &vbl->list[vb].name,
                    &mibView->viewOid,
                    mibView->viewOid.idLength
                    );

        // analyze results based on request type
        fFoundOk = (!nDiff || (fAnyOk && (nDiff < 0)));

		SNMPDBG((
			SNMP_LOG_VERBOSE,
			"SNMP: TFX: View %d: comp(%s, ",
			i,
			SnmpUtilOidToA(&vbl->list[vb].name)
		));
		SNMPDBG((
			SNMP_LOG_VERBOSE,
			"%s, %d) = %d '%s'\n",
			SnmpUtilOidToA(&mibView->viewOid),
			mibView->viewOid.idLength,
			nDiff,
			fFoundOk?"Found":"NotFound"
		));


	    // make sure we can obtain mib entry (if available)
        if (fFoundOk && (mibView->viewType == MIB_VIEW_NORMAL)) {

            // initialize local copy of error status
            UINT mibStatus = SNMP_ERRORSTATUS_NOERROR;

            // store index
            lastViewIndex = i;

            // load mib entry
            VarBindToMibEntry(
                   tfxInfo,
                   &vbl->list[vb],
                   &mibEntry,
                   &mibAction,
                   &tblXlat,
                   i,
                   &mibStatus
                   );

			SNMPDBG((
				SNMP_LOG_VERBOSE,
				"SNMP: TFX: VarBindToMibEntry returned %d.\n",
				mibStatus
			));

            // successfully loaded mib entry information
            fFoundOk = (mibStatus == SNMP_ERRORSTATUS_NOERROR);

            // bail if not searching...
            if (!fFoundOk && !fAnyOk) {
                // pass up error status
                *errorStatus = mibStatus;
                *errorIndex  = vb+1;
                return; // bail...
            }
        }
    }

    // reset error status and index...
    *errorStatus = SNMP_ERRORSTATUS_NOERROR;
    *errorIndex  = 0;

    // found?
    if (fFoundOk) {
        // save query
        MibEntryToQueryList(
               mibEntry,
               mibAction,
               ql,
               tblXlat,
               i-1,
               vbl,
               vb,
               errorStatus
               );

    } else if (fAnyOk){

		if (lastViewIndex == -1)
			lastViewIndex = tfxInfo->numViews - 1;

		// not supported in any view...
		SnmpUtilOidFree(&vbl->list[vb].name);

		// copy varbind
		SnmpUtilOidCpy(
			&vbl->list[vb].name,
			&tfxInfo->tfxViews[lastViewIndex].mibView->viewOid
			);

		// increment last element of view oid
		vbl->list[vb].name.ids[(vbl->list[vb].name.idLength-1)]++;

		SNMPDBG((
			SNMP_LOG_VERBOSE,
			"SNMP: TFX: changing varbind to %s.\n",
			SnmpUtilOidToA(&vbl->list[vb].name)
			));
    }
}


VOID
VarBindListToQueryList(
    SnmpTfxInfo *        tfxInfo,
    RFC1157VarBindList * vbl,
    SnmpExtQueryList *   ql,
    UINT *               errorStatus,
    UINT *               errorIndex
    )

/*++

Routine Description:

    Convert list of varbinds from incoming pdu into a list of
    individual subagent queries.

Arguments:

    tfxInfo     - context handle.
    vbl         - list of varbinds in pdu.
    ql          - list of subagent queries.
    errorStatus - used to indicate success or failure.
    errorIndex  - used to identify an errant varbind.

Return Values:

    None.

--*/

{
    UINT i; // index into varbind list

    SNMPDBG((
        SNMP_LOG_TRACE,
        "SNMP: TFX: processing %s request containing %d variable(s).\n",
        (ql->action == MIB_ACTION_GET)
            ? "get"
            : (ql->action == MIB_ACTION_SET)
                ? "set"
                : (ql->action == MIB_ACTION_GETNEXT)
                    ? "getnext"
                    : "unknown", vbl->len));

    // initialize status return values
    *errorStatus = SNMP_ERRORSTATUS_NOERROR;
    *errorIndex  = 0;

    // process incoming variable bindings
    for (i=0; (i < vbl->len) && !(*errorStatus); i++) {

        // find varbind
        VarBindToQueryList(
            tfxInfo,
            vbl,
            ql,
            i,
            errorStatus,
            errorIndex,
            0
            );
    }
}


UINT
MibStatusToSnmpStatus(
    UINT mibStatus
    )

/*++

Routine Description:

    Translate mib status into snmp error status.

Arguments:

    mibStatus - mib error code.

Return Values:

    Returns snmp error status.

--*/

{
    UINT errorStatus;

    switch (mibStatus) {

    case MIB_S_SUCCESS:
        errorStatus = SNMP_ERRORSTATUS_NOERROR;
        break;

    case MIB_S_INVALID_PARAMETER:
        errorStatus = SNMP_ERRORSTATUS_BADVALUE;
        break;

    case MIB_S_NOT_SUPPORTED:
    case MIB_S_NO_MORE_ENTRIES:
    case MIB_S_ENTRY_NOT_FOUND:
        errorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
        break;

    default:
        errorStatus = SNMP_ERRORSTATUS_GENERR;
        break;
    }

    return errorStatus;
}


VOID
AdjustErrorIndex(
    SnmpExtQuery * q,
    UINT *         errorIndex
    )

/*++

Routine Description:

    Ensure that indices match the original pdu.

Arguments:

    q          - subagent query.
    errorIndex - used to identify an errant varbind.

Return Values:

    None.

--*/

{
    UINT errorIndexOld = *errorIndex;

    // make sure within bounds
    if (errorIndexOld && (errorIndexOld <= q->vblNum)) {

        // determine proper index from xlat info
        *errorIndex = q->vblXlat[errorIndexOld-1].vblIndex+1;

    } else {

        // default to first variable
        *errorIndex = q->vblXlat[0].vblIndex+1;
    }
}


BOOL
ProcessQuery(
    SnmpExtQuery * q,
    UINT *         errorStatus,
    UINT *         errorIndex
    )

/*++

Routine Description:

    Query the subagent for requested items.

Arguments:

    q           - subagent query.
    errorStatus - used to indicate success or failure.
    errorIndex  - used to identify an errant varbind.

Return Values:

    None.

--*/

{
    BOOL fOk = TRUE;
    UINT extStatus = 0;

    AsnAny * objArray;

    // validate...
    if (q == NULL) {
        return TRUE;
    }

    // retrieve asn object array
    objArray = (AsnAny *)(q->extData.data);
    SNMPDBG((
		SNMP_LOG_VERBOSE,
		"SNMP: TFX: ProcessQuery - objArray=%lx\n",
		objArray
		));

    __try {

		SNMPDBG((
			SNMP_LOG_VERBOSE,
			"SNMP: TFX: entering subagent code ....\n"
			));
		// query subagent
        extStatus = (*q->extFunc)(
                            q->mibAction,
                            objArray,
                            errorIndex
                            );
		SNMPDBG((
			SNMP_LOG_VERBOSE,
			"SNMP: TFX: ... subagent code completed.\n"
			));

        SNMPDBG((
            SNMP_LOG_TRACE,
            "SNMP: TFX: subagent returned %s (e=0x%08lx,i=%d).\n",
            (extStatus == MIB_S_SUCCESS)
                ? "MIB_S_SUCCESS"
                : (extStatus == MIB_S_NO_MORE_ENTRIES)
                    ? "MIB_S_NO_MORE_ENTRIES"
                    : (extStatus == MIB_S_ENTRY_NOT_FOUND)
                        ? "MIB_S_ENTRY_NOT_FOUND"
                        : (extStatus == MIB_S_INVALID_PARAMETER)
                            ? "MIB_S_INVALID_PARAMETER"
                            : (extStatus == MIB_S_NOT_SUPPORTED)
                                ? "MIB_S_NOT_SUPPORTED"
                                : "error", extStatus, *errorIndex
                                ));

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        // report exception code
        extStatus = GetExceptionCode();

        // disable
        fOk = FALSE;
    }

    // save error info
    SetLastError(extStatus);

    // pass back translated version
    *errorStatus = MibStatusToSnmpStatus(extStatus);

    return fOk;
}


VOID
ProcessQueryList(
    SnmpTfxInfo *        tfxInfo,
    SnmpExtQueryList *   ql,
    RFC1157VarBindList * vbl,
    UINT *               errorStatus,
    UINT *               errorIndex
    )

/*++

Routine Description:

    Process the query list based on request type.

Arguments:

    tfxInfo     - context information.
    ql          - list of subagent queries.
    vbl         - list of incoming variable bindings.
    errorStatus - used to indicate success or failure.
    errorIndex  - used to identify an errant varbind.

Return Values:

    None.

--*/

{
    INT i=0; // index into query list
    INT j=0; // index into query list

    INT qlLen = ql->len; // save...

    SNMPDBG((
        SNMP_LOG_TRACE,
        "SNMP: TFX: processing %d subagent queries.\n",
        qlLen
        ));

    // sets are processed below...
    if (ql->action != MIB_ACTION_SET) {

        // process list of individual queries
        for (i=0; (i < qlLen) && !(*errorStatus); i++ ) {

            // send query to subagent
            if (ProcessQuery(&ql->query[i], errorStatus, errorIndex)) {

                // need to validate getnext results
                if (ql->action == MIB_ACTION_GETNEXT) {
                    // exhaust all possibilities...
                    ValidateQueryList(
                        tfxInfo,
                        ql,
                        i,
                        vbl,
                        errorStatus,
                        errorIndex
                        );
                }

                // check the subagent status code returned
                if (*errorStatus != SNMP_ERRORSTATUS_NOERROR) {
                    // adjust index to match request pdu
                    AdjustErrorIndex(&ql->query[i], errorIndex);
                }

            } else {

                // subagent unable to process query
                *errorStatus = SNMP_ERRORSTATUS_GENERR;
                *errorIndex  = 1;
                // adjust index to match request pdu
                AdjustErrorIndex(&ql->query[i], errorIndex);
            }
        }

    } else {

        // process all of the validate queries
        for (i=0; (i < qlLen) && !(*errorStatus); i++) {

            // alter query type to validate entries
            ql->query[i].mibAction = MIB_ACTION_VALIDATE;

            // send query to subagent
            if (ProcessQuery(&ql->query[i], errorStatus, errorIndex)) {

                // check the subagent status code returned
                if (*errorStatus != SNMP_ERRORSTATUS_NOERROR) {
                    // adjust index to match request pdu
                    AdjustErrorIndex(&ql->query[i], errorIndex);
                }

            } else {

                // subagent unable to process query
                *errorStatus = SNMP_ERRORSTATUS_GENERR;
                *errorIndex  = 1;
                // adjust index to match request pdu
                AdjustErrorIndex(&ql->query[i], errorIndex);
            }
        }

        // process all of the set queries
        for (j=0; (j < qlLen) && !(*errorStatus); j++) {

            // alter query type to set entries
            ql->query[j].mibAction = MIB_ACTION_SET;

            // send query to subagent
            if (ProcessQuery(&ql->query[j], errorStatus, errorIndex)) {

                // check the subagent status code returned
                if (*errorStatus != SNMP_ERRORSTATUS_NOERROR) {
                    // adjust index to match request pdu
                    AdjustErrorIndex(&ql->query[j], errorIndex);
                }

            } else {

                // subagent unable to process query
                *errorStatus = SNMP_ERRORSTATUS_GENERR;
                *errorIndex  = 1;
                // adjust index to match request pdu
                AdjustErrorIndex(&ql->query[j], errorIndex);
            }
        }

        // cleanup...
        while (i-- > 0) {

            UINT ignoreStatus = 0; // dummy values
            UINT ignoreIndex  = 0; // dummy values

            // alter query type to set entries
            ql->query[i].mibAction = MIB_ACTION_CLEANUP;

            // send the cleanup request success or not
            ProcessQuery(&ql->query[i], &ignoreStatus, &ignoreIndex);
        }
    }
}


VOID
ConstructInstanceIdentifier(
    SnmpTableXlat *       tblXlat,
    AsnAny *              objArray,
    AsnObjectIdentifier * newOid,
    UINT                  mibAction
    )

/*++

Routine Description:

    Convert asn value into index oid.

Arguments:

    tblXlat   - table translation info.
    objArray  - asn object array.
    newOid    - relative oid to return.
    mibAction - action requested of subagent.

Return Values:

    None.

--*/

{
    UINT i;
    UINT j;
    UINT k;
    UINT l;
    UINT m;

    BOOL fFixed;
    BOOL fLimit;
    BOOL fIndex;

    UINT numItems;

    SnmpMibEntry * mibEntry;

    // initialize
    newOid->ids = NULL;
    newOid->idLength = 0;

    SNMPDBG((
        SNMP_LOG_TRACE,
        "SNMP: TFX: converting obj array to index via table 0x%08lx.\n",
        tblXlat->txInfo
        ));

    // retrieve root entry and entry count
    numItems = tblXlat->txInfo->numIndices;

    // see if the table indices are specified
    fIndex = (tblXlat->txInfo->tableIndices != NULL);

    // scan entries of table indices
    for (i=0, j=0; i < numItems; i++) {

        // get mib entry from table or directly
        mibEntry = fIndex ?  tblXlat->txInfo->tableIndices[i]
                          : &tblXlat->txInfo->tableEntry[i+1]
                          ;

        // retrieve array index
        k = (mibAction == MIB_ACTION_SET)
                ? (UINT)(CHAR)mibEntry->mibSetBufOff
                : (UINT)(CHAR)mibEntry->mibGetBufOff
                ;

		SNMPDBG((
			SNMP_LOG_TRACE,
			"SNMP: TFX: ConstructIndexIdentifier - k=%d\n",
			k
			));
        // determine type
        switch (mibEntry->mibType) {

        // variable length types
        case ASN_OBJECTIDENTIFIER:

            // check whether this is a fixed length variable or not
            fLimit = (mibEntry->mibMinimum || mibEntry->mibMaximum);
            fFixed = (fLimit && (mibEntry->mibMinimum == mibEntry->mibMaximum));

            // validate
            if (fFixed) {

                // fixed length
                l = mibEntry->mibMaximum;

                // allocate space
                newOid->idLength += l;
                newOid->ids = (UINT *)SnmpUtilMemReAlloc(
                    newOid->ids,
                    newOid->idLength * sizeof(UINT)
                    );

            } else {

                // determine variable length of object
                l = objArray[k].asnValue.object.idLength;

                // allocate space
                newOid->idLength += (l+1);
                newOid->ids = (UINT *)SnmpUtilMemReAlloc(
                    newOid->ids,
                    newOid->idLength * sizeof(UINT)
                    );

                // save length
                newOid->ids[j++] = l;
            }

            // transfer data
            for (m=0; m < l; m++) {

                // transfer oid element from buffer
                newOid->ids[j++] = objArray[k].asnValue.object.ids[m];
            }

            break;

        case ASN_RFC1155_OPAQUE:
        case ASN_OCTETSTRING:

            // check whether this is a fixed length variable or not
            fLimit = (mibEntry->mibMinimum || mibEntry->mibMaximum);
            fFixed = (fLimit && (mibEntry->mibMinimum == mibEntry->mibMaximum));

            // validate
            if (fFixed) {

                // fixed length
                l = mibEntry->mibMaximum;

                // allocate space
                newOid->idLength += l;
                newOid->ids = (UINT *)SnmpUtilMemReAlloc(
                    newOid->ids,
                    newOid->idLength * sizeof(UINT)
                    );

            } else {

                // determine variable length of object
                l = objArray[k].asnValue.string.length;

                // allocate space
                newOid->idLength += (l+1);
                newOid->ids = (UINT *)SnmpUtilMemReAlloc(
                    newOid->ids,
                    newOid->idLength * sizeof(UINT)
                    );

                // save length
                newOid->ids[j++] = l;
            }

            // transfer data
            for (m=0; m < l; m++) {

                // convert character
                newOid->ids[j++] =
                    (UINT)(CHAR)objArray[k].asnValue.string.stream[m];
            }

            break;

        // implicit fixed size
        case ASN_RFC1155_IPADDRESS:

            // allocate space
            newOid->idLength += 4;
            newOid->ids = (UINT *)SnmpUtilMemReAlloc(
                newOid->ids,
                newOid->idLength * sizeof(UINT)
                );

            // transfer data into buffer
            newOid->ids[j++] = (DWORD)(BYTE)objArray[k].asnValue.string.stream[0];
            newOid->ids[j++] = (DWORD)(BYTE)objArray[k].asnValue.string.stream[1];
            newOid->ids[j++] = (DWORD)(BYTE)objArray[k].asnValue.string.stream[2];
            newOid->ids[j++] = (DWORD)(BYTE)objArray[k].asnValue.string.stream[3];

            break;

        case ASN_RFC1155_COUNTER:
        case ASN_RFC1155_GAUGE:
        case ASN_RFC1155_TIMETICKS:
        case ASN_INTEGER:

            // allocate space
            newOid->idLength += 1;
            newOid->ids = (UINT *)SnmpUtilMemReAlloc(
                newOid->ids,
                newOid->idLength * sizeof(UINT)
                );

            // transfer value as integer
            newOid->ids[j++] = objArray[k].asnValue.number;
            break;

        default:
            // invalidate
            j = INVALID_INDEX;
            break;
        }
    }
}


VOID
DeleteQuery(
    SnmpExtQuery * q
    )

/*++

Routine Description:

    Deletes individual query.

Arguments:

    q - subagent query.

Return Values:

    None.

--*/

{
    UINT i; // index into xlat array
    UINT j; // index into object array

    BOOL fSet;

    AsnAny * objArray;
    SnmpMibEntry * mibEntry;

    SNMPDBG((
        SNMP_LOG_VERBOSE,
        "SNMP: TFX: deleting query 0x%08lx.\n", q
        ));

    // determine whether a set was requested
    fSet = (q->mibAction == MIB_ACTION_SET);

    // retrieve asn object array
    objArray = (AsnAny *)(q->extData.data);

    // free requested entries
    for (i = 0; i < q->vblNum; i++ ) {

        // retrieve mib entry
        mibEntry = q->vblXlat[i].mibEntry;

        j = fSet ? (UINT)(CHAR)mibEntry->mibSetBufOff
                 : (UINT)(CHAR)mibEntry->mibGetBufOff
                 ;

        SnmpUtilAsnAnyFree(&objArray[j]);

        // free any followup queries
        if ((q->vblXlat[i].extQuery != NULL) &&
            (q->vblXlat[i].extQuery != INVALID_QUERY)) {

            SNMPDBG((
                SNMP_LOG_VERBOSE,
                "SNMP: TFX: deleting followup query 0x%08lx.\n",
                q->vblXlat[i].extQuery
                ));

            // free followup query
            DeleteQuery(q->vblXlat[i].extQuery);

            // free query structure itself
            SnmpUtilMemFree(q->vblXlat[i].extQuery);
        }
    }

    // free indices
    if (q->tblXlat) {

        BOOL fIndex;

        // see if the table indices are specified
        fIndex = (q->tblXlat->txInfo->tableIndices != NULL);

        // free the individual indices
        for (i = 0; i < q->tblXlat->txInfo->numIndices; i++) {

            // get mib entry from table or directly from entry
            mibEntry = fIndex ?  q->tblXlat->txInfo->tableIndices[i]
                              : &q->tblXlat->txInfo->tableEntry[i+1]
                              ;

            // determine the buffer offset used
            j = fSet ? (UINT)(CHAR)mibEntry->mibSetBufOff
                     : (UINT)(CHAR)mibEntry->mibGetBufOff
                     ;

            // free individual index
            SnmpUtilAsnAnyFree(&objArray[j]);
        }
    }

    // free buffer
    SnmpUtilMemFree(objArray);

    // free table info
    if (q->tblXlat) {

        // free object identifier
        SnmpUtilOidFree(&q->tblXlat->txOid);

        // free the xlat structure
        SnmpUtilMemFree(q->tblXlat);
    }

    // free translation info
    SnmpUtilMemFree(q->vblXlat);
}


VOID
DeleteQueryList(
    SnmpExtQueryList * ql
    )

/*++

Routine Description:

    Deletes query list.

Arguments:

    ql - list of subagent queries.

Return Values:

    None.

--*/

{
    UINT q; // index into query list

    // process queries
    for (q=0; q < ql->len; q++) {

        // delete query
        DeleteQuery(&ql->query[q]);
    }

    // free query list
    SnmpUtilMemFree(ql->query);
}


VOID
QueryToVarBindList(
    SnmpTfxInfo *        tfxInfo,
    SnmpExtQuery *       q,
    RFC1157VarBindList * vbl
    )

/*++

Routine Description:

    Convert query back into varbind.

Arguments:

    tfxInfo - context info
    q       - subagent query.
    vbl     - list of varbinds in outgoing pdu.

Return Values:

    None.

--*/

{
    UINT i;
    UINT j;
    UINT k;
    UINT l;

    BOOL fSet;

    AsnAny * objArray;
    SnmpMibEntry * mibEntry;

    AsnObjectIdentifier idxOid;

    SNMPDBG((
        SNMP_LOG_VERBOSE,
        "SNMP: TFX: converting query 0x%08lx to varbinds.\n", q
        ));

    // determine whether a set was requested
    fSet = (q->mibAction == MIB_ACTION_SET);

    // retrieve asn object array
    objArray = (AsnAny *)(q->extData.data);

    // copy requested entries
    for (j = 0; j < q->vblNum; j++) {

        // process followup query
        if (q->vblXlat[j].extQuery != NULL) {

            if (q->vblXlat[j].extQuery != INVALID_QUERY) {

                SNMPDBG((
                    SNMP_LOG_VERBOSE,
                    "SNMP: TFX: processing followup query 0x%08lx.\n",
                    q->vblXlat[j].extQuery
                    ));

                QueryToVarBindList(
                    tfxInfo,
                    q->vblXlat[j].extQuery,
                    vbl
                    );
            }

            continue; // skip...
        }

        // retrieve index
        i = q->vblXlat[j].vblIndex;

        // retrieve mib entry for requested item
        mibEntry = q->vblXlat[j].mibEntry;

        k = fSet ? (UINT)(CHAR)mibEntry->mibSetBufOff
                 : (UINT)(CHAR)mibEntry->mibGetBufOff
                 ;

        // free original variable
        SnmpUtilVarBindFree(&vbl->list[i]);

        // copy the asn value first
        SnmpUtilAsnAnyCpy(&vbl->list[i].value, &objArray[k]);

        // copy root oid of view
        SnmpUtilOidCpy(
            &vbl->list[i].name,
            &tfxInfo->tfxViews[(q->vblXlat[j].vlIndex)].mibView->viewOid
            );

        // copy oid of variable
        SnmpUtilOidAppend(
            &vbl->list[i].name,
            &mibEntry->mibOid
            );

        // copy table index
        if (q->tblXlat) {

            // convert value to oid
            ConstructInstanceIdentifier(q->tblXlat, objArray, &idxOid, q->mibAction);

            // append oid to object name
            SnmpUtilOidAppend(&vbl->list[i].name, &idxOid);

            // free temp oid
            SnmpUtilOidFree(&idxOid);
        }

        SNMPDBG((
            SNMP_LOG_TRACE,
            "SNMP: TFX: returning oid %s.\n",
            SnmpUtilOidToA(&vbl->list[i].name)
            ));
    }
}


VOID
QueryListToVarBindList(
    SnmpTfxInfo *        tfxInfo,
    SnmpExtQueryList *   ql,
    RFC1157VarBindList * vbl,
    UINT *               errorStatus,
    UINT *               errorIndex
    )

/*++

Routine Description:

    Convert query list back into outgoing varbinds.

Arguments:

    tfxInfo     - context information.
    ql          - list of subagent queries.
    vbl         - list of varbinds in outgoing pdu.
    errorStatus - used to indicate success or failure.
    errorIndex  - used to identify an errant varbind.

Return Values:

    None.

--*/

{
    UINT q;   // index into queue list
    UINT vb;  // index into queue varbind list
    UINT i;   // index into original varbind list

    SNMPDBG((
        SNMP_LOG_TRACE,
        "SNMP: TFX: request %s, errorStatus=%s, errorIndex=%d.\n",
        (*errorStatus == SNMP_ERRORSTATUS_NOERROR)
            ? "succeeded"
            : "failed",
        (*errorStatus == SNMP_ERRORSTATUS_NOERROR)
            ? "NOERROR"
            : (*errorStatus == SNMP_ERRORSTATUS_NOSUCHNAME)
                  ? "NOSUCHNAME"
                  : (*errorStatus == SNMP_ERRORSTATUS_BADVALUE)
                      ? "BADVALUE"
                      : (*errorStatus == SNMP_ERRORSTATUS_READONLY)
                          ? "READONLY"
                          : (*errorStatus == SNMP_ERRORSTATUS_TOOBIG)
                              ? "TOOBIG"
                              : "GENERR", *errorIndex
                              ));

    // only convert back if error not reported
    if (*errorStatus == SNMP_ERRORSTATUS_NOERROR) {

        // process queries
        for (q=0; q < ql->len; q++) {

            // translate query data
            QueryToVarBindList(tfxInfo, &ql->query[q], vbl);
        }
    }

    // free
    DeleteQueryList(ql);
}


VOID
ValidateQueryList(
    SnmpTfxInfo *        tfxInfo,
    SnmpExtQueryList *   ql,
    UINT                 q,
    RFC1157VarBindList * vbl,
    UINT *               errorStatus,
    UINT *               errorIndex
    )

/*++

Routine Description:

    Validate getnext results and re-query if necessary.

Arguments:

    tfxInfo      - context information.
    ql           - list of subagent queries.
    q            - subagent query of interest.
    vbl          - list of bindings in incoming pdu.
    errorStatus  - used to indicate success or failure.
    errorIndex   - used to identify an errant varbind.

Return Values:

    None.

--*/

{
    UINT i;
    UINT j;

    SnmpExtQueryList tmpQl;

    UINT vlIndex;
    UINT vblIndex;
    UINT mibAction;
    UINT mibStatus;
    SnmpMibEntry * mibEntry;
    SnmpTableXlat * tblXlat;
    RFC1157VarBindList tmpVbl;
    BOOL fFoundOk = FALSE;

    SNMPDBG((
        SNMP_LOG_TRACE,
        "SNMP: TFX: verifying results of query 0x%08lx.\n", &ql->query[q]
        ));

    // bail on any error other than no such name
    if (*errorStatus != SNMP_ERRORSTATUS_NOSUCHNAME) {

        SNMPDBG((
            SNMP_LOG_TRACE,
            "SNMP: TFX: returning error %d.\n",
            GetLastError()
            ));

        return; // bail...
    }

    // scan query list updating variables
    for (i=0; i < ql->query[q].vblNum; i++) {

        // initialize
        mibEntry  = ql->query[q].vblXlat[i].mibEntry;
        vlIndex   = ql->query[q].vblXlat[i].vlIndex;
        j         = ql->query[q].vblXlat[i].vlIndex;

        tblXlat   = NULL;
        mibAction = MIB_ACTION_GETNEXT;

        SNMPDBG((
            SNMP_LOG_TRACE,
            "SNMP: TFX: ValidateQueryList; next of %s.\n",
            SnmpUtilOidToA(&mibEntry->mibOid)
            ));

        // next...
        FindNextMibEntry(
               tfxInfo,
               NULL,
               &mibEntry,
               &mibAction,
               &tblXlat,
               vlIndex,
               errorStatus
               );

        while (*errorStatus == SNMP_ERRORSTATUS_NOERROR) {

            SNMPDBG((
                SNMP_LOG_TRACE,
                "SNMP: TFX: constructing followup to query 0x%08lx:\n"
                "SNMP: TFX: \tmibEntry=0x%08lx\n"
                "SNMP: TFX: \ttblXlat =0x%08lx\n",
                &ql->query[q],
                mibEntry,
                tblXlat
                ));

            // initialize
            tmpQl.len    = 0;
            tmpQl.query  = NULL;
            tmpQl.action = MIB_ACTION_GETNEXT;

            // create query
            MibEntryToQueryList(
                   mibEntry,
                   mibAction,
                   &tmpQl,
                   tblXlat,
                   vlIndex,
                   NULL,
                   ql->query[q].vblXlat[i].vblIndex,
                   errorStatus
                   );

            SNMPDBG((
                SNMP_LOG_TRACE,
                "SNMP: TFX: processing followup query 0x%08lx.\n",
                tmpQl.query
                ));

            // perform query with new oid
            ProcessQuery(tmpQl.query, errorStatus, errorIndex);

            // calculate results of query
            if (*errorStatus == SNMP_ERRORSTATUS_NOERROR) {

                SNMPDBG((
                    SNMP_LOG_TRACE,
                    "SNMP: TFX: saving followup 0x%08lx in query 0x%08lx.\n",
                    tmpQl.query, &ql->query[q]
                    ));

                // copy query for reassembly purposes
                ql->query[q].vblXlat[i].extQuery = tmpQl.query;

                break; // process next varbind...

            } else if (*errorStatus != SNMP_ERRORSTATUS_NOSUCHNAME) {

                SNMPDBG((
                    SNMP_LOG_TRACE,
                    "SNMP: TFX: could not process followup.\n"
                    ));

                // delete...
                DeleteQueryList(&tmpQl);

                return; // bail...
			}

            SNMPDBG((
                SNMP_LOG_TRACE,
                "SNMP: TFX: re-processing followup to query 0x%08lx.\n",
                &ql->query[q]
                ));

            // delete...
            DeleteQueryList(&tmpQl);

            // re-initialize and continue...
            *errorStatus = SNMP_ERRORSTATUS_NOERROR;
            tblXlat = NULL;
            mibAction = MIB_ACTION_GETNEXT;

            // next...
            FindNextMibEntry(
                   tfxInfo,
                   NULL,
                   &mibEntry,
                   &mibAction,
                   &tblXlat,
                   vlIndex,
                   errorStatus
                   );
        }

        // attempt to query next supported subagent view
        if (*errorStatus == SNMP_ERRORSTATUS_NOSUCHNAME) {

            // initialize
            tmpQl.len    = 0;
            tmpQl.query  = NULL;
            tmpQl.action = MIB_ACTION_GETNEXT;

            // retrieve variable binding list index
            vblIndex = ql->query[q].vblXlat[i].vblIndex;

            // release old variable binding
            SnmpUtilVarBindFree(&vbl->list[vblIndex]);

            // copy varbind
            SnmpUtilOidCpy(
                &vbl->list[vblIndex].name,
                &tfxInfo->tfxViews[j].mibView->viewOid
                );

            // increment last sub-identifier of view oid
            vbl->list[vblIndex].name.ids[vbl->list[vblIndex].name.idLength-1]++;

            SNMPDBG((
                SNMP_LOG_TRACE,
                "SNMP: TFX: processing followup to query 0x%08lx with varbind as %s.\n",
                &ql->query[q],
                SnmpUtilOidToA(&vbl->list[vblIndex].name)
                ));

            // we need to query again with new oid
            VarBindToQueryList(
                tfxInfo,
                vbl,
                &tmpQl,
                0,
                errorStatus,
                errorIndex,
                j+1
                );

			SNMPDBG((
				SNMP_LOG_VERBOSE,
				"SNMP: TFX: ValidateQueryList; VarBindToQueryList returned %d.\n",
				*errorStatus
				));

            // make sure we successfully processed query
            if (*errorStatus == SNMP_ERRORSTATUS_NOERROR) {

/*                // perform query with new oid
                ProcessQuery(tmpQl.query, errorStatus, errorIndex);

                // make sure we successfully processed query    
                if (*errorStatus == SNMP_ERRORSTATUS_NOERROR) {

                    // validate...
                    if (tmpQl.query) {

                        SNMPDBG((
                            SNMP_LOG_TRACE,
                            "SNMP: TFX: saving followup 0x%08lx in query 0x%08lx.\n",
                            tmpQl.query, &ql->query[q]
                            ));

                        // copy query for reassembly purposes
                        ql->query[q].vblXlat[i].extQuery = tmpQl.query;

                    } else {

                        // copy query for reassembly purposes
                        ql->query[q].vblXlat[i].extQuery = INVALID_QUERY;
                    }

                    break; // process next varbind...
                }
*/
                // copy query for reassembly purposes
                ql->query[q].vblXlat[i].extQuery = INVALID_QUERY;
                continue; // process next varbind...

            }

            SNMPDBG((
                SNMP_LOG_TRACE,
                "SNMP: TFX: could not process followup (again).\n"
                ));

            // delete...
            DeleteQueryList(&tmpQl);

            return; // bail...
        }
    }
}


SnmpTfxInfo *
AllocTfxInfo(
    )
{
    // simply return results from generic memory allocation
    return (SnmpTfxInfo *)SnmpUtilMemAlloc(sizeof(SnmpTfxInfo));
}


VOID
FreeTfxInfo(
    SnmpTfxInfo * tfxInfo
    )
{
    UINT i;

    if (tfxInfo == NULL) {
        return;
    }

    // walk through list of views
    for (i=0; (i < tfxInfo->numViews); i++) {

        // release memory for view hash tables
        FreeHashTable(tfxInfo->tfxViews[i].hashTable);
    }
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public Procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

SnmpTfxHandle
SNMP_FUNC_TYPE
SnmpTfxOpen(
    DWORD         numViews,
    SnmpMibView * supportedViews
    )
{
    UINT i,j;
    BOOL fOk;
    SnmpTfxInfo * tfxInfo = NULL;

    // validate parameters
    if ((numViews == 0) ||
        (supportedViews == NULL)) {
        return NULL;
    }

    // allocate structure
    tfxInfo = AllocTfxInfo();

    // validate pointer
    if (tfxInfo == NULL) {
        return NULL;
    }

    // copy number of views
    tfxInfo->numViews = numViews;

    // allocate individual view structures
    tfxInfo->tfxViews = SnmpUtilMemAlloc(
        tfxInfo->numViews * sizeof(SnmpTfxView)
        );

    // initialize status
    fOk = (tfxInfo->tfxViews != NULL);

    // initialize each view structure
    for (i=0; (i < tfxInfo->numViews) && fOk; i++) {

        SnmpHashNode ** tmpHashTable;

        // initialize individual view list entry
        tmpHashTable = AllocHashTable(&supportedViews[i]);

        // initialize status
        fOk = (tmpHashTable != NULL);

        // validate
        if (fOk) {

            // save a pointer into the subagent view list
            tfxInfo->tfxViews[i].mibView = &supportedViews[i];

            // save newly allocated view hash table
            tfxInfo->tfxViews[i].hashTable = tmpHashTable;
        }
    }

    // validate
    if (fOk) {

        SnmpTfxView tmpTfxView;

        // make sure views are sorted
        for (i=0; (i < tfxInfo->numViews); i++) {

            for(j=i+1; (j < tfxInfo->numViews); j++) {

                // in lexographic order?
                if (0 < SnmpUtilOidCmp(
                        &(tfxInfo->tfxViews[i].mibView->viewOid),
                        &(tfxInfo->tfxViews[j].mibView->viewOid))) {
                    // no, swap...
                    tmpTfxView = tfxInfo->tfxViews[i];
                    tfxInfo->tfxViews[i] = tfxInfo->tfxViews[j];
                    tfxInfo->tfxViews[i] = tmpTfxView;
                }
            }
			SNMPDBG((
				SNMP_LOG_VERBOSE,
				"SNMP: TFX: Adding view [%d] %s.\n",
				tfxInfo->tfxViews[i].mibView->viewOid.idLength,
				SnmpUtilOidToA(&(tfxInfo->tfxViews[i].mibView->viewOid))
				));

        }

    } else {

        // free structure
        FreeTfxInfo(tfxInfo);

        // reinitialize
        tfxInfo = NULL;
    }

    return (LPVOID)tfxInfo;
}

SNMPAPI
SNMP_FUNC_TYPE
SnmpTfxQuery(
    SnmpTfxHandle        tfxHandle,
    BYTE                 requestType,
    RFC1157VarBindList * vbl,
    AsnInteger *         errorStatus,
    AsnInteger *         errorIndex
    )
{
    SnmpExtQueryList ql;
	SnmpTfxInfo *tfxInfo = tfxHandle;
	int i;

    // initialize
    ql.query  = NULL;
    ql.len    = 0;
    ql.action = requestType;

    // disassemble varbinds
    VarBindListToQueryList(
        (SnmpTfxInfo*)tfxHandle,
        vbl,
        &ql,
        errorStatus,
        errorIndex
        );

    // process queries
    ProcessQueryList(
        (SnmpTfxInfo*)tfxHandle,
        &ql,
        vbl,
        errorStatus,
        errorIndex
        );

    // reassemble varbinds
    QueryListToVarBindList(
        (SnmpTfxInfo*)tfxHandle,
        &ql,
        vbl,
        errorStatus,
        errorIndex
        );

    return TRUE;
}


VOID
SNMP_FUNC_TYPE
SnmpTfxClose(
    SnmpTfxHandle tfxHandle
    )
{
    // simply treat as info and release
    FreeTfxInfo((SnmpTfxInfo *)tfxHandle);
}
