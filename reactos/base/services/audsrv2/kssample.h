//**@@@*@@@****************************************************
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

//
// FileName:    kssample.h
//
// Abstract:    
//      All of the necessary includes in one place
//

#pragma once
#ifndef __COMMONKS_H
#define __COMMONKS_H

#define _WIN32_WINNT 0x0501
#define WINVER 0x0501

#include <windows.h>
#include <ks.h>
#include <ksmedia.h>
#include <tchar.h>
#include <assert.h>
#include <stdio.h>

#define SAFE_DELETE(x) delete x; x = NULL;

// filter types
typedef enum {eUnknown, eAudRen, eAudCap} ETechnology;

typedef struct
{
    KSP_PIN KsPProp;
    KSMULTIPLE_ITEM KsMultiple;
} INTERSECTION;

#include "tlist.h"
#include "util.h"
#include "irptgt.h"
#include "filter.h"
#include "pin.h"
#include "node.h"
#include "enum.h"
#include "audfilter.h"
#include "audpin.h"


#endif //__COMMONKS_H
