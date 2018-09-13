//+-----------------------------------------------------------------------
//
//  Simple Tabular Data
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:      arrayi.cxx
//  Author:    Ido Ben-Shachar (t-idoben@microsoft.com)
//
//  Contents:  Simple tabular data interface.
//             Allows client to create a table of VARIANTS which
//             can be manipulated.
//
//  5 Nov 1996 cfranks
//             Converted to new, Visual Basic & Java friendly STD spec.
//             Note that this involves changing many ULONG variables to LONG
//             variables.  Beware, this code may no longer work correctly on
//             tables with 2^31 or more rows or columns.
//
//------------------------------------------------------------------------


#include <dlaypch.hxx>

#ifndef X_SIMPDATA_H_
#define X_SIMPDATA_H_
#include <simpdata.h>
#endif

#ifndef X_MSTDWRAP_H_
#define X_MSTDWRAP_H_
#include "mstdwrap.h"
#endif

#ifndef X_CCELL_HXX_
#define X_CCELL_HXX_
#include "ccell.hxx"
#endif

#ifndef X_TARRAY_HXX_
#define X_TARRAY_HXX_
#include "tarray.hxx"
#endif

#ifndef X_ARRAYI_HXX_
#define X_ARRAYI_HXX_
#include "arrayi.hxx"
#endif

#define ERROR_MSG _T("#ERROR")              // string error message

DeclareTag(tagSimpleTabularData, "SimpleTabularData",
    "Stand-alone implementation of STD");
MtDefine(STDArray, DataBind, "STDArray")
MtDefine(CSimpleTabularData, DataBind, "CSimpleTabularData")
MtDefine(CSimpleTabularData_ArraypArrayCells, DataBind, "CSimpleTabularData::_ArraypArrayCells[]")
MtDefine(FormsSTDCreatePopulate_p, DataBind, "FormsSTDCreatePopulate p")

//+-----------------------------------------------------------------------
//
//  Constructor for CCell
//
//  Synopsis:  Initializes variant in cell and set cell status to READ/WRITE.
//
//  Arguments: None.
//
//  Returns:   Nothing.
//
//------------------------------------------------------------------------

CCell::CCell ()
{
    TraceTag((tagSimpleTabularData,
        "CCell::default constructor -> %p", this));

    // Initialize variant:
    _vCellVariant.vt = VT_NULL;
    _vCellVariant.pbstrVal = 0;
    // Variant indicates that actual cell is empty.  Current storage type
    //   is thus variant:
    SetIsVariant();
}


//+-----------------------------------------------------------------------
//
//  Destructor for CCell
//
//  Synopsis:  Clears the variant before it gets destroyed.
//
//  Arguments: None.
//
//  Returns:   Nothing.
//
//------------------------------------------------------------------------

inline CCell:: ~CCell()
{
    TraceTag((tagSimpleTabularData,
        "CCell::default destructor -> %p", this));

    // Clear variant:
    VariantClear(&_vCellVariant);
}


//+-----------------------------------------------------------------------
//
//  Member:    IsValidObject
//
//  Synopsis:  Checks to see if STD object is in order.  Makes Assertions
//             about all conditions that must be true.  Only called
//             during debugging.
//
//  Arguments: None.
//
//  Returns:   Nothing.
//
//------------------------------------------------------------------------

#if DBG == 1
void
CSimpleTabularData::IsValidObject ()
{
    // Check that object exists:
    Assert(this);

    // Check that Create() was called after constructor:
    Assert("Create() hasn't been called" && _bInitialized);

    // Number of rows in object should be accurate:
    Assert("Wrong number of rows" &&
           (_cRows + 1 == _ArraypArrayCells.GetSize()));

    // Number of columns in each row should be accurate:
    ULONG i;

    if ((_cDebugLastRowSizeChecked != _cRows) ||
        (_cDebugLastColumnSizeChecked != _cColumns) )
    {
        for (i = 0; i <= _cRows; i++)              // check all rows
        {
            Assert("Wrong number of columns" &&
                   (_cColumns + 1 == _ArraypArrayCells[i]->GetSize()));
        }

        _cDebugLastRowSizeChecked = _cRows;
        _cDebugLastColumnSizeChecked = _cColumns;
    }
}
#endif


//+-----------------------------------------------------------------------
//
//  Constructor for CSimpleTabularData
//
//  Synopsis:  Clears internal data.  Due to the COM model, the
//             member function "Create" should be called to actually
//             initialize the STD data structures.
//
//  Arguments: None.
//
//  Returns:   Nothing.
//
//------------------------------------------------------------------------

CSimpleTabularData::CSimpleTabularData () :
    _cRef(1)
{
    TraceTag((tagSimpleTabularData,
        "CSimpleTabularData::default constructor -> %p", this));

#if DBG == 1
    _bInitialized = FALSE;              // STD isn't initialized yet
    _cDebugLastRowSizeChecked = 0;      // Last row checked in IsValidObject
    _cDebugLastColumnSizeChecked = 0;   // Last column checked in IsValidObject
#endif

}





//+-----------------------------------------------------------------------
//
//  Member:    KillRows
//
//  Synopsis:  Deallocates a range of rows
//
//  Arguments: iRow     first row
//             cRows    number of rows
//
//  Returns:   Nothing.
//
//------------------------------------------------------------------------

void
inline
CSimpleTabularData::KillRows (ULONG iRow, ULONG cRows)
{
    TSTDArray<CCell> *pArrayCellsTempRow;
    ULONG i;

    for (i=iRow; i < (iRow+cRows); i++)
    {
        pArrayCellsTempRow = _ArraypArrayCells[i];

        pArrayCellsTempRow->Passivate();
        delete pArrayCellsTempRow;
    }
}


//+-----------------------------------------------------------------------
//
//  Destructor for CSimpleTabularData
//
//  Synopsis:  De-initialize the STD data structures.
//
//  Arguments: None.
//
//  Returns:   Nothing.
//
//------------------------------------------------------------------------


CSimpleTabularData::~CSimpleTabularData ()
{
    TraceTag((tagSimpleTabularData,
        "CSimpleTabularData::default destructor -> %p", this));

    // Release and NULL the event interface so calls are no longer made to it:
    if (_pEvents)
    {
        _pEvents->Release();
        _pEvents = NULL;
    }

    // First we must delete all rows

    KillRows(0, _cRows + 1);

    // Now we can delete the actual array of rows:

    _ArraypArrayCells.Passivate();
}



////////////////////////////////////////////////////////////////////////////////
//
//  IUnknown specific interfaces
//
////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CSimpleTabularData::QueryInterface (REFIID riid, LPVOID * ppv)
{
    TraceTag((tagSimpleTabularData,
             "CSimpleTabularData::QueryInterface(%p {%p, %p})",
             this, riid, ppv ));

    HRESULT hr;

    Assert(ppv);

    // This is the non-delegating IUnknown implementation
    if (riid == IID_IUnknown || riid == IID_OLEDBSimpleProvider)
    {
        *ppv = this;
        ((LPUNKNOWN)*ppv)->AddRef();
        hr = S_OK;
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }

    RRETURN(hr);
}



