////////////////////////////////////////////////////////////////////////////////
//
// propmisc.h
//
// Common Property Set routines.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __propmisc_h__
#define __propmisc_h__

#include "proptype.h"

    // Update a string value (*lplpstzOrig) with new
    // value lpszNew.
      // Create a list.
  LPLLIST LpllCreate (LPLLIST *lplpll,
                             LPLLCACHE lpllc,
                             DWORD dwc,
                             DWORD cbNode);

      // Get a node from the list
  LPLLIST LpllGetNode (LPLLIST lpll,
                              LPLLCACHE lpllc,
                              DWORD idw);

      // Delete a node from the list
  LPLLIST LpllDeleteNode (LPLLIST lpll,
                                 LPLLCACHE lpllc,
                                 DWORD idw,
                                 DWORD cbNode,
                                 void (*lpfnFreeNode)(LPLLIST)
                                 );

      // Insert a node into the list
  LPLLIST LpllInsertNode (LPLLIST lpll,
                                 LPLLCACHE lpllc,
                                 DWORD idw,
                                 DWORD cbNode);

      // Set the bit indicating that a suminfo filetime/int has been set/loaded
  VOID VSumInfoSetPropBit(LONG pid, BYTE *pbftset);

      // Check the bit indicating that a suminfo filetime/int has been set/loaded
  BOOL FSumInfoPropBitIsSet(LONG pid, BYTE bftset);

      // Set the bit indicating that a docsuminfo filetime/int has been set/loaded
  VOID VDocSumInfoSetPropBit(LONG pid, BYTE *pbftset);

      // Check the bit indicating that a docsuminfo filetime/int has been set/loaded
  BOOL FDocSumInfoPropBitIsSet(LONG pid, BYTE bftset);

      // Free the DocSum headpart plex
  VOID FreeHeadPartPlex(LPDSIOBJ lpDSIObj);

      // Return the size of the FMTID in the thumbnail
  DWORD CbThumbNailFMTID(DWORD cftag);

#endif // __propmisc_h__
