//****************************************************************************
//
//  Module:     RNAUI.DLL
//  File:       contain.h
//  Content:    This file contains the persistent-object-binding mechanism
//              which is slightly different from OLE's binding.
//  History:
//  05-05-93 ViroonT     Modified from winutils
//
//  Copyright (c) Microsoft Corporation 1991-1994
//
//****************************************************************************

#ifndef _CONTAIN_H_
#define _CONTAIN_H_

#define FCE_INIT        0x0000
#define FCE_NEXTOBJECT  0x0001
#define FCE_TERM        0x0002
#define FCE_ACTION      (FCE_INIT|FCE_NEXTOBJECT|FCE_TERM)
#define FCE_SELECTED    0x0004


#endif // _CONTAIN_H_
