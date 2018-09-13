//****************************************************************************
//
//  Module:     RNAUI.DLL
//  File:       rnaguidp.h
//  Content:    This file contains the RNA class GUID for the shell registry.
//
//  History:
//      12/ 9/ 93   ScottH
//
//  Copyright (c) Microsoft Corporation 1991-1994
//
//****************************************************************************

//  CLSIDs of the RNA object class. It doesn't have to be in a public header
// unless we decide to let ISVs to create RNA objects directly by calling
// OleCreateInstance with one of class IDs.
//

DEFINE_GUID(CLSID_Remote, 0x992CFFA0L, 0xF557, 0x101A, 0x88, 0xEC, 0x00, 0xDD, 0x01, 0x0C, 0xCC, 0x48);

