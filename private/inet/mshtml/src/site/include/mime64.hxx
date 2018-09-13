// Are there such things as UNICODE MIME64 files?  That would be very inefficient

#ifndef I_MIME64_HXX_
#define I_MIME64_HXX_
#pragma INCMSG("--- Beg 'mime64.hxx'")

#define LPMSTR LPTSTR
#define LPCMSTR LPCTSTR
#define MCHAR TCHAR

HRESULT Mime64Decode(LPCMSTR pStrData, LPSTREAM *ppstm);
HRESULT Mime64Encode(LPBYTE pData, UINT cbData, LPTSTR pchData);

#pragma INCMSG("--- End 'mime64.hxx'")
#else
#pragma INCMSG("*** Dup 'mime64.hxx'")
#endif
