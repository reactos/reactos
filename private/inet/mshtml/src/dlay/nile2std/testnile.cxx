//+---------------------------------------------------------------------------
//
//  Maintained by: Jerry, Terry, Ted, Charles (Frankston) & Sam (Bent)
//
//  Microsoft Nile/STD provider testing code
//  Copyright (C) Microsoft Corporation, 1994-1995
//  All rights Reserved.
//  Information contained herein is Proprietary and Confidential.
//
//  File:       testnile.cxx
//
//  Contents:   testing code for Nile provider talking to a STD.
//
//  Classes:    None
//
//  Functions:  TestNileSTD
//


#include <dlaypch.hxx>

#ifndef X_ROWSET_HXX_
#define X_ROWSET_HXX_
#include "rowset.hxx"
#endif

#ifndef X_ARRAYI_HXX_
#define X_ARRAYI_HXX_
#include "arrayi.hxx"
#endif

// BUGBUG: this should be in a header file somewhere
HRESULT FormsSTDCreate (OLEDBSimpleProvider **ppSTD, LONG cColumns);
HRESULT FormsSTDCreatePopulate (ULONG cColumns, ULONG cchBuf,
                               OLECHAR *pchBuf, BOOL fLabels,
                               OLEDBSimpleProvider **ppSTD );

enum { MAX_WSTR_LEN = 256 };

// DBTYPE_WSTR w/ length field format (not to exceede MAX_WSTR_LEN).
struct WIDE_STR {
    ULONG   uCount;
    TCHAR   chBuff[MAX_WSTR_LEN];
};

////////////////////////////////////////////////////////////////////////////////
// STD related tests:

//+-----------------------------------------------------------------------
//
//  Member:    CreatePopSTD
//
//  Synopsis:  Populates an STD with data using semicolon-separated string.
//             Verifies that the data inserted is correct.
//
//  Arguments: pSTD         pointer to an initialized STD
//
//  Returns:   Nothing.
//

void
CreatePopSTD (OLEDBSimpleProvider **ppSTD)
{
    HRESULT     hr;
    DBROWCOUNT  uRows, uCols;
    DBROWCOUNT  i, j;
    const ULONG NumRows = 5;
    const ULONG NumCols = 2;
    const ULONG NumElems = (NumRows + 1) * NumCols;  // include column labels
    OLECHAR *paStrings[] =
        { OLESTR("Column #1"),  OLESTR("Column #2"),
          OLESTR("alpha"),      OLESTR("fred"),
          OLESTR("beta"),       OLESTR("bambam"),
          OLESTR("charlie"),    OLESTR("barny"),
          OLESTR("delta"),      OLESTR("barn"),
          OLESTR("epsilon"),    OLESTR("wilma") };
    OLECHAR pBuff[500];
    ULONG cBuffLen;

    // Convert array of strings to semicolon-separated init-string:
    {
        OLECHAR *pBuffIndex = pBuff;
        ULONG cStringLen;

        for (i=0; i < NumElems; i++)
        {
#ifndef _MACUNICODE
            cStringLen = _tcslen(paStrings[i]);
            _tcsncpy(pBuffIndex, paStrings[i], cStringLen);
            pBuffIndex += cStringLen;
            _tcsncpy(pBuffIndex, OLESTR(";"), 1);
#else
            cStringLen = strlen(paStrings[i]);
            strncpy(pBuffIndex, paStrings[i], cStringLen);
            pBuffIndex += cStringLen;
            strncpy(pBuffIndex, OLESTR(";"), 1);
#endif
            pBuffIndex ++;
        }

        cBuffLen = pBuffIndex - pBuff - 1;   // don't want last semicolon
    }

    Verify(!FormsSTDCreatePopulate(2, cBuffLen, pBuff, TRUE, (LPOLEDBSimpleProvider*)ppSTD));

    hr = (*ppSTD)->getRowCount(&uRows);
    hr = (*ppSTD)->getColumnCount(&uCols);
    Assert(!hr && "Couldn't get dimensions.");

    Assert((uRows == 5) && (uCols == 2) && "Dimensions wrong");

    // Verify contents of STD:
    {
        VARIANT var;
        ULONG cStringLen;

        for (j = 0; j <= uRows; j++)
        {
            for (i = 1; i <= uCols; i++)
            {
                VariantInit(&var);
                hr = (*ppSTD)->getVariant(j, i, OSPFORMAT_FORMATTED, &var);
                Assert(!hr && "Couldn't get string.");
                cStringLen= SysStringLen(var.bstrVal);
                Assert((cStringLen == _tcslen(paStrings[j*NumCols+i-1])) &&
                    "String is wrong length.");
                Assert((0 == _tcsncmp((var.bstrVal), cStringLen,
                                (paStrings[j*NumCols+i-1]), cStringLen)) &&
                                      "Strings didn't match.");
                VariantClear(&var);
            }
        }
    }
}



//+-----------------------------------------------------------------------
//
//  Member:    CreateBookmarkAccessor
//
//  Synopsis:  Helper function to create an I4 bookmark accessor for the
//             given Rowset.  Like the rest of the code in this module
//             Asserts, and then dies, on error.
//
//  Arguments: pRowset      pointer to the Rowset
//
//  Returns:   HACCESSOR
//

HACCESSOR
CreateBookmarkAccessor(IRowset *pRowset)
{
    DBBINDING   dbind;
    IAccessor *pAccessor;
    HACCESSOR hAccessorBookmark;
    DBORDINAL uColCount = 0;

    {
        IColumnsInfo *pColumnsInfo;
        DBCOLUMNINFO    *pDBColInfo = NULL;
        TCHAR           *pBuffer = NULL;

        Verify(!pRowset->QueryInterface(IID_IColumnsInfo, (void **)&pColumnsInfo));
        Verify(!pColumnsInfo->GetColumnInfo(&uColCount, &pDBColInfo, &pBuffer) &&
            "couldn't get column info.");
        Assert(pDBColInfo && pBuffer && "GetColumnInfo problem");
        pColumnsInfo->Release();
        CoTaskMemFree(pDBColInfo);
        CoTaskMemFree(pBuffer);
    }
    Verify(!pRowset->QueryInterface(IID_IAccessor, (void **)&pAccessor));
    dbind.iOrdinal = 0;
    dbind.obValue = 0;
    dbind.obLength = 0;
    // dbind.obStatus not set
    // dbind.pTypeInfo not set
    dbind.pObject = 0;
    dbind.pBindExt = 0;
    dbind.dwPart = DBPART_VALUE;
    dbind.dwMemOwner = DBMEMOWNER_CLIENTOWNED;
    dbind.eParamIO = DBPARAMIO_NOTPARAM;
    dbind.cbMaxLen = sizeof(ULONG);
    // dbind.dwFlags not set
    dbind.wType = DBTYPE_I4;
    // dbind.bPrecision not set
    // dbind.bScale not set
    Verify((!pAccessor->CreateAccessor(DBACCESSOR_ROWDATA,
                             1, &dbind, 0,
                             &hAccessorBookmark, NULL )) &&
         "Couldn't create bookmark accessor.");
    pAccessor->Release();
    return hAccessorBookmark;
}

//+-----------------------------------------------------------------------
//
//  Member:    ReleaseAccessor
//
//  Synopsis:  Helper function to destroy an accessor for the
//             given Rowset.  Like the rest of the code in this module
//             Asserts, and then dies, on error.
//
//  Arguments: pRowset      pointer to the Rowset
//             hAccessor    accessor to be destroyed
//

void
ReleaseAccessor(IRowset *pRowset, HACCESSOR hAccessor)
{
    IAccessor *pAccessor;

    Verify(!pRowset->QueryInterface(IID_IAccessor, (void **)&pAccessor));
    pAccessor->ReleaseAccessor(hAccessor, NULL);
    pAccessor->Release();
}

//+-----------------------------------------------------------------------
//
//  Member:    GetHRowFromIndex
//
//  Synopsis:  Helper function for getting hRows for a row from
//             an exact row index.  Since this code used to be based purely
//             on row indexes, it does this a lot.
//
//  Arguments: uRowindex         index of row [Input]
//             puPosition       index of row [Output]
//             pcTotalRows       number of rows [Output]
//
//  Returns:   hr               error code
//

HROW
GetHRowFromIndex(IRowsetLocate *pRowsetlocate, ULONG uRowindex)
{
    HRESULT hr;
    HROW hRow;
    HROW *phRow = &hRow;
    DBCOUNTITEM cRows;
    BYTE FirstBookmark = DBBMK_FIRST; // special bookmark for first row

    hr = pRowsetlocate->GetRowsAt(0, 0, sizeof(FirstBookmark),
                                  &FirstBookmark, uRowindex-1, 1, &cRows, &phRow);
    Assert(!hr && cRows == 1 && "couldn't get row.");

    return hRow;
}


ULONG
GetBookmarkFromIndex(IRowsetLocate *pRowset, ULONG uRowindex)
{
    HRESULT hr;
    HROW hRow;
    ULONG ulBookmark;
    HACCESSOR hAccessorBookmark = CreateBookmarkAccessor(pRowset);
    
    hRow = GetHRowFromIndex(pRowset, uRowindex);
    Assert(hRow && "couldn't get row.");

    hr = pRowset->GetData(hRow, hAccessorBookmark, (BYTE *) &ulBookmark);
    Assert(!hr && "Couldn't get bookmark.");

    ReleaseAccessor(pRowset, hAccessorBookmark);
    pRowset->ReleaseRows(1, &hRow, NULL, NULL, NULL);

    return ulBookmark;
}

//+-----------------------------------------------------------------------
//
//  Member:    TestGetDataAtRowNum
//
//  Synopsis:  Helper function to fetch data from a specific row number
//             of a wrapped-STD rowset.
//             Like the rest of the code in this module
//             Asserts, and then dies, on error.
//
//  Arguments: pRowset      pointer to the Rowset
//             i            row number (one based)
//             hAccessor    Accessor to use for data fetch
//             pData        where to put data read
//

void
TestGetDataAtRowNum(IRowset *pRowset,
                     ULONG i, HACCESSOR hAccessor, BYTE *pData)
{
    HROW hRow;
    HROW *phRow = &hRow;
    HRESULT hr;
    IRowsetLocate *pRowsetLocate;

    Verify(!pRowset->QueryInterface(IID_IRowsetLocate, (void **) &pRowsetLocate));

    hRow = GetHRowFromIndex(pRowsetLocate, i);
    hr = pRowsetLocate->GetData(hRow, hAccessor, pData);
    Assert(!hr && "couldn't fetch data.");
    hr = pRowsetLocate->ReleaseRows(1, phRow, NULL, NULL, NULL);
    Assert(!hr && "couldn't release row.");

    pRowsetLocate->Release();
}

//+-----------------------------------------------------------------------
//
//  Member:    TestSetDataAtRowNum
//
//  Synopsis:  Helper function to Set data into a specific row number
//             of a wrapped-STD rowset.  "Knows" that bookmark == row number.
//             Like the rest of the code in this module
//             Asserts, and then dies, on error.
//
//  Arguments: pRowset      pointer to the Rowset
//             i            row number
//             hAccessor    Accessor to use for data fetch
//             pData        data to write
//

void
TestSetDataAtRowNum(IRowsetChange *pRowsetChange,
                     ULONG i, HACCESSOR hAccessor, BYTE *pData)
{
    HROW hRow;
    HROW *phRow = &hRow;
    HRESULT hr;
    IRowsetLocate *pRowsetLocate;

    Verify(!pRowsetChange->QueryInterface(IID_IRowsetLocate, (void **) &pRowsetLocate));

    hRow = GetHRowFromIndex(pRowsetLocate, i);
    hr = pRowsetChange->SetData(hRow, hAccessor, pData);
    Assert(!hr && "couldn't set data.");
    hr = pRowsetLocate->ReleaseRows(1, phRow, NULL, NULL, NULL);
    Assert(!hr && "couldn't release row.");

    pRowsetLocate->Release();
}