STDMETHODIMP_(ULONG)
CSimpleTabularData::AddRef ()
{
    return ++_cRef;
}



STDMETHODIMP_(ULONG)
CSimpleTabularData::Release ()
{
    _cRef -= 1;
    if (!_cRef)
    {
        _cRef = ULREF_IN_DESTRUCTOR;
        delete this;
        return 0;
    }
    return _cRef;
}



//+-----------------------------------------------------------------------
//
//  Member:    Create
//
//  Synopsis:  Initializes STD.  Creates one cell in the table at row 0,
//             column 0, which is a label.  Also initializes the
//             dimensions of the table to 0 by 0.
//             The table contains 1 cell even though the dimensions are
//             zero because column and row 0 are used to hold labels.
//             Thus, the table can never be truly "empty".
//             It is expected that this be called only after constructing
//             an STD object.
//
//  Arguments: None.
//
//  Returns:   Success if memory can be allocated for table.
//             Returns E_OUTOFMEMORY if can't get memory.
//
//------------------------------------------------------------------------

HRESULT
CSimpleTabularData::Create ()
{
    TSTDArray<CCell> *pArrayCellsTempRow;
    HRESULT hr;

    Assert(this);

#if DBG == 1
    Assert("Only call Create() once" && !_bInitialized);
    _bInitialized = TRUE;
#endif

    // Initialize dimensions:

    _cColumns = 0;
    _cRows = 0;

    // Initializes event interface to null:
    _pEvents = NULL;

    // Initialize table:

    hr = _ArraypArrayCells.Init(1);        // only 1 to start with (label)
    if (hr)
    {
        goto Cleanup;
    }

    pArrayCellsTempRow = new(Mt(CSimpleTabularData_ArraypArrayCells)) TSTDArray<CCell>();
    if (!pArrayCellsTempRow)
    {
        hr = E_OUTOFMEMORY;
        goto Error2;
    }

    hr = pArrayCellsTempRow->Init(1);            // create a row of 1 cell
    if (hr)
    {
        goto Error3;
    }

    _ArraypArrayCells[0] = pArrayCellsTempRow;   // set first row to row created

Cleanup:
    RRETURN(hr);

Error3:
    // We need to deallocate the temporary row
    delete pArrayCellsTempRow;

Error2:
     // We need to deallocate array of rows:
    _ArraypArrayCells.Passivate();
    goto Cleanup;
}



//+-----------------------------------------------------------------------
//
//  Member:    CreatePopulateSTD
//
//  Synopsis:  Initializes STD and inserts data.  Calls Create method to
//             initialize.  If STD can be properly initialized, the appropriate
//             number of columns are inserted, and then rows are inserted.
//             Using a semicolon-separated string, the cells of the STD are
//             filled with strings.  Note that the table that this will create
//             will contain a row (#0) and a column (#0) for labels.  The
//             actual data will be inserted starting at row and column #1.
//
//             The init string may contain null characters.  Each string is
//             separated by a semicolon.  Strings may be surrounded by quotes,
//             in which case they may contain semicolons.  Either double or
//             single quotes may be used, thus allowing one to embed the other
//             type of quotemark within the string.
//
//  Arguments: cColumns           # of data columns to create
//             cchBuf             # of characters in init string
//             pchBuf             pointer to init string
//             fLabels            flag indicating column labels should be used
//
//  Returns:   Success if memory can be allocated for table.
//             Returns E_OUTOFMEMORY if can't get memory.
//
//------------------------------------------------------------------------

