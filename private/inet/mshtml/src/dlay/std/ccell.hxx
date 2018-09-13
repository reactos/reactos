//+-----------------------------------------------------------------------
//
//  Simple Tabular Data
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:      ccell.hxx
//  Author:    Ido Ben-Shachar (t-idoben@microsoft.com)
//
//  Contents:  Definition of the CCell class, which contains the elements
//             of the STD table.
//
//  07/18/95:   TerryLu     Added sentries
//
//------------------------------------------------------------------------

#ifndef _CCELL_HXX_
#define _CCELL_HXX_


// Forward declarations:
class CCell;
template <class TYPE> class TSTDArray;



//+-----------------------------------------------------------------------
//
//  Class:     CCell
//
//  Synopsis:  Contains a VARIANT to hold data and the read/write status.
//             All cells are initialized to allow both read and write.
//             A determination of whether the cell is empty or not can
//             be made by checking whether the variant is empty or not.
//             Also contains a CString for faster access to strings.
//
//             NOTE: Don't add any data members to this cell which would
//               tie it down to a particular memory location, such as
//               pointers to itself.  See member funcion InsertColumns
//               for further details.
//
//  Methods:   GetCellVariant   return cell's variant
//             GetCellString    return cell's cstring
//             GetCellStatus    return cell's read/write status
//             IsVariant()      returns true if cell holds variant
//             IsCString()      returns true if cell holds cstring
//             SetIsVariant()   cell holds variant
//             SetIsCString()   cell holds cstring
//
//------------------------------------------------------------------------

class CCell
{
public:
    CCell();
    ~CCell();

    VARIANT &GetCellVariant() { return _vCellVariant; }
    CStr &GetCellString() { return _cstrCellString; }

    int IsVariant() { return (_CellType == CT_VARIANT); }
    int IsCString() { return (_CellType == CT_CSTRING); }
    void SetIsVariant() { _CellType = CT_VARIANT; }
    void SetIsCString() { _CellType = CT_CSTRING; }
    
private:
    // This tells us what type of data is currently in the cell:
    enum CellType
    {
        CT_VARIANT,                        // Variant
        CT_CSTRING                         // Cstring
    };


// Internal data:


    // NOTE: Cell's variant and cstring cannot be placed into a union because
    //   when a cstring is converted to the variant's bstr, it would write
    //   over itself.
    VARIANT _vCellVariant;
    CStr _cstrCellString;
    CellType _CellType;
};


#endif  // _CCELL_HXX_
