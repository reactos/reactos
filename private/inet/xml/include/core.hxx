/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
// error C4102: '<labelname>' : unreferenced label
#pragma warning(disable: 4102)
#include "core/base/core.hxx"

#include <urlmon.h>
#include <shlwapi.h>

#include <windows.h>
#include <shlwapi.h>
#include <unknwn.h>
#include <objbase.h>
#include <urlmon.h>

// iedev/inc msxml stuff.
#define ORD_MSXML 1
#include <msxml.h>
#include <xmlparser.h>

// XML interfaces for XQL & XTL to use
#include "xml/om/element.hxx"
#include "xml/om/node.hxx"
#include "xml/om/document.hxx"

// error message numbers.

#include "xml/resource/messages.h"

#include "xml/tokenizer/base/common.hxx"

// IIS4 interfaces
#include "asptlb.h"
