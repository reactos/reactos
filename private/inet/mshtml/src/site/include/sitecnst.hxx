//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       sitecnst.hxx
//
//  Contents:   Global constants for the site project.
//
//----------------------------------------------------------------------------

#ifndef I_SITECNST_HXX_
#define I_SITECNST_HXX_
#pragma INCMSG("--- Beg 'sitecnst.hxx'")

#define MISC_STATUS_FORM \
    (OLEMISC_INSIDEOUT | \
     OLEMISC_ACTIVATEWHENVISIBLE | \
     OLEMISC_RECOMPOSEONRESIZE | \
     OLEMISC_SUPPORTSMULTILEVELUNDO | \
     OLEMISC_CANTLINKINSIDE | \
     OLEMISC_SETCLIENTSITEFIRST)

#pragma INCMSG("--- End 'sitecnst.hxx'")
#else
#pragma INCMSG("*** Dup 'sitecnst.hxx'")
#endif
