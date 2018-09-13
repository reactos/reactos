#ifndef __WINDBGRM_H__
#define __WINDBGRM_H__


#include "hmacro.h"


//
// Used by WINDBGRM
//

//
//  + Reg key
//  - Reg value
//  > Reg key that is mirrored
//
//
//  HKEY_CURRENT_USER\\Software\\Microsoft\\windbgrm\\WORKSPACE_REVISION_NUMBER
//    - Name of selected transport layer
//    + CAll_TLs_RM_WKSP (Transport Layer)
//      + CIndiv_TL_RM_WKSP (individual transport, repeated as necessary)
//        - Description
//        - DLL
//        - Params
//        + Kernel Debugger Options
//
//


//
// (windbgrm) Transport Layer - Kernel Debugger Settings:
//
class CKD_RM_WKSP 
: public CGen_WKSP<CDoNothingStub_WKSP, CItemInterface_WKSP> 
{
public:

#define CKD_RM_DEFINE
#include "windbgrm.hxx"
#undef CKD_RM_DEFINE

public:
    CKD_RM_WKSP();
};


//
// (windbgrm) Individual Transport Layer:
//
class CIndiv_TL_RM_WKSP 
: public CGen_WKSP<CKD_RM_WKSP, CItemInterface_WKSP> 
{
public:

#define CINDIV_TL_RM_DEFINE
#include "windbgrm.hxx"
#undef CINDIV_TL_RM_DEFINE

public:
    CIndiv_TL_RM_WKSP();
};

//
// (windbgrm) Dynamic collection of all the Transport Layers:
//
class CAll_TLs_RM_WKSP 
: public CGen_WKSP<CIndiv_TL_RM_WKSP, CItemInterface_WKSP>
{
public:

#define CRM_ALL_TLS_DEFINE
#include "windbgrm.hxx"
#undef CRM_ALL_TLS_DEFINE

public:
    CAll_TLs_RM_WKSP();
};


//
// Root for windbgrm
//
class CWindbgrm_RM_WKSP 
: public CGen_WKSP<CAll_TLs_RM_WKSP, CItemInterface_WKSP>
{
public:
#define CWINDBG_RM_DEFINE
#include "windbgrm.hxx"
#undef CWINDBG_RM_DEFINE

    CWindbgrm_RM_WKSP();

    virtual HKEY GetRegistryKey(PBOOL pbRegKeyCreated = NULL);
};




#endif