HRESULT
CSimpleTabularData::CreatePopulateSTD (ULONG cColumns, ULONG cchBuf,
                                       TCHAR *pchBuf, BOOL fLabels )
{
    HRESULT hr = S_OK;
    ULONG iCurrRow;                               // row to start filling at
    ULONG iCurrCol;                               // column to start filling at
    TCHAR *pchStart = pchBuf;                     // start of next string
    TCHAR *pchEnd = pchBuf + cchBuf;              // 1 char past last in string
    TCHAR *pchIndex = pchBuf;                     // index the string
    TCHAR chDelim;                                // character delimeter
    VARIANT tempVar;

    Assert(this);

    // Check arguments:
    if ((cchBuf && !pchBuf) || cColumns == 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Initialize table:
    hr = Create();
    if (hr)
    {
        goto Cleanup;
    }

    {
        ULONG cTempCols;                  // we don't need this value
        hr = InsertColumns(1, cColumns, (LONG *)&cTempCols);
        if (hr)
        {
            goto Error0;
        }
        Assert ("Didn't get right number of  columns" &&
                (cColumns == cTempCols) );
    }

    // Start filling in strings:
    for (iCurrRow = fLabels ? 0 : 1;
         pchIndex < pchEnd;
         iCurrRow++ )
    {
        if (iCurrRow != 0)           // column 0 already exists for labels
        {
            DBROWCOUNT cTempRows;
            hr = insertRows(iCurrRow, 1, &cTempRows);
            if (hr)
            {
                goto Error0;
            }
            Assert ("Didn't get right number of rows" &&
                    (1 == cTempRows));
        }

        for (iCurrCol = 1;
             pchIndex < pchEnd && iCurrCol <= cColumns;
             iCurrCol++ )
        {
            // Set the delimeter for the end of this string:
            if ((*pchIndex == '"') || (*pchIndex == '\''))
            {
                chDelim = *pchIndex;            // scan for a quote
                pchStart++;                     // skip this quote
                pchIndex++;
            }
            else
            {
                chDelim = ';';
            }

            // Scan for ending delimeter:
            while ((pchIndex < pchEnd) && (*pchIndex != chDelim))
            {
                pchIndex++;
            }

            // Put string in table:
            tempVar.vt = VT_BSTR;
            tempVar.bstrVal = SysAllocStringLen(pchStart, pchIndex - pchStart);
            if (!tempVar.bstrVal)
            {
                hr = E_OUTOFMEMORY;
                goto Error0;
            }

            hr = setVariant(iCurrRow, iCurrCol, OSPFORMAT_FORMATTED, tempVar);
            VariantClear(&tempVar);
            if (hr)
            {
                goto Error0;
            }

            pchIndex++;             // skip past delimeter

            if (chDelim != ';')     // after quote, there should be a semicolon
            {
                while ((pchIndex < pchEnd) && (*pchIndex != ';'))
                {
                    pchIndex++;
                }

                pchIndex++;         // skip past delimeter
            }

            pchStart = pchIndex;    // set new string start
        }
    }

Cleanup:
    RRETURN(hr);

Error0:
    // Delete all rows just created, and then passivate array of rows:
    KillRows(0, _cRows + 1);
    _ArraypArrayCells.Passivate();
    goto Cleanup;
}



//+-----------------------------------------------------------------------
//
//  Member:    GetRowCount
//
//  Synopsis:  Gives user number of rows in the table.  Note that a
//             table of dimensions 0 by 3 actually contains 4 elements,
//             since it has 1 row and 4 (0...3) columns.
//
//  Arguments: pcRows          pointer to number of rows    (OUT)
//             pcColumns       pointer to number of columns (OUT)
//
//  Returns:   Success.
//
//------------------------------------------------------------------------

STDMETHODIMP
CSimpleTabularData::getRowCount (DBROWCOUNT *pcRows)
{
    TraceTag((tagSimpleTabularData,
        "CSimpleTabularData::GetDimensions -> %p", this));

    Assert("Out pointers cannot be null" && pcRows);

    IS_VALID(this);

    *pcRows = (LONG)_cRows;

    RRETURN(S_OK);
}

//+-----------------------------------------------------------------------
//
//  Member:    GetColumnCount
//
//  Synopsis:  Gives user the number of rows in the table.  Note that a
//             table of dimensions 0 by 3 actually contains 4 elements,
//             since it has 1 row and 4 (0...3) columns.
//
//  Arguments: pcRows          pointer to number of rows    (OUT)
//             pcColumns       pointer to number of columns (OUT)
//
//  Returns:   Success.
//
//------------------------------------------------------------------------

STDMETHODIMP
CSimpleTabularData::getColumnCount (DB_LORDINAL *pcColumns)
{
    TraceTag((tagSimpleTabularData,
        "CSimpleTabularData::GetDimensions -> %p", this));

    Assert("Out pointers cannot be null" && pcColumns);

    IS_VALID(this);

    *pcColumns = (LONG)_cColumns;

    RRETURN(S_OK);
}


//+-----------------------------------------------------------------------
//
//  Member:    DeleteRows
//
//  Synopsis:  Used to delete rows from the table.  Bounds are checked
//             to make sure that the rows can all be deleted.  Label row
//             cannot be deleted.
//
//  Arguments: iRow            first row to delete
//             cRows           number of rows to delete
//             pcRowsDeleted   actuall number of rows delete (OUT)
//
//  Returns:   Success if all rows could be deleted
//             E_INVALIDARG if row is out of allowed bounds.
//
//------------------------------------------------------------------------

STDMETHODIMP
CSimpleTabularData::deleteRows (DBROWCOUNT iRow, DBROWCOUNT cRows, DBROWCOUNT *pcRowsDeleted)
{
    HRESULT hr = S_OK;

    TraceTag((tagSimpleTabularData,
        "CSimpleTabularData::deleteRows -> %p", this));

    Assert("Out pointers cannot be null" &&
        (pcRowsDeleted));

    IS_VALID(this);

    *pcRowsDeleted = 0;         // no rows deleted in case of error
    // Check bounds:
    if ((iRow == 0) || (_cRows+1 < (ULONG)(iRow+cRows)))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // First we need to delete the actual rows:

    KillRows((ULONG)iRow, (ULONG)cRows);

    // Now reduce the number of rows:
    _ArraypArrayCells.DeleteElems((ULONG)iRow, (ULONG)cRows);

    // Finally, set the dimensions:
    _cRows -= (ULONG)cRows;

    *pcRowsDeleted = cRows;               // return value

    // Fire events:
    if (_pEvents)
    {
        hr = _pEvents->deletedRows(iRow, cRows);
    }

Cleanup:
    RRETURN(hr);
}



//+-----------------------------------------------------------------------
//
//  Member:    InsertRows
//
//  Synopsis:  Allows for the insertion of new rows.  This can either be
//             used to insert new rows between existing rows, or to
//             append new rows to the end of the table.  Thus, to
//             insert new rows at the end of the table, a user would
//             specify the initial row as 1 greater than the current
//             row dimension.
//             Note that iRow is checked to ensure that it is within the
//             proper bounds (1.._cRows+1).
//             User cannot delete label row.
//
//  Arguments: iRow            where to insert rows
//             cRows           how many rows to insert
//             pcRowsInserted  actuall number of rows inserted (OUT)
//
//  Returns:   Success if all rows could be inserted.
//             E_INVALIDARG if row is out of allowed bounds.
//             It is possible that fewer than the requested rows were
//             inserted.  In this case, E_OUTOFMEMORY would be returned,
//             and the actuall number of rows inserted would be set.
//
//------------------------------------------------------------------------

STDMETHODIMP
CSimpleTabularData::insertRows (DBROWCOUNT iRow, DBROWCOUNT cRows, DBROWCOUNT *pcRowsInserted)
{
    HRESULT hr;
    TSTDArray<CCell> *pArrayCellsTempRow;     // temporary row to be created
    ULONG i = (ULONG)iRow;

    TraceTag((tagSimpleTabularData,
        "CSimpleTabularData::InsertRows -> %p", this));

    Assert("Out pointers cannot be null" &&
        (pcRowsInserted));

    IS_VALID(this);

    if ((iRow == 0) || ((_cRows+1) < (ULONG)iRow))  // check bounds
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // First we need to expand the array of pointers to rows, to accomodate
    //   more rows.
    hr = _ArraypArrayCells.InsertElems(iRow, cRows);
    if (hr)
    {
        goto Cleanup;
    }

    // Now that rows have been shifted, we want to insert all new rows in
    //   their places.  If not all of the rows can be created, we will
    //   reduce the total number of rows and move the old rows back in closer.

    for (i=(ULONG)iRow; i < ((ULONG)iRow + (ULONG)cRows); i++)
    {
        pArrayCellsTempRow = new(Mt(CSimpleTabularData_ArraypArrayCells)) TSTDArray<CCell>();
        if (!pArrayCellsTempRow)
        {
            hr = E_OUTOFMEMORY;
            goto Error1;
        }

        // create a row of _cColumns+1 cells:
        hr = pArrayCellsTempRow->Init(_cColumns+1);
        if (hr)
        {
            goto Error2;
        }

        _ArraypArrayCells[i] = pArrayCellsTempRow;    // set row to row created
    }

    Assert("i isn't where it should be" &&
        ((ULONG)cRows == i - (ULONG)iRow));

Cleanup:
    // Set new dimensions:
    _cRows += (i - (ULONG)iRow);

    // Return number of rows inserted
    *pcRowsInserted = ((LONG)i - iRow);

    // Fire events:
    if (_pEvents)
    {
        hr = _pEvents->insertedRows(iRow, (LONG)i - iRow);
    }

    RRETURN(hr);

Error2:
// Delete row object, then do error cleanup
    delete pArrayCellsTempRow;

Error1:
    // We couldn't create a new row object.
    // We need to move in the old rows, and reduce the size of the row array.
    // Note: Index i is currently on the row that couldn't be inserted

    _ArraypArrayCells.DeleteElems((LONG)i, iRow+cRows-(LONG)i);

    goto Cleanup;
}


//+-----------------------------------------------------------------------
//
//  Member:    DeleteColumns
//
//  Synopsis:  Used to delete columns from the table.  Bounds are checked
//             to make sure that the columns can all be deleted.  Label
//             column cannot be deleted.
//
//  Arguments: iColumn            first column to delete
//             cColumns           number of columns to delete
//             pcColumnsDeleted   actuall number of rows delete (OUT)
//
//  Returns:   Success if all columns could be deleted
//             E_INVALIDARG if column is out of allowed bounds.
//
//------------------------------------------------------------------------

STDMETHODIMP
CSimpleTabularData::DeleteColumns (LONG iColumn, LONG cColumns,
                                   LONG *pcColumnsDeleted)
{
    HRESULT hr = S_OK;

    TraceTag((tagSimpleTabularData,
        "CSimpleTabularData::DeleteColumns -> %p", this));

    Assert("Out pointers cannot be null" &&
        (pcColumnsDeleted));

    IS_VALID(this);

    // Check bounds:
    if ((iColumn == 0) || (_cColumns+1 < (ULONG)(iColumn+cColumns)))
    {
        hr = E_INVALIDARG;
        *pcColumnsDeleted = 0;         // no columns deleted
        goto Cleanup;
    }

    {
        ULONG i;

        for (i=0; i <= _cRows; i++)        // do all including last
        {
            _ArraypArrayCells[i]->DeleteElems((ULONG)iColumn, (ULONG)cColumns);
        }
    }

    // Set new dimensions
    _cColumns -= (ULONG)cColumns;

    // Set number of columns deleted:
    *pcColumnsDeleted = cColumns;

#ifdef NEVER
    // Column deletion events are gone, as is (officially) column deletion.
    // Fire events:
    if (_pEvents)
    {
        hr = _pEvents->DeletedColumns((ULONG)iColumn, (ULONG)cColumns);
    }
#endif

Cleanup:
    RRETURN(hr);
}



//+-----------------------------------------------------------------------
//
//  Member:    InsertColumns
//
//  Synopsis:  Allows for the insertion of new columns.  This can either
//             be used to insert new columns between existing columns, or
//             to append new columns to the end of the table.  Thus, to
//             insert new columns at the end of the table, a user would
//             specify the initial column as 1 greater than the current
//             column dimension.  Note that iColumn is checked to ensure
//             that it is within the proper bounds (1.._cColumns+1).
//             User cannot delete label column.
//
//             NOTE: In this member function, memcopies are done on objects.
//               Although this speeds things up, it also makes the assumption
//               that cells do not contain pointers to themselves, or
//               anything else that would tie them down to a particular
//               memory location.  So, don't add any of those restrictions
//               to a cell if it is to be modified.
//
//  Arguments: iColumn            where to insert column
//             cColumns           how many columns to insert
//             pcColumnsInserted  actuall number of columns inserted (OUT)
//
//  Returns:   Success if all columns could be inserted.
//             E_INVALIDARG if column is out of allowed bounds.
//             E_OUTOFMEMORY is returned if no columns can be inserted.
//             Note that a partial number of the columns won't be inserted.
//
//------------------------------------------------------------------------

STDMETHODIMP
CSimpleTabularData::InsertColumns (LONG iColumn, LONG cColumns,
                                   LONG *pcColumnsInserted)
{
    HRESULT hr = S_OK;
    ULONG i;

    TraceTag((tagSimpleTabularData,
        "CSimpleTabularData::InsertColumns -> %p", this));

    Assert("Out pointers cannot be null" &&
        (pcColumnsInserted));

    IS_VALID(this);

    if ((iColumn == 0) || ((_cColumns+1) < (ULONG)iColumn))  // check bounds
    {
        hr = E_INVALIDARG;
        goto Error1;
    }

    // First we must extend all rows to their new size.
    //   If ANY row cannot be extended, then all rows are reverted back
    //   to their original size, and an error is returned.

    for (i=0; i <= _cRows; i++)        // do all including last
    {
        hr = _ArraypArrayCells[i]->InsertElems((ULONG)iColumn, (ULONG)cColumns);
        if (hr)
        {
            goto Error2;
        }
    }

    // Set new dimensions:
    _cColumns += (ULONG)cColumns;
    // Set number of columns inserted:
    *pcColumnsInserted = cColumns;

#ifdef NEVER
    // Fire events:
    if (_pEvents)
    {
        hr = _pEvents->InsertedColumns((ULONG)iColumn, (ULONG)cColumns);
    }
#endif

Cleanup:
    RRETURN(hr);

Error2:
    // Ran out of memory.  Must revert rows back to old size:
    while (i > 0)         // revert rows i-1 down to and including 0
    {
        i--;
        // Reduce row size:
        _ArraypArrayCells[i]->DeleteElems((ULONG)iColumn, (ULONG)cColumns);
    }

Error1:
    // The bounds were wrong.  Simply set inserted columns to 0 and exit:
    *pcColumnsInserted = 0;
    goto Cleanup;
}



//+-----------------------------------------------------------------------
//
//  Member:    GetCell
//
//  Synopsis:  Gets a pointer to a cell in the table.  Usefull for
//             either looking up a variant or writing to one.  This
//             member function takes on the responsibility to check the
//             bounds on the row and column indices.  However, it
//             does not attempt to access a cell's read/write status,
//             so it is the responsibility of the user of this member
//             function to ensure that they do not write to a cell
//             that is supposed to be written to.
//
//  Arguments: iRow            row index
//             iColumn         column index
//             rpCell          reference of pointer to a cell (OUT)
//
//  Returns:   Success if indices are correct.
//             E_INVALIDARG if indices are out of bounds.
//
//------------------------------------------------------------------------

HRESULT
CSimpleTabularData::GetCell (ULONG iRow, ULONG iColumn, CCell *&rpCell)
{
    HRESULT hr;

    IS_VALID(this);

    // Check bounds first:
    if ((_cRows < iRow) || (_cColumns < iColumn))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Get the cell:
    rpCell = &(*_ArraypArrayCells[iRow])[iColumn];

    hr = S_OK;

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------
//
//  Member:    GetRWStatus
//
//  Synopsis:  Gets the read/write status of a cell.  Cell bound checking
//             is performed by GetCell().  Since this implementation of STD
//             can never set the read/write status of a cell anywhere, all
//             cells are presumed to have READ/WRITE access.  Therefore, it
//             is not necessary to keep track of this information in
//             individual cells, and this function need only return
//             the value RW_READWRITE.
//
//  Arguments: iRow            row index
//             iColumn         column index
//             prwStatus       pointer to read/write status (OUT)
//
//  Returns:   Success if indices are correct.
//             E_INVALIDARG if indices are out of bounds.
//
//------------------------------------------------------------------------

STDMETHODIMP
CSimpleTabularData::getRWStatus (DBROWCOUNT iRow, DB_LORDINAL iColumn, OSPRW *prwStatus)
{
    HRESULT hr = S_OK;

    TraceTag((tagSimpleTabularData,
        "CSimpleTabularData::GetRWStatus -> %p", this));

    Assert("Out pointers cannot be null" && prwStatus);

    IS_VALID(this);

    // Check the boundary of iRow and iColumn.
    if (((iRow != -1) && (_cRows < (ULONG)iRow)) ||
        ((iColumn != -1) && (_cColumns < (ULONG)iColumn)) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *prwStatus = OSPRW_READWRITE;

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------
//
//  Member:    GetVariant
//
//  Synopsis:  Gets the data of a cell.  Cell bound checking
//             is performed by GetCell().  It is important that this
//             function return a true variant.  To achieve this, we
//             convert the string to bstr if necessary.
//
//  Arguments: iRow            row index
//             iColumn         column index
//             pVar            pointer to variant      (OUT)
//
//  Returns:   Success if indices are correct.
//             E_INVALIDARG if indices are out of bounds.
//
//------------------------------------------------------------------------

STDMETHODIMP
CSimpleTabularData::getVariant (DBROWCOUNT iRow, DB_LORDINAL iColumn,
                                OSPFORMAT format, VARIANT *pVar)
{
    HRESULT hr;
    CCell *pCell;

    TraceTag((tagSimpleTabularData,
        "CSimpleTabularData::getVariant -> %p", this));

    Assert("Out pointers cannot be null" &&
        (pVar));

    IS_VALID(this);

    hr = GetCell((ULONG)iRow, (ULONG)iColumn, pCell);   // get the cell
    if (hr)
    {
        goto Cleanup;
    }

    {
        VARIANT &rvTempVar = pCell->GetCellVariant();   // get the variant

        if (rvTempVar.vt==VT_EMPTY)
        {
            // Return unitialized cells as VT_NULL
            rvTempVar.vt = VT_NULL;
        }
        
        // If a cstring is currently in the cell, we need to convert it
        //   to a bstr.  In case this is done repetitively, we change
        //   the cell to contain a bstr in the variant.
        if (pCell->IsCString())
        {
            // Convert cstring to bstr:
            hr = (pCell->GetCellString()).AllocBSTR(&(rvTempVar.bstrVal));
            if (hr)
            {
                goto Cleanup;
            }

            // NOTE: We deallocate the cstring here.
            (pCell->GetCellString()).Free();

            // Set variant to hold a bstr:
            rvTempVar.vt = VT_BSTR;
            // Set type of cell to variant:
            pCell->SetIsVariant();
        }
        else
        {
            if (format==OSPFORMAT_FORMATTED)
            {
                hr = VariantChangeTypeEx(pVar, &rvTempVar, CP_ACP, 0, VT_BSTR);
            }
            else
            {
                hr = VariantCopy(pVar, &rvTempVar);
            }
        }
    }

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------
//
//  Member:    SetVariant
//
//  Synopsis:  Sets the data of a cell.  Cell bound checking
//             is performed by GetCell().  This member function checks
//             to see that the cell status permits writing to it.  A call
//             is made to the destructor of the cell so that the old
//             variant or cstring can be cleaned up before the new one
//             replaces it.
//
//  Arguments: iRow            row index
//             iColumn         column index
//             Var             pointer to variant
//
//  Returns:   Success if indices are correct.
//             E_INVALIDARG if indices are out of bounds.
//             E_INVALIDARG if cell cannot be written to.
//
//------------------------------------------------------------------------

STDMETHODIMP
CSimpleTabularData::setVariant (DBROWCOUNT iRow, DB_LORDINAL iColumn,
                                OSPFORMAT format, VARIANT Var)
{
    HRESULT hr;
    CCell *pCell;

    TraceTag((tagSimpleTabularData,
        "CSimpleTabularData::setVariant -> %p", this));

    IS_VALID(this);

    hr = GetCell((ULONG)iRow, (ULONG)iColumn, pCell);   // get the cell
    if (hr)
    {
        goto Cleanup;
    }

    // NOTE: We free the cstring here.
    if (pCell->IsCString())
    {
        (pCell->GetCellString()).Free();
    }

    if (format==OSPFORMAT_FORMATTED)
    {
        hr = VariantChangeTypeEx(&(pCell->GetCellVariant()), &Var,
                                 CP_ACP, 0, VT_BSTR);
    }
    else {
        // VariantCopy deallocates the old variant.
        hr = VariantCopy(&(pCell->GetCellVariant()), &Var);
    }
    if (hr)
    {
        goto Cleanup;
    }

    // Set cell type to variant:
    pCell->SetIsVariant();

    // Fire events:
    if (_pEvents)
    {
        hr = _pEvents->cellChanged(iRow, iColumn);
    }

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------
//
//  Member:    GetLocale
//
//  Synopsis:  Return the locale of the underlying data set
//
//  Arguments: pbstrLocale     Where to return locale string
//
//  Returns:   S_OK
//
//------------------------------------------------------------------------

STDMETHODIMP
CSimpleTabularData::getLocale(BSTR *pbstrLocale)
{
    HRESULT hr = S_OK;
    if (pbstrLocale)
    {
        *pbstrLocale = NULL;    // BUGBUG - call GetSystemLocale
    }
    else
        hr = E_INVALIDARG;

    return hr;
}


#ifdef NEVER
//+-----------------------------------------------------------------------
//
//  Member:    SetString
//
//  Synopsis:  Used to store a string in a cell.  Checks to make sure cell
//             is valid.  To make sure that VARIANTS are properly cleared,
//             this function has to first fetch the cell it wants to write
//             to.  Strings are stored as CStrings.  This function makes
//             a copy of the string.
//
//  Arguments: iRow           row of cell
//             iColumn        column of cell
//             cchBuf         size of string
//             pchBuf         pointer to the string
//
//  Returns:   Success if string can be set.
//             E_INVALIDARG if cell is out of range
//             Error if string can't be copied.
//
//------------------------------------------------------------------------

STDMETHODIMP
CSimpleTabularData::SetString (LONG iRow, LONG iColumn, BSTR bstr)
{
    HRESULT hr;
    CCell *pCell;

    TraceTag((tagSimpleTabularData,
        "CSimpleTabularData::SetString -> %p", this));

    IS_VALID(this);

    hr = GetCell((ULONG)iRow, (ULONG)iColumn, pCell);   // get the cell
    if (hr)
    {
        goto Cleanup;
    }

    // NOTE: We clear the variant here.
    if (pCell->IsVariant())
    {
        VariantClear(&(pCell->GetCellVariant()));
        pCell->GetCellVariant().vt = VT_NULL;
    }

    // Set the string.  Set() clears the old string.
    hr = (pCell->GetCellString()).SetBSTR(bstr);
    if (hr)
    {
        goto Cleanup;
    }

    // Set type of cell to cstring:
    pCell->SetIsCString();

    // Fire events:
    if (_pEvents)
    {
        hr = _pEvents->CellChanged((ULONG)iRow, (ULONG)iColumn);
    }

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------
//
//  Member:    GetString
//
//  Synopsis:  Used to get a string from a cell.  Checks to make sure cell
//             is valid.  We would make this function use GetVariant as
//             a helper, except that we don't need a true variant that
//             uses bstr.  We only need to get a CString, which is much
//             much quicker.  This function will return either the number
//             of characters that the user requested, or the number of
//             characters stored internally, whichever is shorter.
//             One of the issues in this function is that the string
//             may either be stored in the variant part of the cell or in
//             the cstring part.  This function must handle both cases.
//             Note that a user may pass in a NULL pointer in pchBuf if they
//             also pass in cchBuf==0.  This case allows a user to get the
//             length of the string back.
//
//  Arguments: iRow           row of cell
//             iColumn        column of cell
//             cchBuf         size of string wanted
//             pchBuf         pointer to the string
//             pcchActual     actual size of string
//
//  Returns:   Success if string can be found.
//             E_INVALIDARG if cell is out of range
//             E_INVALIDARG if cell does not contain a string
//             NOTE: On error, fills buffer with "#ERROR"
//
//------------------------------------------------------------------------

STDMETHODIMP
CSimpleTabularData::GetString (LONG iRow, LONG iColumn, BSTR *pbstr)
{
    HRESULT hr;
    CCell *pCell;

    TraceTag((tagSimpleTabularData,
        "CSimpleTabularData::GetString -> %p", this));

    Assert("Out pointers cannot be null" && pbstr);

    IS_VALID(this);

    *pbstr = NULL;

    hr = GetCell((ULONG)iRow, (ULONG)iColumn, pCell);   // get the cell
    if (hr)
    {
        goto Error1;
    }

    if (pCell->IsCString())
    {
        // Now we have to copy the string to the buffer of the user:
        // Note that if the destination buffer is too short, it just won't all
        //   be copied.
        hr = pCell->GetCellString().AllocBSTR(pbstr);
    }
    else
    {
        // Declaration:
        VARIANT &rvTempVar = pCell->GetCellVariant();     // Get the variant

        // Is there actually a bstr inside the variant?!
        if (rvTempVar.vt == VT_BSTR)
        {
            // If so, we need to make a copy of the BSTR
            *pbstr = SysAllocStringLen(rvTempVar.bstrVal,
                                       SysStringLen(rvTempVar.bstrVal));
            if (*pbstr==NULL)
                hr = E_OUTOFMEMORY;
        } else if (rvTempVar.vt == VT_NULL)
        {
            // If it's NULL, we return a NULL string.
            // Note that *pbstr is already NULL, which is a valid representation
            // of an empty string, so we just return it.

        } else
        {
            hr = E_INVALIDARG;
            goto Error1;
        }
    }
Cleanup:
    RRETURN(hr);

Error1:
    *pbstr = SysAllocString(_T("#ERROR"));
    goto Cleanup;
}

//+-----------------------------------------------------------------------
//
//  Member:    FindPrefixString
//
//  Synopsis:  Used to find a string in a column.  A verbose explanation
//             of the search flags can be found in the class definition.
//             User cannot search for a string inside the label column
//             and row (iColumn=0 or iRowStart=0).
//
//  Arguments: iRowStart      first row searched
//             iColumn        column to search in
//             cchBuf         characters in search string
//             pchBuf         search string
//             findFlags      flags for search
//             foundFlag      found flag            (OUT)
//             piRowFound     row string found in   (OUT)
//
//  Returns:   Success if target string is found.
//             E_FAIL if no match could be made.
//             E_INVALIDARG if the rows/columns are out of range, or
//               if the search flags are invalid.
//
//------------------------------------------------------------------------

STDMETHODIMP
CSimpleTabularData::FindPrefixString (LONG iRowStart, LONG iColumn,
                                      BSTR bstr,
                                      OSPFIND findFlags, OSPFIND *foundFlag,
                                      LONG *piRowFound)
{
    HRESULT hr = S_OK;
    CCell *pCellTempCell;               // pointer to current cell
    TCHAR *pstrTempString;              // pointer to current string in cell
    ULONG cTempString;                  // length of string in cell
    ULONG iCurrRow;                     // current cell being scanned
    LONG iFoundExact = -1;              // row of exact match
    LONG iFoundPrefix = -1;             // row of prefix match
    LONG iFoundNearest = -1;            // row of nearest match
    ULONG cchBuf = SysStringLen(bstr);

    int sTempCompare;           // compares search string and string in table
    int ( RTCCONV *pfnComparer)(const TCHAR *string1,
                                int cch1,
                                const TCHAR *string2,
                                int cch2) =
                  ((findFlags & OSPFIND_CASESENSITIVE) ? _tcsncmp : _tcsnicmp);

    int sIncNext = (findFlags & OSPFIND_UP) ? -1 : 1;   // Incr to next cell

    TraceTag((tagSimpleTabularData,
        "CSimpleTabularData::FindPrefixString -> %p", this));

    Assert("Out pointers cannot be null" && piRowFound);

    IS_VALID(this);

    if (iRowStart == 1 && _cRows == 0)
    {
        hr = E_FAIL;                        // no match found
        goto Cleanup;
    }

    // Check parameters:
    if ((iRowStart==0) || (_cRows < (ULONG)iRowStart) ||           // check row
        (iColumn==0) || (_cColumns < (ULONG)iColumn) ||            // check column
        (!(findFlags & (OSPFIND_EXACT | OSPFIND_PREFIX))))  // must have a mode
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Initialize scanning coordinates:
    iCurrRow = iRowStart;


    // Do scan:


    // NOTE: we don't scan the column label.  If we ever do decide to,
    //   then the following should be changed to a do-while loop, since
    //   the check to end the loop here will fail since there are no
    //   negative numbers.
    while((findFlags & OSPFIND_UP) ? (iCurrRow >= 1) : (iCurrRow <= _cRows))
    {
        // We either must get string from the variant or the cstring.
        //   But first get the cell:
        hr = GetCell(iCurrRow, iColumn, pCellTempCell);
        if (hr)
        {
            goto Cleanup;
        }

        // Is it a cstring?
        if (pCellTempCell->IsCString())
        {
            pstrTempString = LPTSTR(pCellTempCell->GetCellString());
            cTempString = pCellTempCell->GetCellString().Length();
        }
        // It must be in the variant.  The variant must contain a string:
        else if ((pCellTempCell->GetCellVariant()).vt != VT_BSTR)
        {
            // This is not a string, so skip it.
            iCurrRow += sIncNext;          // goto next cell
            continue;
        }
        else
        {
            pstrTempString = LPTSTR((pCellTempCell->GetCellVariant()).bstrVal);
            cTempString = FormsStringLen((pCellTempCell->
                                          GetCellVariant()).bstrVal);
        }


        // See if we can find a match:
        // Note that we compare using the minimum length of both strings in
        //   case the cell string is an old string in memory, which can still
        //   match the search string.
        sTempCompare = (*pfnComparer)(bstr, __min(cchBuf, cTempString),
                            pstrTempString, __min(cchBuf, cTempString));


// *** See if we found a match:

        if (sTempCompare == 0)
        {
            // Check to see if match was exact (compared strings same length):
            if (cchBuf == cTempString)
            {
                iFoundExact = iCurrRow;
                if (!(findFlags & OSPFIND_LAST))
                {
                    break;
                }
            } // Only set OSPFIND_PREFIX if we are looking for one.  Ie, if
              // doing a OSPFIND_EXACT only search, don't return OSPFIND_PREFIX.
              // Also, if already found a OSPFIND_PREFIX, and OSPFIND_LAST is
              // not set, should not set this value again.
            else if ((findFlags & OSPFIND_PREFIX) && (cchBuf < cTempString) &&
                ((iFoundPrefix == -1) || (findFlags & OSPFIND_LAST)))
            {
                iFoundPrefix = iCurrRow;
                if (!(findFlags & OSPFIND_LAST) && !(findFlags & OSPFIND_EXACT))
                {
                    break;
                }
            }
        }
        else
        {
            // No match.  See if we found nearest:
            if ((findFlags & OSPFIND_NEAREST) &&   // trying to find nearest
                (iFoundNearest == -1) &&           // didn't already find it
                ((sTempCompare < 0) == !(findFlags & OSPFIND_UP))) // nearest
            {
                // Don't bail here, since might get exact/prefix match later:
                iFoundNearest = iCurrRow;
            }

            // See when to bail here:
            // Bail out at the end of a prefix-matching contiguous block:
            if ((findFlags & OSPFIND_PREFIX) &&
                ((iFoundPrefix != -1) ||
                (iFoundExact != -1)))   // if previous match found
            {
                break;
            }
        }


        // Get the next cell:
        iCurrRow += sIncNext;
    }

// *** Return the correct type of match found:

    if ((iFoundExact != -1) &&
        // If we're looking for the last exact match, return it.  It is
        //   possible that a prefix match was found later in the contiguous
        //   prefix-matching block.
        (((findFlags & OSPFIND_LAST) && (findFlags & OSPFIND_EXACT)) ||
        // It is possible that a prefix match was found after an exact,
        //   as when flags OSPFIND_PREFIX and OSPFIND_LAST are set, and an exact
        //   match is found anyway.  Thus, you want to return the larger of
        //   iFoundExact and iFoundPrefix, not just iFoundExact.
        (iFoundPrefix == -1) ||
        ((iFoundExact > iFoundPrefix) == !(findFlags & OSPFIND_UP))))
    {
        *foundFlag = OSPFIND_EXACT;
        *piRowFound = iFoundExact;
    }
    else if (iFoundPrefix != -1)
    {
        *foundFlag = OSPFIND_PREFIX;
        *piRowFound = iFoundPrefix;
    }
    else if (iFoundNearest != -1)
    {
        *foundFlag = OSPFIND_NEAREST;
        *piRowFound = iFoundNearest;
    }
    else
    {
        hr = E_FAIL;                        // no match found
        goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}
#endif

//+-----------------------------------------------------------------------
//
//  Member:    Find
//
//  Synopsis:  Used to find a value in a column.
//             User cannot search for a string inside the label column
//             and row (iColumn=0 or iRowStart=0).
//
//  Arguments: iRowStart      first row searched
//             iColumn        column to search in
//             val            Value to search for
//             findFlags      flags for search
//             compType       comparison made (NE,LT,LE,EQ,GT,GE)
//             piRowFound     row string found in   (OUT)
//
//  Returns:   Success if target string is found.
//             E_FAIL if no match could be made.
//             E_INVALIDARG if the rows/columns are out of range, or
//               if the search flags are invalid.
//
//------------------------------------------------------------------------
STDMETHODIMP
CSimpleTabularData::find (DBROWCOUNT iRowStart, DB_LORDINAL iColumn,
                          VARIANT var,
                          OSPFIND findFlags, OSPCOMP compType,
                          DBROWCOUNT *piRowFound)
{
    HRESULT hr = S_OK;
    CCell *pCellTempCell;                   // pointer to current cell
    ULONG iCurrRow;                         // current cell being scanned
    ULONG cchBuf;
    int tcompType;

    STRINGCOMPAREFN pfnComparer =
                  ((findFlags & OSPFIND_CASESENSITIVE) ? StrCmpC : StrCmpI);

    int sIncNext = (findFlags & OSPFIND_UP) ? -1 : 1;   // Incr to next cell

    Assert("Out pointers cannot be null" && (piRowFound));

    if (iRowStart == 1 && _cRows == 0)
    {
        hr = E_FAIL;                        // no match found
        goto Cleanup;
    }

    // Check parameters:
    if ((iRowStart == 0) || (_cRows < (ULONG)iRowStart) ||	// check row
        (iColumn == 0) || (_cColumns < (ULONG)iColumn))     // check column
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Initialize scanning coordinates:
    iCurrRow = iRowStart;

    // Do scan:
    
    // Cache the length of string to match
    if (var.vt==VT_BSTR)
    {
        cchBuf = SysStringLen(var.bstrVal);
    }

     // NOTE: we don't scan the column label.  If we ever do decide to,
    //   then the following should be changed to a do-while loop, since
    //   the check to end the loop here will fail since there are no
    //   negative numbers.
    while((findFlags & OSPFIND_UP) ? 
		(iCurrRow >= 1) : (iCurrRow <= _cRows))
    {
        // We either must get string from the variant or the cstring.
        //   But first get the cell:
        hr = GetCell(iCurrRow, iColumn, pCellTempCell);
        if (hr)
        {
            goto Cleanup;
        }

        VARIANT &cellVar = (pCellTempCell->GetCellVariant());
        if (cellVar.vt!=var.vt)
        {
            // Types do not match, skip it
            iCurrRow += sIncNext;          // goto next cell
            continue;            
        }

        tcompType = 0;

        switch (var.vt)
        {
            case VT_EMPTY:              // These always match each other
            case VT_NULL:
                break;

            case VT_I1:
                if (var.cVal < cellVar.cVal) tcompType = -1;
                else if (var.cVal > cellVar.cVal) tcompType = 1;
                break;

            case VT_UI1:
                if (var.bVal < cellVar.bVal) tcompType = -1;
                else if (var.bVal > cellVar.bVal) tcompType = 1;
                break;

            case VT_I2:
                if (var.iVal < cellVar.iVal) tcompType = -1;
                else if (var.iVal > cellVar.iVal) tcompType = 1;
                break;

            case VT_UI2:
                if (var.uiVal < cellVar.uiVal) tcompType = -1;
                else if (var.uiVal > cellVar.uiVal) tcompType = 1;
                break;

            case VT_I4:
                if (var.lVal < cellVar.lVal) tcompType = -1;
                else if (var.lVal > cellVar.lVal) tcompType = 1;
                break;

            case VT_UI4:
                if (var.ulVal < cellVar.ulVal) tcompType = -1;
                else if (var.ulVal > cellVar.ulVal) tcompType = 1;
                break;
                
            case VT_INT:
            case VT_UINT:
                if (var.intVal < cellVar.intVal) tcompType = -1;
                else if (var.intVal > cellVar.intVal) tcompType = 1;                
                break;

            case VT_BOOL:
                tcompType = var.boolVal != cellVar.boolVal;
                break;

            case VT_R4:
                if (var.fltVal < cellVar.fltVal) tcompType = -1;
                else if (var.fltVal > cellVar.fltVal) tcompType = 1;                
                break;

            case VT_R8:
                if (var.dblVal < cellVar.dblVal) tcompType = -1;
                else if (var.dblVal > cellVar.dblVal) tcompType = 1;                
                break;

            case VT_DATE:
                if (var.date < cellVar.date) tcompType = -1;
                else if (var.date > cellVar.date) tcompType = 1;
                break;

            case VT_BSTR:
                tcompType =  (*pfnComparer)(var.bstrVal, cellVar.bstrVal);
                break;

            case VT_CY:
#ifdef NEVER
                // Until we learn how to compare Currency types
                if (var.cyVal < cellVar.cyVal) tcompType = -1;
                else if (var.cyVal > cellVar.cyVal) tcompType = 1;
                break;
#endif

#ifndef WIN16
            case VT_DECIMAL:
#endif
            default:
                hr = E_INVALIDARG;      // Should we stop here?
                break;
        }

        switch (compType)
        {
            case OSPCOMP_EQ:
                if (tcompType==0) goto Done;
                break;
            case OSPCOMP_LT:
                if (tcompType<0) goto Done;
                break;
            case OSPCOMP_LE:
                if (tcompType<=0) goto Done;
                break;
            case OSPCOMP_GT:
                if (tcompType>0) goto Done;
                break;
            case OSPCOMP_GE:
                if (tcompType>=0) goto Done;
                break;
            case OSPCOMP_NE:
                if (tcompType!=0) goto Done;
                break;
        }
    }
    // If we fall through, then the search failed
    *piRowFound = -1;
Done:
Cleanup:
    return hr;
}

STDMETHODIMP
CSimpleTabularData::stopTransfer()
{
    return S_OK;
}


//+-----------------------------------------------------------------------
//
//  Member:    SetAdviseEvent
//
//  Synopsis:  Establish or detach single event sink.  Releases old interface,
//             if one was set.  Passing in NULL clears the previous
//             interface also, so it can be used to turn off event firing.
//             NOTE: Make sure to call release in std destructor on interface.
//
//  Arguments: pEvents        call events on this pointer
//
//  Returns:   Success.
//

STDMETHODIMP
CSimpleTabularData::addOLEDBSimpleProviderListener (OLEDBSimpleProviderListener *pEvent)
{
    // Release old interface:
    if (_pEvents)
    {
        _pEvents->Release();
        _pEvents = NULL;
    }

    // Set new interface:
    if (pEvent)
    {
        _pEvents = pEvent;
        _pEvents->AddRef();
    }

    RRETURN(S_OK);
}

STDMETHODIMP
CSimpleTabularData::removeOLEDBSimpleProviderListener (OLEDBSimpleProviderListener *pEvent)
{
    if (_pEvents==pEvent)
    {
        _pEvents->Release();
        _pEvents = NULL;
    }

    RRETURN(S_OK);
}



//+-----------------------------------------------------------------------
//
//  Member:    FormsSTDCreate
//
//  Synopsis:  Exported function to construct/create an STD, used by DRT and
//             Test Nile/STD.
//
//  Arguments: ppSTD        STD returned.
//
//  Returns:   S_OK
//

HRESULT FormsSTDCreate (LPOLEDBSimpleProvider *ppSTD, LONG cColumns)
{
    HRESULT             hr;
    CSimpleTabularData  *pImpSTD;

    CEnsureThreadState ets;
    hr = ets._hr;
    if (FAILED(hr))
        RRETURN(hr);

    if (!ppSTD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppSTD = NULL;

    pImpSTD = new CSimpleTabularData();
    if (!pImpSTD)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = pImpSTD->Create();
    if (!hr)
    {
        // Insert the initial number of columns
        hr = pImpSTD->InsertColumns(0, cColumns, &cColumns);
        if (!hr)
            *ppSTD = pImpSTD;
    }

    if (hr)
        pImpSTD->Release();

Cleanup:
    RRETURN(hr);
}



//+-----------------------------------------------------------------------
//
//  Member:    FormsSTDCreatePopulate
//
//  Synopsis:  Exported function to create an STD given a semi-colon delimited
//             string used by DRT and Test Nile/STD.
//
//  Arguments: cColumns           # of data columns to create
//             cchBuf             # of characters in init string
//             pchBuf             pointer to init string
//             fLabels            flag indicating column labels should be used
//             ppSTD              STD [out].
//
//  Returns:   S_OK               Everything parsed and allocated
//             E_OUTOFMEMORY      Not enough memory
//
//              
//+-----------------------------------------------------------------------
HRESULT FormsSTDCreatePopulate (ULONG cColumns, ULONG cchBuf,
                                OLECHAR *pchBuf, BOOL fLabels,
                                LPOLEDBSimpleProvider *ppSTD )
{
    HRESULT             hr;
    CSimpleTabularData  *pImpSTD;

    CEnsureThreadState ets;
    hr = ets._hr;
    if (FAILED(hr))
        RRETURN(hr);

    if (!ppSTD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppSTD = NULL;

    pImpSTD = new CSimpleTabularData();
    if (!pImpSTD)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    hr = pImpSTD->CreatePopulateSTD(cColumns, cchBuf, pchBuf, fLabels);
    if (!hr)
    {
        *ppSTD = pImpSTD;
    }
    else
    {
        pImpSTD->Release();
    }

Cleanup:
    RRETURN(hr);
}