#ifdef NEVER
// Temporarily take out find test
HRESULT
FindPrefixTester(OLEDBSimpleProvider *pSTD,
                 ULONG iRowStart, ULONG iColumn,
                 ULONG cchBuf, OLECHAR *pchBuf,
                 DWORD findFlags,
                 OSPFIND *pfoundFlag, LONG *piRowFound)
{
    HRESULT hr;
    BSTR bstrTemp;

    bstrTemp = SysAllocStringLen(pchBuf, cchBuf);
    if (!bstrTemp)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = pSTD->FindPrefixString((LONG) iRowStart, (LONG) iColumn,
                                bstrTemp,
                                (STDFIND) findFlags,
                                pfoundFlag, piRowFound);

    SysFreeString(bstrTemp);

Cleanup:
    return hr;
}
#endif



//+-----------------------------------------------------------------------
//
//  Member:    CheckFindSTD
//
//  Synopsis:  Tests FindPrefixString, to ensure that it can find the correct
//             strings in the STD populated above.
//
//  Arguments: pSTD         pointer to an initialized STD
//
//  Returns:   Nothing.
//

void
CheckFindSTD(OLEDBSimpleProvider *pSTD)
{
#ifdef NEVER
    HRESULT hr;
    OSPFIND FoundFlag;
    LONG iRowFound;

    Assert(pSTD);
    // check case sensitive:
    hr = FindPrefixTester(pSTD, 1, 1, _tcslen(_T("aLpHa")), _T("aLpHa"),
        OSPFIND_EXACT | OSPFIND_CASESENSITIVE,
        &FoundFlag, &iRowFound);
    Assert(hr && "Shouldn't have matched, case sensitive.");

    hr = FindPrefixTester(pSTD, 1, 1, _tcslen(_T("aLpHa")), _T("aLpHa"),
        OSPFIND_EXACT,
        &FoundFlag, &iRowFound);
    Assert(!hr && (FoundFlag == OSPFIND_EXACT) && (iRowFound == 1) &&
        "Case sensitive test failed.");

    // check simple prefix match:
    hr = FindPrefixTester(pSTD, 1, 2, _tcslen(_T("ba")), _T("ba"),
        OSPFIND_PREFIX,
        &FoundFlag, &iRowFound);
    Assert(!hr && (FoundFlag == OSPFIND_PREFIX) && (iRowFound == 2) &&
        "Find prefix test failed.");

    // check reverse scan:
    hr = FindPrefixTester(pSTD, 5, 2, _tcslen(_T("ba")), _T("ba"),
        OSPFIND_PREFIX | OSPFIND_UP,
        &FoundFlag, &iRowFound);
    Assert(!hr && (FoundFlag == OSPFIND_PREFIX) && (iRowFound == 4) &&
        "Reverse scan test failed.");

    // check simple exact match:
    hr = FindPrefixTester(pSTD, 1, 1, _tcslen(_T("charlie")), _T("charlie"),
        OSPFIND_EXACT,
        &FoundFlag, &iRowFound);
    Assert(!hr && (FoundFlag == OSPFIND_EXACT) && (iRowFound == 3) &&
        "Find exact test failed.");

    // check prefix-last match
    hr = FindPrefixTester(pSTD, 1, 2, _tcslen(_T("ba")), _T("ba"),
        OSPFIND_PREFIX | OSPFIND_LAST,
        &FoundFlag, &iRowFound);
    Assert(!hr && (FoundFlag == OSPFIND_PREFIX) && (iRowFound == 4) &&
        "Find prefix-last test failed.");

    // check prefix-exact match
    hr = FindPrefixTester(pSTD, 1, 2, _tcslen(_T("barn")), _T("barn"),
        OSPFIND_PREFIX | OSPFIND_EXACT,
        &FoundFlag, &iRowFound);
    Assert(!hr && (FoundFlag == OSPFIND_EXACT) && (iRowFound == 4) &&
        "Find prefix-exact test failed.");

    // check nearest match
    hr = FindPrefixTester(pSTD, 1, 1, _tcslen(_T("betty")), _T("betty"),
        OSPFIND_EXACT | OSPFIND_NEAREST,
        &FoundFlag, &iRowFound);
    Assert(!hr && (FoundFlag == OSPFIND_NEAREST) && (iRowFound == 3) &&
        "Find nearest test failed.");
#endif
}


//+-----------------------------------------------------------------------
//
//  Member:    TestSTD
//
//  Synopsis:  Tests out some STD functionality, without a rowset.
//
//  Arguments: None.
//
//  Returns:   Nothing.
//

void
TestSTD()
{
    OLEDBSimpleProvider  *pSTD;

    CreatePopSTD(&pSTD);
    Assert(pSTD);

    CheckFindSTD(pSTD);

    ReleaseInterface(pSTD);
}


////////////////////////////////////////////////////////////////////////////////
// STD/Nile-mapper related tests:

#define     MaxColumns              2
#define     ColumnBaseName          _T("Column %i")

#define         Col1MaxRows         20
#define             Col1StartValue  100
#define             Col1Incr        50
#define             Col1Type        VT_I4

#define         Col2MaxRows         10
#define             Col2StartValue  _T("Cell %i")
#define             Col2Type        VT_BSTR

#define     LargestRow              Col1MaxRows + Col2MaxRows


//+-----------------------------------------------------------------------
//
//  Member:    CreateAndFillSTD
//
//  Synopsis:  Fills STD with data.  Column #1 is filled with incremental
//             integer values.  Column #2 is filled with strings naming the
//             cells.
//
//  Arguments: pSTD        STD to populate
//
//  Returns:   Nothing.
//

void
CreateAndFillSTD (OLEDBSimpleProvider *pSTD)
{
    HRESULT     hr;
    DBROWCOUNT  lRows, lCols;
    TCHAR       cBuffer[50];
    VARIANT     var;

    hr = pSTD->getColumnCount(&lCols);

    Assert(!hr && "Couldn't insert columns.");
    {
        for (LONG c = 1; c <= lCols; c++)
        {
            wsprintf(cBuffer, ColumnBaseName, c);
            VariantInit(&var);
            var.vt = VT_BSTR;
            var.bstrVal = SysAllocString(cBuffer);
            
            hr = pSTD->setVariant(0,  // STD_IndexLabel
                                  c,
                                  OSPFORMAT_FORMATTED,
                                  var);
            Assert(!hr && "Set column name failed.");
            VariantClear(&var);
        }
    }

    ////////////////////////////////////////////////////////////////
    // Column #1 population
    hr = pSTD->insertRows(1, Col1MaxRows, &lRows);
    Assert(!hr && "Couldn't insert rows.");

    {
        int     i = Col1StartValue;
        for (LONG r = 1; r <= lRows; r++)
        {
            var.vt = Col1Type;
            var.lVal = i;

            hr = pSTD->setVariant(r, 1, OSPFORMAT_RAW, var);
            Assert(!hr && "Set variant failed.");

            i += Col1Incr;
        }
    }

    ////////////////////////////////////////////////////////////
    // Column #2 population
    hr = pSTD->insertRows(Col1MaxRows + 1,
                          Col2MaxRows,
                          &lRows);
    Assert(!hr && "Couldn't insert rows.");

    {
        for (LONG r = 1; r <= lRows; r++)
        {
            wsprintf(cBuffer, Col2StartValue, r);
            VariantInit(&var);
            var.vt = VT_BSTR;
            var.bstrVal = SysAllocString(cBuffer);
            hr = pSTD->setVariant(r, 2,
                                 OSPFORMAT_FORMATTED,
                                 var);
            Assert(!hr && "Set string failed.");
            VariantClear(&var);
        }
    }

}


//+-----------------------------------------------------------------------
//
//  Member:    VerifySTDContents
//
//  Synopsis:  Checks STD data to see if it is correct.
//
//  Arguments: pSTD             pointer to an STD
//
//  Returns:   Nothing.
//

void
VerifySTDContents(OLEDBSimpleProvider *pSTD)
{
    HRESULT hr;
    DBROWCOUNT lRows, lCols;

    Assert(pSTD);
    // Check dimensions
    hr = pSTD->getRowCount(&lRows);
    Assert(!hr && "GetRowCount failed.");
    hr = pSTD->getColumnCount(&lCols);
    Assert(!hr && "GetColumnCount failed.");

    Assert((lCols == MaxColumns) && "STD columns is incorrect.");

    Assert((lRows == LargestRow) && "STD rows is incorrect.");

    // Validate column names
    {
        TCHAR   cBuffer[50];
        VARIANT var;

        for (LONG c = 1; c <= lCols; c++)
        {
            wsprintf(cBuffer, ColumnBaseName, c);

            VariantInit(&var);
            hr = pSTD->getVariant(OSP_IndexLabel, c, OSPFORMAT_FORMATTED,
                                  &var);
            Assert(!hr && "Get column name failed.");
            Assert(var.bstrVal && "Get column name empty");

            Assert(!StrCmpC(cBuffer, var.bstrVal) && "Column name didn't match.");
            VariantClear(&var);
        }

    }

    ///////////////////////////////////////////////////////////////
    // Column #1 Validation
    {
        int     i = Col1StartValue;
        VARIANT var;
        LONG   r;

        for (r = 1; r <= lRows; r++)
        {
            VariantInit(&var);          // for safety.
            hr = pSTD->getVariant(r, 1, OSPFORMAT_RAW, &var);
            Assert(!hr && "Couldn't get variant.");

            if (r <= Col1MaxRows)
            {
                Assert((i == var.lVal) && (var.vt == Col1Type) &&
                    "Col1 data mismatch.");
            }
            else
            {
                Assert((var.vt == VT_NULL) && "Cell should be null.");
            }

            VariantClear(&var);

            i += Col1Incr;
        }
    }

    ////////////////////////////////////////////////////////////
    // Column #2 Validation
    {
        TCHAR cBuff[50];
        VARIANT var;

        var.vt = VT_EMPTY;
        for (LONG r = 1; r <= lRows; r++)
        {
            wsprintf(cBuff, Col2StartValue, r);

            if (r <= Col2MaxRows)
            {
                VariantInit(&var);
                hr = pSTD->getVariant(r, 2, OSPFORMAT_FORMATTED, &var);
                Assert(!hr && "Couldn't get string.");
                Assert(var.bstrVal && "Couldn't get bstr");

                Assert(!StrCmpC(cBuff, var.bstrVal) && "Cell name didn't match.");
                VariantClear(&var);
            }
            else
            {
                VariantInit(&var);
                hr = pSTD->getVariant(r, 2, OSPFORMAT_RAW, &var);
                Assert(!hr && "Couldn't get variant.");
                Assert((var.vt == VT_NULL) && "Cell should be null.");

                VariantClear(&var);
            }
        }
    }
}


//+-----------------------------------------------------------------------
//
//  Member:    CheckColumnsInfo
//
//  Synopsis:  Checks info generated by GetColumnInfo.
//
//  Arguments: pRowset            an initialized rowset
//
//  Returns:   Nothing.
//

