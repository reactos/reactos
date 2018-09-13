//+------------------------------------------------------------------------
//
//  File:       SELECT.HXX
//
//  Contents:   The selection code classes and structures
//
//  Classes:    CDDocSelection
//              enumQualifiers
//              eReservedBits
//              QUALIFIER
//              SelectUnit
//              PLY
//              CArySelector
//              CAryPly
//              HQualiElement
//              CAryHQualifier
//
//  History:
//      02/01/95    LaszloG Create
//      02/25/95    JerryD  Adapt for intgration of DIRT OLE DB provider
//      03/08/95    LaszloG : code cleanup, updated comments, bug fixes
//
//  @comm
//  Copyright: (c) 1995, Microsoft Corporation
//  All rights Reserved.
//  Information contained herein is Proprietary and Confidential.
//
//-------------------------------------------------------------------------

#ifndef _SELECT_HXX_
#define _SELECT_HXX_

#ifndef _BITARY_HXX_
#   include "bitary.hxx"
#endif

class CSite;
class CBaseFrame;
class CDataDoc;

inline BOOL IsRootFrame(CSite * ps)
{
    return ps->TestClassFlag(SITEDESC_TEMPLATE) && ps->TestClassFlag(SITEDESC_DATAFRAME);
}


//+------------------------------------------------------------------------
//
//  This is the structure holding the qualifier. The qualifier can be mostly anything
//  that can be used to identify a row/database record:
//      bookmark
//      Row index
//      unique key
//      HROW (?)
//
//  Depending on cursor capabilities and speed considerations, we may decide to
//  store multiple qualifier values for a row, e.g. bookmark AND row index.
//  In that case we'll need to move the relevant structures out of the union.
//
//  depending on what the OLE-DB cursor can supply.
//
//-------------------------------------------------------------------------


#define DONT_CLEAR_SELECTION FALSE

enum SELECTION_RESERVED_BITS
{
    SRB_LAYOUTFRAME_SELECTED,
    SRB_NUM_RESERVED_BITS   //  this value is used to offset the bitarray index.
};

enum QUALIFIER_TYPE
{
    QUALI_UNKNOWN,      //  initial value
    QUALI_BOOKMARK,     //  bookmark (with optional chapter info)
    QUALI_HROW,         //  HROW
    QUALI_INDEX,        //  row index
    QUALI_RECID,        //  Record ID
    QUALI_PRIMARY_KEY,  //  primary/unique key
    QUALI_SYNTHETIC,    //  qualifier is synthesized by DataDoc
    QUALI_HEADER,       //  qualifier for the header/footer object
    QUALI_FOOTER,
    QUALI_DETAIL,       //  qualifier for template
    QUALI_UNBOUND_LAYOUT,
    //  spec qualifiers
    QUALI_ALL_RECORDS,
};



struct QUALIFIER
{
#if DBG == 1
    ~QUALIFIER();
#endif

    BOOL operator==(const QUALIFIER &q) const;

    BOOL operator!=(const QUALIFIER &q) const
    {
        return !operator==(q);
    }

    BOOL operator<(const QUALIFIER &q) const;

    void Clear();

    BOOL IsValid()
    {
        return type != QUALI_UNKNOWN;
    }

    QUALIFIER_TYPE  type;
    struct {
        CDataLayerBookmark bookmark;
        DWORD              dwHash;
    };
    union
    {
        unsigned long row;
        unsigned long id;
        struct
        {
            unsigned long      cbKey;
            void *             pvKey;
        };
        struct
        {
            unsigned long      index;
            DWORD              dwCRC;
        };
    };
};



//+------------------------------------------------------------------------
//
//  These structures describe the (possibly) multiple children
//  in the selection tree. They use the child repeater's index in its
//  parent site's _arySites array to select the correct branch of the tree.
//
//-------------------------------------------------------------------------

//  forward declare class
class CArySelector;

struct PLY
{
    UINT idx;
    CArySelector * parySelectors;
};

DECLARE_FORMSDATAARY(CAryPlyBase, PLY, PLY *);

class CAryPly : public CAryPlyBase
{
public:
    ~CAryPly();

    void FreePlies(CSite * pSite);
    PLY * Find(UINT idx);
};


//+------------------------------------------------------------------------
//
//  The structure describing one row-vise contiguous selection unit.
//  It might default to a single line if the data source doesn't provide
//  ordered bookmarks.
//
//  When we need bit flags, we hide them in the bitarray.
//  The number of hidden flags in the bitarray is governed by the enum eReservedBits
//  above.
//
//-------------------------------------------------------------------------
struct SelectUnit
{
    void *              operator new(size_t s) { return MemAllocClear(s); }
#if DBG==1
    ~SelectUnit();
#endif

    void        Clear(void);
    HRESULT     Merge(SelectUnit *psuAnchor);
    HRESULT     Copy(SelectUnit& suOther);
    void        Normalize(void);
    BOOL        IsValid(void)
    {
        return (qStart.IsValid() &&
                ( !pqEnd || pqEnd->IsValid()));
    }

    QUALIFIER       qStart;         //  This is always present.
    QUALIFIER *     pqEnd;          //  If NULL, this is a single row
    CAryPly         arySubLevels;   //  Next level selection in the hierarchy
    CDDSelBitAry    aryfControls;
};

DECLARE_FORMSPTRARY(CArySelectUnit, SelectUnit*, SelectUnit**);

class CArySelector : public CArySelectUnit
{
public:
    ~CArySelector();

    //  extra methods to handle searches and insertions for
    //  incomplete structures (e.g. starting qualifier only)
    HRESULT      Append(QUALIFIER qualifier);
    HRESULT      Append(SelectUnit *pSU) { return CArySelectUnit::Append(pSU); }
    HRESULT      Merge(SelectUnit *psuAnchor, SelectUnit *psuNew);
    HRESULT      Split(SelectUnit *psuSplitter, SelectUnit *psuOld);
    SelectUnit * Find(QUALIFIER qualifier);
    SelectUnit * Find(SelectUnit * pSelectUnit);

    void         FreeQualifierList(CSite * pSite);
};




#if NEVER
//+------------------------------------------------------------------------
//
//  This will be the HBookmark if we ever need one
//
//-------------------------------------------------------------------------
struct HQualiElement
{
    UINT        uPly;           //  Selects the cursor ply if multiple child cursors
    QUALIFIER   qualifier;      //  qualifier in the cursor
};

DECLARE_FORMSDATAARY(CAryHQualifier, HQualiElement, HQualiElement*);

#endif


#endif _SELECT_HXX_

//
//  ****    End of file
//
///////////////////////////////////////////////////////////////////////////////
