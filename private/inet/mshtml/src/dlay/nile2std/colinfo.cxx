//+-----------------------------------------------------------------------
//  Microsoft OLE/DB to STD Mapping Layer
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:      colinfo.cxx
//  Author:    Ido Ben-Shachar (t-idoben@microsoft.com)
//
//  Contents:  Implementation of GetColumnInfo
//
//  History:
//  08/02/95   Ido       GetColumnInfo returns appropriate flags.
//


#include <dlaypch.hxx>

#ifndef X_ROWSET_HXX_
#define X_ROWSET_HXX_
#include "rowset.hxx"
#endif

#define HIER_PREFIX     _T("^")
#define HIER_PREFIX_LEN (1)
#define HIER_SUFFIX     _T("^")
#define HIER_SUFFIX_LEN (1)

MtDefine(CImpIRowset_astdcolinfo, CImpIRowset, "CImpIRowset::_astdcolinfo");

//+---------------------------------------------------------------------------
//
//  Member:     CacheMetaData (public member)
//
//  Synopsis:   read meta data from OSP into a cache
//
//  Returns:    S_OK                if everything is fine
//              E_FAIL              initialization failed
//
HRESULT
CImpIRowset::CacheMetaData()
{
    HRESULT hr = S_OK;
    DBROWCOUNT cColsTemp;
    CSTDColumnInfo *pInfoIndex;
    DBROWCOUNT i;
    OLEDBSimpleProvider *pOSP = GetpMetaOSP();

    Assert(!_astdcolinfo);
    // Go ahead and replace any previous cache, but this shouldn't happen
    delete [] _astdcolinfo;

    hr = pOSP->getColumnCount(&cColsTemp);
    _cCols = (ULONG)cColsTemp;

    if (hr || _cCols == 0)          // We can't make a valid 0 column rowset
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    cColsTemp = 1 + _cCols;         // bookmark column + OSP columns

    // Allocate cache:
    _astdcolinfo = new(Mt(CImpIRowset_astdcolinfo)) CSTDColumnInfo[cColsTemp];
    if (!_astdcolinfo)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // fill in cache for bookmark column
    pInfoIndex = _astdcolinfo;
    pInfoIndex->dwFlags = DBCOLUMNFLAGS_ISBOOKMARK |
                         DBCOLUMNFLAGS_ISFIXEDLENGTH;
    pInfoIndex->dwType = DBTYPE_I4;                     // bookmark is an integer
    pInfoIndex->cbMaxLength = sizeof(ULONG);            // bookmark is 4 bytes
    pInfoIndex->bstrName = NULL;                        // no name
    pInfoIndex->pChapRowset = NULL;                     // no referenced rowset

    // fill in cache for OSP columns
    for (i = 1, ++pInfoIndex;  i < cColsTemp;  ++i, ++pInfoIndex )
    {
        OSPRW  wRW;
        VARIANT tempVar;

        pInfoIndex->dwFlags = DBCOLUMNFLAGS_ISFIXEDLENGTH;
        hr = pOSP->getRWStatus(OSP_IndexAll, i, &wRW);
        if (!hr && wRW != OSPRW_READONLY)
        {
            pInfoIndex->dwFlags |= DBCOLUMNFLAGS_WRITE;
        }
        
        pInfoIndex->dwType = DBTYPE_VARIANT;            // columns hold variants
        pInfoIndex->cbMaxLength = sizeof(VARIANT);
        pInfoIndex->pChapRowset = NULL;
        pInfoIndex->bstrName = NULL;
        
        FastVariantInit(&tempVar);
        hr = pOSP->getVariant(0, i, OSPFORMAT_FORMATTED, &tempVar);
        if (!hr && tempVar.vt == VT_BSTR)
        {
            ULONG cColNameLen = SysStringLen(tempVar.bstrVal);

            // Detect hierarchy columns.
            if (FormsStringNCmp(HIER_PREFIX, HIER_PREFIX_LEN,
                                tempVar.bstrVal, HIER_PREFIX_LEN) == 0 &&
                FormsStringNCmp(HIER_SUFFIX, HIER_SUFFIX_LEN,
                                tempVar.bstrVal + cColNameLen - HIER_SUFFIX_LEN,
                                HIER_SUFFIX_LEN) == 0)
            {
                pInfoIndex->dwFlags |= DBCOLUMNFLAGS_ISCHAPTER;
                pInfoIndex->dwType = DBTYPE_HCHAPTER;
                pInfoIndex->cbMaxLength = sizeof(IUnknown *);
                // We won't save the prefix
                FormsAllocStringLen(tempVar.bstrVal + HIER_PREFIX_LEN,
                        cColNameLen - (HIER_PREFIX_LEN+HIER_SUFFIX_LEN),
                        &pInfoIndex->bstrName);
            }
            else
            {
                pInfoIndex->bstrName = tempVar.bstrVal;
                tempVar.bstrVal = NULL;     // BSTR now owned by pInfoIndex
            }

            VariantClear(&tempVar);
        }
    }

    // initialize my properties
    // BUGBUG this should be table-driven
    {
        DBPROP prop;

        // we don't wast static data with a copy of DB_NULLID
        // prop.colid = DB_NULLID;
        memset(&prop.colid, 0, sizeof(prop.colid));

        prop.dwOptions = DBPROPOPTIONS_REQUIRED;
        prop.dwStatus  = DBPROPSTATUS_OK;

        // LITERALIDENTITY = False
        prop.dwPropertyID = DBPROP_LITERALIDENTITY;
        V_VT(&prop.vValue) = VT_BOOL;
        V_BOOL(&prop.vValue) = VARIANT_FALSE;
        IGNORE_HR(_dbpProperties.SetProperty(DBPROPSET_ROWSET, prop));

        // STRONGIDENTITY = False
        prop.dwPropertyID = DBPROP_STRONGIDENTITY;
        V_VT(&prop.vValue) = VT_BOOL;
        V_BOOL(&prop.vValue) = VARIANT_FALSE;
        IGNORE_HR(_dbpProperties.SetProperty(DBPROPSET_ROWSET, prop));

        //
        // the next 11 properties are the ones ADO queries (msador15 v1929)
        
        // OTHERUPDATEDELETE = True
        prop.dwPropertyID = DBPROP_OTHERUPDATEDELETE;
        V_VT(&prop.vValue) = VT_BOOL;
        V_BOOL(&prop.vValue) = VARIANT_TRUE;
        IGNORE_HR(_dbpProperties.SetProperty(DBPROPSET_ROWSET, prop));
        
        // OTHERINSERT = True
        prop.dwPropertyID = DBPROP_OTHERINSERT;
        V_VT(&prop.vValue) = VT_BOOL;
        V_BOOL(&prop.vValue) = VARIANT_TRUE;
        IGNORE_HR(_dbpProperties.SetProperty(DBPROPSET_ROWSET, prop));
        
        // CANHOLDROWS = True
        prop.dwPropertyID = DBPROP_CANHOLDROWS;
        V_VT(&prop.vValue) = VT_BOOL;
        V_BOOL(&prop.vValue) = VARIANT_TRUE;
        IGNORE_HR(_dbpProperties.SetProperty(DBPROPSET_ROWSET, prop));
        
        // CANSCROLLBACKWARDS = True
        prop.dwPropertyID = DBPROP_CANSCROLLBACKWARDS;
        V_VT(&prop.vValue) = VT_BOOL;
        V_BOOL(&prop.vValue) = VARIANT_TRUE;
        IGNORE_HR(_dbpProperties.SetProperty(DBPROPSET_ROWSET, prop));
        
        // UPDATABILITY = change | delete | insert
        prop.dwPropertyID = DBPROP_UPDATABILITY;
        V_VT(&prop.vValue) = VT_I4;
        V_I4(&prop.vValue) = DBPROPVAL_UP_CHANGE | DBPROPVAL_UP_DELETE |
                                DBPROPVAL_UP_INSERT;
        IGNORE_HR(_dbpProperties.SetProperty(DBPROPSET_ROWSET, prop));
        
        // IRowsetLocate = True
        prop.dwPropertyID = DBPROP_IRowsetLocate;
        V_VT(&prop.vValue) = VT_BOOL;
        V_BOOL(&prop.vValue) = VARIANT_TRUE;
        IGNORE_HR(_dbpProperties.SetProperty(DBPROPSET_ROWSET, prop));
        
        // IRowsetScroll = True
        prop.dwPropertyID = DBPROP_IRowsetScroll;
        V_VT(&prop.vValue) = VT_BOOL;
        V_BOOL(&prop.vValue) = VARIANT_TRUE;
        IGNORE_HR(_dbpProperties.SetProperty(DBPROPSET_ROWSET, prop));
        
        // IRowsetUpdate = False
        prop.dwPropertyID = DBPROP_IRowsetUpdate;
        V_VT(&prop.vValue) = VT_BOOL;
        V_BOOL(&prop.vValue) = VARIANT_FALSE;
        IGNORE_HR(_dbpProperties.SetProperty(DBPROPSET_ROWSET, prop));
        
        // IRowsetResynch = False
        prop.dwPropertyID = DBPROP_IRowsetResynch;
        V_VT(&prop.vValue) = VT_BOOL;
        V_BOOL(&prop.vValue) = VARIANT_FALSE;
        IGNORE_HR(_dbpProperties.SetProperty(DBPROPSET_ROWSET, prop));
        
        // IConnectionPointContainer = True
        prop.dwPropertyID = DBPROP_IConnectionPointContainer;
        V_VT(&prop.vValue) = VT_BOOL;
        V_BOOL(&prop.vValue) = VARIANT_TRUE;
        IGNORE_HR(_dbpProperties.SetProperty(DBPROPSET_ROWSET, prop));
        
        // BOOKMARKSKIPPED = True
        prop.dwPropertyID = DBPROP_BOOKMARKSKIPPED;
        V_VT(&prop.vValue) = VT_BOOL;
        V_BOOL(&prop.vValue) = VARIANT_TRUE;
        IGNORE_HR(_dbpProperties.SetProperty(DBPROPSET_ROWSET, prop));

        // IRowsetAsynch = ASK THE OSP PROVIDER!
        // BUGBUG:: Can all OSP providers actually answer the question this early?
        {
            BOOL fAsync = FALSE;
            prop.dwPropertyID = DBPROP_IDBAsynchStatus;
            V_VT(&prop.vValue) = VT_BOOL;
            V_BOOL(&prop.vValue) = (VARIANT_BOOL)TRUE; // we do support this interface
            IGNORE_HR(_dbpProperties.SetProperty(DBPROPSET_ROWSET, prop));

            IGNORE_HR(GetpMetaOSP()->isAsync(&fAsync));
            prop.dwPropertyID = DBPROP_ROWSET_ASYNCH;
            V_VT(&prop.vValue) = VT_I4;
            V_I4(&prop.vValue) = fAsync ? (DBPROPVAL_ASYNCH_INITIALIZE |
                                           DBPROPVAL_ASYNCH_SEQUENTIALPOPULATION |
                                           DBPROPVAL_ASYNCH_RANDOMPOPULATION)
                                        : 0 ;
            IGNORE_HR(_dbpProperties.SetProperty(DBPROPSET_ROWSET, prop));
        }
    }

Cleanup:
    if (hr && hr != E_OUTOFMEMORY)
    {
        hr = E_FAIL;
    }
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
// IColumnsInfo specific interfaces
//
////////////////////////////////////////////////////////////////////////////////


//+-----------------------------------------------------------------------
//
//  Member:    GetColumnInfo (public member)
//
//  Synopsis:  Returns information on the rows in a table.  This can be
//             used to discover the number of columns in a table, the
//             types of information stored in each column, and the name/label
//             of each column.  In the case of the STD, columns will only
//             be holding variants, and the names of the columns are stored
//             in row #0 (used for labels).
//             The column information is returned in an array of structures,
//             each corresponding to a column in the table.
//             Note that the memory allocated for paInfo and ppStringsBuffer
//             must be deallocated by the caller of this function.
//             Column 0 does not return information about itself since it
//             is just a holder for labels.  However, element 0
//             of paInfo stores information about the "bookmark column."
//             This column does not actually exist in the rowset, but is
//             used so that accessors to bookmarks can be created.
//
//  Arguments: pcColumns        number of columns in table       (OUT)
//             paInfo           array of column-info structures  (OUT)
//             ppStringsBuffer  storage for strings              (OUT)
//
//  Returns:   Success if column-info can be constructed and returned.
//             Returns E_INVALIDARG if argument pointers are null.
//             Returns E_OUTOFMEMORY if can't allocate memory for structures.
//

STDMETHODIMP
CImpIRowset::GetColumnInfo(DBORDINAL *pcColumns,
                           DBCOLUMNINFO **paInfo,
                           OLECHAR **ppStringsBuffer )
{
    TraceTag((tagNileRowsetProvider,
             "CImpIRowset::GetColumnInfo(%p {%p, %p, %p})",
             this, pcColumns, paInfo, ppStringsBuffer) );

    HRESULT hr = S_OK;
    ULONG cTempInfo;                      // num of cols in STD+1 (for bookmark)
    DBCOLUMNINFO *aTempInfo;              // temp data block for output
    OLECHAR *pTempStringsBuffer;          // temp string-buffer block for output
    ULONG cStringBufferRemaining = 0;     // chars in string-buffer block
    DBCOLUMNINFO *pTempColumn;            // used to index data in for-loop
    ULONG i;
    CSTDColumnInfo *pColInfo;
    OLECHAR *pstrTemp;                  // used to step through str buffer

    // Check that output variables aren't null:
    if (!pcColumns || !paInfo || !ppStringsBuffer)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    cTempInfo = _cCols + 1;

    // In case of error, null outputs:
    *pcColumns = 0;
    *paInfo = NULL;
    *ppStringsBuffer = NULL;

    // we must have valid meta data from the OSP
    if (!_astdcolinfo)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // Allocate memory for structures holding column information:
    aTempInfo =
        (DBCOLUMNINFO *)CoTaskMemAlloc(sizeof(DBCOLUMNINFO) * cTempInfo);
    if (!aTempInfo)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Zero structure:
    memset(aTempInfo, '\0', sizeof(DBCOLUMNINFO) * cTempInfo);

// Now we will fill in the structures with actual column information.
// Since the strings in the table are supposed to be stored in one big
//   block, this pass through the structures will not yet allocate that block,
//   but instead calculate its size.  We will need to do a second pass on the
//   columns to actually copy the strings into the buffer.

    // We need to store information about the bookmarks column:
    pColInfo = _astdcolinfo;
    aTempInfo->columnid.eKind = DBKIND_GUID_PROPID;
    aTempInfo->columnid.uGuid.guid = DBCOL_SPECIALCOL;
    aTempInfo->columnid.uName.ulPropid = 2;             // BUGBUG: oledb.h enum?
    Assert(!aTempInfo->pwszName);                       // no name
    Assert(!aTempInfo->iOrdinal);                       // column number 0 in mapper
    aTempInfo->wType = pColInfo->dwType;                // bookmark is 4 bytes
    Assert(!aTempInfo->pTypeInfo);
    aTempInfo->ulColumnSize = pColInfo->cbMaxLength;    // bookmark is 4 bytes
    aTempInfo->bPrecision = (BYTE)~0;                   // no precision
    aTempInfo->bScale = (BYTE)~0;                       // no scale
    aTempInfo->dwFlags = pColInfo->dwFlags;

    // construct the information about the OSP columns
    for (i = 1, pTempColumn = aTempInfo + 1, ++pColInfo;
         i < cTempInfo;
         ++i, ++pTempColumn, ++pColInfo )
    {
        pTempColumn->columnid.eKind = DBKIND_NAME;      // id is col name
        Assert(!pTempColumn->columnid.uName.pwszName);  // fill this in later
        Assert(!pTempColumn->pwszName);                 // fill this in later
        pTempColumn->iOrdinal = i;
        pTempColumn->wType = pColInfo->dwType;          // columns hold variants
        Assert(!pTempColumn->pTypeInfo);
        pTempColumn->ulColumnSize = pColInfo->cbMaxLength;
        pTempColumn->bPrecision = (BYTE)~0;         // no precision
        pTempColumn->bScale = (BYTE)~0;             // no scale
        pTempColumn->dwFlags = pColInfo->dwFlags;

        // Calculate length of string buffer..
        cStringBufferRemaining += FormsStringLen(pColInfo->bstrName) + 1;
    }

    // Allocate the string buffer:
    pTempStringsBuffer =
                (OLECHAR *)CoTaskMemAlloc(sizeof(OLECHAR) * cStringBufferRemaining);
    if (!pTempStringsBuffer)
    {
        hr = E_OUTOFMEMORY;
        goto Error1;
    }

    // Fill the string buffer:
    pstrTemp = pTempStringsBuffer;
    
    for (i = 1, pTempColumn = aTempInfo + 1, pColInfo = _astdcolinfo + 1;
         i < cTempInfo;
         ++i, ++pTempColumn, ++pColInfo )
    {
        ULONG cTempLengthCopied = FormsStringLen(pColInfo->bstrName);
        
        // Set a pointer to this string:
        pTempColumn->pwszName = pstrTemp;
        // Also set id of column to this string:
        pTempColumn->columnid.uName.pwszName = pstrTemp;

        if (cTempLengthCopied > 0)
        {
            _tcsncpy(pstrTemp, pColInfo->bstrName, cTempLengthCopied);
            pstrTemp += cTempLengthCopied;
        }
        pstrTemp[0] = '\0';
        ++ pstrTemp;
        
        cStringBufferRemaining -= cTempLengthCopied + 1;
    }

    Assert("Buffer should be fully used" &&
        (cStringBufferRemaining == 0) );

    // Set output variables:
    *pcColumns = cTempInfo;
    *paInfo = aTempInfo;
    *ppStringsBuffer = pTempStringsBuffer;

Cleanup:
    return hr;

Error1:
    // NOTE: delete pTempStringsBuffer here if this is called after it exists
    // deallocate array of structures:
    CoTaskMemFree(aTempInfo);
    goto Cleanup;
}


//+-----------------------------------------------------------------------
//
//  Member:    MapColumnIDs
//
//  Synopsis:  Given an array of column IDs, returns an array of column
//             ordinals.  Note that the output array of column ordinals
//             is allocated by the caller, and only filled in by this
//             function.  Given the column ID of the bookmark column,
//             column ordinal 0 is returned.  The value DB_INVALIDCOLUMN
//             is returned as an index if the column ID was invalid.
//
//  Arguments: cColumnIDs       number of IDs in array
//             aColumnIDs       array of column IDs
//             aColumns         array of column ordinals         (OUT)
//
//  Returns:   Success if output array can be filled.
//             E_INVALIDARG if cColumnIDs>0 and no input or output array.
//             DB_S_ERRORSOCCURRED if some but not all column IDs were invalid
//             DB_E_ERRORSOCCURRED if all column IDs were invalid
//

STDMETHODIMP
CImpIRowset::MapColumnIDs(DBORDINAL cColumnIDs,
                          const DBID aColumnIDs [],
                          DBORDINAL aColumns [] )
{
    TraceTag((tagNileRowsetProvider,
             "CImpIRowset::MapColumnIDs(%p {%u, %p, %p})",
             this, cColumnIDs, aColumnIDs, aColumns) );

    HRESULT hr = S_OK;
    DBORDINAL cErrors = 0;             // number of invalid column IDs
    DBORDINAL i;
    const DBID *pTempColID;        // used to index source col ids
    DBORDINAL *pTempColOrd;            // used to index dest col ordinals

    // Check arguments:
    if ((cColumnIDs && !aColumnIDs) ||
        !aColumns)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // must have valid metadata from OSP
    if (!_astdcolinfo)
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    // Now translate column IDs:
    for (i = 0, pTempColID = aColumnIDs, pTempColOrd = aColumns;
         i < cColumnIDs;
         i++, pTempColID++, pTempColOrd++ )
    {
        const GUID *pguid = &pTempColID->uGuid.guid;// useful for bookmark check

        *pTempColOrd = DB_INVALIDCOLUMN;    // assume failure
        
        switch(pTempColID->eKind)
        {
        case DBKIND_PGUID_PROPID:
            pguid = pTempColID->uGuid.pguid;
            // FALL THRU
        case DBKIND_GUID_PROPID:
            if (IsEqualGUID(*pguid, DBCOL_SPECIALCOL)
                    && pTempColID->uName.ulPropid == 2) // BUGBUG: oledb.h enum
            {
                *pTempColOrd = 0;         // bookmark column is 0;
            }
            break;
        case DBKIND_NAME:
            if (pTempColID->uName.pwszName != NULL)
            {
                ULONG uIndex;                      // index into column labels
                ULONG cNameLength = _tcslen(pTempColID->uName.pwszName);

                // Check columns to see if they match string:
                for (uIndex = 1; uIndex <= _cCols; uIndex++)
                {
                    BSTR bstrName = _astdcolinfo[uIndex].bstrName;
                    ULONG cColumnNameLen = FormsStringLen(bstrName);
                    
                    if (FormsIsEmptyString(bstrName))
                    {
                        continue;           // this column does not have a name
                    }
                    
                    // Compare strings:
                    // make sure they're the same lengths for match
                    if ((cNameLength == cColumnNameLen) &&
                        (!_tcsnicmp(pTempColID->uName.pwszName,
                                    cNameLength,
                                    bstrName,
                                    cNameLength )))
                    {
                        *pTempColOrd = uIndex; // found it
                        break;
                    }
                }
            }
            break;
        }

        // See if we didn't find a string match:
        if (*pTempColOrd == DB_INVALIDCOLUMN)
        {
            ++ cErrors;
        }
    }

    hr =    cErrors == 0 ?          S_OK :
            cErrors < cColumnIDs ?  DB_S_ERRORSOCCURRED :
                                    DB_E_ERRORSOCCURRED;

Cleanup:
    return hr;
}
