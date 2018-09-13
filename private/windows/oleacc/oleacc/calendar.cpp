// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  CALENDAR.CPP
//
//  Knows how to talk to COMCTL32's calendar and date-picker controls.  These
//  are a lot like a multicolumn listbox and combobox respectively.
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "client.h"
#include "calendar.h"

#define NOSTATUSBAR
#define NOUPDOWN
#define NOMENUHELP
#define NOTRACKBAR
#define NODRAGLIST
#define NOPROGRESS
#define NOHOTKEY
#define NOTABCONTROL
#define NOLISTVIEW
#define NOTREEVIEW
#define NOTOOLBAR
#define NOANIMATE
#define NOHEADER
#include <commctrl.h>

