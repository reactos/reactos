//
// Copyright (c) 1997-1999 Microsoft Corporation.
//

STDAPI_(LPVOID) OleStdMalloc(ULONG ulSize);
STDAPI_(void) OleStdFree(LPVOID pmem);
STDAPI_(BOOL) OleStdCopyFormatEtc(LPFORMATETC petcDest, LPFORMATETC petcSrc);
STDAPI_(DVTARGETDEVICE FAR*) OleStdCopyTargetDevice(DVTARGETDEVICE FAR* ptdSrc);
STDAPI_(LPENUMFORMATETC)
  OleStdEnumFmtEtc_Create(ULONG nCount, LPFORMATETC lpEtc);
