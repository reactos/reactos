////////////////////////////////////////////////////////////////////////////////
//
// Internal.h
//
// MS Office Internal Interfaces.  These interfaces are not supported
// for client code.
//
// Change history:
//
// Date         Who             What
// --------------------------------------------------------------------------
// 07/13/94     B. Wentz        Created file
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __Internal_h__
#define __Internal_h__

#include "offcapi.h"
#include "proptype.h"

  // Flag to | into Id's passed to Summary and Document Summary objects
  // to get a real pointer to data, not a copy.
#define PTRWIZARD       0x1000

  // Flag to | into flags passed to User-defined property streams to get
  // a real pointer to the data, not a copy.
#define UD_PTRWIZARD    0x0002

#ifdef __cplusplus
extern TEXT("C") {
#endif

    // Create a UDPROP structure.
  LPUDPROP LpudpropCreate ( void );

    // Free a UDPROP structure.
  VOID VUdpropFree (LPUDPROP *lplpudp);

    // Create a temporary copy of the User-Defined property data
  BOOL FMakeTmpUDProps (LPUDOBJ lpUDObj);

    // Swap the "temp" copy with the real copy of User-Defined property data
  BOOL FSwapTmpUDProps (LPUDOBJ lpUDObj);

    // Delete the "temp" copy of the data
  BOOL FDeleteTmpUDProps (LPUDOBJ lpUDObj);

    // Look up a node in the UD props
  LPUDPROP PASCAL LpudpropFindMatchingName (LPUDOBJ lpUDObj, LPTSTR lpsz);

  BOOL FSumInfoCreate (LPSIOBJ FAR *lplpSIObj, const void *prglpfn[]);
  BOOL FDocSumCreate (LPDSIOBJ FAR *lplpDSIObj, const void *prglpfn[]);
  BOOL FUserDefCreate (LPUDOBJ FAR *lplpUDObj, const void *prglpfn[]);

    // Clear the data stored in the object, but do not destroy the object
  BOOL FSumInfoClear (LPSIOBJ lpSIObj);

    // Destroy the object
  BOOL FSumInfoDestroy (LPSIOBJ FAR *lplpSIObj);

    // Clear the data stored in the object, but do not destroy the object
  BOOL FDocSumClear (LPDSIOBJ lpDSIObj);

    // Destroy the given object.
  BOOL FDocSumDestroy (LPDSIOBJ FAR *lplpDSIObj);

    // Clear the data stored in object, but do not destroy the object.
  BOOL FUserDefClear (LPUDOBJ lpUDObj);

    // Destroy object,
  BOOL FUserDefDestroy (LPUDOBJ FAR *lplp);
  

     // Misc internal calls & defines

  // Maximum number of dictionary hash table buckets.
#define DICTHASHMAX     20

  void PASCAL FreeUDData (LPUDOBJ lpUDObj, BOOL fTmp);
  BOOL PASCAL FAddPropToList (LPUDOBJ lpUDObj, LPPROPVARIANT lppropvar, STATPROPSTG *lpstatpropstg, LPUDPROP lpudprop);
  void PASCAL AddNodeToList (LPUDOBJ lpUDObj, LPUDPROP lpudprop);

  BOOL PASCAL FUserDefIteratorSetLink (LPUDOBJ lpUDObj, LPUDITER lpUDIter, LPTSTR lpszLink);
  LPUDITER PASCAL LpudiUserDefCreateIterFromLpudp (LPUDOBJ lpUDObj, LPUDPROP lpudp);
  //
  // THIS SHOULD BE USED WHEN COPYING DATA TO BUFFER PROVIDED BY APP
  //
  BOOL PASCAL FCopyValueToBuf (LPVOID lpvBuf, DWORD cbMax, LPVOID lpv, UDTYPES udtype);
  //
  // FOR INTERNAL USE ONLY.  THIS FUNCTION ALLOCATES MEMORY
  //
  LPVOID PASCAL LpvCopyValue (LPVOID *lplpvBuf, DWORD cbMax, LPVOID lpv, UDTYPES udtype, BOOL fFromLpstz, BOOL fToLpstz);
  void PASCAL DeallocValue (LPVOID *lplpvBuf, UDTYPES udtype);
  // flpstz -- TRUE if lpstz is an OLE Prop lpstz DWORD-DWORD-bytes
  //        -- FALSE if lpstz is a regular lpsz
  BOOL PASCAL FConvertDate (LPTSTR lpstz, LPFILETIME lpft);

  BOOL WINAPI FInitFakeKernel32();

VOID PASCAL VFtToSz(LPFILETIME lpft, LPTSTR psz, WORD cbMax, BOOL fMinutes);


#ifdef __cplusplus
}; // extern "C"
#endif

#endif // __Internal_h__
