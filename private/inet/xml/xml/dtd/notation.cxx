/*
 * @(#)Notation.cxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#include "core.hxx"
#pragma hdrstop

#include "notation.hxx"
#include "xmlnames.hxx"

// use ABSTRACT because of no default constructor 
DEFINE_ABSTRACT_CLASS_MEMBERS(Notation, _T("Notation"), Base);

/**
 * This class implements an entity object representing an XML notation.
 *
 * @version 1.0, 6/3/97
 */
