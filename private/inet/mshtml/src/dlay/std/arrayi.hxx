//+-----------------------------------------------------------------------
//
//  Simple Tabular Data
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:      arrayi.hxx
//  Authors:   Jerry Dunietz (jerryd@microsoft.com
//             Ido Ben-Shachar (t-idoben@microsoft.com)
//
//  Contents:  Header file for simple tabular data interface.
//             Allows client to create a table of VARIANTS which
//             can be manipulated.
//
//  Interfaces:ISimpleTabularData
//  Classes:   CArray
//
//  07/18/95:   TerryLu     Added sentries, IUnknown derivation, co-ordinated
//                          w/ IDL file, and bug fixes.
//
//------------------------------------------------------------------------

#ifndef _ARRAYI_HXX_
#define _ARRAYI_HXX_

#ifndef X_TARRAY_HXX_
#define X_TARRAY_HXX_
#include "tarray.hxx"
#endif

#ifndef X_CCELL_HXX_
#define X_CCELL_HXX_
#include "ccell.hxx"
#endif

//+-----------------------------------------------------------------------
//
//  Interface: ISimpleTabularDataEvents
//
//  Synopsis:  Allows ISimpleTabularData to notify others of changes
//             to the data.  When data is changed in the table, these
//             events are fired.
//
//  Methods:   CellChanged       Cell(s) somewhere in table changed
//
//             DeletedRows       Dimensions of table changed
//             InsertedRows
//             DeletedColumns
//             InsertedColumns             
//
//------------------------------------------------------------------------

//+-----------------------------------------------------------------------
//
//  Class:     CSimpleTabularData
//
//  Synopsis:  Allows for the use of a two-dimensional table.
//             Table is implemented as an edge array, or an array of
//             arrays.  The table is densely populated, so its
//             dimensions are always some set of integers, x by y.
//
//
//  Methods:   Create            initializes data structures of STD
//             CreatePopulateSTD initializes with semicolon-separated string
//             DeleteRows        ]
//             InsertRows        ]---Add/remove cells
//             DeleteColumns     ]
//             InsertColumns     ]
//             GetDimensions     get current dimensions of table
//             GetRWStatus       get read/write status of a cell
//             GetVariant        get a variant from a cell
//             SetVariant        write a variant to a cell
//             FindPrefixString  find string in table
//
//------------------------------------------------------------------------

MtExtern(CSimpleTabularData)

class CSimpleTabularData : public OLEDBSimpleProvider
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CSimpleTabularData))
    CSimpleTabularData();

    //  IUnknown
    STDMETHODIMP            QueryInterface (REFIID, LPVOID *);
    STDMETHODIMP_(ULONG)    AddRef ();
    STDMETHODIMP_(ULONG)    Release ();

    // ISimpleTabularData

    STDMETHODIMP Create ();
    STDMETHODIMP CreatePopulateSTD (ULONG cColumns, ULONG cchBuf,
                                    TCHAR *pchBuf, BOOL fLabels);

    // in case of error, provider may choose to insert or delete fewer
    //      rows then client requested.  OUT parameter indicates how many
    //      rows actually inserted or deleted.  In case of success,
    //      OUT parameter should be filled in with cRows.
    STDMETHODIMP deleteRows (DBROWCOUNT iRow, DBROWCOUNT cRows, DBROWCOUNT *pcRowsDeleted);
    STDMETHODIMP insertRows (DBROWCOUNT iRow, DBROWCOUNT cRows, DBROWCOUNT *pcRowsInserted);

    STDMETHODIMP getRowCount(DBROWCOUNT *pcRows);
    STDMETHODIMP getColumnCount (DB_LORDINAL *pcColumns);
    STDMETHODIMP getRWStatus (DBROWCOUNT iRow, DB_LORDINAL iColumn, OSPRW *prwStatus);
    STDMETHODIMP getVariant (DBROWCOUNT iRow, DB_LORDINAL iColumn, OSPFORMAT format,
                             VARIANT *pVar);
    STDMETHODIMP setVariant (DBROWCOUNT iRow, DB_LORDINAL iColumn, OSPFORMAT foramt,
                             VARIANT Var);
    STDMETHODIMP getLocale(BSTR *pbstrLocale);

    STDMETHODIMP find (DBROWCOUNT iRowStart, DB_LORDINAL iColumn,
                       VARIANT val, OSPFIND findFlags,
                       OSPCOMP compType, DBROWCOUNT *piRowFound);

    // establish or detach single event sink
    STDMETHODIMP addOLEDBSimpleProviderListener(OLEDBSimpleProviderListener *pEvent);
    STDMETHODIMP removeOLEDBSimpleProviderListener(OLEDBSimpleProviderListener *pEvent);

    STDMETHODIMP getEstimatedRows(DBROWCOUNT *piRows)
    {
        return getRowCount(piRows);
    }

    STDMETHODIMP isAsync(BOOL *pbAsync) { return FALSE; }

    STDMETHODIMP stopTransfer();

    // deprecated methods:
    STDMETHODIMP DeleteColumns (LONG iColumn, LONG cColumns,
                                LONG *pcColumnsDeleted);
    STDMETHODIMP InsertColumns (LONG iColumn, LONG cColumns,
                                LONG *pcColumnsInserted);
private:
    ~CSimpleTabularData ();         // Hide from the public, use Release().

    HRESULT GetCell (ULONG iRow, ULONG iColumn, CCell *&rpCell);
    void KillRows (ULONG iRow, ULONG cRows);

    // Internal data:

    ULONG                           _cRef;
    TSTDArray<TSTDArray<CCell> *>   _ArraypArrayCells;
    ULONG                           _cRows;
    ULONG                           _cColumns;

    // This is an interface we get so we can fire notification events:
    OLEDBSimpleProviderListener *   _pEvents;

#if DBG == 1
    void IsValidObject ();          // checks object for correctness

    int                             _bInitialized;
    ULONG                           _cDebugLastRowSizeChecked;
    ULONG                           _cDebugLastColumnSizeChecked;
#endif
};


#endif  // _ARRAYI_HXX_
