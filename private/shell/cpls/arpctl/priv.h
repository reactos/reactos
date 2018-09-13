//  PRIV.H
//
//      Precompiled header file
//  
//  Created 20-Apr-98 [JonT]

#ifndef _PRIV_H_
#define _PRIV_H_

// ATL and control stuff
#include "stdafx.h"
#include "resource.h"

// Windows stuff
#include <windowsx.h>
#include <winerror.h>

// Shell stuff
#include <shlobj.h>
#include <shlwapi.h>
#include <ccstock.h>
#include <crtfree.h>
#define DISALLOW_Assert
#include <debug.h>
#include <regstr.h>

// App management stuff
#include <shappmgr.h>           // Header from idl file

// Private shell stuff
#include "shellp.h"

// Stuff for this control
#include "resource.h"
#include "ARPCtl.h"
#include "events.h"
#include "cctl.h"

// Trace flags
#define TF_CTL              0x00000010      // General control traces

// Break flags
#define BF_ONDLLLOAD        0x00000010

// Prototype flags

#endif // #ifndef _PRIV_H_
