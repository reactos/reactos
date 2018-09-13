//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispdefs.hxx
//
//  Contents:   Common definitions for display project.
//
//----------------------------------------------------------------------------

#ifndef I_DISPDEFS_HXX_
#define I_DISPDEFS_HXX_
#pragma INCMSG("--- Beg 'dispdefs.hxx'")

// coordinate systems in which values can be specified
typedef enum tagCOORDINATE_SYSTEM
{
    COORDSYS_GLOBAL             = 0,
    COORDSYS_PARENT             = 1,
    COORDSYS_CONTAINER          = 2,
    COORDSYS_CONTENT            = 3,

    // NOTE: internal use only!
    COORDSYS_NONFLOWCONTENT     = 4
} COORDINATE_SYSTEM;

// client rect types
typedef enum tagCLIENTRECT
{
    CLIENTRECT_BACKGROUND       = 0,
    CLIENTRECT_CONTENT          = 1,
    CLIENTRECT_VISIBLECONTENT   = 2,
    CLIENTRECT_VSCROLLBAR       = 3,
    CLIENTRECT_HSCROLLBAR       = 4,
    CLIENTRECT_SCROLLBARFILLER  = 5,
    CLIENTRECT_USERECT          = 6,
} CLIENTRECT;

// layer types
typedef enum tagDISPNODELAYER
{
    // actual node layers
    DISPNODELAYER_NEGATIVEZ     = 0x01, // 4th layer to draw
    DISPNODELAYER_FLOW          = 0x02, // 5th layer to draw
    DISPNODELAYER_POSITIVEZ     = 0x04, // 6th layer to draw

    // synthesized layers
    DISPNODELAYER_FIRST         = 0x08, // 1st layer to draw
    DISPNODELAYER_BORDER        = 0x10, // 2nd layer to draw
    DISPNODELAYER_BACKGROUND    = 0x20, // 3rd layer to draw
    DISPNODELAYER_SCROLLBARS    = 0x40, // 7th layer to draw
    DISPNODELAYER_LAST          = 0x80, // 8th layer to draw
    
} DISPNODELAYER;

// NOTE! These should match:

// BEHAVIORRENDERINFO_BEFOREBACKGROUND,
// BEHAVIORRENDERINFO_AFTERBACKGROUND,
// BEHAVIORRENDERINFO_BEFORECONTENT,
// BEHAVIORRENDERINFO_AFTERCONTENT,
// BEHAVIORRENDERINFO_AFTERFOREGROUND,

// BEHAVIORRENDERINFO_DISABLEBACKGROUND,
// BEHAVIORRENDERINFO_DISABLENEGATIVEZ,
// BEHAVIORRENDERINFO_DISABLECONTENT,
// BEHAVIORRENDERINFO_DISABLEPOSITIVEZ
//
// (we don't want to have #include dependency on mshtml.idl here so we just redefine the constants)
// A set of asserts in DebugDocStartupCheck() should assert the match here

typedef enum
{
     CLIENTLAYERS_BEFOREBACKGROUND    = 0x001,
     CLIENTLAYERS_AFTERBACKGROUND     = 0x002,
     CLIENTLAYERS_BEFORECONTENT       = 0x004,
     CLIENTLAYERS_AFTERCONTENT        = 0x008,
     CLIENTLAYERS_AFTERFOREGROUND     = 0x020,

     CLIENTLAYERS_ALLLAYERS           = 0x0FF,

     CLIENTLAYERS_DISABLEBACKGROUND   = 0x100,
     CLIENTLAYERS_DISABLENEGATIVEZ    = 0x200,
     CLIENTLAYERS_DISABLECONTENT      = 0x400,
     CLIENTLAYERS_DISABLEPOSITIVEZ    = 0x800,

     CLIENTLAYERS_DISABLEALLLAYERS    = 0xF00

} CLIENTLAYERS;

// visibility modes
// (no longer used by display tree, just clients)
typedef enum tagVISIBILITYMODE
{
    VISIBILITYMODE_INVISIBLE    = 0,
    VISIBILITYMODE_VISIBLE      = 1,
    VISIBILITYMODE_INHERIT      = 2
    
} VISIBILITYMODE;

// border types
typedef enum tagDISPNODEBORDER
{
    DISPNODEBORDER_NONE         = 0,
    DISPNODEBORDER_SIMPLE       = 1,
    DISPNODEBORDER_COMPLEX      = 2
} DISPNODEBORDER;

// scrollbar hints
typedef enum tagDISPSCROLLBARHINT
{
    DISPSCROLLBARHINT_NOBUTTONDRAW  = 1
} DISPSCROLLBARHINT;

// optimization hints for display items
typedef enum tagDISPHINT
{
    DISPHINT_OPAQUE             = 0x80000000,
    DISPHINT_OVERHEADSCALES     = 0x40000000
} DISPHINT;

// scroll directions
enum SCROLL_DIRECTION
{
    SCROLL_UP     = 0x0001,
    SCROLL_DOWN   = 0x0002,
    SCROLL_LEFT   = 0x0004,
    SCROLL_RIGHT  = 0x0008
};


#pragma INCMSG("--- End 'dispdefs.hxx'")
#else
#pragma INCMSG("*** Dup 'dispdefs.hxx'")
#endif

