#include "precomp.hxx"
#pragma hdrstop


#include "cppmacro.h"




//
// (windbgrm) Transport Layer:
//
CAll_TLs_RM_WKSP::
CAll_TLs_RM_WKSP()
{
//#define CRM_ALL_TLS_DEFINE
//#include "windbgrm.hxx"
//#undef CRM_ALL_TLS_DEFINE

    const int nRows = 2;
    const int nCols = 4;

    static LPSTR rgsz[nRows][nCols] = {
        { szRM_DEFAULT_TL_NAME, "Named pipe transport Layer - PIPE=windbg",   "tlpipe.dll",   "windbg" },
        { "Pipes Kd",        "Named pipe transport Layer - PIPE=windbg",   "tlpipe.dll",   "windbg" }
    };

    // Dynamically create the default values


    // First create the specific values used by windbg
    for (int nR = 0; nR<nRows; nR++) {
        CIndiv_TL_RM_WKSP * pCont = new CIndiv_TL_RM_WKSP();
        Assert(pCont);

        pCont->Init(this, rgsz[nR][0], FALSE, FALSE);

        pCont->m_pszDescription = _strdup(rgsz[nR][1]);
        pCont->m_pszDll = _strdup(rgsz[nR][2]);
        pCont->m_pszParams = _strdup(rgsz[nR][3]);

        if (1 == nR) {
            // Enable the kernel debugger pipe.
            pCont->m_contKdSettings.m_bEnable = TRUE;
        }
    }

    // Common values used by windbg & windbgrm
    for (nR = 0; nR<ROWS_SERIAL_TRANSPORT_LAYERS; nR++) {
        CIndiv_TL_RM_WKSP * pCont = new CIndiv_TL_RM_WKSP();
        Assert(pCont);

        pCont->Init(this, rgszSerialTransportLayers[nR][0], FALSE, FALSE);

        pCont->m_pszDescription = _strdup(rgszSerialTransportLayers[nR][1]);
        pCont->m_pszDll = _strdup(rgszSerialTransportLayers[nR][2]);
        pCont->m_pszParams = _strdup(rgszSerialTransportLayers[nR][3]);
    }
}



//
// windbgRM
//

//
// (windbgrm) Transport Layer - Kernel Debugger Settings:
//
CKD_RM_WKSP::
CKD_RM_WKSP()
{
#define CKD_RM_DEFINE
#include "windbgrm.hxx"
#undef CKD_RM_DEFINE
}


//
// (windbgrm) Transport Layer:
//
CIndiv_TL_RM_WKSP::
CIndiv_TL_RM_WKSP()
{
#define CINDIV_TL_RM_DEFINE
#include "windbgrm.hxx"
#undef CINDIV_TL_RM_DEFINE
}


// place here


//
//
// Root for windbgrm
//
CWindbgrm_RM_WKSP::
CWindbgrm_RM_WKSP()
{
    SetRegistryName("Software\\Microsoft\\windbgrm\\" WORKSPACE_REVISION_NUMBER );

#define CWINDBG_RM_DEFINE
#include "windbgrm.hxx"
#undef CWINDBG_RM_DEFINE
}

HKEY
CWindbgrm_RM_WKSP::
GetRegistryKey(
    PBOOL pbRegKeyCreated
    )
{
    if (m_hkeyRegistry) {
        return m_hkeyRegistry;
    }

    return m_hkeyRegistry = WKSP_RegKeyOpenCreate(HKEY_CURRENT_USER, 
                                                  m_pszRegistryName,
                                                  pbRegKeyCreated
                                                  );
}



