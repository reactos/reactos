//
// Private disasm options
//
enum {
    // Never open the disassembly window automatically, it must be
    // opened manually.
    dopNeverOpenAutomatically = dopReserved
    // dopNext = dopReserved << 1
};



#define szRM_DEFAULT_TL_NAME    "Pipes"
#define szWDBG_DEFAULT_TL_NAME  "Local"



#include "windbg.h"
#include "windbgrm.h"


//
// Helper function for the TList.Find function
//
template<class T>
BOOL
WKSP_Generic_CmpRegName(
    const void * const pv, 
    T pGenIntDescendent
    )
{
    Assert(pv);
    Assert(pGenIntDescendent);
    AssertChildOf(pGenIntDescendent, CGenInterface_WKSP);

    return 0 == strcmp( (PSTR) pv, pGenIntDescendent->m_pszRegistryName);
}


//
// WinDbgRM
//
extern CWindbgrm_RM_WKSP            g_RM_Windbgrm_WkSp;
extern CAll_TLs_RM_WKSP             &g_RM_All_Tls_WkSp;



//
// WinDbg
//
// Defined in windbg
