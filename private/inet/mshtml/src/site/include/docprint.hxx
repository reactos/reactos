//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       docprint.hxx
//
//  Contents:   Document Printing
//
//----------------------------------------------------------------------------

#ifndef I_DOCPRINT_HXX
#define I_DOCPRINT_HXX
#pragma INCMSG("--- Beg 'docprint.hxx'")

class CTableSection;

struct CPrintPage
{
    LONG    cpFirst;
    LONG    iliFirst;
    LONG    iliLast;
    LONG    yPageHeight;
    LONG    xPageWidth;

    // Repeated table header/footer support.
    unsigned int    fReprintTableHeader:1;  // Are we reprinting a table header (if attrib set on table and enough y-space)
    unsigned int    fReprintTableFooter:1;  //                           footer
    CTableSection * pTableHeader;
    CTableSection * pTableFooter;
    RECT            rcTableHeader;
    RECT            rcTableFooter;

    CPrintPage()
    {
        pTableHeader = NULL;
        pTableFooter = NULL;
        fReprintTableHeader = FALSE;
        fReprintTableFooter = FALSE;
    }
};

#pragma INCMSG("--- End 'docprint.hxx'")
#else
#pragma INCMSG("*** Dup 'docprint.hxx'")
#endif