void
CheckColumnsInfo (IRowset *pRowset)
{
    HRESULT hr;
    IColumnsInfo *pColInfo = NULL;

    Assert(pRowset);
    hr = pRowset->QueryInterface(IID_IColumnsInfo, (void **)&pColInfo);
    Assert(!hr && "Couldn't get IColumnsInfo interface.");

    Assert(pColInfo);

    DBORDINAL        uColCount = 0;
    DBCOLUMNINFO    *pDBColInfo = NULL;
    TCHAR           *pBuffer = NULL;

    hr = pColInfo->GetColumnInfo(&uColCount, &pDBColInfo, &pBuffer);
    Assert(!hr && "Couldn't get column info.");
    Assert(pDBColInfo && pBuffer && "GetColumnInfo problem");
    pColInfo->Release();

    DBCOLUMNINFO * pTmpColInfo;

    Assert((uColCount == MaxColumns + 1) &&
        "GetColumnInfo column count problem");

    pTmpColInfo = pDBColInfo;
    for (ULONG i = 0; i < uColCount; i++)
    {
        // Currently the 0 element is the bookmark column.
        if (i == 0)
        {
            // Validate it's a bookmark.
            // BUGBUG: oledb.h should have an enum value for ulPropid 2
            Assert((pTmpColInfo->columnid.eKind ==
                    DBKIND_GUID_PROPID)                                 &&
                (pTmpColInfo->columnid.uGuid.guid == DBCOL_SPECIALCOL)  &&
                (pTmpColInfo->columnid.uName.ulPropid == 2)             &&
                "Bookmark not first entry");

            // For now bookmarks are 4 bytes in length.
            Assert((pTmpColInfo->ulColumnSize == sizeof(ULONG)) &&
                "Bookmark wrong size");
        }
        else
        {
            TCHAR   cBuffer[50];

            Assert((pTmpColInfo->iOrdinal == i) && "Wrong column ordinal");

            wsprintf(cBuffer, ColumnBaseName, i);

            Assert(!StrCmpC(cBuffer, pTmpColInfo->pwszName) &&
                "Column has wrong name");

            Assert((pTmpColInfo->wType == DBTYPE_VARIANT) &&
                "Column has wrong type");

            Assert((pTmpColInfo->ulColumnSize == sizeof(VARIANT)) &&
                "Column has wrong length");
        }

        pTmpColInfo++;
    }

    CoTaskMemFree(pDBColInfo);
    CoTaskMemFree(pBuffer);
}


//+-----------------------------------------------------------------------
//
//  Member:    CheckMapColumnIDs
//
//  Synopsis:  Checks to make sure MapColumnIDs works.
//
//  Arguments: pRowset            an initialized rowset
//
//  Returns:   Nothing.
//

void
CheckMapColumnIDs (IRowset *pRowset)
{
    HRESULT hr;
    DBID ColumnIDs[MaxColumns + 1];     // include bookmark
    DBORDINAL ColumnOrds[MaxColumns + 1];
    TCHAR StringBuff[500];
    TCHAR *pStringIndex = StringBuff;
    DBID *pColumnID;
    ULONG i;
    IColumnsInfo *pColumnsInfo;

    Verify(!pRowset->QueryInterface(IID_IColumnsInfo, (void **)&pColumnsInfo));

    pColumnID = ColumnIDs + MaxColumns;
    pColumnID->eKind = DBKIND_GUID_PROPID;
    pColumnID->uGuid.guid = DBCOL_SPECIALCOL;
    pColumnID->uName.ulPropid = 2;        // BUGBUG: oledb.h enum value?

    for (i = 1, pColumnID = ColumnIDs;
         i <= MaxColumns;
         i++, pColumnID++ )
    {
        pColumnID->eKind = DBKIND_NAME;
        pColumnID->uName.pwszName = pStringIndex;
        wsprintf(pStringIndex, ColumnBaseName, i);
        pStringIndex += _tcslen(pStringIndex) + 1;  // null terminated
    }

    hr = pColumnsInfo->MapColumnIDs(MaxColumns + 1, ColumnIDs, ColumnOrds);
    Assert(!hr && "MapColumnIDs failed.");

    for (i = 0; i < MaxColumns; i++)
    {
        Assert((ColumnOrds[i] == i + 1) && "Column ordinal is incorrect.");
    }

    Assert((ColumnOrds[MaxColumns] == 0) &&
        "Bookmark column ordinal is incorrect.");

    pColumnsInfo->Release();
}


//+-----------------------------------------------------------------------
//
//  Member:    VerifyRowCount
//
//  Synopsis:  Makes sure mapper and STD have same number of rows.  Helper
//             function for TestSTDEvents.
//
//  Arguments: pRowset            an initialized rowset
//             pSTD               STD contained in the rowset
//
//  Returns:   Nothing.
//

void VerifyRowCount(IRowset *pRowset, OLEDBSimpleProvider *pSTD)
{
    HRESULT hr;
    DBROWCOUNT cRows, cColumns;
    DBCOUNTITEM cRowsMapper, uPosition;
    IRowsetExactScroll *pRowsetExactScroll;
    BYTE bBmk = DBBMK_FIRST;

    Verify(!pRowset->QueryInterface(IID_IRowsetExactScroll, (void **)&pRowsetExactScroll));

    // Check to make sure the cache and the STD have same info:
    hr = pRowsetExactScroll->GetExactPosition(NULL, 1, &bBmk,
                                   &uPosition, &cRowsMapper );
    Assert(!hr && "Couldn't get position.");

    hr = pSTD->getRowCount(&cRows);
    Assert(!hr && "Couldn't get dimensions.");
    hr = pSTD->getColumnCount(&cColumns);
    Assert(!hr && "Couldn't get dimensions.");

    Assert((cRows == (LONG)cRowsMapper) && "Mapper and STD info don't match.");

    pRowsetExactScroll->Release();
}


//+-----------------------------------------------------------------------
//
//  Member:    TestSTDEvents
//
//  Synopsis:  Checks to make sure that the rowset's cache stays up to date
//             with the STD when STD fires events.
//
//  Arguments: pRowset            an initialized rowset
//             pSTD               STD contained in the rowset
//
//  Returns:   Nothing.
//

void TestSTDEvents(IRowset *pRowset, OLEDBSimpleProvider *pSTD)
{
    HRESULT hr;
    DBBINDING   dbind;
    HACCESSOR   hAccessor;
    VARIANT     var;
    HROW        hRow;
    IAccessor       *pAccessor;
    IRowsetChange	*pRowsetChange;

    Assert(pRowset && pSTD);

    Verify(!pRowset->QueryInterface(IID_IAccessor, (void **)&pAccessor));

    dbind.iOrdinal = 1;
    dbind.obValue = 0;
    dbind.obLength = 0;
    dbind.pObject = 0;
    dbind.pBindExt = 0;
    dbind.dwPart = DBPART_VALUE;
    dbind.dwMemOwner = DBMEMOWNER_CLIENTOWNED;
    dbind.eParamIO = DBPARAMIO_NOTPARAM;
    dbind.cbMaxLen = sizeof(VARIANT);
    dbind.wType = DBTYPE_VARIANT;

    var.vt = VT_NULL;                     // init the variant

    VerifyRowCount(pRowset, pSTD);        // make sure row STD and mapper match

    Verify((!pAccessor->CreateAccessor(DBACCESSOR_ROWDATA,
                                 1,
                                 &dbind,
                                 0,
                                 &hAccessor,
                                 NULL )) &&
        "Couldn't create accessor.");

    Verify(!pRowset->QueryInterface(IID_IRowsetChange, (void **)&pRowsetChange));

    // Insert a row:
    hr = pRowsetChange->InsertRow(NULL,
                             hAccessor,
                             (BYTE *)&var,
                             &hRow );
    Assert(!hr && "Couldn't add row.");

    VerifyRowCount(pRowset, pSTD);        // make sure row STD and mapper match

    hr = pRowsetChange->DeleteRows(0, 1, &hRow, 0);
    Assert(!hr && "Couldn't delete row.");

    VerifyRowCount(pRowset, pSTD);        // make sure row STD and mapper match

    Verify((!pAccessor->ReleaseAccessor(hAccessor, NULL)) &&
        "Couldn't release accessor.");
    Verify((!pRowset->ReleaseRows(1, &hRow, NULL, NULL, NULL)) &&
        "Couldn't release row.");

    VerifySTDContents(pSTD);

    pRowsetChange->Release();
    pAccessor->Release();
}


//+-----------------------------------------------------------------------
//
//  Member:    TestAddRows
//
//  Synopsis:  Adds rows to the rowset using SetNewDataAfter().
//             Adds new row after every row, which contains a variant in
//             column 1.  Because the rowset changes after each
//             row insert, we must start at the end of the rowset, and
//             work backwards.
//
//  Arguments: pRowset            an initialized rowset
//             pSTD               STD contained in the rowset
//
//  Returns:   Nothing.
//

