//+---------------------------------------------------------------------
//
//  File:       fatstg.hxx
//
//  Contents:   IStream on top of a DOS (non-docfile) file
//
//	History:	
//
//----------------------------------------------------------------------

#ifndef I_FATSTG_HXX_
#define I_FATSTG_HXX_
#pragma INCMSG("--- Beg 'fatstg.hxx'")

HRESULT
CreateStreamOnFile(LPCTSTR pchFile, DWORD dwSTGM, LPSTREAM * ppstrm);

HRESULT
CloseStreamOnFile(LPSTREAM pStm);

#pragma INCMSG("--- End 'fatstg.hxx'")
#else
#pragma INCMSG("*** Dup 'fatstg.hxx'")
#endif
