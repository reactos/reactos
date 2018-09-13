/*
 * @(#)osp.hxx 1.0 8/20/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
#ifndef _OSP_HXX
#define _OSP_HXX

class NOVTABLE IOLEDBSimpleProvider : public Object
{
public:
    virtual long    getRowCount() = 0;
    virtual long    getColumnCount() = 0;
    virtual OSPRW   getRWStatus(long iRow, long iColumn) = 0;
    virtual void    getVariant(long iRow, long iColumn, OSPFORMAT format,
                                VARIANT *pVar) = 0;
    virtual void    setVariant(long iRow, long iColumn, OSPFORMAT format,
                                VARIANT var) = 0;
// NOTIMPL virtual BSTR    getLocale() = 0;
    virtual long    deleteRows(long iRow, long cRows) = 0;
    virtual long    insertRows(long iRow, long cRows) = 0;
// NOTIMPL virtual long    find(long iRowStart, long iColumn, VARIANT val,
// NOTIMPL                             OSPFIND findFlags, OSPCOMP compType) = 0;
    virtual void    addOLEDBSimpleProviderListener(
                            OLEDBSimpleProviderListener *pospIListener) = 0;
    virtual void    removeOLEDBSimpleProviderListener(
                            OLEDBSimpleProviderListener *pospIListener) = 0;
    virtual BOOL    isAsync() = 0;
    virtual long    getEstimatedRows() = 0;
    virtual void    stopTransfer() = 0;
};

#endif _OSP_HXX