void
TestAddRows (IRowset *pRowset, OLEDBSimpleProvider *pSTD)
{
    HRESULT     hr;
    DBBINDING   dbind;
    HACCESSOR   hAccessor;
    HACCESSOR   hAccessorBookmark;
    ULONG       ulBookmark;
    VARIANT     var;
    HROW        hRow;
    IAccessor  *pAccessor;
    IRowsetNewRowAfter *pRowsetNewRowAfter;
    IRowsetLocate *pRowsetLocate;
    DBCOUNTITEM cRows;
    HROW        hRow2;
    HROW        *phRow2=&hRow2;
    ULONG       ulBookmark2;
    VARIANT     var2;

    var2.vt = VT_EMPTY; // need to initialize 
    var2.lVal = 0;

    Assert(pRowset && pSTD);
    VerifySTDContents(pSTD);

    Verify(!pRowset->QueryInterface(IID_IAccessor, (void **)&pAccessor));
    Verify(!pRowset->QueryInterface(IID_IRowsetNewRowAfter, (void **)&pRowsetNewRowAfter));
    Verify(!pRowset->QueryInterface(IID_IRowsetLocate, (void **)&pRowsetLocate));

    dbind.iOrdinal = 1;
    dbind.obValue = 0;
    dbind.obLength = 0;
    dbind.pObject = 0;
    dbind.pBindExt = 0;
    dbind.dwPart = DBPART_VALUE;
    dbind.dwMemOwner = DBMEMOWNER_CLIENTOWNED;
    dbind.eParamIO = DBPARAMIO_NOTPARAM;
    dbind.cbMaxLen = sizeof(VARIANT);
    dbind.wType = DBTYPE_VARIANT;

    Verify((!pAccessor->CreateAccessor(DBACCESSOR_ROWDATA,
                                 1,
                                 &dbind,
                                 0,
                                 &hAccessor,
                                 NULL )) &&
        "Couldn't create accessor.");

    hAccessorBookmark = CreateBookmarkAccessor(pRowset);

    var.vt = VT_I4;
    // No more going backwards!
    for (ULONG i = 1; i < LargestRow; i+=2 )
    {
        var.lVal = (i+1) * 2;           // +1 for "After"
        
        // Get an hRow for row i
        hRow = GetHRowFromIndex(pRowsetLocate, i);

        // Get a bookmark for row i
        ulBookmark = GetBookmarkFromIndex(pRowsetLocate, i);

        if (i+1 < LargestRow)
        {
            // Get a bookmark for row i+1
            ulBookmark2 = GetBookmarkFromIndex(pRowsetLocate, i+1);

            // Get the data for i+1
            hRow2 = GetHRowFromIndex(pRowsetLocate, i+1);
            hr = pRowset->GetData(hRow2, hAccessor, &var2);
            Assert(!hr && "Couldn't get data for row i+1");

            pRowset->ReleaseRows(1, &hRow2, NULL, NULL, NULL);
        }

        hr = pRowsetNewRowAfter->SetNewDataAfter(NULL,
                                      sizeof(ulBookmark),
                                      (BYTE *)&ulBookmark,
                                      hAccessor,
                                      (BYTE *)&var,
                                      &hRow );
        Assert(!hr && "Couldn't create new row.");

        // Get a bookmark to the new row
        hr = pRowset->GetData(hRow, hAccessorBookmark, (BYTE *) &ulBookmark);
        Assert(!hr && "couldn't get bookmark.");

        // The real test is whether we can retrive the data for the row AFTER
        // the insertion and still get the same data back..
        if (i+1 < LargestRow)
        {
            // Get a new hRow2 using our bookmark for the old i+1
            hr = pRowsetLocate->GetRowsAt(0, 0, sizeof(ulBookmark2),
                                          (BYTE *) &ulBookmark2, 0, 1,
                                          &cRows, &phRow2);
            Assert(!hr && "Couldn't get a handle from bookmark");
            
            // Get the data from the book mark
            hr = pRowset->GetData(hRow2, hAccessor, &var);
            Assert(!hr && "Couldn't get data for row i+1");
            Assert((var.lVal == var2.lVal) &&
                    "Data does not compare after SetNewDataAfter");

            pRowset->ReleaseRows(1, &hRow2, NULL, NULL, NULL);
        }

        pRowset->ReleaseRows(1, &hRow, NULL, NULL, NULL);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Verify that rows contain new data
    for (i = 2; i <= LargestRow; i+=2)
    {
        TestGetDataAtRowNum(pRowsetLocate, i, hAccessor, (BYTE *) &var);
        Assert((var.vt == VT_I4) && "Variant has wrong type.");
        Assert(((ULONG)var.lVal == i * 2) && "Variant has wrong value.");
    }


    Verify((!pAccessor->ReleaseAccessor(hAccessor, NULL)) &&
        "Couldn't release accessor.");
    Verify((!pAccessor->ReleaseAccessor(hAccessorBookmark, NULL)) &&
        "Couldn't release bookmark accessor.");

    pRowsetNewRowAfter->Release();
    pRowsetLocate->Release();
    pAccessor->Release();
}


//+-----------------------------------------------------------------------
//
//  Member:    TestDeleteRows
//
//  Synopsis:  Deletes rows from the rowset using DeleteRows().  After
//             TestAddRows, this should restore the contents of the rowset
//             to its original state.
//             We will make three calls to DeleteRows.  The first call will be
//             to delete only 1 row, but the row will be invalid, so an error
//             should be returned.  The second call will correctly delete 1
//             row, the last.  The third call will delete multiple rows.  It
//             will also try to delete 1 invalid row and also 1 duplicate row.
//             Error information will be checked to verify this.
//
//  Arguments: pRowset            an initialized rowset
//             pSTD               STD contained in the rowset
//
//  Returns:   Nothing.
//

void
TestDeleteRows (IRowset *pRowset, OLEDBSimpleProvider *pSTD)
{
    HRESULT hr;
    HROW ahRows[LargestRow + 1];             // leave room for invalid HROWs
    HROW *phRow;
    DBCOUNTITEM cRows;
    DBROWSTATUS aRowStatus[LargestRow + 1];
    ULONG i;
    HACCESSOR hAccessorBookmark;
    IRowsetLocate   *pRowsetLocate;
    IRowsetChange	*pRowsetChange;
    ULONG j;
    BYTE FirstBookmark = DBBMK_FIRST;   // special bookmark for first row
    BYTE LastBookmark = DBBMK_LAST;     // special bookmark for last row
    HROW hRow2, hRow3;
    HROW *phRow2=&hRow2, *phRow3=&hRow3;
    ULONG ulBookmark1, ulBookmark2;
    DBCOMPARE dwComparison;

    Assert(pRowset && pSTD);

    Verify(!pRowset->QueryInterface(IID_IRowsetChange, (void **)&pRowsetChange));
    Verify(!pRowset->QueryInterface(IID_IRowsetLocate, (void **)&pRowsetLocate));
    hAccessorBookmark = CreateBookmarkAccessor(pRowset);

    // Attempt to delete invalid row:
    *ahRows = (HROW)(0);
    hr = pRowsetChange->DeleteRows(0, 1, ahRows, 0);
    Assert(hr == DB_E_ERRORSOCCURRED && "Should have gotten error.");

    phRow = ahRows;

    hr = pRowsetLocate->GetRowsAt(NULL, NULL, sizeof(LastBookmark),
                                  &LastBookmark, 0, 1, &cRows, &phRow);
    Assert(!hr && cRows == 1 && "couldn't fetch last row via bookmark.");

    // Get a bookmark for last row.
    hr = pRowset->GetData(ahRows[0], hAccessorBookmark, (BYTE *) &ulBookmark2);
    Assert(!hr && "Couldn't get bookmark.");

    pRowset->ReleaseRows(1, phRow, NULL, NULL, NULL);

#if 0
    // In M6.1 version of spec, we can't compare against FirstBookmark or
    // Lastbookmark, so skip this code for now -cfranks

    // Compare the bookmark we got with last row & first row bookmarks:
    hr = pRowsetLocate->Compare(NULL, sizeof(LastBookmark),
                                &LastBookmark, sizeof(ulBookmark2),
                                (BYTE *)&ulBookmark2, &dwComparison);
    Assert(!hr && "Bookmark compare failed");
    Assert((dwComparison==DBCOMPARE_EQ) && "Bookmarks don't compare");
#endif

    ulBookmark1 = GetBookmarkFromIndex(pRowsetLocate, 2); // get bookmark for row 2

    hr = pRowsetLocate->Compare(NULL, sizeof(ulBookmark1),
                                (BYTE *)&ulBookmark1, sizeof(ulBookmark2),
                                (BYTE *)&ulBookmark2, &dwComparison);
    Assert(!hr && "Bookmark compare failed");
    Assert((dwComparison==DBCOMPARE_LT) && "Bookmarks comparison incorrect");

    // Get a handle to the last row added to the table.
    hr = pRowsetLocate->GetRowsAt(NULL, NULL, sizeof(FirstBookmark),
                                  &FirstBookmark, (LargestRow&~1)-1, 1, &cRows, &phRow2);
    Assert(!hr && cRows == 1 && "couldn't fetch last row added.");

    // Get a bookmark for the same row
    hr = pRowset->GetData(hRow2, hAccessorBookmark, (BYTE *) &ulBookmark2);
    Assert(!hr && "Couldn't get bookmark.");

    // Delete it now so the next deletion can get the error when it
    // tries to delete it again.
    hr = pRowsetChange->DeleteRows(0, 1, &hRow2, 0);
    Assert(!hr && "Couldn't delete row.");

    // Try to get the handle again for the same row again via a bookmark
    hr = pRowsetLocate->GetRowsAt(NULL, NULL, sizeof(ulBookmark2),
                                  (BYTE *)&ulBookmark2, 0, 1, &cRows, &phRow3);
    Assert(hr==DB_S_BOOKMARKSKIPPED && cRows==1 &&
           "DB_S_BOOKMARKSKIPPED not returned, or no next row.");

    // Release the handle
    pRowset->ReleaseRows(1, phRow3, NULL, NULL, NULL);

    // Put the handle we just used to delete the row as the first element of
    // the handle array.  Otherwise, if we get a new handle for the row index,
    // we'll automatically get one to a non-deleted row.
    ahRows[0] = hRow2;

    // Delete all the added rows.
    j = 1;
    for (i = 2; i <= LargestRow-2; i+=2 )
    {
        phRow = &ahRows[j++];
        hr = pRowsetLocate->GetRowsAt(0, 0, sizeof(FirstBookmark),
                                      &FirstBookmark, i-1, 1, &cRows, &phRow);
        Assert(!hr && cRows == 1 && "Couldn't fetch row.");
    }

    ahRows[j] = (HROW)(0);              // invalid row

    hr = pRowsetChange->DeleteRows(0, j + 1, ahRows, aRowStatus);
    Assert((hr == DB_S_ERRORSOCCURRED) &&
        "Should have gotten DB_S_ERRORSOCCURRED error.");
    Assert(aRowStatus[0] == DBROWSTATUS_E_DELETED &&
		"Duplicate row error information is wrong.");
	for (i=1; i<j; ++i)
	{
		Assert(aRowStatus[i] == DBROWSTATUS_S_OK &&
				"Delete failed on valid row");
	}
	Assert(aRowStatus[j] == DBROWSTATUS_E_INVALID &&
		"Invalid row error information is wrong.");

    pRowset->ReleaseRows(1, phRow2, NULL, NULL, NULL);
    Verify(!pRowset->ReleaseRows(j, ahRows, NULL, NULL, NULL) &&
            "couldn't release deleted rows.");

    VerifySTDContents(pSTD);

    pRowsetChange->Release();
    pRowsetLocate->Release();
    ReleaseAccessor(pRowset, hAccessorBookmark);
}


//+-----------------------------------------------------------------------
//
//  Member:    TestGetData
//
//  Synopsis:  Tests GetData to make sure that it doesn't clobber memory.
//             A buffer holding a pattern is used to get data into.  It
//             is then checked to make sure that only the requested data and
//             no more was filled into the buffer by GetData.  This is done
//             on a variety of accessor bindings of different lengths.
//             NOTE: We use the last cell in column 1 for these tests.
//
//  Arguments: pRowset            an initialized rowset
//             pSTD               simple tabular data object
//
//  Returns:   Nothing.
//

void
TestGetData (IRowset *pRowset, OLEDBSimpleProvider *pSTD)
{
    HRESULT     hr;
    DBBINDING   dbind;
    HACCESSOR   hAccessor;
    TCHAR      *pchPattern = _T("Pattern is placed in buffer and overwritten");
    TCHAR      *pchData    = _T("DATA STRING HAS ALL CAPS INSTEAD");
    TCHAR       achBuff[128];
    VARIANT     var;
    ULONG       i;
    IAccessor  *pAccessor;
    IRowsetChange *pRowsetChange;

    Assert(pRowset && pSTD);

    Verify(!pRowset->QueryInterface(IID_IAccessor, (void **)&pAccessor));
    Verify(!pRowset->QueryInterface(IID_IRowsetChange, (void **)&pRowsetChange));

    dbind.iOrdinal = 1;
    dbind.obValue = 0;
    dbind.obLength = 0;
    dbind.pObject = 0;
    dbind.pBindExt = 0;
    dbind.dwPart = DBPART_VALUE;
    dbind.dwMemOwner = DBMEMOWNER_CLIENTOWNED;
    dbind.eParamIO = DBPARAMIO_NOTPARAM;
    dbind.cbMaxLen = sizeof(TCHAR) * _tcslen(pchData);
    dbind.wType = DBTYPE_WSTR;

    Verify((!pAccessor->CreateAccessor(DBACCESSOR_ROWDATA,
                                 1,
                                 &dbind,
                                 0,
                                 &hAccessor,
                                 NULL )) &&
        "Couldn't create accessor.");

    // Now place the data pattern into the table:
    TestSetDataAtRowNum(pRowsetChange, LargestRow, hAccessor, (BYTE *)pchData);

    Verify((!pAccessor->ReleaseAccessor(hAccessor, NULL)) &&
        "Couldn't release accessor.");

    // Using STD, verify that string was correctly set:
    VariantInit(&var);
    hr = pSTD->getVariant(LargestRow, 1, OSPFORMAT_FORMATTED, &var);
    i = SysStringLen(var.bstrVal);
    Assert(!hr && "Couldn't get string.");
    Assert((i == _tcslen(pchData)) && (0 == (_tcsncmp(var.bstrVal, i, pchData, i))) &&
        "Data string was not placed into table correctly.");
    VariantClear(&var);

    for (i = 0; i < (_tcslen(pchData) + 3); // try filling more than pchData len
        (i == 10) ? (i = _tcslen(pchData) - 1) : (i++)) // don't have to do all
    {
        // Reset buffer to pattern:
        _tcsncpy(achBuff, pchPattern, _tcslen(pchPattern));

        // Retrieve i characters from table:
        dbind.cbMaxLen = sizeof(TCHAR) * i;
        dbind.obValue = sizeof(TCHAR);         // skip 1 char

        Verify((!pAccessor->CreateAccessor(DBACCESSOR_ROWDATA,
                                     1,
                                     &dbind,
                                     0,
                                     &hAccessor,
                                     NULL )) &&
            "Couldn't create accessor.");

        // Now get the data pattern from the table:
        TestGetDataAtRowNum(pRowset, LargestRow, hAccessor, (BYTE *)achBuff);

        Verify((!pAccessor->ReleaseAccessor(hAccessor, NULL)) &&
            "Couldn't release accessor.");

        {
            // Can't copy more than length of pchData:
            ULONG cCopied = __min(i, _tcslen(pchData) + 1); // includes null

            // Now we need to make sure the buffer is what we expect:
            //   The first character is from pchPattern.
            //   cCopied characters from pchData (includes null termination).
            //   len(pchPattern) - cCopied - 1 characters from pchPattern.
            Assert((*achBuff == *pchPattern) &&
                "Error in buffer - first char was altered.");

            if (cCopied > 0)
            {
                Assert((0 == _tcsncmp(achBuff + 1, cCopied - 1, pchData, cCopied - 1)) &&
                    "Error in buffer - data retrieved is wrong.");

                Assert((_T('\0') == achBuff[cCopied]) &&
                    "Error in buffer - null termination missing.");
            }

            Assert((0 == _tcsncmp(achBuff + cCopied + 1,
                                  _tcslen(pchPattern) - cCopied - 1,
                                  pchPattern + cCopied + 1,
                                  _tcslen(pchPattern) - cCopied - 1)) &&
                "Error in buffer - pattern does not match buffer.");
        }
    }

    // We need to clear out the cell in the table we've been fiddling with:
    VariantClear(&var);                 // clear variant (maybe redundant)
    dbind.wType = DBTYPE_VARIANT;
    dbind.cbMaxLen = sizeof(VARIANT);
    dbind.obValue = 0;

    Verify((!pAccessor->CreateAccessor(DBACCESSOR_ROWDATA,
                                 1,
                                 &dbind,
                                 0,
                                 &hAccessor,
                                 NULL )) &&
        "Couldn't create accessor.");

    // Now place the empty variant into the table:
    TestSetDataAtRowNum(pRowsetChange, LargestRow, hAccessor, (BYTE *)&var);

    Verify((!pAccessor->ReleaseAccessor(hAccessor, NULL)) &&
        "Couldn't release accessor.");

    pRowsetChange->Release();
    pAccessor->Release();
}


//+-----------------------------------------------------------------------
//
//  Member:    MyGetExactPosition
//
//  Synopsis:  Wrapper for a version of GetExactPosition that works with
//             exact row indexes.
//
//  Arguments: uRowindex         index of row [Input]
//             puPosition       index of row [Output]
//             pcTotalRows       number of rows [Output]
//
//  Returns:   hr               error code
//
HRESULT
MyGetExactPosition(IRowsetExactScroll *pRowsetExactScroll,
                   DBCOUNTITEM uRowindex,
                   DBCOUNTITEM *puPosition,
                   DBCOUNTITEM *pcTotalRows)
{
    HRESULT hr;
    ULONG ulBookmark;

    ulBookmark = GetBookmarkFromIndex(pRowsetExactScroll, uRowindex);

    hr = pRowsetExactScroll->GetExactPosition(NULL, 4, (BYTE *)&ulBookmark,
                                              puPosition, pcTotalRows);
    Assert(!hr && (*puPosition == uRowindex) && "GetExactPosition failed.");

    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    MyGetApproximatePosition
//
//  Synopsis:  Wrapper for a version of GetApproximatePosition that works with
//             exact row indexes.
//
//  Arguments: uRowindex         index of row [Input]
//             puPosition       index of row [Output]
//             pcTotalRows       number of rows [Output]
//
//  Returns:   hr               error code
//
HRESULT
MyGetApproximatePosition(IRowsetScroll *pRowsetScroll,
                         DBCOUNTITEM uRowindex,
                         DBCOUNTITEM *puPosition,
                         DBCOUNTITEM *pcTotalRows)
{
    HRESULT hr;
    ULONG ulBookmark;

    ulBookmark = GetBookmarkFromIndex(pRowsetScroll, uRowindex);

    hr = pRowsetScroll->GetApproximatePosition(NULL, 4, (BYTE *)&ulBookmark,
                                               puPosition, pcTotalRows);
    Assert(!hr && (*puPosition == uRowindex) && "GetApproximatePosition failed.");

    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    CheckScroll
//
//  Synopsis:  Checks IRowsetScroll and IRowsetExactScroll to make sure they
//             work.
//
//  Arguments: pRowset            an initialized rowset
//             pSTD               simple tabular data object
//
//  Returns:   Nothing.
//

void
CheckScroll (IRowset *pRowset, OLEDBSimpleProvider *pSTD)
{
    HRESULT hr;
    DBROWCOUNT uRows, uCols;
    DBCOUNTITEM uPosition, cTotalRows;
    DBROWCOUNT i;
    IRowsetScroll *pRowsetScroll;
    IRowsetExactScroll *pRowsetExactScroll;
#if defined(PRODUCT_97)
    ULONG ulBookmark;
#endif
    HACCESSOR hAccessorBookmark = CreateBookmarkAccessor(pRowset);

    Assert(pRowset && pSTD);

    Verify(!pRowset->QueryInterface(IID_IRowsetScroll, (void **)&pRowsetScroll));
    Verify(!pRowset->QueryInterface(IID_IRowsetExactScroll, (void **)&pRowsetExactScroll));

    hr = pSTD->getRowCount(&uRows);
    Assert(!hr && "GetRowCount failed.");
    hr = pSTD->getColumnCount(&uCols);
    Assert(!hr && "GetColumnCount failed.");

#if defined(PRODUCT_97)
    // Test GetApproximatePosition:
    for (i = 1; i <= uRows; i++)
    {
        hr = MyGetApproximatePosition(pRowSetScroll, i, &uPosition, &cTotalRows);
        Assert(!hr && (uPosition == i) && (cTotalRows == uRows) &&
            "GetApproximatePosition failed.");
    }

    // Test GetRowsAtRatio:
    // Test #1: Starting with last row, try getting one row:
    ULONG cRowsObtained;
    HROW *pHandles = NULL;

    hr = pRowsetScroll->GetRowsAtRatio(NULL, NULL, 1, 1, 1, &cRowsObtained, &pHandles);
    Assert(!hr && (cRowsObtained == 1) && pHandles && "GetRowsAtRatio failed.");

    hr = pRowset->GetData(*pHandles, hAccessorBookmark, (BYTE *) &ulBookmark);
    Assert(!hr && "couldn't get bookmark.");
    Assert((ulBookmark == uRows) && "Wrong row.");

    pRowset->ReleaseRows(cRowsObtained, pHandles, NULL, NULL);
    CoTaskMemFree(pHandles);        // release handles
    pHandles = NULL;

    // Test #2: Starting with first row, try getting too many rows:
    hr = pRowsetScroll->GetRowsAtRatio(NULL, NULL, 0, 1, 2 * uRows, &cRowsObtained,
                                 &pHandles);
    Assert(SUCCEEDED(hr) && (cRowsObtained == uRows) && pHandles &&
           "GetRowsAtRatio failed.");

    for (i = 0; i < uRows; i++)
    {
        hr = pRowset->GetData(pHandles[i], hAccessorBookmark, (BYTE *) &ulBookmark);
        Assert(!hr && "couldn't get bookmark.");
        Assert((ulBookmark == i+1) && "Wrong row handle.");
    }

    pRowset->ReleaseRows(cRowsObtained, pHandles, NULL, NULL);
    CoTaskMemFree(pHandles);
    pHandles = NULL;

    // Test #3: Start with second row, try getting too many rows backwards:
    hr = pRowsetScroll->GetRowsAtRatio(NULL, NULL, 2, uRows, -(LONG)uRows,
                                       &cRowsObtained, &pHandles );
    Assert(SUCCEEDED(hr) && (cRowsObtained == 2) && pHandles &&
        "GetRowsAtRatio failed.");

    hr = pRowset->GetData(pHandles[0], hAccessorBookmark, (BYTE *) &ulBookmark);
    Assert(!hr && "couldn't get bookmark");
    Assert(ulBookmark == 2 && "Wrong row handle.");
    hr = pRowset->GetData(pHandles[1], hAccessorBookmark, (BYTE *) &ulBookmark);
    Assert(!hr && "couldn't get bookmark");
    Assert(ulBookmark == 1 && "Wrong row handle.");

    pRowset->ReleaseRows(cRowsObtained, pHandles, NULL, NULL);
    CoTaskMemFree(pHandles);
    pHandles = NULL;

    // Test #4: Start with last row, try getting too many rows backwards:
    hr = pRowsetScroll->GetRowsAtRatio(NULL, NULL, 1, 1, -(LONG)(2 * uRows),
                                       &cRowsObtained, &pHandles );
    Assert(SUCCEEDED(hr) && (cRowsObtained == uRows) && pHandles &&
        "GetRowsAtRatio failed.");

    for (i = 0; i < uRows; i++)
    {
        hr = pRowset->GetData(pHandles[i], hAccessorBookmark, (BYTE *) &ulBookmark);
        Assert(!hr && "couldn't get bookmark.");
        Assert((ulBookmark == uRows-i) && "Wrong row handle.");
    }

    pRowset->ReleaseRows(cRowsObtained, pHandles, NULL, NULL);
    CoTaskMemFree(pHandles);
    pHandles = NULL;
#endif  // defined(PRODUCT_97)

    // Test GetExactPosition:
    for (i = 1; i <= uRows; i++)
    {
        hr = MyGetExactPosition(pRowsetExactScroll, i, &uPosition, &cTotalRows);
        Assert(!hr && ((DBROWCOUNT) uPosition == i) && (cTotalRows == (DBCOUNTITEM) uRows) &&
               "GetExactPosition failed." );

    }

    pRowsetExactScroll->Release();
    pRowsetScroll->Release();
    ReleaseAccessor(pRowset, hAccessorBookmark);
}



//+-----------------------------------------------------------------------
//
//  Member:    CheckLocateGetRowsAt
//
//  Synopsis:  Checks IRowsetScroll and IRowsetExactScroll to make sure they
//             work.
//
//  Arguments: pRowset            an initialized rowset
//             pSTD               simple tabular data object
//
//  Returns:   Nothing.
//

void
CheckLocateGetRowsAt (IRowset *pRowset)
{
    HRESULT hr;
    DBCOUNTITEM cRows;
    DBROWCOUNT i;
    DBROWCOUNT j;
    DBCOUNTITEM iRow;
    DBCOUNTITEM cRowsObtained;
    ULONG   ulBookmark;
    IRowsetLocate      *pRowsetLocate;
    IRowsetExactScroll *pRowsetExactScroll;

    Assert(pRowset);

    Verify(!pRowset->QueryInterface(IID_IRowsetLocate,
                                    (void **)&pRowsetLocate ));
    Verify(!pRowset->QueryInterface(IID_IRowsetExactScroll,
                                    (void **)&pRowsetExactScroll ));

    iRow = 1;
    hr = MyGetExactPosition(pRowsetExactScroll, iRow, &cRowsObtained, &cRows);
    Assert(!hr && "GetExactPosition failed.");

    const int ROWCNT = 5;
    HROW  ahRows[ROWCNT] = {NULL};
    HROW *phRows = ahRows;

    for (i = ROWCNT; i; i -= 1) // how many rows from end
    {
        iRow = i;

        ulBookmark = GetBookmarkFromIndex(pRowsetLocate, iRow);
        
        for (j = 0; j <= ROWCNT; j += 1)    // how many rows to skip
        {
            hr = pRowsetLocate->GetRowsAt(NULL, NULL, 4, (const BYTE *)&ulBookmark, -j,
                                          -ROWCNT, &cRowsObtained, &phRows );
            Assert(hr == (i - j >= ROWCNT ? S_OK : DB_S_ENDOFROWSET) &&
                   "Wrong hr from GetRowsAt" );
            Assert((long)cRowsObtained == max((DBROWCOUNT)(i - j), (DBROWCOUNT)0) &&
                   "Wrong cRowsObtained from GetRowsAt" );
            pRowsetLocate->ReleaseRows(cRowsObtained, ahRows, NULL, NULL, NULL);
        }
    }

    for (i = ROWCNT; i; i -= 1) // how many rows from end
    {
        iRow = cRows - i + 1;

        ulBookmark = GetBookmarkFromIndex(pRowsetLocate, iRow);
        
        for (j = 0; j < ROWCNT; j += 1)     // how many rows to skip
        {
            hr = pRowsetLocate->GetRowsAt(NULL, NULL, 4, (const BYTE *)&ulBookmark, j,
                                          ROWCNT, &cRowsObtained, &phRows );
            Assert(hr == (i - j >= ROWCNT ? S_OK : DB_S_ENDOFROWSET) &&
                   "Wrong hr from GetRowsAt" );
            Assert((long)cRowsObtained == max((DBROWCOUNT)(i - j), (DBROWCOUNT)0) &&
                   "Wrong cRowsObtained from GetRowsAt" );
            pRowsetLocate->ReleaseRows(cRowsObtained, ahRows, NULL, NULL, NULL);
        }
    }

    pRowsetExactScroll->Release();
    pRowsetLocate->Release();
}




//+-----------------------------------------------------------------------
//
//  Class:     CMyIRowsetNotify
//
//  Synopsis:  Clients IRowsetNotify object, used for notifiations from Nile
//             to the client.
//

class CMyIRowsetNotify : public IRowsetNotify
{
public:
    CMyIRowsetNotify() :
      _cRef(0), _cChapterEvent(0), _cFieldEvent(0), _cRowEvent(0),
      _cRowsetEvent(0), _cDeletedRows(0), _cInsertedRows(0)
#if defined(PRODUCT_97)
      , _cActiveRows(0)
#endif
        {   }

    //  IUnknown members
    STDMETHODIMP            QueryInterface (REFIID, LPVOID *);

    STDMETHODIMP_(ULONG)    AddRef ()
        { return ++_cRef; }

    STDMETHODIMP_(ULONG)    Release ()
        { --_cRef; return _cRef ? _cRef : (delete this, 0); }

    //  IRowsetNotify members
    virtual HRESULT STDMETHODCALLTYPE OnFieldChange( 
        /* [in] */ IRowset __RPC_FAR *pRowset,
        /* [in] */ HROW hRow,
        /* [in] */ DBORDINAL cColumns,
        /* [size_is][in] */ DBORDINAL __RPC_FAR rgColumns[  ],
        /* [in] */ DBREASON eReason,
        /* [in] */ DBEVENTPHASE ePhase,
        /* [in] */ BOOL fCantDeny);
    
    virtual HRESULT STDMETHODCALLTYPE OnRowChange( 
        /* [in] */ IRowset __RPC_FAR *pRowset,
        /* [in] */ DBCOUNTITEM cRows,
        /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
        /* [in] */ DBREASON eReason,
        /* [in] */ DBEVENTPHASE ePhase,
        /* [in] */ BOOL fCantDeny);
    
    virtual HRESULT STDMETHODCALLTYPE OnRowsetChange( 
        /* [in] */ IRowset __RPC_FAR *pRowset,
        /* [in] */ DBREASON eReason,
        /* [in] */ DBEVENTPHASE ePhase,
        /* [in] */ BOOL fCantDeny);

    // help functions
    ULONG   ChapterEvents ()
        { return _cChapterEvent; }
    ULONG   FieldEvents ()
        { return _cFieldEvent; }
    ULONG   RowEvents ()
        { return _cRowEvent; }
    ULONG   RowsetEvents ()
        { return _cRowsetEvent; }
    ULONG   DeletedRows ()
        { return _cDeletedRows; }
    ULONG   InsertedRows ()
        { return _cInsertedRows; }
#if defined(PRODUCT_97)
    ULONG   ActiveRows ()
        { return _cActiveRows; }
#endif

private:
    ULONG       _cRef;
    ULONG       _cChapterEvent;
    ULONG       _cFieldEvent;
    ULONG       _cRowEvent;
    ULONG       _cRowsetEvent;
    ULONG       _cDeletedRows;
    ULONG       _cInsertedRows;
#if defined(PRODUCT_97)
    ULONG       _cActiveRows;
#endif
};


//+---------------------------------------------------------------------------
//  Member:     QueryInterface (public member)
//
//  Synopsis:   Normal IUnknow QI
//
//  Arguments:  riid            IID of requested interface
//              ppv             Interface object to return
//
//  Returns:    S_OK            Interface supported
//              E_NOINTERFACE   Interface not supported.
//

STDMETHODIMP
CMyIRowsetNotify::QueryInterface (REFIID riid, LPVOID *ppv)
{
    Assert(ppv);

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IRowsetNotify))
    {
        *ppv = this;
    }
    else
    {
        *ppv = NULL;
    }

    //  If we're going to return an interface, AddRef it first
    if (*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}


//+---------------------------------------------------------------------------
//  Member:     OnFieldChange (public member)
//
//  Synopsis:   Called by Nile when a field changes
//

STDMETHODIMP
CMyIRowsetNotify::OnFieldChange (IRowset *pRowset, HROW hRow,
                                 DBORDINAL cColumns, DBORDINAL aColumns[],
                                 DBREASON eReason, DBEVENTPHASE ePhase,
                                 BOOL fCantDeny)
{
    Assert(pRowset);

    if (ePhase == DBEVENTPHASE_DIDEVENT) // we only want 1 count for all 4 phases
    {
        _cFieldEvent++;
    }

    return S_OK;
}


//+---------------------------------------------------------------------------
//  Member:     OnRowChange (public member)
//
//  Synopsis:   Called by Nile when a HROW changes
//

STDMETHODIMP
CMyIRowsetNotify::OnRowChange (IRowset *pRowset, DBCOUNTITEM cRows,
                               const HROW rghRows[],
                               DBREASON eReason, DBEVENTPHASE ePhase,
                               BOOL fCantDeny)
{
    Assert(pRowset);

    if (ePhase == DBEVENTPHASE_DIDEVENT) // we only want 1 count for all 4 phases
    {
        _cRowEvent++;
        switch(eReason)
        {
#if defined(PRODUCT_97)
        case DBREASON_ROW_ACTIVATE:
            _cActiveRows += cRows;
            break;
        case DBREASON_ROW_RELEASE:
            _cActiveRows -= cRows;
            break;
#endif
        case DBREASON_ROW_INSERT:
            _cInsertedRows += (ULONG)cRows;
            break;
        case DBREASON_ROW_DELETE:
            _cDeletedRows += (ULONG)cRows;
            break;
        }
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//  Member:     OnRowsetChange (public member)
//
//  Synopsis:   Called by Nile when the rowset has changed
//

STDMETHODIMP
CMyIRowsetNotify::OnRowsetChange (IRowset *pRowset, DBREASON eReason,
					DBEVENTPHASE ePhase, BOOL fCantDeny)
{
    Assert(pRowset);

    if (ePhase == DBEVENTPHASE_DIDEVENT) // we only want 1 count for all 4 phases
    {
        _cRowsetEvent++;
    }

    return S_OK;
}



//+-----------------------------------------------------------------------
//
//  Member:    SimpleWriteTest
//
//  Synopsis:  Do a simple test to check for notifications.
//
//  Arguments: None.
//
//  Returns:   Nothing.
//

void
SimpleWriteTest (IRowset *pRowset)
{
    DBBINDING   dbind;
    HACCESSOR   hAccessor;
    WIDE_STR    buffer;
    IAccessor  *pAccessor;
    IRowsetChange *pRowsetChange;

    Verify(!pRowset->QueryInterface(IID_IAccessor, (void **)&pAccessor));
    Verify(!pRowset->QueryInterface(IID_IRowsetChange, (void **)&pRowsetChange));

    dbind.iOrdinal = 2;
    dbind.obValue = sizeof(ULONG);
    dbind.obLength = 0;
    dbind.pObject = 0;
    dbind.pBindExt = 0;
    dbind.dwPart = DBPART_VALUE | DBPART_LENGTH;
    dbind.dwMemOwner = DBMEMOWNER_CLIENTOWNED;
    dbind.eParamIO = DBPARAMIO_NOTPARAM;
    dbind.cbMaxLen = MAX_WSTR_LEN * sizeof(TCHAR);
    dbind.wType = DBTYPE_WSTR;

    Verify((!pAccessor->CreateAccessor(DBACCESSOR_ROWDATA,
                                    1,
                                    &dbind,
                                    0,
                                    &hAccessor,
                                    NULL )) &&
        "Couldn't create accessor.");

    buffer.uCount = _tcslen(_T("Sample Data"));
    _tcscpy(buffer.chBuff, _T("Sample Data"));

    TestGetDataAtRowNum(pRowset, 1, hAccessor, (BYTE *) &buffer);
    TestSetDataAtRowNum(pRowsetChange, 1, hAccessor, (BYTE *) &buffer);
    Verify((!pAccessor->ReleaseAccessor(hAccessor, NULL)) &&
        "Couldn't release accessor.");

    pRowsetChange->Release();
    pAccessor->Release();
}


//+-----------------------------------------------------------------------
//
//  Member:    TestNotifications
//
//  Synopsis:  Checks IRowsetNotify working.
//
//  Arguments: None.
//
//  Returns:   Nothing.
//

void
TestNotifications (IRowset *pRowset, OLEDBSimpleProvider *pSTD)
{
    IConnectionPointContainer   *pCPC;
    IConnectionPoint            *pCP;
    CMyIRowsetNotify            *pRowNotify = new(Mt(Mem)) CMyIRowsetNotify();
    DWORD                       wAdviseCookie;
    IID                         iid;
    IConnectionPointContainer   *pConnectPointCPC;

    Verify(S_OK == pRowset->QueryInterface(IID_IConnectionPointContainer,
                                           (void **)&pCPC ));
    Verify(S_OK == pCPC->FindConnectionPoint(IID_IRowsetNotify, &pCP));

    // Connect the notification
    Verify(S_OK == pCP->Advise((IUnknown *)pRowNotify, &wAdviseCookie));

    Verify(S_OK == pCP->GetConnectionInterface(&iid));
    Verify(IsEqualIID(iid, IID_IRowsetNotify));

    Verify(S_OK == pCP->GetConnectionPointContainer(&pConnectPointCPC));
    Assert(pConnectPointCPC == pCPC);

    // Test some simple changes notifications should be fired.
    SimpleWriteTest(pRowset);

    // Add and delete some rows directly in the STD:
    DBROWCOUNT cNumRowsChanged;
    Verify(!pSTD->insertRows(1, 5, &cNumRowsChanged));
    Verify(!pSTD->deleteRows(1, 5, &cNumRowsChanged));

    // Did we get the correct notifications.
    Assert((pRowNotify->ChapterEvents()  == 0) &&
           (pRowNotify->FieldEvents()    == 1) &&
           (pRowNotify->InsertedRows()   == 5) &&
           (pRowNotify->DeletedRows()    == 5) &&
           (pRowNotify->RowsetEvents()   == 0) );

#if defined(PRODUCT_97)
    Assert(pRowNotify->ActiveRows()     == 0);
#endif

    // Let's not get notified anymore.
    Verify(S_OK == pCP->Unadvise(wAdviseCookie));

    // Test some simple NILE stuff which would cause notifications to be fired
    // we shouldn't get called, we've disconnected.

    // Test some simple changes notifications should not be fired if they are
    // we will crash, the object is de-allocated.
    SimpleWriteTest(pRowset);

    pConnectPointCPC->Release();
    pCP->Release();
    pCPC->Release();
}

//+-----------------------------------------------------------------------
//
//  Member:    TestGetNextRows
//
//  Synopsis:  Tests IRowset::GetNextRows
//
//  Arguments: None.
//
//  Returns:   Nothing.
//
void
TestGetNextRows(IRowset *pRowset, OLEDBSimpleProvider *pSTD)
{
    HRESULT     hr;
    HROW        ahRows[2];              // array of hRows
    HROW        *pahRows= &ahRows[0];
    DBCOUNTITEM cRowsObtained;
    LONG        cRowsToSkip;
    LONG        cRows;                  // # of rows to get
    ULONG       i;
    DBBINDING   dbind;
    HACCESSOR   hAccessor;
    VARIANT     var;
    IAccessor *pAccessor;
    
    dbind.eParamIO = DBPARAMIO_NOTPARAM;
    dbind.dwPart = DBPART_VALUE;
    dbind.iOrdinal = 1;
    dbind.wType = DBTYPE_VARIANT;
    dbind.bPrecision = 0;
    dbind.bScale = 0;
    dbind.obValue = 0;
    dbind.cbMaxLen = sizeof(VARIANT);
    dbind.obLength = 0;
    dbind.pObject = 0;
    dbind.pBindExt = 0;
    dbind.dwMemOwner = DBMEMOWNER_CLIENTOWNED;

    Verify(!pRowset->QueryInterface(IID_IAccessor, (void **)&pAccessor));
    Verify((!pAccessor->CreateAccessor(DBACCESSOR_ROWDATA,
                                       1,
                                       &dbind,
                                       0,
                                       &hAccessor,
                                       NULL )) &&
           "Couldn't create accessor.");

    // Get rows forward
    for (i=1; i<=Col1MaxRows; i+=5)
    {
        cRows = 2;                      // get two rows at a time
        cRowsToSkip = 3;                // skip 3
        hr = pRowset->GetNextRows(NULL, cRowsToSkip, cRows,
                                 &cRowsObtained, &pahRows);

        Assert(!hr && cRowsObtained==(ULONG)cRows && "GetNextRows failed");
        
        hr = pRowset->GetData(ahRows[1], hAccessor, &var);
        Assert(!hr && var.lVal == Col1StartValue+((LONG)i+3)*Col1Incr &&
               "Data from GetNextRows was bad.");
        pRowset->ReleaseRows(cRowsObtained, pahRows, NULL, NULL, NULL);
    }
           
    pRowset->RestartPosition (NULL);     // reset cursor
    
    // Get rows backwards
    for (i=LargestRow; i>0; i-=5)
    {
        cRows = -2;                     // get two rows at a time
        cRowsToSkip = -3;               // skip 3
        hr = pRowset->GetNextRows(NULL, cRowsToSkip, cRows,
                                 &cRowsObtained, &pahRows);

        Assert(!hr && cRowsObtained==(ULONG)-cRows && "GetNextRows failed");
        
        if (i <= Col1MaxRows)           // Ignore the Col2 stuff
        {
            hr = pRowset->GetData(ahRows[1], hAccessor, &var);
            Assert(!hr && var.lVal == Col1StartValue+((LONG)(i-5))*Col1Incr &&
                   "Data from GetNextRows was bad.");
            pRowset->ReleaseRows(cRowsObtained, pahRows, NULL, NULL, NULL);        
        }
    }

    pRowset->RestartPosition (NULL);     // reset cursor
    // Try to get two past the end
    hr = pRowset->GetNextRows(NULL, LargestRow-1, 2, &cRowsObtained, &pahRows);
    Assert(hr==DB_S_ENDOFROWSET && cRowsObtained==1 &&
           "GetNextRows at end did not do the right thing.");
    pRowset->ReleaseRows(cRowsObtained, pahRows, NULL, NULL, NULL);

    pAccessor->ReleaseAccessor(hAccessor, NULL);
    pAccessor->Release();
}


//+-----------------------------------------------------------------------
//
//  Member:     TestAccessors
//
//  Synopsis:   Tests reference counting of accessors
//
//  Arguments:  pSTD    STD on which to create rowset
//              dbind   binding to use for accessor
//
//  Returns:    Nothing.

void
TestAccessors(DBBINDING& dbind)
{
    OLEDBSimpleProvider *pSTD;
    IUnknown *pUnk;
    IRowset *pRowset;
    IAccessor *pAccessor;
    const cNumAccessors = 5;
    HACCESSOR hAccessor[cNumAccessors];
    ULONG ulRefCount;
    int j, k;
    
    Verify(!FormsSTDCreate((LPOLEDBSimpleProvider*)&pSTD, MaxColumns));
    Assert(pSTD && "Didn't create STD.");

    CreateAndFillSTD(pSTD);
    VerifySTDContents(pSTD);

    Verify(!CTopRowset::CreateRowset(pSTD, &pUnk));
    Assert(pUnk && "Couldn't create rowset.");

    Verify(!pUnk->QueryInterface(IID_IRowset, (void **)&pRowset));
    Verify(!pRowset->QueryInterface(IID_IAccessor, (void **)&pAccessor));

    // create some accessors and addref them
    for (k=0; k<cNumAccessors; ++k)
    {
        Verify((!pAccessor->CreateAccessor(DBACCESSOR_ROWDATA,
                                           1, &dbind, 0, &hAccessor[k], NULL ))
               && "Couldn't create accessor" );
        for (j=0; j<k; ++j)
        {
            Verify((!pAccessor->AddRefAccessor(hAccessor[k], &ulRefCount))
                && "Couldn't addref accessor");
            Assert(ulRefCount-j == 2 && "wrong ref count");
        }
    }

    // now release the accessors
    for (k=0; k<cNumAccessors; ++k)
    {
        for (j=k; j>=0; --j)
        {
            Verify((!pAccessor->ReleaseAccessor(hAccessor[k], &ulRefCount))
                && "Couldn't release accessor");
            Assert(ulRefCount-j == 0 && "wrong ref count");
        }
    }

    // create some accessors and addref them
    for (k=0; k<cNumAccessors; ++k)
    {
        Verify((!pAccessor->CreateAccessor(DBACCESSOR_ROWDATA,
                                           1, &dbind, 0, &hAccessor[k], NULL ))
               && "Couldn't create accessor" );
        for (j=0; j<k; ++j)
        {
            Verify((!pAccessor->AddRefAccessor(hAccessor[k], &ulRefCount))
                && "Couldn't addref accessor");
            Assert(ulRefCount-j == 2 && "wrong ref count");
        }
    }

    // release the rowset;  it should release the live accessors
    ReleaseInterface(pAccessor);
    ReleaseInterface(pRowset);
    ReleaseInterface(pUnk);
    ReleaseInterface(pSTD);
}


//+-----------------------------------------------------------------------
//
//  Member:    TestNileSTD
//
//  Synopsis:  Checks rowset.  This function calls other functions which
//             initialize and verify the contents of an STD.  This STD
//             is then embedded inside a rowset, and tests are performed
//             to see if the rowset can get and set its cells.
//
//  Arguments: None.
//
//  Returns:   Nothing.
//

extern "C"
HRESULT
TestNileSTD()
{
    HRESULT             hr;
    IUnknown            *pUnk;
    IRowset             *pRowset;
    OLEDBSimpleProvider  *pSTD;

    CEnsureThreadState ets;
    hr = ets._hr;
    if (FAILED(hr))
        RRETURN(hr);

    TestSTD();                             // fisrt, test the STD alone

    Verify(!FormsSTDCreate((LPOLEDBSimpleProvider*)&pSTD, MaxColumns));
    Assert(pSTD && "Didn't create STD.");

    CreateAndFillSTD(pSTD);
    VerifySTDContents(pSTD);

    Verify(!CTopRowset::CreateRowset(pSTD, &pUnk));
    Assert(pUnk && "Couldn't create rowset.");

    Verify(!pUnk->QueryInterface(IID_IRowset, (void **)&pRowset));

    CheckColumnsInfo(pRowset);
    CheckMapColumnIDs(pRowset);

    ////////////////////////////////////////////////////////////////
    // Test STD event handlers:
    TestSTDEvents(pRowset, pSTD);

    ////////////////////////////////////////////////////////////////
    // Test add and delete rows
    TestAddRows(pRowset, pSTD);
    TestDeleteRows(pRowset, pSTD);

    ////////////////////////////////////////////////////////////////
    // Test boundary validities on GetData
    TestGetData(pRowset, pSTD);
    VerifySTDContents(pSTD);

    ////////////////////////////////////////////////////////////////
    // Test IRowsetScroll and IRowsetExactScroll
    CheckScroll(pRowset, pSTD);

    ////////////////////////////////////////////////////////////////
    // Test IRowsetLocate::GetRowsAt
    CheckLocateGetRowsAt(pRowset);

    ////////////////////////////////////////////////////////////////
    // Test IRowset::GetNextRows
    TestGetNextRows(pRowset, pSTD);

    ////////////////////////////////////////////////////////////////
    // Data binding.
    DBBINDING       dbind;
    HACCESSOR       hAccessor;
    ULONG           r;
    int             i;
    TCHAR           cBaseBuff[50];
    WIDE_STR        wideBuffer;
    BSTR            cBSTRBuffer;
    VARIANTARG      cVarBuffer;
    DBVECTOR        dbVector;
    IAccessor       *pAccessor;
    IRowsetChange   *pRowsetChange;

    Verify(!pRowset->QueryInterface(IID_IAccessor, (void **)&pAccessor));
    Verify(!pRowset->QueryInterface(IID_IRowsetChange, (void **)&pRowsetChange));

    ////////////////////////////////////////////////////////////////
    // Access column #1 data as VARIANT get/set
    dbind.eParamIO = DBPARAMIO_NOTPARAM;
    dbind.dwPart = DBPART_VALUE;
    dbind.iOrdinal = 1;
    dbind.wType = DBTYPE_VARIANT;
    dbind.obValue = 0;
    dbind.cbMaxLen = sizeof(VARIANT);
    dbind.obLength = 0;
    dbind.pObject = 0;
    dbind.pBindExt = 0;
    dbind.dwMemOwner = DBMEMOWNER_CLIENTOWNED;

    Verify((!pAccessor->CreateAccessor(DBACCESSOR_ROWDATA,
                                 1,
                                 &dbind,
                                 0,
                                 &hAccessor,
                                 NULL )) &&
        "Couldn't create accessor.");

    i = Col1StartValue;

    for (r = 1; r <= Col1MaxRows; r++)
    {
        TestGetDataAtRowNum(pRowset, r,hAccessor, (BYTE *)&cVarBuffer);
        Assert((cVarBuffer.lVal == i) && "Incorrect variant contents.");

        int oldValue = cVarBuffer.lVal;

        cVarBuffer.lVal = 2000;
        TestSetDataAtRowNum(pRowsetChange, r, hAccessor, (BYTE *)&cVarBuffer);


        TestGetDataAtRowNum(pRowset, r, hAccessor, (BYTE *)&cVarBuffer );
        Assert((cVarBuffer.lVal == 2000) && "Incorrect variant contents.");

        cVarBuffer.lVal = oldValue;
        TestSetDataAtRowNum(pRowsetChange, r, hAccessor,(BYTE *)&cVarBuffer );

        i += Col1Incr;
    }

    Verify((!pAccessor->ReleaseAccessor(hAccessor, NULL)) &&
        "Couldn't release accessor.");

    VerifySTDContents(pSTD);         // contents should not have changed

    // The following tests depended on conversions that we don't support
    // anymore.


    ////////////////////////////////////////////////////////////////
    // Access column #1 data as WSTRs w/ length get only
    dbind.dwPart = DBPART_VALUE | DBPART_LENGTH;
    dbind.iOrdinal = 1;
    dbind.wType = DBTYPE_WSTR;
    dbind.obValue = sizeof(ULONG);
    dbind.cbMaxLen = MAX_WSTR_LEN * sizeof(TCHAR);
    dbind.obLength = 0;
    dbind.pObject = 0;
    dbind.pBindExt = 0;
    dbind.dwMemOwner = DBMEMOWNER_CLIENTOWNED;

    Verify((!pAccessor->CreateAccessor(DBACCESSOR_ROWDATA,
                                    1,
                                    &dbind,
                                    0,
                                    &hAccessor,
                                    NULL )) &&
        "Couldn't create accessor.");

    i = Col1StartValue;

    for (r = 1; r <= Col1MaxRows; r++)
    {
        wsprintf(cBaseBuff, _T("%i"), i);

        TestGetDataAtRowNum(pRowset, r, hAccessor, (BYTE *)&wideBuffer );

        Assert(!StrCmpC(wideBuffer.chBuff, cBaseBuff) && "Wrong cell value.");

        i += Col1Incr;
    }

    Verify((!pAccessor->ReleaseAccessor(hAccessor, NULL)) &&
        "Couldn't release accessor.");

    VerifySTDContents(pSTD);         // contents should not have changed

    ////////////////////////////////////////////////////////////////
    // Access column #1 data as BSTRs get/set
    dbind.dwPart = DBPART_VALUE;
    dbind.iOrdinal = 1;
    dbind.wType = DBTYPE_BSTR;
    dbind.obValue = 0;
    dbind.cbMaxLen = sizeof(BSTR);
    dbind.obLength = 0;
    dbind.pObject = 0;
    dbind.pBindExt = 0;
    dbind.dwMemOwner = DBMEMOWNER_CLIENTOWNED;

    Verify((!pAccessor->CreateAccessor(DBACCESSOR_ROWDATA,
                                 1,
                                 &dbind,
                                 0,
                                 &hAccessor,
                                 NULL )) &&
        "Couldn't create accessor.");

    i = Col1StartValue;

    for (r = 1; r <= Col1MaxRows; r++)
    {
        wsprintf(cBaseBuff, _T("%i"), i);

        TestGetDataAtRowNum(pRowset, r, hAccessor, (BYTE *)&cBSTRBuffer);

        Assert(!StrCmpC(cBSTRBuffer, cBaseBuff) && "Wrong cell value.");

        OLECHAR cOld = *cBSTRBuffer;

        *cBSTRBuffer = OLESTR('2');
        *cBaseBuff = _T('2');

        TestSetDataAtRowNum(pRowsetChange, r, hAccessor, (BYTE *)&cBSTRBuffer);
        SysFreeString(cBSTRBuffer);

        TestGetDataAtRowNum(pRowset, r, hAccessor, (BYTE *)&cBSTRBuffer);

        Assert(!StrCmpC(cBSTRBuffer, cBaseBuff) && "Wrong cell value.");

        *cBSTRBuffer = cOld;
        TestSetDataAtRowNum(pRowsetChange, r, hAccessor, (BYTE *)&cBSTRBuffer);

        SysFreeString(cBSTRBuffer);

        i += Col1Incr;
    }

    Verify((!pAccessor->ReleaseAccessor(hAccessor, NULL)) &&
        "Couldn't release accessor.");

    ////////////////////////////////////////////////////////////////
    // Access column #1 data as WSTRs w/ length get/set
    dbind.dwPart = DBPART_VALUE | DBPART_LENGTH;
    dbind.iOrdinal = 1;
    dbind.wType = DBTYPE_WSTR;
    dbind.obValue = sizeof(ULONG);
    dbind.cbMaxLen = MAX_WSTR_LEN * sizeof(TCHAR);
    dbind.obLength = 0;
    dbind.pObject = 0;
    dbind.pBindExt = 0;
    dbind.dwMemOwner = DBMEMOWNER_CLIENTOWNED;

    Verify((!pAccessor->CreateAccessor(DBACCESSOR_ROWDATA,
                                    1,
                                    &dbind,
                                    0,
                                    &hAccessor,
                                    NULL )) &&
        "Couldn't create accessor.");

    i = Col1StartValue;

    for (r = 1; r <= Col1MaxRows; r++)
    {
        wsprintf(cBaseBuff, _T("%i"), i);

        TestGetDataAtRowNum(pRowset, r, hAccessor, (BYTE *)&wideBuffer);
        Assert(!StrCmpC(wideBuffer.chBuff, cBaseBuff) && "Wrong cell value.");

        TCHAR cOld = wideBuffer.chBuff[0];

        wideBuffer.chBuff[0] = _T('2');
        *cBaseBuff = _T('2');

        TestSetDataAtRowNum(pRowsetChange, r, hAccessor, (BYTE *)&wideBuffer);

        TestGetDataAtRowNum(pRowset, r, hAccessor, (BYTE *)&wideBuffer);
        Assert(!StrCmpC(wideBuffer.chBuff, cBaseBuff) && "Wrong cell value.");

        wideBuffer.chBuff[0] = cOld;
        TestSetDataAtRowNum(pRowsetChange, r, hAccessor, (BYTE *)&wideBuffer);

        i += Col1Incr;
    }

    Verify((!pAccessor->ReleaseAccessor(hAccessor, NULL)) &&
        "Couldn't release accessor.");

    ////////////////////////////////////////////////////////////////
    // Access column #2 data as WSTR w/ length
    dbind.dwPart = DBPART_VALUE | DBPART_LENGTH;
    dbind.iOrdinal = 2;
    dbind.wType = DBTYPE_WSTR;
    dbind.obValue = sizeof(ULONG);
    dbind.cbMaxLen = MAX_WSTR_LEN * sizeof(TCHAR);
    dbind.obLength = 0;
    dbind.pObject = 0;
    dbind.pBindExt = 0;
    dbind.dwMemOwner = DBMEMOWNER_CLIENTOWNED;

    Verify((!pAccessor->CreateAccessor(DBACCESSOR_ROWDATA,
                                    1,
                                    &dbind,
                                    0,
                                    &hAccessor,
                                    NULL )) &&
        "Couldn't create accessor.");

    for (r = 1; r <= Col2MaxRows; r++)
    {
        wsprintf(cBaseBuff, Col2StartValue, r);

        TestGetDataAtRowNum(pRowset, r, hAccessor, (BYTE *)&wideBuffer);

        Assert(!StrCmpC(wideBuffer.chBuff, cBaseBuff) && "Wrong cell value.");
    }

    Verify((!pAccessor->ReleaseAccessor(hAccessor, NULL)) &&
        "Couldn't release accessor.");

    ////////////////////////////////////////////////////////////////
    // Access column #2 data as BSTR
    dbind.dwPart = DBPART_VALUE;
    dbind.iOrdinal = 2;
    dbind.wType = DBTYPE_BSTR;
    dbind.obValue = 0;
    dbind.cbMaxLen = sizeof(BSTR);
    dbind.obLength = 0;
    dbind.pObject = 0;
    dbind.pBindExt = 0;
    dbind.dwMemOwner = DBMEMOWNER_CLIENTOWNED;

    Verify((!pAccessor->CreateAccessor(DBACCESSOR_ROWDATA,
                                    1,
                                    &dbind,
                                    0,
                                    &hAccessor,
                                    NULL )) &&
        "Couldn't create accessor.");

    for (r = 1; r <= Col2MaxRows; r++)
    {
        wsprintf(cBaseBuff, Col2StartValue, r);

        TestGetDataAtRowNum(pRowset, r, hAccessor, (BYTE *)&cBSTRBuffer);
        Assert(!StrCmpC(cBSTRBuffer, cBaseBuff) && "Wrong cell value.");

        SysFreeString(cBSTRBuffer);
    }

    Verify((!pAccessor->ReleaseAccessor(hAccessor, NULL)) &&
        "Couldn't release accessor.");

    ////////////////////////////////////////////////////////////////
    // Access column #2 data as VARIANT
    dbind.dwPart = DBPART_VALUE;
    dbind.iOrdinal = 2;
    dbind.wType = DBTYPE_VARIANT;
    dbind.obValue = 0;
    dbind.cbMaxLen = sizeof(VARIANTARG);
    dbind.obLength = 0;
    dbind.pObject = 0;
    dbind.pBindExt = 0;
    dbind.dwMemOwner = DBMEMOWNER_CLIENTOWNED;

    Verify((!pAccessor->CreateAccessor(DBACCESSOR_ROWDATA,
                                    1,
                                    &dbind,
                                    0,
                                    &hAccessor,
                                    NULL )) &&
        "Couldn't create accessor.");

    for (r = 1; r <= Col2MaxRows; r++)
    {
        wsprintf(cBaseBuff, Col2StartValue, r);

        TestGetDataAtRowNum(pRowset, r, hAccessor, (BYTE *)&cVarBuffer);

        Assert((cVarBuffer.vt == VT_BSTR) &&
               (!StrCmpC(cVarBuffer.bstrVal, cBaseBuff)) &&
               "Wrong cell value.");

        VariantClear(&cVarBuffer);
    }

    Verify((!pAccessor->ReleaseAccessor(hAccessor, NULL)) &&
        "Couldn't release accessor.");

    ////////////////////////////////////////////////////////////////
    // Create a bookmark

    dbind.dwPart = DBPART_VALUE;
    dbind.iOrdinal = 0;

    dbind.wType = DBTYPE_VECTOR | DBTYPE_UI1;
    dbind.obValue = 0;
    dbind.cbMaxLen = sizeof(DBVECTOR);
    dbind.obLength = 0;
    dbind.pObject = 0;
    dbind.pBindExt = 0;
    dbind.dwMemOwner = DBMEMOWNER_CLIENTOWNED;

    Verify((!pAccessor->CreateAccessor(DBACCESSOR_ROWDATA,
                                       1, &dbind, 0, &hAccessor, NULL ))
           && "Couldn't create accessor" );

    TestGetDataAtRowNum(pRowset, 5, hAccessor, (BYTE *)&dbVector);
    Assert((dbVector.size == sizeof(ULONG)) && "Bookmark is wrong size.");

    if (dbVector.ptr)
        CoTaskMemFree(dbVector.ptr);

    Verify((!pAccessor->ReleaseAccessor(hAccessor, NULL)) &&
        "Couldn't release accessor.");

    dbind.dwPart = DBPART_VALUE;
    dbind.iOrdinal = 0;
    dbind.wType = DBTYPE_I4;
    dbind.obValue = 0;
    dbind.cbMaxLen = sizeof(LONG);
    dbind.obLength = 0;
    dbind.pObject = 0;
    dbind.pBindExt = 0;

    Verify((!pAccessor->CreateAccessor(DBACCESSOR_ROWDATA,
                                       1, &dbind, 0, &hAccessor, NULL ))
           && "Couldn't create accessor" );

    TestGetDataAtRowNum(pRowset, 5, hAccessor, (BYTE *)&i);

    Verify((!pAccessor->ReleaseAccessor(hAccessor, NULL)) &&
        "Couldn't release accessor.");

    // Test notifications
    TestNotifications(pRowset, pSTD);

    // Test ref counting of accessors
    TestAccessors(dbind);

    ReleaseInterface(pRowsetChange);
    ReleaseInterface(pAccessor);
    ReleaseInterface(pRowset);
    ReleaseInterface(pUnk);
    ReleaseInterface(pSTD);

    return S_OK;
}
